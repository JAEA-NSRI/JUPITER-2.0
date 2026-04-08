#define GEOM_SHAPE_SURFACE_N 3

#include <math.h>
#include <stdlib.h>

#include "2d_circle.h"
#include "2d_inout_tests.h"
#include "defs.h"
#include "geom_static_assert.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "vector.h"
#include "infomap.h"

#include "geom_math.h"
#include "2d_pathcalc.h"
#include "2d_rectangle.h"
#include "2d_level_set.h"
#include "quat.h"
#include "shape_revolution.h"
#include "shape_surface_base.h"
#include "shp_rcc.h"

enum geom_shape_rcc_surface_index
{
  GEOM_RCC_BOTTOM_SURFACE,
  GEOM_RCC_TOP_SURFACE,
  GEOM_RCC_SIDE_SURFACE,
  GEOM_RCC_SURFACE_MAX,
};
GEOM_STATIC_ASSERT(GEOM_RCC_SURFACE_MAX == GEOM_SHAPE_SURFACE_N,
                   "GEOM_RCC_SURFACE_MAX does not match");

enum geom_shape_rcc_index
{
  GEOM_RCC_BASE_POINT = 0,
  GEOM_RCC_HEIGHT_VECTOR,
  GEOM_RCC_RADIUS,
  GEOM_RCC_SURFACE_START,

  GEOM_RCC_N_PARAMS = GEOM_RCC_SURFACE_START + GEOM_RCC_SURFACE_MAX,

  GEOM_RCC_BOT_UV_CENTER = GEOM_RCC_SURFACE_START + GEOM_RCC_BOTTOM_SURFACE,
  GEOM_RCC_TOP_UV_CENTER = GEOM_RCC_SURFACE_START + GEOM_RCC_TOP_SURFACE,
  GEOM_RCC_SIDE_UV_ORIGIN = GEOM_RCC_SURFACE_START + GEOM_RCC_SIDE_SURFACE,
};

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for RCC (Right circular cylinder) shape
 */
struct geom_shape_rcc_data
{
  struct geom_shape_revolution_data rev;
  double radius;                         ///< Radius of cylinder
  struct geom_shape_surface_common surf; ///< Common surface properties
  geom_vec2 surface_bot_center;          ///< UV center of bottom surface
  geom_vec2 surface_top_center;          ///< UV center of top surface
  geom_vec2 surface_side_origin;         ///< UV origin of side surface
};

static geom_variant_type geom_shape_rcc_args_next(geom_args_builder *b,
                                                  geom_variant *description,
                                                  int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);

  switch (l) {
  case GEOM_RCC_BASE_POINT:
    geom_variant_set_string(description, "Base point", 0);
    return GEOM_VARTYPE_VECTOR3;

  case GEOM_RCC_HEIGHT_VECTOR:
    geom_variant_set_string(description, "Height vector", 0);
    return GEOM_VARTYPE_VECTOR3;

  case GEOM_RCC_RADIUS:
    geom_variant_set_string(description, "Radius", 0);
    return GEOM_VARTYPE_DOUBLE;

  case GEOM_RCC_BOT_UV_CENTER:
    geom_variant_set_string(description, "Bottom surface UV center", 0);
    *optional = 1;
    return GEOM_VARTYPE_VECTOR2;

  case GEOM_RCC_TOP_UV_CENTER:
    geom_variant_set_string(description, "Top surface UV center", 0);
    *optional = 1;
    return GEOM_VARTYPE_VECTOR2;

  case GEOM_RCC_SIDE_UV_ORIGIN:
    geom_variant_set_string(description, "Side surface UV origin", 0);
    *optional = 1;
    return GEOM_VARTYPE_VECTOR2;
  }
  return GEOM_VARTYPE_NULL;
}

