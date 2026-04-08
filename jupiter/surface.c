#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef GPU
#  include <cuda.h>
#  include <cutil.h>
#endif

#include "struct.h"
#include "func.h"

#ifdef _TIME_
extern type time_normal_vector_cell;
#endif

/* YSE: Added for temperature dependent property */
#include "tempdep_calc.h"

//==== memo ===
// nvx : cell-face x
// nvy : cell-face y
// nvz : cell-face z
// curv: cell-center
void normal_vector_cell_check(type *nvx, type *nvy, type *nvz, type *curv, type *ls, domain *cdo)
{
  int  j, jx, jy, jz,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  type dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi, det;

  //=== normal vector ===

#pragma omp parallel for private(jz,jy,jx,j,det)

  for(jz = -2; jz < nz+2; jz++) {
    for(jy = -2; jy < ny+2; jy++) {
      for(jx = -2; jx < nx+2; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        nvx[j] = 0.5*(ls[j+1] - ls[j-1])*dxi;
        nvy[j] = 0.5*(ls[j+mx] - ls[j-mx])*dyi;
        nvz[j] = 0.5*(ls[j+mxy] - ls[j-mxy])*dzi;
        det = sqrt(nvx[j]*nvx[j] + nvy[j]*nvy[j] + nvz[j]*nvz[j]) + 1.0e-08;
        nvx[j] /= det;
        nvy[j] /= det;
        nvz[j] /= det;
      }
    }
  }
  if(curv != NULL) {
    //=== curvature ===

#pragma omp parallel for private(jz,jy,jx,j)

    for(jz = -1; jz < nz+1; jz++) {
      for(jy = -1; jy < ny+1; jy++) {
        for(jx = -1; jx < nx+1; jx++) {
          j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

          curv[j] = 0.5*(nvx[j+1  ] - nvx[j-1])*dxi
            + 0.5*(nvy[j+mx ] - nvy[j-mx])*dyi
            + 0.5*(nvz[j+mxy] - nvz[j-mxy])*dzi;
        }
      }
    }
  }
}

void normal_vector(type *nvx, type *nvy, type *nvz, type *curv, type *ls, domain *cdo)
{
  int  j, jx, jy, jz,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  type dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi;

#pragma omp parallel private(jz,jy,jx,j)
  {

    //=== normal vector x ===
#pragma omp for

    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        for(jx = 0; jx < nx+1; jx++) {
          j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

          nvx[j] = (ls[j] - ls[j-1])*dxi;
        }
      }
    }
    //=== normal vector y ===
#pragma omp for

    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny+1; jy++) {
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

          nvy[j] = (ls[j] - ls[j-mx])*dyi;
        }
      }
    }
    //=== normal vector z ===
#pragma omp for

    for(jz = 0; jz < nz+1; jz++) {
      for(jy = 0; jy < ny; jy++) {
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

          nvz[j] = (ls[j] - ls[j-mxy])*dzi;
        }
      }
    }
    //=== curvature ===
#pragma omp for

    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

          curv[j] = (nvx[j+1  ] - nvx[j])*dxi
            + (nvy[j+mx ] - nvy[j])*dyi
            + (nvz[j+mxy] - nvz[j])*dzi;
        }
      }
    }

  }

}

