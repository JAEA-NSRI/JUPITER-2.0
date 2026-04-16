#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#ifdef GPU
#  include <cuda.h>
#  include <cutil.h>
#endif

#include "common_util.h"
#include "component_data.h"
#include "component_info.h"
#include "csv.h"
#include "csvutil.h"
#include "func.h"
#include "materials.h"
#include "physical_properties.h"
#include "struct.h"
#include "os/os.h"

#include "tmcalc.h"
#include "dccalc.h"

/* YSE: Added for temperature dependent property */
#include "tempdep_calc.h"

#ifdef _TIME_
extern type time_materials;
#endif

#define PHV1(val, fs_U, fs_Z, fl_U, fl_Z, fg, ms_U, ms_Z, ml_U, ml_Z, mg) \
  {                                                                     \
    val = ms_U*fs_U + ms_Z*fs_Z + ml_U*fl_U + ml_Z*fl_Z + mg*fg;        \
  }

#define PHV2(val, fs_U, fs_Z, fl_U, fl_Z, fg, ms_U, ms_Z, ml_U, ml_Z, mg) \
  {                                                                     \
    val = ms_U*ms_Z*ml_U*ml_Z*mg/(ms_Z*ml_U*ml_Z*mg*fs_U                \
          + ms_U*ml_U*ml_Z*mg*fs_Z + ms_U*ms_Z*ml_Z*mg*fl_U             \
          + ms_U*ms_Z*ml_U*mg*fl_Z + ms_U*ms_Z*ml_U*ml_Z*fg);           \
  }

#define muexp(mexp, t, tm_A_l, mu_l)      \
  {   type ttm;                           \
    ttm = t - tm_A_l;                     \
    if(abs(ttm) < 500.0) {                \
      mexp = 0.0206*exp(-0.0287*ttm);     \
      if(mexp > 1.0e+2) mexp = 1.0e+2;    \
      if(mexp < mu_l) mexp = mu_l;        \
    } else {                              \
      mexp = 1.0e+2;                      \
    }                                     \
  }

type NPHV1(type *fs, type *fl, type fg, type *ms, type *ml, type mg, domain *cdo)
{
  int i;
  type val=0.0;

  for(i=0; i<cdo->NumberOfComponent; i++){
    val += fs[i]*ms[i] + fl[i]*ml[i];
  }
  val += fg*mg;
  return val;
}

type NPHV1_emi(type *fs, type *fl, type fg, type *ms, type *ml, type *mg, domain *cdo)
{
  int i;
  type val=0.0;

  for(i=0; i<cdo->NumberOfComponent; i++){
    //val += fs[i]*ms[i] + fl[i]*ml[i] + fg*mg[i];
    // H26年度結果との比較のため、fg*mg[i]を消去 2017/01/25tonagi
    val += fs[i]*ms[i] + fl[i]*ml[i] ;
  }
  return val;
}

type NPHV2(type *fs, type *fl, type fg, type *ms, type *ml, type mg, domain *cdo)
{
  int i;
  type val=0.0;

  for(i=0; i<cdo->NumberOfComponent; i++){
    val += 1.0/ms[i]*fs[i] + 1.0/ml[i]*fl[i];
  }
  val += 1.0/mg*fg;
  val = 1.0/val;
  return val;
}

static inline int tempdep_is_const(const tempdep_property *prop)
{
  return prop->type == TEMPDEP_PROPERTY_CONST;
}

static inline type tempdep_calc_cached(const tempdep_property *prop, type temp,
                                       int is_const, type const_value)
{
  if (is_const) {
    return const_value;
  }
  return tempdep_calc((tempdep_property *)prop, temp);
}

static inline void tempdep_init_const_cache(const tempdep_property *prop,
                                            int *is_const, type *const_value)
{
  *is_const = tempdep_is_const(prop);
  *const_value = *is_const ? prop->data.t_const.value : 0.0;
}

