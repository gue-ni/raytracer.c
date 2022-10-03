/*==================[inclusions]============================================*/

#include <omp.h>

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "lib/tinyobj_loader.h"

#include "raytracer.h"

/*==================[macros]================================================*/
/*==================[type definitions]======================================*/
/*==================[external function declarations]========================*/
/*==================[internal function declarations]========================*/

static vec3 add(const vec3, const vec3);
static vec3 sub(const vec3, const vec3);
static vec3 mult(const vec3, const vec3);
static vec3 cross(const vec3, const vec3);
static vec3 clamp(const vec3);
static vec3 normalize(const vec3);
static double length(const vec3);
static double length2(const vec3);
static double dot(const vec3, const vec3);

static vec3 div_s(const vec3, const double);
static vec3 add_s(const vec3, const double);
static vec3 mult_s(const vec3, const double);
static vec2 mult_s2(const vec2, const double);
static vec2 add_s2(const vec2, const vec2);

static vec3 random_in_unit_sphere();
static vec3 random_in_hemisphere(vec3 normal);

static vec3 phong(vec3 color, vec3 light_dir, vec3 normal, vec3 camera_origin, vec3 position, bool in_shadow, double ka, double ks, double kd, double alpha);
static vec3 calculate_color(const ray_t *ray, object_t *scene, size_t n_objects, int depth, hit_t hit);
static ray_t get_camera_ray(const camera_t *camera, double u, double v);

static vec3 trace_path_v1(ray_t *ray, object_t *scene, size_t nobj, int depth);
static vec3 trace_path_v2(ray_t *ray, object_t *scene, size_t nobj, int depth);
static vec3 trace_path_v3(ray_t *ray, object_t *scene, size_t nobj, int depth);

static vec3 reflect(const vec3 In, const vec3 N);
static vec3 refract(const vec3 In, const vec3 N, double iot);

static vec3 sample_texture(vec3 color, double u, double v);

static bool intersect(const ray_t *ray, object_t *objects, size_t n, hit_t *hit);

/*==================[external constants]====================================*/
/*==================[internal constants]====================================*/
/*==================[external data]=========================================*/

int ray_count = 0;
int intersection_test_count = 0;

/*==================[internal data]=========================================*/
/*==================[external function definitions]=========================*/

void init_camera(camera_t *camera, vec3 position, vec3 target, options_t *options)
{
  double theta = 60.0 * (PI / 180);
  double h = tan(theta / 2);
  double viewport_height = 2.0 * h;
  double aspect_ratio = (double)options->width / (double)options->height;
  double viewport_width = aspect_ratio * viewport_height;

  vec3 forward = normalize(sub(target, position));
  vec3 right = normalize(cross((vec3){0, 1, 0}, forward));
  vec3 up = normalize(cross(forward, right));

  camera->position = position;
  camera->vertical = mult_s(up, viewport_height);
  camera->horizontal = mult_s(right, viewport_width);

  vec3 half_vertical = div_s(camera->vertical, 2);
  vec3 half_horizontal = div_s(camera->horizontal, 2);

  vec3 llc_old = sub(
      sub(camera->position, half_horizontal),
      sub(half_vertical, (vec3){0, 0, 1}));

  vec3 llc_new = sub(
      sub(camera->position, half_horizontal),
      sub(half_vertical, mult_s(forward, -1)));

  camera->lower_left_corner = llc_new;
}

bool intersect_sphere(const ray_t *ray, sphere_t *sphere, hit_t *hit)
{
  intersection_test_count++;

  double t0, t1; // solutions for t if the ray intersects
  vec3 L = sub(sphere->center, ray->origin);
  double tca = dot(L, ray->direction);
  if (tca < 0)
    return false;
  double d2 = dot(L, L) - tca * tca;
  double radius2 = sphere->radius * sphere->radius;
  if (d2 > radius2)
    return false;

  double thc = sqrt(radius2 - d2);
  t0 = tca - thc;
  t1 = tca + thc;

  if (t0 > t1)
  {
    double tmp = t0;
    t0 = t1;
    t1 = tmp;
  }

  if (t0 < 0)
  {
    t0 = t1; // if t0 is negative, let's use t1 instead
    if (t0 < 0)
      return false; // both t0 and t1 are negative
  }

  if (t0 > EPSILON)
  {
    hit->t = t0;
    hit->u = 0;
    hit->v = 0;
    return true;
  }
  else
  {
    return false;
  }
}

