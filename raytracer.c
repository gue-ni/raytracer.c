#include "raytracer.h"


int ray_count = 0;
int intersection_test_count = 0;

double random_double() { return (double)rand() / ((double)RAND_MAX + 1); }

static double dot(const vec3 v1, const vec3 v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }

double length2(const vec3 v1) { return v1.x * v1.x + v1.y * v1.y + v1.z * v1.z; }

double length(const vec3 v1) { return sqrt(length2(v1)); }

vec3 cross(const vec3 a, const vec3 b)
{
  return (vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

vec3 mult(const vec3 v1, const vec3 v2)
{
  return (vec3){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z};
}

vec3 sub(const vec3 v1, const vec3 v2) { return (vec3){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z}; }

vec3 add(const vec3 v1, const vec3 v2) { return (vec3){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}; }

vec3 mult_s(const vec3 v1, const double s)
{
  return (vec3){v1.x * s, v1.y * s, v1.z * s};
}

vec3 div_s(const vec3 v1, const double s) { return (vec3){v1.x / s, v1.y / s, v1.z / s}; }

vec3 point_at(const ray_t *ray, double t)
{
  return add(ray->origin, mult_s(ray->direction, t));
}

vec3 clamp(const vec3 v1) { return (vec3){CLAMP(v1.x), CLAMP(v1.y), CLAMP(v1.z)}; }

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

void print_v(const vec3 v)
{
  printf("(vec3) { %f, %f, %f }\n", v.x, v.y, v.z);
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

static vec3 reflect(const vec3 I, const vec3 N)
{
  return sub(I, mult_s(N, 2 * dot(I, N)));
}

static vec3 refract(const vec3 I, const vec3 N, double iot)
{
  double cosi = CLAMP_BETWEEN(dot(I, N), -1, 1);
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
  return k < 0 ? ZERO_VECTOR : add(mult_s(I, eta), mult_s(n, eta * cosi - sqrtf(k)));
}

static void init_camera(camera_t *camera, vec3 position, options_t options)
{
#if 0
    double viewport_height = 2.0;
#else
  double theta = 45.0 * (3.1516 / 180);
  double h = tan(theta / 2);
  double viewport_height = 2.0 * h;
#endif
  double aspect_ratio = (double)options.width / (double)options.height;
  double viewport_width = aspect_ratio * viewport_height;
  double focal_length = 1.0;

  camera->origin = position;
  camera->horizontal = (vec3){viewport_width, 0, 0};
  camera->vertical = (vec3){0, viewport_height, 0};

  vec3 half_h = div_s(camera->horizontal, 2);
  vec3 half_v = div_s(camera->vertical, 2);

  camera->lower_left_corner = sub(sub(camera->origin, half_h), sub(half_v, (vec3){0, 0, focal_length}));
}

static ray_t get_camera_ray(const camera_t *camera, double u, double v)
{
  vec3 direction = sub(
      camera->origin,
      add(
          camera->lower_left_corner,
          add(mult_s(camera->horizontal, u), mult_s(camera->vertical, v))));

  direction = normalize(direction);
  assert(EQ(length(direction), 1.0));
  assert(direction.z < 0);
  return (ray_t){camera->origin, direction};
}

static char rgb_to_char(uint8_t r, uint8_t g, uint8_t b)
{
  char grey[10] = " .:-=+*#%@";
  double grayscale_value = ((r + g + b) / 3.0) * 0.999;
  int index = (int)(sizeof(grey) * (grayscale_value / 255.0));
  assert(0 <= index && index < 10);
  return grey[9 - index];
}

static vec3 sample_texture(double u, double v)
{
  return RED;
}

static void show(const uint8_t *buffer, const options_t options)
{
  // TODO scale to appropriate size
  for (int y = 0; y < options.height; y++)
  {
    for (int x = 0; x < options.width; x++)
    {
      size_t i = (y * options.width + x) * 3;
      printf("%c", rgb_to_char(buffer[i + 0], buffer[i + 1], buffer[i + 2]));
    }
    printf("\n");
  }
}

bool intersect_sphere(const ray_t *ray, const sphere_t *sphere, hit_t *hit)
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
    return true;
  }
  else
  {
    return false;
  }
}

bool intersect_triangle(const ray_t *ray, const triangle_t *triangle, hit_t *hit)
{
  intersection_test_count++;
  // https://en.wikipedia.org/wiki/M%C3%B6ller%E2%80%93Trumbore_intersection_algorithm
  vec3 v0 = triangle->v[0];
  vec3 v1 = triangle->v[1];
  vec3 v2 = triangle->v[2];
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
    hit->u = u;
    hit->v = v;
    return true;
  }
  else
  {
    return false;
  }
}