type set_materials(material *mtl, variable *val, parameter *prm)
{
  flags       *flg = prm->flg;
  domain      *cdo = prm->cdo;
  phase_value *phv = prm->phv;
  int  icompo, ic, j, jx, jy, jz, m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;
  int  NumCompo=cdo->NumberOfComponent;
  int  NumBCompo = cdo->NBaseComponent;
  int  NumGCompo = cdo->NGasComponent;
  type fs_A_tmp, fs_B_tmp, fl_A_tmp, fl_B_tmp, *t=val->t, tg=phv->gas_tmp, tg4=tg*tg*tg*tg;
  type time0 = cpu_time(), *z=cdo->z;
  type fls_sum;
  type *Y = val->Y,*fl=val->fl, fg;//, *fg=val->fg;
  const int update_thermal = (flg->heat_eq == ON || flg->porous == ON ||
                              flg->two_energy == ON || flg->radiation == ON);
  const int update_emi = (flg->radiation == ON);
  const int update_porous = (flg->porous == ON);
  /*
   * YSE: Removed mu_l and tm_l, which is unused.
   * If required, you must calcurate them with:
   *
   *     mu_l = tempdep_calc(&phv->comps[0].mu_l, temp);
   *     tm_l = phv->comps[0].tm_liq;
   */
  type mumin, mumax, mumin_G, mumax_G;
  int rho_s_is_const[NumBCompo], rho_l_is_const[NumBCompo];
  int mu_s_is_const[NumBCompo], mu_l_is_const[NumBCompo];
  int specht_s_is_const[NumBCompo], specht_l_is_const[NumBCompo];
  int thc_s_is_const[NumBCompo], thc_l_is_const[NumBCompo];
  int emi_s_is_const[NumBCompo], emi_l_is_const[NumBCompo], emi_g_is_const[NumBCompo];
  int rho_g_comp_is_const[NumCompo], mu_g_comp_is_const[NumCompo];
  int specht_g_comp_is_const[NumCompo], thc_g_comp_is_const[NumCompo];
  type rho_s_const[NumBCompo], rho_l_const[NumBCompo];
  type mu_s_const[NumBCompo], mu_l_const[NumBCompo];
  type specht_s_const[NumBCompo], specht_l_const[NumBCompo];
  type thc_s_const[NumBCompo], thc_l_const[NumBCompo];
  type emi_s_const[NumBCompo], emi_l_const[NumBCompo], emi_g_const[NumBCompo];
  type rho_g_comp_const[NumCompo], mu_g_comp_const[NumCompo];
  type specht_g_comp_const[NumCompo], thc_g_comp_const[NumCompo];
  int rho_g_is_const, mu_g_is_const, specht_g_is_const, thc_g_is_const;
  int sigma_is_const;
  type rho_g_const, mu_g_const, specht_g_const, thc_g_const;
  type sigma_const;
  type fs_tmp[NumBCompo],fl_tmp[NumBCompo];

  type *eps = val->eps; 
  type *epss = val->epss; 
  type *perm = val->perm;
  type *sgm = val->sgm;
  type *fs = val->fs; 

  for (icompo = 0; icompo < NumBCompo; ++icompo) {
    tempdep_init_const_cache(&phv->comps[icompo].rho_s,
                             &rho_s_is_const[icompo], &rho_s_const[icompo]);
    tempdep_init_const_cache(&phv->comps[icompo].rho_l,
                             &rho_l_is_const[icompo], &rho_l_const[icompo]);
    tempdep_init_const_cache(&phv->comps[icompo].mu_s,
                             &mu_s_is_const[icompo], &mu_s_const[icompo]);
    tempdep_init_const_cache(&phv->comps[icompo].mu_l,
                             &mu_l_is_const[icompo], &mu_l_const[icompo]);
    tempdep_init_const_cache(&phv->comps[icompo].specht_s,
                             &specht_s_is_const[icompo], &specht_s_const[icompo]);
    tempdep_init_const_cache(&phv->comps[icompo].specht_l,
                             &specht_l_is_const[icompo], &specht_l_const[icompo]);
    tempdep_init_const_cache(&phv->comps[icompo].thc_s,
                             &thc_s_is_const[icompo], &thc_s_const[icompo]);
    tempdep_init_const_cache(&phv->comps[icompo].thc_l,
                             &thc_l_is_const[icompo], &thc_l_const[icompo]);
    if (update_emi) {
      tempdep_init_const_cache(&phv->comps[icompo].emi_s,
                               &emi_s_is_const[icompo], &emi_s_const[icompo]);
      tempdep_init_const_cache(&phv->comps[icompo].emi_l,
                               &emi_l_is_const[icompo], &emi_l_const[icompo]);
      tempdep_init_const_cache(&phv->comps[icompo].emi_g,
                               &emi_g_is_const[icompo], &emi_g_const[icompo]);
    }
  }
  for (icompo = 0; icompo < NumGCompo; ++icompo) {
    tempdep_init_const_cache(&phv->comps[icompo + NumBCompo].rho_g,
                             &rho_g_comp_is_const[icompo], &rho_g_comp_const[icompo]);
    tempdep_init_const_cache(&phv->comps[icompo + NumBCompo].mu_g,
                             &mu_g_comp_is_const[icompo], &mu_g_comp_const[icompo]);
    tempdep_init_const_cache(&phv->comps[icompo + NumBCompo].specht_g,
                             &specht_g_comp_is_const[icompo], &specht_g_comp_const[icompo]);
    tempdep_init_const_cache(&phv->comps[icompo + NumBCompo].thc_g,
                             &thc_g_comp_is_const[icompo], &thc_g_comp_const[icompo]);
  }
  tempdep_init_const_cache(&phv->rho_g, &rho_g_is_const, &rho_g_const);
  tempdep_init_const_cache(&phv->mu_g, &mu_g_is_const, &mu_g_const);
  tempdep_init_const_cache(&phv->specht_g, &specht_g_is_const, &specht_g_const);
  tempdep_init_const_cache(&phv->thc_g, &thc_g_is_const, &thc_g_const);
  tempdep_init_const_cache(&phv->comps[0].sigma, &sigma_is_const, &sigma_const);

  /*
   * Please rewrite set_materials() when you add 'generated' component(s).
   * This function only supports when all components has phv->comps[] entry
   * (except ID -1).
   */
  for (icompo = 0; icompo < NumCompo; ++icompo) {
    component_data *d;
    d = component_data_find_by_comp_index(&prm->comps_data_head, icompo);
    CSVASSERT(d->jupiter_id == -1 || d->phase_comps_index >= 0);
  }

  if(flg->solute_diff == OFF) {
    /* Not supported yet */
    CSVASSERT(component_info_ncompo(&cdo->mass_source_g_comps) == 0);

    if (!update_emi) {
#pragma omp parallel for
      for (j = 0; j < cdo->m; j++) {
        mtl->emi[j] = 0.0;
      }
    }

#pragma omp parallel
    {
      int j, ic;
      type vals, vall;

#pragma omp for
      for (j = 0; j < cdo->m; j++) {
        val->fl_sum[j] = 0.0;
        val->fs_sum[j] = 0.0;
      }
#pragma omp for collapse(3)
      for (jz = 0; jz < mz; jz++) {
        for (jy = 0; jy < my; jy++) {
          for (jx = 0; jx < mx; jx++) {
            j = jx + mx * jy + mxy * jz;
            vals = vall = 0.0;

            for (ic = 0; ic < NumBCompo; ic++) {
              vals += val->fs[j + ic * m];
              vall += val->fl[j + ic * m];
            }              

            val->fs_sum[j] = vals;
            val->fl_sum[j] = vall;
             
          }
        }
      }

      if (val->fs_ibm) {
#pragma omp for
        for (j = 0; j < cdo->m; j++) {
          val->fs_ibm[j] = 0.0;
        }
#pragma omp for collapse(3)
        for (jz = 0; jz < mz; jz++) {
          for (jy = 0; jy < my; jy++) {
            for (jx = 0; jx < mx; jx++) {
              j = jx + mx * jy + mxy * jz;
              vals = 0.0;
              for (ic = 0; ic < NumBCompo; ic++) {
                if (phv->comps[ic].sform != SOLID_FORM_IBM)
                  continue;
                vals += val->fs[j + ic * m];
              }
              val->fs_ibm[j] = vals;
            }
          }
        }
      }
    }
    mumax = 0.0;
    mumin = 1.0e+300;
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
        for(jx = 0; jx < mx; jx++) {
          /* YSE: Added to separate tempdep calc and average NPHV1 etc. */
          type s_tmp[NumBCompo], l_tmp[NumBCompo], g_tmp;
          type temp, tempf, temps;

          j = jx + mx*jy + mxy*jz;
          // clipping of VOF value

          val->fls[j] = val->fs_sum[j] + val->fl_sum[j];

          if(flg->multi_layer==ON){
            for (int il = 0; il <cdo->NumberOfLayer ; il++) {
              val->fls_layer[j + il * m] = val->fs_sum[j] + val->fl_layer[j + il * m];
            }              
          }

          for(icompo = 0; icompo < NumBCompo; icompo++) {
            fs_tmp[icompo] = clip(val->fs[j+icompo*m]);
            fl_tmp[icompo] = clip(val->fl[j+icompo*m]);
          }
          fls_sum = 0.0;
          for(icompo = 0; icompo < NumBCompo; icompo++) {
            fls_sum += fs_tmp[icompo] + fl_tmp[icompo];
          }            

          //fg[j] = 1.0 - fls_sum;
          //if(fg < 0.5) fg = 0.0;
          fls_sum = clip(fls_sum);
          fg = 1.0 - fls_sum;

          //temp = val->t[j];
          if (flg->two_energy == OFF)
            tempf = temps = val->t[j];
          else {
            tempf = val->tf[j];
            temps = val->ts[j];
          }
          //--- density
          for (icompo = 0; icompo < NumBCompo; icompo++) {
            s_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].rho_s, temps,
                                                rho_s_is_const[icompo], rho_s_const[icompo]);
            l_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].rho_l, tempf,
                                                rho_l_is_const[icompo], rho_l_const[icompo]);
          }
          g_tmp = tempdep_calc_cached(&phv->rho_g, tempf, rho_g_is_const, rho_g_const);
          mtl->dens[j] = NPHV1(fs_tmp, fl_tmp, fg, s_tmp, l_tmp, g_tmp, cdo);
          mtl->denss[j] = mtl->dens[j];
          //--- viscosity
          for (icompo = 0; icompo < NumBCompo; icompo++) {
            s_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].mu_s, temps,
                                                mu_s_is_const[icompo], mu_s_const[icompo]);
            l_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].mu_l, tempf,
                                                mu_l_is_const[icompo], mu_l_const[icompo]);
          }
          g_tmp = tempdep_calc_cached(&phv->mu_g, tempf, mu_g_is_const, mu_g_const);
          mtl->mu[j] = NPHV1(fs_tmp, fl_tmp, fg, s_tmp, l_tmp, g_tmp, cdo);

          /* film drainage model */
          if(prm->flg->film_drainage==ON){
            if(cdo->z[jz] > cdo->height_threshold){
              if(val->liquid_film[j]>0.0){
                mtl->mu[j] *= val->liquid_film[j]/cdo->rapture_thickness;
              }              
            }
          }

          if (update_thermal) {
            //--- specific heat coefficient
            for (icompo = 0; icompo < NumBCompo; icompo++) {
              s_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].specht_s, temps,
                                                  specht_s_is_const[icompo], specht_s_const[icompo]);
              l_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].specht_l, tempf,
                                                  specht_l_is_const[icompo], specht_l_const[icompo]);
            }
            g_tmp = tempdep_calc_cached(&phv->specht_g, tempf,
                                        specht_g_is_const, specht_g_const);
            mtl->specht[j] = NPHV1(fs_tmp, fl_tmp, fg, s_tmp, l_tmp, g_tmp, cdo);
            //--- thermal conductivity
            for (icompo = 0; icompo < NumBCompo; icompo++) {
              s_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].thc_s, temps,
                                                  thc_s_is_const[icompo], thc_s_const[icompo]);
              l_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].thc_l, tempf,
                                                  thc_l_is_const[icompo], thc_l_const[icompo]);
            }
            g_tmp = tempdep_calc_cached(&phv->thc_g, tempf, thc_g_is_const, thc_g_const);
            mtl->thc[j] = NPHV2(fs_tmp, fl_tmp, fg, s_tmp, l_tmp, g_tmp, cdo);
            mtl->thcs[j] = mtl->thc[j];
          }
          //--- emissivity < 2016 Added by KKE
          if (update_emi) {
            type g_tmpa[NumBCompo];

            for (icompo = 0; icompo < NumBCompo; icompo++) {
              s_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].emi_s, temps,
                                                  emi_s_is_const[icompo], emi_s_const[icompo]);
              l_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].emi_l, tempf,
                                                  emi_l_is_const[icompo], emi_l_const[icompo]);
              g_tmpa[icompo] = tempdep_calc_cached(&phv->comps[icompo].emi_g, tempf,
                                                   emi_g_is_const[icompo], emi_g_const[icompo]);
            }
            mtl->emi[j] = NPHV1_emi(fs_tmp, fl_tmp, fg, s_tmp, l_tmp, g_tmpa, cdo);
          }

          mtl->st[j] = tempdep_calc_cached(&phv->comps[0].sigma, tempf,
                                           sigma_is_const, sigma_const); //! under construction
          if(mtl->mu[j] > mumax) mumax = mtl->mu[j];
          if(mtl->mu[j] < mumin) mumin = mtl->mu[j];

          if (update_porous) {
            //---- porous medium settings, by Yamashita 2020/6/22
            val->eps[j] = 1.0; // porosity
            val->epss[j] = 1.0; // porosity
            {
              int refcompo = -1;
              int is_porous = 0;

              /*
               * Find a material to use as reference.
               *
               * While not supported, but implemented as use properties
               * of last porous media when multiple porous media of
               * fs[j] > 0.
               *
               * While not supported, but implemented as porous media has
               * priority when both fs[j] > 0.
               */
              for (icompo = 0; icompo < NumBCompo; icompo++) {
                enum solid_form sform = phv->comps[icompo].sform;
                if (sform == SOLID_FORM_UNUSED)
                  continue;

                if (val->fs[j + icompo * m] == 0.0)
                  continue;

                if (sform == SOLID_FORM_POROUS) {
                  is_porous = 1;
                  refcompo = icompo;
                } else if (!is_porous) {
                  refcompo = icompo;
                }
              }
              if (refcompo >= 0) {
                if (is_porous) {
                  eps[j] = prm->phv->comps[refcompo].poros; // porosity
                  perm[j] = 1.0 / prm->phv->comps[refcompo].permea; // permeability
                } else {
                  eps[j] = 0.0;
                  perm[j] = 0.0; // mathematically, infinity
                }
              } else { /* no solid exists */
                eps[j] = 1.0;
                perm[j] = 0.0;
              }
            }
          }
        }
      }
    }

    /* Yamashita, 2020/6/22 */
    //bcf_VOF(0, val->eps, val, prm);
    if (update_porous) {
      bcf(eps, prm);
      bcf(perm, prm);
    }

    // if porous == ON, (tentative) 
    if(flg->porous == ON) {
#if 1 //single phase
      // density
      /*
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            type temp, g_tmp;
            type s_tmp[NumBCompo], l_tmp[NumBCompo];
            j = jx + mx*jy + mxy*jz;
            temp = val->t[j];
            g_tmp = tempdep_calc(&phv->rho_g, temp);
            mtl->dens_f[j] = mtl->dens[j];
            for(icompo = 0; icompo < NumBCompo; icompo++) {
              s_tmp[icompo] = tempdep_calc(&phv->comps[icompo].rho_s, temp);
              if(fs[j+icompo*m] == 0.0) continue;
              //mtl->dens_f[j] = eps[j]*g_tmp*fs[j+icompo*m] + (1.0-eps[j])*mtl->dens[j];
              //mtl->dens_f[j] = 1.0/(eps[j]/(g_tmp*fs[j+icompo*m]) + (1.0-eps[j])/s_tmp[icompo]);
              //mtl->dens_f[j] = eps[j]*g_tmp*fs[j+icompo*m] + (1.0-eps[j])*s_tmp[icompo];

              //2020.10.13
              //g_tmp = tempdep_calc(&phv->comps[icompo].rho_l, temp);
              //mtl->dens_f[j] = eps[j]*g_tmp + (1.0-eps[j])*mtl->dens[j];
              mtl->dens[j] = eps[j]*g_tmp + (1.0-eps[j])*mtl->dens[j];
            }
          }
        }
      }
      */
      //--- dens for fluid
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            type tempf, g_tmp;
            j = jx + mx*jy + mxy*jz;
            if (flg->two_energy == OFF) tempf = val->t[j];
            else tempf = val->tf[j];
            g_tmp = tempdep_calc_cached(&phv->rho_g, tempf, rho_g_is_const, rho_g_const);
            mtl->dens_f[j] = g_tmp; //rho_f
          }
        }
      }
      //--- specific heat coefficient
      /*
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            type temp, g_tmp;
            type s_tmp[NumBCompo], l_tmp[NumBCompo];
            j = jx + mx*jy + mxy*jz;
            temp = val->t[j];
            g_tmp = tempdep_calc(&phv->specht_g, temp);
            for(icompo = 0; icompo < NumBCompo; icompo++) {
              s_tmp[icompo] = tempdep_calc(&phv->comps[icompo].specht_s, temp);
              if(fs[j+icompo*m] == 0.0) continue;
              //mtl->specht[j] = 1.0/(eps[j]/(g_tmp*fs[j+icompo*m]) + (1.0-eps[j])/s_tmp[icompo]);
              //mtl->specht[j] = eps[j]*g_tmp*fs[j+icompo*m] + (1.0-eps[j])*s_tmp[icompo];

              //g_tmp = tempdep_calc(&phv->comps[icompo].specht_l, temp);
              mtl->specht[j] = eps[j]*g_tmp + (1.0-eps[j])*s_tmp[icompo];
            }
          }
        }
      }
      */
      //--- specht for fluid
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            type tempf, g_tmp;
            j = jx + mx*jy + mxy*jz;
            if (flg->two_energy == OFF) tempf = val->t[j];
            else tempf = val->tf[j];
            g_tmp = tempdep_calc_cached(&phv->specht_g, tempf,
                                        specht_g_is_const, specht_g_const);
            mtl->c_f[j] = g_tmp; //c_f
          }
        }
      }
      //--- thc for fluid
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            type tempf, g_tmp;
            j = jx + mx*jy + mxy*jz;
            if (flg->two_energy == OFF) tempf = val->t[j];
            else tempf = val->tf[j];
            g_tmp = tempdep_calc_cached(&phv->thc_g, tempf, thc_g_is_const, thc_g_const);
            mtl->thcf[j] = g_tmp;
          }
        }
      }
      //--- Equivalent property of rho*C
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            j = jx + mx*jy + mxy*jz;
            sgm[j] = ( eps[j]*mtl->dens_f[j]*mtl->c_f[j] 
                   + (1.0-eps[j])*mtl->dens[j]*mtl->specht[j] )/mtl->dens_f[j]/mtl->c_f[j];
          }
        }
      }
      //--- dens for fluid (redefine): dens including solid phase --> only fluid
