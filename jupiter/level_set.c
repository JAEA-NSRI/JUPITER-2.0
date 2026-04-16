#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef GPU
#  include <cuda.h>
#  include <cutil.h>
#include "level_set_cuda_wrapper.h"
#include "LevelSetCuda.h"
#endif

#include "struct.h"
#include "func.h"

#include "update_level_set_flags.h"

#ifdef _TIME_
extern type time_level_set_eq;
#endif

static inline int level_set_needs_solid_normals(const parameter *prm)
{
  return prm->flg->wettability == ON && prm->cdo->contact_angle != 90.0;
}

static inline int level_set_needs_ibm_level_set(const flags *flg)
{
  return flg->output_data.ls_ibm.outf == ON || flg->restart_data.ls_ibm.outf == ON;
}


//  flop : 16
//  sf   :  5
#define FD3mp(fx2, fw2,fw1,fc,fe1,fe2, sig,dxs)         \
{   type  fxp, fxm;                                     \
    fxp = -(fe2 - 6.*fe1 + 3.*fc + 2.*fw1)*dxs/6.;      \
    fxm =  (fw2 - 6.*fw1 + 3.*fc + 2.*fe1)*dxs/6.;      \
                                                        \
    if( sig > 0.0 ) {                                   \
        fx2  = MAX2( MAX2(fxm, 0.0)*MAX2(fxm, 0.0),     \
                     MIN2(fxp, 0.0)*MIN2(fxp, 0.0) );   \
    } else {                                            \
        fx2  = MAX2( MIN2(fxm, 0.0)*MIN2(fxm, 0.0),     \
                     MAX2(fxp, 0.0)*MAX2(fxp, 0.0) );   \
    }                                                   \
}

#define FD3mpc(fx2, fw2,fw1,fc,fe1,fe2, sig,dxs)        \
{   type  fxp, fxm;                                     \
    fxp = -(fe2 - 6.*fe1 + 3.*fc + 2.*fw1)*dxs/6.;      \
    fxm =  (fw2 - 6.*fw1 + 3.*fc + 2.*fe1)*dxs/6.;      \
                                                        \
    if( sig > 0.0 ) {                                   \
        fx2 = fxm;                                      \
    } else {                                            \
        fx2 = fxp;                                      \
    }                                                   \
}

#define FD1mp(fx2, fw2,fw1,fc,fe1,fe2, sig,dxs)         \
{   type  fxp, fxm;                                     \
    fxp = (fe1 - fc)*dxs;                               \
    fxm = (fc - fe1)*dxs;                               \
                                                        \
    if( sig > 0.0 ) {                                   \
        fx2  = MAX2( MAX2(fxm, 0.0)*MAX2(fxm, 0.0),     \
                     MIN2(fxp, 0.0)*MIN2(fxp, 0.0) );   \
    } else {                                            \
        fx2  = MAX2( MIN2(fxm, 0.0)*MIN2(fxm, 0.0),     \
                     MAX2(fxp, 0.0)*MAX2(fxp, 0.0) );   \
    }                                                   \
}

//#define LS_ADV_mp FD1mp
#define LS_ADV_mp FD3mp

void vof2ls(int init_flg, type *fs, type *ls, domain *cdo)
{
  int  j, jx, jy, jz, mx=cdo->mx, my=cdo->my, mz=cdo->mz;

  if(init_flg == 1) {

#pragma omp parallel for private(jz,jy,jx,j)
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
        for(jx = 0; jx < mx; jx++) {
          j = jx + mx*jy + mx*my*jz;
          ls[j] = 3.0*cdo->dx*(fs[j] - 0.5);
        }
      }
    }
  } else {

#pragma omp parallel for private(jz,jy,jx,j)
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
        for(jx = 0; jx < mx; jx++) {
          j = jx + mx*jy + mx*my*jz;
          ls[j] = cdo->gLz*(fs[j] - 0.5);
        }
      }
    }
  }
}

static inline type level_set_reinit_band(const domain *cdo)
{
  return 1.5*cdo->width + 2.0*cdo->dx;
}

static inline int level_set_inside_reinit_band(type f, type f0, const domain *cdo)
{
  type band = level_set_reinit_band(cdo);

  return fabs(f) <= band || fabs(f0) <= band;
}

