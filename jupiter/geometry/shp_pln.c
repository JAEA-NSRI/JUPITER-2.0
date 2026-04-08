
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#include "defs.h"
#include "common.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "vector.h"
#include "infomap.h"

#include "polynomial.h"
#include "shp_pln.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for PLN (n-th order polynomial) shape
 */
struct geom_shape_pln_data
{
  struct geom_polynomial_data *poly;
};

static geom_variant_type
geom_shape_pln_args_next(geom_args_builder *b, geom_variant *description,
                         int *optional)
{
  geom_size_type l;
  const geom_variant *degv;
  geom_size_type deg;
  geom_error err;

  l = geom_args_builder_get_loc(b);
  if (l == 0) {
    if (description) {
      geom_variant_set_string(description, "Degree of polynomial", 0);
    }
    return GEOM_VARTYPE_SIZE;
  }

  err = GEOM_SUCCESS;
  degv = geom_args_builder_value_at(b, 0);
  if (degv) {
    deg = geom_variant_get_size_value(degv, &err);
  } else {
    err = GEOM_ERR_RANGE;
  }
  if (err != GEOM_SUCCESS) {
    return GEOM_VARTYPE_NULL;
  }

  degv = geom_args_builder_value_at(b, l - 1);
  return geom_polynomial_args_next_alt(l - 1, description, degv);
}

static geom_error
geom_shape_pln_args_check(void *p, geom_args_builder *b, geom_size_type index,
                          const geom_variant *v, geom_variant *errinfo)
{
  struct geom_shape_pln_data *pp;
  struct geom_polynomial_data *poly;
  struct geom_variant_list *start, *head;
  geom_size_type deg;
  geom_error err;

  err = GEOM_SUCCESS;
  if (index == 0) {
    deg = geom_variant_get_size_value(v, &err);
    if (err == GEOM_SUCCESS) {
      if (deg < 0) {
        if (errinfo) {
          geom_variant_set_string(errinfo, "Degree must be positive", 0);
        } else {
          geom_warn("%" PRIdMAX ": Degree must be positive", (intmax_t)deg);
        }
        return GEOM_ERR_RANGE;
      }
    }
    return err;
  }

  pp = NULL;
  poly = NULL;
  start = NULL;
  head = NULL;
  deg = -1;
  if (p) {
    pp = (struct geom_shape_pln_data *)p;
    poly = pp->poly;
  } else if (b) {
    const geom_variant *degv;
    head = geom_args_builder_get_list(b);
    start = geom_variant_list_next(head);
    degv = NULL;
    if (start != head) {
      degv = geom_variant_list_get(start);
      start = geom_variant_list_next(start);
    }

    if (!degv) {
      return GEOM_ERR_DEPENDENCY;
    }

    deg = geom_variant_get_size_value(degv, &err);
    if (err != GEOM_SUCCESS) {
      return GEOM_ERR_DEPENDENCY;
    }
  }

  return geom_polynomial_args_check_alt(poly, deg, start, head, v, index - 1,
                                        errinfo,
                                        "Factoring 0 will select whole domain");
}

