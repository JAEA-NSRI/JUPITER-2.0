#ifndef JUPITER_GEOMETRY_2D_LINE_SEGMENT_H
#define JUPITER_GEOMETRY_2D_LINE_SEGMENT_H

#include "defs.h"
#include "vector.h"
#include <math.h>

JUPITER_GEOMETRY_DECL_START

/**
 * @brief 2D line segment
 */
struct geom_2d_line_segment
{
  geom_vec2 from;
  geom_vec2 to;
};
typedef struct geom_2d_line_segment geom_2d_line_segment;

/**
 * @brief Create a line segment.
 * @param from A point vector
 * @param to   Another point vector
 */
static inline geom_2d_line_segment geom_2d_line_segment_c(geom_vec2 from,
                                                          geom_vec2 to)
{
  geom_2d_line_segment seg = {.from = from, .to = to};
  return seg;
}

static inline geom_vec2 geom_2d_line_segment_from(geom_2d_line_segment seg)
{
  return seg.from;
}

static inline geom_vec2 geom_2d_line_segment_to(geom_2d_line_segment seg)
{
  return seg.to;
}

/**
 * @brief Calculate the mid-point of a line segment
 * @param seg Line segment to calculate
 * @return The midpoint
 */
static inline geom_vec2 geom_2d_line_segment_midp(geom_2d_line_segment seg)
{
  geom_vec2 v, f, t;
  f = geom_2d_line_segment_from(seg);
  t = geom_2d_line_segment_to(seg);
  v = geom_vec2_add(f, t);
  return geom_vec2_factor(v, 0.5);
}

static inline geom_vec2
geom_2d_line_segment_point(geom_2d_line_segment seg, double k)
{
  geom_vec2 f, d;
  if (k < 0.0 || k > 1.0)
    return geom_vec2_c(HUGE_VAL, HUGE_VAL);

  f = geom_2d_line_segment_from(seg);
  d = geom_vec2_sub(geom_2d_line_segment_to(seg), f);
  return geom_vec2_add(f, geom_vec2_factor(d, k));
}

JUPITER_GEOMETRY_DECL_END


#endif
