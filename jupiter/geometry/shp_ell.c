
#include <stdlib.h>

#include "defs.h"
#include "geom_assert.h"
#include "global.h"
#include "common.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "vector.h"
#include "mat33.h"
#include "geom_math.h"
#include "infomap.h"

#include "shp_ell.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for ELL (ellipsoid) shape
 */
struct geom_shape_ell_data
{
  geom_vec3 basep;     ///< Center point
  geom_vec3 vector[3]; ///< Three vectors
};

static const char *geom_shape_ell_descs[] = {
  "Center point", "Vector 1", "Vector 2", "Vector 3",
};

static geom_variant_type
geom_shape_ell_args_next(geom_args_builder *b, geom_variant *description,
                         int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);
  if (l < 0 || l > 3) return GEOM_VARTYPE_NULL;

  geom_variant_set_string(description, geom_shape_ell_descs[l], 0);
  return GEOM_VARTYPE_VECTOR3;
}

static geom_error
geom_shape_ell_args_check(void *p, geom_args_builder *b, geom_size_type index,
                          const geom_variant *v, geom_variant *errinfo)
{
  geom_error e;
  geom_vec3 vec;

  if (index < 0 || index > 3) return GEOM_ERR_RANGE;

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

  if (index > 0) {
    geom_vec3 v[3];
    int has_v[3];
    int i;

    if (geom_vec3_iszero(vec)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Vector must not be zero", 0);
      } else {
        geom_warn("(%g, %g, %g): Vector must not be zero",
                  geom_vec3_x(vec), geom_vec3_y(vec), geom_vec3_z(vec));
      }
      return GEOM_ERR_RANGE;
    }

    for (i = 0; i < 3; ++i) {
      has_v[i] = 0;
    }
    has_v[index - 1] = 1;
    v[index - 1] = vec;

    if (p) {
      struct geom_shape_ell_data *pp;
      pp = (struct geom_shape_ell_data *)p;

      for (i = 0; i < 3; ++i) {
        if (!has_v[i]) {
          has_v[i] = 1;
          v[i] = pp->vector[i];
        }
      }
    } else if (b) {
      geom_variant_list *lp;
      geom_variant_list *head;
      geom_size_type it;

      head = geom_args_builder_get_list(b);
      lp = geom_variant_list_next(head);
      for (it = 0; it < 4 && lp != head; lp = geom_variant_list_next(lp), ++it) {
        if (it > 0 && !has_v[it - 1]) {
          geom_error err;
          const geom_variant *cv;
          err = GEOM_SUCCESS;
          cv = geom_variant_list_get(lp);
          if (cv) {
            v[it - 1] = geom_variant_get_vec3(cv, &err);
            if (err == GEOM_SUCCESS) {
              has_v[it - 1] = 1;
            }
          }
        }
      }
    }

    for (i = 0; i < 3; ++i) {
      geom_vec3 vr;
      double f;
      char *tmp;
      const char *ctmp;
      int r;

      if (i == index - 1) {
        continue;
      }

      if (!has_v[i]) {
        continue;
      }

      vr = v[i];
      if (geom_vec3_eql(vr, geom_vec3_c(0.0, 0.0, 0.0))) {
        continue;
      }

      f = 0.0;
      if (!geom_vec3_eql(vec, vr)) {
        f = geom_vec3_project_factor(vec, vr);
        vr = geom_vec3_factor(vr, f);
        vr = geom_vec3_sub(vec, vr);
        f = geom_vec3_inner_prod(vr, vr);
      }
      if (f == 0.0) {
        r = geom_asprintf(&tmp, "%s and %s are parallel",
                          geom_shape_ell_descs[i + 1],
                          geom_shape_ell_descs[index]);
      if (r < 0) {
          ctmp = "Two vectors are parallel";
        } else {
          ctmp = tmp;
        }
        if (errinfo) {
          geom_variant_set_string(errinfo, ctmp, 0);
        } else {
          geom_warn("%s", ctmp);
        }
        if (r >= 0) {
          free(tmp);
        }
        return GEOM_ERR_RANGE;
      }
    }
  }
  return GEOM_SUCCESS;
}

