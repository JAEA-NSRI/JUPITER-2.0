#ifndef JUPITER_INIT_COMPONENT_H
#define JUPITER_INIT_COMPONENT_H

#include "geometry/bitarray.h"

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

/**
 * @brief Initialization component ids.
 *
 * Flags that components to be initalized in geometry_in_with().
 */
enum init_component_id
{
  INIT_COMPONENT_BOUNDARY = 0,     /*!< Fluid boundary */
  INIT_COMPONENT_THERMAL_BOUNDARY, /*!< Thrermal boundary */
  INIT_COMPONENT_SURFACE_BOUNDARY, /*!< Surface boundary */
  INIT_COMPONENT_VOF,              /*!< Volume Occupation Fraction */
  INIT_COMPONENT_VELOCITY_U,       /*!< X-axis Velocity */
  INIT_COMPONENT_VELOCITY_V,       /*!< Y-axis Velocity */
  INIT_COMPONENT_VELOCITY_W,       /*!< Z-axis Velocity */
  INIT_COMPONENT_PRESSURE,         /*!< Pressure */
  INIT_COMPONENT_TEMPERATURE,      /*!< Temperature */
  INIT_COMPONENT_FIXED_HSOURCE,    /*!< Fixed or Controlled Heat source */
  INIT_COMPONENT_LPT_PEWALL_N,
  /*!< Normal component of Particle Resistitution coefficient */
  // INIT_COMPONENT_LPT_PEWALL_T,
  // /*!< Tangent component of Particle Resistitution coefficient (not supported) */

  INIT_COMPONENT_GEOM_DUMP, /*!< Execute Geom_dump */
  INIT_COMPONENT_MAX,       /*!< Must be the last element */
};

struct init_component
{
  geom_bitarray_n(bits, INIT_COMPONENT_MAX);
};
typedef struct init_component init_component;

static inline void init_component_clear(init_component *flags)
{
  geom_bitarray_element_setall(flags->bits, INIT_COMPONENT_MAX, 0);
}

static inline void init_component_setall(init_component *flags)
{
  geom_bitarray_element_setall(flags->bits, INIT_COMPONENT_MAX, 1);
}

static inline int init_component_is_set(init_component *flags,
                                        enum init_component_id flag)
{
  return geom_bitarray_element_get(flags->bits, flag);
}

static inline void init_component_set(init_component *flags,
                                      enum init_component_id flag)
{
  geom_bitarray_element_set(flags->bits, flag, 1);
}

static inline void init_component_unset(init_component *flags,
                                        enum init_component_id flag)
{
  geom_bitarray_element_set(flags->bits, flag, 0);
}

static inline int init_component_any(const init_component *flags)
{
  return geom_bitarray_element_getany(flags->bits, INIT_COMPONENT_MAX);
}

static inline int init_component_isall(const init_component *flags)
{
  return geom_bitarray_element_getall(flags->bits, INIT_COMPONENT_MAX);
}

static inline void init_component_copy(init_component *dst,
                                       const init_component *src)
{
  geom_bitarray_element_copy(dst->bits, src->bits, INIT_COMPONENT_MAX);
}

static inline void init_component_band(init_component *dest,
                                       const init_component *s1,
                                       const init_component *s2)
{
  geom_bitarray_element_band(dest->bits, s1->bits, s2->bits,
                             INIT_COMPONENT_MAX);
}

static inline void init_component_bor(init_component *dest,
                                      const init_component *s1,
                                      const init_component *s2)
{
  geom_bitarray_element_bor(dest->bits, s1->bits, s2->bits, INIT_COMPONENT_MAX);
}

static inline init_component init_component_all(void)
{
  init_component c;
  init_component_setall(&c);
  return c;
}

static inline init_component init_component_zero(void)
{
  init_component c;
  init_component_clear(&c);
  return c;
}

#ifdef JUPITER_MPI
static inline int init_component_mpi_bcast(init_component *inp, int root,
                                           MPI_Comm comm)
{
  return geom_bitarray_element_mpi_bcast(inp->bits, INIT_COMPONENT_MAX, root,
                                         comm);
}

static inline int init_component_mpi_bor(init_component *inp, MPI_Comm comm)
{
  return geom_bitarray_element_mpi_bor(inp->bits, INIT_COMPONENT_MAX, comm);
}

static inline int init_component_mpi_band(init_component *inp, MPI_Comm comm)
{
  return geom_bitarray_element_mpi_band(inp->bits, INIT_COMPONENT_MAX, comm);
}

static inline int init_component_mpi_is_set_any(int *ret, init_component *inp,
                                                enum init_component_id flag,
                                                MPI_Comm comm)
{
  return geom_bitarray_element_mpi_is_set_any(ret, inp->bits, flag, comm);
}

static inline int init_component_mpi_is_set_all(int *ret, init_component *inp,
                                                enum init_component_id flag,
                                                MPI_Comm comm)
{
  return geom_bitarray_element_mpi_is_set_all(ret, inp->bits, flag, comm);
}
#endif

#endif
