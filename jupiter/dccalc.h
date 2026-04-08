
#ifndef DCCALC_H
#define DCCALC_H

/* YSE: Solute diffusion coefficient calculation */
#include <math.h>
#include <stdlib.h>
#include <string.h> /* for memset */
#include <limits.h>

#include "dccalc_defs.h"

#ifndef DCCALC_STANDALONE
#include "struct.h"
#include "tempdep_calc.h"
#else
typedef double type;
#endif

#ifdef __cplusplus
extern "C" {
#endif

static inline type
dc_calc_arrhenius(type A, type E, type T)
{
  if (T == 0.0) {
    return 0.0;
  } else {
    return A * exp(- E / T);
  }
}

static inline type
dc_calc_SUS304_B4C(type Ysus, type Yb4c, type T)
{
  if (T < 1473.0) {
    return dc_calc_arrhenius(1.422e-2, 30011.976, T);
  } else {
    return dc_calc_arrhenius(1.094e+10, 65349.669, T);
  }
}

static inline type
dc_calc_B4C_SUS304(type Yb4c, type Ysus, type T)
{
  return dc_calc_SUS304_B4C(Ysus, Yb4c, T);
}

static inline type
dc_calc_SUS304_Zircaloy(type Ysus, type Yzr, type T)
{
  return dc_calc_arrhenius(1.006e+9, 57924.924, T);
}

static inline type
dc_calc_Zircaloy_SUS304(type Yzr, type Ysus, type T)
{
  return dc_calc_arrhenius(1.174e+6, 52366.8739, T);
}

static inline type
dc_calc_B4C_Zircaloy(type Yb4c, type Yzr, type T)
{
  return dc_calc_arrhenius(5.398e-8, 21448.519, T);
}

static inline type
dc_calc_Zircaloy_B4C(type Yzr, type Yb4c, type T)
{
  return dc_calc_arrhenius(5.398e-8, 21448.519, T);
}

static inline type
dc_calc_UO2_Zircaloy(type Yuo2, type Yzr, type T)
{
  return dc_calc_arrhenius(5.398e-8, 21448.519, T);
}

static inline type
dc_calc_Zircaloy_UO2(type Yzr, type Yuo2, type T)
{
  return dc_calc_UO2_Zircaloy(Yuo2, Yzr, T);
}

/**
 * @brief Single step of reductions for material selection
 * @param n Number of elements
 * @param a Array of rates.
 * @return ID to erase.
 *
 * This function returns ID to be removed for reduction. In
 * geomtrically, `a` is nearest to the opposite side of the vertex
 * with returned ID for the simplex in `n - 1` dimension.
 *
 * For example, `n` is 4 (3-simplex is tetrahedron), and if this
 * function returned 1, the point `a` is nearest to the plane of
 * IDs 0, 2 and 3.
 */
static inline int
dc_calc_select_material(int n, type *a)
{
  type bary;
  type r;
  type rmin;
  type p;
  int i, j;
  int id;

  bary = 1.0 / n;
  for (j = 0; j < n; ++j) {
    r = 0.0;
    for (i = 0; i < n; ++i) {
      /*
       * 3-simplex example (for j = 0)
       *                           (Sum of points that ID is not j)
       *               .------------- Point of ID = 1 is full
       *               |     .------- Point of ID = 2 is full
       *               |     |     .- Point of ID = 3 is full
       *               |     |     |   Barycenter (1.0/n)
       *               v     v     v      v
       * i = 0: p0 = (0.0 + 0.0 + 0.0 + 0.25) / 4;
       * i = 1: p1 = (1.0 + 0.0 + 0.0 + 0.25) / 4;
       * i = 2: p2 = (0.0 + 1.0 + 0.0 + 0.25) / 4;
       * i = 3: p3 = (0.0 + 0.0 + 1.0 + 0.25) / 4;
       */
      if (i != j) {
        p = (1.0 + bary) / n;
      } else {
        p = bary / n;
      }
      p = a[i] - p;
      r += p * p;
    }
    if (j == 0 || rmin > r) {
      rmin = r;
      id = j;
    }
  }
  return id;
}

/**
 * @brief Recursively reduce N-ary and get the most relevant binary IDs
 * @param n Number of Element
 * @param a Array of rates
 * @param id1 Sets ID of first element.
 * @param id2 Sets ID of second element.
 *
 * This function modifies `a` and content of `a` becomes the
 * concentration of `id1` and `id2` for index 0 and 1 respectively.
 *
 * Despite of this function name 'recursive', this function is not
 * recursive function.
 */
static inline void
dc_calc_select_material_recursive(int n, type *a, int *id1, int *id2)
{
  int i;
  int j;
  int id;
  int rems[n];

  for (i = 0; i < n; ++i) {
    rems[i] = i;
  }
  for (i = n; i > 2; --i) {
    id = dc_calc_select_material(i, a);
    for (j = id; j < n - 1; ++j) {
      a[j] = a[j + 1];
      rems[j] = rems[j + 1];
    }
  }
  *id1 = rems[0];
  *id2 = rems[1];
}

/**
 * @brief calculate diffusivity on binary components of given model
 * @param param parameter data be used for computation.
 * @param Y1 Molar fraction of the "left-side component"
 * @param Y2 Molar fraction of the "right-side component"
 * @param T  Temperature [K]
 * @return solute diffusivity [m2/s]
 *
 * Arguments `Y1` and `Y2` are remained for historical reasons. Most of
 * available models do not use these values.
 */
static inline type
dc_calc(struct dc_calc_param *param, type Y1, type Y2, type T)
{
  switch(param->model) {
  case DC_FUNCS_SUS304_B4C:
    return dc_calc_SUS304_B4C(Y1, Y2, T);
  case DC_FUNCS_B4C_SUS304:
    return dc_calc_B4C_SUS304(Y1, Y2, T);
  case DC_FUNCS_SUS304_ZIRCALOY:
    return dc_calc_SUS304_Zircaloy(Y1, Y2, T);
  case DC_FUNCS_ZIRCALOY_SUS304:
    return dc_calc_Zircaloy_SUS304(Y1, Y2, T);
  case DC_FUNCS_B4C_ZIRCALOY:
    return dc_calc_B4C_Zircaloy(Y1, Y2, T);
  case DC_FUNCS_ZIRCALOY_B4C:
    return dc_calc_Zircaloy_B4C(Y1, Y2, T);
  case DC_FUNCS_UO2_ZIRCALOY:
    return dc_calc_UO2_Zircaloy(Y1, Y2, T);
  case DC_FUNCS_ZIRCALOY_UO2:
    return dc_calc_Zircaloy_UO2(Y1, Y2, T);
#ifndef DCCALC_STANDALONE
  case DC_FUNCS_TEMPDEP_PROPERTY:
    return tempdep_calc(&param->prop, T);
#endif
  default:
    return 0.0;
  }
}

static inline
void dc_calc_param_init(struct dc_calc_param *param)
{
  param->model = DC_FUNCS_INVALID;
#ifndef DCCALC_STANDALONE
  tempdep_property_init(&param->prop);
#endif
}

static inline
void dc_calc_param_clean(struct dc_calc_param *param)
{
#ifndef DCCALC_STANDALONE
  tempdep_property_clean(&param->prop);
#endif
}

/*
 * Some useful utility functions which is not directly related to the
 * comuptation of solute diffusivity.
 */

/**
 * @brief Required number of elements to store the binary diffusivities.
 * @param ncompo Number of components
 * @return Required number of elements, 0 if ncompo is 0 or negative.
 *
 * Returns $ n \times (n - 1) $ where $ n $ is `ncompo`.
 */
static inline
ptrdiff_t dc_calc_binary_size(int ncompo)
{
  ptrdiff_t t;

  if (ncompo <= 1) return 0;

  t = ncompo - 1;
  return t * ncompo;
}

/**
 * @brief Returns address offset of given pair of components
 * @param diff Material ID for diffusing
 * @param base Material ID for base
 * @param ncompo Number of components
 * @return address offset, -1 if error.
 *
 * Example of `ncompo == 5`:
 *
 *        0  1  2  3  4  (base/diff)
 *      +--+--+--+--+--+
 *      |16|17|18|19|XX| 4
 *      +--+--+--+--+--+
 *      |12|13|14|XX|15| 3
 *      +--+--+--+--+--+
 *      | 8| 9|XX|10|11| 2
 *      +--+--+--+--+--+
 *      | 4|XX| 5| 6| 7| 1
 *      +--+--+--+--+--+
 *      |XX| 0| 1| 2| 3| 0
 *      +--+--+--+--+--+
 */
static inline
ptrdiff_t dc_calc_binary_address(int diff, int base, int ncompo)
{
  ptrdiff_t t;

  if (ncompo <= 1) return -1;
  if (diff < 0 || diff >= ncompo) return -1;
  if (base < 0 || base >= ncompo) return -1;
  if (diff == base) return -1;

  t  = diff;
  t *= ncompo - 1;
  t += base;
  if (base > diff) {
    t -= 1;
  }
  return t;
}

/**
 * @brief Returns ids from address offset of parameter array.
 * @param addr Address offset to compute
 * @param ncompo Number of components
 * @param diff Material ID for diffusing (out)
 * @param base Material ID for base (out)
 * @return 1 if success, 0 if error.
 *
 * If error occured, the value of `diff` and `base` are undefined (may change).
 */
static inline
int dc_calc_binary_ids(ptrdiff_t addr, int ncompo, int *diff, int *base)
{
  ptrdiff_t t;

  if (ncompo <= 1) return 0;
  if (addr < 0) return 0;

  ncompo -= 1;
  t = addr / ncompo;
  if (t > ncompo) return 0;

  *diff = t;
  if (*diff != t) return 0; /* overflow check */

  t = addr % ncompo;
  if (t >= *diff) {
    t += 1;
  }

  /* Never overflow because max of `t` is `ncompo` which is `int`. */
  *base = t;
  return 1;
}

/**
 * @brief Required number of elements to store the binary diffusivities, in commutative version
 * @param ncompo Number of components
 * @return Required number of elements, 0 if ncompo is 0 or negative.
 *
 * Returns $ \frac{1}{2} n \times (n - 1) $ where $ n $ is `ncompo`.
 */
static inline
ptrdiff_t dc_calc_binary_size_commutative(int ncompo)
{
  ptrdiff_t t;

  if (ncompo <= 1) return 0;

  t = ncompo - 1;
  return t * ncompo / 2;
}

/**
 * @brief Returns address offset of given pair of components, commutative version
 * @param diff Material ID for diffusing
 * @param base Material ID for base
 * @param ncompo Number of components
 * @return address offset, -1 if error.
 *
 * Example of `ncompo == 5`:
 *
 *        0  1  2  3  4  (base/diff)
 *      +--+--+--+--+--+
 *      | 3| 6| 8| 9|XX| 4
 *      +--+--+--+--+--+
 *      | 2| 5| 7|XX| 9| 3      2 6 18 9
 *      +--+--+--+--+--+
 *      | 1| 4|XX| 7| 8| 2      3 7 14 7
 *      +--+--+--+--+--+
 *      | 0|XX| 4| 5| 6| 1      4 8  8 4
 *      +--+--+--+--+--+
 *      |XX| 0| 1| 2| 3| 0   4  5 4  0 0
 *      +--+--+--+--+--+
 */
static inline
ptrdiff_t dc_calc_binary_address_commutative(int diff, int base, int ncompo)
{
  ptrdiff_t t;
  ptrdiff_t tsz;

  if (diff < 0) return -1;
  if (base < 0) return -1;
  if (ncompo <= 1) return -1;
  if (diff == base) return -1;

  /* size of triangle */
  tsz  = ncompo;
  tsz -= 1;

  if (base > diff) {
    if (base >= ncompo) return -1;

    t  = tsz - diff + 1;
    t  = (t + tsz) * diff / 2;
    t += base - diff - 1;
  } else {
    if (diff >= ncompo) return -1;

    t  = tsz - base + 1;
    t  = (t + tsz) * base / 2;
    t += diff - base - 1;
  }

  return t;
}

/**
 * @brief Returns ids from address offset of parameter array, commutative version
 * @param addr Address offset to compute
 * @param ncompo Number of components
 * @param diff Material ID for diffusing (out)
 * @param base Material ID for base (out)
 * @return 1 if success, 0 if error.
 *
 * If error occured, the value of `diff` and `base` are undefined (may change).
 *
 * `diff` will be less than `base`.
 *
 * refs: http://oeis.org/A141419
 *
 *     i (right)
 *      0   0   0   0   0   0   0   0   0   0 ... j (down)
 *      1   2   3   4   5   6   7   8   9  10
 *      3   5   7   9  11  13  15  17  19  21
 *      6   9  12  15  18  21  24  27  30  33
 *     10  14  18  22  26  30  34  38  42  46
 *     15  20  25  30  35  40  45  50  55  60
 *     21  27  33  39  45  51  57  63  69  75
 *     28  35  42  49  56  63  70  77  84  91
 *     36  44  52  60  68  76  84  92 100 108
 *     45  54  63  72  81  90  99 108 117 126
 *     55  65  75  85  95 105 115 125 135 145
 *     ...
 *     r := i * j + j * (j + 1) / 2
 *
 *     We needs offset of diagonal of this part, so
 *     r := (n - j) * j + j * (j + 1) / 2, since i = n - j.
 *     r := j * (-j + 2 * n + 1) / 2
 *     r := - (1/2)* j^2 + (2 * n + 1)/2 * j
 *
 *     Therefore, we need maximum j which will fit to
 *     2 * addr > - j*j + (2*n + 1)*j and j < n.
 *     - j*j + (2*n + 1)*j - 2 * addr < 0
 */
static inline
int dc_calc_binary_ids_commutative(ptrdiff_t addr, int ncompo, int *diff, int *base)
{
  ptrdiff_t t, u;

  if (ncompo <= 1) return 0;
  if (addr < 0) return 0;

  ncompo -= 1;
  if (addr < ncompo) {
    *diff = 0;
    *base = addr + 1;
    return 1;
  }

  /* j = ((2*n+1)-sqrt((2*n+1)^2-8*addr))/2 */
  t = ncompo;
  t *= 2;
  t += 1;
  t = t * t;
  t -= addr * 8;

  /*
   * integer square root from
   * http://www.codecodex.com/wiki/Calculate_an_integer_square_root#C
   */
  {
    /* needs to be unsigned */
    size_t op, res, one;

    op = t;
    res = 0;
    one = (size_t)1 << (sizeof(size_t) * CHAR_BIT - 2);
    while (one > op) one >>= 2;
    while (one != 0) {
      if (op >= res + one) {
        op -= res + one;
        res += one << 1;
      }
      res >>= 1;
      one >>= 2;
    }

    /* ceiling function (because root part will be subtrahend) */
    if (op > 0) res++;
    t = res;

    /* Overflow check */
    if (t < 0) return 0;
  }

  u = ncompo;
  u *= 2;
  u += 1;
  t = u - t;
  t /= 2;

  *diff = t;
  if (*diff != t) return 0;

  /* r := j * (j + 2 * (n - j) + 1) / 2 */
  u = ncompo - t;
  u *= 2;
  u += 1 + t;
  t *= u;
  t /= 2;
  t = addr - t + *diff + 1;
  if (t > ncompo) return 0;

  *base = t;
  if (*base != t) return 0;

  return 1;
}

static inline int dc_calc_Ynorm(int nc, type *Ynorm, type *Y)
{
  int i;
  type Ys;

  Ys = 0.0;
  for (i = 0; i < nc; ++i) {
    Ys += Y[i];
  }
  if (Ys < 1.0e-10)
    return 0;
  for (i = 0; i < nc; ++i) {
    Ynorm[i] = Y[i] / Ys;
  }
  return 1;
}

/**
 * @brief calculate Dc
 * @param nc Number of Components
 * @param funcs list of function definitions
 * @param commutative Whether diffusion definition is commutative
 * @param Y Array of Concentrations.
 * @param T Temperature value to calculate [K]
 * @return calculated value
 *
 * Computes cell-center diffusivity used by prior solute diffusion models.
 *
 * Note: If sum of Y is less than 1e-10, this function returns 0.0.
 * Note: If there is no function to be applicable, this function return 0.0.
 *
 * Number of elements in `funcs` must be `dc_calc_binary_size(nc)` if
 * `commutative` is `0` and `dc_calc_binary_size_commutative(nc)`
 * otherwise.
 *
 * Number of elements in `Y` must be `nc`.
 */
static inline type
dc_calc_all(int nc, struct dc_calc_param *funcs, int commutative,
            type *Y, type T)
{
  type Ynorm[nc];
  int id1, id2;
  ptrdiff_t idaddr;

  if (!dc_calc_Ynorm(nc, Ynorm, Y))
    return 0.0;

  dc_calc_select_material_recursive(nc, Ynorm, &id1, &id2);
  if (commutative) {
    idaddr = dc_calc_binary_address_commutative(id1, id2, nc);
  } else {
    idaddr = dc_calc_binary_address(id1, id2, nc);
  }
  if (idaddr < 0) return 0.0;

  return dc_calc(&funcs[idaddr], Ynorm[id1], Ynorm[id2], T);
}

/**
 * Same as dc_calc_all just input array of pointers instead of array of objects.
 */
static inline type
dc_calc_p_all(int nc, struct dc_calc_param **funcs, int commutative,
              type *Y, type T)
{
  type Ynorm[nc];
  int id1, id2;
  ptrdiff_t idaddr;
  struct dc_calc_param *p;

  if (!dc_calc_Ynorm(nc, Ynorm, Y))
    return 0.0;

  dc_calc_select_material_recursive(nc, Ynorm, &id1, &id2);
  if (commutative) {
    idaddr = dc_calc_binary_address_commutative(id1, id2, nc);
  } else {
    idaddr = dc_calc_binary_address(id1, id2, nc);
  }
  if (idaddr < 0)
    return 0.0;

  p = funcs[idaddr];
  if (!p)
    return 0.0;

  return dc_calc(p, Ynorm[id1], Ynorm[id2], T);
}

#ifdef __cplusplus
}
#endif

#endif
