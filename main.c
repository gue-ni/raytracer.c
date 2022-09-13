/**
 * a minimal raytracer
 * https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-overview/light-transport-ray-tracing-whitted
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include <stdint.h>
#include <string.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "tinyobj_loader.h"

#define EPSILON 1e-8
#define MAX_DEPTH 5
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(x) (MAX(0, MIN(x, 1)))
#define CLAMP_BETWEEN(x, min_v, max_v) (MAX(min_v, MIN(max_v, 1)))
#define ABS(x) ((x < 0) ? (-x) : (x))
#define RGB(r, g, b) ((vec3){r / 255.0, g / 255.0, b / 255.0})

#define RED RGB(187, 52, 155)
#define BLUE RGB(122, 94, 147)
#define GREEN RGB(108, 69, 117)
#define ZERO_VECTOR RGB(0, 0, 0)
#define BACKGROUND_COLOR RGB(69, 185, 211)
#define BLACK ZERO_VECTOR

double random_double() { return (double)rand() / ((double)RAND_MAX + 1); }

#define RANDOM_COLOR \
    (vec3) { random_double(), random_double(), random_double() }

#define TEST_CHECK(cond) _test_check((cond), __FILE__, __LINE__, #cond, false)
#define TEST_ASSERT(cond) _test_check((cond), __FILE__, __LINE__, #cond, true)
int TEST_FAILURES = 0;
void _test_check(int cond, const char *filename, int line, const char *expr, bool abort_after_fail)
{
    if (!cond)
    {
        TEST_FAILURES++;
        fprintf(stderr, "[FAIL] [%s:%d] '%s'%s\n", filename, line, expr, abort_after_fail ? ", aborting..." : "");
        if (abort_after_fail)
        {
            exit(1);
        }
    }
    else
    {
        fprintf(stdout, "[ OK ] [%s:%d]\n", filename, line);
    }
}

typedef struct
{
    double x, y, z;
} vec3;

typedef struct
{
    vec3 origin, direction;
} ray_t;

typedef enum
{
    SOLID,
    DIFFUSE,
    REFLECTION,
    REFLECTION_AND_REFRACTION,
} material_type_t;

typedef struct
{
    vec3 color;
    material_type_t type;
} material_t;

typedef struct
{
    vec3 center;
    double radius;
} sphere_t;

typedef struct
{
    vec3 v0, v1, v2;
} triangle_t;

typedef struct
{
    size_t count;
    triangle_t *triangles;
} mesh_t;

typedef union
{
    sphere_t *sphere;
    mesh_t *mesh;
} geometry_t;

typedef enum
{
    SPHERE,
    MESH,
} geometry_type_t;

typedef struct
{
    geometry_type_t type;
    material_t material;
    geometry_t geometry;
} object_t;

typedef struct
{
    double t;
    double u, v;
    vec3 point;
    vec3 normal;
    object_t *object;
} hit_t;

typedef struct
{
    vec3 origin, horizontal, vertical, lower_left_corner;
} camera_t;

typedef struct
{
    double intensity;
    vec3 position, color;
} light_t;

typedef struct
{
    sphere_t *spheres;
    size_t ns;
    light_t *lights;
    size_t nl;
} render_context_t;

typedef struct
{
    int width, height, samples;
    char *result_filename;
    char *obj_filename;
} options_t;

long long ray_count = 0, intersection_test_count = 0;

static inline double dot(const vec3 v1, const vec3 v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }

static inline vec3 cross(const vec3 a, const vec3 b)
{
    return (vec3){a.y * b.z - a.z * b.y, a.z * b.x - a.x * b.z, a.x * b.y - a.y * b.x};
}

static inline double length2(const vec3 v1) { return v1.x * v1.x + v1.y * v1.y + v1.z * v1.z; }

static inline double length(const vec3 v1) { return sqrt(length2(v1)); }

static inline vec3 multiply(const vec3 v1, const vec3 v2)
{
    return (vec3){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z};
}

static inline vec3 divide(const vec3 v1, const vec3 v2)
{
    return (vec3){v1.x / v2.x, v1.y / v2.y, v1.z / v2.z};
}

static inline vec3 sub(const vec3 v1, const vec3 v2) { return (vec3){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z}; }

static inline vec3 add(const vec3 v1, const vec3 v2) { return (vec3){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}; }

static inline vec3 scalar_multiply(const vec3 v1, const double s)
{
    return (vec3){v1.x * s, v1.y * s, v1.z * s};
}

static inline vec3 scalar_divide(const vec3 v1, const double s) { return (vec3){v1.x / s, v1.y / s, v1.z / s}; }

inline static bool scalar_equals(const vec3 v1, const double s) { return v1.x == s && v1.y == s && v1.z == s; }

inline static bool vector_equals(const vec3 v1, const vec3 v2)
{
    return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z;
}

inline static vec3 point_at(const ray_t *ray, double t)
{
    return add(ray->origin, scalar_multiply(ray->direction, t));
}

inline static bool equal_d(double a, double b) { return ABS(a - b) < EPSILON; }

inline static vec3 clamp(const vec3 v1) { return (vec3){CLAMP(v1.x), CLAMP(v1.y), CLAMP(v1.z)}; }

inline static vec3 normalize(const vec3 v1)
{
    double m = length(v1);
    assert(m > 0);
    return (vec3){v1.x / m, v1.y / m, v1.z / m};
}

void print_vec(const vec3 v)
{
    printf("(vec3) { %f, %f, %f }\n", v.x, v.y, v.z);
}

vec3 reflect(const vec3 I, const vec3 N)
{
    return sub(I, scalar_multiply(N, 2 * dot(I, N)));
}

vec3 refract(const vec3 I, const vec3 N, double iot)
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
        n = scalar_multiply(N, -1);
    }
    double eta = etai / etat;
    double k = 1 - eta * eta * (1 - cosi * cosi);
    return k < 0 ? ZERO_VECTOR : add(scalar_multiply(I, eta), scalar_multiply(n, eta * cosi - sqrtf(k)));
}

void init_camera(camera_t *camera, vec3 position, options_t options)
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

    vec3 half_h = scalar_divide(camera->horizontal, 2);
    vec3 half_v = scalar_divide(camera->vertical, 2);

    camera->lower_left_corner = sub(sub(camera->origin, half_h), sub(half_v, (vec3){0, 0, focal_length}));
}

ray_t get_camera_ray(const camera_t *camera, double u, double v)
{
    vec3 direction = sub(
        camera->origin,
        add(
            camera->lower_left_corner,
            add(scalar_multiply(camera->horizontal, u), scalar_multiply(camera->vertical, v))));

    direction = normalize(direction);
    assert(equal_d(length(direction), 1.0));
    assert(direction.z < 0);
    return (ray_t){camera->origin, direction};
}

char rgb_to_char(uint8_t r, uint8_t g, uint8_t b)
{
    static char grey[10] = " .:-=+*#%@";
    double grayscale_value = ((r + g + b) / 3.0) * 0.999;
    int index = (int)(sizeof(grey) * (grayscale_value / 255.0));
    assert(0 <= index && index < 10);
    return grey[9 - index];
}

vec3 sample_texture(double u, double v)
{
    return RED;
}

void show(const uint8_t *buffer, const options_t options)
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
    vec3 v0 = triangle->v0;
    vec3 v1 = triangle->v1;
    vec3 v2 = triangle->v2;
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
            for (int j = 0; j < mesh->count; j++)
            {
                triangle_t triangle = mesh->triangles[j];
                if (intersect_triangle(ray, &triangle, &local) && local.t < min_t)
                {
                    min_t = local.t;
                    local.object = &objects[i];
                    local.point = point_at(ray, local.t);
                    local.normal = cross(triangle.v0, triangle.v1);
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
                local.normal = scalar_divide(sub(local.point, objects[i].geometry.sphere->center),
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

vec3 cast_ray(const ray_t *ray, object_t *scene, size_t n_objects, int depth)
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
    switch (hit.object->material.type)
    {
    case REFLECTION:
    {
        // TODO: compute fresnel equation
        ray_t reflected = {hit.point, normalize(reflect(ray->direction, hit.normal))};
        vec3 reflected_color = cast_ray(&reflected, scene, n_objects, depth + 1);
        double f = 0.5;
        out_color = add(scalar_multiply(reflected_color, f), scalar_multiply(hit.object->material.color, (1 - f)));
        break;
    }

    case REFLECTION_AND_REFRACTION:
    {
        vec3 reflected_dir = normalize(reflect(ray->direction, hit.normal));
        ray_t reflected = {hit.point, reflected_dir};
        vec3 reflected_color = cast_ray(&reflected, scene, n_objects, depth + 1);

        double iot = 1;
        vec3 refracted_dir = normalize(refract(ray->direction, hit.normal, iot));
        ray_t refracted = {hit.point, refracted_dir};
        vec3 refracted_color = cast_ray(&refracted, scene, n_objects, depth + 1);

        double kr = .8;

        out_color = add(scalar_multiply(refracted_color, kr), scalar_multiply(reflected_color, (1 - kr)));
        break;
    }

    case DIFFUSE:
    {
        vec3 light_pos = {1, 3, 0};
        vec3 light_color = {1, 1, 1};
        ray_t light_ray = {hit.point, normalize(sub(light_pos, hit.point))};

        bool in_shadow = intersect(&light_ray, scene, n_objects, NULL);

        double ambient_strength = 0.6;

        vec3 ambient = scalar_multiply((vec3){0.5, 0.5, 0.5}, ambient_strength);

        vec3 normal = normalize(hit.normal);
        vec3 light_dir = normalize(sub(light_pos, hit.point));
        double diff = MAX(dot(normal, light_dir), 0.0);

        vec3 diffuse = scalar_multiply(light_color, diff);
        out_color = clamp(multiply(add(ambient, in_shadow ? ZERO_VECTOR : diffuse), hit.object->material.color));
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

void load_file(void *ctx, const char *filename, const int is_mtl, const char *obj_filename, char **buffer, size_t *len)
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

bool load_obj(const char *filename, object_t *objects, size_t *num_objects)
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

    TEST_ASSERT(result == TINYOBJ_SUCCESS);
    TEST_CHECK(attrib.num_faces == 36);
    TEST_CHECK(attrib.num_face_num_verts == 12); // two triangles per side
    TEST_CHECK(attrib.num_normals == 6);         // one per side
    TEST_CHECK(attrib.num_vertices == 8);        // one per corner
    TEST_CHECK(attrib.num_texcoords == 0);

    triangle_t *triangles = malloc(attrib.num_face_num_verts * sizeof(*triangles));
    TEST_ASSERT(triangles != NULL);

    for (size_t i = 0; i < attrib.num_face_num_verts; i++)
    {

        TEST_ASSERT(attrib.face_num_verts[i] % 3 == 0); // check all faces are triangles

        for (size_t f = 0; f < attrib.face_num_verts[i] / 3; f++)
        {
        }
    }

    if (triangles){
        free(triangles);
    }

    tinyobj_attrib_free(&attrib);
    if (shape)
    {
        tinyobj_shapes_free(shape, num_shapes);
    }
    return true;
}

void render(uint8_t *framebuffer, const options_t options)
{
    sphere_t spheres[] = {
        {
            {0, -100, -15},
            100,
        },
        {
            {1, 0, -2},
            .5,
        },
        {
            {-0.5, 0, -5},
            .5,
        },
        {
            {0, 0, -3},
            .75,
        },
        {
            {-1.5, 0, -3},
            .5,
        },
    };

    vec3 pos = {0, 2, -3};
    vec3 size = {4, 3, 8};
    double w = 3;

    triangle_t triangles[] = {
        {
            {pos.x + size.x, pos.y - size.y, pos.z - size.z}, // lower left
            {pos.x - size.x, pos.y - size.y, pos.z + size.z}, // top right
            {pos.x - size.x, pos.y - size.y, pos.z - size.z}, // lower right
        },
        {
            {pos.x + size.x, pos.y - size.y, pos.z - size.z}, // lower left
            {pos.x - size.x, pos.y - size.y, pos.z + size.z}, // top right
            {pos.x + size.x, pos.y - size.y, pos.z + size.z}, // top left
        },
        {
            {pos.x + w, pos.y - w, pos.z - w}, // lower left
            {pos.x - w, pos.y + w, pos.z - w}, // top right
            {pos.x - w, pos.y - w, pos.z - w}, // lower right
        },
        {
            {pos.x + w, pos.y - w, pos.z - w}, // lower left
            {pos.x + w, pos.y + w, pos.z - w}, // top left
            {pos.x - w, pos.y + w, pos.z - w}, // top right
        },
        {
            {pos.x - w, pos.y - w, pos.z - w}, // top right
            {pos.x - w, pos.y - w, pos.z + w}, // top right
            {pos.x - w, pos.y + w, pos.z + w}, // top right
        },
    };

    mesh_t mesh = {
        2, triangles};

    object_t scene[] = {
        {.type = SPHERE, .material = {RED, DIFFUSE}, .geometry.sphere = &spheres[1]},
        {.type = SPHERE, .material = {RANDOM_COLOR, DIFFUSE}, .geometry.sphere = &spheres[2]},
        {.type = SPHERE, .material = {RANDOM_COLOR, REFLECTION_AND_REFRACTION}, .geometry.sphere = &spheres[3]},
        {.type = SPHERE, .material = {BLUE, REFLECTION}, .geometry.sphere = &spheres[4]},
        {.type = MESH, .material = {GREEN, DIFFUSE}, .geometry.mesh = &mesh}};

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
                pixel = add(pixel, cast_ray(&ray, scene, sizeof(scene) / sizeof(scene[0]), 0));
            }

            pixel = scalar_divide(pixel, options.samples);
            pixel = scalar_multiply(pixel, 255.0); // scale to range 0-255

            size_t index = (y * options.width + x) * 3;
            framebuffer[index + 0] = (uint8_t)(pixel.x);
            framebuffer[index + 1] = (uint8_t)(pixel.y);
            framebuffer[index + 2] = (uint8_t)(pixel.z);
        }
    }
}

/**
 * TEST
 */

