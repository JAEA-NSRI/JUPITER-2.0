#include "component_info.h"
#include "csv.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef GPU
#  include <cuda.h>
#  include <cutil.h>
#endif

#include "struct.h"
/* YSE: Add Utility functions */
#include <errno.h>
#include "func.h"
#include "csvutil.h"
#include "dccalc.h"
/* YSE: end */

#ifdef LPTX
#include "lptx/param.h"
#endif

/*
 * YSE: Add common malloc error handler to avoid abnormal termination
 *      of program such as segmentation fault.
 */
static void *malloc_handler(void *aloc, const char* f, long l)
{
  if (!aloc) {
    csvperror(f, l, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0, NULL);
#ifdef JUPITER_MPI
    MPI_Abort(MPI_COMM_WORLD, 100);
#endif
    exit(EXIT_FAILURE);
  }
  return aloc;
}

/*
 * YSE: substitute malloc-family functions to call error handler
 *      Note: These definitions may be non-compliant to DCL37-C.
 */
#define malloc(s) malloc_handler((errno = 0, malloc(s)), __FILE__, __LINE__)
#define calloc(s, c) malloc_handler((errno = 0, calloc((s), (c))), __FILE__, __LINE__)
#define realloc(p, s) malloc_handler((errno = 0, realloc((p), (s))), __FILE__, __LINE__)
/* YSE: end */

phase_value *malloc_phase_value(domain *cdo, flags *flg)
{
  phase_value *f;
  int icompo;

  /* YSE: Use calloc to clear the content of structure */
  f = (phase_value *) calloc( sizeof(phase_value), 1 );
  /* YSE: return if we could not allocate the memory */
  if (!f) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL,
              __func__, CSV_ERR_NOMEM, 0, 0, NULL);
    return NULL;
  }

  if (cdo->NIComponent > 0) {
    size_t sz;
    int ncompo;

    ncompo = cdo->NIComponent;
    sz = sizeof(phase_value_component) * ncompo;

    f->comps = (phase_value_component *) malloc(sz);

    for (icompo = 0; icompo < cdo->NIComponent; ++icompo) {
      tempdep_property_init(&f->comps[icompo].beta);
      tempdep_property_init(&f->comps[icompo].radf);
      tempdep_property_init(&f->comps[icompo].sigma);
      tempdep_property_init(&f->comps[icompo].rho_s);
      tempdep_property_init(&f->comps[icompo].rho_l);
      tempdep_property_init(&f->comps[icompo].emi_s);
      tempdep_property_init(&f->comps[icompo].emi_l);
      tempdep_property_init(&f->comps[icompo].emi_g);
      tempdep_property_init(&f->comps[icompo].mu_s);
      tempdep_property_init(&f->comps[icompo].mu_l);
      tempdep_property_init(&f->comps[icompo].thc_s);
      tempdep_property_init(&f->comps[icompo].thc_l);
      tempdep_property_init(&f->comps[icompo].specht_s);
      tempdep_property_init(&f->comps[icompo].specht_l);
    }
  }

  geom_list_init(&f->diff_input_head.list);
  geom_list_init(&f->diff_g_input_head.list);
  return f;
}

static void malloc_boundary_array(struct boundary_array *a, int nx, int ny,
                                  domain *cdo)
{
  ptrdiff_t nxy, i;

  CSVASSERT(cdo);
  CSVASSERT(a);

  nxy  = nx;
  nxy *= ny;
  a->fl = (fluid_boundary_data **)malloc(sizeof(fluid_boundary_data *) * nxy);
  a->th = (thermal_boundary_data **)malloc(sizeof(thermal_boundary_data *) * nxy);

  if (a->fl && a->th) {
#pragma omp parallel for
    for (i = 0; i < nxy; ++i) {
      a->fl[i] = &cdo->fluid_boundary_head;
      a->th[i] = &cdo->thermal_boundary_head;
    }
  } else {
    free(a->fl);
    free(a->th);
    a->fl = NULL;
    a->th = NULL;
  }
}

static surface_boundary_data **malloc_surface_boundary_data(int m, domain *cdo)
{
  surface_boundary_data **p;
  p = (surface_boundary_data **)malloc(sizeof(surface_boundary_data *) * m);
  if (!p) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return NULL;
  }

