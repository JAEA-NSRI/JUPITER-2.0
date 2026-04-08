#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef GPU
  #include <cuda.h>
  #include <cutil.h>
#endif

#include "os/os.h"

#include "struct.h"
#include "func.h"
#include "boundary_util.h"

#ifdef _TIME_
extern type time_thinc_wlic;
extern type time_normal_vector;
#endif

#define PI 3.141592653589793

type clip(type f)
{
  if(f > 1.0) return 1.0;
  if(f < 0.0) return 0.0;
  return f;
}

type thinc_flux(type fm2, type fm, type fp, type fp2, type udt, type dx)
{
  type beta = 3.5, fclip_a, fclip_b, fclip_c,
       alpha, gamma, a1, a3, c1, c2, xtil, eps=1.0e-08;

  // VOF値のクリッピング
  if(udt <= 0.0) {
    gamma = 0.0;
    fclip_a = clip(fm);
    fclip_b = clip(fp);
    fclip_c = clip(fp2);
  } else {
    gamma = 1.0;
    fclip_a = clip(fm2);
    fclip_b = clip(fm);
    fclip_c = clip(fp);
  }

  if(fabs(1.0-fclip_b)<=eps || fabs(fclip_b-0.0)<=eps) return fclip_b*udt; // Fukuda 2023/10/7
  if(fabs(fclip_c-fclip_b)<=eps || fabs(fclip_b-fclip_a)<=eps) return fclip_b*udt; // Fukuda 2023/10/7
  else if(((fclip_a<=fclip_b)&&(fclip_b>=fclip_c))||((fclip_a>=fclip_b)&&(fclip_b<=fclip_c))) return fclip_b*udt;  // Fukuda 2023/10/7

  // 界面の法線ベクトル方向の判定
  if(fclip_a <= fclip_c) {alpha =  1.0;}
  else                   {alpha = -1.0;}

  a1 = exp( alpha*beta*(2.0*fclip_b - 1.0) );
  a3 = exp( beta );
  xtil = 0.5/beta*log( (fabs(a3*a3 - a1*a3) + eps)/(fabs(a1*a3 - 1.0) + eps) );

  c1 = cosh(beta*(gamma          - xtil));
  c2 = cosh(beta*(gamma - udt/dx - xtil));

  // フラックスを returnする
  return  0.5*( udt + alpha*dx/beta*log(c1/c2) );
}

type thinc_flux_original(type fm2, type fm, type fp, type fp2, type udt, type dx) // Fukuda 2023/10/7
{
  type beta = 3.5, fclip_a, fclip_b, fclip_c,
       alpha, gamma, delta, a1, a3, c1, c2, xtil, eps=1.0e-08;

  // VOF値のクリッピング
  if(udt <= 0.0) {
    gamma = 0.0;
    fclip_a = clip(fm);
    fclip_b = clip(fp);
    fclip_c = clip(fp2);
  } else {
    gamma = 1.0;
    fclip_a = clip(fm2);
    fclip_b = clip(fm);
    fclip_c = clip(fp);
  }

  if(fabs(1.0-fclip_b)<=eps || fabs(fclip_b-0.0)<=eps) return fclip_b*udt; 
  if(fabs(fclip_c-fclip_b)<=eps || fabs(fclip_b-fclip_a)<=eps) return fclip_b*udt; 
  else if(((fclip_a<=fclip_b)&&(fclip_b>=fclip_c))||((fclip_a>=fclip_b)&&(fclip_b<=fclip_c))) return fclip_b*udt; 

  // 界面の法線ベクトル方向の判定
  if(fclip_a <= fclip_c) {alpha =  1.0; delta=fclip_c;}
  else                   {alpha = -1.0; delta=fclip_a;}

  a1 = exp((beta/(alpha*delta))*(2.0*fclip_b - delta));
  a3 = exp( beta );
  xtil = 0.5/beta*log( (fabs(a3*a3 - a1*a3) + eps)/(fabs(a1*a3 - 1.0) + eps) );

  c1 = cosh(beta*(gamma          - xtil));
  c2 = cosh(beta*(gamma - udt/dx - xtil));

  // フラックスを returnする
  return  0.5*delta*( udt + alpha*dx/beta*log(c1/c2) );
}

