#ifndef JUPITER_LPTX_PRIV_ARRAY_TYPES_P_H
#define JUPITER_LPTX_PRIV_ARRAY_TYPES_P_H

#include "defs.h"
#include "error.h"
#include "jupiter/geometry/bitarray.h"
#include "jupiter/geometry/util.h"
#include "jupiter/random/random.h"
#include "priv_array_data.h"
#include "ptflags.h"
#include "pvector.h"
#include "struct_defs.h"

#include <stddef.h>

JUPITER_LPTX_DECL_START

/*
 * Particle-side data provider implementation of API for extract/assign one of
 * properties of particle data to/from plain array.
 *
 * This file is private header.
 */

#define LPTX_DEFINE_PTMEM_BASE(name, setget_datatype, pfunc_datatype,        \
                               afunc_datatype, pttype, functype, macro, mem, \
                               nfunc)                                        \
  struct name##_data                                                         \
  {                                                                          \
    setget_datatype d;                                                       \
  };                                                                         \
                                                                             \
  static inline void name##_f(pttype *particle, LPTX_idtype component_index, \
                              const pfunc_datatype *pdata,                   \
                              const afunc_datatype *adata)                   \
  {                                                                          \
    macro(particle, component_index, pdata, adata, &particle->base.mem);     \
  }                                                                          \
                                                                             \
  static const functype name##_funcs = {                                     \
    .fn = nfunc,                                                             \
    .f = name##_f,                                                           \
  };

#define LPTX_DEFINE_PTMEM_GET_BASE(name, setget_datatype, macro, mem, nfunc)   \
  LPTX_DEFINE_PTMEM_BASE(name, setget_datatype, struct LPTX_particle_get_data, \
                         struct LPTX_array_geti_data,                          \
                         const LPTX_particle_data,                             \
                         struct LPTX_particle_get_funcs, macro, mem, nfunc)

#define LPTX_DEFINE_PTMEM_SET_BASE(name, setget_datatype, macro, mem, nfunc)   \
  LPTX_DEFINE_PTMEM_BASE(name, setget_datatype, struct LPTX_particle_set_data, \
                         struct LPTX_array_seti_data, LPTX_particle_data,      \
                         struct LPTX_particle_set_funcs, macro, mem, nfunc)

//--- particle base member getter for scalar or for POD copy

static inline size_t
LPTX_particle_get_scalar_n(const LPTX_particle_data *p,
                           const struct LPTX_particle_get_data *args)
{
  return 1;
}

#define LPTX_ARRAY_GET_S_PTMEM_F(pt, ci, pdata, adata, memp) \
  LPTX_array_geti_call(adata, memp)

#define LPTX_DEFINE_ARRAY_GET_S_PTMEM(name, mem)                  \
  LPTX_DEFINE_PTMEM_GET_BASE(name, struct LPTX_particle_get_data, \
                             LPTX_ARRAY_GET_S_PTMEM_F, mem,       \
                             LPTX_particle_get_scalar_n)

