
#include "component_info.h"
#include "csv.h"
#include "geometry/defs.h"
#include "serializer/defs.h"
#include "os/os.h"
#include "struct.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#include <stdio.h>
#include <errno.h>
#include <inttypes.h>

#ifdef LPT
#include "lpt/LPTdefs.h"
#include "lpt/LPTbnd.h"
#endif

#ifdef LPTX
#include "lptx/constants.h"
#include "lptx/defs.h"
#include "lptx/interpolator.h"
#include "lptx/lptcalc.h"
#include "lptx/param.h"
#include "lptx/particle.h"
#include "lptx/ptflags.h"
#include "lptx/type_math.h"
#include "lptx/util.h"
#include "lptx/vector.h"
#endif

#include "lpt.h"
#include "func.h"
#include "csvutil.h"
#include "serializer/msgpackx.h"
#include "serializer/util.h"
#include "geometry/vector.h"
#include "common_util.h"

#ifdef LPT
struct lptwbc_arg_data
{
  variable *val;
  material *mtl;
  parameter *prm;
};

/**
 * @brief wall boundary calculation for JUPITER
 */
static void lptwbc(void *arg, int ip, LPT_type pt[3], LPT_type put[3],
                   LPT_type pdia, const int icf[3], int *iwp, LPT_type *dl);
static void lpt_calc(variable *val, material *mtl, parameter *prm);
#endif

#ifdef LPTX
struct lptwbc_arg_data
{
  variable *val;
  material *mtl;
  parameter *prm;
  LPTX_vector lb, ub;
  LPTX_vector_rect_flags flg;
};

static LPTX_bool lptwbc(LPTX_particle_set *set, void *arg);
static void lptx_calc(variable *val, material *mtl, parameter *prm);
#endif

type calc_lpt(variable *val, material *mtl, parameter *prm)
{
#ifdef HAVE_LPT
  type stime, etime;

  CSVASSERT(val);
  CSVASSERT(mtl);
  CSVASSERT(prm);

  if (prm->flg->lpt_calc != ON)
    return 0.0;

  stime = cpu_time();

#if defined(LPT)
  lpt_calc(val, mtl, prm);
#elif defined(LPTX)
  lptx_calc(val, mtl, prm);
#endif

  etime = cpu_time();

  return etime - stime;
#else
  return 0.0;
#endif
}

static double lptwbc_distance(geom_vec3 *relp, geom_vec3 wallp, geom_vec3 ptv,
                              geom_vec3 norm)
{
  double d;
  geom_vec3 relo;

  /* Relative position to wall origin */
  relo = geom_vec3_sub(ptv, wallp);

  /* distance to wall, positive on fluid side */
  d = geom_vec3_project_factor(relo, norm);

  /* Relative position perpendicular to wall */
  *relp = geom_vec3_factor(norm, d);
  return d;
}

