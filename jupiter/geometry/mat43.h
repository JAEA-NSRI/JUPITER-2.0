
#ifndef JUPITER_GEOMETRY_MAT43_H
#define JUPITER_GEOMETRY_MAT43_H

#include "defs.h"
#include "vector.h"
#include "mat22.h"
#include "mat33.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @ingroup Geometry
 * @brief 4x3 matrix
 *
 * Each elements are stored in row-major order.
 * (i.e., a11 -> a[0][0], a21 -> a[1][0], ...)
 *
 * Actually, matrix size has 4x4. 4th row is always (0, 0, 0, 1).
 */
struct geom_mat43
{
  double a[3][4];
};

/*
 * Recent compiler can optimize these verbosely implemented
 * functions, so readablity has priority.
 */

/**
 * @memberof geom_mat43
 * @brief Create 4x3 matrix
 * @param a11 a11 value
 * @param a12 a12 value
 * @param a13 a13 value
 * @param a14 a14 value
 * @param a21 a21 value
 * @param a22 a22 value
 * @param a23 a23 value
 * @param a14 a14 value
 * @param a31 a31 value
 * @param a32 a32 value
 * @param a33 a33 value
 * @param a34 a34 value
 * @return Created matrix
 */
static inline geom_mat43
geom_mat43_c(double a11, double a12, double a13, double a14,
             double a21, double a22, double a23, double a24,
             double a31, double a32, double a33, double a34)
{
  geom_mat43 m = { .a = {
      { a11, a12, a13, a14 },
      { a21, a22, a23, a24 },
      { a31, a32, a33, a34 },
    } };
  return m;
}

/**
 * @memberof geom_mat43
 * @brief Create 4x3 matrix with column vectors
 * @param a1 first column vector
 * @param a2 second column vector
 * @param a3 third column vector
 * @param a4 fourth column vector
 * @return Created matrix
 */
static inline geom_mat43
geom_mat43_c_cv(geom_vec3 a1, geom_vec3 a2, geom_vec3 a3, geom_vec3 a4)
{
  return geom_mat43_c( //
    geom_vec3_x(a1), geom_vec3_x(a2), geom_vec3_x(a3), geom_vec3_x(a4),
    geom_vec3_y(a1), geom_vec3_y(a2), geom_vec3_y(a3), geom_vec3_y(a4),
    geom_vec3_z(a1), geom_vec3_z(a2), geom_vec3_z(a3), geom_vec3_z(a4));
}

/**
 * @memberof geom_mat43
 * @brief Create 4x3 matrix with row vectors
 * @param a1 first row vector
 * @param a2 second row vector
 * @param a3 third row vector
 * @return Created matrix
 */
static inline geom_mat43
geom_mat43_c_rv(geom_vec4 a1, geom_vec4 a2, geom_vec4 a3)
{
  return geom_mat43_c( //
    geom_vec4_x(a1), geom_vec4_y(a1), geom_vec4_z(a1), geom_vec4_w(a1),
    geom_vec4_x(a2), geom_vec4_y(a2), geom_vec4_z(a2), geom_vec4_w(a2),
    geom_vec4_x(a3), geom_vec4_y(a3), geom_vec4_z(a3), geom_vec4_w(a3));
}

/**
 * @memberof geom_mat43
 * @brief Identity matrix for 4x3 matrix (4-th row hidden)
 * @return 4x3 Identity matrix
 */
static inline geom_mat43
geom_mat43_E(void)
{
  return geom_mat43_c(1.0, 0.0, 0.0, 0.0,
                      0.0, 1.0, 0.0, 0.0,
                      0.0, 0.0, 1.0, 0.0);
}

/**
 * @memberof geom_mat43
 * @brief Get address of specified item
 * @param m matrix
 * @param i row index
 * @param j column index
 * @return pointer to specified element.
 *
 * see geom_mat22_addr for optimization advise.
 *
 * This function supports getting 4-th row value, but storing to 4-th
 * row will cause an undefined behavior.
 */
static inline double *
geom_mat43_addr(geom_mat43 *m, int i, int j)
{
  static const double v0 = 0.0;
  static const double v1 = 1.0;
  if (i == 4) {
    if (j == 4) {
      return (double *)&v1;
    }
    return (double *)&v0;
  }
  return &(m->a[i - 1][j - 1]);
}

