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

int Forchheimer(type mdt, type *ut, type *vt, type *wt, variable *val, material *mtl, parameter *prm)
{
  if(prm->flg->porous != ON) return 1;
  domain      *cdo = prm->cdo;
  phase_value *phv = prm->phv;
  int  j, jx, jy, jz,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my, stm=cdo->stm;
  type *perm = val->perm; // permeability
  type *eps = val->eps;   // porosity
  type *dens = mtl->dens;
  type *mu = mtl->mu;
  type uc, vc, wc, root, per_c, ep_c, dens_c, mu_c;
  //type coef = 1.75/sqrt(150.0);
  //type coef = 0.099418;
  type coef = cdo->Forch_coef;
  static int  n1st = 0;
  size_t size = sizeof(type)*(cdo->m);
  static type *u, *v, *w;

  if ( n1st++ == 0) {
    u = (type *) malloc( size );
    v = (type *) malloc( size );
    w = (type *) malloc( size );
  }

#pragma omp parallel for private(jz,jy,jx,j)
  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*jy + mxy*jz;
        u[j] = ut[j];
        v[j] = vt[j];
        w[j] = wt[j];
      }
    }
  }
  bcu(u, v, w, val, mtl, prm);

#pragma omp parallel private(jz,jy,jx,j,uc,vc,wc,root,per_c,ep_c,dens_c,mu_c)
    {
      //=== u ===
#pragma omp for
      for(jz = 0; jz < nz; jz++) {
        for(jy = 0; jy < ny; jy++) {
          for(jx = 0; jx < nx+1; jx++) {
            j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
            uc = u[j];
            vc = 0.25*(v[j] + v[j-1] + v[j+mx] + v[j-1+mx]);
            wc = 0.25*(w[j] + w[j-1] + w[j+mxy] + w[j-1+mxy]);
            root = sqrt(uc*uc + vc*vc + wc*wc);
            dens_c = 0.5*(dens[j-1] + dens[j]);
            mu_c   = 0.5*(  mu[j-1] +   mu[j]);
            //per_c = MAX2(perm[j], perm[j-1]);
            //ep_c  = MAX2( eps[j],  eps[j-1]);
            per_c = MIN2(perm[j], perm[j-1]);
            ep_c  = MIN2( eps[j],  eps[j-1]);

            if (ep_c > 0.0) {
              if (prm->flg->visc_tvd3 == ON)
                ut[j] = ut[j] - mdt*u[j]*(mu_c*ep_c*per_c/dens_c + coef*sqrt(per_c)/sqrt(ep_c)*root);
              else
                ut[j] = ut[j] - mdt*coef*sqrt(per_c)/sqrt(ep_c)*root*u[j];
            }
          }
        }
      }
      //=== v ===
#pragma omp for
      for(jz = 0; jz < nz; jz++) {
        for(jy = 0; jy < ny+1; jy++) {
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
            uc = 0.25*(u[j] + u[j+1] + u[j-mx] + u[j+1-mx]);
            vc = v[j];
            wc = 0.25*(w[j] + w[j-mx] + w[j+mxy] + w[j-mx+mxy]);
            root = sqrt(uc*uc + vc*vc + wc*wc);
            dens_c = 0.5*(dens[j-mx] + dens[j]);
            mu_c   = 0.5*(  mu[j-mx] +   mu[j]);
            //per_c = MAX2(perm[j], perm[j-mx]);
            //ep_c  = MAX2( eps[j],  eps[j-mx]);
            per_c = MIN2(perm[j], perm[j-mx]);
            ep_c  = MIN2( eps[j],  eps[j-mx]);

            if (ep_c > 0.0) {
              if (prm->flg->visc_tvd3 == ON)
                vt[j] = vt[j] - mdt*v[j]*(mu_c*ep_c*per_c/dens_c + coef*sqrt(per_c)/sqrt(ep_c)*root);
              else
                vt[j] = vt[j] - mdt*coef*sqrt(per_c)/sqrt(ep_c)*root*v[j];
            }
          }
        }
      }
      //=== w ===
#pragma omp for
      for(jz = 0; jz < nz+1; jz++) {
        for(jy = 0; jy < ny; jy++) {
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
            uc = 0.25*(u[j] + u[j+1] + u[j-mxy] + u[j+1-mxy]);
            vc = 0.25*(v[j] + v[j+mx] + v[j-mxy] + v[j+mx-mxy]);
            wc = w[j];
            root = sqrt(uc*uc + vc*vc + wc*wc);
            dens_c = 0.5*(dens[j-mxy] + dens[j]);
            mu_c   = 0.5*(  mu[j-mxy] +   mu[j]);
            //per_c = MAX2(perm[j], perm[j-mxy]);
            //ep_c  = MAX2( eps[j],  eps[j-mxy]);
            per_c = MIN2(perm[j], perm[j-mxy]);
            ep_c  = MIN2( eps[j],  eps[j-mxy]);

            if (ep_c > 0.0) {
              if (prm->flg->visc_tvd3 == ON)
                wt[j] = wt[j] - mdt*w[j]*(mu_c*ep_c*per_c/dens_c + coef*sqrt(per_c)/sqrt(ep_c)*root);
              else
                wt[j] = wt[j] - mdt*coef*sqrt(per_c)/sqrt(ep_c)*root*w[j];
            }
          }
        }
      }
    }   // omp parallel
  return 0;
}