static geom_error geom_shape_pln_set_value(void *p, geom_size_type index,
                                           const geom_variant *value)
{
  geom_error e;
  struct geom_shape_pln_data *pp;

  pp = (struct geom_shape_pln_data *)p;
  e = GEOM_SUCCESS;
  if (index == 0) {
    geom_size_type deg;
    struct geom_polynomial_data *newp;
    deg = geom_variant_get_size_value(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    if (deg >= 0) {
      newp = geom_polynomial_resize(pp->poly, deg);
      if (!newp) {
        return GEOM_ERR_NOMEM;
      }
      pp->poly = newp;
    }
    return GEOM_SUCCESS;
  }

  if (pp->poly->refc > 1) {
    struct geom_polynomial_data *newp;
    newp = geom_polynomial_duplicate(pp->poly);
    if (!newp) {
      return GEOM_ERR_NOMEM;
    }
    pp->poly = newp;
  }
  return geom_polynomial_set_value_alt(pp->poly, index - 1, value);
}

static geom_error geom_shape_pln_get_value(void *p, geom_size_type index,
                                           geom_variant *out_variable)
{
  struct geom_shape_pln_data *pp;

  pp = (struct geom_shape_pln_data *)p;

  if (index == 0) {
    return geom_variant_set_size_value(out_variable, pp->poly->degree);
  }
  return geom_polynomial_set_value_alt(pp->poly, index - 1, out_variable);
}

static geom_size_type geom_shape_pln_n_params(void *p, geom_args_builder *b)
{
  geom_size_type r;
  struct geom_shape_pln_data *pp;
  struct geom_polynomial_data *poly;
  geom_variant_list *start, *head;

  pp = NULL;
  poly = NULL;
  start = NULL;
  head = NULL;
  if (p) {
    pp = (struct geom_shape_pln_data *)p;
    poly = pp->poly;
  } else if (b) {

  }
  r = geom_polynomial_get_n_params_alt(poly, start, head);
  if (r >= 0) {
    return r + 1;
  }
  return r;
}

static void *
geom_shape_pln_allocator(void)
{
  struct geom_shape_pln_data *p;
  p = (struct geom_shape_pln_data *)
    malloc(sizeof(struct geom_shape_pln_data));
  if (!p) return NULL;

  p->poly = geom_polynomial_resize(NULL, 0);
  if (!p->poly) {
    free(p);
    return NULL;
  }

  return p;
}

static void
geom_shape_pln_deallocator(void *p)
{
  struct geom_shape_pln_data *pp;

  if (!p) return;

  pp = (struct geom_shape_pln_data*)p;
  if (pp->poly) {
    geom_polynomial_deallocator(pp->poly);
  }

  free(p);
}

static void *
geom_shape_pln_copy(void *p)
{
  struct geom_shape_pln_data *pp, *copy;

  pp = (struct geom_shape_pln_data *)p;
  copy = (struct geom_shape_pln_data *)
    malloc(sizeof(struct geom_shape_pln_data));
  if (!copy) return NULL;

  copy->poly = geom_polynomial_copy(pp->poly);
  return copy;
}

static int
geom_shape_pln_testf(void *p, double x, double y, double z)
{
  struct geom_shape_pln_data *pp;

  pp = (struct geom_shape_pln_data *)p;

  if (!pp->poly) return 0;

  return geom_polynomial_testf(pp->poly, x, y, z);
}

static geom_error
geom_shape_pln_info_map(void *p, geom_info_map *list)
{
  geom_error e;
  geom_variant *v;
  struct geom_shape_pln_data *pp;

  pp = (struct geom_shape_pln_data *)p;
  e = GEOM_SUCCESS;

  if (!pp->poly) {
    v = geom_variant_new(&e);
    if (e != GEOM_SUCCESS) return e;

    geom_variant_set_string(v, "(memory allocation error)", 0);
    geom_info_map_append(list, v, "Error", "", &e);

    geom_variant_delete(v);
    return e;
  }

  v = geom_variant_new(&e);
  if (e != GEOM_SUCCESS) return e;

  geom_variant_set_size_value(v, pp->poly->degree);
  geom_info_map_append(list, v, "Degree of polynomial", "", &e);

  geom_variant_delete(v);

  return geom_polynomial_info_map(pp->poly, list, pp->poly->degree,
                                  NULL, NULL, NULL, NULL);
}

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_pln = {
  .enum_val = GEOM_SHAPE_PLN,
  .shape_type = GEOM_SHPT_BODY,

  .c = {
    .allocator = geom_shape_pln_allocator,
    .deallocator = geom_shape_pln_deallocator,
    .set_value = geom_shape_pln_set_value,
    .get_value = geom_shape_pln_get_value,
    .n_params = geom_shape_pln_n_params,
    .args_next = geom_shape_pln_args_next,
    .args_check = geom_shape_pln_args_check,
    .infomap_gen = geom_shape_pln_info_map,
    .copy = geom_shape_pln_copy,
  },

  .body_testf = geom_shape_pln_testf,

  /*
   * Cannot define bounding box.
   * Bouding boxes may or may not be present.
   */
  .body_bboxf = NULL,

  .transform_func = NULL,
};

geom_error geom_install_shape_pln(void)
{
  return geom_install_shape_func(&geom_shape_pln);
}
