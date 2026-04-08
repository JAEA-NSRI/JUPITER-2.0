#include "geometry/list.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <stdarg.h>
#ifdef GPU
  #include <cuda.h>
  #include <cutil.h>
#endif
#include "csvutil.h"
#include "struct.h"
#include "func.h"
#include "common_util.h"
#include "component_data_defs.h"
#include "os/os.h"

/* YSE: Added for temperature dependent property */
#include "tempdep_calc.h"

#define SIGN(dg, x, dt)         \
{                               \
  if(dt < 0.0) {dg = -x;}       \
  else {dg = x;}                \
}

/*added by Chai*/
void cp_update(domain *cdo, flags *flg, phase_value *phv, type *fl, type *fs, int *mushy, type *specht, type *latent, type *t_liq, type *t_soli, type *temp)
{
  int jx, jy, jz;
  int icompo, phase_changing_comp;
  int NCompo;
  NCompo = cdo->NBaseComponent;

  if (!mushy) return;

  if(flg->solute_diff == ON) {
#pragma omp parallel for collapse(3)
    for (jz = 0; jz < cdo->nz; jz++) {
      for (jy = 0; jy < cdo->ny; jy++) {
        for (jx = 0; jx < cdo->nx; jx++) {
          ptrdiff_t j;
          j = calc_address(jx + cdo->stm, jy + cdo->stm, jz + cdo->stm,
                           cdo->mx, cdo->my, cdo->mz);

          if (mushy[j] < 0) continue;

          specht[j] = latent[j] / (t_liq[j] - t_soli[j]);
        }
      }
    }
  } else {
    /*
     * phase change model for solute_diff == OFF has been reverted at cf9450e74
     * (2020/09/29), that commit makes this block unreachable.
     */
    CSVASSERT_X(0, "Unreachable");
#pragma omp parallel for collapse(3)
    for (jz = 0; jz < cdo->nz; jz++) {
      for (jy = 0; jy < cdo->ny; jy++) {
        for (jx = 0; jx < cdo->nx; jx++) {
          type t_liq_comp, t_soli_comp;
          type lh;
          type fls_temp=0.0;
          type specht_temp = 0.0;
          type specht_mushy= 0.0;
          type fs_tmp[NCompo], fl_tmp[NCompo];
          type s_tmp[NCompo], l_tmp[NCompo];
          int m = cdo->m;
          ptrdiff_t j;

          j = calc_address(jx + cdo->stm, jy + cdo->stm, jz + cdo->stm,
                           cdo->mx, cdo->my, cdo->mz);

          if (mushy[j] < 0) continue;

          phase_changing_comp = mushy[j];
          t_liq_comp=phv->comps[phase_changing_comp].tm_soli;
          t_soli_comp=phv->comps[phase_changing_comp].tm_liq;
          lh = phv->comps[phase_changing_comp].lh;

          for(icompo= 0; icompo < NCompo; icompo++) {
            if (icompo == phase_changing_comp) continue;
            fs_tmp[icompo] = clip(fs[j+icompo*m]);
            fl_tmp[icompo] = clip(fl[j+icompo*m]);
            s_tmp[icompo] = tempdep_calc(&phv->comps[icompo].specht_s, temp[j]);
            l_tmp[icompo] = tempdep_calc(&phv->comps[icompo].specht_l, temp[j]);
            specht_temp += fs_tmp[icompo]*s_tmp[icompo]+fl_tmp[icompo]*l_tmp[icompo];
            fls_temp += fl_tmp[icompo] + fs_tmp[icompo];
          }
          specht_mushy = lh / (t_liq_comp - t_soli_comp);
          specht[j] = specht_temp + (1.0-fls_temp)*specht_mushy;
        }
      }
    }
  }
}

int t_update(type *t, type delt_t, type t_liq, type t_soli, type entha, type specht, type latent)
{
  /* specht can be zero for very low fl */
  if (specht <= 0.0) return 1;
  if (delt_t < 0.0) {
    *t = t_soli - (entha - latent)/specht;
  } else {
    *t = t_liq + (entha - latent)/specht;
  }
  return 0;
}

