#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include "struct.h"
#include "func.h"

type eutectic(variable *val, material *mtl, parameter *prm)
{
  flags  *flg=prm->flg;
  type   time0 = cpu_time();
  if(flg->eutectic != ON) return cpu_time() - time0;
  domain      *cdo=prm->cdo;
  phase_value *phv=prm->phv;
  int icompo, pare;
  int j,jx,jy,jz, nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
      m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my, stm=cdo->stm;
  type dx=cdo->dx, dy=cdo->dy, dz=cdo->dz, dt=cdo->dt;
  type *ls_n=val->ls_n, *ll_n=val->ll_n, *ll=val->ll, *ls=val->ls;
  type *t=val->t, *fs=val->fs, *fl=val->fl, *fls=val->fls, *fs_sum=val->fs_sum, *fl_sum=val->fl_sum;
  type vall, delta;
  type *tm_liq = phv->tm_liq;
  type dg[cdo->NumberOfComponent];
  type *df=val->df, *dfs=val->dfs;
  type *work=val->work;
  type C_fuel, C_CR;

  delta=1.3*dx;
  //--- make level-set for each element (2015/09/09)
  for(icompo = 0; icompo < cdo->NumberOfComponent; icompo++){
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
        for(jx = 0; jx < mx; jx++) {
          j = jx + mx*jy + mxy*jz;
          val->work[j] = clip(fs[j+icompo*m]);
        }
      }
    }
    bcf(work,prm);
    Level_Set(1, 20, &ls_n[icompo*m], work, prm);
    //Level_Set(1, 20, &ls_n[icompo*m], &fs[icompo*m], prm);
    //Level_Set(1, 30, &val->ll_n[icompo*m], &val->fl[icompo*m], prm);
  }
	//bcf(ll_n, prm);
	bcf_VOF(0,ls_n, prm);

	zero_clear(val->fl_sum, cdo->m);
  for(icompo = 0; icompo < cdo->NumberOfComponent; icompo++) {
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
        for(jx = 0; jx < mx; jx++) {
          j = jx + mx*jy + mxy*jz;
          fl_sum[j] += clip(fl[j+icompo*m]);
        }
      }
    }
  }
  Level_Set(1, 20, ll, val->fl_sum, prm);
	bcf(ll, prm);

