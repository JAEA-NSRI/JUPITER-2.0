
#include <stdlib.h>

#include "defs.h"
#include "common.h"
#include "geom_assert.h"
#include "func_defs.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "vector.h"
#include "infomap.h"

#include "shp_app.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for APP (Arbitrary Parallelepiped) shape
 */
struct geom_shape_app_data
{
  geom_vec3 basep;   ///< Base point
  geom_vec3 v1;      ///< Vector 1
  geom_vec3 v2;      ///< Vector 2
  geom_vec3 v3;      ///< Vector 3
};

static const char *geom_shape_app_descs[] = {
  "Base point", "Vector 1", "Vector 2", "Vector 3",
};

static geom_variant_type
geom_shape_app_args_next(geom_args_builder *b, geom_variant *description,
                         int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);
  if (l < 0 || l > 3) {
    return GEOM_VARTYPE_NULL;
  }

  geom_variant_set_string(description, geom_shape_app_descs[l], 0);
  return GEOM_VARTYPE_VECTOR3;
}

static geom_error
geom_shape_app_args_check(void *p, geom_args_builder *b, geom_size_type index,
                          const geom_variant *v, geom_variant *errinfo)
{
  geom_error e;
  geom_vec3 vec;

  if (index < 0 || index > 3) {
    return GEOM_ERR_RANGE;
  }

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
    int i;
    geom_vec3 other_vec[2];
    int other_vec_present[2];
    geom_error err;

    if (geom_vec3_iszero(vec)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Zero vector given for an edge", 0);
      } else {
        geom_warn("(%g, %g, %g): Zero vector given for an edge",
                  geom_vec3_x(vec), geom_vec3_y(vec), geom_vec3_z(vec));
      }
      return GEOM_ERR_RANGE;
    }

    other_vec_present[0] = 0;
    other_vec_present[1] = 0;
    other_vec[0] = geom_vec3_c(0.0, 0.0, 0.0);
    other_vec[1] = geom_vec3_c(0.0, 0.0, 0.0);

    err = GEOM_SUCCESS;
    if (p) {
      struct geom_shape_app_data *pp;
      pp = (struct geom_shape_app_data *)p;

      switch (index) {
      case 1:
        other_vec[0] = pp->v2;
        other_vec[1] = pp->v3;
        other_vec_present[0] = 2;
        other_vec_present[1] = 3;
        break;
      case 2:
        other_vec[0] = pp->v1;
        other_vec[1] = pp->v3;
        other_vec_present[0] = 1;
        other_vec_present[1] = 3;
        break;
      case 3:
        other_vec[0] = pp->v1;
        other_vec[1] = pp->v2;
        other_vec_present[0] = 1;
        other_vec_present[1] = 2;
        break;
      default:
        GEOM_UNREACHABLE();
        return GEOM_ERR_RANGE;
      }
    } else if (b) {
      geom_size_type j;
      geom_size_type t;
      geom_variant_list *lp, *head;

      head = geom_args_builder_get_list(b);
      lp = geom_variant_list_next(head);
      if (lp != head) {
        lp = geom_variant_list_next(lp);
      }

      t = 0;
      for (j = 1; j < 4 && lp != head; ++j, lp = geom_variant_list_next(lp)) {
        const geom_variant *var;
        geom_vec3 v;

        if (j == index) {
          continue;
        }

        t++;
        var = geom_variant_list_get(lp);
        if (!var) {
          continue;
        }
        v = geom_variant_get_vec3(var, &err);
        if (err != GEOM_SUCCESS) {
          err = GEOM_SUCCESS;
          continue;
        }

        other_vec[t - 1] = v;
        other_vec_present[t - 1] = j;
      }
    }
    for (i = 0; i < 2; ++i) {
      geom_vec3 vp;

      if (other_vec_present[i] < 1) {
        continue;
      }

      vp = geom_vec3_cross_prod(other_vec[i], vec);
      if (geom_vec3_inner_prod(vp, vp) == 0.0) {
        int r;
        char *tmp;
        const char *ctmp;
        r = geom_asprintf(&tmp, "%s and %s are parallel",
                          geom_shape_app_descs[other_vec_present[i]],
                          geom_shape_app_descs[index]);
        if (r < 0) {
          tmp = NULL;
          ctmp = "Two vectors are parallel";
        } else {
          ctmp = tmp;
        }
        if (errinfo) {
          geom_variant_set_string(errinfo, ctmp, 0);
        } else {
          geom_warn("(%g, %g, %g): %s",
                    geom_vec3_x(vec), geom_vec3_y(vec), geom_vec3_z(vec),
                    ctmp);
        }
        free(tmp);
        return GEOM_ERR_RANGE;
      }
    }
  }

  return GEOM_SUCCESS;
}

