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
#include "vector.h"

#define SPHERE(x, y, z, r) \
    .center = { (x), (y) + (r), (z) },\
    .radius = (r),\

#define N_SPHERES (25)

Options options = {
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

bool collision(vec3 center0, double radius0, vec3 center1, double radius1)
{
    return vec3_length(vec3_sub(center0, center1)) < (radius0 + radius1);
}

void test_collision()
{


    Object o_1 = {.radius = 3, .center = {0,0,0}};
    Object o_2 = {.radius = 3, .center = {6,0,0}};

    printf("collision = %d\n", collision(o_1.center, o_1.radius, o_2.center, o_2.radius));
}

void generate_random_spheres(Object *spheres, int num_spheres, vec3 box_min, vec3 box_max)
{
    const int max_iterations = 100000000;

    const double min_radius = 2.0, max_radius = 8.0;

    int iterations = 0;
    int spheres_found = 0;
    const double padding = 0.5;

    while(spheres_found < num_spheres)
    {
        assert(iterations++ < max_iterations);

        double radius = random_range(min_radius, max_radius);
        vec3 vr = { radius, radius, radius };

        vec3 min = vec3_add(box_min, vr);
        vec3 max = vec3_sub(box_max, vr);

        vec3 center = {
            random_range(min.x, max.x),
            random_range(min.y, max.y),
            random_range(min.z, max.z)
        };

        bool coll = false;
        for (int i = 0; i < spheres_found; i++)
        {
            Object *sphere = &spheres[i];
            coll = collision(sphere->center, sphere->radius, center, radius);
            if (coll)
                break;
        }

        if (!coll)
        {
            spheres_found++;

            uint flags = M_DEFAULT;
            vec3 color = WHITE;
            vec3 emission = BLACK;

            double r = random_double();
            if (r < 0.5)
            {
                emission = RANDOM_COLOR;
            }
            else 
            {
                if (r > 0.8)
                    flags = M_REFRACTION;
                else if(r > 0.6)
                    flags = M_REFLECTION;
            }

            if (vec3_length(emission) > 0)
            {
                //printf("[%d] = { .center = { %f, %f, %f }, .radius = %f, .emission = { %f, %f, %f } },\n", spheres_found, center.x, center.y, center.z, radius, emission.x, emission.y, emission.z);
                printf("[%d] = { .center = { %f, %f, %f }, .radius = %f },\n", spheres_found, center.x, center.y, center.z, radius);
            }

            //center = (vec3){-8.053048, 13.375004, -5.639876};

            *(spheres++) = (Object) {
                .center = center,
                .radius = radius,
                .flags = flags,
                .color = color,
                .emission = emission
            };
        }
    }
}

void apply_matrix(TriangleMesh* mesh, mat4 matrix)
{
    #pragma omp parallel for
    for (uint i = 0; i < mesh->num_triangles * 3; i++)
    {
        mesh->vertices[i].pos = mat4_vector_mult(matrix, mesh->vertices[i].pos);
    }
}

void parse_options(int argc, char **argv, Options *options)
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
    unsigned int seed;
#if 0
    seed = (unsigned)time(NULL);
#else
    seed = 1666943821;
#endif

    printf("seed = %d\n", seed);
    srand(seed);


    if (argc <= 1)
    {
        fprintf(stderr, "Usage: %s -w <width> -h <height> -s <samples per pixel> -o <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    parse_options(argc, argv, &options);

    vec3 pos = {0, 0, 0};
    vec3 size = {1, 1, 1.5};

    const TriangleMesh cube = {
        .num_triangles = 2,
        .vertices = (Vertex[]){
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

    const double aspect_ratio = (double)options.width / (double)options.height;
    const double room_depth = 30;
    const double room_height = 20;
    const double room_width = room_height * aspect_ratio;
    const double radius = 10000;
    const  vec3 wall_color = VECTOR(0.75, 0.75, 0.75);
    const double light_radius = 15;
    const double y = -room_height;

    uint lighting = M_DEFAULT;

#if 1
    Object scene[] = {
#if 1 /* walls */
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
            .center = {0, 0, -radius - room_depth}, 
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
#endif
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
#if 0 /* objects */
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
#if 1 /* packed spheres */
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 0, 0, 0 }, .center = { 11.8823, 12.8165, -3.43022 }, .radius = 3.47138 },
{ .color = { 1, 1, 1 }, .flags = M_REFLECTION, .emission = { 0, 0, 0 }, .center = { -4.78617, -10.565, -11.8307 }, .radius = 7.8185 },
{ .color = { 1, 1, 1 }, .flags = M_REFLECTION, .emission = { 0, 0, 0 }, .center = { 16.3283, 15.7456, 8.02745 }, .radius = 3.38449 },
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 0.129721, 1.08691, 0.15077 }, .center = { -7.74563, 7.10781, -1.14851 }, .radius = 5.68239 },
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 0, 0, 0 }, .center = { 0.604958, 13.8198, -10.0857 }, .radius = 3.63955 },
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 0.419482, 0.406897, 0.301653 }, .center = { 2.72773, -3.47742, 7.21287 }, .radius = 5.756 },
{ .color = { 1, 1, 1 }, .flags = M_REFLECTION, .emission = { 0, 0, 0 }, .center = { -11.6808, -15.0112, 10.6413 }, .radius = 3.40004 },
{ .color = { 1, 1, 1 }, .flags = M_REFLECTION, .emission = { 0, 0, 0 }, .center = { 5.28438, -2.58167, -3.87996 }, .radius = 2.20867 },
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 0, 0, 0 }, .center = { -15.1722, -0.318264, -14.8739 }, .radius = 3.31716 },
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 0, 0, 0 }, .center = { 7.05345, -11.9375, -4.08415 }, .radius = 5.01176 },
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 2.02456, 1.14375, 0.22395 }, .center = { -6.64606, 12.5952, -11.8074 }, .radius = 3.57727 },
{ .color = { 1, 1, 1 }, .flags = M_REFLECTION, .emission = { 0, 0, 0 }, .center = { 15.3284, 7.63569, -7.88126 }, .radius = 2.26494 },
{ .color = { 1, 1, 1 }, .flags = M_REFLECTION, .emission = { 0, 0, 0 }, .center = { 5.15508, -13.4632, 12.9555 }, .radius = 4.41505 },
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 0, 0, 0 }, .center = { 6.61409, 15.9581, 13.6585 }, .radius = 2.76828 },
{ .color = { 1, 1, 1 }, .flags = M_REFLECTION, .emission = { 0, 0, 0 }, .center = { 0.00113487, 8.35296, -14.4917 }, .radius = 2.58514 },
{ .color = { 1, 1, 1 }, .flags = M_REFLECTION, .emission = { 0, 0, 0 }, .center = { 9.63578, 9.63074, -16.0336 }, .radius = 2.22603 },
{ .color = { 1, 1, 1 }, .flags = M_REFLECTION, .emission = { 0, 0, 0 }, .center = { 13.105, 1.55555, 2.67293 }, .radius = 4.00109 },
{ .color = { 1, 1, 1 }, .flags = M_REFLECTION, .emission = { 0, 0, 0 }, .center = { -0.0637789, 6.39925, 11.777 }, .radius = 4.99425 },
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 0.403171, 1.90743, 1.59559 }, .center = { 7.11587, 6.96992, 7.24724 }, .radius = 3.28273 },
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 0, 0, 0 }, .center = { -17.0139, 4.27765, 11.924 }, .radius = 2.14903 },
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 0.647167, 1.99216, 1.4463 }, .center = { 15.3924, -4.96949, 12.4327 }, .radius = 3.48512 },
{ .color = { 1, 1, 1 }, .flags = M_REFLECTION, .emission = { 0, 0, 0 }, .center = { -16.0135, 15.9701, 12.4844 }, .radius = 3.00053 },
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 3.16375, 4.44267, 3.49332 }, .center = { -2.87246, -15.5185, 7.78116 }, .radius = 3.4779 },
{ .color = { 1, 1, 1 }, .flags = M_REFLECTION, .emission = { 0, 0, 0 }, .center = { -8.89639, -10.9745, -1.80553 }, .radius = 2.39033 },
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 0.662701, 2.82942, 1.50879 }, .center = { -0.653194, 9.99867, 4.17957 }, .radius = 3.28669 },
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 1.6413, 2.60242, 0.421142 }, .center = { -14.6767, -6.47449, 4.48493 }, .radius = 4.77854 },
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 0.479186, 0.149559, 0.3761 }, .center = { -9.76604, 16.8809, -0.605894 }, .radius = 2.89667 },
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 0, 0, 0 }, .center = { 4.07601, 5.6942, -3.07305 }, .radius = 4.91388 },
{ .color = { 1, 1, 1 }, .flags = M_REFLECTION, .emission = { 0, 0, 0 }, .center = { 15.1469, -13.988, 9.5646 }, .radius = 4.6719 },
{ .color = { 1, 1, 1 }, .flags = M_DEFAULT, .emission = { 0, 0, 0 }, .center = { -7.2047, -5.0758, 7.74727 }, .radius = 2.86742 },

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
#else
    Object scene[N_SPHERES];
