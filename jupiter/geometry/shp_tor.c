
#include <stdlib.h>

#include "defs.h"
#include "geom_assert.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "vector.h"
#include "infomap.h"

#include "shape_revolution.h"
#include "shp_tor.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for TOR (Torus) shape
 */
struct geom_shape_tor_data
{
  struct geom_shape_revolution_data rev;
  double major_radius;     ///< Distance from the center of the tube to the center of the torus.
  double minor_radius;     ///< Radius of the tube
};


static geom_variant_type
geom_shape_tor_args_next(geom_args_builder *b, geom_variant *description,
                         int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);

  switch (l) {
  case 0:
    geom_variant_set_string(description, "Base point", 0);
    return GEOM_VARTYPE_VECTOR3;
  case 1:
    geom_variant_set_string(description, "Normal of the torus", 0);
    return GEOM_VARTYPE_VECTOR3;
  case 2:
    geom_variant_set_string(description, "Major Radius", 0);
    return GEOM_VARTYPE_DOUBLE;
  case 3:
    geom_variant_set_string(description, "Minor Radius", 0);
    return GEOM_VARTYPE_DOUBLE;
  }
  return GEOM_VARTYPE_NULL;
}

static geom_error
geom_shape_tor_args_check(void *p, geom_args_builder *b, geom_size_type index,
                          const geom_variant *v, geom_variant *errinfo)
{
  geom_error e;
  geom_vec3 vec;
  double x;

  if (index < 0 || index > 3) {
    return GEOM_ERR_RANGE;
  }

  if (index == 0 || index == 1) {
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
  if (index == 2 || index == 3) {
    e = GEOM_SUCCESS;
    x = geom_variant_get_double(v, &e);
    if (e != GEOM_SUCCESS) return e;

    if (!isfinite(x) || x <= 0.0) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Radius must be positive", 0);
      } else {
        geom_warn("%g: Radius must be positive", x);
      }
      return GEOM_ERR_RANGE;
    }

    return GEOM_SUCCESS;
  }

  return GEOM_ERR_RANGE;
}

static geom_error geom_shape_tor_set_value(void *p, geom_size_type index,
                                           const geom_variant *value)
{
  geom_error e;
  struct geom_shape_tor_data *pp;

  pp = (struct geom_shape_tor_data *)p;

  e = GEOM_SUCCESS;
  if (index == 0 || index == 1) {
    geom_vec3 v;
    v = geom_variant_get_vec3(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    switch (index) {
    case 0:
      pp->rev.origin = v;
      break;
    case 1:
      pp->rev.axis = v;
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
      pp->major_radius = d;
      break;
    case 3:
      pp->minor_radius = d;
      break;
    default:
      GEOM_UNREACHABLE();
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  }

  return GEOM_ERR_RANGE;
}

static geom_error geom_shape_tor_get_value(void *p, geom_size_type index,
                                           geom_variant *out_variable)
{
  struct geom_shape_tor_data *pp;

  pp = (struct geom_shape_tor_data *)p;

  switch (index) {
  case 0:
    return geom_variant_set_vec3(out_variable, pp->rev.origin);
  case 1:
    return geom_variant_set_vec3(out_variable, pp->rev.axis);
  case 2:
    return geom_variant_set_double(out_variable, pp->major_radius);
  case 3:
    return geom_variant_set_double(out_variable, pp->minor_radius);
  }
  return GEOM_ERR_RANGE;
}

static geom_size_type geom_shape_tor_n_params(void *p, geom_args_builder *b)
{
  return 4;
}

static void *
geom_shape_tor_allocator(void)
{
  struct geom_shape_tor_data *p;
  p = (struct geom_shape_tor_data *)
    malloc(sizeof(struct geom_shape_tor_data));
  if (!p) return NULL;

  p->rev.origin = geom_vec3_c(0.0, 0.0, 0.0);
  p->rev.axis = geom_vec3_c(0.0, 0.0, 0.0);
  p->major_radius = 0.0;
  p->minor_radius = 0.0;

  return p;
}

static void
geom_shape_tor_deallocator(void *p)
{
  free(p);
}

static void *
geom_shape_tor_copy(void *p)
{
  struct geom_shape_tor_data *pp, *copy;

  pp = (struct geom_shape_tor_data *)p;
  copy = (struct geom_shape_tor_data *)geom_shape_tor_allocator();
  if (!copy) return NULL;

  copy->rev = pp->rev;
  copy->major_radius = pp->major_radius;
  copy->minor_radius = pp->minor_radius;
  return copy;
}

static int
geom_shape_tor_testf_plain(geom_vec2 p, void *data)
{
  struct geom_shape_tor_data *pp;
  geom_vec2 c;

  pp = (struct geom_shape_tor_data *)data;

  c = geom_vec2_c(pp->major_radius, 0.0);
  p = geom_vec2_sub(p, c);

  if (geom_vec2_length(p) <= pp->minor_radius) return 1;
  return 0;
}

static int
geom_shape_tor_testf(void *p, double x, double y, double z)
{
  struct geom_shape_tor_data *pp;

  pp = (struct geom_shape_tor_data *)p;

  return geom_shape_revolution_testf(&pp->rev, x, y, z,
                                     geom_shape_tor_testf_plain, p);
}

static geom_error
geom_shape_tor_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_shape_tor_data *pp;

  pp = (struct geom_shape_tor_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  geom_variant_set_vec3(v, pp->rev.origin);
  geom_info_map_append(list, v, "Base point", "L", &e);

  geom_variant_set_vec3(v, pp->rev.axis);
  geom_info_map_append(list, v, "Normal vector", "", &e);

  geom_variant_set_double(v, pp->major_radius);
  geom_info_map_append(list, v, "Major Radius of torus", "L", &e);

  geom_variant_set_double(v, pp->minor_radius);
  geom_info_map_append(list, v, "Minor Radius of torus", "L", &e);

  geom_variant_delete(v);

  return e;
}

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_tor = {
  .enum_val = GEOM_SHAPE_TOR,
  .shape_type = GEOM_SHPT_BODY,

  .c = {
    .allocator = geom_shape_tor_allocator,
    .deallocator = geom_shape_tor_deallocator,
    .set_value = geom_shape_tor_set_value,
    .get_value = geom_shape_tor_get_value,
    .n_params = geom_shape_tor_n_params,
    .args_next = geom_shape_tor_args_next,
    .args_check = geom_shape_tor_args_check,
    .infomap_gen = geom_shape_tor_info_map,
    .copy = geom_shape_tor_copy,
  },

  .body_testf = geom_shape_tor_testf,

  /*
   * Not implemented yet.
   */
  .body_bboxf = NULL,

  .transform_func = NULL,
};

geom_error geom_install_shape_tor(void)
{
  return geom_install_shape_func(&geom_shape_tor);
}
