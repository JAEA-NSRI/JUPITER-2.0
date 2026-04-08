#include "param.h"
#include "comm.h"
#include "defs.h"
#include "error.h"
#include "init_set.h"
#include "overflow.h"
#include "particle.h"
#include "priv_struct_defs.h"
#include "priv_util.h"
#include "ptflags.h"
#include "pvector.h"
#include "stat.h"
#include "struct_defs.h"
#include "util.h"
#include "vector.h"

#include <inttypes.h>
#include <jupiter/geometry/list.h>
#include <jupiter/random/random.h>
#include <stdint.h>

#ifdef JUPITER_LPTX_MPI
#include <mpi.h>
#endif

#include <stdlib.h>

LPTX_type LPTX_cunningham_correction_A1(void) { return LPTX_C(1.257); }

LPTX_type LPTX_cunningham_correction_A2(void) { return LPTX_C(0.400); }

LPTX_type LPTX_cunningham_correction_A3(void) { return LPTX_C(1.1); }

LPTX_type LPTX_thermophoretic_constant_Cs(void) { return LPTX_C(1.17); }

LPTX_type LPTX_thermophoretic_constant_Cm(void) { return LPTX_C(1.14); }

LPTX_type LPTX_thermophoretic_constant_Ct(void) { return LPTX_C(2.18); }

static void LPTX_particle_init(struct LPTX_particle *p)
{
  p->particle_id = -1;
  p->origin_id = LPTX_ORIGIN_UNDEFINED;
  p->velocity = LPTX_vector_zero();
  p->position = LPTX_vector_zero();
  p->fupt = LPTX_vector_zero();
  p->fdut = LPTX_vector_zero();
  p->fxpt = LPTX_vector_zero();
  p->dTdt = LPTX_C(0.);
  p->fluid_velocity = LPTX_vector_zero();
  p->fluid_temperature = LPTX_C(0.);
  p->fluid_temperature_grad = LPTX_vector_zero();
  p->fluid_density = LPTX_C(0.);
  p->fluid_viscosity = LPTX_C(0.);
  p->fluid_specific_heat = LPTX_C(0.);
  p->fluid_thermal_conductivity = LPTX_C(0.);
  p->mean_free_path = LPTX_C(0.);
  p->fluid_molecular_weight = LPTX_C(0.);
  p->icfpt = 0;
  p->jcfpt = 0;
  p->kcfpt = 0;
  p->start_time = LPTX_C(0.);
  p->current_time = LPTX_C(0.);
  p->dlpin = LPTX_C(0.);
  p->density = LPTX_C(0.);
  p->specific_heat = LPTX_C(0.);
  p->thermal_conductivity = LPTX_C(0.);
  p->heat_transfer_rate = LPTX_C(0.);
  p->total_heat_transfer = LPTX_C(0.);
  p->diameter = LPTX_C(0.);
  p->temperature = LPTX_C(0.);
  p->parceln = LPTX_C(1.);
  p->collision_partner = -1;
  p->init_position = LPTX_vector_zero();
  p->init_velocity = LPTX_vector_zero();
  p->init_temperature = LPTX_C(0.);
  p->fbpt = LPTX_vector_zero();
  p->fTpt = LPTX_vector_zero();
  p->flags = LPTX_particle_flags_none();
}

static void LPTX_particle_data_init(
  struct LPTX_particle_data *p, LPTX_idtype number_of_vectors,
  LPTX_particle_vector *vect /* [number_of_vectors] */, LPTX_type *data,
  const LPTX_idtype *vecsizes /* [number_of_vectors] */)
{
  LPTX_particle_init(&p->base);
  *(LPTX_idtype *)&p->number_of_vectors = number_of_vectors;
  *(LPTX_particle_vector **)&p->vectors = vect;
  for (size_t i = 0; i < number_of_vectors; ++i) {
    LPTX_particle_vector_init(&vect[i]);
    if (data) {
      LPTX_idtype sz = vecsizes[i];
      LPTX_particle_vector_bind(&vect[i], data, sz);
      data += sz;
    }
  }
}

static void LPTX_particle_set_list_init(struct LPTX_particle_set_list *p)
{
  geom_list_init(&p->list);
  p->param = NULL;
}

static void LPTX_particle_init_set_init(LPTX_particle_init_set *p)
{
  geom_list_init(&p->list);
  p->number_particles = 0;
  p->origin_id = LPTX_ORIGIN_UNDEFINED;
  p->density = LPTX_C(0.0);
  p->diameter = LPTX_C(0.0);
  p->time_start = LPTX_C(0.0);
  p->time_end = LPTX_C(0.0);
  p->range_start = LPTX_vector_zero();
  p->range_end = LPTX_vector_zero();
  p->velocity = LPTX_vector_zero();
  p->temperature = LPTX_C(0.0);
  p->specific_heat = LPTX_C(0.0);
  p->thermal_conductivity = LPTX_C(0.0);
  p->param = NULL;
}

static void LPTX_param_init(LPTX_param *p)
{
  p->time_scheme = LPTX_TIME_SCHEME_ADAMS_BASHFORTH_2;
  p->heat_scheme = LPTX_HEAT_OFF;
  p->gravity = LPTX_vector_zero();
  p->brownian_force = LPTX_false;
  p->thermophoretic_force = LPTX_false;
  p->use_constant_Cc = LPTX_false;
  p->cunningham_correction = LPTX_C(1.0);
  p->cunningham_corr_const_A1 = LPTX_cunningham_correction_A1();
  p->cunningham_corr_const_A2 = LPTX_cunningham_correction_A2();
  p->cunningham_corr_const_A3 = LPTX_cunningham_correction_A3();
  p->thermophoretic_const_Cs = LPTX_thermophoretic_constant_Cs();
  p->thermophoretic_const_Cm = LPTX_thermophoretic_constant_Cm();
  p->thermophoretic_const_Ct = LPTX_thermophoretic_constant_Ct();
  p->number_of_particle_vectors = 0;
  p->particle_vector_sizes = NULL;
  p->random_seed = (jupiter_random_seed){
    .seed = {UINT64_C(0x28e73b0bd492a3c2), UINT64_C(0x9a5f52086dbb9678),
             UINT64_C(0xbb613d74c4afb663), UINT64_C(0xe4184155aada5071)}};
  p->number_of_substep = 1;
  p->max_number_of_substep = 100;
  p->ptaup_max = LPTX_C(0.);
  LPTX_particle_init_set_init(&p->init_sets_head);
  LPTX_particle_set_list_init(&p->sets_head);
  LPTX_collision_list_init(&p->collision_list_head);
  p->number_of_collisions = 0;
  p->init_sets_head.param = p;
  p->sets_head.param = p;
  LPTX_particle_stat_init(&p->cumulative_stat);
  LPTX_particle_stat_init(&p->last_stat);
#ifdef JUPITER_LPTX_MPI
  p->mpi_comm = MPI_COMM_NULL;
#endif
}

LPTX_param *LPTX_param_new(void)
{
  LPTX_param *p;
  p = (LPTX_param *)calloc(1, sizeof(LPTX_param));
  if (!p)
    return NULL;

  LPTX_param_init(p);
  return p;
}

