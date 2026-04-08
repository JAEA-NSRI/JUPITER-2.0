#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "struct.h"
#include "func.h"
#include "os/os.h"

#ifdef _TIME_
extern int it_T;
#endif

type film_thickness_on_cell_face(type *film, int j, int offset)
{
    return (film[j] > 0.0 && film[j-offset] > 0.0) ? 0.5*(film[j]+film[j-offset]) : 0.0;
}

type viscosity_augmentation(type *film, int j1, int j2, int offset, type rapture_thickness)
{
    if(film_thickness_on_cell_face(film, j1, offset)>0.0 && film_thickness_on_cell_face(film, j2, offset)>0.0){
      return 0.5*(film_thickness_on_cell_face(film, j1, offset) + film_thickness_on_cell_face(film, j2, offset)) / rapture_thickness;
    }else{
      return 1.0;
    }

}

type visc_eq(int tvdflag, type *ut, type *vt, type *wt, type *u, type *v, type *w, type *dens, type *mu, variable *val, parameter *prm)
{
  domain *cdo = prm->cdo;
  int  j, jx, jy, jz,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  type dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi,
       cxp, cxm, cyp, cym, czp, czm, dens_c;
  type *eps = val->eps, *perm = val->perm, eps_s;
  //type *liquid_film = val->liquid_film;
  //type vis_augment_w=1.0, vis_augment_e=1.0, vis_augment_s=1.0, vis_augment_n=1.0, vis_augment_b=1.0, vis_augment_t=1.0;
  //type rapture_thickness = cdo->rapture_thickness;

#pragma omp parallel for private(jz,jy,jx,j,dens_c,eps_s,cxp,cxm,cyp,cym,czp,czm)

  for(jz = 0; jz < nz+1; jz++) {
    for(jy = 0; jy < ny+1; jy++) {
      for(jx = 0; jx < nx+1; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        //===== u =====
        dens_c = 0.5*(dens[j] + dens[j-1]);
        //if(prm->flg->film_drainage){
        //  if(cdo->z[jz] > cdo->height_threshold){
        //    vis_augment_e = viscosity_augmentation(liquid_film, j, j+1  , 1, rapture_thickness);
        //    vis_augment_w = viscosity_augmentation(liquid_film, j, j-1  , 1, rapture_thickness);
        //    vis_augment_n = viscosity_augmentation(liquid_film, j, j+mx , 1, rapture_thickness);
        //    vis_augment_s = viscosity_augmentation(liquid_film, j, j-mx , 1, rapture_thickness);
        //    vis_augment_t = viscosity_augmentation(liquid_film, j, j+mxy, 1, rapture_thickness);
        //    vis_augment_b = viscosity_augmentation(liquid_film, j, j-mxy, 1, rapture_thickness);
        //  }
        //}
        cxp = mu[j  ]*(u[j+1] - u[j  ])*dxi;
        cxm = mu[j-1]*(u[j  ] - u[j-1])*dxi;
        cyp = 0.25*(mu[j] + mu[j-1] + mu[j+mx ] + mu[j-1+mx ])*(u[j+mx ] - u[j    ])*dyi;
        cym = 0.25*(mu[j] + mu[j-1] + mu[j-mx ] + mu[j-1-mx ])*(u[j    ] - u[j-mx ])*dyi;
        czp = 0.25*(mu[j] + mu[j-1] + mu[j+mxy] + mu[j-1+mxy])*(u[j+mxy] - u[j    ])*dzi;
        czm = 0.25*(mu[j] + mu[j-1] + mu[j-mxy] + mu[j-1-mxy])*(u[j    ] - u[j-mxy])*dzi;
        if(tvdflag == 0) {
          if (prm->flg->porous == ON) {
            //eps_s = MAX2(eps[j-1], eps[j]);
            eps_s = MIN2(eps[j-1], eps[j]);
            ut[j] = ut[j] + ( (cxp - cxm)*dxi + (cyp - cym)*dyi + (czp - czm)*dzi)/dens_c + eps_s*prm->cdo->grav_x;
          } else {
            ut[j] = ut[j] + ( (cxp - cxm)*dxi + (cyp - cym)*dyi + (czp - czm)*dzi)/dens_c + prm->cdo->grav_x;
          }
        } else
          ut[j] = ( (cxp - cxm)*dxi + (cyp - cym)*dyi + (czp - czm)*dzi)/dens_c + prm->cdo->grav_x;

        //===== v =====
        dens_c = 0.5*(dens[j] + dens[j-mx]);
        //if(prm->flg->film_drainage){
        //  if(cdo->z[jz] > cdo->height_threshold){
        //    vis_augment_e = viscosity_augmentation(liquid_film, j, j+1  , mx, rapture_thickness);
        //    vis_augment_w = viscosity_augmentation(liquid_film, j, j-1  , mx, rapture_thickness);
        //    vis_augment_n = viscosity_augmentation(liquid_film, j, j+mx , mx, rapture_thickness);
        //    vis_augment_s = viscosity_augmentation(liquid_film, j, j-mx , mx, rapture_thickness);
        //    vis_augment_t = viscosity_augmentation(liquid_film, j, j+mxy, mx, rapture_thickness);
        //    vis_augment_b = viscosity_augmentation(liquid_film, j, j-mxy, mx, rapture_thickness);
        //  }
        //}
        cxp = 0.25*(mu[j] + mu[j+1] + mu[j-mx] + mu[j+1-mx])*(v[j+1] - v[j  ])*dxi;
        cxm = 0.25*(mu[j] + mu[j-1] + mu[j-mx] + mu[j-1-mx])*(v[j  ] - v[j-1])*dxi;
        cyp = mu[j   ]*(v[j+mx] - v[j   ])*dyi;
        cym = mu[j-mx]*(v[j   ] - v[j-mx])*dyi;
        czp = 0.25*(mu[j] + mu[j-mx] + mu[j+mxy] + mu[j-mx+mxy])*(v[j+mxy] - v[j    ])*dzi;
        czm = 0.25*(mu[j] + mu[j-mx] + mu[j-mxy] + mu[j-mx-mxy])*(v[j    ] - v[j-mxy])*dzi;
        if(tvdflag == 0) {
          if (prm->flg->porous == ON) {
            //eps_s = MAX2(eps[j-mx], eps[j]);
            eps_s = MIN2(eps[j-mx], eps[j]);
            vt[j] = vt[j] + ( (cxp - cxm)*dxi + (cyp - cym)*dyi + (czp - czm)*dzi )/dens_c + eps_s*prm->cdo->grav_y;
          } else {
            vt[j] = vt[j] + ( (cxp - cxm)*dxi + (cyp - cym)*dyi + (czp - czm)*dzi )/dens_c + prm->cdo->grav_y;
          }
        } else
          vt[j] = ( (cxp - cxm)*dxi + (cyp - cym)*dyi + (czp - czm)*dzi )/dens_c + prm->cdo->grav_y;

        //===== w =====
        //if(prm->flg->film_drainage){
        //  if(cdo->z[jz] > cdo->height_threshold){
        //    vis_augment_e = viscosity_augmentation(liquid_film, j, j+1  , mxy, rapture_thickness);
        //    vis_augment_w = viscosity_augmentation(liquid_film, j, j-1  , mxy, rapture_thickness);
        //    vis_augment_n = viscosity_augmentation(liquid_film, j, j+mx , mxy, rapture_thickness);
        //    vis_augment_s = viscosity_augmentation(liquid_film, j, j-mx , mxy, rapture_thickness);
        //    vis_augment_t = viscosity_augmentation(liquid_film, j, j+mxy, mxy, rapture_thickness);
        //    vis_augment_b = viscosity_augmentation(liquid_film, j, j-mxy, mxy, rapture_thickness);
        //  }
        //}
        dens_c = 0.5*(dens[j] + dens[j-mxy]);
        cxp = 0.25*(mu[j] + mu[j+1] + mu[j-mxy] + mu[j+1-mxy])*(w[j+1] - w[j  ])*dxi;
        cxm = 0.25*(mu[j] + mu[j-1] + mu[j-mxy] + mu[j-1-mxy])*(w[j  ] - w[j-1])*dxi;
        cyp = 0.25*(mu[j] + mu[j-mxy] + mu[j+mx] + mu[j-mxy+mx])*(w[j+mx] - w[j   ])*dyi;
        cym = 0.25*(mu[j] + mu[j-mxy] + mu[j-mx] + mu[j-mxy-mx])*(w[j   ] - w[j-mx])*dyi;
        czp = mu[j    ]*(w[j+mxy] - w[j    ])*dzi;
        czm = mu[j-mxy]*(w[j    ] - w[j-mxy])*dzi;

        if(tvdflag == 0) {
          if (prm->flg->porous == ON) {
            //eps_s = MAX2(eps[j-mxy], eps[j]);
            eps_s = MIN2(eps[j-mxy], eps[j]);
            wt[j] = wt[j] + ( (cxp - cxm)*dxi + (cyp - cym)*dyi + (czp - czm)*dzi )/dens_c + eps_s*prm->cdo->grav_z;
          } else {
            wt[j] = wt[j] + ( (cxp - cxm)*dxi + (cyp - cym)*dyi + (czp - czm)*dzi )/dens_c + prm->cdo->grav_z;
          }
        } else
          wt[j] = ( (cxp - cxm)*dxi + (cyp - cym)*dyi + (czp - czm)*dzi )/dens_c + prm->cdo->grav_z;
      }
    }
  }
  return 0.0;
}

