
#ifndef JUPITER_GEOMETRY_MAT33_H
#define JUPITER_GEOMETRY_MAT33_H

#include "defs.h"
#include "vector.h"
#include "mat22.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @ingroup Geometry
 * @brief 3x3 matrix
 *
 * Each elements are stored in row-major order.
 * (i.e., a11 -> a[0][0], a21 -> a[1][0], ...)
 */
struct geom_mat33
{
  double a[3][3];
};

/*
 * Recent compiler can optimize these verbosely implemented
 * functions, so readablity has priority.
 */

/**
 * @memberof geom_mat33
 * @brief Create 3x3 matrix
 * @param a11 a11 value
 * @param a12 a12 value
 * @param a13 a13 value
 * @param a21 a21 value
 * @param a22 a22 value
 * @param a23 a23 value
 * @param a31 a31 value
 * @param a32 a32 value
 * @param a33 a33 value
 * @return Created matrix
 */
static inline geom_mat33
geom_mat33_c(double a11, double a12, double a13,
             double a21, double a22, double a23,
             double a31, double a32, double a33)
{
  geom_mat33 m = { .a = {
      { a11, a12, a13 },
      { a21, a22, a23 },
      { a31, a32, a33 },
    } };
  return m;
}

/**
 * @memberof geom_mat33
 * @brief Create 3x3 matrix with three column vector
 * @param a1 Vector for first column
 * @param a2 Vector for second column
 * @param a2 Vector for third column
 *
 * @return Created matrix
 */
static inline geom_mat33
geom_mat33_c_cv(geom_vec3 a1, geom_vec3 a2, geom_vec3 a3)
{
  return geom_mat33_c(geom_vec3_x(a1), geom_vec3_x(a2), geom_vec3_x(a3),
                      geom_vec3_y(a1), geom_vec3_y(a2), geom_vec3_y(a3),
                      geom_vec3_z(a1), geom_vec3_z(a2), geom_vec3_z(a3));
}

/**
 * @memberof geom_mat33
 * @brief Create 3x3 matrix with three row vector
 * @param a1 Vector for first row
 * @param a2 Vector for second row
 * @param a2 Vector for third row
 *
 * @return Created matrix
 */
static inline geom_mat33 geom_mat33_c_rv(geom_vec3 a1, geom_vec3 a2,
                                         geom_vec3 a3)
{
  return geom_mat33_c(geom_vec3_x(a1), geom_vec3_y(a1), geom_vec3_z(a1),
                      geom_vec3_x(a2), geom_vec3_y(a2), geom_vec3_z(a2),
                      geom_vec3_x(a3), geom_vec3_y(a3), geom_vec3_z(a3));
}

/**
 * @memberof geom_mat33
 * @brief Identity matrix for 3x3 matrix
 * @return 3x3 Identity matrix
 */
static inline geom_mat33
geom_mat33_E(void)
{
  return geom_mat33_c(1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0);
}

/**
 * @memberof geom_mat33
 * @brief Get address of specified item
 * @param m matrix
 * @param i row index
 * @param j column index
 * @return pointer to specified element.
 *
 * see geom_mat22_addr for optimization advise.
 */
static inline double *
geom_mat33_addr(geom_mat33 *m, int i, int j)
{
  return &(m->a[i - 1][j - 1]);
}

/**
 * @memberof geom_mat33
 * @brief Get a11 value.
 * @param m matrix
 * @return a11 value
 */
static inline double
geom_mat33_a11(geom_mat33 m)
{
  return *geom_mat33_addr(&m, 1, 1);
}

/**
 * @memberof geom_mat33
 * @brief Get a12 value.
 * @param m matrix
 * @return a12 value
 */
static inline double
geom_mat33_a12(geom_mat33 m)
{
  return *geom_mat33_addr(&m, 1, 2);
}

/**
 * @memberof geom_mat33
 * @brief Get a13 value.
 * @param m matrix
 * @return a13 value
 */
static inline double
geom_mat33_a13(geom_mat33 m)
{
  return *geom_mat33_addr(&m, 1, 3);
}

/**
 * @memberof geom_mat33
 * @brief Get a21 value.
 * @param m matrix
 * @return a21 value
 */
static inline double
geom_mat33_a21(geom_mat33 m)
{
  return *geom_mat33_addr(&m, 2, 1);
}