void level_set_eq(type *fn, type a0, type *f0, type af, type aft, type *f, domain *cdo)
{
  int  j, jx, jy, jz, stm=cdo->stm,
    nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
    mx=cdo->mx, my=cdo->my, mxy=mx*my;
  type dx=cdo->dx, dy=cdo->dy, dz=cdo->dz,
    dxi=1.0/dx, dyi=1.0/dy, dzi=1.0/dz,
    fx2, fy2, fz2, ft, sign;
#ifdef _TIME_
  type time0=cpu_time();
#endif

#pragma omp parallel for private(jz,jy,jx,j,sign,fz2,fy2,fx2,ft)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        if (!level_set_inside_reinit_band(f[j], f0[j], cdo)) {
          fn[j] = a0*f0[j] + af*f[j];
          continue;
        }

        sign = f[j]/sqrt(f[j]*f[j] + dx*dy);
        LS_ADV_mp(fz2, f[j-2*mxy],f[j-mxy],f[j],f[j+mxy],f[j+2*mxy], sign, dzi);
        LS_ADV_mp(fy2, f[j-2*mx ],f[j-mx ],f[j],f[j+mx ],f[j+2*mx ], sign, dyi);
        LS_ADV_mp(fx2, f[j-2    ],f[j-1  ],f[j],f[j+1  ],f[j+2    ], sign, dxi);
        ft = sign*(1.0 - sqrt(fx2 + fy2 + fz2));
        fn[j] = a0*f0[j] + af*f[j] + aft*ft;
      }
    }
  }
#ifdef _TIME_
  time_level_set_eq += cpu_time()-time0;
#endif
}

void level_set_eq_c(type *fn, type a0, type *f0, type af, type aft, type *f,
     type *nwx, type *nwy, type *nwz, type *nsww_x, type *nsww_y, type *nsww_z,
     domain *cdo, type *nvlx, type *nvly, type *nvlz, type theta)
{
  int j, jx, jy, jz, stm=cdo->stm,
      nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
      mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;
  type dx=cdo->dx, dy=cdo->dy, dz=cdo->dz,
       dxi=1.0/dx, dyi=1.0/dy, dzi=1.0/dz,
       fx2, fy2, fz2, ft, det, dext_x, dext_y, dext_z, s;
  type nwnt_x, nwnt_y, nwnt_z;
#ifdef _TIME_
  type time0=cpu_time();
#endif

#pragma omp parallel for private(jz,jy,jx,j,s,nwnt_x,nwnt_y,nwnt_z,det,dext_x,dext_y,dext_z,ft,fx2,fy2,fz2)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        if (!level_set_inside_reinit_band(f[j], f0[j], cdo)) {
          fn[j] = a0*f0[j] + af*f[j];
          continue;
        }

        s = nvlx[j]*nsww_x[j] + nvly[j]*nsww_y[j] + nvlz[j]*nsww_z[j];

        if (s < 0.0) {
          nwnt_x = nwx[j] - nsww_x[j]/tan(M_PI-theta);
          nwnt_y = nwy[j] - nsww_y[j]/tan(M_PI-theta);
          nwnt_z = nwz[j] - nsww_z[j]/tan(M_PI-theta);
          det = sqrt(nwnt_x*nwnt_x + nwnt_y*nwnt_y + nwnt_z*nwnt_z) + 1.0e-10;
          dext_x = nwnt_x/det;
          dext_y = nwnt_y/det;
          dext_z = nwnt_z/det;
        } else if (s > 0.0) {
          nwnt_x = nwx[j] + nsww_x[j]/tan(M_PI-theta);
          nwnt_y = nwy[j] + nsww_y[j]/tan(M_PI-theta);
          nwnt_z = nwz[j] + nsww_z[j]/tan(M_PI-theta);
          det = sqrt(nwnt_x*nwnt_x + nwnt_y*nwnt_y + nwnt_z*nwnt_z) + 1.0e-10;
          dext_x = nwnt_x/det;
          dext_y = nwnt_y/det;
          dext_z = nwnt_z/det;
        } else {
          dext_x = nwx[j];
          dext_y = nwy[j];
          dext_z = nwz[j];
        }
