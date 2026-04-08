/**
 * @file lptx/strust_defs.h
 */

#ifndef JUPITER_LPTX_STRUCT_DEFS_H
#define JUPITER_LPTX_STRUCT_DEFS_H

#include "defs.h"
#include "jupiter/geometry/bitarray.h"
#include "jupiter/random/random.h"

#include <stdint.h>

JUPITER_LPTX_DECL_START

/**
 * The value type of jupiter_random_seed is assumed to be uint64_t.
 *
 * The function called in this function is not defined. So calling this function
 * causes compilation failure.
 */
static inline void LPTX__random_seed_type_check(jupiter_random_seed *s)
{
  void LPTX__random_seed_type_check_(uint64_t *);
  LPTX__random_seed_type_check_(&s->seed[0]);
}

#ifdef JUPITER_LPTX_MPI
#define LPTX_embedded_random_seed_MPI_data(t, type, mem) \
  LPTX_MPI_data(t, MPI_UINT64_T, type, mem seed, JUPITER_RANDOM_SEED_SIZE)

#define LPTX_embedded_random_seedc_MPI_data(t, type, mem) \
  LPTX_embedded_random_seed_MPI_data(t, type, mem seed.), \
    LPTX_MPI_data(t, MPI_UINT64_T, type, mem counter)
#endif

struct LPTX_vector
{
  LPTX_type x;
  LPTX_type y;
  LPTX_type z;
};
#ifdef JUPITER_LPTX_MPI
#define LPTX_embedded_vector_MPI_data(t, type, mem) \
  LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem x),     \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem y),   \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem z)

#define LPTX_vector_MPI_data(t) \
  {LPTX_embedded_vector_MPI_data(t, LPTX_vector, LPTX_empty)}
#endif

struct LPTX_particle_vector
{
  const LPTX_type *const v;
  const LPTX_idtype length;
};
#ifdef JUPITER_LPTX_MPI
#define LPTX_embedded_particle_vector_MPI_data(t, type, mem) \
  LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, type, mem length)

#define LPTX_particle_vector_MPI_data(t) \
  {LPTX_embedded_particle_vector_MPI_data(t, LPTX_particle_vector, LPTX_empty)}
#endif

struct LPTX_particle_flags
{
  geom_bitarray_n(v, LPTX_PTFLAG_MAX);
};
#define LPTX_particle_flags_N \
  (sizeof((LPTX_particle_flags){0}.v) / sizeof((LPTX_particle_flags){0}.v[0]))

#ifdef JUPITER_LPTX_MPI
#define LPTX_embedded_particle_flags_MPI_data(t, type, mem)          \
  LPTX_MPI_data(t, GEOM_BITARRAY_ELEMENT_MPI_BASE_TYPE, type, mem v, \
                LPTX_particle_flags_N)

#define LPTX_particle_flags_MPI_data(t) \
  {LPTX_embedded_particle_flags_MPI_data(t, LPTX_particle_flags, LPTX_empty)}
#endif

