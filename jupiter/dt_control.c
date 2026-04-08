#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <inttypes.h>
#ifdef JUPITER_MPI
#include <mpi.h>
#endif
#ifdef GPU
#  include <cuda.h>
#  include <cutil.h>
#endif
#include "struct.h"
#include "func.h"
#include "csv.h"
#include "csvutil.h"
#include "boundary_util.h"
#include "tempdep_calc.h"

/* YSE: Add for calc_address() */
#include "common_util.h"
#include "os/os.h"

#ifdef LPTX
#include "lptx/defs.h"
#include "lptx/param.h"
#include "lptx/struct_defs.h"
#include "lptx/stat.h"
#endif

type abs_max(type *f, int *i_max, int *j_max, int *k_max, int mx, int my, int is, int ie, int js, int je, int ks, int ke, parameter *prm)
{
  int  i, j, k;
  type max = 0.0, max_global;
  for(k = ks; k < ke+1; k++) {
    for(j = js; j < je+1; j++) {
      for(i = is; i < ie+1; i++) {
        if (max < fabs(f[i + mx*j + mx*my*k])) {
          max = fabs(f[i + mx*j + mx*my*k]);
          *i_max = i;
          *j_max = j;
          *k_max = k;
        }
      }
    }
  }
  max_global = max;
#ifdef JUPITER_MPI
  MPI_Allreduce(&max, &max_global, 1, MPI_TYPE, MPI_MAX, prm->mpi->CommJUPITER);
#endif
  return max_global;
}

type sum_all(type *f, domain *cdo, parameter *prm)
{
  type sum;

#pragma omp parallel
  {
    int i, j, k,
        is = cdo->stm, js = cdo->stm, ks = cdo->stm, 
        ie = is + cdo->nx - 1, je = js + cdo->ny - 1, ke = ks + cdo->nz - 1,
        mx = cdo->mx, my = cdo->my, mz = cdo->mz; 

    ptrdiff_t jj;

//#pragma omp for reduction (+:sum) collapse(3) 
#pragma omp for reduction (+:sum) private (j, i, jj)
    for (k = ks; k < ke + 1; ++k) {
      for (j = js; j < je + 1; ++j) {
        for (i = is; i < ie + 1; ++i) {
          ptrdiff_t jj;
          jj = calc_address(i, j, k, mx, my, mz);
          sum += f[jj];
        }
      }
    }

  }

#ifdef JUPITER_MPI
  MPI_Allreduce(MPI_IN_PLACE, &sum, 1, MPI_TYPE, MPI_SUM, prm->mpi->CommJUPITER); 
#endif

  return sum;
}

static type calc_surface_boundary_gas_fraction(struct surface_boundary_data *sb)
{
  if (!sb || !sb->comps)
    return 0.0;

  int ncompo = inlet_component_data_ncomp(sb->comps);
  type fa = 0.0;
  type fg = 0.0;

  for (int ic = 0; ic < ncompo; ++ic) {
    struct inlet_component_element *e = inlet_component_data_get(sb->comps, ic);
    if (!e)
      continue;
    type fi = e->ratio.current_value;
    int id = e->comp.d->comp_index;
    fa += fi;
    if (id == -1) {
      fg += fi;
    }
  }

  if (fa > 0.0)
    return fg / fa;

  return 0.0;
}