static void lptwbc_impl(parameter *prm, variable *val, material *mtl,
                        geom_vec3 *pptv, geom_vec3 *ppuv, type *dl, int *iwp,
                        int *is_exit, int wbc_test, int do_domain_test,
                        const int icf[3], type pdia)
{
  domain *cdo;
  flags *flg;
  geom_vec3 ptv, puv, prel;
  geom_vec3 cell_origin;
  geom_vec3 cell_center;
  geom_vec3 norm;
  geom_vec3 wallp;
  int iw;
  int j;
  int out_of_domain;
  int jx, jy, jz;
  ptrdiff_t jj;
  double d; /* geom_vec3 is always double */
  const type pradi = pdia * 0.5;
  type pewall;
  int is_out;

  CSVASSERT(prm);
  CSVASSERT(val);
  CSVASSERT(mtl);

  cdo = prm->cdo;
  flg = prm->flg;

  CSVASSERT(cdo);
  CSVASSERT(flg);

  ptv = *pptv;
  puv = *ppuv;

  d = HUGE_VAL;
  j = 0;
  is_out = 0;

  jx = icf[0] + cdo->stm;
  jy = icf[1] + cdo->stm;
  jz = icf[2] + cdo->stm;
  jj = calc_address(jx, jy, jz, cdo->mx, cdo->my, cdo->mz);

  if (wbc_test) {
    norm = geom_vec3_c(val->nvsx[jj], val->nvsy[jj], val->nvsz[jj]);
    if (!geom_vec3_iszero(norm)) {
      /*
       * nvs[xyz] is toward the region where val->ls > 0.
       */
      norm = geom_vec3_factor(norm, -1.0 / geom_vec3_length(norm));
      wallp = geom_vec3_factor(norm, val->ls[jj]);

      cell_origin = geom_vec3_c(cdo->x[jx], cdo->y[jy], cdo->z[jz]);
      cell_center = geom_vec3_c(cdo->x[jx + 1], cdo->y[jy + 1], cdo->z[jz + 1]);
      cell_center = geom_vec3_add(cell_origin, cell_center);
      cell_center = geom_vec3_factor(cell_center, 0.5);
      wallp = geom_vec3_add(cell_center, wallp);

      d = lptwbc_distance(&prel, wallp, ptv, norm);
      j = 1;
    }
  }

  if (do_domain_test) {
    int jmx, jmy, jmz, jtx, jty, jtz;
    geom_vec3 lp, up, lpd, upd, wp, rp, nm;
    double td;
    int jwx, jwy, jwz;

    jmx = jmy = jmz = cdo->stm;
    jtx = jmx + cdo->gnx;
    jty = jmy + cdo->gny;
    jtz = jmz + cdo->gnz;
    lp = geom_vec3_c(cdo->gx[jmx], cdo->gy[jmy], cdo->gz[jmz]);
    up = geom_vec3_c(cdo->gx[jtx], cdo->gy[jty], cdo->gz[jtz]);
    lpd = geom_vec3_add(lp, geom_vec3_c(pradi, pradi, pradi));
    upd = geom_vec3_sub(up, geom_vec3_c(pradi, pradi, pradi));

    jwx = jx;
    jwy = jy;
    jwz = jz;

    wp = ptv;
    nm = geom_vec3_c(0., 0., 0.);
    if (geom_vec3_x(ptv) <= geom_vec3_x(lpd)) {
      is_out = 1;
      wp = geom_vec3_c(geom_vec3_x(lp), geom_vec3_y(wp), geom_vec3_z(wp));
      nm = geom_vec3_c(1., geom_vec3_y(nm), geom_vec3_z(nm));
      jwx = cdo->stm - 1;
    }
    if (geom_vec3_x(ptv) >= geom_vec3_x(upd)) {
      is_out = 1;
      wp = geom_vec3_c(geom_vec3_x(up), geom_vec3_y(wp), geom_vec3_z(wp));
      nm = geom_vec3_c(-1., geom_vec3_y(nm), geom_vec3_z(nm));
      jwx = cdo->stm + cdo->nx;
    }

    if (geom_vec3_y(ptv) <= geom_vec3_y(lpd)) {
      is_out = 1;
      wp = geom_vec3_c(geom_vec3_x(wp), geom_vec3_x(lp), geom_vec3_z(wp));
      nm = geom_vec3_c(geom_vec3_x(nm), 1., geom_vec3_z(nm));
      jwy = cdo->stm - 1;
    }
    if (geom_vec3_y(ptv) >= geom_vec3_y(upd)) {
      is_out = 1;
      wp = geom_vec3_c(geom_vec3_x(wp), geom_vec3_x(up), geom_vec3_z(wp));
      nm = geom_vec3_c(geom_vec3_x(nm), -1., geom_vec3_z(nm));
      jwy = cdo->stm + cdo->ny;
    }

    if (geom_vec3_z(ptv) <= geom_vec3_z(lpd)) {
      is_out = 1;
      wp = geom_vec3_c(geom_vec3_x(wp), geom_vec3_y(wp), geom_vec3_z(lp));
      nm = geom_vec3_c(geom_vec3_x(nm), geom_vec3_y(nm), 1.);
      jwz = cdo->stm - 1;
    }
    if (geom_vec3_z(ptv) >= geom_vec3_z(upd)) {
      is_out = 1;
      wp = geom_vec3_c(geom_vec3_x(wp), geom_vec3_y(wp), geom_vec3_z(up));
      nm = geom_vec3_c(geom_vec3_x(nm), geom_vec3_y(nm), -1.);
      jwz = cdo->stm + cdo->nz;
    }

    if (is_out && !geom_vec3_iszero(nm)) {
      nm = geom_vec3_factor(nm, 1.0 / geom_vec3_length(nm));
      td = lptwbc_distance(&rp, wp, ptv, nm);

      if (!j || td < d) {
        d = td;
        wallp = wp;
        prel = rp;
        norm = nm;
        jx = jwx;
        jy = jwy;
        jz = jwz;
        jj = calc_address(jx, jy, jz, cdo->mx, cdo->my, cdo->mz);
        j = 1;
      } else {
        is_out = 0;
      }
    }
  }

  if (!j) {
    if (iwp)
      *iwp = 0;
    if (dl)
      *dl = -HUGE_VAL;
    return;
  }

  if (dl)
    *dl = -d;

  /*
   *                     0         pradi
   * positive +----------*----------*----------+  [d]
   *          |                     |          |
   *          |                     |          |
   *          |      [bounce]       |          |
   *          |                     |          |
   *        0 *--[stop]--+--[stop]--*          |
   *          |          |                     |
   *          |          |                     |
   *          |  [exit]  |    [keep track]     |
   *          | (track)  |                     |
   * negative +----------*---------------------*
   * [pewall]
   *
   * - [keep track] if the [exit] condition met inside calculation domain.
   */
  if (d > pradi) {
    if (iwp)
      *iwp = 0;
    return;
  }

  pewall = val->lpt_pewall[jj];
  if (pewall < 0.0 && d < 0.0) {
    if (is_out && is_exit)
      *is_exit = 1;
    if (iwp)
      *iwp = 0;
  } else if (pewall >= 0.0) {
    geom_vec3 bounce; /* Base point on bounce */
    geom_vec3 punr, putg;
    double ud;

    bounce = geom_vec3_factor(norm, pradi - d); /* pradi + (-d) */

    /* new particle position */
    prel = geom_vec3_factor(bounce, 1.0 + pewall);
    ptv = geom_vec3_add(prel, ptv);

    /* new particle velocity */
    ud = geom_vec3_project_factor(puv, norm);
    punr = geom_vec3_factor(norm, ud); /* normal */
    if (pewall > 0.0) {
      putg = geom_vec3_sub(puv, punr); /* tangent */
    } else {
      putg = geom_vec3_c(0., 0., 0.);
    }
    puv = geom_vec3_add(putg, geom_vec3_factor(punr, -pewall));

    *pptv = ptv;
    *ppuv = puv;

    if (iwp)
      *iwp = 1;
  } else { /* pewell < 0.0 && d >= 0.0 && d <= pradi */
    if (iwp)
      *iwp = 0;
  }
}

#ifdef LPT
static void lpt_calc(variable *val, material *mtl, parameter *prm)
{
  struct lptwbc_arg_data arg;
  flags *flg;
  domain *cdo;
  int istat;

  flg = prm->flg;
  cdo = prm->cdo;
  istat = 0;

  cLPTsetfield_r(LPT_FV_PEWALL, val->lpt_pewall, cdo->nx, cdo->ny, cdo->nz,
                 cdo->stm, cdo->stp, NULL, 0, &istat);
  if (istat != 0) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Failed to transfer pEwall data to LPT module");
    prm->status = ON;
  }

  arg.mtl = mtl;
  arg.val = val;
  arg.prm = prm;
  cLPTset_udwbc_callback(lptwbc, &arg);
  cLPTcal(1, val->u, val->v, val->w, NULL, NULL, NULL, mtl->dens, NULL, mtl->mu,
          NULL, cdo->x, cdo->y, cdo->z, cdo->grav_x, cdo->grav_y, cdo->grav_z,
          cdo->dt, cdo->time, cdo->nx, cdo->ny, cdo->nz, cdo->stm, cdo->stp,
          &istat);
  cLPTset_udwbc_callback(NULL, NULL);

  if (istat != 0)
    prm->status = ON;
}

static void lptwbc(void *arg, int ip, LPT_type pt[3], LPT_type put[3],
                   LPT_type pdia, const int icf[3], int *iwp, LPT_type *dl)
{
  struct lptwbc_arg_data *la;
  geom_vec3 ptv;
  geom_vec3 puv;
  type tdl;
  int icfc[3];

  la = (struct lptwbc_arg_data *)arg;
  CSVASSERT(la);

  tdl = *dl;
  ptv = geom_vec3_c(pt[0], pt[1], pt[2]);
  puv = geom_vec3_c(put[0], put[1], put[2]);

  for (int i = 0; i < 3; ++i)
    icfc[i] = icf[i] - 1;

  lptwbc_impl(la->prm, la->val, la->mtl, &ptv, &puv, &tdl, iwp, NULL, 1, 0,
              icfc, pdia);

  pt[0] = geom_vec3_x(ptv);
  pt[1] = geom_vec3_y(ptv);
  pt[2] = geom_vec3_z(ptv);

  put[0] = geom_vec3_x(puv);
  put[1] = geom_vec3_y(puv);
  put[2] = geom_vec3_z(puv);

  *dl = tdl;
}
#endif

