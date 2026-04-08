#ifndef JUPITER_GEOMETRY_2D_LINE_H
#define JUPITER_GEOMETRY_2D_LINE_H

#include "defs.h"
#include "vector.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @brief Replesents 2D line
 *
 * \[origin + k * dir\]
 */
struct geom_2d_line
{
  geom_vec2 origin;
  geom_vec2 dir;
};
typedef struct geom_2d_line geom_2d_line;

/**
 * @brief Create a line
 * @param origin The point vector of origin
 * @param dir    Direction vector of the line
 */
static inline geom_2d_line geom_2d_line_c(geom_vec2 origin, geom_vec2 dir)
{
  geom_2d_line l = {.origin = origin, .dir = dir};
  return l;
}

/**
 * @brief Create a line though two points
 * @param p1 A point
 * @param p2 Another point
 */
static inline geom_2d_line geom_2d_line_c_p(geom_vec2 p1, geom_vec2 p2)
{
  return geom_2d_line_c(p1, geom_vec2_sub(p2, p1));
}

/**
 * @brief Create a line by slope and intercept
 * @param slope Slope value
 * @param intersept Intercept
 *
 * This function is not eligible to create \[x = c\] lines, even slope
 * is INFINITY.
 */
static inline geom_2d_line geom_2d_line_c_si(double slope, double intercept)
{
  return geom_2d_line_c(geom_vec2_c(0.0, intercept), geom_vec2_c(1.0, slope));
}

static inline geom_vec2 geom_2d_line_origin(geom_2d_line line)
{
  return line.origin;
}

static inline geom_vec2 geom_2d_line_direction(geom_2d_line line)
{
  return line.dir;
}

static inline geom_vec2 geom_2d_line_point(geom_2d_line line, double k)
{
  geom_vec2 o, d;
  o = geom_2d_line_origin(line);
  d = geom_2d_line_direction(line);
  return geom_vec2_add(o, geom_vec2_factor(d, k));
}

/**
 * @brief Test if two lines are parallel
 * @param l1 Line 1
 * @param l2 Line 2
 * @return non-zero if parallel, 0 otherwise
 */
static inline int geom_2d_line_is_parallel(geom_2d_line l1, geom_2d_line l2)
{
  geom_vec2 l1d, l2d;
  l1d = geom_2d_line_direction(l1);
  l2d = geom_2d_line_direction(l2);
  return geom_vec2_inner_prod(l1d, l2d) == 0.0;
}

JUPITER_GEOMETRY_DECL_END

#endif
