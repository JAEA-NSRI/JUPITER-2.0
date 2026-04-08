
#include <stdlib.h>
#include <math.h>

#include "defs.h"
#include "geom_assert.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "infomap.h"

#include "polynomial.h"
#include "if_poly_n.h"
#include "vector.h"

struct geom_init_func_poly_n_data
{
  geom_vec3 basep;
  struct geom_polynomial_data *data;
};

static geom_variant_type
geom_init_func_poly_n_args_next(geom_args_builder *b,
                                geom_variant *description, int *optional)
{
  geom_size_type l;
  const geom_variant *cv;
  geom_error err;
  geom_size_type deg;

  l = geom_args_builder_get_loc(b);

  if (l == 0) {
    geom_variant_set_string(description, "Base coordinate", 0);
    return GEOM_VARTYPE_VECTOR3;
  }
  if (l == 1) {
    geom_variant_set_string(description, "Degree of polynomial", 0);
    return GEOM_VARTYPE_SIZE;
  }
  if (l < 0) {
    return GEOM_VARTYPE_NULL;
  }

  err = GEOM_SUCCESS;
  cv = geom_args_builder_value_at(b, 1);
  if (cv) {
    deg = geom_variant_get_size_value(cv, &err);
  } else {
    err = GEOM_ERR_RANGE;
  }
  if (err != GEOM_SUCCESS) {
    return GEOM_VARTYPE_NULL;
  }

  cv = geom_args_builder_value_at(b, l - 1);
  return geom_polynomial_args_next_alt(l - 2, description, cv);
}

static geom_error
geom_init_func_poly_n_args_check(void *p, geom_args_builder *b,
                                 geom_size_type index, const geom_variant *v,
                                 geom_variant *errinfo)
{
  geom_error e;
  geom_size_type deg;
  struct geom_polynomial_data *pl;
  struct geom_variant_list *start, *head;

  if (index < 0) {
    return GEOM_ERR_RANGE;
  }
  if (index == 0) {
    geom_vec3 vec;
    vec = geom_variant_get_vec3(v, &e);
    if (e != GEOM_SUCCESS) return e;
    if (!geom_vec3_isfinite(vec)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "All elements must be finite", 0);
      } else {
        geom_warn("(%g, %g, %g): Value should be finite",
                  geom_vec3_x(vec), geom_vec3_y(vec), geom_vec3_z(vec));
      }
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  }
  if (index == 1) {
    deg = geom_variant_get_size_value(v, &e);
    if (e != GEOM_SUCCESS) return e;
    if (deg < 0) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Degree of polynominal must be positive", 0);
      } else {
        geom_warn("%d: Degree of polynominal must be positive", deg);
      }
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  }

  pl = NULL;
  start = NULL;
  head = NULL;
  if (p) {
    struct geom_init_func_poly_n_data *pp;
    pp = (struct geom_init_func_poly_n_data *)p;
    pl = pp->data;
    deg = pl->degree;
  } else {
    const geom_variant *degv;
    GEOM_ASSERT(b);

    degv = NULL;
    head = geom_args_builder_get_list(b);
    start = geom_variant_list_next(head);
    if (start != head) { /* skip vector */
      start = geom_variant_list_next(start);
    }
    if (start != head) {
      degv = geom_variant_list_get(start);
      start = geom_variant_list_next(start);
    }

    if (!degv) {
      return GEOM_ERR_DEPENDENCY;
    }

    deg = geom_variant_get_size_value(degv, &e);
    if (e != GEOM_SUCCESS) {
      return GEOM_ERR_DEPENDENCY;
    }
  }

  return geom_polynomial_args_check_alt(pl, deg, start, head, v,
                                        index - 2, errinfo,
                                        "Factoring 0 will be 0 for whole region");
}

static geom_error
geom_init_func_poly_n_set_value(void *p, geom_size_type index,
                                const geom_variant *value)
{
  struct geom_init_func_poly_n_data *pp;
  geom_error err;

  pp = (struct geom_init_func_poly_n_data *)p;

  err = GEOM_SUCCESS;
  if (index < 0) {
    return GEOM_ERR_RANGE;
  }
  if (index == 0) {
    geom_vec3 basep;
    basep = geom_variant_get_vec3(value, &err);
    if (err == GEOM_SUCCESS) {
      pp->basep = basep;
    }
    return err;
  }
  if (index == 1) {
    struct geom_polynomial_data *np;
    geom_size_type deg;
    deg = geom_variant_get_size_value(value, &err);
    if (err == GEOM_SUCCESS) {
      np = geom_polynomial_resize(pp->data, deg);
      if (!np) {
        return GEOM_ERR_NOMEM;
      }
      pp->data = np;
    }
    return err;
  }

  if (pp->data->refc > 1) {
    struct geom_polynomial_data *np;
    np = geom_polynomial_duplicate(pp->data);
    if (!np) {
      return GEOM_ERR_NOMEM;
    }
    pp->data = np;
  }
  return geom_polynomial_set_value_alt(pp->data, index - 2, value);
}