static type calc_gas_jet_flow_rate(variable *val, parameter *prm)
{
  if (!val || !val->surface_bnd)
    return 0.0;

  domain *cdo = prm->cdo;
  int nx = cdo->nx, ny = cdo->ny, nz = cdo->nz;
  int mx = cdo->mx, mxy = cdo->mxy, stm = cdo->stm;
  type dx = cdo->dx, dy = cdo->dy, dz = cdo->dz;
  type rate = 0.0;
  type area_x = dy * dz;
  type area_y = dx * dz;
  type area_z = dx * dy;

  if (!val->bnd_norm_u || !val->bnd_norm_v || !val->bnd_norm_w)
    return 0.0;

#pragma omp parallel for reduction(+:rate)
  for (int jz = 0; jz < nz + 1; ++jz) {
    for (int jy = 0; jy < ny + 1; ++jy) {
      for (int jx = 0; jx < nx + 1; ++jx) {
        int j = (jx + stm) + mx * (jy + stm) + mxy * (jz + stm);
        struct surface_boundary_data *sb = val->surface_bnd[3 * j];
        if (!sb || sb->cond != INLET || sb->inlet_dir != SURFACE_INLET_DIR_NORMAL)
          continue;
        type bnd_norm = val->bnd_norm_u[3 * j];
        if (bnd_norm == 0.0)
          continue;
        type un = calc_surface_boundary_vel(0.0, sb, bnd_norm);
        type flow = un * bnd_norm;
        if (flow <= 0.0)
          continue;
        type fgas = calc_surface_boundary_gas_fraction(sb);
        if (fgas <= 0.0)
          continue;
        rate += fgas * flow * area_x;
      }
    }
  }

#pragma omp parallel for reduction(+:rate)
  for (int jz = 0; jz < nz + 1; ++jz) {
    for (int jy = 0; jy < ny + 1; ++jy) {
      for (int jx = 0; jx < nx + 1; ++jx) {
        int j = (jx + stm) + mx * (jy + stm) + mxy * (jz + stm);
        struct surface_boundary_data *sb = val->surface_bnd[3 * j + 1];
        if (!sb || sb->cond != INLET || sb->inlet_dir != SURFACE_INLET_DIR_NORMAL)
          continue;
        type bnd_norm = val->bnd_norm_v[3 * j + 1];
        if (bnd_norm == 0.0)
          continue;
        type un = calc_surface_boundary_vel(0.0, sb, bnd_norm);
        type flow = un * bnd_norm;
        if (flow <= 0.0)
          continue;
        type fgas = calc_surface_boundary_gas_fraction(sb);
        if (fgas <= 0.0)
          continue;
        rate += fgas * flow * area_y;
      }
    }
  }

#pragma omp parallel for reduction(+:rate)
  for (int jz = 0; jz < nz + 1; ++jz) {
    for (int jy = 0; jy < ny + 1; ++jy) {
      for (int jx = 0; jx < nx + 1; ++jx) {
        int j = (jx + stm) + mx * (jy + stm) + mxy * (jz + stm);
        struct surface_boundary_data *sb = val->surface_bnd[3 * j + 2];
        if (!sb || sb->cond != INLET || sb->inlet_dir != SURFACE_INLET_DIR_NORMAL)
          continue;
        type bnd_norm = val->bnd_norm_w[3 * j + 2];
        if (bnd_norm == 0.0)
          continue;
        type un = calc_surface_boundary_vel(0.0, sb, bnd_norm);
        type flow = un * bnd_norm;
        if (flow <= 0.0)
          continue;
        type fgas = calc_surface_boundary_gas_fraction(sb);
        if (fgas <= 0.0)
          continue;
        rate += fgas * flow * area_z;
      }
    }
  }

#ifdef JUPITER_MPI
  MPI_Allreduce(MPI_IN_PLACE, &rate, 1, MPI_TYPE, MPI_SUM, prm->mpi->CommJUPITER);
#endif

  return rate;
}

/* YSE: re-implement minmax */
struct ya_max_data_element
{
  int i, j, k;
  type value;
};

struct ya_max_data
{
  struct ya_max_data_element local;
  struct ya_max_data_element global;
};

static void
ya_max_data_local_to_global(mpi_param *mpi, struct ya_max_data *result,
                            int op_is_min, int is, int js, int ks,
                            int ie, int je, int ke)
{
#ifdef JUPITER_MPI
  int tmp[3];

#ifdef JUPITER_DOUBLE
  MPI_Datatype type_int = MPI_DOUBLE_INT;
  struct type_int {
    double val;
    int loc;
  } max;
#else
  MPI_Datatype type_int = MPI_FLOAT_INT;
  struct type_int {
    float val;
    int loc;
  } max;
#endif

  max.val = result->local.value;
  max.loc = mpi->rank;
  if (op_is_min) {
    MPI_Allreduce(MPI_IN_PLACE, &max, 1, type_int, MPI_MINLOC,
                  mpi->CommJUPITER);
  } else {
    MPI_Allreduce(MPI_IN_PLACE, &max, 1, type_int, MPI_MAXLOC,
                  mpi->CommJUPITER);
  }

  tmp[0] = mpi->rank_x;
  tmp[1] = mpi->rank_y;
  tmp[2] = mpi->rank_z;

  MPI_Bcast(tmp, 3, MPI_INT, max.loc, mpi->CommJUPITER);

  if (mpi->rank_y == 0 && mpi->rank_z == 0 && mpi->rank_x < tmp[0]) {
    tmp[0] = ie - is + 1;
  } else {
    tmp[0] = 0;
  }
  if (mpi->rank_x == 0 && mpi->rank_z == 0 && mpi->rank_y < tmp[1]) {
    tmp[1] = je - js + 1;
  } else {
    tmp[1] = 0;
  }
  if (mpi->rank_x == 0 && mpi->rank_y == 0 && mpi->rank_z < tmp[2]) {
    tmp[2] = ke - ks + 1;
  } else {
    tmp[2] = 0;
  }

  MPI_Allreduce(MPI_IN_PLACE, tmp, 3, MPI_INT, MPI_SUM, mpi->CommJUPITER);

  result->global.i = tmp[0];
  result->global.j = tmp[1];
  result->global.k = tmp[2];

  tmp[0] = result->local.i;
  tmp[1] = result->local.j;
  tmp[2] = result->local.k;

  MPI_Bcast(tmp, 3, MPI_INT, max.loc, mpi->CommJUPITER);

  result->global.i = result->global.i + tmp[0] - is;
  result->global.j = result->global.j + tmp[1] - js;
  result->global.k = result->global.k + tmp[2] - ks;
  result->global.value = max.val;

#else
  result->global = result->local;
  result->global.i -= is;
  result->global.j -= js;
  result->global.k -= ks;
#endif
}

