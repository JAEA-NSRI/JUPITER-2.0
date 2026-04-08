#include <stdio.h>
#include <stdlib.h>
#include <float.h>
#include <math.h>

#ifdef GPU
  #include <cuda.h>
  #include <cutil.h>
#endif

#include "common_util.h"
#include "component_info.h"
#include "func.h"
#include "os/asprintf.h"
#include "os/os.h"
#include "struct.h"
#include "tempdep_calc.h"
#include "dccalc.h"

static inline type inline_clip(type f)
{
  if (f >= 1.0) return 1.0;
  if (f <= 0.0) return 0.0;
  return f;
}

static inline type type_min(type a, type b)
{
  return (a < b) ? a : b;
}


static inline type AVEnume(type w, type x)
{
  return 0.5 * (w + x);
}

struct make_matrix_array_Vf_data
{
  type *f;     /*!< VOF (typically for Solid+Liquid) */
  type *t;     /*!< Temperature */
  type *Y;     /*!< Mole fraction of each components */
  type dti;    /*!< Inverse of time-step */
  int ncompo;  /*!< Number of components */
  struct dc_calc_param **params; /*!< Diffusivity parameters */
  int commutative; /*!< Whether binary diffusivities are commutative */
  int target_compo; /*!< Target component ID */
};

struct make_matrix_array_Y_data
{
  type *f;
  type *Dc;
  type dti;
  int f_neg;
};


/**
 * @brief Compute binary diffusivity on cell surface
 * @param T Temperature on cell surface [K]
 * @param Y Array of mole fractions on cell surface [-]
 * @param fp VOF (typically Liquid+Solid) on cell-p [-]
 * @param fn VOF (typically Liquid+Solid) on cell next to cell-p [-]
 * @param params diffusivity calculation parameter array
 * @param diff_compo Component index of diffusing material
 * @param base_compo Component index of base material
 * @param Ncompo Number of components
 * @param commutative 0 for noncommutative, otherwise treat as commutative
 * @return Computed diffusivity
 *
 * @p Y must be array of @p Ncompo elements, for each components.
 *
 * @p params must be array sized by dc_calc_binary_size()
 * (or dc_calc_binary_size_commutative() if commutative) for @p Ncompo.
 *
 * @p diffusing_compo and @p base_compo must be valid combination
 * (positive, less @p Ncompo and not equal each other). See
 * dc_calc_binary_address() (or dc_calc_binary_address_commutative()
 * if commutative) for more information.
 */
static type
compute_D_bin(type T, type *Y, type fp, type fn,
              struct dc_calc_param **params, int diff_compo, int base_compo,
              int Ncompo, int commutative)
{
  type Ydiff, Ybase, f;
  struct dc_calc_param *p;
  ptrdiff_t padr;

  if (fp == 0.0 || fn == 0.0) return 0.0;

  if (commutative) {
    padr = dc_calc_binary_address_commutative(diff_compo, base_compo, Ncompo);
  } else {
    padr = dc_calc_binary_address(diff_compo, base_compo, Ncompo);
  }
  /* Prohibited combination of components are asserted */
  CSVASSERT(padr >= 0);
  /* Avoid SEGV when assertion is skipped */
  if (padr < 0) {
    return 0.0;
  }

  p = params[padr];
  if (!p)
    return 0.0;

  if (fn != 0.0) {
    Ydiff = Y[diff_compo];
    Ybase = Y[base_compo];
  } else {
    Ydiff = 0.0;
    Ybase = 0.0;
  }

  return dc_calc(p, Ydiff, Ybase, T);
}

/**
 * @brief Compute adjusted diffusivity for noncommutative binary diffusivity
 * @param D_bin_ij diffusivity of diffusing material on base material
 * @param D_bin_ji diffusivity of base material on diffusing material
 * @param Y mole fraction of diffusing material on cell surface
 * @retval non-0 adjusted value (for both diffusivity)
 * @retval D_bin_ij @p D_bin_ij and @p D_bin_ji are equal
 * @retval 0 @p D_bin_ij or @p D_bin_ji is zero.
 * @retval 0 The fraction of diffusivities is negative.
 *
 * Still may return negative if both diffusivities are negative, it's
 * not checked.
 */
static type
compute_D_bin_noncommutative(type D_bin_ij, type D_bin_ji, type Y)
{
  type m;

  if (D_bin_ij == D_bin_ji) return D_bin_ij;
  if (D_bin_ji == 0.0) return 0.0;
  m = D_bin_ij / D_bin_ji;

  if (m <= 0.0) return 0.0;
  m = log(m);

  return D_bin_ji * exp(m * Y);
}

/**
 * @brief Compute effective Diffusivity
 * @param Y Mole-fraction on the cell-surface for each components
 * @param D_bin Computed binary diffusivity on the cell-surface
 * @param icompo Target component index
 * @param Ncompo Number of components
 * @param commutative 0 if non-commutative, otherwise commutative.
 * @return computed value, 0 when all binary diffusivities are zero.
 *
 * \f[
 *  D_{mul,i} = \frac{1 - Y_i}{\sum_{0\le j\lt N,i\ne j,D_{i,j}\ne 0}
 *                                \frac{Y_j}{D_{i,j}}}
 * \f]
 *
 * @p D_bin must be an array sized by dc_calc_binary_size() (or
 * dc_calc_binary_size_commutative() if commutative) for @p Ncompo.
 *
 * Expected to be inlined and be fused independent loops upon the
 * optimization. But we don't think there is a huge performance loss
 * here.
 */