//      if (flg->two_energy == OFF) {
        for(jz = 0; jz < mz; jz++) {
          for(jy = 0; jy < my; jy++) {
            for(jx = 0; jx < mx; jx++) {
              int icompo;
              int ibm = -1;

              j = jx + mx*jy + mxy*jz;

              for (icompo = 0; icompo < cdo->NBaseComponent; ++icompo) {
                if (phv->comps[icompo].sform == SOLID_FORM_POROUS) {
                  if (fs[j + icompo * cdo->m] > 0.0) {
                    ibm = -1;
                    break;
                  }
                }

                if (phv->comps[icompo].sform != SOLID_FORM_IBM)
                  continue;

                if (fs[j + icompo * cdo->m] > 0.0) {
                  ibm = icompo;
                }
              }

              /* If porous media exists or no IBM solid exists */
              if (ibm < 0) {
                mtl->dens[j] = mtl->dens_f[j];
              }
            }
          }
        }
//      }
      //--- viscosity
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            type tempf, g_tmp;
            type s_tmp[NumBCompo], l_tmp[NumBCompo];
            j = jx + mx*jy + mxy*jz;
            if (flg->two_energy == OFF) tempf = val->t[j];
            else tempf = val->tf[j];
            g_tmp = tempdep_calc_cached(&phv->mu_g, tempf, mu_g_is_const, mu_g_const);
            {
              int refcompo = -1;
              int is_porous = 0;

              for (icompo = 0; icompo < NumBCompo; icompo++) {
                enum solid_form sform = phv->comps[icompo].sform;
                if (sform == SOLID_FORM_UNUSED)
                  continue;

                if (fs[j + icompo * m] == 0.0)
                  continue;

                if (sform == SOLID_FORM_POROUS) {
                  is_porous = 1;
                  refcompo = icompo;
                } else if (!is_porous) {
                  refcompo = icompo;
                }
              }
              if (is_porous || refcompo < 0)
                mtl->mu[j] = g_tmp;
            }

           /*
            for(icompo = 0; icompo < NumBCompo; icompo++) {
              s_tmp[icompo] = tempdep_calc(&phv->comps[icompo].mu_s, temp);
              if(fs[j+icompo*m] == 0.0) continue;
              //mtl->mu[j] = 1.0/(eps[j]/(g_tmp*fs[j+icompo*m]) + (1.0-eps[j])/s_tmp[icompo]);
              //mtl->mu[j] = eps[j]*g_tmp*fs[j+icompo*m] + (1.0-eps[j])*mtl->mu[j];

              g_tmp = tempdep_calc(&phv->comps[icompo].mu_l, temp);
              //mtl->mu[j] = eps[j]*g_tmp*fs[j+icompo*m] + (1.0-eps[j])*mtl->mu[j];
              mtl->mu[j] = eps[j]*g_tmp + (1.0-eps[j])*mtl->mu[j];
            }
           */ 
          }
        }
      }
      //--- thermal conductivity
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            type tempf, temps, g_tmp;
            type s_tmp[NumBCompo], l_tmp[NumBCompo];
            j = jx + mx*jy + mxy*jz;
            if (flg->two_energy == OFF) tempf = temps = val->t[j];
            else { 
              tempf = val->tf[j];
              temps = val->ts[j];
            }
            g_tmp = tempdep_calc_cached(&phv->thc_g, tempf, thc_g_is_const, thc_g_const);
            for(icompo = 0; icompo < NumBCompo; icompo++) {
              if (phv->comps[icompo].sform != SOLID_FORM_POROUS)
                continue;

              s_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].thc_s, temps,
                                                  thc_s_is_const[icompo], thc_s_const[icompo]);
              if(fs[j+icompo*m] == 0.0) continue;
              //mtl->thc[j] = eps[j]*g_tmp*fs[j+icompo*m] + (1.0-eps[j])*s_tmp[icompo];
              //mtl->thc[j] = 1.0/(eps[j]/(g_tmp*fs[j+icompo*m]) + (1.0-eps[j])/s_tmp[icompo]);
              //g_tmp = tempdep_calc(&phv->comps[icompo].thc_l, temp);
              //mtl->thc[j] = 1.0/(eps[j]/g_tmp + (1.0-eps[j])/s_tmp[icompo]);     // Straight; Min
              //mtl->thc[j] = (2.0+eps[j])/3.0*g_tmp + (1.0-eps[j])/3.0*mtl->thc[j]; // Summation law
              //mtl->thc[j] = eps[j]*g_tmp + (1.0-eps[j])*mtl->thc[j];             // Parallel; Max
              //mtl->thc[j] = sqrt((1.0/(eps[j]/g_tmp + (1.0-eps[j])/s_tmp[icompo]))*(eps[j]*g_tmp + (1.0-eps[j])*mtl->thc[j]));  //kika; Middle
              mtl->thc[j] = pow(g_tmp,eps[j])*pow(s_tmp[icompo],(1-eps[j]));  //kika2; Middle
              if (flg->two_energy == ON) 
                mtl->thc[j] = (2.0+eps[j])/3.0*g_tmp + (1.0-eps[j])/3.0*s_tmp[icompo]; // Summation law
            }
          }
        }
      }
      //--- Effective porosity used for two energy model
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            type tempf, temps, g_tmp;
            type s_tmp[NumBCompo], l_tmp[NumBCompo];
            j = jx + mx*jy + mxy*jz;
            if (flg->two_energy == OFF) tempf = temps = val->t[j];
            else { 
              tempf = val->tf[j];
              temps = val->ts[j];
            }
            g_tmp = tempdep_calc_cached(&phv->thc_g, tempf, thc_g_is_const, thc_g_const);
            for(icompo = 0; icompo < NumBCompo; icompo++) {
              if (phv->comps[icompo].sform != SOLID_FORM_POROUS)
                continue;

              s_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].thc_s, temps,
                                                  thc_s_is_const[icompo], thc_s_const[icompo]);
              if(fs[j+icompo*m] == 0.0) continue;
              val->epss[j] = (s_tmp[icompo] - mtl->thc[j])/(s_tmp[icompo] - g_tmp);
            }
          }
        }
      }