int fsfl_update(type *fl, type *fs, type delt_t, type latent, type specht)
{
  type df;
  int ret;

  /* latent can be zero for very low fl */
  if (latent <= 0.0) return 1;

  ret = 0;
  df = specht*delt_t;
  if (fabs(df) > latent) {
    ret = 2;
  }
  df = df / latent;
  *fl=*fl+df;
  *fs=*fs-df;

  *fl=MIN2(1.0, *fl);
  *fl=MAX2(0.0, *fl);
  *fs=MIN2(1.0, *fs);
  *fs=MAX2(0.0, *fs);

  return ret;
}

/* Thread-safe single-time warning writer */
static void
tsafe_warn(int *wflag, int flag,
           const char *file, long line, csv_error_level errlevel,
           const char *msg, ...);

static void
t_update_warn(int ret, int jx, int jy, int jz, const char *f, long line,
              mpi_param *mpi, domain *cdo);

static void
fsfl_update_warn(int ret, int jx, int jy, int jz, const char *f, long line,
                 mpi_param *mpi, domain *cdo);

type phase_change(variable *val, material *mtl, parameter *prm)
{
  flags       *flg=prm->flg;
  domain      *cdo=prm->cdo;
  phase_value *phv=prm->phv;
  type   time0 = cpu_time();
  if(flg->phase_change != ON) return cpu_time() - time0;
  int jx,jy,jz, nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
      m=cdo->m, mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  int *mushy=val->mushy;
  type *fs=val->fs, *fl=val->fl, *fls=val->fls;
  type *t_pre=val->t_pre,  *entha=val->entha, *specht=mtl->specht;
  type *t_liq=mtl->t_liq, *t_soli=mtl->t_soli, *latent=mtl->latent;
  type *ox_vof = val->ox_vof;
  type *df = val->df, *dfs = val->dfs;

  int icompo,j;
  type delt[cdo->NBaseComponent], dg[cdo->NBaseComponent];
  type *fs_sum=val->fs_sum, dTo;
  type *ts;
  type *tf;

  if (flg->two_energy == ON) {
    ts = val->ts;
    tf = val->tf;
  } else {
    ts = val->t;
    tf = val->t;
  }

  if (flg->solute_diff == ON){
    type *t = ts;
    CSVASSERT(ts == tf); /* not supported */
#pragma omp parallel for collapse(3)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        for(jx = 0; jx < nx; jx++) {
          type delt_t;
          int j;
          int r;
          j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
          mushy[j] = -1;
          if(fls[j]==0.0) continue;
          if((t[j]>t_soli[j]&&fs[j]>0)||(t[j]<t_liq[j]&&fl[j]>0)) {
            delt_t = t[j]-t_pre[j];
            entha[j] += specht[j]*fabs(delt_t);
            if (t_liq[j] == t_soli[j]) {
              if (entha[j] < latent[j]) {
                t[j] = t_pre[j];
              } else {
                r = t_update(&t[j], delt_t, t_liq[j], t_soli[j], entha[j], specht[j], latent[j]);
                t_update_warn(r, jx, jy, jz, __FILE__, __LINE__, prm->mpi, cdo);
              }
              r = fsfl_update(&fl[j],&fs[j],delt_t, latent[j], specht[j]);
              fsfl_update_warn(r, jx, jy, jz, __FILE__, __LINE__, prm->mpi, cdo);
            } else {
               if (entha[j] < latent[j]) {
                 mushy[j] = 0;
               } else {
                 r = t_update(&t[j], delt_t, t_liq[j], t_soli[j], entha[j], specht[j], latent[j]);
                 t_update_warn(r, jx, jy, jz, __FILE__, __LINE__, prm->mpi, cdo);
               }
               r = fsfl_update(&fl[j],&fs[j],delt_t, latent[j], specht[j]);
               fsfl_update_warn(r, jx, jy, jz, __FILE__, __LINE__, prm->mpi, cdo);
             }
          }
          t_pre[j]=t[j];
        }
      }
    }
  } else {
#if 0
#error This section has not been updated after IBM and porous has been able to mix. Please update before use.
#pragma omp parallel for collapse(3)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        for(jx = 0; jx < nx; jx++) {
          int phase_changing;
          int phase_changing_comp;
          int icompo;
          type tm_soli, tm_liq;
          type delt;
          type specht_phase_change, lh;
          int j;
          int r;

          j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

          mushy[j] = -1;
          if(fls[j]==0.0) continue;

          phase_changing = 0;
          phase_changing_comp = 0;

          for(icompo = 0; icompo < cdo->NBaseComponent; icompo++) {
            if((fs[j+icompo*m] > 0.0 && phv->comps[icompo].tm_soli > t[j]) || (fl[j+icompo*m] > 0.0 && phv->comps[icompo].tm_liq < t[j])) continue;
            phase_changing = 1;
            phase_changing_comp = icompo;
            break;
          }

          if (!phase_changing) continue;

          tm_soli = phv->comps[phase_changing_comp].tm_soli;
          tm_liq  = phv->comps[phase_changing_comp].tm_liq;
          lh = phv->comps[phase_changing_comp].lh;

          delt = t[j] - t_pre[j];
          entha[j] += specht[j] * fabs(delt);

          if (t_liq[j] == t_soli[j]) {
            if (entha[j] < lh) {
              t[j] = t_pre[j];
            } else {
              r = t_update(&t[j], delt, tm_liq, tm_soli, entha[j], specht[j], lh);
              t_update_warn(r, jx, jy, jz, __FILE__, __LINE__, prm->mpi, cdo);
            }
            r = fsfl_update(&fl[j+m*phase_changing_comp],&fs[j+m*phase_changing_comp], delt, lh, specht[j]);
            fsfl_update_warn(r, jx, jy, jz, __FILE__, __LINE__, prm->mpi, cdo);
          } else {
            if (entha[j] < lh) {
              mushy[j] = phase_changing_comp;
            } else {
              r = t_update(&t[j], delt, tm_liq, tm_soli, entha[j], specht[j], lh);
              t_update_warn(r, jx, jy, jz, __FILE__, __LINE__, prm->mpi, cdo);
            }
            r = fsfl_update(&fl[j+m*phase_changing_comp],&fs[j+m*phase_changing_comp], delt, lh, specht[j]);
            fsfl_update_warn(r, jx, jy, jz, __FILE__, __LINE__, prm->mpi, cdo);
          }
          t_pre[j]=t[j];
          /*
          if (t[j]<t_liq[j]&&fl[j]>0.0){
            fl[j]=0.0;
            fs[j]=1.0;
          }*/
        }
      }
    }