void normal_vector_ics(type *nvx, type *nvy, type *nvz, type *f, domain *cdo, flags *flg)
{
    int  j, jx, jy, jz,
         nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
         mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
    type dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi, det;
    type nvx_tmp, nvx_tne, nvx_tnw, nvx_tse, nvx_tsw, nvx_bne, nvx_bnw, nvx_bse, nvx_bsw,
         nvy_tmp, nvy_tne, nvy_tnw, nvy_tse, nvy_tsw, nvy_bne, nvy_bnw, nvy_bse, nvy_bsw,
         nvz_tmp, nvz_tne, nvz_tnw, nvz_tse, nvz_tsw, nvz_bne, nvz_bnw, nvz_bse, nvz_bsw;
#ifdef _TIME_
    type time0=cpu_time();
#endif

    //=== normal vector x ===
#pragma omp parallel for private(jz,jy,jx,j,nvx_tne,nvx_tnw,nvx_tse,nvx_tsw,nvx_bne,nvx_bnw,nvx_bse,nvx_bsw,nvy_tne,nvy_tnw,nvy_tse,nvy_tsw,nvy_bne,nvy_bnw,nvy_bse,nvy_bsw,nvz_tne,nvz_tnw,nvz_tse,nvz_tsw,nvz_bne,nvz_bnw,nvz_bse,nvz_bsw,nvx_tmp,nvy_tmp,nvz_tmp,det)

    for(jz = -1; jz < nz+1; jz++) {
        for(jy = -1; jy < ny+1; jy++) {
            for(jx = -1; jx < nx+1; jx++) {
              j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

              nvx_tne = 0.25*(f[j+mx+1] - f[j+mx] + f[j+1] - f[j] + f[j+mx+1+mxy] - f[j+mx+mxy] + f[j+1+mxy] - f[j+mxy])*dxi;
              nvx_tnw = 0.25*(f[j+mx] - f[j+mx-1] + f[j] - f[j-1] + f[j+mx+mxy] - f[j+mx-1+mxy] + f[j+mxy] - f[j-1+mxy])*dxi;
              nvx_tse = 0.25*(f[j+1] - f[j] + f[j-mx+1] - f[j-mx] + f[j+1+mxy] - f[j+mxy] + f[j-mx+1+mxy] - f[j-mx+mxy])*dxi;
              nvx_tsw = 0.25*(f[j] - f[j-1] + f[j-mx] - f[j-mx-1] + f[j+mxy] - f[j-1+mxy] + f[j-mx+mxy] - f[j-mx-1+mxy])*dxi;

              nvx_bne = 0.25*(f[j+mx+1] - f[j+mx] + f[j+1] - f[j] + f[j+mx+1-mxy] - f[j+mx-mxy] + f[j+1-mxy] - f[j-mxy])*dxi;
              nvx_bnw = 0.25*(f[j+mx] - f[j+mx-1] + f[j] - f[j-1] + f[j+mx-mxy] - f[j+mx-1-mxy] + f[j-mxy] - f[j-1-mxy])*dxi;
              nvx_bse = 0.25*(f[j+1] - f[j] + f[j-mx+1] - f[j-mx] + f[j+1-mxy] - f[j-mxy] + f[j-mx+1-mxy] - f[j-mx-mxy])*dxi;
              nvx_bsw = 0.25*(f[j] - f[j-1] + f[j-mx] - f[j-mx-1] + f[j-mxy] - f[j-1-mxy] + f[j-mx-mxy] - f[j-mx-1-mxy])*dxi;

              nvy_tne = 0.25*(f[j+mx+1] - f[j+1] + f[j+mx] - f[j] + f[j+mx+1+mxy] - f[j+1+mxy] + f[j+mx+mxy] - f[j+mxy])*dyi;
              nvy_tnw = 0.25*(f[j+mx] - f[j] + f[j+mx-1] - f[j-1] + f[j+mx+mxy] - f[j+mxy] + f[j+mx-1+mxy] - f[j-1+mxy])*dyi;
              nvy_tse = 0.25*(f[j+1] - f[j+1-mx] + f[j] - f[j-mx] + f[j+1+mxy] - f[j+1-mx+mxy] + f[j+mxy] - f[j-mx+mxy])*dyi;
              nvy_tsw = 0.25*(f[j] - f[j-mx] + f[j-1] - f[j-1-mx] + f[j+mxy] - f[j-mx+mxy] + f[j-1+mxy] - f[j-1-mx+mxy])*dyi;

              nvy_bne = 0.25*(f[j+mx+1] - f[j+1] + f[j+mx] - f[j] + f[j+mx+1-mxy] - f[j+1-mxy] + f[j+mx-mxy] - f[j-mxy])*dyi;
              nvy_bnw = 0.25*(f[j+mx] - f[j] + f[j+mx-1] - f[j-1] + f[j+mx-mxy] - f[j-mxy] + f[j+mx-1-mxy] - f[j-1-mxy])*dyi;
              nvy_bse = 0.25*(f[j+1] - f[j+1-mx] + f[j] - f[j-mx] + f[j+1-mxy] - f[j+1-mx-mxy] + f[j-mxy] - f[j-mx-mxy])*dyi;
              nvy_bsw = 0.25*(f[j] - f[j-mx] + f[j-1] - f[j-1-mx] + f[j-mxy] - f[j-mx-mxy] + f[j-1-mxy] - f[j-1-mx-mxy])*dyi;

              nvz_tne = 0.25*(f[j+mxy] - f[j] + f[j+mxy+mx] - f[j+mx] + f[j+mxy+1] - f[j+1] + f[j+mxy+mx+1] - f[j+mx+1])*dzi;
              nvz_tnw = 0.25*(f[j+mxy] - f[j] + f[j+mxy+mx] - f[j+mx] + f[j+mxy-1] - f[j-1] + f[j+mxy+mx-1] - f[j+mx-1])*dzi;
              nvz_tse = 0.25*(f[j+mxy] - f[j] + f[j+mxy-mx] - f[j-mx] + f[j+mxy+1] - f[j+1] + f[j+mxy-mx+1] - f[j-mx+1])*dzi;
              nvz_tsw = 0.25*(f[j+mxy] - f[j] + f[j+mxy-mx] - f[j-mx] + f[j+mxy-1] - f[j-1] + f[j+mxy-mx-1] - f[j-mx-1])*dzi;

              nvz_bne = 0.25*(f[j] - f[j-mxy] + f[j+mx] - f[j-mxy+mx] + f[j+1] - f[j-mxy+1] + f[j+mx+1] - f[j-mxy+mx+1])*dzi;
              nvz_bnw = 0.25*(f[j] - f[j-mxy] + f[j+mx] - f[j-mxy+mx] + f[j-1] - f[j-mxy-1] + f[j+mx-1] - f[j-mxy+mx-1])*dzi;
              nvz_bse = 0.25*(f[j] - f[j-mxy] + f[j-mx] - f[j-mxy-mx] + f[j+1] - f[j-mxy+1] + f[j-mx+1] - f[j-mxy-mx+1])*dzi;
              nvz_bsw = 0.25*(f[j] - f[j-mxy] + f[j-mx] - f[j-mxy-mx] + f[j-1] - f[j-mxy-1] + f[j-mx-1] - f[j-mxy-mx-1])*dzi;

              nvx_tmp = 0.125*(nvx_bne + nvx_bnw + nvx_bse + nvx_bsw + nvx_tne + nvx_tnw + nvx_tse + nvx_tsw);
              nvy_tmp = 0.125*(nvy_bne + nvy_bnw + nvy_bse + nvy_bsw + nvy_tne + nvy_tnw + nvy_tse + nvy_tsw);
              nvz_tmp = 0.125*(nvz_bne + nvz_bnw + nvz_bse + nvz_bsw + nvz_tne + nvz_tnw + nvz_tse + nvz_tsw);

              if(flg->interface_capturing_scheme == PLIC){             
                det = sqrt(nvx_tmp*nvx_tmp + nvy_tmp*nvy_tmp + nvz_tmp*nvz_tmp +1.0e-16);
                nvx[j]=-nvx_tmp/det;
                nvy[j]=-nvy_tmp/det;
                nvz[j]=-nvz_tmp/det;
              }else if(flg->interface_capturing_scheme == THINC_WLIC){
                det = fabs(nvx_tmp) + fabs(nvy_tmp) + fabs(nvz_tmp) + 1.0e-08;
                nvx[j] = fabs(nvx_tmp)/det;
                nvy[j] = fabs(nvy_tmp)/det;
                nvz[j] = fabs(nvz_tmp)/det;      
              }else if(flg->interface_capturing_scheme == THINC_AWLIC){
                det = nvx_tmp*nvx_tmp + nvy_tmp*nvy_tmp + nvz_tmp*nvz_tmp +1.0e-16;
                nvx[j]=sqrt(nvx_tmp*nvx_tmp/det);
                nvy[j]=sqrt(nvy_tmp*nvy_tmp/det);
                nvz[j]=sqrt(nvz_tmp*nvz_tmp/det);

                type power = 3; // 3 is determined to obtain the balance between advection accuracy and the interface sharpness

                nvx[j]=1-pow(acos(nvx[j])/(PI/2),power);
                nvy[j]=1-pow(acos(nvy[j])/(PI/2),power);
                nvz[j]=1-pow(acos(nvz[j])/(PI/2),power);   
              }

            }
        }
    }
#ifdef _TIME_
    time_normal_vector += cpu_time()-time0;
#endif
}

