#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <math.h>
#include <time.h>
#include <float.h>
#include <stdint.h>
#include <string.h>

#ifdef __unix__
#define TERM_RED "\x1B[31m"
#define TERM_GRN "\x1B[32m"
#define TERM_YEL "\x1B[33m"
#define TERM_BLU "\x1B[34m"
#define TERM_MAG "\x1B[35m"
#define TERM_CYN "\x1B[36m"
#define TERM_WHT "\x1B[37m"
#define TERM_RESET "\x1B[0m"
#else 
#define TERM_RED ""
#define TERM_GRN ""
#define TERM_YEL ""
#define TERM_BLU ""
#define TERM_MAG ""
#define TERM_CYN ""
#define TERM_WHT ""
#define TERM_RESET ""
#endif

#include "raytracer.h"

#define TEST_CHECK(cond) _test_check((cond), __FILE__, __LINE__, #cond, false)
#define TEST_RESULT() _test_check(true, __FILE__, __LINE__, "", false)
#define TEST_ASSERT(cond) _test_check((cond), __FILE__, __LINE__, #cond, true)

int _test_check(int cond, const char *filename, int line, const char *expr, bool exit_after_fail)
{
  static int test_failures_so_far = 0;

  if (!cond)
  {
    test_failures_so_far++;
    fprintf(stderr, "[%sFAIL%s] [%s:%04d] '%s'\n", TERM_RED, TERM_RESET, filename, line, expr);
    if (exit_after_fail)
    {
      exit(test_failures_so_far);
    }
  }
  else
  {
    fprintf(stdout, "[ %sOK%s ] [%s:%04d]\n", TERM_GRN, TERM_RESET, filename, line);
  }
  return test_failures_so_far;
}

void test_intersect()
{
  hit_t hit;
  ray_t ray;

  vertex_t v0, v1, v2;
  v0.pos = (vec3){1, 0, -3};
  v1.pos = (vec3){0, 1, -3};
  v2.pos = (vec3){-1, 0, -3};

  ray = (ray_t){{0, 0, 0}, {0, 0, -1}};
  TEST_CHECK(intersect_triangle(&ray, v0, v1, v2, &hit) == true);
  TEST_CHECK(hit.t == 3.0);
  TEST_CHECK(point_at(&ray, hit.t).z == -3.0);

  sphere_t sphere = {{0, 0, -3}, 2.0};
  ray = (ray_t){{0, 0, 0}, {0, 0, -1}};
  TEST_CHECK(intersect_sphere(&ray, &sphere, &hit) == true);
  TEST_CHECK(hit.t == 1.0);
  TEST_CHECK(point_at(&ray, hit.t).z == -1.0);
}

void test_math()
{
  // matrix * matrix
  mat4 m0 = {{5, 7, 9, 10,
              2, 3, 3, 8,
              8, 10, 2, 3,
              3, 3, 4, 8}};
  mat4 m1 = {{3, 10, 12, 18,
              12, 1, 4, 9,
              9, 10, 12, 2,
              3, 12, 4, 10}};
  mat4 ref_m = {{
      210.0,
      267.0,
      236.0,
      271.0,
      93.0,
      149.0,
      104.0,
      149.0,
      171.0,
      146.0,
      172.0,
      268.0,
      105.0,
      169.0,
      128.0,
      169.0,
  }};
  mat4 res_m = mult_mm(m0, m1);
  TEST_CHECK(memcmp(&res_m, &ref_m, sizeof(mat4)) == 0);

  // matrix * vector
  vec3 v0 = {3, 7, 5};
  mat4 m3 = {{2, 3, 4, 0,
              11, 8, 7, 0,
              3, 2, 9, 0,
              0, 0, 0, 1}};
  vec3 ref_v = {47.0, 124.0, 68.0};
  vec3 res_v = mult_mv(m3, v0);
  TEST_CHECK(memcmp(&ref_v, &res_v, sizeof(vec3)) == 0);
}



bool compare_vector(vec3 a, vec3 b)
{
  return a.x == b.x && a.y == b.y && a.z == b.z;
}

void  test_normal()
{
  {
    vec3 a = {2,3,4};
    vec3 b = {5,6,7};
    vec3 c = cross(a,b);
    TEST_CHECK(compare_vector(c, (vec3){-3, 6, -3}));
  }
  
  const double X = 1, Y = 1, Z = 1;
  {
    vec3 v0 = {-X, -Y, -Z};
    vec3 v1 = {+X, -Y, -Z};
    vec3 v2 = {+X, +Y, -Z};
    vec3 normal = calculate_surface_normal(v0, v1, v2);
    print_v("normal", normal);
  }
  {
    vec3 v0 = {-X, +Y, +Z};
    vec3 v1 = {+X, +Y, +Z};
    vec3 v2 = {+X, +Y, -Z};
    vec3 normal = calculate_surface_normal(v0, v1, v2);
    print_v("normal", normal);
    TEST_CHECK(compare_vector(normal, (vec3){0, 1, 0}));
  }
}

int main()
{
  /*
  test_intersect();
  test_math();
  */

  test_normal();
  return 0;
}