#else //multiphase
      //--- density
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            type temp, g_tmp;
            type s_tmp[NumBCompo], l_tmp[NumBCompo];
            j = jx + mx*jy + mxy*jz;
            temp = val->t[j];
//            g_tmp = tempdep_calc(&phv->rho_g, temp);
            mtl->dens_f[j] = mtl->dens[j];
            for(icompo = 0; icompo < NumBCompo; icompo++) {
              s_tmp[icompo] = tempdep_calc(&phv->comps[icompo].rho_s, temp);
              if(fs[j+icompo*m] == 0.0) continue;
              //mtl->dens_f[j] = eps[j]*g_tmp*fs[j+icompo*m] + (1.0-eps[j])*mtl->dens[j];
              //mtl->dens_f[j] = 1.0/(eps[j]/(g_tmp*fs[j+icompo*m]) + (1.0-eps[j])/s_tmp[icompo]);
              //mtl->dens_f[j] = eps[j]*g_tmp*fs[j+icompo*m] + (1.0-eps[j])*s_tmp[icompo];

              //2020.10.13
              g_tmp = tempdep_calc(&phv->comps[icompo].rho_l, temp);
              mtl->dens_f[j] = eps[j]*g_tmp + (1.0-eps[j])*mtl->dens[j];
            }
          }
        }
      }
      //--- viscosity
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            type temp, g_tmp;
            type s_tmp[NumBCompo], l_tmp[NumBCompo];
            j = jx + mx*jy + mxy*jz;
            temp = val->t[j];
            //g_tmp = tempdep_calc(&phv->mu_g, temp);
            //mtl->mu[j] = g_tmp;
            
            for(icompo = 0; icompo < NumBCompo; icompo++) {
              s_tmp[icompo] = tempdep_calc(&phv->comps[icompo].mu_s, temp);
              if(fs[j+icompo*m] == 0.0) continue;
              //mtl->mu[j] = 1.0/(eps[j]/(g_tmp*fs[j+icompo*m]) + (1.0-eps[j])/s_tmp[icompo]);
              //mtl->mu[j] = eps[j]*g_tmp*fs[j+icompo*m] + (1.0-eps[j])*mtl->mu[j];

              g_tmp = tempdep_calc(&phv->comps[icompo].mu_l, temp);
              mtl->mu[j] = eps[j]*g_tmp + (1.0-eps[j])*mtl->mu[j];
            }
          }
        }
      }
      //--- specific heat coefficient
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            type temp, g_tmp;
            type s_tmp[NumBCompo], l_tmp[NumBCompo];
            j = jx + mx*jy + mxy*jz;
            temp = val->t[j];
            //g_tmp = tempdep_calc(&phv->specht_g, temp);
            for(icompo = 0; icompo < NumBCompo; icompo++) {
              s_tmp[icompo] = tempdep_calc(&phv->comps[icompo].specht_s, temp);
              if(fs[j+icompo*m] == 0.0) continue;
              //mtl->specht[j] = 1.0/(eps[j]/(g_tmp*fs[j+icompo*m]) + (1.0-eps[j])/s_tmp[icompo]);
              //mtl->specht[j] = eps[j]*g_tmp*fs[j+icompo*m] + (1.0-eps[j])*s_tmp[icompo];

              g_tmp = tempdep_calc(&phv->comps[icompo].specht_l, temp);
              //mtl->specht[j] = eps[j]*g_tmp + (1.0-eps[j])*s_tmp[icompo];
              mtl->specht[j] = eps[j]*g_tmp + (1.0-eps[j])*mtl->specht[j];
            }
          }
        }
      }
      //--- thermal conductivity
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            type temp, g_tmp;
            type s_tmp[NumBCompo], l_tmp[NumBCompo];
            j = jx + mx*jy + mxy*jz;
            temp = val->t[j];
            //g_tmp = tempdep_calc(&phv->thc_g, temp);
            for(icompo = 0; icompo < NumBCompo; icompo++) {
              s_tmp[icompo] = tempdep_calc(&phv->comps[icompo].thc_s, temp);
              if(fs[j+icompo*m] == 0.0) continue;
              //mtl->thc[j] = eps[j]*g_tmp*fs[j+icompo*m] + (1.0-eps[j])*s_tmp[icompo];
              //mtl->thc[j] = 1.0/(eps[j]/(g_tmp*fs[j+icompo*m]) + (1.0-eps[j])/s_tmp[icompo]);

              g_tmp = tempdep_calc(&phv->comps[icompo].thc_l, temp);
              mtl->thc[j] = 1.0/(eps[j]/g_tmp + (1.0-eps[j])/mtl->thc[j]);           // Straight; Max
            }
          }
        }
      }
