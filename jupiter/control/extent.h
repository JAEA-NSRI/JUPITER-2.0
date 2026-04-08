#ifndef JUPITER_CONTROL_EXTENT_H
#define JUPITER_CONTROL_EXTENT_H

#include "defs.h"
#include "error.h"
#include "overflow.h"

#include <stdint.h>
#include <limits.h>

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_irange
{
  int is, ie;
};

static inline jcntrl_irange jcntrl_irange_c(int is, int ie)
{
  return (jcntrl_irange){is, ie};
}

static inline int jcntrl_irange_empty(jcntrl_irange r) { return r.is >= r.ie; }

static inline int jcntrl_irange_include(jcntrl_irange r, int i)
{
  return r.is <= i && i < r.ie;
}

static inline int jcntrl_irange_n(jcntrl_irange r)
{
  if (!jcntrl_irange_empty(r)) {
    int n;
    if (jcntrl_i_sub_overflow(r.ie, r.is, &n))
      return -1;
    return n;
  }
  return 0;
}

static inline jcntrl_irange jcntrl_irange_overlap(jcntrl_irange x,
                                                  jcntrl_irange y)
{
  int is, ie;
  if (jcntrl_irange_empty(x) || jcntrl_irange_empty(y))
    return jcntrl_irange_c(0, -1);

  is = (x.is < y.is) ? y.is : x.is;
  ie = (x.ie < y.ie) ? x.ie : y.ie;
  return jcntrl_irange_c(is, ie);
}

static inline jcntrl_irange jcntrl_irange_cover(jcntrl_irange x,
                                                jcntrl_irange y)
{
  int is, ie;
  if (jcntrl_irange_empty(x))
    return y;
  if (jcntrl_irange_empty(y))
    return x;

  is = (x.is < y.is) ? x.is : y.is;
  ie = (x.ie < y.ie) ? y.ie : x.ie;
  return jcntrl_irange_c(is, ie);
}

/**
 * structured extent reprsentation.
 */
struct jcntrl_extent
{
  int extent[6];
};

static inline jcntrl_extent jcntrl_extent_empty(void)
{
  return (jcntrl_extent){.extent = {0, -1, 0, -1, 0, -1}};
}

static inline jcntrl_extent jcntrl_extent_c(const int extent[6])
{
  jcntrl_extent e;
  for (int i = 0; i < 6; ++i)
    e.extent[i] = extent[i];
  return e;
}

static inline jcntrl_extent jcntrl_extent_i(int is, int ie, int js, int je,
                                            int ks, int ke, int include_end)
{
  jcntrl_extent e;

  include_end = !!include_end;
  e.extent[0] = is;
  e.extent[1] = ie + include_end;
  e.extent[2] = js;
  e.extent[3] = je + include_end;
  e.extent[4] = ks;
  e.extent[5] = ke + include_end;
  return e;
}

static inline jcntrl_extent jcntrl_extent_r(jcntrl_irange x, jcntrl_irange y,
                                            jcntrl_irange z)
{
  return jcntrl_extent_i(x.is, x.ie, y.is, y.ie, z.is, z.ie, 0);
}

static inline jcntrl_irange jcntrl_extent__range(jcntrl_extent p, int ids,
                                                 int ide)
{
  return jcntrl_irange_c(p.extent[ids], p.extent[ide]);
}

static inline jcntrl_irange jcntrl_extent_xrange(jcntrl_extent p)
{
  return jcntrl_extent__range(p, 0, 1);
}

static inline jcntrl_irange jcntrl_extent_yrange(jcntrl_extent p)
{
  return jcntrl_extent__range(p, 2, 3);
}

static inline jcntrl_irange jcntrl_extent_zrange(jcntrl_extent p)
{
  return jcntrl_extent__range(p, 4, 5);
}

static inline int jcntrl_extent_nx(jcntrl_extent p)
{
  return jcntrl_irange_n(jcntrl_extent_xrange(p));
}

static inline int jcntrl_extent_ny(jcntrl_extent p)
{
  return jcntrl_irange_n(jcntrl_extent_yrange(p));
}

static inline int jcntrl_extent_nz(jcntrl_extent p)
{
  return jcntrl_irange_n(jcntrl_extent_zrange(p));
}

