#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef GPU
  #include <cuda.h>
  #include <cutil.h>
#endif

#include "common_util.h"
#include "struct.h"
#include "func.h"
#include "os/os.h"

#ifdef _TIME_
extern int it_P;
#endif

static type divergence_surface_boundary_u(type u, struct surface_boundary_data *sb, type bnd_norm)
{
  switch (sb->cond) {
  case INLET:
    switch(sb->inlet_dir) {
    case SURFACE_INLET_DIR_NORMAL:
      return sb->normal_inlet_vel.current_value * bnd_norm;
    case SURFACE_INLET_DIR_INVALID:
      break;
    }
    return u;
  default:
    return u;
  }
}

type divergence(type *div, type *u, type *v, type *w, type *ls, type *fs, type *ls_ibm, type *fs_ibm, struct surface_boundary_data **surf_bnd_array, type *bnd_norm_u, type *bnd_norm_v, type *bnd_norm_w, domain *cdo)
{
  int  j,jx,jy,jz, i,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  type dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi, dti=1.0/cdo->dt, div_max=0.0;

#pragma omp parallel for private(jz,jy,jx,j,i) reduction(max:div_max)

  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        type qs;
        type um, up, vm, vp, wm, wp;
        int calc_surf_bnd;

        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
        i = jx + nx*jy + nx*ny*jz;

        if (fs_ibm) {
          qs = IBM_divergence_vof_qs(u, v, w, fs_ibm, surf_bnd_array,
                                     bnd_norm_u, bnd_norm_v, bnd_norm_w,
                                     j, cdo);
        } else {
          qs = 0.0;
        }

        um = u[j];
        up = u[j + 1];
        vm = v[j];
        vp = v[j + mx];
        wm = w[j];
        wp = w[j + mxy];

        calc_surf_bnd = 0;
        if (surf_bnd_array)
          calc_surf_bnd = 1;
        if (ls_ibm && ls_ibm[j] >= 0.0)
          calc_surf_bnd = 0;

        if (calc_surf_bnd) {
          struct surface_boundary_data *sum, *sup, *svm, *svp, *swm, *swp;
          type snxm, snxp, snym, snyp, snzm, snzp;
          int jxm, jxp, jym, jyp, jzm, jzp;

          jxm = 3 * j;
          jxp = 3 * (j + 1);
          jym = 3 * j + 1;
          jyp = 3 * (j + mx) + 1;
          jzm = 3 * j + 2;
          jzp = 3 * (j + mxy) + 2;

          sum = surf_bnd_array[jxm];
          sup = surf_bnd_array[jxp];
          svm = surf_bnd_array[jym];
          svp = surf_bnd_array[jyp];
          swm = surf_bnd_array[jzm];
          swp = surf_bnd_array[jzp];
          snxm = bnd_norm_u[jxm];
          snxp = bnd_norm_u[jxp];
          snym = bnd_norm_v[jym];
          snyp = bnd_norm_v[jyp];
          snzm = bnd_norm_w[jzm];
          snzp = bnd_norm_w[jzp];
          if (sum)
            um = divergence_surface_boundary_u(um, sum, snxm);
          if (sup)
            up = divergence_surface_boundary_u(up, sup, snxp);
          if (svm)
            vm = divergence_surface_boundary_u(vm, svm, snym);
          if (svp)
            vp = divergence_surface_boundary_u(vp, svp, snyp);
          if (swm)
            wm = divergence_surface_boundary_u(wm, swm, snzm);
          if (swp)
            wp = divergence_surface_boundary_u(wp, swp, snzp);
        }

#ifdef PW
        if (ls_ibm) {
          if (ls_ibm[j] + ls_ibm[j-1  ] >= 0.0)
            um = 0.0;
          if (ls_ibm[j] + ls_ibm[j+1  ] >= 0.0)
            up = 0.0;
          if (ls_ibm[j] + ls_ibm[j-mx ] >= 0.0)
            vm = 0.0;
          if (ls_ibm[j] + ls_ibm[j+mx ] >= 0.0)
            vp = 0.0;
          if (ls_ibm[j] + ls_ibm[j-mxy] >= 0.0)
            wm = 0.0;
          if (ls_ibm[j] + ls_ibm[j+mxy] >= 0.0)
            wp = 0.0;
        }
#endif

        div[i] = ( (up - um)*dxi + (vp - vm)*dyi + (wp - wm)*dzi - qs )*dti;
        if(fabs(div[i]) > div_max) div_max = fabs(div[i]);
      }
    }
  }

  return div_max;
}

