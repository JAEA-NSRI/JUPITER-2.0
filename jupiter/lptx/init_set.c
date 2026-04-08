#include "init_set.h"
#include "defs.h"
#include "priv_struct_defs.h"
#include "struct_defs.h"

#include <jupiter/geometry/list.h>

LPTX_param *LPTX_particle_init_set_parent(LPTX_particle_init_set *set)
{
  return set->param;
}

static int LPTX_particle_init_set_is_head(LPTX_particle_init_set *set)
{
  LPTX_param *p;

  if (!set)
    return 0;

  p = LPTX_particle_init_set_parent(set);
  return p && &p->init_sets_head == set;
}

LPTX_particle_init_set *LPTX_particle_init_set_next(LPTX_particle_init_set *set)
{
  LPTX_particle_init_set *n;
  n = LPTX_particle_init_set_entry(geom_list_next(&set->list));
  if (LPTX_particle_init_set_is_head(n))
    return NULL;
  return n;
}

LPTX_particle_init_set *LPTX_particle_init_set_prev(LPTX_particle_init_set *set)
{
  LPTX_particle_init_set *p;
  p = LPTX_particle_init_set_entry(geom_list_prev(&set->list));
  if (LPTX_particle_init_set_is_head(p))
    return NULL;
  return p;
}

LPTX_particle_init_set *
LPTX_particle_init_set_insert(LPTX_particle_init_set *prev,
                              LPTX_particle_init_set *item)
{
  if (!prev->param)
    return NULL; /* Reject chaining dangling (not in chain from param) sets */

  geom_list_insert_next(&prev->list, &item->list);
  item->param = prev->param;
  return item;
}

LPTX_particle_init_set *
LPTX_particle_init_set_append(LPTX_param *param, LPTX_particle_init_set *set)
{
  geom_list_insert_prev(&param->init_sets_head.list, &set->list);
  set->param = param;
  return set;
}

LPTX_particle_init_set *
LPTX_particle_init_set_prepend(LPTX_param *param, LPTX_particle_init_set *set)
{
  geom_list_insert_next(&param->init_sets_head.list, &set->list);
  set->param = param;
  return set;
}

void LPTX_particle_init_set_set_number_of_particles(
  LPTX_particle_init_set *set, LPTX_idtype number_particles)
{
  set->number_particles = number_particles;
}

void LPTX_particle_init_set_set_origin_id(LPTX_particle_init_set *set,
                                          LPTX_idtype origin_id)
{
  set->origin_id = origin_id;
}

void LPTX_particle_init_set_set_range_start(LPTX_particle_init_set *set,
                                            LPTX_vector range_start)
{
  set->range_start = range_start;
}

void LPTX_particle_init_set_set_range_end(LPTX_particle_init_set *set,
                                          LPTX_vector range_end)
{
  set->range_end = range_end;
}

void LPTX_particle_init_set_set_time_start(LPTX_particle_init_set *set,
                                           LPTX_type time_start)
{
  set->time_start = time_start;
}

void LPTX_particle_init_set_set_time_end(LPTX_particle_init_set *set,
                                         LPTX_type time_end)
{
  set->time_end = time_end;
}

void LPTX_particle_init_set_set_time_random(LPTX_particle_init_set *set,
                                            LPTX_bool time_random)
{
  set->time_random = time_random;
}

void LPTX_particle_init_set_set_diameter(LPTX_particle_init_set *set,
                                         LPTX_type diameter)
{
  set->diameter = diameter;
}

void LPTX_particle_init_set_set_density(LPTX_particle_init_set *set,
                                        LPTX_type density)
{
  set->density = density;
}

void LPTX_particle_init_set_set_initial_velocity(LPTX_particle_init_set *set,
                                                 LPTX_vector velocity)
{
  set->velocity = velocity;
}

void LPTX_particle_init_set_set_temperature(LPTX_particle_init_set *set,
                                            LPTX_type temperature)
{
  set->temperature = temperature;
}

void LPTX_particle_init_set_set_specific_heat(LPTX_particle_init_set *set,
                                              LPTX_type specific_heat)
{
  set->specific_heat = specific_heat;
}

void LPTX_particle_init_set_set_thermal_conductivity(
  LPTX_particle_init_set *set, LPTX_type thermal_cond)
{
  set->thermal_conductivity = thermal_cond;
}

void LPTX_particle_init_set_set_flags(LPTX_particle_init_set *set,
                                      LPTX_particle_flags flags)
{
  set->flags = flags;
}

LPTX_idtype
LPTX_particle_init_set_number_of_particles(LPTX_particle_init_set *set)
{
  return set->number_particles;
}

LPTX_idtype LPTX_particle_init_set_origin_id(LPTX_particle_init_set *set)
{
  return set->origin_id;
}

LPTX_vector LPTX_particle_init_set_range_start(LPTX_particle_init_set *set)
{
  return set->range_start;
}

LPTX_vector LPTX_particle_init_set_range_end(LPTX_particle_init_set *set)
{
  return set->range_end;
}

LPTX_type LPTX_particle_init_set_time_start(LPTX_particle_init_set *set)
{
  return set->time_start;
}

LPTX_type LPTX_particle_init_set_time_end(LPTX_particle_init_set *set)
{
  return set->time_end;
}

LPTX_bool LPTX_particle_init_set_time_random(LPTX_particle_init_set *set)
{
  return set->time_random;
}

LPTX_type LPTX_particle_init_set_diameter(LPTX_particle_init_set *set)
{
  return set->diameter;
}

LPTX_type LPTX_particle_init_set_density(LPTX_particle_init_set *set)
{
  return set->density;
}

LPTX_vector LPTX_particle_init_set_initial_velocity(LPTX_particle_init_set *set)
{
  return set->velocity;
}

LPTX_type LPTX_particle_init_set_temperature(LPTX_particle_init_set *set)
{
  return set->temperature;
}

LPTX_type LPTX_particle_init_set_specific_heat(LPTX_particle_init_set *set)
{
  return set->specific_heat;
}

LPTX_type
LPTX_particle_init_set_thermal_conductivity(LPTX_particle_init_set *set)
{
  return set->thermal_conductivity;
}

LPTX_particle_flags LPTX_particle_init_set_flags(LPTX_particle_init_set *set)
{
  return set->flags;
}
