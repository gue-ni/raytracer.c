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

#define SPHERE(x, y, z, r) \
    .center = { (x), (y) + (r), (z) },\
    .radius = (r),\

options_t options = {
    .width = 320,
    .height = 180,
    .samples = 50,
    .result = "result.png",
    .obj = "assets/cube.obj",
};

uint8_t *framebuffer = NULL;

extern long long ray_count;
extern long long intersection_test_count;

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

    const mesh_t cube = {
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

    //mat4 t = translate((vec3){0,-1,0});
    //mat4 s = scale(VECTOR(16, 0.5, 16));
    //apply_matrix(&cube, s);

    double aspect_ratio = (double)options.width / (double)options.height;
    const double room_depth = 30;
    const double room_height = 20;
    const double room_width = room_height * aspect_ratio;
    const double radius = 10000;
    const  vec3 wall_color = VECTOR(0.75, 0.75, 0.75);
    const double light_radius = 15;
    const double y = -room_height;

    uint lighting = M_DEFAULT;

    object_t scene[] = {
        { // floor
            .color = wall_color, 
            .emission = BLACK,
            .flags = lighting,
            .center =  {0, -radius - room_height, 0},
            .radius = radius,
        },
        { // back wall
            .color = wall_color, 
            .emission = BLACK,
            .flags = lighting,
            .center ={0, 0, -radius - room_depth}, 
            .radius = radius,
        },
        { // left wall
            .color = VECTOR(0.25, 0.75, 0.25), 
            .emission = BLACK,
            .flags = lighting,
            .center =  {-radius - room_width, 0, 0}, 
            .radius = radius,
        },
        { // right wall
            .color = VECTOR(0.75, 0.25, 0.25), 
            .emission = BLACK,
            .flags = lighting,
            .center =  {radius + room_width, 0, 0}, 
            .radius = radius,
        },
        { // ceiling
            .color = wall_color, 
            .emission = BLACK,
            .flags = lighting,
            .center = {0, radius + room_height, 0}, 
            .radius = radius,
        },
        { // front wall
            .color = wall_color, 
            .emission = BLACK,
            .flags = lighting,
            .center = {0, 0, radius + room_depth * 2}, 
            .radius = radius,
        },
#if 0 /* cube */
        {
            .type = GEOMETRY_MESH, 
            .material = { 
                .color = RGB(109,124,187), 
                .flags = lighting | M_CHECKERED
            }, 
            .geometry.mesh = &cube
        },
#endif
#if 1 /* objects */
        {
            .color = WHITE, 
            .emission = BLACK,
            .flags = M_DEFAULT,
            SPHERE(-11, y, -12, 7)
        },
        {
            .color = WHITE, 
            .emission = BLACK,
            .flags =  M_REFLECTION, 
            SPHERE(13, y, -13, 8)
        },
        {
            .color = WHITE, 
            .emission = BLACK,
            .flags = M_REFRACTION,
            SPHERE(0, y, 0, 9)
        },
        {
            .color = WHITE, 
            .flags = M_REFLECTION,
            .emission = BLACK,
            SPHERE(-11, y, 10, 5)
        },
        {
            .color = WHITE, 
            .flags = M_DEFAULT,
            .emission = BLACK,
            SPHERE(11, room_height / 4, 10, 6)
        },
        {
            .color = WHITE, 
            .flags = M_REFLECTION,
            .emission = BLACK,
            SPHERE(-5, 5, -5, 5)
        },
#endif
#if 1 /* light */
        {
            .color = WHITE, 
            .flags = M_DEFAULT,
            .emission = RGB(0x00 * 15, 0x32 * 15, 0xA0 * 15),
            .center = {0, room_height + light_radius * 0.9, 0}, 
            .radius = light_radius,
        },
        {
            .color = WHITE, 
            .flags = M_DEFAULT,
            .emission = RGB(0xD0, 0x00, 0x70),
            SPHERE(2, y, 12, 3)
        },
#endif
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
    init_camera(&camera, VECTOR(0.0, 0, 50), VECTOR(0, 0, 0), &options);

    clock_t tic = clock();

    render(framebuffer, scene, sizeof(scene) / sizeof(scene[0]), &camera, &options);

    clock_t toc = clock();

    double time_taken = (double)((toc - tic) / CLOCKS_PER_SEC);

    printf("%d x %d pixels\n", options.width, options.height);
    printf("cast %lld rays\n", ray_count);
    printf("checked %lld possible intersections\n", intersection_test_count);
    printf("rendering took %f seconds\n", time_taken);
    printf("writing result to '%s'...\n", options.result);
    write_image(0);
    return EXIT_SUCCESS;
}
