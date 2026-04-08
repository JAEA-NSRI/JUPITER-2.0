
#include <stdlib.h>

#include "defs.h"
#include "geom_assert.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "mat22.h"
#include "vector.h"
#include "infomap.h"

#include "shape_revolution.h"
#include "shp_eto.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for ETO (Elliptical Torus) shape
 */
struct geom_shape_eto_data
{
  struct geom_shape_revolution_data rev;
  double major_radius;     ///< Distance from the center of the tube to the center of the torus.
  geom_vec2 minor_a;       ///< Surface vector a for the tube
  geom_vec2 minor_b;       ///< Surface vector b for the tube
};


static geom_variant_type
geom_shape_eto_args_next(geom_args_builder *b, geom_variant *description,
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
    geom_variant_set_string(description, "Tube vector A", 0);
    return GEOM_VARTYPE_VECTOR2;
  case 4:
    geom_variant_set_string(description, "Tube vector B", 0);
    return GEOM_VARTYPE_VECTOR2;
  }
  return GEOM_VARTYPE_NULL;
}

static geom_error
geom_shape_eto_args_check(void *p, geom_args_builder *b, geom_size_type index,
                          const geom_variant *v, geom_variant *errinfo)
{
  geom_error e;
  geom_vec3 vec;
  geom_vec2 v2;
  double x;

  if (index < 0 || index > 4) {
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
  if (index == 2) {
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
  if (index == 3 || index == 4) {
    geom_vec2 other_v;
    int has_other_v;

    e = GEOM_SUCCESS;
    v2 = geom_variant_get_vec2(v, &e);
    if (e != GEOM_SUCCESS) return e;

    if (!geom_vec2_isfinite(v2)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "All elements must be finite", 0);
      } else {
        geom_warn("(%g, %g): Value should be finite",
                  geom_vec2_x(v2), geom_vec2_y(v2));
      }
      return GEOM_ERR_RANGE;
    }

    if (geom_vec2_iszero(v2)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Tube vectors must not be zero", 0);
      } else {
        geom_warn("(%g, %g): Tube vectors must not be zero",
                  geom_vec2_x(v2), geom_vec2_y(v2));
      }
      return GEOM_ERR_RANGE;
    }

    has_other_v = 0;
    if (p) {
      struct geom_shape_eto_data *pp;
      pp = (struct geom_shape_eto_data *)p;
      switch (index) {
      case 3:
        other_v = pp->minor_b;
        has_other_v = 1;
        break;
      case 4:
        other_v = pp->minor_a;
        has_other_v = 1;
        break;
      }
    } else if (b) {
      geom_size_type it;
      geom_variant_list *lp, *head;
      const geom_variant *cv;

      cv = NULL;
      head = geom_args_builder_get_list(b);
      lp = geom_variant_list_next(head);
      for (it = 0; it < 5 && lp != head; ++it, lp = geom_variant_list_next(lp)) {
        if (it != index && (it == 3 || it == 4)) {
          cv = geom_variant_list_get(lp);
          break;
        }
      }
      if (cv) {
        geom_error err;
        err = GEOM_SUCCESS;
        other_v = geom_variant_get_vec2(cv, &err);
        if (err == GEOM_SUCCESS) {
          has_other_v = 1;
        }
      }
    }

    if (has_other_v) {
      double f;
      f = 0.0;
      if (!geom_vec2_eql(other_v, v2)) {
        f = geom_vec2_project_factor(other_v, v2);
        v2 = geom_vec2_factor(v2, f);
        v2 = geom_vec2_sub(other_v, v2);
        f = geom_vec2_inner_prod(v2, v2);
      }
      if (f == 0.0) {
        if (errinfo) {
          geom_variant_set_string(errinfo, "Two tube vectors are in parallel", 0);
        } else {
          geom_warn("Two tube vectors are in parallel");
        }
        return GEOM_ERR_RANGE;
      }
    }

    return GEOM_SUCCESS;
  }


  return GEOM_ERR_RANGE;
}