void backup_visc_eq(type *ut, type *vt, type *wt, type *u, type *v, type *w, type *dens, type *mu, variable *val, parameter *prm)
{
  domain *cdo = prm->cdo;
  int  j, jx, jy, jz,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  type dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi,
       cxp, cxm, cyp, cym, czp, czm, dens_c;

#pragma omp parallel private(jz,jy,jx,j,dens_c,cxp,cxm,cyp,cym,czp,czm)
  {

    //=== u ===
#pragma omp for

  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx+1; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        dens_c = 0.5*(dens[j] + dens[j-1]);
        cxp = mu[j  ]*(u[j+1] - u[j  ])*dxi;
        cxm = mu[j-1]*(u[j  ] - u[j-1])*dxi;
        cyp = 0.25*(mu[j] + mu[j-1] + mu[j+mx] + mu[j-1+mx])*(u[j+mx] - u[j   ])*dyi;
        cym = 0.25*(mu[j] + mu[j-1] + mu[j-mx] + mu[j-1-mx])*(u[j   ] - u[j-mx])*dyi;
        czp = 0.25*(mu[j] + mu[j-1] + mu[j+mxy] + mu[j-1+mxy])*(u[j+mxy] - u[j    ])*dzi;
        czm = 0.25*(mu[j] + mu[j-1] + mu[j-mxy] + mu[j-1-mxy])*(u[j    ] - u[j-mxy])*dzi;

        ut[j] = ut[j] + ( (cxp - cxm)*dxi + (cyp - cym)*dyi + (czp - czm)*dzi)/dens_c + prm->cdo->grav_x;
      }
    }
  }
  //=== v ===