type calc_surface_boundary_vel(type u, struct surface_boundary_data *sb,
                                      type bnd_norm)
{
  switch(sb->cond) {
  case INLET:
    switch(sb->inlet_dir) {
    case SURFACE_INLET_DIR_NORMAL:
      return sb->normal_inlet_vel.current_value * bnd_norm;
    case SURFACE_INLET_DIR_INVALID:
      return u;
    }
    return u;
  default:
    return u;
  }
}

static void
calc_surface_boundary_thinc_inlet(type *fm2, type *fm1, type *fp1, type *fp2,
                                  int icompo, struct surface_boundary_data *sb,
                                  type bnd_norm)

{
  int ncompo;
  type fa;
  type f;

  f = 0.0;
  fa = 0.0;
  ncompo = inlet_component_data_ncomp(sb->comps);
  for (int ic = 0; ic < ncompo; ++ic) {
    type fi;
    int id;
    struct inlet_component_element *e;
    e = inlet_component_data_get(sb->comps, ic);
    fi = e->ratio.current_value;
    id = e->comp.d->comp_index;
    fa += fi;
    if (icompo == id || (icompo < -1 && id != -1)) {
      f += fi;
    }
  }
  if (fa > 0.0) {
    int to_m, to_p;
    to_m = 0;
    to_p = 0;
    if (bnd_norm > 0.0) {
      to_m = 1;
    } else if (bnd_norm < 0.0) {
      to_p = 1;
    }
    if ((to_m || to_p) && sb->normal_inlet_vel.current_value < 0.0) {
      to_m = !to_m;
      to_p = !to_p;
    }

    f = f / fa;
    if (to_m) {
      if (fm2)
        *fm2 = f;
      if (fm1)
        *fm1 = f;
    } else if (to_p) {
      if (fp2)
        *fp2 = f;
      if (fp1)
        *fp1 = f;
    }
  }
}

static void
calc_surface_boundary_thinc_f(type *fm2, type *fm1, type *fp1, type *fp2,
                              int icompo, struct surface_boundary_data *sb,
                              type bnd_norm)
{
  switch (sb->cond) {
  case INLET:
    switch(sb->inlet_dir) {
    case SURFACE_INLET_DIR_NORMAL:
      calc_surface_boundary_thinc_inlet(fm2, fm1, fp1, fp2, icompo, sb, bnd_norm);
      break;
    case SURFACE_INLET_DIR_INVALID:
      break;
    }
    break;
  }
}

static void calc_surface_boundary_wlic_f(type *fm, type *fp, int icompo,
                                          struct surface_boundary_data *sb,
                                          type bnd_norm)
{
  calc_surface_boundary_thinc_f(NULL, fm, fp, NULL, icompo, sb, bnd_norm);
}

