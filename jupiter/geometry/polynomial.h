
#ifndef JUPITER_GEOMETRY_POLYNOMIAL_H
#define JUPITER_GEOMETRY_POLYNOMIAL_H

#include <stdint.h>
#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

#include "defs.h"
#include "common.h"
#include "global.h"
#include "variant.h"
#include "svector.h"
#include "geom_assert.h"
#include "infomap.h"
#include "rbtree.h"

JUPITER_GEOMETRY_DECL_START

struct geom_polynomial_coeff_data
{
  double c;                     ///< Coefficient value
  geom_size_type element_index; ///< Index of set/get for altmode, -1 is unused
  struct geom_rbtree tree;      ///< Search tree
};
#define geom_polynomial_coeff_data_entry(ptr)                                  \
  geom_rbtree_entry(ptr, struct geom_polynomial_coeff_data, tree)

static inline int geom_polynomial_coeff_data_comp(struct geom_rbtree *ta,
                                                  struct geom_rbtree *tb)
{
  struct geom_polynomial_coeff_data *ca;
  struct geom_polynomial_coeff_data *cb;

  ca = geom_polynomial_coeff_data_entry(ta);
  cb = geom_polynomial_coeff_data_entry(tb);
  return ca->element_index - cb->element_index;
}

/**
 * @ingroup Geometry
 * @brief Data for arbitrary Polyonomial function
 *
 * This data type implements two input modes.
 *
 * If geom_polynomial_args_next_alt() (inputs both degree and coeff)
 * is used, geom_polynomial_set_value_alt(),
 * geom_polynomial_get_value_alt() and
 * geom_polynomial_args_check_alt() must be used.
 *
 * If geom_polynomial_args_next() (inputs coeff only) is used,
 * geom_polynomial_set_value(), geom_polynomial_get_value() and
 * geom_polynomial_args_check() must be used.
 *
 */
struct geom_polynomial_data
{
  geom_size_type refc;                        ///< Reference count of this data
  geom_size_type degree;                      ///< Number of degrees
  geom_size_type ndata;                       ///< Number of coeffs allocated
  double factor;                              ///< Global factor
  struct geom_rbtree *root;                   ///< Tree root of index search
  struct geom_polynomial_coeff_data coeffs[]; ///< Coefficients
};

static inline struct geom_polynomial_coeff_data *
geom_polynomial_coeff_data_find(struct geom_polynomial_data *d,
                                geom_size_type i)
{
  struct geom_polynomial_coeff_data t;
  struct geom_rbtree *ret;

  GEOM_ASSERT(d);

  if (i < 0) {
    return NULL;
  }
  if (!d->root) {
    return NULL;
  }

  t.element_index = i;
  ret = geom_rbtree_find(d->root, &t.tree, geom_polynomial_coeff_data_comp);
  if (ret) {
    return geom_polynomial_coeff_data_entry(ret);
  }
  return NULL;
}

/**
 * @memberof geom_polynomial_data
 *
 * @brief Computes the number of all coefficients for specific degree
 *        (and less).
 * @param degree Degree to calculate.
 *
 * @return Number of coefficients.
 *
 * The number of coefficients is equivalent to Tetrahedral numbers.
 * (see http://oeis.org/A000292 ).
 */
static inline geom_size_type geom_polynomial_coeff_count(geom_size_type degree)
{
  if (degree < 0)
    return 0;
  return (degree + 1) * (degree + 2) * (degree + 3) / 6;
}

/**
 * @memberof geom_polynomial_data
 *
 * @brief Computes the number of coefficients for specific degree.
 * @param degree Degree to calculate.
 *
 * @return Number of coefficients.
 *
 * The number of coefficients is equivalent to Triangular numbers.
 * (see http://oeis.org/A000217 ).
 */
static inline geom_size_type
geom_polynomial_coeff_count_at(geom_size_type degree)
{
  if (degree < 0)
    return 0;
  return (degree + 1) * (degree + 2) / 2;
}

/**
 * @memberof geom_polynomial_data
 * @brief Computes the term of specific location
 * @param degree Polynomial degree number
 * @param loc    Location in 0-base.
 * @param cycle  If non-NULL given, order of x, y and z will set.
 *
 * @return vector of (degree for x, degree for y, degree for z).
 *         (-1, -1, -1) if loc is out-of-range.
 *
 * cycle value will set to 0 for xyz, 1 for yzx or 2 for zxy order is
 * prefered for printing.
 *
 * This function uses rotary order.
 *
 * For example, in 6th degree case:
 *
 *       1.  x x x x x x (Only 1 case is present for no separation case)
 *       2.  y y y y y y
 *       3.  z z z z z z
 *       4.  x x x x x|y (n - 1 cases are present for 1 separation case)
 *       5.  y y y y y|z
 *       6.  z z z z z|x
 *       7.  x x x x|y y
 *       8.  y y y y|z z
 *       9.  z z z z|x x
 *      10.  x x x|y y y
 *      11.  y y y|z z z
 *      12.  z z z|x x x
 *      13.  x x|y y y y
 *      14.  y y|z z z z
 *      15.  z z|x x x x
 *      16.  x|y y y y y
 *      17.  y|z z z z z
 *      18.  z|x x x x x
 *                    ((n - 1)_C_2 cases are present for 2 separation cases.)
 *                    (Number of parameter in n-th degree terms are
 *                     (n+1)(n+2)/2. This is easily obtainable from
 *                     the number of total terms, which is (n+1)(n+2)(n+3)/6.
 *      19.  x x x x|y|z ----+-------------+
 *      20.  y y y y|z|x --+-|-------------|+
 *      21.  z z z z|x|y --|+|------------+||
 *      22.  x x x|y y|z --|||--+-------+ |||
 *      23.  y y y|z z|x --|||+-|-------|+|||
 *      24.  z z z|x x|y --||||+|------+|||||
 *      25.  x x|y y y|z --||||||-+---+||||||
 *      26.  y y|z z z|x --||||||-|++ |||||||
 *      27.  z z|x x x|y --||||||+|||+|||||||
 *     ---.  x|y y y y|z --+|||||||||||||||||
 *     ---.  y|z z z z|x ---+||||||||||||||||
 *     ---.  z|x x x x|y ----+|||||||||||||||
 *     ---.  x x x|y|z z -----|||+|||||||||||
 *     ---.  y y y|z|x x -----|||-+||||||||||
 *     ---.  z z z|x|y y -----|||--+|||||||||
 *      28.  x x|y y|z z --+  |||   ||||||||| (3, 6, 9, ...
 *     ---.  y y|z z|x x --+  |||   |||||||||  degrees have 1 symmetry case:
 *     ---.  z z|x x|y y --+  |||   |||||||||  x^n y^n z^n)
 *     ---.  x|y y y|z z -----+||   |||||||||
 *     ---.  y|z z z|x x ------+|   |||||||||
 *     ---.  z|x x x|y y -------+   |||||||||
 *     ---.  x x|y|z z z -----------|||+|||||
 *     ---.  y y|z|x x x -----------|||-+||||
 *     ---.  z z|x|y y y -----------|||--+|||
 *     ---.  x|y y|z z z -----------+||   |||
 *     ---.  y|z z|x x x ------------+|   |||
 *     ---.  z|x x|y y y -------------+   |||
 *     ---.  x|y|z z z z -----------------+||
 *     ---.  y|z|x x x x ------------------+|
 *     ---.  z|x|y y y y -------------------+
 *
 *      29.  x x x x x
 *      30.  y y y y y
 *      31.  z z z z z
 *      32.  x x x x|y
 *      33.  y y y y|z
 *      34.  z z z z|x
 *      35.  x x x|y y
 *      36.  y y y|z z
 *      37.  z z z|x x
 *      38.  x x|y y y
 *      39.  y y|z z z
 *      40.  z z|x x x
 *      41.  x|y y y y
 *      42.  y|z z z z
 *      43.  z|x x x x
 *      44.  x x x|y|z
 *      45.  y y y|z|x
 *      46.  z z z|x|y
 *      47.  x x|y y|z
 *      48.  y y|z z|x
 *      49.  z z|x x|y
 *     ---.  x|y y y|z
 *     ---.  y|z z z|x
 *     ---.  z|x x x|y
 *     ---.  x x|y|z z
 *     ---.  y y|z|x x
 *     ---.  z z|x|y y
 *     ---.  x|y y|z z
 *     ---.  y|z z|x x
 *     ---.  z|x x|y y
 *     ---.  x|y|z z z
 *     ---.  y|z|x x x
 *     ---.  z|x|y y y
 *
 *      50.  x x x x
 *      51.  y y y y
 *      52.  z z z z
 *      53.  x x x|y
 *      54.  y y y|z
 *      55.  z z z|x
 *      56.  x x|y y
 *      57.  y y|z z
 *      58.  z z|x x
 *      59.  x|y y y
 *      60.  y|z z z
 *      61.  z|x x x
 *      62.  x x|y|z
 *      63.  y y|z|x
 *      64.  z z|x|y
 *     ---.  x|y y|z
 *     ---.  y|z z|x
 *     ---.  z|x x|y
 *     ---.  x|y|z z
 *     ---.  y|z|x x
 *     ---.  z|x|y y
 *
 *      65.  x x x
 *      66.  y y y
 *      67.  z z z
 *      68.  x x|y
 *      69.  y y|z
 *      70.  z z|x
 *      71.  x|y y
 *      72.  y|z z
 *      73.  z|x x
 *      74.  x|y|z
 *     ---.  y|z|x
 *     ---.  z|x|y
 *
 *      75.  x x
 *      76.  y y
 *      77.  z z
 *      78.  x|y
 *      79.  y|z
 *      80.  z|x
 *
 *      81.  x
 *      82.  y
 *      83.  z
 *
 *      84.  Intercept
 */
