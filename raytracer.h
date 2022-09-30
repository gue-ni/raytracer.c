#ifndef RAYTRACER_H
#define RAYTRACER_H

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include <stdint.h>
#include <string.h>
#include <omp.h>

#ifndef PI
#define PI 3.14159265359
#endif
#define EPSILON 1e-8
#define MAX_DEPTH 5
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(x) (MAX(0, MIN(x, 1)))
#define CLAMP_BETWEEN(x, min_v, max_v) (MAX(min_v, MIN(max_v, 1)))
#define ABS(x) ((x < 0) ? (-x) : (x))
#define RGB(r, g, b) ((vec3){r / 255.0, g / 255.0, b / 255.0})
#define EQ(a, b) (ABS((a) - (b)) < EPSILON)

#define RED         RGB(255,   0,   0)
#define GREEN       RGB(  0, 192,  48)
#define BLUE        RGB(  0,   0, 255)
#define WHITE       RGB(255, 255, 255)
#define BACKGROUND  RGB(1, 130, 129)
//#define BACKGROUND  RGB(150, 150, 150)

#define ZERO_VECTOR RGB(0, 0, 0)
#define BLACK ZERO_VECTOR

extern int ray_count;
extern int intersection_test_count;

double random_double();

double random_range(double min, double max);

#define RANDOM_COLOR \
  (vec3) { random_double(), random_double(), random_double() }

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
  CHECKERED,
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
  vec3 v[3];
  vec2 uv[3];
} triangle_t;

typedef struct
{
  int *indices;
  vec3 *vertices;
  vec2 *tex;
  size_t num_triangles;
  vertex_t *verts;
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
  int width, height, samples;
  char *result;
  char *obj;
} options_t;

bool intersect_sphere(const ray_t *ray, const sphere_t *sphere, hit_t *hit);

bool intersect_triangle(const ray_t *ray, vertex_t vertex0, vertex_t vertex1,vertex_t vertex2, hit_t *hit);

vec3 point_at(const ray_t *ray, double t);

mat4 mult_mm(const mat4 A, const mat4 B);

vec3 mult_mv(const mat4 A, const vec3 v);

vec3 point_at(const ray_t *ray, double t);

mat4 translate(const vec3 v);

mat4 rotate(const vec3 R);

void print_v(const vec3 v);

void print_t(const triangle_t triangle);

void print_m(const mat4 m);

void init_camera(camera_t *camera, vec3 position, vec3 look_at, options_t options);

void render(uint8_t *framebuffer, object_t *objects, size_t n_objects, camera_t *camera, options_t options);

bool load_obj(const char *filename, mesh_t *mesh);

#endif /* RAYTRACER_H */