bool intersect_triangle(const ray_t *ray, vertex_t vertex0, vertex_t vertex1, vertex_t vertex2, hit_t *hit)
{
  intersection_test_count++;

  // https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
  vec3 v0, v1, v2;
  v0 = vertex0.pos;
  v1 = vertex1.pos;
  v2 = vertex2.pos;

  vec3 edge1, edge2, h, s, q;
  double a, f, u, v, t;
  edge1 = sub(v1, v0);
  edge2 = sub(v2, v0);
  h = cross(ray->direction, edge2);
  a = dot(edge1, h);
  if (a > -EPSILON && a < EPSILON)
    return false; // This ray is parallel to this triangle.
  f = 1.0 / a;
  s = sub(ray->origin, v0);
  u = f * dot(s, h);
  if (u < 0.0 || u > 1.0)
    return false;
  q = cross(s, edge1);
  v = f * dot(ray->direction, q);
  if (v < 0.0 || u + v > 1.0)
    return false;
  // At this stage we can compute t to find out where the intersection point is on the line.
  t = f * dot(edge2, q);

  if (t > EPSILON)
  {
    hit->t = t;

    vec2 st0 = vertex0.tex;
    vec2 st1 = vertex1.tex;
    vec2 st2 = vertex2.tex;

    vec2 tex = add_s2(add_s2(mult_s2(st0, 1 - u - v), mult_s2(st1, u)), mult_s2(st2, v));
    hit->u = tex.x;
    hit->v = tex.y;
    return true;
  }
  else
  {
    return false;
  }
}

void render(uint8_t *framebuffer, object_t *objects, size_t n_objects, camera_t *camera, options_t options)
{
  double u, v, gamma = 1.0;
  ray_t ray;

  for (int y = 0; y < options.height; y++)
  {
    for (int x = 0; x < options.width; x++)
    {
      vec3 pixel = {0, 0, 0};
      for (int s = 0; s < options.samples; s++)
      {
        u = (double)(x + random_double()) / ((double)options.width - 1.0);
        v = (double)(y + random_double()) / ((double)options.height - 1.0);

        ray = get_camera_ray(camera, u, v);
        pixel = add(pixel, clamp(trace_path_v2(&ray, objects, n_objects, 0)));
      }

      pixel = div_s(pixel, options.samples);

      size_t i = (y * options.width + x) * 3;
      framebuffer[i + 0] = (uint8_t)(255.0 * pow(pixel.x, 1 / gamma));
      framebuffer[i + 1] = (uint8_t)(255.0 * pow(pixel.y, 1 / gamma));
      framebuffer[i + 2] = (uint8_t)(255.0 * pow(pixel.z, 1 / gamma));
    }
  }
}

/*==================[internal function definitions]=========================*/

double random_double() 
{ return (double)rand() / ((double)RAND_MAX + 1); }

double random_range(double min, double max)
{ return (double)rand() * (max - min) + min; }

double dot(const vec3 v1, const vec3 v2) 
{ return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }

double length2(const vec3 v1) 
{ return v1.x * v1.x + v1.y * v1.y + v1.z * v1.z; }

double length(const vec3 v1) 
{ return sqrt(length2(v1)); }

vec3 cross(const vec3 a, const vec3 b)
{ return (vec3){
  a.y * b.z - a.z * b.y, 
  a.z * b.x - a.x * b.z, 
  a.x * b.y - a.y * b.x}; 
}

