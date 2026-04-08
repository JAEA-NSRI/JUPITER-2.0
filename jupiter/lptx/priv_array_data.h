#ifndef JUPITER_LPTX_PRIV_ARRAY_DATA_H
#define JUPITER_LPTX_PRIV_ARRAY_DATA_H

#include "defs.h"
#include "error.h"
#include "jupiter/geometry/util.h"
#include "particle.h"
#include "priv_struct_defs.h"
#include "priv_util.h"
#include "ptflags.h"
#include "util.h"

JUPITER_LPTX_DECL_START

/*
 * Core implementation of API for extract/assign one of properties of particle
 * data to/from flat plain array.
 *
 * This file is private header.
 */

//--- Array-side interface

struct LPTX_array_get_data;
struct LPTX_array_set_data;

/**
 * @param args extra arguement data
 * @return Number of components in multicomponent (ex. vector) data of
 *         destination array(s)
 */
typedef size_t LPTX_array_getn(const struct LPTX_array_get_data *args);

/**
 * @param outp Output array
 * @param outidx Index of output array which should be written
 * @param inp Pointer to read from (e.g. member of LPTX_particle)
 * @param component_index Component index should be copy
 * @param args extra argument data
 *
 * @note @p inp may be NULL if no data exists at @p component_index.
 *
 * @p component_index is just provided for informational purpose. In most cases,
 * it not required. @p outidx already counted @p component_index for SOA or AOS
 * vectors.
 */
typedef void LPTX_array_get(void *outp, LPTX_idtype outidx, const void *inp,
                            LPTX_idtype component_index,
                            const struct LPTX_array_get_data *args);

/**
 * @brief Get array from SOA arrays
 * @param outp SOA vector parameter (i.e., array of pointers)
 * @param component_index Component index to get
 *
 * @p outp will be passed array of pointer to destination type (e.g., int **).
 * Return pointer to array of given component_index (e.g., int *).
 *
 * While `void **` and `int **` (for example) are not compatible, using `void *`
 * instead. Be careful.
 *
 * Example implementation:
 *
 * ```c
 * void *foo_vecsoa_helper(void *inpp, LPTX_idtype component_index,
 *                         const struct LPTX_array_set_data *args)
 * {
 *   return ((int **)inpp)[component_index];
 * }
 * ```
 */
typedef void *
LPTX_array_get_vecsoa_helper(void *outpp, LPTX_idtype component_index,
                             const struct LPTX_array_get_data *args);

/**
 * @param args extra argument data
 * @return Number of components in multicomponent (ex. vector) data of
 *         source array(s)
 */
typedef size_t LPTX_array_setn(const struct LPTX_array_set_data *args);

/**
 * @param outp Pointer to store to (e.g. member of LPTX_particle)
 * @param component_index Component index should be copy
 * @param inp Input array
 * @param inidx Index of input array which should be read from
 * @param args extra argument data
 *
 * @note @p inp may be NULL if no data supplied for @p component_index.
 *
 * @p component_index is just provided for informational purpose. In most cases,
 * it not required. @p outidx already counted @p component_index for SOA or AOS
 * vectors.
 */
typedef void LPTX_array_set(void *outp, LPTX_idtype component_index,
                            const void *inp, LPTX_idtype inidx,
                            const struct LPTX_array_set_data *args);

/**
 * @brief Get array from SOA arrays
 * @param inpp SOA vector parameter (i.e., array of pointers)
 * @param component_index Component index to get
 *
 * @p outp will be passed array of pointer to destination type (e.g., const int
 * **).  Return pointer to array of given component_index (e.g., const int *).
 *
 * While `const void **` and `const int **` (for example) are not compatible,
 * using `const void *` instead. Be careful.
 *
 * Example implementation:
 *
 * ```c
 * const void *foo_vecsoa_helper(const void *inpp, LPTX_idtype component_index,
 *                               const struct LPTX_array_set_data *args)
 * {
 *   return ((const int **)outp)[component_index];
 * }
 * ```
 */
typedef const void *
LPTX_array_set_vecsoa_helper(const void *inpp, LPTX_idtype component_index,
                             const struct LPTX_array_set_data *args);

/* function table data */

struct LPTX_array_get_funcs
{
  LPTX_array_getn *const fn;
  LPTX_array_get *const f;
  LPTX_array_get_vecsoa_helper *const vecsoa_helper;
};

struct LPTX_array_set_funcs
{
  LPTX_array_setn *const fn;
  LPTX_array_set *const f;
  LPTX_array_set_vecsoa_helper *const vecsoa_helper;
};

/* base argument and callback data */

struct LPTX_array_get_data
{
  const struct LPTX_array_get_funcs *funcs;
};

struct LPTX_array_set_data
{
  const struct LPTX_array_set_funcs *funcs;
};

/**
 * @param data extra arguement data
 * @return Number of components in multicomponent (ex. vector) data
 */
static inline size_t
LPTX_array_get_calln(const struct LPTX_array_get_data *data)
{
  return data->funcs->fn(data);
}

static inline void LPTX_array_get_call(void *outp, LPTX_idtype outidx,
                                       const void *inp,
                                       LPTX_idtype component_index,
                                       const struct LPTX_array_get_data *data)
{
  data->funcs->f(outp, outidx, inp, component_index, data);
}

static inline void *
LPTX_array_get_call_vecsoa_helper(void *outp, LPTX_idtype component_index,
                                  const struct LPTX_array_get_data *data)
{
  return data->funcs->vecsoa_helper(outp, component_index, data);
}

/**
 * @param data extra argument data
 * @return Number of components in multicomponent (ex. vector) data
 */
static inline size_t
LPTX_array_set_calln(const struct LPTX_array_set_data *data)
{
  return data->funcs->fn(data);
}

static inline void LPTX_array_set_call(void *outp, LPTX_idtype component_index,
                                       const void *inp, LPTX_idtype inidx,
                                       const struct LPTX_array_set_data *data)
{
  data->funcs->f(outp, component_index, inp, inidx, data);
}

static inline const void *
LPTX_array_set_call_vecsoa_helper(const void *inp, LPTX_idtype component_index,
                                  const struct LPTX_array_set_data *data)
{
  return data->funcs->vecsoa_helper(inp, component_index, data);
}

//---- boxed array interface arguments

struct LPTX_array_geti_data
{
  void *const outp;
  const LPTX_idtype outidx;
  const LPTX_idtype component_index;
  const struct LPTX_array_get_data *const data;
};

/**
 * Assign value from @p particle_value to array specified by @p data
 */
static inline void LPTX_array_geti_call(const struct LPTX_array_geti_data *data,
                                        const void *particle_value)
{
  LPTX_array_get_call(data->outp, data->outidx, particle_value,
                      data->component_index, data->data);
}

static inline struct LPTX_array_geti_data
LPTX_array_geti_box(void *outp, LPTX_idtype outidx, LPTX_idtype component_index,
                    const struct LPTX_array_get_data *data)
{
  return (struct LPTX_array_geti_data){
    .outp = outp,
    .outidx = outidx,
    .component_index = component_index,
    .data = data,
  };
}

struct LPTX_array_seti_data
{
  const void *const inp;
  const LPTX_idtype inidx;
  const LPTX_idtype component_index;
  const struct LPTX_array_set_data *const data;
};

/**
 * Assign value from array specified by @p data to @p particle_value
 */
static inline void LPTX_array_seti_call(const struct LPTX_array_seti_data *data,
                                        void *particle_value)
{
  LPTX_array_set_call(particle_value, data->component_index, data->inp,
                      data->inidx, data->data);
}

static inline struct LPTX_array_seti_data
LPTX_array_seti_box(const void *inp, LPTX_idtype inidx,
                    LPTX_idtype component_index,
                    const struct LPTX_array_set_data *data)
{
  return (struct LPTX_array_seti_data){
    .inp = inp,
    .inidx = inidx,
    .component_index = component_index,
    .data = data,
  };
}

//---- particle-side interface

struct LPTX_particle_get_data;
struct LPTX_particle_set_data;

/**
 * @param args extra arguement data
 * @return Number of components in multicomponent (ex. vector) data for
 *         a member of given particle
 */
typedef size_t LPTX_particle_getn(const LPTX_particle_data *particle,
                                  const struct LPTX_particle_get_data *args);

/**
 * @param particle Particle location to obtain from
 */
