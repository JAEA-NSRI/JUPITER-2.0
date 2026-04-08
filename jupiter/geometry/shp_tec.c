
#include <stdlib.h>

#include "defs.h"
#include "common.h"
#include "geom_assert.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "vector.h"
#include "infomap.h"

#include "shape_extrusion.h"
#include "shp_tec.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for TEC (Truncated Elliptic Cone) shape
 */
struct geom_shape_tec_data
{
  struct geom_shape_extrusion_data extr;
  double b_length;   ///< Length of perpendicular vector to base
  double a;          ///< Upper base scale factor for base vector
  double b;          ///< Upper base scale factor for perp vector
};

static const char *geom_shape_tec_descs[] = {
  "Base point", "Height vector", "Bottom base vector A",
  "Length of vector B (perp. to A)", "Upper base factor for vector A",
  "Upper base factor for vector B"
};

static geom_variant_type
geom_shape_tec_args_next(geom_args_builder *b, geom_variant *description,
                         int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);
  if (l < 0 || l > 5) {
    return GEOM_VARTYPE_NULL;
  }

  geom_variant_set_string(description, geom_shape_tec_descs[l], 0);

  if (l >= 3) {
    return GEOM_VARTYPE_DOUBLE;
  }
  return GEOM_VARTYPE_VECTOR3;
}

