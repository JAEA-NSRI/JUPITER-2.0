
#include <stdlib.h>

#include "defs.h"
#include "geom_assert.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "vector.h"
#include "quat.h"
#include "infomap.h"

#include "shape_extrusion.h"
#include "shp_trc.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for TRC (Truncated Right Cone) shape
 */
struct geom_shape_trc_data
{
  struct geom_shape_extrusion_data extr;
  double a;          ///< Upper base scale factor
};

static const char *geom_shape_trc_descs[] = {
  "Base point", "Height vector",
  "Bottom base radius", "Upper base factor",
};

static geom_variant_type
geom_shape_trc_args_next(geom_args_builder *b, geom_variant *description,
                         int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);
  if (l < 0 || l > 3) {
    return GEOM_VARTYPE_NULL;
  }

  geom_variant_set_string(description, geom_shape_trc_descs[l], 0);

  if (l == 2 || l == 3) {
    return GEOM_VARTYPE_DOUBLE;
  }
  return GEOM_VARTYPE_VECTOR3;
}

static geom_error
geom_shape_trc_args_check(void *p, geom_args_builder *b, geom_size_type index,
                          const geom_variant *v, geom_variant *errinfo)
{
  geom_error e;
  geom_vec3 vec;
  double x;

  if (index < 0 || index > 3) {
    return GEOM_ERR_RANGE;
  }

  e = GEOM_SUCCESS;
  if (index == 2 || index == 3) {
    x = geom_variant_get_double(v, &e);
    if (e != GEOM_SUCCESS) return e;

    if (!isfinite(x)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Value must be finite", 0);
      } else {
        geom_warn("%g: Value must be finite", x);
      }
      return GEOM_ERR_RANGE;
    }

    if (index == 2) {
      if (x <= 0.0) {
        if (errinfo) {
          geom_variant_set_string(errinfo, "Base radius must be positive", 0);
        } else {
          geom_warn("%g: Base radius must be positive", x);
        }
        return GEOM_ERR_RANGE;
      }
    }
    return GEOM_SUCCESS;
  }

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

  if (index == 1) {
    if (geom_vec3_iszero(vec)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Height vector is zero", 0);
      } else {
        geom_warn("(%g, %g, %g): Height vector is zero",
                  geom_vec3_x(vec), geom_vec3_y(vec), geom_vec3_z(vec));
      }
      return GEOM_ERR_RANGE;
    }
  }

  return GEOM_SUCCESS;
}

static void
geom_shape_trc_update_base(struct geom_shape_trc_data *pp)
{
  geom_quat q;
  double radius;

  radius = geom_vec3_length(pp->extr.base);
  if (radius > 0.0 &&
      !geom_vec3_eql(pp->extr.height, geom_vec3_c(0.0, 0.0, 0.0))) {
    q = geom_vec3_get_rotation(geom_vec3_c(0.0, 0.0, 1.0), pp->extr.height);
    pp->extr.base = geom_quat_rotate_p(q, geom_vec3_c(radius, 0.0, 0.0));
  } else {
    pp->extr.base = geom_vec3_c(radius, 0.0, 0.0);
  }
}

