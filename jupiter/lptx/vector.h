/**
 * @file lptx/vector.h
 */

#ifndef JUPITER_LPTX_VECTOR_H
#define JUPITER_LPTX_VECTOR_H

#include "defs.h"
#include "struct_defs.h"
#include "type_math.h"

#include <math.h>

JUPITER_LPTX_DECL_START

static inline LPTX_vector LPTX_vector_c(LPTX_type x, LPTX_type y, LPTX_type z)
{
  LPTX_vector v = {.x = x, .y = y, .z = z};
  return v;
}

static inline LPTX_vector LPTX_vector_zero(void)
{
  return LPTX_vector_c(LPTX_C(0.), LPTX_C(0.), LPTX_C(0.));
}

static inline LPTX_vector LPTX_vector_ihat(void)
{
  return LPTX_vector_c(LPTX_C(1.), LPTX_C(0.), LPTX_C(0.));
}

static inline LPTX_vector LPTX_vector_jhat(void)
{
  return LPTX_vector_c(LPTX_C(0.), LPTX_C(1.), LPTX_C(0.));
}

static inline LPTX_vector LPTX_vector_khat(void)
{
  return LPTX_vector_c(LPTX_C(0.), LPTX_C(0.), LPTX_C(1.));
}

static inline LPTX_type LPTX_vector_x(LPTX_vector v) { return v.x; }

static inline LPTX_type LPTX_vector_y(LPTX_vector v) { return v.y; }

static inline LPTX_type LPTX_vector_z(LPTX_vector v) { return v.z; }

static inline LPTX_vector LPTX_vector_set_x(LPTX_vector v, LPTX_type s)
{
  return LPTX_vector_c(s, LPTX_vector_y(v), LPTX_vector_z(v));
}

static inline LPTX_vector LPTX_vector_set_y(LPTX_vector v, LPTX_type s)
{
  return LPTX_vector_c(LPTX_vector_x(v), s, LPTX_vector_z(v));
}

static inline LPTX_vector LPTX_vector_set_z(LPTX_vector v, LPTX_type s)
{
  return LPTX_vector_c(LPTX_vector_x(v), LPTX_vector_y(v), s);
}

static inline LPTX_vector LPTX_vector_add(LPTX_vector a, LPTX_vector b)
{
  return LPTX_vector_c(LPTX_vector_x(a) + LPTX_vector_x(b),
                       LPTX_vector_y(a) + LPTX_vector_y(b),
                       LPTX_vector_z(a) + LPTX_vector_z(b));
}

static inline LPTX_vector LPTX_vector_sub(LPTX_vector a, LPTX_vector b)
{
  return LPTX_vector_c(LPTX_vector_x(a) - LPTX_vector_x(b),
                       LPTX_vector_y(a) - LPTX_vector_y(b),
                       LPTX_vector_z(a) - LPTX_vector_z(b));
}

static inline LPTX_vector LPTX_vector_mul_each(LPTX_vector a, LPTX_vector b)
{
  return LPTX_vector_c(LPTX_vector_x(a) * LPTX_vector_x(b),
                       LPTX_vector_y(a) * LPTX_vector_y(b),
                       LPTX_vector_z(a) * LPTX_vector_z(b));
}

static inline LPTX_vector LPTX_vector_mulf(LPTX_type factor, LPTX_vector a)
{
  return LPTX_vector_c(factor * LPTX_vector_x(a), factor * LPTX_vector_y(a),
                       factor * LPTX_vector_z(a));
}

static inline LPTX_type LPTX_vector_inner_prod(LPTX_vector a, LPTX_vector b)
{
  return LPTX_vector_x(a) * LPTX_vector_x(b) +
         LPTX_vector_y(a) * LPTX_vector_y(b) +
         LPTX_vector_z(a) * LPTX_vector_z(b);
}

static inline LPTX_type LPTX_vector_abssq(LPTX_vector a)
{
  return LPTX_vector_inner_prod(a, a);
}

