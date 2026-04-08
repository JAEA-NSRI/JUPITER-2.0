#ifndef JUPITER_GEOMETRY_2D_CONVERT_H
#define JUPITER_GEOMETRY_2D_CONVERT_H

#include "defs.h"
#include "vector.h"
#include "2d_line.h"
#include "2d_line_segment.h"
#include "2d_quad.h"
#include "2d_rectangle.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @biref Create a line through given segment
 * @param seg Line segment to use
 */
static inline geom_2d_line geom_2d_line_c_seg(geom_2d_line_segment seg)
{
  return geom_2d_line_c_p(geom_2d_line_segment_from(seg),
                          geom_2d_line_segment_to(seg));
}

/**
 * @brief Convert rectangle to quadrilateral.
 * @param rect rectangle to convert
 * @return result
 *
 * Return in clock-wise order, starting with rect.p1
 */
static inline geom_2d_quad geom_2d_rectangle_to_quad(geom_2d_rectangle rect)
{
  geom_vec2 p1, p2, p3, p4, s;
  p1 = rect.p1;
  p3 = rect.p2;
  s = geom_vec2_sub(p3, p1);
  if ((geom_vec2_x(s) < 0.0 && geom_vec2_y(s) >= 0.0) ||
      (geom_vec2_x(s) >= 0.0 && geom_vec2_y(s) < 0.0)) {
    /* move y first */
    p2 = geom_vec2_c(geom_vec2_x(geom_2d_rectangle_p1(rect)),
                     geom_vec2_y(geom_2d_rectangle_p2(rect)));
    p4 = geom_vec2_c(geom_vec2_x(geom_2d_rectangle_p2(rect)),
                     geom_vec2_y(geom_2d_rectangle_p1(rect)));
  } else {
    /* move x first */
    p2 = geom_vec2_c(geom_vec2_x(geom_2d_rectangle_p2(rect)),
                     geom_vec2_y(geom_2d_rectangle_p1(rect)));
    p4 = geom_vec2_c(geom_vec2_x(geom_2d_rectangle_p1(rect)),
                     geom_vec2_y(geom_2d_rectangle_p2(rect)));
  }
  return geom_2d_quad_c(p1, p2, p3, p4);
}


JUPITER_GEOMETRY_DECL_END

#endif