void _test_load_obj()
{
    const char *obj = "assets/cube.obj";

    object_t *scene = NULL;
    size_t num_objects;

    TEST_CHECK(load_obj(obj, scene, &num_objects) == true);
}

void _test_intersect()
{
    hit_t hit;
    ray_t ray;

    triangle_t triangle = {
        {1, 0, -3},
        {0, 1, -3},
        {-1, 0, -3},
    };

    ray = (ray_t){{0, 0, 0}, {0, 0, -1}};
    TEST_CHECK(intersect_triangle(&ray, &triangle, &hit) == true);
    TEST_CHECK(hit.t == 3.0);
    TEST_CHECK(point_at(&ray, hit.t).z == -3.0);

    sphere_t sphere = {{0, 0, -3}, 2.0};
    ray = (ray_t){{0, 0, 0}, {0, 0, -1}};
    TEST_CHECK(intersect_sphere(&ray, &sphere, &hit) == true);
    TEST_CHECK(hit.t == 1.0);
    TEST_CHECK(point_at(&ray, hit.t).z == -1.0);
}

void _test_all()
{
    _test_load_obj();
    _test_intersect();
}

int main()
{
    srand((unsigned)time(NULL));
#ifdef RUN_TESTS
    _test_all();
    return TEST_FAILURES;
#else
    options_t options = {
        .width = 1280,
        .height = 720,
        .samples = 25,
        .result_filename = "result.png",
        .obj_filename = "assets/cube.obj"};

    uint8_t *framebuffer = (uint8_t *)malloc(sizeof(uint8_t) * options.width * options.height * 3);
    assert(framebuffer != NULL);

    clock_t tic = clock();

    render(framebuffer, options);

    clock_t toc = clock();
    double time_taken = (double)(toc - tic) / CLOCKS_PER_SEC;

    printf("%d x %d pixels\n", options.width, options.height);
    printf("cast %lld rays\n", ray_count);
    printf("checked %lld possible intersections\n", intersection_test_count);
    printf("rendering took %f seconds\n", time_taken);

    if (stbi_write_png(options.result_filename, options.width, options.height, 3, framebuffer, options.width * 3) ==
        0)
    {
        puts("failed to write");
        exit(1);
    }

    // show(framebuffer);
    free(framebuffer);
    return 0;
#endif
}
