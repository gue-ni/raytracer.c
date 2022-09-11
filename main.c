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

#define STB_IMAGE_WRITE_IMPLEMENTATION

#include "stb_image_write.h"

#define WIDTH  640
#define HEIGHT 480
#define EPSILON 1e-8
#define MAX_DEPTH 50
#define SAMPLES   50
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(x) (MAX(0, MIN(x, 1)))
#define CLAMP_BETWEEN(x, min_v, max_v) (MAX(min_v, MIN(max_v, 1)))
#define ABS(x) ((x < 0) ? (-x) : (x))
#define RED                     (vec3_t) {1, 0, 0}
#define GREEN                   (vec3_t) {0, 1, 0}
#define BLUE                    (vec3_t) {0, 0, 1}
#define ZERO_VECTOR             (vec3_t) {0, 0, 0}
#define BACKGROUND_COLOR        (vec3_t) { 0.0 / 255.0, 172.0 / 255.0, 214.0 / 255.0 }
#define BLACK                   (vec3_t) {0, 0, 0}

double random() { return (double) rand() / ((double) RAND_MAX + 1); }

#define RANDOM_COLOR (vec3_t) { random(), random(), random() }

typedef struct vec3 {
    double x, y, z;
} vec3_t;

typedef struct ray {
    vec3_t origin, direction;
} ray_t;

typedef enum material_type {
    REFLECTION_AND_REFRACTION,
    REFLECTION,
    DIFFUSE,
} material_type_t;

typedef struct material {
    vec3_t color;
    material_type_t type;
} material_t;

typedef struct sphere {
    vec3_t center;
    double radius;
    material_t material;
} sphere_t;

typedef struct hit {
    double t;
    vec3_t point;
    vec3_t normal;
    sphere_t *sphere;
} hit_t;

typedef struct camera {
    vec3_t origin, horizontal, vertical, lower_left_corner;
} camera_t;

typedef struct light {
    double intensity;
} light_t;

typedef struct render_context {
    sphere_t *spheres;
    size_t ns;

    light_t *lights;
    size_t nl;
} render_context_t;

double dot(const vec3_t v1, const vec3_t v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }

double length2(const vec3_t v1) { return v1.x * v1.x + v1.y * v1.y + v1.z * v1.z; }

double length(const vec3_t v1) { return sqrt(length2(v1)); }

vec3_t multiply(const vec3_t v1, const vec3_t v2) { return (vec3_t) {v1.x * v2.x, v1.y * v2.y, v1.z * v2.z}; }

vec3_t divide(const vec3_t v1, const vec3_t v2) { return (vec3_t) {v1.x / v2.x, v1.y / v2.y, v1.z / v2.z}; }

vec3_t minus(const vec3_t v1, const vec3_t v2) { return (vec3_t) {v1.x - v2.x, v1.y - v2.y, v1.z - v2.z}; }

vec3_t plus(const vec3_t v1, const vec3_t v2) { return (vec3_t) {v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}; }

vec3_t scalar_multiply(const vec3_t v1, const double s) { return (vec3_t) {v1.x * s, v1.y * s, v1.z * s}; }

vec3_t scalar_divide(const vec3_t v1, const double s) { return (vec3_t) {v1.x / s, v1.y / s, v1.z / s}; }

bool scalar_equals(const vec3_t v1, const double s) { return v1.x == s && v1.y == s && v1.z == s; }

bool vector_equals(const vec3_t v1, const vec3_t v2) { return v1.x == v2.x && v1.y == v2.y && v1.z == v2.z; }

vec3_t point_at(const ray_t *ray, double t) { return plus(ray->origin, scalar_multiply(ray->direction, t)); }

bool equal_d(double a, double b) { return ABS(a - b) < EPSILON; }

vec3_t clamp(const vec3_t v1) { return (vec3_t) {CLAMP(v1.x), CLAMP(v1.y), CLAMP(v1.z)}; }