#ifdef LPTX
#define jLPTX__0(s, name, ...) name##s
#define jLPTX__1(s, name, s1, ...) name##s##s1
#define jLPTX__e(n, s, name, ...) jLPTX__##n(s, name, __VA_ARGS__)
#define jLPTX__p(_0, _1, N, ...) N
#define jLPTX__n(...) jLPTX__p(__VA_ARGS__, 1, 0, __)
#define jLPTX__x(n, ...) jLPTX__e(n, __VA_ARGS__)
#define jLPTX__(s, name, ...) \
  jLPTX__x(jLPTX__n(__VA_ARGS__), s, name, __VA_ARGS__)

#ifdef JUPITER_DOUBLE
#define jLPTX_(...) jLPTX__(d, __VA_ARGS__, LPTX_empty)
#else
#define jLPTX_(...) jLPTX__(f, __VA_ARGS__, LPTX_empty)
#endif

static void jLPTX_particle_set_set_fluid_index(LPTX_particle_set *setp,
                                               domain *cdo)
{
#define jLPTX_func jLPTX_(LPTX_particle_set_set_fluid_index_)
  jLPTX_func(setp, cdo->x, cdo->y, cdo->z, cdo->nx, cdo->ny, cdo->nz, cdo->mx,
             cdo->my, cdo->mz, cdo->stm, cdo->stm, cdo->stm);
#undef jLPTX_func
}

static void jLPTX_particle_set_set_fluid_velocity(LPTX_particle_set *setp,
                                                  domain *cdo, variable *val)
{
#define jLPTX_func jLPTX_(LPTX_particle_set_set_fluid_velocity_, _struct)
  jLPTX_func(setp, val->u, val->v, val->w, cdo->x, cdo->y, cdo->z, cdo->nx,
             cdo->ny, cdo->nz, cdo->mx, cdo->my, cdo->mz, cdo->stm, cdo->stm,
             cdo->stm);
#undef jLPTX_func
}

static void jLPTX_particle_set_set_fluid_density(LPTX_particle_set *setp,
                                                 domain *cdo, material *mtl)
{
#define jLPTX_func jLPTX_(LPTX_particle_set_set_fluid_density_, _struct)
  jLPTX_func(setp, mtl->dens, cdo->x, cdo->y, cdo->z, cdo->nx, cdo->ny, cdo->nz,
             cdo->mx, cdo->my, cdo->mz, cdo->stm, cdo->stm, cdo->stm);
#undef jLPTX_func
}

static void jLPTX_particle_set_set_fluid_viscosity(LPTX_particle_set *setp,
                                                   domain *cdo, material *mtl)
{
#define jLPTX_func jLPTX_(LPTX_particle_set_set_fluid_viscosity_, _struct)
  jLPTX_func(setp, mtl->mu, cdo->x, cdo->y, cdo->z, cdo->nx, cdo->ny, cdo->nz,
             cdo->mx, cdo->my, cdo->mz, cdo->stm, cdo->stm, cdo->stm);
#undef jLPTX_func
}

static void jLPTX_particle_set_set_fluid_temperature(LPTX_particle_set *setp,
                                                     flags *flg, domain *cdo,
                                                     variable *val,
                                                     material *mtl)
{
  type *t, *thc;
  if (flg->two_energy == ON) {
    t = val->tf;
    thc = mtl->thcf;
  } else {
    t = val->t;
    thc = mtl->thc;
  }
#define jLPTX_func jLPTX_(LPTX_particle_set_set_fluid_temperature_, _struct)
  jLPTX_func(setp, t, thc, cdo->x, cdo->y, cdo->z, cdo->nx, cdo->ny, cdo->nz,
             cdo->mx, cdo->my, cdo->mz, cdo->stm, cdo->stm, cdo->stm);
#undef jLPTX_func
}

static void jLPTX_particle_set_set_fluid_specific_heat(LPTX_particle_set *setp,
                                                       flags *flg, domain *cdo,
                                                       material *mtl)
{
  type *cp;
  if (flg->porous == ON && flg->two_energy == OFF) {
    cp = mtl->c_f;
  } else {
    cp = mtl->specht;
  }
#define jLPTX_func jLPTX_(LPTX_particle_set_set_fluid_specific_heat_, _struct)
  jLPTX_func(setp, cp, cdo->x, cdo->y, cdo->z, cdo->nx, cdo->ny, cdo->nz,
             cdo->mx, cdo->my, cdo->mz, cdo->stm, cdo->stm, cdo->stm);
#undef jLPTX_func
}

static void jLPTX_particle_set_set_fluid_thermal_conductivity(
  LPTX_particle_set *setp, flags *flg, domain *cdo, material *mtl)
{
  type *thc;
  if (flg->two_energy == ON) {
    thc = mtl->thcf;
  } else {
    thc = mtl->thc;
  }
#define jLPTX_func jLPTX_(LPTX_particle_set_set_fluid_thermal_conductivity_, \
                          _struct)
  jLPTX_func(setp, thc, cdo->x, cdo->y, cdo->z, cdo->nx, cdo->ny, cdo->nz,
             cdo->mx, cdo->my, cdo->mz, cdo->stm, cdo->stm, cdo->stm);
#undef jLPTX_func
}

#ifdef JUPITER_DOUBLE
#define jLPTX_get_scalar_of LPTX_get_scalar_of_double
#else
#define jLPTX_get_scalar_of LPTX_get_scalar_of_float
#endif

struct calc_Tgrad_data
{
  flags *flg;
  domain *cdo;
  variable *val;
};

static LPTX_type calc_grad(LPTX_type fm, LPTX_type fc, LPTX_type fp,
                           LPTX_type dm, LPTX_type dp)
{
  LPTX_type dmdp = dm * dp;
  LPTX_type dm2 = dm * dm;
  LPTX_type dp2 = dp * dp;
  return (fp * dm2 + fc * (dp2 - dm2) - fm * dp2) / (dmdp * (dm + dp));
}