static inline LPTX_type LPTX_vector_abs(LPTX_vector a)
{
  return LPTX_type_sqrt(LPTX_vector_abssq(a));
}

static inline LPTX_vector LPTX_vector_min_each(LPTX_vector p1, LPTX_vector p2)
{
  return LPTX_vector_c(LPTX_type_min(LPTX_vector_x(p1), LPTX_vector_x(p2)),
                       LPTX_type_min(LPTX_vector_y(p1), LPTX_vector_y(p2)),
                       LPTX_type_min(LPTX_vector_z(p1), LPTX_vector_z(p2)));
}

static inline LPTX_vector LPTX_vector_max_each(LPTX_vector p1, LPTX_vector p2)
{
  return LPTX_vector_c(LPTX_type_max(LPTX_vector_x(p1), LPTX_vector_x(p2)),
                       LPTX_type_max(LPTX_vector_y(p1), LPTX_vector_y(p2)),
                       LPTX_type_max(LPTX_vector_z(p1), LPTX_vector_z(p2)));
}

static inline LPTX_bool LPTX_vector_isnan(LPTX_vector p1)
{
  return LPTX_type_isnan(LPTX_vector_x(p1)) ||
         LPTX_type_isnan(LPTX_vector_y(p1)) ||
         LPTX_type_isnan(LPTX_vector_z(p1));
}

/**
 *
 * - `f == LPTX_VECTOR_RECT_B`: Streight below of box (only z is out of box).
 * - `f & LPTX_VECTOR_RECT_B`: Any below of box and bottom face
 * - `f & LPTX_VECTOR_RECT_B && !(f & LPTX_VECTOR_RECT_P)`: Any below of box
 *
 */
enum LPTX_vector_rect_flags
{
  LPTX_VECTOR_RECT_NONE = 0,
  LPTX_VECTOR_RECT_P = 0x0001, ///< Inside box
  LPTX_VECTOR_RECT_W = 0x0002, ///< West of box
  LPTX_VECTOR_RECT_E = 0x0004, ///< East of box
  LPTX_VECTOR_RECT_S = 0x0020, ///< South of box
  LPTX_VECTOR_RECT_N = 0x0040, ///< North of box
  LPTX_VECTOR_RECT_T = 0x0200, ///< Above of box
  LPTX_VECTOR_RECT_B = 0x0400, ///< Below of box
  LPTX_VECTOR_RECT_SW = LPTX_VECTOR_RECT_W | LPTX_VECTOR_RECT_S,
  LPTX_VECTOR_RECT_NW = LPTX_VECTOR_RECT_W | LPTX_VECTOR_RECT_N,
  LPTX_VECTOR_RECT_SE = LPTX_VECTOR_RECT_E | LPTX_VECTOR_RECT_S,
  LPTX_VECTOR_RECT_NE = LPTX_VECTOR_RECT_E | LPTX_VECTOR_RECT_N,
  LPTX_VECTOR_RECT_ST = LPTX_VECTOR_RECT_T | LPTX_VECTOR_RECT_S,
  LPTX_VECTOR_RECT_NT = LPTX_VECTOR_RECT_T | LPTX_VECTOR_RECT_N,
  LPTX_VECTOR_RECT_SB = LPTX_VECTOR_RECT_B | LPTX_VECTOR_RECT_S,
  LPTX_VECTOR_RECT_NB = LPTX_VECTOR_RECT_B | LPTX_VECTOR_RECT_N,
  LPTX_VECTOR_RECT_TW = LPTX_VECTOR_RECT_T | LPTX_VECTOR_RECT_W,
  LPTX_VECTOR_RECT_TE = LPTX_VECTOR_RECT_T | LPTX_VECTOR_RECT_E,
  LPTX_VECTOR_RECT_BW = LPTX_VECTOR_RECT_B | LPTX_VECTOR_RECT_W,
  LPTX_VECTOR_RECT_BE = LPTX_VECTOR_RECT_B | LPTX_VECTOR_RECT_E,
  LPTX_VECTOR_RECT_TSW = LPTX_VECTOR_RECT_SW | LPTX_VECTOR_RECT_T,
  LPTX_VECTOR_RECT_TNW = LPTX_VECTOR_RECT_NW | LPTX_VECTOR_RECT_T,
  LPTX_VECTOR_RECT_TSE = LPTX_VECTOR_RECT_SE | LPTX_VECTOR_RECT_T,
  LPTX_VECTOR_RECT_TNE = LPTX_VECTOR_RECT_NE | LPTX_VECTOR_RECT_T,
  LPTX_VECTOR_RECT_BSW = LPTX_VECTOR_RECT_SW | LPTX_VECTOR_RECT_B,
  LPTX_VECTOR_RECT_BNW = LPTX_VECTOR_RECT_NW | LPTX_VECTOR_RECT_B,
  LPTX_VECTOR_RECT_BSE = LPTX_VECTOR_RECT_SE | LPTX_VECTOR_RECT_B,
  LPTX_VECTOR_RECT_BNE = LPTX_VECTOR_RECT_NE | LPTX_VECTOR_RECT_B,

