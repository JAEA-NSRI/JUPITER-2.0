#ifndef JUPITER_GEOMETRY_2D_QUAD_H
#define JUPITER_GEOMETRY_2D_QUAD_H

#include "defs.h"
#include "vector.h"

JUPITER_GEOMETRY_DECL_START

struct geom_2d_quad
{
  geom_vec2 p1;
  geom_vec2 p2;
  geom_vec2 p3;
  geom_vec2 p4;
};
typedef struct geom_2d_quad geom_2d_quad;

/**
 * @brief Create a quadrilateral
 * @param p1 Point 1
 * @param p2 Point 2
 * @param p3 Point 3
 * @param p4 Point 4
 */
static inline geom_2d_quad geom_2d_quad_c(geom_vec2 p1, geom_vec2 p2,
                                          geom_vec2 p3, geom_vec2 p4)
{
  geom_2d_quad q = {p1, p2, p3, p4};
  return q;
}

/**
 * @brief Get point 1 of quadrilateral
 */
static inline geom_vec2 geom_2d_quad_p1(geom_2d_quad quad) { return quad.p1; }

/**
 * @brief Get point 2 of quadrilateral
 */
static inline geom_vec2 geom_2d_quad_p2(geom_2d_quad quad) { return quad.p2; }

/**
 * @brief Get point 3 of quadrilateral
 */
static inline geom_vec2 geom_2d_quad_p3(geom_2d_quad quad) { return quad.p3; }

/**
 * @brief Get point 4 of quadrilateral
 */
static inline geom_vec2 geom_2d_quad_p4(geom_2d_quad quad) { return quad.p4; }

JUPITER_GEOMETRY_DECL_END

#endif