#define LPTX_PARTICLE_GET_S_PTMEM_INIT(name) \
  (&(((struct name##_data){.d = {.funcs = &name##_funcs}}).d))

#define LPTX_PTMEM_GET_S_DTYPE(mem) \
  (&((const LPTX_particle){.particle_id = 0}.mem))

//--- particle base member setter for scalar or for POD copy

static inline size_t
LPTX_particle_set_scalar_n(const LPTX_particle_data *p,
                           const struct LPTX_particle_set_data *args)
{
  return 1;
}

#define LPTX_ARRAY_SET_S_PTMEM_F(pt, ci, pdata, adata, memp) \
  LPTX_array_seti_call(adata, memp)

#define LPTX_DEFINE_ARRAY_SET_S_PTMEM(name, mem)                  \
  LPTX_DEFINE_PTMEM_SET_BASE(name, struct LPTX_particle_set_data, \
                             LPTX_ARRAY_SET_S_PTMEM_F, mem,       \
                             LPTX_particle_set_scalar_n)

#define LPTX_PARTICLE_SET_S_PTMEM_INIT(name) \
  (&(((struct name##_data){.d = {.funcs = &name##_funcs}}).d))

#define LPTX_PTMEM_SET_S_DTYPE(mem) (&((LPTX_particle){.particle_id = 0}.mem))

//--- particle base member getter for LPTX_vector to scalar types

static inline size_t
LPTX_particle_get_vector_n(const LPTX_particle_data *p,
                           const struct LPTX_particle_get_data *args)
{
  return 3;
}

static inline const LPTX_type *
LPTX_particle_get_vector_p(const LPTX_vector *v, LPTX_idtype component_index)
{
  switch (component_index) {
  case 0:
    return &v->x;
  case 1:
    return &v->y;
  case 2:
    return &v->z;
  }
  return NULL; /* Make error for excess or invalid components */
}

static inline void
LPTX_particle_get_vector_f(const LPTX_vector *v, LPTX_idtype component_index,
                           const struct LPTX_array_geti_data *adata)
{
  LPTX_array_geti_call(adata, LPTX_particle_get_vector_p(v, component_index));
}

#define LPTX_ARRAY_GET_V_PTMEM_F(pt, ci, pdata, adata, memp) \
  LPTX_particle_get_vector_f(memp, ci, adata)

#define LPTX_DEFINE_ARRAY_GET_V_PTMEM(name, mem)                  \
  LPTX_DEFINE_PTMEM_GET_BASE(name, struct LPTX_particle_get_data, \
                             LPTX_ARRAY_GET_V_PTMEM_F, mem,       \
                             LPTX_particle_get_vector_n)

#define LPTX_PARTICLE_GET_V_PTMEM_INIT(name) \
  (&(((struct name##_data){.d = {.funcs = &name##_funcs}}).d))

#define LPTX_PTMEM_GET_V_DTYPE(mem) LPTX_PTMEM_GET_S_DTYPE(mem.x)

//--- particle base member setter for LPTX_vector from scalar types

static inline size_t
LPTX_particle_set_vector_n(const LPTX_particle_data *p,
                           const struct LPTX_particle_set_data *args)
{
  return 3;
}

static inline LPTX_type *LPTX_particle_set_vector_p(LPTX_vector *v,
                                                    LPTX_idtype component_index)
{
  switch (component_index) {
  case 0:
    return &v->x;
  case 1:
    return &v->y;
  case 2:
    return &v->z;
  }
  return NULL; /* Make error for excess or invalid components */
}

static inline void
LPTX_particle_set_vector_f(LPTX_vector *v, LPTX_idtype component_index,
                           const struct LPTX_array_seti_data *adata)
{
  LPTX_array_seti_call(adata, LPTX_particle_set_vector_p(v, component_index));
}

#define LPTX_ARRAY_SET_V_PTMEM_F(pt, ci, pdata, adata, memp) \
  LPTX_particle_set_vector_f(memp, ci, adata)

#define LPTX_DEFINE_ARRAY_SET_V_PTMEM(name, mem)                  \
  LPTX_DEFINE_PTMEM_SET_BASE(name, struct LPTX_particle_set_data, \
                             LPTX_ARRAY_SET_V_PTMEM_F, mem,       \
                             LPTX_particle_set_vector_n)

#define LPTX_PARTICLE_SET_V_PTMEM_INIT(name) \
  (&(((struct name##_data){.d = {.funcs = &name##_funcs}}).d))

#define LPTX_PTMEM_SET_V_DTYPE(mem) LPTX_PTMEM_SET_S_DTYPE(mem.x)

//---- particle base member getter for jupiter_random_seed to scalar type

/*
 * @note Get/Set from narrower than uint64_t type will reduce random quality.
 * @note Get/Set from wider than uint64_t type will discards higher bits.
 * @note Get/Set from signed type is not recommended.
 *
 * It is treated like single 256-bit integer rather than uint64_t array of
 * JUPITER_RANDOM_SEED_SIZE (4) elements.
 */

static inline size_t
LPTX_particle_get_seedi_n(const LPTX_particle_data *p,
                          const struct LPTX_particle_get_data *args)
{
  return JUPITER_RANDOM_SEED_SIZE;
}

static inline const uint64_t *
LPTX_particle_get_seedi_p(const jupiter_random_seed *p,
                          LPTX_idtype component_index)
{
  if (component_index < 0)
    return NULL;
  if (component_index < JUPITER_RANDOM_SEED_SIZE)
    return &p->seed[component_index];
  return NULL;
}

static inline void
LPTX_particle_get_seedi_f(const jupiter_random_seed *p,
                          LPTX_idtype component_index,
                          const struct LPTX_array_geti_data *adata)
{
  LPTX_array_geti_call(adata, LPTX_particle_get_seedi_p(p, component_index));
}

#define LPTX_ARRAY_GET_SEEDI_PTMEM_F(pt, ci, pdata, adata, memp) \
  LPTX_particle_get_seedi_f(memp, ci, adata)

#define LPTX_DEFINE_ARRAY_GET_SEEDI_PTMEM(name, mem)              \
  LPTX_DEFINE_PTMEM_GET_BASE(name, struct LPTX_particle_get_data, \
                             LPTX_ARRAY_GET_SEEDI_PTMEM_F, mem,   \
                             LPTX_particle_get_seedi_n)

#define LPTX_PARTICLE_GET_SEEDI_PTMEM_INIT(name) \
  (&(((struct name##_data){.d = {.funcs = &name##_funcs}}).d))

#define LPTX_PTMEM_GET_SEEDI_DTYPE(mem) LPTX_PTMEM_GET_S_DTYPE(mem.seed[0])

//---- particle base member setter for jupiter_random_seed from scalar type

static inline size_t
LPTX_particle_set_seedi_n(const LPTX_particle_data *p,
                          const struct LPTX_particle_set_data *args)
{
  return JUPITER_RANDOM_SEED_SIZE;
}

static inline uint64_t *LPTX_particle_set_seedi_p(jupiter_random_seed *p,
                                                  LPTX_idtype component_index)
{
  if (component_index < 0)
    return NULL;
  if (component_index < JUPITER_RANDOM_SEED_SIZE)
    return &p->seed[component_index];
  return NULL;
}

static inline void
LPTX_particle_set_seedi_f(jupiter_random_seed *p, LPTX_idtype component_index,
                          const struct LPTX_array_seti_data *adata)
{
  LPTX_array_seti_call(adata, LPTX_particle_set_seedi_p(p, component_index));
}

#define LPTX_ARRAY_SET_SEEDI_PTMEM_F(pt, ci, pdata, adata, memp) \
  LPTX_particle_set_seedi_f(memp, ci, adata)

#define LPTX_DEFINE_ARRAY_SET_SEEDI_PTMEM(name, mem)              \
  LPTX_DEFINE_PTMEM_SET_BASE(name, struct LPTX_particle_set_data, \
                             LPTX_ARRAY_SET_SEEDI_PTMEM_F, mem,   \
                             LPTX_particle_set_seedi_n)

#define LPTX_PARTICLE_SET_SEEDI_PTMEM_INIT(name) \
  (&(((struct name##_data){.d = {.funcs = &name##_funcs}}).d))

#define LPTX_PTMEM_SET_SEEDI_DTYPE(mem) LPTX_PTMEM_SET_S_DTYPE(mem.seed[0])

//---- particle base member getter for each bits in LPTX_particle_flags

/*
 * @note The function uses LPTX_bool to intermediate type
 */

static inline size_t
LPTX_particle_get_flags_n(const LPTX_particle_data *p,
                          const struct LPTX_particle_get_data *args)
{
  return LPTX_PTFLAG_MAX;
}

static inline void
LPTX_particle_get_flags_f(const LPTX_particle_flags *f,
                          LPTX_idtype component_index,
                          const struct LPTX_array_geti_data *adata)
{
  LPTX_bool b;
  LPTX_assert(component_index >= 0 && component_index < LPTX_PTFLAG_MAX);
  b = LPTX_particle_flags_get(f, (LPTX_particle_flag)component_index);
  LPTX_array_geti_call(adata, &b);
}

#define LPTX_ARRAY_GET_FLAGS_PTMEM_F(pt, ci, pdata, adata, memp) \
  LPTX_particle_get_flags_f(memp, ci, adata)

#define LPTX_DEFINE_ARRAY_GET_FLAGS_PTMEM(name, mem)              \
  LPTX_DEFINE_PTMEM_GET_BASE(name, struct LPTX_particle_get_data, \
                             LPTX_ARRAY_GET_FLAGS_PTMEM_F, mem,   \
                             LPTX_particle_get_flags_n)

#define LPTX_PARTICLE_GET_FLAGS_PTMEM_INIT(name) \
  (&(((struct name##_data){.d = {.funcs = &name##_funcs}}).d))

#define LPTX_PTMEM_SET_FLAGS_DTYPE(mem) (&(LPTX_bool){LPTX_false})

//---- particle base member setter for each bits in LPTX_particle_flags

static inline size_t
LPTX_particle_set_flags_n(const LPTX_particle_data *p,
                          const struct LPTX_particle_set_data *args)
{
  return LPTX_PTFLAG_MAX;
}

static inline void
LPTX_particle_set_flags_f(LPTX_particle_flags *f, LPTX_idtype component_index,
                          const struct LPTX_array_seti_data *adata)
{
  LPTX_bool b = LPTX_false;
  LPTX_array_seti_call(adata, &b);
  LPTX_particle_flags_set(f, (LPTX_particle_flag)component_index, b);
}

#define LPTX_ARRAY_SET_FLAGS_PTMEM_F(pt, ci, pdata, adata, memp) \
  LPTX_particle_set_flags_f(memp, ci, adata)

#define LPTX_DEFINE_ARRAY_SET_FLAGS_PTMEM(name, mem)              \
  LPTX_DEFINE_PTMEM_SET_BASE(name, struct LPTX_particle_set_data, \
                             LPTX_ARRAY_SET_FLAGS_PTMEM_F, mem,   \
                             LPTX_particle_set_flags_n)

#define LPTX_PARTICLE_SET_FLAGS_PTMEM_INIT(name) \
  (&(((struct name##_data){.d = {.funcs = &name##_funcs}}).d))

#define LPTX_PTMEM_GET_FLAGS_DTYPE(mem) (&(const LPTX_bool){LPTX_false})

//--- particle base member getter for LPTX_particle_flags from scalar type

/*
 * @note This is not for writing to file, since geom_bitarray_element_type is
 *       not interchangeable type. Treating array of geom_bitarray_element_type
 *       as array of char is possible. However, endianness will not be
 *       converted.
 * @note Set/Get from types other than geom_bitarray_element_type may lose
 *       data for some bits.
 *
 * Extracting single element of geom_bitarray_element_type is pointless.
 */

static inline size_t
LPTX_particle_get_flagsp_n(const LPTX_particle_data *p,
                           const struct LPTX_particle_get_data *args)
{
  return LPTX_particle_flags_N;
}

static inline const geom_bitarray_element_type *
LPTX_particle_get_flagsp_p(const LPTX_particle_flags *f,
                           LPTX_idtype component_index)
{
  if (component_index < 0)
    return NULL;
  if (component_index < LPTX_particle_flags_N)
    return &f->v[component_index];
  return NULL;
}

static inline void
LPTX_particle_get_flagsp_f(const LPTX_particle_flags *f,
                           LPTX_idtype component_index,
                           const struct LPTX_array_geti_data *adata)
{
  LPTX_array_geti_call(adata, LPTX_particle_get_flagsp_p(f, component_index));
}

#define LPTX_ARRAY_GET_FLAGSP_PTMEM_F(pt, ci, pdata, adata, memp) \
  LPTX_particle_get_flagsp_f(memp, ci, adata)

#define LPTX_DEFINE_ARRAY_GET_FLAGSP_PTMEM(name, mem)             \
  LPTX_DEFINE_PTMEM_GET_BASE(name, struct LPTX_particle_get_data, \
                             LPTX_ARRAY_GET_FLAGSP_PTMEM_F, mem,  \
                             LPTX_particle_get_flagsp_n)

#define LPTX_PARTICLE_GET_FLAGSP_PTMEM_INIT(name) \
  (&(((struct name##_data){.d = {.funcs = &name##_funcs}}).d))

#define LPTX_PTMEM_GET_FLAGSP_DTYPE(mem) LPTX_PTMEM_GET_S_DTYPE(mem.v[0])

//--- particle base member setter for LPTX_particle_flags from scalar type

static inline size_t
LPTX_particle_set_flagsp_n(const LPTX_particle_data *p,
                           const struct LPTX_particle_set_data *args)
{
  return LPTX_particle_flags_N;
}

static inline geom_bitarray_element_type *
LPTX_particle_set_flagsp_p(LPTX_particle_flags *f, LPTX_idtype component_index)
{
  if (component_index < 0)
    return NULL;
  if (component_index < LPTX_particle_flags_N)
    return &f->v[component_index];
  return NULL;
}

static inline void
LPTX_particle_set_flagsp_f(LPTX_particle_flags *f, LPTX_idtype component_index,
                           const struct LPTX_array_seti_data *adata)
{
  LPTX_array_seti_call(adata, LPTX_particle_set_flagsp_p(f, component_index));
}

#define LPTX_ARRAY_SET_FLAGSP_PTMEM_F(pt, ci, pdata, adata, memp) \
  LPTX_particle_set_flagsp_f(memp, ci, adata)

#define LPTX_DEFINE_ARRAY_SET_FLAGSP_PTMEM(name, mem)             \
  LPTX_DEFINE_PTMEM_SET_BASE(name, struct LPTX_particle_set_data, \
                             LPTX_ARRAY_SET_FLAGSP_PTMEM_F, mem,  \
                             LPTX_particle_set_flagsp_n)

#define LPTX_PARTICLE_SET_FLAGSP_PTMEM_INIT(name) \
  (&(((struct name##_data){.d = {.funcs = &name##_funcs}}).d))

#define LPTX_PTMEM_SET_FLAGSP_DTYPE(mem) LPTX_PTMEM_SET_S_DTYPE(mem.v[0])

//--- particle vector getter

struct LPTX_particle_get_pvec_data
{
  struct LPTX_particle_get_data d;
  LPTX_idtype vector_index;
};
#define LPTX_particle_get_pvec_entry(ptr) \
  geom_container_of(ptr, struct LPTX_particle_get_pvec_data, d)

static inline size_t
LPTX_particle_get_pvec_n(const LPTX_particle_data *p,
                         const struct LPTX_particle_get_data *args)
{
  const LPTX_particle_vector *v;
  const struct LPTX_particle_get_pvec_data *a;

  a = LPTX_particle_get_pvec_entry(args);
  v = LPTX_particle_data_get_vector(p, a->vector_index);
  if (!v)
    return 0;
  if (v->length < 0)
    return 0;
  return v->length;
}

static inline const LPTX_type *
LPTX_particle_get_pvec_p(const LPTX_particle_data *p,
                         const struct LPTX_particle_get_pvec_data *args,
                         LPTX_idtype component_index)
{
  const LPTX_particle_vector *v;
  v = LPTX_particle_data_get_vector(p, args->vector_index);

  if (!v)
    return NULL;
  return LPTX_particle_vector_getvp(v, component_index);
}

static inline void
LPTX_particle_get_pvec_f(const LPTX_particle_data *particle,
                         LPTX_idtype component_index,
                         const struct LPTX_particle_get_data *pdata,
                         const struct LPTX_array_geti_data *adata)
{
  const LPTX_type *src;
  const struct LPTX_particle_get_pvec_data *a;
  a = LPTX_particle_get_pvec_entry(pdata);
  src = LPTX_particle_get_pvec_p(particle, a, component_index);
  LPTX_array_geti_call(adata, src);
}

static const struct LPTX_particle_get_funcs LPTX_particle_get_pvec_funcs = {
  .fn = LPTX_particle_get_pvec_n,
  .f = LPTX_particle_get_pvec_f,
};

#define LPTX_PARTICLE_GET_PVEC_INIT(vindex)            \
  (&(((struct LPTX_particle_get_pvec_data){            \
        .d = {.funcs = &LPTX_particle_get_pvec_funcs}, \
        .vector_index = vindex,                        \
      })                                               \
       .d))

#define LPTX_PVEC_GET_DTYPE() ((const LPTX_type[]){LPTX_C(0.0)})

//--- particle vector setter

struct LPTX_particle_set_pvec_data
{
  struct LPTX_particle_set_data d;
  LPTX_idtype vector_index;
};
#define LPTX_particle_set_pvec_entry(ptr) \
  geom_container_of(ptr, struct LPTX_particle_set_pvec_data, d)

static inline size_t
LPTX_particle_set_pvec_n(const LPTX_particle_data *p,
                         const struct LPTX_particle_set_data *args)
{
  const LPTX_particle_vector *v;
  const struct LPTX_particle_set_pvec_data *a;

  a = LPTX_particle_set_pvec_entry(args);
  v = LPTX_particle_data_get_vector(p, a->vector_index);
  if (!v)
    return 0;
  if (v->length < 0)
    return 0;
  return v->length;
}

static inline LPTX_type *
LPTX_particle_set_pvec_p(LPTX_particle_data *p,
                         const struct LPTX_particle_set_pvec_data *args,
                         LPTX_idtype component_index)
{
  LPTX_particle_vector *v;
  v = LPTX_particle_data_get_vector_writable(p, args->vector_index);
  if (!v)
    return NULL;
  return LPTX_particle_vector_getvp_writable(v, component_index);
}

static inline void
LPTX_particle_set_pvec_f(LPTX_particle_data *particle,
                         LPTX_idtype component_index,
                         const struct LPTX_particle_set_data *pdata,
                         const struct LPTX_array_seti_data *adata)
{
  LPTX_type *dest;
  const struct LPTX_particle_set_pvec_data *a;
  a = LPTX_particle_set_pvec_entry(pdata);
  dest = LPTX_particle_set_pvec_p(particle, a, component_index);
  LPTX_array_seti_call(adata, dest);
}

static const struct LPTX_particle_set_funcs LPTX_particle_set_pvec_funcs = {
  .fn = LPTX_particle_set_pvec_n,
  .f = LPTX_particle_set_pvec_f,
};

#define LPTX_PARTICLE_SET_PVEC_INIT(vindex)            \
  (&(((struct LPTX_particle_set_pvec_data){            \
        .d = {.funcs = &LPTX_particle_set_pvec_funcs}, \
        .vector_index = vindex,                        \
      })                                               \
       .d))

#define LPTX_PVEC_SET_DTYPE() ((LPTX_type[]){LPTX_C(0.0)})

//--- particle general fluid-side scalar property setter

static inline size_t
LPTX_particle_set_fluid_cb_scalar_n(const LPTX_particle_data *p,
                                    const struct LPTX_particle_set_data *args)
{
  return 1;
}

struct LPTX_particle_set_fluid_cb_scalar_data
{
  struct LPTX_particle_set_data d;
  LPTX_cb_fluid_variable_f *func;
  void *arg;
};
#define LPTX_particle_set_fluid_cb_scalar_entry(ptr) \
  geom_container_of(ptr, struct LPTX_particle_set_fluid_cb_scalar_data, d)

static inline void
LPTX_particle_set_fluid_cb_scalar_f(LPTX_type *dest,
                                    const LPTX_particle_data *particle,
                                    const struct LPTX_particle_set_data *pdata)
{
  const LPTX_particle *b;
  struct LPTX_particle_set_fluid_cb_scalar_data *p;

  p = LPTX_particle_set_fluid_cb_scalar_entry(pdata);
  b = &particle->base;
  *dest = p->func(p->arg, b->position, b->icfpt, b->jcfpt, b->kcfpt);
}

#define LPTX_FLUID_CB_SET_S_PTMEM_F(pt, ci, pdata, adata, memp) \
  LPTX_particle_set_fluid_cb_scalar_f(memp, pt, pdata)

#define LPTX_DEFINE_FLUID_CB_SET_S_PTMEM(name, mem)                         \
  LPTX_DEFINE_PTMEM_SET_BASE(name,                                          \
                             struct LPTX_particle_set_fluid_cb_scalar_data, \
                             LPTX_FLUID_CB_SET_S_PTMEM_F, mem,              \
                             LPTX_particle_set_fluid_cb_scalar_n)

#define LPTX_PARTICLE_FLUID_CB_SET_S_INIT(name, f, a) \
  (&(((struct name##_data){                           \
        .d =                                          \
          {                                           \
            .d = {.funcs = &name##_funcs},            \
            .func = f,                                \
            .arg = a,                                 \
          },                                          \
      })                                              \
       .d.d))

//--- particle general fluid-side vector property setter

static inline size_t
LPTX_particle_set_fluid_cb_vector_n(const LPTX_particle_data *p,
                                    const struct LPTX_particle_set_data *args)
{
  return 1;
}

struct LPTX_particle_set_fluid_cb_vector_data
{
  struct LPTX_particle_set_data d;
  LPTX_cb_fluid_variable_v *func;
  void *arg;
};
#define LPTX_particle_set_fluid_cb_vector_entry(ptr) \
  geom_container_of(ptr, struct LPTX_particle_set_fluid_cb_vector_data, d)

static inline void
LPTX_particle_set_fluid_cb_vector_f(LPTX_vector *dest,
                                    const LPTX_particle_data *particle,
                                    const struct LPTX_particle_set_data *pdata)
{
  const LPTX_particle *b;
  struct LPTX_particle_set_fluid_cb_vector_data *p;

  p = LPTX_particle_set_fluid_cb_vector_entry(pdata);
  b = &particle->base;
  *dest = p->func(p->arg, b->position, b->icfpt, b->jcfpt, b->kcfpt);
}

#define LPTX_FLUID_CB_SET_V_PTMEM_F(pt, ci, pdata, adata, memp) \
  LPTX_particle_set_fluid_cb_vector_f(memp, pt, pdata)

#define LPTX_DEFINE_FLUID_CB_SET_V_PTMEM(name, mem)                         \
  LPTX_DEFINE_PTMEM_SET_BASE(name,                                          \
                             struct LPTX_particle_set_fluid_cb_vector_data, \
                             LPTX_FLUID_CB_SET_V_PTMEM_F, mem,              \
                             LPTX_particle_set_fluid_cb_vector_n)

#define LPTX_PARTICLE_FLUID_CB_SET_V_INIT(name, f, a) \
  (&(((struct name##_data){                           \
        .d =                                          \
          {                                           \
            .d = {.funcs = &name##_funcs},            \
            .func = f,                                \
            .arg = a,                                 \
          },                                          \
      })                                              \
       .d.d))

//--- particle fluid index setter

struct LPTX_particle_set_fluid_cb_index_data
{
  struct LPTX_particle_set_data d;
  LPTX_cb_fluid_index *func;
  void *arg;
};
#define LPTX_particle_set_fluid_cb_index_entry(ptr) \
  geom_container_of(ptr, struct LPTX_particle_set_fluid_cb_index_data, d)

static inline size_t
LPTX_particle_set_fluid_cb_index_n(const LPTX_particle_data *particle,
                                   const struct LPTX_particle_set_data *pdata)
{
  return 1;
}

static inline void
LPTX_particle_set_fluid_cb_index_f(LPTX_particle_data *particle,
                                   LPTX_idtype component_index,
                                   const struct LPTX_particle_set_data *pdata,
                                   const struct LPTX_array_seti_data *adata)
{
  struct LPTX_particle_set_fluid_cb_index_data *a;
  LPTX_particle *p;
  LPTX_cb_fluid_index_flags r;
  LPTX_idtype i, j, k;

  a = LPTX_particle_set_fluid_cb_index_entry(pdata);
  p = &particle->base;
  r = a->func(a->arg, p->position, &i, &j, &k);
  if (r != LPTX_FLUID_INDEX_OK)
    return;

  p->icfpt = i;
  p->jcfpt = j;
  p->kcfpt = k;
}

static const struct LPTX_particle_set_funcs
  LPTX_particle_set_fluid_cb_index_funcs = {
    .fn = LPTX_particle_set_fluid_cb_index_n,
    .f = LPTX_particle_set_fluid_cb_index_f,
};

#define LPTX_PARTICLE_FLUID_CB_SET_I_INIT(f, a)                  \
  (&(((struct LPTX_particle_set_fluid_cb_index_data){            \
        .d = {.funcs = &LPTX_particle_set_fluid_cb_index_funcs}, \
        .func = f,                                               \
        .arg = a,                                                \
      })                                                         \
       .d))

//--- particle fluid scalar property from rectilinear grid definition

typedef LPTX_type
LPTX_cb_fluid_rect_cs_func(const LPTX_rectilinear_grid *grid,
                           const struct LPTX_fluid_rect_args *s,
                           LPTX_vector position, LPTX_idtype icfpt,
                           LPTX_idtype jcfpt, LPTX_idtype kcfpt, void *arg);

struct LPTX_fluid_rect_cs_args
{
  const LPTX_rectilinear_grid *grid;
  const struct LPTX_fluid_rect_args *d;
  LPTX_cb_fluid_rect_cs_func *func;
  void *arg;
};

static inline LPTX_type LPTX_fluid_rect_cs_func(void *arg, LPTX_vector position,
                                                LPTX_idtype icfpt,
                                                LPTX_idtype jcfpt,
                                                LPTX_idtype kcfpt)
{
  struct LPTX_fluid_rect_cs_args *a;
  a = (struct LPTX_fluid_rect_cs_args *)arg;

  return a->func(a->grid, a->d, position, icfpt, jcfpt, kcfpt, a->arg);
}

#define LPTX_fluid_rect_cs__args_init(grid_, d_, func_, arg_) \
  (&((struct LPTX_fluid_rect_cs_args){                        \
    .grid = grid_,                                            \
    .d = d_,                                                  \
    .func = func_,                                            \
    .arg = arg_,                                              \
  }))

#define LPTX_PARTICLE_FLUID_RECT_CS_INIT(generic_cb_type, grid, d, func, arg) \
  LPTX_PARTICLE_FLUID_CB_SET_S_INIT(generic_cb_type, LPTX_fluid_rect_cs_func, \
                                    LPTX_fluid_rect_cs__args_init(grid, d,    \
                                                                  func, arg))

//--- particle fluid vector property from rectilinear grid definition

typedef LPTX_vector
LPTX_cb_fluid_rect_cv_func(const LPTX_rectilinear_grid *grid,
                           const struct LPTX_fluid_rect_args *s,
                           LPTX_vector position, LPTX_idtype icfpt,
                           LPTX_idtype jcfpt, LPTX_idtype kcfpt, void *arg);

struct LPTX_fluid_rect_cv_args
{
  const LPTX_rectilinear_grid *grid;
  const struct LPTX_fluid_rect_args *d;
  LPTX_cb_fluid_rect_cv_func *func;
  void *arg;
};

static inline LPTX_vector
LPTX_fluid_rect_cv_func(void *arg, LPTX_vector position, LPTX_idtype icfpt,
                        LPTX_idtype jcfpt, LPTX_idtype kcfpt)
{
  struct LPTX_fluid_rect_cv_args *a;
  a = (struct LPTX_fluid_rect_cv_args *)arg;

  return a->func(a->grid, a->d, position, icfpt, jcfpt, kcfpt, a->arg);
}

static inline struct LPTX_fluid_rect_cv_args
LPTX_fluid_rect_cv_args_init(const LPTX_rectilinear_grid *grid,
                             const struct LPTX_fluid_rect_args *d,
                             LPTX_cb_fluid_rect_cv_func *func, void *arg)
{
  return (struct LPTX_fluid_rect_cv_args){
    .grid = grid,
    .d = d,
    .func = func,
    .arg = arg,
  };
}

#define LPTX_fluid_rect_cv__args_init(grid_, d_, func_, arg_) \
  (&((struct LPTX_fluid_rect_cv_args){                        \
    .grid = grid_,                                            \
    .d = d_,                                                  \
    .func = func_,                                            \
    .arg = arg_,                                              \
  }))

#define LPTX_PARTICLE_FLUID_RECT_CV_INIT(generic_cb_type, grid, d, func, arg) \
  LPTX_PARTICLE_FLUID_CB_SET_V_INIT(generic_cb_type, LPTX_fluid_rect_cv_func, \
                                    LPTX_fluid_rect_cv__args_init(grid, d,    \
                                                                  func, arg))

JUPITER_LPTX_DECL_END

#endif
