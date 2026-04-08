#ifndef JUPITER_GEOMETRY_BITARRAYV_H
#define JUPITER_GEOMETRY_BITARRAYV_H

#include "bitarray.h"
#include "defs.h"
#include "geom_assert.h"

#ifdef JUPITER_GEOMETRY_USE_MPI
#include <mpi.h>
#endif

JUPITER_GEOMETRY_DECL_START

/**
 * @brief bit array implementiation for C.
 *
 * This is for internal use, and invalid C++ source. Use
 * `std::bitset` from use in C++.
 */
struct geom_bitarray
{
  geom_size_type nbits;
  geom_bitarray_element_type bits[];
};
typedef struct geom_bitarray geom_bitarray;

static inline geom_size_type geom_bitarray_size(geom_size_type nbits)
{
  return sizeof(geom_bitarray) +
         sizeof(geom_bitarray_element_type) * geom_bitarray_n_word(nbits);
}

static inline void geom_bitarray_setall(geom_bitarray *a, int value);

static inline geom_bitarray *geom_bitarray_new(geom_size_type nbits)
{
  geom_bitarray *ptr;
  geom_size_type nsz = geom_bitarray_size(nbits);

  ptr = (geom_bitarray *)malloc(nsz);
  if (!ptr)
    return NULL;

  ptr->nbits = nbits;
  geom_bitarray_setall(ptr, 0);
  return ptr;
}

static inline void geom_bitarray_delete(geom_bitarray *a) { free(a); }

static inline void geom_bitarray_set(geom_bitarray *a, geom_size_type bit,
                                     int value)
{
  GEOM_ASSERT(a);
  GEOM_ASSERT(bit >= 0 && bit < a->nbits);

  geom_bitarray_element_set(a->bits, bit, value);
}

static inline void geom_bitarray_setall(geom_bitarray *a, int value)
{
  GEOM_ASSERT(a);

  geom_bitarray_element_setall(a->bits, a->nbits, value);
}

static inline int geom_bitarray_get(const geom_bitarray *a, geom_size_type bit)
{
  GEOM_ASSERT(a);
  GEOM_ASSERT(bit >= 0 && bit < a->nbits);

  return geom_bitarray_element_get(a->bits, bit);
}

static inline geom_bitarray *geom_bitarray_dup(geom_bitarray *a)
{
  geom_bitarray *ptr;
  GEOM_ASSERT(a);

  ptr = geom_bitarray_new(a->nbits);
  if (!ptr)
    return NULL;

  geom_bitarray_element_copy(ptr->bits, a->bits, a->nbits);
  return ptr;
}

static inline geom_bitarray *
geom_bitarray_bor(geom_bitarray *dest, geom_bitarray *s1, geom_bitarray *s2)
{
  GEOM_ASSERT(dest);
  GEOM_ASSERT(s1);
  GEOM_ASSERT(s2);
  GEOM_ASSERT(s1->nbits == s2->nbits && s1->nbits == dest->nbits);

  geom_bitarray_element_bor(dest->bits, s1->bits, s2->bits, s1->nbits);
  return dest;
}

static inline geom_bitarray *
geom_bitarray_band(geom_bitarray *dest, geom_bitarray *s1, geom_bitarray *s2)
{
  GEOM_ASSERT(dest);
  GEOM_ASSERT(s1);
  GEOM_ASSERT(s2);
  GEOM_ASSERT(s1->nbits == s2->nbits && s1->nbits == dest->nbits);

  geom_bitarray_element_band(dest->bits, s1->bits, s2->bits, s1->nbits);
  return dest;
}

static inline int geom_bitarray_eql(geom_bitarray *s1, geom_bitarray *s2)
{
  GEOM_ASSERT(s1);
  GEOM_ASSERT(s2);
  GEOM_ASSERT(s1->nbits == s2->nbits);

  return geom_bitarray_element_eql(s1->bits, s2->bits, s1->nbits);
}

static inline int geom_bitarray_neq(geom_bitarray *s1, geom_bitarray *s2)
{
  GEOM_ASSERT(s1);
  GEOM_ASSERT(s2);
  GEOM_ASSERT(s1->nbits == s2->nbits);

  return geom_bitarray_element_neq(s1->bits, s2->bits, s1->nbits);
}

JUPITER_GEOMETRY_DECL_END

#endif