static geom_error geom_shape_eto_set_value(void *p, geom_size_type index,
                                           const geom_variant *value)
{
  geom_error e;
  struct geom_shape_eto_data *pp;

  pp = (struct geom_shape_eto_data *)p;

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
  if (index == 2) {
    double r;
    r = geom_variant_get_double(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    pp->major_radius = r;
    return GEOM_SUCCESS;
  }
  if (index == 3 || index == 4) {
    geom_vec2 v;
    v = geom_variant_get_vec2(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    switch (index) {
    case 3:
      pp->minor_a = v;
      break;
    case 4:
      pp->minor_b = v;
      break;
    default:
      GEOM_UNREACHABLE();
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  }
  return GEOM_ERR_RANGE;
}

static geom_error geom_shape_eto_get_value(void *p, geom_size_type index,
                                           geom_variant *out_variable)
{
  struct geom_shape_eto_data *pp;

  pp = (struct geom_shape_eto_data *)p;

  switch (index) {
  case 0:
    return geom_variant_set_vec3(out_variable, pp->rev.origin);
  case 1:
    return geom_variant_set_vec3(out_variable, pp->rev.axis);
  case 2:
    return geom_variant_set_double(out_variable, pp->major_radius);
  case 3:
    return geom_variant_set_vec2(out_variable, pp->minor_a);
  case 4:
    return geom_variant_set_vec2(out_variable, pp->minor_b);
  }
  return GEOM_ERR_RANGE;
}

static geom_size_type geom_shape_eto_n_params(void *p, geom_args_builder *b)
{
  return 5;
}

static void *
geom_shape_eto_allocator(void)
{
  struct geom_shape_eto_data *p;
  p = (struct geom_shape_eto_data *)
    malloc(sizeof(struct geom_shape_eto_data));
  if (!p) return NULL;

  p->rev.origin = geom_vec3_c(0.0, 0.0, 0.0);
  p->rev.axis = geom_vec3_c(0.0, 0.0, 0.0);
  p->major_radius = 0.0;
  p->minor_a = geom_vec2_c(0.0, 0.0);
  p->minor_b = geom_vec2_c(0.0, 0.0);

  return p;
}

static void
geom_shape_eto_deallocator(void *p)
{
  free(p);
}

static void *
geom_shape_eto_copy(void *p)
{
  struct geom_shape_eto_data *pp, *copy;

  pp = (struct geom_shape_eto_data *)p;
  copy = (struct geom_shape_eto_data *)geom_shape_eto_allocator();
  if (!copy) return NULL;

  copy->rev = pp->rev;
  copy->major_radius = pp->major_radius;
  copy->minor_a = pp->minor_a;
  copy->minor_b = pp->minor_b;
  return copy;
}

static int
geom_shape_eto_testf_plain(geom_vec2 p, void *data)
{
  struct geom_shape_eto_data *pp;
  geom_vec2 c;
  geom_mat22 m1, m2, m3;
  double a1, a2, a3;

  pp = (struct geom_shape_eto_data *)data;

  c = geom_vec2_c(pp->major_radius, 0.0);
  p = geom_vec2_sub(p, c);

  m1 = geom_mat22_c_cv(p, pp->minor_b);
  m2 = geom_mat22_c_cv(pp->minor_a, p);
  m3 = geom_mat22_c_cv(pp->minor_a, pp->minor_b);
  a1 = geom_mat22_det(m1);
  a2 = geom_mat22_det(m2);
  a3 = geom_mat22_det(m3);

  if (a1 * a1 + a2 * a2 <= a3 * a3) return 1;
  return 0;
}

static int
geom_shape_eto_testf(void *p, double x, double y, double z)
{
  struct geom_shape_eto_data *pp;

  pp = (struct geom_shape_eto_data *)p;

  return geom_shape_revolution_testf(&pp->rev, x, y, z,
                                     geom_shape_eto_testf_plain, p);
}

static geom_error
geom_shape_eto_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_shape_eto_data *pp;

  pp = (struct geom_shape_eto_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  geom_variant_set_vec3(v, pp->rev.origin);
  geom_info_map_append(list, v, "Base point", "L", &e);

  geom_variant_set_vec3(v, pp->rev.axis);
  geom_info_map_append(list, v, "Normal vector", "", &e);

  geom_variant_set_double(v, pp->major_radius);
  geom_info_map_append(list, v, "Major Radius of torus", "L", &e);

  geom_variant_set_vec2(v, pp->minor_a);
  geom_info_map_append(list, v, "Tube vector A", "L", &e);

  geom_variant_set_vec2(v, pp->minor_b);
  geom_info_map_append(list, v, "Tube vector B", "L", &e);

  geom_variant_delete(v);

  return e;
}

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_eto = {
  .enum_val = GEOM_SHAPE_ETO,
  .shape_type = GEOM_SHPT_BODY,

  .c = {
    .allocator = geom_shape_eto_allocator,
    .deallocator = geom_shape_eto_deallocator,
    .set_value = geom_shape_eto_set_value,
    .get_value = geom_shape_eto_get_value,
    .n_params = geom_shape_eto_n_params,
    .args_next = geom_shape_eto_args_next,
    .args_check = geom_shape_eto_args_check,
    .infomap_gen = geom_shape_eto_info_map,
    .copy = geom_shape_eto_copy,
  },

  .body_testf = geom_shape_eto_testf,

  /*
   * Not implemented yet.
   */
  .body_bboxf = NULL,

  .transform_func = NULL,
};

geom_error geom_install_shape_eto(void)
{
  return geom_install_shape_func(&geom_shape_eto);
}