#pragma omp for

  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny+1; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        dens_c = 0.5*(dens[j] + dens[j-mx]);
        cxp = 0.25*(mu[j] + mu[j+1] + mu[j-mx] + mu[j+1-mx])*(v[j+1] - v[j  ])*dxi;
        cxm = 0.25*(mu[j] + mu[j-1] + mu[j-mx] + mu[j-1-mx])*(v[j  ] - v[j-1])*dxi;
        cyp = mu[j   ]*(v[j+mx] - v[j   ])*dyi;
        cym = mu[j-mx]*(v[j   ] - v[j-mx])*dyi;
        czp = 0.25*(mu[j] + mu[j-mx] + mu[j+mxy] + mu[j-mx+mxy])*(v[j+mxy] - v[j])*dzi;
        czm = 0.25*(mu[j] + mu[j-mx] + mu[j-mxy] + mu[j-mx-mxy])*(v[j] - v[j-mxy])*dzi;

        vt[j] = vt[j] + ( (cxp - cxm)*dxi + (cyp - cym)*dyi + (czp - czm)*dzi )/dens_c + prm->cdo->grav_y;
      }
    }
  }
  //=== w ===
#pragma omp for

  for(jz = 0; jz < nz+1; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        dens_c = 0.5*(dens[j] + dens[j-mxy]);
        cxp = 0.25*(mu[j] + mu[j+1] + mu[j-mxy] + mu[j+1-mxy])*(w[j+1] - w[j])*dxi;
        cxm = 0.25*(mu[j] + mu[j-1] + mu[j-mxy] + mu[j-1-mxy])*(w[j] - w[j-1])*dxi;
        cyp = 0.25*(mu[j] + mu[j-mxy] + mu[j+mx] + mu[j-mxy+mx])*(w[j+mx] - w[j])*dyi;
        cym = 0.25*(mu[j] + mu[j-mxy] + mu[j-mx] + mu[j-mxy-mx])*(w[j] - w[j-mx])*dyi;
        czp = mu[j    ]*(w[j+mxy] - w[j    ])*dzi;
        czm = mu[j-mxy]*(w[j    ] - w[j-mxy])*dzi;

        wt[j] = wt[j] + ( (cxp - cxm)*dxi + (cyp - cym)*dyi + (czp - czm)*dzi )/dens_c + prm->cdo->grav_z;
      }
    }
  }

  }  // omp parallel

}