/**
 * @memberof geom_mat43
 * @brief Get a11 value.
 * @param m matrix
 * @return a11 value
 */
static inline double
geom_mat43_a11(geom_mat43 m)
{
  return *geom_mat43_addr(&m, 1, 1);
}

/**
 * @memberof geom_mat43
 * @brief Get a12 value.
 * @param m matrix
 * @return a12 value
 */
static inline double
geom_mat43_a12(geom_mat43 m)
{
  return *geom_mat43_addr(&m, 1, 2);
}

/**
 * @memberof geom_mat43
 * @brief get a13 value.
 * @param m matrix
 * @return a13 value
 */
static inline double
geom_mat43_a13(geom_mat43 m)
{
  return *geom_mat43_addr(&m, 1, 3);
}

/**
 * @memberof geom_mat43
 * @brief Get a14 value.
 * @param m matrix
 * @return a14 value
 */
static inline double
geom_mat43_a14(geom_mat43 m)
{
  return *geom_mat43_addr(&m, 1, 4);
}

/**
 * @memberof geom_mat43
 * @brief Get a21 value.
 * @param m matrix
 * @return a21 value
 */
static inline double
geom_mat43_a21(geom_mat43 m)
{
  return *geom_mat43_addr(&m, 2, 1);
}

/**
 * @memberof geom_mat43
 * @brief Get a22 value.
 * @param m matrix
 * @return a22 value
 */
static inline double
geom_mat43_a22(geom_mat43 m)
{
  return *geom_mat43_addr(&m, 2, 2);
}

/**
 * @memberof geom_mat43
 * @brief Get a23 value.
 * @param m matrix
 * @return a23 value
 */
static inline double
geom_mat43_a23(geom_mat43 m)
{
  return *geom_mat43_addr(&m, 2, 3);
}

/**
 * @memberof geom_mat43
 * @brief Get a24 value.
 * @param m matrix
 * @return a24 value
 */
static inline double
geom_mat43_a24(geom_mat43 m)
{
  return *geom_mat43_addr(&m, 2, 4);
}

/**
 * @memberof geom_mat43
 * @brief Get a31 value.
 * @param m matrix
 * @return a31 value
 */
static inline double
geom_mat43_a31(geom_mat43 m)
{
  return *geom_mat43_addr(&m, 3, 1);
}

/**
 * @memberof geom_mat43
 * @brief Get a32 value.
 * @param m matrix
 * @return a32 value
 */
static inline double
geom_mat43_a32(geom_mat43 m)
{
  return *geom_mat43_addr(&m, 3, 2);
}

/**
 * @memberof geom_mat43
 * @brief Get a33 value.
 * @param m matrix
 * @return a33 value
 */
static inline double
geom_mat43_a33(geom_mat43 m)
{
  return *geom_mat43_addr(&m, 3, 3);
}

/**
 * @memberof geom_mat43
 * @brief Get a34 value.
 * @param m matrix
 * @return a34 value
 */
static inline double
geom_mat43_a34(geom_mat43 m)
{
  return *geom_mat43_addr(&m, 3, 4);
}

/**
 * @memberof geom_mat43
 * @brief Get a41 value.
 * @param m matrix
 * @return a41 value
 *
 * This function always returns 0.
 *
 * The purpose of this function is to write very meaningful source
 * codes;
 *
 *     a11 = geom_mat43_a11(m);
 *     a21 = geom_mat43_a21(m);
 *     a31 = geom_mat43_a31(m);
 *     a41 = geom_mat43_a41(m);
 *
 * etc.
 */
static inline double
geom_mat43_a41(geom_mat43 m)
{
  return 0.0;
}

/**
 * @memberof geom_mat43
 * @brief Get a42 value.
 * @param m matrix
 * @return a42 value (0)
 *
 * This function always returns 0.
 *
 * @sa geom_mat43_a41
 */
static inline double
geom_mat43_a42(geom_mat43 m)
{
  return 0.0;
}

/**
 * @memberof geom_mat43
 * @brief Get a43 value.
 * @param m matrix
 * @return a43 value (0)
 *
 * This function always returns 0.
 *
 * @sa geom_mat43_a41
 */
static inline double
geom_mat43_a43(geom_mat43 m)
{
  return 0.0;
}

/**
 * @memberof geom_mat43
 * @brief Get a44 value.
 * @param m matrix
 * @return a44 value (1)
 *
 * This function always returns 1.
 *
 * @sa geom_mat43_a41
 */