  LPTX_VECTOR_RECT_FACE_W = LPTX_VECTOR_RECT_P | LPTX_VECTOR_RECT_W,
  LPTX_VECTOR_RECT_FACE_E = LPTX_VECTOR_RECT_P | LPTX_VECTOR_RECT_E,
  LPTX_VECTOR_RECT_FACE_S = LPTX_VECTOR_RECT_P | LPTX_VECTOR_RECT_S,
  LPTX_VECTOR_RECT_FACE_N = LPTX_VECTOR_RECT_P | LPTX_VECTOR_RECT_N,
  LPTX_VECTOR_RECT_FACE_T = LPTX_VECTOR_RECT_P | LPTX_VECTOR_RECT_T,
  LPTX_VECTOR_RECT_FACE_B = LPTX_VECTOR_RECT_P | LPTX_VECTOR_RECT_B,
  LPTX_VECTOR_RECT_EDGE_SW = LPTX_VECTOR_RECT_FACE_W | LPTX_VECTOR_RECT_S,
  LPTX_VECTOR_RECT_EDGE_NW = LPTX_VECTOR_RECT_FACE_W | LPTX_VECTOR_RECT_N,
  LPTX_VECTOR_RECT_EDGE_SE = LPTX_VECTOR_RECT_FACE_E | LPTX_VECTOR_RECT_S,
  LPTX_VECTOR_RECT_EDGE_NE = LPTX_VECTOR_RECT_FACE_E | LPTX_VECTOR_RECT_N,
  LPTX_VECTOR_RECT_EDGE_ST = LPTX_VECTOR_RECT_FACE_T | LPTX_VECTOR_RECT_S,
  LPTX_VECTOR_RECT_EDGE_NT = LPTX_VECTOR_RECT_FACE_T | LPTX_VECTOR_RECT_N,
  LPTX_VECTOR_RECT_EDGE_SB = LPTX_VECTOR_RECT_FACE_B | LPTX_VECTOR_RECT_S,
  LPTX_VECTOR_RECT_EDGE_NB = LPTX_VECTOR_RECT_FACE_B | LPTX_VECTOR_RECT_N,
  LPTX_VECTOR_RECT_EDGE_TW = LPTX_VECTOR_RECT_FACE_T | LPTX_VECTOR_RECT_W,
  LPTX_VECTOR_RECT_EDGE_TE = LPTX_VECTOR_RECT_FACE_T | LPTX_VECTOR_RECT_E,
  LPTX_VECTOR_RECT_EDGE_BW = LPTX_VECTOR_RECT_FACE_B | LPTX_VECTOR_RECT_W,
  LPTX_VECTOR_RECT_EDGE_BE = LPTX_VECTOR_RECT_FACE_B | LPTX_VECTOR_RECT_E,
  LPTX_VECTOR_RECT_VERT_TSW = LPTX_VECTOR_RECT_EDGE_SW | LPTX_VECTOR_RECT_T,
  LPTX_VECTOR_RECT_VERT_TNW = LPTX_VECTOR_RECT_EDGE_NW | LPTX_VECTOR_RECT_T,
  LPTX_VECTOR_RECT_VERT_TSE = LPTX_VECTOR_RECT_EDGE_SE | LPTX_VECTOR_RECT_T,
  LPTX_VECTOR_RECT_VERT_TNE = LPTX_VECTOR_RECT_EDGE_NE | LPTX_VECTOR_RECT_T,
  LPTX_VECTOR_RECT_VERT_BSW = LPTX_VECTOR_RECT_EDGE_SW | LPTX_VECTOR_RECT_B,
  LPTX_VECTOR_RECT_VERT_BNW = LPTX_VECTOR_RECT_EDGE_NW | LPTX_VECTOR_RECT_B,
  LPTX_VECTOR_RECT_VERT_BSE = LPTX_VECTOR_RECT_EDGE_SE | LPTX_VECTOR_RECT_B,
  LPTX_VECTOR_RECT_VERT_BNE = LPTX_VECTOR_RECT_EDGE_NE | LPTX_VECTOR_RECT_B,
};
typedef enum LPTX_vector_rect_flags LPTX_vector_rect_flags;