#else
    //--- Melting
    if(flg->melting == ON) {
      struct geom_list *lp, *lh;
      lh = &prm->comps_data_head.list;
      geom_list_foreach(lp, lh) {
        component_data *comp_data;
        phase_value_component *comp;
        int icompo;

        comp_data = component_data_entry(lp);
        if (comp_data->phase_comps_index < 0)
          continue;

        comp = &phv->comps[comp_data->phase_comps_index];
        icompo = comp_data->comp_index;
        if (icompo < 0)
          continue;

        if (comp->sform == SOLID_FORM_UNUSED)
          continue;

        for(jz = 0; jz < nz; jz++) {
          for(jy = 0; jy < ny; jy++) {
            for(jx = 0; jx < nx; jx++) {
              j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
              //delt[icompo] = phv->tm_liq[icompo] - t[j];
              delt[icompo] = comp->tm_liq - ts[j];

              if(fs[j+icompo*m] > 0.0 && delt[icompo] < 0.0 && fs_sum[j]-fs[j+icompo*m] < 0.01) {
                //dg[icompo] = phv->specht_l[icompo]*delt[icompo]/phv->lh[icompo];
                dg[icompo] = tempdep_calc(&comp->specht_l, ts[j])*delt[icompo]/comp->lh;
                if(fabs(dg[icompo]) >= 1.0) {
                  //dTo = prm->phv->lh[icompo]*(1.0-dg[icompo])/prm->phv->specht_l[icompo];
                  dTo = comp->lh*(1.0-dg[icompo])/tempdep_calc(&comp->specht_l, ts[j]);
                  SIGN(dg[icompo], 1.0, delt[icompo]);
                }
                df[j+icompo*m] += dg[icompo];
                
                if(fabs(df[j+icompo*m]) >= 1.0) {
                  fl[j+icompo*m] = fl[j+icompo*m] - df[j+icompo*m];
                  fs[j+icompo*m] = fs[j+icompo*m] + df[j+icompo*m];
                  df[j+icompo*m] = 0.0;
                  //t[j] = phv->tm_liq[icompo] + fabs(dTo);
                  ts[j] = comp->tm_liq + fabs(dTo);
                  if (tf != ts)
                    tf[j] = ts[j];
                }
                
                fl[j+icompo*m] = fl[j+icompo*m] - dg[icompo];
                fs[j+icompo*m] = fs[j+icompo*m] + dg[icompo];
                if(fl[j+icompo*m] > 1.0) {
                  fl[j+icompo*m] = 1.0;
                  fs[j+icompo*m] = 0.0;
                  df[j+icompo*m] = 0.0;
                  //val->flg_obst_A[j] = 0.0;
                }
              }
            }
          }
        }
      }
    }
    //--- Solidification
    if(flg->solidification == ON) {
      struct geom_list *lp, *lh;
      lh = &prm->comps_data_head.list;
      geom_list_foreach(lp, lh) {
        component_data *comp_data;
        phase_value_component *comp;
        int icompo;

        comp_data = component_data_entry(lp);
        if (comp_data->phase_comps_index < 0)
          continue;

        comp = &phv->comps[comp_data->phase_comps_index];
        icompo = comp_data->comp_index;
        if (icompo < 0)
          continue;

        if (comp->sform == SOLID_FORM_UNUSED)
          continue;

        dTo = 0.0;
        for(jz = 0; jz < nz; jz++) {
          for(jy = 0; jy < ny; jy++) {
            for(jx = 0; jx < nx; jx++) {
              //zc = 0.5*(cdo->z[jz+stm] + cdo->z[jz+1+stm]);
              j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
              //delt[icompo] = phv->tm_soli[icompo] - t[j];
              delt[icompo] = comp->tm_soli - tf[j];

//              if(fl[j+icompo*m] > 0.5 && delt[icompo] > 0.0 && (1.0-fls[j]) < 0.01 
//                  && fs_sum[j]-fs[j+icompo*m] < 0.01) {
              if(fl[j+icompo*m] > 0.5 && delt[icompo] > 0.0 && fs_sum[j]-fs[j+icompo*m] < 0.01) {
                //dg[icompo] = phv->specht_l[icompo]*delt[icompo]/phv->lh[icompo];
                dg[icompo] = tempdep_calc(&comp->specht_l, tf[j])*delt[icompo]/comp->lh;
                if(fabs(dg[icompo]) >= 1.0) {
                  //dTo = prm->phv->lh[icompo]*(1.0-dg[icompo])/prm->phv->specht_l[icompo];
                  dTo = comp->lh*(1.0-dg[icompo])/tempdep_calc(&comp->specht_l, tf[j]);
                  SIGN(dg[icompo], 1.0, delt[icompo]);
                }
                dfs[j+icompo*m] += dg[icompo];
                
                if(fabs(dfs[j+icompo*m]) >= 1.0) {
                  fs[j+icompo*m] = fs[j+icompo*m] + dfs[j+icompo*m];
                  fl[j+icompo*m] = fl[j+icompo*m] - dfs[j+icompo*m];
                  dfs[j+icompo*m]  = 0.0;
                  //t[j] = phv->tm_liq[icompo];// + fabs(dTo);
                  ts[j] = comp->tm_liq + fabs(dTo);
                  if (ts != tf)
                    tf[j] = ts[j];
                }
               
                fl[j+icompo*m] = fl[j+icompo*m] - dg[icompo];
                fs[j+icompo*m] = fs[j+icompo*m] + dg[icompo];
                if(fs[j+icompo*m] > 1.0) {
                  fl[j+icompo*m] = 0.0;
                  fs[j+icompo*m] = 1.0;
                  dfs[j+icompo*m] = 0.0;
                }
              }
            }
          }
        }
      }
    }