#endif

    }

    if (update_porous) {
      bcf(sgm, prm);

      bcf(eps, prm);
      bcf(epss, prm);
      bcf(perm, prm);
    }

    mumax_G = mumax;
    mumin_G = mumin;
#ifdef JUPITER_MPI
    MPI_Allreduce(&mumax, &mumax_G, 1, MPI_TYPE, MPI_MAX, prm->mpi->CommJUPITER);
    MPI_Allreduce(&mumin, &mumin_G, 1, MPI_TYPE, MPI_MIN, prm->mpi->CommJUPITER);
#endif
#ifdef _TIME_
    time_materials += cpu_time()-time0;
#endif
    //	if(prm->mpi->rank == 0){
    //		printf("Mu_max = %le\n", mumax);
    //		printf("Mu_min = %le\n", mumin);
    //	}
    //phv->rad_W = rad_W;
// ----------------- solute diff == ON -----------------------------------------------------------
  } else {
    /* YSE: OpenMP shared table_error */
    static table_error liqterr = TABLE_SUCCESS, solterr = TABLE_SUCCESS;
    static int Ysum_mismatch = 0;
    static int negative_mass_warn = 0;
    const int nmass_src_ids = component_info_ncompo(&cdo->mass_source_g_comps);

    mumax = 0.0;
    mumin = 1.0e+300;

#pragma omp parallel for collapse(3), private(j,icompo), \
  reduction(min: mumin), reduction(max: mumax)
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
        for(jx = 0; jx < mx; jx++) {
          table_error err;
          type fs_tmp,fl_tmp,fg,Y_clip[NumCompo+1], Y_sum;
          type dens_sum, mu_sum, spec_sum, thc_sum, lat_sum, soli_sum, liq_sum;
          type table_err_x, table_err_y;
          /* YSE: Added to separate tempdep calc and average NPHV1 etc. */
          type s_tmp[NumBCompo], l_tmp[NumBCompo];
          type g_tmp[NumCompo + 1], temp;
          type Ygsum;
          type *Yga;

          j = jx + mx*jy + mxy*jz;
          dens_sum = mu_sum = spec_sum = thc_sum = lat_sum = soli_sum = liq_sum = 0.0;
          // clipping of VOF value
          fs_tmp = clipx(val->fs[j]);
          fl_tmp = clipx(val->fl[j]);
          val->fls[j] = val->fs[j] + val->fl[j];
          //fg = 1.0 - fs_tmp - fl_tmp;
          fg = clipx(1.0 - val->fls[j]);
          //if(fg < 0.5) fg = 0.0;
          //val->fg[j] = (1.0 - val->fls[j]);
          //val->fg[j] = (1.0 - val->fls[j]);
          Y_sum = 0.0;
          Ygsum = 0.0;
          for(icompo = 0; icompo < NumCompo; icompo++) {
            Y_clip[icompo] = clipx(Y[j+icompo*m]);
            if (icompo < NumBCompo) {
              Y_sum += Y_clip[icompo];
            } else {
              Ygsum += Y_clip[icompo];
            }
          }
#if defined(JUPITER_MATERIAL_NORMALIZE_Y)
          if (Y_sum > 0.0) {
            for(icompo = 0; icompo < NumBCompo; icompo++) {
              Y_clip[icompo] = Y_clip[icompo] / Y_sum;
            }
          }
          /* Yg cannot be normalized */
#endif
          /* Avoid conditon of sum(Y) == 0 && fs + fl != 0 */
          if (Y_sum <= 0.0) {
            if (fg == 0.0) {
              int f;
#pragma omp atomic read
              f = Ysum_mismatch;
              if (!f) {
#pragma omp critical
                {
                  if (!Ysum_mismatch) {
                    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_WARN, NULL,
                               "sum(Y) == 0 where fl or fs != 0 found at cell "
                               "(%d, %d, %d), fs + fl = %.8e "
                               "(This message will be printed only for the "
                               "first time)",
                               1.0 - fg,
                               jx - cdo->stm, jy - cdo->stm, jz - cdo->stm);
#pragma omp atomic write
                    Ysum_mismatch = 1;
                  }
                }
              }
            }
            fg = 1.0;
            fl_tmp = 0.0;
            fs_tmp = 0.0;
            Y_sum = 0.0;
          }
          /* Content for ID = -1 */
          Y_clip[icompo] = clipx(1.0 - Ygsum);
          Yga = &Y_clip[NumBCompo];

          temp = val->t[j];

          //--- Liquidus Temperature
          for (icompo = 0; icompo < NumBCompo; ++icompo) {
            l_tmp[icompo] = phv->comps[icompo].tm_liq;
            s_tmp[icompo] = phv->comps[icompo].tm_soli;
          }
          err = TABLE_SUCCESS;
          mtl->t_liq[j] = tm_calc_all(NumBCompo, Y_clip,
                                      l_tmp, phv->liq_tables,
                                      phv->liq_funcs,
                                      &err, &table_err_x, &table_err_y);
          if (err != TABLE_SUCCESS) {
#pragma omp critical
            {
              if (liqterr == TABLE_SUCCESS) {
                liqterr = err;
                csvperrorf(__FILE__, __LINE__, 0, CSV_EL_WARN,
                           NULL, "Liquidus temperature table search error at cell (%d, %d, %d): %s for (%g, %g) (Notifies only first time)",
                           jx - cdo->stm, jy - cdo->stm, jz - cdo->stm,
                           table_errorstr(err), table_err_x, table_err_y);
              }
            }
          }

          //--- Solidus Temperature
          err = TABLE_SUCCESS;
          mtl->t_soli[j] = tm_calc_all(NumBCompo, Y_clip,
                                       s_tmp, phv->sol_tables,
                                       phv->sol_funcs,
                                       &err, &table_err_x, &table_err_y);
          if (err != TABLE_SUCCESS) {
#pragma omp critical
            {
              if (solterr == TABLE_SUCCESS) {
                solterr = err;
                csvperrorf(__FILE__, __LINE__, 0, CSV_EL_WARN,
                           NULL, "Solidus temperature table search error at cell (%d, %d, %d): %s for (%g, %g) (Notifies only first time)",
                           jx - cdo->stm, jy - cdo->stm, jz - cdo->stm,
                           table_errorstr(err), table_err_x, table_err_y);
              }
            }
          }

#if !defined(JUPITER_MASS_SOURCE_WITHIN_EQUATION)
          /* Add or subtract mass source */
          if (val->mass_source_g) {
            if (mtl->dens[j] > 0.0 && Ygsum > 0.0) {
              int r;
              type dx, dy, dz, volume;
              type mass[nmass_src_ids];

              for (int i = 0; i < nmass_src_ids; ++i) {
                mass[i] = val->mass_source_g[j + i * cdo->m];
              }

              for (icompo = 0; icompo < NumCompo + 1; ++icompo) {
                g_tmp[icompo] = Y_clip[icompo];
              }

              dx = cdo->x[jx + 1] - cdo->x[jx];
              dy = cdo->y[jy + 1] - cdo->y[jy];
              dz = cdo->z[jz + 1] - cdo->z[jz];
              volume = dx * dy * dz;
              r = add_mass_to_comp(mtl->dens[j], volume, NumBCompo,
                                   NumCompo + 1, Y_clip,
                                   &cdo->mass_source_g_comps, mass);
              if (r > -1) {
                int s;
#pragma omp atomic read
                s = negative_mass_warn;
                if (!s) {
#pragma omp critical
                  {
                    if (!negative_mass_warn) {
                      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_WARN, NULL,
                                 "Mass source on component ID %d causes "
                                 "negative mass (This message will be printed "
                                 "only for the first time)",
                                 r);
                      negative_mass_warn = 1;
                    }
                  }
                }

                /*
                 * Y_clip gets undefined when add_mass_to_comp returns error,
                 * So revert to previous value.
                 */
                for (icompo = 0; icompo < NumCompo + 1; ++icompo) {
                  Y_clip[icompo] = g_tmp[icompo];
                }
              }
            }

            for (int i = 0; i < nmass_src_ids; ++i) {
              val->mass_source_g[j + i * cdo->m] = 0.0;
            }
          }
