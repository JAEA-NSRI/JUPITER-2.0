#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef GPU
  #include <cuda.h>
  #include <cutil.h>
#endif

#include "struct.h"
#include "func.h"
#include "os/os.h"

#ifdef _TIME_
extern type time_thinc_wlic;
extern type time_normal_vector_wlic;
#endif

//==============================================================
//    Conservative Allen-Cahn eq.
//                                 2021/04/22  Kenta Sugihara
//--------------------------------------------------------------
void normal_vector_PF_cellcenter(type *nvx, type *nvy, type *nvz, type *f, domain *cdo)
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

    det = sqrt(nvx_tmp*nvx_tmp + nvy_tmp*nvy_tmp + nvz_tmp*nvz_tmp) + 1.0e-08;

    nvx[j] = nvx_tmp/det;
    nvy[j] = nvy_tmp/det;
    nvz[j] = nvz_tmp/det;
            }
        }
    }
#ifdef _TIME_
    time_normal_vector_wlic += cpu_time()-time0;
#endif
}


void normal_vector_PF(type *nvx, type *nvy, type *nvz, type *ll, domain *cdo)
{
    int  j, jx, jy, jz,
         nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
         mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
    type dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi, det;
    type nvy_w, nvz_w,
         nvx_s, nvz_s,
         nvx_b, nvy_b;
#ifdef _TIME_
    type time0=cpu_time();
#endif

#pragma omp parallel for private(jz,jy,jx,j,nvy_w,nvz_w,nvx_s,nvz_s,nvx_b,nvy_b,det)
    for(jz = -1; jz < nz+1; jz++) {
        for(jy = -1; jy < ny+1; jy++) {
            for(jx = -1; jx < nx+1; jx++) {
    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

    //---[nvx, cell-face-x]---
    nvx[j] = (ll[j] - ll[j-1])*dxi;
    nvy_w  = 0.5*( 0.5*(ll[j+mx   ] - ll[j-mx   ])*dyi
                 + 0.5*(ll[j+mx -1] - ll[j-mx -1])*dyi );
    nvz_w  = 0.5*( 0.5*(ll[j+mxy  ] - ll[j-mxy  ])*dzi
                 + 0.5*(ll[j+mxy-1] - ll[j-mxy-1])*dzi );
    det = sqrt(nvx[j]*nvx[j] + nvy_w*nvy_w + nvz_w*nvz_w) + 1.0e-08;
    nvx[j] = nvx[j]/det;

    //---[nvy, cell-face-y]---
    nvx_s  = 0.5*( 0.5*(ll[j+1   ] - ll[j-1   ])*dxi
                 + 0.5*(ll[j+1-mx] - ll[j-1-mx])*dxi);
    nvy[j] = (ll[j] - ll[j-mx])*dyi;
    nvz_s  = 0.5*( 0.5*(ll[j+mxy   ] - ll[j-mxy   ])*dzi
                 + 0.5*(ll[j+mxy-mx] - ll[j-mxy-mx])*dzi);
    det = sqrt(nvx_s*nvx_s + nvy[j]*nvy[j] + nvz_s*nvz_s) + 1.0e-08;
    nvy[j] = nvy[j]/det;

    //---[nvz, cell-face-z]---
    nvx_b  = 0.5*( 0.5*(ll[j+1    ] - ll[j-1    ])*dxi
                 + 0.5*(ll[j+1-mxy] - ll[j-1-mxy])*dxi);
    nvy_b  = 0.5*( 0.5*(ll[j+mx    ] - ll[j-mx    ])*dyi
                 + 0.5*(ll[j+mx-mxy] - ll[j-mx-mxy])*dyi);
    nvz[j] = (ll[j] - ll[j-mxy])*dzi;
    det = sqrt(nvx_b*nvx_b + nvy_b*nvy_b + nvz[j]*nvz[j]) + 1.0e-08;
    nvz[j] = nvz[j]/det;
            }
        }
    }
#ifdef _TIME_
    time_normal_vector_wlic += cpu_time()-time0;
#endif
}


//=======================
//   3rd-order WENO
//-----------------------
type WENO3(type v1, type v2, type v3)
{
    const type eps = 1.0e-08;
    type IS = ((v2 - v1)*(v2 - v1) + eps)/((v3 - v2)*(v3 - v2) + eps),
         w  = 1./(1. + 2.*IS*IS);
    return 0.5*((v2 + v3) - w*(v1 - 2.*v2 + v3));
}

type WENO3_flux(type u, type fw2, type fw, type fe, type fe2)
{
    type v1, v2, v3;
    if (u > 0.) {
        v1 = fw2;
        v2 = fw;
        v3 = fe;
    } else {
        v3 = fw;
        v2 = fe;
        v1 = fe2;
    }
    return WENO3(v1, v2, v3);
}