int thinc(type *f, type *flx, type *fly, type *flz, type *nvx, type *nvy, type *nvz, type *u, type *v, type *w, type *lss, type *fs, type *fs_ibm, int icompo, type *bnd_norm_u, type *bnd_norm_v, type *bnd_norm_w, struct surface_boundary_data **surf_bnd_array, parameter *prm, int direction_flag) 
{
  flags  *flg=prm->flg;
  domain *cdo=prm->cdo;
  mpi_param *mpi=prm->mpi;
  int  j, jx, jy, jz,
        nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
        mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  type dt=cdo->dt, dx=cdo->dx, dy=cdo->dy, dz=cdo->dz,
        dti=1.0/dt, dxi=1.0/dx, dyi=1.0/dy, dzi=1.0/dz, ft;
#ifdef _TIME_
    type time0=cpu_time();
#endif

// flux on x,y and z direction
#pragma omp parallel for private(jz,jy,jx,j)

  for(jz = 0; jz < nz+1; jz++) {
    for(jy = 0; jy < ny+1; jy++) {
      for(jx = 0; jx < nx+1; jx++) {

        type um, vm, wm;
        type fw2, fw1, fe1, fe2;
        type fs2, fs1, fn1, fn2;
        type fb2, fb1, ft1, ft2;

        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        if(direction_flag==0){ // ------------------------- x direction -------------------------
            um = u[j];
            fw2 = f[j - 2];
            fw1 = f[j - 1];
            fe1 = f[j];
            fe2 = f[j + 1];

            if (surf_bnd_array) {
              struct surface_boundary_data *spu;
              type snx;
              spu = surf_bnd_array[3 * j];
              snx = bnd_norm_u[3 * j];
              if (spu) {
                um = calc_surface_boundary_vel(um, spu, snx);
                calc_surface_boundary_thinc_f(&fw2, &fw1, &fe1, &fe2, icompo, spu, snx);
              }
            }
#ifdef PW
            if (lss[j] + lss[j - 1] >= 0.0) um = 0.0;
#endif         

            if(prm->flg->interface_capturing_scheme==THINC_WLIC){
              flx[j] = thinc_flux(fw2, fw1, fe1, fe2, um * dt, dx);
            }else{
              flx[j] = thinc_flux_original(fw2, fw1, fe1, fe2, um * dt, dx);
            }
            
            
        }else if(direction_flag==1){ // ------------------------- y direction -------------------------
            vm = v[j];
            fs2 = f[j - 2 * mx];
            fs1 = f[j - 1 * mx];
            fn1 = f[j];
            fn2 = f[j + 1 * mx];

            if (surf_bnd_array) {
              struct surface_boundary_data *spv;
              type sny;
              spv = surf_bnd_array[3 * j + 1];
              sny = bnd_norm_v[3 * j + 1];
              if (spv) {
                vm = calc_surface_boundary_vel(vm, spv, sny);
                calc_surface_boundary_thinc_f(&fs2, &fs1, &fn1, &fn2, icompo, spv, sny);
              }
            }

#ifdef PW
            if (lss[j] + lss[j-mx ] >= 0.0) vm = 0.0;
#endif
            if(prm->flg->interface_capturing_scheme==THINC_WLIC){
              fly[j] = thinc_flux(fs2, fs1, fn1, fn2, vm * dt, dy);
            }else{
              fly[j] = thinc_flux_original(fs2, fs1, fn1, fn2, vm * dt, dy);
            }
            

        }else if(direction_flag==2){// ------------------------- z direction -------------------------
            wm = w[j];
            fb2 = f[j - 2 * mxy];
            fb1 = f[j - 1 * mxy];
            ft1 = f[j];
            ft2 = f[j + 1 * mxy];

            if (surf_bnd_array) {
              struct surface_boundary_data *spw;
              type snz;
              spw = surf_bnd_array[3 * j + 2];
              snz = bnd_norm_w[3 * j + 2];
              if (spw) {
                wm = calc_surface_boundary_vel(wm, spw, snz);
                calc_surface_boundary_thinc_f(&fb2, &fb1, &ft1, &ft2, icompo, spw, snz);
              }
            }
#ifdef PW
            if (lss[j] + lss[j-mxy] >= 0.0) wm = 0.0;
#endif

            if(prm->flg->interface_capturing_scheme==THINC_WLIC){
              flz[j] = thinc_flux(fb2, fb1, ft1, ft2, wm * dt, dz);
            }else{
              flz[j] = thinc_flux_original(fb2, fb1, ft1, ft2, wm * dt, dz);
            }
            
        }

      }
    }
  }

  // WLIC method
  if(prm->flg->interface_capturing_scheme==THINC_WLIC || prm->flg->interface_capturing_scheme==THINC_AWLIC) {
#pragma omp parallel for private(jz,jy,jx,j)
    for(jz = 0; jz < nz+1; jz++) {
      for(jy = 0; jy < ny+1; jy++) {
        for(jx = 0; jx < nx+1; jx++) {
          type um, vm, wm;
          type fw, fe;
          type fs, fn;
          type fb, ft;
          type nvxw, nvxe;
          type nvys, nvyn;
          type nvzb, nvzt;
          type flxp, flyp, flzp;

          j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

          if(direction_flag==0){ // ------------------------- x direction -------------------------
            um = u[j];
            fw = f[j - 1];
            fe = f[j];
            nvxw = nvx[j - 1];
            nvxe = nvx[j];
            flxp = flx[j];


            if (surf_bnd_array) {
              struct surface_boundary_data *spu;
              type snx;

              spu = surf_bnd_array[3 * j];
              snx = bnd_norm_u[3 * j];

              if (spu) {
                um = calc_surface_boundary_vel(um, spu, snx);
                calc_surface_boundary_wlic_f(&fw, &fe, icompo, spu, snx);
              }
            }

#ifdef PW
            if (lss[j] + lss[j-1  ] >= 0.0) um = 0.0;
#endif

            if(um > 0.0) flx[j] = nvxw * flxp + (1.0 - nvxw) * fw * um * dt;
            else         flx[j] = nvxe * flxp + (1.0 - nvxe) * fe * um * dt;

          }else if(direction_flag==1){ // ------------------------- y direction -------------------------
            vm = v[j];
            fs = f[j - mx];
            fn = f[j];
            nvys = nvy[j - mx];
            nvyn = nvy[j];
            flyp = fly[j];

            if (surf_bnd_array) {
              struct surface_boundary_data *spv;
              type sny;

              spv = surf_bnd_array[3 * j + 1];
              sny = bnd_norm_v[3 * j + 1];

              if (spv) {
                vm = calc_surface_boundary_vel(vm, spv, sny);
                calc_surface_boundary_wlic_f(&fs, &fn, icompo, spv, sny);
              }
            }
#ifdef PW
            if (lss[j] + lss[j-mx ] >= 0.0) vm = 0.0;
#endif

            if(vm > 0.0) fly[j] = nvys * flyp + (1.0 - nvys) * fs * vm * dt;
            else         fly[j] = nvyn * flyp + (1.0 - nvyn) * fn * vm * dt;

          }else if(direction_flag==2){ // ------------------------- z direction -------------------------
            wm = w[j];
            fb = f[j - mxy];
            ft = f[j];
            nvzb = nvz[j - mxy];
            nvzt = nvz[j];
            flzp = flz[j];

            if (surf_bnd_array) {
              struct surface_boundary_data *spw;
              type snz;

              spw = surf_bnd_array[3 * j + 2];
              snz = bnd_norm_w[3 * j + 2];

              if (spw) {
                wm = calc_surface_boundary_vel(wm, spw, snz);
                calc_surface_boundary_wlic_f(&fb, &ft, icompo, spw, snz);
              }
            }
#ifdef PW
            if (lss[j] + lss[j-mxy] >= 0.0) wm = 0.0;
#endif

            if(wm > 0.0) flz[j] = nvzb * flzp + (1.0 - nvzb) * fb * wm * dt;
            else         flz[j] = nvzt * flzp + (1.0 - nvzt) * ft * wm * dt;
          
          }
        }
      }
    }
  }

  // time integration
  type flw, fle, fls, fln, flb, flt;
  type up, um, vp, vm, wp, wm;

#pragma omp parallel for private(jz,jy,jx,j,flw,fle,fls,fln,flb,flt,up,um,vp,vm,wp,wm,ft)

  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        if(direction_flag==0){ // ------------------------- x direction -------------------------

            if(fs[j] > 0.5){
              up = 0.0; um = 0.0;
            }else{
              up = u[j+1];   um = u[j];
            }

            if (surf_bnd_array) {
              struct surface_boundary_data *spum, *spup;
              type snxm, snxp;

              spum = surf_bnd_array[3 * j];
              snxm = bnd_norm_u[3 * j];
              if (spum) {
                um = calc_surface_boundary_vel(um, spum, snxm);
              }

              spup = surf_bnd_array[3 * (j+1)];
              snxp = bnd_norm_u[3 * (j+1)];
              if (spup) {
                up = calc_surface_boundary_vel(up, spup, snxp);
              }
            }

            flw=flx[j]   - f[j]*um*dt;
            fle=flx[j+1] - f[j]*up*dt; 

            if(flg->IBM == ON) {
              //IBM_flux_corr_vof_direction_split(&flw,&fle,j,fs_ibm,cdo,direction_flag);
            }
            //outflow_flux2(&flw,&fle,&fls,&fln,&flb,&flt, jx,jy,jz, nx,ny,nz, mpi,flg);
            outflow_flux_direction_split(&flw,&fle,jx,nx,mpi,direction_flag);

            //ft = - ( (fle - flw)*dxi )*dti + f[j]*(up-um)*dxi;
            ft = - ( (fle - flw)*dxi )*dti;
            
       }else if(direction_flag==1){ // ------------------------- y direction -------------------------

            if(fs[j] > 0.5){
              vp = 0.0; vm = 0.0;
            }else{
              vp = v[j+mx];  vm = v[j];
            }

            if (surf_bnd_array) {
              struct surface_boundary_data *spvm, *spvp;
              type snym, snyp;

              spvm = surf_bnd_array[3 * j + 1];
              snym = bnd_norm_v[3 * j + 1];
              if (spvm) {
                vm = calc_surface_boundary_vel(vm, spvm, snym);
              }

              spvp = surf_bnd_array[3 * (j+mx) + 1];
              snyp = bnd_norm_v[3 * (j+mx) + 1];
              if (spvp) {
                vp = calc_surface_boundary_vel(vp, spvp, snyp);
              }
            }

            fls=fly[j]    - f[j]*vm*dt;
            fln=fly[j+mx] - f[j]*vp*dt; 

            if(flg->IBM == ON) {
              //IBM_flux_corr_vof_direction_split(&fls,&fln, j,fs_ibm,cdo,direction_flag);
            }
            //outflow_flux2(&flw,&fle,&fls,&fln,&flb,&flt, jx,jy,jz, nx,ny,nz, mpi,flg);
            outflow_flux_direction_split(&fls,&fln,jy,ny, mpi,direction_flag);

            //ft = - ( (fln - fls)*dyi )*dti + f[j]*(vp-vm)*dyi;
            ft = - ( (fln - fls)*dyi )*dti;

        }else if(direction_flag==2){ // ------------------------- z direction -------------------------

            if(fs[j] > 0.5){
              wp = 0.0; wm = 0.0;
            }else{
              wp = w[j+mxy]; wm = w[j];
            }

            if (surf_bnd_array) {
              struct surface_boundary_data *spwm, *spwp;
              type snzm, snzp;

              spwm = surf_bnd_array[3 * j + 2];
              snzm = bnd_norm_w[3 * j + 2];
              if (spwm) {
                wm = calc_surface_boundary_vel(wm, spwm, snzm);
              }

              spwp = surf_bnd_array[3 * (j+mxy) + 2];
              snzp = bnd_norm_w[3 * (j+mxy) + 2];
              if (spwp) {
                wp = calc_surface_boundary_vel(wp, spwp, snzp);
              }
            }

            flb=flz[j]     - f[j]*wm*dt;
            flt=flz[j+mxy] - f[j]*wp*dt; 

            if(flg->IBM == ON) {
              //IBM_flux_corr_vof_direction_split(&flb,&flt, j,fs_ibm,cdo,direction_flag);
            }
            //outflow_flux2(&flw,&fle,&fls,&fln,&flb,&flt, jx,jy,jz, nx,ny,nz, mpi,flg);
            outflow_flux_direction_split(&flb,&flt,jz,nz, mpi,direction_flag);

            //ft = - ( (flt - flb)*dzi )*dti + f[j]*(wp-wm)*dzi;
            ft = - ( (flt - flb)*dzi )*dti;
        }
        
        f[j] = f[j] + ft*dt;

      }
    }
  }
  return 0;
}

