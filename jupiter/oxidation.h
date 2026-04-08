
#ifndef JUPITER_OXIDATION_H
#define JUPITER_OXIDATION_H

#include "common.h"
#include "component_data_defs.h"
#include "component_info_frac.h"
#include "oxidation_defs.h"
#include "struct.h"
#include "csv.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline int ox_component_info_ncompo(struct ox_component_info *info)
{
  return component_info_frac_ncompo(&info->comps);
}

static inline void ox_component_info_init(struct ox_component_info *info)
{
  component_info_frac_init(&info->comps);
}

static inline int ox_component_info_resize(struct ox_component_info *info,
                                           int n, int copy)
{
  return component_info_frac_resize(&info->comps, n, copy);
}

static inline void ox_component_info_clear(struct ox_component_info *info)
{
  component_info_frac_clear(&info->comps);
}

static inline void ox_component_info_seti(struct ox_component_info *info,
                                          int index, int compo)
{
  component_info_frac_seti(&info->comps, index, compo);
}

static inline void ox_component_info_setc(struct ox_component_info *info,
                                          int index, struct component_data *d)
{
  component_info_frac_setc(&info->comps, index, d);
}

static inline void ox_component_info_setf(struct ox_component_info *info,
                                          int index, component_info_frac_type f)
{
  component_info_frac_setf(&info->comps, index, f);
}

static inline int ox_component_info_geti(struct ox_component_info *info,
                                         int index)
{
  return component_info_frac_geti(&info->comps, index);
}

static inline struct component_data *
ox_component_info_getc(struct ox_component_info *info, int index)
{
  return component_info_frac_getc(&info->comps, index);
}

static inline component_info_frac_type
ox_component_info_getf(struct ox_component_info *info, int index)
{
  return component_info_frac_getf(&info->comps, index);
}

static inline struct component_info_data *
ox_component_info_getcp(struct ox_component_info *info, int index)
{
  return component_info_frac_getcp(&info->comps, index);
}

/**
 * @param idx Index of ox_component_info array.
 * @param id ID given as input
 * @param d Corresponding component data
 * @param fname Input file name
 * @param row Row pointer in input
 * @param col Column pointer in input
 * @retval 1 the id is ok.
 * @retval 0 the id is not ok.
 *
 * You have not to check for duplications.
 *
 * The function will not be called if corresponding component_data is not found.
 *
 * If optional is true, this function will not be called for ID -1.
 */
typedef int ox_set_component_info_id_check(int idx, int id, component_data *d,
                                           const char *fname, csv_row *row,
                                           csv_column *col, void *arg);

/**
 * @param idx Index of ox_component_info array.
 * @param id ID given as input
 * @param d Corresponding component data
 * @param frac Fraction value.
 * @param fname Input file name
 * @param row Row pointer in input
 * @param col Column pointer in input
 * @retval 1 the fraction is ok.
 * @retval 0 the fraction is not ok.
 *
 * You have not to check for sums. @p d may be NULL.
 */
typedef int ox_set_component_info_frac_check(int idx, int id, component_data *d,
                                             type frac, const char *fname,
                                             csv_row *row, csv_column *col,
                                             void *arg);

JUPITER_DECL
int ox_set_component_info(struct ox_component_info *info,
                          component_data *comp_data_head, int max_num_compo,
                          int read_fraction, int optional, const char *keyname,
                          csv_data *csv, const char *fname, csv_row **row,
                          csv_column **col,
                          ox_set_component_info_id_check *id_check,
                          ox_set_component_info_frac_check *frac_check,
                          void *arg, int *stat);

/*
 * These checkers does not use @p arg.
 */

JUPITER_DECL
int ox_set_component_info_id_check_zry(int idx, int id, component_data *d,
                                       const char *fname, csv_row *row,
                                       csv_column *col, void *arg);

JUPITER_DECL
int ox_set_component_info_id_check_zro2(int idx, int id, component_data *d,
                                        const char *fname, csv_row *row,
                                        csv_column *col, void *arg);

JUPITER_DECL
int ox_set_component_info_id_check_h2(int idx, int id, component_data *d,
                                      const char *fname, csv_row *row,
                                      csv_column *col, void *arg);

JUPITER_DECL
int ox_set_component_info_id_check_h2o(int idx, int id, component_data *d,
                                       const char *fname, csv_row *row,
                                       csv_column *col, void *arg);

JUPITER_DECL
int oxidation_init(parameter *prm, domain *cdo, type *Y, type *Yt, type *fs,
                   type *fl, int ncompo, struct ox_component_info *ox_zry,
                   struct ox_component_info *ox_zro2,
                   struct ox_component_info *ox_h2o, type *ox_lset,
                   type *ox_vof, int *ox_flag, type *ox_f_h2o,
                   type ox_h2o_threshold, type *ox_lset_h2o,
                   type *ox_lset_h2o_s);

