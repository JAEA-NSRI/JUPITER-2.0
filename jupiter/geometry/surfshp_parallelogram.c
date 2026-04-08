
#include <stdlib.h>

/*
 * All geometry definitions
 */
#include "common.h"
#include "defs.h"

/*
 * Include global.h to implement install function and
 * to call warning functions
 */
#include "geom_assert.h"
#include "global.h"

/*
 * Include func_data.h to set content of geom_init_funcs data.
 */
#include "func_data.h"

/*
 * Include abuilder.h to use geom_args_builder functions.
 */
#include "abuilder.h"

/*
 * Include variant.h to set/get geom_variant / geom_variant_list values.
 */
#include "variant.h"

/*
 * Include to use vector calculation.
 *
 * For matrix calculation, use mat22.h, mat33.h or mat43.h.
 */
#include "mat22.h"
#include "vector.h"

/*
 * Include to use infomap functions.
 */
#include "infomap.h"

/*
 * For conversion to path
 */
#include "2d_path.h"
#include "2d_path_element.h"

#include "surfshp_parallelogram.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for PARALLELOGRAM shape
 */
struct geom_surface_shape_parallelogram_data
{
  geom_vec2 basep;  ///< Base point
  geom_vec2 v1;     ///< Vector for an edge
  geom_vec2 v2;     ///< Vector for another edge
};

/*
 * Mandatory functions are
 *  - Argument type request response function (.c.args_next)
 *
 * Mandatory functions for defining surface shape are
 *  - In-body test function (.body_testf)
 *  - Copy function (.c.copy)
 *    (Copy function will be used when a shape is copied by
 *     transformation element is set to generate tranformed copies, so
 *     some cases will work without copy function, but mandatory)
 *    (Since surface shapes do not support transformation and so
 *     copy function will never be used, but future functionality,
 *     treat as mandatory)
 *
 * Optional but definitely required functions are
 *  - Allocator of function specific data (.c.allocator)
 *  - Deallocator of function specific data (.c.deallocator)
 *  - Initialization function for function specific data (.c.init_set)
 *
 * Optional functions which is used for improving the user experience only:
 *  - Argument value checker (.c.args_check)
 *  - Parameter description generator (.c.infomap_gen)
 *
 * Optional functions for defining surface shape are
 *  - Boundary box function (.body_bboxf)
 *  - To-path converter (.to_pathf)
 *    (Currently unused, required for exact area calculation)
 *
 * See if_const.c for implementing directions for common functions
 * (.c.*)
 */

static geom_variant_type geom_surface_shape_parallelogram_args_next(
  geom_args_builder *b, geom_variant *description, int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);

  switch (l) {
  case 0:
    geom_variant_set_string(description, "Base point", 0);
    return GEOM_VARTYPE_VECTOR2;
  case 1:
    geom_variant_set_string(description, "Vector 1", 0);
    return GEOM_VARTYPE_VECTOR2;
  case 2:
    geom_variant_set_string(description, "Vector 2", 0);
    return GEOM_VARTYPE_VECTOR2;
  }
  return GEOM_VARTYPE_NULL;
}