type vel_correction(variable *val, material *mtl, parameter *prm)
{
  domain *cdo = prm->cdo;
  int  j, jx,jy,jz,
    nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
    mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  type dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi, dt=cdo->dt;
  type *u=val->u, *v=val->v, *w=val->w, *p=val->p, *dens=mtl->dens;
  type *fl=val->fl;
  type xc,yc,zc;

  /* 2020/6/9, yamashita */
  type *eps=val->eps;

  //=== u ===
#pragma omp parallel private(jz,jy,jx,j)
  {
    type ep_c, ep_n;

#pragma omp for
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx+1; jx++) {
        j = (jx+stm) + mx*(jy+stm) +mxy*(jz+stm);
        ep_c = eps[j];
        ep_n = eps[j - 1];
        if (ep_c <= 0.0) {
          ep_c = ep_n;
        } else if (ep_n <= 0.0) {
          ep_n = ep_c;
        }
        // ep_c = MAX2(ep_c, ep_n);
        ep_c = MIN2(ep_c, ep_n);
        if (ep_c <= 0.0) {
          ep_c = 1.0;
        }
        u[j] = u[j] - ep_c*2.0/(dens[j] + dens[j-1])*(p[j] - p[j-1])*dxi*dt;
      }
    }
  }
  //=== v ===
#pragma omp for
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny+1; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) +mxy*(jz+stm);
        ep_c = eps[j];
        ep_n = eps[j - mx];
        if (ep_c <= 0.0) {
          ep_c = ep_n;
        } else if (ep_n <= 0.0) {
          ep_n = ep_c;
        }
        // ep_c = MAX2(ep_c, ep_n);
        ep_c = MIN2(ep_c, ep_n);
        if (ep_c <= 0.0) {
          ep_c = 1.0;
        }
        v[j] = v[j] - ep_c*2.0/(dens[j] + dens[j-mx])*(p[j] - p[j-mx])*dyi*dt;
      }
    }
  }
  //=== w ===
#pragma omp for
  for(jz = 0; jz < nz+1; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) +mxy*(jz+stm);
        ep_c = eps[j];
        ep_n = eps[j - mxy];
        if (ep_c <= 0.0) {
          ep_c = ep_n;
        } else if (ep_n <= 0.0) {
          ep_n = ep_c;
        }
        // ep_c = MAX2(ep_c, ep_n);
        ep_c = MIN2(ep_c, ep_n);
        if (ep_c <= 0.0) {
          ep_c = 1.0;
        }
        w[j] = w[j] - ep_c*2.0/(dens[j] + dens[j-mxy])*(p[j] - p[j-mxy])*dzi*dt;
      }
    }
  }
  }

  return 0.0;
}