//==== memo ===
// nvx : cell-center
// nvy : cell-center
// nvz : cell-center
// curv: cell-center
void normal_vector_cell(type *nvx, type *nvy, type *nvz, type *curv, type *f, domain *cdo)
{
  int  j, jx, jy, jz,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  type dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi;
  type nvx_tmp, nvy_tmp, nvz_tmp, det;
  type nvx_bne,nvx_bnw,nvx_bse,nvx_bsw, nvx_tne,nvx_tnw,nvx_tse,nvx_tsw,
       nvy_bne,nvy_bnw,nvy_bse,nvy_bsw, nvy_tne,nvy_tnw,nvy_tse,nvy_tsw,
       nvz_bne,nvz_bnw,nvz_bse,nvz_bsw, nvz_tne,nvz_tnw,nvz_tse,nvz_tsw;

#ifdef _TIME_
  type time0=cpu_time();
#endif

  //=== normal vector x,y,z ===


#pragma omp parallel for private(jz,jy,jx,j,nvx_bne,nvx_bnw,nvx_bse,nvx_bsw,nvx_tne,nvx_tnw,nvx_tse,nvx_tsw,nvy_bne,nvy_bnw,nvy_bse,nvy_bsw,nvy_tne,nvy_tnw,nvy_tse,nvy_tsw,nvz_bne,nvz_bnw,nvz_bse,nvz_bsw,nvz_tne,nvz_tnw,nvz_tse,nvz_tsw,nvx_tmp,nvy_tmp,nvz_tmp,det)

  for(jz = -2; jz < nz+2; jz++) {
    for(jy = -2; jy < ny+2; jy++) {
      for(jx = -2; jx < nx+2; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        nvx_bne = 0.25*(f[j+mx+1] - f[j+mx] + f[j+1] - f[j] + f[j+mx+1-mxy] - f[j+mx-mxy] + f[j+1-mxy] - f[j-mxy])*dxi;
        nvx_bnw = 0.25*(f[j+mx] - f[j+mx-1] + f[j] - f[j-1] + f[j+mx-mxy] - f[j+mx-1-mxy] + f[j-mxy] - f[j-1-mxy])*dxi;
        nvx_bse = 0.25*(f[j+1] - f[j] + f[j-mx+1] - f[j-mx] + f[j+1-mxy] - f[j-mxy] + f[j-mx+1-mxy] - f[j-mx-mxy])*dxi;
        nvx_bsw = 0.25*(f[j] - f[j-1] + f[j-mx] - f[j-mx-1] + f[j-mxy] - f[j-1-mxy] + f[j-mx-mxy] - f[j-mx-1-mxy])*dxi;

        nvx_tne = 0.25*(f[j+mx+1] - f[j+mx] + f[j+1] - f[j] + f[j+mx+1+mxy] - f[j+mx+mxy] + f[j+1+mxy] - f[j+mxy])*dxi;
        nvx_tnw = 0.25*(f[j+mx] - f[j+mx-1] + f[j] - f[j-1] + f[j+mx+mxy] - f[j+mx-1+mxy] + f[j+mxy] - f[j-1+mxy])*dxi;
        nvx_tse = 0.25*(f[j+1] - f[j] + f[j-mx+1] - f[j-mx] + f[j+1+mxy] - f[j+mxy] + f[j-mx+1+mxy] - f[j-mx+mxy])*dxi;
        nvx_tsw = 0.25*(f[j] - f[j-1] + f[j-mx] - f[j-mx-1] + f[j+mxy] - f[j-1+mxy] + f[j-mx+mxy] - f[j-mx-1+mxy])*dxi;

        nvy_bne = 0.25*(f[j+mx+1] - f[j+1] + f[j+mx] - f[j] + f[j+mx+1-mxy] - f[j+1-mxy] + f[j+mx-mxy] - f[j-mxy])*dyi;
        nvy_bnw = 0.25*(f[j+mx] - f[j] + f[j+mx-1] - f[j-1] + f[j+mx-mxy] - f[j-mxy] + f[j+mx-1-mxy] - f[j-1-mxy])*dyi;
        nvy_bse = 0.25*(f[j+1] - f[j+1-mx] + f[j] - f[j-mx] + f[j+1-mxy] - f[j+1-mx-mxy] + f[j-mxy] - f[j-mx-mxy])*dyi;
        nvy_bsw = 0.25*(f[j] - f[j-mx] + f[j-1] - f[j-1-mx] + f[j-mxy] - f[j-mx-mxy] + f[j-1-mxy] - f[j-1-mx-mxy])*dyi;

        nvy_tne = 0.25*(f[j+mx+1] - f[j+1] + f[j+mx] - f[j] + f[j+mx+1+mxy] - f[j+1+mxy] + f[j+mx+mxy] - f[j+mxy])*dyi;
        nvy_tnw = 0.25*(f[j+mx] - f[j] + f[j+mx-1] - f[j-1] + f[j+mx+mxy] - f[j+mxy] + f[j+mx-1+mxy] - f[j-1+mxy])*dyi;
        nvy_tse = 0.25*(f[j+1] - f[j+1-mx] + f[j] - f[j-mx] + f[j+1+mxy] - f[j+1-mx+mxy] + f[j+mxy] - f[j-mx+mxy])*dyi;
        nvy_tsw = 0.25*(f[j] - f[j-mx] + f[j-1] - f[j-1-mx] + f[j+mxy] - f[j-mx+mxy] + f[j-1+mxy] - f[j-1-mx+mxy])*dyi;

        nvz_bne = 0.25*(f[j] - f[j-mxy] + f[j+1] - f[j-mxy+1] + f[j+mx] - f[j+mx-mxy] + f[j+mx+1] - f[j+mx+1-mxy])*dzi;
        nvz_bnw = 0.25*(f[j] - f[j-mxy] + f[j-1] - f[j-mxy-1] + f[j+mx] - f[j+mx-mxy] + f[j+mx-1] - f[j+mx-1-mxy])*dzi;
        nvz_bse = 0.25*(f[j] - f[j-mxy] + f[j+1] - f[j-mxy+1] + f[j-mx] - f[j-mx-mxy] + f[j-mx+1] - f[j-mx+1-mxy])*dzi;
        nvz_bsw = 0.25*(f[j] - f[j-mxy] + f[j-1] - f[j-mxy-1] + f[j-mx] - f[j-mx-mxy] + f[j-mx-1] - f[j-mx-1-mxy])*dzi;

        nvz_tne = 0.25*(f[j+mxy] - f[j] + f[j+mxy+1] - f[j+1] + f[j+mxy+mx] - f[j+mx] + f[j+mxy+mx+1] - f[j+mx+1])*dzi;
        nvz_tnw = 0.25*(f[j+mxy] - f[j] + f[j+mxy-1] - f[j-1] + f[j+mxy+mx] - f[j+mx] + f[j+mxy+mx-1] - f[j+mx-1])*dzi;
        nvz_tse = 0.25*(f[j+mxy] - f[j] + f[j+mxy+1] - f[j+1] + f[j+mxy-mx] - f[j-mx] + f[j+mxy-mx+1] - f[j-mx+1])*dzi;
        nvz_tsw = 0.25*(f[j+mxy] - f[j] + f[j+mxy-1] - f[j-1] + f[j+mxy-mx] - f[j-mx] + f[j+mxy-mx-1] - f[j-mx-1])*dzi;

        nvx_tmp = 0.125*(nvx_bne + nvx_bnw + nvx_bse + nvx_bsw + nvx_tne + nvx_tnw + nvx_tse + nvx_tsw);
        nvy_tmp = 0.125*(nvy_bne + nvy_bnw + nvy_bse + nvy_bsw + nvy_tne + nvy_tnw + nvy_tse + nvy_tsw);
        nvz_tmp = 0.125*(nvz_bne + nvz_bnw + nvz_bse + nvz_bsw + nvz_tne + nvz_tnw + nvz_tse + nvz_tsw);

        det = sqrt(nvx_tmp*nvx_tmp + nvy_tmp*nvy_tmp + nvz_tmp*nvz_tmp) + 1.0e-08;

        nvx[j] = nvx_tmp/det;
        nvy[j] = nvy_tmp/det;
        nvz[j] = nvz_tmp/det;
        if(f[j] < - 1.5*cdo->width || 1.5*cdo->width < f[j]) {
          nvx[j] = 0.0;
          nvy[j] = 0.0;
          nvz[j] = 0.0;
        }
      }
    }
  }
  //=== curvature ===
  if(curv != NULL) {

#pragma omp parallel for private(jz,jy,jx,j)

    for(jz = -1; jz < nz+1; jz++) {
      for(jy = -1; jy < ny+1; jy++) {
        for(jx = -1; jx < nx+1; jx++) {
          j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

          curv[j] = 0.5*(nvx[j+1  ] - nvx[j-1  ])*dxi
            + 0.5*(nvy[j+mx ] - nvy[j-mx ])*dyi
            + 0.5*(nvz[j+mxy] - nvz[j-mxy])*dzi;

          if(f[j] < - cdo->width || cdo->width < f[j]) curv[j] = 0.0;
        }
      }
    }
  }
#ifdef _TIME_
  time_normal_vector_cell += cpu_time()-time0;
#endif
}

