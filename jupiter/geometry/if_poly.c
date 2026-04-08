
#include <stdlib.h>
#include <math.h>

#include "defs.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "infomap.h"

#include "polynomial.h"
#include "if_poly.h"
#include "vector.h"

struct geom_init_func_poly_data
{
  geom_vec3 basep;
  struct geom_polynomial_data *data;
};

static geom_variant_type
geom_init_func_poly_args_next(geom_args_builder *b,
                              geom_variant *description, int *optional)
{
  geom_size_type l;
  const geom_variant *cv;
  geom_size_type deg;
  geom_error e;

  l = geom_args_builder_get_loc(b);

  if (l == 0) {
    geom_variant_set_string(description, "Base coordinate", 0);
    return GEOM_VARTYPE_VECTOR3;
  }
  if (l == 1) {
    geom_variant_set_string(description, "Degree of polynomial", 0);
    return GEOM_VARTYPE_SIZE;
  }

  cv = geom_args_builder_value_at(b, 1);
  GEOM_ASSERT(cv);

  e = GEOM_SUCCESS;
  deg = geom_variant_get_size_value(cv, &e);
  if (e != GEOM_SUCCESS || deg < 0) {
    return GEOM_VARTYPE_NULL;
  }

  return geom_polynomial_args_next(l - 2, description, deg);
}

static geom_error
geom_init_func_poly_args_check(void *p, geom_args_builder *b,
                               geom_size_type index, const geom_variant *v,
                               geom_variant *errinfo)
{
  geom_error e;
  geom_size_type deg;
  const geom_variant *degv;

  e = GEOM_SUCCESS;

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

  index -= 2;
  if (p) {
    struct geom_init_func_poly_data *pp;
    pp = (struct geom_init_func_poly_data *)p;
    deg = pp->data->degree;
  } else {
    degv = geom_args_builder_value_at(b, 1);
    deg = geom_variant_get_size_value(degv, &e);
  }
  if (e != GEOM_SUCCESS || deg < 0) {
    return GEOM_ERR_DEPENDENCY;
  }

  return geom_polynomial_args_check(v, deg, index, errinfo,
                                    "Factoring 0 will be 0 for whole region");
}

static geom_error
geom_init_func_poly_set_value(void *p, geom_size_type index,
                              const geom_variant *value)
{
  geom_error e;
  struct geom_init_func_poly_data *pp;
  struct geom_polynomial_data *newp;

  e = GEOM_SUCCESS;
  pp = (struct geom_init_func_poly_data *)p;

  if (index == 0) {
    geom_vec3 v;
    v = geom_variant_get_vec3(value, &e);
    if (e == GEOM_SUCCESS) {
      pp->basep = v;
    }
    return e;
  }
  if (index == 1) {
    geom_size_type deg;
    deg = geom_variant_get_size_value(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    if (deg >= 0) {
      newp = geom_polynomial_resize(pp->data, deg);
      if (!newp) {
        return GEOM_ERR_NOMEM;
      }
      pp->data = newp;
    }
    return GEOM_SUCCESS;
  }

  if (pp->data->refc > 1) {
    newp = geom_polynomial_duplicate(pp->data);
    if (!newp) {
      return GEOM_ERR_NOMEM;
    }
    pp->data = newp;
  }
  return geom_polynomial_set_value(pp->data, index - 2, value);
}

static geom_error
geom_init_func_poly_get_value(void *p, geom_size_type index,
                              geom_variant *out_variable)
{
  struct geom_init_func_poly_data *pp;

  pp = (struct geom_init_func_poly_data *)p;

  switch (index) {
  case 0:
    return geom_variant_set_vec3(out_variable, pp->basep);
  case 1:
    return geom_variant_set_size_value(out_variable, pp->data->degree);
  default:
    return geom_polynomial_get_value(pp->data, index - 2, out_variable);
  }
}

static geom_size_type
geom_init_func_poly_n_params(void *p, geom_args_builder *b)
{
  geom_size_type plv;

  if (p) {
    struct geom_init_func_poly_data *pp;
    pp = (struct geom_init_func_poly_data *)p;
    plv = geom_polynomial_get_n_params(pp->data, -1);
  } else if (b) {
    geom_size_type deg;
    const geom_variant *v;
    geom_error err;

    v = geom_args_builder_value_at(b, 1);
    plv = -1;
    if (v) {
      err = GEOM_SUCCESS;
      deg = geom_variant_get_size_value(v, &err);
      if (err == GEOM_SUCCESS) {
        plv = geom_polynomial_get_n_params(NULL, -1);
      }
    }
  } else {
    plv = -1;
  }
  if (plv >= 0) {
    return plv + 2;
  }
  return -1;
}

static void *
geom_init_func_poly_allocator(void)
{
  struct geom_init_func_poly_data *p;
  p = (struct geom_init_func_poly_data*)
    malloc(sizeof(struct geom_init_func_poly_data));

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
geom_init_func_poly_deallocator(void *p)
{
  struct geom_init_func_poly_data *pp;
  if (!p) return;

  pp = (struct geom_init_func_poly_data *)p;
  if (pp->data) {
    geom_polynomial_deallocator(pp->data);
  }

  free(p);
}

static double
geom_init_func_poly_initf(void *p, double x, double y, double z, void *a)
{
  struct geom_init_func_poly_data *pp;
  geom_vec3 pnt;

  pp = (struct geom_init_func_poly_data *)p;
  if (!pp->data) return 0.0;

  pnt = geom_vec3_sub(geom_vec3_c(x, y, z), pp->basep);
  x = geom_vec3_x(pnt);
  y = geom_vec3_y(pnt);
  z = geom_vec3_z(pnt);

  return geom_polynomial_calc(pp->data, x, y, z);
}

static geom_error
geom_init_func_poly_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_init_func_poly_data *pp;

  pp = (struct geom_init_func_poly_data *)p;

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
geom_init_func_poly_copy(void *p)
{
  struct geom_init_func_poly_data *pp, *np;
  pp = (struct geom_init_func_poly_data*)p;
  np = (struct geom_init_func_poly_data*)geom_init_func_poly_allocator();
  if (!np) return NULL;

  np->basep = pp->basep;
  np->data = geom_polynomial_copy(pp->data);
  return np;
}

static
geom_init_funcs geom_init_func_poly = {
  .enum_val = GEOM_INIT_FUNC_POLY,
  .c = {
    .allocator = geom_init_func_poly_allocator,
    .deallocator = geom_init_func_poly_deallocator,
    .get_value = geom_init_func_poly_get_value,
    .set_value = geom_init_func_poly_set_value,
    .n_params = geom_init_func_poly_n_params,
    .args_next = geom_init_func_poly_args_next,
    .args_check = geom_init_func_poly_args_check,
    .infomap_gen = geom_init_func_poly_info_map,
    .copy = geom_init_func_poly_copy,
  },
  .func = geom_init_func_poly_initf,
};

geom_error geom_install_init_func_poly(void)
{
  return geom_install_init_func(&geom_init_func_poly);
}
