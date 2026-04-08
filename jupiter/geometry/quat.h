/**
 * @file quat.h
 * @brief Quaternion for geometry calculation.
 * @ingroup Geometry
 */

#ifndef JUPITER_GEOMETRY_QUAT_H
#define JUPITER_GEOMETRY_QUAT_H

#include "defs.h"
#include "vector.h"
#include "geom_math.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @brief Quaternion
 * @sa geom_vec4
 */
struct geom_quat
{
  geom_vec4 vec; ///< data
};

/**
 * @memberof geom_quat
 * @brief get vector X value.
 * @param a quaternion to get.
 * @return x value
 */
static inline double
geom_quat_x(geom_quat a)
{
  return geom_vec4_x(a.vec);
}

/**
 * @memberof geom_quat
 * @brief get vector Y value.
 * @param a quaternion to get.
 * @return y value
 */
static inline double
geom_quat_y(geom_quat a)
{
  return geom_vec4_y(a.vec);
}

/**
 * @memberof geom_quat
 * @brief get vector Z value.
 * @param a quaternion to get.
 * @return z value
 */
static inline double
geom_quat_z(geom_quat a)
{
  return geom_vec4_z(a.vec);
}

/**
 * @memberof geom_quat
 * @brief get W value.
 * @param a quaternion to get.
 * @return w value
 */
static inline double
geom_quat_w(geom_quat a)
{
  return geom_vec4_w(a.vec);
}

/**
 * @memberof geom_quat
 * @brief get vector part of quaternion
 * @param a quaternion to get
 * @return vector part of `a`
 */
static inline geom_vec3
geom_quat_v(geom_quat a)
{
  return a.vec.xyz;
}

/**
 * @memberof geom_quat
 * @brief Create quaternion by each elements
 * @param x Vector x value
 * @param y Vector y value
 * @param z Vector z value
 * @param w w value
 * @return Created quaternion
 */
static inline geom_quat
geom_quat_c(double x, double y, double z, double w)
{
  geom_quat q;
  q.vec = geom_vec4_c(x, y, z, w);
  return q;
}

/**
 * @memberof geom_quat
 * @brief Create quaternion by vector and scalar
 * @param v Vector value
 * @param w Scalar value
 * @return Created quaternion
 */
static inline geom_quat
geom_quat_c_vw(geom_vec3 v, double w)
{
  geom_quat q;
  q.vec = geom_vec4_c_v3w(v, w);
  return q;
}

/**
 * @memberof geom_quat
 * @brief Create quaternion by `geom_vec4`
 * @param v Vector data.
 * @return Created quaternion
 */
static inline geom_quat
geom_quat_c_v4(geom_vec4 v)
{
  geom_quat q;
  q.vec = v;
  return q;
}

/**
 * @memberof geom_quat
 * @brief test equality of two quaternions
 * @param a a quaternion
 * @param b another quaternion
 * @retval 0 not equal
 * @retval non-0 equal
 */
static inline int
geom_quat_eql(geom_quat a, geom_quat b)
{
  return geom_vec4_eql(a.vec, b.vec);
}

/**
 * @memberof geom_quat
 * @brief Quaternion addition
 * @param a augend quaternion
 * @param b addend quaternion
 * @return result
 */
static inline geom_quat
geom_quat_add(geom_quat a, geom_quat b)
{
  return geom_quat_c_v4(geom_vec4_add(a.vec, b.vec));
}

/**
 * @memberof geom_quat
 * @brief Quaternion subtraction
 * @param a minuend quaternion
 * @param b subtrahend quaternion
 * @return result
 */
static inline geom_quat
geom_quat_sub(geom_quat a, geom_quat b)
{
  return geom_quat_c_v4(geom_vec4_sub(a.vec, b.vec));
}

/*
 * Translation note:
 *
 * In Japanese, "multiplier" (乗数) and "multiplicand" (被乗数) is
 * written in reverse order. So, `a` must be translated to 被乗数 or
 * かけられる数 and `b` must be translated to 乗数 or かける数.
 *
 * The order of multiplication is important, but it seems that these
 * aries do not have much meanings.
 */
