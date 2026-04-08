#ifndef JUPITER_LPTX_INTERPOLATOR_H
#define JUPITER_LPTX_INTERPOLATOR_H

#include "defs.h"
#include "util.h"
#include "vector.h"

JUPITER_LPTX_DECL_START

/* Interpolater for rectilinear grid (LPTX_rectilinear_grid) */

/**
 * Compute array addresses of cell-center values surrounding @p p
 */
static inline void LPTX_interp_centered_addr(
  const LPTX_rectilinear_grid *grid, LPTX_vector *wl, LPTX_vector *wp,
  LPTX_idtype jj[8], int stmx, int stmy, int stmz, int mx, int my, int mz,
  LPTX_idtype i, LPTX_idtype j, LPTX_idtype k, LPTX_vector p)
{
  LPTX_vector wc, wn;
  LPTX_idtype jjv, in, jn, kn;

  wc = LPTX_rectilinear_grid_cc(grid, i, j, k);
  in = i + ((LPTX_vector_x(wc) < LPTX_vector_x(p)) ? 1 : -1);
  jn = j + ((LPTX_vector_y(wc) < LPTX_vector_y(p)) ? 1 : -1);
  kn = k + ((LPTX_vector_z(wc) < LPTX_vector_z(p)) ? 1 : -1);

  jjv = LPTX_rectilinear_addr(i + stmx, j + stmy, k + stmz, mx, my, mz);
  jj[0] = jjv;
  if (in > i) {
    jj[1] = jjv + 1;
  } else {
    jj[1] = jj[0];
    jj[0] -= 1;
  }
  if (jn > j) {
    for (int i = 0; i < 2; ++i)
      jj[i + 2] = jj[i] + mx;
  } else {
    for (int i = 0; i < 2; ++i) {
      jj[i + 2] = jj[i];
      jj[i] -= mx;
    }
  }
  if (kn > k) {
    for (int i = 0; i < 4; ++i)
      jj[i + 4] = jj[i] + mx * my;
  } else {
    for (int i = 0; i < 4; ++i) {
      jj[i + 4] = jj[i];
      jj[i] -= mx * my;
    }
  }

  wn = LPTX_rectilinear_grid_cc(grid, in, jn, kn);
  *wp = LPTX_vector_max_each(wc, wn);
  *wl = LPTX_vector_min_each(wc, wn);
}

/**
 * @brief Linear interpolation of scalar values for corners @p x
 * @param x Corner values (rectiliner array of (nx, ny, nz) = (2, 2, 2))
 * @param m Loewr-left corner coordinate
 * @param p Upper-right corner coordinate
 * @param r Requesting coordinate for interpolation
 * @return interpolation result
 */
static inline LPTX_type LPTX_interp_box(const LPTX_type x[8], LPTX_vector m,
                                        LPTX_vector p, LPTX_vector r)
{
  LPTX_type fxm, fxp, fym, fyp, fzm, fzp;
  LPTX_vector d, l, u;
  d = LPTX_vector_max_each(m, p);
  m = LPTX_vector_min_each(m, p);
  p = d;

  d = LPTX_vector_sub(p, m);
  l = LPTX_vector_sub(r, m);
  u = LPTX_vector_sub(p, r);
  if (LPTX_vector_x(d) > LPTX_C(0.)) {
    fxp = LPTX_vector_x(u) / LPTX_vector_x(d);
    fxm = LPTX_vector_x(l) / LPTX_vector_x(d);
  } else {
    fxp = LPTX_C(1.);
    fxm = LPTX_C(0.);
  }
  if (LPTX_vector_y(d) > LPTX_C(0.)) {
    fyp = LPTX_vector_y(u) / LPTX_vector_y(d);
    fym = LPTX_vector_y(l) / LPTX_vector_y(d);
  } else {
    fyp = LPTX_C(1.);
    fym = LPTX_C(0.);
  }
  if (LPTX_vector_z(d) > LPTX_C(0.)) {
    fzp = LPTX_vector_z(u) / LPTX_vector_z(d);
    fzm = LPTX_vector_z(l) / LPTX_vector_z(d);
  } else {
    fzp = LPTX_C(1.);
    fzm = LPTX_C(0.);
  }

  return fxp * fyp * fzp * x[LPTX_rectilinear_addr(0, 0, 0, 2, 2, 2)] +
         fxp * fyp * fzm * x[LPTX_rectilinear_addr(0, 0, 1, 2, 2, 2)] +
         fxp * fym * fzp * x[LPTX_rectilinear_addr(0, 1, 0, 2, 2, 2)] +
         fxp * fym * fzm * x[LPTX_rectilinear_addr(0, 1, 1, 2, 2, 2)] +
         fxm * fyp * fzp * x[LPTX_rectilinear_addr(1, 0, 0, 2, 2, 2)] +
         fxm * fyp * fzm * x[LPTX_rectilinear_addr(1, 0, 1, 2, 2, 2)] +
         fxm * fym * fzp * x[LPTX_rectilinear_addr(1, 1, 0, 2, 2, 2)] +
         fxm * fym * fzm * x[LPTX_rectilinear_addr(1, 1, 1, 2, 2, 2)];
}