#endif
  }

  if (flg->vaporization == ON || flg->condensation == ON) {
#pragma omp parallel for collapse(3)
    for (jz = 0; jz < nz; jz++) {
      for (jy = 0; jy < ny; jy++) {
        for (jx = 0; jx < nx; jx++) {
          component_data *d;
          type delt, dgv, dgc, dTov, dToc, flj, flj1, fgj, fsj, lhv;
          int icompo, ipcompo, j;

          d = cdo->vap_cond_liquid_id.d;
          icompo = d->comp_index;
          ipcompo = d->phase_comps_index;

          lhv = phv->comps[ipcompo].lhv;

          j = (jx + stm) + mx * (jy + stm) + mxy * (jz + stm);
          delt = phv->comps[ipcompo].tb - tf[j];
          if (flg->solute_diff == ON) {
            flj = clip(fl[j] * val->Y[j + icompo * m]);
            fsj = fs[j];
          } else {
            flj = fl[j + icompo * m];
            fsj = fs_sum[j];
          }
          fgj = 1.0 - fls[j];

          dgv = 0.0;
          dgc = 0.0;
          dToc = 0.0;
          dTov = 0.0;

          if (flg->vaporization == ON && flj > 0.0 && delt < 0.0 && fsj < 0.01) {
            // dg[icompo] = phv->specht_l[icompo]*delt[icompo]/phv->lh[icompo];
            dgv = tempdep_calc(&phv->specht_g, tf[j]) * delt / lhv;
            if (fabs(dgv) >= 1.0) {
              dTov = lhv * (1.0 - dgv) / tempdep_calc(&phv->specht_g, tf[j]);
              SIGN(dgv, 1.0, delt);
            }
          }
          if (flg->condensation == ON && fgj > 0.01 && delt > 0.0 && fsj < 0.01) {
            // dg[icompo] = phv->specht_l[icompo]*delt[icompo]/phv->lh[icompo];
            dgc = tempdep_calc(&phv->specht_g, tf[j]) * delt / lhv;
            if (fabs(dgc) >= 1.0) {
              dToc = lhv * (1.0 - dgc) / tempdep_calc(&phv->specht_g, tf[j]);
              SIGN(dgc, 1.0, delt);
            }
          }
          //Y flj1 = flj + dgc + dgv;
          flj1 = clip(flj + dgc + dgv);

          if (flj > 0.0 && flj1 <= 0.0) {
            tf[j] = phv->comps[ipcompo].tb + fabs(dTov);
          }
          if (flj < 1.0 && flj1 >= 1.0) {
            tf[j] = phv->comps[ipcompo].tb + fabs(dToc);
          }

          if (flg->solute_diff == ON) {
            fl[j] = flj1;
            val->fl_sum[j] = flj1;
          } else {
            fl[j + icompo * m] = flj1;
            val->fl_sum[j] += flj1 - flj;
          }
          /* fls[j] = val->fl_sum[j] + val->fs_sum[j]; */
        }
      }
    }
  }

  /* YSE: Removed original implementation of oxidation. */
  /* Update ox_lset by change of ox_lset */
  if (ox_vof) {
    bcf(ox_vof, prm);

    Level_Set(1, 20, val->ox_lset, ox_vof, prm);
    bcf(val->ox_lset, prm);
  }

  bcf_VOF(0, fs, val, prm);
  bcf_VOF(1, fl, val, prm);
  
  bct(ts,val,prm); //Y
  if (ts != tf)
    bct(tf,val,prm);

  return cpu_time() - time0;
}