typedef void LPTX_particle_getp(const LPTX_particle_data *particle,
                                LPTX_idtype component_index,
                                const struct LPTX_particle_get_data *pdata,
                                const struct LPTX_array_geti_data *adata);

/**
 * @param args extra arguement data
 * @return Number of components in multicomponent (ex. vector) data for
 *         a member of given particle
 */
typedef size_t LPTX_particle_setn(const LPTX_particle_data *particle,
                                  const struct LPTX_particle_set_data *args);

/**
 * @param particle Particle location to store to
 */
typedef void LPTX_particle_setp(LPTX_particle_data *particle,
                                LPTX_idtype component_index,
                                const struct LPTX_particle_set_data *pdata,
                                const struct LPTX_array_seti_data *adata);

/* function table data */

struct LPTX_particle_get_funcs
{
  LPTX_particle_getn *const fn;
  LPTX_particle_getp *const f;
};

struct LPTX_particle_set_funcs
{
  LPTX_particle_setn *const fn;
  LPTX_particle_setp *const f;
};

/* base argument and callback data */

struct LPTX_particle_get_data
{
  const struct LPTX_particle_get_funcs *funcs;
};

struct LPTX_particle_set_data
{
  const struct LPTX_particle_set_funcs *funcs;
};

static inline size_t
LPTX_particle_get_calln(const LPTX_particle_data *particle,
                        const struct LPTX_particle_get_data *data)
{
  return data->funcs->fn(particle, data);
}

static inline void
LPTX_particle_get_call(const LPTX_particle_data *particle,
                       LPTX_idtype component_index,
                       const struct LPTX_array_geti_data *adata,
                       const struct LPTX_particle_get_data *pdata)
{
  pdata->funcs->f(particle, component_index, pdata, adata);
}

static inline size_t
LPTX_particle_set_calln(const LPTX_particle_data *particle,
                        const struct LPTX_particle_set_data *data)
{
  return data->funcs->fn(particle, data);
}

static inline void
LPTX_particle_set_call(LPTX_particle_data *particle,
                       LPTX_idtype component_index,
                       const struct LPTX_array_seti_data *adata,
                       const struct LPTX_particle_set_data *pdata)
{
  pdata->funcs->f(particle, component_index, pdata, adata);
}

//----

/**
 * @param outp Output array
 * @param outidx Index of Output array which should be written
 * @param particle Particle to get from
 * @param data extra argument data
 */
static inline void
LPTX_array_get_scl(void *outp, LPTX_idtype outidx,
                   const LPTX_particle_data *particle,
                   const struct LPTX_array_get_data *adata,
                   const struct LPTX_particle_get_data *pdata)
{
  struct LPTX_array_geti_data d = LPTX_array_geti_box(outp, outidx, 0, adata);

  LPTX_assert(LPTX_array_get_calln(adata) == 1);
  LPTX_assert(LPTX_particle_get_calln(particle, pdata) == 1);

  LPTX_particle_get_call(particle, 0, &d, pdata);
}

/**
 * @param number_of_arrays Number of output arrays
 * @param outpp Array of pointer to output arrays
 * @param outidx Index of each output arrays which should be written
 * @param particle Particle to get from
 * @param data extra argument data
 */
static inline void
LPTX_array_get_vecsoa(size_t number_of_arrays, void *outpp, LPTX_idtype outidx,
                      const LPTX_particle_data *particle,
                      const struct LPTX_array_get_data *adata,
                      const struct LPTX_particle_get_data *pdata)
{
  LPTX_assert(LPTX_array_get_calln(adata) == number_of_arrays);

  for (size_t i = 0; i < number_of_arrays; ++i) {
    void *outp = LPTX_array_get_call_vecsoa_helper(outpp, i, adata);
    struct LPTX_array_geti_data d = LPTX_array_geti_box(outp, outidx, i, adata);
    LPTX_particle_get_call(particle, i, &d, pdata);
  }
}

/**
 * @param outp Output array
 * @param baseidx Base index of output array which should be written
 * @param particle Particle to get from
 * @param data extra argument data
 */
static inline void
LPTX_array_get_vecaos(void *outp, LPTX_idtype baseidx,
                      const LPTX_particle_data *particle,
                      const struct LPTX_array_get_data *adata,
                      const struct LPTX_particle_get_data *pdata)
{
  size_t number_of_components = LPTX_array_get_calln(adata);

  for (size_t i = 0; i < number_of_components; ++i) {
    struct LPTX_array_geti_data d =
      LPTX_array_geti_box(outp, baseidx + i, i, adata);
    LPTX_particle_get_call(particle, i, &d, pdata);
  }
}

/**
 * @param outp Output array
 * @param outidx Index of output array which should be written
 * @param inp Pointer to member of LPTX_particle
 * @param component_index Index of components to extract
 * @param data extra arguement data
 */
static inline void LPTX_array_get_veccmp(
  void *outp, LPTX_idtype outidx, const LPTX_particle_data *particle,
  LPTX_idtype component_index, const struct LPTX_array_get_data *adata,
  const struct LPTX_particle_get_data *pdata)
{
  struct LPTX_array_geti_data d =
    LPTX_array_geti_box(outp, outidx, component_index, adata);

  LPTX_assert(0 <= component_index);

  LPTX_particle_get_call(particle, component_index, &d, pdata);
}

//---

/**
 * @param particle Particle to write to
 * @param inp Array of pointer to input arrays
 * @param inidx Index of each input arrays which should be read from
 * @param data extra argument data
 */
static inline void
LPTX_array_set_scl(LPTX_particle_data *particle, const void *inp,
                   LPTX_idtype inidx, const struct LPTX_array_set_data *adata,
                   const struct LPTX_particle_set_data *pdata)
{
  struct LPTX_array_seti_data d = LPTX_array_seti_box(inp, inidx, 0, adata);

  LPTX_assert(LPTX_array_set_calln(adata) == 1);
  LPTX_assert(LPTX_particle_set_calln(particle, pdata) == 1);

  LPTX_particle_set_call(particle, 0, &d, pdata);
}

/**
 * @param particle Particle to write to
 * @param number_of_array Number of input arrays
 * @param inpp Array of pointer to input arrays
 * @param inidx Index of each input arrays which should be read from
 * @param data extra argument data
 */
static inline void
LPTX_array_set_vecsoa(LPTX_particle_data *particle, size_t number_of_arrays,
                      const void *inpp, LPTX_idtype inidx,
                      const struct LPTX_array_set_data *adata,
                      const struct LPTX_particle_set_data *pdata)
{
  size_t number_of_components;

  LPTX_assert(LPTX_array_set_calln(adata) == number_of_arrays);

  number_of_components = LPTX_particle_set_calln(particle, pdata);

  for (size_t i = 0; i < number_of_components; ++i) {
    const void *inp = (i < number_of_arrays)
                        ? LPTX_array_set_call_vecsoa_helper(inpp, i, adata)
                        : NULL;
    struct LPTX_array_seti_data d = LPTX_array_seti_box(inp, inidx, i, adata);
    LPTX_particle_set_call(particle, i, &d, pdata);
  }
}

/**
 * @param particle Particle to write to
 * @param inp Input array
 * @param baseidx Base index of input array which should be read from
 * @param data extra argument data
 */
static inline void
LPTX_array_set_vecaos(LPTX_particle_data *particle, const void *inp,
                      LPTX_idtype baseidx,
                      const struct LPTX_array_set_data *adata,
                      const struct LPTX_particle_set_data *pdata)
{
  size_t number_of_components;
  size_t number_of_arrays;

  number_of_components = LPTX_particle_set_calln(particle, pdata);
  number_of_arrays = LPTX_array_set_calln(adata);

  for (size_t i = 0; i < number_of_components; ++i) {
    const void *inpi = (i < number_of_arrays) ? inp : NULL;
    struct LPTX_array_seti_data d =
      LPTX_array_seti_box(inpi, baseidx + i, i, adata);
    LPTX_particle_set_call(particle, i, &d, pdata);
  }
}

/**
 * @param particle Particle to write to
 * @param component_index Component index to set
 * @param inp Input array
 * @param inidx Index of input array which should be read from
 * @param data extra argument data
 */
static inline void
LPTX_array_set_veccmp(LPTX_particle_data *particle, LPTX_idtype component_index,
                      const void *inp, LPTX_idtype inidx,
                      const struct LPTX_array_set_data *adata,
                      const struct LPTX_particle_set_data *pdata)
{
  struct LPTX_array_seti_data d =
    LPTX_array_seti_box(inp, inidx, component_index, adata);

  LPTX_assert(0 <= component_index);

  LPTX_particle_set_call(particle, component_index, &d, pdata);
}