static inline geom_svec3 geom_polynomial_coeff_degree_at(geom_size_type degree,
                                                         geom_size_type loc,
                                                         geom_size_type *cycle)
{
  geom_size_type l, rem;
  geom_size_type lx, ly, lz;

  while (loc > 0) {
    l = geom_polynomial_coeff_count_at(degree);
    if (loc < l)
      break;

    degree -= 1;
    loc = loc - l;
  }

  if (degree < 0 || loc < 0) {
    if (cycle)
      *cycle = 0;
    return geom_svec3_c(-1, -1, -1);
  }
  if (degree == 0) {
    if (cycle)
      *cycle = 0;
    return geom_svec3_c(0, 0, 0);
  }

  l = loc / 3;
  rem = loc % 3;
  if (cycle)
    *cycle = rem;

  if (l == 0) {
    switch (rem) {
    case 0:
      return geom_svec3_c(degree, 0, 0);
    case 1:
      return geom_svec3_c(0, degree, 0);
    case 2:
      return geom_svec3_c(0, 0, degree);
    }
    GEOM_UNREACHABLE();
    return geom_svec3_c(-1, -1, -1);
  }

  if (l < degree) {
    switch (rem) {
    case 0:
      return geom_svec3_c(degree - l, l, 0);
    case 1:
      return geom_svec3_c(0, degree - l, l);
    case 2:
      return geom_svec3_c(l, 0, degree - l);
    }
    GEOM_UNREACHABLE();
    return geom_svec3_c(-1, -1, -1);
  }

  l -= degree;
  /* REAL loop end is not easily obtainable */
  for (lz = 1; lz < degree; ++lz) {
    // ly = lz;
    // lx = degree - (ly + lz); // degree - 2 * lz;
    // lx = lx - ly;
    // if (lx <= 0) lx = 1;
    lx = degree - 3 * lz;
    if (lx <= 0)
      lx = 1;
    if (l >= lx) {
      l = l - lx;
      continue;
    }
    break;
  }
  ly = l + lz;
  lx = degree - (ly + lz);

  switch (rem) {
  case 0:
    return geom_svec3_c(lx, ly, lz);
  case 1:
    return geom_svec3_c(lz, lx, ly);
  case 2:
    return geom_svec3_c(ly, lz, lx);
  }
  GEOM_UNREACHABLE();
  return geom_svec3_c(-1, -1, -1);
}

static inline geom_size_type geom_polynomial_coeff_loc(geom_size_type degree,
                                                       geom_svec3 power)
{
  geom_size_type spdeg;
  geom_size_type loc, i, lx, ly, lz, shift;
  geom_svec3 s[3];

  lx = geom_svec3_x(power);
  ly = geom_svec3_y(power);
  lz = geom_svec3_z(power);
  spdeg = lx + ly + lz;

  if (lx < 0 || ly < 0 || lz < 0)
    return -1;
  if (spdeg < 0 || spdeg > degree)
    return -1;

  s[0] = power;
  s[1] = geom_svec3_rotate_yzx(s[0]);
  s[2] = geom_svec3_rotate_yzx(s[1]);

  if (geom_svec3_eql(s[0], s[1])) {
    /* lx == ly == lz */
    shift = 0;

  } else {
    /* select vector with minimum z. */
    lx = geom_svec3_z(s[0]);
    ly = geom_svec3_z(s[1]);
    lz = geom_svec3_z(s[2]);

    /*
     * If given vector is (a, b, a) (b > a), the rotations are
     *
     *   - 0: (a, b, a) lx: a
     *   - 1: (b, a, a) ly: a -- Use this.
     *   - 2: (a, a, b) lz: b
     *
     * If given vector is (b, a, a) (b > a), the rotations are
     *
     *   - 0: (b, a, a) lx: a -- Use this.
     *   - 1: (a, a, b) ly: b
     *   - 2: (a, b, a) lz: a
     *
     * If given vector is (a, a, b) (b > a), the rotations are
     *
     *   - 0: (a, a, b) lx: b
     *   - 1: (a, b, a) ly: a
     *   - 2: (b, a, a) lz: a -- Use this.
     */

    i = lx;
    shift = 0;
    if (ly <= i) {
      i = ly;
      shift = 1;
    }
    if ((shift > 0 && lz == i) || lz < i) {
      i = lz;
      shift = 2;
    }
  }
  power = s[shift];

  lx = geom_svec3_x(power);
  ly = geom_svec3_y(power);
  lz = geom_svec3_z(power);
  loc = 0;
  for (i = 0; i < lz; ++i) {
    loc += spdeg - 3 * i; /* see geom_polynomial_coeff_degree_at(). */
  }
  loc += ly - lz;
  loc *= 3;
  loc += shift;

  while (spdeg < degree) {
    loc += geom_polynomial_coeff_count_at(spdeg + 1);
    spdeg++;
  }

  return loc;
}

/**
 * @memberof geom_polynomial_data
 * @brief Create text represention of coefficient at specific loc
 *
 * @param buf    Location to set
 * @param degree Degree of polynomial
 * @param loc    Coefficient location.
 *
 * @return Number of bytes written or -1 if allocation failed.
 *
 * For example, geom_polynomial_coeff_name(&buf, 5, 0) will be
 * "Coefficient of x^5", geom_polynomial_coeff_name(&buf, 5, 16) will
 * be "Coefficient of xy^3 z", and geom_polynomial_coeff_name(&buf, 5,
 * 55) will be "Intercept".
 */