/**
 * @memberof geom_mat33
 * @brief Get a22 value.
 * @param m matrix
 * @return a22 value
 */
static inline double
geom_mat33_a22(geom_mat33 m)
{
  return *geom_mat33_addr(&m, 2, 2);
}

/**
 * @memberof geom_mat33
 * @brief Get a23 value.
 * @param m matrix
 * @return a23 value
 */
static inline double
geom_mat33_a23(geom_mat33 m)
{
  return *geom_mat33_addr(&m, 2, 3);
}

/**
 * @memberof geom_mat33
 * @brief Get a31 value.
 * @param m matrix
 * @return a31 value
 */
static inline double
geom_mat33_a31(geom_mat33 m)
{
  return *geom_mat33_addr(&m, 3, 1);
}

/**
 * @memberof geom_mat33
 * @brief Get a32 value.
 * @param m matrix
 * @return a32 value
 */
static inline double
geom_mat33_a32(geom_mat33 m)
{
  return *geom_mat33_addr(&m, 3, 2);
}

/**
 * @memberof geom_mat33
 * @brief Get a33 value.
 * @param m matrix
 * @return a11 value
 */
static inline double
geom_mat33_a33(geom_mat33 m)
{
  return *geom_mat33_addr(&m, 3, 3);
}

/**
 * @memberof geom_mat33
 * @brief Set value by element.
 * @param m matrix to set
 * @param i row index
 * @param j column index
 * @param v value to set.
 */
static inline void
geom_mat33_set(geom_mat33 *m, int i, int j, double v)
{
  if (i < 1 || i > 3 || j < 1 || j > 3) return;
  *geom_mat33_addr(m, i, j) = v;
}

/**
 * @memberof geom_mat33
 * @brief Get column vector of matrix
 * @paarm m matrix
 * @param j column index to generate
 * @return created vector
 */
static inline geom_vec3
geom_mat33_colv(geom_mat33 m, int j)
{
  return geom_vec3_c(*geom_mat33_addr(&m, 1, j),
                     *geom_mat33_addr(&m, 2, j),
                     *geom_mat33_addr(&m, 3, j));
}

/**
 * @memberof geom_mat33
 * @brief Get row vector of matrix
 * @paarm m matrix
 * @param j column index to generate
 * @return created vector
 */
static inline geom_vec3
geom_mat33_rowv(geom_mat33 m, int i)
{
  return geom_vec3_c(*geom_mat33_addr(&m, i, 1),
                     *geom_mat33_addr(&m, i, 2),
                     *geom_mat33_addr(&m, i, 3));
}

/**
 * @memberof geom_mat33
 * @brief Equality of matrices
 * @param a matrix to compare
 * @param b matrix to compare
 * @return 1 if true, 0 if false.
 */
static inline int
geom_mat33_eql(geom_mat33 a, geom_mat33 b)
{
  geom_vec3 a1, a2, a3;
  geom_vec3 b1, b2, b3;
  a1 = geom_mat33_rowv(a, 1);
  a2 = geom_mat33_rowv(a, 2);
  a3 = geom_mat33_rowv(a, 3);
  b1 = geom_mat33_rowv(b, 1);
  b2 = geom_mat33_rowv(b, 2);
  b3 = geom_mat33_rowv(b, 3);
  return geom_vec3_eql(a1, b1) && geom_vec3_eql(a2, b2) && geom_vec3_eql(a3, b3);
}

/**
 * @memberof geom_mat33
 * @brief Calculate determinant of matrix
 * @param m 3x3 matrix
 * @return calculated value
 *
 * Sarrus's rule is used.
 */
static inline double
geom_mat33_det(geom_mat33 m)
{
  return
    geom_mat33_a11(m) * geom_mat33_a22(m) * geom_mat33_a33(m) +
    geom_mat33_a12(m) * geom_mat33_a23(m) * geom_mat33_a31(m) +
    geom_mat33_a13(m) * geom_mat33_a21(m) * geom_mat33_a32(m) -
    geom_mat33_a13(m) * geom_mat33_a22(m) * geom_mat33_a31(m) -
    geom_mat33_a12(m) * geom_mat33_a21(m) * geom_mat33_a33(m) -
    geom_mat33_a11(m) * geom_mat33_a23(m) * geom_mat33_a32(m);
}

