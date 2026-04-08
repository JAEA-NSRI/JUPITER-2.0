#ifndef JUPITER_LPTX_STAT_H
#define JUPITER_LPTX_STAT_H

#include "defs.h"
#include "struct_defs.h"

#ifdef JUPITER_LPTX_MPI
#include <mpi.h>
#endif

JUPITER_LPTX_DECL_START

static inline void LPTX_particle_stat_init(LPTX_particle_stat *stat)
{
  stat->nstep = 0;
  stat->allocated = 0;
  stat->tracked = 0;
  stat->exited = 0;
  stat->collided = 0;
  stat->sent = 0;
  stat->recved = 0;
}

static inline LPTX_idtype
LPTX_particle_stat_get_nstep(const LPTX_particle_stat *stat)
{
  return stat->nstep;
}

static inline LPTX_idtype
LPTX_particle_stat_get_allocated(const LPTX_particle_stat *stat)
{
  return stat->allocated;
}

static inline LPTX_idtype
LPTX_particle_stat_get_tracked(const LPTX_particle_stat *stat)
{
  return stat->tracked;
}

static inline LPTX_idtype
LPTX_particle_stat_get_exited(const LPTX_particle_stat *stat)
{
  return stat->exited;
}

static inline LPTX_idtype
LPTX_particle_stat_get_collided(const LPTX_particle_stat *stat)
{
  return stat->collided;
}

static inline LPTX_idtype
LPTX_particle_stat_get_sent(const LPTX_particle_stat *stat)
{
  return stat->sent;
}

static inline LPTX_idtype
LPTX_particle_stat_get_recved(const LPTX_particle_stat *stat)
{
  return stat->recved;
}

#ifdef JUPITER_LPTX_MPI
/**
 * @brief Gather
 */
JUPITER_LPTX_DECL
int LPTX_particle_stat_gather(LPTX_particle_stat **outp,
                              const LPTX_particle_stat *stat, MPI_Comm comm);
#endif

JUPITER_LPTX_DECL_END

#endif