#pragma omp parallel for
  for (int i = 0; i < m; ++i) {
    p[i] = &cdo->surface_boundary_head;
  }
  return p;
}


variable  *malloc_variable(domain *cdo, flags *flg)
// --------------------------------------------------------------------
{
  variable *f;
  /* YSE: Use calloc to clear structure by default. */
  f = (variable *) calloc( sizeof(variable) , 1 );
  size_t size = sizeof(type)*(cdo->m);
  size_t sizeN = sizeof(type)*(cdo->NumberOfComponent*cdo->m);
  size_t isize = sizeof(int)*(cdo->m);
  /* YSE: Add allocation array size by NumberOfComponent */
  size_t sizeC = sizeof(type)*(cdo->NumberOfComponent);
  /* YSE: Add allocation array size for NBaseComponent/NGasComponent*m */
  size_t sizeNB = sizeof(type)*cdo->NBaseComponent*cdo->m;
  size_t sizeNG = sizeof(type)*cdo->NGasComponent*cdo->m;
  size_t sizeNL = sizeof(type)*cdo->NumberOfLayer*cdo->m;
  size_t isizeNL = sizeof(int)*cdo->NumberOfLayer*cdo->m;

#ifdef GPU
  cudaMalloc((void**) &f->u, size);  cudaMemset(f->u, 0, size);
  cudaMalloc((void**) &f->v, size);  cudaMemset(f->v, 0, size);
  cudaMalloc((void**) &f->w, size);  cudaMemset(f->w, 0, size);
  cudaMalloc((void**) &f->t, size);  cudaMemset(f->t, 0, size);
  cudaMalloc((void**) &f->p, size);  cudaMemset(f->p, 0, size);

  cudaMalloc((void**) &f->fl,  size);  cudaMemset(f->fl,  0, size);
  cudaMalloc((void**) &f->fs,  size);  cudaMemset(f->fs,  0, size);
  cudaMalloc((void**) &f->fls, size);  cudaMemset(f->fls, 0, size);
  cudaMalloc((void**) &f->ll,  size);  cudaMemset(f->ll,  0, size);
  cudaMalloc((void**) &f->ls,  size);  cudaMemset(f->ls,  0, size);
  cudaMalloc((void**) &f->lls, size);  cudaMemset(f->lls, 0, size);

  cudaMalloc((void**) &f->nvlx,  size);  cudaMemset(f->nvlx,  0, size);
  cudaMalloc((void**) &f->nvly,  size);  cudaMemset(f->nvly,  0, size);
  cudaMalloc((void**) &f->nvlz,  size);  cudaMemset(f->nvly,  0, size);
  cudaMalloc((void**) &f->nvsx,  size);  cudaMemset(f->nvsx,  0, size);
  cudaMalloc((void**) &f->nvsy,  size);  cudaMemset(f->nvsy,  0, size);
  cudaMalloc((void**) &f->nvsz,  size);  cudaMemset(f->nvsy,  0, size);
#else

  f->u = (type *) malloc( size );  zero_clear(f->u, cdo->m);
  f->v = (type *) malloc( size );  zero_clear(f->v, cdo->m);
  f->w = (type *) malloc( size );  zero_clear(f->w, cdo->m);
  f->t = (type *) malloc( size );  zero_clear(f->t, cdo->m);
  f->p = (type *) malloc( size );  zero_clear(f->p, cdo->m);

  if(flg->solute_diff == OFF) {
    f->fl =  (type *) malloc( sizeNB ); zero_clear(f->fl,  cdo->NBaseComponent*cdo->m);
    f->fs =  (type *) malloc( sizeNB ); zero_clear(f->fs,  cdo->NBaseComponent*cdo->m);
    f->df =  (type *) malloc( sizeNB ); zero_clear(f->df,  cdo->NBaseComponent*cdo->m);
    f->dfs = (type *) malloc( sizeNB ); zero_clear(f->dfs, cdo->NBaseComponent*cdo->m);
  } else {
    f->fl =  (type *) malloc( size ); zero_clear(f->fl,  cdo->m);
    f->fs =  (type *) malloc( size ); zero_clear(f->fs,  cdo->m);
    f->fg  = (type *) malloc( size ); zero_clear(f->fg, cdo->m);
    /* See below on comment entha/mushy */
    /* f->df =  (type *) malloc( size ); zero_clear(f->df,  cdo->m); */
    /* f->dfs = (type *) malloc( size ); zero_clear(f->dfs, cdo->m); */
    f->df  = NULL;
    f->dfs = NULL;
    f->Y  =  (type *) malloc( sizeN ); zero_clear(f->Y, cdo->NumberOfComponent*cdo->m);
    f->Vf  = (type *) malloc( sizeN); zero_clear(f->Vf, cdo->NumberOfComponent*cdo->m);//added by Chai
    f->Yt  = (type *) malloc( size ); zero_clear(f->Yt, cdo->m);
    f->Sy  = (type *) malloc( sizeN ); zero_clear(f->Sy, cdo->NumberOfComponent*cdo->m);
    f->Y0 =  (type *) malloc( sizeC ); zero_clear(f->Y0, cdo->NumberOfComponent);
    f->Y0_G =(type *) malloc( sizeC ); zero_clear(f->Y0_G, cdo->NumberOfComponent);
    f->vfx = (type *) malloc( size );  zero_clear(f->vfx, cdo->m);
    f->vfy = (type *) malloc( size );  zero_clear(f->vfy, cdo->m);
    f->vfz = (type *) malloc( size );  zero_clear(f->vfz, cdo->m);
    f->vfxg = (type *) malloc( size );  zero_clear(f->vfxg, cdo->m);
    f->vfyg = (type *) malloc( size );  zero_clear(f->vfyg, cdo->m);
    f->vfzg = (type *) malloc( size );  zero_clear(f->vfzg, cdo->m);
  }    

  f->fl_sum = (type *) malloc( size );  zero_clear(f->fl_sum, cdo->m);
  f->fs_sum = (type *) malloc( size );  zero_clear(f->fs_sum, cdo->m);
  
  f->fls= (type *) malloc( size );  zero_clear(f->fls,cdo->m);

  f->ls = (type *) malloc( size );  zero_clear(f->ls, cdo->m);

  // Normal vectors for solid phase
  f->nvsx = (type *) malloc( size );  zero_clear(f->nvsx, cdo->m);
  f->nvsy = (type *) malloc( size );  zero_clear(f->nvsy, cdo->m);
  f->nvsz = (type *) malloc( size );  zero_clear(f->nvsz, cdo->m);

  f->ll = (type *) malloc( size );  zero_clear(f->ll, cdo->m);
  f->lls= (type *) malloc( size );  zero_clear(f->lls,cdo->m);

  // Normal vectors for liquid phase
  f->nvlx = (type *) malloc( size );  zero_clear(f->nvlx, cdo->m);
  f->nvly = (type *) malloc( size );  zero_clear(f->nvly, cdo->m);
  f->nvlz = (type *) malloc( size );  zero_clear(f->nvlz, cdo->m);
  f->curv = (type *) malloc( size );  zero_clear(f->curv, cdo->m);     


  if(flg->multi_layer == ON){

    f->bubble_cnt = (int  *) malloc( sizeof(int)*cdo->NumberOfLayer );  zero_clear_int(f->bubble_cnt, cdo->NumberOfLayer);

    f->fl_layer =  (type *) malloc( sizeNL ); zero_clear(f->fl_layer,  cdo->NumberOfLayer*cdo->m);

    // fls_layer is defined only when multi_layer is ON
    f->fls_layer = (type *) malloc( sizeNL );  zero_clear(f->fls_layer, cdo->NumberOfLayer*cdo->m);

    f->ll_layer = (type *) malloc( sizeNL );  zero_clear(f->ll_layer, cdo->NumberOfLayer*cdo->m);
    f->lls_layer = (type *) malloc( sizeNL );  zero_clear(f->lls_layer,cdo->NumberOfLayer*cdo->m);

    // Normal vectors for liquid phase
    f->nvlx_layer = (type *) malloc( sizeNL );  zero_clear(f->nvlx_layer, cdo->NumberOfLayer*cdo->m);
    f->nvly_layer = (type *) malloc( sizeNL );  zero_clear(f->nvly_layer, cdo->NumberOfLayer*cdo->m);
    f->nvlz_layer = (type *) malloc( sizeNL );  zero_clear(f->nvlz_layer, cdo->NumberOfLayer*cdo->m);
    f->curv_layer = (type *) malloc( sizeNL );  zero_clear(f->curv_layer, cdo->NumberOfLayer*cdo->m);
    f->label_layer = (int  *) malloc( isizeNL );  zero_clear_int(f->label_layer, cdo->NumberOfLayer*cdo->m);
    f->is_orifice_layer = (int  *) malloc( isizeNL );  zero_clear_int(f->is_orifice_layer, cdo->NumberOfLayer*cdo->m);
    f->is_orifice_layer_initialized = OFF;

    if(flg->film_drainage){
      f->liquid_film = (type  *) malloc( size );  zero_clear(f->liquid_film, cdo->m);
    }

  }

  if (flg->IBM == ON && flg->porous == ON) {
    f->fs_ibm = (type *) malloc( size );  zero_clear(f->fs_ibm, cdo->m);
    f->ls_ibm = (type *) malloc( size );  zero_clear(f->ls_ibm, cdo->m);
    f->nvibmx = (type *) malloc( size );  zero_clear(f->nvibmx, cdo->m);
    f->nvibmy = (type *) malloc( size );  zero_clear(f->nvibmy, cdo->m);
    f->nvibmz = (type *) malloc( size );  zero_clear(f->nvibmz, cdo->m);
  } else {
    f->fs_ibm = NULL;
    f->ls_ibm = NULL;
    f->nvibmx = NULL;
    f->nvibmy = NULL;
    f->nvibmz = NULL;
  }

  f->flg_obst_A = (type *) malloc( size ); zero_clear(f->flg_obst_A, cdo->m);
  f->flg_obst_B = (type *) malloc( size ); zero_clear(f->flg_obst_B, cdo->m);
  f->vel_c = (type *) malloc( size ); zero_clear(f->vel_c, cdo->m);
  f->work = (type *) malloc( size ); zero_clear(f->work, cdo->m);
  f->work_multi = (type *) malloc( sizeNL );  zero_clear(f->work_multi, cdo->NumberOfLayer*cdo->m);
  if(flg->oxidation == ON){
    f->ox_dt = (type *)malloc( size ); zero_clear(f->ox_dt, cdo->m);
    f->ox_dt_local = (type *)malloc( size ); zero_clear(f->ox_dt_local, cdo->m);
    f->ox_flag = (int *)malloc( isize ); zero_clear_int(f->ox_flag, cdo->m);
    f->ox_vof = (type *)malloc( size ); zero_clear(f->ox_vof, cdo->m);
    f->ox_lset = (type *)malloc( size ); zero_clear(f->ox_lset, cdo->m);
    f->ox_q = (type *)malloc( size ); zero_clear(f->ox_q, cdo->m);
    if (flg->solute_diff == ON) {
      f->ox_h2 = (type *)malloc( size ); zero_clear(f->ox_h2, cdo->m);
    } else {
      f->ox_h2 = NULL;
    }
    f->ox_f_h2o = (type *)malloc( size ); zero_clear(f->ox_f_h2o, cdo->m);
    f->ox_lset_h2o = (type *)malloc( size ); zero_clear(f->ox_lset_h2o, cdo->m);
    f->ox_lset_h2o_s = (type *)malloc( size ); zero_clear(f->ox_lset_h2o_s, cdo->m);
  }
  if(flg->h2_absorp_eval == ON) {
    /* Currently only supports averaging over X-Y plane */
    f->h2_absorp_eval = (type *)calloc(cdo->nz, sizeof(type));
    f->h2_absorp_Ks = (type *)calloc(cdo->nz, sizeof(type));
    f->h2_absorp_P = (type *)calloc(cdo->nz, sizeof(type));
    f->h2_absorp_T = (type *)calloc(cdo->nz, sizeof(type));
    f->h2_absorp_Zr = (type *)calloc(cdo->nz, sizeof(type));
  } else {
    f->h2_absorp_eval = NULL;
    f->h2_absorp_Ks = NULL;
    f->h2_absorp_P = NULL;
    f->h2_absorp_T = NULL;
    f->h2_absorp_Zr = NULL;
  }

#ifdef LPTX
  f->lpt_param = NULL;
#endif
  f->qpt = NULL;

  if(flg->lpt_calc == ON) {
    int j;

    f->lpt_pewall = (type *)malloc( size );

#pragma omp parallel for private(j)
    for(j = 0; j < cdo->m; j++) {
      f->lpt_pewall[j] = -999.999;
    }

    f->qpt = (type *)malloc( size );
    zero_clear(f->qpt, cdo->m);
  } else {
    f->lpt_pewall = NULL;
  }

  f->qgeom = (type *)malloc(size); zero_clear(f->qgeom, cdo->m);

  /* YSE: Added boundary array allocation */
  malloc_boundary_array(&f->bnd_B, cdo->nbx, cdo->nby, cdo);
  malloc_boundary_array(&f->bnd_T, cdo->nbx, cdo->nby, cdo);
  malloc_boundary_array(&f->bnd_N, cdo->nbx, cdo->nbz, cdo);
  malloc_boundary_array(&f->bnd_S, cdo->nbx, cdo->nbz, cdo);
  malloc_boundary_array(&f->bnd_W, cdo->nby, cdo->nbz, cdo);
  malloc_boundary_array(&f->bnd_E, cdo->nby, cdo->nbz, cdo);

  /* These variables are always allocated on-demand */
  f->surface_bnd = NULL;
  f->bnd_norm_u = NULL;
  f->bnd_norm_v = NULL;
  f->bnd_norm_w = NULL;

  /*added by Chai*/
  f->t_pre = (type *) malloc( size );  zero_clear(f->t_pre, cdo->m);

  /*
   * New model which uses entha and mushy is applied only for solute_diff
   * is set to ON, otherwise NULL. You can add or remove conditions to be
   * applicable to new phase chage model.
   */
  if (flg->solute_diff == ON) {
    f->entha = (type *) malloc( size );  zero_clear(f->entha, cdo->m);
    f->mushy = (int  *) malloc( size );  zero_clear_int(f->mushy, cdo->m);
  } else {
    f->entha = NULL;
    f->mushy = NULL;
  }

  /* Added by Susumu for porus medium model */
  //f->eps = (type *) malloc( sizeN );  zero_clear(f->eps, cdo->NumberOfComponent*cdo->m);
  f->eps  = (type *) malloc( size ); zero_clear(f->eps,  cdo->m);
  f->perm = (type *) malloc( size ); zero_clear(f->perm, cdo->m);
  f->sgm  = (type *) malloc( size ); zero_clear(f->sgm,  cdo->m);
  f->epss = (type *) malloc( size ); zero_clear(f->epss, cdo->m);
  if (flg->two_energy == ON) {
    f->tf = (type *) malloc( size ); zero_clear(f->tf,  cdo->m);
    f->ts = (type *) malloc( size ); zero_clear(f->ts,  cdo->m);
  } else {
    f->tf = NULL;
    f->ts = NULL;
  }
  {
    int mcompo = component_info_ncompo(&cdo->mass_source_g_comps);
    if (mcompo > 0) {
      f->mass_source_g = (type *)malloc(size * mcompo);
      zero_clear(f->mass_source_g, cdo->m * mcompo);
    } else {
      f->mass_source_g = NULL;
    }
  }

  /*Fukuda: enthaply monitor*/
  f->enthalpy = (type *) malloc( size );  zero_clear(f->enthalpy, cdo->m); 
  f->init_enthalpy = (type *) malloc( size );  zero_clear(f->init_enthalpy, cdo->m); 
  f->enthalpy_time_derivative = (type *) malloc( size );  zero_clear(f->enthalpy_time_derivative, cdo->m);

#endif
  printf("Allocate memory of u, v, w, VOFs, level-set func...\n");

  return f;
}

