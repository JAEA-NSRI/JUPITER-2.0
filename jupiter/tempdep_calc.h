
/**
 * @file tempdep_calc.h
 * @brief Temperature dependent property calculations.
 */

#ifndef TEMPDEP_CALC_H
#define TEMPDEP_CALC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <math.h>

#include "tempdep_properties.h"

static inline double tempdep_calc(tempdep_property *prop, double temp);

/**
 * @memberof tempdep_property
 * @brief Find index of given region belongs
 * @param f Value to search
 * @param ta Array of region boundaries
 * @param n Number of elements in @p ta
 * @return Upper boundary index (i.e., 0 means f < ta[0],
 *         1 means ta[0] <= f < ta[1])
 * @retval -1 @p f is NaN
 *
 * @p ta must be sorted by ascending order, otherwise returns
 * incorrect result.
 *
 * This function uses binary search.
 */
static inline int tempdep_domain_search(double f, double *ta, int n)
{
  int i, j, k;

  if (f < ta[0]) {
    return 0;
  }
  if (f >= ta[n - 1]) {
    return n;
  }

  i = 0;
  j = n - 1;
  k = i;
  while (i != j) {
    k = (j - i) / 2 + i;
    if (f < ta[k]) {
      if (f >= ta[k - 1]) {
        return k;
      }
      j = k;
    } else {
      if (f < ta[k + 1]) {
        return k + 1;
      }
      i = k;
    }
  }
  if (f >= ta[k - 1] && f < ta[k]) {
    return k;
  }
  return -1;
}

/**
 * @memberof tempdep_property
 * @brief Temperature dependent property calculation for CONST.
 * @param data Property data
 * @param temp Temperature value to be used for.
 * @return result
 *
 * @note Do not call this function directly.
 */
static inline
double tempdep_const_calc(union tempdep_property_u *data, double temp)
{
  struct tempdep_const_data *cdata;

  cdata = &data->t_const;
  return cdata->value;
}

/**
 * @memberof tempdep_property
 * @brief Temperature dependent property calculation for TABLE.
 * @param data Property data
 * @param temp Temperature value to be used for.
 * @return result
 *
 * @note Do not call this function directly.
 */
static inline
double tempdep_table_calc(union tempdep_property_u *data, double temp)
{
  struct tempdep_table_data *tdata;
  double r;

  r = 0.0;
  tdata = &data->t_table;

  if (tdata->data) {
    r = table_search(tdata->data, temp, 0.0, NULL);
  }
  return r;
}

/**
 * @memberof tempdep_property
 * @brief Temperature dependent property domain restriction.
 * @param dom Domain data
 * @param temp Temperature value to be used for.
 * @return result
 *
 * Minimum possible return value will be @p dom->minT and maximum
 * possible return vlaue will be @p dom->maxT. If @p temp is NaN,
 * this function returns @p temp itself (i.e., NaN).
 *
 * @note Do not call this function directly.
 */
static inline
double tempdep_limit_domain(struct tempdep_domain_data *dom, double temp)
{
  if (temp <= dom->minT) return dom->minT;
  if (temp >= dom->maxT) return dom->maxT;
  return temp;
}

/**
 * @memberof tempdep_property
 * @brief Temperature dependent property domain restriction.
 * @param dom Domain data
 * @param temp Temperature value to be used for.
 * @param low_out Temperature to be used for temp is less than lower boundary
 * @param up_out Temperature to be used for temp is greater than upper boundary
 * @return result
 *
 * If @p temp is NaN, this function returns @p temp itself (i.e.,
 * NaN).
 *
 * @note Do not call this function directly.
 */
static inline double
tempdep_limit_domain_specify(struct tempdep_domain_data *dom, double temp,
                             double low_out, double up_out)
{
  if (temp < dom->minT) return low_out;
  if (temp > dom->maxT) return up_out;
  return temp;
}

