
#ifndef JUPITER_GEOMETRY_SHAPE_REVOLUTION_H
#define JUPITER_GEOMETRY_SHAPE_REVOLUTION_H

#include "defs.h"
#include "vector.h"
#include "quat.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @ingroup Geometry
 *
 * @brief Data for arbitrary solid of revolution shape
 */
struct geom_shape_revolution_data
{
  geom_vec3 origin;   ///< Origin point
  geom_vec3 axis;   ///< Revolution axis
};

/**
 * @brief Get rotation quaternion to make axis of revolution to be (0, 0, 1)
 * @brief p Revoluted shape data
 * @return result
 */
static inline geom_quat
geom_shape_revolution_quat(struct geom_shape_revolution_data *p)
{
  return geom_vec3_get_rotation(p->axis, geom_vec3_c(0.0, 0.0, 1.0));
}

/**
 * @brief Calculate the X axis for revolution axis
 * @param p Revoluted shape data
 * @param x_axis X axis vector that height vector is z vector, (0, 0, 1).
 * @return result
 *
 * @p x_axis does not need to be perpendicular to (0, 0, 1), but must
 * not be parallel.
 *
 * @p x_axis is ok for (1, 0, 0) in most case, but you can adjust
 * them.
 */
static inline geom_vec3
geom_shape_revolution_x_axis(struct geom_shape_revolution_data *p,
                             geom_vec3 x_axis)
{
  geom_quat q;
  q = geom_quat_inv(geom_shape_revolution_quat(p));
  return geom_quat_rotate_p(q, x_axis);
}

/**
 * @brief Get vector of R and H coordinate in cylinderical coordiate
 * @param p revolution set
 * @param xyz Point to coordinate
 * @param rh   If not null, sets consists of r and h coordinate
 * @param rvec If not null, sets radial vector of xyz in cartesian coordinate
 * @param hvec If not null, sets height vector of xyz in cartesian coordinate
 *
 * This function returns
 * \$[
 * \left(\left|\mathbfit{r}\right|, \left|\mathbfit{h}\right|\right),
 * \$]
 * for @p rh.
 *
 * Splits input coordinate @p xyz (\$\mathbfit{p}\$) to the SOR (shape
 * of revolution) origin (\$\mathbfit{O}\$), radial and height
 * vector.
 * \$[
 * \mathbfit{p} - \mathbfit{O} = \mathbfit{r} + \mathbfit{h}
 * \$]
 */
static inline void
geom_shape_revolution_rhvec(const struct geom_shape_revolution_data *p,
                            geom_vec3 xyz, geom_vec2 *rh, geom_vec3 *rvec,
                            geom_vec3 *hvec)
{
  geom_vec3 vp, n, h;
  double tr, th;

  vp = geom_vec3_sub(xyz, p->origin);
  th = geom_vec3_project_factor(vp, p->axis);
  h = geom_vec3_factor(p->axis, th);
  n = geom_vec3_sub(vp, h);
  if (rh) {
    tr = geom_vec3_length(n);
    *rh = geom_vec2_c(tr, th * geom_vec3_length(p->axis));
  }
  if (hvec)
    *hvec = h;
  if (rvec)
    *rvec = n;
}

/**
 * @brief Transform a point in cartesian coordinate to cylinderical coordinate
 * @param p Revoluted shape data
 * @param x_axis X axis vector that height vector is z vector, (0, 0, 1).
 * @param rz Vector of r and z coordinate that return
 * @param phi Phi coordinate that return
 *
 * @p rz and @p phi can be NULL. If NULL, the corresponding coordinate
 * will not be computed.
 *
 * @note @p phi returns angle in degrees (not radian).
 *
 * @p x_axis value is not used when @p phi is NULL.
 *
 * @p x_axis does not need to be perpendicular to (0, 0, 1), but must
 * not be parallel.
 *
 * @p x_axis is ok for (1, 0, 0) in most case, but you can adjust
 * them.
 */
static inline void
geom_shape_revolution_cart2cyl(struct geom_shape_revolution_data *p,
                               geom_vec3 xyz, geom_vec3 x_axis, geom_vec2 *rz,
                               double *phi)
{
  if (phi) {
    double f, phi_a;
    geom_vec3 rv;
    geom_shape_revolution_rhvec(p, xyz, rz, &rv, NULL);
    x_axis = geom_shape_revolution_x_axis(p, x_axis);
    f = geom_vec3_project_factor(x_axis, p->axis);
    x_axis = geom_vec3_sub(x_axis, geom_vec3_factor(p->axis, f));
    phi_a = geom_vec3_angle(x_axis, rv);
    if (geom_vec3_inner_prod(geom_vec3_cross_prod(x_axis, rv), p->axis) < 0.0) {
      phi_a = -phi_a;
    }
    *phi = phi_a;
  } else {
    geom_shape_revolution_rhvec(p, xyz, rz, NULL, NULL);
  }
}

/**
 * @memberof geom_shape_revolution_data
 * @brief Template function to test solids of revolution data.
 * @param p Revolution data
 * @param x X position to test
 * @param y Y position to test
 * @param z Z position to test
 * @param test Tester function delegate
 * @param data Data passing for function test.
 *
 * @return Test result.
 *
 * Provided 2D vector for \p test will be radial position for X
 * component and height position for Y. Since revolution data does not
 * define base direction, angle won't be calculated. Consider use
 * geom_shape_extrusion_data instead (it supports polar coordinate).
 *
 * This function is remained for historical reason and is
 * deprecated. Recommend to use `geom_shape_revolution_rhvec()` to get
 * `rh` vector that is passed to @p test function.
 */
GEOM_DEPRECATED("consider replace with geom_shape_revolution_rhvec")
static inline int
geom_shape_revolution_testf(struct geom_shape_revolution_data *p, double x,
                            double y, double z,
                            int (*test)(geom_vec2 p, void *data), void *data)
{
  geom_vec2 rh;
  geom_shape_revolution_rhvec(p, geom_vec3_c(x, y, z), &rh, NULL, NULL);
  return test(rh, data);
}

JUPITER_GEOMETRY_DECL_END

#endif
