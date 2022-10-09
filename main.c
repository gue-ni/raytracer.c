#include <stdio.h>
#include <signal.h>
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

#define SPHERE(x, y, z, r) ((sphere_t) { { (x), (y) + (r), (z) }, (r) })

options_t options = {
    .width = 320,
    .height = 180,
    .samples = 50,
    .result = "result.png",
    .obj = "assets/cube.obj"
};

uint8_t *framebuffer = NULL;

extern int ray_count;
extern int intersection_test_count;

void write_image(int signal)
{
    if (framebuffer != NULL)
    {
        if (stbi_write_png(options.result, options.width, options.height, 3, framebuffer, options.width * 3) == 0)
            exit(EXIT_FAILURE);
        else
            printf("done.\n");

        free(framebuffer);
    }
}

void apply_matrix(mesh_t* mesh, mat4 matrix)
{
    #pragma omp parallel for
    for (uint i = 0; i < mesh->num_triangles * 3; i++)
    {
        mesh->vertices[i].pos = mult_mv(matrix, mesh->vertices[i].pos);
    }
}

void parse_options(int argc, char **argv, options_t *options)
{
    uint optind;
    for (optind = 1; optind < argc; optind++)
    {
        switch (argv[optind][1])
        {
        case 'h':
            options->height = atoi(argv[optind + 1]);
            break;
        case 'w':
            options->width = atoi(argv[optind + 1]);
            break;
        case 's':
            options->samples = atoi(argv[optind + 1]);
            break;
        case 'o':
            options->result = argv[optind + 1];
            break;

        default:
            break;
        }
    }
    argv += optind;
}