void LPTX_param_delete(LPTX_param *param)
{
  free(param->particle_vector_sizes);
  LPTX_param_delete_all_init_sets(param);
  LPTX_param_delete_all_particles(param);
#ifdef JUPITER_LPTX_MPI
  if (param->mpi_comm != MPI_COMM_NULL)
    MPI_Comm_free(&param->mpi_comm);
#endif
  free(param);
}

void LPTX_param_delete_all_init_sets(LPTX_param *param)
{
  struct geom_list *lp, *ln, *lh;

  lh = &param->init_sets_head.list;
  geom_list_foreach_safe (lp, ln, lh) {
    LPTX_particle_init_set *p = LPTX_particle_init_set_entry(lp);
    LPTX_particle_init_set_delete(p);
  }
}

void LPTX_param_delete_all_particles(LPTX_param *param)
{
  struct geom_list *lp, *ln, *lh;

  lh = &param->sets_head.list;
  geom_list_foreach_safe (lp, ln, lh) {
    LPTX_particle_set *p = LPTX_particle_set_entry(lp);
    LPTX_particle_set_delete(p);
  }
}

struct LPTX_param_join_count
{
  LPTX_idtype nset;
  LPTX_idtype np;
  LPTX_idtype nvec;
  LPTX_idtype *vecsizes;
  LPTX_particle_set *newset;
  LPTX_idtype istart;
};

/* Count number of sets, number of particles and maximum number of vector */
static LPTX_bool LPTX_param_join_count_p1(LPTX_particle_set *set, void *arg)
{
  LPTX_idtype nv;
  struct LPTX_param_join_count *p;
  p = (struct LPTX_param_join_count *)arg;
  p->nset += 1;
  p->np += LPTX_particle_set_number_of_particles(set);
  nv = LPTX_particle_set_number_of_vectors(set);
  if (nv > p->nvec)
    p->nvec = nv;
  return LPTX_false;
}

/* Compute maximum of vector sizes */
static LPTX_bool LPTX_param_join_count_p2(LPTX_particle_set *set, void *arg)
{
  LPTX_idtype nv;
  const LPTX_idtype *vs;
  struct LPTX_param_join_count *p;
  p = (struct LPTX_param_join_count *)arg;
  nv = LPTX_particle_set_number_of_vectors(set);
  vs = LPTX_particle_set_vector_sizes(set);
  for (LPTX_idtype j = 0; j < nv; ++j) {
    if (vs[j] > p->vecsizes[j])
      p->vecsizes[j] = vs[j];
  }
  return LPTX_false;
}

/* Copy to joined particle set */
static LPTX_bool LPTX_param_join_impl(LPTX_particle_set *set, void *arg)
{
  LPTX_idtype np;
  struct LPTX_param_join_count *p;
  p = (struct LPTX_param_join_count *)arg;
  np = LPTX_particle_set_number_of_particles(set);

#ifdef _OPENMP
#pragma omp parallel for if (np > LPTX_omp_small_threshold)
#endif
  for (LPTX_idtype i = 0; i < np; ++i)
    LPTX_particle_copy_fill(&p->newset->particles[i + p->istart],
                            &set->particles[i], LPTX_C(0.0));

  p->istart += np;
  return LPTX_false;
}

struct LPTX_param_join_count LPTX_param_join_count_init(void)
{
  return (struct LPTX_param_join_count){
    .nset = 0,
    .np = 0,
    .nvec = 0,
    .vecsizes = NULL,
    .newset = NULL,
    .istart = 0,
  };
}

LPTX_particle_set *LPTX_param_join_particle_sets(LPTX_param *param)
{
  struct LPTX_param_join_count p = LPTX_param_join_count_init();

  LPTX_param_foreach_particle_sets_i(param, LPTX_param_join_count_p1, &p);
  if (p.nset == 1) {
    LPTX_particle_set *p = LPTX_param_get_first_particle_set(param);
    LPTX_assert(LPTX_particle_set_param(p) == param);
    return p;
  }

  if (p.nset < 1 || p.np < 1)
    return LPTX_param_new_particle_set(param, 0);

  if (p.nvec > 0) {
    p.vecsizes = (LPTX_idtype *)malloc(sizeof(LPTX_idtype) * p.nvec);
    if (!p.vecsizes)
      return NULL;

    LPTX_param_foreach_particle_sets_i(param, LPTX_param_join_count_p2, &p);
  }

  p.newset = LPTX_particle_set_new(p.np, p.nvec, p.vecsizes);
  if (p.vecsizes)
    free(p.vecsizes);
  if (!p.newset)
    return NULL;

  LPTX_param_foreach_particle_sets_i(param, LPTX_param_join_impl, &p);
  return p.newset;
}

const LPTX_particle_set *
LPTX_param_join_particle_sets_inplace(LPTX_param *param)
{
  struct LPTX_param_join_count p = LPTX_param_join_count_init();
  static const struct LPTX_particle_set s0 = { .number_particles = 0 };

  LPTX_param_foreach_particle_sets_i(param, LPTX_param_join_count_p1, &p);
  if (p.np > 0 && p.nset == 1)
    return LPTX_param_get_first_particle_set(param);

  if (p.nset < 1 || p.np < 1) {
    if (p.nset > 0)
      LPTX_param_delete_all_particles(param);
    return &s0;
  }

  if (p.nvec > 0) {
    p.vecsizes = (LPTX_idtype *)malloc(sizeof(LPTX_idtype) * p.nvec);
    if (!p.vecsizes)
      return NULL;

    LPTX_param_foreach_particle_sets_i(param, LPTX_param_join_count_p2, &p);
  }

  p.newset = LPTX_particle_set_new(p.np, p.nvec, p.vecsizes);
  if (p.vecsizes)
    free(p.vecsizes);
  if (!p.newset)
    return NULL;

  LPTX_param_foreach_particle_sets_i(param, LPTX_param_join_impl, &p);
  LPTX_param_delete_all_particles(param);
  LPTX_particle_set_append(param, p.newset);
  return p.newset;
}

LPTX_bool LPTX_param_set_particle_vectors(LPTX_param *param,
                                          LPTX_idtype number_of_vectors,
                                          const LPTX_idtype *numbers_of_data)
{
  LPTX_idtype *nd;
  LPTX_assert(number_of_vectors >= 0);

  if (number_of_vectors == 0) {
    if (param->particle_vector_sizes)
      free(param->particle_vector_sizes);
    param->particle_vector_sizes = NULL;
    param->number_of_particle_vectors = 0;
    return LPTX_true;
  }

  nd = (LPTX_idtype *)realloc(param->particle_vector_sizes,
                              sizeof(LPTX_idtype) * number_of_vectors);
  if (!nd)
    return LPTX_false;

  for (LPTX_idtype jj = 0; jj < number_of_vectors; ++jj) {
    LPTX_idtype n = 0;
    if (numbers_of_data) {
      LPTX_assert(numbers_of_data[jj] >= 0);
      n = numbers_of_data[jj];
    }
    nd[jj] = n;
  }

  param->particle_vector_sizes = nd;
  param->number_of_particle_vectors = number_of_vectors;
  return LPTX_true;
}

LPTX_idtype LPTX_param_get_number_of_particle_vectors(LPTX_param *param)
{
  return param->number_of_particle_vectors;
}