// delta function
#define DELTA( delta, dist, width )                             \
  {                                                             \
    if(      dist >   width ) { delta = 0.0; }                  \
    else if( dist < - width ) { delta = 0.0; }                  \
    else {                                                      \
      delta = ( 0.5/width )*( 1.0 + cos( dist*M_PI/width ) );   \
    }                                                           \
  }

// delta function density scaling
#define DELTA_SC( delta, dist, width )                             \
  {                                                             \
    if(      dist >   width ) { delta = 0.0; }                  \
    else if( dist < - width ) { delta = 0.0; }                  \
    else {                                                      \
      delta_sc = ( 0.5/width )*( 1.0 + cos( dist*M_PI/width ) );   \
    }                                                           \
  }
// Heaviside
#define HA( Has, dist, width )                                    \
  {                                                               \
    if(      dist >   width ) { Has = 1.0; }                      \
    else if( dist < - width ) { Has = 0.0; }                      \
    else {                                                        \
      Has = 0.5*( 1.0 + dist/width + sin(M_PI*dist/width)/M_PI ); \
    }                                                             \
  }


// delta function (scaling)
#define HA_SCALING( Has, dist, width )                          \
  {                                                             \
    if(      dist >   width ) { Has = 1.0; }                    \
    else if( dist < - width ) { Has = 0.0; }                    \
    else {                                                      \
      Has = 0.5*( 0.5 + dist/width + 0.5*dist*dist/width/width  \
          - 0.25*(cos(2.0*M_PI*dist/width) - 1.0)/M_PI/M_PI     \
          + (width+dist)*sin(M_PI*dist/width)/width/M_PI );     \
    }                                                           \
  }