/**
 * @brief interpolation for cell corner of rectilinear grid from
 *        staggered vector field
 * @param grid Rectilinear grid
 * @param vector Vector field to calculate
 * @param i I-corner index
 * @param j J-corner index
 * @param k K-corner index
 * @return interpolated vector
 *
 * This function exists for emulating the behavior of misimplemented old LPT
 * module. We can interpolate staggered vector directly (since the each
 * components of vector are independent). See issue #401.
 *
 * Least one stencil required.
 */
static LPTX_vector
LPTX_interp_staggered_corner(const LPTX_rectilinear_grid *grid,
                             const LPTX_rectilinear_vector *vect, //
                             LPTX_idtype i, LPTX_idtype j, LPTX_idtype k)
{
  /* LPTvfs */
  LPTX_type xm, x, xp, dxm, dxp, umm, upm, ump, upp;
  LPTX_type ym, y, yp, dym, dyp, vmm, vpm, vmp, vpp;
  LPTX_type zm, z, zp, dzm, dzp, wmm, wpm, wmp, wpp;
  LPTX_idtype jjv, jjxm, jjym, jjzm, jjxym, jjyzm, jjxzm;

  jjv = LPTX_rectilinear_addr(i + vect->stmx, j + vect->stmy, k + vect->stmz,
                              vect->mx, vect->my, vect->mz);
  jjxm = (i + vect->stmx > 0) ? jjv - 1 : jjv;
  jjym = (j + vect->stmy > 0) ? jjv - vect->mx : jjv;
  jjzm = (k + vect->stmz > 0) ? jjv - (vect->mx * vect->my) : jjv;
  jjxym = (i + vect->stmx > 0) ? jjym - 1 : jjym;
  jjyzm = (j + vect->stmy > 0) ? jjzm - vect->mx : jjzm;
  jjxzm = (i + vect->stmx > 0) ? jjzm - 1 : jjzm;

  i += grid->stmx;
  j += grid->stmy;
  k += grid->stmz;

  x = grid->get_coord_i(grid->coords_i, i);
  y = grid->get_coord_j(grid->coords_j, j);
  z = grid->get_coord_k(grid->coords_k, k);
  xm = (i > 0) ? grid->get_coord_i(grid->coords_i, i - 1) : x;
  ym = (j > 0) ? grid->get_coord_j(grid->coords_j, j - 1) : y;
  zm = (k > 0) ? grid->get_coord_k(grid->coords_k, k - 1) : z;
  xp = (i < grid->mx) ? grid->get_coord_i(grid->coords_i, i + 1) : x;
  yp = (j < grid->my) ? grid->get_coord_j(grid->coords_j, j + 1) : y;
  zp = (k < grid->mz) ? grid->get_coord_k(grid->coords_k, k + 1) : z;

  /* Staggard grid factor of 1/2 will be eliminated. */
  dxm = (xp > x) ? (x - xm) / (xp - xm) : LPTX_C(0.);
  dym = (yp > y) ? (y - ym) / (yp - ym) : LPTX_C(0.);
  dzm = (zp > z) ? (z - zm) / (zp - zm) : LPTX_C(0.);
  dxp = LPTX_C(1.) - dxm;
  dyp = LPTX_C(1.) - dym;
  dzp = LPTX_C(1.) - dzm;

  umm = vect->get_func_x(vect->vector_x, jjyzm);
  ump = vect->get_func_x(vect->vector_x, jjym);
  upm = vect->get_func_x(vect->vector_x, jjzm);
  upp = vect->get_func_x(vect->vector_x, jjv);

  vmm = vect->get_func_y(vect->vector_y, jjxzm);
  vmp = vect->get_func_y(vect->vector_y, jjxm);
  vpm = vect->get_func_y(vect->vector_y, jjzm);
  vpp = vect->get_func_y(vect->vector_y, jjv);

  wmm = vect->get_func_z(vect->vector_z, jjxym);
  wmp = vect->get_func_z(vect->vector_z, jjxm);
  wpm = vect->get_func_z(vect->vector_z, jjym);
  wpp = vect->get_func_z(vect->vector_z, jjv);

  return LPTX_vector_c(dym * dzm * umm + dym * dzp * ump + //
                         dyp * dzm * upm + dyp * dzp * upp,
                       dxm * dzm * vmm + dxm * dzp * vmp + //
                         dxp * dzm * vpm + dxp * dzp * vpp,
                       dxm * dym * wmm + dxm * dyp * wmp + //
                         dxp * dym * wpm + dxp * dyp * wpp);
}

