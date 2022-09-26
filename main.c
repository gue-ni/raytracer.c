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

#define TINYOBJ_LOADER_C_IMPLEMENTATION
#include "lib/tinyobj_loader.h"

#include "raytracer.h"

extern int ray_count;
extern int intersection_test_count;

void exit_error(const char *message)
{
    fprintf(stderr, "[ERROR] (%s:%d) %s\n", __FILE__, __LINE__, message);
    exit(EXIT_FAILURE);
}

void load_file(void *ctx, const char *filename, const int is_mtl, const char *obj, char **buffer, size_t *len)
{
  long string_size = 0, read_size = 0;
  FILE *handler = fopen(filename, "r");

  if (handler)
  {
    fseek(handler, 0, SEEK_END);
    string_size = ftell(handler);
    rewind(handler);
    *buffer = (char *)malloc(sizeof(char) * (string_size + 1));
    read_size = fread(*buffer, sizeof(char), (size_t)string_size, handler);
    (*buffer)[string_size] = '\0';
    if (string_size != read_size)
    {
      free(buffer);
      *buffer = NULL;
    }
    fclose(handler);
  }
  *len = read_size;
}

bool load_obj(const char *filename, mesh_t *mesh)
{
  tinyobj_attrib_t attrib;
  tinyobj_shape_t *shape = NULL;
  tinyobj_material_t *material = NULL;

  size_t num_shapes;
  size_t num_materials;

  tinyobj_attrib_init(&attrib);

  int result = tinyobj_parse_obj(
      &attrib,
      &shape,
      &num_shapes,
      &material,
      &num_materials,
      filename,
      load_file,
      NULL,
      TINYOBJ_FLAG_TRIANGULATE);

  assert(result == TINYOBJ_SUCCESS);
  assert(attrib.num_faces == 36);
  assert(attrib.num_normals == 6);  // one per side
  assert(attrib.num_vertices == 8); // one per corner
  assert(attrib.num_texcoords == 0);
  assert(attrib.num_face_num_verts == 12); // two triangles per side

  assert(mesh != NULL);

  mesh->num_triangles = attrib.num_face_num_verts;
  mesh->triangles = malloc(mesh->num_triangles * sizeof(*mesh->triangles));

  assert(mesh->triangles != NULL);

  size_t face_offset = 0;
  // iterate over all faces
  for (size_t i = 0; i < attrib.num_face_num_verts; i++)
  {
    const int face_num_verts = attrib.face_num_verts[i]; // num verts per face

    assert(face_num_verts == 3);     // assert all faces are triangles
    assert(face_num_verts % 3 == 0); // assert all faces are triangles
    // printf("[%d] face_num_verts = %d\n", i, face_num_verts);

    tinyobj_vertex_index_t idx0 = attrib.faces[face_offset + 0];
    tinyobj_vertex_index_t idx1 = attrib.faces[face_offset + 1];
    tinyobj_vertex_index_t idx2 = attrib.faces[face_offset + 2];

    triangle_t triangle;

    for (int k = 0; k < 3; k++)
    {
      int f0 = idx0.v_idx;
      int f1 = idx1.v_idx;
      int f2 = idx2.v_idx;

      assert(f0 >= 0);
      assert(f1 >= 0);
      assert(f2 >= 0);

      vec3 v = {
          attrib.vertices[3 * (size_t)f0 + k],
          attrib.vertices[3 * (size_t)f1 + k],
          attrib.vertices[3 * (size_t)f2 + k],
      };

      triangle.v[k] = v;
    }

    mesh->triangles[i] = triangle;

    face_offset += (size_t)face_num_verts;
  }

  if (shape)
    tinyobj_shapes_free(shape, num_shapes);

  tinyobj_attrib_free(&attrib);
  return true;
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

  vec3 pos = {0, 2, -3};
  vec3 size = {4, 3, 8};
  // double w = 3;

  triangle_t triangles[] = {
      {{
          {pos.x + size.x, pos.y - size.y, pos.z - size.z}, // lower left
          {pos.x - size.x, pos.y - size.y, pos.z + size.z}, // top right
          {pos.x - size.x, pos.y - size.y, pos.z - size.z}, // lower right
      }},
      {{
          {pos.x + size.x, pos.y - size.y, pos.z - size.z}, // lower left
          {pos.x - size.x, pos.y - size.y, pos.z + size.z}, // top right
          {pos.x + size.x, pos.y - size.y, pos.z + size.z}, // top left
      }},
  };

  mesh_t mesh = {2, triangles};

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

  object_t scene[] = {
      {.type = SPHERE, .material = {RANDOM_COLOR, REFLECTION}, .geometry.sphere = &spheres[1]},
      {.type = SPHERE, .material = {RED, PHONG}, .geometry.sphere = &spheres[2]},
      {.type = SPHERE, .material = {RANDOM_COLOR, REFLECTION_AND_REFRACTION}, .geometry.sphere = &spheres[3]},
      {.type = SPHERE, .material = {BLUE, PHONG}, .geometry.sphere = &spheres[4]},
      {.type = MESH, .material = {GREEN, PHONG}, .geometry.mesh = &mesh},
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
