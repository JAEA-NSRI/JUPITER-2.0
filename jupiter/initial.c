#include "csv.h"
#include "geometry/list.h"
#include "lpt.h"
#include "random/random.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#ifdef GPU
#  include <cuda.h>
#  include <cutil.h>
#endif

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

/* YSE: Use CSV input utility functions. */
#include <string.h>
#include "csvutil.h"
/* YSE: end */

#include "init_component.h"
#include "update_level_set_flags.h"
#include "struct.h"
#include "func.h"
#include "boundary_util.h"
#include "common_util.h"
#include "field_control.h"
#include "oxidation.h"
#include "os/os.h"
#include "os/asprintf.h"

#define PI 3.141592653589793
#include "tempdep_calc.h"

#include "dccalc.h"

#ifdef LPT
#include "lpt/LPTbnd.h"
#endif

#ifdef LPTX
#include "lptx/defs.h"
#include "lptx/init_set.h"
#include "lptx/param.h"
#include "lptx/particle.h"
#include "lptx/ptflags.h"
#include "lptx/vector.h"
#endif

/* YSE: Added to compute the minimum and maximum in the data */
static void calc_min_max(mpi_param *mpi, domain *cdo, type *arr,
                         type *min, type *max)
{
  int j, jx, jy, jz, mx, mxy;
  type lmin, lmax;

  CSVASSERT(arr);
  CSVASSERT(min);
  CSVASSERT(max);
  CSVASSERT(mpi);
  CSVASSERT(cdo);

  mx  = cdo->mx;
  mxy = cdo->mxy;

  j = cdo->stm + mx*(cdo->stm) + mxy*(cdo->stm);
  lmin = arr[j];
  lmax = arr[j];

#pragma omp parallel for private(jz,jy,jx,j) reduction(min: lmin) reduction(max: lmax)
  for(jz = 0; jz < cdo->nz; jz++) {
    for(jy = 0; jy < cdo->ny; jy++) {
      for(jx = 0; jx < cdo->nx; jx++) {
        j = jx+cdo->stm + mx*(jy + cdo->stm) + mxy*(jz + cdo->stm);
        if (arr[j] < lmin) lmin = arr[j];
        if (arr[j] > lmax) lmax = arr[j];
      }
    }
  }
#ifdef JUPITER_MPI
  MPI_Allreduce(MPI_IN_PLACE, &lmin, 1, MPI_TYPE, MPI_MIN, mpi->CommJUPITER);
  MPI_Allreduce(MPI_IN_PLACE, &lmax, 1, MPI_TYPE, MPI_MAX, mpi->CommJUPITER);
#endif

  *min = lmin;
  *max = lmax;
}
/* YSE: end */

int init_boundary(mpi_param *mpi, domain *cdo, flags *flg, variable *val);

void init_variables(variable *val, material *mtl, parameter *prm)
{
  flags  *flg;
  domain *cdo;
  phase_value *phv;
  int  icompo, j, jx, jy, jz, m, mx, my, mz, mxy;

  /* YSE: Test the parameters and then use them. */
  CSVASSERT(prm);
  CSVASSERT(prm->flg);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->phv);
  CSVASSERT(val);
  CSVASSERT(mtl);

  flg = prm->flg;
  cdo = prm->cdo;
  phv = prm->phv;
  m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;

  /* YSE: Skip variable initialization if restart is requested. */
  if (flg->restart >= 0) return;

  if (flg->lpt_calc == ON) {
    ptrdiff_t jj;
    mpi_param *mpi = prm->mpi;

    /* Set initial LPT field variables which is not be zero... */
#pragma omp parallel for
    for (jj = 0; jj < cdo->m; ++jj) {
      int i, j, k;

      calc_struct_index(jj, cdo->mx, cdo->my, cdo->mz, &i, &j, &k);

      i -= cdo->stm;
      j -= cdo->stm;
      k -= cdo->stm;
      if (mpi->nrk[0] == -1 && k < 0) {
        val->lpt_pewall[jj] = cdo->lpt_wallref_zm;
      } else if (mpi->nrk[1] == -1 && k >= cdo->nz) {
        val->lpt_pewall[jj] = cdo->lpt_wallref_zp;
      } else if (mpi->nrk[2] == -1 && j < 0) {
        val->lpt_pewall[jj] = cdo->lpt_wallref_ym;
      } else if (mpi->nrk[3] == -1 && j >= cdo->ny) {
        val->lpt_pewall[jj] = cdo->lpt_wallref_yp;
      } else if (mpi->nrk[4] == -1 && i < 0) {
        val->lpt_pewall[jj] = cdo->lpt_wallref_xm;
      } else if (mpi->nrk[5] == -1 && i >= cdo->nx) {
        val->lpt_pewall[jj] = cdo->lpt_wallref_xp;
      } else {
        val->lpt_pewall[jj] = -999.999;
      }
    }
  }

  // VOF (liquid - (solid+gas))
  // VOF (solid - (liquid+gas))
  /* YSE: Read geometry data, or initalize by hard coded program. */
  if (flg->geom_in == ON) {
    init_component comps = init_component_all();
    init_component_for_initial(&comps, prm);

    geometry_in(prm->cdo->icnt, val, prm, comps);
  } else {
#ifndef NDEBUG
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_DEBUG,
               __func__, "Initializing VOF with init_VOF%s",
               (flg->solute_diff == ON) ? " and init_Y" : "");
#endif
    init_VOF(val->fl, val->fs, val->flg_obst_A, val->flg_obst_B, prm);
    if (flg->solute_diff == ON) {
      init_Y(val, prm);
    }
  }

  init_boundary(prm->mpi, cdo, flg, val);

  if (flg->solute_diff == ON) {
    /* Vf is not available yet */
    bcs(val->Y, NULL, val, prm, mtl);
  }

  bcf_VOF(1,val->fl, val, prm);
  bcf_VOF(0,val->fs, val, prm);

  if (flg->solute_diff == ON) {
    CSVASSERT(!val->fs_ibm);
  }

