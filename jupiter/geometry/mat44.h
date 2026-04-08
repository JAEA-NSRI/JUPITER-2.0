
#ifndef JUPITER_GEOMETRY_MAT44_H
#define JUPITER_GEOMETRY_MAT44_H

#include "defs.h"
#include "vector.h"
#include "quat.h"
#include "mat43.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @ingroup Geometry
 * @brief full 4x4 matrix
 */
struct geom_mat44
{
  geom_mat43 m43;
  double m4[4];
};

/**
 * @memberof geom_mat44
 * @brief Create full 4x4 matrix
 *
 * @param a11 a11 element
 * @param a12 a12 element
 * @param a13 a13 element
 * @param a14 a14 element
 * @param a21 a21 element
 * @param a22 a22 element
 * @param a23 a23 element
 * @param a24 a24 element
 * @param a31 a31 element
 * @param a32 a32 element
 * @param a33 a33 element
 * @param a34 a34 element
 * @param a41 a41 element
 * @param a42 a42 element
 * @param a43 a43 element
 * @param a44 a44 element
 *
 * @return Created matrix
 */
static inline geom_mat44
geom_mat44_c(double a11, double a12, double a13, double a14,
             double a21, double a22, double a23, double a24,
             double a31, double a32, double a33, double a34,
             double a41, double a42, double a43, double a44)
{
  geom_mat44 m = {
    .m43 = geom_mat43_c(a11, a12, a13, a14,
                        a21, a22, a23, a24,
                        a31, a32, a33, a34),
    .m4  = { a41, a42, a43, a44 },
  };
  return m;
}

/**
 * @memberof geom_mat44
 * @brief Create full 4x4 matrix with column vectors
 *
 * @param a1 first column vector
 * @param a2 second column vector
 * @param a3 third column vector
 * @param a4 fourth column vector
 *
 * @return Created matrix
 */
static inline geom_mat44
geom_mat44_c_cv(geom_vec4 a1, geom_vec4 a2, geom_vec4 a3, geom_vec4 a4)
{
  return geom_mat44_c( //
    geom_vec4_x(a1), geom_vec4_x(a2), geom_vec4_x(a3), geom_vec4_x(a4),
    geom_vec4_y(a1), geom_vec4_y(a2), geom_vec4_y(a3), geom_vec4_y(a4),
    geom_vec4_z(a1), geom_vec4_z(a2), geom_vec4_z(a3), geom_vec4_z(a4),
    geom_vec4_w(a1), geom_vec4_w(a2), geom_vec4_w(a3), geom_vec4_w(a4));
}

/**
 * @memberof geom_mat44
 * @brief Create full 4x4 matrix with row vectors
 *
 * @param a1 first row vector
 * @param a2 second row vector
 * @param a3 third row vector
 * @param a4 fourth row vector
 *
 * @return Created matrix
 */
static inline geom_mat44
geom_mat44_c_rv(geom_vec4 a1, geom_vec4 a2, geom_vec4 a3, geom_vec4 a4)
{
  return geom_mat44_c( //
    geom_vec4_x(a1), geom_vec4_y(a1), geom_vec4_z(a1), geom_vec4_w(a1),
    geom_vec4_x(a2), geom_vec4_y(a2), geom_vec4_z(a2), geom_vec4_w(a2),
    geom_vec4_x(a3), geom_vec4_y(a3), geom_vec4_z(a3), geom_vec4_w(a3),
    geom_vec4_x(a4), geom_vec4_y(a4), geom_vec4_z(a4), geom_vec4_w(a4));
}

/**
 * @memberof geom_mat44
 * @brief Create full 4x4 matrix from 4x3 matrix and 4th row vector
 * @param m43 4x3 matrix
 * @praam rv4 4th row vector
 *
 * @return Created matrix
 */
static inline geom_mat44 geom_mat44_c_m43rv4(geom_mat43 m43, geom_vec4 rv4)
{
  return geom_mat44_c_rv(geom_mat43_rowv(m43, 1), geom_mat43_rowv(m43, 2),
                         geom_mat43_rowv(m43, 3), rv4);
}

/**
 * @memberof geom_mat44
 * @brief Create full 4x4 matrix from 4x3 matrix and (0, 0, 0, 1), which is
 *        mathematically implied row.
 * @param m43 4x3 matrix
 *
 * @return Created matrix
 */
static inline geom_mat44 geom_mat44_c_m43(geom_mat43 m43)
{
  return geom_mat44_c_m43rv4(m43, geom_mat43_rowv(m43, 4));
}

