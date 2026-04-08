#ifndef JUPITER_LPTX_PRIV_ARRAY_TYPES_A_H
#define JUPITER_LPTX_PRIV_ARRAY_TYPES_A_H

#include "defs.h"
#include "jupiter/random/random.h"
#include "priv_array_data.h"
#include "struct_defs.h"

#include <stddef.h>

JUPITER_LPTX_DECL_START

/*
 * Array-side data type conversion implementation of API for extract/assign one
 * of properties of particle data to/from plain array.
 *
 * This file is private header.
 */

struct LPTX_array_get_variable_comps
{
  struct LPTX_array_get_data d;
  size_t number_of_components;
};
#define LPTX_array_get_variable_comps_entry(ptr) \
  geom_container_of(ptr, struct LPTX_array_get_variable_comps, d)

struct LPTX_array_set_variable_comps
{
  struct LPTX_array_set_data d;
  size_t number_of_components;
};
#define LPTX_array_set_variable_comps_entry(ptr) \
  geom_container_of(ptr, struct LPTX_array_set_variable_comps, d)

//--- Number of components for scalar properties

static inline size_t
LPTX_array_get_scalar_n(const struct LPTX_array_get_data *args)
{
  return 1;
}

static inline size_t
LPTX_array_set_scalar_n(const struct LPTX_array_set_data *args)
{
  return 1;
}

//--- Number of components for vector properties

static inline size_t
LPTX_array_get_vector_n(const struct LPTX_array_get_data *args)
{
  return 3;
}

static inline size_t
LPTX_array_set_vector_n(const struct LPTX_array_set_data *args)
{
  return 3;
}

//--- Number of components for flags property as bits

static inline size_t
LPTX_array_get_flags_n(const struct LPTX_array_get_data *args)
{
  return LPTX_PTFLAG_MAX;
}

static inline size_t
LPTX_array_set_flags_n(const struct LPTX_array_set_data *args)
{
  return LPTX_PTFLAG_MAX;
}

//--- Number of components for flags property as raw data type

static inline size_t
LPTX_array_get_flagse_n(const struct LPTX_array_get_data *args)
{
  return LPTX_particle_flags_N;
}

static inline size_t
LPTX_array_set_flagse_n(const struct LPTX_array_set_data *args)
{
  return LPTX_particle_flags_N;
}

//--- Number of components for random seed property as raw data type

static inline size_t
LPTX_array_get_seedi_n(const struct LPTX_array_get_data *args)
{
  return JUPITER_RANDOM_SEED_SIZE;
}

static inline size_t
LPTX_array_set_seedi_n(const struct LPTX_array_set_data *args)
{
  return JUPITER_RANDOM_SEED_SIZE;
}

//--- Number of components for variable number of components property

static inline size_t
LPTX_array_get_variable_comps_n(const struct LPTX_array_get_data *args)
{
  const struct LPTX_array_get_variable_comps *p;
  p = LPTX_array_get_variable_comps_entry(args);
  return p->number_of_components;
}

static inline size_t
LPTX_array_set_variable_comps_n(const struct LPTX_array_set_data *args)
{
  const struct LPTX_array_set_variable_comps *p;
  p = LPTX_array_set_variable_comps_entry(args);
  return p->number_of_components;
}

//--- Scalar types

#define LPTX_DEFINE_ARRAY_GET_S_TYPE0(name, func, soahelper) \
  struct name##_data                                         \
  {                                                          \
    struct LPTX_array_get_data d;                            \
  };                                                         \
                                                             \
  static const struct LPTX_array_get_funcs name##_funcs = {  \
    .fn = LPTX_array_get_scalar_n,                           \
    .f = func,                                               \
    .vecsoa_helper = soahelper,                              \
  };