#pragma omp parallel
  {
    int jx, jy, jz, j, icompo;
    type vals, vall, vals_ibm;

    if (flg->solute_diff == OFF) {
#pragma omp for
      for (j = 0; j < cdo->m; ++j) {
        vals = vall = 0.0;
        for (icompo = 0; icompo < cdo->NBaseComponent; icompo++) {
          vals += val->fs[j + icompo * m];
          vall += val->fl[j + icompo * m];
        }
        val->fs_sum[j] = vals;
        val->fl_sum[j] = vall;
      }

      if (val->fs_ibm) {
#pragma omp for
        for (j = 0; j < cdo->m; ++j) {
          vals_ibm = 0.0;
          for (icompo = 0; icompo < cdo->NBaseComponent; ++icompo) {
            if (phv->comps[icompo].sform == SOLID_FORM_IBM) {
              vals_ibm += val->fs[j + icompo * m];
            }
          }
          val->fs_ibm[j] = vals_ibm;
        }
      }
    } else {
#pragma omp for
      for (j = 0; j < cdo->m; j++) {
        val->fl_sum[j] = val->fl[j];
        val->fs_sum[j] = val->fs[j];
        val->t_pre[j] = val->t[j];
      }
    }

#pragma omp for
    for (j = 0; j < prm->cdo->m; j++) {
      val->work[j] = val->fs_sum[j] + val->fl_sum[j];
    }
  }

  if(flg->multi_layer==ON) define_fl_layer_from_fl(val,prm);

  if(flg->multi_layer==OFF){
    // Level Set (solid -(liquid+gas))
    Level_Set(1, 20, val->ls, val->fs_sum, prm);
    bcf(val->ls, prm);
    // Level Set (liquid -(solid+gas))
    Level_Set(1, 20, val->ll, val->fl_sum, prm);
    // Level Set (solid A )
    Level_Set(1, 20, val->lls, val->work, prm);
    bcf(val->ll, prm);
    bcf(val->lls, prm);    
  }else{
    /* -- Multi_layerモデル -- */
    // same process is in restart()
    // ls is NOT defined layer-wise
    Level_Set(1, 20, val->ls, val->fs_sum, prm);
    bcf(val->ls, prm);

    // ll and lls are layer-wise
    for(int ilayer= 0; ilayer<cdo->NumberOfLayer; ilayer++){
      Level_Set(1, 20, &val->ll_layer[ilayer*m] , &val->fl_layer[ilayer*m] , prm);
      bcf(&val->ll_layer[ilayer*m], prm);

      Level_Set(1, 20, &val->lls_layer[ilayer*m], &val->fls_layer[ilayer*m], prm);
      bcf(&val->lls_layer[ilayer*m], prm);    
    }
      
  }


  // Level Set ((gas+liquid+non-IBM solid) - IBM solid)
  if (val->ls_ibm && val->fs_ibm) {
    Level_Set(1, 20, val->ls_ibm, val->fs_ibm, prm);
    bcf(val->ls_ibm, prm);
  }

  // Initial normal vector
  normal_vector_cell(val->nvsx, val->nvsy, val->nvsz,      NULL, val->ls, cdo);
  normal_vector_cell(val->nvlx, val->nvly, val->nvlz, val->curv, val->lls, cdo);
  //normal_vector_cell(val->nvlx, val->nvly, val->nvlz, val->curv, val->ll, cdo);
  if (val->ls_ibm && val->nvibmx && val->nvibmy && val->nvibmz) {
    normal_vector_cell(val->nvibmx, val->nvibmy, val->nvibmz, NULL, val->ls_ibm, cdo);
  }

  /* Yamashita, 2020/6/9 */
  //bcf_VOF(0, val->eps, val, prm);
  bcf(val->eps, prm);
  bcf(val->perm, prm);

  if (flg->oxidation == ON) {
    int r;
    type *Y;
    type *Yt;
    type *fs;
    type *fl;

    if (flg->solute_diff == ON) {
      Y = val->Y;
      Yt = val->fs;
      fs = val->fs;
      fl = val->fl;
    } else {
      Y = val->fs;
      Yt = val->fs_sum;
      fs = val->fs_sum;
      fl = val->fl_sum;
    }

    r = oxidation_init(prm, prm->cdo, Y, Yt, fs, fl, cdo->NumberOfComponent,
                       &cdo->ox_zry, &cdo->ox_zro2, &cdo->ox_h2o, val->ox_lset,
                       val->ox_vof, val->ox_flag, val->ox_f_h2o,
                       cdo->ox_h2o_threshold, val->ox_lset_h2o,
                       val->ox_lset_h2o_s);
    if (r) {
      prm->status = ON;
    }
  }

  materials(mtl, val, prm); //Y!

  /* YSE: Initialize only if flg->geom_in is set to OFF. */
  // velocity
  if (flg->geom_in == OFF) {
#ifndef NDEBUG
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_DEBUG,
               __func__, "Initializing Velocity by init_vel");
#endif
    init_vel(val->u, val->v, val->w, prm);
  }
  bcu(val->u, val->v, val->w, val, mtl, prm);

  // temperature
  if (flg->geom_in == OFF) {
#ifndef NDEBUG
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_DEBUG,
               __func__, "Initializing Temperature by init_temp");
#endif
    init_temp(val, prm);
  }
  bct(val->t, val, prm);

  /*initial t_pre*///added by chai
  for(j = 0; j < cdo->m; j++) {
    val->t_pre[j] = val->t[j];//added by chai
  }

  if (flg->solute_diff == ON) {//added by Chai
    init_partial_volume(val, mtl, prm);//added by Chai
    init_Vf(val, mtl, prm);//added by Chai
    set_diff_func(val, mtl, prm);//added by Chai
    bcs(val->Y, val->Vf, val, prm, mtl);
  }

  if (flg->phase_change == ON && val->mushy) {
    for (j = 0; j < cdo->m; j++) {
      val->mushy[j] = -1; /* 0 is "inside of mushy zone" */
    }
  }

  /* Initialize porous model parameters for two energy model */
  if (flg->porous == ON && flg->two_energy == ON) {
    type r_tmp, m_tmp, c_tmp, k_tmp, Re, Pr;
    cdo->a_sf = sqrt(2.0)*PI/cdo->d_p; // Menshin
    //cdo->a_sf = 3.0*sqrt(3.0)*M_PI/cdo->d_p/4.0; // Taishin
    r_tmp = tempdep_calc(&phv->rho_g, 293.15);
    m_tmp = tempdep_calc(&phv->mu_g, 293.15);
    c_tmp = tempdep_calc(&phv->specht_g, 293.15);
    k_tmp = tempdep_calc(&phv->thc_g, 293.15);
    Re = cdo->U_ref*cdo->L_ref*r_tmp/m_tmp;
    Pr = c_tmp*m_tmp/k_tmp;
    //printf("%lf %lf\n", Re, Pr);
    cdo->h_f = (2.0 + 1.1*pow(Pr,1./3.)*pow(Re,0.6))*k_tmp/cdo->d_p;
    //printf("%lf\n", cdo->h_f);
    for(j = 0; j < cdo->m; j++) {
      val->ts[j] = val->t[j];
      val->tf[j] = val->t[j];
    }
    bct(val->ts, val, prm);
    bct(val->tf, val, prm);
  }

  /* Initialize LPT */
#ifdef HAVE_LPT
  lpt_set_jupiter_error_function();
  if (flg->lpt_calc == ON) {
    init_lpt(0, flg, cdo, prm->mpi, val, &prm->status);
    lpt_send_constant_field_vars(cdo, val, &prm->status);
  }
