#ifndef JUPITER_GEOMETRY_2D_INOUT_TESTS_H
#define JUPITER_GEOMETRY_2D_INOUT_TESTS_H

#include "defs.h"
#include "vector.h"
#include "2d_circle.h"
#include "2d_rectangle.h"

JUPITER_GEOMETRY_DECL_START

static inline int geom_2d_circle_inout(geom_vec2 pnt, geom_2d_circle c)
{
  return geom_vec2_length(geom_vec2_sub(pnt, geom_2d_circle_center(c))) <=
         geom_2d_circle_radius(c);
}

static inline int geom_2d_rectangle_inout(geom_vec2 pnt, geom_2d_rectangle rect)
{
  geom_vec2 ll, ur;
  double x, y;
  ll = geom_2d_rectangle_lower_left(rect);
  ur = geom_2d_rectangle_upper_right(rect);
  x = geom_vec2_x(pnt);
  y = geom_vec2_y(pnt);
  return (x >= geom_vec2_x(ll) && x <= geom_vec2_x(ur)) &&
         (y >= geom_vec2_y(ll) && y <= geom_vec2_y(ur));
}

JUPITER_GEOMETRY_DECL_END

#endif