int main(int argc, char **argv)
{
    srand((unsigned)time(NULL));

    if (argc <= 1)
    {
        fprintf(stderr, "Usage: %s -w <width> -h <height> -s <samples per pixel> -o <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    parse_options(argc, argv, &options);

    vec3 pos = {0, 0, 0};
    vec3 size = {1, 1, 1.5};

    mesh_t cube = {
        .num_triangles = 2,
        .vertices = (vertex_t[]){
            {{ -0.5f, +0.5f, -0.5f },{ 0.0f, 1.0f }},
            {{ +0.5f, +0.5f, -0.5f },{ 1.0f, 1.0f }},
            {{ +0.5f, +0.5f, +0.5f },{ 1.0f, 0.0f }},
            {{ +0.5f, +0.5f, +0.5f },{ 1.0f, 0.0f }},
            {{ -0.5f, +0.5f, +0.5f },{ 0.0f, 0.0f }},
            {{ -0.5f, +0.5f, -0.5f },{ 0.0f, 1.0f }},
            {{ -0.5f, -0.5f, -0.5f },{ 0.0f, 0.0f }},
            {{ +0.5f, -0.5f, -0.5f },{ 1.0f, 0.0f }},
            {{ +0.5f, +0.5f, -0.5f },{ 1.0f, 1.0f }},
            {{ +0.5f, +0.5f, -0.5f },{ 1.0f, 1.0f }},
            {{ -0.5f, +0.5f, -0.5f },{ 0.0f, 1.0f }},
            {{ -0.5f, -0.5f, -0.5f },{ 0.0f, 0.0f }},
            {{ -0.5f, -0.5f, +0.5f },{ 0.0f, 0.0f }},
            {{ +0.5f, -0.5f, +0.5f },{ 1.0f, 0.0f }},
            {{ +0.5f, +0.5f, +0.5f },{ 1.0f, 1.0f }},
            {{ +0.5f, +0.5f, +0.5f },{ 1.0f, 1.0f }},
            {{ -0.5f, +0.5f, +0.5f },{ 0.0f, 1.0f }},
            {{ -0.5f, -0.5f, +0.5f },{ 0.0f, 0.0f }},
            {{ -0.5f, +0.5f, +0.5f },{ 1.0f, 0.0f }},
            {{ -0.5f, +0.5f, -0.5f },{ 1.0f, 1.0f }},
            {{ -0.5f, -0.5f, -0.5f },{ 0.0f, 1.0f }},
            {{ -0.5f, -0.5f, -0.5f },{ 0.0f, 1.0f }},
            {{ -0.5f, -0.5f, +0.5f },{ 0.0f, 0.0f }},
            {{ -0.5f, +0.5f, +0.5f },{ 1.0f, 0.0f }},
            {{ +0.5f, +0.5f, +0.5f },{ 1.0f, 0.0f }},
            {{ +0.5f, +0.5f, -0.5f },{ 1.0f, 1.0f }},
            {{ +0.5f, -0.5f, -0.5f },{ 0.0f, 1.0f }},
            {{ +0.5f, -0.5f, -0.5f },{ 0.0f, 1.0f }},
            {{ +0.5f, -0.5f, +0.5f },{ 0.0f, 0.0f }},
            {{ +0.5f, +0.5f, +0.5f },{ 1.0f, 0.0f }},
            {{ -0.5f, -0.5f, -0.5f },{ 0.0f, 1.0f }},
            {{ +0.5f, -0.5f, -0.5f },{ 1.0f, 1.0f }},
            {{ +0.5f, -0.5f, +0.5f },{ 1.0f, 0.0f }},
            {{ +0.5f, -0.5f, +0.5f },{ 1.0f, 0.0f }},
            {{ -0.5f, -0.5f, +0.5f },{ 0.0f, 0.0f }},
            {{ -0.5f, -0.5f, -0.5f },{ 0.0f, 1.0f }},
        }
    };

    mat4 t = translate((vec3){0,-1,0});
    mat4 s = scale((vec3){16, 0.5, 16});
    apply_matrix(&cube, s);

    const double y = 0.25;

    object_t scene[] = {
        {
            .type = GEOMETRY_MESH, 
            .material = { 
                .color = RGB(109,124,187), 
                .flags = M_PHONG | M_CHECKERED | M_GLOBAL
            }, 
            .geometry.mesh = &cube
        },
        /*
        {
            .type = GEOMETRY_SPHERE, 
            .material = { 
                .color = RGB(20, 20, 20), 
                .flags = M_PHONG | M_REFLECTION
            }, 
            .geometry.sphere = &SPHERE(-1.1, y, -1.2, 0.7)
        },
        {
            .type = GEOMETRY_SPHERE, 
            .material = { 
                .color = RGB(20, 20, 20), 
                .flags = M_PHONG | M_REFLECTION 
            }, 
            .geometry.sphere = &SPHERE(1.3, y, -1.3, 0.8)
        },
        {
            .type = GEOMETRY_SPHERE, 
            .material = { 
                .color = RGB(20, 20, 20), 
                .flags = M_PHONG | M_REFLECTION | M_REFRACTION 
            }, 
            .geometry.sphere = &SPHERE(0.0, y, 0.0, 0.9)
        },
        {
            .type = GEOMETRY_SPHERE, 
            .material = { 
                .color = RGB(20, 20, 20), 
                .flags = M_PHONG | M_REFLECTION 
            }, 
            .geometry.sphere = &SPHERE(-1.1, y, 1.0, 0.5)
        },
        */
        {
            .type = GEOMETRY_SPHERE, 
            .material = { 
                .color = BLUE, 
                .flags = M_PHONG | M_GLOBAL
            }, 
            .geometry.sphere = &SPHERE(1.1, y, 1.0, 0.4)
        },
        {
            .type = GEOMETRY_SPHERE, 
            .material = { 
                .color = RED, 
                .flags = M_PHONG | M_LIGHT
            }, 
            .geometry.sphere = &SPHERE(0.3, y, 1.2, 0.3)
        },

    };

    size_t buff_len = sizeof(*framebuffer) * options.width * options.height * 3;
    framebuffer = malloc(buff_len);
    if (framebuffer == NULL)
    {
        fprintf(stderr, "could not allocate framebuffer\n");
        exit(EXIT_FAILURE);
    }

    memset(framebuffer, 0x0,buff_len);
    signal(SIGINT, &write_image);

    camera_t camera;
    init_camera(&camera, (vec3){0.0, 1, 3.25}, (vec3){0, 1, 0}, &options);
    //init_camera(&camera, (vec3){0.0001, 5, 0.0}, (vec3){0, 0, 0}, &options);

    clock_t tic = clock();

    render(framebuffer, scene, sizeof(scene) / sizeof(scene[0]), &camera, &options);

    clock_t toc = clock();
    double time_taken = (double)(toc - tic) / CLOCKS_PER_SEC;

    printf("%d x %d pixels\n", options.width, options.height);
    printf("cast %d rays\n", ray_count);
    printf("checked %d possible intersections\n", intersection_test_count);
    printf("rendering took %f seconds\n", time_taken);
    printf("writing result to '%s'...\n", options.result);
    write_image(0);
    return EXIT_SUCCESS;
}