#endif
  /* Make sure to be consistent among processes */
  update_level_set_flags_share_flag(&flg->update_level_set_ll, prm->mpi);
  update_level_set_flags_share_flag(&flg->update_level_set_ls, prm->mpi);
  update_level_set_flags_share_flag(&flg->update_level_set_lls, prm->mpi);

  /* Test for update is needed */
  {
    update_level_set_flags *flags[] = {
      &flg->update_level_set_lls,
      // &flg->update_level_set_ll,
    };
    const int nflags = sizeof(flags) / sizeof(flags[0]);

    update_level_set_flags_mark_if_fl_exists(val->fl_sum, cdo, prm->mpi,
                                             nflags, flags,
                                             UPDATE_LEVEL_SET_BY_INITIAL_VOF);
  }

  /* YSE: Print out the initialization result in ASCII-Art. */
  /*-
   * YSE: If some errors occured and output destination is terminal,
   *      skip printing.
   */
  if (for_all_rank(prm->mpi, flg->print == ON && flg->fp &&
                   !((flg->fp == stdout || flg->fp == stderr) &&
                     prm->status == ON))) {
    char *buf;
    int ret;
    type min;
    type max;

    if (flg->solute_diff == ON) {
      dumb_visualizer(prm, val->fs,
                      cdo->mx, cdo->my, cdo->mz,
                      cdo->stm, cdo->stp, cdo->x, cdo->y, cdo->z,
                      0.0, 1.0, "Solid VOF", "xyz");

      dumb_visualizer(prm, val->fl,
                      cdo->mx, cdo->my, cdo->mz,
                      cdo->stm, cdo->stp, cdo->x, cdo->y, cdo->z,
                      0.0, 1.0, "Liquid VOF", "xyz");

      for (j = 0; j < cdo->NumberOfComponent; ++j) {
        ret = jupiter_asprintf(&buf, "Y, component %d", j);
        if (ret < 0) break;
        dumb_visualizer(prm, val->Y + j * cdo->m,
                        cdo->mx, cdo->my, cdo->mz,
                        cdo->stm, cdo->stp, cdo->x, cdo->y, cdo->z,
                        0.0, 1.0, buf, "xyz");
        free(buf);
      }

    } else {
      for (j = 0; j < cdo->NumberOfComponent; ++j) {
        ret = jupiter_asprintf(&buf, "Solid VOF, component %d", j);
        if (ret < 0) break;
        dumb_visualizer(prm, val->fs + j * cdo->m,
                        cdo->mx, cdo->my, cdo->mz,
                        cdo->stm, cdo->stp, cdo->x, cdo->y, cdo->z,
                        0.0, 1.0, buf, "xyz");
        free(buf);

        ret = jupiter_asprintf(&buf, "Liquid VOF, component %d", j);
        if (ret < 0) break;
        dumb_visualizer(prm, val->fl + j * cdo->m,
                        cdo->mx, cdo->my, cdo->mz,
                        cdo->stm, cdo->stp, cdo->x, cdo->y, cdo->z,
                        0.0, 1.0, buf, "xyz");
        free(buf);
      }
    }

    calc_min_max(prm->mpi, cdo, val->t, &min, &max);
    if (min == max) {
      if (min == 0.0) {
        max = 1.0;
      } else {
        max = min + min * 0.1;
      }
    }
    dumb_visualizer(prm, val->t,
                    cdo->mx, cdo->my, cdo->mz,
                    cdo->stm, cdo->stp, cdo->x, cdo->y, cdo->z,
                    min, max, "Temperature [K]", "xyz");

    calc_min_max(prm->mpi, cdo, val->u, &min, &max);
    if (min == max) {
      if (min == 0.0) {
        max = 1.0;
      } else {
        max = min + min * 0.1;
      }
    }
    dumb_visualizer(prm, val->u,
                    cdo->mx, cdo->my, cdo->mz,
                    cdo->stm, cdo->stp, cdo->x, cdo->y, cdo->z,
                    min, max, "Velocity U [m/s]", "xyz");

    calc_min_max(prm->mpi, cdo, val->v, &min, &max);
    if (min == max) {
      if (min == 0.0) {
        max = 1.0;
      } else {
        max = min + min * 0.1;
      }
    }
    dumb_visualizer(prm, val->v,
                    cdo->mx, cdo->my, cdo->mz,
                    cdo->stm, cdo->stp, cdo->x, cdo->y, cdo->z,
                    min, max, "Velocity V [m/s]", "xyz");

    calc_min_max(prm->mpi, cdo, val->w, &min, &max);
    if (min == max) {
      if (min == 0.0) {
        max = 1.0;
      } else {
        max = min + min * 0.1;
      }
    }
    dumb_visualizer(prm, val->w,
                    cdo->mx, cdo->my, cdo->mz,
                    cdo->stm, cdo->stp, cdo->x, cdo->y, cdo->z,
                    min, max, "Velocity W [m/s]", "xyz");

    if (val->qgeom) {
      calc_min_max(prm->mpi, cdo, val->qgeom, &min, &max);
      if (min == max) {
        if (min == 0.0) {
          max = 1.0;
        } else {
          max = min + min * 0.1;
        }
      }
      dumb_visualizer(prm, val->qgeom,
                      cdo->mx, cdo->my, cdo->mz,
                      cdo->stm, cdo->stp, cdo->x, cdo->y, cdo->z,
                      min, max, "Fixed Heat Source [W/m3]", "xyz");
    }

    if (val->lpt_pewall) {
      dumb_visualizer(prm, val->lpt_pewall,
                      cdo->mx, cdo->my, cdo->mz,
                      cdo->stm, cdo->stp, cdo->x, cdo->y, cdo->z, 0.0, 1.0,
                      "Wall restitution coefficient for LPT [-]", "xyz");
    }

    dumb_visualize_boundary(prm, prm->cdo, &val->bnd_B, cdo->nbx, cdo->nby,
                            "x", "y", "Bottom (Z-) boundary map");
    dumb_visualize_boundary(prm, prm->cdo, &val->bnd_T, cdo->nbx, cdo->nby,
                            "x", "y", "Top (Z+) boundary map");

    dumb_visualize_boundary(prm, prm->cdo, &val->bnd_S, cdo->nbx, cdo->nbz,
                            "x", "z", "South (Y-) boundary map");
    dumb_visualize_boundary(prm, prm->cdo, &val->bnd_N, cdo->nbx, cdo->nbz,
                            "x", "z", "North (Y+) boundary map");

    dumb_visualize_boundary(prm, prm->cdo, &val->bnd_W, cdo->nby, cdo->nbz,
                            "y", "z", "West (X-) boundary map");
    dumb_visualize_boundary(prm, prm->cdo, &val->bnd_E, cdo->nby, cdo->nbz,
                            "y", "z", "East (X+) boundary map");

    if (val->surface_bnd && val->bnd_norm_u && val->bnd_norm_v &&
        val->bnd_norm_w) {
      report_surface_boundary_area(prm->flg, prm->mpi,
                                   &cdo->surface_boundary_head,
                                   val->surface_bnd, val->bnd_norm_u,
                                   val->bnd_norm_v, val->bnd_norm_w, cdo->x,
                                   cdo->y, cdo->z, cdo->mx, cdo->my, cdo->mz,
                                   cdo->stm, cdo->stm, cdo->stm, cdo->stp,
                                   cdo->stp, cdo->stp);
    }
  }
#ifndef NDEBUG
  csvperrorf(__FILE__, __LINE__, 0, CSV_EL_DEBUG,
             __func__, "Initializing Completed");
#endif
}


