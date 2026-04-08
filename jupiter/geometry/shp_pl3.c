
#include <stdlib.h>

#include "defs.h"
#include "common.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "vector.h"
#include "infomap.h"

#include "polynomial.h"
#include "shp_pl3.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for PL3 (3rd order polynomial) shape
 */
struct geom_shape_pl3_data
{
  struct geom_polynomial_data *poly;
};

#define GEOM_PLN_DEG 3

static geom_variant_type
geom_shape_pl3_args_next(geom_args_builder *b, geom_variant *description,
                         int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);
  return geom_polynomial_args_next(l, description, GEOM_PLN_DEG);
}

static geom_error
geom_shape_pl3_args_check(void *p, geom_args_builder *b,
                          geom_size_type index, const geom_variant *v,
                          geom_variant *errinfo)
{
  return geom_polynomial_args_check(v, GEOM_PLN_DEG, index, errinfo,
                                    "Factoring 0 will select whole domain");
}

static geom_error
geom_shape_pl3_get_value(void *p, geom_size_type index,
                         geom_variant *out_variable)
{
  struct geom_shape_pl3_data *pp;

  pp = (struct geom_shape_pl3_data *)p;
  if (!pp->poly)
    return GEOM_ERR_RANGE;

  return geom_polynomial_get_value(pp->poly, index, out_variable);
}

static geom_error
geom_shape_pl3_set_value(void *p, geom_size_type index,
                         const geom_variant *value)
{
  struct geom_polynomial_data *newp;
  struct geom_shape_pl3_data *pp;

  pp = (struct geom_shape_pl3_data *)p;

  if (pp->poly->refc > 1) {
    newp = geom_polynomial_duplicate(pp->poly);
    if (!newp) {
      return GEOM_ERR_NOMEM;
    }
    pp->poly = newp;
  }
  return geom_polynomial_set_value(pp->poly, index, value);
}

static geom_size_type
geom_shape_pl3_n_params(void *p, geom_args_builder *b)
{
  return geom_polynomial_get_n_params(NULL, GEOM_PLN_DEG);
}

static void *
geom_shape_pl3_allocator(void)
{
  struct geom_shape_pl3_data *p;
  p = (struct geom_shape_pl3_data *)malloc(sizeof(struct geom_shape_pl3_data));
  if (!p) return NULL;

  p->poly = geom_polynomial_resize(NULL, GEOM_PLN_DEG);
  if (!p->poly) {
    free(p);
    return NULL;
  }

  return p;
}

static void
geom_shape_pl3_deallocator(void *p)
{
  struct geom_shape_pl3_data *pp;

  if (!p) return;

  pp = (struct geom_shape_pl3_data*)p;
  if (pp->poly) {
    geom_polynomial_deallocator(pp->poly);
  }

  free(p);
}

static void *
geom_shape_pl3_copy(void *p)
{
  struct geom_shape_pl3_data *pp, *copy;

  pp = (struct geom_shape_pl3_data *)p;
  copy = (struct geom_shape_pl3_data *)
    malloc(sizeof(struct geom_shape_pl3_data));
  if (!copy) return NULL;

  copy->poly = geom_polynomial_copy(pp->poly);
  return copy;
}

static int
geom_shape_pl3_testf(void *p, double x, double y, double z)
{
  struct geom_shape_pl3_data *pp;

  pp = (struct geom_shape_pl3_data *)p;

  if (!pp->poly) return 0;

  return geom_polynomial_testf(pp->poly, x, y, z);
}

static geom_error
geom_shape_pl3_info_map(void *p, geom_info_map *list)
{
  geom_error e;
  geom_variant *v;
  struct geom_shape_pl3_data *pp;

  pp = (struct geom_shape_pl3_data *)p;
  e = GEOM_SUCCESS;

  if (!pp->poly) {
    v = geom_variant_new(&e);
    if (e != GEOM_SUCCESS) return e;

    geom_variant_set_string(v, "(memory allocation error)", 0);
    geom_info_map_append(list, v, "Error", "", &e);

    geom_variant_delete(v);
    return e;
  }

  return geom_polynomial_info_map(pp->poly, list, GEOM_PLN_DEG,
                                  NULL, NULL, NULL, NULL);
}

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_pl3 = {
  .enum_val = GEOM_SHAPE_PL3,
  .shape_type = GEOM_SHPT_BODY,

  .c = {
    .allocator = geom_shape_pl3_allocator,
    .deallocator = geom_shape_pl3_deallocator,
    .set_value = geom_shape_pl3_set_value,
    .get_value = geom_shape_pl3_get_value,
    .n_params = geom_shape_pl3_n_params,
    .args_next = geom_shape_pl3_args_next,
    .args_check = geom_shape_pl3_args_check,
    .infomap_gen = geom_shape_pl3_info_map,
    .copy = geom_shape_pl3_copy,
  },

  .body_testf = geom_shape_pl3_testf,

  /*
   * Cannot define bounding box.
   * Bouding boxes may or may not be present.
   */
  .body_bboxf = NULL,

  .transform_func = NULL,
};

geom_error geom_install_shape_pl3(void)
{
  return geom_install_shape_func(&geom_shape_pl3);
}