/**
 * @memberof geom_quat
 * @brief Quaternion multiplication
 * @param a Multiplier quaternion a
 * @param b Multiplicand quaternion b
 * @return result.
 *
 * \f[
 *   \mathbf{a} \mathbf{b}
 *       = (a_w \vec{b_v} + b_w \vec{a_v} + \vec{a_v} \times \vec{b_v},\,
 *          a_w b_w - \vec{a_v} \cdot \vec{b_v})
 * \f]
 *
 * Note that quaternion multiplication is **NOT** commutative.
 */
static inline geom_quat
geom_quat_mul(geom_quat a, geom_quat b)
{
  double w;
  geom_vec3 v, va, vb;

  va = geom_quat_v(a);
  vb = geom_quat_v(b);
  w  = geom_quat_w(a) * geom_quat_w(b);
  w  = w - geom_vec3_inner_prod(va, vb);
  v  = geom_vec3_factor(va, geom_quat_w(b));
  v  = geom_vec3_add(v, geom_vec3_factor(vb, geom_quat_w(a)));
  v  = geom_vec3_add(v, geom_vec3_cross_prod(va, vb));
  return geom_quat_c_vw(v, w);
}

/**
 * @memberof geom_quat
 * @brief Norm of a quaternion
 * @param q Quaternion to compute
 * @return Norm of quaternion
 *
 * \f[
 *    \lVert\mathbf{q}\rVert = \sqrt{q_x^2 + q_y^2 + q_z^2 + q_w^2}
 * \f]
 */
static inline double
geom_quat_norm(geom_quat q)
{
  return geom_vec4_length(q.vec);
}

/**
 * @memberof geom_quat
 * @brief Scalar Factorization to a quaternion
 * @param q Quaternion
 * @param f Scalar factor
 * @return factored quaternion
 */
static inline geom_quat
geom_quat_factor(geom_quat q, double f)
{
  return geom_quat_c_v4(geom_vec4_factor(q.vec, f));
}

/**
 * @memberof geom_quat
 * @brief Conjugation of a quaternion
 * @param q Quaternion
 * @return result
 */
static inline geom_quat
geom_quat_conjugate(geom_quat q)
{
  return geom_quat_c_vw(geom_vec3_factor(geom_quat_v(q), -1.0),
                         geom_quat_w(q));
}

/**
 * @memberof geom_quat
 * @brief Inversion of a quaternion
 * @param q Quaternion
 * @return result
 *
 * \f[
 *  \mathbf{q}^{-1} = \frac{\mathbf{q}^*}{\lVert\mathbf{q}\rVert^2}
 * \f]
 */
static inline geom_quat
geom_quat_inv(geom_quat q)
{
  double n2;
  n2 = geom_vec4_inner_prod(q.vec, q.vec);
  q = geom_quat_conjugate(q);
  return geom_quat_factor(q, 1.0 / n2);
}

/**
 * @memberof geom_quat
 * @brief Create a quaternion for 3D rotation
 * @param axis axis vector \f$\mathbf{v}\f$
 * @param angle Angle \f$\mathbf{\theta}\f$ in degrees
 * @return result
 *
 * The `angle` should be normalized to range of -360 < angle <= 360
 * for good precision, but not strictly required.
 *
 * \f[
 *  \boldsymbol q =
 *   \begin{pmatrix}
 *    \mathbf{v} \sin \frac{\theta}{2} \\
 *    \cos\frac{\theta}{2} \\
 *   \end{pmatrix}
 * \f]
 */
static inline geom_quat
geom_quat_rotation(geom_vec3 axis, double angle)
{
  double l;
  angle = angle / 2.0;
  l = geom_vec3_length(axis);
  if (l == 0.0) return geom_quat_c(0.0, 0.0, 0.0, 0.0);
  axis = geom_vec3_factor(axis, geom_sind(angle) / l);
  return geom_quat_c_vw(axis, geom_cosd(angle));
}

/**
 * @memberof geom_quat
 * @brief Calculate rotated point by quaternion
 * @param q Quaternion to perform rotation
 * @param pnt Point to rotate
 * @return Rotated point
 */
