#ifndef JUPITER_LPTX_PRIV_STRUCT_DEFS_H
#define JUPITER_LPTX_PRIV_STRUCT_DEFS_H

#include "defs.h"
#include "struct_defs.h"

#include <jupiter/geometry/list.h>
#include <jupiter/geometry/util.h>
#include <jupiter/random/random.h>

#ifdef JUPITER_LPTX_MPI
#include <mpi.h>
#endif

JUPITER_LPTX_DECL_START

/*
 * This type is used for aligning data on mixed storage data.
 */
union LPTX_particle_storage
{
  LPTX_particle_data *pt;
  LPTX_particle_data p;
  LPTX_particle_vector v;
  LPTX_type s;
  LPTX_idtype i;
};

struct LPTX_particle_set_list
{
  struct geom_list list; ///< Linked-list chain
  LPTX_param *param;     ///< Param that holding this set
};
#define LPTX_particle_set_list_entry(ptr) \
  geom_list_entry(ptr, struct LPTX_particle_set_list, list)

struct LPTX_particle_set
{
  struct LPTX_particle_set_list list;    ///< List entry
  LPTX_idtype number_particles;          ///< Number of particles in this set
  LPTX_idtype number_data;               ///< Number of data per particle
  LPTX_idtype number_vectors;            ///< Number of vectors per particle
  LPTX_idtype number_storage;            ///< Number of storage item
  LPTX_idtype *vector_sizes;             ///< Array of sizes for each vector
  LPTX_idtype *sorted_indices;           ///< Array of indices of @p sorted
  LPTX_particle_data **sorted;           ///< Array of sorted particle data
  LPTX_particle_data *particles;         ///< Array of unsorted particle data
  LPTX_particle_vector *vectors;         ///< Array of vector data
  LPTX_type *data;                       ///< Dynamic vector data
  union LPTX_particle_storage storage[]; ///< Local storage of data
};
#define LPTX_particle_set_entry(ptr) \
  geom_list_entry(ptr, struct LPTX_particle_set, list.list)

#ifdef JUPITER_LPTX_MPI
/* MPI data for transferring metadata of LPTX_particle_set */
#define LPTX_particle_set_MPI_data(t)                                        \
  {                                                                          \
    LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, LPTX_particle_set, number_particles), \
    LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, LPTX_particle_set, number_data),      \
    LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, LPTX_particle_set, number_vectors),   \
    LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, LPTX_particle_set, number_storage),   \
  }
#endif

struct LPTX_particle_init_set
{
  struct geom_list list;        ///< Linked-list chain
  LPTX_idtype number_particles; ///< Number of particles to generate [nistpt]
  LPTX_idtype origin_id;        ///< Origin ID
  LPTX_vector range_start;      ///< Start of box domain of generating [pset*s]
  LPTX_vector range_end;        ///< End of box domain of generating [pset*e]
  LPTX_type time_start;         ///< Start time of injection [psetTms]
  LPTX_type time_end;           ///< End time of injection   [psetTme]
  LPTX_bool time_random;        ///< Let injection time random [itrdm]
  LPTX_type diameter;           ///< Diameter of particles [psetDi]
  LPTX_type density;            ///< Density of particles [psetRi]
  LPTX_vector velocity;         ///< Initial velocity of particles [psetU*]
  LPTX_type temperature;        ///< Initial temperature of particles
  LPTX_type specific_heat;      ///< Specific heat of particles
  LPTX_type thermal_conductivity; ///< Thermal conductivity of particles
  LPTX_particle_flags flags;      ///< Initial particle flags
  LPTX_param *param;              ///< Parameter that holding this set
};
#define LPTX_particle_init_set_entry(ptr) \
  geom_list_entry(ptr, struct LPTX_particle_init_set, list)

struct LPTX_collision_list_data
{
  struct geom_list list; ///< Link to another entries that share @p a or @p b
  LPTX_particle_data *a;
  LPTX_particle_data *b;
  LPTX_idtype new_p;
  LPTX_bool processed;
};

#define LPTX_collision_list_data_entry(ptr) \
  geom_list_entry(ptr, struct LPTX_collision_list_data, list)

struct LPTX_collision_list
{
  struct geom_list list; ///< Linked-list chain
};

#define LPTX_collision_list_entry(ptr) \
  geom_list_entry(ptr, struct LPTX_collision_list, list)

struct LPTX_collision_list_set
{
  struct LPTX_collision_list list;
  LPTX_idtype number_of_entries; ///< Number of entry in this list
  LPTX_collision_list_data entries[];
};

#define LPTX_collision_list_set_entry(ptr) \
  geom_list_entry(ptr, struct LPTX_collision_list_set, list.list)

struct LPTX_param
{
  LPTX_time_scheme time_scheme;       ///< Time scheme
  LPTX_heat_scheme heat_scheme;       ///< Heat exchange scheme
  LPTX_vector gravity;                ///< Gravitational acceleration [m/s2]
  LPTX_bool brownian_force;           ///< Brownian Force
  LPTX_bool thermophoretic_force;     ///< Thermophoretic Force
  LPTX_bool use_constant_Cc;          ///< Use constant Cunningham correction
  LPTX_type cunningham_correction;    ///< Stokes-Cunningham slip correction Cc
  LPTX_type cunningham_corr_const_A1; ///< Cunningham correction Cc constant A1
  LPTX_type cunningham_corr_const_A2; ///< Cunningham correction Cc constant A2
  LPTX_type cunningham_corr_const_A3; ///< Cunningham correction Cc constant A3
  LPTX_type thermophoretic_const_Cs;  ///< Thermophoretic force constant Cs
  LPTX_type thermophoretic_const_Cm;  ///< Thermophoretic force constant Cm
  LPTX_type thermophoretic_const_Ct;  ///< Thermophoretic force constant Ct
  LPTX_idtype number_of_particle_vectors; ///< Number of particle vectors
  LPTX_idtype *particle_vector_sizes;     ///< Each size for particle vector
  LPTX_particle_init_set init_sets_head;
  struct LPTX_particle_set_list sets_head;
  LPTX_collision_list collision_list_head;
  LPTX_idtype number_of_collisions; ///< Number of collisions in the list
  LPTX_idtype number_of_substep;    ///< Number of subcycle for next integration
  LPTX_idtype max_number_of_substep; ///< Maximum number of subcycle
  LPTX_type ptaup_max;               ///< Maximum time constant
  jupiter_random_seed random_seed;
  LPTX_particle_stat cumulative_stat; ///< Cumlative statistics from start
  LPTX_particle_stat last_stat;       ///< Statistics on last integration
#ifdef JUPITER_LPTX_MPI
  MPI_Comm mpi_comm; ///< Communicator to be used (should be same as the
                     ///< communicator of fluid domain)
#endif
};

JUPITER_LPTX_DECL_END

#endif
