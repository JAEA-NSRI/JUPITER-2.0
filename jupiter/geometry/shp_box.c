
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
#include "vector.h"

/*
 * Include to use infomap functions.
 */
#include "infomap.h"

#include "shp_box.h"

/* shape specific data */
/**
 * @ingroup Geometry
 * @brief Data for BOX shape
 */
struct geom_shape_box_data
{
  geom_vec3 startp;   ///< start point
  geom_vec3 endp;     ///< end point
};

/*
 * Mandatory functions are
 *  - Argument type request response function (.c.args_next)
 *
 * Mandatory functions for defining body are
 *  - In-body test function (.body_testf)
 *  - Copy function (.c.copy)
 *    (Copy function will be used when a shape is copied by
 *     transformation element is set to generate tranformed copies, so
 *     some cases will work without copy function, but mandatory)
 *
 * Mandatory functions for defining body with supporting surface boundaries are
 *  - Unwrap UV (aka. XYZ to (nearest) UV conversion) function (.body_unwrapf)
 *  - Wrap UV (aka. UV to XYZ conversion) function (.body_wrapf)
 *
 * Mandatory functions for defining transformation are
 *  - Transformation function (.transform_func)
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
 * Optional functions for defining body are
 *  - Boundary box function (.body_bboxf)
 *
 * Optional functions for defining body with supporting surface boundaries are
 *  - Number of surfaces in body
 *    (If this function is not defined, library treats it as 1)
 *
 *  - Surface path extractor in UV surface
 *    (This function is for calculating the exact area of the
 *     surface. However, the functionality is not implemented yet and
 *     will never be used. You can skip implementing this function not
 *     to support exact area calculation, for example, if the mapped
 *     shape does not represent the correct area.)
 *
 * See if_const.c for implementing directions for common functions
 * (.c.*)
 */

static geom_variant_type
geom_shape_box_args_next(geom_args_builder *b, geom_variant *description,
                         int *optional)
{
  geom_size_type l;

  l = geom_args_builder_get_loc(b);

  switch (l) {
  case 0:
    geom_variant_set_string(description, "Minimum position", 0);
    return GEOM_VARTYPE_VECTOR3;
  case 1:
    geom_variant_set_string(description, "Maximum position", 0);
    return GEOM_VARTYPE_VECTOR3;
  }
  return GEOM_VARTYPE_NULL;
}