/**
 * @memberof tempdep_property
 * @brief Temperature dependent property calculation for POLY.
 * @param data Property data
 * @param temp Temperature value to be used for.
 * @return result
 *
 * @note Do not call this function directly.
 */
static inline
double tempdep_poly_calc(union tempdep_property_u *data, double temp)
{
  struct tempdep_poly_data *pdata;
  double r;
  int i;

  r = 0.0;
  pdata = &data->t_poly;

  temp = tempdep_limit_domain(&pdata->dom, temp);

  if (pdata->vec) {
    for (i = -1; i < pdata->polymax; ++i) {
      r = r * temp + pdata->vec[i + 1];
    }
  }
  return r;
}

/**
 * @memberof tempdep_property
 * @brief Temperature dependent property calculation for POLY_L.
 * @param data Property data
 * @param temp Temperature value to be used for.
 * @return result
 *
 * @note Do not call this function directly.
 */
static inline
double tempdep_poly_l_calc(union tempdep_property_u *data, double temp)
{
  struct tempdep_poly_l_data *pdata;
  double r;
  double t[3];
  int q;

  r = 0.0;
  pdata = &data->t_poly_l;

  temp = tempdep_limit_domain(&pdata->dom, temp);
  if (temp == 0.0 && pdata->polymin < 0) {
    return HUGE_VAL;
  }

  if (pdata->vec) {
    if (pdata->polymin >= 0) {
      t[0] = temp;
      t[1] = t[0] * t[0];
      t[2] = t[1] * t[0];

      q = pdata->polymax;
      for (; q >= pdata->polymin; --q) {
        r = r * temp + pdata->vec[pdata->polymax - q];
      }

      for (; q > 2; q -= 3) {
        r = r * t[2];
      }
      if (q > 0) {
        r = r * t[q - 1];
      }

    } else if (pdata->polymax < 0) {
      t[0] = 1.0 / temp;
      t[1] = t[0] * t[0];
      t[2] = t[1] * t[0];

      q = pdata->polymin;
      for (; q <= pdata->polymax; ++q) {
        r = r * t[0] + pdata->vec[pdata->polymax - q];
      }

      for (; q < -2; q += 3) {
        r = r * t[2];
      }
      if (q <= 0) {
        r = r * t[-q];
      }

    } else {
      t[0] = 1.0 / temp;

      r = 0.0;
      q = pdata->polymax;
      for (; q >= 0; --q) {
        r = r * temp + pdata->vec[pdata->polymax - q];
      }

      t[1] = r;
      r = 0.0;
      q = pdata->polymin;
      for (; q < 0; ++q) {
        r = r * t[0] + pdata->vec[pdata->polymax - q];
      }

      r = t[1] + r * t[0];
    }
  }
  return r;
}

/**
 * @memberof tempdep_property
 * @brief Temperature dependent property calculation for ARRHENIUS.
 * @param data Property data
 * @param temp Temperature value to be used for.
 * @return result
 *
 * @note Do not call this function directly.
 *
 * This function returns 0 if @p temp is exact 0 (and it's in the
 * domain).
 */
static inline
double tempdep_arrhenius_calc(union tempdep_property_u *data, double temp)
{
  struct tempdep_arrhenius_data *adata;

  adata = &data->t_arrhenius;

  temp = tempdep_limit_domain(&adata->dom, temp);

  if (temp == 0.0) return 0.0;
  return adata->coeff_A * exp(- adata->coeff_E / temp);
}

/**
 * @memberof tempdep_property
 * @brief Temperature dependent property calculation for PIECEWISE
 * @param data Property data
 * @param temp Temperature value to be used for.
 * @return result
 *
 * @note Do not call this function directly.
 */
static inline
double tempdep_piecewise_calc(union tempdep_property_u *data, double temp)
{
  int idx;
  struct tempdep_property *prop;
  struct tempdep_piecewise_data *pdata;

  pdata = &data->t_piecewise;
  idx = tempdep_domain_search(temp, pdata->domain, pdata->npieces + 1);
  if (idx < 0) {
    return temp;
  }

  if (idx == 0) {
    idx = 1;
  }
  if (idx == pdata->npieces + 1) {
    idx = pdata->npieces;
  }
  prop = &pdata->func[idx - 1];
  return tempdep_calc(prop, temp);
}