/**
 * @brief Linear interpolation of staggered vector field in rectilinear grid
 * @param grid Rectilinear grid
 * @param vector Vector field to calculate
 * @param i I-corner index
 * @param j J-corner index
 * @param k K-corner index
 * @param p Interpolation point
 * @return interpolated vector
 *
 * Least one stencil required.
 */
static inline LPTX_vector LPTX_interp_staggered_vector_linear(
  const LPTX_rectilinear_grid *grid, const LPTX_rectilinear_vector *vect,
  LPTX_idtype i, LPTX_idtype j, LPTX_idtype k, LPTX_vector p)
{
  /* Direct interpolation from staggared grid */
  LPTX_vector vc, vl, vu, vul, vup, vvl, vvp, vwl, vwp;
  LPTX_type x, y, z, ut[8], u, v, w;
  LPTX_idtype iv, jv, kv, iuv, juv, kuv, ivv, jvv, kvv, iwv, jwv, kwv;

  iv = i + vect->stmx;
  jv = j + vect->stmy;
  kv = k + vect->stmz;
  i += grid->stmx;
  j += grid->stmy;
  k += grid->stmz;

  vl = LPTX_vector_c(grid->get_coord_i(grid->coords_i, i),
                     grid->get_coord_j(grid->coords_j, j),
                     grid->get_coord_k(grid->coords_k, k));
  vu = LPTX_vector_c(grid->get_coord_i(grid->coords_i, i + 1),
                     grid->get_coord_j(grid->coords_j, j + 1),
                     grid->get_coord_k(grid->coords_k, k + 1));
  vc = LPTX_vector_mulf(0.5, LPTX_vector_add(vl, vu));

  vul = LPTX_vector_c(LPTX_vector_x(vl), LPTX_C(0.), LPTX_C(0.));
  vup = LPTX_vector_c(LPTX_vector_x(vu), LPTX_C(0.), LPTX_C(0.));
  vvl = LPTX_vector_c(LPTX_C(0.), LPTX_vector_y(vl), LPTX_C(0.));
  vvp = LPTX_vector_c(LPTX_C(0.), LPTX_vector_y(vu), LPTX_C(0.));
  vwl = LPTX_vector_c(LPTX_C(0.), LPTX_C(0.), LPTX_vector_z(vl));
  vwp = LPTX_vector_c(LPTX_C(0.), LPTX_C(0.), LPTX_vector_z(vu));
  iuv = ivv = iwv = iv;
  juv = jvv = jwv = jv;
  kuv = kvv = kwv = kv;

  if (LPTX_vector_x(p) < LPTX_vector_x(vc)) {
    x = LPTX_vector_x(vc);
    x = (i > 0) ? grid->get_coord_i(grid->coords_i, i - 1) : x;
    x = LPTX_C(0.5) * (x + LPTX_vector_x(vl));

    vvl = LPTX_vector_set_x(vvl, (ivv > 0) ? x : LPTX_vector_x(vc));
    vwl = LPTX_vector_set_x(vwl, (iwv > 0) ? x : LPTX_vector_x(vc));
    vvp = LPTX_vector_set_x(vvp, LPTX_vector_x(vc));
    vwp = LPTX_vector_set_x(vwp, LPTX_vector_x(vc));

    if (ivv > 0)
      ivv -= 1;
    if (iwv > 0)
      iwv -= 1;
  } else {
    x = LPTX_vector_x(vc);
    x = (i + 1 < grid->mx) ? grid->get_coord_i(grid->coords_i, i + 2) : x;
    x = LPTX_C(0.5) * (x + LPTX_vector_x(vu));

    vvl = LPTX_vector_set_x(vvl, LPTX_vector_x(vc));
    vwl = LPTX_vector_set_x(vwl, LPTX_vector_x(vc));
    vvp = LPTX_vector_set_x(vvp, x);
    vwp = LPTX_vector_set_x(vwp, x);
  }

  if (LPTX_vector_y(p) < LPTX_vector_y(vc)) {
    y = LPTX_vector_y(vc);
    y = (j > 0) ? grid->get_coord_j(grid->coords_j, j - 1) : y;
    y = LPTX_C(0.5) * (y + LPTX_vector_y(vl));

    vul = LPTX_vector_set_y(vul, (juv > 0) ? y : LPTX_vector_y(vc));
    vwl = LPTX_vector_set_y(vwl, (jwv > 0) ? y : LPTX_vector_y(vc));
    vup = LPTX_vector_set_y(vup, LPTX_vector_y(vc));
    vwp = LPTX_vector_set_y(vwp, LPTX_vector_y(vc));

    if (juv > 0)
      juv -= 1;
    if (jwv > 0)
      jwv -= 1;
  } else {
    y = LPTX_vector_y(vc);
    y = (j + 1 < grid->my) ? grid->get_coord_j(grid->coords_j, j + 2) : y;
    y = LPTX_C(0.5) * (y + LPTX_vector_y(vu));

    vul = LPTX_vector_set_y(vul, LPTX_vector_y(vc));
    vwl = LPTX_vector_set_y(vwl, LPTX_vector_y(vc));
    vup = LPTX_vector_set_y(vup, y);
    vwp = LPTX_vector_set_y(vwp, y);
  }

  if (LPTX_vector_z(p) < LPTX_vector_z(vc)) {
    z = LPTX_vector_z(vc);
    z = (k > 0) ? grid->get_coord_k(grid->coords_k, k - 1) : z;
    z = LPTX_C(0.5) * (z + LPTX_vector_z(vl));

    vul = LPTX_vector_set_z(vul, (kuv > 0) ? z : LPTX_vector_z(vc));
    vvl = LPTX_vector_set_z(vvl, (kvv > 0) ? z : LPTX_vector_z(vc));
    vup = LPTX_vector_set_z(vup, LPTX_vector_z(vc));
    vvp = LPTX_vector_set_z(vvp, LPTX_vector_z(vc));

    if (kuv > 0)
      kuv -= 1;
    if (kvv > 0)
      kvv -= 1;
  } else {
    z = LPTX_vector_z(vc);
    z = (k + 1 < grid->mz) ? grid->get_coord_k(grid->coords_k, k + 2) : z;
    z = LPTX_C(0.5) * (z + LPTX_vector_z(vu));

    vul = LPTX_vector_set_z(vul, LPTX_vector_z(vc));
    vvl = LPTX_vector_set_z(vvl, LPTX_vector_z(vc));
    vup = LPTX_vector_set_z(vup, z);
    vvp = LPTX_vector_set_z(vvp, z);
  }

  iuv -= vect->stmx;
  juv -= vect->stmy;
  kuv -= vect->stmz;
  ivv -= vect->stmx;
  jvv -= vect->stmy;
  kvv -= vect->stmz;
  iwv -= vect->stmx;
  jwv -= vect->stmy;
  kwv -= vect->stmz;

  // calc u
  ut[0] = LPTX_rectilinear_vector_x(vect, iuv + 0, juv + 0, kuv + 0);
  ut[1] = LPTX_rectilinear_vector_x(vect, iuv + 1, juv + 0, kuv + 0);
  ut[2] = LPTX_rectilinear_vector_x(vect, iuv + 0, juv + 1, kuv + 0);
  ut[3] = LPTX_rectilinear_vector_x(vect, iuv + 1, juv + 1, kuv + 0);
  ut[4] = LPTX_rectilinear_vector_x(vect, iuv + 0, juv + 0, kuv + 1);
  ut[5] = LPTX_rectilinear_vector_x(vect, iuv + 1, juv + 0, kuv + 1);
  ut[6] = LPTX_rectilinear_vector_x(vect, iuv + 0, juv + 1, kuv + 1);
  ut[7] = LPTX_rectilinear_vector_x(vect, iuv + 1, juv + 1, kuv + 1);
  u = LPTX_interp_box(ut, vul, vup, p);

  // calc v
  ut[0] = LPTX_rectilinear_vector_y(vect, ivv + 0, jvv + 0, kvv + 0);
  ut[1] = LPTX_rectilinear_vector_y(vect, ivv + 1, jvv + 0, kvv + 0);
  ut[2] = LPTX_rectilinear_vector_y(vect, ivv + 0, jvv + 1, kvv + 0);
  ut[3] = LPTX_rectilinear_vector_y(vect, ivv + 1, jvv + 1, kvv + 0);
  ut[4] = LPTX_rectilinear_vector_y(vect, ivv + 0, jvv + 0, kvv + 1);
  ut[5] = LPTX_rectilinear_vector_y(vect, ivv + 1, jvv + 0, kvv + 1);
  ut[6] = LPTX_rectilinear_vector_y(vect, ivv + 0, jvv + 1, kvv + 1);
  ut[7] = LPTX_rectilinear_vector_y(vect, ivv + 1, jvv + 1, kvv + 1);
  v = LPTX_interp_box(ut, vvl, vvp, p);

  // calc w
  ut[0] = LPTX_rectilinear_vector_z(vect, iwv + 0, jwv + 0, kwv + 0);
  ut[1] = LPTX_rectilinear_vector_z(vect, iwv + 1, jwv + 0, kwv + 0);
  ut[2] = LPTX_rectilinear_vector_z(vect, iwv + 0, jwv + 1, kwv + 0);
  ut[3] = LPTX_rectilinear_vector_z(vect, iwv + 1, jwv + 1, kwv + 0);
  ut[4] = LPTX_rectilinear_vector_z(vect, iwv + 0, jwv + 0, kwv + 1);
  ut[5] = LPTX_rectilinear_vector_z(vect, iwv + 1, jwv + 0, kwv + 1);
  ut[6] = LPTX_rectilinear_vector_z(vect, iwv + 0, jwv + 1, kwv + 1);
  ut[7] = LPTX_rectilinear_vector_z(vect, iwv + 1, jwv + 1, kwv + 1);
  w = LPTX_interp_box(ut, vwl, vwp, p);

  return LPTX_vector_c(u, v, w);
}