#endif

          //--- density
          for (icompo = 0; icompo < NumBCompo; icompo++) {
            s_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].rho_s, temp,
                                                rho_s_is_const[icompo], rho_s_const[icompo]);
            l_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].rho_l, temp,
                                                rho_l_is_const[icompo], rho_l_const[icompo]);
          }
          for (icompo = 0; icompo < NumGCompo; icompo++) {
            g_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo + NumBCompo].rho_g, temp,
                                                rho_g_comp_is_const[icompo], rho_g_comp_const[icompo]);
          }
          g_tmp[icompo] = tempdep_calc_cached(&phv->rho_g, temp, rho_g_is_const, rho_g_const);
          mtl->dens[j] = YNPHV2c(NumBCompo, NumGCompo + 1,
                                 Y_clip, Yga, &fs_tmp, &fl_tmp, fg,
                                 s_tmp, l_tmp, g_tmp);

#if !defined(JUPITER_MASS_SOURCE_WITHIN_EQUATION)
          /* Renormalize Yg to sum(Yg) == 1. */
          if (val->mass_source_g) {
            Ygsum = 0.0;
            for (icompo = NumBCompo; icompo < NumCompo + 1; ++icompo) {
              Ygsum += Y_clip[icompo];
            }
            if (Ygsum != 0.0) {
              Ygsum = 1.0 / Ygsum;
              for (icompo = NumBCompo; icompo < NumCompo; ++icompo) {
                type y = Y_clip[icompo] * Ygsum;
                Y_clip[icompo] = y;
                val->Y[j + icompo * cdo->m] = y;
              }
            }
          }