/**
 * @memberof geom_mat33
 * @brief Multiply matrix a into (column) vector v from left.
 * @param a matrix
 * @param v (column) vector
 * @return result (column) vector.
 */
static inline geom_vec3
geom_mat33_mul_column_vec(geom_mat33 a, geom_vec3 v)
{
  geom_vec3 a1, a2, a3;
  double x, y, z;
  a1 = geom_mat33_rowv(a, 1);
  a2 = geom_mat33_rowv(a, 2);
  a3 = geom_mat33_rowv(a, 3);
  x = geom_vec3_inner_prod(a1, v);
  y = geom_vec3_inner_prod(a2, v);
  z = geom_vec3_inner_prod(a3, v);
  return geom_vec3_c(x, y, z);
}

/**
 * @memberof geom_mat33
 * @brief Multiply two matrices
 * @param a left matrix
 * @param b right matrix
 * @return result matrix
 */
static inline geom_mat33
geom_mat33_mul(geom_mat33 a, geom_mat33 b)
{
  geom_vec3 a1, a2, a3;
  geom_vec3 b1, b2, b3;
  double a11, a12, a13, a21, a22, a23, a31, a32, a33;
  a1 = geom_mat33_rowv(a, 1);
  a2 = geom_mat33_rowv(a, 2);
  a3 = geom_mat33_rowv(a, 3);
  b1 = geom_mat33_colv(b, 1);
  b2 = geom_mat33_colv(b, 2);
  b3 = geom_mat33_colv(b, 3);
  a11 = geom_vec3_inner_prod(a1, b1);
  a12 = geom_vec3_inner_prod(a1, b2);
  a13 = geom_vec3_inner_prod(a1, b3);
  a21 = geom_vec3_inner_prod(a2, b1);
  a22 = geom_vec3_inner_prod(a2, b2);
  a23 = geom_vec3_inner_prod(a2, b3);
  a31 = geom_vec3_inner_prod(a3, b1);
  a32 = geom_vec3_inner_prod(a3, b2);
  a33 = geom_vec3_inner_prod(a3, b3);
  return geom_mat33_c(a11, a12, a13, a21, a22, a23, a31, a32, a33);
}

/**
 * @memberof geom_mat33
 * @brief scalar multiplication to m
 * @param m matrix
 * @param f scalar value to multiply
 * @return result matrix
 */
static inline geom_mat33
geom_mat33_factor(geom_mat33 m, double f)
{
  return geom_mat33_c(
    f * geom_mat33_a11(m), f * geom_mat33_a12(m), f * geom_mat33_a13(m),
    f * geom_mat33_a21(m), f * geom_mat33_a22(m), f * geom_mat33_a23(m),
    f * geom_mat33_a31(m), f * geom_mat33_a32(m), f * geom_mat33_a33(m));
}

/**
 * @memberof geom_mat33
 * @brief 2x2 Submatrix extraction
 * @param m matrix
 * @param i first row index
 * @param j second row index
 * @param k first column index
 * @param l second column index
 * @return extracted matrix
 *
 * `geom_mat33_submat(m, 1, 2, 1, 3)` will be extract following cross
 * points,
 *
 * ```
 *    k       l
 *    v       v
 * / a11 a12 a13 \ <- i
 * | a21 a22 a23 | <- j
 * \ a31 a32 a33 /
 * ```
 *
 * and resulting,
 *
 * ```
 * / a11 a13 \
 * \ a21 a23 /
 * ```
 */
static inline geom_mat22
geom_mat33_submat(geom_mat33 m, int i, int j, int k, int l)
{
  return geom_mat22_c(*geom_mat33_addr(&m, i, k),
                      *geom_mat33_addr(&m, i, l),
                      *geom_mat33_addr(&m, j, k),
                      *geom_mat33_addr(&m, j, l));
}

/**
 * @memberof geom_mat33
 * @brief matrix inversion
 * @param m matrix
 * @return result matrix
 */
