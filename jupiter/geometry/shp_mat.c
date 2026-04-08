
#include <stdlib.h>

#include "defs.h"
#include "common.h"
#include "global.h"
#include "func_data.h"
#include "abuilder.h"
#include "variant.h"
#include "vector.h"
#include "mat43.h"
#include "infomap.h"

#include "shp_mat.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for MAT transformation (Specify matrix directly)
 */
struct geom_shape_mat_data
{
  geom_mat43 mat;     ///< Matrix
};

static geom_variant_type
geom_shape_mat_args_next(geom_args_builder *b, geom_variant *description,
                         int *optinoal)
{
  char *buf;
  geom_size_type l;
  int r;
  int x, y;

  l = geom_args_builder_get_loc(b);
  if (l < 0 || l > 11) return GEOM_VARTYPE_NULL;

  x = l % 12;
  y = x / 4;
  x = x % 4;
  r = geom_asprintf(&buf, "Matrix element (%d,%d)", x + 1, y + 1);
  if (r < 0) {
    geom_variant_set_string(description, "Matrix element", 0);
  } else {
    geom_variant_set_string(description, buf, 0);
    free(buf);
  }
  return GEOM_VARTYPE_DOUBLE;
}

static geom_error
geom_shape_mat_args_check(void *p, geom_args_builder *b, geom_size_type index,
                          const geom_variant *v, geom_variant *errinfo)
{
  geom_error e;
  geom_variant_type t;
  double x;

  e = GEOM_SUCCESS;

  t = geom_variant_get_type(v);
  if (index == 0 && t == GEOM_VARTYPE_MATRIX43) {
    geom_mat43 m43;
    m43 = geom_variant_get_mat43(v, &e);
    if (e != GEOM_SUCCESS)
      return e;
    if (geom_mat43_det(m43) == 0.0) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Given matrix is singular", 0);
      }
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  } else if (index >= 0 && index < 12) {
    /* singularity is not checked here */
    x = geom_variant_get_double(v, &e);
    if (e != GEOM_SUCCESS)
      return e;
    if (!isfinite(x)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Value must be finite", 0);
      } else {
        geom_warn("%g: Value should be finite", x);
      }
      return GEOM_ERR_RANGE;
    }
    return GEOM_SUCCESS;
  }
  return GEOM_ERR_RANGE;
}

static geom_error geom_shape_mat_set_value(void *p, geom_size_type index,
                                           const geom_variant *value)
{
  geom_error e;
  struct geom_shape_mat_data *pp;
  geom_variant_type t;

  pp = (struct geom_shape_mat_data *)p;
  e = GEOM_SUCCESS;

  t = geom_variant_get_type(value);
  if (index == 0 && t == GEOM_VARTYPE_MATRIX43) {
    geom_mat43 m = geom_variant_get_mat43(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    pp->mat = m;
    return GEOM_SUCCESS;
  } else if (index >= 0 && index < 12) {
    double d;
    int i, j;
    d = geom_variant_get_double(value, &e);
    if (e != GEOM_SUCCESS) {
      return e;
    }
    i = index % 12;
    j = i % 4 + 1;
    i = i / 4 + 1;
    geom_mat43_set(&pp->mat, i, j, d); /* see geom_mat43_set() */
    return GEOM_SUCCESS;
  }
  return GEOM_ERR_RANGE;
}

static geom_error geom_shape_mat_get_value(void *p, geom_size_type index,
                                           geom_variant *out_variable)
{
  struct geom_shape_mat_data *pp;

  pp = (struct geom_shape_mat_data *)p;
  if (index >= 0 && index < 12) {
    int i, j;
    i = index % 12;
    j = i % 4;
    i = i / 4;
    return geom_variant_set_double(out_variable,
                                   *geom_mat43_addr(&pp->mat, i, j));
  }
  return GEOM_ERR_RANGE;
}

static geom_size_type geom_shape_mat_n_params(void *p, geom_args_builder *b)
{
  return 12;
}

static void *
geom_shape_mat_allocator(void)
{
  struct geom_shape_mat_data *p;
  p = (struct geom_shape_mat_data *)
    malloc(sizeof(struct geom_shape_mat_data));
  if (!p) return NULL;

  p->mat = geom_mat43_E();

  return p;
}

static void
geom_shape_mat_deallocator(void *p)
{
  free(p);
}

static geom_error
geom_shape_mat_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_shape_mat_data *pp;

  pp = (struct geom_shape_mat_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  geom_variant_set_mat43(v, pp->mat);
  geom_info_map_append(list, v, "Transformation Matrix", "", &e);

  geom_variant_delete(v);

  return e;
}

static geom_mat43
geom_shape_mat_func(void *p)
{
  struct geom_shape_mat_data *pp;

  pp = (struct geom_shape_mat_data *)p;

  return pp->mat;
}

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_mat = {
  .enum_val = GEOM_SHAPE_MAT,
  .shape_type = GEOM_SHPT_TRANS,

  .c = {
    .allocator = geom_shape_mat_allocator,
    .deallocator = geom_shape_mat_deallocator,
    .set_value = geom_shape_mat_set_value,
    .get_value = geom_shape_mat_get_value,
    .n_params = geom_shape_mat_n_params,
    .args_next = geom_shape_mat_args_next,
    .args_check = geom_shape_mat_args_check,
    .infomap_gen = geom_shape_mat_info_map,
    .copy = NULL,
  },

  .body_testf = NULL,
  .body_bboxf = NULL,

  /* For transformation, set transformation function */
  .transform_func = geom_shape_mat_func,
};

geom_error geom_install_shape_mat(void)
{
  return geom_install_shape_func(&geom_shape_mat);
}
