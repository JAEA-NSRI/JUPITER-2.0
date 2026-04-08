
#include "common.h"
#include "matrix.h"

int geom_mat22_to_str(char *buf[2], geom_mat22 m22, int align,
                      const char *sep, const char *parst, const char *pared,
                      const char *fmt, int width, int precision)
{
  double d[4];
  d[0] = geom_mat22_a11(m22);
  d[1] = geom_mat22_a12(m22);
  d[2] = geom_mat22_a21(m22);
  d[3] = geom_mat22_a22(m22);
  return geom_matrix_to_str(buf, d, 2, 2, sep, parst, pared, align,
                            fmt, width, precision);
}

int geom_mat33_to_str(char *buf[3], geom_mat33 m33, int align,
                      const char *sep, const char *parst, const char *pared,
                      const char *fmt, int width, int precision)
{
  double d[9];
  d[0] = geom_mat33_a11(m33);
  d[1] = geom_mat33_a12(m33);
  d[2] = geom_mat33_a13(m33);
  d[3] = geom_mat33_a21(m33);
  d[4] = geom_mat33_a22(m33);
  d[5] = geom_mat33_a23(m33);
  d[6] = geom_mat33_a31(m33);
  d[7] = geom_mat33_a32(m33);
  d[8] = geom_mat33_a33(m33);
  return geom_matrix_to_str(buf, d, 3, 3, sep, parst, pared, align,
                            fmt, width, precision);
}

int geom_mat43_to_str(char *buf[4], geom_mat43 m43, int align,
                      const char *sep, const char *parst, const char *pared,
                      const char *fmt, int width, int precision)
{
  double d[16];
  d[ 0] = geom_mat43_a11(m43);
  d[ 1] = geom_mat43_a12(m43);
  d[ 2] = geom_mat43_a13(m43);
  d[ 3] = geom_mat43_a14(m43);
  d[ 4] = geom_mat43_a21(m43);
  d[ 5] = geom_mat43_a22(m43);
  d[ 6] = geom_mat43_a23(m43);
  d[ 7] = geom_mat43_a24(m43);
  d[ 8] = geom_mat43_a31(m43);
  d[ 9] = geom_mat43_a32(m43);
  d[10] = geom_mat43_a33(m43);
  d[11] = geom_mat43_a34(m43);
  d[12] = geom_mat43_a41(m43);
  d[13] = geom_mat43_a42(m43);
  d[14] = geom_mat43_a43(m43);
  d[15] = geom_mat43_a44(m43);
  return geom_matrix_to_str(buf, d, 4, 4, sep, parst, pared, align,
                            fmt, width, precision);
}
