#ifndef JUPITER_GEOMETRY_2D_LEVEL_SET_H
#define JUPITER_GEOMETRY_2D_LEVEL_SET_H

#include "2d_line.h"
#include "2d_line_segment.h"
#include "2d_convert.h"
#include "2d_quad.h"
#include "2d_circle.h"
#include "2d_circular_arc.h"
#include "2d_rectangle.h"
#include "defs.h"
#include "vector.h"

#include <math.h>

JUPITER_GEOMETRY_DECL_START

/**
 * @brief Calculate the point which the nearest point from @p p on @p line
 * @param line Line to compute
 * @param p Point to compute
 */
static inline geom_vec2 geom_2d_line_nearest(geom_2d_line line, geom_vec2 p)
{
  geom_vec2 o, d, op;
  o = geom_2d_line_origin(line);
  d = geom_2d_line_direction(line);
  op = geom_vec2_sub(p, o);
  return geom_2d_line_point(line, geom_vec2_project_factor(d, op));
}

/**
 * @brief Calculate the point which the nearest point from @p p on @p seg
 * @param seg Line segment to compute
 * @param p Point to compute
 */
static inline geom_vec2 geom_2d_line_segment_nearest(geom_2d_line_segment seg,
                                                     geom_vec2 p)
{
  geom_vec2 o, d, op;
  double f;
  o = geom_2d_line_segment_from(seg);
  d = geom_vec2_sub(geom_2d_line_segment_to(seg), o);
  op = geom_vec2_sub(p, o);
  f = geom_vec2_project_factor(d, op);
  if (f < 0.0) {
    return geom_2d_line_segment_from(seg);
  } else if (f > 1.0) {
    return geom_2d_line_segment_to(seg);
  } else {
    return geom_2d_line_segment_point(seg, f);
  }
}

/**
 * @brief Get the nearest point on a set of connected line segments
 *        (aka. line strip or broken line (not dashed line))
 * @param pnts Array of points
 * @param npnts Number of points
 * @param p Point to calculate
 * @param min_edge If non-null value given, sets index of the segment
 *                 that returned point lays.
 * @return result
 */
static inline geom_vec2 geom_2d_line_strip_nearest(const geom_vec2 *pnts,
                                                   geom_size_type npnts,
                                                   geom_vec2 p,
                                                   geom_size_type *min_edge)
{
  geom_2d_line_segment seg;
  geom_vec2 pmin;
  double lmin;
  geom_size_type mini;

  if (npnts <= 0) {
    if (min_edge)
      *min_edge = -1;
    return geom_vec2_c(HUGE_VAL, HUGE_VAL);
  }
  if (npnts == 1) {
    if (min_edge)
      *min_edge = 0;
    return pnts[0];
  }

  seg = geom_2d_line_segment_c(pnts[0], pnts[1]);
  pmin = geom_2d_line_segment_nearest(seg, p);
  lmin = geom_vec2_length(geom_vec2_sub(p, pmin));
  mini = 1;
  for (geom_size_type i = 1; i < npnts - 1; ++i) {
    geom_vec2 o;
    double l;
    seg = geom_2d_line_segment_c(pnts[i], pnts[i + 1]);
    o = geom_2d_line_segment_nearest(seg, p);
    l = geom_vec2_length(geom_vec2_sub(p, o));
    if (l < lmin) {
      lmin = l;
      pmin = o;
      mini = i + 1;
    }
  }
  if (min_edge)
    *min_edge = mini;
  return pmin;
}

/**
 * @brief Get the nearest point on a polygon
 * @param vertices Array of vertices
 * @param nvertices Number of vertices
 * @param p Point to calculate
 * @param min_edge If non-null value given, sets index of the segment
 *                 that returned point lays.
 * @return result
 */
static inline geom_vec2 geom_2d_polygon_nearest(const geom_vec2 *vertices,
                                                geom_size_type nvertices,
                                                geom_vec2 p,
                                                geom_size_type *min_edge)
{
  geom_2d_line_segment seg;
  geom_vec2 pmin;
  double lmin;
  geom_size_type mini;

  if (nvertices <= 0) {
    if (min_edge)
      *min_edge = -1;
    return geom_vec2_c(HUGE_VAL, HUGE_VAL);
  }
  if (nvertices == 1) {
    if (min_edge)
      *min_edge = 0;
    return vertices[0];
  }

  seg = geom_2d_line_segment_c(vertices[0], vertices[1]);
  pmin = geom_2d_line_segment_nearest(seg, p);
  lmin = geom_vec2_length(geom_vec2_sub(p, pmin));
  mini = 1;
  if (nvertices > 2) {
    for (geom_size_type i = 1; i < nvertices; ++i) {
      geom_vec2 o;
      double l;
      if (i == nvertices - 1) {
        seg = geom_2d_line_segment_c(vertices[i], vertices[0]);
      } else {
        seg = geom_2d_line_segment_c(vertices[i], vertices[i + 1]);
      }
      o = geom_2d_line_segment_nearest(seg, p);
      l = geom_vec2_length(geom_vec2_sub(p, o));
      if (l < lmin) {
        lmin = l;
        pmin = o;
        mini = i + 1;
      }
    }
  }
  if (min_edge)
    *min_edge = mini;
  return pmin;
}

/**
 * @brief Get the nearest point on a circle
 * @param circle Circle to compute
 * @param p Point to calculate
 */
static inline geom_vec2 geom_2d_circle_nearest(geom_2d_circle circle,
                                               geom_vec2 p)
{
  geom_vec2 x, y, c;
  double r, l;
  c = geom_2d_circle_center(circle);
  r = geom_2d_circle_radius(circle);
  x = geom_vec2_sub(p, c);
  l = geom_vec2_length(x);
  if (l <= 0.0 || !geom_vec2_isfinite(y = geom_vec2_factor(x, r / l))) {
    return geom_vec2_c(r, 0.0);
  }
  return geom_vec2_add(c, y);
}

/**
 * @brief Get the nearest point on a quadrilateral
 * @param quad Quadrilateral to compute
 * @param p Point to calculate
 * @param min_edge If non-null value given, sets index of the segment
 *                 that returned point lays.
 * @return result
 */
static inline geom_vec2 geom_2d_quad_nearest(geom_2d_quad quad, geom_vec2 p,
                                             geom_size_type *min_edge)
{
  geom_vec2 pnts[4] = {geom_2d_quad_p1(quad), geom_2d_quad_p2(quad),
                       geom_2d_quad_p3(quad), geom_2d_quad_p4(quad)};

  return geom_2d_polygon_nearest(pnts, 4, p, min_edge);
}

/**
 * @brief Get the nearest point on a rectangle
 * @param quad Quadrilateral to compute
 * @param p Point to calculate
 * @param min_edge If non-null value given, sets index of the segment
 *                 that returned point lays.
 * @return result
 *
 * This function normalizes the rectangle and then calculate the
 * point. So @p min_edge value will be following:
 *
 * | Value | Meaning    |
 * |-------|------------|
 * | 1     | south edge |
 * | 2     | east edge  |
 * | 3     | north edge |
 * | 4     | west edge  |
 */
static inline geom_vec2 geom_2d_rectangle_nearest(geom_2d_rectangle rect,
                                                  geom_vec2 p,
                                                  geom_size_type *min_edge)
{
  geom_2d_quad q;
  q = geom_2d_rectangle_to_quad(geom_2d_rectangle_norm(rect));

  return geom_2d_quad_nearest(q, p, min_edge);
}

JUPITER_GEOMETRY_DECL_END

#endif