vec3 mult(const vec3 v1, const vec3 v2)
{ return (vec3){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z}; }

vec3 sub(const vec3 v1, const vec3 v2) 
{ return (vec3){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z}; }

vec3 add(const vec3 v1, const vec3 v2) 
{ return (vec3){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}; }

vec3 mult_s(const vec3 v1, const double s)
{ return (vec3){v1.x * s, v1.y * s, v1.z * s}; }

vec3 div_s(const vec3 v1, const double s) 
{ return (vec3){v1.x / s, v1.y / s, v1.z / s}; }

vec2 mult_s2(const vec2 v, const double s) 
{ return (vec2){v.x * s, v.y * s}; }

vec2 add_s2(const vec2 v1, const vec2 v2) 
{ return (vec2){v1.x + v2.x, v1.y + v2.y}; }

vec3 point_at(const ray_t *ray, double t)
{ return add(ray->origin, mult_s(ray->direction, t)); }

vec3 clamp(const vec3 v1) 
{ return (vec3){CLAMP(v1.x), CLAMP(v1.y), CLAMP(v1.z)}; }

vec3 normalize(const vec3 v1)
{
  double m = length(v1);
  assert(m > 0);
  return (vec3){v1.x / m, v1.y / m, v1.z / m};
}

mat4 mult_mm(const mat4 A, const mat4 B)
{
  // A = N x M (N rows and M columns)
  // B = M x P
  mat4 C;
  size_t N = 4, M = 4, P = 4;

  for (size_t i = 0; i < N; i++)
  {
    for (size_t j = 0; j < P; j++)
    {
      C.m[i * P + j] = 0;
      for (size_t k = 0; k < M; k++)
      {
        C.m[i * P + j] += (A.m[i * M + k] * B.m[k * P + j]);
      }
    }
  }
  return C;
}

vec3 mult_mv(const mat4 A, const vec3 v)
{
  size_t N = 4, M = 4, P = 1;

  double B[4] = {v.x, v.y, v.z, 1.0};
  double C[4];

  for (size_t i = 0; i < N; i++)
  {
    for (size_t j = 0; j < P; j++)
    {
      C[i * P + j] = 0;
      for (size_t k = 0; k < M; k++)
      {
        C[i * P + j] += (A.m[i * M + k] * B[k * P + j]);
      }
    }
  }

  return (vec3){C[0], C[1], C[2]};
}

mat4 translate(const vec3 v)
{
  mat4 m = {{1, 0, 0, v.x,
             0, 1, 0, v.y,
             0, 0, 1, v.z,
             0, 0, 0, 1}};
  return m;
}

mat4 rotate(const vec3 R)
{
  mat4 rotate_x = {{1, 0, 0, 0,
                    0, cos(R.x), -sin(R.x), 0,
                    0, sin(R.x), cos(R.x), 0,
                    0, 0, 0, 1}};

  mat4 rotate_y = {{
      cos(R.y),
      0,
      sin(R.y),
      0,
      0,
      1,
      0,
      0,
      -sin(R.y),
      0,
      cos(R.y),
      0,
      0,
      0,
      0,
      1,
  }};

  mat4 rotate_z = {{
      cos(R.z),
      -sin(R.z),
      0,
      0,
      sin(R.z),
      cos(R.z),
      0,
      0,
      0,
      0,
      1,
      0,
      0,
      0,
      0,
      1,
  }};

  return rotate_y;
}

void print_v(const char *msg, const vec3 v)
{
  printf("%s: (vec3) { %f, %f, %f }\n", msg, v.x, v.y, v.z);
}

void print_t(const triangle_t triangle)
{
  printf("(triangle_t) {\n");
  for (size_t i = 0; i < 3; i++)
  {
    printf("   (vec3) { %f, %f, %f },\n", triangle.v[i].x, triangle.v[i].y, triangle.v[i].z);
  }
  printf("}\n");
}

