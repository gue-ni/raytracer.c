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

inline vec3 mult(const vec3 v1, const vec3 v2){ return (vec3){v1.x * v2.x, v1.y * v2.y, v1.z * v2.z}; }
inline vec3 sub (const vec3 v1, const vec3 v2){ return (vec3){v1.x - v2.x, v1.y - v2.y, v1.z - v2.z}; }
inline vec3 add (const vec3 v1, const vec3 v2){ return (vec3){v1.x + v2.x, v1.y + v2.y, v1.z + v2.z}; }
inline vec3 div (const vec3 v1, const vec3 v2){ return (vec3){v1.x / v2.x, v1.y / v2.y, v1.z / v2.z}; }

inline double dot(const vec3 v1, const vec3 v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }
inline double length(const vec3 v) { return sqrt(dot(v, v)); }

inline vec3 mult_s(const vec3 v, const double s) { return (vec3){v.x * s, v.y * s, v.z * s}; }
inline vec3 div_s (const vec3 v, const double s)  { return mult_s(v, 1/s); }

inline vec2 mult_s2(const vec2 v, const double s) { return (vec2){v.x * s, v.y * s}; }
inline vec2 add_s2 (const vec2 v1, const vec2 v2)  { return (vec2){v1.x + v2.x, v1.y + v2.y}; }

inline vec3 cross(const vec3 a, const vec3 b)
{ return (vec3){
  a.y * b.z - a.z * b.y, 
  a.z * b.x - a.x * b.z, 
  a.x * b.y - a.y * b.x }; 
}

inline vec3 normalize(const vec3 v)
{
  double m = length(v);
  assert(m > 0);
  return mult_s(v, 1 / m);
}

#endif /* VECTOR_M */