////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Solute distribution, added 2017/09/22
int init_Y(variable *val, parameter *prm)
{
  domain *cdo = prm->cdo;
  flags *flg = prm->flg;
  int icompo, j, jx, jy, jz, m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz, stm=cdo->stm;
  int nx=cdo->nx, ny=cdo->ny, nz=cdo->nz;
  type xc,yc,zc;
  type *Y = val->Y, *Yt = val->Yt, *Y0 = val->Y0, *Y0_G = val->Y0_G;

  /*
  if(prm->flg->debug == OFF){
//--- debug == OFF ------------------------------------------------------------------------------

  } else {
  */
//--- debug == ON --------------------------------------------------------------------------------

    // test run 1  square box
    /*
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            j = jx + mx*jy + mx*my*jz;
            xc = 0.5*(cdo->x[jx] + cdo->x[jx+1]);
            yc = 0.5*(cdo->y[jy] + cdo->y[jy+1]);
            if(xc <= 0.25*cdo->gLx && yc <= 0.5*cdo->gLy) Y[j] = 1.0;
            if(xc > 0.25*cdo->gLx && xc <= 0.5*cdo->gLx && yc <= 0.5*cdo->gLy) Y[j+m] = 1.0;
          }
        }
      }
      for(icompo = 0; icompo < cdo->NumberOfComponent; icompo++){
        for(jz = 0; jz < nz; jz++) {
          for(jy = 0; jy < ny; jy++) {
            for(jx = 0; jx < nx; jx++) {
              j = jx+stm + mx*(jy+stm) + mx*my*(jz+stm);
              Y0[icompo] += Y[j+icompo*m];
            }
          }
        }
      }
      for(jz = 0; jz < nz; jz++) {
        for(jy = 0; jy < ny; jy++) {
          for(jx = 0; jx < nx; jx++) {
            j = jx+stm + mx*(jy+stm) + mx*my*(jz+stm);
            for(icompo = 0; icompo < cdo->NumberOfComponent; icompo++){
              Yt[j] += Y[j+icompo*m];
            }
          }
        }
      }
    */
    // test run 2 liquefaction SUS-B4C
    // SUS : 0, B4C : 1
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
        for(jx = 0; jx < mx; jx++) {
          j = jx + mx*jy + mx*my*jz;
          xc = 0.5*(cdo->x[jx] + cdo->x[jx+1]);
          if(xc <= 0.35*cdo->gLx ) {
            Y[j+m]   = 0.8; //B
            Y[j+2*m] = 0.2; //C
          }
          if(xc > 0.35*cdo->gLx && xc <= 0.7*cdo->gLx) Y[j] = 1.0; //SUS
        }
      }
    }
    //*/
    for(icompo = 0; icompo < cdo->NumberOfComponent; icompo++){
      for(jz = 0; jz < nz; jz++) {
        for(jy = 0; jy < ny; jy++) {
          for(jx = 0; jx < nx; jx++) {
            j = jx+stm + mx*(jy+stm) + mx*my*(jz+stm);
            Y0[icompo] += Y[j+icompo*m];
          }
        }
      }
    }
    /* YSE: Skip setting for Vf, but it should be done after init_Vf */
    bcs(Y,NULL,val,prm,NULL);
    bcf(Yt,prm);
    for(j=0;j<cdo->NumberOfComponent;j++) Y0_G[j] = Y0[j];
#ifdef JUPITER_MPI
    MPI_Allreduce(Y0, Y0_G, 2, MPI_TYPE, MPI_SUM, MPI_COMM_WORLD);
#endif
    for(j=0;j<cdo->NumberOfComponent;j++) Y0[j] = Y0_G[j];
    /*
  }
    */

  return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////////
//  Volume Of Fluid (solid & liquid)
int init_VOF(type *fl, type *fs, type *flg_obst_A, type *flg_obst_B, parameter *prm)
{
  domain *cdo = prm->cdo;
  flags *flg = prm->flg;
  int  j, jx, jy, jz, m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz, stm=cdo->stm;
  type xc,yc,zc,xr,yr,xr_init,yr_init,dxr,dyr,lx,ly,lz;
  int x_nMax, y_nMax;

  /*
  if(prm->flg->debug == OFF){
//--- debug == OFF ------------------------------------------------------------------------------
    type w_crb = 0.004; // Width of control rod blade
    type t_crb = 0.001; // Thickness of control rod blade
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
      j = jx + mx*jy + mx*my*jz;
      xc = 0.5*(cdo->x[jx] + cdo->x[jx+1]);
          yc = 0.5*(cdo->y[jy] + cdo->y[jy+1]);
          zc = 0.5*(cdo->z[jz] + cdo->z[jz+1]);

          if((xc-0.5*cdo->gLx)*(xc-0.5*cdo->gLx)+(yc-0.5*cdo->gLy)*(yc-0.5*cdo->gLy)+(zc-cdo->gLz)*(zc-cdo->gLz) <= 0.25*cdo->gLx*cdo->gLx)
            fs[j+m] = 1.0;
          if((xc-0.5*cdo->gLx)*(xc-0.5*cdo->gLx)+(yc-0.5*cdo->gLy)*(yc-0.5*cdo->gLy)+(zc-cdo->gLz)*(zc-cdo->gLz) <= 0.15*cdo->gLx*cdo->gLx)
            fs[j+m] = 0.0;
          //if((xc-0.5*cdo->gLx)*(xc-0.5*cdo->gLx)+(yc-0.5*cdo->gLy)*(yc-0.5*cdo->gLy) <= 0.2*0.2)
          // fs[j+m] = 0.0;
          if((xc-0.5*cdo->gLx)*(xc-0.5*cdo->gLx)+(yc-0.5*cdo->gLy)*(yc-0.5*cdo->gLy)+(zc-cdo->gLz)*(zc-cdo->gLz) <= 0.15*cdo->gLx*cdo->gLx)
            fl[j] = 1.0;

          //if(xc >= 0.3*cdo->gLx && xc <= 0.7*cdo->gLx && yc >= 0.3*cdo->gLy && yc <= 0.7*cdo->gLy && zc >= 0.3*cdo->gLz && zc <= 0.7*cdo->gLz)
          // fs[j+m] = 1.0; //SUS (CR blade)

        }
      }
    }
  } else {
  */
//--- debug == ON --------------------------------------------------------------------------------
    /*
    0: SUS
    1: B4C
    */
    type a,b,z_bottom;
    //y_ent = cdo->gLy - 1.086;
    cdo->y_ent = 0.15;
    //cdo->y_ent = 0.0;
    cdo->mass_before = 0.0;
    cdo->mass_after = 0.0;
    //a = (cdo->gLy - y_ent)/(cdo->gLx - 0.098);
    a = (1.086 - cdo->y_ent)/(cdo->gLx - 0.098);
    b = cdo->y_ent - 0.098*a;
    z_bottom = 0.1*cdo->gLz;

    // test run 1  square box
    /*
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
        for(jx = 0; jx < mx; jx++) {
        j = jx + mx*jy + mx*my*jz;
        xc = 0.5*(cdo->x[jx] + cdo->x[jx+1]);
        yc = 0.5*(cdo->y[jy] + cdo->y[jy+1]);
        if(xc <= 0.5*cdo->gLx && yc <= 0.5*cdo->gLy) fl[j] = 1.0;
        }
      }
    }
    */
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
        for(jx = 0; jx < mx; jx++) {
          j = jx + mx*jy + mx*my*jz;
          xc = 0.5*(cdo->x[jx] + cdo->x[jx+1]);
          if(xc <= 0.7*cdo->gLx) fs[j] = 1.0;
        }
      }
    }
    //*/
    /*
  }
    */
  return 0;
}