void print_m(const mat4 m)
{
  for (int i = 0; i < 4; i++)
  {
    for (int j = 0; j < 4; j++)
    {
      printf(" %6.1f, ", m.m[i * 4 + j]);
    }
    printf("\n");
  }
}

vec3 reflect(const vec3 In, const vec3 N)
{
  return sub(In, mult_s(N, 2 * dot(In, N)));
}

vec3 refract(const vec3 In, const vec3 N, double iot)
{
  double cosi = CLAMP_BETWEEN(dot(In, N), -1, 1);
  double etai = 1, etat = iot;
  vec3 n = N;
  if (cosi < 0)
  {
    cosi = -cosi;
  }
  else
  {
    double tmp = etai;
    etai = etat;
    etat = tmp;
    n = mult_s(N, -1);
  }
  double eta = etai / etat;
  double k = 1 - eta * eta * (1 - cosi * cosi);
  return k < 0 ? ZERO_VECTOR : add(mult_s(In, eta), mult_s(n, eta * cosi - sqrtf(k)));
}

ray_t get_camera_ray(const camera_t *camera, double u, double v)
{
  vec3 direction = sub(
      camera->position,
      add(
          camera->lower_left_corner,
          add(mult_s(camera->horizontal, u), mult_s(camera->vertical, v))));

  return (ray_t){camera->position, normalize(direction)};
}

vec3 sample_texture(vec3 color, double u, double v)
{
  double M = 10;
  double checker = (fmod(u * M, 1.0) > 0.5) ^ (fmod(v * M, 1.0) < 0.5);
  double c = 0.3 * (1 - checker) + 0.7 * checker;
  return mult_s(color, c);
}

bool intersect(const ray_t *ray, object_t *objects, size_t n, hit_t *hit)
{
  // ray_count++;
  double old_t = hit != NULL ? hit->t : DBL_MAX;
  double min_t = old_t;

  hit_t local = {.t = DBL_MAX};

  for (int i = 0; i < n; i++)
  {
    switch (objects[i].type)
    {
    case MESH:
    {
      mesh_t *mesh = objects[i].geometry.mesh;
      for (int ti = 0; ti < mesh->num_triangles; ti++)
      {
        vertex_t v0 = mesh->verts[(ti * 3) + 0];
        vertex_t v1 = mesh->verts[(ti * 3) + 1];
        vertex_t v2 = mesh->verts[(ti * 3) + 2];

        /*
        printf("vertex_t {\n");
        print_v(v0.pos);
        print_v(v1.pos);
        print_v(v1.pos);
        printf("}\n");
        */

        if (intersect_triangle(ray, v0, v1, v2, &local) && local.t < min_t)
        {
          min_t = local.t;
          local.object = &objects[i];
          local.point = point_at(ray, local.t);
          local.normal = normalize(cross(v1.pos, v0.pos));
        }
      }

      break;
    }
    case SPHERE:
    {
      if (intersect_sphere(ray, objects[i].geometry.sphere, &local) && local.t < min_t)
      {
        min_t = local.t;
        local.object = &objects[i];
        local.point = point_at(ray, local.t);
        local.normal = normalize(div_s(sub(local.point, objects[i].geometry.sphere->center),
                                       objects[i].geometry.sphere->radius));
      }
      break;
    }
    default:
    {
      puts("unknown geometry");
      exit(EXIT_FAILURE);
    }
    }
  }

  if (hit != NULL)
  {
    memcpy(hit, &local, sizeof(*hit));
  }

  return min_t < old_t;
}