static geom_error geom_surface_shape_parallelogram_args_check(
  void *p, geom_args_builder *b, geom_size_type index, const geom_variant *v,
  geom_variant *errinfo)
{
  geom_error e;
  geom_vec2 pnt;

  if (index < 0 || index > 2) {
    return GEOM_ERR_RANGE;
  }

  e = GEOM_SUCCESS;
  pnt = geom_variant_get_vec2(v, &e);
  if (e != GEOM_SUCCESS) {
    return e;
  }
  if (!geom_vec2_isfinite(pnt)) {
    if (errinfo) {
      geom_variant_set_string(errinfo, "Value must be finite", 0);
    } else {
      geom_warn("(%g, %g): %s values should be finite",
                geom_vec2_x(pnt), geom_vec2_y(pnt),
                (index == 0) ? "Point" : "Vector");
    }
    return GEOM_ERR_RANGE;
  }

  if (index > 0) {
    int i = 0;
    geom_vec2 other_vec;
    int other_vec_present;
    geom_error err;

    if (geom_vec2_iszero(pnt)) {
      if (errinfo) {
        geom_variant_set_string(errinfo, "Zero vector is given for an edge", 0);
      } else {
        geom_warn("(%g, %g): Zero vector given for an edge",
                  geom_vec2_x(pnt), geom_vec2_y(pnt));
      }
      return GEOM_ERR_RANGE;
    }

    other_vec_present = 0;
    other_vec = geom_vec2_c(0.0, 0.0);

    err = GEOM_SUCCESS;
    if (p) {
      struct geom_surface_shape_parallelogram_data *pp;
      pp = (struct geom_surface_shape_parallelogram_data *)p;

      switch(index) {
      case 1:
        other_vec = pp->v2;
        other_vec_present = 1;
        break;
      case 2:
        other_vec = pp->v1;
        other_vec_present = 1;
        break;
      default:
        GEOM_UNREACHABLE();
        return GEOM_ERR_RANGE;
      }
    } else if (b) {
      geom_size_type j, t;
      geom_variant_list *lp, *head;

      head = geom_args_builder_get_list(b);
      lp = geom_variant_list_next(head);

      j = 2 - (index - 1); // 1: 2 - (1 - 1) -> 2, 2: 3 - (2 - 1) -> 1
      t = j;
      for (; j > 0 && lp != head; --j) {
        lp = geom_variant_list_next(lp);
      }

      if (lp != head) {
        const geom_variant *var;
        var = geom_variant_list_get(lp);

        if (var) {
          other_vec = geom_variant_get_vec2(var, &err);
          if (err == GEOM_SUCCESS) {
            other_vec_present = 1;
          }
        }
      }
    }

    if (other_vec_present) {
      geom_vec3 vp;
      vp = geom_vec3_cross_prod(geom_vec3_c_v2z(other_vec, 0.0),
                                geom_vec3_c_v2z(pnt, 0.0));
      if (geom_vec3_inner_prod(vp, vp) == 0.0) {
        if (errinfo) {
          geom_variant_set_string(errinfo, "Two vectors are parallel", 0);
        } else {
          geom_warn("(%g, %g) and (%g, %g): Two vectors are parallel",
                    geom_vec2_x(pnt), geom_vec2_y(pnt), //
                    geom_vec2_x(other_vec), geom_vec2_y(other_vec));
        }
        return GEOM_ERR_RANGE;
      }
    }
  }

  return e;
}

static geom_error
geom_surface_shape_parallelogram_set_value(void *p, geom_size_type index,
                                          const geom_variant *value)
{
  geom_error err;
  geom_vec2 v;
  struct geom_surface_shape_parallelogram_data *pp;
  pp = (struct geom_surface_shape_parallelogram_data *)p;

  if (index < 0 || index > 2) {
    return GEOM_ERR_RANGE;
  }

  err = GEOM_SUCCESS;
  v = geom_variant_get_vec2(value, &err);
  if (err != GEOM_SUCCESS) {
    return err;
  }

  switch (index) {
  case 0:
    pp->basep = v;
    return GEOM_SUCCESS;
  case 1:
    pp->v1 = v;
    return GEOM_SUCCESS;
  case 2:
    pp->v2 = v;
    return GEOM_SUCCESS;
  }
  GEOM_UNREACHABLE();
  return GEOM_ERR_RANGE;
}

static geom_error
geom_surface_shape_parallelogram_get_value(void *p, geom_size_type index,
                                           geom_variant *out_variable)
{
  struct geom_surface_shape_parallelogram_data *pp;
  pp = (struct geom_surface_shape_parallelogram_data *)p;

  switch (index) {
  case 0:
    return geom_variant_set_vec2(out_variable, pp->basep);
  case 1:
    return geom_variant_set_vec2(out_variable, pp->v1);
  case 2:
    return geom_variant_set_vec2(out_variable, pp->v2);
  }
  return GEOM_ERR_RANGE;
}