static bool intersect(const ray_t *ray, object_t *objects, size_t n, hit_t *hit)
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
      for (int j = 0; j < mesh->num_triangles; j++)
      {
        triangle_t triangle = mesh->triangles[j];
        if (intersect_triangle(ray, &triangle, &local) && local.t < min_t)
        {
          min_t = local.t;
          local.object = &objects[i];
          local.point = point_at(ray, local.t);
          local.normal = cross(triangle.v[0], triangle.v[1]);
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
        local.normal = div_s(sub(local.point, objects[i].geometry.sphere->center),
                             objects[i].geometry.sphere->radius);
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

static vec3 phong_lighting(vec3 color, vec3 light_dir, vec3 normal, vec3 camera_origin, vec3 position, bool in_shadow)
{
  double ka = 0.18;
  double ks = 0.4;
  double kd = 0.8;
  double alpha = 10;

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

static vec3 cast_ray(const ray_t *ray, object_t *scene, size_t n_objects, int depth)
{
  ray_count++;

  if (depth > MAX_DEPTH)
  {
    return BACKGROUND_COLOR;
  }

  hit_t hit = {.t = DBL_MAX, .object = NULL};

  if (!intersect(ray, scene, n_objects, &hit))
  {
    return BACKGROUND_COLOR;
  }

  vec3 out_color = ZERO_VECTOR;

  vec3 light_pos = {1, 3, 0};
  vec3 light_color = {1, 1, 1};
  vec3 light_dir = normalize(sub(light_pos, hit.point));
  ray_t light_ray = {hit.point, normalize(sub(light_pos, hit.point))};
  bool in_shadow = intersect(&light_ray, scene, n_objects, NULL);

  switch (hit.object->material.type)
  {
  case REFLECTION:
  {
    // TODO: compute fresnel equation
    ray_t reflected = {hit.point, normalize(reflect(ray->direction, hit.normal))};
    vec3 reflected_color = cast_ray(&reflected, scene, n_objects, depth + 1);
    double f = 0.5;
    out_color = add(mult_s(reflected_color, f), mult_s(hit.object->material.color, (1 - f)));
    out_color = phong_lighting(out_color, light_dir, hit.normal, ray->origin, hit.point, in_shadow);
    break;
  }

  case REFLECTION_AND_REFRACTION:
  {
    ray_t reflected = {hit.point, normalize(reflect(ray->direction, hit.normal))};
    vec3 reflected_color = cast_ray(&reflected, scene, n_objects, depth + 1);

    double iot = 1;
    ray_t refracted = {hit.point, normalize(refract(ray->direction, hit.normal, iot))};
    vec3 refracted_color = cast_ray(&refracted, scene, n_objects, depth + 1);

    double kr = .8;

    vec3 refracted_reflected = add(mult_s(refracted_color, kr), mult_s(reflected_color, (1 - kr)));
    out_color = refracted_reflected;
    // out_color = phong_light(out_color, light_dir, hit.normal, ray->origin, hit.point, in_shadow);
    break;
  }

  case PHONG:
  {
    out_color = phong_lighting(hit.object->material.color, light_dir, hit.normal, ray->origin, hit.point, in_shadow);
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



void render(uint8_t *framebuffer, object_t *objects, size_t n_objects, const options_t options)
{
  camera_t camera;
  init_camera(&camera, (vec3){0, 0, 1}, options);

  int x, y, s;
  double u, v;
  ray_t ray;

  for (y = 0; y < options.height; y++)
  {
    for (x = 0; x < options.width; x++)
    {
      vec3 pixel = {0, 0, 0};
      for (s = 0; s < options.samples; s++)
      {
        u = (double)(x + random_double()) / ((double)options.width - 1.0);
        v = (double)(y + random_double()) / ((double)options.height - 1.0);

        ray = get_camera_ray(&camera, u, v);
        pixel = add(pixel, cast_ray(&ray, objects, n_objects,, 0));
      }

      pixel = div_s(pixel, options.samples);
      pixel = mult_s(pixel, 255.0); // scale to range 0-255

      size_t index = (y * options.width + x) * 3;
      framebuffer[index + 0] = (uint8_t)(pixel.x);
      framebuffer[index + 1] = (uint8_t)(pixel.y);
      framebuffer[index + 2] = (uint8_t)(pixel.z);
    }
  }
}
