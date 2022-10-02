/**
 * a minimal raytracer
 * https://www.scratchapixel.com/lessons/3d-basic-rendering/ray-tracing-overview/light-transport-ray-tracing-whitted
 */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <assert.h>
#include <tgmath.h>
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

int main(int argc, char **argv)
{
    srand((unsigned)time(NULL));

    if (argc <= 1)
    {
        fprintf(stderr, "Usage: %s -w <width> -h <height> -s <samples per pixel> -o <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    options_t options = {
        .width = 320,
        .height = 180,
        .samples = 50,
        .result = "result.png",
        .obj = "assets/cube.obj"};

    size_t optind;
    for (optind = 1; optind < argc; optind++)
    {
        switch (argv[optind][1])
        {
        case 'h':
            options.height = atoi(argv[optind + 1]);
            break;
        case 'w':
            options.width = atoi(argv[optind + 1]);
            break;
        case 's':
            options.samples = atoi(argv[optind + 1]);
            break;
        case 'o':
            options.result = argv[optind + 1];
            break;

        default:
            break;
        }
    }
    argv += optind;

    sphere_t spheres[] = {
        {
            {1.1, 0, 0},
            .5,
        },
        {
            {0, 0, 0},
            .5,
        },

        {
            {-1.1, 0, 0},
            .5,
        },
    };

    vec3 pos = {0, -2.5, 0};
    vec3 size = {8, 2, 8};

    mesh_t mesh = {
        .num_triangles = 2,
        .verts = (vertex_t[]){
            /* top */
            {{pos.x - size.x, pos.y + size.y, pos.z - size.z}, /* 6 top back left */        {1,0}},
            {{pos.x + size.x, pos.y + size.y, pos.z - size.z}, /* 4 top back right */       {0,0}},
            {{pos.x - size.x, pos.y + size.y, pos.z + size.z}, /* 5 top front left */       {1,1}},

            {{pos.x + size.x, pos.y + size.y, pos.z - size.z}, /* 4 top back right */       {0,0}},
            {{pos.x + size.x, pos.y + size.y, pos.z + size.z}, /* 8 top front right */      {0,1}},
            {{pos.x - size.x, pos.y + size.y, pos.z + size.z}, /* 5 top front left */       {1,1}},

            /* bottom */
            {{pos.x + size.x, pos.y - size.y, pos.z + size.z}, /* 3 bottom front right */   {0,1}},
            {{pos.x - size.x, pos.y - size.y, pos.z + size.z}, /* 1 bottom front left */    {1,1}},
            {{pos.x + size.x, pos.y - size.y, pos.z - size.z}, /* 0 bottom back right */    {0,0}},

            {{pos.x - size.x, pos.y - size.y, pos.z - size.z}, /* 2 bottom back right */     {1,0}},
            {{pos.x + size.x, pos.y - size.y, pos.z - size.z}, /* 0 bottom back left */    {0,0}},
            {{pos.x - size.x, pos.y - size.y, pos.z + size.z}, /* 1 bottom front left */    {1,1}},

            /* back */
            {{pos.x - size.x, pos.y - size.y, pos.z - size.z}, /* 2 bottom back right */     {0,0}},
            {{pos.x + size.x, pos.y - size.y, pos.z - size.z}, /* 0 bottom back left m */    {1,0}},
            {{pos.x + size.x, pos.y + size.y, pos.z - size.z}, /* 4 top back right */       {1,1}},

            {{pos.x - size.x, pos.y - size.y, pos.z - size.z}, /* 2 bottom back right */     {0,0}},
            {{pos.x - size.x, pos.y + size.y, pos.z - size.z}, /* 6 top back left */        {0,1}},
            {{pos.x + size.x, pos.y + size.y, pos.z - size.z}, /* 4 top back right */       {1,1}},


            /* right */
            {{pos.x - size.x, pos.y - size.y, pos.z - size.z}, /* 2 bottom back right */     {0,0}},
            {{pos.x - size.x, pos.y - size.y, pos.z + size.z}, /* 1 bottom front left */    {1,1}},
            {{pos.x - size.x, pos.y + size.y, pos.z - size.z}, /* 6 top back left */        {1,0}},

            {{pos.x - size.x, pos.y - size.y, pos.z + size.z}, /* 1 bottom front left */    {1,1}},
            {{pos.x - size.x, pos.y + size.y, pos.z + size.z}, /* 5 top front left */       {1,1}},
            {{pos.x - size.x, pos.y + size.y, pos.z - size.z}, /* 6 top back left */        {1,0}},

            /* left */
            {{pos.x + size.x, pos.y - size.y, pos.z - size.z}, /* 0 bottom back left m */    {1,0}},
            {{pos.x + size.x, pos.y - size.y, pos.z + size.z}, /* 3 bottom front right */   {0,1}},
            {{pos.x + size.x, pos.y + size.y, pos.z + size.z}, /* 8 top front right */      {0,1}},

            {{pos.x + size.x, pos.y - size.y, pos.z - size.z}, /* 0 bottom back left m */    {1,0}},
            {{pos.x + size.x, pos.y + size.y, pos.z - size.z}, /* 4 top back right */       {1,1}},
            {{pos.x + size.x, pos.y + size.y, pos.z + size.z}, /* 8 top front right */      {0,1}},

            /* front */
            {{pos.x - size.x, pos.y - size.y, pos.z + size.z}, /* 1 bottom front left */    {1,1}},
            {{pos.x + size.x, pos.y - size.y, pos.z + size.z}, /* 3 bottom front right */   {0,1}},
            {{pos.x + size.x, pos.y + size.y, pos.z + size.z}, /* 8 top front right */      {0,1}},
        },
    };

   object_t scene[] = {
        {.type = MESH, .material = {GREEN, CHECKERED}, .geometry.mesh = &mesh},
        {.type = SPHERE, .material = {RED, PHONG  }, .geometry.sphere = &spheres[0]},
        {.type = SPHERE, .material = {RANDOM_COLOR, PHONG}, .geometry.sphere = &spheres[1]},
        {.type = SPHERE, .material = {GREEN, PHONG}, .geometry.sphere = &spheres[2]},
    };

    uint8_t *framebuffer = malloc(sizeof(*framebuffer) * options.width * options.height * 3);
    if (framebuffer == NULL)
    {
        fprintf(stderr, "could not allocate framebuffer\n");
        exit(1);
    }

    camera_t camera;
    init_camera(&camera, (vec3){1, 1, 3.5}, (vec3){0, 0, 0}, &options);

    clock_t tic = clock();

    render(framebuffer, scene, sizeof(scene) / sizeof(scene[0]), &camera, options);

    clock_t toc = clock();
    double time_taken = (double)(toc - tic) / CLOCKS_PER_SEC;

    printf("%d x %d pixels\n", options.width, options.height);
    printf("cast %d rays\n", ray_count);
    printf("checked %d possible intersections\n", intersection_test_count);
    printf("rendering took %f seconds\n", time_taken);
    printf("writing result to '%s'...\n", options.result);

    if (stbi_write_png(options.result, options.width, options.height, 3, framebuffer, options.width * 3) == 0)
    {
        fprintf(stderr, "failed to write");
        exit(EXIT_FAILURE);
    }
    else
    {
        printf("done.\n");
    }

    free(framebuffer);
    return 0;
}