static geom_size_type
geom_surface_shape_parallelogram_n_params(void *p, geom_args_builder *b)
{
  return 3;
}

static void *
geom_surface_shape_parallelogram_allocator(void)
{
  struct geom_surface_shape_parallelogram_data *p;
  p = (struct geom_surface_shape_parallelogram_data *)
    malloc(sizeof(struct geom_surface_shape_parallelogram_data));
  if (!p) return NULL;

  p->basep = geom_vec2_c(0.0, 0.0);
  p->v1 = geom_vec2_c(0.0, 0.0);
  p->v2 = geom_vec2_c(0.0, 0.0);

  return p;
}

static void
geom_surface_shape_parallelogram_deallocator(void *p)
{
  free(p);
}

static void *
geom_surface_shape_parallelogram_copy(void *p)
{
  struct geom_surface_shape_parallelogram_data *pp, *copy;

  pp = (struct geom_surface_shape_parallelogram_data *)p;
  copy = (struct geom_surface_shape_parallelogram_data *)
    geom_surface_shape_parallelogram_allocator();
  if (!copy) return NULL;

  copy->basep = pp->basep;
  copy->v1 = pp->v1;
  copy->v2 = pp->v2;
  return copy;
}

static int
geom_surface_shape_parallelogram_testf(void *p, double x, double y)
{
  geom_vec2 v;
  struct geom_surface_shape_parallelogram_data *pp;

  pp = (struct geom_surface_shape_parallelogram_data *)p;
  v = geom_vec2_sub(geom_vec2_c(x, y), pp->basep);
  v = geom_vec2_split(v, pp->v1, pp->v2);

  x = geom_vec2_x(v);
  y = geom_vec2_y(v);
  if (0.0 <= x && x <= 1.0 && 0.0 <= y && y <= 1.0) {
    return 1;
  }
  return 0;
}

static void geom_surface_shape_parallelogram_bboxf(void *p, geom_vec2 *start,
                                                   geom_vec2 *end)
{
  geom_vec2 a, b, min, max;
  double ax, ay, bx, by;
  struct geom_surface_shape_parallelogram_data *pp;
  pp = (struct geom_surface_shape_parallelogram_data *)p;

  a = pp->v1;
  b = pp->v2;
  ax = geom_vec2_x(a);
  ay = geom_vec2_y(a);
  bx = geom_vec2_x(b);
  by = geom_vec2_y(b);
  if (ax * bx < 0.0) {
    /* a_x < (a + b)_x < b_x or b_x < (a + b)_x < a_x */
    if (ax < bx) {
      min = geom_vec2_c(ax, 0.0);
      max = geom_vec2_c(bx, 0.0);
    } else {
      min = geom_vec2_c(bx, 0.0);
      max = geom_vec2_c(ax, 0.0);
    }
  } else {
    /* (a + b)_x < (a_x, b_x) < 0 or 0 < (a_x, b_x) < (a + b)_x */
    if (ax < 0.0) {
      min = geom_vec2_c(ax + bx, 0.0);
      max = geom_vec2_c(0.0, 0.0);
    } else {
      min = geom_vec2_c(0.0, 0.0);
      max = geom_vec2_c(ax + bx, 0.0);
    }
  }

  if (ay * by < 0.0) {
    /* Same as X-axis */
    if (ay < by) {
      min = geom_vec2_c(geom_vec2_x(min), ay);
      max = geom_vec2_c(geom_vec2_x(max), by);
    } else {
      min = geom_vec2_c(geom_vec2_x(min), by);
      max = geom_vec2_c(geom_vec2_x(max), ay);
    }
  } else {
    if (ay < 0.0) {
      min = geom_vec2_c(geom_vec2_x(min), ay + by);
      max = geom_vec2_c(geom_vec2_x(max), 0.0);
    } else {
      min = geom_vec2_c(geom_vec2_x(min), 0.0);
      max = geom_vec2_c(geom_vec2_x(max), ay + by);
    }
  }

  *start = geom_vec2_add(pp->basep, min);
  *end = geom_vec2_add(pp->basep, max);
}

