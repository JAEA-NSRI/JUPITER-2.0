#ifndef JUPITER_GEOMETRY_BITARRAY_H
#define JUPITER_GEOMETRY_BITARRAY_H

#include <string.h>
#include <stdlib.h>
#include <limits.h>

#include "defs.h"
#include "geom_assert.h"

#ifdef JUPITER_GEOMETRY_USE_MPI
#include <mpi.h>
#endif

JUPITER_GEOMETRY_DECL_START

typedef unsigned int geom_bitarray_element_type;
#ifdef JUPITER_GEOMETRY_USE_MPI
#define GEOM_BITARRAY_ELEMENT_MPI_BASE_TYPE MPI_UNSIGNED
#endif

#define GEOM_BITARRAY_WORD_BITS (sizeof(geom_bitarray_element_type) * CHAR_BIT)
#define GEOM_BITARRAY_N_WORD(nbits) \
  ((nbits <= 0) ? 0 : ((nbits - 1) / GEOM_BITARRAY_WORD_BITS + 1))

#define GEOM_BITARRAY_WORD_ALL (~((geom_bitarray_elmeent_type)0))

static inline geom_bitarray_element_type geom_bitarray_word_all(void)
{
  return ~((geom_bitarray_element_type)0);
}

static inline geom_size_type geom_bitarray_word_bits(void)
{
  return GEOM_BITARRAY_WORD_BITS;
}

static inline geom_bitarray_element_type
geom_bitarray_word_mask_for_least(geom_size_type nbits)
{
  if (nbits >= geom_bitarray_word_bits())
    return geom_bitarray_word_all();
  return ~(geom_bitarray_word_all() << nbits);
}

static inline geom_size_type geom_bitarray_n_word(geom_size_type nbits)
{
  return GEOM_BITARRAY_N_WORD(nbits);
}

static inline void geom_bitarray_element_addr(geom_size_type bit,
                                              geom_size_type *array_idx,
                                              geom_size_type *bit_idx,
                                              geom_bitarray_element_type *mask)
{
  geom_size_type w = geom_bitarray_word_bits();
  geom_size_type i = bit / w;
  geom_size_type j = bit % w;

  GEOM_ASSERT(i >= 0);
  GEOM_ASSERT(j >= 0);

  *array_idx = i;
  *bit_idx = j;
  if (mask)
    *mask = ~(1 << j);
}

static inline void geom_bitarray_element_set(geom_bitarray_element_type *e,
                                             geom_size_type bit, int value)
{
  geom_bitarray_element_type mask;
  geom_size_type i, j;
  geom_bitarray_element_addr(bit, &i, &j, &mask);

  GEOM_ASSERT(e);

  /* normalize to 0 or 1 */
  value = !!value;

  e[i] = (e[i] & mask) | (value << j);
}

static inline void geom_bitarray_element_setall(geom_bitarray_element_type *e,
                                                geom_size_type nbits, int value)
{
  geom_size_type w = geom_bitarray_n_word(nbits);
  geom_bitarray_element_type v = 0;

  GEOM_ASSERT(e);

  if (value)
    v = ~v;

  for (geom_size_type i = 0; i < w; ++i) {
    e[i] = v;
  }
}

static inline int geom_bitarray_element_get(const geom_bitarray_element_type *e,
                                            geom_size_type bit)
{
  geom_size_type i, j;
  geom_bitarray_element_addr(bit, &i, &j, NULL);

  return !!(e[i] & (1 << j));
}

static inline int
geom_bitarray_element_getany(const geom_bitarray_element_type *e,
                             geom_size_type nbits)
{
  geom_size_type nword = geom_bitarray_n_word(nbits);
  geom_size_type nrem;

  for (geom_size_type i = 0; i < nword - 1; ++i) {
    if (e[i])
      return 1;
  }

  nrem = nbits % geom_bitarray_word_bits();
  if (e[nword - 1] & geom_bitarray_word_mask_for_least(nrem))
    return 1;
  return 0;
}

static inline int
geom_bitarray_element_getall(const geom_bitarray_element_type *e,
                             geom_size_type nbits)
{
  geom_size_type nword = geom_bitarray_n_word(nbits);
  geom_size_type nrem;

  for (geom_size_type i = 0; i < nword - 1; ++i) {
    if (~e[i])
      return 0;
  }

  nrem = nbits % geom_bitarray_word_bits();
  if ((~e[nword - 1]) & geom_bitarray_word_mask_for_least(nrem))
    return 0;
  return 1;
}

static inline void
geom_bitarray_element_copy(geom_bitarray_element_type *dest,
                           const geom_bitarray_element_type *src,
                           geom_size_type nbits)
{
  if (dest == src)
    return;

  geom_size_type w = geom_bitarray_n_word(nbits);

  memmove(dest, src, w * sizeof(geom_bitarray_element_type));
}

static inline void geom_bitarray_element_bor(
  geom_bitarray_element_type *dest, const geom_bitarray_element_type *s1,
  const geom_bitarray_element_type *s2, geom_size_type nbits)
{
  if (s1 != s2) {
    geom_size_type w = geom_bitarray_n_word(nbits);
    for (geom_size_type i = 0; i < w; ++i)
      dest[i] = s1[i] | s2[i];
  } else {
    geom_bitarray_element_copy(dest, s1, nbits);
  }
}