static LPTX_vector calc_Tgrad(void *args, LPTX_vector position, LPTX_idtype i,
                              LPTX_idtype j, LPTX_idtype k)
{
  struct calc_Tgrad_data *d = args;
  domain *cdo = d->cdo;
  LPTX_rectilinear_grid grid;
  LPTX_idtype jj[8];
  int bi, bj, bk;
  LPTX_vector w[4];
  LPTX_type T[64];
  LPTX_type Tg[24], Tgx, Tgy, Tgz;
  LPTX_rectilinear_scalar Ts;

  grid = LPTX_rectilinear_grid_g(cdo->x, cdo->y, cdo->z, cdo->nx, cdo->ny,
                                 cdo->nz, cdo->mx, cdo->my, cdo->mz, cdo->stm,
                                 cdo->stm, cdo->stm, jLPTX_get_scalar_of);

  LPTX_interp_centered_addr(&grid, &w[1], &w[2], jj, cdo->stm, cdo->stm,
                            cdo->stm, cdo->mx, cdo->my, cdo->mz, i, j, k,
                            position);
  calc_struct_index(jj[0], cdo->mx, cdo->my, cdo->mz, &bi, &bj, &bk);

  bi -= cdo->stm;
  bj -= cdo->stm;
  bk -= cdo->stm;
  w[0] = LPTX_rectilinear_grid_v000(&grid, bi - 1, bj - 1, bk - 1);
  w[3] = LPTX_rectilinear_grid_v111(&grid, bi + 1, bj + 1, bk + 1);

  for (ptrdiff_t jf = 0; jf < 64; ++jf) {
    int pi, pj, pk;
    ptrdiff_t jj;
    calc_struct_index(jf, 4, 4, 4, &pi, &pj, &pk);
    pi += bi - 1 + cdo->stm;
    pj += bj - 1 + cdo->stm;
    pk += bk - 1 + cdo->stm;
    T[jf] = d->val->t[calc_address(pi, pj, pk, cdo->mx, cdo->my, cdo->mz)];
  }

  Ts = LPTX_rectilinear_scalar_c(T, 2, 2, 2, 4, 4, 4, 1, 1, 1,
                                 LPTX_get_scalar_of_type);
  for (ptrdiff_t jf = 0; jf < 8; ++jf) {
    int pi, pj, pk;
    LPTX_type dm, dp, fm, fc, fp;
    calc_struct_index(jf, 2, 2, 2, &pi, &pj, &pk);

    /* x */
    fc = LPTX_rectilinear_scalar_at(&Ts, pi + 0, pj, pk);
    fm = LPTX_rectilinear_scalar_at(&Ts, pi - 1, pj, pk);
    fp = LPTX_rectilinear_scalar_at(&Ts, pi + 1, pj, pk);
    dm = LPTX_vector_x(w[pi + 1]) - LPTX_vector_x(w[pi + 0]);
    dp = LPTX_vector_x(w[pi + 2]) - LPTX_vector_x(w[pi + 1]);
    Tg[jf + 0 * 8] = calc_grad(fm, fc, fp, dm, dp);

    /* y */
    fc = LPTX_rectilinear_scalar_at(&Ts, pi, pj + 0, pk);
    fm = LPTX_rectilinear_scalar_at(&Ts, pi, pj - 1, pk);
    fp = LPTX_rectilinear_scalar_at(&Ts, pi, pj + 1, pk);
    dm = LPTX_vector_y(w[pj + 1]) - LPTX_vector_y(w[pj + 0]);
    dp = LPTX_vector_y(w[pj + 2]) - LPTX_vector_y(w[pj + 1]);
    Tg[jf + 1 * 8] = calc_grad(fm, fc, fp, dm, dp);

    /* z */
    fc = LPTX_rectilinear_scalar_at(&Ts, pi, pj, pk + 0);
    fm = LPTX_rectilinear_scalar_at(&Ts, pi, pj, pk - 1);
    fp = LPTX_rectilinear_scalar_at(&Ts, pi, pj, pk + 1);
    dm = LPTX_vector_z(w[pk + 1]) - LPTX_vector_z(w[pk + 0]);
    dp = LPTX_vector_z(w[pk + 2]) - LPTX_vector_z(w[pk + 1]);
    Tg[jf + 2 * 8] = calc_grad(fm, fc, fp, dm, dp);
  }

  Tgx = LPTX_interp_box(&Tg[0], w[1], w[2], position);
  Tgy = LPTX_interp_box(&Tg[8], w[1], w[2], position);
  Tgz = LPTX_interp_box(&Tg[16], w[1], w[2], position);
  return LPTX_vector_c(Tgx, Tgy, Tgz);
}

static void jLPTX_particle_set_set_fluid_temperature_grad(
  LPTX_particle_set *setp, flags *flg, domain *cdo, variable *val)
{
  struct calc_Tgrad_data d = {
    .flg = flg,
    .cdo = cdo,
    .val = val,
  };
  LPTX_particle_set_set_fluid_temperature_grad_cb(setp, calc_Tgrad, &d);
}

struct calc_molar_weight_data
{
  flags *flg;
  domain *cdo;
  phase_value *phv;
  variable *val;
};

