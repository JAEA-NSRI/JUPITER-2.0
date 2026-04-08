/**
 * @file lptx/init_set.h
 */

#ifndef JUPITER_LPTX_INIT_SET_H
#define JUPITER_LPTX_INIT_SET_H

#include "defs.h"

JUPITER_LPTX_DECL_START

JUPITER_LPTX_DECL
LPTX_particle_init_set *LPTX_particle_init_set_new(void);

JUPITER_LPTX_DECL
void LPTX_particle_init_set_delete(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_param *LPTX_particle_init_set_parent(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_particle_init_set *
LPTX_particle_init_set_next(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_particle_init_set *
LPTX_particle_init_set_prev(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_particle_init_set *
LPTX_particle_init_set_insert(LPTX_particle_init_set *prev,
                              LPTX_particle_init_set *item);

JUPITER_LPTX_DECL
LPTX_particle_init_set *
LPTX_particle_init_set_append(LPTX_param *param, LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_particle_init_set *
LPTX_particle_init_set_prepend(LPTX_param *param, LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
void LPTX_particle_init_set_set_number_of_particles(
  LPTX_particle_init_set *set, LPTX_idtype number_particles);

JUPITER_LPTX_DECL
void LPTX_particle_init_set_set_origin_id(LPTX_particle_init_set *set,
                                          LPTX_idtype origin_id);

JUPITER_LPTX_DECL
void LPTX_particle_init_set_set_range_start(LPTX_particle_init_set *set,
                                            LPTX_vector range_start);

JUPITER_LPTX_DECL
void LPTX_particle_init_set_set_range_end(LPTX_particle_init_set *set,
                                          LPTX_vector range_end);

JUPITER_LPTX_DECL
void LPTX_particle_init_set_set_time_start(LPTX_particle_init_set *set,
                                           LPTX_type time_start);

JUPITER_LPTX_DECL
void LPTX_particle_init_set_set_time_end(LPTX_particle_init_set *set,
                                         LPTX_type time_end);

JUPITER_LPTX_DECL
void LPTX_particle_init_set_set_time_random(LPTX_particle_init_set *set,
                                            LPTX_bool time_random);

JUPITER_LPTX_DECL
void LPTX_particle_init_set_set_diameter(LPTX_particle_init_set *set,
                                         LPTX_type diameter);

JUPITER_LPTX_DECL
void LPTX_particle_init_set_set_density(LPTX_particle_init_set *set,
                                        LPTX_type density);

JUPITER_LPTX_DECL
void LPTX_particle_init_set_set_initial_velocity(LPTX_particle_init_set *set,
                                                 LPTX_vector velocity);

JUPITER_LPTX_DECL
void LPTX_particle_init_set_set_temperature(LPTX_particle_init_set *set,
                                            LPTX_type temperature);

JUPITER_LPTX_DECL
void LPTX_particle_init_set_set_specific_heat(LPTX_particle_init_set *set,
                                              LPTX_type specific_heat);

JUPITER_LPTX_DECL
void LPTX_particle_init_set_set_thermal_conductivity(
  LPTX_particle_init_set *set, LPTX_type thermal_cond);

JUPITER_LPTX_DECL
void LPTX_particle_init_set_set_flags(LPTX_particle_init_set *set,
                                      LPTX_particle_flags flags);

JUPITER_LPTX_DECL
LPTX_idtype
LPTX_particle_init_set_number_of_particles(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_idtype LPTX_particle_init_set_origin_id(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_vector LPTX_particle_init_set_range_start(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_vector LPTX_particle_init_set_range_end(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_type LPTX_particle_init_set_time_start(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_type LPTX_particle_init_set_time_end(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_bool LPTX_particle_init_set_time_random(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_type LPTX_particle_init_set_diameter(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_type LPTX_particle_init_set_density(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_vector
LPTX_particle_init_set_initial_velocity(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_type LPTX_particle_init_set_temperature(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_type LPTX_particle_init_set_specific_heat(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_type
LPTX_particle_init_set_thermal_conductivity(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL
LPTX_particle_flags LPTX_particle_init_set_flags(LPTX_particle_init_set *set);

JUPITER_LPTX_DECL_END

#endif
