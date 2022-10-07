#ifndef RAYTRACER_H
#define RAYTRACER_H

/*==================[inclusions]============================================*/

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <tgmath.h>
#include <time.h>
#include <float.h>
#include <stdint.h>
#include <string.h>
#include <omp.h>

/*==================[macros]================================================*/

#ifndef PI
#define PI 3.14159265359
#endif
#define EPSILON 1e-8
#define MAX_DEPTH 5
#define MONTE_CARLO_SAMPLES 1
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(x) (MAX(0, MIN(x, 1)))
#define CLAMP_BETWEEN(x, min_v, max_v) (MAX(min_v, MIN(max_v, 1)))
#define ABS(x) ((x < 0) ? (-x) : (x))
#define RGB(r, g, b) ((vec3){r / 255.0, g / 255.0, b / 255.0})
#define EQ(a, b) (ABS((a) - (b)) < EPSILON)
#define RED RGB(255, 0, 0)
#define GREEN RGB(0, 192, 48)
#define BLUE RGB(0, 0, 255)
#define WHITE RGB(255, 255, 255)
#define BACKGROUND RGB(1, 130, 129)
//#define BACKGROUND  RGB(150, 150, 150)
#define ZERO_VECTOR RGB(0, 0, 0)
#define ONE_VECTOR ((vec3) {1.0, 1.0, 1.0})
#define VECTOR(x, y, z) ((vec3) {(x), (y), (z)})
#define SINGLE_VECTOR(x) ((vec3) {x, x, x})
#define BLACK ZERO_VECTOR
#define RANDOM_COLOR \
  (vec3) { random_double(), random_double(), random_double() }

/*==================[type definitions]======================================*/

typedef struct
{
  double x, y;
} vec2;

typedef struct
{
  double x, y, z;
} vec3;

typedef struct
{
  double x, y, z, w;
} vec4;

typedef struct
{
  vec3 pos;
  vec2 tex;
} vertex_t;

typedef struct
{
  double m[16];
} mat4;

typedef struct
{
  vec3 origin, direction;
} ray_t;

typedef enum
{
  SOLID,
  LIGHT,
  PHONG,
  NORMAL,
  CHECKERED,
  DIFFUSE,
  REFLECTION,
  WIKIPEDIA_ALGORITHM,
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
  size_t num_triangles;
  vertex_t *vertices;
} mesh_t;

typedef union
{
  sphere_t *sphere;
  mesh_t *mesh;
} geometry_t;

typedef enum
{
  GEOMETRY_SPHERE,
  GEOMETRY_MESH,
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
  vec3 position, horizontal, vertical, lower_left_corner;
} camera_t;

typedef struct
{
  double intensity;
  vec3 position, color;
} light_t;

typedef struct
{
  int width, height, samples;
  char *result, *obj;
} options_t;

/*==================[external function declarations]========================*/

double random_double();

vec3 point_at(const ray_t *ray, double t);

vec3 mult_mv(mat4, vec3);
mat4 mult_mm(mat4, mat4);

vec3 add(const vec3, const vec3);
vec3 sub(const vec3, const vec3);
vec3 mult(const vec3, const vec3);
vec3 clamp(const vec3);
double length(const vec3);
double length2(const vec3);
double dot(const vec3, const vec3);

vec3 calculate_surface_normal(vec3 v0, vec3 v1, vec3 v2);

vec3 cross(const vec3, const vec3);
vec3 normalize(const vec3);

bool intersect_sphere(const ray_t *ray, sphere_t *sphere, hit_t *hit);
bool intersect_triangle(const ray_t *ray, vertex_t vertex0, vertex_t vertex1, vertex_t vertex2, hit_t *hit);

mat4 translate(vec3);
mat4 rotate(vec3);
mat4 scale(vec3);

void print_v(const char* msg, const vec3 v);
void print_m(const mat4 m);

void init_camera(camera_t *camera, vec3 position, vec3 target, options_t *options);

void render(uint8_t *framebuffer, object_t *objects, size_t n_objects, camera_t *camera, options_t *options);

bool load_obj(const char *filename, mesh_t *mesh);

/*==================[external constants]====================================*/
/*==================[external data]=========================================*/

extern int ray_count;
extern int intersection_test_count;

/*==================[end of file]===========================================*/

#endif /* RAYTRACER_H */