//-----------  velocity on x(and y)-direction
int init_vel(type *u, type *v, type *w, parameter *prm)
{
  domain *cdo = prm->cdo;
  int  j, jx, jy, jz, mx=cdo->mx, my=cdo->my, mz=cdo->mz;

#pragma omp parallel for private(jz,jy,jx,j)
  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*jy + mx*my*jz;
        u[j] = 1.e-4;
        v[j] = 1.e-4;
        w[j] = 1.e-4;
      }
    }
  }
  return 0;
}
//==============================  TEMPERATURE ========================================
//int init_temp(type *t, type *fl, type *fs, int *flg_obst, parameter *prm);
int init_temp(variable *val, parameter *prm)
{
  flags  *flg = prm->flg;
  domain *cdo = prm->cdo;
  phase_value *phv = prm->phv;
  int  j, jx, jy, jz, m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz;
  type *t=val->t, *fl=val->fl, *fs=val->fs, *ls = val->ls;
  type *flg_obst_A=val->flg_obst_A;
  type tr=prm->phv->tr, t_s=tr, t_l=tr, t_g=tr;
  type tz, xc, yc, zc, tz1, tz2, t_top, t_btm, zb, zm, a, b;

  if(cdo->icnt != -1) return 0;
  t_s = phv->sol_tmp;
  t_l = phv->liq_tmp; //1800
  t_g = phv->gas_tmp;

//-------- debug == OFF -------------------------------------------------------------
  if(flg->debug == OFF) {
/*
  0 : UO2
  1 : SUS
  2 : Zry
  3 : B4C
  4 : UO2-Zry (0-2)
  5 : SUS-B4C (1-3)
*/
    type t_top_UO2, t_btm_UO2, t_top_SUS, t_btm_SUS, a_UO2, a_SUS, T_UO2, T_SUS, b_UO2, b_SUS;
    //type T_UO2 = 2000.0;
    //type T_SUS = 1490.0;
    //type T_Zry = 1490.0;
    //type T_B4C = 1490.0;
    T_UO2 = 1795.0;
    T_SUS = 1573.0;
    //type T_Zry = 1790.0;
    //type T_B4C = 1490.0;
    type T_AIR = 1300.0;

    t_top_UO2 = T_UO2;
    t_top_SUS = T_SUS;
    t_btm_UO2 = 1775.0;
    t_btm_SUS = 1563.0;
    a_UO2 = (t_top_UO2 - t_btm_UO2)/cdo->gLz;
    a_SUS = (t_top_SUS - t_btm_SUS)/cdo->gLz;
    b = t_btm;
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
        for(jx = 0; jx < mx; jx++) {
          j = jx + mx*jy + mx*my*jz;
          xc = 0.5*(cdo->x[jx] + cdo->x[jx+1]);
          yc = 0.5*(cdo->y[jy] + cdo->y[jy+1]);
          zc = 0.5*(cdo->z[jz] + cdo->z[jz+1]);
          // 0: f->tm_liq[i] = 3140; UO2
          // 1: f->tm_liq[i] = 1500; SUS
          // 2: f->tm_liq[i] = 2128; Zry
          // 3: f->tm_liq[i] = 1500; B4C
          // 4: f->tm_liq[i] = 2963; ZrO2
          //T = a*yc + b;
          T_UO2 = a_UO2*zc + t_btm_UO2;
          T_SUS = a_SUS*zc + t_btm_SUS;
          t[j] = T_UO2*fs[j] + T_SUS*fs[j+m] + T_UO2*fs[j+2*m] + T_SUS*fs[j+3*m]
            + T_AIR*(1.0-fs[j]-fl[j]-fs[j+m]-fl[j+m]-fs[j+2*m]-fl[j+2*m]-fs[j+3*m]-fl[j+3*m]);

          // t[j] = 2500.0*fs[j] + 1490.0*fs[j+m] + 1800.0*fs[j+2*m] + 1490.0*fs[j+3*m]
          // + 300.0*(1.0-fs[j]-fl[j]-fs[j+m]-fl[j+m]-fs[j+2*m]-fl[j+2*m]-fs[j+3*m]-fl[j+3*m]);
        }
      }
    }
  }else{
//-------- debug == ON -------------------------------------------------------------
    type z_bottom = 0.1*cdo->gLz;
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
        for(jx = 0; jx < mx; jx++) {
          j = jx + mx*jy + mx*my*jz;
          xc = 0.5*(cdo->x[jx] + cdo->x[jx+1]);
          yc = 0.5*(cdo->y[jy] + cdo->y[jy+1]);
          zc = 0.5*(cdo->z[jz] + cdo->z[jz+1]);
          // test run 1 square box
          t[j] = 1460.0*fs[j] + 500.0*fl[j]
            + 300.0*(1.0-fs[j]-fl[j]);

        }
      }
    }
  }
  return 0;
}

int set_fuel_rod(int flag, type R, type value, type xr, type yr, type zr_b, type zr_t, type *fs, type *flg_obst, parameter *prm)
{
  domain *cdo = prm->cdo;
  //flags *flg = prm->flg;
  int  j, jx, jy, jz, m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz;
  type xc,yc,zc,theta,r;

#pragma omp parallel for private(jz,jy,jx,j,xc,yc,zc,r,theta)

  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*jy + mx*my*jz;
        xc = 0.5*(cdo->x[jx] + cdo->x[jx+1]);
        yc = 0.5*(cdo->y[jy] + cdo->y[jy+1]);
        zc = 0.5*(cdo->z[jz] + cdo->z[jz+1]);
        r = sqrt(xc*xc + yc*yc);
        theta = acos(xc/r);

        if(flag == 1){
          if( ((xc-xr)*(xc-xr) + (yc-yr)*(yc-yr) <= R*R) && zc >= zr_b && zc <= zr_t) { 
            fs[j] += value;
            flg_obst[j] = 1.0;
          }
        } else if(flag == 2){
          if( ((xc-xr)*(xc-xr) + (yc-yr)*(yc-yr) <= R*R) && zc >= zr_b && zc <= zr_t) { 
            fs[j] = value;
            flg_obst[j] = 0.0;
          }
        }else{
          if( ((xc-xr)*(xc-xr) + (yc-yr)*(yc-yr) <= R*R) && zc >= zr_b && zc <= zr_t) { 
            fs[j] = 0.0;
            flg_obst[j] = 0.0;
          }
        }
      }
    }
  }
  return 0;
}

int set_fuel_channel(int flag, type lx, type ly, type xr, type yr, type zr_b, type zr_t, type *fs, type *flg_obst, parameter *prm)
{
  domain *cdo = prm->cdo;
  //flags *flg = prm->flg;
  int  j, jx, jy, jz, mx=cdo->mx, my=cdo->my, mz=cdo->mz;
  type xs,ys,xe,ye,xc,yc,zc;

#pragma omp parallel for private(jz,jy,jx,j,xc,yc,zc,xs,ys,xe,ye)

  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*jy + mx*my*jz;
        xc = 0.5*(cdo->x[jx] + cdo->x[jx+1]);
        yc = 0.5*(cdo->y[jy] + cdo->y[jy+1]);
        zc = 0.5*(cdo->z[jz] + cdo->z[jz+1]);
        xs = xr - 0.5*lx;
        ys = yr - 0.5*ly;
        xe = xr + 0.5*lx;
        ye = yr + 0.5*ly;
        if( xc >= xs && xc <= xe && yc >= ys && yc <= ye && zc >= zr_b && zc <= zr_t){
          fs[j] = 1.0;
          flg_obst[j] = 1.0;
        }
      }
    }
  }
  return 0;
}