static geom_error
geom_surface_shape_parallelogram_to_path(void *p, geom_2d_path_data **path)
{
  geom_2d_path_data *path_data;
  geom_2d_path_element elements[5];
  struct geom_surface_shape_parallelogram_data *pp;

  pp = (struct geom_surface_shape_parallelogram_data *)p;
  path_data = *path;
  path_data = geom_2d_path_data_resize(path_data, 5);
  if (!path)
    return GEOM_ERR_NOMEM;

  *path = path_data;

  geom_2d_path_set_absmove_element(&elements[0], pp->basep);
  geom_2d_path_set_relline_element(&elements[1], pp->v1);
  geom_2d_path_set_relline_element(&elements[2], pp->v2);
  geom_2d_path_set_relline_element(&elements[3],
                                   geom_vec2_factor(pp->v1, -1.0));
  geom_2d_path_set_end_element(&elements[4]);
  if (geom_2d_path_from_elements(path_data, elements, 5) == 5)
    return GEOM_SUCCESS;
  return GEOM_ERR_RANGE;
}

static geom_error
geom_surface_shape_parallelogram_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_surface_shape_parallelogram_data *pp;

  pp = (struct geom_surface_shape_parallelogram_data *)p;

  e = GEOM_SUCCESS;

  v = geom_variant_new(&e);
  if (!v) return e;

  /*
   * Note: geometry library does not define the unit of length
   *
   * Use 'L' for unit of length. The user can replace this with
   * actual unit of length, such as 'm', 'in', 'mm', etc.
   *
   * In case of area, the unit will be "L^2", and "L^3" for volume.
   *
   * Degree angle should be "deg" to be ASCII safe.
   */
  geom_variant_set_vec2(v, pp->basep);
  geom_info_map_append(list, v, "Base Point", "L", &e);

  geom_variant_set_vec2(v, pp->v1);
  geom_info_map_append(list, v, "Vector 1", "L", &e);

  geom_variant_set_vec2(v, pp->v2);
  geom_info_map_append(list, v, "Vector 2", "L", &e);

  geom_variant_delete(v);

  return e;
}

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_surface_shape_funcs geom_surface_shape_parallelogram = {
  /* Set corresponding enum value of shape to .enum_val */
  .enum_val = GEOM_SURFACE_SHAPE_PARALLELOGRAM,

  /* Set shape type:
   *   For defining body, use GEOM_SHPT_BODY.
   *   GEOM_SHPT_TRANS is not supported yet.
   */
  .shape_type = GEOM_SHPT_BODY,

  /* Set common functions to init and shape function are set to .c member */
  .c = {
    /* See if_none.c and if_const.c for further documentation. */
    .allocator = geom_surface_shape_parallelogram_allocator,
    .deallocator = geom_surface_shape_parallelogram_deallocator,
    .set_value = geom_surface_shape_parallelogram_set_value,
    .get_value = geom_surface_shape_parallelogram_get_value,
    .n_params = geom_surface_shape_parallelogram_n_params,
    .args_next = geom_surface_shape_parallelogram_args_next,
    .args_check = geom_surface_shape_parallelogram_args_check,
    .infomap_gen = geom_surface_shape_parallelogram_info_map,

    /* For body definition, copy is required. */
    .copy = geom_surface_shape_parallelogram_copy,
  },

  /* For body definition, set in-body test function */
  .body_testf = geom_surface_shape_parallelogram_testf,

  /*
   * For body definition, you can set boundary box function if it is
   * explicitly exist. This function only has effect to improve
   * performance. If wrongly defined, you will mess up the result.
   */
  .body_bboxf = geom_surface_shape_parallelogram_bboxf,

  .to_pathf = geom_surface_shape_parallelogram_to_path,
};

geom_error geom_install_surface_shape_parallelogram(void)
{
  return geom_install_surface_shape_func(&geom_surface_shape_parallelogram);
}