/*
        fn[j] = f0[j]
              - 0.5*aft*( (dext_x*(f[j+1  ]-f[j-1  ]) - fabs(dext_x)*(f[j+1  ]-2.0*f[j]+f[j-1  ]))*dxi
              +           (dext_y*(f[j+mx ]-f[j-mx ]) - fabs(dext_y)*(f[j+mx ]-2.0*f[j]+f[j-mx ]))*dyi
              +           (dext_z*(f[j+mxy]-f[j-mxy]) - fabs(dext_z)*(f[j+mxy]-2.0*f[j]+f[j-mxy]))*dzi );
*/
        FD3mpc(fz2, f[j-2*mxy], f[j-mxy], f[j], f[j+mxy], f[j+2*mxy], dext_z, dzi);
        FD3mpc(fy2, f[j-2*mx ], f[j-mx ], f[j], f[j+mx ], f[j+2*mx ], dext_y, dyi);
        FD3mpc(fx2, f[j-2    ], f[j-1  ], f[j], f[j+1  ], f[j+2    ], dext_x, dxi);
        ft = - (dext_x*fx2 + dext_y*fy2 + dext_z*fz2);
        fn[j] = a0*f0[j] + af*f[j] + aft*ft;
      }
    }
  }
#ifdef _TIME_
  time_level_set_eq += cpu_time()-time0;
#endif
}
#undef LS_ADV_mp
#undef FD3mp
#undef FD3mpc
#undef FD1mp


type Level_Set(int init_flg, int itr_max, type *ls, type *fs, parameter *prm)
{

//cuda
// #if 1
#ifdef GPU
  Level_Set_cuda(init_flg, itr_max, ls, fs, prm);

#else
    domain *cdo = prm->cdo;
    // memory allocation for work array.
    static int  n1st = 0;
  static type *tmp1;
    size_t size = sizeof(type)*(cdo->m);
    if (n1st++ == 0) {
  #ifdef _MEM_ALIGN_
          tmp1 = (type *) _mm_malloc( size,32 );
  #else
          tmp1 = (type *) malloc( size );
  #endif
      }

    // VOF => init Level Set
    vof2ls(init_flg, fs, ls, cdo);
    bcf(ls, prm);

    // 2nd-order TVD Runge-Kutta
    int  itr = 0;
    type dt = cdo->coef_lsts*cdo->dx;
    while (itr++ < itr_max) {

      level_set_eq(tmp1,    1., ls,    0.,      dt, ls,   cdo);
      bcf(tmp1, prm);

      level_set_eq(ls,    0.5, ls,    0.5,  0.5*dt, tmp1, cdo);
      bcf(ls,   prm);

    }
#endif

    return 0.0;
}

//---- Reconstruction of the Level-set function with the contact angle correction
type Level_Set_contact(int init_flg, int itr_max, type *ls, type *nvlx, type *nvly, type *nvlz, type *nvsx, type *nvsy, type *nvsz, type *fs, parameter *prm)
{
  domain *cdo = prm->cdo;
  // memory allocation for work array.
  int  j, jx, jy, jz, stm=cdo->stm,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;
  static int  n1st = 0;
  static type *tmp1, *tmp2;
  static type *nwx, *nwy, *nwz;
  static type *nsww_x, *nsww_y, *nsww_z;
  size_t size = sizeof(type)*(cdo->m);
  type theta = cdo->contact_angle*M_PI/180.0, det;
  type band = level_set_reinit_band(cdo);

  if (n1st++ == 0) {
#ifdef _MEM_ALIGN_
    tmp1 = (type *) _mm_malloc( size,32 );
    tmp2 = (type *) _mm_malloc( size,32 );
    nwx  = (type *) _mm_malloc( size,32 );
    nwy  = (type *) _mm_malloc( size,32 );
    nwz  = (type *) _mm_malloc( size,32 );
    nsww_x  = (type *) _mm_malloc( size,32 );
    nsww_y  = (type *) _mm_malloc( size,32 );
    nsww_z  = (type *) _mm_malloc( size,32 );
#else
    tmp1 = (type *) malloc( size );
    tmp2 = (type *) malloc( size );
    nwx  = (type *) malloc( size );
    nwy  = (type *) malloc( size );
    nwz  = (type *) malloc( size );
    nsww_x  = (type *) malloc( size );
    nsww_y  = (type *) malloc( size );
    nsww_z  = (type *) malloc( size );
#endif
  }

#pragma omp parallel for private(jz,jy,jx,j)
  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*jy + mxy*jz;

        if (fabs(ls[j]) > band) {
          nwx[j] = 0.0;
          nwy[j] = 0.0;
          nwz[j] = 0.0;
          continue;
        }

        nwx[j] = fs[j]*nvsx[j];
        nwy[j] = fs[j]*nvsy[j];
        nwz[j] = fs[j]*nvsz[j];
      }
    }
  }
  bcn(nwx, nwy, nwz, prm);