/**
 * @param particle Particle to write to
 * @param pdata extra argument data
 *
 * Set by implementation of @p pdata
 *
 * Expected to be implementation function fill all components.
 */
static inline void
LPTX_array_set_byfunc(LPTX_particle_data *particle,
                      const struct LPTX_particle_set_data *pdata)
{
  LPTX_assert(LPTX_particle_set_calln(particle, pdata) == 1);
  LPTX_particle_set_call(particle, 0, NULL, pdata);
}

//--------------------------------------
//
// Function for outer loop
//

struct LPTX_general_setget_args;

/**
 * @param particle Particle to process
 * @param particle_index Index in the set of @p particle
 * @apram array_index Index of set/get array
 * @param args Parameters
 *
 * To pass arguments (including the array to set/get):
 *
 * ```
 * struct your_data
 * {
 *   struct LPTX_particle_setget_args d;
 *   void *array;
 * };
 *
 * // ...
 *
 * void your_func_setget(LPTX_particle_data *particle,
 *                       LPTX_idtype particle_index,
 *                       LPTX_array_index array_index,
 *                       LPTX_particle_setget_args *args)
 *
 * {
 *   struct your_data *p = geom_container_of(args, struct your_data, d);
 *   p->array[array_index] = particle->...;
 *   // use p
 * ]
 * ```
 */
typedef void
LPTX_cb_general_setget(LPTX_particle_data *particle, LPTX_idtype particle_index,
                       LPTX_idtype array_index,
                       const struct LPTX_general_setget_args *args);

struct LPTX_general_setget_args
{
  LPTX_idtype size; ///< Available space per **particle** in output array
  LPTX_idtype last; ///< Number of **particles** written
  LPTX_cb_general_setget *const func; ///< Function to use set or get values
};

static inline struct LPTX_general_setget_args
LPTX_general_setget_args_init(LPTX_idtype size, LPTX_idtype last,
                              LPTX_cb_general_setget *func)
{
  return (struct LPTX_general_setget_args){
    .size = size,
    .last = last,
    .func = func,
  };
}

static inline struct LPTX_general_setget_args
LPTX_general_setget_args_init0(LPTX_idtype size, LPTX_cb_general_setget *func)
{
  return LPTX_general_setget_args_init(size, 0, func);
}

static inline LPTX_bool LPTX_general_setget_f(LPTX_particle_set *set,
                                              LPTX_idtype start,
                                              LPTX_idtype *last, void *args)
{
  struct LPTX_general_setget_args *a = args;
  LPTX_idtype count = *last - start;

#ifdef _OPENMP
#pragma omp for
#endif
  for (LPTX_idtype jj = 0; jj < count; ++jj) {
    LPTX_particle_data *p = &set->particles[jj + start];
    a->func(&set->particles[jj + start], jj + start, jj + a->last, a);
  }

  a->last += count;
  return LPTX_false;
}

static inline LPTX_bool LPTX_general_setget_af(LPTX_particle_set *set,
                                               void *args)
{
  LPTX_idtype count = LPTX_particle_set_number_of_particles(set);
  return LPTX_general_setget_f(set, 0, &count, args);
}

static inline LPTX_idtype
LPTX_param_pt_general_setget(LPTX_param *p, LPTX_idtype start,
                             struct LPTX_general_setget_args *args)
{
  if (args->size >= 0) {
    LPTX_param_foreach_particle_set_range_pi(p, start, start + args->size,
                                             LPTX_general_setget_f, args);
  } else {
    LPTX_param_foreach_particle_sets_i(p, LPTX_general_setget_af, args);
  }
  return args->last;
}

static inline LPTX_idtype
LPTX_particle_general_setget(LPTX_particle_set *set, LPTX_idtype start,
                             struct LPTX_general_setget_args *args)
{
  LPTX_idtype size = args->size;

  if (size >= 0) {
    LPTX_idtype np = LPTX_particle_set_number_of_particles(set);
    size += start;
    if (size > np)
      size = np;
    LPTX_general_setget_f(set, start, &size, args);
  } else {
    LPTX_general_setget_af(set, args);
  }
  return args->last;
}

//--- scalar get

struct LPTX_general_scl_getter_args
{
  struct LPTX_general_setget_args d;
  void *outp;
  const struct LPTX_array_get_data *adata;
  const struct LPTX_particle_get_data *pdata;
};
#define LPTX_general_scl_getter_entry(ptr) \
  geom_container_of(ptr, struct LPTX_general_scl_getter_args, d)

static inline void
LPTX_general_scl_getter(LPTX_particle_data *p, LPTX_idtype particle_index,
                        LPTX_idtype array_index,
                        const struct LPTX_general_setget_args *args)
{
  const struct LPTX_general_scl_getter_args *a;
  a = LPTX_general_scl_getter_entry(args);
  LPTX_array_get_scl(a->outp, array_index, p, a->adata, a->pdata);
}

static inline struct LPTX_general_scl_getter_args
LPTX_general_scl_getter_init(void *outp, LPTX_idtype outsize,
                             const struct LPTX_array_get_data *adata,
                             const struct LPTX_particle_get_data *pdata)
{
  LPTX_assert(outsize >= 0);
  return (struct LPTX_general_scl_getter_args){
    .d = LPTX_general_setget_args_init0(outsize, LPTX_general_scl_getter),
    .outp = outp,
    .adata = adata,
    .pdata = pdata,
  };
}

static inline LPTX_idtype
LPTX_param_pt_general_scl_getter(LPTX_param *p, LPTX_idtype start, void *outp,
                                 LPTX_idtype outsize,
                                 const struct LPTX_array_get_data *adata,
                                 const struct LPTX_particle_get_data *pdata)
{
  struct LPTX_general_scl_getter_args a =
    LPTX_general_scl_getter_init(outp, outsize, adata, pdata);
  return LPTX_param_pt_general_setget(p, start, &a.d);
}

static inline LPTX_idtype
LPTX_particle_general_scl_getter(LPTX_particle_set *set, LPTX_idtype start,
                                 void *outp, LPTX_idtype outsize,
                                 const struct LPTX_array_get_data *adata,
                                 const struct LPTX_particle_get_data *pdata)
{
  struct LPTX_general_scl_getter_args a =
    LPTX_general_scl_getter_init(outp, outsize, adata, pdata);
  return LPTX_particle_general_setget(set, start, &a.d);
}

//--- scalar set

struct LPTX_general_scl_setter_args
{
  struct LPTX_general_setget_args d;
  const void *inp;
  const struct LPTX_array_set_data *adata;
  const struct LPTX_particle_set_data *pdata;
};
#define LPTX_general_scl_setter_entry(ptr) \
  geom_container_of(ptr, struct LPTX_general_scl_setter_args, d)

static inline void
LPTX_general_scl_setter(LPTX_particle_data *p, LPTX_idtype particle_index,
                        LPTX_idtype array_index,
                        const struct LPTX_general_setget_args *args)
{
  const struct LPTX_general_scl_setter_args *a;
  a = LPTX_general_scl_setter_entry(args);
  LPTX_array_set_scl(p, a->inp, array_index, a->adata, a->pdata);
}

static inline struct LPTX_general_scl_setter_args
LPTX_general_setter_init(const void *inp, LPTX_idtype insize,
                         const struct LPTX_array_set_data *adata,
                         const struct LPTX_particle_set_data *pdata)
{
  LPTX_assert(insize >= 0);
  return (struct LPTX_general_scl_setter_args){
    .d = LPTX_general_setget_args_init0(insize, LPTX_general_scl_setter),
    .inp = inp,
    .adata = adata,
    .pdata = pdata,
  };
}

static inline LPTX_idtype
LPTX_param_pt_general_scl_setter(LPTX_param *p, LPTX_idtype start,
                                 const void *inp, LPTX_idtype insize,
                                 const struct LPTX_array_set_data *adata,
                                 const struct LPTX_particle_set_data *pdata)
{
  struct LPTX_general_scl_setter_args a =
    LPTX_general_setter_init(inp, insize, adata, pdata);
  return LPTX_param_pt_general_setget(p, start, &a.d);
}