const LPTX_idtype *
LPTX_param_get_number_of_particle_vector_sizes(LPTX_param *param)
{
  return param->particle_vector_sizes;
}

LPTX_idtype
LPTX_param_get_number_of_particle_vector_size(LPTX_param *param,
                                              LPTX_idtype vector_index)
{
  LPTX_assert(!!param);
  LPTX_assert(vector_index >= 0);
  LPTX_assert(vector_index < param->number_of_particle_vectors);
  return param->particle_vector_sizes[vector_index];
}

void LPTX_param_set_number_of_particle_vector_size(LPTX_param *param,
                                                   LPTX_idtype vector_index,
                                                   LPTX_idtype vector_size)
{
  LPTX_assert(!!param);
  LPTX_assert(vector_index >= 0);
  LPTX_assert(vector_index < param->number_of_particle_vectors);
  LPTX_assert(vector_size >= 0);
  param->particle_vector_sizes[vector_index] = vector_size;
}

LPTX_idtype LPTX_param_get_allocated_particle_vectors(LPTX_param *param)
{
  LPTX_particle_set *setp;
  LPTX_idtype r = 0;

  LPTX_foreach_particle_sets (setp, param) {
    LPTX_idtype nv = LPTX_particle_set_number_of_vectors(setp);
    if (nv > r)
      r = nv;
  }
  return r;
}

void LPTX_param_get_allocated_particle_vector_sizes(LPTX_idtype *out,
                                                    LPTX_idtype nout,
                                                    LPTX_param *param)
{
  LPTX_particle_set *setp;

  for (LPTX_idtype ni = 0; ni < nout; ++ni)
    out[ni] = 0;

  LPTX_foreach_particle_sets (setp, param) {
    LPTX_idtype nv = LPTX_particle_set_number_of_vectors(setp);
    const LPTX_idtype *nvs = LPTX_particle_set_vector_sizes(setp);
    if (!nvs)
      continue;

    for (LPTX_idtype ni = 0; ni < nout && ni < nv; ++ni) {
      LPTX_idtype vs = nvs[ni];
      if (vs > out[ni])
        out[ni] = vs;
    }
  }
}

LPTX_idtype
LPTX_param_get_allocated_particle_vector_size(LPTX_param *param,
                                              LPTX_idtype vector_index)
{
  LPTX_idtype r = 0;
  LPTX_particle_set *setp;

  LPTX_assert(vector_index >= 0);

  LPTX_foreach_particle_sets (setp, param) {
    LPTX_idtype nv = LPTX_particle_set_number_of_vectors(setp);
    const LPTX_idtype *nvs = LPTX_particle_set_vector_sizes(setp);

    if (vector_index >= nv || !nvs)
      continue;

    if (r < nvs[vector_index])
      r = nvs[vector_index];
  }

  return r;
}

LPTX_idtype LPTX_param_get_global_particle_vectors(LPTX_param *param, int *r)
{
  int rm = 0;
  LPTX_idtype l;
  l = LPTX_param_get_allocated_particle_vectors(param);

#ifdef JUPITER_LPTX_MPI
  rm = MPI_SUCCESS;
  if (param->mpi_comm != MPI_COMM_NULL)
    rm = MPI_Allreduce(MPI_IN_PLACE, &l, 1, LPTX_MPI_TYPE_ID, MPI_MAX,
                       param->mpi_comm);
#endif

  if (r)
    *r = rm;
  return l;
}

int LPTX_param_get_global_particle_vector_sizes(LPTX_idtype *out, int nout,
                                                LPTX_param *param)
{
  int rm = 0;
  LPTX_param_get_allocated_particle_vector_sizes(out, nout, param);

#ifdef JUPITER_LPTX_MPI
  rm = MPI_SUCCESS;
  if (param->mpi_comm != MPI_COMM_NULL)
    rm = MPI_Allreduce(MPI_IN_PLACE, out, nout, LPTX_MPI_TYPE_ID, MPI_MAX,
                       param->mpi_comm);
#endif
  return rm;
}

LPTX_idtype LPTX_param_get_global_particle_vector_size(LPTX_param *param,
                                                       LPTX_idtype vector_index,
                                                       int *r)
{
  int rm = 0;
  LPTX_idtype l;
  l = LPTX_param_get_allocated_particle_vector_size(param, vector_index);

#ifdef JUPITER_LPTX_MPI
  rm = MPI_SUCCESS;
  if (param->mpi_comm != MPI_COMM_NULL)
    rm = MPI_Allreduce(MPI_IN_PLACE, &l, 1, LPTX_MPI_TYPE_ID, MPI_MAX,
                       param->mpi_comm);
#endif

  if (r)
    *r = rm;
  return l;
}

void LPTX_param_set_seed(LPTX_param *param, jupiter_random_seed seed)
{
  param->random_seed = seed;
}

void LPTX_param_set_random_seed(LPTX_param *param)
{
  jupiter_random_seed_fill_random(&param->random_seed);
}

jupiter_random_seed LPTX_param_get_seed(LPTX_param *param)
{
  return param->random_seed;
}

void LPTX_param_set_time_scheme(LPTX_param *param, LPTX_time_scheme scheme)
{
  param->time_scheme = scheme;
}

LPTX_time_scheme LPTX_param_time_scheme(LPTX_param *param)
{
  return param->time_scheme;
}

void LPTX_param_set_heat_scheme(LPTX_param *param, LPTX_heat_scheme scheme)
{
  param->heat_scheme = scheme;
}

LPTX_heat_scheme LPTX_param_heat_scheme(LPTX_param *param)
{
  return param->heat_scheme;
}

void LPTX_param_set_gravity(LPTX_param *param, LPTX_vector gravity)
{
  param->gravity = gravity;
}

LPTX_vector LPTX_param_gravity(LPTX_param *param) { return param->gravity; }

void LPTX_param_set_brownian_force(LPTX_param *param, LPTX_bool f)
{
  param->brownian_force = f;
}

LPTX_bool LPTX_param_brownian_force(LPTX_param *param)
{
  return param->brownian_force;
}

void LPTX_param_set_thermophoretic_force(LPTX_param *param, LPTX_bool f)
{
  param->thermophoretic_force = f;
}

LPTX_bool LPTX_param_thermophoretic_force(LPTX_param *param)
{
  return param->thermophoretic_force;
}

LPTX_bool LPTX_param_external_force(LPTX_param *param)
{
  if (LPTX_param_brownian_force(param))
    return LPTX_true;
  if (LPTX_param_thermophoretic_force(param))
    return LPTX_true;
  return LPTX_false;
}

LPTX_bool LPTX_param_heat_exchange(LPTX_param *param)
{
  switch (param->heat_scheme) {
  case LPTX_HEAT_FOLLOW_FLUID:
  case LPTX_HEAT_RANZ_MARSHALL:
    return LPTX_true;

  case LPTX_HEAT_OFF:
  case LPTX_HEAT_INVALID:
    break;
  }
  return LPTX_false;
}

LPTX_bool LPTX_param_heat_exchange_by_htr(LPTX_param *param)
{
  if (!LPTX_param_heat_exchange(param))
    return LPTX_false;

  if (param->heat_scheme == LPTX_HEAT_FOLLOW_FLUID)
    return LPTX_false;
  return LPTX_true;
}