static type
compute_D_mul(type *Y, type *D_bin, int icompo, int Ncompo, int commutative)
{
  type D;
  type deno;
  int m_index;
  ptrdiff_t padr;

  deno = 0.0;
  for (m_index = 0; m_index < Ncompo; m_index++) {
    if (commutative) {
      padr = dc_calc_binary_address_commutative(icompo, m_index, Ncompo);
    } else {
      padr = dc_calc_binary_address(icompo, m_index, Ncompo);
    }
    if (padr < 0) continue;

    D = D_bin[padr];
    if (D != 0) {
      deno += Y[m_index] / D;
    }
  }
  if (deno != 0.0) {
    return (1.0 - Y[icompo]) / deno;
  } else {
    return 0.0;
  }
}

static int make_matrix_array_Vf(int topo_flag, type *A, type *b,
                                int stmx, int stmy, int stmz,
                                int stpx, int stpy, int stpz,
                                int mx, int my, int mz,
                                parameter *prm, void *arg);

static int make_matrix_array_Y(int topo_flag, type *A, type *b,
                               int stmx, int stmy, int stmz,
                               int stpx, int stpy, int stpz,
                               int mx, int my, int mz,
                               parameter *prm, void *arg);

type solute_transport(variable *val, material *mtl, parameter *prm)
{
  flags  *flg = prm->flg;
  domain *cdo = prm->cdo;
  if(flg->solute_diff == OFF) return 0.0;
  type *x=cdo->x, *y=cdo->y, *z=cdo->z;
  int jx,jy,jz,j,i,icompo;
  int itr_diff= 1,itr;
  static int  n1st = 0;
  const int nx=cdo->nx, ny=cdo->ny, nz=cdo->nz;
  const int mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=cdo->mx*cdo->my, m=cdo->m, stm=cdo->stm;
  const int NCompo = cdo->NBaseComponent;
  const size_t size = sizeof(type)*(m*NCompo);
  const size_t sizeN = sizeof(type)*(NCompo);
  const size_t sizeNN= sizeof(type)*(NCompo*NCompo);
  const size_t sizem = sizeof(type)*(m);//added by Chai
  type dt_d,diff_num;
  const type dt = cdo->dt;
  const type time0 = cpu_time();
  const type dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi;
  const type dxi2=cdo->dxi*cdo->dxi, dyi2=cdo->dyi*cdo->dyi, dzi2=cdo->dzi*cdo->dzi;
  type time=cdo->time;
  type *fls=val->fls, *Y=val->Y, *Yt = val->Yt;
  type *Vf=val->Vf, *Vp=mtl->Vp;
  type *t=val->t;
  type *Vfn;

  //--------- Determine sub-cycling time step for diffusion term -----------------------------------
  type Dg;
  type Dg_max=val->Dg_max;
  Dg = Dg_max;

  if (NCompo > 1) {
    /* YSE: default in implicit method */
    if (1) {
      type dti;
      int NBCompo = cdo->NBaseComponent;
      struct make_matrix_array_Vf_data d;
      int r;
      char *name;
      const char *cname;
      const int commutative = 0;

      d.dti = 1.0 / cdo->dt;
      d.f = val->fls;
      d.Y = val->Y;
      d.ncompo = NBCompo;
      d.commutative = commutative;
      d.params = prm->phv->diff_params;
      d.t = val->t;

      for (icompo = 0; icompo < NBCompo; ++icompo) {
        d.target_compo = icompo;

#pragma omp parallel for
        for(i = 0; i < cdo->n; i++) {
          int jx, jy, jz;
          ptrdiff_t jj;

          calc_struct_index(i, nx, ny, nz, &jx, &jy, &jz);
          jj = calc_address(jx + cdo->stm, jy + cdo->stm, jz + cdo->stm,
                            mx, my, mz);

          mtl->div_b[i] = - Vf[jj+icompo*m]*d.dti;
        }

        r = jupiter_asprintf(&name, "Vf%d", icompo);
        if (r < 0) {
          cname = "Y";
          name = NULL;
        } else {
          cname = name;
        }

#ifdef CCSE
        ccse_poisson_f(0, cname, &Vf[icompo * cdo->m], mtl->div_b, prm,
                       30000, 1.0e-6, 1.0e-50, make_matrix_array_Vf, &d);
#endif
        free(name);
      }
      /* YSE: end */
    } else /* explicit formulation */ {
      dt_d = dt;
      diff_num = dt_d*Dg*(dxi2 + dyi2 + dzi2);
      for(;;){
        if(diff_num < cdo->diff_num)
          break;
        itr_diff *= 2;
        dt_d /= 2.0;
        diff_num = dt_d*Dg*(dxi2 + dyi2 + dzi2);
      }
      //  printf("%d\n", itr_diff);
      Vfn =(type *) malloc (size ); zero_clear(Vfn, m*NCompo);

      //-------- Calculate diffusion term ------------
      for(itr = 0; itr < itr_diff; itr++){
//#pragma omp parallel for private(jx,jy,j,Dgeff_w,Dgeff_e,Dgeff_s,Dgeff_n,Dgeff_b,Dgeff_t)
        for(jz = 0; jz < nz; jz++) {
          for(jy = 0; jy < ny; jy++) {
            for(jx = 0; jx < nx; jx++) {
              j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

              if(val->fls[j] == 0.0) continue;

              // Effective diffusion coefficient
              // Temperary variables at six surfaces for solute diffusion calculation
              type t_e, t_w, t_n, t_s, t_t, t_b;//average temperature
              type *Y_e, *Y_w, *Y_n, *Y_s, *Y_t, *Y_b;//average volume fraction
              type *D_bin_e, *D_bin_w, *D_bin_n, *D_bin_s, *D_bin_t, *D_bin_b; //binary diffusion coefficient
              int m_index;//material index
              type Vfnt = 0.0;

              // Malloc the temp variables
              Y_e = (type *) malloc (sizeN); zero_clear(Y_e, NCompo);
              Y_w = (type *) malloc (sizeN); zero_clear(Y_w, NCompo);
              Y_n = (type *) malloc (sizeN); zero_clear(Y_n, NCompo);
              Y_s = (type *) malloc (sizeN); zero_clear(Y_s, NCompo);
              Y_t = (type *) malloc (sizeN); zero_clear(Y_t, NCompo);
              Y_b = (type *) malloc (sizeN); zero_clear(Y_b, NCompo);

              D_bin_e = (type *) malloc (sizeNN); zero_clear(D_bin_e, NCompo * NCompo);
              D_bin_w = (type *) malloc (sizeNN); zero_clear(D_bin_w, NCompo * NCompo);
              D_bin_n = (type *) malloc (sizeNN); zero_clear(D_bin_n, NCompo * NCompo);
              D_bin_s = (type *) malloc (sizeNN); zero_clear(D_bin_s, NCompo * NCompo);
              D_bin_t = (type *) malloc (sizeNN); zero_clear(D_bin_t, NCompo * NCompo);
              D_bin_b = (type *) malloc (sizeNN); zero_clear(D_bin_b, NCompo * NCompo);

              // Average variables for diffusion coefficient calculation
              t_e = AVEnume(t[j], t[j +   1]);
              t_w = AVEnume(t[j], t[j -   1]);
              t_n = AVEnume(t[j], t[j +  mx]);
              t_s = AVEnume(t[j], t[j -  mx]);
              t_t = AVEnume(t[j], t[j + mxy]);
              t_b = AVEnume(t[j], t[j - mxy]);

              for(icompo = 0; icompo < NCompo; icompo++){
                Y_e[icompo] = (val->fls[j + 1  ]!=0) ? AVEnume(Y[j+m*icompo], Y[j +   1 + m*icompo]) : 0.0;
                Y_w[icompo] = (val->fls[j - 1  ]!=0) ? AVEnume(Y[j+m*icompo], Y[j -   1 + m*icompo]) : 0.0;
                Y_n[icompo] = (val->fls[j + mx ]!=0) ? AVEnume(Y[j+m*icompo], Y[j +  mx + m*icompo]) : 0.0;
                Y_s[icompo] = (val->fls[j - mx ]!=0) ? AVEnume(Y[j+m*icompo], Y[j -  mx + m*icompo]) : 0.0;
                Y_t[icompo] = (val->fls[j + mxy]!=0) ? AVEnume(Y[j+m*icompo], Y[j + mxy + m*icompo]) : 0.0;
                Y_b[icompo] = (val->fls[j - mxy]!=0) ? AVEnume(Y[j+m*icompo], Y[j - mxy + m*icompo]) : 0.0;
              }

              for(icompo = 0; icompo < NCompo; icompo++){
                for(m_index = 0; m_index < NCompo; m_index++){
                  struct dc_calc_param *p;
                  ptrdiff_t padr;

                  if (m_index == icompo) continue;

                  padr = dc_calc_binary_address(icompo, m_index, NCompo);
                  if (padr < 0) continue;

                  p = prm->phv->diff_params[padr];
                  if (Y_e[icompo]!=0.0||Y_e[m_index]!=0.0)
                    D_bin_e[icompo + m_index * NCompo] = dc_calc(p, Y_e[icompo], Y_e[m_index], t_e);
                  if (Y_w[icompo]!=0.0||Y_w[m_index]!=0.0)
                    D_bin_w[icompo + m_index * NCompo] = dc_calc(p, Y_w[icompo], Y_w[m_index], t_w);
                  if (Y_n[icompo]!=0.0||Y_n[m_index]!=0.0)
                    D_bin_n[icompo + m_index * NCompo] = dc_calc(p, Y_n[icompo], Y_n[m_index], t_n);
                  if (Y_s[icompo]!=0.0||Y_s[m_index]!=0.0)
                    D_bin_s[icompo + m_index * NCompo] = dc_calc(p, Y_s[icompo], Y_s[m_index], t_s);
                  if (Y_t[icompo]!=0.0||Y_t[m_index]!=0.0)
                    D_bin_t[icompo + m_index * NCompo] = dc_calc(p, Y_t[icompo], Y_t[m_index], t_t);
                  if (Y_b[icompo]!=0.0||Y_b[m_index]!=0.0)
                    D_bin_b[icompo + m_index * NCompo] = dc_calc(p, Y_b[icompo], Y_b[m_index], t_b);
                }
              }

              for(icompo = 0; icompo < NCompo; icompo++){
                for(m_index = 0; m_index < NCompo; m_index++){
                  if (m_index == icompo) continue;
                  if (D_bin_e[icompo + m_index * NCompo] != D_bin_e[m_index + icompo * NCompo]) {
                    type m;
                    m = D_bin_e[icompo + m_index * NCompo]/D_bin_e[m_index + icompo * NCompo];
                    m = log(m);
                    D_bin_e[m_index + icompo * NCompo] = D_bin_e[icompo + m_index * NCompo] = D_bin_e[m_index + icompo * NCompo] * exp(m * Y_e[icompo]);
                  }

                  if (D_bin_w[icompo + m_index * NCompo] != D_bin_w[m_index + icompo * NCompo]) {
                    type m;
                    m = D_bin_w[icompo + m_index * NCompo]/D_bin_w[m_index + icompo * NCompo];
                    m = log(m);
                    D_bin_w[m_index + icompo * NCompo] = D_bin_w[icompo + m_index * NCompo] = D_bin_w[m_index + icompo * NCompo] * exp(m * Y_w[icompo]);
                  }

                  if (D_bin_n[icompo + m_index * NCompo] != D_bin_n[m_index + icompo * NCompo]) {
                    type m;
                    m = D_bin_n[icompo + m_index * NCompo]/D_bin_n[m_index + icompo * NCompo];
                    m = log(m);
                    D_bin_n[m_index + icompo * NCompo] = D_bin_n[icompo + m_index * NCompo] = D_bin_n[m_index + icompo * NCompo] * exp(m * Y_n[icompo]);
                  }

                  if (D_bin_s[icompo + m_index * NCompo] != D_bin_s[m_index + icompo * NCompo]) {
                    type m;
                    m = D_bin_s[icompo + m_index * NCompo]/D_bin_s[m_index + icompo * NCompo];
                    m = log(m);
                    D_bin_s[m_index + icompo * NCompo] = D_bin_s[icompo + m_index * NCompo] = D_bin_s[m_index + icompo * NCompo] * exp(m * Y_s[icompo]);
                  }

                  if (D_bin_t[icompo + m_index * NCompo] != D_bin_t[m_index + icompo * NCompo]) {
                    type m;
                    m = D_bin_t[icompo + m_index * NCompo]/D_bin_t[m_index + icompo * NCompo];
                    m = log(m);
                    D_bin_t[m_index + icompo * NCompo] = D_bin_t[icompo + m_index * NCompo] = D_bin_t[m_index + icompo * NCompo] * exp(m * Y_t[icompo]);
                  }

                  if (D_bin_b[icompo + m_index * NCompo] != D_bin_b[m_index + icompo * NCompo]) {
                    type m;
                    m = D_bin_b[icompo + m_index * NCompo]/D_bin_b[m_index + icompo * NCompo];
                    m = log(m);
                    D_bin_b[m_index + icompo * NCompo] = D_bin_b[icompo + m_index * NCompo] = D_bin_b[m_index + icompo * NCompo] * exp(m * Y_b[icompo]);
                  }
                }
              }

              // Effective multi-component diffusion coefficient
              for(icompo = 0; icompo < NCompo; icompo++){
                type D_mul_e, D_mul_w, D_mul_n, D_mul_s, D_mul_t, D_mul_b; //multi-component diffusion coefficient
                type deno_e, deno_w, deno_n, deno_s, deno_t, deno_b;
                D_mul_e = D_mul_w = D_mul_n = D_mul_s = D_mul_t = D_mul_b= 0.0;
                deno_e = deno_w = deno_n = deno_s = deno_t = deno_b = 0.0;
                for(m_index = 0; m_index < NCompo; m_index++){
                  if (m_index == icompo) continue;
                  if (D_bin_e[icompo + m_index * NCompo]!=0.0)
                    deno_e = deno_e + Y_e[m_index]/D_bin_e[icompo + m_index * NCompo];
                  if (D_bin_w[icompo + m_index * NCompo]!=0.0)
                    deno_w = deno_w + Y_w[m_index]/D_bin_w[icompo + m_index * NCompo];
                  if (D_bin_n[icompo + m_index * NCompo]!=0.0)
                    deno_n = deno_n + Y_n[m_index]/D_bin_n[icompo + m_index * NCompo];
                  if (D_bin_s[icompo + m_index * NCompo]!=0.0)
                    deno_s = deno_s + Y_s[m_index]/D_bin_s[icompo + m_index * NCompo];
                  if (D_bin_t[icompo + m_index * NCompo]!=0.0)
                    deno_t = deno_t + Y_t[m_index]/D_bin_t[icompo + m_index * NCompo];
                  if (D_bin_b[icompo + m_index * NCompo]!=0.0)
                    deno_b = deno_b + Y_b[m_index]/D_bin_b[icompo + m_index * NCompo];
                }

                D_mul_e = (deno_e != 0.0) ? ((1.0 - Y_e[icompo])/deno_e):0.0;
                if (D_mul_e > Dg_max) Dg_max = D_mul_e;
                D_mul_w = (deno_w != 0.0) ? ((1.0 - Y_w[icompo])/deno_w):0.0;
                if (D_mul_w > Dg_max) Dg_max = D_mul_w;
                D_mul_n = (deno_n != 0.0) ? ((1.0 - Y_n[icompo])/deno_n):0.0;
                if (D_mul_n > Dg_max) Dg_max = D_mul_n;
                D_mul_s = (deno_s != 0.0) ? ((1.0 - Y_s[icompo])/deno_s):0.0;
                if (D_mul_s > Dg_max) Dg_max = D_mul_s;
                D_mul_t = (deno_t != 0.0) ? ((1.0 - Y_t[icompo])/deno_t):0.0;
                if (D_mul_t > Dg_max) Dg_max = D_mul_t;
                D_mul_b = (deno_b != 0.0) ? ((1.0 - Y_b[icompo])/deno_b):0.0;
                if (D_mul_b > Dg_max) Dg_max = D_mul_b;

                Vfn[j+m*icompo] = Vf[j+m*icompo]
                  + dt_d*dxi2*(D_mul_e*(Vf[j+  1 +m*icompo] - Vf[j+m*icompo]) - D_mul_w*(Vf[j+m*icompo] - Vf[j-  1 +m*icompo]))
                  + dt_d*dyi2*(D_mul_n*(Vf[j+ mx +m*icompo] - Vf[j+m*icompo]) - D_mul_s*(Vf[j+m*icompo] - Vf[j- mx +m*icompo]))
                  + dt_d*dzi2*(D_mul_t*(Vf[j+ mxy+m*icompo] - Vf[j+m*icompo]) - D_mul_b*(Vf[j+m*icompo] - Vf[j- mxy+m*icompo]));

                Vfnt = Vfnt + Vfn[j+m*icompo];
//+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
              }
              Vfnt = inline_clip(Vfnt);
              free(Y_e);
              free(Y_w);
              free(Y_n);
              free(Y_s);
              free(Y_t);
              free(Y_b);
              free(D_bin_e);
              free(D_bin_w);
              free(D_bin_n);
              free(D_bin_s);
              free(D_bin_t);
              free(D_bin_b);
            }
          }
        }

        for(j = 0; j< NCompo*m; j++) Vf[j] = Vfn[j];
        bcs(Y, Vf, val, prm, mtl);
      }

      free(Vfn);
    }

    /* YSE: Update Vp (rho may be changed because of temperature change) */
    init_partial_volume(val, mtl, prm);

//#pragma omp parallel for private(jx,jy,j)
/*added by Chai*/
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        for(jx = 0; jx < nx; jx++) {
          type fj, Vpi;
          type deno;

          j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
          fj = inline_clip(fls[j]);
          if(fj == 0.0) continue;
          deno = 0.0;
          /* Small Vf may cause underflow */
          for(icompo = 0; icompo < NCompo; icompo++) {
            Vpi = Vp[icompo];
            if (Vpi > 0.0) {
              deno += Vf[j+m*icompo]/Vpi;
            }
          }
          for(icompo = 0; icompo < NCompo; icompo++) {
            Vpi = Vp[icompo];
            if (Vpi > 0.0 && deno > 0.0) {
              Y[j+m*icompo] = (Vf[j+m*icompo]/Vpi)/deno;
            } else {
              Y[j+m*icompo] = 0.0;
            }
          }
        }
      }
    }
  }

  /* YSE: Solve diffusion term in gas phase */
  {
    type dti;
    int NBCompo = cdo->NBaseComponent;
    int NTCompo = cdo->NumberOfComponent;
    struct make_matrix_array_Y_data d;
    int r;
    char *name;
    const char *cname;
    type *dens = (flg->two_energy == ON) ? mtl->dens_f : mtl->dens;

    d.dti = 1.0 / cdo->dt;
    d.Dc = mtl->Dcg;
    d.f = val->fls;
    d.f_neg = 1;

    for (icompo = NBCompo; icompo < NTCompo; ++icompo) {
#if defined(JUPITER_MASS_SOURCE_WITHIN_EQUATION)
      type *mass_source_g = NULL;
      int scompo;
      scompo = component_info_find_sorted(icompo, &cdo->mass_source_g_comps);
      if (scompo >= 0)
        mass_source_g = &val->mass_source_g[scompo * cdo->m];
#endif

#pragma omp parallel for
      for(i = 0; i < cdo->n; i++) {
        int jx, jy, jz;
        ptrdiff_t jj;
        type mass;

        calc_struct_index(i, nx, ny, nz, &jx, &jy, &jz);
        jj = calc_address(jx + cdo->stm, jy + cdo->stm, jz + cdo->stm,
                          mx, my, mz);

        mass = 0.0;
#if defined(JUPITER_MASS_SOURCE_WITHIN_EQUATION)
        if (mass_source_g) {
          type dx, dy, dz, volume;
          dx = cdo->x[jx + 1 + cdo->stm] - cdo->x[jx + cdo->stm];
          dy = cdo->y[jy + 1 + cdo->stm] - cdo->y[jy + cdo->stm];
          dz = cdo->z[jz + 1 + cdo->stm] - cdo->z[jz + cdo->stm];
          volume = dx * dy * dz;
          if (dens[jj] > 0.0)
            mass = mass_source_g[jj] / (dens[jj] * volume);
          mass_source_g[jj] = 0.0;
        }
#endif

        mtl->div_b[i] = - (Y[jj+icompo*m] + mass)*d.dti;
      }

      r = jupiter_asprintf(&name, "Yg%d", icompo);
      if (r < 0) {
        cname = "Yg";
        name = NULL;
      } else {
        cname = name;
      }

#ifdef CCSE
      ccse_poisson_f(0, cname, &Y[icompo * cdo->m], mtl->div_b, prm,
                     30000, 1.0e-6, 1.0e-50, make_matrix_array_Y, &d);
#endif
      free(name);
    }
  }

  bcs(Y, Vf, val, prm, mtl);