#endif

          //--- vicosity
          for (icompo = 0; icompo < NumBCompo; icompo++) {
            s_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].mu_s, temp,
                                                mu_s_is_const[icompo], mu_s_const[icompo]);
            l_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].mu_l, temp,
                                                mu_l_is_const[icompo], mu_l_const[icompo]);
          }
          for (icompo = 0; icompo < NumGCompo; icompo++) {
            g_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo + NumBCompo].mu_g, temp,
                                                mu_g_comp_is_const[icompo], mu_g_comp_const[icompo]);
          }
          g_tmp[icompo] = tempdep_calc_cached(&phv->mu_g, temp, mu_g_is_const, mu_g_const);
          mtl->mu[j] = YNPHV2c(NumBCompo, NumGCompo + 1,
                               Y_clip, Yga, &fs_tmp, &fl_tmp, fg,
                               s_tmp, l_tmp, g_tmp);
          //--- specific heat coefficient
          for (icompo = 0; icompo < NumBCompo; icompo++) {
            s_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].specht_s, temp,
                                                specht_s_is_const[icompo], specht_s_const[icompo]);
            l_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].specht_l, temp,
                                                specht_l_is_const[icompo], specht_l_const[icompo]);
          }
          for (icompo = 0; icompo < NumGCompo; icompo++) {
            g_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo + NumBCompo].specht_g, temp,
                                                specht_g_comp_is_const[icompo], specht_g_comp_const[icompo]);
          }
          g_tmp[icompo] = tempdep_calc_cached(&phv->specht_g, temp,
                                              specht_g_is_const, specht_g_const);
          mtl->specht[j] = YNPHV2c(NumBCompo, NumGCompo + 1,
                                   Y_clip, Yga, &fs_tmp, &fl_tmp, fg,
                                   s_tmp, l_tmp, g_tmp);
          //--- thermal conductivity
          for (icompo = 0; icompo < NumBCompo; icompo++) {
            s_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].thc_s, temp,
                                                thc_s_is_const[icompo], thc_s_const[icompo]);
            l_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo].thc_l, temp,
                                                thc_l_is_const[icompo], thc_l_const[icompo]);
          }
          for (icompo = 0; icompo < NumGCompo; icompo++) {
            g_tmp[icompo] = tempdep_calc_cached(&phv->comps[icompo + NumBCompo].thc_g, temp,
                                                thc_g_comp_is_const[icompo], thc_g_comp_const[icompo]);
          }
          g_tmp[icompo] = tempdep_calc_cached(&phv->thc_g, temp, thc_g_is_const, thc_g_const);
          mtl->thc[j] = YNPHV2c(NumBCompo, NumGCompo + 1,
                                Y_clip, Yga, &fs_tmp, &fl_tmp, fg,
                                s_tmp, l_tmp, g_tmp);
          //--- solute diffusion
          //modified by Chai
          /*
          if (phv->diff_params) {
            mtl->Dc[j] = dc_calc_all(NumBCompo, phv->diff_params,
                                     phv->diff_id_list, Y_clip, temp);
          } else {
            for (icompo = 0; icompo < NumBCompo; icompo++) {
              s_tmp[icompo] = tempdep_calc(&phv->comps[icompo].diff, temp);
            }
            mtl->Dc[j] = YNPHV1c(NumBCompo, 0, Y_clip, NULL, NULL, NULL,
                                 s_tmp, NULL, NULL);
          }*/
          //added by Chai