static inline int geom_polynomial_coeff_name(char **buf, geom_size_type degree,
                                             geom_size_type loc)
{
  const char *ctmp[3];
  const char *sym[3] = {"x", "y", "z"};
  char *tmp[3];
  int r;
  int i;
  geom_svec3 s;
  geom_size_type x[3];
  geom_size_type cyc;
  geom_size_type ai;
  geom_size_type val;
  const char *name;

  s = geom_polynomial_coeff_degree_at(degree, loc, &cyc);
  if (geom_svec3_eql(s, geom_svec3_c(0, 0, 0))) {
    return geom_asprintf(buf, "Intercept");
  }

  x[0] = geom_svec3_x(s);
  x[1] = geom_svec3_y(s);
  x[2] = geom_svec3_z(s);
  for (i = 0; i < 3; ++i) {
    tmp[i] = NULL;
  }
  for (i = 0; i < 3; ++i) {
    ai = (i + cyc) % 3;
    val = x[ai];
    name = sym[ai];
    if (val > 1) {
      r = geom_asprintf(&tmp[i], "%s^%" PRIdMAX " ", name, (intmax_t)val);
      if (r < 0) {
        tmp[i] = NULL;
        goto clean;
      }
      ctmp[i] = tmp[i];
    } else {
      if (val == 1) {
        ctmp[i] = name;
      } else {
        ctmp[i] = "";
      }
    }
  }
  r = geom_asprintf(buf, "Coefficient of %s%s%s", ctmp[0], ctmp[1], ctmp[2]);

clean:
  for (i = 0; i < 3; ++i) {
    free(tmp[i]);
  }
  return r;
}

static inline geom_error
geom_polynomial_args_check_coeffs(geom_size_type degree,
                                  geom_size_type coeff_index,
                                  const geom_variant *v, geom_variant *errinfo)
{
  geom_error e;
  double val;

  e = GEOM_SUCCESS;
  val = geom_variant_get_double(v, &e);
  if (e != GEOM_SUCCESS) {
    return e;
  }

  if (!isfinite(val)) {
    char *tmp1, *tmp2;
    const char *ctmp;
    int r;
    tmp2 = NULL;
    r = geom_polynomial_coeff_name(&tmp1, degree, coeff_index);
    if (r >= 0) {
      r = geom_asprintf(&tmp2, "Coefficient for %s must be finite", tmp1);
      free(tmp1);
      if (r >= 0) {
        ctmp = tmp2;
      } else {
        tmp2 = NULL;
      }
    }
    if (!tmp2) {
      ctmp = "Coefficients must be finite";
    }
    if (errinfo) {
      geom_variant_set_string(errinfo, ctmp, 0);
    } else {
      geom_warn("%g: %s", val, ctmp);
    }
    free(tmp2);
    return GEOM_ERR_RANGE;
  }
  return e;
}

static inline geom_error
geom_polynomial_args_check_factor(const geom_variant *v, geom_variant *errinfo,
                                  const char *zero_factor_msg)
{
  geom_error e;
  double val;

  e = GEOM_SUCCESS;
  val = geom_variant_get_double(v, &e);
  if (e != GEOM_SUCCESS) {
    return e;
  }

  if (!isfinite(val)) {
    if (errinfo) {
      geom_variant_set_string(errinfo, "factor must be finite", 0);
    } else {
      geom_warn("%g: Global factor must be finitie");
    }
    return GEOM_ERR_RANGE;
  } else if (val == 0.0 && zero_factor_msg) {
    if (errinfo) {
      geom_variant_set_string(errinfo, zero_factor_msg, 0);
    } else {
      geom_warn("%s", zero_factor_msg);
    }
  }

  return e;
}

static inline geom_error geom_polynomial_args_check_power(
  geom_size_type index, geom_svec3 pwr, struct geom_polynomial_data *p,
  geom_size_type degree, geom_variant_list *args, geom_variant_list *head,
  geom_variant *errinfo, int emit_warn)
{
  geom_size_type already_given;
  geom_size_type coeff_loc;

  if (p) {
    args = NULL;
    head = NULL;
    degree = p->degree;
  } else {
    GEOM_ASSERT(args);
    GEOM_ASSERT(head);
  }
  if (degree < 0) {
    return GEOM_ERR_DEPENDENCY;
  }

  coeff_loc = geom_polynomial_coeff_loc(degree, pwr);
  if (coeff_loc < 0) {
    const char *ctxt;
    if (geom_svec3_x(pwr) < 0 || geom_svec3_y(pwr) < 0 || geom_svec3_z(pwr)) {
      ctxt = "Negative power specified";
    } else {
      ctxt = "The sum of each elements is greater than given degree";
    }
    if (errinfo) {
      geom_variant_set_string(errinfo, ctxt, 0);
    } else if (emit_warn) {
      char *buf;
      int r;
      r = geom_svec3_to_str(&buf, pwr, -1, -1);
      if (r >= 0) {
        geom_warn("%s: %s", buf, ctxt);
        free(buf);
      } else {
        geom_warn("(vector format error): %s", buf, ctxt);
      }
    }
    return GEOM_ERR_RANGE;
  }

  already_given = -1;
  if (p) {
    struct geom_polynomial_coeff_data *cp;
    cp = &p->coeffs[coeff_loc];
    if (cp->element_index >= 0 && cp->element_index != index) {
      already_given = cp->element_index;
    }
  } else {
    geom_size_type lp;
    geom_variant_list *curs;

    GEOM_ASSERT(head);
    GEOM_ASSERT(args);

    for (curs = args, lp = 0; curs != head;
         lp++, curs = geom_variant_list_next(curs)) {
      geom_svec3 opwr;
      geom_error xerr;
      geom_size_type ip;

      if (lp % 2 == 1)
        continue;
      ip = lp / 2;
      if (ip == index)
        continue;

      xerr = GEOM_SUCCESS;
      opwr = geom_variant_get_svec3(geom_variant_list_get(curs), &xerr);
      if (xerr == GEOM_SUCCESS) {
        if (geom_svec3_eql(pwr, opwr)) {
          already_given = ip;
        }
      }
    }
  }
  if (already_given >= 0) {
    if (errinfo || emit_warn) {
      char *buf;
      const char *ctxt;
      int r;
      ctxt = "Given power is already specified at another index";
      buf = NULL;
      r = geom_asprintf(&buf,
                        "Given power is already specified at index %" PRIdMAX,
                        (intmax_t)already_given);
      if (r >= 0) {
        ctxt = buf;
      } else {
        buf = NULL;
      }
      if (errinfo) {
        geom_variant_set_string(errinfo, ctxt, 0);
      } else {
        char *sbuf;
        GEOM_ASSERT(emit_warn);
        r = geom_svec3_to_str(&sbuf, pwr, -1, -1);
        if (r >= 0) {
          geom_warn("%s: %s", sbuf, ctxt);
        } else {
          geom_warn("(error formatting vector): %s", sbuf, ctxt);
        }
      }
      free(buf);
    }
    return GEOM_ERR_RANGE;
  }
  return GEOM_SUCCESS;
}

static inline geom_error
geom_polynomial_args_check_end_marker(int ival, geom_variant *errinfo,
                                      int emit_warn)
{
  if (ival >= 0) {
    if (errinfo) {
      geom_variant_set_string(errinfo, "End marker value must be negative", 0);
    } else if (emit_warn) {
      geom_warn("%d: End marker value must be negative", ival);
    }
    return GEOM_ERR_RANGE;
  }
  return GEOM_SUCCESS;
}

