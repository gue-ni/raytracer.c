#ifndef VECTOR_M
#define VECTOR_M

#include <tgmath.h>
#include <assert.h>

typedef struct { double x, y; }       vec2;
typedef struct { double x, y, z; }    vec3;
typedef struct { double x, y, z, w; } vec4;
typedef double mat2[2*2];
typedef double mat3[3*3];
typedef double mat4[4*4];

inline vec3 vec3_mult(vec3 a, vec3 b)
{ return (vec3){a.x * b.x, a.y * b.y, a.z * b.z}; }

inline vec3 vec3_sub(vec3 a, vec3 b)
{ return (vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }

inline vec3 vec3_add(vec3 a, vec3 b)
{ return (vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }

inline double vec3_dot(vec3 a, vec3 b) 
{ return a.x * b.x + a.y * b.y + a.z * b.z; }

inline double vec3_length(vec3 v) 
{ return sqrt(vec3_dot(v, v)); }

inline vec3 vec3_scalar_mult(vec3 v, double s) 
{ return (vec3){v.x * s, v.y * s, v.z * s}; }

inline vec3 vec3_scalar_div(vec3 v, double s) 
{ return vec3_scalar_mult(v, 1.0 / s); }

inline vec2 vec2_scalar_mult(vec2 v, double s) 
{ return (vec2){v.x * s, v.y * s}; }

inline vec2 vec2_add(vec2 a, vec2 b)  
{  return (vec2){a.x + b.x, a.y + b.y}; }

inline vec3 vec3_cross(vec3 a, vec3 b)
{ return (vec3){
  a.y * b.z - a.z * b.y, \
  a.z * b.x - a.x * b.z, \
  a.x * b.y - a.y * b.x, }; 
}

inline int vec3_equal(vec3 a, vec3 b)
{ return a.x == b.x && a.y == b.y && a.z == b.z; }

inline vec3 vec3_normalize(vec3 v)
{
  double m = vec3_length(v);
  assert(m > 0);
  return vec3_scalar_mult(v, 1.0 / m);
}

#define MAT4_D (4)
#define MAT4_P (1) 

inline vec3 mat4_vector_mult(mat4 A, vec3 v)
{
  unsigned i, j, k;
  double B[MAT4_D] = {v.x, v.y, v.z, 1.0}, C[MAT4_D];

  for (i = 0; i < MAT4_D; i++)
  {
    for (j = 0; j < MAT4_P; j++)
    {
      for (k = 0, C[i * MAT4_P + j] = 0; k < MAT4_D; k++)
      {
        C[i * MAT4_P + j] += (A[i * MAT4_D + k] * B[k * MAT4_P + j]);
      }
    }
  }
  return (vec3){C[0], C[1], C[2]};
}

inline void mat4_mult(mat4 A, mat4 B, mat4 C)
{
  // A = N x M (N rows and M columns)
  // B = M x P
  unsigned i, j, k;

  for (i = 0; i < MAT4_D; i++)
  {
    for (j = 0; j < MAT4_D; j++)
    {
      for (k = 0, C[i * MAT4_D + j] = 0; k < MAT4_D; k++)
      {
        C[i * MAT4_P + j] += (A[i * MAT4_D + k] * B[k * MAT4_P + j]);
      }
    }
  }
}

#endif /* VECTOR_M */