static void copy_vof_field(type *dst, const type *src, domain *cdo)
{
  int j;

#pragma omp parallel for private(j)
  for (j = 0; j < cdo->m; ++j) {
    dst[j] = src[j];
  }
}

static void combine_vof_field(type *dst, type a0, const type *f0,
                              type a1, const type *f1, domain *cdo)
{
  int j;

#pragma omp parallel for private(j)
  for (j = 0; j < cdo->m; ++j) {
    dst[j] = a0 * f0[j] + a1 * f1[j];
  }
}

static void advance_split_plic_step(type *f, type *flx, type *fly, type *flz,
                                    type *nvx, type *nvy, type *nvz,
                                    type *alpha, type *u, type *v, type *w,
                                    type *ls, type *fs, type *fs_ibm,
                                    int icompo, int ilayer, variable *val,
                                    parameter *prm, const int order[3])
{
  domain *cdo = prm->cdo;
  flags *flg = prm->flg;

  normal_vector_ics(nvx, nvy, nvz, f, cdo, flg);
  plic(f, flx, fly, flz, nvx, nvy, nvz, alpha, u, v, w, ls, fs, fs_ibm,
       icompo, ilayer, val->bnd_norm_u, val->bnd_norm_v, val->bnd_norm_w,
       val->surface_bnd, prm, val, order[0]);
  bcf(f, prm);

  normal_vector_ics(nvx, nvy, nvz, f, cdo, flg);
  plic(f, flx, fly, flz, nvx, nvy, nvz, alpha, u, v, w, ls, fs, fs_ibm,
       icompo, ilayer, val->bnd_norm_u, val->bnd_norm_v, val->bnd_norm_w,
       val->surface_bnd, prm, val, order[1]);
  bcf(f, prm);

  normal_vector_ics(nvx, nvy, nvz, f, cdo, flg);
  plic(f, flx, fly, flz, nvx, nvy, nvz, alpha, u, v, w, ls, fs, fs_ibm,
       icompo, ilayer, val->bnd_norm_u, val->bnd_norm_v, val->bnd_norm_w,
       val->surface_bnd, prm, val, order[2]);
  bcf(f, prm);
}