static LPTX_type calc_molecular_weight(void *args, LPTX_vector position,
                                       LPTX_idtype i, LPTX_idtype j,
                                       LPTX_idtype k)
{
  LPTX_type ls;
  struct calc_molar_weight_data *d = args;
  domain *cdo = d->cdo;
  LPTX_rectilinear_grid grid;
  LPTX_rectilinear_scalar lsc;
  grid = LPTX_rectilinear_grid_g(cdo->x, cdo->y, cdo->z, cdo->nx, cdo->ny,
                                 cdo->nz, cdo->mx, cdo->my, cdo->mz, cdo->stm,
                                 cdo->stm, cdo->stm, jLPTX_get_scalar_of);
  lsc = LPTX_rectilinear_scalar_c(d->val->lls, cdo->nx, cdo->ny, cdo->nz,
                                  cdo->mx, cdo->my, cdo->mz, cdo->stm, cdo->stm,
                                  cdo->stm, jLPTX_get_scalar_of);
  ls = LPTX_interp_scalar_linear(&grid, &lsc, i, j, k, position);

  /* Average for liquid if ls > 0 and average for gas if ls < 0 */
  if (d->flg->solute_diff == ON) {
    if (ls > 0) {
      LPTX_rectilinear_scalar Yc;
      LPTX_type Y;
      LPTX_type mw = LPTX_C(0.0);
      for (int ic = 0; ic < cdo->NIBaseComponent; ++ic) {
        Yc = LPTX_rectilinear_scalar_c(d->val->Y + cdo->m * ic, cdo->nx,
                                       cdo->ny, cdo->nz, cdo->mx, cdo->my,
                                       cdo->mz, cdo->stm, cdo->stm, cdo->stm,
                                       jLPTX_get_scalar_of);
        Y = LPTX_interp_scalar_linear(&grid, &Yc, i, j, k, position);
        mw += d->phv->comps[i].molar_mass * Y;
      }
      return mw;
    } else {
      LPTX_rectilinear_scalar Yc;
      LPTX_type Y, Ys = LPTX_C(0.0);
      LPTX_type mw = LPTX_C(0.0);
      for (int ic = cdo->NBaseComponent; ic < cdo->NumberOfComponent; ++ic) {
        Yc = LPTX_rectilinear_scalar_c(d->val->Y + cdo->m * ic, cdo->nx,
                                       cdo->ny, cdo->nz, cdo->mx, cdo->my,
                                       cdo->mz, cdo->stm, cdo->stm, cdo->stm,
                                       jLPTX_get_scalar_of);
        Y = LPTX_interp_scalar_linear(&grid, &Yc, i, j, k, position);
        mw += d->phv->comps[i].molar_mass * Y;
        Ys += Y;
      }
      mw += d->phv->molar_mass_g * (1.0 - Ys);
      return mw;
    }
  } else {
    if (ls > 0) {
      /**
       * @todo We should return the molar mass of specific component where the
       * particle is located, instead of averaging them.
       */
      LPTX_rectilinear_scalar flc;
      LPTX_type fl;
      LPTX_type mw = LPTX_C(0.0);
      for (int ic = 0; ic < cdo->NBaseComponent; ++ic) {
        flc = LPTX_rectilinear_scalar_c(d->val->fl + cdo->m * ic, cdo->nx,
                                        cdo->ny, cdo->nz, cdo->mx, cdo->my,
                                        cdo->mz, cdo->stm, cdo->stm, cdo->stm,
                                        jLPTX_get_scalar_of);
        fl = LPTX_interp_scalar_linear(&grid, &flc, i, j, k, position);
        mw += d->phv->comps[i].molar_mass * fl;
      }
      return mw;
    } else {
      return d->phv->molar_mass_g;
    }
  }
}

static void jLPTX_particle_set_set_molar_weight(LPTX_particle_set *setp,
                                                flags *flg, domain *cdo,
                                                phase_value *phv, variable *val)
{
  struct calc_molar_weight_data d = {
    .flg = flg,
    .cdo = cdo,
    .phv = phv,
    .val = val,
  };
  LPTX_particle_set_set_fluid_molecular_weight_cb(setp, calc_molecular_weight,
                                                  &d);
}

/**
 * Compute mean free path by:
 * Harry K. Moffat. CADS Cantera Aerosol Dynamics Simulator, SANDIA REPORT,
 * SAND2007-4216. July 2007, Sandia National Laboratories, p.68, from
 * https://www.osti.gov/servlets/purl/912650
 */
static LPTX_type calc_mean_free_path(LPTX_type molecular_weight,
                                     LPTX_type temperature, LPTX_type viscosity,
                                     LPTX_type density)
{
  return LPTX_C(2.0) * viscosity / density *
         LPTX_type_sqrt(LPTX_M_PI * molecular_weight /
                        (LPTX_C(8.) * LPTX_gas_constant * temperature));
}

static LPTX_bool lptx_calc_loop(LPTX_param *param,
                                LPTX_single_time_integrate_data *id, void *arg)
{
  flags *flg;
  domain *cdo;
  struct lptwbc_arg_data *d;
  LPTX_bool want_tempf;
  LPTX_bool want_tgradf;
  LPTX_bool want_mwf;
  LPTX_bool want_pathf;
  LPTX_bool want_cpf;
  LPTX_bool want_thcf;

  d = (struct lptwbc_arg_data *)arg;
  flg = d->prm->flg;
  cdo = d->prm->cdo;

  want_tempf = LPTX_false;
  if (!want_tempf)
    want_tempf = LPTX_param_want_fluid_temperature(param);
  want_tgradf = LPTX_param_want_fluid_temperature_grad(param);
  want_mwf = LPTX_param_want_fluid_molecular_weight(param);
  want_pathf = LPTX_param_want_mean_free_path(param);
  want_cpf = LPTX_param_want_fluid_specific_heat(param);
  want_thcf = LPTX_param_want_fluid_thermal_conductivity(param);

#ifdef _OPENMP
#pragma omp parallel
#endif
  {
    LPTX_particle_set *setp;

    LPTX_foreach_particle_sets (setp, param) {
      LPTX_idtype np;
      jLPTX_particle_set_set_fluid_index(setp, cdo);
      jLPTX_particle_set_set_fluid_velocity(setp, cdo, d->val);
      jLPTX_particle_set_set_fluid_density(setp, cdo, d->mtl);
      jLPTX_particle_set_set_fluid_viscosity(setp, cdo, d->mtl);
      if (want_tempf)
        jLPTX_particle_set_set_fluid_temperature(setp, flg, cdo, d->val,
                                                 d->mtl);
      if (want_cpf)
        jLPTX_particle_set_set_fluid_specific_heat(setp, flg, cdo, d->mtl);
      if (want_thcf)
        jLPTX_particle_set_set_fluid_thermal_conductivity(setp, flg, cdo,
                                                          d->mtl);
      if (want_tgradf)
        jLPTX_particle_set_set_fluid_temperature_grad(setp, flg, cdo, d->val);
      if (want_mwf)
        jLPTX_particle_set_set_molar_weight(setp, flg, cdo, d->prm->phv,
                                            d->val);

      if (want_pathf) {
        np = LPTX_particle_set_number_of_particles(setp);
#ifdef _OPENMP
#pragma omp for
#endif
        for (LPTX_idtype j = 0; j < np; ++j) {
          const LPTX_particle *p;
          p = LPTX_particle_set_get_particle_at(setp, j, NULL, NULL);
          LPTX_particle pp;
          LPTX_type mw, t, mu, rho;
          if (!LPTX_particle_is_used(p))
            continue;

          mw = p->fluid_molecular_weight;
          t = p->fluid_temperature;
          mu = p->fluid_viscosity;
          rho = p->fluid_density;
          pp = *p;
          pp.mean_free_path = calc_mean_free_path(mw, t, mu, rho);
          LPTX_particle_set_set_particle_at(setp, j, &pp, 0, NULL, NULL);
        }
      }
    }

    LPTX_single_time_integrate(param, id);

    if (LPTX_param_heat_exchange_by_htr(param)) {
      LPTX_foreach_particle_sets (setp, param) {
        LPTX_idtype np;
        np = LPTX_particle_set_number_of_particles(setp);
#ifdef _OPENMP
#pragma omp for
#endif
        for (LPTX_idtype j = 0; j < np; ++j) {
          const LPTX_particle *p;
          int jx, jy, jz;
          ptrdiff_t jj;
          type q, dx, dy, dz;

          p = LPTX_particle_set_get_particle_at(setp, j, NULL, NULL);
          jx = p->icfpt + cdo->stm;
          jy = p->jcfpt + cdo->stm;
          jz = p->kcfpt + cdo->stm;
          jj = calc_address(jx, jy, jz, cdo->mx, cdo->my, cdo->mz);
          dx = cdo->x[jx + 1] - cdo->x[jx];
          dy = cdo->y[jy + 1] - cdo->y[jy];
          dz = cdo->z[jz + 1] - cdo->z[jz];
          q = p->total_heat_transfer / (dx * dy * dz);
          d->val->qpt[jj] += -q;
        }
      }
    }
  }

#ifdef JUPITER_MPI
  {
    int r;
    r = LPTX_param_redistribute_particles_rect_v(param, d->lb, d->ub, d->flg);
    if (r != MPI_SUCCESS) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL,
                "Failed to redistribute particles", CSV_ERR_MPI, 0, r, NULL);
      d->prm->status = ON;
    }
  }