/**
 * @memberof tempdep_property
 * @brief Temperature dependent property calculation for Arrhenius
 *        function for oxidation.
 * @param data Property data
 * @param temp Temperature value to be used for.
 * @return result
 *
 * This function is same to tempdep_arrhenius_calc(), except for
 * returning 0 if @p temp is not in the given domain.
 *
 * @note Do not call this function directly.
 */
static inline
double tempdep_ox_arrhenius_calc(union tempdep_property_u *data, double temp)
{
  struct tempdep_arrhenius_data *adata;

  adata = &data->t_arrhenius;

  temp = tempdep_limit_domain_specify(&adata->dom, temp, 0.0, 0.0);
  if (temp == 0.0) return 0.0;

  return tempdep_arrhenius_calc(data, temp);
}

/**
 * @memberof tempdep_property
 * @brief Oxidation reaction rete in Urbanic-Heidrick model.
 * @param data Property data (not used)
 * @param temp Temperature value to be used for.
 * @return result
 */
static inline
double tempdep_ox_urbanic_heidrick(union tempdep_property_u *data, double temp)
{
  union tempdep_property_u lodata = {
    .t_arrhenius = {
      .dom = { .minT = 1100.0, .maxT = 1850.0 },
      .coeff_A = 29.6,
      .coeff_E = 16820.0,
    },
  }, updata = {
    .t_arrhenius = {
      .dom = { .minT = 1850.0, .maxT = 2123.0 },
      .coeff_A = 87.9,
      .coeff_E = 16610.0,
    },
  };

  if (temp <= 1850.0) {
    return tempdep_ox_arrhenius_calc(&lodata, temp);
  } else {
    return tempdep_ox_arrhenius_calc(&updata, temp);
  }
}

/**
 * @memberof tempdep_property
 * @brief Oxidation reaction rete in Baker-Just model.
 * @param data Property data (not used)
 * @param temp Temperature value to be used for.
 * @return result
 */
static inline
double tempdep_ox_baker_just(union tempdep_property_u *data, double temp)
{
  union tempdep_property_u bdata = {
    .t_arrhenius = {
      .dom = { .minT = 1273.0, .maxT = 2123.0 },
      .coeff_A = 3330.0,
      .coeff_E = 22896.0,
    },
  };

  return tempdep_ox_arrhenius_calc(&bdata, temp);
}

/**
 * @memberof tempdep_property
 * @brief Oxidation reaction rete in Cathcart-Pawel model.
 * @param data Property data (not used)
 * @param temp Temperature value to be used for.
 * @return result
 */
static inline
double tempdep_ox_cathcart_pawel(union tempdep_property_u *data, double temp)
{
  union tempdep_property_u cdata = {
    .t_arrhenius = {
      .dom = { .minT = 1273.0, .maxT = 1773.0 },
      .coeff_A = 294.2,
      .coeff_E = 20100.0,
    }
  };

  return tempdep_ox_arrhenius_calc(&cdata, temp);
}

/**
 * @memberof tempdep_property
 * @brief Oxidation reaction rete in Leistikow-Schanz model.
 * @param data Property data (not used)
 * @param temp Temperature value to be used for.
 * @return result
 */
static inline
double tempdep_ox_leistikow_schanz(union tempdep_property_u *data, double temp)
{
  union tempdep_property_u ldata = {
    .t_arrhenius = {
      .dom = { .minT = 1273.0, .maxT = 1773.0 },
      .coeff_A = 425.8,
      .coeff_E = 20962.0,
    },
  };

  return tempdep_ox_arrhenius_calc(&ldata, temp);
}