static void
ya_abs_max(mpi_param *mpi, type *f, struct ya_max_data *result,
           int mx, int my, int mz,
           int is, int ie, int js, int je, int ks, int ke)
{
  int gmi, gmj, gmk;
  type max;

  max = f[calc_address(is, js, ks, mx, my, mz)];
  gmi = is;
  gmj = js;
  gmk = ks;

#pragma omp parallel
  {
    int i, j, k;
    int lmi, lmj, lmk;
    type lmax;
    ptrdiff_t jj;

#pragma omp atomic read
    lmax = max;

#pragma omp for collapse(3)
    for (k = ks; k < ke + 1; ++k) {
      for (j = js; j < je + 1; ++j) {
        for (i = is; i < ie + 1; ++i) {
          ptrdiff_t jj;
          type v;
          jj = calc_address(i, j, k, mx, my, mz);
          v = fabs(f[jj]);
          if (lmax < v) {
            lmax = v;
            lmi = i;
            lmj = j;
            lmk = k;
          }
        }
      }
    }

#pragma omp critical
    {
      if (max < lmax) {
        max = lmax;
        gmi = lmi;
        gmj = lmj;
        gmk = lmk;
      }
    }
  }

  result->local.i = gmi;
  result->local.j = gmj;
  result->local.k = gmk;
  result->local.value = max;

  ya_max_data_local_to_global(mpi, result, 0, is, js, ks, ie, je, ke);
}

static void
ya_abs_max_exclude_fs(mpi_param *mpi, type *f, type *fs, type fs_thr,
                      struct ya_max_data *result,
                      int mx, int my, int mz,
                      int is, int ie, int js, int je, int ks, int ke)
{
  int gmi, gmj, gmk;
  type max;

  if (!fs) {
    ya_abs_max(mpi, f, result, mx, my, mz, is, ie, js, je, ks, ke);
    return;
  }

  max = 0.0;
  gmi = is;
  gmj = js;
  gmk = ks;

#pragma omp parallel
  {
    int i, j, k;
    int lmi, lmj, lmk;
    type lmax;
    ptrdiff_t jj;

    lmi = gmi;
    lmj = gmj;
    lmk = gmk;

#pragma omp atomic read
    lmax = max;

#pragma omp for collapse(3)
    for (k = ks; k < ke + 1; ++k) {
      for (j = js; j < je + 1; ++j) {
        for (i = is; i < ie + 1; ++i) {
          ptrdiff_t jj;
          type v;
          jj = calc_address(i, j, k, mx, my, mz);
          if (fs[jj] >= fs_thr)
            continue;
          v = fabs(f[jj]);
          if (lmax < v) {
            lmax = v;
            lmi = i;
            lmj = j;
            lmk = k;
          }
        }
      }
    }

#pragma omp critical
    {
      if (max < lmax) {
        max = lmax;
        gmi = lmi;
        gmj = lmj;
        gmk = lmk;
      }
    }
  }

  result->local.i = gmi;
  result->local.j = gmj;
  result->local.k = gmk;
  result->local.value = max;

  ya_max_data_local_to_global(mpi, result, 0, is, js, ks, ie, je, ke);
}

static void
ya_max_cdo(mpi_param *mpi, domain *cdo, type *f,
           int istag, int jstag, int kstag, struct ya_max_data *result,
           void (*fun)(mpi_param *mpi, type *f, struct ya_max_data *result,
                       int mx, int my, int mz,
                       int is, int ie, int js, int je, int ks, int ke))
{
  int mx, my, mz;
  int nx, ny, nz;
  int is, js, ks;
  int ie, je, ke;

  mx = cdo->mx;
  my = cdo->my;
  mz = cdo->mz;

  nx = cdo->nx;
  ny = cdo->ny;
  nz = cdo->nz;

  if (istag && mpi->rank_x == mpi->npe_x - 1) {
    nx++;
  }
  if (jstag && mpi->rank_y == mpi->npe_y - 1) {
    ny++;
  }
  if (kstag && mpi->rank_z == mpi->npe_z - 1) {
    nz++;
  }

  is = cdo->stm;
  js = cdo->stm;
  ks = cdo->stm;
  ie = is + nx - 1;
  je = js + ny - 1;
  ke = ks + nz - 1;

  fun(mpi, f, result, mx, my, mz, is, ie, js, je, ks, ke);
}