static geom_error
geom_shape_app_set_value(void *p, geom_size_type index,
                         const geom_variant *value)
{
  geom_error err;
  geom_vec3 v;
  struct geom_shape_app_data *pp;
  pp = (struct geom_shape_app_data *)p;

  if (index < 0 || index > 3) {
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
    break;
  case 1:
    pp->v1 = v;
    break;
  case 2:
    pp->v2 = v;
    break;
  case 3:
    pp->v3 = v;
    break;
  default:
    GEOM_UNREACHABLE();
    return GEOM_ERR_RANGE;
  }
  return GEOM_SUCCESS;
}

static geom_error
geom_shape_app_get_value(void *p, geom_size_type index,
                         geom_variant *out_variable)
{
  struct geom_shape_app_data *pp;
  pp = (struct geom_shape_app_data *)p;

  switch (index) {
  case 0:
    return geom_variant_set_vec3(out_variable, pp->basep);
  case 1:
    return geom_variant_set_vec3(out_variable, pp->v1);
  case 2:
    return geom_variant_set_vec3(out_variable, pp->v2);
  case 3:
    return geom_variant_set_vec3(out_variable, pp->v3);
  default:
    return GEOM_ERR_RANGE;
  }
}

static geom_size_type
geom_shape_app_n_params(void *p, geom_args_builder *b)
{
  return 4;
}

static void *
geom_shape_app_allocator(void)
{
  struct geom_shape_app_data *p;
  p = (struct geom_shape_app_data *)
    malloc(sizeof(struct geom_shape_app_data));
  if (!p) return NULL;

  p->v1 = geom_vec3_c(0.0, 0.0, 0.0);
  p->v2 = geom_vec3_c(0.0, 0.0, 0.0);
  p->v3 = geom_vec3_c(0.0, 0.0, 0.0);

  return p;
}

static void
geom_shape_app_deallocator(void *p)
{
  free(p);
}

static void *
geom_shape_app_copy(void *p)
{
  struct geom_shape_app_data *pp, *copy;

  pp = (struct geom_shape_app_data *)p;
  copy = (struct geom_shape_app_data *)geom_shape_app_allocator();
  if (!copy) return NULL;

  copy->basep = pp->basep;
  copy->v1 = pp->v1;
  copy->v2 = pp->v2;
  copy->v3 = pp->v3;
  return copy;
}

static int
geom_shape_app_testf(void *p, double x, double y, double z)
{
  struct geom_shape_app_data *pp;
  geom_vec3 vp;

  pp = (struct geom_shape_app_data *)p;

  vp = geom_vec3_c(x, y, z);
  vp = geom_vec3_sub(vp, pp->basep);
  vp = geom_vec3_split(vp, pp->v1, pp->v2, pp->v3);
  x  = geom_vec3_x(vp);
  y  = geom_vec3_y(vp);
  z  = geom_vec3_z(vp);
  if (0.0 <= x && x <= 1.0 &&
      0.0 <= y && y <= 1.0 &&
      0.0 <= z && z <= 1.0) {
    return 1;
  }
  return 0;
}

static geom_error
geom_shape_app_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_shape_app_data *pp;

  pp = (struct geom_shape_app_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  geom_variant_set_vec3(v, pp->basep);
  geom_info_map_append(list, v, "Base point", "L", &e);

  geom_variant_set_vec3(v, pp->v1);
  geom_info_map_append(list, v, "Vector 1", "L", &e);

  geom_variant_set_vec3(v, pp->v2);
  geom_info_map_append(list, v, "Vector 2", "L", &e);

  geom_variant_set_vec3(v, pp->v3);
  geom_info_map_append(list, v, "Vector 3", "L", &e);

  geom_variant_delete(v);

  return e;
}

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_app = {
  .enum_val = GEOM_SHAPE_APP,
  .shape_type = GEOM_SHPT_BODY,

  .c = {
    .allocator = geom_shape_app_allocator,
    .deallocator = geom_shape_app_deallocator,
    .set_value = geom_shape_app_set_value,
    .get_value = geom_shape_app_get_value,
    .n_params = geom_shape_app_n_params,
    .args_next = geom_shape_app_args_next,
    .args_check = geom_shape_app_args_check,
    .infomap_gen = geom_shape_app_info_map,
    .copy = geom_shape_app_copy,
  },

  .body_testf = geom_shape_app_testf,

  /*
   * Not implemented.
   */
  .body_bboxf = NULL,

  .transform_func = NULL,
};

geom_error geom_install_shape_app(void)
{
  return geom_install_shape_func(&geom_shape_app);
}