/**
 * @memberof tempdep_property
 * @brief Oxidation reaction rete in Prater-Courtright model.
 * @param data Property data (not used)
 * @param temp Temperature value to be used for.
 * @return result
 */
static inline
double tempdep_ox_prater_courtright(union tempdep_property_u *data, double temp)
{
  union tempdep_property_u pdata = {
    .t_arrhenius = {
      .dom = { .minT = 1783.0, .maxT = 2673.0 },
      .coeff_A = 26763.6,
      .coeff_E = 26440.0,
    }
  };

  return tempdep_ox_arrhenius_calc(&pdata, temp);
}

/**
 * @memberof tempdep_property
 * @brief Oxidation recession rate
 * @param data Property data (not used)
 * @param temp Temperature value to be used for.
 * @return result.
 */
static inline
double tempdep_ox_recess_rate(union tempdep_property_u *data, double temp)
{
  union tempdep_property_u pdata = {
    .t_arrhenius = {
      .dom = { .minT = 0.0, .maxT = 5000.0 }, /* No limit defined */
      .coeff_A = 1.29e+5,
      .coeff_E = 320000.0 / 8.31446261815324, /* 320000.0 [unit unknown] / R */
    }
  };

  return tempdep_ox_arrhenius_calc(&pdata, temp);
}

/**
 * @memberof tempdep_property
 * @brief Temperature depeedent property calculation.
 * @param prop Property data
 * @param temp Temperature value to be used.
 * @return result
 *
 * @todo I'm not sure which is faster, using the function pointer or
 *       using switch (with inline expanding). In later version, which
 *       is currently used, the calling address is fixed, and so easy
 *       predictable, easy optimizable, but extra operetions
 *       (conditional switch) will performed every call of this
 *       function. Also in later version, it is hard to maintain
 *       because you must modify this function every time when you add
 *       the new function, and it is unable to add new function
 *       without modifying JUPITER.
 *
 * @note By investigation, GCC 7 or 8, Intel Compiler 2017 and Clang 6
 *       does not inline this function into the caller (with -O3).
 *       Because this function seems to be already huge to inline.
 */
static inline
double tempdep_calc(tempdep_property *prop, double temp)
{
  switch(prop->type) {
  case TEMPDEP_PROPERTY_INVALID:
    break;
  case TEMPDEP_PROPERTY_CONST:
    return tempdep_const_calc(&prop->data, temp);
  case TEMPDEP_PROPERTY_TABLE:
    return tempdep_table_calc(&prop->data, temp);
  case TEMPDEP_PROPERTY_POLY:
    return tempdep_poly_calc(&prop->data, temp);
  case TEMPDEP_PROPERTY_POLY_L:
    return tempdep_poly_l_calc(&prop->data, temp);
  case TEMPDEP_PROPERTY_ARRHENIUS:
    return tempdep_arrhenius_calc(&prop->data, temp);
  case TEMPDEP_PROPERTY_PIECEWISE:
    return tempdep_piecewise_calc(&prop->data, temp);

    /* Oxidation specific */
  case TEMPDEP_PROPERTY_OX_BAKER_JUST:
    return tempdep_ox_baker_just(&prop->data, temp);
  case TEMPDEP_PROPERTY_OX_CATHCART_PAWEL:
    return tempdep_ox_cathcart_pawel(&prop->data, temp);
  case TEMPDEP_PROPERTY_OX_LEISTIKOW_SCHANZ:
    return tempdep_ox_leistikow_schanz(&prop->data, temp);
  case TEMPDEP_PROPERTY_OX_PRATER_COURTRIGHT:
    return tempdep_ox_prater_courtright(&prop->data, temp);
  case TEMPDEP_PROPERTY_OX_URBANIC_HEIDRICK:
    return tempdep_ox_urbanic_heidrick(&prop->data, temp);

  case TEMPDEP_PROPERTY_OX_RECESSION:
    return tempdep_ox_recess_rate(&prop->data, temp);
  }
  return HUGE_VAL;
}

#ifdef __cplusplus
}
#endif

#endif