type divergence_free(variable *val, material *mtl, parameter *prm)
{
  if(prm->flg->fluid_dynamics == OFF) return 0.0;
  type   time0 = cpu_time();
  flags  *flg = prm->flg;
  domain *cdo = prm->cdo;
  type *ls = val->ls;
  type *ls_ibm = val->ls_ibm ? val->ls_ibm : ls;
  type *fs = (prm->flg->solute_diff == ON) ? val->fs : val->fs_sum;
  type *fs_ibm = val->fs_ibm ? val->fs_ibm : fs;
  type *div_ls_ibm;
  type *div_fs_ibm;

#ifdef AMGS
  int nx = cdo->nx, ny = cdo->ny, nz = cdo->nz, n = cdo->n;
  int mx = cdo->mx, my = cdo->my, mxy = cdo->mxy, m = cdo->m;
  int stm = cdo->stm;
  type dx = cdo->dx, dy = cdo->dy, dz = cdo->dz;
  type dxi2=cdo->dxi*cdo->dxi, dyi2=cdo->dyi*cdo->dyi, dzi2=cdo->dzi*cdo->dzi, dti=1.0/cdo->dt;
  type bc_xm=flg->bc_xm, bc_xp=flg->bc_xp,
    bc_ym=flg->bc_ym, bc_yp=flg->bc_yp,
    bc_zm=flg->bc_zm, bc_zp=flg->bc_zp;
  mpi_param *mpi = prm->mpi;
  int param[6];
  int mtl_flg, vel_flg;
  printf("AMGS\n");
#endif

  type div_max=0.0;
  //-- divergence
  if (flg->IBM == ON) {
    div_ls_ibm = ls_ibm;
    div_fs_ibm = fs_ibm;
  } else {
    div_ls_ibm = NULL;
    div_fs_ibm = NULL;
  }
  div_max = divergence(mtl->div_b, val->u, val->v, val->w, ls, fs, div_ls_ibm,
                       div_fs_ibm, val->surface_bnd, val->bnd_norm_u,
                       val->bnd_norm_v, val->bnd_norm_w, cdo);

  //--- copy for output
  if (mtl->div_u) {
#pragma omp parallel for
    for (ptrdiff_t j = 0; j < cdo->m; ++j) {
      ptrdiff_t i;
      int jx, jy, jz;
      calc_struct_index(j, cdo->mx, cdo->my, cdo->mz, &jx, &jy, &jz);
      jx -= cdo->stm;
      jy -= cdo->stm;
      jz -= cdo->stm;
      if (jx >= 0 && jx < cdo->nx && jy >= 0 && jy < cdo->ny &&
          jz >= 0 && jz < cdo->nz) {
        ptrdiff_t i;
        i = calc_address(jx, jy, jz, cdo->nx, cdo->ny, cdo->nz);
        mtl->div_u[j] = mtl->div_b[i];
      } else {
        mtl->div_u[j] = 0.0;
      }
    }
  }

  //-- poisson
#ifdef AMGS
  param[0] = bc_xm;
  param[1] = bc_xp;
  param[2] = bc_ym;
  param[3] = bc_yp;
  param[4] = bc_zm;
  param[5] = bc_zp;
  mtl_flg = 1;
  vel_flg = 0;
  amgs_solve_(&mtl_flg, &vel_flg, val->p, mtl->div_b , mtl->dens, mtl->mu, mtl->thc, mtl->specht, &nx, &ny, &nz, &n, &mx, &my, &mxy, &m, &stm,
              &dxi2, &dyi2, &dzi2, &dti, mpi->nrk, param, &mpi->npe_x, &mpi->npe_xy, &mpi->npe, &mpi->rank);
#elif PETSc
  petsc_poisson(0,val->p, mtl->div_b, mtl->dens, mtl, val, prm);
#elif CCSE
#ifdef _TIME_
  it_P += ccse_poisson(3,val->p, mtl->div_b, mtl->dens, mtl, val, prm);
#else
  ccse_poisson(3,val->p, mtl->div_b, mtl->dens, mtl, val, prm);
#endif
#endif

  bcp(val->p,val,prm);
  //-- velocity correction
  vel_correction(val, mtl, prm);
  //Y bcu(val->u,val->v,val->w,val,prm);
  //Y 20200410 bcu_correct(val->u,val->v,val->w,val,mtl,prm);

  bcu(val->u,val->v,val->w,val,mtl,prm); //by Sato

  div_max = divergence(mtl->div_b, val->u, val->v, val->w, ls, fs, div_ls_ibm,
                       div_fs_ibm, val->surface_bnd, val->bnd_norm_u,
                       val->bnd_norm_v, val->bnd_norm_w, cdo);

  return cpu_time() - time0;
}