/**
 * @brief Extract Y for IDs defined by a component information
 * @param Yret Array of Y (out, selective)
 * @param idret Array of IDs (out, selective)
 * @param oncompo Allocated number of elements of @p Yret and/or @p idret
 * @param Y Array of Y (in, JUPITER-dimension)
 * @param m Number of elements per component in Y (= cdo->m)
 * @param index Index value for extraction.
 * @param incompo Maximum number of component ID.
 * @param comp_info Component information to be used for extraction.
 * @retval -1 Error, no component IDs defined in @p comp_info
 * @retval  0 Error, one or more IDs out-of-range (0...incompo).
 * @retval  n Number of elements has been set to @p Yret and/or @p idret
 *
 * @p Yret and/or @p idret can be NULL. If so, the extraction will not
 * be done for NULL-given array(s).
 */
JUPITER_DECL
int ox_component_make_Yarr(type *Yret, int *idret, int oncompo, type *Y,
                           ptrdiff_t m, ptrdiff_t index, int incompo,
                           struct ox_component_info *comp_info);

struct oxidation_calc_args
{
  parameter *prm;      ///< [In] JUPITER parameter
  domain *cdo;         ///< [In] JUPITER domain
  int fluid_dynamics;  ///< [In] Whether fluid dynamics are calculated
  type delta_t;        ///< Time step width
  type ox_q_fac;       ///< Factor for generating heat by oxidation
  type *Y;             ///< [In/Out] Concentration data [m * NCompo]
  type *fs;            ///< [In] (total) Solid VOF [m]
  type *fl;            ///< [In] (total) Liquid VOF [m]
  type *t;             ///< [In] Temperature data [m]
  type *ox_dt;         ///< [In/Out] Oxidation thickness [m]
  type *ox_dt_local;   ///< [In/Out] Cell local Oxidation thickness [m]
  int *ox_flag;        ///< [In/Out] Oxidation flag [m]
  type *ox_lset;       ///< [In/Out] Oxidation level-set [m]
  type *ox_vof;        ///< [In/Out] Oxidation VOF [m]
  type *ox_q;          ///< [Out] Generated heat by oxidation (optional) [m]
  type *ox_Yh2;        ///< [In/Out] Amount of H2 for generation (optional) [m]
  type *ls;            ///< [In] Level Set (surface of Solid) [m]
  type *ox_kp;         ///< [Out] Reaction Rate (optional) [m]
  type *ox_dens;       ///< [Out] Density of Zircaloy (optional) [m]
  type *ox_Yh2_div;    ///< [Out] RHS for solving Yh2 in implicit method [n]
  type ox_diff_h2_sol; ///< Diffusion coeff. of Yh2 in Solid
  type ox_diff_h2_srf; ///< Diffusion coeff. of Yh2 at surface
  tempdep_property *ox_kp_func;      ///< [In] Reaction Rate function [1]
  phase_value_component *comps;      ///< [In] Physical property data [NCompo]
  struct ox_component_info *ox_zry;  ///< [In] Component Data for Zircaloy [1]
  struct ox_component_info *ox_zro2; ///< [In] Component Data for ZrO2 [1]
  struct ox_component_info *ox_h2;   ///< [In] Component Data for H2 [1]
  struct ox_component_info *ox_h2o;  ///< [In] Component Data for H2O [1]
  type ox_recess_init;               ///< [In] Initial recession delta
  type ox_recess_min;                ///< [In] Minimum recession thickness
  tempdep_property *ox_recess_rate;  ///< [In] Recession rate function [1]
  type
    ox_h2o_threshold; ///< [In] Threshold that evaluates to enough H2O present
  type ox_h2o_lset_min_to_recess; ///< [In] Minimum required increase in the
                                  ///< ox_h2o_lset
  type *ox_f_h2o;                 ///< [Out] Region of enough H2O is exist [m]
  type *ox_lset_h2o;   ///< [In/Out] Level Set (surface of enough H2O) [m]
                       ///< Inputted values will be used for initial values for
                       ///< iteration of LevelSet().
  type *ox_lset_h2o_s; ///< [In/Out] Saved value for ox_lset_h2o to see change
  type *ox_recess_k;   ///< [Out] Recession rate (optional) [m]

  type *p; ///< [In] Relative Pressure for H2 absorption calculation
  type *h2_absorp_eval; ///< [Out] H2 absorption prediction [nz], skips if NULL
  type *h2_absorp_Ks; ///< [Out] H2 absorption Ks [nz] (reuired if h2_aborp_eval
                      ///< is not NULL)
  type *h2_absorp_P;  ///< [Out] H2 absorption averaged pressure [nz] (ditto)
  type *h2_absorp_T;  ///< [Out] H2 absorption prediction [nz] (ditto)
  type *h2_absorp_Zr; ///< [Out] H2 absorption prediction [nz] (ditto)
  type h2_absorp_base_p;       ///< [In] Reference pressure for H2 absorption
  int h2_absorp_eval_P_change; ///< [In] Whether include P change for H2
                               ///< absorption
};