static geom_error
geom_init_func_poly_n_get_value(void *p, geom_size_type index,
                                geom_variant *out_variable)
{
  struct geom_init_func_poly_n_data *pp;
  pp = (struct geom_init_func_poly_n_data *)p;

  if (index < 0) {
    return GEOM_ERR_RANGE;
  }
  switch (index) {
  case 0:
    return geom_variant_set_vec3(out_variable, pp->basep);
  case 1:
    return geom_variant_set_size_value(out_variable, pp->data->degree);
  default:
    return geom_polynomial_get_value_alt(pp->data, index - 2, out_variable);
  }
  GEOM_UNREACHABLE();
}

static geom_size_type
geom_init_func_poly_n_n_params(void *p, geom_args_builder *b)
{
  geom_size_type rst;
  struct geom_polynomial_data *pd;
  geom_variant_list *start, *head;

  pd = NULL;
  start = NULL;
  head = NULL;
  if (p) {
    struct geom_init_func_poly_n_data *pp;
    pp = (struct geom_init_func_poly_n_data *)p;
    pd = pp->data;
  } else {
    int i;
    head = geom_args_builder_get_list(b);
    start = geom_variant_list_next(head);
    for (i = 0; start != head && i < 2; ++i) {
      start = geom_variant_list_next(start);
    }
  }
  rst = geom_polynomial_get_n_params_alt(pd, start, head);
  if (rst >= 0) {
    return rst + 2;
  }
  return rst;
}

static void *
geom_init_func_poly_n_allocator(void)
{
  struct geom_init_func_poly_n_data *p;
  p = (struct geom_init_func_poly_n_data*)
    malloc(sizeof(struct geom_init_func_poly_n_data));

  if (!p) return NULL; /* If could not allocate, return NULL */

  p->basep = geom_vec3_c(0.0, 0.0, 0.0);
  p->data = geom_polynomial_resize(NULL, 0);
  if (!p->data) {
    free(p);
    return NULL;
  }

  return p;
}

static void
geom_init_func_poly_n_deallocator(void *p)
{
  struct geom_init_func_poly_n_data *pp;
  if (!p) return;

  pp = (struct geom_init_func_poly_n_data *)p;
  if (pp->data) {
    geom_polynomial_deallocator(pp->data);
  }

  free(p);
}

static double
geom_init_func_poly_n_initf(void *p, double x, double y, double z, void *a)
{
  struct geom_init_func_poly_n_data *pp;
  geom_vec3 pnt;

  pp = (struct geom_init_func_poly_n_data *)p;
  if (!pp->data) return 0.0;

  pnt = geom_vec3_sub(geom_vec3_c(x, y, z), pp->basep);
  x = geom_vec3_x(pnt);
  y = geom_vec3_y(pnt);
  z = geom_vec3_z(pnt);

  return geom_polynomial_calc(pp->data, x, y, z);
}

static geom_error
geom_init_func_poly_n_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_init_func_poly_n_data *pp;

  pp = (struct geom_init_func_poly_n_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  geom_variant_set_vec3(v, pp->basep);
  geom_info_map_append(list, v, "Location of the origin", "L", &e);

  if (pp->data) {
    geom_variant_set_size_value(v, pp->data->degree);
    geom_info_map_append(list, v, "Number of degree", "", &e);

    e = geom_polynomial_info_map(pp->data, list, pp->data->degree,
                                 NULL, NULL, NULL, NULL);
  } else {
    geom_variant_set_string(v, "(Memory allocation failed)", 0);
    geom_info_map_append(list, v, "Error", "", &e);
  }

  geom_variant_delete(v);

  return e;
}

static void *
geom_init_func_poly_n_copy(void *p)
{
  struct geom_init_func_poly_n_data *pp, *np;
  pp = (struct geom_init_func_poly_n_data*)p;
  np = (struct geom_init_func_poly_n_data*)geom_init_func_poly_n_allocator();
  if (!np) return NULL;

  np->basep = pp->basep;
  np->data = geom_polynomial_copy(pp->data);
  return np;
}

static
geom_init_funcs geom_init_func_poly_n = {
  .enum_val = GEOM_INIT_FUNC_POLY_N,
  .c = {
    .allocator = geom_init_func_poly_n_allocator,
    .deallocator = geom_init_func_poly_n_deallocator,
    .set_value = geom_init_func_poly_n_set_value,
    .get_value = geom_init_func_poly_n_get_value,
    .n_params = geom_init_func_poly_n_n_params,
    .args_next = geom_init_func_poly_n_args_next,
    .args_check = geom_init_func_poly_n_args_check,
    .infomap_gen = geom_init_func_poly_n_info_map,
    .copy = geom_init_func_poly_n_copy,
  },
  .func = geom_init_func_poly_n_initf,
};

geom_error geom_install_init_func_poly_n(void)
{
  return geom_install_init_func(&geom_init_func_poly_n);
}