//  bcf(Y, prm);

//#pragma omp parallel for private(jx,jy,j)
  for(j = 0; j< m; j++) Yt[j] = 0.0;
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
        for(icompo = 0; icompo < NCompo; icompo++)
          Yt[j] += Y[j+m*icompo];
      }
    }
  }
  bcf(Yt, prm);
  return cpu_time() - time0;
}

/* YSE: make matrix array for Vf */
static int make_matrix_array_Vf(int topo_flag, type *A, type *b,
                                int stmx, int stmy, int stmz,
                                int stpx, int stpy, int stpz,
                                int mx, int my, int mz,
                                parameter *prm, void *arg)
{
  domain *cdo = prm->cdo;
  int nx;
  int ny;
  int nz;
  ptrdiff_t nxy;
  ptrdiff_t n;
  ptrdiff_t ns; /* Number of elements in params */
  int m_ = mx * my * mz;
  struct make_matrix_array_Vf_data *data;
  int ncompo;
  int scompo;
  struct dc_calc_param **params;
  type dxi2=cdo->dxi*cdo->dxi,
       dyi2=cdo->dyi*cdo->dyi,
       dzi2=cdo->dzi*cdo->dzi;
  type dti;
  type *t;
  type *Y;
  type *f;
  int commutative;
  int tcompo;
  int m;

  nx = mx - stmx - stpx;
  ny = my - stmy - stpy;
  nz = mz - stmz - stpz;
  nxy = nx * ny;
  n = nxy * nz;

  m = cdo->m;

  data = (struct make_matrix_array_Vf_data *)arg;
  params = data->params;
  ncompo = data->ncompo;
  dti = data->dti;
  t = data->t;
  Y = data->Y;
  f = data->f;
  commutative = data->commutative;
  tcompo = data->target_compo;

  if (commutative) {
    ns = dc_calc_binary_size_commutative(ncompo);
  } else {
    ns = dc_calc_binary_size(ncompo);
  }

#pragma omp parallel
  {
    ptrdiff_t ii;
    type *D_bin_t, *D_bin_b, *D_bin_n, *D_bin_s, *D_bin_e, *D_bin_w;
    type *Y_t, *Y_b, *Y_n, *Y_s, *Y_e, *Y_w;
    int r;
    int jx, jy, jz;

    r = 0;
    D_bin_t = (type *)malloc(sizeof(type) * ns * 6);
    Y_t = (type *)malloc(sizeof(type) * ncompo * 6);
    if (!D_bin_t) {
      r = 1;
    }
    if (!Y_t) {
      r = 1;
    }
    if (r) {
#pragma omp atomic write
      n = 0;
      D_bin_b = D_bin_n = D_bin_s = D_bin_e = D_bin_w = NULL;
      Y_b = Y_n = Y_s = Y_e = Y_w = NULL;
    } else {
      D_bin_b = D_bin_t + ns;
      D_bin_n = D_bin_b + ns;
      D_bin_s = D_bin_n + ns;
      D_bin_e = D_bin_s + ns;
      D_bin_w = D_bin_e + ns;
      Y_b = Y_t + ncompo;
      Y_n = Y_b + ncompo;
      Y_s = Y_n + ncompo;
      Y_e = Y_s + ncompo;
      Y_w = Y_e + ncompo;
    }
#pragma omp for
    for (ii = 0; ii < n; ++ii) {
      type cc, cw, ce, cs, cn, cb, ct;
      int jx, jy, jz;
      ptrdiff_t j_;
      ptrdiff_t jj;
      ptrdiff_t jc, jw, je, js, jn, jb, jt;
      type tc, tw, te, ts, tn, tb, tt;
      type fc, fw, fe, fs, fn, fb, ft;
      type D_mul_w, D_mul_e, D_mul_s, D_mul_n, D_mul_b, D_mul_t;
      ptrdiff_t padr;
      int icompo;
      // index of matrix
      ptrdiff_t icc;
      ptrdiff_t icw;
      ptrdiff_t ice;
      ptrdiff_t ics;
      ptrdiff_t icn;
      ptrdiff_t icb;
      ptrdiff_t ict;

      calc_struct_index(ii, nx, ny, nz, &jx, &jy, &jz);
      j_ = calc_address(jx + stmx, jy + stmy, jz + stmz, mx, my, mz);
      jj = calc_address(jx + cdo->stm, jy + cdo->stm, jz + cdo->stm,
                        cdo->mx, cdo->my, cdo->mz);

      jc = jj;
      jw = jj - 1;
      je = jj + 1;
      js = jj - cdo->mx;
      jn = jj + cdo->mx;
      jb = jj - cdo->mxy;
      jt = jj + cdo->mxy;

      tc = t[jj];
      tt = AVEnume(tc, t[jt]);
      tb = AVEnume(tc, t[jb]);
      tn = AVEnume(tc, t[jn]);
      ts = AVEnume(tc, t[js]);
      te = AVEnume(tc, t[je]);
      tw = AVEnume(tc, t[jw]);

      fc = f[jj];
      ft = f[jt];
      fb = f[jb];
      fn = f[jn];
      fs = f[js];
      fe = f[je];
      fw = f[jw];

      for (icompo = 0; icompo < ncompo; ++icompo) {
        type Y_p;
        Y_p = Y[jj + icompo * m];
        Y_t[icompo] = AVEnume(Y_p, Y[jt + icompo * m]);
        Y_b[icompo] = AVEnume(Y_p, Y[jb + icompo * m]);
        Y_n[icompo] = AVEnume(Y_p, Y[jn + icompo * m]);
        Y_s[icompo] = AVEnume(Y_p, Y[js + icompo * m]);
        Y_e[icompo] = AVEnume(Y_p, Y[je + icompo * m]);
        Y_w[icompo] = AVEnume(Y_p, Y[jw + icompo * m]);
      }

      for (padr = 0; padr < ns; ++padr) {
        int diff_compo;
        int base_compo;
        int ret;

        if (commutative) {
          ret = dc_calc_binary_ids_commutative(padr, ncompo, &diff_compo, &base_compo);
        } else {
          ret = dc_calc_binary_ids(padr, ncompo, &diff_compo, &base_compo);
        }
        if (!ret) continue;
        if (diff_compo != tcompo && base_compo != tcompo) continue;

        D_bin_t[padr] = compute_D_bin(tt, Y_t, fc, ft, params,
                                      diff_compo, base_compo, ncompo,
                                      commutative);
        D_bin_b[padr] = compute_D_bin(tb, Y_b, fc, fb, params,
                                      diff_compo, base_compo, ncompo,
                                      commutative);
        D_bin_n[padr] = compute_D_bin(tn, Y_n, fc, fn, params,
                                      diff_compo, base_compo, ncompo,
                                      commutative);
        D_bin_s[padr] = compute_D_bin(ts, Y_s, fc, fs, params,
                                      diff_compo, base_compo, ncompo,
                                      commutative);
        D_bin_e[padr] = compute_D_bin(te, Y_e, fc, fe, params,
                                      diff_compo, base_compo, ncompo,
                                      commutative);
        D_bin_w[padr] = compute_D_bin(tw, Y_w, fc, fw, params,
                                      diff_compo, base_compo, ncompo,
                                      commutative);
      }

      if (!commutative) {
        ptrdiff_t nsc;

        nsc = dc_calc_binary_size_commutative(ncompo);
        for (padr = 0; padr < nsc; ++padr) {
          ptrdiff_t pfwd, prev;
          type Dfwd, Drev;
          int diff_compo;
          int base_compo;

          if (!dc_calc_binary_ids_commutative(padr, ncompo,
                                              &diff_compo, &base_compo)) {
            continue;
          }
          if (diff_compo != tcompo && base_compo != tcompo) continue;

          pfwd = dc_calc_binary_address(diff_compo, base_compo, ncompo);
          prev = dc_calc_binary_address(base_compo, diff_compo, ncompo);

          Dfwd = D_bin_t[pfwd];
          Drev = D_bin_t[prev];
          Dfwd = compute_D_bin_noncommutative(Dfwd, Drev, Y_t[tcompo]);
          D_bin_t[pfwd] = Dfwd;
          D_bin_t[prev] = Dfwd;

          Dfwd = D_bin_b[pfwd];
          Drev = D_bin_b[prev];
          Dfwd = compute_D_bin_noncommutative(Dfwd, Drev, Y_b[tcompo]);
          D_bin_b[pfwd] = Dfwd;
          D_bin_b[prev] = Dfwd;

          Dfwd = D_bin_n[pfwd];
          Drev = D_bin_n[prev];
          Dfwd = compute_D_bin_noncommutative(Dfwd, Drev, Y_n[tcompo]);
          D_bin_n[pfwd] = Dfwd;
          D_bin_n[prev] = Dfwd;

          Dfwd = D_bin_s[pfwd];
          Drev = D_bin_s[prev];
          Dfwd = compute_D_bin_noncommutative(Dfwd, Drev, Y_s[tcompo]);
          D_bin_s[pfwd] = Dfwd;
          D_bin_s[prev] = Dfwd;

          Dfwd = D_bin_e[pfwd];
          Drev = D_bin_e[prev];
          Dfwd = compute_D_bin_noncommutative(Dfwd, Drev, Y_e[tcompo]);
          D_bin_e[pfwd] = Dfwd;
          D_bin_e[prev] = Dfwd;

          Dfwd = D_bin_w[pfwd];
          Drev = D_bin_w[prev];
          Dfwd = compute_D_bin_noncommutative(Dfwd, Drev, Y_w[tcompo]);
          D_bin_w[pfwd] = Dfwd;
          D_bin_w[prev] = Dfwd;
        }
      }

      D_mul_t = compute_D_mul(Y_t, D_bin_t, tcompo, ncompo, commutative);
      D_mul_b = compute_D_mul(Y_b, D_bin_b, tcompo, ncompo, commutative);
      D_mul_n = compute_D_mul(Y_n, D_bin_n, tcompo, ncompo, commutative);
      D_mul_s = compute_D_mul(Y_s, D_bin_s, tcompo, ncompo, commutative);
      D_mul_e = compute_D_mul(Y_e, D_bin_e, tcompo, ncompo, commutative);
      D_mul_w = compute_D_mul(Y_w, D_bin_w, tcompo, ncompo, commutative);

      cw = D_mul_w * dxi2;
      ce = D_mul_e * dxi2;
      cs = D_mul_s * dyi2;
      cn = D_mul_n * dyi2;
      cb = D_mul_b * dzi2;
      ct = D_mul_t * dzi2;
      cc = -(cw + ce + cs + cn + cb + ct + dti);

      // index of matrix
      icc = j_;
      icw = j_ - 1;
      ice = j_ + 1;
      ics = j_ - nx;
      icn = j_ + nx;
      icb = j_ - nxy;
      ict = j_ + nxy;

      // Neumann-boundary
      if(prm->mpi->nrk[4] == -1 && jx == 0   ) { cc += cw;  icw = -1; }
      if(prm->mpi->nrk[5] == -1 && jx == nx-1) { cc += ce;  ice = -1; }
      if(prm->mpi->nrk[2] == -1 && jy == 0   ) { cc += cs;  ics = -1; }
      if(prm->mpi->nrk[3] == -1 && jy == ny-1) { cc += cn;  icn = -1; }
      if(prm->mpi->nrk[0] == -1 && jz == 0   ) { cc += cb;  icb = -1; }
      if(prm->mpi->nrk[1] == -1 && jz == nz-1) { cc += ct;  ict = -1; }

      if(icb != -1){A[j_+0*m_]=cb;}else{A[j_+0*m_]=0.0;}
      if(ics != -1){A[j_+1*m_]=cs;}else{A[j_+1*m_]=0.0;}
      if(icw != -1){A[j_+2*m_]=cw;}else{A[j_+2*m_]=0.0;}
      if(icc != -1){A[j_+3*m_]=cc;}else{A[j_+3*m_]=0.0;}
      if(ice != -1){A[j_+4*m_]=ce;}else{A[j_+4*m_]=0.0;}
      if(icn != -1){A[j_+5*m_]=cn;}else{A[j_+5*m_]=0.0;}
      if(ict != -1){A[j_+6*m_]=ct;}else{A[j_+6*m_]=0.0;}
    }
    free(D_bin_t);
    free(Y_t);
  }
  if (n == 0) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
              CSV_ERR_NOMEM, 0, 0, NULL);
    prm->status = ON;
  }

  return 0;
}

