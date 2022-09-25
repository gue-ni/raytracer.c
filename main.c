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
#include "lib/stb_image_write.h"

#include "raytracer.h"


extern int ray_count;
extern int intersection_test_count;

#define TEST_CHECK(cond) _test_check((cond), __FILE__, __LINE__, #cond, false)

#define TEST_ASSERT(cond) _test_check((cond), __FILE__, __LINE__, #cond, true)

int _test_check(int cond, const char *filename, int line, const char *expr, bool exit_after_fail)
{
    static int _test_failures = 0;
    if (!cond)
    {
        _test_failures++;
        fprintf(stderr, "[FAIL] [%s:%d] '%s'\n", filename, line, expr);
        if (exit_after_fail)
        {
            exit(_test_failures);
        }
    }
    else
    {
        fprintf(stdout, "[ OK ] [%s:%d]\n", filename, line);
    }
    return _test_failures;
}

void exit_error(const char *message)
{
    fprintf(stderr, "[ERROR] (%s:%d) %s\n", __FILE__, __LINE__, message);
    exit(EXIT_FAILURE);
}

/*
void _test_load_obj()
{
    mesh_t mesh;
    load_obj("assets/cube.obj", &mesh);

    TEST_CHECK(mesh.num_triangles == 12);
    TEST_CHECK(mesh.triangles != NULL);

    for (int i = 0; i < mesh.num_triangles; i++)
    {
        print_t(mesh.triangles[i]);
    }

    if (mesh.triangles)
        free(mesh.triangles);
}

void _test_intersect()
{
    hit_t hit;
    ray_t ray;

    triangle_t triangle = {
        {
            {1, 0, -3},
            {0, 1, -3},
            {-1, 0, -3},
        }};

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

void _test_math()
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

void _test_all()
{
    _test_intersect();
    //_test_load_obj();
    _test_math();
}
*/

int main()
{
    srand((unsigned)time(NULL));
#ifdef RUN_TESTS
    _test_all();
    return TEST_CHECK(true); // return number of failed tests
#else
    options_t options = {
        .width = 6,
        .height = 4,
        .samples = 5,
        .result = "result.png",
        .obj = "assets/cube.obj"};

    uint8_t *framebuffer = malloc(sizeof(*framebuffer) * options.width * options.height * 3);
    if (framebuffer == NULL)
    {
        fprintf(stderr, "could not allocate framebuffer\n");
        exit(1);
    }

    clock_t tic = clock();

    render(framebuffer, options);

    clock_t toc = clock();
    double time_taken = (double)(toc - tic) / CLOCKS_PER_SEC;

    printf("%d x %d pixels\n", options.width, options.height);
    printf("cast %d rays\n", ray_count);
    printf("checked %d possible intersections\n", intersection_test_count);
    printf("rendering took %f seconds\n", time_taken);
    printf("writing result to '%s'...\n", options.result);

    int r = stbi_write_png(options.result, options.width, options.height, 3, framebuffer, options.width * 3);
    if (r == 0)
    {
        fprintf(stderr, "failed to write");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("done.\n");
    }

    // show(framebuffer);
    free(framebuffer);
    return 0;
#endif
}
