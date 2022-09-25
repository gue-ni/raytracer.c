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



void exit_error(const char *message)
{
    fprintf(stderr, "[ERROR] (%s:%d) %s\n", __FILE__, __LINE__, message);
    exit(EXIT_FAILURE);
}



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