/**
 * @brief Checks power argument on "alt" mode
 * @param v variant to check
 * @param index index of pair list to set
 * @param p Polynomial data (if already initialized, pass `NULL` otherwise)
 * @param args List of variants (if initializing, pass `NULL` otherwise)
 * @param head List end (for passing @p args)
 * @param degree Degree value (for passing @p args)
 * @param errinfo Sets error information if given.
 * @param emit_warn whether emits warning when errinfo is `NULL`.
 * @retval GEOM_SUCCESS No error
 * @retval GEOM_ERR_RANGE Invalid value range
 * @retval GEOM_ERR_DEPENDENCY Invalid degree
 * @retval GEOM_ERR_VARIANT_TYPE Invalid variant type
 */
static inline geom_error geom_polynomial_args_check_power_and_end(
  const geom_variant *v, geom_size_type index, struct geom_polynomial_data *p,
  geom_variant_list *args, geom_variant_list *head, geom_size_type degree,
  geom_variant *errinfo, int emit_warn)
{
  int ival;
  geom_error e;
  geom_variant_type t;
  geom_svec3 pwr;

  pwr = geom_svec3_c(-1, -1, -1);
  ival = 0;
  e = GEOM_SUCCESS;
  t = geom_variant_get_type(v);
  if (t == GEOM_VARTYPE_SIZE_VECTOR3) {
    pwr = geom_variant_get_svec3(v, &e);
  } else {
    ival = geom_variant_get_int(v, &e);
  }
  if (e != GEOM_SUCCESS)
    return e;

  if (t == GEOM_VARTYPE_SIZE_VECTOR3) {
    return geom_polynomial_args_check_power(index, pwr, p, degree, args, head,
                                            errinfo, emit_warn);
  } else if (t == GEOM_VARTYPE_INT) {
    return geom_polynomial_args_check_end_marker(ival, errinfo, emit_warn);
  }
  return GEOM_ERR_VARIANT_TYPE;
}

static inline geom_error geom_polynomial_args_check_alt_coeffs(
  const geom_variant *v, geom_size_type index, struct geom_polynomial_data *p,
  geom_variant_list *args, geom_variant_list *head, geom_size_type degree,
  geom_variant *errinfo)
{
  geom_svec3 pwr;
  geom_error err;
  geom_size_type coeff_loc;

  err = GEOM_SUCCESS;
  coeff_loc = -1;
  if (p) {
    struct geom_polynomial_coeff_data *cd;
    degree = p->degree;
    cd = geom_polynomial_coeff_data_find(p, index);
    if (cd) {
      coeff_loc = cd - p->coeffs;
      pwr = geom_polynomial_coeff_degree_at(degree, coeff_loc, NULL);
    } else {
      err = GEOM_ERR_DEPENDENCY;
    }
  } else {
    geom_size_type lcur;
    geom_variant_list *curs;

    GEOM_ASSERT(args);
    GEOM_ASSERT(head);

    curs = args;
    for (lcur = 0; curs != head; ++lcur, curs = geom_variant_list_next(curs)) {
      if (lcur == index * 2)
        break;
    }
    if (lcur == index * 2) {
      pwr = geom_variant_get_svec3(geom_variant_list_get(curs), &err);
    } else {
      err = GEOM_ERR_DEPENDENCY;
    }
  }
  if (err == GEOM_SUCCESS) {
    err = geom_polynomial_args_check_power(index, pwr, p, degree, args, head,
                                           NULL, 0);
  }
  if (err != GEOM_SUCCESS) {
    const char *ctxt;
    ctxt = "Because of the internal structure, powers must be set properly "
      "before setting coefficients.";
    if (errinfo) {
      geom_variant_set_string(errinfo, ctxt, 0);
    } else {
      geom_warn("%s", ctxt);
    }
    return GEOM_ERR_DEPENDENCY;
  }

  if (coeff_loc < 0) {
    coeff_loc = geom_polynomial_coeff_loc(degree, pwr);
  }
  return geom_polynomial_args_check_coeffs(degree, coeff_loc, v, errinfo);
}

/**
 * @memberof geom_polynomial_data
 * @brief Template function for "Argument check"
 *
 * @param v       Variant to check
 * @param degree  Degree number to be used
 * @param loc     Location of the argument
 * @param errinfo Error information if errors set.
 * @param factor_zero_msg Message for factoring 0.
 *
 * If factor_zero_msg is NULL, warning will be disabled.
 *
 * @retval GEOM_SUCCESS no error
 * @retval GEOM_ERR_VARIANT_TYPE Type mismatch.
 * @retval GEOM_ERR_RANGE        loc or value of v out-of-range.
 */
static inline geom_error geom_polynomial_args_check(const geom_variant *v,
                                                    geom_size_type degree,
                                                    geom_size_type loc,
                                                    geom_variant *errinfo,
                                                    const char *factor_zero_msg)
{
  geom_size_type sz;

  sz = geom_polynomial_coeff_count(degree);
  if (loc < 0 || loc > sz)
    return GEOM_ERR_RANGE;

  if (loc == sz) {
    return geom_polynomial_args_check_factor(v, errinfo, factor_zero_msg);
  }
  return geom_polynomial_args_check_coeffs(degree, loc, v, errinfo);
}

/**
 * @memberof geom_polynomial_data
 * @brief Alternative Template function for "Argument check"
 *
 * @param p       Pointer to polynomial structure (can be `NULL`)
 * @param degree  Degree value for @p p is NULL
 * @param args    Start position of polynomial data in argument list (for @p p is NULL)
 * @param head    Head of argument list (for @p p is NULL)
 * @param v       Variant to test
 * @param loc     Location of the argument
 * @param errinfo Error information if errors set.
 * @param factor_zero_msg Message for factoring 0.
 *
 * If factor_zero_msg is NULL, warning will be disabled.
 *
 * @retval GEOM_SUCCESS no error
 * @retval GEOM_ERR_VARIANT_TYPE Type mismatch
 * @retval GEOM_ERR_RANGE v out-of-range.
 */
static inline geom_error geom_polynomial_args_check_alt(
  struct geom_polynomial_data *p, geom_size_type degree,
  struct geom_variant_list *args, struct geom_variant_list *head,
  const geom_variant *v, geom_size_type loc,
  geom_variant *errinfo, const char *factor_zero_msg)
{
  geom_size_type index;

  if (loc < 0) {
    return GEOM_ERR_RANGE;
  }
  if (loc == 0) {
    return geom_polynomial_args_check_factor(v, errinfo, factor_zero_msg);
  }
  loc -= 1;
  index = loc / 2;

  if (args != head) {
    args = geom_variant_list_next(args);
  }
  if (loc % 2 == 0) {
    return geom_polynomial_args_check_power_and_end(v, index, p, args, head,
                                                    degree, errinfo, 1);
  }
  return geom_polynomial_args_check_alt_coeffs(v, index, p, args, head,
                                               degree, errinfo);
}

/**
 * @memberof geom_polynomial_data
 * @brief Template function for "Set Data"
 * @param p Polynomial data to set to.
 * @param index Index to set
 * @param value Value to set
 *
 * This function modifies itself
 */
static inline geom_error
geom_polynomial_set_value(struct geom_polynomial_data *p, geom_size_type index,
                          const geom_variant *value)
{
  geom_size_type sz;
  geom_error e;
  sz = geom_polynomial_coeff_count(p->degree);

  if (index < 0 || index > sz) {
    return GEOM_ERR_RANGE;
  }

  e = GEOM_SUCCESS;
  if (index == sz) {
    p->factor = geom_variant_get_double(value, &e);
  } else {
    p->coeffs[index].c = geom_variant_get_double(value, &e);
  }
  return e;
}