static inline LPTX_vector_rect_flags
LPTX_vector_rect_in(LPTX_vector p, LPTX_vector p1, LPTX_vector p2)
{
  LPTX_vector lb, ut;
  LPTX_vector_rect_flags f;
  if (LPTX_vector_isnan(p) || LPTX_vector_isnan(p1) || LPTX_vector_isnan(p2))
    return LPTX_VECTOR_RECT_NONE;

  lb = LPTX_vector_min_each(p1, p2);
  ut = LPTX_vector_max_each(p1, p2);
  f = LPTX_VECTOR_RECT_NONE;
  if (LPTX_vector_x(p) < LPTX_vector_x(lb))
    f |= LPTX_VECTOR_RECT_W;
  if (LPTX_vector_x(ut) < LPTX_vector_x(p))
    f |= LPTX_VECTOR_RECT_E;
  if (LPTX_vector_y(p) < LPTX_vector_y(lb))
    f |= LPTX_VECTOR_RECT_S;
  if (LPTX_vector_y(ut) < LPTX_vector_y(p))
    f |= LPTX_VECTOR_RECT_N;
  if (LPTX_vector_z(p) < LPTX_vector_z(lb))
    f |= LPTX_VECTOR_RECT_B;
  if (LPTX_vector_z(ut) < LPTX_vector_z(p))
    f |= LPTX_VECTOR_RECT_T;
  if (f != LPTX_VECTOR_RECT_NONE)
    return f;

  f = LPTX_VECTOR_RECT_P;
  if (LPTX_vector_x(p) <= LPTX_vector_x(lb))
    f |= LPTX_VECTOR_RECT_W;
  if (LPTX_vector_x(ut) <= LPTX_vector_x(p))
    f |= LPTX_VECTOR_RECT_E;
  if (LPTX_vector_y(p) <= LPTX_vector_y(lb))
    f |= LPTX_VECTOR_RECT_S;
  if (LPTX_vector_y(ut) <= LPTX_vector_y(p))
    f |= LPTX_VECTOR_RECT_N;
  if (LPTX_vector_z(p) <= LPTX_vector_z(lb))
    f |= LPTX_VECTOR_RECT_B;
  if (LPTX_vector_z(ut) <= LPTX_vector_z(p))
    f |= LPTX_VECTOR_RECT_T;
  return f;
}

JUPITER_LPTX_DECL_END

#endif