void LPTX_param_set_use_constant_cunningham_correction(LPTX_param *param,
                                                       LPTX_bool f)
{
  param->use_constant_Cc = f;
}

LPTX_bool LPTX_param_use_constant_cunningham_correction(LPTX_param *param)
{
  return param->use_constant_Cc;
}

void LPTX_param_set_cunningham_correction(LPTX_param *param, LPTX_type cc)
{
  param->cunningham_correction = cc;
}

LPTX_type LPTX_param_cunningham_correction(LPTX_param *param)
{
  return param->cunningham_correction;
}

void LPTX_param_set_cunningham_correction_A1(LPTX_param *param, LPTX_type value)
{
  param->cunningham_corr_const_A1 = value;
}

LPTX_type LPTX_param_cunningham_correction_A1(LPTX_param *param)
{
  return param->cunningham_corr_const_A1;
}

void LPTX_param_set_cunningham_correction_A2(LPTX_param *param, LPTX_type value)
{
  param->cunningham_corr_const_A2 = value;
}

LPTX_type LPTX_param_cunningham_correction_A2(LPTX_param *param)
{
  return param->cunningham_corr_const_A2;
}

void LPTX_param_set_cunningham_correction_A3(LPTX_param *param, LPTX_type value)
{
  param->cunningham_corr_const_A3 = value;
}

LPTX_type LPTX_param_cunningham_correction_A3(LPTX_param *param)
{
  return param->cunningham_corr_const_A3;
}

void LPTX_param_set_thermophoretic_force_constant_Cs(LPTX_param *param,
                                                     LPTX_type value)
{
  param->thermophoretic_const_Cs = value;
}

LPTX_type LPTX_param_thermophoretic_force_constant_Cs(LPTX_param *param)
{
  return param->thermophoretic_const_Cs;
}

void LPTX_param_set_thermophoretic_force_constant_Cm(LPTX_param *param,
                                                     LPTX_type value)
{
  param->thermophoretic_const_Cm = value;
}

LPTX_type LPTX_param_thermophoretic_force_constant_Cm(LPTX_param *param)
{
  return param->thermophoretic_const_Cm;
}

void LPTX_param_set_thermophoretic_force_constant_Ct(LPTX_param *param,
                                                     LPTX_type value)
{
  param->thermophoretic_const_Ct = value;
}

LPTX_type LPTX_param_thermophoretic_force_constant_Ct(LPTX_param *param)
{
  return param->thermophoretic_const_Ct;
}

void LPTX_param_set_number_of_substep(LPTX_param *param,
                                      LPTX_idtype number_of_substep)
{
  param->number_of_substep = number_of_substep;
}

LPTX_idtype LPTX_param_number_of_substep(LPTX_param *param)
{
  return param->number_of_substep;
}

void LPTX_param_set_max_number_of_substep(LPTX_param *param,
                                          LPTX_idtype max_number_of_substep)
{
  param->max_number_of_substep = max_number_of_substep;
}

LPTX_idtype LPTX_param_max_number_of_substep(LPTX_param *param)
{
  return param->max_number_of_substep;
}

LPTX_bool LPTX_param_want_fluid_temperature(LPTX_param *param)
{
  if (LPTX_param_heat_exchange(param))
    return LPTX_true;
  if (LPTX_param_brownian_force(param))
    return LPTX_true;
  if (LPTX_param_thermophoretic_force(param))
    return LPTX_true;
  if (LPTX_param_want_mean_free_path(param))
    return LPTX_true;
  return LPTX_false;
}

LPTX_bool LPTX_param_want_fluid_temperature_grad(LPTX_param *param)
{
  if (LPTX_param_thermophoretic_force(param))
    return LPTX_true;
  return LPTX_false;
}

LPTX_bool LPTX_param_want_fluid_specific_heat(LPTX_param *param)
{
  if (LPTX_param_heat_exchange(param))
    return LPTX_true;
  return LPTX_false;
}

LPTX_bool LPTX_param_want_fluid_thermal_conductivity(LPTX_param *param)
{
  if (LPTX_param_heat_exchange(param))
    return LPTX_true;
  return LPTX_false;
}

LPTX_bool LPTX_param_want_mean_free_path(LPTX_param *param)
{
  if (LPTX_param_brownian_force(param)) {
    if (LPTX_param_use_constant_cunningham_correction(param))
      return LPTX_false;
    return LPTX_true;
  }

  return LPTX_false;
}

LPTX_bool LPTX_param_want_fluid_molecular_weight(LPTX_param *param)
{
  if (LPTX_param_thermophoretic_force(param))
    return LPTX_true;
  if (LPTX_param_want_mean_free_path(param))
    return LPTX_true;
  return LPTX_false;
}

LPTX_bool LPTX_param_want_temperature(LPTX_param *param)
{
  if (LPTX_param_heat_exchange(param))
    return LPTX_true;
  return LPTX_false;
}

LPTX_bool LPTX_param_want_specific_heat(LPTX_param *param)
{
  if (LPTX_param_heat_exchange(param))
    return LPTX_true;
  return LPTX_false;
}

LPTX_bool LPTX_param_want_thermal_conductivity(LPTX_param *param)
{
  if (LPTX_param_heat_exchange(param))
    return LPTX_true;
  if (LPTX_param_thermophoretic_force(param))
    return LPTX_true;
  return LPTX_false;
}

const LPTX_particle_stat *LPTX_param_get_cumulative_stat(LPTX_param *param)
{
  return &param->cumulative_stat;
}

const LPTX_particle_stat *LPTX_param_get_last_step_stat(LPTX_param *param)
{
  return &param->last_stat;
}

void LPTX_param_reset_cumulative_stat(LPTX_param *param)
{
  LPTX_particle_stat_init(&param->cumulative_stat);
}

void LPTX_param_reset_last_stat(LPTX_param *param)
{
  LPTX_particle_stat_init(&param->last_stat);
}

void LPTX_param_count_tracked(LPTX_param *param, LPTX_idtype icount)
{
#ifdef _OPENMP
#pragma omp atomic update
#endif
  param->cumulative_stat.tracked += icount;

#ifdef _OPENMP
#pragma omp atomic update
#endif
  param->last_stat.tracked += icount;
}

void LPTX_param_count_exited(LPTX_param *param, LPTX_idtype icount)
{
#ifdef _OPENMP
#pragma omp atomic update
#endif
  param->cumulative_stat.exited += icount;

#ifdef _OPENMP
#pragma omp atomic update
#endif
  param->last_stat.exited += icount;
}

void LPTX_param_count_sent(LPTX_param *param, LPTX_idtype icount)
{
#ifdef _OPENMP
#pragma omp atomic update
#endif
  param->cumulative_stat.sent += icount;

#ifdef _OPENMP
#pragma omp atomic update
#endif
  param->last_stat.sent += icount;
}

void LPTX_param_count_recved(LPTX_param *param, LPTX_idtype icount)
{
#ifdef _OPENMP
#pragma omp atomic update
#endif
  param->cumulative_stat.recved += icount;

#ifdef _OPENMP
#pragma omp atomic update
#endif
  param->last_stat.recved += icount;
}