static inline double
geom_mat43_a44(geom_mat43 m)
{
  return 1.0;
}

/**
 * @memberof geom_mat43
 * @brief Set value by element.
 * @param m matrix to set
 * @param i row index
 * @param j column index
 * @param v value to set.
 */
static inline void
geom_mat43_set(geom_mat43 *m, int i, int j, double v)
{
  if (i < 1 || i > 3 || j < 1 || j > 4) return;
  *geom_mat43_addr(m, i, j) = v;
}

/**
 * @memberof geom_mat43
 * @brief Get column vector of matrix (exclude 4-th row)
 * @param m matrix
 * @param j column index to generate
 * @return created vector
 *
 * Element of 4-th row is omitted.
 */
static inline geom_vec3
geom_mat43_colv(geom_mat43 m, int j)
{
  return geom_vec3_c(*geom_mat43_addr(&m, 1, j),
                     *geom_mat43_addr(&m, 2, j),
                     *geom_mat43_addr(&m, 3, j));
}

/**
 * @memberof geom_mat43
 * @brief Get column vector of matrix (with 4-th row)
 * @param m matrix
 * @param j column index to generate
 * @return created vector
 */
static inline geom_vec4
geom_mat43_colv4(geom_mat43 m, int j)
{
  return geom_vec4_c_v3w(geom_mat43_colv(m, j),
                         *geom_mat43_addr(&m, 4, j));
}

/**
 * @memberof geom_mat43
 * @brief Get row vector of matrix
 * @param m matrix
 * @param j column index to generate
 * @return created vector
 */
static inline geom_vec4
geom_mat43_rowv(geom_mat43 m, int i)
{
  return geom_vec4_c(*geom_mat43_addr(&m, i, 1),
                     *geom_mat43_addr(&m, i, 2),
                     *geom_mat43_addr(&m, i, 3),
                     *geom_mat43_addr(&m, i, 4));
}

/**
 * @memberof geom_mat43
 * @brief Equality of matrices
 * @param a matrix to compare
 * @param b matrix to compare
 * @return 1 if true, 0 if false.
 */
static inline int
geom_mat43_eql(geom_mat43 a, geom_mat43 b)
{
  geom_vec4 a1, a2, a3;
  geom_vec4 b1, b2, b3;
  a1 = geom_mat43_rowv(a, 1);
  a2 = geom_mat43_rowv(a, 2);
  a3 = geom_mat43_rowv(a, 3);
  b1 = geom_mat43_rowv(b, 1);
  b2 = geom_mat43_rowv(b, 2);
  b3 = geom_mat43_rowv(b, 3);
  return geom_vec4_eql(a1, b1) && geom_vec4_eql(a2, b2) && geom_vec4_eql(a3, b3);
}

/**
 * @memberof geom_mat43
 * @brief 2x2 Submatrix extraction
 * @param m matrix
 * @param i first row index
 * @param j second row index
 * @param k first column index
 * @param l second column index
 * @return extracted matrix
 *
 * `geom_mat43_submat22(m, 1, 2, 1, 3)` will be extract following cross
 * points,
 *
 *
 *        k       l
 *        v       v
 *     / a11 a12 a13 a14 \ <- i
 *     | a21 a22 a23 a24 | <- j
 *     | a31 a32 a33 a34 |
 *     \  0   0   0   1  /
 *
 *
 * and resulting,
 *
 *
 *     / a11 a13 \
 *     \ a21 a23 /
 *
 */
static inline geom_mat22
geom_mat43_submat22(geom_mat43 a, int i, int j, int k, int l)
{
  return geom_mat22_c(*geom_mat43_addr(&a, i, k),
                      *geom_mat43_addr(&a, i, l),
                      *geom_mat43_addr(&a, j, k),
                      *geom_mat43_addr(&a, j, l));
}

/**
 * @memberof geom_mat43
 * @brief 3x3 Submatrix extraction
 * @param m matrix
 * @param i first row index
 * @param j second row index
 * @param k third row index
 * @param l first column index
 * @param m second column index
 * @param n third column index
 * @return extracted matrix
 *
 * `geom_mat43_submat33(m, 1, 2, 3, 1, 3, 4)` will be extract
 * following cross points,
 *
 *        l       m   n
 *        v       v   v
 *     / a11 a12 a13 a14 \ <- i
 *     | a21 a22 a23 a24 | <- j
 *     | a31 a32 a33 a34 | <- k
 *     \  0   0   0   1  /
 *
 * and resulting,
 *
 *     / a11 a13 a14 \
 *     | a21 a23 a24 |
 *     \ a31 a33 a34 /
 */
