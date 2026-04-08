/**
 * @file   mat22.h
 * @ingroup Geometry
 * @brief 2x2 matrix
 */


#ifndef JUPITER_GEOMETRY_MAT22_H
#define JUPITER_GEOMETRY_MAT22_H

#include "defs.h"
#include "geom_math.h"
#include "vector.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @ingroup Geometry
 * @brief 2x2 matrix
 *
 * Each elements are stored in row-major order.
 * (i.e., a11 -> a[0][0], a21 -> a[1][0], ...)
 */
struct geom_mat22
{
  double a[2][2];
};

/*
 * Recent compiler can optimize these verbosely implemented
 * functions, so readablity has priority.
 */

/**
 * @memberof geom_vec2
 * @brief Create 2x2 matrix
 * @param a11 a11 value
 * @param a12 a12 value
 * @param a21 a21 value
 * @param a22 a22 value
 * @return Created matrix
 */
static inline geom_mat22
geom_mat22_c(double a11, double a12, double a21, double a22)
{
  geom_mat22 m = { .a = { {a11, a12}, {a21, a22} } };
  return m;
}

/**
 * @memberof geom_vec2
 * @brief Create 2x2 matrix with column vectors
 * @param a1 vector for first column
 * @param a2 vector for second column
 * @return Created matrix
 */
static inline geom_mat22
geom_mat22_c_cv(geom_vec2 a1, geom_vec2 a2)
{
  return geom_mat22_c(geom_vec2_x(a1), geom_vec2_x(a2), //
                      geom_vec2_y(a1), geom_vec2_y(a2));
}

/**
 * @memberof geom_vec2
 * @brief Create 2x2 matrix with row vectors
 * @param a1 vector for first row
 * @param a2 vector for second row
 * @return Created matrix
 */
static inline geom_mat22
geom_mat22_c_rv(geom_vec2 a1, geom_vec2 a2)
{
  return geom_mat22_c(geom_vec2_x(a1), geom_vec2_y(a1), //
                      geom_vec2_x(a2), geom_vec2_y(a2));
}

/**
 * @memberof geom_vec2
 * @brief Identity matrix of 2x2 matrix
 * @return 2x2 identity matrix
 */
static inline geom_mat22
geom_mat22_E(void)
{
  return geom_mat22_c(1.0, 0.0, 0.0, 1.0);
}

/**
 * @memberof geom_vec2
 * @brief Get address of specified item
 * @param m matrix
 * @param i row index
 * @param j column index
 * @return pointer to specified element.
 *
 * If you use this function, you should use or set the value
 * immediately (in other words, you should not use the pointer for
 * address arithmetic).  Otherwise, it might break the compiler
 * optimization, because the structure content must be laid on the
 * memory in specified order.
 *
 * To improve optimization, range of i and j is not tested. Use
 * `geom_mat22_a??` to get by element and `geom_mat22_set` to set by
 * element.
 */
static inline double *
geom_mat22_addr(geom_mat22 *m, int i, int j)
{
  return &(m->a[i - 1][j - 1]);
}

/**
 * @memberof geom_vec2
 * @brief Get a11 value.
 * @param m matrix
 * @return a11 value
 */
static inline double
geom_mat22_a11(geom_mat22 m)
{
  return *geom_mat22_addr(&m, 1, 1);
}

/**
 * @memberof geom_mat22
 * @brief Get a12 value.
 * @param m matrix
 * @return a12 value
 */
static inline double
geom_mat22_a12(geom_mat22 m)
{
  return *geom_mat22_addr(&m, 1, 2);
}

/**
 * @memberof geom_mat22
 * @brief Get a21 value.
 * @param m matrix
 * @return a21 value
 */
static inline double
geom_mat22_a21(geom_mat22 m)
{
  return *geom_mat22_addr(&m, 2, 1);
}

/**
 * @memberof geom_mat22
 * @brief Get a22 value.
 * @param m matrix
 * @return a22 value
 */
static inline double
geom_mat22_a22(geom_mat22 m)
{
  return *geom_mat22_addr(&m, 2, 2);
}

/**
 * @memberof geom_mat22
 * @brief Set value by element.
 * @param m matrix to set
 * @param i row index
 * @param j column index
 * @param v value to set.
 */