static geom_error
geom_shape_box_args_check(void *p, geom_args_builder *b, geom_size_type index,
                          const geom_variant *v, geom_variant *errinfo)
{
  geom_error e;
  geom_vec3 pnt;
  geom_vec3 opnt;
  int has_opnt;

  if (index < 0 || index > 2) {
    return GEOM_ERR_RANGE;
  }

  e = GEOM_SUCCESS;
  pnt = geom_variant_get_vec3(v, &e);
  if (e != GEOM_SUCCESS) {
    return e;
  }
  if (!geom_vec3_isfinite(pnt)) {
    if (errinfo) {
      geom_variant_set_string(errinfo, "Value must be finite", 0);
    } else {
      geom_warn("(%g, %g, %g): Point values should be finite",
                geom_vec3_x(pnt), geom_vec3_y(pnt), geom_vec3_z(pnt));
    }
    return GEOM_ERR_RANGE;
  }

  has_opnt = 0;
  if (p) {
    struct geom_shape_box_data *pp;
    pp = (struct geom_shape_box_data *)p;
    switch (index) {
    case 0:
      opnt = pp->endp;
      has_opnt = 1;
      break;
    case 1:
      opnt = pp->startp;
      has_opnt = 1;
      break;
    default:
      GEOM_UNREACHABLE();
      return GEOM_ERR_RANGE;
    }
  } else if (b) {
    const geom_variant *lv;
    geom_error err;

    GEOM_ASSERT(index >= 0 && index <= 1);

    err  = GEOM_SUCCESS;
    lv = geom_args_builder_value_at(b, 1 - index);
    if (lv) {
      opnt = geom_variant_get_vec3(lv, &err);
      if (err == GEOM_SUCCESS) {
        has_opnt = 1;
      }
    }
  }

  e = GEOM_SUCCESS;
  if (has_opnt) {
    double minp[3];
    double maxp[3];
    int ieql[3];
    int ineg[3];
    int ic[2];
    int i;
    if (index == 0) {
      minp[0] = geom_vec3_x(pnt);
      minp[1] = geom_vec3_y(pnt);
      minp[2] = geom_vec3_z(pnt);
      maxp[0] = geom_vec3_x(opnt);
      maxp[1] = geom_vec3_y(opnt);
      maxp[2] = geom_vec3_z(opnt);
    } else {
      minp[0] = geom_vec3_x(opnt);
      minp[1] = geom_vec3_y(opnt);
      minp[2] = geom_vec3_z(opnt);
      maxp[0] = geom_vec3_x(pnt);
      maxp[1] = geom_vec3_y(pnt);
      maxp[2] = geom_vec3_z(pnt);
    }
    for (i = 0; i < 3; ++i) {
      ieql[i] = 0;
      ineg[i] = 0;
    }
    ic[0] = 0;
    ic[1] = 0;
    for (i = 0; i < 3; ++i) {
      if (minp[i] == maxp[i]) {
        ieql[ic[0]++] = i;
      } else if (minp[i] > maxp[i]) {
        ineg[ic[1]++] = i;
      }
    }
    if (ic[0] > 0 || ic[1] > 0) {
      const char *axes[3];
      int r;
      char *buf;
      const char *cmsg;
      int *iflg;

      axes[0] = "X";
      axes[1] = "Y";
      axes[2] = "Z";
      r = -1;

      e = GEOM_SUCCESS;
      if (ic[1] > 0) {
        i = 1;
        e = GEOM_ERR_RANGE;
      } else {
        i = 0;
      }

      if (i == 0) {
        cmsg = "Max position is equal to min";
        iflg = ieql;
      } else {
        cmsg = "Max position is greater than min";
        iflg = ineg;
      }

      switch(ic[i]) {
      case 1:
        r = geom_asprintf(&buf, "%s on axis %s", cmsg, axes[iflg[0]]);
        break;
      case 2:
        r = geom_asprintf(&buf, "%s on axes %s and %s",
                          cmsg, axes[iflg[0]], axes[iflg[1]]);
        break;
      case 3:
        r = geom_asprintf(&buf, "%s on axes %s, %s and %s",
                          cmsg, axes[iflg[0]], axes[iflg[1]], axes[iflg[2]]);
        break;
      }

      if (r < 0) {
        if (errinfo) {
          geom_variant_set_string(errinfo, cmsg, 0);
        } else {
          geom_warn("%s on some axes", cmsg);
        }
      } else {
        if (errinfo) {
          geom_variant_set_string(errinfo, buf, 0);
        } else {
          geom_warn("(%g, %g, %g): %s",
                    geom_vec3_x(pnt), geom_vec3_y(pnt), geom_vec3_z(pnt), buf);
        }
        free(buf);
      }
    }
  }

  return e;
}

static geom_error geom_shape_box_set_value(void *p, geom_size_type index,
                                           const geom_variant *value)
{
  geom_error err;
  geom_vec3 v;
  struct geom_shape_box_data *pp;
  pp = (struct geom_shape_box_data *)p;

  if (index < 0 || index > 1) {
    return GEOM_ERR_RANGE;
  }

  err = GEOM_SUCCESS;
  v = geom_variant_get_vec3(value, &err);
  if (err != GEOM_SUCCESS) {
    return err;
  }

  switch (index) {
  case 0:
    pp->startp = v;
    return GEOM_SUCCESS;
  case 1:
    pp->endp = v;
    return GEOM_SUCCESS;
  }
  GEOM_UNREACHABLE();
  return GEOM_ERR_RANGE;
}

static geom_error geom_shape_box_get_value(void *p, geom_size_type index,
                                           geom_variant *out_variable)
{
  struct geom_shape_box_data *pp;
  pp = (struct geom_shape_box_data *)p;

  switch (index) {
  case 0:
    return geom_variant_set_vec3(out_variable, pp->startp);
  case 1:
    return geom_variant_set_vec3(out_variable, pp->endp);
  }
  return GEOM_ERR_RANGE;
}