vec3_t clamp_between(const vec3_t v1, double min, double max) {
    return (vec3_t) {CLAMP_BETWEEN(v1.x, min, max), CLAMP_BETWEEN(v1.y, min, max), CLAMP_BETWEEN(v1.z, min, max)};
}

vec3_t normalize(const vec3_t v1) {
    double m = length(v1);
    assert(m > 0);
    return (vec3_t) {v1.x / m, v1.y / m, v1.z / m};
}

void print_vec(const vec3_t v) {
    printf("(vec3_t) { %f, %f, %f }\n", v.x, v.y, v.z);
}

vec3_t reflect(const vec3_t I, const vec3_t N) {
    return minus(I, scalar_multiply(N, 2 * dot(I, N)));
}

vec3_t refract(const vec3_t I, const vec3_t N, double iot) {
    float cosi = CLAMP_BETWEEN(dot(I, N), -1, 1);
    float etai = 1, etat = iot;
    vec3_t n = N;
    if (cosi < 0) {
        cosi = -cosi;
    } else {
        double tmp = etai;
        etai = etat;
        etat = tmp;
        n = scalar_multiply(N, -1);
    }
    float eta = etai / etat;
    float k = 1 - eta * eta * (1 - cosi * cosi);
    return k < 0 ? ZERO_VECTOR : plus(scalar_multiply(I, eta), scalar_multiply(n, eta * cosi - sqrtf(k)));

}

void init_camera(camera_t *camera) {
#if 0
    double viewport_height = 2.0;
#else
    double theta = 45.0 * (3.1516 / 180);
    double h = tan(theta / 2);
    double viewport_height = 2.0 * h;
#endif
    double aspect_ratio = (double) WIDTH / (double) HEIGHT;
    double viewport_width = aspect_ratio * viewport_height;
    double focal_length = 1.0;

    camera->origin = (vec3_t) {0, 0, 0};
    camera->horizontal = (vec3_t) {viewport_width, 0, 0};
    camera->vertical = (vec3_t) {0, viewport_height, 0};

    vec3_t half_h = scalar_divide(camera->horizontal, 2);
    vec3_t half_v = scalar_divide(camera->vertical, 2);

    camera->lower_left_corner = minus(
            minus(camera->origin, half_h),
            minus(half_v, (vec3_t) {0, 0, focal_length})
    );
}

ray_t get_camera_ray(const camera_t *camera, double u, double v) {
    vec3_t direction = minus(
            camera->origin,
            plus(
                    camera->lower_left_corner,
                    plus(scalar_multiply(camera->horizontal, u), scalar_multiply(camera->vertical, v)))
    );

    direction = normalize(direction);
    assert(equal_d(length(direction), 1.0));
    assert(direction.z < 0);
    return (ray_t) {camera->origin, direction};
}

bool intersect_sphere(const ray_t *ray, const sphere_t *sphere, double *t) {
    double t0, t1; // solutions for t if the ray intersects


    vec3_t L = minus(sphere->center, ray->origin);

    double tca = dot(L, ray->direction);
    if (tca < 0) return false;

    double d2 = dot(L, L) - tca * tca;

    double radius2 = sphere->radius * sphere->radius;

    if (d2 > radius2)
        return false;

    double thc = sqrt(radius2 - d2);

    t0 = tca - thc;
    t1 = tca + thc;

    if (t0 > t1) {
        double tmp = t0;
        t0 = t1;
        t1 = tmp;
    }

    if (t0 < 0) {
        t0 = t1; // if t0 is negative, let's use t1 instead
        if (t0 < 0)
            return false; // both t0 and t1 are negative
    }
    //printf("t0=%f, t1=%f\normal", t0, t1);
    //printf("}\normal");
    *t = t0;
    return true;
}