/**
 * @memberof geom_polynomial_data
 * @brief Template function for "Get Data"
 * @param p Polynomial data to get from.
 * @param index Index to get
 * @param out_value destination
 */
static inline geom_error
geom_polynomial_get_value(struct geom_polynomial_data *p, geom_size_type index,
                          geom_variant *out_value)
{
  geom_size_type sz;
  sz = geom_polynomial_coeff_count(p->degree);

  if (index < 0 || index > sz) {
    return GEOM_ERR_RANGE;
  }

  if (index == sz) {
    return geom_variant_set_double(out_value, p->factor);
  } else {
    return geom_variant_set_double(out_value, p->coeffs[index].c);
  }
}

static inline geom_size_type
geom_polynomial_get_max_index_alt(struct geom_polynomial_data *p)
{
  geom_size_type max_index;
  struct geom_rbtree *maxt;
  struct geom_polynomial_coeff_data *maxc;

  if (!p->root) {
    return 0;
  }

  maxt = geom_rbtree_maximum(p->root);
  maxc = geom_polynomial_coeff_data_entry(maxt);
  max_index = maxc->element_index;
  GEOM_ASSERT(max_index >= 0);

  max_index *= 2;
  if (max_index / 2 != maxc->element_index) {
    return -1;
  }
  if (max_index + 1 <= max_index) {
    return -1;
  }
  max_index += 1; /* for global factor */
  return max_index;
}

/**
 * @memberof geom_polynomial_data
 * @brief Alternative Template function for "Set Value"
 *
 * @param p      Polynomial data to set to.
 * @param index  Index to set
 * @param value  Value to set
 *
 * @return Last occured error.
 *
 * To set factor, use index 0.
 * To remove an entry, index which specify the powers to integer -1.
 *  - This automatically sets coefficients to 0.
 * To add an entry, svec3 variant at max_index.
 *
 * This function modifies itself.
 */
static inline geom_error
geom_polynomial_set_value_alt(struct geom_polynomial_data *p,
                              geom_size_type index, const geom_variant *value)
{
  geom_size_type ncoeff;
  geom_svec3 sv;
  geom_error e;
  geom_size_type max_index;
  geom_size_type type;
  struct geom_polynomial_coeff_data *dest;

  GEOM_ASSERT(p);
  GEOM_ASSERT(value);

  ncoeff = geom_polynomial_coeff_count(p->degree);
  max_index = geom_polynomial_get_max_index_alt(p);

  e = GEOM_SUCCESS;

  if (max_index < 0) {
    return GEOM_ERR_RANGE;
  }
  if (index < 0) {
    return GEOM_ERR_RANGE;
  }
  if (index == 0) {
    p->factor = geom_variant_get_double(value, &e);
    return e;
  }

  index--;
  type = index % 2;
  index /= 2;

  if (index > max_index) {
    return GEOM_ERR_RANGE;
  }

  dest = geom_polynomial_coeff_data_find(p, index);
  if (type == 0) {
    struct geom_polynomial_coeff_data *newloc;

    if (geom_variant_get_type(value) == GEOM_VARTYPE_INT) {
      int ival;
      ival = geom_variant_get_int(value, &e);
      if (e != GEOM_SUCCESS) {
        return e;
      }
      if (ival >= 0) {
        return GEOM_ERR_RANGE;
      }
      newloc = NULL;

    } else {
      geom_size_type lp;

      sv = geom_variant_get_svec3(value, &e);
      if (e != GEOM_SUCCESS) {
        return e;
      }

      lp = geom_polynomial_coeff_loc(p->degree, sv);
      if (lp < 0 || lp >= ncoeff) {
        return GEOM_ERR_RANGE;
      }

      newloc = &p->coeffs[lp];
    }
    if (!dest && !newloc) {
      return GEOM_ERR_RANGE;
    }
    if (dest != newloc) {
      double cval;
      cval = 0.0;
      if (newloc && newloc->element_index >= 0) {
        return GEOM_ERR_RANGE;
      }
      if (dest) {
        struct geom_rbtree *iter;
        iter = geom_rbtree_succ_next(&dest->tree);
        p->root = geom_rbtree_delete(p->root, &dest->tree, NULL);
        dest->element_index = -1;
        cval = dest->c;
        dest->c = 0.0;
        geom_rbtree_init(&dest->tree);
        if (!newloc) {
          for (; iter; iter = geom_rbtree_succ_next(iter)) {
            struct geom_polynomial_coeff_data *cp;
            cp = geom_polynomial_coeff_data_entry(iter);
            GEOM_ASSERT(cp->element_index > 0);
            cp->element_index--;
          }
        }
      }
      if (newloc) {
        struct geom_rbtree *root;
        newloc->element_index = index;
        newloc->c = cval;
        root = geom_rbtree_insert(p->root, &newloc->tree,
                                  geom_polynomial_coeff_data_comp, NULL);
        if (!root) {
          return GEOM_ERR_RANGE;
        }
        p->root = root;
      }
    }

  } else {
    if (!dest) {
      return GEOM_ERR_RANGE;
    }

    dest->c = geom_variant_get_double(value, &e);
  }
  return e;
}

static inline geom_error
geom_polynomial_get_value_alt(struct geom_polynomial_data *p,
                              geom_size_type index, geom_variant *out_value)
{
  struct geom_polynomial_coeff_data *cp;

  if (index < 0) {
    return GEOM_ERR_RANGE;
  }
  if (index == 0) {
    return geom_variant_set_double(out_value, p->factor);
  }
  index--;
  cp = geom_polynomial_coeff_data_find(p, index / 2);
  if (cp) {
    if (index % 2 == 0) {
      geom_svec3 degv;
      degv = geom_polynomial_coeff_degree_at(p->degree, cp - p->coeffs, NULL);
      return geom_variant_set_svec3(out_value, degv);
    }
    return geom_variant_set_double(out_value, cp->c);
  }
  return geom_variant_set_int(out_value, -1);
}

/**
 * @memberof geom_polynomial_data
 * @brief Unlink to data.
 * @param p Data to unlink
 *
 * @retval p    If no other data uses \p p, you can reuse it.
 * @retval NULL If some other data still uses \p p.
 */
static inline struct geom_polynomial_data *
geom_polynomial_delink(struct geom_polynomial_data *p)
{
  if (p->refc > 1) {
    p->refc--;
    return NULL;
  }
  p->refc = 1;
  return p;
}

/**
 * @member geom_polynomial_data
 * @brief Simple allocator of geom_polynomial.
 *
 * @param ndegree Number of degrees to use.
 *
 * @return Allocated pointer, or NULL if allocation failed.
 *
 * Coefficients are not initialized.
 *
 * Use geom_polynomial_resize() instead. It does full initialization.
 */
static inline struct geom_polynomial_data *
geom_polynomial_allocator(geom_size_type ndegree)
{
  struct geom_polynomial_data *p;
  geom_size_type ndata;
  geom_size_type nbytes;

  ndata = geom_polynomial_coeff_count(ndegree);
  nbytes = ndata * sizeof(struct geom_polynomial_coeff_data);
  if (nbytes / ndata != sizeof(struct geom_polynomial_coeff_data)) {
    return NULL;
  }
  if (nbytes + sizeof(struct geom_polynomial_data) < nbytes) {
    return NULL;
  }
  nbytes += sizeof(struct geom_polynomial_data);

  p = (struct geom_polynomial_data *)malloc(nbytes);
  if (!p) {
    return NULL;
  }

  p->refc = 1;
  p->root = NULL;
  p->degree = ndegree;
  p->ndata = ndata;
  p->factor = 0.0;
  return p;
}