type heat_eq(type *tn, type *t, type *dens, type *thc, type *specht, type *q, type *rad, type dt, domain *cdo)
{
  int  j, jx, jy, jz,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  type dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi,
       cxp, cxm, cyp, cym, czp, czm, dfdt;

  //=== t ===

#pragma omp parallel for private(jz,jy,jx,j,cxp,cxm,cyp,cym,czp,czm,dfdt)

  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        // harmonic mean
        cxp = 2.0/(1.0/thc[j] + 1.0/thc[j+1])*(t[j+1] - t[j  ])*dxi;
        cxm = 2.0/(1.0/thc[j] + 1.0/thc[j-1])*(t[j  ] - t[j-1])*dxi;
        cyp = 2.0/(1.0/thc[j] + 1.0/thc[j+mx])*(t[j+mx] - t[j   ])*dyi;
        cym = 2.0/(1.0/thc[j] + 1.0/thc[j-mx])*(t[j   ] - t[j-mx])*dyi;
        czp = 2.0/(1.0/thc[j] + 1.0/thc[j+mxy])*(t[j+mxy] - t[j    ])*dzi;
        czm = 2.0/(1.0/thc[j] + 1.0/thc[j-mxy])*(t[j    ] - t[j-mxy])*dzi;

        // Arithmetric mean
        //cxp = 0.5*(thc[j] + thc[j+1])*(t[j+1] - t[j  ])*dxi;
        //cxm = 0.5*(thc[j] + thc[j-1])*(t[j  ] - t[j-1])*dxi;
        //cyp = 0.5*(thc[j] + thc[j+mx])*(t[j+mx] - t[j   ])*dyi;
        //cym = 0.5*(thc[j] + thc[j-mx])*(t[j   ] - t[j-mx])*dyi;
        //czp = 0.5*(thc[j] + thc[j+mxy])*(t[j+mxy] - t[j    ])*dzi;
        //czm = 0.5*(thc[j] + thc[j-mxy])*(t[j    ] - t[j-mxy])*dzi;

        dfdt = ( (cxp - cxm)*dxi + (cyp - cym)*dyi + (czp - czm)*dzi + q[j] - rad[j])/dens[j]/specht[j];

        tn[j] = t[j] + dfdt*dt;
      }
    }
  }
  return 0.0;
}

void heat_eq_tvd3(type *fn, type a0, type *f0, type af, type aft, type *f,
     type *dens, type *thc, type *specht, type *q, type *rad,domain *cdo, variable *val, parameter *prm)
     
