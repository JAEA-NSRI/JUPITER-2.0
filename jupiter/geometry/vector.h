/**
 * @file vector.h
 * @brief Vectors and Quarternion for geometry calculation.
 * @ingroup Geometry
 *
 * Each function provides get by element, equallity, addition,
 * subtraction, multiply each element, inner production, cross
 * production (for \f$ \mathbb{R}^3 \f$), length, scalar
 * factorization.
 */

#ifndef JUPITER_GEOMETRY_VECTOR_H
#define JUPITER_GEOMETRY_VECTOR_H

#include <math.h>

#include "defs.h"
#include "geom_math.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @ingroup Geometry
 * @brief \f$ \mathbb{R}^2 \f$ vector
 */
struct geom_vec2
{
  double x;
  double y;
};

/**
 * @ingroup Geometry
 * @brief \f$ \mathbb{R}^3 \f$ vector
 *
 * Consists \f$ \mathbb{R}^2 \f$ of \f$(x, y)\f$ and extra \f$z\f$.
 */
struct geom_vec3
{
  geom_vec2 xy;
  double z;
};

/**
 * @ingroup Geometry
 * @brief \f$ \mathbb{R}^4 \f$ vector
 *
 * Consists \f$ \mathbb{R}^3 \f$ of \f$(x, y, z)\f$ and extra \f$w\f$.
 *
 * This type is used to define quarternion.
 */
struct geom_vec4
{
  geom_vec3 xyz;
  double w;
};

/*
 * Recent compiler can optimize these verbosely implementated
 * functions, so readablity has priority.
 */

/**
 * @memberof geom_vec2
 * @brief get X component
 * @param v vector to get
 * @return result
 *
 * `v.x` is sufficient for this purpose, but `geom_vec3` and `geom_vec4`
 * have little complicated to get each element of vector.
 *
 * So, this function is provied for contrast to `geom_vec3` or
 * `geom_vec4`.
 *
 * @sa geom_vec3_x geom_vec4_x geom_quart_x
 */
static inline double
geom_vec2_x(geom_vec2 v)
{
  return v.x;
}

/**
 * @memberof geom_vec2
 * @brief get Y component
 * @param v vector to get
 * @return result
 *
 * @sa geom_vec2_x
 */
static inline double
geom_vec2_y(geom_vec2 v)
{
  return v.y;
}

/**
 * @memberof geom_vec2
 * @brief create 2D vector by components
 * @param x X component value
 * @param y Y component value
 * @return created vector
 *
 * @sa geom_vec3_c geom_vec4_c geom_quart_c
 */
static inline geom_vec2
geom_vec2_c(double x, double y)
{
  geom_vec2 v;
  v.x = x;
  v.y = y;
  return v;
}

/**
 * @memberof geom_vec2
 * @brief 2D vector equality
 * @param a lhs
 * @param b rhs
 * @retval 0 if two vectors are not equal
 * @retval non-0 if two vectors are equal
 *
 * @sa geom_vec3_eql geom_vec4_eql geom_quart_eql
 */
static inline int
geom_vec2_eql(geom_vec2 a, geom_vec2 b)
{
  return (a.x == b.x && a.y == b.y);
}

/**
 * @memberof geom_vec2
 * @brief 2D vector value is finite
 * @param a vector to test
 *
 * @retval non-0 if all elements are finite
 * @retval 0 if one or more elements are infinite or NaN.
 *
 * This function returns 1 (All elements are finite) if the system
 * does not provide isfinite macro.
 */
static inline int
geom_vec2_isfinite(geom_vec2 a)
{
#ifdef isfinite
  return (isfinite(a.x) && isfinite(a.y));
#else
  return 1;
#endif
}

/**
 * @memberof geom_vec2
 * @brief 2D vector test zero vector
 * @param a vector to test
 *
 * @retval non-0 if all elements are zero.
 * @retval 0 if not zero vector
 *
 * This function is shorthand for `geom_vec2_eql(a, geom_vec2_c(0.0, 0.0))`.
 */
