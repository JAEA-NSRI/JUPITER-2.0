
#ifndef MATERIALS_H
#define MATERIALS_H

#include "common.h"
#ifdef __cplusplus
extern "C" {
#endif

#include "struct.h"
#include "tempdep_calc.h"
#include "component_info.h"

/*
 * YSE: Inline functions to improve performance
 */

/**
 * @brief clipping function
 * @param x value to clip [0, 1]
 * @return clipped value
 */
static inline type clipx(type x)
{
  if (x < 0.0) return 0.0;
  if (x > 1.0) return 1.0;
  return x;
}

/*
 * YSE: Commonized mean functions
 *
 * This function set can compute any mean of
 *
 * ```
 *  Σ n(w_i, x_i)
 * --------------- .
 *  Σ d(w_i, x_i)
 * ```
 * with providing n(w, x) and d(w, x), where w is weight (VOF or Y
 * here) for x and x is value.
 */
/*
 * Note: function inlining should be performed at optimization,
 *       othrewise can be slow.
 */

/* Numerator of Weighted Arithmetic Mean */
static inline type WAMnume(type w, type x)
{
  return w * x;
}

/* Denominator of Weighted Arithmetic Mean */
static inline type WAMdeno(type w, type x)
{
  return w;
}

/* Numerator of Weighted Harmonic Mean */
static inline type WHMnume(type w, type x)
{
  return w;
}

/* Denominator of Weighted Harmonic Mean */
static inline type WHMdeno(type w, type x)
{
  return w / x;
}

static inline type
YNPHVb(int nc, int ncg, type *Y, type *Yg, type *fs, type *fl, type fg, type *ms, type *ml, type *mg, type (*nfunc)(type, type), type (*dfunc)(type, type))
{
  int i;
  type denom;
  type numer;

  denom = 0.0;
  numer = 0.0;
  for (i = 0; i < nc; ++i) {
    type s, l;
    s = 1.0;
    l = 1.0;
    if (Y) {
      if (ms) s = Y[i] * (fs ? fs[0] : 1.0);
      if (ml) l = Y[i] * (fl ? fl[0] : 1.0);
    } else {
      if (ms) s = (fs ? fs[i] : 1.0);
      if (ml) l = (fl ? fl[i] : 1.0);
    }
    if (ms) {
      numer += nfunc(s, ms[i]);
      denom += dfunc(s, ms[i]);
    }
    if (ml) {
      numer += nfunc(l, ml[i]);
      denom += dfunc(l, ml[i]);
    }
  }
  if (mg) {
    type g;
    if (Yg) {
      for (i = 0; i < ncg; ++i) {
        g = fg * Yg[i];
        numer += nfunc(g, mg[i]);
        denom += dfunc(g, mg[i]);
      }
    } else {
      numer += nfunc(fg, mg[0]);
      denom += nfunc(fg, mg[0]);
    }
  }
  if (numer == 0.0 && denom == 0.0) {
    return 0.0;
  } else {
    return numer / denom;
  }
}

/**
 * @brief Weighted Arithmetic Mean of components
 * @param nc  Number of solid-liquid components
 * @param ncg Number of gas components
 * @param Y  Solute density Y
 * @param Yg Gas fraction Yg
 * @param fs Solid VOF
 * @param fl Liquid VOF
 * @param fg Gas VOF
 * @param ms Properties for Solid components
 * @param ml Properties for Liquid components
 * @param mg Property for Gas component
 * @return computed result
 *
 * Each arguments should be defined as followings:
 *  - Common
 *    * ms --- array of nc if property is applicable to fs, or give NULL
 *    * ml --- array of nc if property is applicable to fl, or give NULL
 *    * mg --- array of ncg if property is applicable to fg and Yg is not NULL,
 *             or array of 1 if property is applicable to fg and Yg is NULL,
 *             or give NULL
 *
 *  - If solute diff model is applicable:
 *    * Y  --- array of nc
 *    * fs --- array of 1 if specify
 *    * fl --- array of 1 if specify
 *
 *  - If solute diff model is not applicable:
 *    * Y  --- Give NULL
 *    * fs --- array of nc if specify
 *    * fl --- array of nc if specify
 *
 *  - If gas component is present (density, spec heat, etc.):
 *    * Yg --- array of ncg
 *    * mg --- array of ncg
 *
 *  - If gas component is not present (beta, sigma, tm_liq, etc.):
 *    * fg or mg --- Give NULL
 *
 * `fs` or `fl` is treated as 1.0 if the corresponding variable is NULL.
 */
static inline type
YNPHV1c(int nc, int ncg, type *Y, type *Yg, type *fs, type *fl, type fg, type *ms, type *ml, type *mg)
{
  return YNPHVb(nc, ncg, Y, Yg, fs, fl, fg, ms, ml, mg, WAMnume, WAMdeno);
}

/**
 * @brief General weighted arithmetic average
 * @param nc Number of elements
 * @param w Array of weights
 * @param a Array of value to average
 * @return result
 */
static inline type YNPHV1w(int nc, type *w, type *a)
{
  return YNPHV1c(nc, 0, w, NULL, NULL, NULL, 0.0, a, NULL, NULL);
}

/**
 * @brief General unweighted arithmetic average
 * @param nc Number of elements
 * @param a Array of value to average
 * @return result
 */
static inline type YNPHV1u(int nc, type *a)
{
  return YNPHV1c(nc, 0, NULL, NULL, NULL, NULL, 0.0, a, NULL, NULL);
}

/**
 * @brief Weighted Harmonic Mean of components
 * @param nc  Number of soild-liquid components
 * @param ncg Number of gas components
 * @param Y  Solute density Y
 * @param fs Solid VOF
 * @param fl Liquid VOF
 * @param fg Gas VOF
 * @param ms Properties for Solid components
 * @param ml Properties for Liquid components
 * @param mg Property for Gas component
 * @return computed result
 *
 * See NPHV1c for argument details.
 */
static inline type
YNPHV2c(int nc, int ncg, type *Y, type *Yg, type *fs, type *fl, type fg, type *ms, type *ml, type *mg)
{
  return YNPHVb(nc, ncg, Y, Yg, fs, fl, fg, ms, ml, mg, WHMnume, WHMdeno);
}

/**
 * @brief General weighted harmonic average
 * @param nc Number of elements
 * @param w Array of weights
 * @param a Array of value to average
 * @return result
 */
static inline type YNPHV2w(int nc, type *w, type *a)
{
  return YNPHV2c(nc, 0, w, NULL, NULL, NULL, 0.0, a, NULL, NULL);
}

/**
 * @brief General unweighted harmonic average
 * @param nc Number of elements
 * @param a Array of value to average
 * @return result
 */
static inline type YNPHV2u(int nc, type *a)
{
  return YNPHV2c(nc, 0, NULL, NULL, NULL, NULL, 0.0, a, NULL, NULL);
}

/**
 * @brief Add or Remove mass of specific component from a cell using
 * renormalization
 * @param dens Density of the cell [kg/m3]
 * @param volume Volume of the cell [m3]
 * @param nbcompo Number of solid-liquid components
 * @param ncompo Number of components (= array size) of @p Y
 * @param Y array of mass fractions [-]
 * @param comps_to_add Component id array on corresponding @p comps_to_add
 * @param mass_to_add Array of mass values to add.
 * @return -1 if success, ncompo if total mass is zero, or first
 *         component ID that will cause negative mass
 *
 * First @p nbcompo if @p Y is treated as solid-liquid components.
 * Other components treated as gas-only components
 *
 * @note The content of @p Y will be undefined when returning error.
 */
static inline int add_mass_with_renormal(type dens, type volume,
                                         int nbcompo, int ncompo, type *Y,
                                         struct component_info *comps_to_add,
                                         const type *mass_to_add)
{
  int nids;
  int i;
  type total_sl_mass, total_g_mass;
  type rho_v = dens * volume;

  nids = component_info_ncompo(comps_to_add);

  /* Use Y as temporary array */
  for (i = 0; i < ncompo; ++i) {
    if (Y[i] > 0.0) {
      Y[i] = rho_v * Y[i];
    } else {
      Y[i] = 0.0;
    }
  }

  for (i = 0; i < nids; ++i) {
    int id;
    id = component_info_getc(comps_to_add, i)->comp_index;
    if (id >= 0 && id < ncompo)
      Y[id] += mass_to_add[i];
  }

  total_sl_mass = 0.0;
  total_g_mass = 0.0;
  for (i = 0; i < ncompo; ++i) {
    if (Y[i] < 0.0)
      return i;
    if (i < nbcompo) {
      total_sl_mass += Y[i];
    } else {
      total_g_mass += Y[i];
    }
  }
  if (total_sl_mass <= 0.0 && total_g_mass <= 0.0)
    return ncompo;

  for (i = 0; i < ncompo; ++i) {
    if (i < nbcompo) {
      if (total_sl_mass > 0.0) {
        Y[i] /= total_sl_mass;
      } else {
        Y[i] = 0.0;
      }
    } else {
      if (total_g_mass > 0.0) {
        Y[i] /= total_g_mass;
      } else {
        Y[i] = 0.0;
      }
    }
  }
  return -1;
}

/**
 * @brief Add or Remove mass of specific component from a cell with
 * breking constraint on Y (sum(Y) == 1)
 * @param dens Density of the cell [kg/m3]
 * @param volume Volume of the cell [m3]
 * @param nbcompo Number of solid-liquid components
 * @param ncompo Number of components (= array size) of @p Y
 * @param Y array of mass fractions [-]
 * @param comps_to_add Component id array on corresponding @p comps_to_add
 * @param mass_to_add Array of mass values to add.
 * @return always -1.
 *
 * First @p nbcompo if @p Y is treated as solid-liquid components.
 * Other components treated as gas-only components
 *
 * @note You should renormal @p Y after setting density in set_materials().
 *
 * The return value is matching to `add_mass_with_renormal()`. This function
 * ignores negative mass.
 */
static inline int
add_mass_with_break_constraint(type dens, type volume, int nbcompo,
                               int ncompo, type *Y,
                               struct component_info *comps_to_add,
                               const type *mass_to_add)
{
  type irho_v;
  int i;
  int nids = component_info_ncompo(comps_to_add);

  irho_v = 1.0 / (dens * volume);
  for (i = 0; i < nids; ++i) {
    int id = component_info_getc(comps_to_add, i)->comp_index;
    if (id >= 0 && id < ncompo)
      Y[i] += mass_to_add[i] * irho_v;
  }

  return -1;
}

/**
 * @brief Add or Remove mass of specific component from a cell
 * @param dens Density of the cell [kg/m3]
 * @param volume Volume of the cell [m3]
 * @param nbcompo Number of solid-liquid components
 * @param ncompo Number of components (= array size) of @p Y
 * @param Y array of mass fractions [-]
 * @param comps_to_add Component id array on corresponding @p comps_to_add
 * @param mass_to_add Array of mass values to add.
 * @return -1 if success, ncompo if total mass is zero, or first
 *         component ID that will cause negative mass
 *
 * First @p nbcompo if @p Y is treated as solid-liquid components.
 * Other components treated as gas-only components
 */
JUPITER_DECL
int add_mass_to_comp(type dens, type volume, int nbcompo, int ncompo, type *Y,
                     struct component_info *comps_to_add,
                     const type *mass_to_add);

/**
 * @brief Converts mass fractions Y to mole fractions X.
 * @param X mole fraction [-] [out] [ncompo]
 * @param Y mass fraction [-] [in]  [ncompo]
 * @param molar masses for each components [kg/kmol] [in] [ncompo]
 * @param ncompo Number of components
 *
 * @p ncompo is equal to the array size of @p X or @p Y, and no
 * 'ramainder' components assumed. Include it explicitly if exists.
 *
 * @note This function never normalize Y (i.e., assumes that sum(Y) == 1).
 */
static inline void mass_fraction_to_mole_fraction(type *X, const type *Y,
                                                  const type *molar_mass,
                                                  int ncompo)
{
  int f;
  type yj = 0.0;
  for (int i = 0; i < ncompo; ++i)
    yj += (X[i] = Y[i] / molar_mass[i]);
  if (yj > 0.0) {
    for (int i = 0; i < ncompo; ++i)
      X[i] /= yj;
  } else {
    for (int i = 0; i < ncompo; ++i)
      X[i] = 0.0;
  }
}

/**
 * @brief Converts mass fractions Y to mole fractions X.
 * @param Y mass fraction [-] [out] [ncompo]
 * @param X mole fraction [-] [in]  [ncompo]
 * @param molar masses for each components [kg/kmol] [in] [ncompo]
 * @param ncompo Number of components
 *
 * @p ncompo is equal to the array size of @p X or @p Y, and no
 * 'ramainder' components assumed. Include it explicitly if exists.
 *
 * @note This function never normalize X (i.e., assumes that sum(X) == 1).
 */
static inline void mole_fraction_to_mass_fraction(type *Y, const type *X,
                                                  const type *molar_mass,
                                                  int ncompo)
{
  type xj = 0.0;
  for (int i = 0; i < ncompo; ++i)
    xj += (Y[i] = X[i] * molar_mass[i]);
  if (xj > 0.0) {
    for (int i = 0; i < ncompo; ++i)
      Y[i] /= xj;
  } else {
    for (int i = 0; i < ncompo; ++i)
      Y[i] = 0.0;
  }
}

#ifdef __cplusplus
}
#endif

#endif