type advection_vof(type *f, type *u, type *v, type *w, type *ls, type *fs, type *fs_ibm, int icompo, int ilayer, variable *val, parameter *prm)
{
  domain *cdo = prm->cdo;
  flags  *flg=prm->flg;
  type   time0 = cpu_time();
  // memory allocation for work array.
  static int  n1st = 0;
  static type *flx, *fly, *flz, *nvx, *nvy, *nvz, *alpha;
  static type *frk0, *frk1, *frk2;
  size_t size = sizeof(type)*(cdo->m);
  int  j, jx, jy, jz, m=cdo->m, nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       NumCompo=cdo->NumberOfComponent, mx=cdo->mx, mxy=cdo->mxy, stm=cdo->stm;
  type dt = cdo->dt;
  type *lls = val->lls;

  int order[3];

  if (n1st++ == 0) {
    flx = (type *) malloc( size );
    fly = (type *) malloc( size );
    flz = (type *) malloc( size );
    nvx = (type *) malloc( size );
    nvy = (type *) malloc( size );
    nvz = (type *) malloc( size );
    alpha = (type *) malloc( size );
    frk0 = (type *) malloc( size );
    frk1 = (type *) malloc( size );
    frk2 = (type *) malloc( size );
  }

 
  // Fukuda 2023/10/10. Shuffling the order of the direction of advection. 
  if(cdo->icnt % 6 == 0)     {order[0]=0; order[1]=1; order[2]=2;}
  else if(cdo->icnt % 6 == 1){order[0]=2; order[1]=1; order[2]=0;}
  else if(cdo->icnt % 6 == 2){order[0]=1; order[1]=2; order[2]=0;}
  else if(cdo->icnt % 6 == 3){order[0]=0; order[1]=2; order[2]=1;}
  else if(cdo->icnt % 6 == 4){order[0]=2; order[1]=0; order[2]=1;}
  else if(cdo->icnt % 6 == 5){order[0]=1; order[1]=0; order[2]=2;}

  if(prm->flg->interface_capturing_scheme==PLIC){
    copy_vof_field(frk0, f, cdo);

    copy_vof_field(frk1, frk0, cdo);
    advance_split_plic_step(frk1, flx, fly, flz, nvx, nvy, nvz, alpha,
                            u, v, w, ls, fs, fs_ibm, icompo, ilayer,
                            val, prm, order);

    copy_vof_field(frk2, frk1, cdo);
    advance_split_plic_step(frk2, flx, fly, flz, nvx, nvy, nvz, alpha,
                            u, v, w, ls, fs, fs_ibm, icompo, ilayer,
                            val, prm, order);
    combine_vof_field(frk1, 3.0 / 4.0, frk0, 1.0 / 4.0, frk2, cdo);
    bcf(frk1, prm);

    copy_vof_field(frk2, frk1, cdo);
    advance_split_plic_step(frk2, flx, fly, flz, nvx, nvy, nvz, alpha,
                            u, v, w, ls, fs, fs_ibm, icompo, ilayer,
                            val, prm, order);
    combine_vof_field(f, 1.0 / 3.0, frk0, 2.0 / 3.0, frk2, cdo);
    bcf(f, prm);

    
  }else if(prm->flg->interface_capturing_scheme==THINC){ 

    thinc(f, flx,fly,flz, nvx,nvy,nvz, u,v,w, ls, fs, fs_ibm, icompo,
              val->bnd_norm_u, val->bnd_norm_v, val->bnd_norm_w,
              val->surface_bnd, prm, order[0]); // VOF value update by flux along 1st-direction. 
    bcf(f,prm);

    thinc(f, flx,fly,flz, nvx,nvy,nvz, u,v,w, ls, fs, fs_ibm, icompo,
              val->bnd_norm_u, val->bnd_norm_v, val->bnd_norm_w,
              val->surface_bnd, prm, order[1]); // VOF value update by flux along 2nd-direction. 
    bcf(f,prm);

    thinc(f, flx,fly,flz, nvx,nvy,nvz, u,v,w, ls, fs, fs_ibm, icompo,
              val->bnd_norm_u, val->bnd_norm_v, val->bnd_norm_w,
              val->surface_bnd, prm, order[2]); // VOF value update by flux along 3rd-direction. 
    bcf(f,prm);

  }else if(prm->flg->interface_capturing_scheme==THINC_WLIC || prm->flg->interface_capturing_scheme==THINC_AWLIC ){

    normal_vector_ics(nvx, nvy, nvz, f, cdo, flg);
    thinc(f, flx,fly,flz, nvx,nvy,nvz, u,v,w, ls, fs, fs_ibm, icompo,
              val->bnd_norm_u, val->bnd_norm_v, val->bnd_norm_w,
              val->surface_bnd, prm, order[0]); // VOF value update by flux along 1st-direction. 
    bcf(f,prm);

    normal_vector_ics(nvx, nvy, nvz, f, cdo, flg);
    thinc(f, flx,fly,flz, nvx,nvy,nvz, u,v,w, ls, fs, fs_ibm, icompo,
              val->bnd_norm_u, val->bnd_norm_v, val->bnd_norm_w,
              val->surface_bnd, prm, order[1]); // VOF value update by flux along 2nd-direction. 
    bcf(f,prm);

    normal_vector_ics(nvx, nvy, nvz, f, cdo, flg);
    thinc(f, flx,fly,flz, nvx,nvy,nvz, u,v,w, ls, fs, fs_ibm, icompo,
              val->bnd_norm_u, val->bnd_norm_v, val->bnd_norm_w,
              val->surface_bnd, prm, order[2]); // VOF value update by flux along 3rd-direction. 
    bcf(f,prm);

  }

  // added 2017/09/22
  if(prm->flg->solute_diff == ON){
    for(j = 0; j < cdo->m; j++){
      //val->vfx[j] = flx[j]/dt;
      //val->vfy[j] = fly[j]/dt;
      //val->vfz[j] = flz[j]/dt;
      //val->vfx[j] = u[j];
      //val->vfy[j] = v[j];
      //val->vfz[j] = w[j];
    }
  }

  return cpu_time() - time0;
}