static geom_error
geom_shape_trc_set_value(void *p, geom_size_type index,
                         const geom_variant *value)
{
  geom_error e;
  struct geom_shape_trc_data *pp;

  pp = (struct geom_shape_trc_data *)p;

  e = GEOM_SUCCESS;
  if (index == 0 || index == 1) {
    geom_vec3 v;
    v = geom_variant_get_vec3(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    switch (index) {
    case 0:
      pp->extr.origin = v;
      break;
    case 1:
      pp->extr.height = v;
      geom_shape_trc_update_base(pp);
      geom_shape_extrusion_adjust_base(&pp->extr);
      break;
    default:
      GEOM_UNREACHABLE();
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  }
  if (index == 2 || index == 3) {
    double d;
    d = geom_variant_get_double(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    switch (index) {
    case 2:
      pp->extr.base = geom_vec3_c(d, 0.0, 0.0);
      geom_shape_trc_update_base(pp);
      geom_shape_extrusion_adjust_base(&pp->extr);
      break;
    case 3:
      pp->a = d;
      break;
    default:
      GEOM_UNREACHABLE();
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  }
  return GEOM_ERR_RANGE;
}

static geom_error
geom_shape_trc_get_value(void *p, geom_size_type index,
                         geom_variant *out_variable)
{
  struct geom_shape_trc_data *pp;
  pp = (struct geom_shape_trc_data *)p;

  switch (index) {
  case 0:
    return geom_variant_set_vec3(out_variable, pp->extr.origin);
  case 1:
    return geom_variant_set_vec3(out_variable, pp->extr.height);
  case 2:
    return geom_variant_set_double(out_variable, geom_vec3_length(pp->extr.base));
  case 3:
    return geom_variant_set_double(out_variable, pp->a);
  }
  return GEOM_ERR_RANGE;
}

static geom_size_type
geom_shape_trc_n_params(void *p, geom_args_builder *b)
{
  return 4;
}

static void *
geom_shape_trc_allocator(void)
{
  struct geom_shape_trc_data *p;
  p = (struct geom_shape_trc_data *)
    malloc(sizeof(struct geom_shape_trc_data));
  if (!p) return NULL;

  p->a = 0.0;
  p->extr.origin = geom_vec3_c(0.0, 0.0, 0.0);
  p->extr.base   = geom_vec3_c(0.0, 0.0, 0.0);
  p->extr.height = geom_vec3_c(0.0, 0.0, 0.0);

  return p;
}

static void
geom_shape_trc_deallocator(void *p)
{
  free(p);
}

static void *
geom_shape_trc_copy(void *p)
{
  struct geom_shape_trc_data *pp, *copy;

  pp = (struct geom_shape_trc_data *)p;
  copy = (struct geom_shape_trc_data *)geom_shape_trc_allocator();
  if (!copy) return NULL;

  copy->extr = pp->extr;
  copy->a = pp->a;
  return copy;
}


static int
geom_shape_trc_base_plane(double r, double t, double h, void *p)
{
  struct geom_shape_trc_data *pp;
  double base;
  geom_vec3 vb;
  double a;

  pp = (struct geom_shape_trc_data *)p;

  vb = pp->extr.base;
  base = geom_vec3_length(vb);
  if (base == 0.0) return 0;

  a = geom_shape_extrusion_linear_scale_factor(pp->a, h);

  vb = geom_vec3_factor(vb, a);
  base *= a;
  if (base < 0.0) base = -base;
  if (r <= base) return 1;

  return 0;
}

static int
geom_shape_trc_testf(void *p, double x, double y, double z)
{
  struct geom_shape_trc_data *pp;

  pp = (struct geom_shape_trc_data *)p;

  return geom_shape_extrusion_testf(&pp->extr, x, y, z, NULL,
                                    geom_shape_trc_base_plane, pp);
}

static geom_error
geom_shape_trc_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_shape_trc_data *pp;

  pp = (struct geom_shape_trc_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  geom_variant_set_vec3(v, pp->extr.origin);
  geom_info_map_append(list, v, "Base point", "L", &e);

  geom_variant_set_vec3(v, pp->extr.height);
  geom_info_map_append(list, v, "Height vector", "L", &e);

  geom_variant_set_double(v, geom_vec3_length(pp->extr.base));
  geom_info_map_append(list, v, "Bottom base radius", "L", &e);

  geom_variant_set_double(v, pp->a);
  geom_info_map_append(list, v, "Upper base factor", "", &e);

  geom_variant_delete(v);

  return e;
}

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_trc = {
  .enum_val = GEOM_SHAPE_TRC,
  .shape_type = GEOM_SHPT_BODY,

  .c = {
    .allocator = geom_shape_trc_allocator,
    .deallocator = geom_shape_trc_deallocator,
    .set_value = geom_shape_trc_set_value,
    .get_value = geom_shape_trc_get_value,
    .n_params = geom_shape_trc_n_params,
    .args_next = geom_shape_trc_args_next,
    .args_check = geom_shape_trc_args_check,
    .infomap_gen = geom_shape_trc_info_map,
    .copy = geom_shape_trc_copy,
  },

  .body_testf = geom_shape_trc_testf,

  /*
   * Not implemented.
   */
  .body_bboxf = NULL,

  .transform_func = NULL,
};

geom_error geom_install_shape_trc(void)
{
  return geom_install_shape_func(&geom_shape_trc);
}