/*
  0 : UO2
  1 : SUS
  2 : Zry
  3 : B4C
  4 : UO2-Zry (0-2)
  5 : SUS-B4C (1-3)
*/

  type d2 = dx*dx, d3 = d2*dx;
  type v_FCC = 9.1e-6, //[m/s]
       v_B4C = 5.7e-6; //[m/s]
  C_fuel = 0.0001; C_CR = 0.0001;
  //C_fuel = 1.0; C_CR = 1.0;
  //--- Solid - Solid case
  for(icompo = 0; icompo < cdo->NumberOfComponent-1; icompo++) {
    for(pare = icompo+1; pare < cdo->NumberOfComponent; pare++) {
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            j = jx + mx*jy + mxy*jz;
            //--------- Left side from the interface
            if( (ls_n[j+m*icompo] <=  delta && ls_n[j+m*icompo] > 0.0) && 
                (ls_n[j+m*pare  ] >= -delta && ls_n[j+m*pare  ] < 0.0)) 
            {
              //------- UO2 - Zry ----------------
              if(icompo == 0 && pare == 2 && t[j] >= tm_liq[4] && fl[j+4*m] < 1.0) 
              {
                dg[4] += C_fuel; 
                df[j+4*m] += dg[4];
                if(fabs(df[j+4*m]) > 1.0) {
                  df[j+4*m] = 0.0;
                  fl[j+4*m] = 1.0;
                  fs[j] = 0.0;
                  fs[j+2*m] = 0.0;
                }
              }
              //------- SUS - B4C ----------------
              if(icompo == 1 && pare == 3 && t[j] >= tm_liq[5] && fl[j+5*m] < 1.0) 
              {
                dg[5] += dt*v_FCC*d2/d3;
                df[j+5*m] += dg[5];
                if(fabs(df[j+5*m]) > 1.0) {
                  df[j+5*m] = 0.0;
                  fl[j+5*m] = 1.0;
                  fs[j+m] = 0.0;
                  fs[j+3*m] = 0.0;
                }
              }
            }
            //--------- Right side from the interface
            if( (ls_n[j+m*icompo] >= -delta && ls_n[j+m*icompo] < 0.0) && 
                (ls_n[j+m*pare  ] <=  delta && ls_n[j+m*pare  ] > 0.0)) 
            {
              //------- UO2 - Zry ----------------
              if(icompo == 0 && pare == 2 && t[j] >= tm_liq[4] && fl[j+4*m] < 1.0) 
              {
                dg[4] += C_fuel; 
                df[j+4*m] += dg[4];
                if(fabs(df[j+4*m]) > 1.0) {
                  df[j+4*m] = 0.0;
                  fl[j+4*m] = 1.0;
                  fs[j] = 0.0;
                  fs[j+2*m] = 0.0;
                }
              }
              //------- SUS - B4C ----------------
              if(icompo == 1 && pare == 3 && t[j] >= tm_liq[5] && fl[j+5*m] < 1.0) 
              {
                dg[5] += dt*v_B4C*d2/d3;
                df[j+5*m] += dg[5];
                if(fabs(df[j+5*m]) > 1.0) {
                  df[j+5*m] = 0.0;
                  fl[j+5*m] = 1.0;
                  fs[j+m] = 0.0;
                  fs[j+3*m] = 0.0;
                }
              }
            }
          }
        }
      }
    }
  }
/*
  0 : UO2
  1 : SUS
  2 : Zry
  3 : B4C
  4 : UO2-Zry (0-2)
  5 : SUS-B4C (1-3)
*/
  //--- Solid - liquid case
  for(pare = 0; pare < cdo->NumberOfComponent; pare++) {
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
        for(jx = 0; jx < mx; jx++) {
          j = jx + mx*jy + mxy*jz;

          if( (ll[j] >= -delta && ll[j] < 0.0) && 
              (ls_n[j+m*pare] <= delta && ls_n[j+m*pare] > 0.0 )) 
          {
            //------- UO2 - Zry ----------------
            if(pare == 0 || pare == 2) 
            {
              if(t[j] >= tm_liq[4] && fl[j+4*m] < 1.0)
              {
                dg[4] += C_fuel; 
                df[j+4*m] += dg[4];
                if(fabs(df[j+4*m]) > 1.0) {
                  df[j+4*m] = 0.0;
                  fl[j+4*m] = 1.0;
                  fs[j] = 0.0;
                  fs[j+2*m] = 0.0;
                }
              }
            }
            //------- SUS - B4C ----------------
            if(pare == 1) 
            {
              if(t[j] >= tm_liq[5] && fl[j+5*m] < 1.0)
              {
                dg[5] += dt*v_FCC*d2/d3;
                df[j+5*m] += dg[5];
                if(fabs(df[j+5*m]) > 1.0) {
                  df[j+5*m] = 0.0;
                  fl[j+5*m] = 1.0;
                  fs[j+m] = 0.0;
                  fs[j+3*m] = 0.0;
                }
              }
            } else if (pare == 3) {
              if(t[j] >= tm_liq[5] && fl[j+5*m] < 1.0)
              {
                dg[5] += dt*v_B4C*d2/d3;
                df[j+5*m] += dg[5];
                if(fabs(df[j+5*m]) > 1.0) {
                  df[j+5*m] = 0.0;
                  fl[j+5*m] = 1.0;
                  fs[j+m] = 0.0;
                  fs[j+3*m] = 0.0;
                }
              }
            }
          }
        }
      }
    }
  }


	bcf_VOF(0,fs, prm);
	bcf_VOF(0,fl, prm);
	return  cpu_time() - time0;
}
