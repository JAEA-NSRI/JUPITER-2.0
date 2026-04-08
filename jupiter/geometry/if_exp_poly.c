
#include <stdlib.h>
#include <math.h>

#include "defs.h"
#include "geom_assert.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "svector.h"
#include "variant.h"
#include "infomap.h"

#include "polynomial.h"
#include "if_exp_poly.h"
#include "vector.h"

struct geom_init_func_exp_poly_data
{
  double factor;
  geom_vec3 basep;
  struct geom_polynomial_data *data;
};

static geom_variant_type
geom_init_func_exp_poly_args_next(geom_args_builder *b,
                                  geom_variant *description, int *optional)
{
  geom_size_type l;
  const geom_variant *cv;
  geom_size_type deg;
  geom_error e;

  l = geom_args_builder_get_loc(b);

  if (l == 0) {
    geom_variant_set_string(description, "Factor before exponent", 0);
    return GEOM_VARTYPE_DOUBLE;
  }
  if (l == 1) {
    geom_variant_set_string(description, "Base coordinate", 0);
    return GEOM_VARTYPE_VECTOR3;
  }
  if (l == 2) {
    geom_variant_set_string(description, "Degree of polynomial", 0);
    return GEOM_VARTYPE_SIZE;
  }

  cv = geom_args_builder_value_at(b, 2);
  GEOM_ASSERT(cv);

  e = GEOM_SUCCESS;
  deg = geom_variant_get_size_value(cv, &e);
  if (e != GEOM_SUCCESS || deg < 0) {
    return GEOM_VARTYPE_NULL;
  }

  return geom_polynomial_args_next(l - 3, description, deg);
}

static geom_error geom_init_func_exp_poly_args_check(void *p,
                                                     geom_args_builder *b,
                                                     geom_size_type index,
                                                     const geom_variant *v,
                                                     geom_variant *errinfo)
{
  geom_error e;
  geom_size_type deg;
  struct geom_polynomial_data *poly;
  geom_variant_list *start, *head;

  e = GEOM_SUCCESS;
  if (index == 0) {
    double d;
    d = geom_variant_get_double(v, &e);
    if (e != GEOM_SUCCESS)
      return e;
    if (!isfinite(d)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Factor must be finite", 0);
      } else {
        geom_warn("%g: Value must be finite", d);
      }
      return GEOM_ERR_RANGE;
    }
    if (d == 0.0) {
      if (errinfo) {
        geom_variant_set_string(errinfo,
                                "Factoring 0 will become 0 for whole domain",
                                0);
      } else {
        geom_warn("%g: Factoring 0 will become 0 for whole domain", d);
      }
    }
    return GEOM_SUCCESS;
  }
  if (index == 1) {
    geom_vec3 vec;
    vec = geom_variant_get_vec3(v, &e);
    if (e != GEOM_SUCCESS)
      return e;
    if (!geom_vec3_isfinite(vec)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "All elements must be finite", 0);
      } else {
        geom_warn("(%g, %g, %g): Value should be finite", geom_vec3_x(vec),
                  geom_vec3_y(vec), geom_vec3_z(vec));
      }
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  }
  if (index == 2) {
    deg = geom_variant_get_size_value(v, &e);
    if (e != GEOM_SUCCESS)
      return e;
    if (deg < 0) {
      if (errinfo) {
        geom_variant_set_string(errinfo,
                                "Degree of polynominal must be positive", 0);
      } else {
        geom_warn("%d: Degree of polynominal must be positive", deg);
      }
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  }

  poly = NULL;
  deg = -1;
  if (p) {
    struct geom_init_func_exp_poly_data *pp;
    pp = (struct geom_init_func_exp_poly_data *)p;
    poly = pp->data;
    GEOM_ASSERT(poly);
    deg = poly->degree;
  }
  if (b) {
    geom_size_type ll;
    head = geom_args_builder_get_list(b);
    start = geom_variant_list_next(head);
    for (ll = 0; start != head && ll < 3;
         ll++, start = geom_variant_list_next(start)) {
      if (ll == 2) {
        const geom_variant *degv;
        degv = geom_variant_list_get(start);
        deg = geom_variant_get_size_value(degv, &e);
      }
    }
  }
  if (e != GEOM_SUCCESS || deg < 0) {
    return GEOM_ERR_DEPENDENCY;
  }
  return geom_polynomial_args_check(
    v, deg, index - 3, errinfo,
    "Factoring 0 will be \"pre-factor\" for whole region");
}

static geom_error geom_init_func_exp_poly_set_value(void *p,
                                                    geom_size_type index,
                                                    const geom_variant *value)
{
  geom_error e;
  struct geom_init_func_exp_poly_data *pp;
  pp = (struct geom_init_func_exp_poly_data *)p;

  e = GEOM_SUCCESS;
  if (index == 0) {
    double factor;
    factor = geom_variant_get_double(value, &e);
    if (e == GEOM_SUCCESS) {
      pp->factor = factor;
    }
    return e;
  }
  if (index == 1) {
    geom_vec3 basep;
    basep = geom_variant_get_vec3(value, &e);
    if (e == GEOM_SUCCESS) {
      pp->basep = basep;
    }
    return e;
  }
  if (index == 2) {
    geom_size_type deg;
    struct geom_polynomial_data *np;
    deg = geom_variant_get_size_value(value, &e);
    if (e == GEOM_SUCCESS) {
      if (deg >= 0) {
        np = geom_polynomial_resize(pp->data, deg);
        if (!np) {
          return GEOM_ERR_NOMEM;
        }
        pp->data = np;
      }
    }
    return e;
  }

  if (pp->data->refc > 1) {
    struct geom_polynomial_data *np;
    np = geom_polynomial_duplicate(pp->data);
    if (!np) {
      return GEOM_ERR_NOMEM;
    }
    pp->data = np;
  }
  return geom_polynomial_set_value(pp->data, index - 3, value);
}