#endif

  if (d->prm->status == ON)
    return LPTX_false;
  return LPTX_true;
}

static void lptx_calc(variable *val, material *mtl, parameter *prm)
{
  domain *cdo;
  struct lptwbc_arg_data d = {
    .val = val,
    .mtl = mtl,
    .prm = prm,
  };

  cdo = prm->cdo;

  {
    LPTX_vector_rect_flags f;
    int is, ie, js, je, ks, ke;
    is = js = ks = cdo->stm;
    ie = is + cdo->nx;
    je = js + cdo->ny;
    ke = ks + cdo->nz;
    f = LPTX_VECTOR_RECT_P | LPTX_VECTOR_RECT_W | LPTX_VECTOR_RECT_S |
        LPTX_VECTOR_RECT_B;

    if (prm->mpi->nrk[0] == -1) { // z-
      ks = 0;
    }
    if (prm->mpi->nrk[1] == -1) { // z+
      ke = cdo->mz;
      f |= LPTX_VECTOR_RECT_T;
    }
    if (prm->mpi->nrk[2] == -1) { // y-
      js = 0;
    }
    if (prm->mpi->nrk[3] == -1) { // y+
      je = cdo->my;
      f |= LPTX_VECTOR_RECT_N;
    }
    if (prm->mpi->nrk[4] == -1) { // x-
      is = 0;
    }
    if (prm->mpi->nrk[5] == -1) { // x+
      ie = cdo->mx;
      f |= LPTX_VECTOR_RECT_E;
    }

    d.lb = LPTX_vector_c(cdo->x[is], cdo->y[js], cdo->z[ks]);
    d.ub = LPTX_vector_c(cdo->x[ie], cdo->y[je], cdo->z[ke]);
    d.flg = f;
  }

  LPTX_param_set_gravity(val->lpt_param,
                         LPTX_vector_c(cdo->grav_x, cdo->grav_y, cdo->grav_z));

  if (LPTX_param_heat_exchange(val->lpt_param)) {
#ifdef _OPENMP
#pragma omp parallel for
#endif
    for (int i = 0; i < cdo->m; ++i)
      val->qpt[i] = 0.0;
  }

#ifdef JUPITER_MPI
  {
    int r;
    r = LPTX_param_redistribute_particles_rect_v(val->lpt_param, d.lb, d.ub,
                                                 d.flg);
    if (r != MPI_SUCCESS) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL,
                "Failed to redistribute particles", CSV_ERR_MPI, 0, r, NULL);
      prm->status = ON;
      return;
    }
  }
#endif

  {
    LPTX_particle_set *setp;
    LPTX_foreach_particle_sets (setp, val->lpt_param) {
      jLPTX_particle_set_set_fluid_index(setp, cdo);
      jLPTX_particle_set_set_fluid_velocity(setp, cdo, val);
      jLPTX_particle_set_set_fluid_density(setp, cdo, mtl);
      jLPTX_particle_set_set_fluid_viscosity(setp, cdo, mtl);
    }
  }

  LPTX_update_number_of_substep(val->lpt_param, cdo->dt);

  LPTX_substep_loop_f(val->lpt_param, cdo->icnt, cdo->time, cdo->dt,
                      lptx_calc_loop, &d);

  LPTX_param_foreach_particle_sets(val->lpt_param, lptwbc, &d);
}

static LPTX_bool lptwbc(LPTX_particle_set *set, void *arg)
{
  struct lptwbc_arg_data *d;
  LPTX_idtype np;

  np = LPTX_particle_set_number_of_particles(set);
  d = (struct lptwbc_arg_data *)arg;

#ifdef _OPENMP
#pragma omp for
#endif
  for (LPTX_idtype ip = 0; ip < np; ++ip) {
    const LPTX_particle *pt;
    LPTX_particle wpt;
    geom_vec3 ptv;
    geom_vec3 puv;
    type dl;
    int iwp;
    int iex;
    int icf[3];

    pt = LPTX_particle_set_get_particle_at(set, ip, NULL, NULL);

    if (!LPTX_particle_is_used(pt))
      continue;

    if (LPTX_particle_is_exited(pt))
      continue;

    wpt = *pt;
    ptv = geom_vec3_c(LPTX_vector_x(pt->position), LPTX_vector_y(pt->position),
                      LPTX_vector_z(pt->position));
    puv = geom_vec3_c(LPTX_vector_x(pt->velocity), LPTX_vector_y(pt->velocity),
                      LPTX_vector_z(pt->velocity));
    iwp = 0;
    dl = pt->dlpin;
    icf[0] = pt->icfpt;
    icf[1] = pt->jcfpt;
    icf[2] = pt->kcfpt;
    iex = !!LPTX_particle_is_exited(pt);

    lptwbc_impl(d->prm, d->val, d->mtl, &ptv, &puv, &dl, &iwp, &iex,
                d->prm->flg->lpt_wbcal == ON, 1, icf, pt->diameter);

    if (iex) {
      LPTX_param *prm = LPTX_particle_set_param(set);

      LPTX_particle_set_exited(&wpt, LPTX_true);
      LPTX_param_count_exited(prm, 1);
    }

    wpt.position =
      LPTX_vector_c(geom_vec3_x(ptv), geom_vec3_y(ptv), geom_vec3_z(ptv));
    wpt.velocity =
      LPTX_vector_c(geom_vec3_x(puv), geom_vec3_y(puv), geom_vec3_z(puv));
    wpt.dlpin = dl;
    LPTX_particle_set_set_particle_at(set, ip, &wpt, 0, NULL, NULL);
  }

  return LPTX_false;
}
#endif

