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

int main(int argc, char **argv)
{
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

    vec3 pos = {0, 0, -3};
    vec3 size = {4, 2, 4};

    mesh_t mesh = {
        .vertices = (vec3[]){
            {pos.x + size.x, pos.y - size.y, pos.z - size.z}, // 0 bottom back right
            {pos.x - size.x, pos.y - size.y, pos.z + size.z}, // 1 bottom front left
            {pos.x - size.x, pos.y - size.y, pos.z - size.z}, // 2 bottom back left
            {pos.x + size.x, pos.y - size.y, pos.z + size.z}, // 3 bottom front right

            {pos.x + size.x, pos.y + size.y, pos.z - size.z}, // 4 top back right
            {pos.x - size.x, pos.y + size.y, pos.z + size.z}, // 5 top front left
            {pos.x - size.x, pos.y + size.y, pos.z - size.z}, // 6 top back left
            {pos.x + size.x, pos.y + size.y, pos.z + size.z}, // 8 top front right
        },
        .indices = (int[]){
            0,1,2,
            0,1,3,
            0,2,4,
        },
        .tex = (vec2[]){
            {0, 0},{1, 1},{1, 0},

            {0, 1},
        },
        .verts = (vertex_t[]){
            {{pos.x + size.x, pos.y - size.y, pos.z - size.z}, /* 0 bottom back right */    {0,0}},
            {{pos.x - size.x, pos.y - size.y, pos.z + size.z}, /* 1 bottom front left */    {1,1}},
            {{pos.x - size.x, pos.y - size.y, pos.z - size.z}, /* 2 bottom back left */     {1,0}},

            {{pos.x + size.x, pos.y - size.y, pos.z - size.z}, /* 0 bottom back right */    {0,0}},
            {{pos.x - size.x, pos.y - size.y, pos.z + size.z}, /* 1 bottom front left */    {1,1}},
            {{pos.x + size.x, pos.y - size.y, pos.z + size.z}, /* 3 bottom front right */   {0,1}},

            {{pos.x + size.x, pos.y - size.y, pos.z - size.z}, /* 0 bottom back right */    {0,0}},
            {{pos.x - size.x, pos.y - size.y, pos.z - size.z}, /* 2 bottom back left */     {1,0}},
            {{pos.x + size.x, pos.y + size.y, pos.z - size.z}, /* 4 top back right */       {0,0}},

            {{pos.x - size.x, pos.y + size.y, pos.z - size.z}, /* 6 top back left */        {0,0}},
            {{pos.x + size.x, pos.y - size.y, pos.z - size.z}, /* 0 bottom back right */    {0,0}},
            {{pos.x - size.x, pos.y - size.y, pos.z - size.z}, /* 2 bottom back left */     {1,0}},

            {{pos.x + size.x, pos.y + size.y, pos.z - size.z}, /* 4 top back right */       {0,0}},
            {{pos.x - size.x, pos.y + size.y, pos.z + size.z}, /* 5 top front left */       {1,1}},
            {{pos.x - size.x, pos.y + size.y, pos.z - size.z}, /* 6 top back left */        {1,0}},

            {{pos.x + size.x, pos.y + size.y, pos.z - size.z}, /* 4 top back right */       {0,0}},
            {{pos.x - size.x, pos.y + size.y, pos.z + size.z}, /* 5 top front left */       {1,1}},
            {{pos.x + size.x, pos.y + size.y, pos.z + size.z}, /* 8 top front right */      {0,1}},


            {{pos.x + size.x, pos.y + size.y, pos.z - size.z}, /* 4 top back right */       {0,0}},
            {{pos.x - size.x, pos.y + size.y, pos.z + size.z}, /* 5 top front left */       {0,0}},
            {{pos.x - size.x, pos.y + size.y, pos.z - size.z}, /* 6 top back left */        {0,0}},
            {{pos.x + size.x, pos.y + size.y, pos.z + size.z}, /* 8 top front right */      {0,0}},


        },
        .num_triangles = 5,
        .triangles = NULL,
    };

    /*
    mesh_t cube;
    load_obj("assets/cube.obj", &cube);

    vec3 euler = {.5, .5, .5};
    mat4 rot = rotate(euler);

    vec3 new_pos = {0, 0, -3};
    mat4 trans = translate(new_pos);

    for (size_t i = 0; i < cube.num_triangles; i++)
    {
        for (size_t j = 0; j < 3; j++)
        {
            vec3 *v = &cube.triangles[i].v[j];
            *v = mult_mv(mult_mm(trans, rot), *v);
        }
    }
    */

    object_t scene[] = {
        {.type = MESH, .material = {RGB(200, 200, 200), CHECKERED}, .geometry.mesh = &mesh},
        {.type = SPHERE, .material = {GREEN, REFLECTION}, .geometry.sphere = &spheres[1]},
        {.type = SPHERE, .material = {RED, PHONG}, .geometry.sphere = &spheres[2]},
        {.type = SPHERE, .material = {RANDOM_COLOR, REFLECTION_AND_REFRACTION}, .geometry.sphere = &spheres[3]},
        {.type = SPHERE, .material = {BLUE, REFLECTION}, .geometry.sphere = &spheres[4]},
    };

    uint8_t *framebuffer = malloc(sizeof(*framebuffer) * options.width * options.height * 3);
    if (framebuffer == NULL)
    {
        fprintf(stderr, "could not allocate framebuffer\n");
        exit(1);
    }

    srand((unsigned)time(NULL));

    clock_t tic = clock();

    render(framebuffer, scene, sizeof(scene) / sizeof(scene[0]), options);

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
}