char rgb_to_char(uint8_t r, uint8_t g, uint8_t b) {
    static char grey[10] = " .:-=+*#%@";
    double grayscale_value = ((r + g + b) / 3.0) * 0.999;
    int index = (int) (sizeof(grey) * (grayscale_value / 255.0));
    assert(0 <= index && index < 10);
    return grey[9 - index];
}

void show(const uint8_t *buffer) {
    // TODO scale to appropriate size
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            size_t i = (y * WIDTH + x) * 3;
            uint8_t r, g, b;
            r = buffer[i + 0];
            g = buffer[i + 1];
            b = buffer[i + 2];
            printf("%c", rgb_to_char(r, g, b));
        }
        printf("\n");
    }
}

bool intersect_all(const ray_t *ray, const sphere_t *spheres, size_t n_spheres, hit_t *hit) {
    double t = DBL_MAX;
    for (int i = 0; i < n_spheres; i++) {
        if (intersect_sphere(ray, &spheres[i], &t) && t < hit->t) {
            hit->t = t;
            hit->sphere = &spheres[i];
            hit->point = point_at(ray, t);
            hit->normal = scalar_divide(minus(hit->point, hit->sphere->center), hit->sphere->radius);
        }
    }
}

vec3_t cast_ray(const ray_t *ray, const sphere_t *spheres, size_t n, int depth) {
    if (depth > MAX_DEPTH) {
        return BACKGROUND_COLOR;
    }

    double t = DBL_MAX;
    hit_t hit = {.t = DBL_MAX};
    const sphere_t *sphere = NULL;

    for (int i = 0; i < n; i++) {
        if (intersect_sphere(ray, &spheres[i], &t) && 1e-8 < t && t < hit.t) {
            sphere = &spheres[i];
            hit.t = t;
            hit.point = point_at(ray, t);
            hit.normal = scalar_divide(minus(hit.point, spheres[i].center), spheres[i].radius);
        }
    }

    if (sphere == NULL) {
        return BACKGROUND_COLOR;
    }

#if 0
    return (vec3_t) { 1, 0, 0};
#else
    vec3_t out_color = ZERO_VECTOR;
    switch (sphere->material.type) {
        case REFLECTION: {
            // TODO: compute fresnel equation
            ray_t reflected = {hit.point, normalize(reflect(ray->direction, hit.normal))};
            vec3_t reflected_color = cast_ray(&reflected, spheres, n, depth + 1);

            out_color = reflected_color;

            double f = 0.5;
            out_color = plus(scalar_multiply(reflected_color, f), scalar_multiply(sphere->material.color, (1 - f)));
            break;
        }

        case REFLECTION_AND_REFRACTION: {
            vec3_t reflected_dir = normalize(reflect(ray->direction, hit.normal));
            ray_t reflected = {hit.point, reflected_dir};
            vec3_t reflected_color = cast_ray(&reflected, spheres, n, depth + 1);

            double iot = 1;
            vec3_t refracted_dir = normalize(refract(ray->direction, hit.normal, iot));
            ray_t refracted = {hit.point, refracted_dir};
            vec3_t refracted_color = cast_ray(&refracted, spheres, n, depth + 1);

            double kr = .8;

            out_color = plus(scalar_multiply(refracted_color, kr), scalar_multiply(reflected_color, (1 - kr)));
            break;
        }

        case DIFFUSE: {
            vec3_t light_pos = {1, 3, 0};
            vec3_t light_color = {1, 1, 1};
            double intensity = 1.0;
            ray_t light_ray = {hit.point, normalize(minus(light_pos, hit.point))};

            double t_min;
            bool in_shadow = false;
            for (int i = 0; i < n; i++) {
                if (intersect_sphere(&light_ray, &spheres[i], &t_min)) {
                    in_shadow = true;
                }
            }
            double ambient_strength = 0.6;

            vec3_t ambient = scalar_multiply((vec3_t) {0.5, 0.5, 0.5}, ambient_strength);

            vec3_t normal = normalize(hit.normal);
            vec3_t light_dir = normalize(minus(light_pos, hit.point));
            double diff = MAX(dot(normal, light_dir), 0.0);

            vec3_t diffuse = scalar_multiply(light_color, diff);
            out_color = clamp(multiply(plus(ambient, in_shadow ? ZERO_VECTOR : diffuse), sphere->material.color));
            break;
        }

        default: {
            puts("no material type set");
            exit(1);
        }
    }
    return out_color;
#endif
}