/*(          for (icompo = 0; icompo < NumCompo; icompo++) {
	  mtl->Dc[j+m*icompo]=calc_Dc(icompo, temp);
	    }
  */
          //--- gas diffusion
          if (NumGCompo >= 1) {
            for (icompo = 0; icompo < NumGCompo; icompo++) {
              g_tmp[icompo + 1] = fg * Yga[icompo];
            }
            g_tmp[0] = fg * Yga[NumGCompo];
            mtl->Dcg[j] = dc_calc_p_all(NumGCompo + 1, phv->diff_g_params, 1, g_tmp, temp);
          } else {
            mtl->Dcg[j] = 0.0;
          }

          //--- Latent Heat
          for (icompo = 0; icompo < NumBCompo; icompo++) {
            s_tmp[icompo] = phv->comps[icompo].lh;
          }
          mtl->latent[j] = YNPHV2c(NumBCompo, 0, Y_clip, NULL, NULL, NULL, 0.0,
                                   s_tmp, NULL, NULL);

          for(icompo = 0; icompo < NumCompo; icompo++) {
            //Y_clip = clip(Y[j+icompo*m]);
            //--- density
            //dens_sum += Y_clip/phv->rho_l[icompo];
            //--- viscosity
            //mu_sum += Y_clip[icompo]/phv->mu_l[icompo];
            //--- specific heat coefficient
            //spec_sum += Y_clip[icompo]/phv->specht_l[icompo];
            //--- thermal conductivity
            //thc_sum += Y_clip[icompo]/phv->thc_l[icompo];
            //--- solute diffusion
            //Dc_sum += Y_clip[icompo]*phv->diff[icompo];
            //Dc_sum += Y_clip*diff_coef(icompo,t[j]);
            //lat_sum += Y_clip[icompo]/phv->lh[icompo];
            //liq_sum += Y_clip/phv->tm_liq[icompo];
            //liq_sum += Y_clip[icompo]/liq(icompo,Y_clip[icompo]);
            //soli_sum += Y_clip/phv->tm_soli[icompo];
          }
          //mtl->dens[j] = 1.0/(dens_sum+fg/phv->rho_g);
          //mtl->mu[j] = 1.0/(mu_sum+fg/phv->mu_g);
          //mtl->specht[j] = 1.0/(spec_sum+fg/phv->specht_g);
          //mtl->thc[j] = 1.0/(thc_sum+fg/phv->thc_g);

          //mtl->Dc[j] = 1.0/(Dc_sum+fg/1.0e-10);//+fg*1.0e-10;
          //mtl->Dc[j] = Dc_sum;
          //printf("%le\n",mtl->Dc[j]);
          //mtl->latent[j] = 1.0/lat_sum;
          //mtl->t_liq[j] = 1.0/liq_sum;
          //mtl->t_soli[j] = mtl->t_liq[j];

          mtl->st[j] = tempdep_calc_cached(&phv->comps[0].sigma, temp,
                                           sigma_is_const, sigma_const); //! under construction

          //---- temperature dependency
          //if(prm->flg->debug == ON){
          //	if(fl_tmp[0] > 0.0) 
          //		muexp(mtl->mu[j], t[j], tm_l, mu_l);
          //}

          /*
           * YSE: unused
           */
          /*
           if(mtl->mu[j] > mumax) mumax = mtl->mu[j];
           if(mtl->mu[j] < mumin) mumin = mtl->mu[j];
          */
        }
      }
    }

  }
  return  cpu_time() - time0;
}

type materials(material *mtl, variable *val, parameter *prm)
{
  return set_materials(mtl, val, prm);
}

#undef PHV1
#undef PHV2

int add_mass_to_comp(type dens, type volume, int nbcompo, int ncompo, type *Y,
                     struct component_info *comps_to_add,
                     const type *mass_to_add)
{
#if JUPITER_MASS_SOURCE_USE_BREAK_CONSTRAINT
  return add_mass_with_break_constraint(dens, volume, nbcompo, ncompo, Y,
                                        comps_to_add, mass_to_add);
#else
  return add_mass_with_renormal(dens, volume, nbcompo, ncompo, Y,
                                comps_to_add, mass_to_add);
#endif
}
