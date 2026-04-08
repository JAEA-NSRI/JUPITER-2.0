#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef GPU
  #include <cuda.h>
  #include <cutil.h>
#endif

#include "struct.h"
#include "func.h"

#include "update_level_set_flags.h"
#include "csvutil.h"
#include "os/os.h"

#ifdef _TIME_
extern int it_U,it_V,it_W;
#endif

void update_tvd3(type *fn, type a0, type *f0, type af, type aft, type *f, type *ft, domain *cdo)
{
  int  j,jx,jy,jz,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;

#pragma omp parallel for private(jz,jy,jx,j)
  for(jz = 0; jz < nz+1; jz++) {
    for(jy = 0; jy < ny+1; jy++) {
      for(jx = 0; jx < nx+1; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        fn[j] = a0*f0[j] + af*f[j] + aft*ft[j];
      }
    }
  }
}

void set_b(int flag, type *f, type *f0, type *f1, type *f2, variable *val, parameter *prm)
{
  domain *cdo=prm->cdo;
  int  j,jx,jy,jz,i,nx=cdo->nx,ny=cdo->ny,nz=cdo->nz,
       mx=cdo->mx,my=cdo->my,mxy=mx*my,stm=cdo->stm;
  type dti = 1.0/cdo->dt;
  type dens_c;
/* Yamashita 2020/06/18 */
  type ep_c;
  type *eps = val->eps;

  switch(flag) {
  case 0:
    if (prm->mpi->rank_x == prm->mpi->npe_x - 1) {
      nx += 1;
    }
    break;
  case 1:
    if (prm->mpi->rank_y == prm->mpi->npe_y - 1) {
      ny += 1;
    }
    break;
  case 2:
    if (prm->mpi->rank_z == prm->mpi->npe_z - 1) {
      nz += 1;
    }
    break;
  default:
    break;
  }

#pragma omp parallel for private(jz,jy,jx,j,i,dens_c,ep_c)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
        i = jx + nx*jy + nx*ny*jz;
        if(flag == 2){
          dens_c = 0.5*(f1[j-mxy]+f1[j]);
          //ep_c = MAX2(eps[j-mxy], eps[j]);
          ep_c = MIN2(eps[j], eps[j-mxy]);
          if (prm->flg->porous == ON) {
            f[i] = - f0[j]*dens_c*dti - ep_c*dens_c*cdo->grav_z;
          } else {
            f[i] = - f0[j]*dens_c*dti - dens_c*cdo->grav_z;
          }
        }else if(flag == 1){
          dens_c = 0.5*(f1[j-mx]+f1[j]);
          //ep_c = MAX2(eps[j-mx], eps[j]);
          ep_c = MIN2(eps[j], eps[j-mx]);
          if (prm->flg->porous == ON) {
            f[i] = - f0[j]*dens_c*dti - ep_c*dens_c*cdo->grav_y;
          } else {
            f[i] = - f0[j]*dens_c*dti - dens_c*cdo->grav_y;
          }
        }else{
          dens_c = 0.5*(f1[j-1]+f1[j]);
          //ep_c = MAX2(eps[j-1], eps[j]);
          ep_c = MIN2(eps[j], eps[j-1]);
          if (prm->flg->porous == ON) {
            f[i] = - f0[j]*dens_c*dti - ep_c*dens_c*cdo->grav_x;
          } else {
            f[i] = - f0[j]*dens_c*dti - dens_c*cdo->grav_x;
          }
        }
      }
    }
  }
}

void zero_clear(type *f, int n)
{
  int j;
#pragma omp parallel for private(j)
  for(j = 0; j < n; j++) f[j] = 0.0;
}
void zero_clear_int(int *f, int n)
{
  int j;
#pragma omp parallel for private(j)
  for(j = 0; j < n; j++) f[j] = 0;
}