int set_fuel_rod_orifice(int flag, type R_in, type R_out, type value, type xr, type yr, type zr_b, type zr_t, type *fs, type *flg_obst, parameter *prm)
{
  domain *cdo = prm->cdo;
  //flags *flg = prm->flg;
  int  j, jx, jy, jz, m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz;
  type xc,yc,zc;

#pragma omp parallel for private(jz,jy,jx,j,xc,yc,zc)

  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*jy + mx*my*jz;
        xc = 0.5*(cdo->x[jx] + cdo->x[jx+1]);
        yc = 0.5*(cdo->y[jy] + cdo->y[jy+1]);
        zc = 0.5*(cdo->z[jz] + cdo->z[jz+1]);
        if( ((xc-xr)*(xc-xr) + (yc-yr)*(yc-yr) <= R_in*R_in) && zc >= zr_b && zc <= zr_t) {
          fs[j] = value;
          flg_obst[j] = 1.0;
        }
        if(zc >= 1.58*zr_b && zc <= 0.85*zr_t){
          if( ((xc-xr)*(xc-xr) + (yc-yr)*(yc-yr) <= 1.1*R_out*R_out) && xc >= xr - 2.0*cdo->dx && xc <= xr + 2.0*cdo->dx ){
            fs[j] = value;
            flg_obst[j] = 1.0;
          }
          if( ((xc-xr)*(xc-xr) + (yc-yr)*(yc-yr) <= 1.1*R_out*R_out) && yc >= yr - 2.0*cdo->dy && yc <= yr + 2.0*cdo->dy ){
            fs[j] = value;
            flg_obst[j] = 1.0;
          }
        }
      }
    }
  }
  return 0;
}

static int
init_boundary_for(int nbx, int nby, int bc, int bct,
                  controllable_type *tw,
                  struct vin_data *vinp, struct pout_data *poutp,
                  int neighbor_rank, domain *cdo,
                  fluid_boundary_data *fhead, thermal_boundary_data *thead,
                  struct boundary_array *bndp)
{
  int fl_used, th_used;
  fluid_boundary_data *fl_data;
  thermal_boundary_data *th_data;
  int i, j;
  int ret;

  if (neighbor_rank != -1) {
    fl_data = fhead;
    th_data = thead;
  } else {
    fl_data = fluid_boundary_data_new(&cdo->fluid_boundary_head);
    th_data = thermal_boundary_data_new(&cdo->thermal_boundary_head);

    if (fl_data) {
      fl_data->cond = bc;
      if (bc == INLET) {
        controllable_type_copy(&fl_data->inlet_vel_u, &vinp->u);
        controllable_type_copy(&fl_data->inlet_vel_v, &vinp->v);
        controllable_type_copy(&fl_data->inlet_vel_w, &vinp->w);
        controllable_type_remove_from_list(&vinp->u);
        controllable_type_remove_from_list(&vinp->v);
        controllable_type_remove_from_list(&vinp->w);
        if (vinp->comps) {
          fl_data->comps = inlet_component_data_dup(vinp->comps);
          if (!fl_data->comps) {
            fluid_boundary_data_delete(fl_data);
            fl_data = NULL;
          }
          inlet_component_data_remove_from_list(vinp->comps);
        }
      }
      if (bc == OUT) {
        fl_data->out_p_cond = poutp->cond;
        controllable_type_copy(&fl_data->const_p, &poutp->const_p);
        controllable_type_remove_from_list(&poutp->const_p);
      }
    }

    if (th_data) {
      th_data->cond = bct;
      if (bct == ISOTHERMAL) {
        th_data->control = TRIP_CONTROL_CONTROL;
        controllable_type_copy(&th_data->temperature, tw);
        controllable_type_remove_from_list(tw);
      } else if (bct == DIFFUSION) {
        th_data->diffusion_limit = tw->current_value;
      }
    }
  }

  fl_used = 0;
  th_used = 0;

  if (neighbor_rank != -1) {
    fl_used = 1;
    th_used = 1;

#pragma omp parallel for collapse(2)
    for (j = 0; j < nby; ++j) {
      for (i = 0; i < nbx; ++i) {
        ptrdiff_t jj;

        jj = calc_address(i, j, 0, nbx, nby, 1);
        bndp->fl[jj] = fl_data;
        bndp->th[jj] = th_data;
      }
    }
  } else {
#pragma omp parallel for collapse(2)
    for (j = 0; j < nby; ++j) {
      for (i = 0; i < nbx; ++i) {
        ptrdiff_t jj;

        jj = calc_address(i, j, 0, nbx, nby, 1);
        if (bndp->fl[jj] == fhead) {
          if (fl_data) {
            bndp->fl[jj] = fl_data;
          }
#pragma omp atomic write
          fl_used = 1;
        }
        if (bndp->th[jj] == thead) {
          if (th_data) {
            bndp->th[jj] = th_data;
          }
#pragma omp atomic write
          th_used = 1;
        }
      }
    }
  }

  ret = 0;
  if ((fl_used && !fl_data) || (th_used && !th_data)) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    ret = 1;
  }
  if (!fl_used) {
    fluid_boundary_data_delete(fl_data);
  }
  if (!th_used) {
    thermal_boundary_data_delete(th_data);
  }

  return ret;
}

int init_boundary(mpi_param *mpi, domain *cdo, flags *flg, variable *val)
{
  int r;

  CSVASSERT(mpi);
  CSVASSERT(cdo);
  CSVASSERT(val);
  CSVASSERT(flg);

  /* Fill boundary data with specified by cdo and flg */
  r = init_boundary_for(cdo->nbx, cdo->nby, flg->bc_zm, flg->bct_zm,
                        &cdo->tw_zm, &cdo->vin_zm, &cdo->p_zm, mpi->nrk[0], cdo,
                        &cdo->fluid_boundary_head, &cdo->thermal_boundary_head,
                        &val->bnd_B);
  if (r) goto error;

  r = init_boundary_for(cdo->nbx, cdo->nby, flg->bc_zp, flg->bct_zp,
                        &cdo->tw_zp, &cdo->vin_zp, &cdo->p_zp, mpi->nrk[1], cdo,
                        &cdo->fluid_boundary_head, &cdo->thermal_boundary_head,
                        &val->bnd_T);
  if (r) goto error;

  r = init_boundary_for(cdo->nbx, cdo->nbz, flg->bc_ym, flg->bct_ym,
                        &cdo->tw_ym, &cdo->vin_ym, &cdo->p_ym, mpi->nrk[2], cdo,
                        &cdo->fluid_boundary_head, &cdo->thermal_boundary_head,
                        &val->bnd_S);
  if (r) goto error;

  r = init_boundary_for(cdo->nbx, cdo->nbz, flg->bc_yp, flg->bct_yp,
                        &cdo->tw_yp, &cdo->vin_yp, &cdo->p_yp, mpi->nrk[3], cdo,
                        &cdo->fluid_boundary_head, &cdo->thermal_boundary_head,
                        &val->bnd_N);
  if (r) goto error;

  r = init_boundary_for(cdo->nby, cdo->nbz, flg->bc_xm, flg->bct_xm,
                        &cdo->tw_xm, &cdo->vin_xm, &cdo->p_xm, mpi->nrk[4], cdo,
                        &cdo->fluid_boundary_head, &cdo->thermal_boundary_head,
                        &val->bnd_W);
  if (r) goto error;

  r = init_boundary_for(cdo->nby, cdo->nbz, flg->bc_xp, flg->bct_xp,
                        &cdo->tw_xp, &cdo->vin_xp, &cdo->p_xp, mpi->nrk[5], cdo,
                        &cdo->fluid_boundary_head, &cdo->thermal_boundary_head,
                        &val->bnd_E);
  if (r) goto error;

  /* Mark to update level-set ll(s) if liquid inlet exists */
  {
    update_level_set_flags *flags[] = {
      &flg->update_level_set_lls,
      // &flg->update_level_set_ll
    };
    int nflags = sizeof(flags) / sizeof(flags[0]);

    update_level_set_flags_mark_if_liquid_inlet_exists(
      &cdo->fluid_boundary_head, nflags, flags,
      UPDATE_LEVEL_SET_BY_LIQUID_INLET);
  }

  return 0;

error:
  return 1;
}



