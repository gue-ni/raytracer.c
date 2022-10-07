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

options_t options = {
    .width = 320,
    .height = 180,
    .samples = 50,
    .result = "result.png",
    .obj = "assets/cube.obj"};

uint8_t *framebuffer = NULL;

extern int ray_count;
extern int intersection_test_count;

void write_image(int signal)
{
    printf("on exit\n");
    if (framebuffer != NULL)
    {
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
    }
}

void apply_matrix(mesh_t* mesh, mat4 matrix)
{
    #pragma omp parallel for
    for (int i = 0; i < mesh->num_triangles * 3; i++)
    {
        mesh->vertices[i].pos = mult_mv(matrix, mesh->vertices[i].pos);
    }
}

int main(int argc, char **argv)
{
    srand((unsigned)time(NULL));

    if (argc <= 1)
    {
        fprintf(stderr, "Usage: %s -w <width> -h <height> -s <samples per pixel> -o <filename>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

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

    vec3 pos = {0, 0, 0};
    vec3 size = {1, 1, 1.5};

    mesh_t mesh_2 = {
        .num_triangles = 2,
        .vertices = (vertex_t[]){
            {{ -0.5f,  0.5f, -0.5f,},{    0.0f, 1.0f,  }},
            {{ 0.5f,  0.5f, -0.5f, },{   1.0f, 1.0f,   }},
            {{ 0.5f,  0.5f,  0.5f, },{   1.0f, 0.0f,   }},

            {{ 0.5f,  0.5f,  0.5f, },{   1.0f, 0.0f,   }},
            {{ -0.5f,  0.5f,  0.5f,},{    0.0f, 0.0f,  }},
            {{ -0.5f,  0.5f, -0.5f,},{    0.0f, 1.0f   }},


            {{ -0.5f, -0.5f, -0.5f,},{    0.0f, 0.0f,  }},
            {{ 0.5f, -0.5f, -0.5f, },{   1.0f, 0.0f,   }},
            {{ 0.5f,  0.5f, -0.5f, },{   1.0f, 1.0f,   }},
            {{ 0.5f,  0.5f, -0.5f, },{   1.0f, 1.0f,   }},
            {{ -0.5f,  0.5f, -0.5f,},{    0.0f, 1.0f,  }},
            {{ -0.5f, -0.5f, -0.5f,},{    0.0f, 0.0f,  }},
            {{ -0.5f, -0.5f,  0.5f,},{    0.0f, 0.0f,  }},
            {{ 0.5f, -0.5f,  0.5f, },{   1.0f, 0.0f,   }},
            {{ 0.5f,  0.5f,  0.5f, },{   1.0f, 1.0f,   }},
            {{ 0.5f,  0.5f,  0.5f, },{   1.0f, 1.0f,   }},
            {{ -0.5f,  0.5f,  0.5f,},{    0.0f, 1.0f,  }},
            {{ -0.5f, -0.5f,  0.5f,},{    0.0f, 0.0f,  }},
            {{ -0.5f,  0.5f,  0.5f,},{    1.0f, 0.0f,  }},
            {{ -0.5f,  0.5f, -0.5f,},{    1.0f, 1.0f,  }},
            {{ -0.5f, -0.5f, -0.5f,},{    0.0f, 1.0f,  }},
            {{ -0.5f, -0.5f, -0.5f,},{    0.0f, 1.0f,  }},
            {{ -0.5f, -0.5f,  0.5f,},{    0.0f, 0.0f,  }},
            {{ -0.5f,  0.5f,  0.5f,},{    1.0f, 0.0f,  }},
            {{ 0.5f,  0.5f,  0.5f, },{   1.0f, 0.0f,   }},
            {{ 0.5f,  0.5f, -0.5f, },{   1.0f, 1.0f,   }},
            {{ 0.5f, -0.5f, -0.5f, },{   0.0f, 1.0f,   }},
            {{ 0.5f, -0.5f, -0.5f, },{   0.0f, 1.0f,   }},
            {{ 0.5f, -0.5f,  0.5f, },{   0.0f, 0.0f,   }},
            {{ 0.5f,  0.5f,  0.5f, },{   1.0f, 0.0f,   }},
            {{ -0.5f, -0.5f, -0.5f,},{    0.0f, 1.0f,  }},
            {{ 0.5f, -0.5f, -0.5f, },{   1.0f, 1.0f,   }},
            {{ 0.5f, -0.5f,  0.5f, },{   1.0f, 0.0f,   }},
            {{ 0.5f, -0.5f,  0.5f, },{   1.0f, 0.0f,   }},
            {{ -0.5f, -0.5f,  0.5f,},{    0.0f, 0.0f,  }},
            {{ -0.5f, -0.5f, -0.5f,},{    0.0f, 1.0f,  }},
        }
    };

    mat4 t = translate((vec3){0,-1,0});
    mat4 s = scale((vec3){8,.5,8});
    //mat4 transform = mult_mm(s, t);
    apply_matrix(&mesh_2, s);

    mesh_t mesh = {
        .num_triangles = 12,
        .vertices = (vertex_t[]){
            /* top */
            {{pos.x - size.x, pos.y + size.y, pos.z + size.z}, /* 5 top front left */       {1,1}},
            {{pos.x + size.x, pos.y + size.y, pos.z - size.z}, /* 4 top back right */       {0,0}},
            {{pos.x - size.x, pos.y + size.y, pos.z - size.z}, /* 6 top back left */        {1,0}},

            {{pos.x - size.x, pos.y + size.y, pos.z + size.z}, /* 5 top front left */       {1,1}},
            {{pos.x + size.x, pos.y + size.y, pos.z - size.z}, /* 4 top back right */       {0,0}},
            {{pos.x + size.x, pos.y + size.y, pos.z + size.z}, /* 8 top front right */      {0,1}},

            /* bottom */
            {{pos.x + size.x, pos.y - size.y, pos.z + size.z}, /* 3 bottom front right */   {0,1}},
            {{pos.x - size.x, pos.y - size.y, pos.z + size.z}, /* 1 bottom front left */    {1,1}},
            {{pos.x + size.x, pos.y - size.y, pos.z - size.z}, /* 0 bottom back right */    {0,0}},

            {{pos.x - size.x, pos.y - size.y, pos.z - size.z}, /* 2 bottom back right */     {1,0}},
            {{pos.x + size.x, pos.y - size.y, pos.z - size.z}, /* 0 bottom back left */    {0,0}},
            {{pos.x - size.x, pos.y - size.y, pos.z + size.z}, /* 1 bottom front left */    {1,1}},

            /* back, normal works */
            {{pos.x + size.x, pos.y - size.y, pos.z - size.z}, /* 0 bottom back left m */    {1,0}},
            {{pos.x - size.x, pos.y - size.y, pos.z - size.z}, /* 2 bottom back right */     {0,0}},
            {{pos.x + size.x, pos.y + size.y, pos.z - size.z}, /* 4 top back right */       {1,1}},

            {{pos.x - size.x, pos.y - size.y, pos.z - size.z}, /* 2 bottom back right */     {0,0}},
            {{pos.x - size.x, pos.y + size.y, pos.z - size.z}, /* 6 top back left */        {0,1}},
            {{pos.x + size.x, pos.y + size.y, pos.z - size.z}, /* 4 top back right */       {1,1}},

            /* front */
            {{pos.x + size.x, pos.y - size.y, pos.z + size.z}, /* 0 bottom back left m */    {1,0}},
            {{pos.x - size.x, pos.y - size.y, pos.z + size.z}, /* 2 bottom back right */     {0,0}},
            {{pos.x + size.x, pos.y + size.y, pos.z + size.z}, /* 4 top back right */       {1,1}},

            {{pos.x - size.x, pos.y - size.y, pos.z + size.z}, /* 2 bottom back right */     {0,0}},
            {{pos.x - size.x, pos.y + size.y, pos.z + size.z}, /* 6 top back left */        {0,1}},
            {{pos.x + size.x, pos.y + size.y, pos.z + size.z}, /* 4 top back right */       {1,1}},


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
        },
    };

   object_t scene[] = {
        {
            .type = GEOMETRY_MESH, 
            .material = { .color = RGB(100,100,100), .type = CHECKERED}, 
            .geometry.mesh = &mesh_2
        },
        {
            .type = GEOMETRY_SPHERE, 
            .material = { .color = RANDOM_COLOR, .type = WIKIPEDIA_ALGORITHM  }, 
            .geometry.sphere = &(sphere_t){ {1.1, 1, -1}, 0.5 }
        },
        {
            .type = GEOMETRY_SPHERE, 
            .material = { .color = RED, .type = WIKIPEDIA_ALGORITHM}, 
            .geometry.sphere = &(sphere_t){ {0, 1, 0}, 0.75}
        },
        {
            .type = GEOMETRY_SPHERE, 
            .material = { .color = GREEN, .type = WIKIPEDIA_ALGORITHM}, 
            .geometry.sphere = &(sphere_t){{-1.1, 1, 1}, 0.5}
        },
    };

    size_t buff_len = sizeof(*framebuffer) * options.width * options.height * 3;
    framebuffer = malloc(buff_len);
    if (framebuffer == NULL)
    {
        fprintf(stderr, "could not allocate framebuffer\n");
        exit(1);
    }

    memset(framebuffer, 0x0,buff_len);
    signal(SIGINT, &write_image);

    camera_t camera;
    init_camera(&camera, (vec3){0.0, 1, 3.5}, (vec3){0, 1, 0}, &options);

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
    return 0;
}
