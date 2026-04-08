
#include <stdlib.h>

#include "defs.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "vector.h"
#include "mat43.h"
#include "infomap.h"

#include "shp_tra.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for TRA transformation (Translate)
 */
struct geom_shape_tra_data
{
  geom_vec3 amount;   ///< Translate amount
};

static geom_variant_type
geom_shape_tra_args_next(geom_args_builder *b, geom_variant *description,
                         int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);
  if (l != 0) {
    return GEOM_VARTYPE_NULL;
  }

  geom_variant_set_string(description, "Translate amount", 0);
  return GEOM_VARTYPE_VECTOR3;
}

static geom_error
geom_shape_tra_args_check(void *p, geom_args_builder *b, geom_size_type index,
                          const geom_variant *v, geom_variant *errinfo)
{
  geom_error e;
  geom_vec3 vec;

  if (index != 0) {
    return GEOM_ERR_RANGE;
  }

  e = GEOM_SUCCESS;
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

static geom_error geom_shape_tra_set_value(void *p, geom_size_type index,
                                           const geom_variant *value)
{
  geom_vec3 vec;
  geom_error e;
  struct geom_shape_tra_data *pp;

  pp = (struct geom_shape_tra_data *)p;

  if (index != 0) {
    return GEOM_ERR_RANGE;
  }

  e = GEOM_SUCCESS;
  vec = geom_variant_get_vec3(value, &e);
  if (e != GEOM_SUCCESS) {
    return e;
  }

  pp->amount = vec;
  return GEOM_SUCCESS;
}

static geom_error geom_shape_tra_get_value(void *p, geom_size_type index,
                                           geom_variant *out_variable)
{
  struct geom_shape_tra_data *pp;

  pp = (struct geom_shape_tra_data *)p;

  if (index != 0) {
    return GEOM_ERR_RANGE;
  }

  return geom_variant_set_vec3(out_variable, pp->amount);
}

static geom_size_type geom_shape_tra_n_params(void *p, geom_args_builder *b)
{
  return 1;
}

static void *
geom_shape_tra_allocator(void)
{
  struct geom_shape_tra_data *p;
  p = (struct geom_shape_tra_data *)
    malloc(sizeof(struct geom_shape_tra_data));
  if (!p) return NULL;

  p->amount = geom_vec3_c(0.0, 0.0, 0.0);

  return p;
}

static void
geom_shape_tra_deallocator(void *p)
{
  free(p);
}

static geom_error
geom_shape_tra_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_shape_tra_data *pp;

  pp = (struct geom_shape_tra_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  geom_variant_set_vec3(v, pp->amount);
  geom_info_map_append(list, v, "Translate amount", "L", &e);

  geom_variant_delete(v);

  return e;
}

static geom_mat43
geom_shape_tra_func(void *p)
{
  struct geom_shape_tra_data *pp;

  pp = (struct geom_shape_tra_data *)p;

  return geom_mat43_c(1.0, 0.0, 0.0, geom_vec3_x(pp->amount),
                      0.0, 1.0, 0.0, geom_vec3_y(pp->amount),
                      0.0, 0.0, 1.0, geom_vec3_z(pp->amount));
}

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_tra = {
  .enum_val = GEOM_SHAPE_TRA,
  .shape_type = GEOM_SHPT_TRANS,

  .c = {
    .allocator = geom_shape_tra_allocator,
    .deallocator = geom_shape_tra_deallocator,
    .set_value = geom_shape_tra_set_value,
    .get_value = geom_shape_tra_get_value,
    .n_params = geom_shape_tra_n_params,
    .args_next = geom_shape_tra_args_next,
    .args_check = geom_shape_tra_args_check,
    .infomap_gen = geom_shape_tra_info_map,

    /* Transformation does not require copy function */
    .copy = NULL,
  },

  .body_testf = NULL,
  .body_bboxf = NULL,

  /* For transformation, set transformation function */
  .transform_func = geom_shape_tra_func,
};

geom_error geom_install_shape_tra(void)
{
  return geom_install_shape_func(&geom_shape_tra);
}
