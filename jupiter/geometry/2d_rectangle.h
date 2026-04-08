#ifndef JUPITER_GEOMETRY_2D_RECTANGLE_H
#define JUPITER_GEOMETRY_2D_RECTANGLE_H

#include "defs.h"
#include "vector.h"

JUPITER_GEOMETRY_DECL_START

struct geom_2d_rectangle
{
  geom_vec2 p1;
  geom_vec2 p2;
};
typedef struct geom_2d_rectangle geom_2d_rectangle;

/**
 * @brief Create a rectangle (parallel to axis)
 * @param p1 Point 1
 * @param p2 Point 2
 *
 * Returning rectangle is not normalized (i.e. constructs as-is)
 */
static inline geom_2d_rectangle geom_2d_rectangle_c(geom_vec2 p1, geom_vec2 p2)
{
  geom_2d_rectangle r = {p1, p2};
  return r;
}

/**
 * @brief Get point 1 of rectangle
 */
static inline geom_vec2 geom_2d_rectangle_p1(geom_2d_rectangle rect)
{
  return rect.p1;
}

/**
 * @brief Get point 2 of rectangle
 */
static inline geom_vec2 geom_2d_rectangle_p2(geom_2d_rectangle rect)
{
  return rect.p2;
}

/**
 * @brief Obtain lower left corner of rectangle
 * @param rect Rectangle to compute
 * @return result
 */
static inline geom_vec2 geom_2d_rectangle_lower_left(geom_2d_rectangle rect)
{
  geom_vec2 s;
  s = geom_vec2_sub(rect.p2, rect.p1);
  if (geom_vec2_x(s) >= 0.0) {
    if (geom_vec2_y(s) >= 0.0) {
      return geom_2d_rectangle_p1(rect);
    } else {
      /*
       *  p1 *---*
       *     |   |
       *     *---* p2
       */
      return geom_vec2_c(geom_vec2_x(geom_2d_rectangle_p1(rect)),
                         geom_vec2_y(geom_2d_rectangle_p2(rect)));
    }
  } else {
    if (geom_vec2_y(s) >= 0.0) {
      /*
       *  p2 *---*
       *     |   |
       *     *---* p1
       */
      return geom_vec2_c(geom_vec2_x(geom_2d_rectangle_p2(rect)),
                         geom_vec2_y(geom_2d_rectangle_p1(rect)));
    } else {
      return geom_2d_rectangle_p2(rect);
    }
  }
}

/**
 * @brief Obtain upper right corner of rectangle
 * @param rect Rectangle to compute
 * @return result
 */
static inline geom_vec2 geom_2d_rectangle_upper_right(geom_2d_rectangle rect)
{
  geom_vec2 s;
  s = geom_vec2_sub(rect.p2, rect.p1);
  if (geom_vec2_x(s) >= 0.0) {
    if (geom_vec2_y(s) >= 0.0) {
      return geom_2d_rectangle_p2(rect);
    } else {
      return geom_vec2_c(geom_vec2_x(geom_2d_rectangle_p2(rect)),
                         geom_vec2_y(geom_2d_rectangle_p1(rect)));
    }
  } else {
    if (geom_vec2_y(s) >= 0.0) {
      return geom_vec2_c(geom_vec2_x(geom_2d_rectangle_p1(rect)),
                         geom_vec2_y(geom_2d_rectangle_p2(rect)));
    } else {
      return geom_2d_rectangle_p1(rect);
    }
  }
}

/**
 * @brief Normalize rectangle
 * @param rect rectangle to normalize
 *
 * Return rectangle with lower left corner and upper right corner.
 */
static inline geom_2d_rectangle geom_2d_rectangle_norm(geom_2d_rectangle rect)
{
  return geom_2d_rectangle_c(geom_2d_rectangle_lower_left(rect),
                             geom_2d_rectangle_upper_right(rect));
}

JUPITER_GEOMETRY_DECL_END

#endif