static inline LPTX_idtype
LPTX_particle_general_scl_setter(LPTX_particle_set *set, LPTX_idtype start,
                                 const void *inp, LPTX_idtype insize,
                                 const struct LPTX_array_set_data *adata,
                                 const struct LPTX_particle_set_data *pdata)
{
  struct LPTX_general_scl_setter_args a =
    LPTX_general_setter_init(inp, insize, adata, pdata);
  return LPTX_particle_general_setget(set, start, &a.d);
}

//--- vector in AOS format getter

struct LPTX_general_vecaos_getter_args
{
  struct LPTX_general_setget_args d;
  LPTX_idtype ndim;
  void *outp;
  const struct LPTX_array_get_data *adata;
  const struct LPTX_particle_get_data *pdata;
};
#define LPTX_general_vecaos_getter_entry(ptr) \
  geom_container_of(ptr, struct LPTX_general_vecaos_getter_args, d)

static inline void
LPTX_general_vecaos_getter(LPTX_particle_data *p, LPTX_idtype particle_index,
                           LPTX_idtype array_index,
                           const struct LPTX_general_setget_args *args)
{
  const struct LPTX_general_vecaos_getter_args *a;
  a = LPTX_general_vecaos_getter_entry(args);
  LPTX_array_get_vecaos(a->outp, a->ndim * array_index, p, a->adata, a->pdata);
}

static inline struct LPTX_general_vecaos_getter_args
LPTX_general_vecaos_getter_args_init(void *outp, LPTX_idtype outsize,
                                     const struct LPTX_array_get_data *adata,
                                     const struct LPTX_particle_get_data *pdata)
{
  LPTX_idtype ndim = LPTX_array_get_calln(adata);
  LPTX_assert(ndim > 0);
  LPTX_assert(outsize >= 0);
  return (struct LPTX_general_vecaos_getter_args){
    .d = LPTX_general_setget_args_init0(outsize / ndim,
                                        LPTX_general_vecaos_getter),
    .ndim = ndim,
    .outp = outp,
    .adata = adata,
    .pdata = pdata,
  };
}

static inline LPTX_idtype
LPTX_param_pt_general_vecaos_getter(LPTX_param *p, LPTX_idtype start,
                                    void *outp, LPTX_idtype outsize,
                                    const struct LPTX_array_get_data *adata,
                                    const struct LPTX_particle_get_data *pdata)
{
  struct LPTX_general_vecaos_getter_args a =
    LPTX_general_vecaos_getter_args_init(outp, outsize, adata, pdata);
  return a.ndim * LPTX_param_pt_general_setget(p, start, &a.d);
}

static inline LPTX_idtype
LPTX_particle_general_vecaos_getter(LPTX_particle_set *set, LPTX_idtype start,
                                    void *outp, LPTX_idtype outsize,
                                    const struct LPTX_array_get_data *adata,
                                    const struct LPTX_particle_get_data *pdata)
{
  struct LPTX_general_vecaos_getter_args a =
    LPTX_general_vecaos_getter_args_init(outp, outsize, adata, pdata);
  return a.ndim * LPTX_particle_general_setget(set, start, &a.d);
}

//--- vector in AOS format setter

struct LPTX_general_vecaos_setter_args
{
  struct LPTX_general_setget_args d;
  LPTX_idtype ndim;
  const void *inp;
  const struct LPTX_array_set_data *adata;
  const struct LPTX_particle_set_data *pdata;
};
#define LPTX_general_vecaos_setter_entry(ptr) \
  geom_container_of(ptr, struct LPTX_general_vecaos_setter_args, d)

static inline void
LPTX_general_vecaos_setter(LPTX_particle_data *p, LPTX_idtype particle_index,
                           LPTX_idtype array_index,
                           const struct LPTX_general_setget_args *args)
{
  const struct LPTX_general_vecaos_setter_args *a;
  a = LPTX_general_vecaos_setter_entry(args);
  LPTX_array_set_vecaos(p, a->inp, a->ndim * array_index, a->adata, a->pdata);
}

static inline struct LPTX_general_vecaos_setter_args
LPTX_general_vecaos_setter_args_init(const void *inp, LPTX_idtype insize,
                                     const struct LPTX_array_set_data *adata,
                                     const struct LPTX_particle_set_data *pdata)
{
  LPTX_idtype ndim = LPTX_array_set_calln(adata);
  LPTX_assert(ndim > 0);
  LPTX_assert(insize >= 0);
  return (struct LPTX_general_vecaos_setter_args){
    .d =
      LPTX_general_setget_args_init0(insize / ndim, LPTX_general_vecaos_setter),
    .ndim = ndim,
    .inp = inp,
    .adata = adata,
    .pdata = pdata,
  };
}

static inline LPTX_idtype
LPTX_param_pt_general_vecaos_setter(LPTX_param *p, LPTX_idtype start,
                                    const void *inp, LPTX_idtype insize,
                                    const struct LPTX_array_set_data *adata,
                                    const struct LPTX_particle_set_data *pdata)
{
  struct LPTX_general_vecaos_setter_args a =
    LPTX_general_vecaos_setter_args_init(inp, insize, adata, pdata);
  return a.ndim * LPTX_param_pt_general_setget(p, start, &a.d);
}

static inline LPTX_idtype
LPTX_particle_general_vecaos_setter(LPTX_particle_set *set, LPTX_idtype start,
                                    const void *inp, LPTX_idtype insize,
                                    const struct LPTX_array_set_data *adata,
                                    const struct LPTX_particle_set_data *pdata)
{
  struct LPTX_general_vecaos_setter_args a =
    LPTX_general_vecaos_setter_args_init(inp, insize, adata, pdata);
  return a.ndim * LPTX_particle_general_setget(set, start, &a.d);
}

//--- vector in SOA format (separated arrays) getter

struct LPTX_general_vecsoa_getter_args
{
  struct LPTX_general_setget_args d;
  LPTX_idtype ndim;
  void *outpp;
  const struct LPTX_array_get_data *adata;
  const struct LPTX_particle_get_data *pdata;
};
#define LPTX_general_vecsoa_getter_entry(ptr) \
  geom_container_of(ptr, struct LPTX_general_vecsoa_getter_args, d)

static inline void
LPTX_general_vecsoa_getter(LPTX_particle_data *p, LPTX_idtype particle_index,
                           LPTX_idtype array_index,
                           const struct LPTX_general_setget_args *args)
{
  const struct LPTX_general_vecsoa_getter_args *a;
  a = LPTX_general_vecsoa_getter_entry(args);
  LPTX_array_get_vecsoa(a->ndim, a->outpp, array_index, p, a->adata, a->pdata);
}

static inline struct LPTX_general_vecsoa_getter_args
LPTX_general_vecsoa_getter_args_init(size_t number_of_arrays, void *outpp,
                                     LPTX_idtype outsize,
                                     const struct LPTX_array_get_data *adata,
                                     const struct LPTX_particle_get_data *pdata)
{
  LPTX_idtype ndim;
  ndim = LPTX_array_get_calln(adata);
  LPTX_assert(ndim > 0);
  LPTX_assert(ndim == number_of_arrays);
  LPTX_assert(outsize >= 0);
  return (struct LPTX_general_vecsoa_getter_args){
    .d = LPTX_general_setget_args_init0(outsize, LPTX_general_vecsoa_getter),
    .ndim = ndim,
    .outpp = outpp,
    .adata = adata,
    .pdata = pdata,
  };
}

static inline LPTX_idtype LPTX_param_pt_general_vecsoa_getter(
  LPTX_param *p, LPTX_idtype start, size_t number_of_arrays, void *outpp,
  LPTX_idtype outsize, const struct LPTX_array_get_data *adata,
  const struct LPTX_particle_get_data *pdata)
{
  struct LPTX_general_vecsoa_getter_args a =
    LPTX_general_vecsoa_getter_args_init(number_of_arrays, outpp, outsize,
                                         adata, pdata);
  return LPTX_param_pt_general_setget(p, start, &a.d);
}

static inline LPTX_idtype LPTX_particle_general_vecsoa_getter(
  LPTX_particle_set *set, LPTX_idtype start, size_t number_of_arrays,
  void *outpp, LPTX_idtype outsize, const struct LPTX_array_get_data *adata,
  const struct LPTX_particle_get_data *pdata)
{
  struct LPTX_general_vecsoa_getter_args a =
    LPTX_general_vecsoa_getter_args_init(number_of_arrays, outpp, outsize,
                                         adata, pdata);
  return LPTX_particle_general_setget(set, start, &a.d);
}

