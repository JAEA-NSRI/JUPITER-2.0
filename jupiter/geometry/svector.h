/* Vector type with signed integer */

#ifndef JUPITER_GEOMETRY_SVECTOR_H
#define JUPITER_GEOMETRY_SVECTOR_H

#include "defs.h"
#include "svector_defs.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @memberof geom_svec3
 * @brief Get X value of geom_svec3
 * @param v Vector
 */
static inline
geom_size_type geom_svec3_x(geom_svec3 v)
{
  return v.x;
}

/**
 * @memberof geom_svec3
 * @brief Get Y value of geom_svec3
 * @param v Vector
 */
static inline
geom_size_type geom_svec3_y(geom_svec3 v)
{
  return v.y;
}

/**
 * @memberof geom_svec3
 * @brief Get Z value of geom_svec3
 * @param v Vector
 */
static inline
geom_size_type geom_svec3_z(geom_svec3 v)
{
  return v.z;
}

/**
 * @memberof geom_svec3
 * @brief Create vector from independent integer values
 * @param x X value
 * @param y Y value
 * @param z Z value
 * @return Created vector
 */
static inline
geom_svec3 geom_svec3_c(geom_size_type x, geom_size_type y, geom_size_type z)
{
  geom_svec3 v = { .x = x, .y = y, .z = z };
  return v;
}

/**
 * @memberof geom_svec3
 * @brief Vector addition
 * @param a Augend vector
 * @param b Addend vector
 * @return result
 */
static inline
geom_svec3 geom_svec3_add(geom_svec3 a, geom_svec3 b)
{
  return geom_svec3_c(geom_svec3_x(a) + geom_svec3_x(b),
                      geom_svec3_y(a) + geom_svec3_y(b),
                      geom_svec3_z(a) + geom_svec3_z(b));
}

/**
 * @memberof geom_svec3
 * @brief Vector subtraction
 * @param a Minuend vector
 * @param b Subtrahend vector
 * @return result
 */
static inline
geom_svec3 geom_svec3_sub(geom_svec3 a, geom_svec3 b)
{
  return geom_svec3_c(geom_svec3_x(a) - geom_svec3_x(b),
                      geom_svec3_y(a) - geom_svec3_y(b),
                      geom_svec3_z(a) - geom_svec3_z(b));
}

/**
 * @memberof geom_svec3
 * @brief Multiply each element
 * @param a Multiplier vector
 * @param b Multiplicand vector
 * @return result
 */
static inline
geom_svec3 geom_svec3_mul_each_element(geom_svec3 a, geom_svec3 b)
{
  return geom_svec3_c(geom_svec3_x(a) * geom_svec3_x(b),
                      geom_svec3_y(a) * geom_svec3_y(b),
                      geom_svec3_z(a) * geom_svec3_z(b));
}

/**
 * @memberof geom_svec3
 * @brief perform left axis rotation
 * @param v vector to rotate
 * @return rotated vector
 *
 * Shift to left (i.e, (y, z, x)) will be returned.
 */
static inline
geom_svec3 geom_svec3_rotate_yzx(geom_svec3 v)
{
  return geom_svec3_c(geom_svec3_y(v), geom_svec3_z(v), geom_svec3_x(v));
}

/**
 * @memberof geom_svec3
 * @brief perform right axis rotation
 * @param v vector to rotate
 * @return rotated vector
 *
 * Shift to right (i.e, (z, x, y)) will be returned.
 */
static inline
geom_svec3 geom_svec3_rotate_zxy(geom_svec3 v)
{
  return geom_svec3_c(geom_svec3_z(v), geom_svec3_x(v), geom_svec3_y(v));
}

/**
 * @memberof geom_svec3
 * @brief Test equality of vectors
 * @param a One vector
 * @param b Another vector
 * @return non-0 if a == b, 0 if a != b
 */
static inline
int geom_svec3_eql(geom_svec3 a, geom_svec3 b)
{
  return (a.x == b.x && a.y == b.y && a.z == b.z);
}

/**
 * @ingroup Geometry
 * @brief Integral range data
 */
struct geom_range {
  geom_size_type start; ///< Start value
  geom_size_type end;   ///< End value
  int include_end;      ///< Whether `end` is included or not.
};
typedef struct geom_range geom_range;

/**
 * @memberof geom_range
 * @brief Create range
 * @param start Start position
 * @param end End position
 * @param include_end Whether include @p end value or not.
 */
static inline geom_range
geom_range_c(geom_size_type start, geom_size_type end, int include_end)
{
  geom_range r;
  r.start = start;
  r.end   = end;
  r.include_end = !!include_end;
  return r;
}

/**
 * @memberof geom_range
 * @brief Get start value
 * @return start value
 */
static inline geom_size_type
geom_range_start(geom_range x)
{
  return x.start;
}

