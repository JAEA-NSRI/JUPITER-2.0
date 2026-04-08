#ifndef JUPITER_LPTX_PTFLAGS_H
#define JUPITER_LPTX_PTFLAGS_H

#include "defs.h"
#include "jupiter/geometry/bitarray.h"
#include "struct_defs.h"

JUPITER_LPTX_DECL_START

static inline LPTX_particle_flags LPTX_particle_flags_none(void)
{
  LPTX_particle_flags f;
  geom_bitarray_element_setall(f.v, LPTX_PTFLAG_MAX, 0);
  return f;
}

static inline LPTX_bool LPTX_particle_flags_get(const LPTX_particle_flags *f,
                                                LPTX_particle_flag e)
{
  return geom_bitarray_element_get(f->v, e) ? LPTX_true : LPTX_false;
}

static inline void LPTX_particle_flags_set(LPTX_particle_flags *f,
                                           LPTX_particle_flag e,
                                           LPTX_bool value)
{
  geom_bitarray_element_set(f->v, e, (value == LPTX_true) ? 1 : 0);
}

#define LPTX_DEFINE_PARTICLE_FLAGS_GETTER(fname, e)                     \
  static inline LPTX_bool LPTX_particle_##fname(const LPTX_particle *p) \
  {                                                                     \
    return LPTX_particle_flags_get(&p->flags, e);                       \
  }                                                                     \
                                                                        \
  static inline LPTX_bool LPTX_particle_data_##fname(                   \
    const LPTX_particle_data *p)                                        \
  {                                                                     \
    return LPTX_particle_##fname(&p->base);                             \
  }

LPTX_DEFINE_PARTICLE_FLAGS_GETTER(is_used, LPTX_PT_IS_USED)
LPTX_DEFINE_PARTICLE_FLAGS_GETTER(is_exited, LPTX_PT_IS_EXITED)
LPTX_DEFINE_PARTICLE_FLAGS_GETTER(is_collided, LPTX_PT_IS_COLLIDED)
LPTX_DEFINE_PARTICLE_FLAGS_GETTER(is_in_gas, LPTX_PT_IS_IN_GAS)
LPTX_DEFINE_PARTICLE_FLAGS_GETTER(is_in_liquid, LPTX_PT_IS_IN_LIQUID)
LPTX_DEFINE_PARTICLE_FLAGS_GETTER(is_in_solid, LPTX_PT_IS_IN_SOLID)
LPTX_DEFINE_PARTICLE_FLAGS_GETTER(can_collide, LPTX_PT_CAN_COLLIDE)
LPTX_DEFINE_PARTICLE_FLAGS_GETTER(can_evaporate, LPTX_PT_CAN_COLLIDE)
LPTX_DEFINE_PARTICLE_FLAGS_GETTER(can_condensate, LPTX_PT_CAN_CONDENSATE)

#define LPTX_DEFINE_PARTICLE_FLAGS_SETTER(fname, e)                           \
  static inline void LPTX_particle_##fname(LPTX_particle *p, LPTX_bool value) \
  {                                                                           \
    LPTX_particle_flags_set(&p->flags, e, value);                             \
  }                                                                           \
                                                                              \
  static inline void LPTX_particle_data_##fname(LPTX_particle_data *p,        \
                                                LPTX_bool value)              \
  {                                                                           \
    LPTX_particle_##fname(&p->base, value);                                   \
  }

LPTX_DEFINE_PARTICLE_FLAGS_SETTER(set_used, LPTX_PT_IS_USED)
LPTX_DEFINE_PARTICLE_FLAGS_SETTER(set_exited, LPTX_PT_IS_EXITED)
LPTX_DEFINE_PARTICLE_FLAGS_SETTER(set_collided, LPTX_PT_IS_COLLIDED)
LPTX_DEFINE_PARTICLE_FLAGS_SETTER(set_in_gas, LPTX_PT_IS_IN_GAS)
LPTX_DEFINE_PARTICLE_FLAGS_SETTER(set_in_liquid, LPTX_PT_IS_IN_LIQUID)
LPTX_DEFINE_PARTICLE_FLAGS_SETTER(set_in_solid, LPTX_PT_IS_IN_SOLID)
LPTX_DEFINE_PARTICLE_FLAGS_SETTER(set_can_collide, LPTX_PT_CAN_COLLIDE)
LPTX_DEFINE_PARTICLE_FLAGS_SETTER(set_can_evaporate, LPTX_PT_CAN_COLLIDE)
LPTX_DEFINE_PARTICLE_FLAGS_SETTER(set_can_condensate, LPTX_PT_CAN_CONDENSATE)

JUPITER_LPTX_DECL_END

#endif