/**
 * @memberof geom_mat44
 * @brief Create full 4x4 matrix from 4x3 matrix and 4th row elements
 * @param m43 4x3 matrix
 * @param a41 a41 element
 * @param a42 a42 element
 * @param a43 a43 element
 * @param a44 a44 element
 *
 * @return Created matrix
 */
static inline geom_mat44 geom_mat44_c_m43r4(geom_mat43 m43, double a41,
                                            double a42, double a43, double a44)
{
  return geom_mat44_c_m43rv4(m43, geom_vec4_c(a41, a42, a43, a44));
}

/**
 * @memberof geom_mat44
 * @brief Extract column vector
 *
 * @param m Matrix to extract
 * @param j Index (1 to 4)
 *
 * @return Extracted vector
 */
static inline geom_vec4
geom_mat44_colv(geom_mat44 m, int j)
{
  return geom_vec4_c_v3w(geom_mat43_colv(m.m43, j), m.m4[j - 1]);
}

/**
 * @memberof geom_mat44
 * @brief Extract row vector
 *
 * @param m Matrix to extract
 * @param i Index (1 to 4)
 *
 * @return Extracted vector
 */
static inline geom_vec4
geom_mat44_rowv(geom_mat44 m, int i)
{
  if (i < 4) {
    return geom_mat43_rowv(m.m43, i);
  } else {
    return geom_vec4_c(m.m4[0], m.m4[1], m.m4[2], m.m4[3]);
  }
}

/**
 * @memberof geom_mat44
 * @brief 4x4 Matrix multiplication
 *
 * @param a Left matrix
 * @param b Right matrix
 *
 * @return Result
 */
static inline geom_mat44
geom_mat44_mul(geom_mat44 a, geom_mat44 b)
{
  geom_vec4 a1, a2, a3, a4;
  geom_vec4 b1, b2, b3, b4;
  double m11, m12, m13, m14;
  double m21, m22, m23, m24;
  double m31, m32, m33, m34;
  double m41, m42, m43, m44;
  a1 = geom_mat44_rowv(a, 1);
  a2 = geom_mat44_rowv(a, 2);
  a3 = geom_mat44_rowv(a, 3);
  a4 = geom_mat44_rowv(a, 4);
  b1 = geom_mat44_colv(b, 1);
  b2 = geom_mat44_colv(b, 2);
  b3 = geom_mat44_colv(b, 3);
  b4 = geom_mat44_colv(b, 4);
  m11 = geom_vec4_inner_prod(a1, b1);
  m12 = geom_vec4_inner_prod(a1, b2);
  m13 = geom_vec4_inner_prod(a1, b3);
  m14 = geom_vec4_inner_prod(a1, b4);
  m21 = geom_vec4_inner_prod(a2, b1);
  m22 = geom_vec4_inner_prod(a2, b2);
  m23 = geom_vec4_inner_prod(a2, b3);
  m24 = geom_vec4_inner_prod(a2, b4);
  m31 = geom_vec4_inner_prod(a3, b1);
  m32 = geom_vec4_inner_prod(a3, b2);
  m33 = geom_vec4_inner_prod(a3, b3);
  m34 = geom_vec4_inner_prod(a3, b4);
  m41 = geom_vec4_inner_prod(a4, b1);
  m42 = geom_vec4_inner_prod(a4, b2);
  m43 = geom_vec4_inner_prod(a4, b3);
  m44 = geom_vec4_inner_prod(a4, b4);
  return geom_mat44_c(m11, m12, m13, m14, m21, m22, m23, m24,
                      m31, m32, m33, m34, m41, m42, m43, m44);
}

/**
 * @memberof geom_mat44
 * @brief Extract mat43 from mat44
 * @param m Matrix to extract
 * @return Result
 *
 * The mathematical meaning of this function is setting 4th row into
 * (0, 0, 0, 1). You may use this function if you can assume that.
 */
static inline geom_mat43
geom_mat44_submat43(geom_mat44 m)
{
  return m.m43;
}

/**
 * @memberof geom_mat44
 * @brief Multiply matrix a into (column) vector v from left.
 * @param a matrix
 * @param v (column) vector
 * @return result (column) vector.
 */