type advection_vof_fls(type *f, type *u, type *v, type *w, type *ls, type *fs, type *fs_ibm, int icompo, variable *val, parameter *prm)
{
  domain *cdo = prm->cdo;
  flags  *flg=prm->flg;
  type   time0 = cpu_time();
  int    j;
  // memory allocation for work array.
  static int  n1st = 0;
  static type *flx, *fly, *flz, *nvx, *nvy, *nvz;
  size_t size = sizeof(type)*(cdo->m);
  if (n1st++ == 0) {
#ifdef _MEM_ALIGN_
    flx = (type *) _mm_malloc( size,32 );
    fly = (type *) _mm_malloc( size,32 );
    flz = (type *) _mm_malloc( size,32 );
    nvx = (type *) _mm_malloc( size,32 );
    nvy = (type *) _mm_malloc( size,32 );
    nvz = (type *) _mm_malloc( size,32 );
#else
    flx = (type *) malloc( size );
    fly = (type *) malloc( size );
    flz = (type *) malloc( size );
    nvx = (type *) malloc( size );
    nvy = (type *) malloc( size );
    nvz = (type *) malloc( size );
#endif
  }
  /*
  normal_vector_ics(nvx, nvy, nvz, f, cdo);
  thinc_wlic(f, flx,fly,flz, nvx,nvy,nvz, u,v,w, ls, fs, fs_ibm, icompo,
             val->bnd_norm_u, val->bnd_norm_v, val->bnd_norm_w,
             val->surface_bnd, prm);  
  */

  int order[3];
  // Fukuda 2023/10/10. Shuffling the order of the direction of advection. 
  if(cdo->icnt % 6 == 0)     {order[0]=0; order[1]=1; order[2]=2;}
  else if(cdo->icnt % 6 == 1){order[0]=2; order[1]=1; order[2]=0;}
  else if(cdo->icnt % 6 == 2){order[0]=1; order[1]=2; order[2]=0;}
  else if(cdo->icnt % 6 == 3){order[0]=0; order[1]=2; order[2]=1;}
  else if(cdo->icnt % 6 == 4){order[0]=2; order[1]=0; order[2]=1;}
  else if(cdo->icnt % 6 == 5){order[0]=1; order[1]=0; order[2]=2;}

    normal_vector_ics(nvx, nvy, nvz, f, cdo, flg);
    thinc(f, flx,fly,flz, nvx,nvy,nvz, u,v,w, ls, fs, fs_ibm, icompo,
              val->bnd_norm_u, val->bnd_norm_v, val->bnd_norm_w,
              val->surface_bnd, prm, order[0]); // VOF value update by flux along 1st-direction. Fukuda 2023/10/7

    normal_vector_ics(nvx, nvy, nvz, f, cdo, flg);
    thinc(f, flx,fly,flz, nvx,nvy,nvz, u,v,w, ls, fs, fs_ibm, icompo,
              val->bnd_norm_u, val->bnd_norm_v, val->bnd_norm_w,
              val->surface_bnd, prm, order[1]); // VOF value update by flux along 2nd-direction. Fukuda 2023/10/7

    normal_vector_ics(nvx, nvy, nvz, f, cdo, flg);
    thinc(f, flx,fly,flz, nvx,nvy,nvz, u,v,w, ls, fs, fs_ibm, icompo,
              val->bnd_norm_u, val->bnd_norm_v, val->bnd_norm_w,
              val->surface_bnd, prm, order[2]); // VOF value update by flux along 3rd-direction. Fukuda 2023/10/7


  return 0.0;
}