int expand_mushy_array(int imushy, int *input_address, int *input_comp,
                       int *output, int m)
{
#pragma omp parallel
  {
    int j;

#pragma omp for
    for (j = 0; j < m; ++j) {
      output[j] = -1;
    }

    if (imushy > 0) {
#pragma omp for
      for (j = 0; j < imushy; ++j) {
        int jj;
        jj = input_address[j];
        if (input_comp) {
          output[jj] = input_comp[j];
        } else {
          output[jj] = 0;
        }
      }
    }
  }
  return 0;
}

int compress_mushy_array(int *input, int m,
                         int *output_address, int *output_comp, int *imushy)
{
  int iimushy;
  int j;

  iimushy = 0;

#ifdef _OPENMP
#pragma omp parallel
  {
    int j;
    int lmushy;
    int smushy;

    lmushy = 0;

#pragma omp for
    for (j = 0; j < m; ++j) {
      if (input[j] >= 0) {
        lmushy++;
      }
    }

#pragma omp critical
    {
      smushy = iimushy;
      iimushy = smushy + lmushy;
    }

#pragma omp for
    for (j = 0; j < m; ++j) {
      if (input[j] > 0) {
        output_address[smushy] = j;
        if (output_comp) {
          output_comp[smushy] = input[j];
        }
        smushy++;
      }
    }
  }

#else
  for (j = 0; j < m; ++j) {
    if (input[j] > 0) {
      output_address[iimushy] = j;
      if (output_comp) {
        output_comp[iimushy] = input[j] - 1;
      }
      iimushy++;
    }
  }
#endif

  *imushy = iimushy;
  return 0;
}