{
  int  j, jx, jy, jz, nx=cdo->nx, ny=cdo->ny, nz=cdo->nz, mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  type dx=cdo->dx, dy=cdo->dy, dz=cdo->dz, dxi=1.0/dx, dyi=1.0/dy, dzi=1.0/dz,
       cxp, cxm, cyp, cym, czp, czm, dfdt;

  //=== t ===
#pragma omp parallel for private(jz,jy,jx,j,cxp,cxm,cyp,cym,czp,czm,dfdt)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        // harmonic mean
        cxp = 2.0/(1.0/thc[j] + 1.0/thc[j+1])*(f[j+1] - f[j  ])*dxi;
        cxm = 2.0/(1.0/thc[j] + 1.0/thc[j-1])*(f[j  ] - f[j-1])*dxi;
        cyp = 2.0/(1.0/thc[j] + 1.0/thc[j+mx])*(f[j+mx] - f[j   ])*dyi;
        cym = 2.0/(1.0/thc[j] + 1.0/thc[j-mx])*(f[j   ] - f[j-mx])*dyi;
        czp = 2.0/(1.0/thc[j] + 1.0/thc[j+mxy])*(f[j+mxy] - f[j    ])*dzi;
        czm = 2.0/(1.0/thc[j] + 1.0/thc[j-mxy])*(f[j    ] - f[j-mxy])*dzi;

        // Arithmetric mean
        //cxp = 0.5*(thc[j] + thc[j+1])*(f[j+1] - f[j  ])*dxi;
        //cxm = 0.5*(thc[j] + thc[j-1])*(f[j  ] - f[j-1])*dxi;
        //cyp = 0.5*(thc[j] + thc[j+mx])*(f[j+mx] - f[j   ])*dyi;
        //cym = 0.5*(thc[j] + thc[j-mx])*(f[j   ] - f[j-mx])*dyi;
        //czp = 0.5*(thc[j] + thc[j+mxy])*(f[j+mxy] - f[j    ])*dzi;
        //czm = 0.5*(thc[j] + thc[j-mxy])*(f[j    ] - f[j-mxy])*dzi;

        if (prm->flg->porous == ON)
          dfdt = ( (cxp - cxm)*dxi + (cyp - cym)*dyi + (czp - czm)*dzi 
               +   (1.0-val->eps[j])*q[j] - rad[j])/dens[j]/specht[j]/val->sgm[j];
        else
          dfdt = ( (cxp - cxm)*dxi + (cyp - cym)*dyi + (czp - czm)*dzi + q[j] - rad[j])/dens[j]/specht[j];

        fn[j] = a0*f0[j] + af*f[j] + aft*dfdt;
      }
    }
  }
}

