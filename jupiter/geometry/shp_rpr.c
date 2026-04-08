
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
#include "shp_rpr.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for RPR (Right Prism) shape
 */
struct geom_shape_rpr_data
{
  struct geom_shape_extrusion_data extr;
  int n;             ///< Number of n-gonal
};

static const char *geom_shape_rpr_descs[] = {
  "Base point", "Radial vector", "Height vector",
  "Number of vertices",
};

static geom_variant_type
geom_shape_rpr_args_next(geom_args_builder *b, geom_variant *description,
                         int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);
  if (l < 0 || l > 3) {
    return GEOM_VARTYPE_NULL;
  }

  geom_variant_set_string(description, geom_shape_rpr_descs[l], 0);

  if (l == 3) {
    return GEOM_VARTYPE_INT;
  }
  return GEOM_VARTYPE_VECTOR3;
}

static geom_error
geom_shape_rpr_args_check(void *p, geom_args_builder *b, geom_size_type index,
                          const geom_variant *v, geom_variant *errinfo)
{
  geom_error e;
  geom_vec3 vec;

  if (index < 0 || index > 3) {
    return GEOM_ERR_RANGE;
  }

  e = GEOM_SUCCESS;
  if (index == 3) {
    int i;
    i = geom_variant_get_int(v, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    if (i <= 2) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Number of vertices must be >= 3", 0);
      } else {
        geom_warn("%d: Number of vertices must be >= 3", i);
      }
      return GEOM_ERR_RANGE;
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

  if (index > 0) {
    if (geom_vec3_iszero(vec)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Vector must not be zero", 0);
      } else {
        geom_warn("(%g, %g, %g): Vector must not be zero",
                  geom_vec3_x(vec), geom_vec3_y(vec), geom_vec3_z(vec));
      }
      return GEOM_ERR_RANGE;
    }
  }

  if (index == 1 || index == 2) {
    struct geom_shape_extrusion_data *extr;
    struct geom_shape_rpr_data *pp;
    const geom_variant *base, *height;

    e = GEOM_SUCCESS;
    pp = NULL;
    base = NULL;
    height = NULL;
    extr = NULL;
    if (p) {
      pp = (struct geom_shape_rpr_data *)p;
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
          base = geom_variant_list_get(lp);
          break;
        case 2:
          height = geom_variant_list_get(lp);
          break;
        }
      }
      if (!base && index == 2) {
        if (errinfo) {
          geom_variant_set_string(errinfo, "Radial vector is not set", 0);
        } else {
          geom_warn("Radial vector is not set");
        }
        /* We should allow to set height vector first. */
        return GEOM_ERR_DEPENDENCY;
      }
      if (!height && index == 1) {
        if (errinfo) {
          geom_variant_set_string(errinfo, "Height vector is not set", 0);
        } else {
          geom_warn("Height vector is not set");
        }
        return GEOM_SUCCESS;
      }
    }
    if (index == 1) {
      base = v;
    } else {
      height = v;
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

static geom_error
geom_shape_rpr_set_value(void *p, geom_size_type index,
                         const geom_variant *value)
{
  geom_error e;
  struct geom_shape_rpr_data *pp;

  pp = (struct geom_shape_rpr_data *)p;

  e = GEOM_SUCCESS;
  if (index == 0 || index == 1 || index == 2) {
    geom_vec3 v;
    v = geom_variant_get_vec3(value, &e);
    if (e != GEOM_SUCCESS)
      return e;
    switch (index) {
    case 0:
      pp->extr.origin = v;
      break;
    case 1:
      pp->extr.base = v;
      geom_shape_extrusion_adjust_base(&pp->extr);
      break;
    case 2:
      pp->extr.height = v;
      geom_shape_extrusion_adjust_base(&pp->extr);
      break;
    default:
      GEOM_UNREACHABLE();
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  }
  if (index == 3) {
    int n;
    n = geom_variant_get_int(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    pp->n = n;
    return GEOM_SUCCESS;
  }
  return GEOM_ERR_RANGE;
}

static geom_error
geom_shape_rpr_get_value(void *p, geom_size_type index,
                         geom_variant *out_variable)
{
  struct geom_shape_rpr_data *pp;
  pp = (struct geom_shape_rpr_data *)p;

  switch (index) {
  case 0:
    return geom_variant_set_vec3(out_variable, pp->extr.origin);
  case 1:
    return geom_variant_set_vec3(out_variable, pp->extr.base);
  case 2:
    return geom_variant_set_vec3(out_variable, pp->extr.height);
  case 3:
    return geom_variant_set_int(out_variable, pp->n);
  }
  return GEOM_ERR_RANGE;
}

static geom_size_type
geom_shape_rpr_n_params(void *p, geom_args_builder *b)
{
  return 4;
}

static void *
geom_shape_rpr_allocator(void)
{
  struct geom_shape_rpr_data *p;
  p = (struct geom_shape_rpr_data *)
    malloc(sizeof(struct geom_shape_rpr_data));
  if (!p) return NULL;

  p->n = 0;
  p->extr.origin = geom_vec3_c(0.0, 0.0, 0.0);
  p->extr.base   = geom_vec3_c(0.0, 0.0, 0.0);
  p->extr.height = geom_vec3_c(0.0, 0.0, 0.0);

  return p;
}

static void
geom_shape_rpr_deallocator(void *p)
{
  free(p);
}

static void *
geom_shape_rpr_copy(void *p)
{
  struct geom_shape_rpr_data *pp, *copy;

  pp = (struct geom_shape_rpr_data *)p;
  copy = (struct geom_shape_rpr_data *)geom_shape_rpr_allocator();
  if (!copy) return NULL;

  copy->extr = pp->extr;
  copy->n = pp->n;
  return copy;
}


static int
geom_shape_rpr_base_plane(double r, double t, double h, void *p)
{
  struct geom_shape_rpr_data *pp;
  double alp, bet, nangle, rd, base;
  geom_vec3 qa, qb, vt, d;
  double u, v;

  pp = (struct geom_shape_rpr_data *)p;

  base = geom_vec3_length(pp->extr.base);
  if (r > base) return 0;
  if (base == 0.0) return 0;

  nangle = 180.0 / pp->n;
  rd = base * geom_cosd(nangle);
  if (r <= rd) return 1;

  nangle = nangle * 2.0;
  alp = floor(t / nangle);
  alp = alp * nangle;
  bet = alp + nangle;

  qa = geom_quat_rotate_vp(pp->extr.height, alp, pp->extr.base);
  qb = geom_quat_rotate_vp(pp->extr.height, bet, pp->extr.base);
  d  = geom_quat_rotate_vp(pp->extr.height, t,   pp->extr.base);
  d  = geom_vec3_factor(d, r / base);
  vt = geom_vec3_split(d, qa, qb, pp->extr.height);
  u  = geom_vec3_x(vt);
  v  = geom_vec3_y(vt);
  if (u >= 0.0 && v >= 0.0 && u + v <= 1.0) return 1;
  return 0;
}

static int
geom_shape_rpr_testf(void *p, double x, double y, double z)
{
  struct geom_shape_rpr_data *pp;

  pp = (struct geom_shape_rpr_data *)p;

  return geom_shape_extrusion_testf(&pp->extr, x, y, z, NULL,
                                    geom_shape_rpr_base_plane, pp);
}

static geom_error
geom_shape_rpr_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_shape_rpr_data *pp;

  pp = (struct geom_shape_rpr_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  geom_variant_set_vec3(v, pp->extr.origin);
  geom_info_map_append(list, v, "Base point", "L", &e);

  geom_variant_set_vec3(v, pp->extr.base);
  geom_info_map_append(list, v, "Radial vector", "L", &e);

  geom_variant_set_vec3(v, pp->extr.height);
  geom_info_map_append(list, v, "Height vector", "L", &e);

  geom_variant_set_int(v, pp->n);
  geom_info_map_append(list, v, "Number of vertices", "", &e);

  geom_variant_delete(v);

  return e;
}

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_rpr = {
  .enum_val = GEOM_SHAPE_RPR,
  .shape_type = GEOM_SHPT_BODY,

  .c = {
    .allocator = geom_shape_rpr_allocator,
    .deallocator = geom_shape_rpr_deallocator,
    .set_value = geom_shape_rpr_set_value,
    .get_value = geom_shape_rpr_get_value,
    .n_params = geom_shape_rpr_n_params,
    .args_next = geom_shape_rpr_args_next,
    .args_check = geom_shape_rpr_args_check,
    .infomap_gen = geom_shape_rpr_info_map,
    .copy = geom_shape_rpr_copy,
  },

  .body_testf = geom_shape_rpr_testf,

  /*
   * Not implemented.
   */
  .body_bboxf = NULL,

  .transform_func = NULL,
};

geom_error geom_install_shape_rpr(void)
{
  return geom_install_shape_func(&geom_shape_rpr);
}