#define JUPITER_LPT_CTRL_TITLE "lpt_ctrl"
#define JUPITER_LPT_CTRL_VERSION 1

jupiter_lpt_ctrl_data *new_lpt_ctrl_data(void)
{
  return (jupiter_lpt_ctrl_data *)calloc(sizeof(jupiter_lpt_ctrl_data), 1);
}

int write_lpt_ctrl_data(const char *path, const jupiter_lpt_ctrl_data *data)
{
  FILE *fp;
  msgpackx_data *mdata;
  msgpackx_array_node *ahead, *anode;
  msgpackx_map_node *mhead;
  msgpackx_error err;
  int retv;

  CSVASSERT(path);
  CSVASSERT(data);

  retv = 0;
  fp = NULL;

  mdata = msgpackx_data_new();
  if (!mdata)
    goto allocerror;

  err = MSGPACKX_SUCCESS;
  ahead = msgpackx_make_header_data(mdata, JUPITER_LPT_CTRL_TITLE, NULL, &err);
  if (!ahead)
    goto serror;

  err = msgpackx_array_insert_prev_map(ahead, &mhead);
  if (err != MSGPACKX_SUCCESS)
    goto serror;

  err = msgpackx_map_skey_set_int(mhead, MCSTR("v"), JUPITER_LPT_CTRL_VERSION);
  if (err != MSGPACKX_SUCCESS)
    goto serror;

  err = msgpackx_map_skey_set_int(mhead, MCSTR("npt"), data->npt);
  if (err != MSGPACKX_SUCCESS)
    goto serror;

#ifdef JUPITER_DOUBLE
  err = msgpackx_map_skey_set_double(mhead, MCSTR("timprn"), data->timprn);
#else
  err = msgpackx_map_skey_set_float(mhead, MCSTR("timprn"), data->timprn);
#endif
  if (err != MSGPACKX_SUCCESS)
    goto serror;

  errno = 0;
  fp = fopen(path, "wb");
  if (!fp) {
    if (errno != 0) {
      csvperror(path, 0, 0, CSV_EL_ERROR, "open", CSV_ERR_SYS, errno, 0, NULL);
    } else {
      csvperror(path, 0, 0, CSV_EL_ERROR, "open", CSV_ERR_FOPEN, 0, 0, NULL);
    }
    retv = 2;
    goto cleanup;
  }

  msgpackx_data_write(mdata, fp);

cleanup:
  if (fp)
    fclose(fp);
  if (mdata)
    msgpackx_data_delete(mdata);
  return retv;

serror:
  csvperror(path, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, 0, err, NULL);
  retv = 1;
  goto cleanup;

allocerror:
  csvperror(path, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0, NULL);
  retv = 1;
  goto cleanup;

  //--
  CSVUNREACHABLE();
}

int read_lpt_ctrl_data(const char *path, jupiter_lpt_ctrl_data **data)
{
  FILE *fp;
  jupiter_lpt_ctrl_data *xdata;
  msgpackx_data *mdata;
  msgpackx_error err;
  msgpackx_array_node *anode;
  msgpackx_map_node *mhead;
  int retv;

  err = MSGPACKX_SUCCESS;
  mdata = NULL;
  xdata = NULL;
  retv = 0;
  errno = 0;
  fp = fopen(path, "rb");
  if (!fp) {
    if (errno != 0) {
      csvperror(path, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SYS, errno, 0, NULL);
    } else {
      csvperror(path, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_FOPEN, 0, 0, NULL);
    }
    retv = 2;
    goto error;
  }

  mdata = msgpackx_data_read(fp, &err, NULL);
  fclose(fp);
  if (!mdata) {
    csvperror(path, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, errno, err,
              NULL);
    goto error;
  }

  anode = msgpackx_read_header_data(mdata, JUPITER_LPT_CTRL_TITLE, NULL, &err);
  if (!anode) {
    csvperror(path, 0, 0, CSV_EL_ERROR, NULL, CSV_ERR_SERIALIZE, errno, err,
              NULL);
    goto error;
  }

  mhead = msgpackx_array_node_get_map(anode, &err);
  if (err != MSGPACKX_SUCCESS) {
    csvperrorf(path, 0, 0, CSV_EL_ERROR, NULL, "lpt_ctrl data must be a map");
    goto error;
  }

  CSVASSERT(mhead);
  {
    intmax_t iv;
    iv = msgpackx_map_skey_get_int(mhead, MCSTR("v"), &err);
    if (iv != JUPITER_LPT_CTRL_VERSION || err != MSGPACKX_SUCCESS) {
      csvperrorf(path, 0, 0, CSV_EL_ERROR, NULL,
                 "Version does not match to expected or failed to fetch");
      goto error;
    }
  }

  xdata = new_lpt_ctrl_data();
  if (!xdata) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    retv = 3;
    goto error;
  }

  {
    intmax_t iv;
    iv = msgpackx_map_skey_get_int(mhead, MCSTR("npt"), &err);
    if (err != MSGPACKX_SUCCESS) {
      csvperror(path, 0, 0, CSV_EL_ERROR, "npt", CSV_ERR_SERIALIZE, errno, err,
                NULL);
      goto error;
    }
    xdata->npt = iv;
    if (xdata->npt != iv) {
      csvperrorf(path, 0, 0, CSV_EL_ERROR, NULL,
                 "Number of particles %" PRIdMAX " is too big", iv);
      goto error;
    }
  }

  {
    msgpackx_node *node;
    msgpackx_map_node *mnode;
    double dv;
    float fv;
    type tv;

    mnode = msgpackx_map_node_find_by_str(mhead, MCSTR("timprn"), &err);
    if (err == MSGPACKX_SUCCESS) {
      if (mnode == mhead) {
        err = MSGPACKX_ERR_KEYNOTFOUND;
      } else {
        node = msgpackx_map_node_get_value(mnode);
        if (!node)
          err = MSGPACKX_ERR_MSG_TYPE;
      }
    }
    if (err != MSGPACKX_SUCCESS) {
      csvperror(path, 0, 0, CSV_EL_ERROR, "timprn", CSV_ERR_SERIALIZE, errno,
                err, NULL);
      goto error;
    }

#ifdef JUPITER_DOUBLE
    dv = msgpackx_node_get_double(node, &err);
#else
    fv = msgpackx_node_get_float(node, &err);
#endif
    if (err != MSGPACKX_SUCCESS) {
      if (err != MSGPACKX_ERR_MSG_TYPE) {
        csvperrorf(path, 0, 0, CSV_EL_ERROR, NULL,
                   "Failed to obtain timprn data");
        goto error;
      }
      err = MSGPACKX_SUCCESS;
#ifdef JUPITER_DOUBLE
      fv = msgpackx_node_get_float(node, &err);
#else
      dv = msgpackx_node_get_double(node, &err);
#endif
      if (err != MSGPACKX_SUCCESS) {
        csvperrorf(path, 0, 0, CSV_EL_ERROR, NULL,
                   "Failed to obtain timprn data");
        goto error;
      }

      retv = 0;
#ifdef JUPITER_DOUBLE
      tv = fv;
      /* possibly always false for IEEE-754 binary float */
      if (tv != fv)
        retv = 1;
#else
      tv = dv;
      if (tv != dv)
        retv = 1;
#endif
      if (retv) {
        csvperrorf(path, 0, 0, CSV_EL_WARN, NULL,
                   "Truncation error occured upon reading TIMPRN data");
      } else {
        const char *ftype, *ttype;
#ifdef JUPITER_DOUBLE
        ftype = "SINGLE";
        ttype = "DOUBLE";
#else
        ftype = "DOUBLE";
        ttype = "SINGLE";
#endif
        csvperrorf(path, 0, 0, CSV_EL_WARN, NULL,
                   "TIMPRN is stored in %s precision data, converted to %s "
                   "precision",
                   ftype, ttype);
      }
      retv = 0;
    } else {
#ifdef JUPITER_DOUBLE
      tv = dv;
#else
      tv = fv;
#endif
    }
    xdata->timprn = tv;
  }

  *data = xdata;
  msgpackx_data_delete(mdata);
  return 0;