static void
tsafe_warn(int *wflag, int flag,
           const char *file, long line, csv_error_level errlevel,
           const char *msg, ...)
{
  int w;
  va_list ap;
  va_start(ap, msg);

#pragma omp atomic read
  w = *wflag;
  if ((w & flag) == 0) {
#pragma omp critical
    {
      if ((*wflag & flag) == 0) {
        csvperrorv(file, line, 0, errlevel, NULL, msg, ap);
        *wflag |= flag;
      }
    }
  }

  va_end(ap);
}

static void
t_update_warn(int ret, int jx, int jy, int jz, const char *f, long line,
              mpi_param *mpi, domain *cdo)
{
  static int t_update_warn_s = 0;
  if (ret) {
    tsafe_warn(&t_update_warn_s, ret, __FILE__, __LINE__,
               CSV_EL_WARN,
               "Specific heat is zero at cell (%d, %d, %d) (This message only shows for the first time)",
               jx + mpi->rank_x * cdo->nx,
               jy + mpi->rank_y * cdo->ny,
               jz + mpi->rank_z * cdo->nz);
  }
}

static void
fsfl_update_warn(int ret, int jx, int jy, int jz, const char *f, long line,
                 mpi_param *mpi, domain *cdo)
{
  static int fsfl_update_warn_s = 0;
  if (ret) {
    const char *msg;
    switch(ret) {
    case 1:
      msg = "Latent heat is zero";
      break;
    case 2:
      msg = "Instant phase change occured (|df| > 1)";
      break;
    default:
      msg = "Unknown fsfl_update error";
      break;
    }
    tsafe_warn(&fsfl_update_warn_s, ret, __FILE__, __LINE__,
               CSV_EL_WARN,
               "%s at cell (%d, %d, %d) (This message only shows for the first time)",
               msg,
               jx + mpi->rank_x * cdo->nx,
               jy + mpi->rank_y * cdo->ny,
               jz + mpi->rank_z * cdo->nz);
  }
}
