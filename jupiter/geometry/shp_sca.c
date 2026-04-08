
#include <stdlib.h>

#include "defs.h"
#include "geom_assert.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "vector.h"
#include "mat43.h"
#include "infomap.h"

#include "shp_sca.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for SCA transformation (Scale)
 */
struct geom_shape_sca_data
{
  geom_vec3 base_pnt; ///< Base point
  double x;           ///< Factor for X direction
  double y;           ///< Factor for Y direction
  double z;           ///< Factor for Z direction
};

static geom_variant_type
geom_shape_sca_args_next(geom_args_builder *b, geom_variant *description,
                         int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);
  switch (l) {
  case 0:
    geom_variant_set_string(description, "Base point vector", 0);
    return GEOM_VARTYPE_VECTOR3;
  case 1:
    geom_variant_set_string(description, "Factor for X direction", 0);
    return GEOM_VARTYPE_DOUBLE;
  case 2:
    geom_variant_set_string(description, "Factor for Y direction", 0);
    return GEOM_VARTYPE_DOUBLE;
  case 3:
    geom_variant_set_string(description, "Factor for Z direction", 0);
    return GEOM_VARTYPE_DOUBLE;
  default:
    return GEOM_VARTYPE_NULL;
  }
}

static geom_error
geom_shape_sca_args_check(void *p, geom_args_builder *b, geom_size_type index,
                          const geom_variant *v, geom_variant *errinfo)
{
  geom_error e;
  geom_vec3 vec;
  double x;

  e = GEOM_SUCCESS;

  switch (index) {
  case 0:
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
  case 1:
  case 2:
  case 3:
    x = geom_variant_get_double(v, &e);
    if (e != GEOM_SUCCESS) return e;
    if (!isfinite(x)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Factor must be finite", 0);
      } else {
        geom_warn("%g: Value should be finite", x);
      }
      return GEOM_ERR_RANGE;
    }
    if (x == 0.0) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Factor must not be zero", 0);
      } else {
        geom_warn("%g: Value must not be zero", x);
      }
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  default:
    return GEOM_ERR_RANGE;
  }
}

static geom_error geom_shape_sca_set_value(void *p, geom_size_type index,
                                           const geom_variant *value)
{
  geom_error e;
  struct geom_shape_sca_data *pp;

  pp = (struct geom_shape_sca_data *)p;

  e = GEOM_SUCCESS;
  if (index == 0) {
    geom_vec3 v;
    v = geom_variant_get_vec3(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    pp->base_pnt = v;
    return GEOM_SUCCESS;
  }
  if (index == 1 || index == 2 || index == 3) {
    double d;
    d = geom_variant_get_double(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    switch (index) {
    case 1:
      pp->x = d;
      break;
    case 2:
      pp->y = d;
      break;
    case 3:
      pp->z = d;
      break;
    default:
      GEOM_UNREACHABLE();
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  }
  return GEOM_ERR_RANGE;
}

static geom_error geom_shape_sca_get_value(void *p, geom_size_type index,
                                           geom_variant *out_variable)
{
  struct geom_shape_sca_data *pp;

  pp = (struct geom_shape_sca_data *)p;

  switch (index) {
  case 0:
    return geom_variant_set_vec3(out_variable, pp->base_pnt);
  case 1:
    return geom_variant_set_double(out_variable, pp->x);
  case 2:
    return geom_variant_set_double(out_variable, pp->y);
  case 3:
    return geom_variant_set_double(out_variable, pp->z);
  }
  return GEOM_ERR_RANGE;
}

static geom_size_type geom_shape_sca_n_params(void *p, geom_args_builder *b)
{
  return 4;
}

static void *
geom_shape_sca_allocator(void)
{
  struct geom_shape_sca_data *p;
  p = (struct geom_shape_sca_data *)
    malloc(sizeof(struct geom_shape_sca_data));
  if (!p) return NULL;

  p->base_pnt = geom_vec3_c(0.0, 0.0, 0.0);
  p->x = 0.0;
  p->y = 0.0;
  p->z = 0.0;

  return p;
}

static void
geom_shape_sca_deallocator(void *p)
{
  free(p);
}

static geom_error
geom_shape_sca_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_shape_sca_data *pp;

  pp = (struct geom_shape_sca_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  geom_variant_set_vec3(v, pp->base_pnt);
  geom_info_map_append(list, v, "Base point", "L", &e);

  geom_variant_set_double(v, pp->x);
  geom_info_map_append(list, v, "Scale factor in X direction", "", &e);

  geom_variant_set_double(v, pp->y);
  geom_info_map_append(list, v, "Scale factor in Y direction", "", &e);

  geom_variant_set_double(v, pp->z);
  geom_info_map_append(list, v, "Scale factor in Z direction", "", &e);

  geom_variant_delete(v);

  return e;
}

static geom_mat43
geom_shape_sca_func(void *p)
{
  struct geom_shape_sca_data *pp;
  geom_mat43 f, r, sca;
  double x, y, z;

  pp = (struct geom_shape_sca_data *)p;

  x = geom_vec3_x(pp->base_pnt);
  y = geom_vec3_y(pp->base_pnt);
  z = geom_vec3_z(pp->base_pnt);

  f = geom_mat43_c(1.0, 0.0, 0.0,  x,
                   0.0, 1.0, 0.0,  y,
                   0.0, 0.0, 1.0,  z);
  r = geom_mat43_c(1.0, 0.0, 0.0, -x,
                   0.0, 1.0, 0.0, -y,
                   0.0, 0.0, 1.0, -z);
  sca = geom_mat43_c(pp->x, 0.0, 0.0, 0.0,
                     0.0, pp->y, 0.0, 0.0,
                     0.0, 0.0, pp->z, 0.0);
  f = geom_mat43_mul(f, sca);
  return geom_mat43_mul(f, r);
 }

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_sca = {
  .enum_val = GEOM_SHAPE_SCA,
  .shape_type = GEOM_SHPT_TRANS,

  .c = {
    .allocator = geom_shape_sca_allocator,
    .deallocator = geom_shape_sca_deallocator,
    .set_value = geom_shape_sca_set_value,
    .get_value = geom_shape_sca_get_value,
    .n_params = geom_shape_sca_n_params,
    .args_next = geom_shape_sca_args_next,
    .args_check = geom_shape_sca_args_check,
    .infomap_gen = geom_shape_sca_info_map,
    .copy = NULL,
  },

  .body_testf = NULL,
  .body_bboxf = NULL,

  /* For transformation, set transformation function */
  .transform_func = geom_shape_sca_func,
};

geom_error geom_install_shape_sca(void)
{
  return geom_install_shape_func(&geom_shape_sca);
}