static inline geom_mat33
geom_mat43_submat33(geom_mat43 a, int i, int j, int k, int l, int m, int n)
{
  return geom_mat33_c(*geom_mat43_addr(&a, i, l),
                      *geom_mat43_addr(&a, i, m),
                      *geom_mat43_addr(&a, i, n),
                      *geom_mat43_addr(&a, j, l),
                      *geom_mat43_addr(&a, j, m),
                      *geom_mat43_addr(&a, j, n),
                      *geom_mat43_addr(&a, k, l),
                      *geom_mat43_addr(&a, k, m),
                      *geom_mat43_addr(&a, k, n));
}

/**
 * @memberof geom_mat43
 * @brief Calculate determinant of matrix
 * @param m 4x3 matrix
 * @return calculated value
 *
 * Since 4-th row is \f$(0, 0, 0, 1)\f$, result is equivalent to
 * determinant of 3x3 upper left matrix (C).
 *
 * \f[
 *   |M| = |C||1|, \quad
 *   C = \begin{pmatrix}
 *        a_{11} & a_{12} & a_{13} \\
 *        a_{21} & a_{22} & a_{23} \\
 *        a_{31} & a_{32} & a_{33} \\
 *       \end{pmatrix}
 *
 * %     / a11 a12 a13 \
 * % C = | a21 a22 a23 |
 * %     \ a31 a32 a33 /
 * \f]
 */
static inline double
geom_mat43_det(geom_mat43 m)
{
  return geom_mat33_det(geom_mat43_submat33(m, 1, 2, 3, 1, 2, 3));
}

/**
 * @memberof geom_mat43
 * @brief Multiply matrix a into (column) vector v from left.
 * @param a matrix
 * @param v (column) vector
 * @return result (column) vector.
 */
static inline geom_vec4
geom_mat43_mul_column_vec4(geom_mat43 a, geom_vec4 v)
{
  geom_vec4 a1, a2, a3, a4;
  double x, y, z, w;
  a1 = geom_mat43_rowv(a, 1);
  a2 = geom_mat43_rowv(a, 2);
  a3 = geom_mat43_rowv(a, 3);
  a4 = geom_mat43_rowv(a, 4); /* (0, 0, 0, 1) */
  x = geom_vec4_inner_prod(a1, v);
  y = geom_vec4_inner_prod(a2, v);
  z = geom_vec4_inner_prod(a3, v);
  w = geom_vec4_inner_prod(a4, v);
  return geom_vec4_c(x, y, z, w); /* w will be unchanged so. */
}

/**
 * @memberof geom_mat43
 * @brief Multiply matrix a into (column) vector v from left.
 *        (w is assumed 1)
 * @param a matrix
 * @param v (column) vector
 * @return result (column) vector.
 */
static inline geom_vec3
geom_mat43_mul_column_vec3(geom_mat43 a, geom_vec3 v)
{
  geom_vec4 a1, a2, a3, v4;
  double x, y, z;
  a1 = geom_mat43_rowv(a, 1);
  a2 = geom_mat43_rowv(a, 2);
  a3 = geom_mat43_rowv(a, 3);
  v4 = geom_vec4_c_v3w(v, 1.0);
  x = geom_vec4_inner_prod(a1, v4);
  y = geom_vec4_inner_prod(a2, v4);
  z = geom_vec4_inner_prod(a3, v4);
  return geom_vec3_c(x, y, z);
}

/**
 * @memberof geom_mat43
 * @brief Multiply two matrices
 * @param a left matrix
 * @param b right matrix
 * @return result matrix
 */