static void
ya_max_cdo_exclude_fs(mpi_param *mpi, domain *cdo, type *f, type *fs,
                      type fs_thr, int istag, int jstag, int kstag,
                      struct ya_max_data *result)
{
  int mx, my, mz;
  int nx, ny, nz;
  int is, js, ks;
  int ie, je, ke;

  mx = cdo->mx;
  my = cdo->my;
  mz = cdo->mz;

  nx = cdo->nx;
  ny = cdo->ny;
  nz = cdo->nz;

  if (istag && mpi->rank_x == mpi->npe_x - 1) {
    nx++;
  }
  if (jstag && mpi->rank_y == mpi->npe_y - 1) {
    ny++;
  }
  if (kstag && mpi->rank_z == mpi->npe_z - 1) {
    nz++;
  }

  is = cdo->stm;
  js = cdo->stm;
  ks = cdo->stm;
  ie = is + nx - 1;
  je = js + ny - 1;
  ke = ks + nz - 1;

  ya_abs_max_exclude_fs(mpi, f, fs, fs_thr, result,
                        mx, my, mz, is, ie, js, je, ks, ke);
}

static void
ya_print_max(FILE *fp, struct ya_max_data *data, const char *title,
             int width, int precision)
{
  fprintf(fp, "  %-7s(%4d,%4d,%4d) = %*.*e\n",
          title, data->global.i, data->global.j, data->global.k,
          width, precision, data->global.value);
}

static int
ox_flag_counter(mpi_param *mpi, domain *cdo, int *ox_flag,
                int value_to_count, int neighbor_value)
{
  int icnt, j;
  icnt = 0;

#pragma omp parallel for reduction(+: icnt)
  for (j = 0; j < cdo->n; ++j) {
    ptrdiff_t jj;
    int jx, jy, jz;
    int icnd;

    calc_struct_index(j, cdo->nx, cdo->ny, cdo->nz, &jx, &jy, &jz);
    jj = calc_address(jx + cdo->stm, jy + cdo->stm, jz + cdo->stm,
                      cdo->mx, cdo->my, cdo->mz);

    icnd = 0;
    if (neighbor_value >= 0) { /* Assumes negative values are not used */
      if (mpi->nrk[0] != -1 || jz > 0) {
        if (ox_flag[jj - cdo->mxy] == neighbor_value) {
          icnd = 1;
        }
      }
      if (mpi->nrk[1] != -1 || jz < cdo->nz - 1) {
        if (ox_flag[jj + cdo->mxy] == neighbor_value) {
          icnd = 1;
        }
      }
      if (mpi->nrk[2] != -1 || jy > 0) {
        if (ox_flag[jj - cdo->mx] == neighbor_value) {
          icnd = 1;
        }
      }
      if (mpi->nrk[3] != -1 || jy < cdo->ny - 1) {
        if (ox_flag[jj + cdo->mx] == neighbor_value) {
          icnd = 1;
        }
      }
      if (mpi->nrk[4] != -1 || jx > 0) {
        if (ox_flag[jj - 1] == neighbor_value) {
          icnd = 1;
        }
      }
      if (mpi->nrk[5] != -1 || jx < cdo->nx - 1) {
        if (ox_flag[jj + 1] == neighbor_value) {
          icnd = 1;
        }
      }
    } else {
      icnd = 1;
    }
    if (icnd) {
      if (ox_flag[jj] == value_to_count) {
        icnt++;
      }
    }
  }

#ifdef JUPITER_MPI
  MPI_Allreduce(MPI_IN_PLACE, &icnt, 1, MPI_INT, MPI_SUM, mpi->CommJUPITER);
#endif
  return icnt;
}