static inline int
geom_vec2_iszero(geom_vec2 a)
{
  return geom_vec2_eql(a, geom_vec2_c(0.0, 0.0));
}

/**
 * @memberof geom_vec3
 * @brief get X component
 * @param v vector to get
 * @return result
 *
 * @geom_vec2_x
 */
static inline double
geom_vec3_x(geom_vec3 v)
{
  return geom_vec2_x(v.xy);
}

/**
 * @memberof geom_vec3
 * @brief get Y component
 * @param v vector to get
 * @return result
 */
static inline double
geom_vec3_y(geom_vec3 v)
{
  return geom_vec2_y(v.xy);
}

/**
 * @memberof geom_vec3
 * @brief get Z component
 * @param v vector to get
 * @return result
 */
static inline double
geom_vec3_z(geom_vec3 v)
{
  return v.z;
}

/**
 * @memberof geom_vec3
 * @brief Create 3D vector by components
 * @param x X component value
 * @param y Y component value
 * @param z Z component value
 * @return created vector
 *
 * @sa geom_vec2_c geom_vec4_c geom_quart_c
 */
static inline geom_vec3
geom_vec3_c(double x, double y, double z)
{
  geom_vec3 v;
  v.xy = geom_vec2_c(x, y);
  v.z = z;
  return v;
}

/**
 * @memberof geom_vec3
 * @brief Create 3D vector by 2D vector and z
 * @param v2 (X, Y) vector
 * @param z  Z component value
 * @return created vector
 *
 * @sa geom_vec4_c_v3w geom_quart_c_vw
 */
static inline geom_vec3
geom_vec3_c_v2z(geom_vec2 v2, double z)
{
  geom_vec3 v;
  v.xy = v2;
  v.z = z;
  return v;
}

/**
 * @memberof geom_vec3
 * @brief 3D vector equality
 * @param a lhs
 * @param b rhs
 * @retval 0 if two vectors are not equal
 * @retval non-0 if two vectors are equal
 *
 * @sa geom_vec2_eql geom_vec4_eql geom_quat_eql
 */
static inline int
geom_vec3_eql(geom_vec3 a, geom_vec3 b)
{
  return (geom_vec2_eql(a.xy, b.xy) && a.z == b.z);
}

/**
 * @memberof geom_vec3
 * @brief 3D vector value is finite
 * @param a vector to test
 *
 * @retval non-0 if all elements are finite
 * @retval 0 if one or more elements are infinite or NaN.
 *
 * This function returns 1 (All elements are finite) if the system
 * does not provide isfinite macro.
 */
static inline int
geom_vec3_isfinite(geom_vec3 a)
{
#ifdef isfinite
  return (geom_vec2_isfinite(a.xy) && isfinite(a.z));
#else
  return 1;
#endif
}

/**
 * @memberof geom_vec3
 * @brief 3D vector test zero vector
 * @param a vector to test
 *
 * @retval non-0 if all elements are zero.
 * @retval 0 if not zero vector
 *
 * This function is shorthand for `geom_vec3_eql(a, geom_vec3_c(0.0, 0.0, 0.0))`.
 */
static inline int
geom_vec3_iszero(geom_vec3 a)
{
  return geom_vec3_eql(a, geom_vec3_c(0.0, 0.0, 0.0));
}

/**
 * @memberof geom_vec4
 * @brief get X component
 * @param v vector to get
 * @return result
 *
 * @sa geom_vec2_x
 */
static inline double
geom_vec4_x(geom_vec4 v)
{
  return geom_vec3_x(v.xyz);
}

/**
 * @memberof geom_vec4
 * @brief get Y component
 * @param v vector to get
 * @return result
 */
static inline double
geom_vec4_y(geom_vec4 v)
{
  return geom_vec3_y(v.xyz);
}

/**
 * @memberof geom_vec4
 * @brief get Z component
 * @param v vector to get
 * @return result
 */
