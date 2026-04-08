
#include <stdlib.h>

#include "defs.h"
#include "geom_assert.h"
#include "func_defs.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "vector.h"
#include "infomap.h"

#include "shp_pla.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for PLA (plane with normal) shape
 */
struct geom_shape_pla_data
{
  geom_vec3 basep;   ///< Base point
  geom_vec3 normal;  ///< Normal vector
};


static geom_variant_type
geom_shape_pla_args_next(geom_args_builder *b, geom_variant *description,
                         int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);
  if (l < 0 || l > 1) {
    return GEOM_VARTYPE_NULL;
  }

  switch (l) {
  case 0: geom_variant_set_string(description, "Base point", 0); break;
  case 1: geom_variant_set_string(description, "Normal vector", 0); break;
  }
  return GEOM_VARTYPE_VECTOR3;
}

static geom_error
geom_shape_pla_args_check(void *p, geom_args_builder *b, geom_size_type index,
                          const geom_variant *v, geom_variant *errinfo)
{
  geom_error e;
  geom_vec3 vec;

  if (index < 0 || index > 1) {
    return GEOM_ERR_RANGE;
  }

  e = GEOM_SUCCESS;
  vec = geom_variant_get_vec3(v, &e);
  if (e != GEOM_SUCCESS) return e;

  if (index == 1) {
    if (geom_vec3_iszero(vec)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Zero vector given for normal", 0);
      } else {
        geom_warn("(%g, %g, %g): Zero vector given for normal",
                  geom_vec3_x(vec), geom_vec3_y(vec), geom_vec3_z(vec));
      }
      return GEOM_ERR_RANGE;
    }
  }

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

static geom_error geom_shape_pla_set_value(void *p, geom_size_type index,
                                           const geom_variant *value)
{
  geom_error err;
  geom_vec3 v;
  struct geom_shape_pla_data *pp;
  pp = (struct geom_shape_pla_data *)p;

  if (index < 0 || index > 1) {
    return GEOM_ERR_RANGE;
  }

  err = GEOM_SUCCESS;
  v = geom_variant_get_vec3(value, &err);
  if (err != GEOM_SUCCESS) {
    return err;
  }

  switch (index) {
  case 0:
    pp->basep = v;
    return GEOM_SUCCESS;
  case 1:
    pp->normal = v;
    return GEOM_SUCCESS;
  }
  GEOM_UNREACHABLE();
  return GEOM_ERR_RANGE;
}

static geom_error geom_shape_pla_get_value(void *p, geom_size_type index,
                                           geom_variant *out_variable)
{
  struct geom_shape_pla_data *pp;
  pp = (struct geom_shape_pla_data *)p;

  switch (index) {
  case 0:
    return geom_variant_set_vec3(out_variable, pp->basep);
  case 1:
    return geom_variant_set_vec3(out_variable, pp->normal);
  }
  return GEOM_ERR_RANGE;
}

static geom_size_type geom_shape_pla_n_params(void *p, geom_args_builder *b)
{
  return 2;
}

static void *
geom_shape_pla_allocator(void)
{
  struct geom_shape_pla_data *p;
  p = (struct geom_shape_pla_data *)
    malloc(sizeof(struct geom_shape_pla_data));
  if (!p) return NULL;

  p->basep  = geom_vec3_c(0.0, 0.0, 0.0);
  p->normal = geom_vec3_c(0.0, 0.0, 0.0);

  return p;
}

static void
geom_shape_pla_deallocator(void *p)
{
  free(p);
}

static void *
geom_shape_pla_copy(void *p)
{
  struct geom_shape_pla_data *pp, *copy;

  pp = (struct geom_shape_pla_data *)p;
  copy = (struct geom_shape_pla_data *)geom_shape_pla_allocator();
  if (!copy) return NULL;

  copy->basep = pp->basep;
  copy->normal = pp->normal;
  return copy;
}

static int
geom_shape_pla_testf(void *p, double x, double y, double z)
{
  struct geom_shape_pla_data *pp;
  geom_vec3 vp;
  double t;

  pp = (struct geom_shape_pla_data *)p;

  vp = geom_vec3_c(x, y, z);
  vp = geom_vec3_sub(vp, pp->basep);
  t = geom_vec3_inner_prod(vp, pp->normal);
  if (t <= 0.0) return 1;
  return 0;
}

static geom_error
geom_shape_pla_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_shape_pla_data *pp;

  pp = (struct geom_shape_pla_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  geom_variant_set_vec3(v, pp->basep);
  geom_info_map_append(list, v, "Base point", "L", &e);

  geom_variant_set_vec3(v, pp->normal);
  geom_info_map_append(list, v, "Normal vector", "", &e);

  geom_variant_delete(v);

  return e;
}

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_pla = {
  .enum_val = GEOM_SHAPE_PLA,
  .shape_type = GEOM_SHPT_BODY,

  .c = {
    .allocator = geom_shape_pla_allocator,
    .deallocator = geom_shape_pla_deallocator,
    .set_value = geom_shape_pla_set_value,
    .get_value = geom_shape_pla_get_value,
    .n_params = geom_shape_pla_n_params,
    .args_next = geom_shape_pla_args_next,
    .args_check = geom_shape_pla_args_check,
    .infomap_gen = geom_shape_pla_info_map,
    .copy = geom_shape_pla_copy,
  },

  .body_testf = geom_shape_pla_testf,

  /*
   * Plane has infinite bounding box.
   */
  .body_bboxf = NULL,

  .transform_func = NULL,
};

geom_error geom_install_shape_pla(void)
{
  return geom_install_shape_func(&geom_shape_pla);
}