material  *malloc_material(domain *cdo, flags *flg)
// --------------------------------------------------------------------
{
    material *f;
    f = (material *) malloc( sizeof(material) );
    size_t size = sizeof(type)*(cdo->m);
    /* YSE: Add allocation array size for NBaseComponent/NGasComponent*m */
    size_t sizeNB = sizeof(type)*cdo->NBaseComponent*cdo->m;
    size_t sizeNG = sizeof(type)*cdo->NGasComponent*cdo->m;
    size_t sizeCB = sizeof(type)*cdo->NBaseComponent;
#ifdef GPU
    cudaMalloc((void**) &f->dens,   size);  cudaMemset(f->dens,   0, size);
    cudaMalloc((void**) &f->mu,     size);  cudaMemset(f->mu,     0, size);
    cudaMalloc((void**) &f->nu,     size);  cudaMemset(f->nu,     0, size);
    cudaMalloc((void**) &f->specht, size);  cudaMemset(f->specht, 0, size);
    cudaMalloc((void**) &f->thc,    size);  cudaMemset(f->thc,    0, size);
    cudaMalloc((void**) &f->st,     size);  cudaMemset(f->st,     0, size);
    cudaMalloc((void**) &f->q,      size);  cudaMemset(f->q,      0, size);
    cudaMalloc((void**) &f->rad,    size);  cudaMemset(f->rad,    0, size); // < 2016 Added by KKE
    cudaMalloc((void**) &f->emi,    size);  cudaMemset(f->emi,    0, size); // < 2016 Added by KKE
#else
    f->dens   = (type *) malloc( size );  zero_clear(f->dens, cdo->m);
    f->mu     = (type *) malloc( size );  zero_clear(f->mu, cdo->m);
    f->nu     = (type *) malloc( size );  zero_clear(f->nu, cdo->m);
    f->specht = (type *) malloc( size );  zero_clear(f->specht, cdo->m);
    f->st     = (type *) malloc( size );  zero_clear(f->st, cdo->m);
    f->thc    = (type *) malloc( size );  zero_clear(f->thc, cdo->m);
    f->thcs    = (type *) malloc( size );  zero_clear(f->thcs, cdo->m);
    f->thcf    = (type *) malloc( size );  zero_clear(f->thcf, cdo->m);
    f->denss    = (type *) malloc( size );  zero_clear(f->denss, cdo->m);
    f->Dcg    = (type *) malloc( size );  zero_clear(f->Dcg, cdo->m);
    f->latent = (type *) malloc( size );  zero_clear(f->latent, cdo->m);
    f->q      = (type *) malloc( size );  zero_clear(f->q, cdo->m);
    f->rad    = (type *) malloc( size );  zero_clear(f->rad, cdo->m);
    f->Vp     = (type *) malloc(sizeCB);  zero_clear(f->Vp, cdo->NBaseComponent);//added by Chai
    f->t_liq = (type *) malloc( size ); zero_clear(f->t_liq, cdo->m);//2017/11/16
    f->t_soli = (type *) malloc( size ); zero_clear(f->t_soli, cdo->m);//2017/11/16
    f->emi    = (type *) malloc( size );  zero_clear(f->emi, cdo->m); // < 2016 Added by KKE

    f->div_b = (type *) malloc( size );  zero_clear(f->div_b, cdo->m);
    f->div_a = (type *) malloc( size );  zero_clear(f->div_a, cdo->m);

    if (flg->output_data.div_u.outf == ON ||
        flg->restart_data.div_u.outf == ON) {
      f->div_u = (type *) malloc( size );  zero_clear(f->div_u, cdo->m);
    } else {
      f->div_u = NULL;
    }

    f->dens_f = (type *) malloc( size );  zero_clear(f->dens_f, cdo->m);
    f->c_f = (type *) malloc( size );  zero_clear(f->c_f, cdo->m);

    /* YSE: Oxidation optional array */
    if (flg->output_data.ox_dens.outf == ON ||
        flg->restart_data.ox_dens.outf == ON) {
      f->ox_dens = (type *)malloc( size ); zero_clear(f->ox_dens, cdo->m);
    } else {
      f->ox_dens = NULL;
    }
    if (flg->output_data.ox_kp.outf == ON ||
        flg->restart_data.ox_kp.outf == ON) {
      f->ox_kp = (type *)malloc( size ); zero_clear(f->ox_kp, cdo->m);
    } else {
      f->ox_kp = NULL;
    }
    if (flg->output_data.ox_recess_rate.outf == ON ||
        flg->restart_data.ox_recess_rate.outf == ON) {
      f->ox_recess_k = (type *)malloc( size );
      zero_clear(f->ox_recess_k, cdo->m);
    } else {
      f->ox_recess_k = NULL;
    }
#endif
    printf("Allocate memory of dens,mu,nu,specht,thc,st,q,rad,emi.\n"); // < 2016 Modifed by KKE
    return f;
}