#pragma omp parallel for private(jz,jy,jx,j,det)
  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*jy + mxy*jz;

        if (fabs(ls[j]) > band) {
          nsww_x[j] = 0.0;
          nsww_y[j] = 0.0;
          nsww_z[j] = 0.0;
          continue;
        }

        nsww_x[j] = (nvlz[j]*nwx[j] - nvlx[j]*nwz[j])*nwz[j] - (nvlx[j]*nwy[j] - nvly[j]*nwx[j])*nwy[j];
        nsww_y[j] = (nvlx[j]*nwy[j] - nvly[j]*nwx[j])*nwx[j] - (nvly[j]*nwz[j] - nvlz[j]*nwy[j])*nwz[j];
        nsww_z[j] = (nvly[j]*nwz[j] - nvlz[j]*nwy[j])*nwy[j] - (nvlz[j]*nwx[j] - nvlx[j]*nwz[j])*nwx[j];
        det = sqrt(nsww_x[j]*nsww_x[j] + nsww_y[j]*nsww_y[j] + nsww_z[j]*nsww_z[j]) + 1.0e-10;
        nsww_x[j] = nsww_x[j]/det;
        nsww_y[j] = nsww_y[j]/det;
        nsww_z[j] = nsww_z[j]/det;
      }
    }
  }

  // 3rd-order TVD Runge-Kutta
  int  itr = 0;
  //type dt = cdo->coef_lsts*cdo->dx;
  type dt = 0.5*cdo->dx;
  while (itr++ < itr_max) {
    level_set_eq_c(tmp1,    1., ls,    0.,       dt, ls,   nwx,nwy,nwz,nsww_x,nsww_y,nsww_z,cdo,nvlx, nvly, nvlz, theta); bcc(tmp1, prm);
    level_set_eq_c(tmp2, 3./4., ls, 1./4., 1./4.*dt, tmp1, nwx,nwy,nwz,nsww_x,nsww_y,nsww_z,cdo,nvlx, nvly, nvlz, theta); bcc(tmp2, prm);
    level_set_eq_c(ls,   1./3., ls, 2./3., 2./3.*dt, tmp2, nwx,nwy,nwz,nsww_x,nsww_y,nsww_z,cdo,nvlx, nvly, nvlz, theta); bcc(ls,  prm);
  }

  return 0.0;
}