/**
 * @memberof geom_range
 * @brief Get end value
 * @return end value
 *
 * This function returns the end value as-is.
 */
static inline geom_size_type
geom_range_end(geom_range x)
{
  return x.end;
}

/**
 * @memberof geom_range
 * @brief Get include_end value
 * @return whether end point should be included or not
 */
static inline int
geom_range_end_included(geom_range x)
{
  return x.include_end;
}

/**
 * @memberof geom_range
 * @brief Get overlapped range
 * @param a One range
 * @param b Another range
 * @return Range of overlapped region of two ranges.
 *
 * If two ranges do not overlap each other, this function returns a
 * range which will be computed as empty.
 */
static inline geom_range
geom_range_overlap(geom_range a, geom_range b)
{
  geom_size_type as, bs, ae, be;
  as = a.start;
  bs = b.start;
  ae = a.end + a.include_end;
  be = b.end + b.include_end;
  as = (as < bs) ? bs : as;
  ae = (ae < be) ? ae : be;
  if (ae < as) ae = as;
  return geom_range_c(as, ae, 0);
}

/**
 * @memberof geom_range
 * @brief Test whether single point value is included in the range
 * @param r Range to test
 * @param pnt Point to test
 * @return non-0 if pnt is included in the range, 0 if not.
 */
static inline int
geom_range_include(geom_range r, geom_size_type pnt)
{
  if (pnt <  r.start) return 0;
  if (pnt >= r.end + r.include_end) return 0;
  return 1;
}

/**
 * @memberof geom_range
 * @brief Get size of range
 * @param a Range to compute
 * @return Size of range, 0 if empty.
 */
static inline geom_size_type
geom_range_size(geom_range a)
{
  return (a.start < a.end) ? (a.end - a.start + a.include_end) : 0;
}

/**
 * @brief Iterate over a range
 * @param it Iterator variable (geom_size_type)
 * @param range Range to iterate over
 * @param offset Step value
 * @relates geom_range
 */
#define geom_range_foreach(it, range, offset)                       \
  for((it) = geom_range_start(range);                               \
      geom_range_include((range), (it)); (it) = (it) + (offset))

/**
 * @ingroup Geometry
 *
 * Rectilinear 3D-world range.
 */
struct geom_range3 {
  geom_range x; ///< Range of X-axis
  geom_range y; ///< Range of Y-axis
  geom_range z; ///< Range of Z-axis
};
typedef struct geom_range3 geom_range3;

/**
 * @memberof geom_range3
 * @brief Create 3D-world range from vectors
 * @param start start position
 * @param end end position
 * @param include_end Whether include end point.
 */
static inline geom_range3
geom_range3_c_vec(geom_svec3 start, geom_svec3 end, int include_end)
{
  geom_range3 r;
  r.x = geom_range_c(geom_svec3_x(start), geom_svec3_x(end), include_end);
  r.y = geom_range_c(geom_svec3_y(start), geom_svec3_y(end), include_end);
  r.z = geom_range_c(geom_svec3_z(start), geom_svec3_z(end), include_end);
  return r;
}

/**
 * @memberof geom_range3
 * @brief Create 3D-world range from ranges
 * @param x X range
 * @param y Y range
 * @param z Z range
 *
 * All include-end-point flags should be same, but it is safe even if
 * it's not.
 */
static inline geom_range3
geom_range3_c_range(geom_range x, geom_range y, geom_range z)
{
  geom_range3 r = { .x = x, .y = y, .z = z };
  return r;
}

/**
 * @memberof geom_range3
 * @brief Get range for X-axis
 * @param r 3D-world range
 * @return X-axis range
 */
static inline geom_range
geom_range3_xrange(geom_range3 r)
{
  return r.x;
}

/**
 * @memberof geom_range3
 * @brief Get range for Y-axis
 * @param r 3D-world range
 * @return Y-axis range
 */
static inline geom_range
geom_range3_yrange(geom_range3 r)
{
  return r.y;
}

/**
 * @memberof geom_range3
 * @brief Get range for Z-axis
 * @param r 3D-world range
 * @return Z-axis range
 */
static inline geom_range
geom_range3_zrange(geom_range3 r)
{
  return r.z;
}

/**
 * @memberof geom_range3
 * @brief Get start position
 * @param r 3D-world range
 * @return start position vector
 */
static inline geom_svec3
geom_range3_start(geom_range3 r)
{
  return geom_svec3_c(geom_range_start(geom_range3_xrange(r)),
                      geom_range_start(geom_range3_yrange(r)),
                      geom_range_start(geom_range3_zrange(r)));
}

/**
 * @memberof geom_range3
 * @brief Get start position
 * @param r 3D-world range
 * @return end position vector
 */