static geom_error geom_shape_rcc_args_check(void *p, geom_args_builder *b,
                                            geom_size_type index,
                                            const geom_variant *v,
                                            geom_variant *errinfo)
{
  if (index == GEOM_RCC_BASE_POINT || index == GEOM_RCC_HEIGHT_VECTOR) {
    geom_error e;
    geom_vec3 vec;
    e = GEOM_SUCCESS;
    vec = geom_variant_get_vec3(v, &e);
    if (e != GEOM_SUCCESS)
      return e;

    if (index == GEOM_RCC_HEIGHT_VECTOR) {
      if (geom_vec3_iszero(vec)) {
        if (errinfo) {
          geom_variant_set_string(errinfo, "Zero vector given for height", 0);
        } else {
          geom_warn("(%g, %g, %g): Zero vector given for height",
                    geom_vec3_x(vec), geom_vec3_y(vec), geom_vec3_z(vec));
        }
        return GEOM_ERR_RANGE;
      }
    }

    if (!geom_vec3_isfinite(vec)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "All elements must be finite", 0);
      } else {
        geom_warn("(%g, %g, %g): Value should be finite", geom_vec3_x(vec),
                  geom_vec3_y(vec), geom_vec3_z(vec));
      }
      return GEOM_ERR_RANGE;
    }

    return GEOM_SUCCESS;
  }
  if (index == GEOM_RCC_RADIUS) {
    geom_error e;
    double x;
    e = GEOM_SUCCESS;
    x = geom_variant_get_double(v, &e);
    if (e != GEOM_SUCCESS)
      return e;

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
  if (index == GEOM_RCC_BOT_UV_CENTER || index == GEOM_RCC_SIDE_UV_ORIGIN ||
      index == GEOM_RCC_TOP_UV_CENTER) {
    geom_error e;
    geom_vec2 v2;
    e = GEOM_SUCCESS;
    if (!geom_variant_is_null(v)) {
      v2 = geom_variant_get_vec2(v, &e);
      if (e != GEOM_SUCCESS)
        return e;
    }

    /* Treats infinite (incldes NaN) vector as disable the surface */
    return GEOM_SUCCESS;
  }

  return GEOM_ERR_RANGE;
}