#endif


#if 0
    generate_random_spheres
    (
        &scene[6], 
        N_SPHERES, 
        VECTOR(-room_width, -room_height, -room_depth), 
        VECTOR(room_width, room_height, room_depth)
    );
#endif

    size_t buff_len = sizeof(*framebuffer) * options.width * options.height * 3;
    framebuffer = malloc(buff_len);
    if (framebuffer == NULL)
    {
        fprintf(stderr, "could not allocate framebuffer\n");
        exit(EXIT_FAILURE);
    }

    memset(framebuffer, 0x0,buff_len);
    signal(SIGINT, &write_image);

    Camera camera;
    init_camera(&camera, VECTOR(0.0, 0, 50), VECTOR(0, 0, 0), &options);

    clock_t tic = clock();

    render(framebuffer, scene, sizeof(scene) / sizeof(scene[0]), &camera, &options);

    clock_t toc = clock();

    double time_taken = (double)((toc - tic) / CLOCKS_PER_SEC);

    printf("%d x %d (%d) pixels\n", options.width, options.height, options.width * options.height);
    printf("cast %lld rays\n", ray_count);
    printf("checked %lld possible intersections\n", intersection_test_count);
    printf("rendering took %f seconds\n", time_taken);
    printf("writing result to '%s'...\n", options.result);

#ifndef VALGRIND
    write_image(0);
#endif
    return EXIT_SUCCESS;
}