//--- vector in SOA format (separated arrays) setter

struct LPTX_general_vecsoa_setter_args
{
  struct LPTX_general_setget_args d;
  LPTX_idtype ndim;
  const void *inpp;
  const struct LPTX_array_set_data *adata;
  const struct LPTX_particle_set_data *pdata;
};
#define LPTX_general_vecsoa_setter_entry(ptr) \
  geom_container_of(ptr, struct LPTX_general_vecsoa_setter_args, d)

static inline void
LPTX_general_vecsoa_setter(LPTX_particle_data *p, LPTX_idtype particle_index,
                           LPTX_idtype array_index,
                           const struct LPTX_general_setget_args *args)
{
  const struct LPTX_general_vecsoa_setter_args *a;
  a = LPTX_general_vecsoa_setter_entry(args);
  LPTX_array_set_vecsoa(p, a->ndim, a->inpp, array_index, a->adata, a->pdata);
}

static inline struct LPTX_general_vecsoa_setter_args
LPTX_general_vecsoa_setter_args_init(size_t number_of_arrays, const void *inpp,
                                     LPTX_idtype insize,
                                     const struct LPTX_array_set_data *adata,
                                     const struct LPTX_particle_set_data *pdata)
{
  LPTX_idtype ndim;
  ndim = LPTX_array_set_calln(adata);
  LPTX_assert(ndim > 0);
  LPTX_assert(ndim == number_of_arrays);
  LPTX_assert(insize >= 0);
  return (struct LPTX_general_vecsoa_setter_args){
    .d = LPTX_general_setget_args_init0(insize, LPTX_general_vecsoa_setter),
    .ndim = ndim,
    .inpp = inpp,
    .adata = adata,
    .pdata = pdata,
  };
}

static inline LPTX_idtype LPTX_param_pt_general_vecsoa_setter(
  LPTX_param *p, LPTX_idtype start, size_t number_of_arrays, const void *inpp,
  LPTX_idtype insize, const struct LPTX_array_set_data *adata,
  const struct LPTX_particle_set_data *pdata)
{
  struct LPTX_general_vecsoa_setter_args a =
    LPTX_general_vecsoa_setter_args_init(number_of_arrays, inpp, insize, adata,
                                         pdata);
  return LPTX_param_pt_general_setget(p, start, &a.d);
}

static inline LPTX_idtype LPTX_particle_general_vecsoa_setter(
  LPTX_particle_set *set, LPTX_idtype start, size_t number_of_arrays,
  const void *inpp, LPTX_idtype insize, const struct LPTX_array_set_data *adata,
  const struct LPTX_particle_set_data *pdata)
{
  struct LPTX_general_vecsoa_setter_args a =
    LPTX_general_vecsoa_setter_args_init(number_of_arrays, inpp, insize, adata,
                                         pdata);
  return LPTX_particle_general_setget(set, start, &a.d);
}

//--- Extract specific component only

struct LPTX_general_veccmp_getter_args
{
  struct LPTX_general_setget_args d;
  void *outp;
  LPTX_idtype component_index;
  const struct LPTX_array_get_data *adata;
  const struct LPTX_particle_get_data *pdata;
};
#define LPTX_general_veccmp_getter_entry(ptr) \
  geom_container_of(ptr, struct LPTX_general_veccmp_getter_args, d)

static inline void
LPTX_general_veccmp_getter(LPTX_particle_data *p, LPTX_idtype particle_index,
                           LPTX_idtype array_index,
                           const struct LPTX_general_setget_args *args)
{
  const struct LPTX_general_veccmp_getter_args *a;
  a = LPTX_general_veccmp_getter_entry(args);
  LPTX_array_get_veccmp(a->outp, array_index, p, a->component_index, a->adata,
                        a->pdata);
}

static inline struct LPTX_general_veccmp_getter_args
LPTX_general_veccmp_getter_args_init(void *outp, LPTX_idtype component_index,
                                     LPTX_idtype outsize,
                                     const struct LPTX_array_get_data *adata,
                                     const struct LPTX_particle_get_data *pdata)
{
  LPTX_assert(outsize >= 0);
  return (struct LPTX_general_veccmp_getter_args){
    .d = LPTX_general_setget_args_init0(outsize, LPTX_general_veccmp_getter),
    .outp = outp,
    .component_index = component_index,
    .adata = adata,
    .pdata = pdata,
  };
}

static inline LPTX_idtype LPTX_param_pt_general_veccmp_getter(
  LPTX_param *p, LPTX_idtype start, void *outp, LPTX_idtype component_index,
  LPTX_idtype outsize, const struct LPTX_array_get_data *adata,
  const struct LPTX_particle_get_data *pdata)
{
  struct LPTX_general_veccmp_getter_args a =
    LPTX_general_veccmp_getter_args_init(outp, component_index, outsize, adata,
                                         pdata);
  return LPTX_param_pt_general_setget(p, start, &a.d);
}

static inline LPTX_idtype
LPTX_particle_general_veccmp_getter(LPTX_particle_set *set, LPTX_idtype start,
                                    void *outp, LPTX_idtype component_index,
                                    LPTX_idtype outsize,
                                    const struct LPTX_array_get_data *adata,
                                    const struct LPTX_particle_get_data *pdata)
{
  struct LPTX_general_veccmp_getter_args a =
    LPTX_general_veccmp_getter_args_init(outp, component_index, outsize, adata,
                                         pdata);
  return LPTX_particle_general_setget(set, start, &a.d);
}

//--- Assign single component only

struct LPTX_general_veccmp_setter_args
{
  struct LPTX_general_setget_args d;
  const void *inp;
  LPTX_idtype component_index;
  const struct LPTX_array_set_data *adata;
  const struct LPTX_particle_set_data *pdata;
};
#define LPTX_general_veccmp_setter_entry(ptr) \
  geom_container_of(ptr, struct LPTX_general_veccmp_setter_args, d)

static inline void
LPTX_general_veccmp_setter(LPTX_particle_data *p, LPTX_idtype particle_index,
                           LPTX_idtype array_index,
                           const struct LPTX_general_setget_args *args)
{
  const struct LPTX_general_veccmp_setter_args *a;
  a = LPTX_general_veccmp_setter_entry(args);
  LPTX_array_set_veccmp(p, a->component_index, a->inp, array_index, a->adata,
                        a->pdata);
}

static inline struct LPTX_general_veccmp_setter_args
LPTX_general_veccmp_setter_args_init(const void *inp,
                                     LPTX_idtype component_index,
                                     LPTX_idtype insize,
                                     const struct LPTX_array_set_data *adata,
                                     const struct LPTX_particle_set_data *pdata)
{
  LPTX_assert(insize >= 0);
  return (struct LPTX_general_veccmp_setter_args){
    .d = LPTX_general_setget_args_init0(insize, LPTX_general_veccmp_setter),
    .inp = inp,
    .component_index = component_index,
    .adata = adata,
    .pdata = pdata,
  };
}

static inline LPTX_idtype LPTX_param_pt_general_veccmp_setter(
  LPTX_param *p, LPTX_idtype start, const void *inp,
  LPTX_idtype component_index, LPTX_idtype insize,
  const struct LPTX_array_set_data *adata,
  const struct LPTX_particle_set_data *pdata)
{
  struct LPTX_general_veccmp_setter_args a =
    LPTX_general_veccmp_setter_args_init(inp, component_index, insize, adata,
                                         pdata);
  return LPTX_param_pt_general_setget(p, start, &a.d);
}

static inline LPTX_idtype LPTX_particle_general_veccmp_setter(
  LPTX_particle_set *set, LPTX_idtype start, const void *inp,
  LPTX_idtype component_index, LPTX_idtype insize,
  const struct LPTX_array_set_data *adata,
  const struct LPTX_particle_set_data *pdata)
{
  struct LPTX_general_veccmp_setter_args a =
    LPTX_general_veccmp_setter_args_init(inp, component_index, insize, adata,
                                         pdata);
  return LPTX_particle_general_setget(set, start, &a.d);
}

//--- Assign by user-provided function

struct LPTX_general_byfunc_setter_args
{
  struct LPTX_general_setget_args d;
  const struct LPTX_particle_set_data *pdata;
  LPTX_cb_particle_if *cond;
  void *arg;
};
#define LPTX_general_byfunc_setter_entry(ptr) \
  geom_container_of(ptr, struct LPTX_general_byfunc_setter_args, d)