vec3 phong(vec3 color, vec3 light_dir, vec3 normal, vec3 camera_origin, vec3 position, bool in_shadow, double ka, double ks, double kd, double alpha)
{
  /*
  double ka = 0.18;
  double ks = 0.4;
  double kd = 0.8;
  double alpha = 10;
  */

  // ambient
  vec3 ambient = mult_s(color, ka);

  // diffuse
  vec3 norm = normalize(normal);
  vec3 diffuse = mult_s(color, kd * MAX(dot(norm, light_dir), 0.0));

  // specular
  vec3 view_dir = normalize(sub(position, camera_origin));
  vec3 reflected = reflect(light_dir, norm);
  vec3 specular = mult_s(color, ks * pow(MAX(dot(view_dir, reflected), 0.0), alpha));

  return in_shadow ? ambient : clamp(add(add(ambient, diffuse), specular));
}

vec3 calculate_color(const ray_t *ray, object_t *objects, size_t n_objects, int depth, hit_t hit)
{
  vec3 out_color = ZERO_VECTOR;

  vec3 light_pos = {0, 3, -1};
  vec3 light_color = {1, 1, 1};
  vec3 light_dir = normalize(sub(light_pos, hit.point));
  ray_t light_ray = {hit.point, normalize(sub(light_pos, hit.point))};
  bool in_shadow = intersect(&light_ray, objects, n_objects, NULL);

  double ka = 0.18, ks = 0.4, kd = 0.8, alpha = 5;

  switch (hit.object->material.type)
  {
  case REFLECTION:
  {
    // TODO: compute fresnel equation
    ray_t reflected = {hit.point, normalize(reflect(ray->direction, hit.normal))};
    vec3 reflected_color = trace_path_v3(&reflected, objects, n_objects, depth + 1);
    double f = kd;
    out_color = add(mult_s(reflected_color, (1 - f)), mult_s(hit.object->material.color, (f)));
    out_color = phong(out_color, light_dir, hit.normal, ray->origin, hit.point, in_shadow, ka, ks, kd, alpha);
    break;
  }

  case REFLECTION_AND_REFRACTION:
  {
    ray_t reflected = {hit.point, normalize(reflect(ray->direction, hit.normal))};
    vec3 reflected_color = trace_path_v3(&reflected, objects, n_objects, depth + 1);

    double iot = 1;
    ray_t refracted = {hit.point, normalize(refract(ray->direction, hit.normal, iot))};
    vec3 refracted_color = trace_path_v3(&refracted, objects, n_objects, depth + 1);

    double kr = .8;

    vec3 refracted_reflected = add(mult_s(refracted_color, kr), mult_s(reflected_color, (1 - kr)));
    out_color = refracted_reflected;
    // out_color = phong_light(out_color, light_dir, hit.normal, ray->origin, hit.point, in_shadow);
    break;
  }

  case PHONG:
  {
    vec3 material_color = hit.object->material.color;
    out_color = phong(material_color, light_dir, hit.normal, ray->origin, hit.point, in_shadow, ka, ks, kd, alpha);
    break;
  }

  case CHECKERED:
  {
    vec3 material_color = sample_texture(hit.object->material.color, hit.u, hit.v);
    out_color = phong(material_color, light_dir, hit.normal, ray->origin, hit.point, in_shadow, ka, ks, kd, alpha);
    break;
  }

  case DIFFUSE:
  {

    double ks = 0.6;
    vec3 ambient = mult_s((vec3){0.5, 0.5, 0.5}, ks);

    vec3 normal = normalize(hit.normal);
    vec3 light_dir = normalize(sub(light_pos, hit.point));
    double diff = MAX(dot(normal, light_dir), 0.0);

    vec3 diffuse = mult_s(light_color, diff);
    out_color = clamp(mult(add(ambient, in_shadow ? ZERO_VECTOR : diffuse), hit.object->material.color));
    break;
  }

  case SOLID:
  {
    out_color = hit.object->material.color;
    break;
  }

  default:
  {
    puts("no material type set");
    exit(1);
  }
  }

  return out_color;
}

vec3 random_in_unit_sphere()
{
  int loop_counter = 0;
  while (true)
  {
    /*
    vec3 p = (vec3){
      random_range(-1, 1),
      random_range(-1, 1),
      random_range(-1, 1),
    };
    */

    vec3 p = RANDOM_COLOR;

    if (length2(p) < 1)
    {
      return p;
    }
    assert(++loop_counter < 100);
  }
}