static inline jcntrl_size_type jcntrl_extent_size(jcntrl_extent p)
{
  jcntrl_size_type n;
  int nx, ny, nz;
  nx = jcntrl_extent_nx(p);
  ny = jcntrl_extent_ny(p);
  nz = jcntrl_extent_nz(p);
  n = nx;
  if (jcntrl_s_mul_overflow(n, ny, &n))
    return -1;
  if (jcntrl_s_mul_overflow(n, nz, &n))
    return -1;
  return n;
}

static inline jcntrl_size_type jcntrl_extent_addr(jcntrl_extent p, int i, int j,
                                                  int k)
{
  jcntrl_size_type jj;
  int nx, ny, nz;
  nx = jcntrl_extent_nx(p);
  ny = jcntrl_extent_ny(p);
  nz = jcntrl_extent_nz(p);
  if (nx <= 0 || ny <= 0 || nz <= 0)
    return -1;

  i -= p.extent[0];
  j -= p.extent[2];
  k -= p.extent[4];
  if (i < 0 || j < 0 || k < 0 || i >= nx || j >= ny || k >= nz)
    return -1;

  jj = k;
  if (jcntrl_s_mul_overflow(jj, ny, &jj))
    return -2;
  if (jcntrl_s_add_overflow(jj, j, &jj))
    return -2;
  if (jcntrl_s_mul_overflow(jj, nx, &jj))
    return -2;
  if (jcntrl_s_add_overflow(jj, i, &jj))
    return -2;
  return jj;
}

static inline int jcntrl_extent_index(jcntrl_extent p, jcntrl_size_type addr,
                                      int *i, int *j, int *k)
{
  jcntrl_size_type nxy;
  int nx, ny, nz;
  int ii, jj, kk;

  if (addr < 0)
    return 0;

  nx = jcntrl_extent_nx(p);
  ny = jcntrl_extent_ny(p);
  nz = jcntrl_extent_nz(p);
  if (nx <= 0 || ny <= 0 || nz <= 0)
    return 0;

  nxy = nx;
  if (jcntrl_s_mul_overflow(nxy, ny, &nxy))
    return -2;

  if (addr / nxy > INT_MAX)
    return 0;

  kk = addr / nxy;
  if (kk >= nz)
    return 0;

  addr %= nxy;
  if (addr / ny > INT_MAX) {
    jcntrl_raise_overflow_error(__FILE__, __LINE__);
    return -2;
  }

  jj = addr / nx;
  ii = addr % nx;

  if (jcntrl_i_add_overflow(ii, p.extent[0], i))
    return -2;
  if (jcntrl_i_add_overflow(jj, p.extent[2], j))
    return -2;
  if (jcntrl_i_add_overflow(kk, p.extent[4], k))
    return -2;
  return 1;
}

static inline jcntrl_extent jcntrl_extent_overlap(jcntrl_extent a,
                                                  jcntrl_extent b)
{
  jcntrl_irange x, y, z;
  x = jcntrl_irange_overlap(jcntrl_extent_xrange(a), jcntrl_extent_xrange(b));
  y = jcntrl_irange_overlap(jcntrl_extent_yrange(a), jcntrl_extent_yrange(b));
  z = jcntrl_irange_overlap(jcntrl_extent_zrange(a), jcntrl_extent_zrange(b));
  return jcntrl_extent_r(x, y, z);
}

static inline jcntrl_extent jcntrl_extent_cover(jcntrl_extent a,
                                                jcntrl_extent b)
{
  jcntrl_irange x, y, z;
  x = jcntrl_irange_cover(jcntrl_extent_xrange(a), jcntrl_extent_xrange(b));
  y = jcntrl_irange_cover(jcntrl_extent_yrange(a), jcntrl_extent_yrange(b));
  z = jcntrl_irange_cover(jcntrl_extent_zrange(a), jcntrl_extent_zrange(b));
  return jcntrl_extent_r(x, y, z);
}

static inline int jcntrl_extent_include(jcntrl_extent a, int i, int j, int k)
{
  return jcntrl_irange_include(jcntrl_extent_xrange(a), i) &&
         jcntrl_irange_include(jcntrl_extent_yrange(a), j) &&
         jcntrl_irange_include(jcntrl_extent_zrange(a), k);
}

JUPITER_CONTROL_DECL_END

#endif