type heat_conduction_tvd3(type *t, material *mtl, variable *val, parameter *prm)
{
  flags       *flg=prm->flg;
  phase_value *phv=prm->phv;
  domain *cdo=prm->cdo;
  type time0 = cpu_time();
  if(flg->heat_eq != ON) return 0.0;
  // memory allocation for work array.
  static int  n1st = 0;
  static type *tn, *tp;
  size_t size = sizeof(type)*(cdo->m);
  if (n1st++ == 0) {
#ifdef _MEM_ALIGN_
    tn = (type *) _mm_malloc( size,32 );
    tp = (type *) _mm_malloc( size,32 );
#else
    tn = (type *) malloc( size );
    tp = (type *) malloc( size );
#endif
  }
  const int NCompo = cdo->NBaseComponent;//added by Chai
  int  itr = 0, itr_max = cdo->nsub_step_t;
  type mdt = cdo->dt / ((type) cdo->nsub_step_t);
  type *thc=mtl->thc, *q=mtl->q, *rad=mtl->rad, *latent=mtl->latent;
  type *dens,*specht;
  type *t_liq=mtl->t_liq, *t_soli=mtl->t_soli;
  type *fs=val->fs, *fl=val->fl;
  int *mushy=val->mushy;//added by Chai
  int m=cdo->m;
  if (flg->phase_change == ON) {
    cp_update(cdo, flg, phv, fl, fs, mushy, mtl->specht, latent, t_liq, t_soli, t);
  }
  if (flg->porous == ON) {
    dens = mtl->dens_f;
    specht = mtl->c_f;
  } else {
    dens = mtl->dens;
    specht = mtl->specht;
  }

/*Fukuda enthaply monitor*/
  if (flg->phase_change == OFF && flg->two_energy == OFF){
    type *sgm=val->sgm;
    type prv_enthalpy; 
    type dv = cdo->dx*cdo->dy*cdo->dz;
    int  j,jx,jy,jz, i, nx=cdo->nx,ny=cdo->ny,nz=cdo->nz,mx=cdo->mx,my=cdo->my,mxy=mx*my,stm=cdo->stm;
  #pragma omp parallel for private(jz,jy,jx,j,i,prv_enthalpy)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
          i = jx + nx*jy + nx*ny*jz;
          if (flg->porous == ON) {
            prv_enthalpy = val->enthalpy[j];
            val->enthalpy[j] = sgm[j]*dens[j]*specht[j]*t[j]*dv;
            if(cdo->icnt==0) val->init_enthalpy[j] = val->enthalpy[j];
            if(cdo->icnt>0) val->enthalpy_time_derivative[j] = val->enthalpy[j] - prv_enthalpy; // dividec by cdo->dt in dt_control.c
          } else {
            prv_enthalpy = val->enthalpy[j];
            val->enthalpy[j] = dens[j]*specht[j]*t[j]*dv;
            if(cdo->icnt==0) val->init_enthalpy[j] = val->enthalpy[j];
            if(cdo->icnt>0) val->enthalpy_time_derivative[j] = val->enthalpy[j] - prv_enthalpy; // dividec by cdo->dt in dt_control.c
          }
        }
      }
    }    
  }


  while (itr++ < itr_max) {
    heat_eq_tvd3(tn,    1., t,    0.,      mdt, t,  dens,thc,specht,q,rad,cdo,val,prm); bct(tn, val, prm);
    heat_eq_tvd3(tp, 3./4., t, 1./4.,1./4.*mdt, tn, dens,thc,specht,q,rad,cdo,val,prm); bct(tp, val, prm);
    heat_eq_tvd3(t,  1./3., t, 2./3.,2./3.*mdt, tp, dens,thc,specht,q,rad,cdo,val,prm); bct(t,  val, prm);
  }
  return cpu_time() - time0;
}