vec3 random_in_hemisphere(vec3 normal)
{
  vec3 d = random_in_unit_sphere();

  if (dot(d, normal) < 0)
  {
    return mult_s(d, -1);
  }
  else
  {
    return d;
  }
}

vec3 trace_path_v1(ray_t *ray, object_t *objects, size_t nobj, int depth)
{
  // https://en.wikipedia.org/wiki/Path_tracing
  ray_count++;

  hit_t hit = {.t = DBL_MAX, .object = NULL};

  if (depth > MAX_DEPTH || !intersect(ray, objects, nobj, &hit))
  {
    return RGB(25, 25, 25);
  }

#if 0
  vec3 emittance_color = hit.object->material.color;
#else
  vec3 emittance_color;
  if (hit.object->material.type == LIGHT)
  {
    emittance_color = mult_s((vec3){1, 1, 1}, 4);
  }
  else
  {
    emittance_color = RGB(5, 5, 5);
  }
#endif

  vec3 attenuation = hit.object->material.color;

  ray_t new_ray;
  new_ray.origin = hit.point;
  new_ray.direction = random_in_hemisphere(hit.normal);

  double p = 1 / (2 * PI);

  double cos_theta = dot(new_ray.direction, hit.normal);

  // double reflectance = .5;
  // vec3 BRDF = reflectance / PI;

  vec3 recursive_color = trace_path_v1(&new_ray, objects, nobj, depth + 1);

  double d = CLAMP(dot(hit.normal, new_ray.direction));

  // return add(emittance_color, mult(mult_s(recursive_color, d), attenuation));
  // return mult_s(recursive_color, 0.5);
  return add(emittance_color, mult(attenuation, recursive_color));
  // return add(emittance_color, mult_s(recursive_color, CLAMP(cos_theta / p)));
}

vec3 trace_path_v2(ray_t *ray, object_t *objects, size_t nobj, int depth)
{
  ray_count++;

  hit_t hit = {.t = DBL_MAX, .object = NULL};

  if (depth > MAX_DEPTH || !intersect(ray, objects, nobj, &hit))
  {
    return BACKGROUND;
  }

  vec3 direct_light = ZERO_VECTOR, indirect_light = ZERO_VECTOR;

  light_t light = {
      .intensity = .75,
      .color = (vec3){1, 1, 1},
      .position = (vec3){2, 10, 2},
  };

  vec3 light_dir = normalize(sub(light.position, hit.point));
  ray_t light_ray = (ray_t){.origin = hit.point, .direction = light_dir};
  vec3 light_color = mult_s(light.color, light.intensity);

#if 1
  hit_t tmp;
  if (!intersect(&light_ray, objects, nobj, &tmp))
  {
    direct_light = add(direct_light, mult_s(light_color, MAX(dot(hit.normal, light_dir), 0)));
  }
#else
  if (hit.object->material.type == LIGHT)
  {
    return light_color;
  }
#endif

  size_t nsamples = MONTE_CARLO_SAMPLES;
  for (size_t i = 0; i < nsamples; i++)
  {
    ray_t new_ray = {.origin = hit.point, .direction = random_in_hemisphere(hit.normal)};
    vec3 color = trace_path_v2(&new_ray, objects, nobj, depth + 1);

    double theta = random_range(0.0, 1.0) * PI;
    double cosTheta = cos(theta);

    indirect_light = add(indirect_light, color);
    // indirect_light = add(indirect_light, mult_s(color, cosTheta));
  }

  indirect_light = mult_s(indirect_light, 1 / (double)nsamples);

  vec3 object_color = hit.object->material.color;
  if (hit.object->material.type == CHECKERED)
  {
    object_color = sample_texture(object_color, hit.u, hit.v);
  }

  // return mult_s(indirect_light, 0.5);
  return mult(add(direct_light, indirect_light), object_color);
  // return mult_s(mult(add(direct_light, indirect_light), object_color), 1 / PI);
}