type vof_advection(variable *val, parameter *prm)
{
  type time0 = cpu_time();
  domain *cdo=prm->cdo;
  static int n1st = 0;
  int  icompo, ic, j, jx, jy, jz, m=cdo->m, nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       NumCompo=cdo->NumberOfComponent, mx=cdo->mx, mxy=cdo->mxy, stm=cdo->stm;
  type valf;
  int NBaseCompo = cdo->NBaseComponent;

  if(prm->flg->fluid_dynamics == OFF) return 0.0;
//  if(prm->flg->porous == ON) return 0.0;

  if(prm->flg->vof_adv_fls == ON) {
    type *fs = val->fs_sum;
    type *fs_ibm = val->fs_ibm ? val->fs_ibm : fs;

    CSVASSERT(prm->flg->solute_diff != ON);

/*
#pragma omp parallel private(j,valf,ic)
    {
#pragma omp for
      for(j = 0; j < m; j++) val->fls[j] = 0.0;

#pragma omp for
      for(j = 0; j < m; j++) {
        valf = 0.0;
        for(ic = 0; ic < NBaseCompo; ic++){
          valf += val->fl[j+ic*m];
        }
        val->fls[j] = valf;
      }
#pragma omp for
      for(j = 0; j < m; j++) val->fls[j] += val->fs_sum[j];

    }

    advection_vof_fls(val->fls, val->u, val->v, val->w, val->ls, val->fs_sum, val, prm);
*/
    if(prm->flg->multi_layer==OFF){
      for(icompo = 0; icompo < NBaseCompo; icompo++) {
        //advection_vof(&val->fl[icompo*m], val->u, val->v, val->w, val->ls, val->fs_sum, val, prm);
        if(prm->flg->PHASEFIELD == ON) {
          //advection_phase_field(&val->fl[icompo*m], val->u, val->v, val->w, val->ls, val->fs_sum, val, prm);
          //advection_phase_field(&val->fl[icompo*m], val->u, val->v, val->w, val->ll, val->fs_sum, val, prm);
          advection_phase_field(&val->fl[icompo*m], val->u, val->v, val->w, val->lls, fs, fs_ibm, val, prm);
        } else {
          advection_vof(&val->fl[icompo*m], val->u, val->v, val->w, val->ls, val->fs, fs_ibm, icompo, 0, val, prm);
        }
      }      
    }else{

      if(prm->flg->multi_layer_less_coalescence==ON){ 

        advection_vof(val->fl, val->u, val->v, val->w, val->ls, val->fs, fs_ibm, 0, 0, val, prm);

      }else if(prm->flg->multi_layer_no_coalescence==ON){

        for(int ilayer = 0; ilayer < cdo->NumberOfLayer; ilayer++) {
          if(ilayer!=0 && val->bubble_cnt[ilayer]==0) continue;
          advection_vof(&val->fl_layer[ilayer*m], val->u, val->v, val->w, val->ls, val->fs, fs_ibm, 0, ilayer, val, prm);
        }   
        layer_sum_up(val, prm);   

      }

    }

    /*
    for(icompo = 0; icompo < NBaseCompo; icompo++) {

#pragma omp parallel for private(j,valf,ic)

      for(j = 0; j < cdo->m; j++) {
        valf = 0.0;
        for(ic = 0; ic < NBaseCompo; ic++){
          if(ic == icompo) {
            continue;
          }else{
            valf += val->fl[j+ic*m];
          }
        }
        val->fl[j+icompo*m] = val->fls[j] - valf - val->fs_sum[j];
      }
    }
    */
    bcf_VOF(1,val->fl,val,prm);
    return cpu_time() - time0;
  } else {
      if (prm->flg->solute_diff == ON) {
        //advection_vof(val->fl, val->u, val->v, val->w, val->ls, val->fs, val, prm);
        type *fs = val->fs;
        type *fs_ibm = val->fs_ibm ? val->fs_ibm : fs;

        if(prm->flg->PHASEFIELD == ON) {
          //advection_phase_field(val->fl, val->u, val->v, val->w, val->ls, val->fs, val, prm);
          //advection_phase_field(val->fl, val->u, val->v, val->w, val->ll, val->fs, val, prm);
          advection_phase_field(val->fl, val->u, val->v, val->w, val->lls, fs, fs_ibm, val, prm);
        } else {
          advection_vof(val->fl, val->u, val->v, val->w, val->ls, fs, fs_ibm, -2, 0, val, prm);
        }
        /* YSE: Use bcf_VOF to set boundary of fs/fl. */
        bcf_VOF(0, fs, val, prm);
        if (fs != fs_ibm) {
          bcf_VOF(0, fs_ibm, val, prm);
        }
        bcf_VOF(1, val->fl, val, prm);

        return cpu_time() - time0;
      } // else { should be added someday
    }
    return 0.0;
}