static inline geom_svec3
geom_range3_end(geom_range3 r)
{
  return geom_svec3_c(geom_range_end(geom_range3_xrange(r)),
                      geom_range_end(geom_range3_yrange(r)),
                      geom_range_end(geom_range3_zrange(r)));
}

/**
 * @memberof geom_range3
 * @brief Get overlap of ranges
 * @param a One range
 * @param b Another range
 * @return overlapping range, or empty range if not.
 */
static inline geom_range3
geom_range3_overlap(geom_range3 a, geom_range3 b)
{
  return geom_range3_c_range(geom_range_overlap(a.x, b.x),
                             geom_range_overlap(a.y, b.y),
                             geom_range_overlap(a.z, b.z));
}

/**
 * @memberof geom_range3
 * @brief Total size of range
 * @param r Range to compute
 * @return Total range size
 */
static inline geom_size_type
geom_range3_size(geom_range3 r)
{
  return geom_range_size(geom_range3_xrange(r))
    *    geom_range_size(geom_range3_yrange(r))
    *    geom_range_size(geom_range3_zrange(r));
}

/**
 * @memberof geom_range3
 * @brief Size of each ranges
 * @param r Range to compute
 * @return Vector of each range size
 */
static inline geom_svec3
geom_range3_size_vec(geom_range3 r)
{
  return geom_svec3_c(geom_range_size(geom_range3_xrange(r)),
                      geom_range_size(geom_range3_yrange(r)),
                      geom_range_size(geom_range3_zrange(r)));
}

/**
 * @memberof geom_range3
 * @brief Test whether a point is in the range.
 * @param r Range to test
 * @param pnt A point to test
 * @return non-0 if range includes the given point, 0 if not.
 */
static inline int
geom_range3_include(geom_range3 range, geom_svec3 pnt)
{
  return geom_range_include(geom_range3_xrange(range), geom_svec3_x(pnt))
    &&   geom_range_include(geom_range3_yrange(range), geom_svec3_y(pnt))
    &&   geom_range_include(geom_range3_zrange(range), geom_svec3_z(pnt));
}

/**
 * @memberof geom_range3
 * @brief Get next point of integeral point, in rectilinear geometry
 * @param iter Current point
 * @param range Range to iterate over
 * @param offs Vector of offets for each axis.
 *
 * You may want to use geom_range3_foreach() macro instead.
 */
static inline geom_svec3
geom_range3_foreach_next(geom_svec3 iter, geom_range3 range, geom_svec3 offs)
{
  geom_size_type ix, iy, iz;
  ix = geom_svec3_x(iter) + geom_svec3_x(offs);
  iy = geom_svec3_y(iter);
  iz = geom_svec3_z(iter);
  if (!geom_range_include(geom_range3_xrange(range), ix)) {
    iy += geom_svec3_y(offs);
    ix  = geom_range_start(geom_range3_xrange(range));
    if (!geom_range_include(geom_range3_yrange(range), iy)) {
      iz += geom_svec3_z(offs);
      iy  = geom_range_start(geom_range3_yrange(range));
    }
  }
  return geom_svec3_c(ix, iy, iz);
}

/**
 * @relates geom_range3
 * @brief Iterate over rectilinear 3D-world range
 * @param it Iterating point vector variable (geom_svec3)
 * @param range Range to iterate over (geom_range3)
 * @param offs Iteration step (geom_svec3)
 *
 * This macro creates a single loop, not nested loop.
 */
#define geom_range3_foreach(it, range, offs)                            \
  for((it) = geom_range3_start((range));                                \
      geom_range3_include((range), (it));                               \
      (it) = geom_range3_foreach_next((it), (range), (offs)))

/**
 * @memberof geom_variant
 * @brief Set an integral 3D-vector from the variant.
 * @param v Variant to set
 * @return GEOM_SUCCESS. This function won't fail.
 *
 * Set the integral 3D-vector into a variant.
 */
JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_svec3(geom_variant *v, geom_svec3 vec);

/**
 * @memberof geom_variant
 * @brief Get an integral 3D-vector from the variant.
 * @param v Variant to get
 * @param e If error, sets the error info here.
 * @return result.
 *
 * Get an integral 3D-vector from the variant.
 */
JUPITER_GEOMETRY_DECL
geom_svec3 geom_variant_get_svec3(const geom_variant *v, geom_error *e);

/**
 * @brief Convert geom_svec3 to text representation.
 * @param buf Location to store the result.
 * @param vec vector to convert.
 * @param width Column width of each element (-1 for flexible)
 * @param precision Precision value of each element (-1 for minimal output)
 * @return Number fo bytes written, -1 if errors occured.
 */
JUPITER_GEOMETRY_DECL
int geom_svec3_to_str(char **buf, geom_svec3 vec, int width, int precision);

JUPITER_GEOMETRY_DECL_END

#endif
