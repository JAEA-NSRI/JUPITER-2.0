
#include <stdlib.h>

#include "common.h"
#include "defs.h"
#include "geom_assert.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "vector.h"
#include "quat.h"
#include "mat43.h"
#include "mat44.h"
#include "geom_math.h"
#include "infomap.h"

#include "shp_rot.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for ROT transformation (Rotate)
 */
struct geom_shape_rot_data
{
  geom_vec3 base_pnt; ///< Base point
  geom_vec3 axis;     ///< Rotation axis vector
  double angle;       ///< Rotation angle in degrees
};

static geom_variant_type
geom_shape_rot_args_next(geom_args_builder *b, geom_variant *description,
                         int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);
  switch (l) {
  case 0:
    geom_variant_set_string(description, "Base point vector", 0);
    return GEOM_VARTYPE_VECTOR3;
  case 1:
    geom_variant_set_string(description, "Rotation axis vector", 0);
    return GEOM_VARTYPE_VECTOR3;
  case 2:
    geom_variant_set_string(description, "Rotation angle in degrees", 0);
    return GEOM_VARTYPE_DOUBLE;
  default:
    return GEOM_VARTYPE_NULL;
  }
}

static geom_error
geom_shape_rot_args_check(void *p, geom_args_builder *b, geom_size_type index,
                          const geom_variant *v, geom_variant *errinfo)
{
  geom_error e;
  geom_vec3 vec;
  double x;

  e = GEOM_SUCCESS;

  switch (index) {
  case 1:
    vec = geom_variant_get_vec3(v, &e);
    if (e != GEOM_SUCCESS) return e;
    if (geom_vec3_iszero(vec)) {
      if (errinfo) {
        geom_variant_set_string(errinfo,
                                "Rotatation axis must not be zero vector", 0);
      } else {
        geom_warn("(%g, %g, %g): Rotation axis must not be zero vector",
                  geom_vec3_x(vec), geom_vec3_y(vec), geom_vec3_z(vec));
      }
      return GEOM_ERR_RANGE;
    }
    GEOM_DOFALLTHRU(); // fall through
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
  case 2:
    x = geom_variant_get_double(v, &e);
    if (e != GEOM_SUCCESS) return e;
    if (!isfinite(x)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Angle value must be finite", 0);
      } else {
        geom_warn("%g: Value should be finite", x);
      }
      return GEOM_ERR_RANGE;
    }
    if (x > 1e+15 || x < -1e+15) { /* see geom_degree_norm(). */
      if (errinfo) {
        geom_variant_set_string(errinfo, "Too large angle", 0);
      } else {
        geom_warn("%g: Too large angle", x);
      }
      return GEOM_ERR_RANGE;
    }
    if (geom_degree_norm(x) != x) {
      geom_warn("%g: Unnormalized angle (should be -180 < x <= 180)", x);
    }
    return GEOM_SUCCESS;
  default:
    return GEOM_ERR_RANGE;
  }
}

static geom_error geom_shape_rot_set_value(void *p, geom_size_type index,
                                           const geom_variant *value)
{
  geom_error e;
  struct geom_shape_rot_data *pp;

  pp = (struct geom_shape_rot_data *)p;

  e = GEOM_SUCCESS;
  if (index == 0 || index == 1) {
    geom_vec3 v;
    v = geom_variant_get_vec3(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    switch (index) {
    case 0:
      pp->base_pnt = v;
      break;
    case 1:
      pp->axis = v;
      break;
    default:
      GEOM_UNREACHABLE();
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  }
  if (index == 2) {
    double d;
    d = geom_variant_get_double(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    pp->angle = d;
    return GEOM_SUCCESS;
  }

  return GEOM_ERR_RANGE;
}

static geom_error geom_shape_rot_get_value(void *p, geom_size_type index,
                                           geom_variant *out_variable)
{
  struct geom_shape_rot_data *pp;

  pp = (struct geom_shape_rot_data *)p;

  switch (index) {
  case 0:
    return geom_variant_set_vec3(out_variable, pp->base_pnt);
  case 1:
    return geom_variant_set_vec3(out_variable, pp->axis);
  case 2:
    return geom_variant_set_double(out_variable, pp->angle);
  }
  return GEOM_ERR_RANGE;
}

static geom_size_type geom_shape_rot_n_params(void *p, geom_args_builder *b)
{
  return 3;
}

static void *
geom_shape_rot_allocator(void)
{
  struct geom_shape_rot_data *p;
  p = (struct geom_shape_rot_data *)
    malloc(sizeof(struct geom_shape_rot_data));
  if (!p) return NULL;

  p->angle = 0.0;
  p->axis = geom_vec3_c(0.0, 0.0, 0.0);
  p->base_pnt = geom_vec3_c(0.0, 0.0, 0.0);

  return p;
}

static void
geom_shape_rot_deallocator(void *p)
{
  free(p);
}

static geom_error
geom_shape_rot_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_shape_rot_data *pp;

  pp = (struct geom_shape_rot_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  geom_variant_set_vec3(v, pp->base_pnt);
  geom_info_map_append(list, v, "Base point", "L", &e);

  geom_variant_set_vec3(v, pp->axis);
  geom_info_map_append(list, v, "Rotation axis vector", "", &e);

  geom_variant_set_double(v, pp->angle);
  geom_info_map_append(list, v, "Rotation angle", "deg", &e);

  geom_variant_delete(v);

  return e;
}

static geom_mat43
geom_shape_rot_func(void *p)
{
  struct geom_shape_rot_data *pp;
  geom_mat43 f, r, rot;
  geom_quat q;
  double x, y, z;

  pp = (struct geom_shape_rot_data *)p;

  x = geom_vec3_x(pp->base_pnt);
  y = geom_vec3_y(pp->base_pnt);
  z = geom_vec3_z(pp->base_pnt);

  f = geom_mat43_c(1.0, 0.0, 0.0,  x,
                   0.0, 1.0, 0.0,  y,
                   0.0, 0.0, 1.0,  z);
  r = geom_mat43_c(1.0, 0.0, 0.0, -x,
                   0.0, 1.0, 0.0, -y,
                   0.0, 0.0, 1.0, -z);
  if (!geom_vec3_iszero(pp->axis)) {
    q = geom_quat_rotation(pp->axis, pp->angle);
  } else {
    q = geom_quat_rotation(geom_vec3_c(1.0, 0.0, 0.0), 0.0);
  }
  rot = geom_quat_to_mat43(q);
  f = geom_mat43_mul(f, rot);
  return geom_mat43_mul(f, r);
 }

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_rot = {
  .enum_val = GEOM_SHAPE_ROT,
  .shape_type = GEOM_SHPT_TRANS,

  .c = {
    .allocator = geom_shape_rot_allocator,
    .deallocator = geom_shape_rot_deallocator,
    .set_value = geom_shape_rot_set_value,
    .get_value = geom_shape_rot_get_value,
    .n_params = geom_shape_rot_n_params,
    .args_next = geom_shape_rot_args_next,
    .args_check = geom_shape_rot_args_check,
    .infomap_gen = geom_shape_rot_info_map,
    .copy = NULL,
  },

  .body_testf = NULL,
  .body_bboxf = NULL,

  /* For transformation, set transformation function */
  .transform_func = geom_shape_rot_func,
};

geom_error geom_install_shape_rot(void)
{
  return geom_install_shape_func(&geom_shape_rot);
}