#ifdef LPTX
static void print_lptx_state(FILE *fp, const LPTX_particle_stat *stat, int npe,
                             int print_by_rank, const char *title,
                             LPTX_idtype (*getter)(const LPTX_particle_stat *))
{
  LPTX_idtype s = 0;
  for (int i = 0; i < npe; ++i) {
    s += getter(&stat[i]);
  }
  fprintf(fp, " * %-9s = %" PRIdMAX "\n", title, (intmax_t)s);

  if (print_by_rank) {
    int nc = 1;
    int ne = 1;
    for (int i = 0; i < npe; ++i) {
      s = getter(&stat[i]);
      for (int n = 2; s > 9; s /= 10, n++) {
        if (n > nc)
          nc = n;
      }
    }
    for (int npec = npe; npec > 9; npec /= 10, ne++)
      /* nop */;
    for (int i = 0; i < npe; ++i) {
      s = getter(&stat[i]);
      fprintf(fp, "  (in rank %*d: %*" PRIdMAX ")\n", ne, i, nc, (intmax_t)s);
    }
  }
}

static void print_lptx_stat1(FILE *fp, const LPTX_particle_stat *stat, int npe,
                             int print_by_rank)
{
  print_lptx_state(fp, stat, npe, print_by_rank, "Allocated",
                   LPTX_particle_stat_get_allocated);
  print_lptx_state(fp, stat, npe, print_by_rank, "Tracked",
                   LPTX_particle_stat_get_tracked);
  print_lptx_state(fp, stat, npe, print_by_rank, "Exited",
                   LPTX_particle_stat_get_exited);
  print_lptx_state(fp, stat, npe, print_by_rank, "Collided",
                   LPTX_particle_stat_get_collided);
  if (npe > 1) {
    print_lptx_state(fp, stat, npe, print_by_rank, "Sent",
                     LPTX_particle_stat_get_sent);
    print_lptx_state(fp, stat, npe, print_by_rank, "Received",
                     LPTX_particle_stat_get_recved);
  }
}

static void print_lptx_stat(FILE *fp, const LPTX_particle_stat *cuml,
                            const LPTX_particle_stat *last, int npe,
                            int print_by_rank)
{
  fprintf(fp, "-----[LPT calculation stats]----------------\n");
  print_lptx_stat1(fp, last, npe, print_by_rank);

  if (cuml->nstep > 1) {
    fprintf(fp, "-- for last %" PRIdMAX" steps\n", (intmax_t)cuml->nstep);
    print_lptx_stat1(fp, cuml, npe, print_by_rank);
  }
}
#endif