/**
 * @member geom_polynomial_data
 * @brief Make deep copy
 * @param oldp Data to copy
 * @return copied data, or NULL if allocation failed.
 *
 * The tree will not be same as original.
 */
static inline struct geom_polynomial_data *
geom_polynomial_duplicate(struct geom_polynomial_data *oldp)
{
  geom_size_type it;
  struct geom_polynomial_data *p;
  p = geom_polynomial_allocator(oldp->degree);
  if (!p) {
    return NULL;
  }

  p->factor = oldp->factor;
  p->root = NULL;
  for (it = 0; it < p->ndata; ++it) {
    p->coeffs[it] = oldp->coeffs[it];
    geom_rbtree_init(&p->coeffs[it].tree);
    if (p->coeffs[it].element_index >= 0) {
      if (p->root) {
        struct geom_rbtree *nroot;
        nroot = geom_rbtree_insert(p->root, &p->coeffs[it].tree,
                                   geom_polynomial_coeff_data_comp, NULL);
        GEOM_ASSERT(nroot);
        if (!nroot) {
          free(p);
          return NULL;
        }
        p->root = nroot;
      } else {
        p->root = &p->coeffs[it].tree;
      }
    }
  }

  return p;
}

static inline void
geom_polynomial_coeff_data_init(struct geom_polynomial_coeff_data *data)
{
  GEOM_ASSERT(data);
  data->c = 0.0;
  data->element_index = -1;
  geom_rbtree_init(&data->tree);
}

/**
 * @member geom_polynomial_data
 * @brief Shrink polynomial data
 * @param p Polynomial data to shrink
 * @param ndegree New maximum degree
 * @param from_p Copy from
 *
 * This function never fails.
 *
 * This function does nothing if @p ndegree is greater than degree of
 * @p from_p. Note that data would not be copied in this case.
 *
 * This function is inplace safe, so @p p can be equal to @p from_p.
 */
static inline void
geom_polynomial_shrink(struct geom_polynomial_data *p,
                       geom_size_type ndegree,
                       struct geom_polynomial_data *from_p)
{
  geom_size_type it;

  GEOM_ASSERT(p);
  GEOM_ASSERT(from_p);
  GEOM_ASSERT(ndegree >= 0);

  if (from_p->degree <= ndegree) {
    return;
  }

  if (p != from_p) {
    p->root = NULL;
    p->factor = from_p->factor;
  }
  for (it = 0; it < p->ndata; ++it) {
    geom_svec3 degv;
    geom_size_type from;
    degv = geom_polynomial_coeff_degree_at(ndegree, it, NULL);
    from = geom_polynomial_coeff_loc(from_p->degree, degv);
    GEOM_ASSERT(from >= it);
    if (from != it || p != from_p) {
      p->coeffs[it] = from_p->coeffs[from];
      if (p == from_p) {
        if (p->root && p->coeffs[from].element_index >= 0) {
          p->root = geom_rbtree_delete(p->root, &p->coeffs[from].tree, NULL);
        }
        geom_polynomial_coeff_data_init(&p->coeffs[from]);
      }

      geom_rbtree_init(&p->coeffs[it].tree);
      if (p->coeffs[it].element_index >= 0) {
        if (!p->root) {
          p->root = &p->coeffs[it].tree;
        } else {
          struct geom_rbtree *nroot;
          nroot = geom_rbtree_insert(p->root, &p->coeffs[it].tree,
                                     geom_polynomial_coeff_data_comp, NULL);
          GEOM_ASSERT(nroot);
          p->root = nroot;
        }
      }
    }
  }
  p->degree = ndegree;
}

/**
 * @memberof geom_polynomial_data
 * @brief Extend degree (without reallocation)
 * @param p Polynomial data of extend
 * @param ndegree New degree
 * @param from_p Copy from
 * @retval 0 No error (including @p p's degree is greater than @p ndegree)
 * @retval 1 Error (Trying to expand to larger than @p p accepts)
 *
 * This function is inplace safe, so @p p can be equal to @p from_p.
 */
static inline int
geom_polynomial_extend(struct geom_polynomial_data *p, geom_size_type ndegree,
                       struct geom_polynomial_data *from_p)
{
  geom_size_type it;
  geom_size_type ndata;
  GEOM_ASSERT(p);
  GEOM_ASSERT(from_p);
  GEOM_ASSERT(ndegree >= 0);

  if (from_p->degree >= ndegree) {
    return 0;
  }

  ndata = geom_polynomial_coeff_count(ndegree);
  if (p->ndata < ndata) {
    return 1;
  }

  if (p != from_p) {
    p->root = NULL;
    p->factor = from_p->factor;
  }
  for (it = ndata - 1; it >= 0; --it) {
    geom_svec3 degv;
    geom_size_type from;
    degv = geom_polynomial_coeff_degree_at(ndegree, it, NULL);
    from = geom_polynomial_coeff_loc(from_p->degree, degv);
    GEOM_ASSERT(from <= it);
    if (from < 0) {
      geom_polynomial_coeff_data_init(&p->coeffs[it]);
    } else if (it != from || p != from_p) {
      p->coeffs[it] = from_p->coeffs[from];
      if (p == from_p) {
        if (p->coeffs[from].element_index >= 0) {
          p->root = geom_rbtree_delete(p->root, &p->coeffs[from].tree, NULL);
        }
        geom_polynomial_coeff_data_init(&p->coeffs[from]);
      }

      geom_rbtree_init(&p->coeffs[it].tree);
      if (p->coeffs[it].element_index >= 0) {
        if (!p->root) {
          p->root = &p->coeffs[it].tree;
        } else {
          struct geom_rbtree *nroot;
          nroot = geom_rbtree_insert(p->root, &p->coeffs[it].tree,
                                     geom_polynomial_coeff_data_comp, NULL);
          GEOM_ASSERT(nroot);
          p->root = nroot;
        }
      }
    }
  }
  p->degree = ndegree;
  return 0;
}

/**
 * @member geom_polynomial_data
 * @brief Template function of "Allocator"
 *
 * @param oldp    Old data.
 * @param ndegree Number of degrees to use.
 *
 * @return Allocated pointer, or NULL if allocation failed.
 *
 * When allocation failed, @p oldp is not changed.
 *
 * @p oldp allowed to be NULL. If so, this function creates a new
 * one. This is why this function is suitable for "Allocator"
 * function.
 */
static inline struct geom_polynomial_data *
geom_polynomial_resize(struct geom_polynomial_data *oldp,
                       geom_size_type ndegree)
{
  struct geom_polynomial_data *p, *newp;
  geom_size_type ndata;
  geom_size_type nbytes;
  geom_size_type it;

  ndata = geom_polynomial_coeff_count(ndegree);
  if (ndata < 0) {
    return NULL;
  }
  nbytes = sizeof(struct geom_polynomial_coeff_data) * ndata;
  if (ndata > 0 &&
      nbytes / ndata != sizeof(struct geom_polynomial_coeff_data)) {
    return NULL;
  }
  if (nbytes + sizeof(struct geom_polynomial_data) < nbytes) {
    return NULL;
  }
  nbytes += sizeof(struct geom_polynomial_data);

  p = NULL;
  if (oldp) {
    p = geom_polynomial_delink(oldp);
    if (p) { /* reusable (not referenced by others) */
      if (p->degree > ndegree) {
        geom_polynomial_shrink(p, ndegree, p);
        return p;
      }
      if (p->degree < ndegree) {
        int r;

        if (geom_polynomial_extend(p, ndegree, p) == 0) {
          return p;
        }

        newp = (struct geom_polynomial_data *)realloc(p, nbytes);
        if (!newp) {
          return NULL;
        }

        newp->ndata = ndata;
        r = geom_polynomial_extend(newp, ndegree, newp);
        GEOM_ASSERT(r == 0);
        return newp;
      }
      return p; /* p->degree == ndegree */
    }

    if (oldp->degree == ndegree) {
      return geom_polynomial_duplicate(oldp);
    }

    newp = geom_polynomial_allocator(ndegree);
    if (!newp) {
      /* recall ownership */
      oldp->refc++;
      return NULL;
    }

    if (oldp->degree > ndegree) {
      geom_polynomial_shrink(newp, ndegree, oldp);
    }
    if (oldp->degree < ndegree) {
      geom_polynomial_extend(newp, ndegree, oldp);
    }
    return newp;
  }

  newp = geom_polynomial_allocator(ndegree);
  if (!newp) {
    return NULL;
  }

  GEOM_ASSERT(newp->ndata == ndata);
  for (it = 0; it < newp->ndata; ++it) {
    geom_polynomial_coeff_data_init(&newp->coeffs[it]);
  }

  return newp;
}