static inline geom_mat33
geom_mat33_inv(geom_mat33 m)
{
  geom_mat33 n;
  double m11, m12, m13, m21, m22, m23, m31, m32, m33;
  double d;
  d = geom_mat33_det(m);
  if (d != 0.0) {
    d = 1.0 / d;
    m11 =  geom_mat22_det(geom_mat33_submat(m, 2, 3, 2, 3));
    m21 = -geom_mat22_det(geom_mat33_submat(m, 2, 3, 1, 3));
    m31 =  geom_mat22_det(geom_mat33_submat(m, 2, 3, 1, 2));
    m12 = -geom_mat22_det(geom_mat33_submat(m, 1, 3, 2, 3));
    m22 =  geom_mat22_det(geom_mat33_submat(m, 1, 3, 1, 3));
    m32 = -geom_mat22_det(geom_mat33_submat(m, 1, 3, 1, 2));
    m13 =  geom_mat22_det(geom_mat33_submat(m, 1, 2, 2, 3));
    m23 = -geom_mat22_det(geom_mat33_submat(m, 1, 2, 1, 3));
    m33 =  geom_mat22_det(geom_mat33_submat(m, 1, 2, 1, 2));
    n = geom_mat33_c(m11, m12, m13, m21, m22, m23, m31, m32, m33);
    return geom_mat33_factor(n, d);
  } else {
    d = HUGE_VAL;
    return geom_mat33_c(d, d, d, d, d, d, d, d, d);
  }
}

/**
 * @memberof geom_mat33
 * @brief transpose matrix
 * @param m matrix
 * @return result matrix
 */
static inline geom_mat33
geom_mat33_transpose(geom_mat33 m)
{
  return geom_mat33_c_cv(geom_mat33_rowv(m, 1), geom_mat33_rowv(m, 2),
                         geom_mat33_rowv(m, 3));
}

/**
 * @memberof geom_vec3
 * @brief 3d-world vector split
 * @param p Vector to split
 * @param a first orientation
 * @param b second orientation
 * @param c third orientation
 * @return vector consist with (factor to a, factor to b, factor to c)
 *
 * Compute \f$u\f$, \f$v\f$ and \f$w\f$ which satisfy \f$u\ \vec{a} +
 * v\ \vec{b} + w\ \vec{c} = \vec{p}\f$, and return vector of \f$(u,
 * v, w)\f$.
 *
 * Result value will be INFINITY in following cases:
 *   1. each two of vectors are parallel
 *   2. one of vectors is 0.
 *   3. all vectors lie on a same plane.
 *
 * @note This function uses a matrix inside. Include mat33.h to use
 *       this function.
 */
static inline geom_vec3
geom_vec3_split(geom_vec3 p, geom_vec3 a, geom_vec3 b, geom_vec3 c)
{
  geom_mat33 m;
  m = geom_mat33_c_cv(a, b, c);
  m = geom_mat33_inv(m);
  return geom_mat33_mul_column_vec(m, p);
}

/**
 * @memberof geom_mat33
 * @brief format 3x3 matrix into strings
 * @param buf Buffer to store the result.
 * @param m33 Matrix to format
 * @parma align whether align each columns
 * @param sep Separator for columns (may be ", ")
 * @param parst String to be used for opening (may be "(")
 * @param pared String to be used for closing (may be ")")
 * @param fmt C style format text for each element
 * @param width Width value if `fmt` specifies `*` as width.
 * @param precision Precision value if `fmt` specifies `*` as precision
 * @return Number of bytes written, -1 if errors occured.
 *
 * Stores formatted text of first row into `buf[0]`, second row
 * into `buf[1]` and third row into `buf[2]`.
 *
 * @note The function prototype indicates the semantic above as
 *       denoting `char *buf[3]`, the number of its element will not
 *       be checked by the compiler; you must guaratee this at
 *       runtime.
 *
 * After use, free the allocated memory by `free(buf[0])`. It's not
 * required to call `free(buf[1])`, and must not do
 * (i.e., `free(buf[0])` also dealloctes the region of `buf[1]`).
 *
 * @sa geom_mat22_to_str
 */
JUPITER_GEOMETRY_DECL
int geom_mat33_to_str(char *buf[3], geom_mat33 m33, int align, const char *sep,
                      const char *parst, const char *pared, const char *fmt,
                      int width, int precision);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_mat33(geom_variant *v, geom_mat33 m33);

JUPITER_GEOMETRY_DECL
geom_mat33 geom_variant_get_mat33(const geom_variant *v, geom_error *e);

JUPITER_GEOMETRY_DECL_END

#endif