static inline LPTX_bool LPTX_fluid_cb_default_cond(const LPTX_particle_data *p,
                                                   void *arg)
{
  /* Do not use @p arg */
  return LPTX_particle_is_used(&p->base);
}

static inline void
LPTX_general_byfunc_setter(LPTX_particle_data *p, LPTX_idtype particle_index,
                           LPTX_idtype array_index,
                           const struct LPTX_general_setget_args *args)
{
  const struct LPTX_general_byfunc_setter_args *a;
  a = LPTX_general_byfunc_setter_entry(args);

  if ((a->cond ? a->cond : LPTX_fluid_cb_default_cond)(p, a->arg))
    LPTX_array_set_byfunc(p, a->pdata);
}

static inline struct LPTX_general_byfunc_setter_args
LPTX_general_byfunc_setter_args_init(LPTX_idtype count,
                                     LPTX_cb_particle_if *cond, void *arg,
                                     const struct LPTX_particle_set_data *pdata)
{
  return (struct LPTX_general_byfunc_setter_args){
    .d = LPTX_general_setget_args_init0(count, LPTX_general_byfunc_setter),
    .pdata = pdata,
  };
}

/**
 * Set -1 for @p count to set process on all particles.
 */

static inline LPTX_idtype
LPTX_param_pt_general_byfunc_setter(LPTX_param *p, LPTX_idtype start,
                                    LPTX_idtype count,
                                    LPTX_cb_particle_if *cond, void *arg,
                                    const struct LPTX_particle_set_data *pdata)
{
  struct LPTX_general_byfunc_setter_args a =
    LPTX_general_byfunc_setter_args_init(count, cond, arg, pdata);
  return LPTX_param_pt_general_setget(p, start, &a.d);
}

static inline LPTX_idtype
LPTX_particle_general_byfunc_setter(LPTX_particle_set *set, LPTX_idtype start,
                                    LPTX_idtype count,
                                    LPTX_cb_particle_if *cond, void *arg,
                                    const struct LPTX_particle_set_data *pdata)
{
  struct LPTX_general_byfunc_setter_args a =
    LPTX_general_byfunc_setter_args_init(count, cond, arg, pdata);
  return LPTX_particle_general_setget(set, start, &a.d);
}

//--- rectilinear grid wrapper

struct LPTX_fluid_rect_args
{
  size_t number_of_scalars;
  const LPTX_rectilinear_scalar *const *scalars;
  size_t number_of_vectors;
  const LPTX_rectilinear_vector *const *vectors;
};

#define LPTX_FRECT_N(...) \
  (sizeof((const void *const[]){__VA_ARGS__}) / sizeof(const void *))
#define LPTX_FRECT_S(...) \
  ((const LPTX_rectilinear_scalar *const[]){__VA_ARGS__})
#define LPTX_FRECT_V(...) \
  ((const LPTX_rectilinear_vector *const[]){__VA_ARGS__})

#define LPTX__FRECT_I(ns, sv, nv, vv) \
  (&((struct LPTX_fluid_rect_args){   \
    .number_of_scalars = ns,          \
    .scalars = sv,                    \
    .number_of_vectors = nv,          \
    .vectors = vv,                    \
  }))

/**
 * Create LPTX_fluid_rect_args with scalars and vectors
 *
 * scalars and vectors are list of variable names wrapped by parantheses:
 * ex. `LPTX_FRECT_ISV((s1, s2, s3), (v1, v2))`
 *
 * @note both entries require least one variable. Use LPTX_FRECT_IS or
 * LPTX_FRECT_IV for zero arguments
 */
#define LPTX_FRECT_ISV(scalars, vectors)                    \
  LPTX__FRECT_I(LPTX_FRECT_N scalars, LPTX_FRECT_S scalars, \
                LPTX_FRECT_N vectors, LPTX_FRECT_V vectors)

/* scalar arguments */
#define LPTX_FRECT_IS(...) \
  LPTX__FRECT_I(LPTX_FRECT_N(__VA_ARGS__), LPTX_FRECT_S(__VA_ARGS__), 0, NULL)

/* vector arguments */
#define LPTX_FRECT_IV(...) \
  LPTX__FRECT_I(0, NULL, LPTX_FRECT_N(__VA_ARGS__), LPTX_FRECT_V(__VA_ARGS__))

//-----------------

#define LPTX_TYPE_CHK(type, pt) ((0) ? ((int (*)(type *)){0})(pt) : 0)

#define LPTX_TYPE_CHKV(type, ...) \
  ((0) ? ((int (*)(type *[])){0})((type *[]){__VA_ARGS__}) : 0)