static inline void geom_bitarray_element_band(
  geom_bitarray_element_type *dest, const geom_bitarray_element_type *s1,
  const geom_bitarray_element_type *s2, geom_size_type nbits)
{
  if (s1 != s2) {
    geom_size_type w = geom_bitarray_n_word(nbits);
    for (geom_size_type i = 0; i < w; ++i)
      dest[i] = s1[i] & s2[i];
  } else {
    geom_bitarray_element_copy(dest, s1, nbits);
  }
}

static inline int geom_bitarray_element_eql(geom_bitarray_element_type *s1,
                                            geom_bitarray_element_type *s2,
                                            geom_size_type nbits)
{
  if (s1 != s2) {
    geom_size_type w = geom_bitarray_n_word(nbits);
    geom_size_type nrem, nmsk;
    nrem = nbits % geom_bitarray_word_bits();
    nmsk = geom_bitarray_word_mask_for_least(nrem);

    for (geom_size_type i = 0; i < w - 1; ++i) {
      if (s1[i] != s2[i])
        return 0;
    }

    if ((s1[w - 1] & nmsk) != (s2[w - 1] & nmsk))
      return 0;
  }
  return 1;
}

static inline int geom_bitarray_element_neq(geom_bitarray_element_type *s1,
                                            geom_bitarray_element_type *s2,
                                            geom_size_type nbits)
{
  if (s1 != s2) {
    geom_size_type w = geom_bitarray_n_word(nbits);
    geom_size_type nrem, nmsk;
    nrem = nbits % geom_bitarray_word_bits();
    nmsk = geom_bitarray_word_mask_for_least(nrem);

    for (geom_size_type i = 0; i < w - 1; ++i) {
      if (s1[i] != s2[i])
        return 1;
    }

    if ((s1[w - 1] & nmsk) != (s2[w - 1] & nmsk))
      return 1;
  }
  return 0;
}

/**
 * @brief Define statically width bit array
 *
 * This macro can be used both typedef and variable declaration.
 */
#define geom_bitarray_n(array_name, nbits) \
  geom_bitarray_element_type array_name[GEOM_BITARRAY_N_WORD(nbits)]

#ifdef JUPITER_GEOMETRY_USE_MPI
static inline int
geom_bitarray_element_mpi_bcast(geom_bitarray_element_type *arr,
                                geom_size_type nbits, int root, MPI_Comm comm)
{
  geom_size_type sz = geom_bitarray_n_word(nbits);
  int isz = sz;

  GEOM_ASSERT(arr);
  if (isz < 0 || isz != sz)
    return MPI_ERR_ARG;
  return MPI_Bcast(arr, isz, GEOM_BITARRAY_ELEMENT_MPI_BASE_TYPE, root, comm);
}

static inline int geom_bitarray_element_mpi_bor(geom_bitarray_element_type *arr,
                                                geom_size_type nbits,
                                                MPI_Comm comm)
{
  geom_size_type sz = geom_bitarray_n_word(nbits);
  int isz = sz;

  GEOM_ASSERT(arr);
  if (isz <= 0 || isz != sz)
    return MPI_ERR_ARG;
  return MPI_Allreduce(MPI_IN_PLACE, arr, isz,
                       GEOM_BITARRAY_ELEMENT_MPI_BASE_TYPE, MPI_BOR, comm);
}

static inline int
geom_bitarray_element_mpi_band(geom_bitarray_element_type *arr,
                               geom_size_type nbits, MPI_Comm comm)
{
  geom_size_type sz = geom_bitarray_n_word(nbits);
  int isz = sz;

  GEOM_ASSERT(arr);
  if (isz <= 0 || isz != sz)
    return MPI_ERR_ARG;
  return MPI_Allreduce(MPI_IN_PLACE, arr, isz,
                       GEOM_BITARRAY_ELEMENT_MPI_BASE_TYPE, MPI_BAND, comm);
}

static inline int
geom_bitarray_element_mpi_is_set_in(int *ret, geom_bitarray_element_type *arr,
                                    geom_size_type bit, int rank, MPI_Comm comm)
{
  int b;
  int r;
  int my;

  GEOM_ASSERT(ret);
  GEOM_ASSERT(arr);

  MPI_Comm_rank(comm, &my);
  if (rank == my) {
    b = geom_bitarray_element_get(arr, bit);
  }
  r = MPI_Bcast(&b, 1, MPI_INT, my, comm);
  if (r != MPI_SUCCESS)
    return r;

  *ret = b;
  return MPI_SUCCESS;
}

static inline int
geom_bitarray_element_mpi_is_set_all(int *ret, geom_bitarray_element_type *arr,
                                     geom_size_type bit, MPI_Comm comm)
{
  int r;
  int b;

  GEOM_ASSERT(ret);
  GEOM_ASSERT(arr);

  b = geom_bitarray_element_get(arr, bit);
  r = MPI_Allreduce(MPI_IN_PLACE, &b, 1, MPI_INT, MPI_LAND, comm);
  if (r != MPI_SUCCESS)
    return r;

  *ret = b;
  return MPI_SUCCESS;
}

static inline int
geom_bitarray_element_mpi_is_set_any(int *ret, geom_bitarray_element_type *arr,
                                     geom_size_type bit, MPI_Comm comm)
{
  int r;
  int b;

  GEOM_ASSERT(ret);
  GEOM_ASSERT(arr);

  b = geom_bitarray_element_get(arr, bit);
  r = MPI_Allreduce(MPI_IN_PLACE, &b, 1, MPI_INT, MPI_LOR, comm);
  if (r != MPI_SUCCESS)
    return r;

  *ret = b;
  return MPI_SUCCESS;
}
#endif

JUPITER_GEOMETRY_DECL_END

#endif
