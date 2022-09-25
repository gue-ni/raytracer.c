#ifndef RAYTRACER_H
#define RAYTRACER_H

#define EPSILON 1e-8
#define MAX_DEPTH 5
#define MAX(a, b) ((a) > (b) ? (a) : (b))
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#define CLAMP(x) (MAX(0, MIN(x, 1)))
#define CLAMP_BETWEEN(x, min_v, max_v) (MAX(min_v, MIN(max_v, 1)))
#define ABS(x) ((x < 0) ? (-x) : (x))
#define RGB(r, g, b) ((vec3){r / 255.0, g / 255.0, b / 255.0})

#define RED RGB(187, 52, 155)
#define BLUE RGB(122, 94, 147)
#define GREEN RGB(108, 69, 117)
#define ZERO_VECTOR RGB(0, 0, 0)
#define BACKGROUND_COLOR RGB(69, 185, 211)
#define BLACK ZERO_VECTOR

#endif /* RAYTRACER_H */