/*2019, added by Chai until the end of this page*/
void init_Vf(variable *val, material *mtl, parameter *prm)
{
  domain      *cdo = prm->cdo;
  int  icompo, j, jx, jy, jz, m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;
  int  NumCompo=cdo->NBaseComponent;
  type *Y = val->Y, *Vf = val->Vf;
  type *Vp= mtl->Vp;
  int base;
  type temp, temp2;
  type *fls=val->fls;


  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*jy + mxy*jz;
        if (val->fls[j]==0.0) continue;
        temp = 0.0;
        temp2= 0.0;
        base = -1;

        for (icompo = 0; icompo < NumCompo; icompo++) {
          if (Y[j + m*icompo] == 0.0) continue;

          base    = icompo; //find base compo that not equal to zero;
          break;
        }
        if (base < 0) {
          /* All Ys are zero. */
          for (icompo = 0; icompo < NumCompo; icompo++) {
            Vf[j + m * icompo] = 0.0;
          }
        } else {
          type vpbase;
          vpbase = Vp[base] * Y[j + m * base];
          if (vpbase > 0.0) {
            for (icompo = 0; icompo < NumCompo; icompo++) {
              temp = temp + (Vp[icompo] * Y[j + m*icompo]) / vpbase;
            }
          }
          /* Multiplication may also cause underflow, letting 0. */
          if (temp > 0.0) {
            Vf[j + m * base] = 1.0/temp;
          } else {
            Vf[j + m * base] = 0.0;
          }

          for (icompo = 0; icompo < NumCompo; icompo++) {
            temp2 = (Vp[icompo] * Y[j + m*icompo]) / (Vp[base] * Y[j + m*base]);
            Vf[j + m*icompo] = Vf[j + m*base] * temp2;
          }
        }
      }
    }
  }
}

void init_partial_volume (variable *val, material *mtl, parameter *prm)
{
  /*initialation of the partial volume based on the material properties*/
  phase_value *phv = prm->phv;
  domain      *cdo = prm->cdo;
  type *Vp= mtl->Vp;
  int  m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;
  int  NumCompo=cdo->NBaseComponent;
  type rho_sum;
  int icompo;

#pragma omp parallel
  {
    int icompo;
    int jx, jy, jz;
    type lrho_sum;

    for (icompo = 0; icompo < NumCompo; icompo++) {
      type rho_ave;
      lrho_sum = 0.0;

#pragma omp for collapse(3)
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            type temp;
            int j;

            j = jx + mx*jy + mxy*jz;
            temp = val->t[j];
            lrho_sum = lrho_sum + tempdep_calc(&phv->comps[icompo].rho_s, temp);
          }
        }
      }

#pragma omp single
      {
        rho_sum = 0.0;
      }
#pragma omp atomic update
      rho_sum += lrho_sum;

#pragma omp barrier
#pragma omp master
      {
        Vp[icompo] = rho_sum;
      }
    }
  }

#ifdef JUPITER_MPI
  MPI_Allreduce(MPI_IN_PLACE, Vp, NumCompo, MPI_TYPE, MPI_SUM,
                prm->mpi->CommJUPITER);
#endif

  for (icompo = 0; icompo < NumCompo; ++icompo) {
    type rho_ave, molar_mass;

    molar_mass = phv->comps[icompo].molar_mass;
    rho_ave = Vp[icompo] / cdo->gm;
    Vp[icompo] = molar_mass * 0.001 / rho_ave;
  }
}

/*initial the coefficients in diffusion equaiton*/
void set_diff_func(variable *val, material *mtl, parameter *prm)
{
  domain      *cdo = prm->cdo;
  int  NumCompo=cdo->NBaseComponent;
  struct dc_calc_param *pp;
  type Dg_max;
  phase_value *phv;

  Dg_max = 0.0;
  phv = prm->phv;

  if (for_all_rank(prm->mpi, !phv->diff_params)) {
    val->Dg_max = 0.0;
    return;
  }

#pragma omp parallel
  {
    int  icompo, m_index;//binary index
    int  mx, my, mz, m;
    int  i,  j,  k;
    type Dg_max_local;
    type Dg_max_save;

    mx = cdo->mx;
    my = cdo->my;
    mz = cdo->mz;
    m  = cdo->m;

#pragma omp atomic read
    Dg_max_local = Dg_max;
    Dg_max_save = Dg_max_local;

#pragma omp for collapse(5)
    for (k = 0; k < mz; ++k) {
      for (j = 0; j < my; ++j) {
        for (i = 0; i < mx; ++i) {
          for(icompo = 0; icompo < NumCompo; icompo++){
            for(m_index = 0; m_index < NumCompo; m_index++){
              struct dc_calc_param *p;
              ptrdiff_t jj;
              ptrdiff_t jd;
              type t;
              type Y1, Y2;
              type d;

              jd = dc_calc_binary_address(icompo, m_index, NumCompo);
              if (jd < 0) continue;

              jj = calc_address(i, j, k, mx, my, mz);
              t = val->t[jj];

              p = phv->diff_params[jd];
              Y1 = val->Y[jj +  icompo * m];
              Y2 = val->Y[jj + m_index * m];

              if (p) {
                d = dc_calc(p, Y1, Y2, t);
              } else {
                d = 0.0;
              }
              if (Dg_max_local < d) {
                Dg_max_local = d;
              }
            }
          }
        }
      }
    }

    if (Dg_max_save < Dg_max_local) {
#pragma omp critical
      {
        if (Dg_max < Dg_max_local) {
          Dg_max = Dg_max_local;
        }
      }
    }
  }

#ifdef MPI
  MPI_Allreduce(MPI_IN_PLACE, &Dg_max, 1, MPI_TYPE, MPI_MAX, prm->mpi->CommJUPITER);
#endif
  val->Dg_max = Dg_max;
}

struct jupiter_lpt_error_handler_data {
  int stat;
};
static struct jupiter_lpt_error_handler_data lpt_err_handler_data;

static
void jupiter_lpt_error_handler(void *arg, int ierr, int len, const char *msg)
{
  lpt_err_handler_data.stat = ON;
  csvperrorf("LPT error", 0, 0, CSV_EL_ERROR, NULL, "%.*s", len, msg);
}