//=======================================
//  surface tension force by CSF model
//---------------------------------------
int surface_tension_eq(type *tt, type *ut, type *vt, type *wt, type *st, type *dens, 
                       variable *val, parameter *prm)
{
  if(prm->flg->surface_tension != ON) return 1;
  flags       *flg = prm->flg;
  domain      *cdo = prm->cdo;
  phase_value *phv = prm->phv;
  int NBCompo_surface = cdo->NBaseComponent;    //< Number of Solid-Liquid components
  int  j, jx, jy, jz,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  type dens_c,curv_c,ll_c,ls_c,delta, width=0.5*cdo->width,st_c,dxi=cdo->dxi,dyi=cdo->dyi,dzi=cdo->dzi;
  type fl_c;
  type *fs;
  type bussine;
  type Has_m, Has_p;

  if (prm->flg->solute_diff == ON) {
    fs = val->fs;
  } else {
    fs = val->fs_sum;
  }
  type fs_c;

  bussine = 0.0;
  //if(prm->flg->debug == ON) {

  if(flg->multi_layer==OFF) cdo->NumberOfLayer = 1; // to make sure the following ilayer routine finish by one time if multi_layer==OFF

  int ilayer;
  for(ilayer = 0; ilayer < cdo->NumberOfLayer; ilayer++) {

    if(flg->multi_layer==ON){
      if(val->bubble_cnt[ilayer]==0){
        continue;
      }      
    }

    type *ll, *ls, *fl, *nvx, *nvy, *nvz, *curv;

    if(flg->multi_layer==OFF){
      ll=val->lls; //*ll=val->lls  or ll
      //ll=val->ll; //*ll=val->lls  or ll
      ls=val->ls;
      fl=val->fl_sum;
      nvx=val->nvlx, nvy=val->nvly, nvz=val->nvlz, curv=val->curv;
    } else {
      ll=&val->lls_layer[ilayer*cdo->m]; //*ll=val->lls  or ll
      //ll=val->ll; //*ll=val->lls  or ll
      ls=val->ls;
      fl=&val->fl_layer[ilayer*cdo->m];
      nvx=&val->nvlx_layer[ilayer*cdo->m], nvy=&val->nvly_layer[ilayer*cdo->m], nvz=&val->nvlz_layer[ilayer*cdo->m], curv=&val->curv_layer[ilayer*cdo->m];    
    }

#pragma omp parallel private(jz,jy,jx,j,st_c,dens_c,curv_c,ll_c,ls_c,fl_c,delta,Has_m,Has_p)
    {

      //=== u ===
#pragma omp for

      for(jz = 0; jz < nz; jz++) {
        for(jy = 0; jy < ny; jy++) {
          for(jx = 0; jx < nx+1; jx++) {
            j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

            st_c   = 0.5*(st[j] + st[j-1]);
            dens_c = 0.5*(dens[j] + dens[j-1]);
/*
            if(dens_c > tempdep_calc(&phv->comps[0].rho_l, val->t[j]))
              dens_c = tempdep_calc(&phv->comps[0].rho_l, val->t[j]);
            if(dens_c < tempdep_calc(&phv->rho_g, val->t[j]))
              dens_c = tempdep_calc(&phv->rho_g, val->t[j]);
              
*/

            curv_c = 0.5*(curv[j] + curv[j-1]);
            ll_c   = 0.5*(ll[j] + ll[j-1]);
            ls_c   = 0.5*(ls[j] + ls[j-1]);
            fl_c = 0.5*(fl[j] + fl[j-1]);
            fs_c = 0.5*(fs[j] + fs[j-1]);
            //DELTA(delta, ll_c, width);
            HA(Has_m, ll[j-1], width);
            HA(Has_p, ll[j], width);
            //HA_SCALING(Has_m, ll[j-1], width);
            //HA_SCALING(Has_p, ll[j], width);

            //ut[j] = ut[j] - st_c*delta*curv_c*0.5*(nvx[j]+nvx[j-1])/dens_c;
            //ut[j] = ut[j] - st_c*(Has_p-Has_m)*dxi*curv_c/dens_c;
            //
            /* The case of wettability = ON may be not available. Should be check later */
            /*
            if (fabs(ls_c) <= cdo->width)
              continue;
            ut[j] = ut[j] - st_c*(Has_p-Has_m)*dxi*curv_c/dens_c;
            */
            ut[j] = ut[j] - (1.0-fs_c)*st_c*(Has_p-Has_m)*dxi*curv_c/dens_c;
          }
        }
      }
      //=== v ===
#pragma omp for

      for(jz = 0; jz < nz; jz++) {
        for(jy = 0; jy < ny+1; jy++) {
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

            st_c   = 0.5*(st[j] + st[j-mx]);
            dens_c = 0.5*(dens[j] + dens[j-mx]);
/*
            if(dens_c > tempdep_calc(&phv->comps[0].rho_l, val->t[j]))
              dens_c = tempdep_calc(&phv->comps[0].rho_l, val->t[j]);
            if(dens_c < tempdep_calc(&phv->rho_g, val->t[j]))
              dens_c = tempdep_calc(&phv->rho_g, val->t[j]);
*/
            curv_c = 0.5*(curv[j] + curv[j-mx]);
            ll_c   = 0.5*(ll[j] + ll[j-mx]);
            ls_c   = 0.5*(ls[j] + ls[j-mx]);
            fl_c = 0.5*(fl[j] + fl[j-mx]);
            fs_c = 0.5*(fs[j] + fs[j-mx]);
            //DELTA(delta, ll_c, width);
            HA(Has_m, ll[j-mx], width);
            HA(Has_p, ll[j], width);
            //HA_SCALING(Has_m, ll[j-mx], width);
            //HA_SCALING(Has_p, ll[j], width);

            //vt[j] = vt[j] - st_c*delta*curv_c*0.5*(nvy[j]+nvy[j-mx])/dens_c;
            //vt[j] = vt[j] - st_c*(Has_p-Has_m)*dyi*curv_c/dens_c;
            /*
            if (fabs(ls_c) <= cdo->width)
              continue;
            vt[j] = vt[j] - st_c*(Has_p-Has_m)*dyi*curv_c/dens_c;
            */
            vt[j] = vt[j] - (1.0-fs_c)*st_c*(Has_p-Has_m)*dyi*curv_c/dens_c;
          }
        }
      }
      //=== w ===
#pragma omp for

      for(jz = 0; jz < nz+1; jz++) {
        for(jy = 0; jy < ny; jy++) {
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

            st_c   = 0.5*(st[j] + st[j-mxy]);
            dens_c = 0.5*(dens[j] + dens[j-mxy]);
/*
            if(dens_c > tempdep_calc(&phv->comps[0].rho_l, val->t[j]))
              dens_c = tempdep_calc(&phv->comps[0].rho_l, val->t[j]);
            if(dens_c < tempdep_calc(&phv->rho_g, val->t[j]))
              dens_c = tempdep_calc(&phv->rho_g, val->t[j]);
*/
            curv_c = 0.5*(curv[j] + curv[j-mxy]);
            ll_c   = 0.5*(ll[j] + ll[j-mxy]);
            ls_c   = 0.5*(ls[j] + ls[j-mxy]);
            fl_c = 0.5*(fl[j] + fl[j-mxy]);
            fs_c = 0.5*(fs[j] + fs[j-mxy]);
            //DELTA(delta, ll_c, width);
            HA(Has_m, ll[j-mxy], width);
            HA(Has_p, ll[j], width);
            //HA_SCALING(Has_m, ll[j-mxy], width);
            //HA_SCALING(Has_p, ll[j], width);

            //wt[j] = wt[j] - st_c*delta*curv_c*0.5*(nvz[j]+nvz[j-mxy])/dens_c;
            //wt[j] = wt[j] - st_c*(Has_p-Has_m)*dzi*curv_c/dens_c;
            /*
            if (fabs(ls_c) <= cdo->width)
              continue;
            wt[j] = wt[j] - st_c*(Has_p-Has_m)*dzi*curv_c/dens_c;
            */
            wt[j] = wt[j] - (1.0-fs_c)*st_c*(Has_p-Has_m)*dzi*curv_c/dens_c;
          }
        }
      }

    }   // omp parallel

/*
  }else{

    //=== v ===
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny+1; jy++) {
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

          st_c   = 0.5*(st[j] + st[j-mx]);
          //dens_c = 0.5*(phv->rho_l + phv->rho_l);
          dens_c = 0.5*(dens[j] + dens[j-mx]);
          curv_c = 0.5*(curv[j] + curv[j-mx]);
          ll_c   = 0.5*(ll[j] + ll[j-mx]);
          ls_c   = 0.5*(ls[j] + ls[j-mx]);
          fl_c = 0.5*(fl[j] + fl[j-mx]);
          DELTA(delta, ll_c, width);
          if(prm->flg->debug == OFF) {
            type beta, tm_liq;
            beta = tempdep_calc(&phv->comps[0].beta, val->t[j]);
            tm_liq = phv->comps[0].tm_liq;
            bussine = - prm->cdo->grav_y*beta*(tt[j] - tm_liq);
            //        printf("bussine %lf\n",bussine);
          }

          vt[j] = vt[j];  //- bussine;
        }
      }
    }

  }
  */
  }
  return 0;
}