static inline void
geom_mat22_set(geom_mat22 *m, int i, int j, double v)
{
  if (i < 1 || i > 2 || j < 1 || j > 2) return;
  *geom_mat22_addr(m, i, j) = v;
}

/**
 * @memberof geom_mat22
 * @brief Get column vector of matrix
 * @paarm m matrix
 * @param j column index to generate
 * @return created vector
 */
static inline geom_vec2
geom_mat22_colv(geom_mat22 m, int j)
{
  return geom_vec2_c(*geom_mat22_addr(&m, 1, j),
                     *geom_mat22_addr(&m, 2, j));
}

/**
 * @memberof geom_mat22
 * @brief Get row vector of matrix
 * @paarm m matrix
 * @param j column index to generate
 * @return created vector
 */
static inline geom_vec2
geom_mat22_rowv(geom_mat22 m, int i)
{
  return geom_vec2_c(*geom_mat22_addr(&m, i, 1),
                     *geom_mat22_addr(&m, i, 2));
}

/**
 * @memberof geom_mat22
 * @brief Equality of matrices
 * @param a matrix to compare
 * @param b matrix to compare
 * @return 1 if true, 0 if false.
 */
static inline int
geom_mat22_eql(geom_mat22 a, geom_mat22 b)
{
  geom_vec2 a1, a2;
  geom_vec2 b1, b2;
  a1 = geom_mat22_rowv(a, 1);
  a2 = geom_mat22_rowv(a, 2);
  b1 = geom_mat22_rowv(b, 1);
  b2 = geom_mat22_rowv(b, 2);
  return geom_vec2_eql(a1, b1) && geom_vec2_eql(a2, b2);
}

/**
 * @memberof geom_mat22
 * @brief Calculate determinant of matrix
 * @param m 2x2 matrix
 * @return calculated value
 */
static inline double
geom_mat22_det(geom_mat22 m)
{
  return
    geom_mat22_a11(m) * geom_mat22_a22(m) -
    geom_mat22_a12(m) * geom_mat22_a21(m);
}

/**
 * @memberof geom_mat22
 * @brief Multiply matrix a into (column) vector v from left.
 * @param a matrix
 * @param v (column) vector
 * @return result (column) vector.
 */
static inline geom_vec2
geom_mat22_mul_column_vec(geom_mat22 a, geom_vec2 v)
{
  geom_vec2 a1, a2;
  double x, y;
  a1 = geom_mat22_rowv(a, 1);
  a2 = geom_mat22_rowv(a, 2);
  x = geom_vec2_inner_prod(a1, v);
  y = geom_vec2_inner_prod(a2, v);
  return geom_vec2_c(x, y);
}

/**
 * @memberof geom_mat22
 * @brief Calculate multiplication of two matrices
 * @param a left matrix
 * @param b right matrix
 * @return calculated matrix
 */
static inline geom_mat22
geom_mat22_mul(geom_mat22 a, geom_mat22 b)
{
  geom_vec2 a1, a2;
  geom_vec2 b1, b2;
  double a11, a12, a21, a22;
  a1 = geom_mat22_rowv(a, 1);
  a2 = geom_mat22_rowv(a, 2);
  b1 = geom_mat22_colv(b, 1);
  b2 = geom_mat22_colv(b, 2);
  a11 = geom_vec2_inner_prod(a1, b1);
  a12 = geom_vec2_inner_prod(a1, b2);
  a21 = geom_vec2_inner_prod(a2, b1);
  a22 = geom_vec2_inner_prod(a2, b2);
  return geom_mat22_c(a11, a12, a21, a22);
}

/**
 * @memberof geom_mat22
 * @brief Scalar multiplication to matrix
 * @param m matrix
 * @param f factor
 * @return calculated matrix
 */
static inline geom_mat22
geom_mat22_factor(geom_mat22 m, double f)
{
  return geom_mat22_c(f * geom_mat22_a11(m), f * geom_mat22_a12(m),
                      f * geom_mat22_a21(m), f * geom_mat22_a22(m));
}

/**
 * @memberof geom_vec2
 * @brief Matrix inversion
 * @param m matrix
 * @return inversed matrix
 *
 * This function does not make any considerations for singular
 * matrices. In that case, all element will be INFINITY if the system
 * is based on IEEE-754 standard. Or, raises SIGFPE if enabled.
 */