static geom_error
geom_shape_ell_set_value(void *p, geom_size_type index,
                         const geom_variant *value)
{
  geom_error e;
  struct geom_shape_ell_data *pp;

  pp = (struct geom_shape_ell_data *)p;

  e = GEOM_SUCCESS;
  if (index >= 0 && index <= 4) {
    geom_vec3 v;
    v = geom_variant_get_vec3(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    switch (index) {
    case 0:
      pp->basep = v;
      break;
    case 1:
    case 2:
    case 3:
      pp->vector[index - 1] = v;
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
geom_shape_ell_get_value(void *p, geom_size_type index,
                         geom_variant *out_variant)
{
  struct geom_shape_ell_data *pp;

  pp = (struct geom_shape_ell_data *)p;

  switch (index) {
  case 0:
    return geom_variant_set_vec3(out_variant, pp->basep);
  case 1:
  case 2:
  case 3:
    return geom_variant_set_vec3(out_variant, pp->vector[index - 1]);
  }
  return GEOM_ERR_RANGE;
}

static geom_size_type
geom_shape_ell_n_params(void *p, geom_args_builder *b)
{
  return 4;
}

static void *
geom_shape_ell_allocator(void)
{
  struct geom_shape_ell_data *p;
  int i;

  p = (struct geom_shape_ell_data *)
    malloc(sizeof(struct geom_shape_ell_data));
  if (!p) return NULL;

  p->basep = geom_vec3_c(0.0, 0.0, 0.0);
  for (i = 0; i < 3; ++i) {
    p->vector[i] = geom_vec3_c(0.0, 0.0, 0.0);
  }

  return p;
}

static void
geom_shape_ell_deallocator(void *p)
{
  free(p);
}

static void *
geom_shape_ell_copy(void *p)
{
  int i;
  struct geom_shape_ell_data *pp, *copy;

  pp = (struct geom_shape_ell_data *)p;
  copy = (struct geom_shape_ell_data *)geom_shape_ell_allocator();
  if (!copy) return NULL;

  copy->basep = pp->basep;
  for (i = 0; i < 3; ++i) {
    copy->vector[i] = pp->vector[i];
  }
  return copy;
}

static int
geom_shape_ell_testf(void *p, double x, double y, double z)
{
  struct geom_shape_ell_data *pp;
  geom_vec3 vp;
  geom_mat33 a, b, c, d;
  double det1, det2, det3, det4;

  pp = (struct geom_shape_ell_data *)p;
  vp = geom_vec3_c(x, y, z);
  vp = geom_vec3_sub(vp, pp->basep);
  a  = geom_mat33_c_cv(vp, pp->vector[1], pp->vector[2]);
  b  = geom_mat33_c_cv(pp->vector[0], vp, pp->vector[2]);
  c  = geom_mat33_c_cv(pp->vector[0], pp->vector[1], vp);
  d  = geom_mat33_c_cv(pp->vector[0], pp->vector[1], pp->vector[2]);
  det1 = geom_mat33_det(a);
  det2 = geom_mat33_det(b);
  det3 = geom_mat33_det(c);
  det4 = geom_mat33_det(d);
  det1 = det1 * det1 + det2 * det2 + det3 * det3 - det4 * det4;
  if (det1 <= 0.0) return 1;
  return 0;
}

static geom_error
geom_shape_ell_info_map(void *p, geom_info_map *list)
{
  int i;
  geom_variant *v;
  geom_error e;
  struct geom_shape_ell_data *pp;

  pp = (struct geom_shape_ell_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  geom_variant_set_vec3(v, pp->basep);
  geom_info_map_append(list, v, geom_shape_ell_descs[0], "L", &e);

  for (i = 0; i < 3; ++i) {
    geom_variant_set_vec3(v, pp->vector[i]);
    geom_info_map_append(list, v, geom_shape_ell_descs[i + 1], "L", &e);
  }

  geom_variant_delete(v);

  return e;
}

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_ell = {
  .enum_val = GEOM_SHAPE_ELL,
  .shape_type = GEOM_SHPT_BODY,

  .c = {
    .allocator = geom_shape_ell_allocator,
    .deallocator = geom_shape_ell_deallocator,
    .set_value = geom_shape_ell_set_value,
    .get_value = geom_shape_ell_get_value,
    .n_params = geom_shape_ell_n_params,
    .args_next = geom_shape_ell_args_next,
    .args_check = geom_shape_ell_args_check,
    .infomap_gen = geom_shape_ell_info_map,
    .copy = geom_shape_ell_copy,
  },

  .body_testf = geom_shape_ell_testf,

  /*
   * It's difficult to compute bounding box for ELL, so it's not
   * calculated yet.
   */
  .body_bboxf = NULL,

  .transform_func = NULL,
};

geom_error geom_install_shape_ell(void)
{
  return geom_install_shape_func(&geom_shape_ell);
}