//------ implicit
type heat_conduction_imp(type *t, material *mtl, variable *val, parameter *prm)
{
  type time0 = cpu_time();
  if(prm->flg->heat_eq != ON) return 0.0;
  domain *cdo=prm->cdo;
  flags *flg=prm->flg;
  int  j,jx,jy,jz, i, nx=cdo->nx,ny=cdo->ny,nz=cdo->nz,mx=cdo->mx,my=cdo->my,mxy=mx*my,stm=cdo->stm;
  type *thc=mtl->thc, *specht, *dens;
  type *sgm=val->sgm, *eps=val->eps;
  type *tf=val->tf, *ts=val->ts;

  type dti=1.0/cdo->dt;
  type hex; //heat exchange
  type rscs; // rho_s*c_s
  
/*Fukuda enthaply monitor*/
  type prv_enthalpy; 
  type dv = cdo->dx*cdo->dy*cdo->dz; 

#ifdef AMGS
  int n = cdo->n;
  int m = cdo->m;
  type dx = cdo->dx, dy = cdo->dy, dz = cdo->dz;
  type dxi2=cdo->dxi*cdo->dxi, dyi2=cdo->dyi*cdo->dyi, dzi2=cdo->dzi*cdo->dzi;
  type bc_xm=flg->bc_xm, bc_xp=flg->bc_xp,
       bc_ym=flg->bc_ym, bc_yp=flg->bc_yp,
       bc_zm=flg->bc_zm, bc_zp=flg->bc_zp;
  mpi_param *mpi = prm->mpi;
  int param[6];
  int mtl_flg, vel_flg;
#endif

  if (flg->phase_change == ON) {
    cp_update(cdo, flg, prm->phv, val->fl, val->fs, val->mushy, mtl->specht, mtl->latent, mtl->t_liq, mtl->t_soli, val->t);
  }
  if (flg->porous == ON) {
    dens = mtl->dens_f;
    specht = mtl->c_f;
  } else {
    dens = mtl->dens;
    specht = mtl->specht;
  }
   
#pragma omp parallel for private(jz,jy,jx,j,i,hex,rscs,prv_enthalpy)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
        i = jx + nx*jy + nx*ny*jz;
        if (flg->porous == ON) {
          if (flg->two_energy == OFF) {
            mtl->div_b[i] = -sgm[j]*dens[j]*specht[j]*t[j]*dti - (1.0-eps[j])*mtl->q[j] - mtl->rad[j]; 

            /*Fukuda enthaply monitor*/
            prv_enthalpy = val->enthalpy[j];
            val->enthalpy[j] = sgm[j]*dens[j]*specht[j]*t[j]*dv;
            if(cdo->icnt==0) val->init_enthalpy[j] = val->enthalpy[j];
            if(cdo->icnt>0) val->enthalpy_time_derivative[j] = val->enthalpy[j] - prv_enthalpy; // dividec by cdo->dt in dt_control.c

          } else {
            /* Fluid phase */
            hex = 0.0;
            if (eps[j] < 1.0) hex = cdo->a_sf*cdo->h_f*(ts[j] - tf[j]);
            mtl->div_b[i] = -eps[j]*dens[j]*specht[j]*tf[j]*dti - hex - mtl->rad[j];
            /* Porous phase */
            hex = 0.0;
            rscs = mtl->denss[j]*mtl->specht[j]; // Including porous phase
            if (eps[j] < 1.0) hex = cdo->a_sf*cdo->h_f*(tf[j] - ts[j]);
            mtl->div_a[i] = -(1.0-eps[j])*rscs*ts[j]*dti - hex - (1.0-eps[j])*mtl->q[j] - mtl->rad[j];
          }
        } else {
          mtl->div_b[i] = - dens[j]*specht[j]*t[j]*dti - mtl->q[j] - mtl->rad[j];

          /*Fukuda enthaply monitor*/
          prv_enthalpy = val->enthalpy[j];
          val->enthalpy[j] = dens[j]*specht[j]*t[j]*dv;
          if(cdo->icnt==0) val->init_enthalpy[j] = val->enthalpy[j];
          if(cdo->icnt>0) val->enthalpy_time_derivative[j] = val->enthalpy[j] - prv_enthalpy; // dividec by cdo->dt in dt_control.c

        }
      }
    }
  }

#ifdef AMGS
  param[0] = bc_xm;
  param[1] = bc_xp;
  param[2] = bc_ym;
  param[3] = bc_yp;
  param[4] = bc_zm;
  param[5] = bc_zp;
  mtl_flg = 2;
  vel_flg = 0;
  amgs_solve_(&mtl_flg, &vel_flg, val->t, mtl->div_b , dens, mtl->mu, mtl->thc, mtl->specht, &nx, &ny, &nz, &n, &mx, &my, &mxy, &m, &stm,
              &dxi2, &dyi2, &dzi2, &dti, mpi->nrk, param, &mpi->npe_x, &mpi->npe_xy, &mpi->npe, &mpi->rank);
#elif PETSc
  petsc_poisson_T(1,val->t, mtl->div_b, thc, mtl, val, prm);
#elif CCSE
#ifdef _TIME_
  it_T += ccse_poisson(4,val->t, mtl->div_b, dens, mtl, val, prm);
#else
  if (flg->two_energy == OFF) {
    ccse_poisson(4,val->t, mtl->div_b, dens, mtl, val, prm);
    bct(t, val, prm);
  } else {
    ccse_poisson(5,val->tf, mtl->div_b, dens, mtl, val, prm); //dens = dens_f
    ccse_poisson(6,val->ts, mtl->div_a, mtl->denss, mtl, val, prm);
    bct(tf, val, prm);
    bct(ts, val, prm);
  }
#endif
#endif

  return cpu_time() - time0;
}


type heat_conduction(type *t, material *mtl, variable *val, parameter *prm)
{
  if(prm->flg->heat_tvd3 == ON) {
    return heat_conduction_tvd3(t, mtl, val, prm);
  } else {
    return heat_conduction_imp(t, mtl, val, prm);
  }
}
