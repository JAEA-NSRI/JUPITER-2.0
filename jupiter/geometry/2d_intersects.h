#ifndef JUPITER_GEOMETRY_2D_INTERSECTS_H
#define JUPITER_GEOMETRY_2D_INTERSECTS_H

#include "defs.h"
#include "vector.h"
#include "mat22.h"
#include "2d_line.h"
#include "2d_line_segment.h"
#include "2d_circle.h"
#include "2d_circular_arc.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @brief Calculate the intersection of two lines
 * @param l1 Line 1
 * @param l2 Line 2
 * @return intersection point
 *
 * If two lines are parallel or same line, returns a vector with
 * INFINITY.
 */
static inline geom_vec2 geom_2d_line_intersect(geom_2d_line l1, geom_2d_line l2)
{
  geom_vec2 v;
  geom_vec2 o1, o2, d1, d2;

  o1 = geom_2d_line_origin(l1);
  o2 = geom_2d_line_origin(l2);
  d1 = geom_2d_line_direction(l1);
  d2 = geom_2d_line_direction(l2);

  /*
   * O1 + t D1 = O2 + u D2  -> t D1 + (- u) D2 = O2 - O1.
   *
   * So obtain t by splitting.
   */
  v = geom_vec2_split(geom_vec2_sub(o2, o1), d1, d2);
  if (!geom_vec2_isfinite(v)) {
    return v;
  }
  return geom_2d_line_point(l1, geom_vec2_x(v));
}

/**
 * @brief Calculate the intersection of two line segments
 * @param s1 segment 1
 * @param s2 segment 2
 *
 * If two line segments does not intersect each other, returns a
 * vector with INFINITY.
 */
static inline geom_vec2 geom_2d_line_segment_intersect(geom_2d_line_segment s1,
                                                       geom_2d_line_segment s2)
{
  geom_vec2 o1, o2, d1, d2, v;
  o1 = geom_2d_line_segment_from(s1);
  o2 = geom_2d_line_segment_from(s2);
  d1 = geom_vec2_sub(geom_2d_line_segment_to(s1), o1);
  d2 = geom_vec2_sub(geom_2d_line_segment_to(s2), o2);

  /*
   * See geom_2d_line_intersect() for more info.
   */
  v = geom_vec2_split(geom_vec2_sub(o2, o1), d1, d2);
  if (!geom_vec2_isfinite(v)) {
    return v;
  }

  return geom_2d_line_segment_point(s1, geom_vec2_x(v));
}


/**
 * @brief Get the center point of given circular arc
 * @param arc Arc to calculate
 * @return center point
 *
 * If the diameter of given arc is smaller than distance of from and
 * to points, this function returns mid point of them.
 */
static inline geom_vec2 geom_2d_circular_arc_center(geom_2d_circular_arc arc)
{
  geom_vec2 f, t, mp, hv;
  double l, r, h;

  f = geom_2d_circular_arc_from(arc);
  t = geom_2d_circular_arc_to(arc);
  mp = geom_2d_line_segment_midp(geom_2d_line_segment_c(f, t));
  l = geom_vec2_length(geom_vec2_sub(mp, f));
  r = geom_2d_circular_arc_radius(arc);

  if (r < 0.0 || l <= 0.0 || l <= r) {
    return mp;
  }

  /*
   * The triangle with points arc.from, arc.to and the obtaining
   * center forms an isoceles. So the distance to the center from
   * segment can be obtained by the Pythagorean theorem.
   */
  h = sqrt(r * r - l * l);

  /*
   *             ,--->----.
   *            /          \
   *           /            \ (larger and not sweep)
   *          /              \
   *         |                |
   *         |       *(h+)    |
   *         |   ,--->----.   |
   *          \ /          \ /  (not larger and not sweep)
   *    (from) *------------* (to)
   *          / \          / \  (not larger and seep)
   *         |   '---<----'   |
   *         |       * (h-)   |
   *         |                | (larger and sweep)
   *          \              /
   *           \            /
   *            \          /
   *             '---<----'
   */
  if ((!arc.larger_arc_flag && !arc.sweep_flag) ||
      (arc.larger_arc_flag && arc.sweep_flag)) {
    h *= -1.0;
  }

  hv = geom_vec2_rotate90(geom_vec2_sub(arc.to, arc.from));
  return geom_vec2_add(mp, geom_vec2_factor(hv, h / geom_vec2_length(hv)));
}

/**
 * @brief Line-Circle intersection
 * @param l Line
 * @param c Circle
 * @parem intersects intersection points (out)
 * @return number of intersections
 *
 * This function uses geometric method.
 */
static inline int geom_2d_line_circle_intersect(geom_2d_line l,
                                                geom_2d_circle c,
                                                geom_vec2 intersects[2])
{
  geom_vec2 lc, lb, lh, lp;
  double r, h, p, f;

  lc = geom_vec2_sub(c.center, l.origin);
  lb = geom_vec2_factor(l.dir, geom_vec2_project_factor(lc, l.dir));
  lh = geom_vec2_sub(c.center, lb);
  h = geom_vec2_length(lh);
  r = c.radius;
  if (h > r)
    return 0;

  lp = geom_vec2_add(c.center, lh);
  if (h == r) {
    intersects[0] = lp;
    return 1;
  }

  p = sqrt(r * r - h * h);
  f = p / geom_vec2_length(lb);
  intersects[0] = geom_vec2_add(lp, geom_vec2_factor(lb, -f));
  intersects[1] = geom_vec2_add(lp, geom_vec2_factor(lb, f));
  return 2;
}

/**
 * @brief Circle-Circle intersection
 * @param c1 Circle 1
 * @param c2 Circle 2
 * @param intersects intersection points (out)
 * @return number of intersections
 * @retval  0 No intersection
 * @retval -1 Two circles are identical.
 *
 * Performs geometric transformation to simplify the equation.
 */
static inline int geom_2d_circle_intersect(geom_2d_circle c1, geom_2d_circle c2,
                                           geom_vec2 intersects[2])
{
  geom_vec2 xvec, yvec;
  double d, x, y, r1, r2;

  r1 = c1.radius;
  r2 = c2.radius;
  if (r1 <= 0.0 || r2 <= 0.0)
    return 0;

  xvec = geom_vec2_sub(c2.center, c1.center);

  d = geom_vec2_length(xvec);
  if (d <= 0.0) {
    if (r1 == r2)
      return -1;
    return 0;
  }
  if (d < fabs(r1 - r2))
    return 0;
  if (d > r1 + r2)
    return 0;

  /*
   * Now, we only have to solve the following equation:
   *
   *  x^2      + y^2 = r1^2
   * (x - d)^2 + y^2 = r2^2
   *
   * -> 2 d x - d^2 = r1^2 - r2^2
   */
  x = (r1 * r1 - r2 * r2 + d * d) / (2.0 * d);
  y = sqrt(r1 * r1 - x * x);

  xvec = geom_vec2_factor(xvec, 1.0 / d);
  yvec = geom_vec2_rotate90(xvec);
  xvec = geom_vec2_factor(xvec, x);

  if (y <= 0.0) {
    intersects[0] = xvec;
    return 1;
  }

  intersects[0] = geom_vec2_add(xvec, geom_vec2_factor(yvec, y));
  intersects[1] = geom_vec2_add(xvec, geom_vec2_factor(yvec, -y));
  return 2;
}

JUPITER_GEOMETRY_DECL_END

#endif