static inline geom_vec3
geom_quat_rotate_p(geom_quat q, geom_vec3 pnt)
{
  geom_quat p;
  p = geom_quat_c_vw(pnt, 0.0);
  p = geom_quat_mul(q, p);
  q = geom_quat_mul(p, geom_quat_inv(q));
  return geom_quat_v(q);
}

/**
 * @memberof geom_quat
 * @brief Calculate rotated point by axis and angle using quaternion
 * @param axis axis vector \f$\mathbf{v}\f$
 * @param angle Angle \f$\mathbf{\theta}\f$ in degrees
 * @param pnt Point to rotate
 * @return Rotated point
 */
static inline geom_vec3
geom_quat_rotate_vp(geom_vec3 axis, double angle, geom_vec3 pnt)
{
  return geom_quat_rotate_p(geom_quat_rotation(axis, angle), pnt);
}

/**
 * @memberof geom_vec3
 * @brief Create rotatation quaternion which one vector to another
 * @param from Origin vector
 * @param to   Destination vector
 *
 * @return Quaternion rotate \p from to \p to.
 *
 * Since this function returns quaternion, defined in quat.h
 */
static inline geom_quat
geom_vec3_get_rotation(geom_vec3 from, geom_vec3 to)
{
  geom_vec3 a;
  geom_quat q;
  double d;
  double la, lf, lt;

  a  = geom_vec3_cross_prod(from, to);
  d  = geom_vec3_inner_prod(from, to);
  lf = geom_vec3_inner_prod(from, from);
  lt = geom_vec3_inner_prod(to, to);

  /**
   * For @p from and @p to are parallel (\f$\theta = 0\f$ or
   * \f$\theta = \frac{\pi}{2}\f$, so \f$\cos(\theta) = \pm 1\f$),
   * \f$dot(\mathbf{from}, \mathbf{to}) = \pm|from||to|\f$.
   *
   * So this function uses the following equation to check this:
   * \f$dot(\mathbf{from}, \mathbf{to})^2 \ge |from|^2|to|^2\f$.
   */
  if (d * d >= lf * lt) {
    if (d >= 0.0) {
      /* return geom_quat_c(0.0, 0.0, 0.0, 1.0); */
      return geom_quat_rotation(geom_vec3_c(1.0, 0.0, 0.0), 0.0);
    } else {
      d = geom_vec3_inner_prod(geom_vec3_c(1.0, 0.0, 0.0), from);
      if (d * d >= lf /* * 1.0 */) {
        a = geom_vec3_c(0.0, 1.0, 0.0);
      } else {
        a = geom_vec3_c(1.0, 0.0, 0.0);
      }
      d = geom_vec3_project_factor(a, from);
      a = geom_vec3_sub(a, geom_vec3_factor(from, d));
      return geom_quat_rotation(a, 180.0);
    }
  }

  q = geom_quat_c_vw(a, sqrt(lf * lt) + d);
  la = geom_quat_norm(q);
  return geom_quat_factor(q, 1.0 / la);
}

/**
 * @memberof geom_quat
 * @brief format a quaternion to text string
 * @param dest Location to store the result
 * @param q quaternion to format
 * @param fmt format of each element
 * @param width width value if `fmt` uses '*' for width
 * @param precision precision value if `fmt` uses '*' for precision
 * @return number of bytes to written, -1 if error.
 *
 * `fmt` must contain **ONE** floating point value conversion.
 *
 * This function contains a library call (cannot use without linking
 * against the geometry library).
 *
 * @sa geom_matrix_to_str geom_vec4_to_str
 */
static inline int
geom_quat_to_str(char **dest, geom_quat q, const char *fmt,
                  int width, int precision)
{
  return geom_vec4_to_str(dest, q.vec, fmt, width, precision);
}

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_quat(geom_variant *v, geom_quat qu);

JUPITER_GEOMETRY_DECL
geom_quat geom_variant_get_quat(const geom_variant *v, geom_error *e);

JUPITER_GEOMETRY_DECL_END

#endif
