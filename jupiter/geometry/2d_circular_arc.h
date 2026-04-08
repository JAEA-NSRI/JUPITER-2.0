#ifndef JUPITER_GEOMETRY_2D_CIRCULAR_ARC_H
#define JUPITER_GEOMETRY_2D_CIRCULAR_ARC_H

#include "defs.h"
#include "vector.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @brief 2D circular arc
 */
struct geom_2d_circular_arc
{
  geom_vec2 from;
  double radius;
  int larger_arc_flag;
  int sweep_flag;
  geom_vec2 to;
};
typedef struct geom_2d_circular_arc geom_2d_circular_arc;

static inline geom_2d_circular_arc
geom_2d_circular_arc_c(geom_vec2 from, geom_vec2 to, double radius,
                       int larger_arc_flag, int sweep_flag)
{
  geom_2d_circular_arc arc = {
    .from = from, .to = to, .radius = radius,
    .larger_arc_flag = larger_arc_flag, .sweep_flag = sweep_flag
  };
  return arc;
}

static inline geom_vec2 geom_2d_circular_arc_from(geom_2d_circular_arc arc)
{
  return arc.from;
}

static inline geom_vec2 geom_2d_circular_arc_to(geom_2d_circular_arc arc)
{
  return arc.to;
}

static inline double geom_2d_circular_arc_radius(geom_2d_circular_arc arc)
{
  return arc.radius;
}

static inline int geom_2d_circular_arc_is_larger_arc(geom_2d_circular_arc arc)
{
  return arc.larger_arc_flag;
}

static inline int geom_2d_circular_arc_is_sweep(geom_2d_circular_arc arc)
{
  return arc.sweep_flag;
}

JUPITER_GEOMETRY_DECL_END

#endif