static inline geom_mat22
geom_mat22_inv(geom_mat22 m)
{
  double d;
  geom_mat22 n;

  d = geom_mat22_det(m);
  n = geom_mat22_c(geom_mat22_a22(m), -geom_mat22_a12(m),
                   -geom_mat22_a21(m), geom_mat22_a11(m));
  return geom_mat22_factor(n, 1.0 / d);
}

/**
 * @memberof geom_vec2
 * @brief Transpose matrix
 * @param m matrix
 * @return transposed matrix
 */
static inline geom_mat22
geom_mat22_transpose(geom_mat22 m)
{
  return geom_mat22_c_cv(geom_mat22_rowv(m, 1), geom_mat22_rowv(m, 2));
}

/**
 * @memberof geom_vec2
 * @brief 2d-world vector split
 * @param p Vector to split
 * @param a first orientation
 * @param b second orientation
 * @return vector consist with (factor to a, factor to b)
 *
 * Compute \f$u\f$ and \f$v\f$ which satisfy \f$u\ \vec{a} + v\
 * \vec{b} = \vec{p}\f$, and return vector of \f$(u, v)\f$.
 *
 * Result value will be INFINITY in following cases:
 *   1. vectors are parallel
 *   2. one of vectors is 0.
 *
 * @note This function uses a matrix inside. Include mat22.h to use
 *       this function.
 */
static inline geom_vec2
geom_vec2_split(geom_vec2 p, geom_vec2 a, geom_vec2 b)
{
  geom_mat22 m;
  m = geom_mat22_c_cv(a, b);
  m = geom_mat22_inv(m);
  return geom_mat22_mul_column_vec(m, p);
}

/**
 * @brief Return rotation matrix for 2D world
 * @param angle_deg Rotation angle in degrees
 * @return result matrix
 */
static inline geom_mat22 geom_mat22_rotation(double angle_deg)
{
  return geom_mat22_c(geom_cosd(angle_deg), -geom_sind(angle_deg),
                      geom_sind(angle_deg), geom_cosd(angle_deg));
}

/**
 * @brief Return rotatation matrix that rotates 90 degrees in 2D world
 * @return result matrix
 */
static inline geom_mat22 geom_mat22_rotate90(void)
{
  return geom_mat22_c(0.0, -1.0, 1.0, 0.0);
}

static inline geom_vec2 geom_vec2_rotate(geom_vec2 v, double angle_deg)
{
  return geom_mat22_mul_column_vec(geom_mat22_rotation(angle_deg), v);
}

static inline geom_vec2 geom_vec2_rotate90(geom_vec2 v)
{
  return geom_mat22_mul_column_vec(geom_mat22_rotate90(), v);
}

/**
 * @memberof geom_mat22
 * @brief format 2x2 matrix into strings
 * @param buf Buffer to store the result.
 * @param m22 Matrix to format
 * @parma align whether align each columns
 * @param sep Separator for columns (may be ", ")
 * @param parst String to be used for opening (may be "(")
 * @param pared String to be used for closing (may be ")")
 * @param fmt C style format text for each element
 * @param width Width value if `fmt` specifies `*` as width.
 * @param precision Precision value if `fmt` specifies `*` as precision
 * @return Number of bytes written, -1 if errors occured.
 *
 * Stores formatted text of first row into `buf[0]` and second row
 * into `buf[1]`.
 *
 * @note The function prototype indicates the semantic above as
 *       denoting `char *buf[2]`, the number of its element will not
 *       be checked by the compiler; you must guaratee this at
 *       runtime.
 *
 * After use, free the allocated memory by `free(buf[0])`. It's not
 * required to call `free(buf[1])`, and must not do
 * (i.e., `free(buf[0])` also dealloctes the region of `buf[1]`).
 *
 * @sa geom_matrix_to_str
 */
JUPITER_GEOMETRY_DECL
int geom_mat22_to_str(char *buf[2], geom_mat22 m22, int align, const char *sep,
                      const char *parst, const char *pared, const char *fmt,
                      int width, int precision);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_mat22(geom_variant *v, geom_mat22 m22);

JUPITER_GEOMETRY_DECL
geom_mat22 geom_variant_get_mat22(const geom_variant *v, geom_error *e);

JUPITER_GEOMETRY_DECL_END

#endif
