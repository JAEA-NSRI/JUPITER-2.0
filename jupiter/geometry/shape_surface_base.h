#ifndef JUPITER_GEOMETRY_SHAPE_SURFACE_COMMON_H
#define JUPITER_GEOMETRY_SHAPE_SURFACE_COMMON_H

#include "defs.h"
#include "geom_assert.h"
#include "bitarray.h"

#include <stdlib.h>
#include <limits.h>

JUPITER_GEOMETRY_DECL_START

/**
 * @brief Number of surfaces in a shape.
 *
 * Make sure to define the correct value before include the file
 *
 * - Positive values are fixed to that value
 * - Negative values are defined to dynamic value
 */
#ifndef GEOM_SHAPE_SURFACE_N
#define GEOM_SHAPE_SURFACE_N 1
#error Please define GEOM_SHAPE_SURFACE_N to respective value
#endif
#if GEOM_SHAPE_SURFACE_N > 0
#define GEOM_SHAPE_SURFACE_N_STATIC 1
#elif GEOM_SHAPE_SURFACE_N < 0
#define GEOM_SHAPE_SURFACE_N_STATIC 0
#else
#error GEOM_SHAPE_SURFACE_N must not be 0 (negative if dynamic, positive if fixed)
#endif

/**
 * @brief Base commonized surface shape operation
 *
 * The purpose of this struct is basically for future expansion, to
 * include properties which all shapes which supports its surface
 * should have.
 *
 * This struct is for implementing a shape. Not for general use.
 */
struct geom_shape_surface_common
{
#if GEOM_SHAPE_SURFACE_N_STATIC
  geom_bitarray_n(enable_surface, GEOM_SHAPE_SURFACE_N);
#else
  geom_bitarray *enable_surface;
#endif
};

#if GEOM_SHAPE_SURFACE_N_STATIC
static inline void
geom_shape_surface_common_init(struct geom_shape_surface_common *b)
{
  GEOM_ASSERT(b);
  geom_bitarray_element_setall(b->enable_surface, GEOM_SHAPE_SURFACE_N, 0);
}

static inline void
geom_shape_surface_common_clean(struct geom_shape_surface_common *b)
{
  /* nop */
}

#else
static inline void
geom_shape_surface_common_init(struct geom_shape_surface_common *b,
                               int nsurface)
{
  GEOM_ASSERT(b);
  b->enable_surface = geom_bitarray_new(nsurface);
}

static inline void
geom_shape_surface_common_clean(struct geom_shape_surface_common *b)
{
  geom_bitarray_delete(b->enable_surface);
  b->enable_surface = NULL;
}
#endif

static inline void
geom_shape_surface_common_enable(struct geom_shape_surface_common *b,
                                 int surfid)
{
  GEOM_ASSERT(b);
#if GEOM_SHAPE_SURFACE_N_STATIC
  GEOM_ASSERT(surfid >= 0 && surfid < GEOM_SHAPE_SURFACE_N);
  geom_bitarray_element_set(b->enable_surface, surfid, 1);
#else
  geom_bitarray_set(b->enable_surface, surfid, 1);
#endif
}

static inline void
geom_shape_surface_common_disable(struct geom_shape_surface_common *b,
                                  int surfid)
{
  GEOM_ASSERT(b);
#if GEOM_SHAPE_SURFACE_N_STATIC
  GEOM_ASSERT(surfid >= 0 && surfid < GEOM_SHAPE_SURFACE_N);
  geom_bitarray_element_set(b->enable_surface, surfid, 0);
#else
  geom_bitarray_set(b->enable_surface, surfid, 0);
#endif
}

static inline int
geom_shape_surface_common_is_enabled(struct geom_shape_surface_common *b,
                                     int surfid)
{
  GEOM_ASSERT(b);
#if GEOM_SHAPE_SURFACE_N_STATIC
  GEOM_ASSERT(surfid >= 0 && surfid < GEOM_SHAPE_SURFACE_N);
  return geom_bitarray_element_get(b->enable_surface, surfid);
#else
  return geom_bitarray_get(b->enable_surface, surfid);
#endif
}

static inline void
geom_shape_surface_common_copy(struct geom_shape_surface_common *dest,
                               const struct geom_shape_surface_common *src)
{
  GEOM_ASSERT(dest);
  GEOM_ASSERT(src);
#if GEOM_SHAPE_SURFACE_N_STATIC
  geom_bitarray_element_copy(dest->enable_surface, src->enable_surface,
                             GEOM_SHAPE_SURFACE_N);
#else
  if (src->enable_surface) {
    dest->enable_surface = geom_bitarray_dup(src->enable_surface);
  } else {
    dest->enable_surface = NULL;
  }
#endif
}

JUPITER_GEOMETRY_DECL_END

#endif
