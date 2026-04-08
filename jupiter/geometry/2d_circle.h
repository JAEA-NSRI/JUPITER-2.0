#ifndef JUPITER_GEOMETRY_2D_CIRCLE_H
#define JUPITER_GEOMETRY_2D_CIRCLE_H

#include "defs.h"
#include "vector.h"

JUPITER_GEOMETRY_DECL_START

struct geom_2d_circle
{
  geom_vec2 center;
  double radius;
};
typedef struct geom_2d_circle geom_2d_circle;

static inline geom_2d_circle geom_2d_circle_c(geom_vec2 center, double radius)
{
  geom_2d_circle c = {.center = center, .radius = radius};
  return c;
}

static inline geom_vec2 geom_2d_circle_center(geom_2d_circle c)
{
  return c.center;
}

static inline double geom_2d_circle_radius(geom_2d_circle c)
{
  return c.radius;
}

JUPITER_GEOMETRY_DECL_END

#endif