static geom_size_type geom_shape_box_n_params(void *p, geom_args_builder *b)
{
  return 2;
}

static void *
geom_shape_box_allocator(void)
{
  struct geom_shape_box_data *p;
  p = (struct geom_shape_box_data *)
    malloc(sizeof(struct geom_shape_box_data));
  if (!p) return NULL;

  p->startp = geom_vec3_c(0.0, 0.0, 0.0);
  p->endp   = geom_vec3_c(0.0, 0.0, 0.0);

  return p;
}

static void
geom_shape_box_deallocator(void *p)
{
  free(p);
}

static void *
geom_shape_box_copy(void *p)
{
  struct geom_shape_box_data *pp, *copy;

  pp = (struct geom_shape_box_data *)p;
  copy = (struct geom_shape_box_data *)geom_shape_box_allocator();
  if (!copy) return NULL;

  copy->startp = pp->startp;
  copy->endp = pp->endp;
  return copy;
}

static int
geom_shape_box_testf(void *p, double x, double y, double z)
{
  struct geom_shape_box_data *pp;
  pp = (struct geom_shape_box_data *)p;

  if (geom_vec3_x(pp->startp) <= x && x <= geom_vec3_x(pp->endp) &&
      geom_vec3_y(pp->startp) <= y && y <= geom_vec3_y(pp->endp) &&
      geom_vec3_z(pp->startp) <= z && z <= geom_vec3_z(pp->endp)) {
    return 1;
  }
  return 0;
}

/*
 * Example implemention of boundary box.
 * For box shape, boundary box is equivalent to shape itself,
 * so not defined.
 */
/*
 * static void
 * geom_shape_box_body_bboxf(void *p, geom_vec3 *start, geom_vec3 *end)
 * {
 *   struct geom_shape_box_data *pp;
 *   pp = (struct geom_shape_box_data *)p;
 *   *start = pp->startp;
 *   *end   = pp->endp;
 * }
 */

static geom_error
geom_shape_box_info_map(void *p, geom_info_map *list)
{
  geom_variant *v;
  geom_error e;
  struct geom_shape_box_data *pp;

  pp = (struct geom_shape_box_data *)p;

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
  geom_variant_set_vec3(v, pp->startp);
  geom_info_map_append(list, v, "Start position", "L", &e);

  geom_variant_set_vec3(v, pp->endp);
  geom_info_map_append(list, v, "End position", "L", &e);

  geom_variant_delete(v);

  return e;
}

/* Use C99 notation for setting functions, which is highly recommended */
static
geom_shape_funcs geom_shape_box = {
  /* Set corresponding enum value of shape to .enum_val */
  .enum_val = GEOM_SHAPE_BOX,

  /* Set shape type:
   *   For defining body, use GEOM_SHPT_BODY.
   *   For defining transformation, use GEOM_SHPT_TRANS.
   */
  .shape_type = GEOM_SHPT_BODY,

  /* Set common functions to init and shape function are set to .c member */
  .c = {
    /* See if_none.c and if_const.c for further documentation. */
    .allocator = geom_shape_box_allocator,
    .deallocator = geom_shape_box_deallocator,
    .set_value = geom_shape_box_set_value,
    .get_value = geom_shape_box_get_value,
    .n_params = geom_shape_box_n_params,
    .args_next = geom_shape_box_args_next,
    .args_check = geom_shape_box_args_check,
    .infomap_gen = geom_shape_box_info_map,

    /* For body definition, copy is required. */
    .copy = geom_shape_box_copy,
  },

  /* For body definition, set in-body test function */
  .body_testf = geom_shape_box_testf,

  /*
   * For body definition, you can set boundary box function if it is
   * explicitly exist. This function only has effect to improve
   * performance. If wrongly defined, you will mess up the result.
   */
  .body_bboxf = NULL,

  /* For transformation, set transformation function */
  .transform_func = NULL,
};

geom_error geom_install_shape_box(void)
{
  return geom_install_shape_func(&geom_shape_box);
}