static inline geom_mat43
geom_mat43_mul(geom_mat43 a, geom_mat43 b)
{
  geom_mat33 c1, c2;
  geom_vec4  a1, a2, a3, d;
  double a11, a12, a13, a14, a21, a22, a23, a24, a31, a32, a33, a34;
  c1 = geom_mat43_submat33(a, 1, 2, 3, 1, 2, 3);
  c2 = geom_mat43_submat33(b, 1, 2, 3, 1, 2, 3);
  c1 = geom_mat33_mul(c1, c2);
  a1 = geom_mat43_rowv(a, 1);
  a2 = geom_mat43_rowv(a, 2);
  a3 = geom_mat43_rowv(a, 3);
  d = geom_mat43_colv4(b, 4);
  a11 = geom_mat33_a11(c1);
  a12 = geom_mat33_a12(c1);
  a13 = geom_mat33_a13(c1);
  a14 = geom_vec4_inner_prod(a1, d);
  a21 = geom_mat33_a21(c1);
  a22 = geom_mat33_a22(c1);
  a23 = geom_mat33_a23(c1);
  a24 = geom_vec4_inner_prod(a2, d);
  a31 = geom_mat33_a31(c1);
  a32 = geom_mat33_a32(c1);
  a33 = geom_mat33_a33(c1);
  a34 = geom_vec4_inner_prod(a3, d);
  return geom_mat43_c(a11, a12, a13, a14,
                      a21, a22, a23, a24,
                      a31, a32, a33, a34);
}

/**
 * @memberof geom_mat43
 * @brief scalar multiplication to m
 * @param m matrix
 * @param f scalar value to multiply
 * @return result matrix
 */
static inline geom_mat43
geom_mat43_factor(geom_mat43 m, double f)
{
  return geom_mat43_c(
    f * geom_mat43_a11(m), f * geom_mat43_a12(m),
    f * geom_mat43_a13(m), f * geom_mat43_a14(m),

    f * geom_mat43_a21(m), f * geom_mat43_a22(m),
    f * geom_mat43_a23(m), f * geom_mat43_a24(m),

    f * geom_mat43_a31(m), f * geom_mat43_a32(m),
    f * geom_mat43_a33(m), f * geom_mat43_a34(m));
}

/**
 * @memberof geom_mat43
 * @brief matrix inversion
 * @param m matrix
 * @return result matrix
 */
static inline geom_mat43
geom_mat43_inv(geom_mat43 m)
{
  geom_mat43 n;
  double m11, m12, m13, m14, m21, m22, m23, m24, m31, m32, m33, m34;
  double d;
  d = geom_mat43_det(m);
  if (d != 0.0) {
    d = 1.0 / d;
    m11 =  geom_mat22_det(geom_mat43_submat22(m, 2, 3, 2, 3));
    m21 = -geom_mat22_det(geom_mat43_submat22(m, 2, 3, 1, 3));
    m31 =  geom_mat22_det(geom_mat43_submat22(m, 2, 3, 1, 2));
    m12 = -geom_mat22_det(geom_mat43_submat22(m, 1, 3, 2, 3));
    m22 =  geom_mat22_det(geom_mat43_submat22(m, 1, 3, 1, 3));
    m32 = -geom_mat22_det(geom_mat43_submat22(m, 1, 3, 1, 2));
    m13 =  geom_mat22_det(geom_mat43_submat22(m, 1, 2, 2, 3));
    m23 = -geom_mat22_det(geom_mat43_submat22(m, 1, 2, 1, 3));
    m33 =  geom_mat22_det(geom_mat43_submat22(m, 1, 2, 1, 2));
    m14 = -geom_mat33_det(geom_mat43_submat33(m, 1, 2, 3, 2, 3, 4));
    m24 =  geom_mat33_det(geom_mat43_submat33(m, 1, 2, 3, 1, 3, 4));
    m34 = -geom_mat33_det(geom_mat43_submat33(m, 1, 2, 3, 1, 2, 4));
    n = geom_mat43_c(m11, m12, m13, m14,
                     m21, m22, m23, m24,
                     m31, m32, m33, m34);
    return geom_mat43_factor(n, d);
  } else {
    d = HUGE_VAL;
    return geom_mat43_c(d, d, d, d, d, d, d, d, d, d, d, d);
  }
}

JUPITER_GEOMETRY_DECL
int geom_mat43_to_str(char *buf[4], geom_mat43 m43, int align, const char *sep,
                      const char *parst, const char *pared, const char *fmt,
                      int width, int precision);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_mat43(geom_variant *v, geom_mat43 m43);

JUPITER_GEOMETRY_DECL
geom_mat43 geom_variant_get_mat43(const geom_variant *v, geom_error *e);

JUPITER_GEOMETRY_DECL_END

#endif
