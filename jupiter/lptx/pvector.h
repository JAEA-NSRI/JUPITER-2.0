#ifndef JUPITER_LPTX_PVECTOR_H
#define JUPITER_LPTX_PVECTOR_H

#include "defs.h"
#include "error.h"
#include "struct_defs.h"

JUPITER_LPTX_DECL_START

static inline void LPTX_particle_vector_init(LPTX_particle_vector *p)
{
  *(LPTX_type **)&p->v = NULL;
  *(LPTX_idtype *)&p->length = 0;
}

static inline const LPTX_type *
LPTX_particle_vector_getp(const LPTX_particle_vector *p)
{
  return p->v;
}

static inline LPTX_type *
LPTX_particle_vector_getp_writable(LPTX_particle_vector *p)
{
  return (LPTX_type *)p->v;
}

static inline const LPTX_type *
LPTX_particle_vector_getvp(const LPTX_particle_vector *p, LPTX_idtype index)
{
  if (index >= 0 && index < p->length)
    return &LPTX_particle_vector_getp(p)[index];
  return NULL;
}

static inline LPTX_type *
LPTX_particle_vector_getvp_writable(LPTX_particle_vector *p, LPTX_idtype index)
{
  if (index >= 0 && index < p->length)
    return &LPTX_particle_vector_getp_writable(p)[index];
  return NULL;
}

static inline LPTX_type LPTX_particle_vector_getv(const LPTX_particle_vector *p,
                                                  LPTX_idtype index)
{
  const LPTX_type *v = LPTX_particle_vector_getvp(p, index);
  if (v)
    return *v;
  return LPTX_TYPE_HUGE_VAL;
}

static inline void LPTX_particle_vector_setv(LPTX_particle_vector *p,
                                             LPTX_idtype index, LPTX_type value)
{
  LPTX_type *v = LPTX_particle_vector_getvp_writable(p, index);
  if (v)
    *v = value;
}

static inline void LPTX_particle_vector_bind(LPTX_particle_vector *p,
                                             LPTX_type *d, LPTX_idtype length)
{
  *(LPTX_type **)&p->v = d;
  *(LPTX_idtype *)&p->length = length;
}

static inline void LPTX_particle_vector_copy(LPTX_particle_vector *dest,
                                             const LPTX_particle_vector *source)
{
  LPTX_assert(dest->length == source->length);

  if (dest == source)
    return;

  for (LPTX_idtype jj = 0; jj < source->length; ++jj) {
    LPTX_type value;
    value = LPTX_particle_vector_getv(source, jj);
    LPTX_particle_vector_setv(dest, jj, value);
  }
}

/**
 * Copy source to dest, filling with given value for missing elements
 */
static inline void
LPTX_particle_vector_copy_fill(LPTX_particle_vector *dest,
                               const LPTX_particle_vector *source,
                               LPTX_type fill)
{
  if (source)
    LPTX_assert(dest->length >= source->length);

  if (dest == source)
    return;

  for (LPTX_idtype jj = 0; jj < dest->length; ++jj) {
    LPTX_type value;
    if (source && jj < source->length) {
      value = LPTX_particle_vector_getv(source, jj);
    } else {
      value = fill;
    }
    LPTX_particle_vector_setv(dest, jj, value);
  }
}

static inline const LPTX_particle_vector *
LPTX_particle_data_get_vector(const LPTX_particle_data *p,
                              LPTX_idtype vector_index)
{
  if (vector_index >= 0 && vector_index < p->number_of_vectors)
    return &p->vectors[vector_index];
  return NULL;
}

static inline LPTX_particle_vector *
LPTX_particle_data_get_vector_writable(LPTX_particle_data *p,
                                       LPTX_idtype vector_index)
{
  if (vector_index >= 0 && vector_index < p->number_of_vectors)
    return (LPTX_particle_vector *)&p->vectors[vector_index];
  return NULL;
}

/**
 * Allocate vector for @p with length of @p length
 *
 * The allocation is only for temporary use.
 *
 * @warning This function will discard the pointer specified in @p p without
 * deletion.
 *
 * If @p length is 0 or less, this function is equivalent to
 * LPTX_particle_vector_init().
 */
JUPITER_LPTX_DECL
void LPTX_particle_vector_allocate(LPTX_particle_vector *p, LPTX_idtype length);

/**
 * Free vector in @p
 *
 * @warning This function is only for LPTX_particle_vector allocated using
 * LPTX_particle_vector_allcate(). This is **not** applicable for vectors bound
 * to particles.
 */
JUPITER_LPTX_DECL
void LPTX_particle_vector_free(LPTX_particle_vector *p);

JUPITER_LPTX_DECL_END

#endif