#define LPTX_SET_TYPE_CHK(array, adata, pdata)      \
  (LPTX_TYPE_CHK(const LPTX_inptype##adata, array), \
   LPTX_TYPE_CHK(LPTX_outtype##adata, LPTX_dtype##pdata))

#define LPTX_GET_TYPE_CHK(array, adata, pdata) \
  (LPTX_TYPE_CHK(LPTX_outtype##adata, array),  \
   LPTX_TYPE_CHK(const LPTX_inptype##adata, LPTX_dtype##pdata))

#define LPTX_SET_TYPE_CHKV(adata, pdata, ...)              \
  (LPTX_TYPE_CHKV(const LPTX_inptype##adata, __VA_ARGS__), \
   LPTX_TYPE_CHK(LPTX_outtype##adata, LPTX_dtype##pdata))

#define LPTX_GET_TYPE_CHKV(adata, pdata, ...)        \
  (LPTX_TYPE_CHKV(LPTX_outtype##adata, __VA_ARGS__), \
   LPTX_TYPE_CHK(const LPTX_inptype##adata, LPTX_dtype##pdata))

#define LPTX_SNGL_SCALAR_SET(sclvar, array_setter, particle_setter)         \
  (LPTX_SET_TYPE_CHK(sclvar, _##array_setter, _##particle_setter),          \
   LPTX_particle_general_scl_setter(set, start, sclvar, size, array_setter, \
                                    particle_setter))

#define LPTX_SNGL_SCALAR_GET(sclvar, array_getter, particle_getter)         \
  (LPTX_GET_TYPE_CHK(sclvar, _##array_getter, _##particle_getter),          \
   LPTX_particle_general_scl_getter(set, start, sclvar, size, array_getter, \
                                    particle_getter))

#define LPTX_PACK_SCALAR_SET(sclvar, array_setter, particle_setter)           \
  (LPTX_SET_TYPE_CHK(sclvar, _##array_setter, _##particle_setter),            \
   LPTX_param_pt_general_scl_setter(param, start, sclvar, size, array_setter, \
                                    particle_setter))

#define LPTX_PACK_SCALAR_GET(sclvar, array_getter, particle_getter)           \
  (LPTX_GET_TYPE_CHK(sclvar, _##array_getter, _##particle_getter),            \
   LPTX_param_pt_general_scl_getter(param, start, sclvar, size, array_getter, \
                                    particle_getter))

#define LPTX_SNGL_VECCMP_SET(sclvar, compidx, array_setter, particle_setter) \
  (LPTX_SET_TYPE_CHK(sclvar, _##array_setter, _##particle_setter),           \
   LPTX_particle_general_veccmp_setter(set, start, sclvar, compidx, size,    \
                                       array_setter, particle_setter))

#define LPTX_SNGL_VECCMP_GET(sclvar, compidx, array_getter, particle_getter) \
  (LPTX_GET_TYPE_CHK(sclvar, _##array_getter, _##particle_getter),           \
   LPTX_particle_general_veccmp_getter(set, start, sclvar, compidx, size,    \
                                       array_getter, particle_getter))

#define LPTX_PACK_VECCMP_SET(sclvar, compidx, array_setter, particle_setter) \
  (LPTX_SET_TYPE_CHK(sclvar, _##array_setter, _##particle_setter),           \
   LPTX_param_pt_general_veccmp_setter(param, start, sclvar, compidx, size,  \
                                       array_setter, particle_setter))

#define LPTX_PACK_VECCMP_GET(sclvar, compidx, array_getter, particle_getter) \
  (LPTX_GET_TYPE_CHK(sclvar, _##array_getter, _##particle_getter),           \
   LPTX_param_pt_general_veccmp_getter(param, start, sclvar, compidx, size,  \
                                       array_getter, particle_getter))

#define LPTX_SNGL_AOSVEC_SET(aosvec, array_setter, particle_setter)            \
  (LPTX_SET_TYPE_CHK(aosvec, _##array_setter, _##particle_setter),             \
   LPTX_particle_general_vecaos_setter(set, start, aosvec, size, array_setter, \
                                       particle_setter))

#define LPTX_SNGL_AOSVEC_GET(aosvec, array_getter, particle_getter)            \
  (LPTX_GET_TYPE_CHK(aosvec, _##array_getter, _##particle_getter),             \
   LPTX_particle_general_vecaos_getter(set, start, aosvec, size, array_getter, \
                                       particle_getter))

#define LPTX_PACK_AOSVEC_SET(aosvec, array_setter, particle_setter) \
  (LPTX_SET_TYPE_CHK(aosvec, _##array_setter, _##particle_setter),  \
   LPTX_param_pt_general_vecaos_setter(param, start, aosvec, size,  \
                                       array_setter, particle_setter))

#define LPTX_PACK_AOSVEC_GET(aosvec, array_getter, particle_getter) \
  (LPTX_GET_TYPE_CHK(aosvec, _##array_getter, _##particle_getter),  \
   LPTX_param_pt_general_vecaos_getter(param, start, aosvec, size,  \
                                       array_getter, particle_getter))

#define LPTX_SNGL_SOAVECV_SET(number_of_arrays, arrays, array_setter,        \
                              particle_setter)                               \
  (LPTX_SET_TYPE_CHK(*arrays, _##array_setter, _##particle_setter),          \
   LPTX_particle_general_vecsoa_setter(set, start, number_of_arrays, arrays, \
                                       size, array_setter, particle_setter))

#define LPTX_SNGL_SOAVECV_GET(number_of_arrays, arrays, array_getter,        \
                              particle_getter)                               \
  (LPTX_GET_TYPE_CHK(*arrays, _##array_getter, _##particle_getter),          \
   LPTX_particle_general_vecsoa_getter(set, start, number_of_arrays, arrays, \
                                       size, array_getter, particle_getter))

#define LPTX_PACK_SOAVECV_SET(number_of_arrays, arrays, array_setter,          \
                              particle_setter)                                 \
  (LPTX_SET_TYPE_CHK(*arrays, _##array_setter, _##particle_setter),            \
   LPTX_param_pt_general_vecsoa_setter(param, start, number_of_arrays, arrays, \
                                       size, array_setter, particle_setter))

#define LPTX_PACK_SOAVECV_GET(number_of_arrays, arrays, array_getter,          \
                              particle_getter)                                 \
  (LPTX_GET_TYPE_CHK(*arrays, _##array_getter, _##particle_getter),            \
   LPTX_param_pt_general_vecsoa_getter(param, start, number_of_arrays, arrays, \
                                       size, array_getter, particle_getter))

#define LPTX_SOAVEC_N(t, ...) (sizeof((t *[]){__VA_ARGS__}) / sizeof(t *))
#define LPTX_SOAVEC_C(t, ...) ((t *[]){__VA_ARGS__})

#define LPTX_SET_SOAVEC_N(array_setter, ...) \
  LPTX_SOAVEC_N(const LPTX_inptype##array_setter, __VA_ARGS__)
#define LPTX_SET_SOAVEC_C(array_setter, ...) \
  LPTX_SOAVEC_C(const LPTX_inptype##array_setter, __VA_ARGS__)

#define LPTX_GET_SOAVEC_N(array_setter, ...) \
  LPTX_SOAVEC_N(LPTX_outtype##array_setter, __VA_ARGS__)
#define LPTX_GET_SOAVEC_C(array_setter, ...) \
  LPTX_SOAVEC_C(LPTX_outtype##array_setter, __VA_ARGS__)

#define LPTX_SNGL_SOAVEC_SET(array_setter, particle_setter, ...)          \
  (LPTX_SET_TYPE_CHKV(_##array_setter, _##particle_setter, __VA_ARGS__),  \
   LPTX_particle_general_vecsoa_setter(set, start,                        \
                                       LPTX_SET_SOAVEC_N(_##array_setter, \
                                                         __VA_ARGS__),    \
                                       LPTX_SET_SOAVEC_C(_##array_setter, \
                                                         __VA_ARGS__),    \
                                       size, array_setter, particle_setter))

#define LPTX_SNGL_SOAVEC_GET(array_getter, particle_getter, ...)          \
  (LPTX_GET_TYPE_CHKV(_##array_getter, _##particle_getter, __VA_ARGS__),  \
   LPTX_particle_general_vecsoa_getter(set, start,                        \
                                       LPTX_GET_SOAVEC_N(_##array_getter, \
                                                         __VA_ARGS__),    \
                                       LPTX_GET_SOAVEC_C(_##array_getter, \
                                                         __VA_ARGS__),    \
                                       size, array_getter, particle_getter))

#define LPTX_PACK_SOAVEC_SET(array_setter, particle_setter, ...)          \
  (LPTX_SET_TYPE_CHKV(_##array_setter, _##particle_setter, __VA_ARGS__),  \
   LPTX_param_pt_general_vecsoa_setter(param, start,                      \
                                       LPTX_SET_SOAVEC_N(_##array_setter, \
                                                         __VA_ARGS__),    \
                                       LPTX_SET_SOAVEC_C(_##array_setter, \
                                                         __VA_ARGS__),    \
                                       size, array_setter, particle_setter))

#define LPTX_PACK_SOAVEC_GET(array_getter, particle_getter, ...)          \
  (LPTX_GET_TYPE_CHKV(_##array_getter, _##particle_getter, __VA_ARGS__),  \
   LPTX_param_pt_general_vecsoa_getter(param, start,                      \
                                       LPTX_GET_SOAVEC_N(_##array_getter, \
                                                         __VA_ARGS__),    \
                                       LPTX_GET_SOAVEC_C(_##array_getter, \
                                                         __VA_ARGS__),    \
                                       size, array_getter, particle_getter))

#define LPTX_SNGL_SCALAR_FLUID_CB_SET(setter) \
  LPTX_particle_general_byfunc_setter(set, 0, -1, NULL, NULL, setter)

#define LPTX_PACK_SCALAR_FLUID_CB_SET(setter) \
  LPTX_param_pt_general_byfunc_setter(param, 0, -1, NULL, NULL, setter)

#define LPTX_SNGL_SCALAR_FLUID_CBCD_SET(setter) \
  LPTX_particle_general_byfunc_setter(set, 0, -1, cond, condarg, setter)

#define LPTX_PACK_SCALAR_FLUID_CBCD_SET(setter) \
  LPTX_param_pt_general_byfunc_setter(param, 0, -1, cond, condarg, setter)

#define LPTX_SNGL_VECTOR_FLUID_CB_SET(setter) \
  LPTX_particle_general_byfunc_setter(set, 0, -1, NULL, NULL, setter)

#define LPTX_PACK_VECTOR_FLUID_CB_SET(setter) \
  LPTX_param_pt_general_byfunc_setter(param, 0, -1, NULL, NULL, setter)

#define LPTX_SNGL_VECTOR_FLUID_CBCD_SET(setter) \
  LPTX_particle_general_byfunc_setter(set, 0, -1, cond, condarg, setter)

#define LPTX_PACK_VECTOR_FLUID_CBCD_SET(setter) \
  LPTX_param_pt_general_byfunc_setter(param, 0, -1, cond, condarg, setter)

// LPTX_PARTICLE_FLUID_CB_SET_I_INIT() is defined in priv_array_data_p.h
#define LPTX__INDEX_FLUID_CB(f, a) LPTX_PARTICLE_FLUID_CB_SET_I_INIT(f, a)

#define LPTX_SNGL_INDEX_FLUID_CB_SET()                        \
  LPTX_particle_general_byfunc_setter(set, 0, -1, NULL, NULL, \
                                      LPTX__INDEX_FLUID_CB(func, arg))

#define LPTX_PACK_INDEX_FLUID_CB_SET()                          \
  LPTX_param_pt_general_byfunc_setter(param, 0, -1, NULL, NULL, \
                                      LPTX__INDEX_FLUID_CB(func, arg))

#define LPTX_SNGL_INDEX_FLUID_CBCD_SET()                         \
  LPTX_particle_general_byfunc_setter(set, 0, -1, cond, condarg, \
                                      LPTX__INDEX_FLUID_CB(func, funcarg))

#define LPTX_PACK_INDEX_FLUID_CBCD_SET()                           \
  LPTX_param_pt_general_byfunc_setter(param, 0, -1, cond, condarg, \
                                      LPTX__INDEX_FLUID_CB(func, funcarg))

//--- Wrap raw array into LPTX_rectilinear_*

static inline const LPTX_rectilinear_grid *LPTX_rectilinear_grid_wrap_t(
  LPTX_rectilinear_grid *grid, const LPTX_type *coords_i,
  const LPTX_type *coords_j, const LPTX_type *coords_k, LPTX_idtype nx,
  LPTX_idtype ny, LPTX_idtype nz, LPTX_idtype mx, LPTX_idtype my,
  LPTX_idtype mz, LPTX_idtype stmx, LPTX_idtype stmy, LPTX_idtype stmz)
{
  *grid = LPTX_rectilinear_grid_t(coords_i, coords_j, coords_k, nx, ny, nz, mx,
                                  my, mz, stmx, stmy, stmz);
  return grid;
}

static inline const LPTX_rectilinear_grid *LPTX_rectilinear_grid_wrap_f(
  LPTX_rectilinear_grid *grid, const float *coords_i, const float *coords_j,
  const float *coords_k, LPTX_idtype nx, LPTX_idtype ny, LPTX_idtype nz,
  LPTX_idtype mx, LPTX_idtype my, LPTX_idtype mz, LPTX_idtype stmx,
  LPTX_idtype stmy, LPTX_idtype stmz)
{
  *grid = LPTX_rectilinear_grid_f(coords_i, coords_j, coords_k, nx, ny, nz, mx,
                                  my, mz, stmx, stmy, stmz);
  return grid;
}

static inline const LPTX_rectilinear_grid *LPTX_rectilinear_grid_wrap_d(
  LPTX_rectilinear_grid *grid, const double *coords_i, const double *coords_j,
  const double *coords_k, LPTX_idtype nx, LPTX_idtype ny, LPTX_idtype nz,
  LPTX_idtype mx, LPTX_idtype my, LPTX_idtype mz, LPTX_idtype stmx,
  LPTX_idtype stmy, LPTX_idtype stmz)
{
  *grid = LPTX_rectilinear_grid_d(coords_i, coords_j, coords_k, nx, ny, nz, mx,
                                  my, mz, stmx, stmy, stmz);
  return grid;
}

#define LPTX_RECT_GRID_WRAP(fn)                                             \
  fn(&((LPTX_rectilinear_grid){.nx = 0}), coords_i, coords_j, coords_k, nx, \
     ny, nz, mx, my, mz, stmx, stmy, stmz)

#define LPTX_RECT_GRID_T() LPTX_RECT_GRID_WRAP(LPTX_rectilinear_grid_wrap_t)
#define LPTX_RECT_GRID_F() LPTX_RECT_GRID_WRAP(LPTX_rectilinear_grid_wrap_f)
#define LPTX_RECT_GRID_D() LPTX_RECT_GRID_WRAP(LPTX_rectilinear_grid_wrap_d)

static inline const LPTX_rectilinear_scalar *LPTX_rectilinear_scalar_wrap_t(
  LPTX_rectilinear_scalar *scalar, const LPTX_type *value, LPTX_idtype nx,
  LPTX_idtype ny, LPTX_idtype nz, LPTX_idtype mx, LPTX_idtype my,
  LPTX_idtype mz, LPTX_idtype stmx, LPTX_idtype stmy, LPTX_idtype stmz)
{
  *scalar =
    LPTX_rectilinear_scalar_t(value, nx, ny, nz, mx, my, mz, stmx, stmy, stmz);
  return scalar;
}

static inline const LPTX_rectilinear_scalar *LPTX_rectilinear_scalar_wrap_f(
  LPTX_rectilinear_scalar *scalar, const float *value, LPTX_idtype nx,
  LPTX_idtype ny, LPTX_idtype nz, LPTX_idtype mx, LPTX_idtype my,
  LPTX_idtype mz, LPTX_idtype stmx, LPTX_idtype stmy, LPTX_idtype stmz)
{
  *scalar =
    LPTX_rectilinear_scalar_f(value, nx, ny, nz, mx, my, mz, stmx, stmy, stmz);
  return scalar;
}

static inline const LPTX_rectilinear_scalar *LPTX_rectilinear_scalar_wrap_d(
  LPTX_rectilinear_scalar *scalar, const double *value, LPTX_idtype nx,
  LPTX_idtype ny, LPTX_idtype nz, LPTX_idtype mx, LPTX_idtype my,
  LPTX_idtype mz, LPTX_idtype stmx, LPTX_idtype stmy, LPTX_idtype stmz)
{
  *scalar =
    LPTX_rectilinear_scalar_d(value, nx, ny, nz, mx, my, mz, stmx, stmy, stmz);
  return scalar;
}

#define LPTX_RECT_SCALAR_WRAP(fn, value)                                   \
  fn(&((LPTX_rectilinear_scalar){.nx = 0}), value, nx, ny, nz, mx, my, mz, \
     stmx, stmy, stmz)

#define LPTX_RECT_SCALAR_T(value) \
  LPTX_RECT_SCALAR_WRAP(LPTX_rectilinear_scalar_wrap_t, value)
#define LPTX_RECT_SCALAR_F(value) \
  LPTX_RECT_SCALAR_WRAP(LPTX_rectilinear_scalar_wrap_f, value)
#define LPTX_RECT_SCALAR_D(value) \
  LPTX_RECT_SCALAR_WRAP(LPTX_rectilinear_scalar_wrap_d, value)

static inline const LPTX_rectilinear_vector *LPTX_rectilinear_vector_wrap_t(
  LPTX_rectilinear_vector *vector, const LPTX_type *value_x,
  const LPTX_type *value_y, const LPTX_type *value_z, LPTX_idtype nx,
  LPTX_idtype ny, LPTX_idtype nz, LPTX_idtype mx, LPTX_idtype my,
  LPTX_idtype mz, LPTX_idtype stmx, LPTX_idtype stmy, LPTX_idtype stmz)
{
  *vector = LPTX_rectilinear_vector_t(value_x, value_y, value_z, nx, ny, nz, mx,
                                      my, mz, stmx, stmy, stmz);
  return vector;
}

static inline const LPTX_rectilinear_vector *LPTX_rectilinear_vector_wrap_f(
  LPTX_rectilinear_vector *vector, const float *value_x, const float *value_y,
  const float *value_z, LPTX_idtype nx, LPTX_idtype ny, LPTX_idtype nz,
  LPTX_idtype mx, LPTX_idtype my, LPTX_idtype mz, LPTX_idtype stmx,
  LPTX_idtype stmy, LPTX_idtype stmz)
{
  *vector = LPTX_rectilinear_vector_f(value_x, value_y, value_z, nx, ny, nz, mx,
                                      my, mz, stmx, stmy, stmz);
  return vector;
}

static inline const LPTX_rectilinear_vector *LPTX_rectilinear_vector_wrap_d(
  LPTX_rectilinear_vector *vector, const double *value_x, const double *value_y,
  const double *value_z, LPTX_idtype nx, LPTX_idtype ny, LPTX_idtype nz,
  LPTX_idtype mx, LPTX_idtype my, LPTX_idtype mz, LPTX_idtype stmx,
  LPTX_idtype stmy, LPTX_idtype stmz)
{
  *vector = LPTX_rectilinear_vector_d(value_x, value_y, value_z, nx, ny, nz, mx,
                                      my, mz, stmx, stmy, stmz);
  return vector;
}

#define LPTX_RECT_VECTOR_WRAP(fn, vx, vy, vz)                               \
  fn(&((LPTX_rectilinear_vector){.nx = 0}), vx, vy, vz, nx, ny, nz, mx, my, \
     mz, stmx, stmy, stmz)

#define LPTX_RECT_VECTOR_T(vx, vy, vz) \
  LPTX_RECT_VECTOR_WRAP(LPTX_rectilinear_vector_wrap_t, vx, vy, vz)
#define LPTX_RECT_VECTOR_F(vx, vy, vz) \
  LPTX_RECT_VECTOR_WRAP(LPTX_rectilinear_vector_wrap_f, vx, vy, vz)
#define LPTX_RECT_VECTOR_D(vx, vy, vz) \
  LPTX_RECT_VECTOR_WRAP(LPTX_rectilinear_vector_wrap_d, vx, vy, vz)

JUPITER_LPTX_DECL_END

#endif