void render() {
    sphere_t spheres[] = {
            {{0,    -100, -15}, 100, {GREEN,        DIFFUSE}},
            {{1,    0,    -2},  .5,  {RED,          DIFFUSE}},
            {{0.5,  0,    -5},  .5,  {RANDOM_COLOR,         DIFFUSE}},
            {{0,    0,    -3},  .75, {RANDOM_COLOR, REFLECTION_AND_REFRACTION}},
            {{-1.5, 0,    -3},  .5,  {BLUE,         REFLECTION}},
    };

    camera_t camera;
    init_camera(&camera);

    uint8_t framebuffer[WIDTH * HEIGHT * 3] = {0};

    int x, y, s;
    double u, v;
    ray_t ray;

    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
            vec3_t pixel = {0, 0, 0};
            for (s = 0; s < SAMPLES; s++) {
                u = (double) (x + random()) / ((double) WIDTH - 1.0);
                v = (double) (y + random()) / ((double) HEIGHT - 1.0);

                ray = get_camera_ray(&camera, u, v);
                pixel = plus(pixel, cast_ray(&ray, spheres, sizeof(spheres) / sizeof(spheres[0]), 0));
            }

            pixel = scalar_divide(pixel, SAMPLES);
            pixel = scalar_multiply(pixel, 255); // scale to range 0-255

            size_t index = (y * WIDTH + x) * 3;
            framebuffer[index + 0] = (uint8_t) (pixel.x);
            framebuffer[index + 1] = (uint8_t) (pixel.y);
            framebuffer[index + 2] = (uint8_t) (pixel.z);
        }
    }

    if (stbi_write_png(
            "image.png",
            WIDTH,
            HEIGHT,
            3,
            framebuffer,
            WIDTH * 3) == 0) {
        puts("failed to write");
        exit(1);
    }

    //show(framebuffer);
}

void run_tests() {
    printf("TEST\n");

    // test math
    {
        {
            vec3_t a = {1, 0, 3,};
            vec3_t b = {-1, 4, 2};
            vec3_t r = {0, 4, 5};
            assert(vector_equals(plus(a, b), r));
        }
        {
            vec3_t a = {1, 0, 3,};
            vec3_t b = {-1, 4, 2};
            vec3_t r = {2, -4, 1};
            assert(vector_equals(minus(a, b), r));
        }
    }

    {
        sphere_t sphere = {{0, 0, -3}, 2.0};
        vec3_t origin = {0, 0, 0};

        vec3_t dir1 = (vec3_t) {0.001545, 0.001740, 0.999997};
        vec3_t dir2 = (vec3_t) {0.0, 0.0, -1};
        //vec3_t direction = normalize(minus(sphere.center, origin));

        ray_t ray = {origin, dir2};
        double t0 = 0, t1 = 0;

        assert(intersect_sphere_v1(&ray, &sphere, 0, DBL_MAX, &t0));
        printf("%f\n", t0);
        print_vec(point_at(&ray, t0));

        assert(intersect_sphere_v2(&ray, &sphere, 0, DBL_MAX, &t1));
        printf("%f\n", t1);
        print_vec(point_at(&ray, t1));

        assert(t0 == t1);
    }
}

int main() {
    srand((unsigned) time(NULL));
    clock_t tic = clock();
#if 1

    render();
#else
    run_tests();
#endif
    clock_t toc = clock();
    double time_taken = (double) (toc - tic) / CLOCKS_PER_SEC;
    printf("Finished (%f seconds)\n", time_taken);
    return 0;
}