type dt_control(variable *val, material *mtl, parameter *prm)
{
  domain      *cdo = prm->cdo;
  phase_value *phv = prm->phv;
  int    nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
         mx=cdo->mx, my=cdo->my, stm=cdo->stm;
  type   dt_cfl=1.0, dt_diff=1.0, dt_diff_org=1.0, dx=cdo->dx, dy=cdo->dy;
  type   time0 = cpu_time();
  type dt_g, dt_s;
  FILE   *fp = prm->flg->fp;
  FILE   *pp;
  char filename[100];

  struct ya_max_data u_maxs, v_maxs, w_maxs, t_maxs, tf_maxs, ts_maxs, q_maxs, rad_maxs;
  struct ya_max_data ox_dt_maxs, ox_q_maxs;
  type enthalpy_sum, init_enthalpy_sum, enthalpy_time_derivative_sum; /*Fukuda enthaply monitor*/
  type enthalpy_icrease_from_beginning_sum;
  type u_max, v_max, w_max, vel_max, t_max, tf_max, ts_max;
  type fs_thr = (type)0.999;
  int ox_f_IB, ox_f_OB, ox_f_S, ox_f_F, ox_f_G, ox_f_R;
  int ox_f_IB_nG, ox_f_OB_nG, ox_f_S_nG, ox_f_F_nG, ox_f_R_nG;

#ifdef LPTX
  const LPTX_particle_stat *cstat, *lstat;
  LPTX_particle_stat *castat, *lastat;
  int nastat;
#endif

  // search maximum value and its index
  ya_max_cdo_exclude_fs(prm->mpi, cdo, val->u, val->fs, fs_thr, 1, 0, 0, &u_maxs);
  ya_max_cdo_exclude_fs(prm->mpi, cdo, val->v, val->fs, fs_thr, 0, 1, 0, &v_maxs);
  ya_max_cdo_exclude_fs(prm->mpi, cdo, val->w, val->fs, fs_thr, 0, 0, 1, &w_maxs);
  if (prm->flg->two_energy == OFF)
    ya_max_cdo(prm->mpi, cdo, val->t, 0, 0, 0, &t_maxs, ya_abs_max);
  else {
    ya_max_cdo(prm->mpi, cdo, val->tf, 0, 0, 0, &tf_maxs, ya_abs_max);
    ya_max_cdo(prm->mpi, cdo, val->ts, 0, 0, 0, &ts_maxs, ya_abs_max);
  }
  ya_max_cdo(prm->mpi, cdo, mtl->q, 0, 0, 0, &q_maxs, ya_abs_max);
  ya_max_cdo(prm->mpi, cdo, mtl->rad, 0, 0, 0, &rad_maxs, ya_abs_max);
  if (val->ox_dt) {
    ya_max_cdo(prm->mpi, cdo, val->ox_dt, 0, 0, 0, &ox_dt_maxs, ya_abs_max);
  }
  if (val->ox_q) {
    ya_max_cdo(prm->mpi, cdo, val->ox_q, 0, 0, 0, &ox_q_maxs, ya_abs_max);
  }

    /*Fukuda enthaply monitor*/
  if(prm->flg->heat_eq ==ON && prm->flg->phase_change == OFF && prm->flg->two_energy == OFF){
    enthalpy_sum = sum_all(val->enthalpy, cdo, prm);
    enthalpy_time_derivative_sum = sum_all(val->enthalpy_time_derivative, cdo, prm);
    if(cdo->icnt==1){
      cdo->init_total_enthalpy = sum_all(val->init_enthalpy, cdo, prm);
    }
    if(cdo->icnt>=1){
      enthalpy_icrease_from_beginning_sum = enthalpy_sum - cdo->init_total_enthalpy;
    }
  }

  if (val->ox_flag) {
    mpi_param *mpi = prm->mpi;
    int *f = val->ox_flag;
    ox_f_IB = ox_flag_counter(mpi, cdo, f, OX_STATE_IN_BOUNDS, -1);
    ox_f_OB = ox_flag_counter(mpi, cdo, f, OX_STATE_OUT_OF_BOUNDS, -1);
    ox_f_S = ox_flag_counter(mpi, cdo, f, OX_STATE_STARTED, -1);
    ox_f_F = ox_flag_counter(mpi, cdo, f, OX_STATE_FINISHED, -1);
    ox_f_G = ox_flag_counter(mpi, cdo, f, OX_STATE_GAS, -1);
    ox_f_R = ox_flag_counter(mpi, cdo, f, OX_STATE_RECESSING, -1);
    ox_f_IB_nG = ox_flag_counter(mpi, cdo, f, OX_STATE_IN_BOUNDS, OX_STATE_GAS);
    ox_f_OB_nG = ox_flag_counter(mpi, cdo, f, OX_STATE_OUT_OF_BOUNDS, OX_STATE_GAS);
    ox_f_S_nG = ox_flag_counter(mpi, cdo, f, OX_STATE_STARTED, OX_STATE_GAS);
    ox_f_F_nG = ox_flag_counter(mpi, cdo, f, OX_STATE_FINISHED, OX_STATE_GAS);
    ox_f_R_nG = ox_flag_counter(mpi, cdo, f, OX_STATE_RECESSING, OX_STATE_GAS);
  }
  if (prm->flg->oxidation == ON && cdo->icnt % cdo->view == 0) {
#ifdef JUPITER_MPI
    int i;
    size_t buf[prm->mpi->npe];
    /* MPI_Allreduce cannot be performed on size_t. */
    MPI_Allgather(&cdo->ox_nox_calc, sizeof(size_t), MPI_CHAR,
                  buf, sizeof(size_t), MPI_CHAR, prm->mpi->CommJUPITER);
    cdo->ox_nox_calc = 0;
    for (i = 0; i < prm->mpi->npe; ++i) {
      cdo->ox_nox_calc += buf[i];
    }

    MPI_Allgather(&cdo->ox_nre_calc, sizeof(size_t), MPI_CHAR,
                  buf, sizeof(size_t), MPI_CHAR, prm->mpi->CommJUPITER);
    cdo->ox_nre_calc = 0;
    for (i = 0; i < prm->mpi->npe; ++i) {
      cdo->ox_nre_calc += buf[i];
    }
#endif
  }

#ifdef LPTX
  castat = NULL;
  lastat = NULL;
  nastat = 0;

  if (for_any_rank(prm->mpi, !!val->lpt_param)) {
    int r;
    LPTX_particle_stat dummy;
    if (val->lpt_param) {
      LPTX_param_count_allocated(val->lpt_param);
      cstat = LPTX_param_get_cumulative_stat(val->lpt_param);
      lstat = LPTX_param_get_last_step_stat(val->lpt_param);
      nastat = 1;
    } else {
      LPTX_particle_stat_init(&dummy);
      cstat = &dummy;
      lstat = &dummy;
      nastat = 0;
    }

#ifdef JUPITER_MPI
    r = LPTX_particle_stat_gather(&castat, cstat, prm->mpi->CommJUPITER);
    if (r != MPI_SUCCESS) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_MPI, 0, r,
                NULL);
      prm->status = ON;
    }

    r = LPTX_particle_stat_gather(&lastat, lstat, prm->mpi->CommJUPITER);
    if (r != MPI_SUCCESS) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_MPI, 0, r,
                NULL);
      prm->status = ON;
    }

    cstat = castat;
    lstat = lastat;
    nastat = prm->mpi->npe;