static inline double
geom_vec4_z(geom_vec4 v)
{
  return geom_vec3_z(v.xyz);
}

/**
 * @memberof geom_vec4
 * @brief get W component
 * @param v vector to get
 * @return result
 */
static inline double
geom_vec4_w(geom_vec4 v)
{
  return v.w;
}

/**
 * @memberof geom_vec4
 * @brief Create 4D vector by components
 * @param x X component value
 * @param y Y component value
 * @param z Z component value
 * @param w W component value
 * @return Created vector
 *
 * @sa geom_vec2_c geom_vec3_c geom_quart_c
 */
static inline geom_vec4
geom_vec4_c(double x, double y, double z, double w)
{
  geom_vec4 v;
  v.xyz = geom_vec3_c(x, y, z);
  v.w = w;
  return v;
}

/**
 * @memberof geom_vec4
 * @brief Create 4D vector by 3D vector and W component
 * @param v3 3D vector
 * @param w  W component value
 * @return Created vector
 *
 * @sa geom_vec3_c_v2z geom_quart_c_vw
 */
static inline geom_vec4
geom_vec4_c_v3w(geom_vec3 v3, double w)
{
  geom_vec4 v;
  v.xyz = v3;
  v.w = w;
  return v;
}

/**
 * @memberof geom_vec4
 * @brief 4D vector equality
 * @param a lhs
 * @param b rhs
 * @retval 0 if vectors are not equal
 * @retval non-0 if vectors are equal
 *
 * @sa geom_vec2_eql geom_vec3_eql geom_quart_eql
 */
static inline int
geom_vec4_eql(geom_vec4 a, geom_vec4 b)
{
  return (geom_vec3_eql(a.xyz, b.xyz) && a.w == b.w);
}

/**
 * @memberof geom_vec4
 * @brief 4D vector value is finite
 * @param a vector to test
 *
 * @retval non-0 if all elements are finite
 * @retval 0 if one or more elements are infinite or NaN.
 *
 * This function returns 1 (All elements are finite) if the system
 * does not provide isfinite macro.
 */
static inline int
geom_vec4_isfinite(geom_vec4 a)
{
#ifdef isfinite
  return (geom_vec3_isfinite(a.xyz) && isfinite(a.w));
#else
  return 1;
#endif
}

/**
 * @memberof geom_vec4
 * @brief 4D vector test zero vector
 * @param a vector to test
 *
 * @retval non-0 if all elements are zero.
 * @retval 0 if not zero vector
 *
 * This function is shorthand for
 * `geom_vec4_eql(a, geom_vec4_c(0.0, 0.0, 0.0, 0.0))`.
 */
static inline int
geom_vec4_iszero(geom_vec4 a)
{
  return geom_vec4_eql(a, geom_vec4_c(0.0, 0.0, 0.0, 0.0));
}

static inline geom_vec2
geom_vec2_add(geom_vec2 a, geom_vec2 b)
{
  return geom_vec2_c(geom_vec2_x(a) + geom_vec2_x(b),
                     geom_vec2_y(a) + geom_vec2_y(b));
}

static inline geom_vec2
geom_vec2_sub(geom_vec2 a, geom_vec2 b)
{
  return geom_vec2_c(geom_vec2_x(a) - geom_vec2_x(b),
                     geom_vec2_y(a) - geom_vec2_y(b));
}

static inline double
geom_vec2_inner_prod(geom_vec2 a, geom_vec2 b)
{
  return geom_vec2_x(a) * geom_vec2_x(b) + geom_vec2_y(a) * geom_vec2_y(b);
}

static inline geom_vec2
geom_vec2_mul_each_element(geom_vec2 a, geom_vec2 b)
{
  return geom_vec2_c(geom_vec2_x(a) * geom_vec2_x(b),
                     geom_vec2_y(a) * geom_vec2_y(b));
}

static inline double
geom_vec2_length(geom_vec2 a)
{
  return sqrt(geom_vec2_inner_prod(a, a));
}