static geom_error
geom_shape_tec_args_check(void *p, geom_args_builder *b, geom_size_type index,
                          const geom_variant *v, geom_variant *errinfo)
{
  geom_error e;
  geom_vec3 vec;
  double x;

  if (index < 0 || index > 5) {
    return GEOM_ERR_RANGE;
  }

  e = GEOM_SUCCESS;
  if (index >= 3) {
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

    if (index == 3) {
      if (x <= 0.0) {
        if (errinfo) {
          geom_variant_set_string(errinfo, "Length of vector B must be positive", 0);
        } else {
          geom_warn("%g: Length of vector B must be positive", x);
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

  if (index == 1 || index == 2) {
    struct geom_shape_extrusion_data *extr;
    struct geom_shape_tec_data *pp;
    const geom_variant *base, *height;

    if (geom_vec3_iszero(vec)) {
      int r;
      char *tmp;
      const char *ctmp;
      r = geom_asprintf(&tmp, "%s must not be zero", geom_shape_tec_descs[index]);
      if (r < 0) {
        ctmp = "Vector must not be zero";
        tmp = NULL;
      } else {
        ctmp = tmp;
      }
      if (errinfo) {
        geom_variant_set_string(errinfo, ctmp, 0);
      } else {
        geom_warn("(%g, %g, %g): %s",
                  geom_vec3_x(vec), geom_vec3_y(vec), geom_vec3_z(vec), ctmp);
      }
      free(tmp);
      return GEOM_ERR_RANGE;
    }

    e = GEOM_SUCCESS;
    pp = NULL;
    base = NULL;
    height = NULL;
    extr = NULL;
    if (p) {
      pp = (struct geom_shape_tec_data *)p;
    } else if (b) {
      geom_variant_list *lp, *head;
      const geom_variant *cv;
      geom_size_type it;

      cv = NULL;
      head = geom_args_builder_get_list(b);
      lp = geom_variant_list_next(head);
      for (it = 0; it < 3 && lp != head;
           ++it, lp = geom_variant_list_next(lp)) {
        switch (it) {
        case 1:
          height = geom_variant_list_get(lp);
          break;
        case 2:
          base = geom_variant_list_get(lp);
          break;
        }
      }
      if (!base && index == 1) {
        if (errinfo) {
          geom_variant_set_string(errinfo, "Base vector is not set", 0);
        } else {
          geom_warn("Base vector is not set");
        }
        return GEOM_SUCCESS;
      }
      if (!height && index == 2) {
        if (errinfo) {
          geom_variant_set_string(errinfo, "Height vector is not set", 0);
        } else {
          geom_warn("Height vector is not set");
        }
        return GEOM_SUCCESS;
      }
    }
    if (index == 1) {
      height = v;
    } else {
      base = v;
    }
    if (pp) {
      extr = &pp->extr;
    }

    e = geom_shape_extrusion_check_perpendicularity(extr, base, height, NULL,
                                                    errinfo);
    return e;
  }

  return GEOM_SUCCESS;
}

static geom_error geom_shape_tec_set_value(void *p, geom_size_type index,
                                           const geom_variant *value)
{
  geom_error e;
  struct geom_shape_tec_data *pp;

  pp = (struct geom_shape_tec_data *)p;

  e = GEOM_SUCCESS;
  if (index == 0 || index == 1 || index == 2) {
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
      geom_shape_extrusion_adjust_base(&pp->extr);
      break;
    case 2:
      pp->extr.base = v;
      geom_shape_extrusion_adjust_base(&pp->extr);
      break;
    default:
      GEOM_UNREACHABLE();
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  }
  if (index == 3 || index == 4 || index == 5) {
    double d;
    d = geom_variant_get_double(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    switch (index) {
    case 3:
      pp->b_length = d;
      break;
    case 4:
      pp->a = d;
      break;
    case 5:
      pp->b = d;
      break;
    default:
      GEOM_UNREACHABLE();
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  }
  return GEOM_ERR_RANGE;
}

static geom_error geom_shape_tec_get_value(void *p, geom_size_type index,
                                           geom_variant *out_variable)
{
  struct geom_shape_tec_data *pp;

  pp = (struct geom_shape_tec_data *)p;

  switch (index) {
  case 0:
    return geom_variant_set_vec3(out_variable, pp->extr.origin);
  case 1:
    return geom_variant_set_vec3(out_variable, pp->extr.height);
  case 2:
    return geom_variant_set_vec3(out_variable, pp->extr.base);
  case 3:
    return geom_variant_set_double(out_variable, pp->b_length);
  case 4:
    return geom_variant_set_double(out_variable, pp->a);
  case 5:
    return geom_variant_set_double(out_variable, pp->b);
  }
  return GEOM_ERR_RANGE;
}

static geom_size_type geom_shape_tec_n_params(void *p, geom_args_builder *b)
{
  return 6;
}

static void *
geom_shape_tec_allocator(void)
{
  struct geom_shape_tec_data *p;
  p = (struct geom_shape_tec_data *)
    malloc(sizeof(struct geom_shape_tec_data));
  if (!p) return NULL;

  p->a = 0.0;
  p->b = 0.0;
  p->b_length = 0.0;
  p->extr.origin = geom_vec3_c(0.0, 0.0, 0.0);
  p->extr.base   = geom_vec3_c(0.0, 0.0, 0.0);
  p->extr.height = geom_vec3_c(0.0, 0.0, 0.0);

  return p;
}

static void
geom_shape_tec_deallocator(void *p)
{
  free(p);
}

static void *
geom_shape_tec_copy(void *p)
{
  struct geom_shape_tec_data *pp, *copy;

  pp = (struct geom_shape_tec_data *)p;
  copy = (struct geom_shape_tec_data *)geom_shape_tec_allocator();
  if (!copy) return NULL;

  copy->extr = pp->extr;
  copy->a = pp->a;
  copy->b = pp->b;
  copy->b_length = pp->b_length;
  return copy;
}


static int
geom_shape_tec_base_plane(double x, double y, double h, void *p)
{
  struct geom_shape_tec_data *pp;
  double a, b;

  pp = (struct geom_shape_tec_data *)p;

  a = geom_shape_extrusion_linear_scale_factor(pp->a, h);
  b = geom_shape_extrusion_linear_scale_factor(pp->b, h);

  a = a * geom_vec3_length(pp->extr.base);
  b = b * pp->b_length;
  a = a * a;
  b = b * b;

  if (b * x * x + a * y * y <= a * b) return 1;
  return 0;
}

static int
geom_shape_tec_testf(void *p, double x, double y, double z)
{
  struct geom_shape_tec_data *pp;

  pp = (struct geom_shape_tec_data *)p;

  return geom_shape_extrusion_testf(&pp->extr, x, y, z,
                                    geom_shape_tec_base_plane, NULL, pp);
}

static geom_error
geom_shape_tec_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_shape_tec_data *pp;

  pp = (struct geom_shape_tec_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  geom_variant_set_vec3(v, pp->extr.origin);
  geom_info_map_append(list, v, "Base point", "L", &e);

  geom_variant_set_vec3(v, pp->extr.height);
  geom_info_map_append(list, v, "Height vector", "L", &e);

  geom_variant_set_vec3(v, pp->extr.base);
  geom_info_map_append(list, v, "Bottom base vector A", "L", &e);

  geom_variant_set_double(v, pp->b_length);
  geom_info_map_append(list, v, "Length of vector B (perp. to A)", "L", &e);

  geom_variant_set_double(v, pp->a);
  geom_info_map_append(list, v, "Upper base factor for vector A", "", &e);

  geom_variant_set_double(v, pp->b);
  geom_info_map_append(list, v, "Upper base factor for vector B", "", &e);

  geom_variant_delete(v);

  return e;
}

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_tec = {
  .enum_val = GEOM_SHAPE_TEC,
  .shape_type = GEOM_SHPT_BODY,

  .c = {
    .allocator = geom_shape_tec_allocator,
    .deallocator = geom_shape_tec_deallocator,
    .set_value = geom_shape_tec_set_value,
    .get_value = geom_shape_tec_get_value,
    .n_params = geom_shape_tec_n_params,
    .args_next = geom_shape_tec_args_next,
    .args_check = geom_shape_tec_args_check,
    .infomap_gen = geom_shape_tec_info_map,
    .copy = geom_shape_tec_copy,
  },

  .body_testf = geom_shape_tec_testf,

  /*
   * Not implemented.
   */
  .body_bboxf = NULL,

  .transform_func = NULL,
};

geom_error geom_install_shape_tec(void)
{
  return geom_install_shape_func(&geom_shape_tec);
}