struct LPTX_particle
{
  LPTX_idtype particle_id;    ///< Particle ID
  LPTX_idtype origin_id;      ///< Origin ID
  LPTX_vector fluid_velocity; ///< Fluid velocity around particle [m/s] [Uxf]
  LPTX_type fluid_density;    ///< Fluid density around particle [kg/m3] [rhof]
  LPTX_type fluid_viscosity; ///< Static viscosity around particle [Pa.s] [vmuf]
  LPTX_type fluid_specific_heat;        ///< Fluid specific heat [J/kg K]
  LPTX_type fluid_thermal_conductivity; ///< Fluid thermal conductivity [W/m K]
  LPTX_type fluid_temperature;        ///< Fluid temperature around particle [K]
  LPTX_vector fluid_temperature_grad; ///< Fluid temperature gradient [K/m]
  LPTX_type mean_free_path;           ///< Mean free path of fluid [m]
  LPTX_type fluid_molecular_weight;   ///< Molecular weight of fluid [kg/kmol]
  LPTX_vector position;               ///< Particle position [m] [Xpt]
  LPTX_vector velocity;               ///< Particle velocity [m/s] [Uxpt]
  LPTX_type current_time;             ///< Particle current time [s]
  LPTX_type start_time;               ///< Particle start time [s] [Timpt]
  LPTX_type density;                  ///< Particle density [kg/m3] [Rhopt]
  LPTX_type specific_heat;            ///< Particle specific heat [J/kg K]
  LPTX_type thermal_conductivity;     ///< Particle thermal conductivity [W/m.K]
  LPTX_type heat_transfer_rate;  ///< Heat transfer rate at surface [W/m2 K]
  LPTX_type total_heat_transfer; ///< Total heat transfer at surface [W]
  LPTX_type diameter;            ///< Particle diameter [m] [Diapt]
  LPTX_vector fupt;              ///< Particle [FUxpt]
  LPTX_vector fdut;              ///< Particle [FdUxt]
  LPTX_vector fxpt;              ///< Particle [fFxpt]
  LPTX_type dTdt;    ///< Particle temperature difference last time-scheme [K/s]
  LPTX_vector fbpt;  ///< Particle Brownian force [N]
  LPTX_vector fTpt;  ///< Particle Thermophoretic force [N]
  LPTX_type dlpin;   ///< Distance between particle and interface [m] [dLpin]
  LPTX_idtype icfpt; ///< I-index in structured fluid cell [icfpt]
  LPTX_idtype jcfpt; ///< J-index in structured fluid cell [jcfpt]
  LPTX_idtype kcfpt; ///< K-index in structured fluid cell [kcfpt]
  LPTX_type parceln; ///< Number of particles in parcel
  LPTX_type temperature;         ///< Particle temperature [K]
  LPTX_vector init_position;     ///< Particle initial position [m] (unused)
  LPTX_vector init_velocity;     ///< Particle initial velocity [m/s] (unused)
  LPTX_type init_temperature;    ///< Particle initial temperature [K] (unused)
  LPTX_particle_flags flags;     ///< Particle state flags
  LPTX_idtype collision_partner; ///< Collision partner ID
  jupiter_random_seed ptseed;    ///< Particle-bounded random seed
};
#ifdef JUPITER_LPTX_MPI
#define LPTX_embedded_particle_MPI_data(t, type, mem)                      \
  LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, type, mem particle_id),               \
    LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, type, mem origin_id),               \
    LPTX_embedded_vector_MPI_data(t, type, mem fluid_velocity.),           \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem fluid_density),              \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem fluid_viscosity),            \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem fluid_specific_heat),        \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem fluid_thermal_conductivity), \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem fluid_temperature),          \
    LPTX_embedded_vector_MPI_data(t, type, mem fluid_temperature_grad.),   \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem mean_free_path),             \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem fluid_molecular_weight),     \
    LPTX_embedded_vector_MPI_data(t, type, mem position.),                 \
    LPTX_embedded_vector_MPI_data(t, type, mem velocity.),                 \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem current_time),               \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem start_time),                 \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem density),                    \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem specific_heat),              \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem thermal_conductivity),       \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem heat_transfer_rate),         \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem total_heat_transfer),        \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem diameter),                   \
    LPTX_embedded_vector_MPI_data(t, type, mem fupt.),                     \
    LPTX_embedded_vector_MPI_data(t, type, mem fdut.),                     \
    LPTX_embedded_vector_MPI_data(t, type, mem fxpt.),                     \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem dTdt),                       \
    LPTX_embedded_vector_MPI_data(t, type, mem fbpt.),                     \
    LPTX_embedded_vector_MPI_data(t, type, mem fTpt.),                     \
    LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, type, mem icfpt),                   \
    LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, type, mem jcfpt),                   \
    LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, type, mem kcfpt),                   \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem parceln),                    \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem temperature),                \
    LPTX_embedded_vector_MPI_data(t, type, mem init_position.),            \
    LPTX_embedded_vector_MPI_data(t, type, mem init_velocity.),            \
    LPTX_MPI_data(t, LPTX_MPI_TYPE, type, mem init_temperature),           \
    LPTX_embedded_particle_flags_MPI_data(t, type, mem flags.),            \
    LPTX_embedded_random_seed_MPI_data(t, type, mem ptseed.)

#define LPTX_particle_MPI_data(t) \
  {LPTX_embedded_particle_MPI_data(t, LPTX_particle, LPTX_empty)}