//=======================
//   3rd-order  MUSCLE
//-----------------------
type muscl_3rd(type fup2, type fup, type fdown)
{
    const type dfp = fup2 - fup;
    const type dfm = fup - fdown;
    return fup - 0.25 * (4.0/3.0 * dfm + 2.0/3.0 * dfp );
}

type minmod(type a, type b)
{
    const type sa = a >= 0.0? 1.0 : -1.0;
    return sa * fmax(0.0, fmin(sa*a, sa*b));
}

type muscl_3rd_tvd(type fup2, type fup, type fdown)
{
    const type b = 4.0;
    const type dfp = fup2 - fup;
    const type dfm = fup - fdown;
    return fup - 0.25 * (4.0/3.0 * minmod(dfm, dfp*b) + 2.0/3.0 * minmod(dfp, dfm*b));
}

type MUSCL3 (
   const type u,
   const type fw2,
   const type fw,
   const type fe,
   const type fe2
)
//-----
{
    return (u > 0.0)? muscl_3rd_tvd(fw2, fw, fe)
                    : muscl_3rd_tvd(fe2, fe, fw);
}

//=======================
//   Phase Field
//-----------------------
void phase_field_eq
(
    type *fnew, type a0,
    type *f0,   type af, type aft,
    type *f, type *u, type *v, type *w,
    type *nvx, type *nvy, type *nvz,
    type *flx, type *fly, type *flz,
    type diff_coef, type anti_coef,
    type *fs, type *fs_ibm,
    parameter *prm
) {
  flags     *flg=prm->flg;
  domain    *cdo=prm->cdo;
  mpi_param *mpi=prm->mpi;
  int  j, jx, jy, jz,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  type dt=cdo->dt, dx=cdo->dx, dy=cdo->dy, dz=cdo->dz,
       dti=1.0/dt, dxi=1.0/dx, dyi=1.0/dy, dzi=1.0/dz;

  // VOF flux calculation
#pragma omp parallel for private(jz,jy,jx,j)
  for(jz = 0; jz < nz+1; jz++) {
    for(jy = 0; jy < ny+1; jy++) {
      for(jx = 0; jx < nx+1; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        type u_w = u[j],//cell-face-x
             v_s = v[j],//cell-face-y
             w_b = w[j];//cell-face-z
//        type fup_w = MUSCL3(u_w, f[j-2    ], f[j-1  ], f[j], f[j+1  ] ),
//             fup_s = MUSCL3(v_s, f[j-2*mx ], f[j-mx ], f[j], f[j+mx ] ),
//             fup_b = MUSCL3(w_b, f[j-2*mxy], f[j-mxy], f[j], f[j+mxy] );
        type fup_w = WENO3_flux(u_w, f[j-2    ], f[j-1  ], f[j], f[j+1  ] ),
             fup_s = WENO3_flux(v_s, f[j-2*mx ], f[j-mx ], f[j], f[j+mx ] ),
             fup_b = WENO3_flux(w_b, f[j-2*mxy], f[j-mxy], f[j], f[j+mxy] );
        type fx_w = (f[j] - f[j-1  ])*dxi,
             fy_s = (f[j] - f[j-mx ])*dyi,
             fz_b = (f[j] - f[j-mxy])*dzi;
        type f_w = 0.5*(f[j] + f[j-1  ]),
             f_s = 0.5*(f[j] + f[j-mx ]),
             f_b = 0.5*(f[j] + f[j-mxy]);

        flx[j] = - fup_w*u_w + diff_coef*fx_w - anti_coef*f_w*(1. - f_w)*0.5*(nvx[j] + nvx[j-1]);
        fly[j] = - fup_s*v_s + diff_coef*fy_s - anti_coef*f_s*(1. - f_s)*0.5*(nvy[j] + nvy[j-mx]);
        flz[j] = - fup_b*w_b + diff_coef*fz_b - anti_coef*f_b*(1. - f_b)*0.5*(nvz[j] + nvz[j-mxy]);
      }//end of jx
    }//end of jy
  }//end of jz

  // time integration
#pragma omp parallel for private(jz,jy,jx,j)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        type flw = flx[j    ],
             fle = flx[j+1  ],
             fls = fly[j    ],
             fln = fly[j+mx ],
             flb = flz[j    ],
             flt = flz[j+mxy];

        if(flg->IBM == ON) {
          IBM_flux_corr_vof(&flw,&fle,&fls,&fln,&flb,&flt, j,fs_ibm,cdo);
        }
        //outflow_flux2(&flw,&fle,&fls,&fln,&flb,&flt, jx,jy,jz, nx,ny,nz, mpi,flg);
        outflow_flux(&flw,&fle,&fls,&fln,&flb,&flt, jx,jy,jz, nx,ny,nz, mpi);

        type dfdt =   (fle - flw)*dxi
                    + (fln - fls)*dyi
                    + (flt - flb)*dzi;
        fnew[j] = a0*f0[j] + af*f[j] + aft*dfdt;
      }//end of jx
    }//end of jy
  }//end of jz

}