error:
  if (retv == 0)
    retv = 1;
  if (xdata)
    delete_lpt_ctrl_data(xdata);
  if (mdata)
    msgpackx_data_delete(mdata);
  return retv;
}

jupiter_lpt_ctrl_data *get_lpt_ctrl_data(void)
{
  jupiter_lpt_ctrl_data *retv;

  retv = new_lpt_ctrl_data();
  if (!retv)
    return NULL;

#ifdef LPT
  retv->npt = cLPTgetnpt();
  retv->timprn = cLPTgettimprn();
#else
  retv->npt = 0;
  retv->timprn = 0.0;
#endif
  return retv;
}

void set_lpt_ctrl_data(jupiter_lpt_ctrl_data *data)
{
  CSVASSERT(data);

#ifdef LPT
  cLPTsettimprn(data->timprn);
#endif
}

void delete_lpt_ctrl_data(jupiter_lpt_ctrl_data *data) { free(data); }

#ifdef LPTX
static LPTX_bool jLPTX_bool(int jbool)
{
  if (jbool == ON)
    return LPTX_true;
  return LPTX_false;
}

static int jLPTX_param_set_impl(LPTX_param *lpt_param, flags *flg, domain *cdo,
                                mpi_param *mpi)
{
  if (flg) {
    LPTX_bool br, thpf, use_const_Cc;
    LPTX_param_set_time_scheme(lpt_param, flg->lpt_ipttim);
    LPTX_param_set_heat_scheme(lpt_param, flg->lpt_heat);

    br = jLPTX_bool(flg->lpt_brownian);
    LPTX_param_set_brownian_force(lpt_param, br);

    thpf = jLPTX_bool(flg->lpt_thermophoretic);
    LPTX_param_set_thermophoretic_force(lpt_param, thpf);

    use_const_Cc = jLPTX_bool(flg->lpt_use_constant_Cc);
    LPTX_param_set_use_constant_cunningham_correction(lpt_param, use_const_Cc);
  }

  if (cdo) {
    LPTX_idtype ncompo;
    LPTX_vector grav = LPTX_vector_c(cdo->grav_x, cdo->grav_y, cdo->grav_z);
    LPTX_param_set_gravity(lpt_param, grav);

    LPTX_param_set_cunningham_correction(lpt_param, cdo->lpt_cc);
    LPTX_param_set_cunningham_correction_A1(lpt_param, cdo->lpt_cc_A1);
    LPTX_param_set_cunningham_correction_A2(lpt_param, cdo->lpt_cc_A2);
    LPTX_param_set_cunningham_correction_A3(lpt_param, cdo->lpt_cc_A3);
    LPTX_param_set_thermophoretic_force_constant_Cm(lpt_param, cdo->lpt_tp_Cm);
    LPTX_param_set_thermophoretic_force_constant_Cs(lpt_param, cdo->lpt_tp_Cs);
    LPTX_param_set_thermophoretic_force_constant_Ct(lpt_param, cdo->lpt_tp_Ct);

    ncompo = component_info_ncompo(&cdo->lpt_mass_fractions);
    if (ncompo > 0) {
      LPTX_param_set_particle_vectors(lpt_param, LPTX_NUM_VECTORS, NULL);
      LPTX_param_set_number_of_particle_vector_size(lpt_param,
                                                    LPTX_VI_MASS_FRACTION,
                                                    ncompo);
    }
  }

  if (mpi) {
#ifdef JUPITER_MPI
    LPTX_param_set_mpi_comm(lpt_param, mpi->CommJUPITER);
#endif
  }

  return 0;
}

int jLPTX_param_set(LPTX_param *lpt_param, flags *flg, domain *cdo,
                    mpi_param *mpi)
{
  CSVASSERT(flg);
  CSVASSERT(cdo);
  CSVASSERT(mpi);

  return jLPTX_param_set_impl(lpt_param, flg, cdo, mpi);
}

LPTX_param *jLPTX_param_for_flg(flags *flg)
{
  CSVASSERT(flg);

  LPTX_param *p = LPTX_param_new();
  if (!p) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

  jLPTX_param_set_impl(p, flg, NULL, NULL);
  return p;
}

#endif