static inline geom_vec2
geom_vec2_factor(geom_vec2 a, double f)
{
  return geom_vec2_c(geom_vec2_x(a) * f, geom_vec2_y(a) * f);
}

static inline double
geom_vec2_project_factor(geom_vec2 v, geom_vec2 target)
{
  double l;
  l = geom_vec2_inner_prod(target, target);
  if (l == 0.0) return 0.0;

  return geom_vec2_inner_prod(v, target) / l;
}

static inline double geom_vec2_angle(geom_vec2 a, geom_vec2 b)
{
  double l, f;
  if (geom_vec2_eql(a, b)) {
    f = 0.0;
  } else {
    f = geom_vec2_inner_prod(a, b);
    l = geom_vec2_length(a) * geom_vec2_length(b);
    if (l != 0.0) {
      f = f / l;
      if (f < -1.0) f = -1.0;
      if (f >  1.0) f =  1.0;
      f = geom_acosd(f);
    } else {
      f = 0.0;
    }
  }
  return f;
}

static inline geom_vec2
geom_vec2_angle_bisector(geom_vec2 a, geom_vec2 b)
{
  return geom_vec2_add(geom_vec2_factor(a, geom_vec2_length(b)),
                       geom_vec2_factor(b, geom_vec2_length(a)));
}

/**
 * @brief Vector which is normal to the angle bisector of @p a and @p b.
 * @param a Vector a
 * @param b Vector b
 * @return result
 *
 * \$(a|b| + b|a|)\$ and \$(a|b| - b|a|)\$ are perpendicular, because their
 * lengths are equal.
 */
static inline geom_vec2
geom_vec2_normal_angle_bisector(geom_vec2 a, geom_vec2 b)
{
  return geom_vec2_sub(geom_vec2_factor(a, geom_vec2_length(b)),
                       geom_vec2_factor(b, geom_vec2_length(a)));
}

static inline geom_vec3
geom_vec3_add(geom_vec3 a, geom_vec3 b)
{
  return geom_vec3_c_v2z(geom_vec2_add(a.xy, b.xy),
                         geom_vec3_z(a) + geom_vec3_z(b));
}

static inline geom_vec3
geom_vec3_sub(geom_vec3 a, geom_vec3 b)
{
  return geom_vec3_c_v2z(geom_vec2_sub(a.xy, b.xy),
                         geom_vec3_z(a) - geom_vec3_z(b));
}

static inline double
geom_vec3_inner_prod(geom_vec3 a, geom_vec3 b)
{
  return geom_vec2_inner_prod(a.xy, b.xy) + geom_vec3_z(a) * geom_vec3_z(b);
}

static inline geom_vec3
geom_vec3_mul_each_element(geom_vec3 a, geom_vec3 b)
{
  return geom_vec3_c_v2z(geom_vec2_mul_each_element(a.xy, b.xy),
                         geom_vec3_z(a) * geom_vec3_z(b));
}

static inline double
geom_vec3_length(geom_vec3 a)
{
  return sqrt(geom_vec3_inner_prod(a, a));
}

static inline geom_vec3
geom_vec3_factor(geom_vec3 a, double f)
{
  return geom_vec3_c_v2z(geom_vec2_factor(a.xy, f), geom_vec3_z(a) * f);
}

static inline geom_vec3
geom_vec3_cross_prod(geom_vec3 a, geom_vec3 b)
{
  double ax, ay, az;
  double bx, by, bz;
  ax = geom_vec3_x(a);
  ay = geom_vec3_y(a);
  az = geom_vec3_z(a);
  bx = geom_vec3_x(b);
  by = geom_vec3_y(b);
  bz = geom_vec3_z(b);
  return geom_vec3_c(ay * bz - az * by,
                     az * bx - ax * bz,
                     ax * by - ay * bx);
}

static inline double
geom_vec3_project_factor(geom_vec3 v, geom_vec3 target)
{
  double l;
  l = geom_vec3_inner_prod(target, target);
  if (l == 0.0) return 0.0;

  return geom_vec3_inner_prod(v, target) / l;
}