#endif
  }
#endif

  u_max = u_maxs.global.value;
  v_max = v_maxs.global.value;
  w_max = w_maxs.global.value;
  if (prm->flg->two_energy == OFF) {
    t_max = t_maxs.global.value;
    tf_max = 0.0;
    ts_max = 0.0;
  } else {
    t_max = 0.0;
    tf_max = tf_maxs.global.value;
    ts_max = ts_maxs.global.value;
  }

  vel_max = MAX3(u_max, v_max, w_max);
//  vel_max = MAX2(vel_max, 1.0);//<= 1.0 is minimum velocity
  vel_max = MAX2(vel_max, 0.5);//<= 0.1 is minimum velocity, added by Chai
  // vel_max = MAX2(vel_max, 1.0e-02);// < 2016 Added by KKE

  // advection velocity limmiting
  dt_cfl  = cdo->cfl_num * dx / vel_max;
  // Gravity limiting
  dt_g = cdo->cfl_num*dx/(sqrt(u_max*u_max + v_max*v_max + w_max*w_max) + sqrt(u_max*u_max + v_max*v_max + w_max*w_max + 4.0*(-cdo->grav_z)*dx));
  dt_cfl = MIN2(dt_cfl, dt_g);

  // Surface tension limiting
  if (prm->flg->surface_tension == ON) {
    // dt_s = cdo->cfl_num*0.5*pow(cdo->dx,1.5)*sqrt(phv->rho_l[0]/8.0/3.14/phv->sigma[0]);
    dt_s = cdo->cfl_num*sqrt(0.5*(tempdep_calc(&phv->rho_g,273.0)+tempdep_calc(&phv->comps[0].rho_l,273.0))*cdo->dx*cdo->dx*cdo->dx/2.0/M_PI/tempdep_calc(&phv->comps[0].sigma,273.0));
    dt_cfl = MIN2(dt_cfl, dt_s);
  }

  // thermal conductivity limmiting
  /*
    if(prm->flg->heat_eq == ON) {
      if(prm->flg->heat_tvd3 == ON){
        dt_diff_org = cdo->diff_num * dx*dy
          * phv->rho_g
          * phv->specht_s[0]
          / phv->thc_l[1];
        //cdo->nsub_step_t = (int) ( dt_cfl / dt_diff_org ) + 1;// adjust substep number
        cdo->nsub_step_t = 1 * ( (int) ( dt_cfl / dt_diff_org ) + 1 );// adjust substep number
        dt_diff = dt_diff_org*cdo->nsub_step_t;
      }else{
        dt_diff = dt_cfl;
      }
    }
  */

  // choose smaller time step
  //cdo->dt = MIN2(dt_cfl, dt_diff);
  cdo->dt = dt_cfl;
  
  //    if(prm->flg->fluid_dynamics == OFF) {
  //      cdo->dt = dt_diff;
  //    }

  if (cdo->icnt % cdo->view == 0) {
    cdo->gas_jet_flow_rate = calc_gas_jet_flow_rate(val, prm);
  }

  if ( cdo->icnt % cdo->view == 0 && prm->mpi->rank == 0) {
    /* YSE: Set view flag */
    cdo->viewflg = 1;
    fprintf(fp, "\n====[%08d steps, %05d outputs]=============\n", cdo->icnt, cdo->iout);
    fprintf(fp, "  dt(cfl = %1.2f, diff = %1.2f [substep = %03d]) = %9.3e\n",
            cdo->cfl_num, cdo->diff_num, cdo->nsub_step_t, cdo->dt);
    fprintf(fp, "  dt_cfl  = %9.3e\n", dt_cfl);
    fprintf(fp, "  dt_diff = %9.3e (/%d) = %9.3e\n", dt_diff, cdo->nsub_step_t, dt_diff/cdo->nsub_step_t);
    fprintf(fp, "  time    = %9.3e\n", cdo->time);
    ya_print_max(fp, &u_maxs, "u_max", 9, 3);
    ya_print_max(fp, &v_maxs, "v_max", 9, 3);
    ya_print_max(fp, &w_maxs, "w_max", 9, 3);
    if (prm->flg->two_energy == OFF)
      ya_print_max(fp, &t_maxs, "t_max", 15, 10);
    else {
      ya_print_max(fp, &tf_maxs, "tf_max", 15, 10);
      ya_print_max(fp, &ts_maxs, "ts_max", 15, 10);
    }
    ya_print_max(fp, &q_maxs, "q_max", 9, 3);
    ya_print_max(fp, &rad_maxs, "rad_max", 9, 3);
    if (val->ox_dt) {
      ya_print_max(fp, &ox_dt_maxs, "ox_dt_max", 9, 3);
    }
    if (val->ox_q) {
      ya_print_max(fp, &ox_q_maxs, "ox_q_max", 9, 3);
    }
    fprintf(fp, "  flow_rate [kg/s] = %9.3e\n", cdo->flow_rate);
    fprintf(fp, "  gas_jet_flow_rate [m^3/s] = %9.3e\n", cdo->gas_jet_flow_rate);

    /*Fukuda enthaply monitor*/
    if(prm->flg->heat_eq ==ON && prm->flg->phase_change == OFF && prm->flg->two_energy == OFF){
      //if(cdo->icnt>=1) fprintf(fp, "  Initial_total_enthalpy[J] = %9.3e\n", cdo->init_total_enthalpy);
      if(cdo->icnt>=1) fprintf(fp, "  total_enthalpy_change_from_beginning[J] = %9.3e\n", enthalpy_icrease_from_beginning_sum); 
      if(cdo->icnt>=1) fprintf(fp, "  total_enthalpy_time_derivative[J/s] = %9.3e\n", enthalpy_time_derivative_sum/cdo->dt);   
    }

#ifdef LPTX
#ifdef JUPITER_LPTX_REPORT_BY_RANK
#define LPTX_REPORT_BY_RANK ON
#else
#define LPTX_REPORT_BY_RANK OFF
#endif
    if (nastat > 0)
      print_lptx_stat(fp, cstat, lstat, nastat, LPTX_REPORT_BY_RANK);
#undef LPTX_REPORT_BY_RANK
#endif
    
    if (prm->flg->oxidation == ON) {
      fprintf(fp, "----[oxidation status statistics]-----------\n");
      if (val->ox_flag) {
        fprintf(fp, " * Total cell count on this step\n"
                    "   IB = %10d   OB = %10d\n"
                    "    S = %10d    F = %10d\n"
                    "    R = %10d    G = %10d\n"
                    " * Cell count of next to GAS on this step\n"
                    "   IB = %10d   OB = %10d\n"
                    "    S = %10d    F = %10d\n"
                    "    R = %10d\n",
                ox_f_IB, ox_f_OB, ox_f_S, ox_f_F, ox_f_R, ox_f_G,
                ox_f_IB_nG, ox_f_OB_nG, ox_f_S_nG, ox_f_F_nG, ox_f_R_nG);
      }
      fprintf(fp, " * Number of calculations for %d steps:\n"
                  "   Oxidation = %" PRIuMAX "\n"
                  "   Recession = %" PRIuMAX "\n",
              cdo->view, (uintmax_t)cdo->ox_nox_calc,
              (uintmax_t)cdo->ox_nre_calc);
    }
    fprintf(fp, "============================================\n");
    //-- for tuning --
    sprintf(filename,"phys_values.dat");
    if(cdo->iout == 1) pp = fopen(filename,"w");
    else               pp = fopen(filename,"a");
    if(cdo->iout == 1) {
      if (prm->flg->two_energy == OFF) 
        fprintf(pp,"# TIme [sec], u_max, v_max, w_max [m/s], t_max [K]\n");
      else 
        fprintf(pp,"# TIme [sec], u_max, v_max, w_max [m/s], tf_max [K], ts_max [K]\n");
    }
    if (prm->flg->two_energy == OFF) {
      fprintf(pp,"%9.5e %9.5e %9.5e %9.5e %9.5e\n", cdo->time, u_max, v_max, w_max, t_max);
      fclose(pp);
    } else {
      fprintf(pp,"%9.5e %9.5e %9.5e %9.5e %9.5e %9.5e\n", cdo->time, u_max, v_max, w_max, tf_max, ts_max);
      fclose(pp);
    }
  } else {
    cdo->viewflg = 0;
  }
  if (cdo->icnt % cdo->view == 0) {
    cdo->ox_nox_calc = 0;
    cdo->ox_nre_calc = 0;
  }

#ifdef LPTX
  if (val->lpt_param) {
    if (cdo->icnt % cdo->view == 0)
      LPTX_param_reset_cumulative_stat(val->lpt_param);
    LPTX_param_reset_last_stat(val->lpt_param);
  }
#endif

#ifdef LPTX
  if (castat)
    free(castat);
  if (lastat)
    free(lastat);
#endif
  cdo->vel_max = vel_max;
  cdo->t_max   = t_max;
  cdo->tf_max  = tf_max;
  cdo->ts_max  = ts_max;

  return cpu_time() - time0;
}
