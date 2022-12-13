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

static int _test_check(int cond, const char *filename, int line, const char *expr, bool exit_after_fail)
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

void test_normal()
{
  {
    vec3 a = {2, 3, 4};
    vec3 b = {5, 6, 7};
    vec3 c = vec3_cross(a, b);
    TEST_CHECK(vec3_equal(c, (vec3){-3, 6, -3}));
  }

  const double X = 1, Y = 1, Z = 1;
  {
    vec3 v0 = {-X, -Y, -Z};
    vec3 v1 = {+X, -Y, -Z};
    vec3 v2 = {+X, +Y, -Z};
    vec3 normal = calculate_surface_normal(v0, v1, v2);
  }
  {
    vec3 v0 = {-X, +Y, +Z};
    vec3 v1 = {+X, +Y, +Z};
    vec3 v2 = {+X, +Y, -Z};
    vec3 normal = calculate_surface_normal(v0, v1, v2);
    TEST_CHECK(vec3_equal(normal, (vec3){0, 1, 0}));
  }
}

int main()
{
  test_normal();
  return 0;
}