vec3 trace_path_v3(ray_t *ray, object_t *objects, size_t nobj, int depth)
{
  ray_count++;

  if (depth > MAX_DEPTH)
  {
    return BACKGROUND;
  }

  hit_t hit = {.t = DBL_MAX, .object = NULL};

  if (!intersect(ray, objects, nobj, &hit))
  {
    return BACKGROUND;
  }

  return calculate_color(ray, objects, nobj, depth, hit);
}

#if 0
void load_file(void *ctx, const char *filename, const int is_mtl, const char *obj, char **buffer, size_t *len)
{
  long string_size = 0, read_size = 0;
  FILE *handler = fopen(filename, "r");

  if (handler)
  {
    fseek(handler, 0, SEEK_END);
    string_size = ftell(handler);
    rewind(handler);
    *buffer = (char *)malloc(sizeof(char) * (string_size + 1));
    read_size = fread(*buffer, sizeof(char), (size_t)string_size, handler);
    (*buffer)[string_size] = '\0';
    if (string_size != read_size)
    {
      free(buffer);
      *buffer = NULL;
    }
    fclose(handler);
  }
  *len = read_size;
}

bool load_obj(const char *filename, mesh_t *mesh)
{
  tinyobj_attrib_t attrib;
  tinyobj_shape_t *shape = NULL;
  tinyobj_material_t *material = NULL;

  size_t num_shapes;
  size_t num_materials;

  tinyobj_attrib_init(&attrib);

  int result = tinyobj_parse_obj(
      &attrib,
      &shape,
      &num_shapes,
      &material,
      &num_materials,
      filename,
      load_file,
      NULL,
      TINYOBJ_FLAG_TRIANGULATE);

  assert(result == TINYOBJ_SUCCESS);
  assert(attrib.num_faces == 36);
  assert(attrib.num_normals == 6);  // one per side
  assert(attrib.num_vertices == 8); // one per corner
  assert(attrib.num_texcoords == 0);
  assert(attrib.num_face_num_verts == 12); // two triangles per side

  assert(mesh != NULL);

  mesh->num_triangles = attrib.num_face_num_verts;
  mesh->triangles = malloc(mesh->num_triangles * sizeof(*mesh->triangles));

  assert(mesh->triangles != NULL);

  size_t face_offset = 0;
  // iterate over all faces
  for (size_t i = 0; i < attrib.num_face_num_verts; i++)
  {
    const int face_num_verts = attrib.face_num_verts[i]; // num verts per face

    assert(face_num_verts == 3);     // assert all faces are triangles
    assert(face_num_verts % 3 == 0); // assert all faces are triangles
    // printf("[%d] face_num_verts = %d\n", i, face_num_verts);

    tinyobj_vertex_index_t idx0 = attrib.faces[face_offset + 0];
    tinyobj_vertex_index_t idx1 = attrib.faces[face_offset + 1];
    tinyobj_vertex_index_t idx2 = attrib.faces[face_offset + 2];

    triangle_t triangle;

    for (int k = 0; k < 3; k++)
    {
      int f0 = idx0.v_idx;
      int f1 = idx1.v_idx;
      int f2 = idx2.v_idx;

      assert(f0 >= 0);
      assert(f1 >= 0);
      assert(f2 >= 0);

      vec3 v = {
          attrib.vertices[3 * (size_t)f0 + k],
          attrib.vertices[3 * (size_t)f1 + k],
          attrib.vertices[3 * (size_t)f2 + k],
      };

      triangle.v[k] = v;
    }

    mesh->triangles[i] = triangle;

    face_offset += (size_t)face_num_verts;
  }

  if (shape)
    tinyobj_shapes_free(shape, num_shapes);

  tinyobj_attrib_free(&attrib);
  return true;
}
#endif

/*==================[end of file]===========================================*/