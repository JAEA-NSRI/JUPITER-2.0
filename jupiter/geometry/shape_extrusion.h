
#ifndef JUPITER_GEOMETRY_SHAPE_EXTRUSION_H
#define JUPITER_GEOMETRY_SHAPE_EXTRUSION_H

#include <stdlib.h>

#include "common.h"
#include "defs.h"
#include "vector.h"
#include "geom_math.h"
#include "variant.h"
#include "global.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @ingroup Geometry
 *
 * @brief Data for arbitrary extruded shape (Prism, Pyramid, etc).
 *
 * Geomtery data limited to inside of height vector length.
 */
struct geom_shape_extrusion_data
{
  geom_vec3 origin;       ///< Origin point
  geom_vec3 height;       ///< Height vector
  geom_vec3 base;         ///< Base vector
};

/**
 * @brief Checks perpendicularity of base (radial) and height vectors
 * @param p Extrusion data (can be NULL)
 * @param base Variant of base vector for assigning it.
 * @param height Variant of height vector for assigning it.
 * @param angle Sets angle of two vectors if non-NULL given (unit: degrees)
 * @param errinfo Error information to set
 * @retval GEOM_SUCCESS Two vectors are *not* parallel.
 * @retval GEOM_ERR_RANGE Two vectors are parallel.
 * @retval GEOM_ERR_DEPENDENCY Any one of two vectors are not given.
 * @retval GEOM_ERR_VARTYPE Any one of two variants are not vector.
 *
 * This function sets error message if angle is less than 89.999 or
 * greater than 90.001 degrees, however still returns GEOM_SUCCESS, because
 * it is very sensitive.
 */
static inline geom_error
geom_shape_extrusion_check_perpendicularity(struct geom_shape_extrusion_data *p,
                                            const geom_variant *base,
                                            const geom_variant *height,
                                            double *angle,
                                            geom_variant *errinfo)
{
  double f;
  geom_error err;
  geom_vec3 vb, vh;

  err = GEOM_SUCCESS;
  if (base) {
    vb = geom_variant_get_vec3(base, &err);
    if (err != GEOM_SUCCESS) {
      return err;
    }
  } else {
    if (p) {
      vb = p->base;
    } else {
      return GEOM_ERR_DEPENDENCY;
    }
  }
  if (height) {
    vh = geom_variant_get_vec3(height, &err);
    if (err != GEOM_SUCCESS) {
      return err;
    }
  } else {
    if (p) {
      vh = p->height;
    } else {
      return GEOM_ERR_DEPENDENCY;
    }
  }

  f = geom_vec3_angle(vb, vh);
  if (angle) {
    *angle = f;
  }
  if (f == 0.0 || f == 180.0) {
    const char *msg;
    msg = "Height vector is parallel to radial vector";
    if (errinfo) {
      geom_variant_set_string(errinfo, msg, 0);
    } else {
      geom_warn(msg);
    }
    return GEOM_ERR_RANGE;
  }
  if (fabs(fabs(f) - 90.0) > 0.001) {
    const char *msg;
    char *buf;
    msg = "Radial vector is not perpendicular to height vector";
    if (errinfo) {
      int r;
      r = geom_asprintf(&buf, "%s: Angle %g degree", msg, f);
      if (r < 0) {
        geom_variant_set_string(errinfo, msg, 0);
      } else {
        geom_variant_set_string(errinfo, buf, 0);
        free(buf);
      }
    } else {
      geom_warn("%f: Angle %g degrees", msg, f);
    }
  }
  return GEOM_SUCCESS;
}

/**
 * @memberof geom_shape_extrusion_data
 * @param p Data to adjust
 */
static inline void
geom_shape_extrusion_adjust_base(struct geom_shape_extrusion_data *p)
{
  double f;
  geom_vec3 t, r, h;

  r = p->base;
  h = p->height;
  f = geom_vec3_project_factor(r, h);
  h = geom_vec3_factor(h, f);
  t = geom_vec3_sub(r, h);
  f = geom_vec3_length(t);
  if (f != 0.0) {
    f = geom_vec3_length(r) / f;
  }
  p->base = geom_vec3_factor(t, f);
}

/**
 * @memberof geom_shape_extrusion_data
 * @brief Calculates scale factor
 * @param factor Scale factor on upper base
 * @param h Relative height (0 is bottom base, 1 is upper base)
 *
 * @return Scale factor at height \p h.
 */
static inline double
geom_shape_extrusion_linear_scale_factor(double factor, double h)
{
  return (factor - 1.0) * h + 1.0;
}

/**
 * @memberof geom_shape_extrusion_data
 *
 * @param p Extruded data
 * @param x X-coordinate to test
 * @param y Y-coordinate to test
 * @param z Z-coordinate to test
 * @param testxy Test function in cartesian coordinate
 * @param testrt Test function in polar coordinate
 * @param data Extra data to be passed for test function
 *
 * @retval 0 outside of the shape
 * @retval non-0 inside of the shape
 *
 * If both of testxy and testrt given, testxy will be used.
 *
 * For polar coordinate, angle t is in degrees.
 *
 * For parameter h, we will pass ratio of the height vector (0 <= h <=
 * 1, 0 is base, 1 is head).
 */
static inline int
geom_shape_extrusion_testf(struct geom_shape_extrusion_data *p,
                           double x, double y, double z,
                           int (*testxy)(double x, double y, double h,
                                         void *data),
                           int (*testrt)(double r, double t, double h,
                                         void *data),
                           void *data)
{
  geom_vec3 vp, d;
  double h;

  vp = geom_vec3_c(x, y, z);
  vp = geom_vec3_sub(vp, p->origin);
  h  = geom_vec3_project_factor(vp, p->height);
  if (h < 0.0 || h > 1.0) return 0;

  d  = geom_vec3_factor(p->height, h);
  d  = geom_vec3_sub(vp, d);

  if (testxy) {
    geom_vec3 vx, vy;
    double t;

    t  = geom_vec3_project_factor(d, p->base);
    vx = geom_vec3_factor(p->base, t);
    x  = t * geom_vec3_length(p->base);
    vy = geom_vec3_sub(d, vx);
    t  = geom_vec3_project_factor(d, vy);
    y  = t * geom_vec3_length(vy);
    return testxy(x, y, h, data);
  }
  if (testrt) {
    double r, t, l;
    r = geom_vec3_length(d);
    l = geom_vec3_length(p->base);
    if (r > 0.0 && l > 0.0) {
      t = geom_vec3_inner_prod(p->base, d);
      t = t / (l * r);
      t = geom_acosd(t);
    } else {
      t = 0.0;
    }
    return testrt(r, t, h, data);
  }
  return 0;
}

JUPITER_GEOMETRY_DECL_END

#endif
