#ifndef VECTOR_M
#define VECTOR_M

#include <tgmath.h>
#include <assert.h>

typedef struct { double x, y; }       vec2;
typedef struct { double x, y, z; }    vec3;
typedef struct { double x, y, z, w; } vec4;
typedef struct { double m[2*2]; }     mat2;
typedef struct { double m[3*3]; }     mat3;
typedef struct { double m[4*4]; }     mat4;

inline vec3 mult(vec3 a, vec3 b)
{ return (vec3){a.x * b.x, a.y * b.y, a.z * b.z}; }

inline vec3 sub(vec3 a, vec3 b)
{ return (vec3){a.x - b.x, a.y - b.y, a.z - b.z}; }

inline vec3 add(vec3 a, vec3 b)
{ return (vec3){a.x + b.x, a.y + b.y, a.z + b.z}; }

inline double dot(vec3 a, vec3 b) { return a.x * b.x + a.y * b.y + a.z * b.z; }
inline double length(vec3 v) { return sqrt(dot(v, v)); }

inline vec3 mult_s(vec3 v, double s) { return (vec3){v.x * s, v.y * s, v.z * s}; }
inline vec3 div_s (vec3 v, double s) { return mult_s(v, 1/s); }

inline vec2 mult_s2(vec2 v, double s) { return (vec2){v.x * s, v.y * s}; }
inline vec2 add_s2 (vec2 a, vec2 b)  {  return (vec2){a.x + b.x, a.y + b.y}; }

inline vec3 cross(vec3 a, vec3 b)
{ return (vec3){
  a.y * b.z - a.z * b.y, \
  a.z * b.x - a.x * b.z, \
  a.x * b.y - a.y * b.x, }; 
}

inline vec3 normalize(vec3 v)
{
  double m = length(v);
  assert(m > 0);
  return mult_s(v, 1 / m);
}

#endif /* VECTOR_M */