static geom_error geom_init_func_exp_poly_get_value(void *p,
                                                    geom_size_type index,
                                                    geom_variant *out_variable)
{
  struct geom_init_func_exp_poly_data *pp;

  pp = (struct geom_init_func_exp_poly_data *)p;
  switch (index) {
  case 0:
    return geom_variant_set_double(out_variable, pp->factor);
  case 1:
    return geom_variant_set_vec3(out_variable, pp->basep);
  case 2:
    return geom_variant_set_size_value(out_variable, pp->data->degree);
  default:
    return geom_polynomial_get_value(pp->data, index - 3, out_variable);
  }
}

static geom_size_type
geom_init_func_exp_poly_n_params(void *p, geom_args_builder *b)
{
  geom_size_type plv;
  plv = -1;
  if (p) {
    struct geom_init_func_exp_poly_data *pp;
    pp = (struct geom_init_func_exp_poly_data *)p;
    plv = geom_polynomial_get_n_params(pp->data, -1);
  } else if (b) {
    geom_variant_list *lp, *head;
    geom_size_type ll;
    geom_size_type deg;
    geom_error err;

    err = GEOM_SUCCESS;
    deg = -1;
    head = geom_args_builder_get_list(b);
    lp = geom_variant_list_next(head);
    for (ll = 0; lp != head; lp = geom_variant_list_next(lp), ll++) {
      if (ll == 2) {
        const geom_variant *cv;
        cv = geom_variant_list_get(lp);
        deg = geom_variant_get_size_value(cv, &err);
      }
    }
    if (err != GEOM_SUCCESS) {
      deg = -1;
    }
    plv = geom_polynomial_get_n_params(NULL, deg);
  }
  if (plv < 0) {
    return -1;
  }
  return plv + 3;
}

static void *geom_init_func_exp_poly_allocator(void)
{
  struct geom_init_func_exp_poly_data *p;
  p = (struct geom_init_func_exp_poly_data *)malloc(
    sizeof(struct geom_init_func_exp_poly_data));

  if (!p)
    return NULL; /* If could not allocate, return NULL */

  p->factor = 1.0;
  p->basep = geom_vec3_c(0.0, 0.0, 0.0);
  p->data = geom_polynomial_resize(NULL, 0);
  if (!p->data) {
    free(p);
    return NULL;
  }

  return p;
}

static void geom_init_func_exp_poly_deallocator(void *p)
{
  struct geom_init_func_exp_poly_data *pp;
  if (!p)
    return;

  pp = (struct geom_init_func_exp_poly_data *)p;
  if (pp->data) {
    geom_polynomial_deallocator(pp->data);
  }

  free(p);
}

static double geom_init_func_exp_poly_initf(void *p, double x, double y,
                                            double z, void *a)
{
  struct geom_init_func_exp_poly_data *pp;
  geom_vec3 pnt;
  double f;

  pp = (struct geom_init_func_exp_poly_data *)p;
  if (!pp->data)
    return 0.0;

  pnt = geom_vec3_sub(geom_vec3_c(x, y, z), pp->basep);
  x = geom_vec3_x(pnt);
  y = geom_vec3_y(pnt);
  z = geom_vec3_z(pnt);

  f = geom_polynomial_calc(pp->data, x, y, z);
  return pp->factor * exp(f);
}

static geom_error geom_init_func_exp_poly_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_init_func_exp_poly_data *pp;

  pp = (struct geom_init_func_exp_poly_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v)
    return e;

  geom_variant_set_double(v, pp->factor);
  geom_info_map_append(list, v, "Factor before exponential", "", &e);

  geom_variant_set_vec3(v, pp->basep);
  geom_info_map_append(list, v, "Location of the origin", "L", &e);

  if (pp->data) {
    geom_variant_set_size_value(v, pp->data->degree);
    geom_info_map_append(list, v, "Number of degree", "", &e);

    e = geom_polynomial_info_map(pp->data, list, pp->data->degree, NULL, NULL,
                                 NULL, NULL);
  } else {
    geom_variant_set_string(v, "(Memory allocation failed)", 0);
    geom_info_map_append(list, v, "Error", "", &e);
  }

  geom_variant_delete(v);

  return e;
}

static void *geom_init_func_exp_poly_copy(void *p)
{
  struct geom_init_func_exp_poly_data *pp, *np;
  pp = (struct geom_init_func_exp_poly_data *)p;
  np =
    (struct geom_init_func_exp_poly_data *)geom_init_func_exp_poly_allocator();
  if (!np)
    return NULL;

  np->factor = pp->factor;
  np->basep = pp->basep;
  np->data = geom_polynomial_copy(pp->data);
  return np;
}

static geom_init_funcs geom_init_func_exp_poly = {
  .enum_val = GEOM_INIT_FUNC_EXP_POLY,
  .c =
    {
      .allocator = geom_init_func_exp_poly_allocator,
      .deallocator = geom_init_func_exp_poly_deallocator,
      .set_value = geom_init_func_exp_poly_set_value,
      .get_value = geom_init_func_exp_poly_get_value,
      .n_params = geom_init_func_exp_poly_n_params,
      .args_next = geom_init_func_exp_poly_args_next,
      .args_check = geom_init_func_exp_poly_args_check,
      .infomap_gen = geom_init_func_exp_poly_info_map,
      .copy = geom_init_func_exp_poly_copy,
    },
  .func = geom_init_func_exp_poly_initf,
};

geom_error geom_install_init_func_exp_poly(void)
{
  return geom_install_init_func(&geom_init_func_exp_poly);
}