type tvd_runge_kutta_3(variable *val, material *mtl, parameter *prm)
{
  domain *cdo = prm->cdo;
  flags *flg = prm->flg;
  type   dt = cdo->dt, dti = 1.0/cdo->dt, time0=cpu_time();
  // memory allocation for work array.
  static int  n1st = 0;
  static type *up,  *vp,  *wp,  *tp, *t, //2020.4.14
              *un,  *vn,  *wn,  *tn,
              *ut,  *vt,  *wt,  *tt;
  //  static type *dfp, *dfn, *dft, *dfsp, *dfsn, *dfst;
  size_t size = sizeof(type)*(cdo->m);

  /* YSE: Number of Components */
  int icompo;
  int NTCompo = cdo->NumberOfComponent;  //< Total Number of components
  int NBCompo = cdo->NBaseComponent;    //< Number of Solid-Liquid components
  int NGCompo = cdo->NGasComponent;     //< Number of Gas only components
  int NSBCompo; //< Number of Solid-Liquid variables to calculate advection
  int NSGCompo; //< Number of Gas variables to calculate advection
  int NCompo;   //< Number of variables to calculate advection
  int NSCompo;  //< Number of sum-of-componets variables to calcutale advection
  static int oldNCompo = -1; //< Saved NCompo

  /* YSE: work arrays for Y, and Vf. */
  static type *sYt, *sYn, *sYp, *sY;
  static type **Yt, **Yn, **Yp, **Ytt, **Y;
  type *Yts, *Ytg, *Vft;
  int j;

  type *fs_ibm = val->fs_ibm;

  /* Yamashita: epsilon (prosity) */
  type *eps = (flg->porous == ON) ? val->eps : NULL;
  type *dens=mtl->dens;
  type *sgm = (flg->porous == ON) ? val->sgm : NULL;

  if (prm->flg->fluid_dynamics == OFF) return 0.0;

  /*
   * YSE: Use condition NCompo < 1 whether advection calculation is
   *      required or not.
   */
  if (flg->solute_diff != ON) {
    NCompo = 0;
    NSBCompo = 0;
    NSGCompo = 0;
    NSCompo = 0;
  } else {
    NCompo = 0;
    NSBCompo = 0;
    NSGCompo = 0;
    NSCompo = 0;
    if (NBCompo > 0) {
      NSBCompo = NBCompo * 2;
      NCompo += NSBCompo;
      if (NBCompo > 1) {
        NCompo += 2;
        NSCompo += 2;
      }
    }
    if (NGCompo > 0) {
      NSGCompo = NGCompo;
      NCompo += NSGCompo;
      if (NGCompo > 1) {
        NCompo += 1;
        NSCompo += 1;
      }
    }
  }
  if (oldNCompo < 0) {
    oldNCompo = NCompo;
  } else {
    CSVASSERT(oldNCompo >= NCompo);
  }

#ifdef AMGS
  int nx = cdo->nx, ny = cdo->ny, nz = cdo->nz, n = cdo->n;
  int mx = cdo->mx, my = cdo->my, mxy = cdo->mxy, m = cdo->m;
  int stm = cdo->stm;
  type dx = cdo->dx, dy = cdo->dy, dz = cdo->dz;
  type dxi2=cdo->dxi*cdo->dxi, dyi2=cdo->dyi*cdo->dyi, dzi2=cdo->dzi*cdo->dzi;
  type bc_xm=flg->bc_xm, bc_xp=flg->bc_xp,
       bc_ym=flg->bc_ym, bc_yp=flg->bc_yp,
       bc_zm=flg->bc_zm, bc_zp=flg->bc_zp;
  mpi_param *mpi = prm->mpi;
  int param[6];
  int mtl_flg, vel_flg;
#endif

  if (n1st++ == 0) {
#ifdef _MEM_ALIGN_
    // time gradient of variables
    ut = (type *) _mm_malloc( size,32 );
    vt = (type *) _mm_malloc( size,32 );
    wt = (type *) _mm_malloc( size,32 );
    tt = (type *) _mm_malloc( size,32 );
    // work arrays
    up = (type *) _mm_malloc( size,32 );
    un = (type *) _mm_malloc( size,32 );
    vp = (type *) _mm_malloc( size,32 );
    vn = (type *) _mm_malloc( size,32 );
    wp = (type *) _mm_malloc( size,32 );
    wn = (type *) _mm_malloc( size,32 );
    tp = (type *) _mm_malloc( size,32 );
    tn = (type *) _mm_malloc( size,32 );
    if (NCompo > 0) {
      sYt = (type *) _mm_malloc( size * NCompo, 32 );
      sYn = (type *) _mm_malloc( size * NCompo, 32 );
      sYp = (type *) _mm_malloc( size * NCompo, 32 );
      Yt = (type **) malloc( sizeof(type *) * NCompo );
      Yn = (type **) malloc( sizeof(type *) * NCompo );
      Yp = (type **) malloc( sizeof(type *) * NCompo );
      Y  = (type **) malloc( sizeof(type *) * NCompo );
    } else {
      sYt = NULL;
      sYn = NULL;
      sYp = NULL;
      Yt = NULL;
      Yn = NULL;
      Yp = NULL;
      Y  = NULL;
    }
    if (NSCompo > 0) {
      sYtt = (type *) _mm_malloc( size * NSCompo, 32 );
    } else {
      sYtt = NULL;
    }
#else
    // time gradient of variables
    ut = (type *) malloc( size );
    vt = (type *) malloc( size );
    wt = (type *) malloc( size );
    tt = (type *) malloc( size );
    // work arrays
    up = (type *) malloc( size );
    un = (type *) malloc( size );
    vp = (type *) malloc( size );
    vn = (type *) malloc( size );
    wp = (type *) malloc( size );
    wn = (type *) malloc( size );
    tp = (type *) malloc( size );
    tn = (type *) malloc( size );
		t  = (type *) malloc( size ); // by Sato
    if (NCompo > 0) {
      sYt = (type *) malloc( size * NCompo );
      sYn = (type *) malloc( size * NCompo );
      sYp = (type *) malloc( size * NCompo );
      sY  = (type *) malloc( size * NCompo );
      Yt = (type **) malloc( sizeof(type *) * NCompo );
      Yn = (type **) malloc( sizeof(type *) * NCompo );
      Yp = (type **) malloc( sizeof(type *) * NCompo );
      Y  = (type **) malloc( sizeof(type *) * NCompo );
    } else {
      sYt = NULL;
      sYn = NULL;
      sYp = NULL;
      Yt = NULL;
      Yn = NULL;
      Yp = NULL;
      Y  = NULL;
    }
#endif
  }

  level_set_all(val, prm); // contain Level_Set, Level_Set_contact and normal_vector_cell

  // zero clear of ft
  zero_clear(ut, cdo->m);
  zero_clear(vt, cdo->m);
  zero_clear(wt, cdo->m);
  zero_clear(tt, cdo->m);

  /* YSE: Init of advection of Y. */
  if (NCompo > 0) {
    zero_clear(sYt, NCompo * cdo->m);

    /* Set storage location
     *
     * Storage order is Y(ls), Y(g), Vf(ls), Ysum(ls), Ysum(g), which
     * might be confusing, but simpler code. Because there are no
     * variable-specific operations here.
     */
    for (icompo = 0; icompo < NCompo; ++icompo) {
      Yt[icompo] = &sYt[icompo * cdo->m];
      Yn[icompo] = &sYn[icompo * cdo->m];
      Yp[icompo] = &sYp[icompo * cdo->m];
      if (icompo >= NBCompo && icompo < NTCompo) {
        /* Gas-only phase does not contain solid */
        Y[icompo] = &val->Y[icompo * cdo->m];
      } else {
        Y[icompo] = &sY[icompo * cdo->m];
      }
    }
    icompo = NSBCompo + NSGCompo;
    if (NBCompo > 1) {
      Yts = Y[icompo++];
    } else {
      Yts = NULL;
    }
    if (NGCompo > 1) {
      Ytg = Y[icompo++];
    } else {
      Ytg = NULL;
    }
    if (NBCompo > 1) {
      Vft = Y[icompo++];
    } else {
      Vft = NULL;
    }

    /* Separate Y into "solid" and "non-solid" */
    if (NBCompo > 0) {
#pragma omp parallel for
      for (j = 0; j < cdo->m; ++j) {
        int ic;
        type y;
        type f;

        for (ic = 0; ic < NBCompo; ++ic) {
          f = clip(1.0 - val->fs[j]);
          y = f * val->Y[ic * cdo->m + j];
          Y[ic][j] = y;
          val->Y[ic * cdo->m + j] -= y;

          y = f * val->Vf[ic * cdo->m + j];
          Y[ic + NTCompo][j] = y;
          val->Vf[ic * cdo->m + j] -= y;
        }
      }
    }

    /* Calculate sum(s) */
    if (Yts || Ytg || Vft) {
#pragma omp parallel for
      for (j = 0; j < cdo->m; ++j) {
        int ic;
        if (Yts) {
          Yts[j] = 0.0;
          for (ic = 0; ic < NBCompo; ++ic) {
            Yts[j] += Y[ic][j];
          }
        }
        if (Ytg) {
          Ytg[j] = 0.0;
          for (ic = NBCompo; ic < NTCompo; ++ic) {
            Ytg[j] += Y[ic][j];
          }
        }
        if (Vft) {
          Vft[j] = 0.0;
          for (ic = 0; ic < NBCompo; ++ic) {
            Vft[j] += Y[ic + NTCompo][j];
          }
        }
      }
    }
  }

  if (flg->heat_eq == ON) {
    //  Separate t into "solid" and "non-solid  12/2019 by Sato

#if 0
#error This block has not been updated when IBM/porous are changed to be mixible, because it has not been tested. Please modify if you want to re-enable this part.
    //--- flux form
#pragma omp parallel for
    for (j = 0; j < cdo->m; ++j) {
      t[j] = 0.0;
      //				if(val->fs[j] < 0.99){
      //					t[j] = val->t[j] * clip(1.0 - val->fs[j]);
      //					val->t[j] -= t[j];
      t[j] = val->t[j];
      val->t[j] -= t[j]*clip(1.0 - val->fs[j]);
      //				}
    }

    // ===== 1st stage =====
    //advection_eq(1, tt, val->t, val->u,val->v,val->w, fs, prm);//fs_sum -> fs, 2017/11/16
    advection_flux(1, tt, t, val->u, val->v, val->w, fs, prm);//fs_sum -> fs, 2017/11/16
    update_tvd3(tp, 1., t, 0., dt, t, tt, cdo); // 12/2019 by Sato
    bct(tp,val,prm);

    // ===== 2nd stage =====
    //advection_eq(1, tt, tp, val->u, val->v, val->w, fs, prm);//fs_sum -> fs, 2017/11/16
    advection_flux(1, tt, tp, val->u, val->v, val->w, fs, prm);//fs_sum -> fs, 2017/11/16
    update_tvd3(tn, 3./4., t, 1./4., 1./4.*dt, tp, tt, cdo); // 12/2019 by Sato
    bct(tn,val,prm);

    // ===== 3rd stage =====
    //advection_eq(1, tt, tn, val->u,val->v,val->w, fs, prm);//fs_sum -> fs, 2017/11/16
    advection_flux(1, tt, tn, val->u,val->v,val->w, fs, prm);//fs_sum -> fs, 2017/11/16
    update_tvd3(t, 1./3., t, 2./3., 2./3.*dt, tn, tt, cdo); // 12/2019 by Sato
    bct(t,val,prm); // 12/2019 by Sato

    //  Re-unite "solid" and "non-solid" t  12/2019 by Sato
#pragma omp parallel for
    for (j = 0; j < cdo->m; ++j) {
      //			if( val->fs[j] < 0.99){
      val->t[j] += t[j]*clip(1.0 - val->fs[j]);
      //			}
    }
    bct(val->t,val,prm);
#else
    {
      type *val_temp; //< Temperature to solve
      type *val_eps;  //< eps/sgm to be used (NULL if porous != ON)
      if (prm->flg->two_energy == ON) {
        CSVASSERT(prm->flg->porous == ON);
        val_temp = val->tf;
        val_eps = eps;
      } else {
        val_temp = val->t;
        val_eps = sgm;
      }
      //--- original
      // ===== 1st stage =====
      advection_eq(1, tt, val_temp, val->u, val->v, val->w, val_eps, fs_ibm,
           val->surface_bnd, prm);
      update_tvd3(tp, 1., val_temp, 0., dt, t, tt, cdo);
      bct(tp, val, prm);
      // ===== 2nd stage =====
      advection_eq(1, tt, tp, val->u, val->v, val->w, val_eps, fs_ibm,
           val->surface_bnd, prm);
      update_tvd3(tn, 3./4., val_temp, 1./4., 1./4.*dt, tp, tt, cdo);
      bct(tn,val,prm);
      // ===== 3rd stage =====
      advection_eq(1, tt, tn, val->u, val->v, val->w, val_eps, fs_ibm,
           val->surface_bnd, prm);
      update_tvd3(val_temp, 1./3., val_temp, 2./3., 2./3.*dt, tn, tt, cdo);
      bct(val_temp, val, prm);
    }
#endif
  }
  // ===== 1st stage =====
  //-- calc df/dt
  advection_eq(2, ut, val->u, val->u, val->v, val->w, eps, NULL,
               val->surface_bnd, prm);
  advection_eq(3, vt, val->v, val->u, val->v, val->w, eps, NULL,
               val->surface_bnd, prm);
  advection_eq(4, wt, val->w, val->u, val->v, val->w, eps, NULL,
               val->surface_bnd, prm);
  /* YSE: calculation of concentration advection */
  for (icompo = 0; icompo < NCompo; ++icompo) {
    advection_eq(1, Yt[icompo], Y[icompo], val->u, val->v, val->w, NULL, fs_ibm,
           val->surface_bnd, prm);
  }

  if (prm->flg->visc_tvd3 == ON)
    visc_eq(0,ut,vt,wt, val->u,val->v,val->w, dens, mtl->mu, val, prm);
  surface_tension_eq(tt,ut,vt,wt, mtl->st, dens, val, prm);
  //-- update
  update_tvd3(up, 1., val->u, 0., dt, val->u, ut, cdo);
  update_tvd3(vp, 1., val->v, 0., dt, val->v, vt, cdo);
  update_tvd3(wp, 1., val->w, 0., dt, val->w, wt, cdo);
  /* YSE: calculation of concentration advection */
  for (icompo = 0; icompo < NCompo; ++icompo) {
    update_tvd3(Yp[icompo], 1., Y[icompo], 0., dt, Y[icompo], Yt[icompo], cdo);
  }

  bcu(up,vp,wp,val,mtl,prm);
  /* YSE: boundary set for concentration */
  if (NCompo > 0) {
    bcs(Yp[0], Yp[NTCompo], val, prm, mtl);
  }
  for (icompo = NSBCompo + NSGCompo; icompo < NCompo; ++icompo) {
    /*
     * YSE: We may need boundary setter for sums of concentrations.
     *      But not supported yet.
     */
    bcf(Yp[icompo], prm);
  }

  // ===== 2nd stage =====
  // calc df/dt
  advection_eq(2, ut, up, up, vp, wp, eps, NULL, val->surface_bnd, prm);
  advection_eq(3, vt, vp, up, vp, wp, eps, NULL, val->surface_bnd, prm);
  advection_eq(4, wt, wp, up, vp, wp, eps, NULL, val->surface_bnd, prm);
  /* YSE: calculation of concentration advection */
  for (icompo = 0; icompo < NCompo; ++icompo) {
    advection_eq(1, Yt[icompo], Yp[icompo], up, vp, wp, NULL, fs_ibm,
           val->surface_bnd, prm);
  }

  if(prm->flg->visc_tvd3 == ON)
    visc_eq(0,ut,vt,wt, up,vp,wp, dens, mtl->mu, val, prm);
  surface_tension_eq(tt,ut,vt,wt, mtl->st, dens, val, prm);
  //-- update
  update_tvd3(un, 3./4., val->u, 1./4.,1./4.*dt, up, ut, cdo);
  update_tvd3(vn, 3./4., val->v, 1./4.,1./4.*dt, vp, vt, cdo);
  update_tvd3(wn, 3./4., val->w, 1./4.,1./4.*dt, wp, wt, cdo);
  /* YSE: calculation of concentration advection. */
  for (icompo = 0; icompo < NCompo; ++icompo) {
    update_tvd3(Yn[icompo], 3./4., Y[icompo], 1./4., 1./4. * dt, Yp[icompo], Yt[icompo], cdo);
  }
  bcu(un,vn,wn,val,mtl,prm);
  /* YSE: boundary set for concentration */
  if (NCompo > 0) {
    bcs(Yn[0], Yn[NTCompo], val, prm, mtl);
  }
  for (icompo = NSBCompo + NSGCompo; icompo < NCompo; ++icompo) {
    /*
     * YSE: We may need boundary setter for sums of concentrations.
     *      But not supported yet.
     */
    bcf(Yn[icompo], prm);
  }

  // ===== 3rd stage =====
  //-- calc df/dt
  advection_eq(2, ut, un, un, vn, wn, eps, NULL, val->surface_bnd, prm);
  advection_eq(3, vt, vn, un, vn, wn, eps, NULL, val->surface_bnd, prm);
  advection_eq(4, wt, wn, un, vn, wn, eps, NULL, val->surface_bnd, prm);
  /* YSE: calculation of concentration advection */
  for (icompo = 0; icompo < NCompo; ++icompo) {
    advection_eq(1, Yt[icompo], Yn[icompo], un, vn, wn, NULL, fs_ibm,
           val->surface_bnd, prm);
  }

  if(prm->flg->visc_tvd3 == ON)
    visc_eq(0,ut,vt,wt, un,vn,wn, dens, mtl->mu, val, prm);
  surface_tension_eq(tt,ut,vt,wt, mtl->st, dens, val, prm);
  //-- update
  update_tvd3(val->u, 1./3., val->u, 2./3.,2./3.*dt, un, ut, cdo);
  update_tvd3(val->v, 1./3., val->v, 2./3.,2./3.*dt, vn, vt, cdo);
  update_tvd3(val->w, 1./3., val->w, 2./3.,2./3.*dt, wn, wt, cdo);
  /* YSE: calculation of concentration advection. */
  for (icompo = 0; icompo < NCompo; ++icompo) {
    update_tvd3(Y[icompo], 1./3., Y[icompo], 2./3., 2./3.*dt, Yn[icompo], Yt[icompo], cdo);
  }

  /* Substep time integration for Forchheimer term, Susumu 2020/6/18 (tentative) */
  if (prm->flg->porous == ON) {
    int   itr = 0, itr_max = cdo->nsub_step_mu;
    type  mdt = cdo->dt / ((type) cdo->nsub_step_mu);
    while (itr++ < itr_max) {
      Forchheimer(mdt, val->u, val->v, val->w, val, mtl, prm);
      bcu(val->u,val->v,val->w,val,mtl,prm);
    }
  }

  bcu(val->u,val->v,val->w,val,mtl,prm);

  /* YSE: boundary set for concentration */
  if (flg->solute_diff == ON) {
    bcs(Y[0], Y[NTCompo], val, prm, mtl);
    if (NGCompo > 0) {
      bcs(val->Y, NULL, val, prm, mtl);
    }
  }
  if (NCompo > NTCompo) {
    for (icompo = NTCompo; icompo < NCompo; ++icompo) {
      /*
       * YSE: We may need boundary setter for sums of concentrations.
       *      But not supported yet.
       */
      bcf(Y[icompo], prm);
    }
  }

  if (flg->solute_diff == ON) {
    /* YSE: Redistribute Y from sums. */
    if (Yts || Ytg || Vft) {
      int ics;
      int ice;
      ics = 0;
      ice = 0;
      if (Yts) { ice = NBCompo; }
      if (Ytg) { ice = NTCompo; if (!Yts) ics = NBCompo; }
      if (Vft) {
        ice = NTCompo + NBCompo;
        if (Ytg) {
          ics = NBCompo;
        } else {
          if (!Yts) {
            ics = NTCompo;
          }
        }
      }
      for(icompo = ics; icompo < ice; icompo++) {
        int is, ie;
        type *Yt;
        int m = cdo->m;
        if (icompo < NBCompo) {
          is = 0;
          ie = NBCompo;
          Yt = Yts;
        } else if (icompo < NTCompo) {
          is = NBCompo;
          ie = NTCompo;
          Yt = Ytg;
        } else {
          is = NTCompo;
          ie = NTCompo + NBCompo;
          Yt = Vft;
        }
        if (!Yt) { /* Yt exists only if it requires redistribution */
          continue;
        }

#pragma omp parallel for
        for(j = 0; j < m; j++) {
          type valf;
          int ic;

          valf = 0.0;
          for(ic = is; ic < ie; ic++){
            if(ic == icompo) {
              continue;
            }else{
              valf += Y[ic][j];
            }
          }
          Y[icompo][j] = Yt[j] - valf;
        }
      }
    }

    /* Re-unite "solid" and "non-solid" Y */
    if (NBCompo > 0) {
#pragma omp parallel for
      for (j = 0; j < cdo->m; j++) {
        int ic;
        type valf;
        type y, v;

        valf = 0.0;
        for (ic = 0; ic < NBCompo; ++ic) {
          val->Y[ic * cdo->m + j] += Y[ic][j];
          valf += val->Y[ic * cdo->m + j];

          val->Vf[ic * cdo->m + j] += Y[ic + NTCompo][j];
        }
        val->Yt[j] = valf;
      }
    }

    bcs(val->Y, val->Vf, val, prm, mtl);
  }

  if(prm->flg->visc_tvd3 == OFF) {
    /*      Implicit method         */
    //-- U
    set_b(0,mtl->div_b, val->u, dens, mtl->mu, val, prm);
#ifdef _TIME_
    it_U += ccse_poisson(0, val->u, mtl->div_b, dens, mtl, val, prm);
#else
    ccse_poisson(0, val->u, mtl->div_b, dens, mtl, val, prm);
#endif

    //-- V
    set_b(1, mtl->div_b, val->v, dens, mtl->mu, val, prm);
#ifdef _TIME_
    it_V += ccse_poisson(1, val->v, mtl->div_b, dens, mtl, val, prm);
#else
    ccse_poisson(1, val->v, mtl->div_b, dens, mtl, val, prm);
#endif

    //-- W
    set_b(2,mtl->div_b, val->w, dens, mtl->mu, val, prm);
#ifdef _TIME_
    it_W += ccse_poisson(2, val->w, mtl->div_b, dens, mtl, val, prm);
#else
    ccse_poisson(2, val->w, mtl->div_b, dens, mtl, val, prm);
#endif

    bcu(val->u,val->v,val->w,val,mtl,prm);
  }
  return cpu_time() - time0;
}
