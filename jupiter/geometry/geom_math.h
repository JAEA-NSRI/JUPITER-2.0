/**
 * @file geom_math.h
 * @ingroup Geometry
 * @brief Extra math function used by geometry library
 */

#ifndef JUPITER_GEOMETRY_GEOM_MATH_H
#define JUPITER_GEOMETRY_GEOM_MATH_H

#include <math.h>

#include "defs.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @ingroup Geometry
 * @brief sign function
 * @param value value to calculate
 * @retval   0.0 if value ==  0.0
 * @retval  -0.0 if value == -0.0
 * @retval   1.0 if value  >  0.0
 * @retval  -1.0 if value  <  0.0 (not including -0.0)
 * @retval value if value is NAN
 *
 * @sa https://stackoverflow.com/a/4609795
 */
static inline double
geom_sign(double value)
{
#ifdef isnan
  if (isnan(value)) return value;
#endif
  if (value == 0.0) return value;
  return (0.0 < value) - (value < 0.0);
}

/**
 * @ingroup Geometry
 * @brief Normalize degree angles into -180 < angle <= 180.
 * @param angle Angles to be normalized in degrees
 * @return normalized angles in degrees
 *
 * If the absolute value is greater than 1e+15, returns 0.0.
 */
static inline double
geom_degree_norm(double angle)
{
  if (angle <= 180.0) {
    if (angle > -180.0) return angle;
    if (angle > -540.0) return angle + 360.0;
  } else {
    if (angle <= 540.0) return angle - 360.0;
  }
  if (fabs(angle) > 1.0e+15) return 0.0;
  angle -= floor((angle + 180.0) / 360.0) * 360.0;
  return angle;
}

/**
 * @brief Pi
 *
 * pi value for `long double` type defined in the glibc.
 */
#ifdef M_PI
#define GEOM_M_PI      M_PI
#else
#define GEOM_M_PI      3.141592653589793238462643383279502884
#endif
#define GEOM_M_SQRT2   1.414213562373095048801688724209698078
#define GEOM_M_SQRT3   1.732050807568877293527446341505872367
#define GEOM_M_SQRT2_2 0.707106781186547524400844362104849039
#define GEOM_M_SQRT3_2 0.866025403784438646763723170752936183
#define GEOM_M_SIN15   0.258819045102520762348898837624048328
#define GEOM_M_COS15   0.965925826289068286749743199728897367

/**
 * @ingroup Geometry
 * @brief Convert degrees to radian
 */
static inline double
geom_deg_to_rad(double angle)
{
  return angle / 180.0 * GEOM_M_PI;
}

/**
 * @ingroup Geometry
 * @brief Convert radian to degrees
 */
static inline double
geom_rad_to_deg(double angle)
{
  return angle / GEOM_M_PI * 180.0;
}

/**
 * @ingroup Geometry
 * @brief Trigonometric sine in degrees
 * @param angle angle in degrees.
 * @return result.
 */
static inline double
geom_sind(double angle)
{
  if (angle ==    0.0) return  0.0;
  if (angle ==   30.0) return  0.5;
  if (angle ==   90.0) return  1.0;
  if (angle ==  180.0) return  0.0;
  if (angle ==  150.0) return  0.5;
  if (angle ==  -30.0) return -0.5;
  if (angle ==  -90.0) return -1.0;
  if (angle == -150.0) return -0.5;
  if (angle == -180.0) return  0.0;
  return sin(geom_deg_to_rad(angle));
}

/**
 * @ingroup Geometry
 * @brief Trigonometric cosine in degrees
 * @param angle angle in degrees.
 * @return result.
 */
static inline double
geom_cosd(double angle)
{
  if (angle ==    0.0) return  1.0;
  if (angle ==   60.0) return  0.5;
  if (angle ==   90.0) return  0.0;
  if (angle ==  180.0) return -1.0;
  if (angle ==  120.0) return -0.5;
  if (angle ==  -60.0) return  0.5;
  if (angle ==  -90.0) return  0.0;
  if (angle == -120.0) return -0.5;
  if (angle == -180.0) return -1.0;
  return cos(geom_deg_to_rad(angle));
}

/**
 * @ingroup Geometry
 * @brief Trigonometric tangent in degrees
 * @param angle angle in degrees.
 * @return result.
 */
static inline double
geom_tand(double angle)
{
  if (angle ==    0.0) return  0.0;
  if (angle ==   45.0) return  1.0;
  if (angle ==   90.0) return  HUGE_VAL;
  if (angle ==  180.0) return  0.0;
  if (angle ==  135.0) return -1.0;
  if (angle ==  -45.0) return -1.0;
  if (angle ==  -90.0) return -HUGE_VAL;
  if (angle == -135.0) return  1.0;
  if (angle == -180.0) return  0.0;
  return tan(geom_deg_to_rad(angle));
}

/**
 * @ingroup Geometry
 * @brief Trigonometric arc sine in degrees
 * @param value value to calculate
 * @return result.
 */
static inline double
geom_asind(double value)
{
  if (value ==  0.0) return    0.0;
  if (value ==  0.5) return   30.0;
  if (value ==  1.0) return   90.0;
  if (value == -0.5) return  -30.0;
  if (value == -1.0) return  -90.0;
  return geom_rad_to_deg(asin(value));
}

/**
 * @ingroup Geometry
 * @brief Trigonometric arc cosine in degrees
 * @param value value to calculate
 * @return result.
 */
static inline double
geom_acosd(double value)
{
  if (value ==  1.0) return    0.0;
  if (value ==  0.5) return   60.0;
  if (value ==  0.0) return   90.0;
  if (value == -0.5) return  120.0;
  if (value == -1.0) return  180.0;
  return geom_rad_to_deg(acos(value));
}

/**
 * @ingroup Geometry
 * @brief Trigonometric arc tangent in degrees
 * @param value value to calculate
 * @return result.
 */
static inline double
geom_atand(double value)
{
  if (value ==  0.0) return    0.0;
  if (value ==  1.0) return   45.0;
  if (value == -1.0) return  -45.0;
  if (value >=  1.0e+16) return  90.0;
  if (value <= -1.0e+16) return -90.0;
  return geom_rad_to_deg(acos(value));
}

/**
 * @ingroup Geometry
 * @brief Trigonometric arc tangent in degrees
 * @param x X value
 * @param y Y value
 * @return degrees from x-axis, positive direction
 */
static inline double
geom_atan2d(double y, double x)
{
  double rad;

  if (y != 0.0 && fabs(y) == fabs(x)) { /* includes inf. */
    if (y > 0.0) {
      if (x > 0.0) {
        return 45.0;
      }
      return 135.0;
    }
    if (x > 0.0) {
      return -45.0;
    }
    return -135.0;
  }
  rad = atan2(y, x);
  /*
   * signess of zeros are ignored at comparison.
   *
   * (so both `rad == +0.0` and `rad == -0.0` are filter out by next
   *  `if`, to keep signess of `rad`)
   */
  if (rad == 0.0) return rad;
  if (y == 0.0) {
    if (x == 0.0) return geom_sign(rad) * 90.0;
    return geom_sign(rad) * 180.0;
  }
  return geom_rad_to_deg(rad);
}

JUPITER_GEOMETRY_DECL_END

#endif