static geom_error geom_shape_rcc_set_value(void *p, geom_size_type index,
                                           const geom_variant *value)
{
  geom_error e;
  struct geom_shape_rcc_data *pp;

  pp = (struct geom_shape_rcc_data *)p;

  if (index == GEOM_RCC_BASE_POINT || index == GEOM_RCC_HEIGHT_VECTOR) {
    geom_vec3 v;
    v = geom_variant_get_vec3(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    switch (index) {
    case GEOM_RCC_BASE_POINT:
      pp->rev.origin = v;
      break;
    case GEOM_RCC_HEIGHT_VECTOR:
      pp->rev.axis = v;
      break;
    }
    return GEOM_SUCCESS;
  }
  if (index == GEOM_RCC_RADIUS) {
    double r;
    r = geom_variant_get_double(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    pp->radius = r;
    return GEOM_SUCCESS;
  }
  if (index == GEOM_RCC_BOT_UV_CENTER || index == GEOM_RCC_SIDE_UV_ORIGIN ||
      index == GEOM_RCC_TOP_UV_CENTER) {
    geom_vec2 v2;
    int enabled;
    geom_vec2 *dest;

    if (geom_variant_is_null(value)) {
      enabled = 0;
    } else {
      v2 = geom_variant_get_vec2(value, &e);
      if (e != GEOM_SUCCESS) {
        return e;
      }
      if (geom_vec2_isfinite(v2)) {
        enabled = 1;
      } else {
        enabled = 0;
      }
    }

    int surfid;
    switch (index) {
    case GEOM_RCC_BOT_UV_CENTER:
      surfid = GEOM_RCC_BOTTOM_SURFACE;
      dest = &pp->surface_bot_center;
      break;

    case GEOM_RCC_SIDE_UV_ORIGIN:
      surfid = GEOM_RCC_SIDE_SURFACE;
      dest = &pp->surface_side_origin;
      break;

    case GEOM_RCC_TOP_UV_CENTER:
      surfid = GEOM_RCC_TOP_SURFACE;
      dest = &pp->surface_top_center;
      break;

    default:
      GEOM_UNREACHABLE();
      return GEOM_ERR_RANGE;
    }

    if (enabled) {
      geom_shape_surface_common_enable(&pp->surf, surfid);
      *dest = v2;
    } else {
      geom_shape_surface_common_disable(&pp->surf, surfid);
    }
    return GEOM_SUCCESS;
  }

  return GEOM_ERR_RANGE;
}

static geom_error geom_shape_rcc_get_value(void *p, geom_size_type index,
                                           geom_variant *out_variable)
{
  struct geom_shape_rcc_data *pp;

  pp = (struct geom_shape_rcc_data *)p;

  switch (index) {
  case GEOM_RCC_BASE_POINT:
    return geom_variant_set_vec3(out_variable, pp->rev.origin);
  case GEOM_RCC_HEIGHT_VECTOR:
    return geom_variant_set_vec3(out_variable, pp->rev.axis);
  case GEOM_RCC_RADIUS:
    return geom_variant_set_double(out_variable, pp->radius);

  case GEOM_RCC_BOT_UV_CENTER:
    if (!geom_shape_surface_common_is_enabled(&pp->surf,
                                              GEOM_RCC_BOTTOM_SURFACE)) {
      geom_variant_nullify(out_variable);
      return GEOM_SUCCESS;
    }
    return geom_variant_set_vec2(out_variable, pp->surface_bot_center);

  case GEOM_RCC_SIDE_UV_ORIGIN:
    if (!geom_shape_surface_common_is_enabled(&pp->surf,
                                              GEOM_RCC_SIDE_SURFACE)) {
      geom_variant_nullify(out_variable);
      return GEOM_SUCCESS;
    }
    return geom_variant_set_vec2(out_variable, pp->surface_side_origin);

  case GEOM_RCC_TOP_UV_CENTER:
    if (!geom_shape_surface_common_is_enabled(&pp->surf,
                                              GEOM_RCC_TOP_SURFACE)) {
      geom_variant_nullify(out_variable);
      return GEOM_SUCCESS;
    }
    return geom_variant_set_vec2(out_variable, pp->surface_top_center);
  }
  return GEOM_ERR_RANGE;
}

static geom_size_type geom_shape_rcc_n_params(void *p, geom_args_builder *b)
{
  return GEOM_RCC_N_PARAMS;
}

static int geom_shape_rcc_n_surff(void *p) { return GEOM_RCC_SURFACE_MAX; }

static int geom_shape_rcc_has_surff(void *p, int surfid)
{
  struct geom_shape_rcc_data *pp;
  pp = (struct geom_shape_rcc_data *)p;

  switch (surfid) {
  case GEOM_RCC_BOTTOM_SURFACE:
  case GEOM_RCC_TOP_SURFACE:
  case GEOM_RCC_SIDE_SURFACE:
    return geom_shape_surface_common_is_enabled(&pp->surf, surfid);
  }
  return 0;
}

static void *geom_shape_rcc_allocator(void)
{
  struct geom_shape_rcc_data *p;
  p = (struct geom_shape_rcc_data *)malloc(sizeof(struct geom_shape_rcc_data));
  if (!p)
    return NULL;

  p->rev.origin = geom_vec3_c(0.0, 0.0, 0.0);
  p->rev.axis = geom_vec3_c(0.0, 0.0, 0.0);
  p->radius = 0.0;
  geom_shape_surface_common_init(&p->surf);
  p->surface_bot_center = geom_vec2_c(0.0, 0.0);
  p->surface_side_origin = geom_vec2_c(0.0, 0.0);
  p->surface_top_center = geom_vec2_c(0.0, 0.0);

  return p;
}

static void geom_shape_rcc_deallocator(void *p)
{
  if (p) {
    struct geom_shape_rcc_data *pp;
    pp = (struct geom_shape_rcc_data *)p;
    geom_shape_surface_common_clean(&pp->surf);
  }
  free(p);
}

static void *geom_shape_rcc_copy(void *p)
{
  struct geom_shape_rcc_data *pp, *copy;

  pp = (struct geom_shape_rcc_data *)p;
  copy = (struct geom_shape_rcc_data *)geom_shape_rcc_allocator();
  if (!copy)
    return NULL;

  copy->rev = pp->rev;
  copy->radius = pp->radius;
  geom_shape_surface_common_copy(&copy->surf, &pp->surf);
  copy->surface_bot_center = pp->surface_bot_center;
  copy->surface_side_origin = pp->surface_side_origin;
  copy->surface_top_center = pp->surface_top_center;
  return copy;
}

static int geom_shape_rcc_testf(void *p, double x, double y, double z)
{
  geom_vec2 rh;
  double r, h, l;
  struct geom_shape_rcc_data *pp;

  pp = (struct geom_shape_rcc_data *)p;
  geom_shape_revolution_rhvec(&pp->rev, geom_vec3_c(x, y, z), &rh, NULL, NULL);

  r = geom_vec2_x(rh);
  h = geom_vec2_y(rh);
  if (h < 0.0)
    return 0;
  if (r > pp->radius)
    return 0;

  l = geom_vec3_length(pp->rev.axis);
  if (h <= l)
    return 1;
  return 0;
}

static geom_2d_circle
geom_shape_rcc_top_circle(struct geom_shape_rcc_data *pp)
{
  return geom_2d_circle_c(pp->surface_top_center, pp->radius);
}

static geom_2d_circle
geom_shape_rcc_bot_circle(struct geom_shape_rcc_data *pp)
{
  return geom_2d_circle_c(pp->surface_bot_center, pp->radius);
}

static geom_2d_rectangle
geom_shape_rcc_side_rect(struct geom_shape_rcc_data *pp)
{
  double h;
  geom_vec2 p;
  h = geom_vec3_length(pp->rev.axis);
  p = geom_vec2_c(pp->radius * 2.0 * GEOM_M_PI, h);
  return geom_2d_rectangle_c(pp->surface_side_origin,
                             geom_vec2_add(pp->surface_side_origin, p));
}

static geom_vec3 geom_shape_rcc_x(void) { return geom_vec3_c(1.0, 0.0, 0.0); }

static geom_vec3 geom_shape_rcc_base_x(struct geom_shape_rcc_data *pp)
{
  return geom_shape_revolution_x_axis(&pp->rev, geom_shape_rcc_x());
}

static int  geom_shape_rcc_unwrapf(void *p, geom_vec3 xyz, int surfid,
                                   geom_vec2 *uv)
{
  geom_vec2 rz;
  double phi;
  struct geom_shape_rcc_data *pp;

  pp = (struct geom_shape_rcc_data *)p;
  geom_shape_revolution_cart2cyl(&pp->rev, xyz, geom_shape_rcc_x(), &rz, &phi);

  switch (surfid) {
  case GEOM_RCC_BOTTOM_SURFACE:
  case GEOM_RCC_TOP_SURFACE:
  case GEOM_RCC_SIDE_SURFACE:
    break;

  default:
  {
    double l;
    geom_2d_rectangle rect;
    geom_size_type side;

    l = geom_vec3_length(pp->rev.axis);
    rect = geom_2d_rectangle_c(geom_vec2_c(-pp->radius, 0.0),
                               geom_vec2_c(pp->radius, l));
    geom_2d_rectangle_nearest(rect, rz, &side);

    if (side == 1) {
      surfid = GEOM_RCC_BOTTOM_SURFACE;
    } else if (side == 3) {
      surfid = GEOM_RCC_TOP_SURFACE;
    } else {
      surfid = GEOM_RCC_SIDE_SURFACE;
    }
  } break;
  }

  if (!geom_shape_surface_common_is_enabled(&pp->surf, surfid)) {
    return 1;
  }

  if (surfid == GEOM_RCC_BOTTOM_SURFACE || surfid == GEOM_RCC_TOP_SURFACE) {
    geom_vec2 v1, v2;

    // v1 = geom_vec2_2d_rot(geom_vec2_c(geom_vec2_x(rz), 0.0), phi);
    v1 = geom_vec2_c(geom_vec2_x(rz) * geom_cosd(phi),
                     geom_vec2_x(rz) * geom_sind(phi));
    if (surfid == GEOM_RCC_BOTTOM_SURFACE) {
      v2 = pp->surface_bot_center;
    } else {
      v2 = pp->surface_top_center;
    }
    *uv = geom_vec2_add(v2, v1);
    return 0;

  } else {
    double l, x, z;

    GEOM_ASSERT(surfid == GEOM_RCC_SIDE_SURFACE);

    z = geom_vec2_y(rz);
    if (z < 0.0)
      z = 0.0;

    l = geom_vec3_length(pp->rev.axis);
    if (z > l)
      z = l;

    phi = geom_degree_norm(phi);
    if (phi < 0.0)
      phi += 360;

    x = geom_deg_to_rad(phi) * pp->radius;
    *uv = geom_vec2_add(pp->surface_side_origin, geom_vec2_c(x, z));
    return 0;
  }
}

static int geom_shape_rcc_wrapf(void *p, geom_vec2 uv, int surfid,
                                geom_vec3 *xyz, geom_vec3 *norm)
{
  struct geom_shape_rcc_data *pp;

  pp = (struct geom_shape_rcc_data *)p;

  if (surfid == GEOM_RCC_BOTTOM_SURFACE || surfid == GEOM_RCC_TOP_SURFACE) {
    geom_2d_circle c;
    geom_vec3 axis, xvec, yvec;
    double l;

    if (!geom_shape_surface_common_is_enabled(&pp->surf, surfid)) {
      return 1;
    }

    if (surfid == GEOM_RCC_TOP_SURFACE) {
      c = geom_shape_rcc_top_circle(pp);
    } else  {
      c = geom_shape_rcc_bot_circle(pp);
    }
    if (!geom_2d_circle_inout(uv, c)) {
      return 1;
    }

    uv = geom_vec2_sub(uv, geom_2d_circle_center(c));
    xvec = geom_shape_rcc_base_x(pp);
    xvec = geom_vec3_factor(xvec, geom_vec2_x(uv));

    axis = pp->rev.axis;
    yvec = geom_vec3_cross_prod(axis, xvec);
    l = geom_vec3_length(yvec);
    if (l > 0.0)
      l = 1.0 / l;
    yvec = geom_vec3_factor(yvec, l);
    yvec = geom_vec3_factor(yvec, geom_vec2_y(uv));

    xvec = geom_vec3_add(xvec, yvec);
    xvec = geom_vec3_add(xvec, pp->rev.origin);

    l = geom_vec3_length(axis);
    if (l > 0.0) {
      l = 1.0 / l;
    }
    if (surfid == GEOM_RCC_TOP_SURFACE) {
      xvec = geom_vec3_add(xvec, axis);
    } else {
      l *= -1.0;
    }
    *norm = geom_vec3_factor(axis, l);
    *xyz = xvec;
    return 0;

  } else if (surfid == GEOM_RCC_SIDE_SURFACE) {
    geom_2d_rectangle r;
    geom_vec3 xvec, yvec;
    double a, h, l;

    if (!geom_shape_surface_common_is_enabled(&pp->surf, surfid)) {
      return 1;
    }

    r = geom_shape_rcc_side_rect(pp);
    if (!geom_2d_rectangle_inout(uv, r)) {
      return 1;
    }

    uv = geom_vec2_sub(uv, geom_2d_rectangle_lower_left(r));
    a = geom_rad_to_deg(geom_vec2_x(uv) / pp->radius);
    h = geom_vec2_y(uv);

    xvec = geom_shape_rcc_base_x(pp);
    xvec = geom_quat_rotate_vp(pp->rev.axis, a, xvec);
    l = geom_vec3_length(pp->rev.axis);
    if (l > 0.0) {
      h = h / l;
    } else {
      h = 0.0;
    }
    yvec = geom_vec3_factor(pp->rev.axis, h);

    yvec = geom_vec3_add(geom_vec3_factor(xvec, pp->radius), yvec);
    *xyz = geom_vec3_add(yvec, pp->rev.origin);
    *norm = xvec;
    return 0;

  } else {
    int r;
    r = geom_shape_rcc_wrapf(p, uv, GEOM_RCC_BOTTOM_SURFACE, xyz, norm);
    if (!r)
      return r;

    r = geom_shape_rcc_wrapf(p, uv, GEOM_RCC_TOP_SURFACE, xyz, norm);
    if (!r)
      return r;

    r = geom_shape_rcc_wrapf(p, uv, GEOM_RCC_SIDE_SURFACE, xyz, norm);
    return r;
  }
}

static void geom_shape_rcc_set_vec2_if_enabled(geom_variant *v,
                                               struct geom_shape_rcc_data *pp,
                                               int surfid, geom_vec2 data)
{
  if (geom_shape_surface_common_is_enabled(&pp->surf, surfid)) {
    geom_variant_set_vec2(v, data);
  } else {
    geom_variant_nullify(v);
  }
}

static geom_error geom_shape_rcc_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_shape_rcc_data *pp;

  pp = (struct geom_shape_rcc_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v)
    return e;

  geom_variant_set_vec3(v, pp->rev.origin);
  geom_info_map_append(list, v, "Base point", "L", &e);

  geom_variant_set_vec3(v, pp->rev.axis);
  geom_info_map_append(list, v, "Height vector", "L", &e);

  geom_variant_set_double(v, pp->radius);
  geom_info_map_append(list, v, "Radius of cylinder", "L", &e);

  geom_shape_rcc_set_vec2_if_enabled(v, pp, GEOM_RCC_BOTTOM_SURFACE,
                                     pp->surface_bot_center);
  geom_info_map_append(list, v, "Bottom surface center in UV space", "L", &e);

  geom_shape_rcc_set_vec2_if_enabled(v, pp, GEOM_RCC_TOP_SURFACE,
                                     pp->surface_top_center);
  geom_info_map_append(list, v, "Top surface center in UV space", "L", &e);

  geom_shape_rcc_set_vec2_if_enabled(v, pp, GEOM_RCC_SIDE_SURFACE,
                                     pp->surface_side_origin);
  geom_info_map_append(list, v, "Side surface origin in UV space", "L", &e);

  geom_variant_delete(v);

  return e;
}

/* Use C99 notation for setting functions, which is highly recommended */
static geom_shape_funcs geom_shape_rcc = {
  .enum_val = GEOM_SHAPE_RCC,
  .shape_type = GEOM_SHPT_BODY,

  .c =
    {
      .allocator = geom_shape_rcc_allocator,
      .deallocator = geom_shape_rcc_deallocator,
      .set_value = geom_shape_rcc_set_value,
      .get_value = geom_shape_rcc_get_value,
      .n_params = geom_shape_rcc_n_params,
      .args_next = geom_shape_rcc_args_next,
      .args_check = geom_shape_rcc_args_check,
      .infomap_gen = geom_shape_rcc_info_map,
      .copy = geom_shape_rcc_copy,
    },

  .body_testf = geom_shape_rcc_testf,
  .body_nsurff = geom_shape_rcc_n_surff,
  .body_has_surff = geom_shape_rcc_has_surff,
  .body_unwrapf = geom_shape_rcc_unwrapf,
  .body_wrapf = geom_shape_rcc_wrapf,

  /*
   * Not implemented yet.
   */
  .body_bboxf = NULL,

  .transform_func = NULL,
};

geom_error geom_install_shape_rcc(void)
{
  return geom_install_shape_func(&geom_shape_rcc);
}