/**
 * @brief Angle of two vectors
 * @param a vector 1
 * @param b vector 2
 * @return angle
 *
 * Returning angle is 0 <= angle <= 180 (acute, right or obtuse)
 * degrees.
 *
 * If you want the rotation angle with -180 < angle <= 180 for
 * rotation axis of geom_vec3_cross_prod(a, b), the signess can be
 * test with following:
 *
 *     geom_vec3_inner_prod(geom_vec3_cross_prod(a, b), axis) < 0.0
 *
 * @sa geom_acosd, acos (libc)
 */
static inline double
geom_vec3_angle(geom_vec3 a, geom_vec3 b)
{
  double l, f;

  if (geom_vec3_eql(a, b)) {
    f = 0.0;
  } else {
    f = geom_vec3_inner_prod(a, b);
    l = geom_vec3_length(a) * geom_vec3_length(b);
    if (l != 0.0) {
      f = f / l;
      if (f < -1.0) f = -1.0;
      if (f >  1.0) f =  1.0;
      f = geom_acosd(f);
    } else {
      f = 0.0;
    }
  }
  return f;
}

static inline geom_vec3
geom_vec3_angle_bisector(geom_vec3 a, geom_vec3 b)
{
  return geom_vec3_add(geom_vec3_factor(a, geom_vec3_length(b)),
                       geom_vec3_factor(b, geom_vec3_length(a)));
}

static inline geom_vec3
geom_vec3_normal_angle_bisector(geom_vec3 a, geom_vec3 b)
{
  return geom_vec3_sub(geom_vec3_factor(a, geom_vec3_length(b)),
                       geom_vec3_factor(b, geom_vec3_length(a)));
}


static inline geom_vec4
geom_vec4_add(geom_vec4 a, geom_vec4 b)
{
  return geom_vec4_c_v3w(geom_vec3_add(a.xyz, b.xyz),
                         geom_vec4_w(a) + geom_vec4_w(b));
}

static inline geom_vec4
geom_vec4_sub(geom_vec4 a, geom_vec4 b)
{
  return geom_vec4_c_v3w(geom_vec3_sub(a.xyz, b.xyz),
                         geom_vec4_w(a) - geom_vec4_w(b));
}

static inline double
geom_vec4_inner_prod(geom_vec4 a, geom_vec4 b)
{
  return geom_vec3_inner_prod(a.xyz, b.xyz) + geom_vec4_w(a) * geom_vec4_w(b);
}

static inline double
geom_vec4_length(geom_vec4 a)
{
  return sqrt(geom_vec4_inner_prod(a, a));
}

static inline geom_vec4
geom_vec4_factor(geom_vec4 a, double f)
{
  return geom_vec4_c_v3w(geom_vec3_factor(a.xyz, f), geom_vec4_w(a) * f);
}

JUPITER_GEOMETRY_DECL
int geom_vec2_to_str(char **dest, geom_vec2 v, const char *fmt, int width,
                     int prec);

JUPITER_GEOMETRY_DECL
int geom_vec3_to_str(char **dest, geom_vec3 v, const char *fmt, int width,
                     int prec);

JUPITER_GEOMETRY_DECL
int geom_vec4_to_str(char **dest, geom_vec4 v, const char *fmt, int width,
                     int prec);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_vec2(geom_variant *v, geom_vec2 vec);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_vec3(geom_variant *v, geom_vec3 vec);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_vec4(geom_variant *v, geom_vec4 vec);

JUPITER_GEOMETRY_DECL
geom_vec2 geom_variant_get_vec2(const geom_variant *v, geom_error *err);

JUPITER_GEOMETRY_DECL
geom_vec3 geom_variant_get_vec3(const geom_variant *v, geom_error *err);

JUPITER_GEOMETRY_DECL
geom_vec4 geom_variant_get_vec4(const geom_variant *v, geom_error *err);

JUPITER_GEOMETRY_DECL_END

#endif