/**
 * @member geom_polynomial_data
 * @brief Template function of "Deallocator"
 *
 * @param p Pointer to deallocate.
 *
 * If \p p is refered by another data, unlink \p p.
 *
 * If \p p is the last using data, deallocates \p p.
 */
static inline void geom_polynomial_deallocator(struct geom_polynomial_data *p)
{
  if (p) {
    p = geom_polynomial_delink(p);
  }
  free(p);
}

/**
 * @memberof geom_polynomial_data
 * @brief Make shallow copy of data
 * @param p Data to copy.
 * @return always return \p p (include for NULL)
 */
static inline struct geom_polynomial_data *
geom_polynomial_copy(struct geom_polynomial_data *p)
{
  if (p) {
    p->refc++;
    return p;
  }
  return NULL;
}

/**
 * @memberof geom_polynomial_data
 * @brief Template function of "Next argument"
 * @param l           Position to get next for.
 * @param description Parameter description will set here.
 * @param degree      Polynomial degrees to use for.
 *
 * @retval ::GEOM_VARTYPE_NULL   \p l out of range.
 * @retval ::GEOM_VARTYPE_DOUBLE Success (all parameters are double).
 *
 * You can adjust \p l if extra argument present, before calling this
 * function.
 *
 * Number of parameters required for \p degree can be obtained by
 * geom_polynomial_coeff_count(), and global factor is present at
 * last.
 */
static inline geom_variant_type
geom_polynomial_args_next(geom_size_type l, geom_variant *description,
                          geom_size_type degree)
{
  int r;
  geom_size_type sz;
  char *buf;
  const char *cf;

  sz = geom_polynomial_coeff_count(degree);

  if (l < 0 || l > sz) {
    return GEOM_VARTYPE_NULL;
  }

  if (l == sz) {
    geom_variant_set_string(description, "Global factor", 0);
  } else {
    r = geom_polynomial_coeff_name(&buf, degree, l);
    if (r < 0) {
      cf = "Coefficient (could not show details)";
      buf = NULL;
    } else {
      cf = buf;
    }
    geom_variant_set_string(description, cf, 0);
    free(buf);
  }

  return GEOM_VARTYPE_DOUBLE;
}

/**
 * @memberof geom_polynomial_data
 * @brief Alternative Template function of "Next argument"
 * @param l           Position to get next for.
 * @param description Parameter description will set here.
 * @param last_val    Last data item.
 *
 * @retval ::GEOM_VARTYPE_NULL @p l out of range, end marker (negative
 * int) is given for @p last_val, or errors occured.
 * @retval ::GEOM_VARTYPE_INT_OR_SVEC3 Give degree or end
 * @retval ::GEOM_VARTYPE_DOUBLE       Give coefficient, or factor for l == 0
 *
 * You can adjust \p l if extra argument present, before calling this
 * function.
 */
static inline geom_variant_type
geom_polynomial_args_next_alt(geom_size_type l, geom_variant *description,
                              const geom_variant *last_val)
{
  if (l < 0) {
    return GEOM_VARTYPE_NULL;
  }
  if (l == 0) {
    geom_variant_set_string(description, "Global factor", 0);
    return GEOM_VARTYPE_DOUBLE;
  }
  l -= 1;

  if (!last_val)
    return GEOM_VARTYPE_NULL;

  if (l % 2 == 0) {
    return GEOM_VARTYPE_INT_OR_SVEC3;
  } else {
    int lval;
    geom_error err;
    geom_variant_type lval_type;
    err = GEOM_SUCCESS;
    lval = 0;
    lval_type = geom_variant_get_type(last_val);
    if (lval_type == GEOM_VARTYPE_INT) {
      lval = geom_variant_get_int(last_val, &err);
      if (err == GEOM_SUCCESS && lval < 0) {
        return GEOM_VARTYPE_NULL;
      }
    } else if (lval_type != GEOM_VARTYPE_SIZE_VECTOR3) {
      err = GEOM_ERR_VARIANT_TYPE;
    }
    if (err != GEOM_SUCCESS) {
      return GEOM_VARTYPE_NULL;
    }
    return GEOM_VARTYPE_DOUBLE;
  }

  GEOM_UNREACHABLE();
  return GEOM_VARTYPE_NULL;
}

/**
 * @brief Get N parameters for polynomials
 * @param p      Polynomial data (can be NULL)
 * @param degree degree value if @p p is NULL.
 * @return number of parameters
 */
static inline geom_size_type
geom_polynomial_get_n_params(struct geom_polynomial_data *p,
                             geom_size_type degree)
{
  if (p) {
    degree = p->degree;
  }
  if (degree < 0) {
    return -1;
  }
  return geom_polynomial_coeff_count(degree) + 1; /* for global factor */
}

/**
 * @brief Get N parameters for polynomials in 'alt' mode
 * @param p  Polynomial data (can be NULL)
 * @param start Start of the argument list if @p p is NULL
 * @param head  Head of the argument list if @p p is NULL
 * @return number of parameters currently has
 * @retval -1 List is not completed.
 */
static inline geom_size_type
geom_polynomial_get_n_params_alt(struct geom_polynomial_data *p,
                                 geom_variant_list *start,
                                 geom_variant_list *head)
{
  geom_size_type np;

  np = 1; /* for global factor */
  if (p) {
    if (p->root) {
      struct geom_rbtree *max;
      struct geom_polynomial_coeff_data *cp;
      max = geom_rbtree_maximum(p->root);
      cp = geom_polynomial_coeff_data_entry(max);
      np += cp->element_index * 2;
    }
    return np;
  }
  if (start && head) {
    geom_size_type nc;
    int completed;
    completed = 0;
    if (start != head) {
      start = geom_variant_list_next(start);
    }
    for (nc = 0; start != head; start = geom_variant_list_next(start), nc++) {
      const geom_variant *el;
      geom_variant_type t;
      el = geom_variant_list_get(start);
      t = geom_variant_get_type(el);
      if (nc % 2 == 0) {
        if (t == GEOM_VARTYPE_INT) {
          completed = 1;
          break;
        }
      }
    }
    if (completed) {
      return np + nc / 2 + 1; /* +1 is for end marker */
    }
    return -1;
  }
  return -1;
}

/**
 * @memberof geom_polynomial_data
 * @brief Polyominal calculation
 *
 * @param p Polynomial function data
 * @param x X-coordinate to test
 * @param y Y-coordinate to test
 * @param z Z-coordinate to test
 *
 * @retval result value
 *
 * In polynomial calculation of x (or y or z), this function makes
 * groups of 3rd degrees. For example, \f$ x^8 \f$ is calculated as
 * (x*x*x)*(x*x*x)*(x*x). Note that the result is different to
 * multiplicating one-by-one: (((((((x*x)*x)*x)*x)*x)*x)*x).
 */