/* YSE: make matrix array for Y */
static int make_matrix_array_Y(int topo_flag, type *A, type *b,
                               int stmx, int stmy, int stmz,
                               int stpx, int stpy, int stpz,
                               int mx, int my, int mz,
                               parameter *prm, void *arg)
{
  domain *cdo = prm->cdo;
  int nx;
  int ny;
  int nz;
  ptrdiff_t nxy;
  ptrdiff_t n;
  ptrdiff_t ii;
  int m_ = mx * my * mz;
  struct make_matrix_array_Y_data *data;
  struct phase_value_component *compo;
  type *f;
  int f_neg;
  type *Dca;
  type dxi2=cdo->dxi*cdo->dxi,
       dyi2=cdo->dyi*cdo->dyi,
       dzi2=cdo->dzi*cdo->dzi;
  type dti;

  nx = mx - stmx - stpx;
  ny = my - stmy - stpy;
  nz = mz - stmz - stpz;
  nxy = nx * ny;
  n = nxy * nz;

  data = (struct make_matrix_array_Y_data *)arg;
  f = data->f;
  f_neg = data->f_neg;
  Dca = data->Dc;
  dti = data->dti;

#pragma omp parallel for
  for (ii = 0; ii < n; ++ii) {
    type cc, cw, ce, cs, cn, cb, ct;
    int jx, jy, jz;
    ptrdiff_t j_;
    ptrdiff_t jj;
    ptrdiff_t jc, jw, je, js, jn, jb, jt;
    type fc, fw, fe, fs, fn, fb, ft;
    type Dc, Dw, De, Ds, Dn, Db, Dt;

    calc_struct_index(ii, nx, ny, nz, &jx, &jy, &jz);
    j_ = calc_address(jx + stmx, jy + stmy, jz + stmz, mx, my, mz);
    jj = calc_address(jx + cdo->stm, jy + cdo->stm, jz + cdo->stm,
                      cdo->mx, cdo->my, cdo->mz);

    jc = jj;
    jw = jj - 1;
    je = jj + 1;
    js = jj - cdo->mx;
    jn = jj + cdo->mx;
    jb = jj - cdo->mxy;
    jt = jj + cdo->mxy;

    if (f_neg) {
      fc = inline_clip(1.0 - f[jc]);
      fw = inline_clip(1.0 - f[jw]);
      fe = inline_clip(1.0 - f[je]);
      fs = inline_clip(1.0 - f[js]);
      fn = inline_clip(1.0 - f[jn]);
      fb = inline_clip(1.0 - f[jb]);
      ft = inline_clip(1.0 - f[jt]);
    } else {
      fc = inline_clip(f[jc]);
      fw = inline_clip(f[jw]);
      fe = inline_clip(f[je]);
      fs = inline_clip(f[js]);
      fn = inline_clip(f[jn]);
      fb = inline_clip(f[jb]);
      ft = inline_clip(f[jt]);
    }
    Dc = Dca[jc];
    Dw = Dca[jw];
    De = Dca[je];
    Ds = Dca[js];
    Dn = Dca[jn];
    Db = Dca[jb];
    Dt = Dca[jt];

    cw = type_min(fc, fw) * 0.5 * (Dw + Dc)*dxi2;
    ce = type_min(fc, fe) * 0.5 * (Dc + De)*dxi2;
    cs = type_min(fc, fs) * 0.5 * (Ds + Dc)*dyi2;
    cn = type_min(fc, fn) * 0.5 * (Dc + Dn)*dyi2;
    cb = type_min(fc, fb) * 0.5 * (Db + Dc)*dzi2;
    ct = type_min(fc, ft) * 0.5 * (Dc + Dt)*dzi2;
    cc = - (cw + ce + cs + cn + cb + ct + dti + 1.0e-8);

    // index of matrix
    ptrdiff_t icc = j_;
    ptrdiff_t icw = j_ - 1;
    ptrdiff_t ice = j_ + 1;
    ptrdiff_t ics = j_ - nx;
    ptrdiff_t icn = j_ + nx;
    ptrdiff_t icb = j_ - nxy;
    ptrdiff_t ict = j_ + nxy;
    // Neumann-boundary
    if(prm->mpi->nrk[4] == -1 && jx == 0   ) { cc += cw;  icw = -1; }
    if(prm->mpi->nrk[5] == -1 && jx == nx-1) { cc += ce;  ice = -1; }
    if(prm->mpi->nrk[2] == -1 && jy == 0   ) { cc += cs;  ics = -1; }
    if(prm->mpi->nrk[3] == -1 && jy == ny-1) { cc += cn;  icn = -1; }
    if(prm->mpi->nrk[0] == -1 && jz == 0   ) { cc += cb;  icb = -1; }
    if(prm->mpi->nrk[1] == -1 && jz == nz-1) { cc += ct;  ict = -1; }

    if(icb != -1){A[j_+0*m_]=cb;}else{A[j_+0*m_]=0.0;}
    if(ics != -1){A[j_+1*m_]=cs;}else{A[j_+1*m_]=0.0;}
    if(icw != -1){A[j_+2*m_]=cw;}else{A[j_+2*m_]=0.0;}
    if(icc != -1){A[j_+3*m_]=cc;}else{A[j_+3*m_]=0.0;}
    if(ice != -1){A[j_+4*m_]=ce;}else{A[j_+4*m_]=0.0;}
    if(icn != -1){A[j_+5*m_]=cn;}else{A[j_+5*m_]=0.0;}
    if(ict != -1){A[j_+6*m_]=ct;}else{A[j_+6*m_]=0.0;}
  }

  return 0;
}