void lpt_set_jupiter_error_function(void)
{
#ifdef LPT
  cLPTset_error_callback(&jupiter_lpt_error_handler, NULL);
#endif
}

void lpt_send_constant_field_vars(domain *cdo, variable *val, int *stat)
{
  int istat;

  CSVASSERT(cdo);
  CSVASSERT(val);

#ifdef LPT
  istat = 0;
  cLPTsetfield_r(LPT_FV_PEWALL, val->lpt_pewall, cdo->nx, cdo->ny, cdo->nz,
                 cdo->stm, cdo->stp, NULL, 0, &istat);
  if (istat != 0) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
               "Failed to transfer pEwall data to LPT module");
    if (stat) *stat = ON;
    return;
  }
#endif
}

#ifdef LPT
static void init_lpt_impl(int restart, flags *flg, domain *cdo, mpi_param *mpi,
                          variable *val, int *stat)
{
  struct geom_list *lp, *ln, *lh;
  int npset, ipset, istat;
  ptrdiff_t npttot;
  int i;

  CSVASSERT(mpi);
  CSVASSERT(cdo);
  CSVASSERT(sizeof(ptrdiff_t) <= (size_t)INT_MAX);

#ifdef JUPITER_LPT_USE_MPI
  cLPTsetmpicomm(mpi->CommJUPITER);
  cLPTsetmpineighbors(mpi->nrk[0], mpi->nrk[1], mpi->nrk[2], mpi->nrk[3],
                      mpi->nrk[4], mpi->nrk[5]);
#endif

  istat = 0;
  npset = 0;
  npttot = 0;
  lh = &cdo->lpt_particle_set_head.list;
  geom_list_foreach(lp, lh) {
    struct particle_set_input *p;
    p = particle_set_input_entry(lp);

    if (npset + 1 < npset || npttot + p->set.nistpt < npttot) {
      if (stat) *stat = ON;
      break;
    }
    npset += 1;
    npttot += p->set.nistpt;
  }
  cLPTsetnpset(npset, &istat);
  if (for_any_rank(mpi, istat != 0)) {
    if (stat != 0) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                 "Could not set the number of particle sets to %d", npset);
    }
    if (stat) *stat = ON;
    return;
  }

  ipset = 0;
  geom_list_foreach(lp, lh) {
    struct particle_set_input *p;
    p = particle_set_input_entry(lp);
    if (ipset >= npset) {
      break;
    }
    ipset += 1;
    cLPTsetpsetm(ipset, &p->set);
  }

  if (flg->lpt_wbcal == ON) {
    cLPTsetwbcal(1);
  } else {
    cLPTsetwbcal(0);
  }

  for (i = 1; i < 3; i++) {
    cLPTalloc(i, npttot, cdo->nx, cdo->ny, cdo->nz, &istat);
    if (for_any_rank(mpi, istat != 0)) {
      if (istat != 0) {
        csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                   "Cannot allocate LPT memory for sets %d", istat);
      }
      if (stat) *stat = ON;
      return;
    }
  }

  /* Let open file only for MPI rank 0 */
  if (mpi->rank != 0 || !cdo->lpt_outname || cdo->lpt_outinterval <= 0.0) {
    int iunit;
    iunit = cLPTget_output_unit();
    cLPTopenlogfile(iunit, 0, NULL, 0, &istat);
    if (istat != 0) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_WARN, NULL,
                 "Could not set the unit number of LPT output to %d", iunit);
    }
    istat = 0;
    if (for_any_rank(mpi, 0)) { /* response to (2) */
      if (stat) *stat = ON;
      return;
    }
  } else {
    char *dir;
    int dir_alloc;
    errno = 0;
    dir_alloc = extract_dirname_allocate(&dir, cdo->lpt_outname);
    if (dir_alloc >= 0) {
      istat = make_directory_recursive(dir);
      if (istat == 0) {
        errno = 0;
        cLPTopenlogfile(-1, strlen(cdo->lpt_outname) + 1, cdo->lpt_outname,
                        restart, &istat);
        if (istat != 0) {
          csvperror(cdo->lpt_outname, 0, 0, CSV_EL_ERROR, NULL,
                    CSV_ERR_FOPEN, errno, 0, NULL);
        }
      } else {
        csvperror("Making directory", 0, 0, CSV_EL_ERROR, dir, CSV_ERR_SYS,
                  errno, 0, NULL);
      }
      free(dir);
    } else {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM,
                errno, 0, NULL);
      istat = 1;
    }
    if (for_any_rank(mpi, istat != 0)) { /* (2) */
      if (stat) *stat = ON;
      return;
    }
  }

  cLPTsetipttim(flg->lpt_ipttim);
  if (restart) {
    /* To avoid printing message */
    cLPTsettip(0.0);
  } else {
    cLPTsettip(cdo->lpt_outinterval);
  }
  cLPTsettimprn(cdo->time);

  cLPTcal0(cdo->x, cdo->y, cdo->z, cdo->time,
           cdo->nx, cdo->ny, cdo->nz, cdo->stm, cdo->stp, &istat);
  if (for_any_rank(mpi, istat != 0)) {
    if (istat != 0) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                 "Failed to initialize LPT module");
    }
    if (stat) *stat = ON;
    return;
  }

  cLPTsettip(cdo->lpt_outinterval);
}
#endif // LPT

#ifdef LPTX
static void init_lptx_impl(int restart, flags *flg, domain *cdo, mpi_param *mpi,
                           variable *val, int *stat)
{
  struct geom_list *lp, *ln, *lh;
  LPTX_particle_set *set;

  if (val->lpt_param)
    LPTX_param_delete(val->lpt_param);

  val->lpt_param = LPTX_param_new();
  if (!val->lpt_param) {
    csvperror(__FILE__, 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0, NULL);
    if (stat)
      *stat = ON;
    return;
  }

  if (jLPTX_param_set(val->lpt_param, flg, cdo, mpi)) {
    if (stat)
      *stat = ON;
  }

  lh = &cdo->lpt_particle_set_head.list;
  geom_list_foreach_safe (lp, ln, lh) {
    struct particle_set_input *ip;
    ip = particle_set_input_entry(lp);
    if (ip->set)
      LPTX_particle_init_set_append(val->lpt_param, ip->set);

    geom_list_delete(&ip->list);
    free(ip);
  }

  if (mpi->rank == 0) {
    set = LPTX_particle_set_from_init_set(val->lpt_param, NULL);
    if (set)
      LPTX_particle_set_append(val->lpt_param, set);
  }
}
#endif

void init_lpt(int restart, flags *flg, domain *cdo, mpi_param *mpi,
              variable *val, int *stat)
{
#ifdef LPT
  init_lpt_impl(restart, flg, cdo, mpi, val, stat);
#endif

#ifdef LPTX
  init_lptx_impl(restart, flg, cdo, mpi, val, stat);
#endif
}

void post_initial_check(parameter *prm, variable *val, material *mtl)
{
  if (prm->flg->restart < 0) {
    if (prm->flg->validate_VOF_init == ON) {
      validate_VOF_init(prm->mpi, prm->cdo, prm->flg,
                        prm->flg->validate_VOF_print_max, val->Y, val->fs,
                        val->fl, &prm->status);
    }

    if (for_any_rank(prm->mpi, prm->status == ON)) {
      prm->status = ON;
    }
  }
}