void LPTX_param_count_allocated(LPTX_param *param)
{
  LPTX_idtype cnp;
  LPTX_idtype np = LPTX_param_get_total_particles(param);

#ifdef _OPENMP
#pragma omp atomic read
#endif
  cnp = param->cumulative_stat.allocated;
  if (np > cnp) {
#ifdef _OPENMP
#pragma omp critical
#endif
    {
      if (np > param->cumulative_stat.allocated)
        param->cumulative_stat.allocated = np;
    }
  }

#ifdef _OPENMP
#pragma omp atomic write
#endif
  param->last_stat.allocated = np;
}

#ifdef JUPITER_LPTX_MPI
MPI_Comm LPTX_param_mpi_comm(LPTX_param *param) { return param->mpi_comm; }

int LPTX_param_set_mpi_comm(LPTX_param *param, MPI_Comm comm)
{
  return MPI_Comm_dup(comm, &param->mpi_comm);
}

int LPTX_param_mpi_errhandler(LPTX_param *param, MPI_Errhandler *handler)
{
  if (param->mpi_comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;
  return MPI_Comm_get_errhandler(param->mpi_comm, handler);
}

int LPTX_param_set_mpi_errhandler(LPTX_param *param, MPI_Errhandler handler)
{
  if (param->mpi_comm == MPI_COMM_NULL)
    return MPI_ERR_ARG;
  return MPI_Comm_set_errhandler(param->mpi_comm, handler);
}
#endif

LPTX_particle_init_set *LPTX_param_get_init_set(LPTX_param *param, int n)
{
  LPTX_particle_init_set *p = &param->init_sets_head;
  if (n >= 0) {
    for (; p && n >= 0; --n)
      p = LPTX_particle_init_set_next(p);
  } else {
    for (; p && n < 0; ++n)
      p = LPTX_particle_init_set_prev(p);
  }
  return p;
}

LPTX_particle_init_set *LPTX_param_get_first_init_set(LPTX_param *param)
{
  return LPTX_param_get_init_set(param, 0);
}

LPTX_particle_init_set *LPTX_param_get_last_init_set(LPTX_param *param)
{
  return LPTX_param_get_init_set(param, -1);
}

LPTX_particle_set *LPTX_param_get_particle_set(LPTX_param *param, int n)
{
  if (n >= 0) {
    LPTX_particle_set *p = LPTX_param_get_first_particle_set(param);
    for (; p && n > 0; --n)
      p = LPTX_particle_set_next(p);
    return p;
  } else {
    LPTX_particle_set *p = LPTX_param_get_last_particle_set(param);
    for (; p && n < -1; ++n)
      p = LPTX_particle_set_prev(p);
    return p;
  }
}

LPTX_particle_set *LPTX_param_get_first_particle_set(LPTX_param *param)
{
  struct geom_list *p;
  p = geom_list_next(&param->sets_head.list);
  if (p == &param->sets_head.list)
    return NULL;
  return LPTX_particle_set_entry(p);
}

LPTX_particle_set *LPTX_param_get_last_particle_set(LPTX_param *param)
{
  struct geom_list *p;
  p = geom_list_prev(&param->sets_head.list);
  if (p == &param->sets_head.list)
    return NULL;
  return LPTX_particle_set_entry(p);
}

struct LPTX_nptsum_data
{
  LPTX_idtype sum;
};

static LPTX_bool LPTX_param_get_total_particles_s(LPTX_particle_set *p, void *a)
{
  struct LPTX_nptsum_data *d = (struct LPTX_nptsum_data *)a;
  d->sum += LPTX_particle_set_number_of_particles(p);
  return LPTX_false;
}

LPTX_idtype LPTX_param_get_total_particles(LPTX_param *param)
{
  struct LPTX_nptsum_data d = {.sum = 0};
  LPTX_param_foreach_particle_sets_i(param, LPTX_param_get_total_particles_s,
                                     &d);
  return d.sum;
}

static LPTX_bool
LPTX_param_get_total_particles_in_init_sets_s(LPTX_particle_init_set *p,
                                              void *a)
{
  struct LPTX_nptsum_data *d = (struct LPTX_nptsum_data *)a;
  d->sum += LPTX_particle_init_set_number_of_particles(p);
  return LPTX_false;
}

LPTX_idtype LPTX_param_get_total_particles_in_init_sets(LPTX_param *param)
{
  struct LPTX_nptsum_data d = {.sum = 0};
  LPTX_param_foreach_init_sets_i(param,
                                 LPTX_param_get_total_particles_in_init_sets_s,
                                 &d);
  return d.sum;
}

void LPTX_param_foreach_particle_sets(LPTX_param *param,
                                      LPTX_cb_foreach_particle_sets *func,
                                      void *arg)
{
  LPTX_param_foreach_particle_sets_i(param, func, arg);
}

void LPTX_param_foreach_particle_sets_safe(LPTX_param *param,
                                           LPTX_cb_foreach_particle_sets *func,
                                           void *arg)
{
  LPTX_param_foreach_particle_sets_si(param, func, arg);
}

void LPTX_param_foreach_particle_set_range(LPTX_param *param, LPTX_idtype start,
                                           LPTX_idtype last,
                                           LPTX_bool pts_range,
                                           LPTX_cb_foreach_particle_range *func,
                                           void *arg)
{
  if (pts_range) {
    LPTX_param_foreach_particle_set_range_pi(param, start, last, func, arg);
  } else {
    LPTX_param_foreach_particle_set_range_i(param, start, last, func, arg);
  }
}

void LPTX_param_foreach_particle_set_range_safe(
  LPTX_param *param, LPTX_idtype start, LPTX_idtype last, LPTX_bool pts_range,
  LPTX_cb_foreach_particle_range *func, void *arg)
{
  if (pts_range) {
    LPTX_param_foreach_particle_set_range_spi(param, start, last, func, arg);
  } else {
    LPTX_param_foreach_particle_set_range_si(param, start, last, func, arg);
  }
}

struct LPTX_foreach_particles_data
{
  LPTX_bool parallel;
  LPTX_cb_foreach_particles *func;
  void *arg;
};

static LPTX_bool LPTX_param_foreach_particles_i(LPTX_particle_set *set,
                                                void *arg)
{
  struct LPTX_foreach_particles_data *d = arg;
  LPTX_bool parallel = LPTX_false;
  LPTX_idtype np;

#ifdef _OPENMP
  parallel = d->parallel;
#endif
  np = LPTX_particle_set_number_of_particles(set);

  if (parallel) {
#ifdef _OPENMP
    static LPTX_bool grt;
#pragma omp atomic write
    grt = LPTX_false;
#endif
    LPTX_bool rt = LPTX_false;
#ifdef _OPENMP
#pragma omp for
#endif
    for (LPTX_idtype jj = 0; jj < np; ++jj) {
      if (d->func(&set->particles[jj], d->arg))
        rt = LPTX_true;
    }
#ifdef _OPENMP
    if (rt) {
#pragma omp atomic write
      grt = LPTX_true;
    }
#pragma omp barrier
#pragma omp atomic read
    rt = grt;
#endif
    return rt;
  } else {
    for (LPTX_idtype jj = 0; jj < np; ++jj) {
      if (d->func(&set->particles[jj], d->arg))
        return LPTX_true;
    }
    return LPTX_false;
  }
}

void LPTX_param_foreach_particles(LPTX_param *param, LPTX_bool parallel,
                                  LPTX_cb_foreach_particles *func, void *arg)
{
  struct LPTX_foreach_particles_data d = {
    .parallel = parallel,
    .func = func,
    .arg = arg,
  };
  LPTX_param_foreach_particle_sets_i(param, LPTX_param_foreach_particles_i, &d);
}

struct LPTX_foreach_particle_range_data
{
  LPTX_bool parallel;
  LPTX_cb_foreach_particles *countif;
  void *countarg;
  LPTX_cb_foreach_particles *func;
  void *funcarg;
  LPTX_idtype cnt;
};

static LPTX_bool
LPTX_param_foreach_particle_range_ip(LPTX_particle_set *set, LPTX_idtype jj,
                                     struct LPTX_foreach_particle_range_data *d,
                                     LPTX_idtype *lcnt)
{
  LPTX_particle_data *p = &set->particles[jj];
  if (!d->countif(p, d->countarg)) {
    (*lcnt)++;
    return LPTX_false;
  }
  return d->func(p, d->funcarg);
}

static LPTX_bool LPTX_param_foreach_particle_range_i(LPTX_particle_set *set,
                                                     LPTX_idtype start,
                                                     LPTX_idtype *last,
                                                     void *arg)

{
  struct LPTX_foreach_particle_range_data *d = arg;
  LPTX_bool parallel = LPTX_false;
  LPTX_idtype llcnt = 0; /* count for skipped particles */

#ifdef _OPENMP
  parallel = d->parallel;
#endif
  llcnt = 0;

  if (parallel) {
    LPTX_bool rt = LPTX_false;
#ifdef _OPENMP
    static LPTX_idtype lgcnt;
    static LPTX_bool grt;
#pragma omp atomic write
    lgcnt = 0;
#pragma omp atomic write
    grt = LPTX_false;
#endif

#ifdef _OPENMP
#pragma omp for
#endif
    for (LPTX_idtype jj = start; jj < *last; ++jj) {
      if (LPTX_param_foreach_particle_range_ip(set, jj, d, &llcnt))
        rt = LPTX_true;
    }

#ifdef _OPENMP
#pragma omp atomic update
    lgcnt += llcnt;
#pragma omp barrier
#pragma omp atomic read
    llcnt = lgcnt;
#endif
    d->cnt += *last - start - llcnt;
    *last += llcnt;

#ifdef _OPENMP
    if (rt) {
#pragma omp atomic write
      grt = rt;
    }
#pragma omp barrier
#pragma omp atomic read
    rt = grt;
#endif
    if (rt)
      return rt;
    return LPTX_false;

  } else {
    for (LPTX_idtype jj = start; jj < *last; ++jj) {
      if (LPTX_param_foreach_particle_range_ip(set, jj, d, &llcnt)) {
        d->cnt += jj - start - llcnt;
        return LPTX_true;
      }
    }

    d->cnt += *last - start - llcnt;
    *last += llcnt;
    return LPTX_false;
  }
}

LPTX_idtype LPTX_param_foreach_particle_range(
  LPTX_param *param, LPTX_bool parallel, LPTX_idtype start, LPTX_idtype last,
  LPTX_cb_foreach_particles *countif, void *countarg,
  LPTX_cb_foreach_particles *func, void *funcarg)
{
  struct LPTX_foreach_particle_range_data d = {
    .parallel = parallel,
    .countif = countif,
    .countarg = countarg,
    .func = func,
    .funcarg = funcarg,
    .cnt = 0,
  };
  LPTX_param_foreach_particle_set_range(param, start, last, LPTX_true,
                                        LPTX_param_foreach_particle_range_i,
                                        &d);
  return d.cnt;
}

void LPTX_param_foreach_init_sets(LPTX_param *param,
                                  LPTX_cb_foreach_init_sets *func, void *arg)
{
  LPTX_param_foreach_init_sets_i(param, func, arg);
}

void LPTX_param_foreach_init_sets_safe(LPTX_param *param,
                                       LPTX_cb_foreach_init_sets *func,
                                       void *arg)
{
  LPTX_param_foreach_init_sets_si(param, func, arg);
}

static int LPTX_param_mpi_foreach_particle_sets(
  LPTX_param *param,
  int (*func)(LPTX_param *param, LPTX_particle_set *set, void *a), void *a)
{
#ifdef JUPITER_LPTX_MPI
  int r;
  struct geom_list *lp, *ln, *lh;

  if (param->mpi_comm == MPI_COMM_NULL)
    return MPI_ERR_COMM;

  r = MPI_SUCCESS;
  lh = &param->sets_head.list;
  lp = geom_list_next(lh);
  ln = geom_list_next(lp);
  while (1) {
    LPTX_particle_set *set;
    if (LPTX_MPI_forall(lp == lh, param->mpi_comm, &r))
      break;
    if (r != MPI_SUCCESS)
      break;

    set = (lp == lh) ? NULL : LPTX_particle_set_entry(lp);

    r = func(param, set, a);
    if (lp != lh) {
      lp = ln;
      ln = geom_list_next(ln);
    }

    if (!LPTX_MPI_forall(r == MPI_SUCCESS, param->mpi_comm, &r))
      break;
  }
  return r;
#else
  return 0;
#endif
}

#ifdef JUPITER_LPTX_MPI
struct LPTX_param_redist_particles_impl_data
{
  LPTX_cb_mpirank *mpirank_func;
  void *arg;
  int *rank;
};

static int LPTX_param_redist_particles_impl(LPTX_param *param,
                                            LPTX_particle_set *set, void *a)
{
  LPTX_idtype np;
  int r;
  int szr;
  struct LPTX_param_redist_particles_impl_data *d = a;

  r = MPI_SUCCESS;
  if (!d->rank) {
    LPTX_idtype max_npt = 0;
    struct geom_list *lp, *lh;

    lh = &param->sets_head.list;
    geom_list_foreach (lp, lh) {
      LPTX_idtype np;
      LPTX_particle_set *s;
      s = LPTX_particle_set_entry(lp);
      np = LPTX_particle_set_number_of_particles(s);
      if (np > max_npt)
        max_npt = np;
    }
    if (max_npt < 1)
      max_npt = 1;
    d->rank = (int *)calloc(max_npt, sizeof(int));
  }
  if (!LPTX_MPI_forall(!!d->rank, param->mpi_comm, &r)) {
    if (r == MPI_SUCCESS)
      r = MPI_ERR_NO_MEM;
    return r;
  }

  np = 0;
  if (set)
    np = LPTX_particle_set_number_of_particles(set);
  szr = np;
  if (set && szr == np) {
    np = LPTX_particle_set_calc_mpirank_c(set, 0, szr, d->rank, d->mpirank_func,
                                          d->arg);
  } else {
    /* Local looping for overflowing number of particles is not supported yet */
    np = 0;
  }

  if (LPTX_MPI_forany(np > 0, param->mpi_comm, &r)) {
    LPTX_particle_set *out;
    r = LPTX_particle_set_alltoall(&out, set, 0, szr, d->rank, param->mpi_comm);
    if (r == MPI_SUCCESS && out) {
      if (set) {
        LPTX_particle_set_insert_prev(set, out);
        LPTX_particle_set_delete(set);
      } else {
        LPTX_particle_set_append(param, out);
      }
    }

    do {
      int scnt, rcnt;
      int irank;
      int rr;

      if (np < 0)
        break;

      rr = MPI_Comm_rank(param->mpi_comm, &irank);
      if (rr != MPI_SUCCESS)
        break;

      scnt = 0;
      rcnt = 0;
      for (int i = 0; i < szr; ++i) {
        if (d->rank[i] == irank) {
          rcnt += 1;
        } else {
          scnt += 1;
        }
      }

      np = 0;
      if (out)
        np = LPTX_particle_set_number_of_particles(out);
      LPTX_param_count_sent(param, scnt);
      LPTX_param_count_recved(param, np - rcnt);
    } while (0);
  }

  return r;
}
#endif

int LPTX_param_redistribute_particles(LPTX_param *param,
                                      LPTX_cb_mpirank *mpirank_func, void *arg)
{
#ifdef JUPITER_LPTX_MPI
  int r;
  struct LPTX_param_redist_particles_impl_data d = {
    .mpirank_func = mpirank_func,
    .arg = arg,
    .rank = NULL,
  };

  r =
    LPTX_param_mpi_foreach_particle_sets(param,
                                         LPTX_param_redist_particles_impl, &d);

  if (d.rank)
    free(d.rank);

  return r;
#else
  return 0;
#endif
}

/*
 * This function is defined in particle.c
 */
int LPTX_param_redistribute_particles_rect_v(LPTX_param *param, LPTX_vector lb,
                                             LPTX_vector ub,
                                             LPTX_vector_rect_flags flg);

#ifdef JUPITER_LPTX_MPI
struct LPTX_param_gather_data
{
  int root;
};

static int LPTX_param_gather_impl(LPTX_param *param, LPTX_particle_set *set,
                                  void *arg)
{
  int irank;
  int r;
  struct LPTX_param_gather_data *d;
  LPTX_particle_set *outp;

  d = (struct LPTX_param_gather_data *)arg;

  r = MPI_Comm_rank(param->mpi_comm, &irank);
  if (r != MPI_SUCCESS)
    return r;

  outp = NULL;
  r = LPTX_particle_set_gather(&outp, set, d->root, param->mpi_comm);
  if (r != MPI_SUCCESS)
    return r;

  if (!outp) {
    outp = LPTX_param_new_particle_set(param, 0);
    if (!outp)
      r = MPI_ERR_NO_MEM;
  }
  if (!LPTX_MPI_forall(r == MPI_SUCCESS, param->mpi_comm, &r)) {
    if (r == MPI_SUCCESS)
      r = MPI_ERR_NO_MEM;
  }

  if (irank == d->root) {
    if (outp) {
      LPTX_idtype np;

      np = LPTX_particle_set_number_of_particles(outp);
      if (set)
        np -= LPTX_particle_set_number_of_particles(set);
      if (np > 0)
        LPTX_param_count_recved(param, np);
    }
  } else {
    if (set)
      LPTX_param_count_sent(param, LPTX_particle_set_number_of_particles(set));
  }

  if (outp) {
    if (set) {
      LPTX_particle_set_insert_prev(set, outp);
      LPTX_particle_set_delete(set);
    } else {
      LPTX_particle_set_append(param, outp);
    }
  }

  return r;
}
#endif

int LPTX_param_gather_particles(LPTX_param *param, int root)
{
#ifdef JUPITER_LPTX_MPI
  int r;
  struct LPTX_param_gather_data d = {
    .root = root,
  };

  r = LPTX_param_mpi_foreach_particle_sets(param, LPTX_param_gather_impl, &d);

  return r;
#else
  return 0;
#endif
}

//---

LPTX_particle_init_set *LPTX_particle_init_set_new(void)
{
  LPTX_particle_init_set *pset;
  pset = (LPTX_particle_init_set *)calloc(1, sizeof(LPTX_particle_init_set));
  if (!pset)
    return NULL;

  LPTX_particle_init_set_init(pset);
  return pset;
}

void LPTX_particle_init_set_delete(LPTX_particle_init_set *set)
{
  if (!set)
    return;

  geom_list_delete(&set->list);
  free(set);
}

//---

static LPTX_idtype LPTX_calc_storage_size(LPTX_idtype nelement,
                                          LPTX_idtype elsize)
{
  LPTX_idtype sz;

  if (nelement <= 0)
    return 0;

  if (LPTX_s_mul_overflow(nelement, elsize, &sz))
    return -1;
  if (LPTX_s_sub_overflow(sz, 1, &sz))
    return -1;

  sz /= sizeof(LPTX_particle_storage);
  if (LPTX_s_add_overflow(sz, 1, &sz))
    return -1;
  return sz;
}

LPTX_particle_set *LPTX_particle_set_new(LPTX_idtype number_of_particles,
                                         LPTX_idtype number_of_vectors,
                                         const LPTX_idtype *numbers_of_data)
{
  LPTX_idtype num_vectors, num_data;
  LPTX_idtype nd, ns, nsr, nstor, np, nv, nnv, nt;
  LPTX_particle_set *set;
  LPTX_idtype nvecsize;

  enum sizes
  {
    ptr_size = sizeof(LPTX_particle_data *),
    pt_size = sizeof(LPTX_particle_data),
    v_size = sizeof(LPTX_particle_vector),
    d_size = sizeof(LPTX_type),
    id_size = sizeof(LPTX_idtype),
    unit_size = sizeof(union LPTX_particle_storage),
    set_size = sizeof(LPTX_particle_set),
  };

  LPTX_assert(number_of_particles >= 0);
  LPTX_assert(number_of_vectors >= 0);

  if (LPTX_s_mul_overflow(number_of_vectors, number_of_particles, &num_vectors))
    return NULL;

  nvecsize = 0;
  for (LPTX_idtype jj = 0; jj < number_of_vectors; ++jj) {
    LPTX_assert(numbers_of_data[jj] >= 0);
    if (LPTX_s_add_overflow(nvecsize, numbers_of_data[jj], &nvecsize))
      return NULL;
  }

  if (LPTX_s_mul_overflow(nvecsize, number_of_particles, &num_data))
    return NULL;

  nnv = LPTX_calc_storage_size(number_of_vectors, id_size);
  ns = LPTX_calc_storage_size(number_of_particles, ptr_size);
  nsr = LPTX_calc_storage_size(number_of_particles, id_size);
  np = LPTX_calc_storage_size(number_of_particles, pt_size);
  nv = LPTX_calc_storage_size(num_vectors, v_size);
  nd = LPTX_calc_storage_size(num_data, d_size);

  if (LPTX_s_add_overflow(nnv, ns, &nt))
    return NULL;
  if (LPTX_s_add_overflow(nt, nsr, &nt))
    return NULL;
  if (LPTX_s_add_overflow(nt, np, &nt))
    return NULL;
  if (LPTX_s_add_overflow(nt, nv, &nt))
    return NULL;
  if (LPTX_s_add_overflow(nt, nd, &nt))
    return NULL;

  nstor = nt;
  if (LPTX_s_mul_overflow(nt, unit_size, &nt))
    return NULL;

  if (LPTX_s_add_overflow(nt, set_size, &nt))
    return NULL;

  set = (LPTX_particle_set *)malloc(nt);
  if (!set)
    return NULL;

  LPTX_particle_set_list_init(&set->list);
  set->number_particles = number_of_particles;
  set->number_vectors = number_of_vectors;
  set->number_data = nvecsize;
  set->number_storage = nstor;

  nt = 0;
  set->vector_sizes = (number_of_vectors > 0) ? &set->storage[nt].i : NULL;
  nt += nnv;
  set->sorted = (number_of_particles > 0) ? &set->storage[nt].pt : NULL;
  nt += ns;
  set->sorted_indices = (number_of_particles > 0) ? &set->storage[nt].i : NULL;
  nt += nsr;
  set->particles = (number_of_particles > 0) ? &set->storage[nt].p : NULL;
  nt += np;
  set->vectors = (num_vectors > 0) ? &set->storage[nt].v : NULL;
  nt += nv;
  set->data = (num_data > 0) ? &set->storage[nt].s : NULL;

  for (LPTX_idtype jj = 0; jj < number_of_vectors; ++jj)
    set->vector_sizes[jj] = numbers_of_data[jj];

#ifdef _OPENMP
#pragma omp parallel if (number_of_particles >= LPTX_omp_small_threshold)
#endif
  {
    LPTX_type *data_p;
    LPTX_particle_vector *vect_p;

#ifdef _OPENMP
#pragma omp for
#endif
    for (LPTX_idtype jj = 0; jj < number_of_particles; ++jj) {
      set->sorted[jj] = &set->particles[jj];
      set->sorted_indices[jj] = jj;
      data_p = NULL;
      vect_p = &set->vectors[jj * number_of_vectors];
      if (set->data)
        data_p = &set->data[jj * nvecsize];
      LPTX_particle_data_init(&set->particles[jj], number_of_vectors, vect_p,
                              data_p, set->vector_sizes);
    }
  }

  return set;
}

LPTX_particle_set *LPTX_param_new_particle_set(LPTX_param *param,
                                               LPTX_idtype number_of_particles)
{
  LPTX_assert(!!param);

  return LPTX_particle_set_new(number_of_particles,
                               param->number_of_particle_vectors,
                               param->particle_vector_sizes);
}

LPTX_particle_set *LPTX_particle_set_replicate(LPTX_idtype number_of_particles,
                                               const LPTX_particle_set *base)
{
  LPTX_idtype nv;
  const LPTX_idtype *vsz;

  LPTX_assert(!!base);

  nv = LPTX_particle_set_number_of_vectors(base);
  vsz = LPTX_particle_set_vector_sizes(base);
  return LPTX_particle_set_new(number_of_particles, nv, vsz);
}

LPTX_particle_set *LPTX_particle_set_from_init_set(LPTX_param *param,
                                                   LPTX_particle_set *old)
{
  LPTX_particle_init_set *iset;
  LPTX_particle_set *set;
  LPTX_idtype npts, old_npts;
  jupiter_random_seed_counter randc;

  iset = LPTX_param_get_init_set(param, 0);
  if (!iset)
    return NULL;

  npts = LPTX_param_get_total_particles_in_init_sets(param);
  if (npts < 0)
    return NULL;

  old_npts = old ? LPTX_particle_set_number_of_particles(old) : 0;
  if (old_npts < npts) {
    set = LPTX_param_new_particle_set(param, npts);
    if (!set)
      return NULL;
    LPTX_assert_particle_set_are_mergeable(set, old);
  } else {
    set = old;
  }

  randc.counter = 0;
  randc.seed = param->random_seed;
#ifdef _OPENMP
#pragma omp parallel if (npts >= LPTX_omp_small_threshold)
#endif
  {
    LPTX_particle_init_set *liset = iset;
    struct geom_list *lp, *lh;
    LPTX_idtype lb = 0;
    LPTX_idtype ub = liset->number_particles;
    jupiter_random_seed_counter seed;
#ifdef _OPENMP
    jupiter_random_jump128c_omp(&randc, &seed);
#else
    seed = randc;
#endif

#ifdef _OPENMP
#pragma omp for
#endif
    for (LPTX_idtype jj = 0; jj < npts; ++jj) {
#ifdef _OPENMP
      if (!liset)
        continue; /* cannot break in openmp loop */
#endif

      while (jj >= ub || jj < lb) {
        if (jj >= ub) {
          liset = LPTX_particle_init_set_next(liset);
        } else {
          /* possibly unreachable */
          liset = LPTX_particle_init_set_prev(liset);
        }
        if (!liset)
          break;

        if (jj >= ub) {
          lb = ub;
          ub = lb + liset->number_particles;
        } else {
          ub = lb;
          lb = ub - liset->number_particles;
        }
      }

#ifdef _OPENMP
      if (!liset)
        continue;
#else
      if (!liset)
        break;
#endif

      const LPTX_particle *opt;
      LPTX_particle pt;
      LPTX_bool used;

      if (old) {
        opt = LPTX_particle_set_get_particle_at(old, jj, NULL, NULL);
        pt = *opt;
      } else {
        opt = NULL;
        pt = *LPTX_particle_set_get_particle_at(set, jj, NULL, NULL);
      }

      used = LPTX_particle_is_used(&pt);
      pt.diameter = liset->diameter;
      pt.density = liset->density;
      pt.thermal_conductivity = liset->thermal_conductivity;
      pt.specific_heat = liset->specific_heat;
      pt.flags = liset->flags;
      LPTX_particle_set_used(&pt, used);

      if (!LPTX_particle_is_used(&pt)) {
        LPTX_vector l;
        LPTX_vector v;
        LPTX_type x, y, z, t;

        LPTX_particle_set_used(&pt, LPTX_true);
        x = LPTX_jupiter_random_nexttc(&seed);
        y = LPTX_jupiter_random_nexttc(&seed);
        z = LPTX_jupiter_random_nexttc(&seed);
        v = LPTX_vector_c(x, y, z);
        l = LPTX_vector_sub(liset->range_end, liset->range_start);
        v = LPTX_vector_add(liset->range_start, LPTX_vector_mul_each(l, v));

        t = liset->time_end - liset->time_start;
        if (liset->time_random) {
          t = t * LPTX_jupiter_random_nexttc(&seed);
        } else {
          t = t * ((LPTX_type)(jj - lb)) / liset->number_particles;
        }

        pt.particle_id = jj;
        pt.origin_id = liset->origin_id;
        pt.velocity = liset->velocity;
        pt.position = v;
        pt.parceln = 1.0;
        pt.start_time = t + liset->time_start;
        pt.temperature = liset->temperature;
        pt.init_position = pt.position;
        pt.init_velocity = pt.velocity;
        pt.init_temperature = pt.temperature;
        pt.ptseed = seed.seed;
        jupiter_random_jump32c(&seed);
      }

      LPTX_particle_set_set_particle_at(set, jj, &pt, 0, NULL, NULL);
    }
#ifdef _OPENMP
    jupiter_random_syncc_omp(&randc, &seed);
#else
    randc = seed;
#endif
  }
  param->random_seed = randc.seed;

  return set;
}

void LPTX_particle_set_delete(LPTX_particle_set *set)
{
  geom_list_delete(&set->list.list);
  free(set);
}