JUPITER_DECL
type oxidation_calc(struct oxidation_calc_args *p);

/* Inlinable oxidation primitives */

/**
 * @brief Max
 * @param a Comparator
 * @param b Comparator
 * @return Maximum value of @p a and @p b
 */
static inline type ox_type_max(type a, type b)
{
  if (a < b)
    return b;
  return a;
}

/**
 * @brief Min
 * @param a Comparator
 * @param b Comparator
 * @return Minimum value of @p a and @p b
 */
static inline type ox_type_min(type a, type b)
{
  if (a > b)
    return b;
  return a;
}

/**
 * @brief Lamdba function prototype for ox_type_a_inject.
 * @param a Last evaluated value or First element of the list
 *          for first evaluation
 * @param b Next element of the list
 * @param i Array index of @p b
 * @param arg Extra arguments given for ox_type_a_inject().
 * @return Evaluated result.
 */
typedef type ox_type_a_inject_lambda(type a, type b, int i, void *arg);

/**
 * @brief General injection (aka. reduction) loop for type array
 * @param a List of type.
 * @param n Number of elements in @p a
 * @param arg Extra arguments should be given for @p lambda
 * @param lambda function operation should be performed
 * @return Last evaluated value of @p lambda
 *
 * @note This function cannot be make parallel. This possibilty
 *       depends on the content of @p lambda function.
 */
static inline type ox_type_a_inject(type *a, int n, void *arg,
                                    ox_type_a_inject_lambda *lambda)
{
  int i;
  type m;
  m = a[0];
  for (i = 1; i < n; ++i) {
    m = lambda(m, a[i], i, arg);
  }
  return m;
}

static inline type ox_type_a_sum_proc(type a, type b, int i, void *arg)
{
  return a + b;
}

static inline type ox_type_a_min_proc(type a, type b, int i, void *arg)
{
  if (a > b) {
    if (arg)
      *((int *)arg) = i;
    return b;
  }
  return a;
}

static inline type ox_type_a_max_proc(type a, type b, int i, void *arg)
{
  if (a < b) {
    if (arg)
      *((int *)arg) = i;
    return b;
  }
  return a;
}

/**
 * @brief List max
 * @param a List to compute maximum
 * @param n Number of elements
 * @param max_i If given, sets index for maximum
 * @return maximum value.
 */
static inline type ox_type_amax(type *a, int n, int *max_i)
{
  if (max_i)
    *max_i = 0;
  return ox_type_a_inject(a, n, max_i, ox_type_a_max_proc);
}

/**
 * @brief List min
 * @param a List to compute minimum
 * @param n Number of elements
 * @param min_i If given, sets index for minimum
 * @return minimum value.
 */
static inline type ox_type_amin(type *a, int n, int *min_i)
{
  if (min_i)
    *min_i = 0;
  return ox_type_a_inject(a, n, min_i, ox_type_a_min_proc);
}

/**
 * @brief List sum
 * @param a List to compute sum
 * @param n Number of elements
 * @return minimum value.
 */
static inline type ox_type_asum(type *a, int n)
{
  return ox_type_a_inject(a, n, NULL, ox_type_a_sum_proc);
}

/**
 * @brief Concentration clip
 * @param a value to clip
 * @retval 1 if @p a is greater than 1.
 * @retval 0 if @p a is less than 0.
 * @retval a otherwise
 */
static inline type ox_f_clip(type a)
{
  if (a >= 1.0)
    return 1.0;
  if (a <= 0.0)
    return 0.0;
  return a;
}

/**
 * @brief huge value
 * @return Positive huge value of type.
 *
 * If the target system uses IEEE-754 format for floating point
 * values, the returning value can be INFINITY.
 *
 * If the compiler does not define `HUGE_VALF`, this function returns
 * `HUGE_VAL` instead. However this can cause overflow error when FPE
 * is enabled.
 */
static inline type ox_huge_val(void)
{
#if defined(JUPITER_DOUBLE) || !defined(HUGE_VALF)
  return HUGE_VAL;
#else
  return HUGE_VALF;
#endif
}

/**
 * @brief Return true the component is 'enabled' for evaluation.
 * @param comp Component data to be tested
 * @retval 1 enabled
 * @retval 0 disabled
 *
 * Returns 0 if @p comp is NULL, number of components defined is 0 (or
 * negative).
 *
 * Returns 1 otherwise. Note that this includes the ID may contain -1.
 * The input or initialization process should adjust if you want to
 * allow the ID -1 for disabling some feature.
 */
static inline int ox_is_enabled_component(struct ox_component_info *comp)
{
  if (!comp)
    return 0;
  if (ox_component_info_ncompo(comp) <= 0)
    return 0;
  return 1;
}

#ifdef __cplusplus
}
#endif

#endif