#endif

struct LPTX_particle_data
{
  LPTX_particle base; ///< Base scalar and fixed-length vector data
  const LPTX_idtype number_of_vectors; ///< Number of available vectors
  LPTX_particle_vector *const vectors;
  ///< Variable-length vectors (see LPTX_particle_vectors for contents)
};
#ifdef JUPITER_LPTX_MPI
#define LPTX_embedded_particle_data_MPI_data(t, type, mem) \
  LPTX_embedded_particle_MPI_data(t, type, mem base.),     \
    LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, type, mem number_of_vectors)

#define LPTX_particle_data_MPI_data(t) \
  {LPTX_embedded_particle_data_MPI_data(t, LPTX_particle_data, LPTX_empty)}
#endif

struct LPTX_rectilinear_grid
{
  const void *coords_i;
  const void *coords_j;
  const void *coords_k;
  LPTX_idtype nx, ny, nz; ///< Number of cells in grid
  LPTX_idtype mx, my, mz; ///< Number of cells including ghost cells
  LPTX_idtype stmx, stmy, stmz;
  ///< Number of ghost cells in East, South or Bottom.
  LPTX_cb_get_scalar_of *get_coord_i;
  LPTX_cb_get_scalar_of *get_coord_j;
  LPTX_cb_get_scalar_of *get_coord_k;
};

struct LPTX_rectilinear_scalar
{
  const void *scalar;
  LPTX_idtype nx, ny, nz; ///< Number of cells in grid
  LPTX_idtype mx, my, mz; ///< Number of cells including ghost cells
  LPTX_idtype stmx, stmy, stmz;
  ///< Number of ghost cells in East, South or Bottom.
  LPTX_cb_get_scalar_of *get_func;
};

struct LPTX_rectilinear_vector
{
  const void *vector_x;
  const void *vector_y;
  const void *vector_z;
  LPTX_idtype nx, ny, nz; ///< Number of cells in grid
  LPTX_idtype mx, my, mz; ///< Number of cells including ghost cells
  LPTX_idtype stmx, stmy, stmz;
  ///< Number of ghost cells in East, South or Bottom.
  LPTX_cb_get_scalar_of *get_func_x;
  LPTX_cb_get_scalar_of *get_func_y;
  LPTX_cb_get_scalar_of *get_func_z;
};

struct LPTX_particle_stat
{
  LPTX_idtype nstep;     ///< Number of integrated time steps
  LPTX_idtype allocated; ///< Number of particles exist in memory
  LPTX_idtype tracked;   ///< Number of integrated particles
  LPTX_idtype exited;    ///< Number of exited particles
  LPTX_idtype collided;  ///< Number of particle collisions
  LPTX_idtype sent;      ///< Number of sent particles to other MPI ranks
  LPTX_idtype recved;    ///< Number of received particles from other MPI ranks
};
#ifdef JUPITER_LPTX_MPI
#define LPTX_particle_stat_MPI_data(t)                                 \
  {                                                                    \
    LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, LPTX_particle_stat, nstep),     \
    LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, LPTX_particle_stat, allocated), \
    LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, LPTX_particle_stat, tracked),   \
    LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, LPTX_particle_stat, exited),    \
    LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, LPTX_particle_stat, collided),  \
    LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, LPTX_particle_stat, sent),      \
    LPTX_MPI_data(t, LPTX_MPI_TYPE_ID, LPTX_particle_stat, recved),    \
  }
#endif

struct LPTX_pcoef
{
  LPTX_type a, b, g;
};

struct LPTX_single_time_integrate_data
{
  LPTX_pcoef pcoef;               ///< Scheme integration coefficients
  LPTX_int iptirk;                ///< Scheme integration coefficient index
  LPTX_int iptist;                ///< Scheme integration step number
  LPTX_idtype stepno;             ///< Current time step number
  LPTX_idtype substepno;          ///< Current time substep number
  LPTX_idtype number_of_substeps; ///< Number of substeps
  LPTX_type current_time;         ///< Current time
  LPTX_type time_step_width;      ///< Current time step width (of substep)
};

JUPITER_LPTX_DECL_END

#endif