//================================================================================
// Interface Profile can be expressed tanh function                               
// Like Below Ascii Art                                                           
//                                                                                
//                                                                                
//                            - - - - - - - - - -  ..-----Liquid Phase------ = 1.0
//                (1 - lambda)_ _ _ _ _ _ _ _ _ _／                               
//                                             ／ |                               
//                                            /                                   
//                                           /    |                               
//                                          /                                     
//                                         /      |                               
//                                        /                                       
//                                       /        |                               
//                                      /                                         
//                                     /          |                               
//                                    /                                           
//                                   /            |                               
//                   lambda _ _ _ _／                                             
//                               ／|              |                               
//   0.0 = ------Gas Phase-----"                                                  
//                                 |              |                               
//                                                                                
//                                 |<--  delta -->|                               
//                                ( = c_delta * dx )                              
//--------------------------------------------------------------------------------

void phase_field(type *f, type *flx, type *fly, type *flz, type *nvx, type *nvy, type *nvz,
                 type *u, type *v, type *w, type *ls, type *fs, type *fs_ibm, parameter *prm)
{
    domain *cdo=prm->cdo;
    // memory allocation for work array.
    static int  n1st = 0;
    static type *fm,
                *fn;
    size_t size = sizeof(type)*(cdo->m);
    if (n1st++ == 0) {
        fm = (type *) malloc( size );
        fn = (type *) malloc( size );
    }

    //=== Phase Field Parameter ===============
    type umax    = cdo->vel_max,
         vel_ref = 1.0; //reference velocity ( > 0)
    if (vel_ref > umax) umax = vel_ref;

    type delta     = cdo->c_delta * cdo->dx,
         b         = 2.0*atanh(1.0-2.0*cdo->lambda);
    type epsilon = delta / (2.0*b),
         gamma_bar = cdo->mobility * cdo->sigma_pf * umax;
    type anti_coef = gamma_bar,
         diff_coef = gamma_bar * epsilon;
    //-----------------------------------------

    //[1/3]
    phase_field_eq
        ( fn, 1.,
          f,  0., cdo->dt,
          f, u, v, w,
          nvx, nvy, nvz,
          flx, fly, flz,
          diff_coef, anti_coef,
          fs, fs_ibm,
          prm);
    bcf(fn, prm);

    //[2/3]
    phase_field_eq
        ( fm, 3./4.,
          f,  1./4., 1./4.*cdo->dt,
          fn, u, v, w,
          nvx, nvy, nvz,
          flx, fly, flz,
          diff_coef, anti_coef,
          fs, fs_ibm,
          prm);
    bcf(fm, prm);

    //[3/3]
    phase_field_eq
        ( f,  1./3.,
          f,  2./3., 2./3.*cdo->dt,
          fm, u, v, w,
          nvx, nvy, nvz,
          flx, fly, flz,
          diff_coef, anti_coef,
          fs, fs_ibm,
          prm);
    bcf(f, prm);
}


type advection_phase_field(type *f, type *u, type *v, type *w, type *ll, type *fs, type *fs_ibm, variable *val, parameter *prm)
{
  domain *cdo = prm->cdo;
  type   time0 = cpu_time();
  // memory allocation for work array.
  static int  n1st = 0;
  static type *flx, *fly, *flz, *nvx, *nvy, *nvz;
  size_t size = sizeof(type)*(cdo->m);
  int  icompo, ic, j, jx, jy, jz, m=cdo->m, nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       NumCompo=cdo->NumberOfComponent, mx=cdo->mx, mxy=cdo->mxy, stm=cdo->stm;
  type dt = cdo->dt;
  type *lls = val->lls;

  if (n1st++ == 0) {
    flx = (type *) malloc( size );
    fly = (type *) malloc( size );
    flz = (type *) malloc( size );
    nvx = (type *) malloc( size );
    nvy = (type *) malloc( size );
    nvz = (type *) malloc( size );
  }

  //normal_vector_PF(nvx, nvy, nvz, ll, cdo);
  normal_vector_PF_cellcenter(nvx, nvy, nvz, ll, cdo);
  phase_field(f, flx,fly,flz, nvx,nvy,nvz, u,v,w, ll, fs, fs_ibm, prm);

  return cpu_time() - time0;
}
//--------------------------------------------------------------
//  end of  Conservative Allen-Cahn eq.
//==============================================================