/* YSE: Add deallocate functions */
void free_boundary_array(struct boundary_array *a)
{
  CSVASSERT(a);

  free(a->fl);
  free(a->th);
}


void free_variable(variable *v)
{
  if (v) {
    free(v->u);
    free(v->v);
    free(v->w);
    free(v->t);
    free(v->p);
    free(v->fl);
    free(v->fs);
    free(v->df);
    free(v->dfs);
    free(v->fg);
    free(v->Y);
    free(v->Yt);
    free(v->Sy);
    free(v->Y0);
    free(v->Y0_G);
    free(v->vfx);
    free(v->vfy);
    free(v->vfz);
    free(v->vfxg);
    free(v->vfyg);
    free(v->vfzg);
    free(v->fl_sum);
    free(v->fs_sum);

    free(v->fls);
    free(v->ll);
    free(v->ls);
    free(v->lls);

    free(v->nvsx);
    free(v->nvsy);
    free(v->nvsz);
    free(v->nvlx);
    free(v->nvly);
    free(v->nvlz);
    free(v->curv);

    // multi_layer
    free(v->bubble_cnt);
    free(v->fl_layer);
    free(v->fls_layer);
    free(v->ll_layer);
    free(v->lls_layer);
    free(v->nvlx_layer);
    free(v->nvly_layer);
    free(v->nvlz_layer);
    free(v->curv_layer);
    free(v->label_layer);
    free(v->is_orifice_layer);
    free(v->liquid_film);

    free(v->fs_ibm);
    free(v->ls_ibm);
    free(v->nvibmx);
    free(v->nvibmy);
    free(v->nvibmz);

    free(v->flg_obst_A);
    free(v->flg_obst_B);
    free(v->vel_c);
    free(v->work);
    free(v->work_multi);

    free(v->qgeom);

    free(v->ox_dt);
    free(v->ox_dt_local);
    free(v->ox_flag);
    free(v->ox_vof);
    free(v->ox_lset);
    free(v->ox_q);
    free(v->ox_h2);
    free(v->ox_f_h2o);
    free(v->ox_lset_h2o);
    free(v->ox_lset_h2o_s);

    /*added by Chai*/
    free(v->t_pre);
    free(v->mushy);
    free(v->entha);
    free(v->Vf);

    free(v->lpt_pewall);

    /* Added by Susumu */
    free(v->eps);
    free(v->epss);
    free(v->perm);
    free(v->sgm);
    free(v->tf);
    free(v->ts);

    free_boundary_array(&v->bnd_B);
    free_boundary_array(&v->bnd_T);
    free_boundary_array(&v->bnd_N);
    free_boundary_array(&v->bnd_S);
    free_boundary_array(&v->bnd_E);
    free_boundary_array(&v->bnd_W);

    free(v->surface_bnd);
    free(v->bnd_norm_u);
    free(v->bnd_norm_v);
    free(v->bnd_norm_w);

    free(v->enthalpy); 
    free(v->init_enthalpy); 
    free(v->enthalpy_time_derivative);


#ifdef LPTX
    if (v->lpt_param)
      LPTX_param_delete(v->lpt_param);
#endif
    if (v->qpt)
      free(v->qpt);
  }
  free(v);
}

void free_material(material *m)
{
  if (m) {
    free(m->dens);
    free(m->mu);
    free(m->nu);
    free(m->specht);
    free(m->st);
    free(m->thc);
    free(m->latent);
    free(m->q);
    free(m->rad);
    free(m->t_liq);//2017/11/16
    free(m->t_soli);//2017/11/16
    free(m->emi); // < 2016 Added by KKE

    free(m->Vp); // < 2019 Added by Chai
    free(m->div_b);
    free(m->div_a);
    free(m->div_u);

    free(m->ox_dens);
    free(m->ox_kp);
    free(m->ox_recess_k);

    free(m->dens_f); // 2020/06/17 by Susumu
    free(m->c_f); 
    free(m->thcs);
    free(m->thcf);
    free(m->denss);
    free(m->Dcg);
  }
  free(m);
}