static inline geom_vec4
geom_mat44_mul_column_vec(geom_mat44 m, geom_vec4 v)
{
  geom_vec4 a1, a2, a3, a4;
  double x, y, z, w;
  a1 = geom_mat44_rowv(m, 1);
  a2 = geom_mat44_rowv(m, 2);
  a3 = geom_mat44_rowv(m, 3);
  a4 = geom_mat44_rowv(m, 4);
  x = geom_vec4_inner_prod(a1, v);
  y = geom_vec4_inner_prod(a2, v);
  z = geom_vec4_inner_prod(a3, v);
  w = geom_vec4_inner_prod(a4, v);
  return geom_vec4_c(x, y, z, w);
}

/**
 * @memberof geom_mat44
 * @brief transpose matrix
 * @param m Matrix
 * @return Result matrix
 */
static inline geom_mat44
geom_mat44_transpose(geom_mat44 m)
{
  return geom_mat44_c_cv(geom_mat44_rowv(m, 1), geom_mat44_rowv(m, 2),
                         geom_mat44_rowv(m, 3), geom_mat44_rowv(m, 4));
}

/**
 * @memberof geom_mat43
 * @brief transpose matrix
 * @param m Matrix
 * @return Result matrix (4x4, with implied column)
 *
 * @note This function is defined in mat44.h, since the result needs
 *       4x4 matrix
 */
static inline geom_mat44 geom_mat43_transpose(geom_mat43 m)
{
  return geom_mat44_transpose(geom_mat44_c_m43(m));
}

/**
 * @memberof geom_quart
 * @brief Convert quarternion rotation to 4x4 matrix
 *
 * @param q Input quarternion
 *
 * @return result
 *
 * Only if q is normalized (\f$|\boldsymbol q| = 1\f$), the 4-th row
 * will gets (0, 0, 0, 1).
 *
 * This function uses 4x4 matrix and is defined in mat44.h
 *
 * 4x4 matrix method found here:
 * http://www.euclideanspace.com/maths/geometry/rotations/conversions/quaternionToMatrix/jay.htm
 *
 * The description of 4x4 matrix multiplication notation in above
 * referenced page seems to have an error; the multiplication \f$AB\f$
 * where
 *
 * \f[
 *   A = \begin{pmatrix}
 *         q_w &  q_z & -q_y &  q_x \\
 *        -q_z &  q_w &  q_x &  q_y \\
 *         q_y & -q_x &  q_w &  q_z \\
 *        -q_x & -q_y & -q_z &  q_w \\
 *       \end{pmatrix}, \quad
 *   B = \begin{pmatrix}
 *         q_w &  q_z & -q_y & -q_x \\
 *        -q_z &  q_w &  q_x & -q_y \\
 *         q_y & -q_x &  q_w & -q_z \\
 *         q_x &  q_y &  q_z &  q_w \\
 *       \end{pmatrix}
 * \f]
 *
 * will not equal to expanded version, and I found that transposed
 * matrix \f$(AB)^T\f$ is the correct answer. So we use \f$B^T A^T\f$
 * instead.
 *
 * Further explnation is here:
 *   https://en.wikipedia.org/wiki/Quaternions_and_spatial_rotation
 */
static inline geom_mat44
geom_quat_to_mat44(geom_quat q)
{
  double qx, qy, qz, qw;
  geom_mat44 a, b;
  qx = geom_quat_x(q);
  qy = geom_quat_y(q);
  qz = geom_quat_z(q);
  qw = geom_quat_w(q);
  a = geom_mat44_c(+qw, -qz, +qy, +qx,
                   +qz, +qw, -qx, +qy,
                   -qy, +qx, +qw, +qz,
                   +qx, +qy, +qz, +qw);
  b = geom_mat44_c(+qw, -qz, +qy, -qx,
                   +qz, +qw, -qx, -qy,
                   -qy, +qx, +qw, -qz,
                   +qx, +qy, +qz, +qw);
  return geom_mat44_mul(a, b);
}

/**
 * @memberof geom_quat
 * @brief Convert quaternion rotation to 4x3 matrix
 * @param q Input quaternion
 *
 * @return result
 *
 * This function normalize quaternion,
 *
 * This function uses 4x4 matrix and is defined in mat44.h.
 */
static inline geom_mat43
geom_quat_to_mat43(geom_quat q)
{
  double n;
  geom_mat44 m;
  n = geom_quat_norm(q);
  q = geom_quat_factor(q, 1.0 / n);
  /* 4th row is (0, 0, 0, 1) now iff q is normalized */
  m = geom_quat_to_mat44(q);

  return geom_mat44_submat43(m);
}

JUPITER_GEOMETRY_DECL_END

#endif