void level_set_all(variable *val, parameter *prm)
{

  domain *cdo = prm->cdo;
  flags *flg = prm->flg;
  const int need_solid_normals = level_set_needs_solid_normals(prm);

  type *fl, *fs, *fs_ibm;

  if (flg->solute_diff == ON) {
    fs = val->fs;
    fl = val->fl;
  } else {
    fs = val->fs_sum;
    fl = val->fl_sum;      
  }
  fs_ibm = fs;

if(flg->multi_layer == OFF){
    // calc Level Set function
    if (update_level_set_flags_wants_update(&flg->update_level_set_ll))
      Level_Set(1, cdo->ls_iteration, val->ll,  fl,  prm);
    if (update_level_set_flags_wants_update(&flg->update_level_set_ls))
      Level_Set(1, cdo->ls_iteration, val->ls,  fs,  prm);
    if (update_level_set_flags_wants_update(&flg->update_level_set_lls))
      Level_Set(1, cdo->ls_iteration, val->lls, val->fls, prm);
    // calc normal vector & curvature
    if (need_solid_normals && update_level_set_flags_wants_update(&flg->update_level_set_ls))
      normal_vector_cell(val->nvsx, val->nvsy, val->nvsz, NULL,      val->ls, cdo);
    if (update_level_set_flags_wants_update(&flg->update_level_set_lls))
      normal_vector_cell(val->nvlx, val->nvly, val->nvlz, val->curv, val->lls, cdo);
    //normal_vector_cell(val->nvlx, val->nvly, val->nvlz, val->curv, val->ll, cdo);

    if (flg->wettability== ON) {
      if (cdo->contact_angle != 90.0) {
        if (update_level_set_flags_wants_update(&flg->update_level_set_lls)) {
          Level_Set_contact(1, cdo->CA_iteration, val->lls, val->nvlx, val->nvly, val->nvlz, val->nvsx, val->nvsy, val->nvsz, val->fs_sum, prm);
          normal_vector_cell(val->nvlx, val->nvly, val->nvlz, val->curv, val->lls, cdo);
        }
        if (update_level_set_flags_wants_update(&flg->update_level_set_ll)) {
          Level_Set_contact(1, cdo->CA_iteration, val->ll, val->nvlx, val->nvly, val->nvlz, val->nvsx, val->nvsy, val->nvsz, val->fs_sum, prm);
          //normal_vector_cell(val->nvlx, val->nvly, val->nvlz, val->curv, val->ll, cdo);
        }
      }
    }

  }else{

    /*-- Multi_Layer --*/
    // For the multi_layer model assume the liquid phase exists only for property ID=0
    // (i.e. only fl_0 is non-zero, fl_1 and fl_2 are zero). Therefore level-sets
    // should be expanded only for fl_layer entries under fl_0.
    
      // calc Level Set function for solid
      if (update_level_set_flags_wants_update(&flg->update_level_set_ls))
        Level_Set(1, cdo->ls_iteration, val->ls,  fs,  prm);
      // calc normal vector & curvature for solid
      if (need_solid_normals && update_level_set_flags_wants_update(&flg->update_level_set_ls))
        normal_vector_cell(val->nvsx, val->nvsy, val->nvsz, NULL,      val->ls, cdo);

    for(int ilayer = 0; ilayer < cdo->NumberOfLayer; ilayer++) {

      if(val->bubble_cnt[ilayer]==0){
        zero_clear(&val->ll_layer[ilayer*cdo->m], cdo->m);
        zero_clear(&val->lls_layer[ilayer*cdo->m], cdo->m);
        zero_clear(&val->nvlx_layer[ilayer*cdo->m], cdo->m);
        zero_clear(&val->nvly_layer[ilayer*cdo->m], cdo->m);
        zero_clear(&val->nvlz_layer[ilayer*cdo->m], cdo->m);
        zero_clear(&val->curv_layer[ilayer*cdo->m], cdo->m);
        continue;
      }

      // calc Level Set function for fluid
      if (update_level_set_flags_wants_update(&flg->update_level_set_ll))
        Level_Set(1, cdo->ls_iteration, &val->ll_layer[ilayer*cdo->m],  &val->fl_layer[ilayer*cdo->m],  prm);
      if (update_level_set_flags_wants_update(&flg->update_level_set_lls))
        Level_Set(1, cdo->ls_iteration, &val->lls_layer[ilayer*cdo->m], &val->fls_layer[ilayer*cdo->m], prm);

      // calc normal vector & curvature for fluid
      if (update_level_set_flags_wants_update(&flg->update_level_set_lls))
        normal_vector_cell(&val->nvlx_layer[ilayer*cdo->m], &val->nvly_layer[ilayer*cdo->m], &val->nvlz_layer[ilayer*cdo->m], &val->curv_layer[ilayer*cdo->m], &val->lls_layer[ilayer*cdo->m], cdo);

      if (flg->wettability== ON) {
        if (cdo->contact_angle != 90.0) {
          if (update_level_set_flags_wants_update(&flg->update_level_set_lls)) {
            Level_Set_contact(1, cdo->CA_iteration, &val->lls_layer[ilayer*cdo->m], &val->nvlx_layer[ilayer*cdo->m], &val->nvly_layer[ilayer*cdo->m], &val->nvlz_layer[ilayer*cdo->m], val->nvsx, val->nvsy, val->nvsz, val->fs_sum, prm);
            normal_vector_cell(&val->nvlx_layer[ilayer*cdo->m], &val->nvly_layer[ilayer*cdo->m], &val->nvlz_layer[ilayer*cdo->m], &val->curv_layer[ilayer*cdo->m], &val->lls_layer[ilayer*cdo->m], cdo);
          }
          if (update_level_set_flags_wants_update(&flg->update_level_set_ll)) {
            Level_Set_contact(1, cdo->CA_iteration, &val->ll_layer[ilayer*cdo->m], &val->nvlx_layer[ilayer*cdo->m], &val->nvly_layer[ilayer*cdo->m], &val->nvlz_layer[ilayer*cdo->m], val->nvsx, val->nvsy, val->nvsz, val->fs_sum, prm);
            //normal_vector_cell(val->nvlx, val->nvly, val->nvlz, val->curv, val->ll, cdo);
          }
        }
      }

    }

    if(prm->flg->film_drainage) search_liquid_film(val,prm);

  }

  if (level_set_needs_ibm_level_set(flg)
      && update_level_set_flags_wants_update(&flg->update_level_set_ls)
      && val->fs_ibm) {
  CSVASSERT(val->ls_ibm);

  Level_Set(1, cdo->ls_iteration, val->ls_ibm, val->fs_ibm, prm);
  if (val->nvibmx && val->nvibmy && val->nvibmz)
    normal_vector_cell(val->nvibmx, val->nvibmy, val->nvibmz, NULL, val->ls_ibm, cdo);
  }    

}