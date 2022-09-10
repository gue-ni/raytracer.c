/**
 * a minimal raytracer
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
#define HEIGHT 380
#define EPSILON 1e-8
#define MAX_DEPTH 5
#define SAMPLES   25
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(x) (MAX(0, MIN(x, 1)))
#define RED                 (vec3_t) {1, 0, 0}
#define GREEN               (vec3_t) {0, 1, 0}
#define BLUE                (vec3_t) {0, 0, 1}
#define BACKGROUND_COLOR    (vec3_t) { 135.0 / 255.0, 206.0 / 255.0, 235.0 / 255.0 }

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
    PHONG,
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
    vec3_t point;
    vec3_t normal;
    double t; // hit time
} hit_t;

typedef struct camera {
    vec3_t origin, horizontal, vertical, lower_left_corner;
} camera_t;

double max(double a, double b) { return a > b ? a : b; }

double dot(const vec3_t v1, const vec3_t v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }

double length2(const vec3_t v1) { return v1.x  * v1.x + v1.y * v1.y + v1.z * v1.z; }

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

bool equal_d(double a, double b) { return fabs(a - b) < EPSILON; }

vec3_t clamp(const vec3_t v1) { return (vec3_t) {CLAMP(v1.x), CLAMP(v1.y), CLAMP(v1.z)}; }

vec3_t normalize(const vec3_t v1) {
    double m = length(v1);
    assert(m > 0);
    return (vec3_t) {v1.x / m, v1.y / m, v1.z / m};
}

void print_vec(const vec3_t v) {
    printf("(vec3_t) { %f, %f, %f }\n", v.x, v.y, v.z);
}

ray_t reflect_ray(const ray_t *ray, const vec3_t normal) {
    double tmp = 2 * dot(ray->direction, normal);
    vec3_t reflected = minus(ray->direction, scalar_multiply(normal, tmp));
    return (ray_t) {ray->origin, reflected};
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

    vec3_t hh = scalar_divide(camera->horizontal, 2);
    vec3_t hv = scalar_divide(camera->vertical, 2);

    camera->lower_left_corner = minus(
            minus(camera->origin, hh),
            minus(hv, (vec3_t) {0, 0, focal_length})
    );

    //camera->lower_left_corner = origin - (horizontal / double(2)) - (vertical / double(2)) - Vec3d(0, 0, focal_length);
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
    //direction.z = -direction.z;
    return (ray_t) {camera->origin, direction};
}

bool solve_quadratic(double a, double b, double c, double *x0, double *x1) {
    double discr = b * b - 4 * a * c;

    if (discr < 0) {
        return false;
    } else if (discr == 0) {
        *x1 = -0.5 * (b + sqrt(discr));
        *x0 = *x1;
    } else {
        double q = (b > 0) ? -0.5 * (b + sqrt(discr)) : -0.5 * (b - sqrt(discr));
        *x0 = q / a;
        *x1 = c / q;
    }

    if (x0 > x1) {
        double tmp = *x0;
        *x0 = *x1;
        *x1 = tmp;
    }
    return true;
}

bool intersect_sphere_v1(const ray_t *ray, const sphere_t *sphere, double t_min, double t_max, double *t) {
#if 0
    vec3_t oc = minus(ray->origin, sphere->center);
    double a = dot(ray->direction, ray->direction);
    double c = dot(oc, oc) - sphere->radius * sphere->radius;
    double b = 2.0 * dot(oc, ray->direction);
    double discriminant = b * b - 4 * a * c;
    return (discriminant > 0);
#else
    vec3_t oc = minus(ray->origin, sphere->center);
    double a = length2(ray->direction);
    double half_b = dot(oc, ray->direction);
    double c = length2(oc) - (sphere->radius * sphere->radius);

    double discriminant = half_b * half_b - a * c;
    if (discriminant < 0)
        return false;

    double sqrt_d = sqrt(discriminant);

    double root = (-half_b - sqrt_d) / a;
    if (root < t_min || t_max < root) {
        root = (-half_b + sqrt_d) / a;

        if (root < t_min) return false;
        if (root > t_max) return false;
    }

    *t = root;
    return true;
#endif
}

// this is somehow broken
bool intersect_sphere_v2(const ray_t *ray, const sphere_t *sphere, double t_min, double t_max, double *t) {
    double t0, t1; // solutions for t if the ray intersects

    //printf("intersect_sphere() { \normal");

    vec3_t L = minus(sphere->center, ray->origin);
    //print_vec(L);

    double tca = dot(L, ray->direction);
    //printf("tca=%f\normal", tca);
    if (tca < 0) return false;

    double d2 = dot(L, L) - tca * tca;
    //printf("d2=%f\normal", d2);

    double radius2 = sphere->radius * sphere->radius;
    //printf("radius2=%f\normal", radius2);

    if (d2 > radius2)
        return false;

    double thc = sqrt(radius2 - d2);
    //printf("thc=%f\normal", thc);
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
    for (int y = 0; y < HEIGHT; y++) {
        for (int x = 0; x < WIDTH; x++) {
            //vec3_t pixel = buffer[y * WIDTH + x];
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

vec3_t cast_ray(const ray_t *ray, const sphere_t *spheres, size_t n, int depth) {
    if (depth > MAX_DEPTH) {
        return BACKGROUND_COLOR;
    }

    hit_t hit = {.t = DBL_MAX};

    double t = DBL_MAX, t2;
    const sphere_t *sphere = NULL;

    int i;
    for (i = 0; i < n; i++) {
        if (intersect_sphere_v1(ray, &spheres[i], 1e-8, hit.t, &t) && t < hit.t) {
            //puts("hit");
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

    vec3_t color;

    switch (sphere->material.type) {
        case REFLECTION: {
            // TODO: compute reflection ray
            // TODO: compute fresnel equation

            ray_t reflected = reflect_ray(ray, hit.normal);
            color = cast_ray(&reflected, spheres, n, depth + 1);
            break;
        }

        case REFLECTION_AND_REFRACTION: {
            color = (vec3_t) {1, 1, 0};
            break;
        }

        case PHONG: {
            vec3_t light_pos = {1, 3, 0};
            vec3_t light_color = {1, 1, 1};
            double ambient_strength = 0.6;

            vec3_t ambient = scalar_multiply((vec3_t) {0.5, 0.5, 0.5}, ambient_strength);

            vec3_t normal = normalize(hit.normal);
            vec3_t light_dir = normalize(minus(light_pos, hit.point));
            double diff = max(dot(normal, light_dir), 0.0);

            vec3_t diffuse = scalar_multiply(light_color, diff);
            color = clamp(multiply(clamp(plus(ambient, diffuse)), sphere->material.color));
            break;
        }

        default: {
            puts("no material type set");
            exit(1);
        }
    }
    return color;
#endif
}

void render() {

    sphere_t spheres[] = {
            {{0, -100, -10}, 100, { GREEN, PHONG }},
            {{1,  0, -3.5}, .5,  {RED,          PHONG}},
            {{0,  0, -3},   .75, {RANDOM_COLOR, PHONG}},
            {{-1, 0, -2.5}, .5,  {BLUE,         PHONG}},
    };

    camera_t camera;
    init_camera(&camera);

    uint8_t framebuffer[WIDTH * HEIGHT * 3] = { 0 };

    int x, y, s;
    double u, v;
    ray_t  ray;

    for (y = 0; y < HEIGHT; y++) {
        for (x = 0; x < WIDTH; x++) {
            vec3_t pixel = {0, 0, 0};
            for (s = 0; s < SAMPLES; s++) {
                u = (double) (x + random()) / ((double) WIDTH - 1.0);
                v = (double) (y + random()) / ((double) HEIGHT - 1.0);

                ray = get_camera_ray(&camera, u, v);
                pixel = plus(pixel, cast_ray(&ray, spheres, sizeof(spheres) / sizeof(spheres[0]), 0));
            }

            if (x == (int)(WIDTH/ 2) && y == (int)(HEIGHT / 2)){
                print_vec(ray.direction);
            }

            pixel = scalar_divide(pixel, SAMPLES);
            pixel = scalar_multiply(pixel, 255); // scale to range 0-255

            size_t index = (y * WIDTH + x) * 3;
            framebuffer[index + 0] = (uint8_t)(pixel.x);
            framebuffer[index + 1] = (uint8_t)(pixel.y);
            framebuffer[index + 2] = (uint8_t)(pixel.z);
        }
    }

    //show(framebuffer);

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

        vec3_t dir1 = (vec3_t) { 0.001545, 0.001740, 0.999997 };
        vec3_t dir2 = (vec3_t) { 0.0, 0.0, -1 };
        //vec3_t direction = normalize(minus(sphere.center, origin));

        ray_t ray = {origin, dir2};
        double t0, t1;

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

#if 1
    render();
#else
    run_tests();
#endif
    return 0;
}