/**
 * @brief Linear interpolation of cell-centered vector field in rectilinear grid
 * @param grid Rectilinear grid
 * @param vector Vector field
 * @param i I-corner index
 * @param j J-corner index
 * @param k K-corner index
 * @param p Interpolation point
 * @return interpolated vector
 *
 * Least one stencil required.
 */
static inline LPTX_vector LPTX_interp_centered_vector_linear(
  const LPTX_rectilinear_grid *grid, const LPTX_rectilinear_vector *vector,
  LPTX_idtype i, LPTX_idtype j, LPTX_idtype k, LPTX_vector p)
{
  LPTX_vector wl, wp;
  LPTX_idtype jj[8];
  LPTX_type x, y, z;
  LPTX_type v[8];

  LPTX_interp_centered_addr(grid, &wl, &wp, jj, vector->stmx, vector->stmy,
                            vector->stmz, vector->mx, vector->my, vector->mz, //
                            i, j, k, p);

  for (int i = 0; i < 8; ++i)
    v[i] = vector->get_func_x(vector->vector_x, jj[i]);

  x = LPTX_interp_box(v, wl, wp, p);

  for (int i = 0; i < 8; ++i)
    v[i] = vector->get_func_y(vector->vector_y, jj[i]);

  y = LPTX_interp_box(v, wl, wp, p);

  for (int i = 0; i < 8; ++i)
    v[i] = vector->get_func_z(vector->vector_z, jj[i]);

  z = LPTX_interp_box(v, wl, wp, p);

  return LPTX_vector_c(x, y, z);
}

/**
 * @brief Linear interpolation of cell-centered scalar field in rectilinear grid
 * @param grid Rectilinear grid
 * @param scl Scalar field to calculate
 * @param i I-corner index
 * @param j J-corner index
 * @param k K-corner index
 * @param p Interpolation point
 * @return interpolated scalar
 *
 * Least one stencil required.
 */
static inline LPTX_type
LPTX_interp_scalar_linear(const LPTX_rectilinear_grid *grid,
                          const LPTX_rectilinear_scalar *scl, LPTX_idtype i,
                          LPTX_idtype j, LPTX_idtype k, LPTX_vector p)
{
  LPTX_vector wl, wp;
  LPTX_idtype jj[8];
  LPTX_type v[8];

  LPTX_interp_centered_addr(grid, &wl, &wp, jj, scl->stmx, scl->stmy, scl->stmz,
                            scl->mx, scl->my, scl->mz, i, j, k, p);

  for (int i = 0; i < 8; ++i)
    v[i] = scl->get_func(scl->scalar, jj[i]);

  return LPTX_interp_box(v, wl, wp, p);
}

JUPITER_LPTX_DECL_END

#endif