static inline double geom_polynomial_calc(struct geom_polynomial_data *p,
                                          double x, double y, double z)
{
  geom_size_type sz, i, j;
  geom_svec3 s;
  double sum;
  geom_size_type se[3];
  double t[3];
  double coe[3][3];

  if (p->factor == 0.0)
    return 0.0;

  for (i = 0; i < 3; ++i) {
    coe[i][1] = coe[i][2] = 0.0;
  }
  coe[0][0] = x;
  coe[1][0] = y;
  coe[2][0] = z;

  if (p->degree > 1) {
    for (i = 0; i < 3; ++i) {
      coe[i][1] = coe[i][0] * coe[i][0];
    }
    if (p->degree > 2) {
      for (i = 0; i < 3; ++i) {
        coe[i][2] = coe[i][1] * coe[i][0];
      }
    }
  }

  sum = 0.0;
  sz = geom_polynomial_coeff_count(p->degree);
  for (i = 0; i < sz; ++i) {
    s = geom_polynomial_coeff_degree_at(p->degree, i, NULL);
    se[0] = geom_svec3_x(s);
    se[1] = geom_svec3_y(s);
    se[2] = geom_svec3_z(s);
    for (j = 0; j < 3; ++j) {
      t[j] = 1.0;
      while (se[j] > 0) {
        if (t[j] == 0.0)
          break;
        if (se[j] >= 3) {
          t[j] *= coe[j][2];
          se[j] -= 3;
          continue;
        }
        if (se[j] == 2) {
          t[j] *= coe[j][1];
          break;
        }
        if (se[j] == 1) {
          t[j] *= coe[j][0];
          break;
        }
      }
    }
    sum += t[0] * t[1] * t[2] * p->coeffs[i].c;
  }
  return sum * p->factor;
}

/**
 * @memberof geom_polynomial_data
 * @brief Template function for body test
 *
 * @param p Polynomial function data
 * @param x X-coordinate to test
 * @param y Y-coordinate to test
 * @param z Z-coordinate to test
 *
 * @retval 0     If outside of the shape
 * @retval non-0 If inside of the shape
 *
 * If global factor is equal to zero, this function will always return
 * 1. This is different to calculating literally if some parameters
 * overflowed into INFINITY or -INFINITY, which may returns 0 in
 * IEEE-754 compiliant systems because 0 * INFINITY gets NAN and NAN
 * >= 0 evaluates to false.
 */
static inline int geom_polynomial_testf(struct geom_polynomial_data *p,
                                        double x, double y, double z)
{
  double ret;

  /*
   * Inclusive comparison will be done.
   * This means factoring 0 becomes 0 >= 0, which is always true.
   */
  if (p->factor == 0.0)
    return 1;

  ret = geom_polynomial_calc(p, x, y, z);
  if (ret <= 0.0)
    return 1;
  return 0;
}

/**
 * @memberof geom_polynomial_data
 * @brief Template function of "Information map"
 *
 * @param p Polynomial Data to map
 * @param list Infomap list to set
 * @param degree Number of degrees of polynomial
 * @param prefix  Common prefix text
 * @param postfix Common postfix text
 * @param fb_coeff  Fallback text for "Coefficients"
 * @param fb_factor Fallback text for "Global factor"
 *
 * If degree is positive (or 0), we rely and use it (for improving
 * compiler optimization).
 *
 * If degree is negative, `p->degree` will be used.
 *
 * Prefix, postfix, and fallback texts can be NULL. If so, "" will be
 * used for prefix and postfix, predefined text used for fallbacks.
 */
static inline geom_error
geom_polynomial_info_map(struct geom_polynomial_data *p, geom_info_map *list,
                         geom_size_type degree, const char *prefix,
                         const char *postfix, const char *fb_coeff,
                         const char *fb_factor)
{
  geom_variant *v;
  geom_error e;
  geom_size_type sz, i;
  int format_prefix;

  if (!fb_coeff)
    fb_coeff = "Polynomial Coefficient";
  if (!fb_factor)
    fb_factor = "Polynomial Global factor";

  format_prefix = 0;
  if (prefix)
    format_prefix += strlen(prefix);
  if (postfix)
    format_prefix += strlen(postfix);
  if (!prefix)
    prefix = "";
  if (!postfix)
    postfix = "";

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v)
    return e;

  if (degree < 0) {
    sz = geom_polynomial_coeff_count(p->degree);
    degree = p->degree;
  } else {
    sz = geom_polynomial_coeff_count(degree);
  }

  if (p->root) {
    struct geom_rbtree *lp;
    char *buf;
    const char *ctxt;
    int r;

    ctxt = "Global Factor";
    buf = NULL;
    if (format_prefix) {
      r = geom_asprintf(&buf, "%s%s%s", prefix, buf, postfix);
      if (r >= 0) {
        ctxt = buf;
      } else {
        buf = NULL;
      }
    }
    geom_variant_set_double(v, p->factor);
    geom_info_map_append(list, v, ctxt, "", &e);
    free(buf);

    geom_rbtree_foreach_succ (lp, p->root) {
      struct geom_polynomial_coeff_data *cp;
      geom_size_type lc;
      geom_svec3 degv;

      cp = geom_polynomial_coeff_data_entry(lp);
      r = geom_asprintf(&buf, "Degree at index %" PRIdMAX,
                        (intmax_t)cp->element_index);
      if (r < 0) {
        ctxt = "Degree in order of assignment";
        buf = NULL;
      } else {
        ctxt = buf;
      }

      lc = cp - p->coeffs;
      degv = geom_polynomial_coeff_degree_at(p->degree, lc, NULL);
      geom_variant_set_svec3(v, degv);
      geom_info_map_append(list, v, ctxt, "", &e);
      free(buf);

      r = geom_polynomial_coeff_name(&buf, p->degree, lc);
      if (r >= 0) {
        char *buf2;
        r = geom_asprintf(&buf2, "%s%s%s", prefix, buf, postfix);
        if (r >= 0) {
          free(buf);
          buf = buf2;
        }
      } else {
        buf = NULL;
      }
      if (buf) {
        ctxt = buf;
      } else {
        ctxt = "Coefficient";
      }
      geom_variant_set_double(v, cp->c);
      geom_info_map_append(list, v, ctxt, "", &e);
      free(buf);
    }
  } else {
    for (i = 0; i < (sz + 1); ++i) {
      int r;
      char *buf, *b2;
      const char *ctxt, *cb2;

      r = 1;
      buf = NULL;
      if (i < sz) {
        geom_variant_set_double(v, p->coeffs[i].c);
        r = geom_polynomial_coeff_name(&b2, degree, i);
        if (r > 0) {
          cb2 = b2;
        } else {
          b2 = NULL;
          cb2 = fb_coeff;
          ctxt = fb_coeff;
        }
      } else {
        geom_variant_set_double(v, p->factor);
        b2 = NULL;
        cb2 = "Global factor";
        ctxt = fb_factor;
      }
      if (r >= 0) {
        if (format_prefix) {
          r = geom_asprintf(&buf, "%s%s%s", prefix, cb2, postfix);
          free(b2);
          if (r < 0) {
            buf = NULL;
          } else {
            ctxt = buf;
          }
        } else {
          ctxt = cb2;
          if (b2) {
            buf = b2;
          }
        }
      }

      geom_info_map_append(list, v, ctxt, "", &e);
      free(buf);
    }
  }

  geom_variant_delete(v);
  return e;
}

JUPITER_GEOMETRY_DECL_END

#endif