#define LPTX_ARRAY_GET_S_TYPE0_INIT(name) \
  (&(((struct name##_data){.d = {.funcs = &name##_funcs}}).d))

//--- LPTX_idtype -> LPTX_idtype scalar getter

static void LPTX_array_get_spp_f(void *outp, LPTX_idtype outidx,
                                 const void *inp, LPTX_idtype component_index,
                                 const struct LPTX_array_get_data *args)
{
  *((LPTX_idtype *)outp + outidx) = *(const LPTX_idtype *)inp;
}

static void *LPTX_array_get_spp_vh(void *outpp, LPTX_idtype component_index,
                                   const struct LPTX_array_get_data *args)
{
  LPTX_idtype *p = ((LPTX_idtype **)outpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_spp, LPTX_array_get_spp_f,
                              LPTX_array_get_spp_vh)

/**
 * LPTX_idtype entry to LPTX_idtype array
 */
#define LPTX_array_get_spp() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_spp)
#define LPTX_inptype_LPTX_array_get_spp() LPTX_idtype
#define LPTX_outtype_LPTX_array_get_spp() LPTX_idtype

//--- LPTX_idtype -> int scalar getter

static void LPTX_array_get_spi_f(void *outp, LPTX_idtype outidx,
                                 const void *inp, LPTX_idtype component_index,
                                 const struct LPTX_array_get_data *args)
{
  *((int *)outp + outidx) = *(const LPTX_idtype *)inp;
}

static void *LPTX_array_get_spi_vh(void *outpp, LPTX_idtype component_index,
                                   const struct LPTX_array_get_data *args)
{
  int *p = ((int **)outpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_spi, LPTX_array_get_spi_f,
                              LPTX_array_get_spi_vh)

/**
 * LPTX_idtype entry to int array
 */
#define LPTX_array_get_spi() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_spi)
#define LPTX_inptype_LPTX_array_get_spi() LPTX_idtype
#define LPTX_outtype_LPTX_array_get_spi() int

//--- LPTX_int (as boolean) -> LPTX_bool scalar getter

static void LPTX_array_get_sib_f(void *outp, LPTX_idtype outidx,
                                 const void *inp, LPTX_idtype component_index,
                                 const struct LPTX_array_get_data *args)
{
  *((LPTX_bool *)outp + outidx) =
    (*(const LPTX_int *)inp) ? LPTX_true : LPTX_false;
}

static void *LPTX_array_get_sib_vh(void *outpp, LPTX_idtype component_index,
                                   const struct LPTX_array_get_data *args)
{
  LPTX_bool *p = ((LPTX_bool **)outpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_sib, LPTX_array_get_sib_f,
                              LPTX_array_get_sib_vh)

/**
 * LPTX_int (as boolean) entry to LPTX_bool array
 */
#define LPTX_array_get_sib() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_sib)
#define LPTX_inptype_LPTX_array_get_sib() LPTX_int
#define LPTX_outtype_LPTX_array_get_sib() LPTX_bool

//--- LPTX_bool -> LPTX_bool scalar getter

static void LPTX_array_get_sbb_f(void *outp, LPTX_idtype outidx,
                                 const void *inp, LPTX_idtype component_index,
                                 const struct LPTX_array_get_data *args)
{
  *((LPTX_bool *)outp + outidx) = *(const LPTX_bool *)inp;
}

static void *LPTX_array_get_sbb_vh(void *outpp, LPTX_idtype component_index,
                                   const struct LPTX_array_get_data *args)
{
  LPTX_bool *p = ((LPTX_bool **)outpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_sbb, LPTX_array_get_sbb_f,
                              LPTX_array_get_sbb_vh)

/**
 * LPTX_bool entry to LPTX_bool array
 *
 * @note LPTX_particle cannot have LPTX_bool entry. Since they cannot be
 * communicated using MPI.
 */
#define LPTX_array_get_sbb() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_sbb)
#define LPTX_inptype_LPTX_array_get_sbb() LPTX_bool
#define LPTX_outtype_LPTX_array_get_sbb() LPTX_bool

//--- LPTX_bool -> int (as boolean) scalar getter

static void LPTX_array_get_sbi_f(void *outp, LPTX_idtype outidx,
                                 const void *inp, LPTX_idtype component_index,
                                 const struct LPTX_array_get_data *args)
{
  *((int *)outp + outidx) = ((*(const LPTX_bool *)inp) == LPTX_true) ? 1 : 0;
}

static void *LPTX_array_get_sbi_vh(void *outp, LPTX_idtype component_index,
                                   const struct LPTX_array_get_data *args)
{
  int *p = ((int **)outp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_sbi, LPTX_array_get_sbi_f,
                              LPTX_array_get_sbi_vh)
#define LPTX_inptype_LPTX_array_get_sbi() LPTX_bool
#define LPTX_outtype_LPTX_array_get_sbi() int

/**
 * LPTX_bool entry to int (as boolean) array
 *
 * @note LPTX_particle cannot have LPTX_bool entry. Since they cannot be
 * communicated using MPI.
 */
#define LPTX_array_get_sbi() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_sbi)

//--- LPTX_bool -> char (as boolean) scalar getter

static void LPTX_array_get_sbc_f(void *outp, LPTX_idtype outidx,
                                 const void *inp, LPTX_idtype component_index,
                                 const struct LPTX_array_get_data *args)
{
  *((char *)outp + outidx) = ((*(const LPTX_bool *)inp) == LPTX_true) ? 1 : 0;
}

static void *LPTX_array_get_sbc_vh(void *outp, LPTX_idtype component_index,
                                   const struct LPTX_array_get_data *args)
{
  char *p = ((char **)outp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_sbc, LPTX_array_get_sbc_f,
                              LPTX_array_get_sbc_vh)

/**
 * LPTX_bool entry to char (as boolean) array
 *
 * @note LPTX_particle cannot have LPTX_bool entry. Since they cannot be
 * communicated using MPI.
 */
#define LPTX_array_get_sbc() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_sbc)
#define LPTX_inptype_LPTX_array_get_sbc() LPTX_bool
#define LPTX_outtype_LPTX_array_get_sbc() char

//--- LPTX_int -> int scalar getter

static void LPTX_array_get_sii_f(void *outp, LPTX_idtype outidx,
                                 const void *inp, LPTX_idtype component_index,
                                 const struct LPTX_array_get_data *args)
{
  *((int *)outp + outidx) = *(const LPTX_int *)inp;
}

static void *LPTX_array_get_sii_vh(void *outp, LPTX_idtype component_index,
                                   const struct LPTX_array_get_data *args)
{
  int *p = ((int **)outp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_sii, LPTX_array_get_sii_f,
                              LPTX_array_get_sii_vh)

/**
 * LPTX_int entry to int array
 */
#define LPTX_array_get_sii() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_sii)
#define LPTX_inptype_LPTX_array_get_sii() LPTX_int
#define LPTX_outtype_LPTX_array_get_sii() int

//--- uint64_t -> uint64_t scalar getter

static void LPTX_array_get_su64_f(void *outp, LPTX_idtype outidx,
                                  const void *inp, LPTX_idtype component_index,
                                  const struct LPTX_array_get_data *args)
{
  *((uint64_t *)outp + outidx) = *(const uint64_t *)inp;
}

static void *LPTX_array_get_su64_vh(void *outpp, LPTX_idtype component_index,
                                    const struct LPTX_array_get_data *args)
{
  uint64_t *p = ((uint64_t **)outpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_su64, LPTX_array_get_su64_f,
                              LPTX_array_get_su64_vh)

/**
 * uint64_t entry to uint64_t array
 */
#define LPTX_array_get_su64() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_su64)
#define LPTX_inptype_LPTX_array_get_su64() uint64_t
#define LPTX_outtype_LPTX_array_get_su64() uint64_t

//--- geom_bitarray_element_type -> geom_bitarray_element_type scalar getter

static void LPTX_array_get_sgb_f(void *outp, LPTX_idtype outidx,
                                 const void *inp, LPTX_idtype component_index,
                                 const struct LPTX_array_get_data *args)
{
  *((geom_bitarray_element_type *)outp + outidx) =
    *(const geom_bitarray_element_type *)inp;
}

static void *LPTX_array_get_sgb_vh(void *outp, LPTX_idtype component_index,
                                   const struct LPTX_array_get_data *args)
{
  geom_bitarray_element_type *p;
  p = ((geom_bitarray_element_type **)outp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_sgb, LPTX_array_get_sgb_f,
                              LPTX_array_get_sgb_vh)

/**
 * geom_bitarray_element_type to geom_bitarray_element array
 */
#define LPTX_array_get_sgb() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_sgb)
#define LPTX_inptype_LPTX_array_get_sgb() geom_bitarray_element_type
#define LPTX_outtype_LPTX_array_get_sgb() geom_bitarray_element_type

//--- LPTX_type -> LPTX_type scalar getter

static void LPTX_array_get_stt_f(void *outp, LPTX_idtype outidx,
                                 const void *inp, LPTX_idtype component_index,
                                 const struct LPTX_array_get_data *args)
{
  *((LPTX_type *)outp + outidx) = *(const LPTX_type *)inp;
}

static void *LPTX_array_get_stt_vh(void *outpp, LPTX_idtype component_index,
                                   const struct LPTX_array_get_data *args)
{
  LPTX_type *p = ((LPTX_type **)outpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_stt, LPTX_array_get_stt_f,
                              LPTX_array_get_stt_vh)

/**
 * LPTX_type entry to LPTX_type array
 */
#define LPTX_array_get_stt() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_stt)
#define LPTX_inptype_LPTX_array_get_stt() LPTX_type
#define LPTX_outtype_LPTX_array_get_stt() LPTX_type

static void LPTX_array_get_stt0_f(void *outp, LPTX_idtype outidx,
                                  const void *inp, LPTX_idtype component_index,
                                  const struct LPTX_array_get_data *args)
{
  if (inp) {
    LPTX_array_get_stt_f(outp, outidx, inp, component_index, args);
  } else {
    LPTX_type zero = LPTX_C(0.);
    LPTX_array_get_stt_f(outp, outidx, &zero, component_index, args);
  }
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_stt0, LPTX_array_get_stt0_f,
                              LPTX_array_get_stt_vh)

/**
 * LPTX_type entry to LPTX_type array, or fill 0 if no value exists
 */
#define LPTX_array_get_stt0() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_stt0)
#define LPTX_inptype_LPTX_array_get_stt0() LPTX_inptype_LPTX_array_get_stt()
#define LPTX_outtype_LPTX_array_get_stt0() LPTX_outtype_LPTX_array_get_stt()

//--- LPTX_type -> float scalar getter

static void LPTX_array_get_stf_f(void *outp, LPTX_idtype outidx,
                                 const void *inp, LPTX_idtype component_index,
                                 const struct LPTX_array_get_data *args)
{
  *((float *)outp + outidx) = *(const LPTX_type *)inp;
}

static void *LPTX_array_get_stf_vh(void *outpp, LPTX_idtype component_index,
                                   const struct LPTX_array_get_data *args)
{
  float *p = ((float **)outpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_stf, LPTX_array_get_stf_f,
                              LPTX_array_get_stf_vh)

/**
 * LPTX_type entry to float array
 */
#define LPTX_array_get_stf() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_stf)
#define LPTX_inptype_LPTX_array_get_stf() LPTX_type
#define LPTX_outtype_LPTX_array_get_stf() float

static void LPTX_array_get_stf0_f(void *outp, LPTX_idtype outidx,
                                  const void *inp, LPTX_idtype component_index,
                                  const struct LPTX_array_get_data *args)
{
  if (inp) {
    LPTX_array_get_stf_f(outp, outidx, inp, component_index, args);
  } else {
    LPTX_type zero = LPTX_C(0.);
    LPTX_array_get_stf_f(outp, outidx, &zero, component_index, args);
  }
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_stf0, LPTX_array_get_stf0_f,
                              LPTX_array_get_stf_vh)

/**
 * LPTX_type entry to float array, or fill 0 if no value exists
 */
#define LPTX_array_get_stf0() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_stf0)
#define LPTX_inptype_LPTX_array_get_stf0() LPTX_inptype_LPTX_array_get_stf()
#define LPTX_outtype_LPTX_array_get_stf0() LPTX_outtype_LPTX_array_get_stf()

//--- LPTX_type -> double scalar getter

static void LPTX_array_get_std_f(void *outp, LPTX_idtype outidx,
                                 const void *inp, LPTX_idtype component_index,
                                 const struct LPTX_array_get_data *args)
{
  *((double *)outp + outidx) = *(const LPTX_type *)inp;
}

static void *LPTX_array_get_std_vh(void *outpp, LPTX_idtype component_index,
                                   const struct LPTX_array_get_data *args)
{
  double *p = ((double **)outpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_std, LPTX_array_get_std_f,
                              LPTX_array_get_std_vh)

/**
 * LPTX_type entry to double array
 */
#define LPTX_array_get_std() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_std)
#define LPTX_inptype_LPTX_array_get_std() LPTX_type
#define LPTX_outtype_LPTX_array_get_std() double

static void LPTX_array_get_std0_f(void *outp, LPTX_idtype outidx,
                                  const void *inp, LPTX_idtype component_index,
                                  const struct LPTX_array_get_data *args)
{
  if (inp) {
    LPTX_array_get_std_f(outp, outidx, inp, component_index, args);
  } else {
    LPTX_type zero = LPTX_C(0.);
    LPTX_array_get_std_f(outp, outidx, &zero, component_index, args);
  }
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_std0, LPTX_array_get_std0_f,
                              LPTX_array_get_std_vh)

/**
 * LPTX_type entry to double array, or fill 0 if no values exists
 */
#define LPTX_array_get_std0() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_std0)
#define LPTX_inptype_LPTX_array_get_std0() LPTX_inptype_LPTX_array_get_std()
#define LPTX_outtype_LPTX_array_get_std0() LPTX_outtype_LPTX_array_get_std()

//--- LPTX_vector -> LPTX_vector scalar (as POD) getter

static void LPTX_array_get_svv_f(void *outp, LPTX_idtype outidx,
                                 const void *inp, LPTX_idtype component_index,
                                 const struct LPTX_array_get_data *args)
{
  *((LPTX_vector *)outp + outidx) = *(const LPTX_vector *)inp;
}

static void *LPTX_array_get_svv_vh(void *outpp, LPTX_idtype component_index,
                                   const struct LPTX_array_get_data *args)
{
  LPTX_vector *p = ((LPTX_vector **)outpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_svv, LPTX_array_get_svv_f,
                              LPTX_array_get_svv_vh)

/**
 * LPTX_vector entry to LPTX_vector array
 */
#define LPTX_array_get_svv() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_svv)
#define LPTX_inptype_LPTX_array_get_svv() LPTX_vector
#define LPTX_outtype_LPTX_array_get_svv() LPTX_vector

//--- jupiter_random_seed -> jupiter_random_seed scalar (POD) getter

static void LPTX_array_get_rndsv_f(void *outp, LPTX_idtype outidx,
                                   const void *inp, LPTX_idtype component_index,
                                   const struct LPTX_array_get_data *args)
{
  *((jupiter_random_seed *)outp + outidx) = *(const jupiter_random_seed *)inp;
}

static void *LPTX_array_get_rndsv_vh(void *outpp, LPTX_idtype component_index,
                                     const struct LPTX_array_get_data *args)
{
  jupiter_random_seed *p;
  p = ((jupiter_random_seed **)outpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_rndsv, LPTX_array_get_rndsv_f,
                              LPTX_array_get_rndsv_vh)

/**
 * jupiter_random_seed entry to jupiter_random_seed array
 */
#define LPTX_array_get_rndsv() LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_rndsv)
#define LPTX_inptype_LPTX_array_get_rndsv() jupiter_random_seed
#define LPTX_outtype_LPTX_array_get_rndsv() jupiter_random_seed

//--- LPTX_particle_flags -> LPTX_particle_flags scalar (POD) getter

static void LPTX_array_get_flagsv_f(void *outp, LPTX_idtype outidx,
                                    const void *inp,
                                    LPTX_idtype component_index,
                                    const struct LPTX_array_get_data *args)
{
  *((LPTX_particle_flags *)outp + outidx) = *(const LPTX_particle_flags *)inp;
}

static void *LPTX_array_get_flags_vh(void *outpp, LPTX_idtype component_index,
                                     const struct LPTX_array_get_data *args)
{
  LPTX_particle_flags *p;
  p = ((LPTX_particle_flags **)outpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_GET_S_TYPE0(LPTX_array_get_flagsv, LPTX_array_get_flagsv_f,
                              LPTX_array_get_flags_vh)

/**
 * LPTX_particle_flags entry to LPTX_particle_flags array
 */
#define LPTX_array_get_flagsv() \
  LPTX_ARRAY_GET_S_TYPE0_INIT(LPTX_array_get_flagsv)
#define LPTX_inptype_LPTX_array_get_flagsv() LPTX_particle_flags
#define LPTX_outtype_LPTX_array_get_flagsv() LPTX_particle_flags

//--- scalar setter

#define LPTX_DEFINE_ARRAY_SET_S_TYPE0(name, func, soahelper) \
  struct name##_data                                         \
  {                                                          \
    struct LPTX_array_set_data d;                            \
  };                                                         \
                                                             \
  static const struct LPTX_array_set_funcs name##_funcs = {  \
    .fn = LPTX_array_set_scalar_n,                           \
    .f = func,                                               \
    .vecsoa_helper = soahelper,                              \
  };

#define LPTX_ARRAY_SET_S_TYPE0_INIT(name) \
  (&(((struct name##_data){.d = {.funcs = &name##_funcs}}).d))

//--- LPTX_idtype -> LPTX_idtype scalar setter

static void LPTX_array_set_spp_f(void *outp, LPTX_idtype component_index,
                                 const void *inp, LPTX_idtype inidx,
                                 const struct LPTX_array_set_data *args)
{
  *(LPTX_idtype *)outp = *((const LPTX_idtype *)inp + inidx);
}

static const void *LPTX_array_set_spp_vh(const void *inpp,
                                         LPTX_idtype component_index,
                                         const struct LPTX_array_set_data *args)
{
  const LPTX_idtype *p = ((const LPTX_idtype **)inpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_spp, LPTX_array_set_spp_f,
                              LPTX_array_set_spp_vh)

/**
 * LPTX_idtype entry from LPTX_idtype array
 */
#define LPTX_array_set_spp() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_spp)
#define LPTX_inptype_LPTX_array_set_spp() LPTX_idtype
#define LPTX_outtype_LPTX_array_set_spp() LPTX_idtype

//--- int -> LPTX_idtype scalar setter

static void LPTX_array_set_spi_f(void *outp, LPTX_idtype component_index,
                                 const void *inp, LPTX_idtype inidx,
                                 const struct LPTX_array_set_data *args)
{
  *(LPTX_idtype *)outp = *((const int *)inp + inidx);
}

static const void *LPTX_array_set_spi_vh(const void *inpp,
                                         LPTX_idtype component_index,
                                         const struct LPTX_array_set_data *args)
{
  const int *p = ((const int **)inpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_spi, LPTX_array_set_spi_f,
                              LPTX_array_set_spi_vh)

/**
 * LPTX_idtype entry from int array
 */
#define LPTX_array_set_spi() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_spi)
#define LPTX_inptype_LPTX_array_set_spi() int
#define LPTX_outtype_LPTX_array_set_spi() LPTX_idtype

//--- LPTX_bool -> LPTX_int (as boolean) scalar setter

static void LPTX_array_set_sib_f(void *outp, LPTX_idtype component_index,
                                 const void *inp, LPTX_idtype inidx,
                                 const struct LPTX_array_set_data *args)
{
  *(LPTX_int *)outp = (*((const LPTX_bool *)inp + inidx) == LPTX_true) ? 1 : 0;
}

static const void *LPTX_array_set_sib_vh(const void *inpp,
                                         LPTX_idtype component_index,
                                         const struct LPTX_array_set_data *args)
{
  const LPTX_bool *p = ((const LPTX_bool **)inpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_sib, LPTX_array_set_sib_f,
                              LPTX_array_set_sib_vh)

/**
 * LPTX_int entry (as boolean) from LPTX_bool array
 */
#define LPTX_array_set_sib() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_sib)
#define LPTX_inptype_LPTX_array_set_sib() LPTX_bool
#define LPTX_outtype_LPTX_array_set_sib() LPTX_int

//--- LPTX_bool -> LPTX_bool scalar setter

static void LPTX_array_set_sbb_f(void *outp, LPTX_idtype component_index,
                                 const void *inp, LPTX_idtype inidx,
                                 const struct LPTX_array_set_data *args)
{
  *(LPTX_bool *)outp = *((const LPTX_bool *)inp + inidx);
}

static const void *LPTX_array_set_sbb_vh(const void *inpp,
                                         LPTX_idtype component_index,
                                         const struct LPTX_array_set_data *args)
{
  const LPTX_bool *p = ((const LPTX_bool **)inpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_sbb, LPTX_array_set_sbb_f,
                              LPTX_array_set_sbb_vh)

/**
 * LPTX_bool entry from LPTX_bool array
 *
 * @note LPTX_particle cannot have LPTX_bool entry. Since they cannot be
 * communicated via MPI.
 */
#define LPTX_array_set_sbb() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_sbb)
#define LPTX_inptype_LPTX_array_set_sbb() LPTX_bool
#define LPTX_outtype_LPTX_array_set_sbb() LPTX_bool

//--- int (as boolean) -> LPTX_bool scalar setter

static void LPTX_array_set_sbi_f(void *outp, LPTX_idtype component_index,
                                 const void *inp, LPTX_idtype inidx,
                                 const struct LPTX_array_set_data *args)
{
  *(LPTX_bool *)outp = (*((const int *)inp + inidx)) ? LPTX_true : LPTX_false;
}

static const void *LPTX_array_set_sbi_vh(const void *inpp,
                                         LPTX_idtype component_index,
                                         const struct LPTX_array_set_data *args)
{
  const int *p = ((const int **)inpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_sbi, LPTX_array_set_sbi_f,
                              LPTX_array_set_sbi_vh)

/**
 * LPTX_bool entry from int array
 *
 * @note LPTX_particle cannot have LPTX_bool entry. Since they cannot be
 * communicated via MPI.
 */
#define LPTX_array_set_sbi() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_sbi)
#define LPTX_inptype_LPTX_array_set_sbi() int
#define LPTX_outtype_LPTX_array_set_sbi() LPTX_bool

//--- char (as boolean) -> LPTX_bool scalar setter

static void LPTX_array_set_sbc_f(void *outp, LPTX_idtype component_index,
                                 const void *inp, LPTX_idtype inidx,
                                 const struct LPTX_array_set_data *args)
{
  *(LPTX_bool *)outp = (*((const char *)inp + inidx)) ? LPTX_true : LPTX_false;
}

static const void *LPTX_array_set_sbc_vh(const void *inpp,
                                         LPTX_idtype component_index,
                                         const struct LPTX_array_set_data *args)
{
  const char *p = ((const char **)inpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_sbc, LPTX_array_set_sbc_f,
                              LPTX_array_set_sbc_vh)

/**
 * LPTX_bool entry from char array
 *
 * @note LPTX_particle cannot have LPTX_bool entry. Since they cannot be
 * communicated via MPI.
 */
#define LPTX_array_set_sbc() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_sbc)
#define LPTX_inptype_LPTX_array_set_sbc() char
#define LPTX_outtype_LPTX_array_set_sbc() LPTX_bool

//--- int -> LPTX_int scalar setter

static void LPTX_array_set_sii_f(void *outp, LPTX_idtype component_index,
                                 const void *inp, LPTX_idtype inidx,
                                 const struct LPTX_array_set_data *args)
{
  *(LPTX_int *)outp = *((const int *)inp + inidx);
}

static const void *LPTX_array_set_sii_vh(const void *inpp,
                                         LPTX_idtype component_index,
                                         const struct LPTX_array_set_data *args)
{
  const int *p = ((const int **)inpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_sii, LPTX_array_set_sii_f,
                              LPTX_array_set_sii_vh)

/**
 * LPTX_int entry from int array
 */
#define LPTX_array_set_sii() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_sii)
#define LPTX_inptype_LPTX_array_set_sii() int
#define LPTX_outtype_LPTX_array_set_sii() LPTX_int

//--- uint64_t -> uint64_t scalar setter

static void LPTX_array_set_su64_f(void *outp, LPTX_idtype component_index,
                                  const void *inp, LPTX_idtype inidx,
                                  const struct LPTX_array_set_data *args)
{
  *(uint64_t *)outp = *((const uint64_t *)inp + inidx);
}

static const void *
LPTX_array_set_su64_vh(const void *inpp, LPTX_idtype component_index,
                       const struct LPTX_array_set_data *args)
{
  const uint64_t *p = ((const uint64_t **)inpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_su64, LPTX_array_set_su64_f,
                              LPTX_array_set_su64_vh)

/**
 * uint64_t entry from uint64_t array
 */
#define LPTX_array_set_su64() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_su64)
#define LPTX_inptype_LPTX_array_set_su64() uint64_t
#define LPTX_outtype_LPTX_array_set_su64() uint64_t

//--- geom_bitarray_element_type -> geom_bitarray_element_type setter

static void LPTX_array_set_sgb_f(void *outp, LPTX_idtype component_index,
                                 const void *inp, LPTX_idtype inidx,
                                 const struct LPTX_array_set_data *args)
{
  *(geom_bitarray_element_type *)outp =
    *((const geom_bitarray_element_type *)inp + inidx);
}

static const void *LPTX_array_set_sgb_vh(const void *inpp,
                                         LPTX_idtype component_index,
                                         const struct LPTX_array_set_data *args)
{
  const geom_bitarray_element_type *p;
  p = ((const geom_bitarray_element_type **)inpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_sgb, LPTX_array_set_sgb_f,
                              LPTX_array_set_sgb_vh)

/**
 * geom_bitarray_element_type entry from geom_bitarray_element_type array
 */
#define LPTX_array_set_sgb() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_sgb)
#define LPTX_inptype_LPTX_array_set_sgb() geom_bitarray_element_type
#define LPTX_outtype_LPTX_array_set_sgb() geom_bitarray_element_type

//--- LPTX_type -> LPTX_type scalar setter

static void LPTX_array_set_stt_f(void *outp, LPTX_idtype component_index,
                                 const void *inp, LPTX_idtype inidx,
                                 const struct LPTX_array_set_data *args)
{
  *(LPTX_type *)outp = *((const LPTX_type *)inp + inidx);
}

static const void *LPTX_array_set_stt_vh(const void *inpp,
                                         LPTX_idtype component_index,
                                         const struct LPTX_array_set_data *args)
{
  const LPTX_type *p = ((const LPTX_type **)inpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_stt, LPTX_array_set_stt_f,
                              LPTX_array_set_stt_vh)

/**
 * LPTX_type entry from LPTX_type array
 */
#define LPTX_array_set_stt() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_stt)
#define LPTX_inptype_LPTX_array_set_stt() LPTX_type
#define LPTX_outtype_LPTX_array_set_stt() LPTX_type

static void LPTX_array_set_stt0_f(void *outp, LPTX_idtype component_index,
                                  const void *inp, LPTX_idtype inidx,
                                  const struct LPTX_array_set_data *args)
{
  if (inp) {
    LPTX_array_set_stt_f(outp, component_index, inp, inidx, args);
  } else {
    LPTX_type zero = LPTX_C(0.);
    LPTX_array_set_stt_f(outp, component_index, &zero, 0, args);
  }
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_stt0, LPTX_array_set_stt0_f,
                              LPTX_array_set_stt_vh)

/**
 * LPTX_type entry from LPTX_type array, or 0 if array's missing (NULL).
 */
#define LPTX_array_set_stt0() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_stt0)
#define LPTX_inptype_LPTX_array_set_stt0() LPTX_inptype_LPTX_array_set_stt()
#define LPTX_outtype_LPTX_array_set_stt0() LPTX_outtype_LPTX_array_set_stt()

//--- float -> LPTX_type scalar setter

static void LPTX_array_set_stf_f(void *outp, LPTX_idtype component_index,
                                 const void *inp, LPTX_idtype inidx,
                                 const struct LPTX_array_set_data *args)
{
  *(LPTX_type *)outp = *((const float *)inp + inidx);
}

static const void *LPTX_array_set_stf_vh(const void *inpp,
                                         LPTX_idtype component_index,
                                         const struct LPTX_array_set_data *args)
{
  const float *p = ((const float **)inpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_stf, LPTX_array_set_stf_f,
                              LPTX_array_set_stf_vh)

/**
 * LPTX_type entry from float array
 */
#define LPTX_array_set_stf() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_stf)
#define LPTX_inptype_LPTX_array_set_stf() float
#define LPTX_outtype_LPTX_array_set_stf() LPTX_type

static void LPTX_array_set_stf0_f(void *outp, LPTX_idtype component_index,
                                  const void *inp, LPTX_idtype inidx,
                                  const struct LPTX_array_set_data *args)
{
  if (inp) {
    LPTX_array_set_stf_f(outp, component_index, inp, inidx, args);
  } else {
    float zero = 0.0f;
    LPTX_array_set_stf_f(outp, component_index, &zero, 0, args);
  }
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_stf0, LPTX_array_set_stf0_f,
                              LPTX_array_set_stf_vh)

/**
 * LPTX_type entry from float array, or 0 if array's missing (NULL)
 */
#define LPTX_array_set_stf0() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_stf0)
#define LPTX_inptype_LPTX_array_set_stf0() LPTX_inptype_LPTX_array_set_stf()
#define LPTX_outtype_LPTX_array_set_stf0() LPTX_outtype_LPTX_array_set_stf()

//--- double -> LPTX_type scalar setter

static void LPTX_array_set_std_f(void *outp, LPTX_idtype component_index,
                                 const void *inp, LPTX_idtype inidx,
                                 const struct LPTX_array_set_data *args)
{
  *(LPTX_type *)outp = *((const double *)inp + inidx);
}

static const void *LPTX_array_set_std_vh(const void *inpp,
                                         LPTX_idtype component_index,
                                         const struct LPTX_array_set_data *args)
{
  const double *p = ((const double **)inpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_std, LPTX_array_set_std_f,
                              LPTX_array_set_std_vh)

/**
 * LPTX_type entry from double array
 */
#define LPTX_array_set_std() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_std)
#define LPTX_inptype_LPTX_array_set_std() double
#define LPTX_outtype_LPTX_array_set_std() LPTX_type

static void LPTX_array_set_std0_f(void *outp, LPTX_idtype component_index,
                                  const void *inp, LPTX_idtype inidx,
                                  const struct LPTX_array_set_data *args)
{
  if (inp) {
    LPTX_array_set_std_f(outp, component_index, inp, inidx, args);
  } else {
    double zero = 0.0;
    LPTX_array_set_std_f(outp, component_index, &zero, 0, args);
  }
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_std0, LPTX_array_set_std0_f,
                              LPTX_array_set_std_vh)

/**
 * LPTX_type entry from double array, or 0 if array's missing (NULL)
 */
#define LPTX_array_set_std0() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_std0)
#define LPTX_inptype_LPTX_array_set_std0() LPTX_inptype_LPTX_array_set_std()
#define LPTX_outtype_LPTX_array_set_std0() LPTX_outtype_LPTX_array_set_std()

//--- LPTX_vector -> LPTX_vector scalar (as POD) setter

static void LPTX_array_set_svv_f(void *outp, LPTX_idtype component_index,
                                 const void *inp, LPTX_idtype inidx,
                                 const struct LPTX_array_set_data *args)
{
  *(LPTX_vector *)outp = *((const LPTX_vector *)inp + inidx);
}

static const void *LPTX_array_set_svv_vh(const void *inpp,
                                         LPTX_idtype component_index,
                                         const struct LPTX_array_set_data *args)
{
  const LPTX_vector *p = ((const LPTX_vector **)inpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_svv, LPTX_array_set_svv_f,
                              LPTX_array_set_svv_vh)

/**
 * LPTX_vector entry from LPTX_vector array
 */
#define LPTX_array_set_svv() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_svv)
#define LPTX_inptype_LPTX_array_set_svv() LPTX_vector
#define LPTX_outtype_LPTX_array_set_svv() LPTX_vector

//--- jupiter_radnom_seed -> jupiter_random_seed scalar (as POD) setter

static void LPTX_array_set_rndsv_f(void *outp, LPTX_idtype component_index,
                                   const void *inp, LPTX_idtype inidx,
                                   const struct LPTX_array_set_data *args)
{
  *(jupiter_random_seed *)outp = *((const jupiter_random_seed *)inp + inidx);
}

static const void *
LPTX_array_set_rndsv_vh(const void *inpp, LPTX_idtype component_index,
                        const struct LPTX_array_set_data *args)
{
  const jupiter_random_seed *p;
  p = ((const jupiter_random_seed **)inpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_rndsv, LPTX_array_set_rndsv_f,
                              LPTX_array_set_rndsv_vh)

/**
 * jupiter_random_seed entry from jupiter_random_seed array
 */
#define LPTX_array_set_rndsv() LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_rndsv)
#define LPTX_inptype_LPTX_array_set_rndsv() jupiter_random_seed
#define LPTX_outtype_LPTX_array_set_rndsv() jupiter_random_seed

//--- LPTX_particle_flags -> LPTX_particle_flags scalar (as POD) setter

static void LPTX_array_set_flagsv_f(void *outp, LPTX_idtype component_index,
                                    const void *inp, LPTX_idtype inidx,
                                    const struct LPTX_array_set_data *args)
{
  *(LPTX_particle_flags *)outp = *((const LPTX_particle_flags *)inp + inidx);
}

static const void *
LPTX_array_set_flagsv_vh(const void *inpp, LPTX_idtype component_index,
                         const struct LPTX_array_set_data *args)
{
  const LPTX_particle_flags *p;
  p = ((const LPTX_particle_flags **)inpp)[component_index];
  return p;
}

LPTX_DEFINE_ARRAY_SET_S_TYPE0(LPTX_array_set_flagsv, LPTX_array_set_flagsv_f,
                              LPTX_array_set_flagsv_vh)

/**
 * LPTX_particle_flags entry from LPTX_particle_flags array
 */
#define LPTX_array_set_flagsv() \
  LPTX_ARRAY_SET_S_TYPE0_INIT(LPTX_array_set_flagsv)
#define LPTX_inptype_LPTX_array_set_flagsv() LPTX_particle_flags
#define LPTX_outtype_LPTX_array_set_flagsv() LPTX_particle_flags

//--- vector using arbitrary scalar function

/**
 * Get to compile-time constant (i.e. assumed) sized vector
 */
#define LPTX_DEFINE_ARRAY_GET_VN_TYPE0(name, vnfunc, element_type_name)        \
  struct name##_data                                                           \
  {                                                                            \
    struct LPTX_array_get_data d;                                              \
  };                                                                           \
                                                                               \
  static inline void name##_f(void *outp, LPTX_idtype outidx, const void *inp, \
                              LPTX_idtype component_index,                     \
                              const struct LPTX_array_get_data *args)          \
  {                                                                            \
    LPTX_assert(component_index >= 0);                                         \
    LPTX_assert(component_index < vnfunc(args));                               \
    LPTX_array_get_call(outp, outidx, inp, component_index,                    \
                        element_type_name());                                  \
  }                                                                            \
                                                                               \
  static inline void *name##_vh(void *outpp, LPTX_idtype component_index,      \
                                const struct LPTX_array_get_data *data)        \
  {                                                                            \
    return LPTX_array_get_call_vecsoa_helper(outpp, component_index,           \
                                             element_type_name());             \
  }                                                                            \
                                                                               \
  static const struct LPTX_array_get_funcs name##_funcs = {                    \
    .fn = vnfunc,                                                              \
    .f = name##_f,                                                             \
    .vecsoa_helper = name##_vh,                                                \
  };

#define LPTX_ARRAY_GET_VN_TYPE0_INIT(name) \
  (&(((struct name##_data){.d = {.funcs = &name##_funcs}}).d))

/**
 * Set from compile-time constant (i.e. assumed) sized vector
 */
#define LPTX_DEFINE_ARRAY_SET_VN_TYPE0(name, vnfunc, element_type_name)       \
  struct name##_data                                                          \
  {                                                                           \
    struct LPTX_array_set_data d;                                             \
  };                                                                          \
                                                                              \
  static inline void name##_f(void *outp, LPTX_idtype component_index,        \
                              const void *inp, LPTX_idtype inidx,             \
                              const struct LPTX_array_set_data *args)         \
  {                                                                           \
    LPTX_assert(component_index >= 0);                                        \
    LPTX_assert(component_index < vnfunc(args));                              \
    LPTX_array_set_call(outp, component_index, inp, inidx,                    \
                        element_type_name());                                 \
  }                                                                           \
                                                                              \
  static inline const void *name##_vh(const void *inpp,                       \
                                      LPTX_idtype component_index,            \
                                      const struct LPTX_array_set_data *data) \
  {                                                                           \
    return LPTX_array_set_call_vecsoa_helper(inpp, component_index,           \
                                             element_type_name());            \
  }                                                                           \
                                                                              \
  static const struct LPTX_array_set_funcs name##_funcs = {                   \
    .fn = vnfunc,                                                             \
    .f = name##_f,                                                            \
    .vecsoa_helper = name##_vh,                                               \
  };

#define LPTX_ARRAY_SET_VN_TYPE0_INIT(name) \
  (&(((struct name##_data){.d = {.funcs = &name##_funcs}}).d))

/**
 * Get to variable sized vector
 */
#define LPTX_DEFINE_ARRAY_GET_VV_TYPE0(name, element_type_name)                \
  struct name##_data                                                           \
  {                                                                            \
    struct LPTX_array_get_variable_comps d;                                    \
  };                                                                           \
                                                                               \
  static inline void name##_f(void *outp, LPTX_idtype outidx, const void *inp, \
                              LPTX_idtype component_index,                     \
                              const struct LPTX_array_get_data *args)          \
  {                                                                            \
    LPTX_array_get_call(outp, outidx, inp, component_index,                    \
                        element_type_name());                                  \
  }                                                                            \
                                                                               \
  static inline void *name##_vh(void *outpp, LPTX_idtype component_index,      \
                                const struct LPTX_array_get_data *args)        \
  {                                                                            \
    return LPTX_array_get_call_vecsoa_helper(outpp, component_index,           \
                                             element_type_name());             \
  }                                                                            \
                                                                               \
  static const struct LPTX_array_get_funcs name##_funcs = {                    \
    .fn = LPTX_array_get_variable_comps_n,                                     \
    .f = name##_f,                                                             \
    .vecsoa_helper = name##_vh,                                                \
  };

#define LPTX_ARRAY_GET_VV_TYPE0_INIT(name, ncomp) \
  (&(((struct name##_data){                       \
        .d =                                      \
          {                                       \
            .d = {.funcs = &name##_funcs},        \
            .number_of_components = ncomp,        \
          },                                      \
      })                                          \
       .d.d))

/**
 * Set from variable sized vector
 */
#define LPTX_DEFINE_ARRAY_SET_VV_TYPE0(name, element_type_name)               \
  struct name##_data                                                          \
  {                                                                           \
    struct LPTX_array_set_variable_comps d;                                   \
  };                                                                          \
                                                                              \
  static inline void name##_f(void *outp, LPTX_idtype component_index,        \
                              const void *inp, LPTX_idtype inidx,             \
                              const struct LPTX_array_set_data *args)         \
  {                                                                           \
    LPTX_array_set_call(outp, component_index, inp, inidx,                    \
                        element_type_name());                                 \
  }                                                                           \
                                                                              \
  static inline const void *name##_vh(const void *inpp,                       \
                                      LPTX_idtype component_index,            \
                                      const struct LPTX_array_set_data *args) \
  {                                                                           \
    return LPTX_array_set_call_vecsoa_helper(inpp, component_index,           \
                                             element_type_name());            \
  }                                                                           \
                                                                              \
  static const struct LPTX_array_set_funcs name##_funcs = {                   \
    .fn = LPTX_array_set_variable_comps_n,                                    \
    .f = name##_f,                                                            \
    .vecsoa_helper = name##_vh,                                               \
  };

#define LPTX_ARRAY_SET_VV_TYPE0_INIT(name, ncomp) \
  (&(((struct name##_data){                       \
        .d =                                      \
          {                                       \
            .d = {.funcs = &name##_funcs},        \
            .number_of_components = ncomp,        \
          },                                      \
      })                                          \
       .d.d))

//--- Assuming extracting scalar elements from LPTX_vector

#define LPTX_DEFINE_ARRAY_GET_V_TYPE0(name, element_type_name)  \
  LPTX_DEFINE_ARRAY_GET_VN_TYPE0(name, LPTX_array_get_vector_n, \
                                 element_type_name)

#define LPTX_ARRAY_GET_V_TYPE0_INIT(name) LPTX_ARRAY_GET_VN_TYPE0_INIT(name)

#define LPTX_DEFINE_ARRAY_SET_V_TYPE0(name, element_type_name)  \
  LPTX_DEFINE_ARRAY_SET_VN_TYPE0(name, LPTX_array_set_vector_n, \
                                 element_type_name)

#define LPTX_ARRAY_SET_V_TYPE0_INIT(name) LPTX_ARRAY_SET_VN_TYPE0_INIT(name)

LPTX_DEFINE_ARRAY_GET_V_TYPE0(LPTX_array_get_vvt, LPTX_array_get_stt)
LPTX_DEFINE_ARRAY_GET_V_TYPE0(LPTX_array_get_vvd, LPTX_array_get_std)
LPTX_DEFINE_ARRAY_GET_V_TYPE0(LPTX_array_get_vvf, LPTX_array_get_stf)
LPTX_DEFINE_ARRAY_SET_V_TYPE0(LPTX_array_set_vvt, LPTX_array_set_stt)
LPTX_DEFINE_ARRAY_SET_V_TYPE0(LPTX_array_set_vvd, LPTX_array_set_std)
LPTX_DEFINE_ARRAY_SET_V_TYPE0(LPTX_array_set_vvf, LPTX_array_set_stf)

#define LPTX_array_get_vvt() LPTX_ARRAY_GET_V_TYPE0_INIT(LPTX_array_get_vvt)
#define LPTX_array_get_vvd() LPTX_ARRAY_GET_V_TYPE0_INIT(LPTX_array_get_vvd)
#define LPTX_array_get_vvf() LPTX_ARRAY_GET_V_TYPE0_INIT(LPTX_array_get_vvf)
#define LPTX_array_set_vvt() LPTX_ARRAY_SET_V_TYPE0_INIT(LPTX_array_set_vvt)
#define LPTX_array_set_vvd() LPTX_ARRAY_SET_V_TYPE0_INIT(LPTX_array_set_vvd)
#define LPTX_array_set_vvf() LPTX_ARRAY_SET_V_TYPE0_INIT(LPTX_array_set_vvf)

#define LPTX_inptype_LPTX_array_get_vvt() LPTX_inptype_LPTX_array_get_stt()
#define LPTX_inptype_LPTX_array_get_vvd() LPTX_inptype_LPTX_array_get_std()
#define LPTX_inptype_LPTX_array_get_vvf() LPTX_inptype_LPTX_array_get_stf()
#define LPTX_inptype_LPTX_array_get_vvt() LPTX_inptype_LPTX_array_get_stt()
#define LPTX_inptype_LPTX_array_get_vvd() LPTX_inptype_LPTX_array_get_std()
#define LPTX_inptype_LPTX_array_get_vvf() LPTX_inptype_LPTX_array_get_stf()
#define LPTX_outtype_LPTX_array_get_vvt() LPTX_outtype_LPTX_array_get_stt()
#define LPTX_outtype_LPTX_array_get_vvd() LPTX_outtype_LPTX_array_get_std()
#define LPTX_outtype_LPTX_array_get_vvf() LPTX_outtype_LPTX_array_get_stf()
#define LPTX_outtype_LPTX_array_get_vvt() LPTX_outtype_LPTX_array_get_stt()
#define LPTX_outtype_LPTX_array_get_vvd() LPTX_outtype_LPTX_array_get_std()
#define LPTX_outtype_LPTX_array_get_vvf() LPTX_outtype_LPTX_array_get_stf()

#define LPTX_inptype_LPTX_array_set_vvt() LPTX_inptype_LPTX_array_set_stt()
#define LPTX_inptype_LPTX_array_set_vvd() LPTX_inptype_LPTX_array_set_std()
#define LPTX_inptype_LPTX_array_set_vvf() LPTX_inptype_LPTX_array_set_stf()
#define LPTX_inptype_LPTX_array_set_vvt() LPTX_inptype_LPTX_array_set_stt()
#define LPTX_inptype_LPTX_array_set_vvd() LPTX_inptype_LPTX_array_set_std()
#define LPTX_inptype_LPTX_array_set_vvf() LPTX_inptype_LPTX_array_set_stf()
#define LPTX_outtype_LPTX_array_set_vvt() LPTX_outtype_LPTX_array_set_stt()
#define LPTX_outtype_LPTX_array_set_vvd() LPTX_outtype_LPTX_array_set_std()
#define LPTX_outtype_LPTX_array_set_vvf() LPTX_outtype_LPTX_array_set_stf()
#define LPTX_outtype_LPTX_array_set_vvt() LPTX_outtype_LPTX_array_set_stt()
#define LPTX_outtype_LPTX_array_set_vvd() LPTX_outtype_LPTX_array_set_std()
#define LPTX_outtype_LPTX_array_set_vvf() LPTX_outtype_LPTX_array_set_stf()

//--- Assuming extracting bits of LPTX_particle_flags

#define LPTX_DEFINE_ARRAY_GET_FLAGS_TYPE0(name, element_type_name) \
  LPTX_DEFINE_ARRAY_GET_VN_TYPE0(name, LPTX_array_get_flags_n,     \
                                 element_type_name)

#define LPTX_ARRAY_GET_FLAGS_TYPE0_INIT(name) LPTX_ARRAY_GET_VN_TYPE0_INIT(name)

#define LPTX_DEFINE_ARRAY_SET_FLAGS_TYPE0(name, element_type_name) \
  LPTX_DEFINE_ARRAY_SET_VN_TYPE0(name, LPTX_array_set_flags_n,     \
                                 element_type_name)

#define LPTX_ARRAY_SET_FLAGS_TYPE0_INIT(name) LPTX_ARRAY_SET_VN_TYPE0_INIT(name)

LPTX_DEFINE_ARRAY_GET_FLAGS_TYPE0(LPTX_array_get_flagsb, LPTX_array_get_sbb)
LPTX_DEFINE_ARRAY_GET_FLAGS_TYPE0(LPTX_array_get_flagsc, LPTX_array_get_sbc)
LPTX_DEFINE_ARRAY_GET_FLAGS_TYPE0(LPTX_array_get_flagsi, LPTX_array_get_sbi)
LPTX_DEFINE_ARRAY_SET_FLAGS_TYPE0(LPTX_array_set_flagsb, LPTX_array_set_sbb)
LPTX_DEFINE_ARRAY_SET_FLAGS_TYPE0(LPTX_array_set_flagsc, LPTX_array_set_sbc)
LPTX_DEFINE_ARRAY_SET_FLAGS_TYPE0(LPTX_array_set_flagsi, LPTX_array_set_sbi)

#define LPTX_array_get_flagsb() \
  LPTX_ARRAY_GET_FLAGS_TYPE0_INIT(LPTX_array_get_flagsb)
#define LPTX_array_get_flagsc() \
  LPTX_ARRAY_GET_FLAGS_TYPE0_INIT(LPTX_array_get_flagsc)
#define LPTX_array_get_flagsi() \
  LPTX_ARRAY_GET_FLAGS_TYPE0_INIT(LPTX_array_get_flagsi)

#define LPTX_array_set_flagsb() \
  LPTX_ARRAY_SET_FLAGS_TYPE0_INIT(LPTX_array_set_flagsb)
#define LPTX_array_set_flagsc() \
  LPTX_ARRAY_SET_FLAGS_TYPE0_INIT(LPTX_array_set_flagsc)
#define LPTX_array_set_flagsi() \
  LPTX_ARRAY_SET_FLAGS_TYPE0_INIT(LPTX_array_set_flagsi)

#define LPTX_inptype_LPTX_array_get_flagsb() LPTX_inptype_LPTX_array_get_sbb()
#define LPTX_inptype_LPTX_array_get_flagsc() LPTX_inptype_LPTX_array_get_sbc()
#define LPTX_inptype_LPTX_array_get_flagsi() LPTX_inptype_LPTX_array_get_sbi()
#define LPTX_outtype_LPTX_array_get_flagsb() LPTX_outtype_LPTX_array_get_sbb()
#define LPTX_outtype_LPTX_array_get_flagsc() LPTX_outtype_LPTX_array_get_sbc()
#define LPTX_outtype_LPTX_array_get_flagsi() LPTX_outtype_LPTX_array_get_sbi()

#define LPTX_inptype_LPTX_array_set_flagsb() LPTX_inptype_LPTX_array_set_sbb()
#define LPTX_inptype_LPTX_array_set_flagsc() LPTX_inptype_LPTX_array_set_sbc()
#define LPTX_inptype_LPTX_array_set_flagsi() LPTX_inptype_LPTX_array_set_sbi()
#define LPTX_outtype_LPTX_array_set_flagsb() LPTX_outtype_LPTX_array_set_sbb()
#define LPTX_outtype_LPTX_array_set_flagsc() LPTX_outtype_LPTX_array_set_sbc()
#define LPTX_outtype_LPTX_array_set_flagsi() LPTX_outtype_LPTX_array_set_sbi()

//--- Assuming extracting raw elements of LPTX_particle_flags

#define LPTX_DEFINE_ARRAY_GET_FLAGSP_TYPE0(name, element_type_name) \
  LPTX_DEFINE_ARRAY_GET_VN_TYPE0(name, LPTX_array_get_flagse_n,     \
                                 element_type_name)

#define LPTX_ARRAY_GET_FLAGSP_TYPE0_INIT(name) \
  LPTX_ARRAY_GET_VN_TYPE0_INIT(name)

#define LPTX_DEFINE_ARRAY_SET_FLAGSP_TYPE0(name, element_type_name) \
  LPTX_DEFINE_ARRAY_SET_VN_TYPE0(name, LPTX_array_set_flagse_n,     \
                                 element_type_name)

#define LPTX_ARRAY_SET_FLAGSP_TYPE0_INIT(name) \
  LPTX_ARRAY_SET_VN_TYPE0_INIT(name)

LPTX_DEFINE_ARRAY_GET_FLAGSP_TYPE0(LPTX_array_get_flagsp, LPTX_array_get_sgb)
LPTX_DEFINE_ARRAY_SET_FLAGSP_TYPE0(LPTX_array_set_flagsp, LPTX_array_set_sgb)

#define LPTX_array_get_flagsp() \
  LPTX_ARRAY_GET_FLAGSP_TYPE0_INIT(LPTX_array_get_flagsp)

#define LPTX_array_set_flagsp() \
  LPTX_ARRAY_SET_FLAGSP_TYPE0_INIT(LPTX_array_set_flagsp)

#define LPTX_inptype_LPTX_array_get_flagsp() LPTX_inptype_LPTX_array_get_sgb()
#define LPTX_inptype_LPTX_array_set_flagsp() LPTX_inptype_LPTX_array_set_sgb()
#define LPTX_outtype_LPTX_array_get_flagsp() LPTX_outtype_LPTX_array_get_sgb()
#define LPTX_outtype_LPTX_array_set_flagsp() LPTX_outtype_LPTX_array_set_sgb()

//--- Assuming extracting raw elements of jupiter_random_seed

#define LPTX_DEFINE_ARRAY_GET_SEEDI_TYPE0(name, element_type_name) \
  LPTX_DEFINE_ARRAY_GET_VN_TYPE0(name, LPTX_array_get_seedi_n,     \
                                 element_type_name)

#define LPTX_ARRAY_GET_SEEDI_TYPE0_INIT(name) LPTX_ARRAY_GET_VN_TYPE0_INIT(name)

#define LPTX_DEFINE_ARRAY_SET_SEEDI_TYPE0(name, element_type_name) \
  LPTX_DEFINE_ARRAY_SET_VN_TYPE0(name, LPTX_array_set_seedi_n,     \
                                 element_type_name)

#define LPTX_ARRAY_SET_SEEDI_TYPE0_INIT(name) LPTX_ARRAY_SET_VN_TYPE0_INIT(name)

LPTX_DEFINE_ARRAY_GET_SEEDI_TYPE0(LPTX_array_get_seedi, LPTX_array_get_su64)
LPTX_DEFINE_ARRAY_SET_SEEDI_TYPE0(LPTX_array_set_seedi, LPTX_array_set_su64)

#define LPTX_array_get_seedi() \
  LPTX_ARRAY_GET_SEEDI_TYPE0_INIT(LPTX_array_get_seedi)
#define LPTX_array_set_seedi() \
  LPTX_ARRAY_SET_SEEDI_TYPE0_INIT(LPTX_array_set_seedi)

#define LPTX_inptype_LPTX_array_get_seedi() LPTX_inptype_LPTX_array_get_su64()
#define LPTX_inptype_LPTX_array_set_seedi() LPTX_inptype_LPTX_array_set_su64()
#define LPTX_outtype_LPTX_array_get_seedi() LPTX_outtype_LPTX_array_get_su64()
#define LPTX_outtype_LPTX_array_set_seedi() LPTX_outtype_LPTX_array_set_su64()

//--- Extracts elements of variable-sized array

LPTX_DEFINE_ARRAY_GET_VV_TYPE0(LPTX_array_get_pvt, LPTX_array_get_stt0)
LPTX_DEFINE_ARRAY_GET_VV_TYPE0(LPTX_array_get_pvd, LPTX_array_get_std0)
LPTX_DEFINE_ARRAY_GET_VV_TYPE0(LPTX_array_get_pvf, LPTX_array_get_stf0)
LPTX_DEFINE_ARRAY_SET_VV_TYPE0(LPTX_array_set_pvt, LPTX_array_set_stt0)
LPTX_DEFINE_ARRAY_SET_VV_TYPE0(LPTX_array_set_pvd, LPTX_array_set_std0)
LPTX_DEFINE_ARRAY_SET_VV_TYPE0(LPTX_array_set_pvf, LPTX_array_set_stf0)

#define LPTX_array_get_pvt(number_of_components) \
  LPTX_ARRAY_GET_VV_TYPE0_INIT(LPTX_array_get_pvt, number_of_components)
#define LPTX_array_get_pvd(number_of_components) \
  LPTX_ARRAY_GET_VV_TYPE0_INIT(LPTX_array_get_pvd, number_of_components)
#define LPTX_array_get_pvf(number_of_components) \
  LPTX_ARRAY_GET_VV_TYPE0_INIT(LPTX_array_get_pvf, number_of_components)

#define LPTX_array_set_pvt(number_of_components) \
  LPTX_ARRAY_SET_VV_TYPE0_INIT(LPTX_array_set_pvt, number_of_components)
#define LPTX_array_set_pvd(number_of_components) \
  LPTX_ARRAY_SET_VV_TYPE0_INIT(LPTX_array_set_pvd, number_of_components)
#define LPTX_array_set_pvf(number_of_components) \
  LPTX_ARRAY_SET_VV_TYPE0_INIT(LPTX_array_set_pvf, number_of_components)

#define LPTX_inptype_LPTX_array_get_pvt(n) LPTX_inptype_LPTX_array_get_stt0()
#define LPTX_inptype_LPTX_array_set_pvt(n) LPTX_inptype_LPTX_array_set_stt0()
#define LPTX_outtype_LPTX_array_get_pvt(n) LPTX_outtype_LPTX_array_get_stt0()
#define LPTX_outtype_LPTX_array_set_pvt(n) LPTX_outtype_LPTX_array_set_stt0()

#define LPTX_inptype_LPTX_array_get_pvd(n) LPTX_inptype_LPTX_array_get_std0()
#define LPTX_inptype_LPTX_array_set_pvd(n) LPTX_inptype_LPTX_array_set_std0()
#define LPTX_outtype_LPTX_array_get_pvd(n) LPTX_outtype_LPTX_array_get_std0()
#define LPTX_outtype_LPTX_array_set_pvd(n) LPTX_outtype_LPTX_array_set_std0()

#define LPTX_inptype_LPTX_array_get_pvf(n) LPTX_inptype_LPTX_array_get_stf0()
#define LPTX_inptype_LPTX_array_set_pvf(n) LPTX_inptype_LPTX_array_set_stf0()
#define LPTX_outtype_LPTX_array_get_pvf(n) LPTX_outtype_LPTX_array_get_stf0()
#define LPTX_outtype_LPTX_array_set_pvf(n) LPTX_outtype_LPTX_array_set_stf0()

JUPITER_LPTX_DECL_END

#endif
