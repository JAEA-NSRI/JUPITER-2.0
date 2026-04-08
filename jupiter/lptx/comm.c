#include "comm.h"
#include "defs.h"
#include "overflow.h"
#include "particle.h"
#include "priv_struct_defs.h"
#include "priv_util.h"
#include "ptflags.h"
#include "struct_defs.h"
#include "util.h"
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef JUPITER_LPTX_MPI
#include <mpi.h>
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef JUPITER_LPTX_MPI

static int LPTX_MPI_Error_no_mem(char *string, int *resultlen)
{
  *resultlen =
    snprintf(string, LPTX_MPI_MAX_ERROR_STRING, "Memory allocation failed");
  return MPI_SUCCESS;
}

static int LPTX_MPI_Error_vector_size_mismatch(char *string, int *resultlen)
{
  *resultlen = snprintf(string, LPTX_MPI_MAX_ERROR_STRING,
                        "Number of vectors or size of any vectors in "
                        "communicating particle sets are mismatched");
  return MPI_SUCCESS;
}

static int LPTX_MPI_Error_remote(char *string, int *resultlen)
{
  *resultlen =
    snprintf(string, LPTX_MPI_MAX_ERROR_STRING, "Error occured on remote rank");
  return MPI_SUCCESS;
}

static int LPTX_MPI_Error_overflow(char *string, int *resultlen)
{
  *resultlen =
    snprintf(string, LPTX_MPI_MAX_ERROR_STRING, "Arithmetic overflow detected");
  return MPI_SUCCESS;
}

int LPTX_MPI_Error_string(int errorcode, char *string, int *resultlen)
{
  switch ((LPTX_MPI_errors)errorcode) {
  case LPTX_MPI_ERR_NO_MEM:
    return LPTX_MPI_Error_no_mem(string, resultlen);
  case LPTX_MPI_ERR_VECTOR_SIZE_MISMATCH:
    return LPTX_MPI_Error_vector_size_mismatch(string, resultlen);
  case LPTX_MPI_ERR_REMOTE:
    return LPTX_MPI_Error_remote(string, resultlen);
  case LPTX_MPI_ERR_OVERFLOW:
    return LPTX_MPI_Error_overflow(string, resultlen);

  case LPTX_MPI_ERR_BASE:
  case LPTX_MPI_ERR_LASTCODE:
    break;
  }
  return MPI_Error_string(errorcode, string, resultlen);
}

//---

static int
LPTX_mpi_type_create_type_struct(int count, const int *array_of_block_lengths,
                                 const MPI_Aint *array_of_displacements,
                                 const MPI_Datatype *array_of_types, size_t ub,
                                 MPI_Datatype *newtype)
{
  MPI_Datatype type;
  int r;

  r = MPI_Type_create_struct(count, array_of_block_lengths,
                             array_of_displacements, array_of_types, &type);
  if (r != MPI_SUCCESS)
    return r;

  r = MPI_Type_create_resized(type, 0, ub, newtype);

  MPI_Type_free(&type);
  if (r != MPI_SUCCESS)
    return r;

  r = MPI_Type_commit(newtype);
  if (r != MPI_SUCCESS) {
    MPI_Type_free(newtype);
    return r;
  }

  return r;
}

int LPTX_vector_mpi_type(MPI_Datatype *out)
{
  int bls[] = LPTX_vector_MPI_data(len);
  MPI_Aint displs[] = LPTX_vector_MPI_data(displs);
  MPI_Datatype types[] = LPTX_vector_MPI_data(types);
  int count = sizeof(types) / sizeof(*types);

  return LPTX_mpi_type_create_type_struct(count, bls, displs, types,
                                          sizeof(LPTX_vector), out);
}

int LPTX_particle_mpi_type(MPI_Datatype *out)
{
  int bls[] = LPTX_particle_MPI_data(len);
  MPI_Aint displs[] = LPTX_particle_MPI_data(displs);
  MPI_Datatype types[] = LPTX_particle_MPI_data(types);
  int count = sizeof(types) / sizeof(*types);

  return LPTX_mpi_type_create_type_struct(count, bls, displs, types,
                                          sizeof(LPTX_particle), out);
}

int LPTX_particle_data_mpi_type(MPI_Datatype *out)
{
  int bls[] = LPTX_particle_data_MPI_data(len);
  MPI_Aint displs[] = LPTX_particle_data_MPI_data(displs);
  MPI_Datatype types[] = LPTX_particle_MPI_data(types);
  int count = sizeof(types) / sizeof(*types);

  return LPTX_mpi_type_create_type_struct(count, bls, displs, types,
                                          sizeof(LPTX_particle_data), out);
}

static int LPTX_particle_set_mpi_type(MPI_Datatype *out)
{
  int bls[] = LPTX_particle_set_MPI_data(len);
  MPI_Aint displs[] = LPTX_particle_set_MPI_data(displs);
  MPI_Datatype types[] = LPTX_particle_set_MPI_data(types);
  int count = sizeof(types) / sizeof(*types);

  return LPTX_mpi_type_create_type_struct(count, bls, displs, types,
                                          sizeof(LPTX_particle_set), out);
}

static int LPTX_particle_vector_mpi_type(MPI_Datatype *out)
{
  int bls[] = LPTX_particle_vector_MPI_data(len);
  MPI_Aint displs[] = LPTX_particle_vector_MPI_data(displs);
  MPI_Datatype types[] = LPTX_particle_vector_MPI_data(types);
  int count = sizeof(types) / sizeof(*types);

  return LPTX_mpi_type_create_type_struct(count, bls, displs, types,
                                          sizeof(LPTX_particle_vector), out);
}

//---- (for range-based communication)

static int LPTX_create_vector_type(LPTX_idtype count, LPTX_idtype blocklength,
                                   LPTX_idtype stride, MPI_Datatype oldtype,
                                   MPI_Datatype *newtype)
{
  int r;

  LPTX_assert(!!newtype);
  LPTX_assert(count > 0);
  LPTX_assert(count <= INT_MAX); /* not supported yet */
  LPTX_assert(blocklength > 0);
  LPTX_assert(blocklength <= INT_MAX); /* not supported yet */

  r = MPI_Type_create_hvector(count, blocklength, stride, oldtype, newtype);
  if (r != MPI_SUCCESS)
    return r;

  r = MPI_Type_commit(newtype);
  if (r != MPI_SUCCESS) {
    MPI_Type_free(newtype);
    return r;
  }

  return r;
}

static int LPTX_particle_data_range_type(MPI_Datatype *out, LPTX_idtype count)
{
  int r;
  MPI_Datatype t;

  r = LPTX_particle_data_mpi_type(&t);
  if (r != MPI_SUCCESS)
    return r;

  r = LPTX_create_vector_type(count, 1, sizeof(LPTX_particle_data), t, out);
  MPI_Type_free(&t);
  return r;
}

static int LPTX_particle_vector_range_type(MPI_Datatype *out, LPTX_idtype count,
                                           LPTX_idtype number_vector)
{
  int r;
  MPI_Datatype t;
  LPTX_idtype stride;

  r = LPTX_particle_vector_mpi_type(&t);
  if (r != MPI_SUCCESS)
    return r;

  stride = number_vector * sizeof(LPTX_particle_vector);
  r = LPTX_create_vector_type(count, number_vector, stride, t, out);
  MPI_Type_free(&t);
  return r;
}

static int LPTX_particle_vector_data_range_type(MPI_Datatype *out,
                                                LPTX_idtype count,
                                                LPTX_idtype number_data)
{
  int r;
  LPTX_idtype stride;

  stride = number_data * sizeof(LPTX_type);
  r = LPTX_create_vector_type(count, number_data, stride, LPTX_MPI_TYPE, out);
  return r;
}

//---- (for index-based communication)

/**
 * You can pass @p blocklengths for NULL. If @p blocklengths is NULL, the
 * constant @p blocklength will be used.
 */
static int LPTX_create_index_type(LPTX_idtype count, int *blocklengths,
                                  LPTX_idtype blocklength,
                                  LPTX_idtype *array_of_displacements,
                                  MPI_Datatype oldtype, MPI_Datatype *newtype)
{
  int r;
  int *ablocklengths = NULL;

  LPTX_assert(count > 0);
  LPTX_assert(count <= INT_MAX);

  if (!blocklengths) {
    LPTX_assert(blocklength > 0);
    LPTX_assert(blocklength <= INT_MAX);

    ablocklengths = (int *)malloc(sizeof(int) * count);
    if (!ablocklengths)
      return LPTX_MPI_ERR_NO_MEM;

#ifdef _OPENMP
#pragma omp parallel if (count > LPTX_omp_small_threshold)
#endif
    {
#ifdef _OPENMP
#pragma omp for
#endif
      for (LPTX_idtype jj = 0; jj < count; ++jj)
        ablocklengths[jj] = blocklength;
    }
    blocklengths = ablocklengths;
  }

  r = MPI_SUCCESS;
  do {
    r = MPI_Type_create_hindexed(count, blocklengths, array_of_displacements,
                                 oldtype, newtype);
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Type_commit(newtype);
    if (r != MPI_SUCCESS) {
      MPI_Type_free(newtype);
      break;
    }
  } while (0);

  if (ablocklengths)
    free(ablocklengths);
  return r;
}

static int LPTX_create_displacements_from_indices(LPTX_idtype **displacements,
                                                  LPTX_idtype size_of_indices,
                                                  const LPTX_idtype *indices,
                                                  LPTX_idtype size_per_perticle)
{
  LPTX_idtype *pdisp;
  LPTX_bool overflow;
  LPTX_assert(size_of_indices > 0);

  pdisp = (LPTX_idtype *)malloc(sizeof(LPTX_idtype) * size_of_indices);
  if (!pdisp)
    return LPTX_MPI_ERR_NO_MEM;

  overflow = LPTX_false;
#ifdef _OPENMP
#pragma omp parallel if (size_of_indices > LPTX_omp_small_threshold)
#endif
  {
    LPTX_bool lovf = LPTX_false;
#ifdef _OPENMP
#pragma omp for
#endif
    for (LPTX_idtype jj = 0; jj < size_of_indices; ++jj) {
      if (LPTX_s_mul_overflow(indices[jj], size_per_perticle, &pdisp[jj]))
        lovf = LPTX_true;
    }
    if (lovf) {
#ifdef _OPENMP
#pragma omp atomic write
#endif
      overflow = lovf;
    }
  }

  if (overflow) {
    free(pdisp);
    return LPTX_MPI_ERR_OVERFLOW;
  }

  *displacements = pdisp;
  return MPI_SUCCESS;
}

static int LPTX_particle_data_index_type(MPI_Datatype *out,
                                         LPTX_idtype size_of_indices,
                                         const LPTX_idtype *indices)
{
  int r;
  LPTX_idtype *displs;
  const LPTX_idtype bls = sizeof(LPTX_particle_data);

  r = LPTX_create_displacements_from_indices(&displs, size_of_indices, indices,
                                             bls);
  if (r != MPI_SUCCESS)
    return r;

  do {
    MPI_Datatype t;
    r = LPTX_particle_data_mpi_type(&t);
    if (r != MPI_SUCCESS)
      break;

    r = LPTX_create_index_type(size_of_indices, NULL, 1, displs, t, out);
    MPI_Type_free(&t);
  } while (0);

  free(displs);
  return r;
}

static int LPTX_particle_vector_index_type(MPI_Datatype *out,
                                           LPTX_idtype number_vectors,
                                           LPTX_idtype size_of_indices,
                                           const LPTX_idtype *indices)
{
  int r;
  LPTX_idtype *displs;
  LPTX_idtype bls;

  bls = sizeof(LPTX_particle_vector);
  if (LPTX_s_mul_overflow(number_vectors, bls, &bls))
    return LPTX_MPI_ERR_OVERFLOW;

  r = LPTX_create_displacements_from_indices(&displs, size_of_indices, indices,
                                             bls);
  if (r != MPI_SUCCESS)
    return r;

  do {
    MPI_Datatype t;
    r = LPTX_particle_vector_mpi_type(&t);
    if (r != MPI_SUCCESS)
      break;

    r = LPTX_create_index_type(size_of_indices, NULL, number_vectors, displs, t,
                               out);
    MPI_Type_free(&t);
  } while (0);

  free(displs);
  return r;
}

static int LPTX_particle_vector_data_index_type(MPI_Datatype *out,
                                                LPTX_idtype number_data,
                                                LPTX_idtype size_of_indices,
                                                const LPTX_idtype *indices)
{
  int r;
  LPTX_idtype *displs;
  LPTX_idtype bls;

  bls = sizeof(LPTX_type);
  if (LPTX_s_mul_overflow(number_data, bls, &bls))
    return LPTX_MPI_ERR_OVERFLOW;

  r = LPTX_create_displacements_from_indices(&displs, size_of_indices, indices,
                                             bls);
  if (r != MPI_SUCCESS)
    return r;

  r = LPTX_create_index_type(size_of_indices, NULL, number_data, displs,
                             LPTX_MPI_TYPE, out);
  free(displs);
  return r;
}

//---

typedef int LPTX_particle_type_creater(MPI_Datatype *out,
                                       LPTX_idtype number_vectors,
                                       LPTX_idtype number_data, void *args);

/*
 * Create single composite MPI type based on @p set.
 */
static int LPTX_composite_particle_type(
  MPI_Datatype *out, const LPTX_particle_set *set, LPTX_idtype start,
  LPTX_particle_type_creater *particle_data_type_creater,
  LPTX_particle_type_creater *particle_vector_type_creater,
  LPTX_particle_type_creater *vector_data_type_creater, void *args)
{
  int counts[3];
  MPI_Aint displs[3];
  MPI_Datatype types[3];
  LPTX_idtype nv, nd;
  const LPTX_particle_data *pstart;
  const LPTX_particle_vector *vstart;
  const LPTX_type *dstart;
  int ncomp = 0;
  int r;

  nv = set->number_vectors;
  nd = set->number_data;

  for (int i = 0; i < 3; ++i) {
    counts[i] = 1;
    types[i] = MPI_DATATYPE_NULL;
  }

  do {
    pstart = &set->particles[start];
    displs[ncomp] = (const char *)pstart - (const char *)set;
    r = particle_data_type_creater(&types[ncomp], nv, nd, args);
    if (r != MPI_SUCCESS)
      break;
    ++ncomp;

    if (nv > 0) {
      LPTX_assert(nd >= 0);
      vstart = &set->vectors[set->number_vectors * start];
      displs[ncomp] = (const char *)vstart - (const char *)set;
      r = particle_vector_type_creater(&types[ncomp], nv, nd, args);
      if (r != MPI_SUCCESS)
        break;
      ++ncomp;

      if (nd > 0) {
        dstart = &set->data[set->number_data * start];
        displs[ncomp] = (const char *)dstart - (const char *)set;
        r = vector_data_type_creater(&types[ncomp], nv, nd, args);
        if (r != MPI_SUCCESS)
          break;
        ++ncomp;
      }
    } else {
      LPTX_assert(nd <= 0);
    }

    r = MPI_Type_create_struct(ncomp, counts, displs, types, out);
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Type_commit(out);
    if (r != MPI_SUCCESS) {
      MPI_Type_free(out);
      break;
    }
  } while (0);

  for (int i = 0; i < 3; ++i)
    if (types[i] != MPI_DATATYPE_NULL)
      MPI_Type_free(&types[i]);

  return r;
}

//---

struct LPTX_range_type_args
{
  int count;
};

static struct LPTX_range_type_args LPTX_range_type_args_init(int count)
{
  return (struct LPTX_range_type_args){.count = count};
}

static int LPTX_particle_data_range_type_creater(MPI_Datatype *out,
                                                 LPTX_idtype number_vectors,
                                                 LPTX_idtype number_data,
                                                 void *args)
{
  struct LPTX_range_type_args *a = args;
  return LPTX_particle_data_range_type(out, a->count);
}

static int LPTX_particle_vector_range_type_creater(MPI_Datatype *out,
                                                   LPTX_idtype number_vectors,
                                                   LPTX_idtype number_data,
                                                   void *args)
{
  struct LPTX_range_type_args *a = args;
  return LPTX_particle_vector_range_type(out, a->count, number_vectors);
}

static int LPTX_vector_data_range_type_creater(MPI_Datatype *out,
                                               LPTX_idtype number_vectors,
                                               LPTX_idtype number_data,
                                               void *args)
{
  struct LPTX_range_type_args *a = args;
  return LPTX_particle_vector_data_range_type(out, a->count, number_data);
}

static int LPTX_composite_particle_range_type(MPI_Datatype *out,
                                              const LPTX_particle_set *set,
                                              LPTX_idtype start, int count)
{
  struct LPTX_range_type_args args;
  args = LPTX_range_type_args_init(count);
  return LPTX_composite_particle_type(out, set, start,
                                      LPTX_particle_data_range_type_creater,
                                      LPTX_particle_vector_range_type_creater,
                                      LPTX_vector_data_range_type_creater,
                                      &args);
}

//---

struct LPTX_index_type_args
{
  LPTX_idtype size_of_indices;
  const LPTX_idtype *indices;
};

static struct LPTX_index_type_args
LPTX_index_type_args_init(LPTX_idtype size_of_indices,
                          const LPTX_idtype *indices)
{
  return (struct LPTX_index_type_args){
    .size_of_indices = size_of_indices,
    .indices = indices,
  };
}

static int LPTX_particle_data_index_type_creater(MPI_Datatype *out,
                                                 LPTX_idtype number_vectors,
                                                 LPTX_idtype number_data,
                                                 void *args)
{
  struct LPTX_index_type_args *a = args;
  return LPTX_particle_data_index_type(out, a->size_of_indices, a->indices);
}

static int LPTX_particle_vector_index_type_creater(MPI_Datatype *out,
                                                   LPTX_idtype number_vectors,
                                                   LPTX_idtype number_data,
                                                   void *args)
{
  struct LPTX_index_type_args *a = args;
  return LPTX_particle_vector_index_type(out, number_vectors,
                                         a->size_of_indices, a->indices);
}

static int LPTX_vector_data_index_type_creater(MPI_Datatype *out,
                                               LPTX_idtype number_vectors,
                                               LPTX_idtype number_data,
                                               void *args)
{
  struct LPTX_index_type_args *a = args;
  return LPTX_particle_vector_data_index_type(out, number_data,
                                              a->size_of_indices, a->indices);
}

static int LPTX_composite_particle_index_type(MPI_Datatype *out,
                                              const LPTX_particle_set *set,
                                              LPTX_idtype size_of_indices,
                                              const LPTX_idtype *indices)
{
  struct LPTX_index_type_args args;
  args = LPTX_index_type_args_init(size_of_indices, indices);
  return LPTX_composite_particle_type(out, set, 0,
                                      LPTX_particle_data_index_type_creater,
                                      LPTX_particle_vector_index_type_creater,
                                      LPTX_vector_data_index_type_creater,
                                      &args);
}

//----

/**
 * @param set_root The first root rank that @p set is not NULL [out]
 * @param irank The caller's rank
 * @return any MPI error
 */
static int LPTX_particle_set_root(int *set_root, int irank,
                                  const LPTX_particle_set *set, MPI_Comm comm)
{
  int r;
  int set_root_p[2];

  r = MPI_SUCCESS;

  set_root_p[0] = !set;
  set_root_p[1] = irank;

  /* MPI_MINLOC is guaranteed to be return minimum irank */
  r = MPI_Allreduce(MPI_IN_PLACE, set_root_p, 1, MPI_2INT, MPI_MINLOC, comm);
  if (r != MPI_SUCCESS)
    return r;

  *set_root = set_root_p[1];
  return r;
}

/**
 * @brief set_root chooser for bcast, gather and scatter
 * @param set_root Result [out]
 * @param irank The caller's rank
 *
 * 1. If @p set in @p np_root is not NULL, sets @p set_root to be @p np_root.
 * 2. If @p set in @p np_root is NULL, sets @p set_root to be the first rank
 *    that @p set is not NULL.
 */
static int LPTX_particle_set_root_nproot(int *set_root, int irank, int np_root,
                                         const LPTX_particle_set *set,
                                         MPI_Comm comm)
{
  int r;
  LPTX_assert(irank >= 0);
  LPTX_assert(np_root != MPI_ANY_SOURCE);

  r = MPI_SUCCESS;

  *set_root = -1;
  if (set && irank == np_root)
    *set_root = irank;

  r = MPI_Bcast(set_root, 1, MPI_INT, np_root, comm);
  if (r != MPI_SUCCESS)
    return r;

  if (*set_root >= 0)
    return r;

  return LPTX_particle_set_root(set_root, irank, set, comm);
}

/**
 * @brief set_root chooser for allgather and alltoall
 * @param set_root Result [out]
 * @param irank The caller's rank
 *
 * 1. If @p set in all ranks are all non-NULL, sets @p set_root to be -1.
 * 2. If @p set in one or more ranks are NULL, sets @p set_root to be the
 *    first rank that @p set is not NULL.
 */
static int LPTX_particle_set_root_noroot(int *set_root, int irank,
                                         const LPTX_particle_set *set,
                                         MPI_Comm comm)
{
  int r;
  LPTX_assert(irank >= 0);

  r = MPI_SUCCESS;

  if (LPTX_MPI_forall(!!set, comm, &r)) {
    *set_root = -1;
    return r;
  }

  return LPTX_particle_set_root(set_root, irank, set, comm);
}

//---- (remote replicator)

struct LPTX_remote_replicator_data
{
  const LPTX_idtype number_procs;        ///< Number of particle entries
  const LPTX_idtype number_vector;       ///< Number of vectors
  const LPTX_idtype number_data;         ///< Number of entries in data
  const LPTX_idtype *const vector_sizes; ///< Vector sizes to be used (for read)
  LPTX_idtype *const vector_sizes_w; ///< Vector sizes to be used (read write)
  LPTX_idtype *const number_of_particles; ///< Numbers of particles (read write)
  LPTX_idtype data[];                     ///< Data
};
typedef struct LPTX_remote_replicator_data LPTX_remote_replicator_data;

static LPTX_remote_replicator_data *LPTX__remote_replicator_init(
  LPTX_remote_replicator_data *p, LPTX_idtype number_data,
  LPTX_idtype number_procs, LPTX_idtype number_vector,
  const LPTX_idtype *vector_sizes, LPTX_bool copy_vector_sizes)
{
  *(LPTX_idtype *)&p->number_procs = number_procs;
  *(LPTX_idtype *)&p->number_vector = number_vector;
  *(LPTX_idtype *)&p->number_data = number_data;
  *(LPTX_idtype **)&p->number_of_particles = &p->data[0];

  for (LPTX_idtype jj = 0; jj < number_procs; ++jj)
    p->number_of_particles[jj] = 0;

  if (copy_vector_sizes) {
    *(LPTX_idtype **)&p->vector_sizes_w = &p->data[number_procs];
    *(const LPTX_idtype **)&p->vector_sizes = p->vector_sizes_w;
    for (LPTX_idtype jj = 0; jj < number_vector; ++jj)
      p->vector_sizes_w[jj] = vector_sizes ? vector_sizes[jj] : 0;
  } else {
    *(LPTX_idtype **)&p->vector_sizes_w = NULL;
    *(const LPTX_idtype **)&p->vector_sizes = vector_sizes;
  }
  return p;
}

static LPTX_remote_replicator_data *LPTX_remote_replicator_data_new_all(
  LPTX_idtype number_procs, LPTX_idtype number_vector,
  const LPTX_idtype *vector_sizes, LPTX_bool copy_vector_sizes)
{
  LPTX_remote_replicator_data *p;
  LPTX_idtype ns, nb, nd;
  LPTX_assert(number_procs >= 1);
  LPTX_assert(number_vector >= 0);

  ns = sizeof(LPTX_remote_replicator_data);
  nd = sizeof(LPTX_idtype);
  nb = number_procs;
  if (copy_vector_sizes) {
    if (LPTX_s_add_overflow(nb, number_vector, &nb))
      return NULL;
  }
  if (LPTX_s_mul_overflow(nb, nd, &nd))
    return NULL;
  if (LPTX_s_add_overflow(nd, ns, &ns))
    return NULL;

  p = (LPTX_remote_replicator_data *)malloc(ns);
  if (!p)
    return NULL;
  return LPTX__remote_replicator_init(p, nb, number_procs, number_vector,
                                      vector_sizes, copy_vector_sizes);
}

static LPTX_remote_replicator_data *
LPTX_remote_replicator_data_new_nv(LPTX_idtype number_procs,
                                   LPTX_idtype number_vector)
{
  return LPTX_remote_replicator_data_new_all(number_procs, number_vector, NULL,
                                             LPTX_true);
}

static LPTX_remote_replicator_data *
LPTX_remote_replicator_data_new_set(LPTX_idtype number_procs,
                                    const LPTX_particle_set *set,
                                    LPTX_bool copy_vector_sizes)
{
  LPTX_idtype nv;
  const LPTX_idtype *nvs;

  nv = LPTX_particle_set_number_of_vectors(set);
  nvs = LPTX_particle_set_vector_sizes(set);
  return LPTX_remote_replicator_data_new_all(number_procs, nv, nvs,
                                             copy_vector_sizes);
}

static LPTX_remote_replicator_data *
LPTX__remote_replicator_ro_set(LPTX_idtype number_procs,
                               const LPTX_particle_set *set,
                               LPTX_remote_replicator_data *p)
{
  LPTX_idtype nv;
  const LPTX_idtype *nvs;

  nv = LPTX_particle_set_number_of_vectors(set);
  nvs = LPTX_particle_set_vector_sizes(set);
  return LPTX__remote_replicator_init(p, number_procs, number_procs, nv, nvs,
                                      LPTX_false);
}

#define LPTX_remote_replicator_ro_size(number_procs) \
  (((number_procs) * sizeof(LPTX_idtype) +           \
    sizeof(LPTX_remote_replicator_data) - 1) /       \
     sizeof(LPTX_idtype) +                           \
   1)

#define LPTX_remote_replicator_ro_p(number_procs) \
  ((LPTX_remote_replicator_data *)((              \
    LPTX_idtype[LPTX_remote_replicator_ro_size(number_procs)]){0}))

#define LPTX_remote_replicator_ro_set(number_procs, set) \
  LPTX__remote_replicator_ro_set(number_procs, set,      \
                                 LPTX_remote_replicator_ro_p(number_procs))

static void LPTX_remote_replicator_delete(LPTX_remote_replicator_data *p)
{
  free(p);
}

static int LPTX_particle_set_new_from_remote_replicator_data(
  LPTX_particle_set **outp, const LPTX_remote_replicator_data *p)
{
  LPTX_idtype number_particles;
  LPTX_particle_set *set;

  number_particles = 0;
  for (LPTX_idtype jj = 0; jj < p->number_procs; ++jj) {
    if (LPTX_s_add_overflow(number_particles, p->number_of_particles[jj],
                            &number_particles))
      return LPTX_MPI_ERR_OVERFLOW;
  }

  *outp =
    LPTX_particle_set_new(number_particles, p->number_vector, p->vector_sizes);
  return MPI_SUCCESS;
}

//----

static int LPTX_replicate_send(const LPTX_remote_replicator_data *p, int dest,
                               int tag, MPI_Comm comm)
{
  int r;
  LPTX_assert(!!p);
  LPTX_assert(p->number_procs == 1);

  do {
    r = MPI_Send(&p->number_vector, 1, LPTX_MPI_TYPE_ID, dest, tag, comm);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  LPTX_MPI_p2p_sync (dest, tag, comm, &r) {
    /* nop */
  }

  if (r == MPI_SUCCESS) {
    do {
      r =
        MPI_Send(p->number_of_particles, 1, LPTX_MPI_TYPE_ID, dest, tag, comm);
      if (r != MPI_SUCCESS)
        break;

      r = MPI_Send(p->vector_sizes, p->number_vector, LPTX_MPI_TYPE_ID, dest,
                   tag, comm);
      if (r != MPI_SUCCESS)
        break;
    } while (0);
  }

  return r;
}

static int LPTX_replicate_recv(LPTX_remote_replicator_data **outp, int source,
                               int tag, MPI_Comm comm)
{
  int r;
  LPTX_idtype number_vector;
  LPTX_remote_replicator_data *p;
  LPTX_assert(!!outp);

  p = NULL;

  do {
    r = MPI_Recv(&number_vector, 1, LPTX_MPI_TYPE_ID, source, tag, comm,
                 MPI_STATUS_IGNORE);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  LPTX_MPI_p2p_sync (source, tag, comm, &r) {
    p = LPTX_remote_replicator_data_new_nv(1, number_vector);
    if (!p) {
      r = LPTX_MPI_ERR_NO_MEM;
      break;
    }
  }

  if (r == MPI_SUCCESS) {
    do {
      r = MPI_Recv(p->number_of_particles, 1, LPTX_MPI_TYPE_ID, source, tag,
                   comm, MPI_STATUS_IGNORE);
      if (r != MPI_SUCCESS)
        break;

      r = MPI_Recv(p->vector_sizes_w, p->number_vector, LPTX_MPI_TYPE_ID,
                   source, tag, comm, MPI_STATUS_IGNORE);
      if (r != MPI_SUCCESS)
        break;
    } while (0);
  }

  if (r == MPI_SUCCESS) {
    *outp = p;
    p = NULL;
  }

  if (p)
    LPTX_remote_replicator_delete(p);
  return r;
}

int LPTX_particle_set_replicate_send(const LPTX_particle_set *set,
                                     LPTX_idtype number_of_particles, int dest,
                                     int tag, MPI_Comm comm)
{
  LPTX_remote_replicator_data *p;
  int r;

  LPTX_assert(!!set);

  p = LPTX_remote_replicator_ro_set(1, set);
  p->number_of_particles[0] = number_of_particles;

  r = MPI_SUCCESS;

  do {
    r = LPTX_replicate_send(p, dest, tag, comm);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  LPTX_MPI_p2p_sync (dest, tag, comm, &r) {
    /* nop */
  }

  return r;
}

int LPTX_particle_set_replicate_recv(LPTX_particle_set **outset, int source,
                                     int tag, MPI_Comm comm)
{
  int r;
  LPTX_remote_replicator_data *p;

  LPTX_assert(!!outset);

  p = NULL;
  r = MPI_SUCCESS;

  do {
    r = LPTX_replicate_recv(&p, source, tag, comm);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  LPTX_MPI_p2p_sync (source, tag, comm, &r) {
    r = LPTX_particle_set_new_from_remote_replicator_data(outset, p);
    if (r != MPI_SUCCESS)
      break;
  }

  if (p)
    LPTX_remote_replicator_delete(p);
  return r;
}

static int LPTX_replicate_bcast(LPTX_remote_replicator_data **outp,
                                int *irank_out, const LPTX_particle_set *set,
                                LPTX_idtype number_of_particles, int np_root,
                                int set_root, MPI_Comm comm)
{
  LPTX_remote_replicator_data *p;
  int r;
  int irank;
  LPTX_idtype nv;

  LPTX_assert(!!outp);

  p = NULL;
  r = MPI_SUCCESS;
  nv = -1;

  LPTX_MPI_coll_sync (comm, &r) {
    r = MPI_Comm_rank(comm, &irank);
    if (r != MPI_SUCCESS)
      break;
  }

  if (r == MPI_SUCCESS) {
    do {
      if (set_root == MPI_ANY_SOURCE) {
        r = LPTX_particle_set_root_nproot(&set_root, irank, np_root, set, comm);
        if (r != MPI_SUCCESS)
          break;
      }

      if (irank == set_root) {
        LPTX_assert(!!set);
        nv = LPTX_particle_set_number_of_vectors(set);
      }

      if (np_root == set_root) {
        LPTX_idtype npnv[] = {number_of_particles, nv};

        r = MPI_Bcast(npnv, 2, LPTX_MPI_TYPE_ID, np_root, comm);
        if (r != MPI_SUCCESS)
          break;

        number_of_particles = npnv[0];
        nv = npnv[1];
      } else {
        r = MPI_Bcast(&nv, 1, LPTX_MPI_TYPE_ID, set_root, comm);
        if (r != MPI_SUCCESS)
          break;

        r = MPI_Bcast(&number_of_particles, 1, LPTX_MPI_TYPE_ID, np_root, comm);
        if (r != MPI_SUCCESS)
          break;
      }
    } while (0);
  }

  LPTX_MPI_coll_sync (comm, &r) {
    if (irank != set_root) {
      p = LPTX_remote_replicator_data_new_nv(1, nv);
    } else {
      p = LPTX_remote_replicator_data_new_set(1, set, LPTX_true);
    }
    if (!p) {
      r = LPTX_MPI_ERR_NO_MEM;
      break;
    }

    p->number_of_particles[0] = number_of_particles;
  }

  if (r == MPI_SUCCESS) {
    do {
      if (nv <= 0)
        break;

      r = MPI_Bcast(p->vector_sizes_w, nv, LPTX_MPI_TYPE_ID, set_root, comm);
      if (r != MPI_SUCCESS)
        break;
    } while (0);
  }

  if (r == MPI_SUCCESS) {
    *outp = p;
    p = NULL;
    if (irank_out)
      *irank_out = irank;
  }

  if (p)
    LPTX_remote_replicator_delete(p);
  return r;
}

int LPTX_particle_set_replicate_bcast(LPTX_particle_set **outset,
                                      const LPTX_particle_set *set,
                                      LPTX_idtype number_of_particles,
                                      int np_root, int set_root,
                                      LPTX_bool allocate_in_root, MPI_Comm comm)
{
  LPTX_particle_set *oset;
  LPTX_remote_replicator_data *p;
  int r;
  int irank;

  LPTX_assert(!!outset);

  oset = NULL;
  p = NULL;

  r = LPTX_replicate_bcast(&p, &irank, set, number_of_particles, np_root,
                           set_root, comm);

  LPTX_MPI_coll_sync (comm, &r) {
    if (irank != set_root || allocate_in_root) {
      r = LPTX_particle_set_new_from_remote_replicator_data(&oset, p);
      if (r != MPI_SUCCESS)
        break;
    }
  }

  if (r == MPI_SUCCESS && oset) {
    *outset = oset;
    oset = NULL;
  }

  if (oset)
    LPTX_particle_set_delete(oset);
  if (p)
    LPTX_remote_replicator_delete(p);
  return r;
}

static int LPTX_replicate_gather(LPTX_remote_replicator_data **outp,
                                 int *irank_out, int *nproc_out,
                                 const LPTX_particle_set *set,
                                 LPTX_idtype number_of_particles,
                                 int gather_root, int set_root, MPI_Comm comm)
{
  LPTX_remote_replicator_data *p;
  LPTX_idtype nv;
  int irank;
  int nproc;
  int r;

  p = NULL;
  r = MPI_SUCCESS;

  LPTX_MPI_coll_sync (comm, &r) {
    r = MPI_Comm_rank(comm, &irank);
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Comm_size(comm, &nproc);
    if (r != MPI_SUCCESS)
      break;
  }

  if (r == MPI_SUCCESS) {
    do {
      if (set_root == MPI_ANY_SOURCE) {
        r = LPTX_particle_set_root_nproot(&set_root, irank, gather_root, set,
                                          comm);
        if (r != MPI_SUCCESS)
          break;
      }

      nv = -1;
      if (irank == set_root)
        nv = LPTX_particle_set_number_of_vectors(set);

      if (gather_root != set_root) {
        if (irank == set_root) {
          r = MPI_Send(&nv, 1, LPTX_MPI_TYPE_ID, gather_root, 0, comm);
        } else if (irank == gather_root) {
          r = MPI_Recv(&nv, 1, LPTX_MPI_TYPE_ID, set_root, 0, comm,
                       MPI_STATUS_IGNORE);
        }
        if (!LPTX_MPI_forall(r == MPI_SUCCESS, comm, &r)) {
          if (r != MPI_SUCCESS)
            r = LPTX_MPI_ERR_REMOTE;
          break;
        }
      }
    } while (0);
  }

  LPTX_MPI_coll_sync (comm, &r) {
    if (irank != gather_root)
      break;

    if (gather_root == set_root) {
      p = LPTX_remote_replicator_data_new_set(nproc, set, LPTX_false);
    } else {
      p = LPTX_remote_replicator_data_new_nv(nproc, nv);
    }
    if (!p) {
      r = LPTX_MPI_ERR_NO_MEM;
      break;
    }
  }

  if (r == MPI_SUCCESS) {
    do {
      if (gather_root == set_root)
        break;

      if (irank == set_root) {
        r = MPI_Send(set->vector_sizes, nv, LPTX_MPI_TYPE_ID, gather_root, 0,
                     comm);
      } else if (irank == gather_root) {
        r = MPI_Recv(p->vector_sizes_w, nv, LPTX_MPI_TYPE_ID, set_root, 0, comm,
                     MPI_STATUS_IGNORE);
      }
      if (!LPTX_MPI_forall(r == MPI_SUCCESS, comm, &r)) {
        if (r != MPI_SUCCESS)
          r = LPTX_MPI_ERR_REMOTE;
        break;
      }
    } while (0);
  }

  if (r == MPI_SUCCESS) {
    do {
      void *recvp = p ? p->number_of_particles : NULL;
      r = MPI_Gather(&number_of_particles, 1, LPTX_MPI_TYPE_ID, recvp, 1,
                     LPTX_MPI_TYPE_ID, gather_root, comm);
      if (r != MPI_SUCCESS)
        break;
    } while (0);
  }

  if (r == MPI_SUCCESS) {
    *outp = p;
    p = NULL;

    if (irank_out)
      *irank_out = irank;
    if (nproc_out)
      *nproc_out = nproc;
  }

  if (p)
    LPTX_remote_replicator_delete(p);
  return r;
}

int LPTX_particle_set_replicate_gather(LPTX_particle_set **outset,
                                       const LPTX_particle_set *set,
                                       LPTX_idtype number_of_particles,
                                       int gather_root, int set_root,
                                       MPI_Comm comm)
{
  LPTX_particle_set *oset;
  LPTX_remote_replicator_data *p;
  int irank;
  int r;

  oset = NULL;
  p = NULL;
  r = MPI_SUCCESS;

  r = LPTX_replicate_gather(&p, &irank, NULL, set, number_of_particles,
                            gather_root, set_root, comm);

  LPTX_MPI_coll_sync (comm, &r) {
    if (irank != gather_root)
      break;

    r = LPTX_particle_set_new_from_remote_replicator_data(&oset, p);
    if (r != MPI_SUCCESS)
      break;
  }

  if (oset && r == MPI_SUCCESS) {
    *outset = oset;
    oset = NULL;
  }

  if (oset)
    LPTX_particle_set_delete(oset);
  if (p)
    LPTX_remote_replicator_delete(p);

  return r;
}

static int LPTX_replicate_allgather(LPTX_remote_replicator_data **outp,
                                    int *irank_out,
                                    const LPTX_particle_set *set,
                                    LPTX_idtype number_of_particles,
                                    int set_root, MPI_Comm comm)
{
  LPTX_remote_replicator_data *p;
  LPTX_idtype nv;
  int r;
  int irank;
  int nproc;

  p = NULL;
  r = MPI_SUCCESS;
  nv = -1;

  LPTX_MPI_coll_sync (comm, &r) {
    r = MPI_Comm_rank(comm, &irank);
    if (r == MPI_SUCCESS)
      break;

    r = MPI_Comm_size(comm, &nproc);
    if (r != MPI_SUCCESS)
      break;
  }

  if (r == MPI_SUCCESS) {
    do {
      if (set_root == MPI_ANY_SOURCE) {
        r = LPTX_particle_set_root_noroot(&set_root, irank, set, comm);
        if (r != MPI_SUCCESS)
          break;
      }

      if (set_root < 0)
        break;

      nv = -1;
      if (irank == set_root)
        nv = LPTX_particle_set_number_of_vectors(set);

      r = MPI_Bcast(&nv, 1, LPTX_MPI_TYPE_ID, set_root, comm);
      if (r != MPI_SUCCESS)
        break;

      LPTX_assert(set_root >= 0);
    } while (0);
  }

  LPTX_MPI_coll_sync (comm, &r) {
    r = MPI_Comm_size(comm, &nproc);
    if (r != MPI_SUCCESS)
      break;

    if (set_root < 0) {
      p = LPTX_remote_replicator_data_new_set(nproc, set, LPTX_false);
    } else if (irank == set_root) {
      p = LPTX_remote_replicator_data_new_set(nproc, set, LPTX_true);
    } else {
      p = LPTX_remote_replicator_data_new_nv(nproc, nv);
    }
    if (!p) {
      r = LPTX_MPI_ERR_NO_MEM;
      break;
    }
  }

  if (r == MPI_SUCCESS) {
    do {
      r = MPI_Allgather(&number_of_particles, 1, LPTX_MPI_TYPE_ID,
                        p->number_of_particles, 1, LPTX_MPI_TYPE_ID, comm);
      if (r != MPI_SUCCESS)
        break;

      if (set_root < 0)
        break;

      r = MPI_Bcast(p->vector_sizes_w, nv, LPTX_MPI_TYPE_ID, set_root, comm);
      if (r != MPI_SUCCESS)
        break;
    } while (0);
  }

  if (r == MPI_SUCCESS) {
    *outp = p;
    p = NULL;

    if (irank_out)
      *irank_out = irank;
  }

  if (p)
    LPTX_remote_replicator_delete(p);
  return r;
}

int LPTX_particle_set_replicate_allgather(LPTX_particle_set **outset,
                                          const LPTX_particle_set *set,
                                          LPTX_idtype number_of_particles,
                                          int set_root, MPI_Comm comm)
{
  LPTX_particle_set *oset;
  LPTX_remote_replicator_data *p;
  int r;

  oset = NULL;
  p = NULL;
  r = MPI_SUCCESS;

  r = LPTX_replicate_allgather(&p, NULL, set, number_of_particles, set_root,
                               comm);

  LPTX_MPI_coll_sync (comm, &r) {
    r = LPTX_particle_set_new_from_remote_replicator_data(&oset, p);
    if (r != MPI_SUCCESS)
      break;
  }

  if (r == MPI_SUCCESS) {
    *outset = oset;
    oset = NULL;
  }

  if (oset)
    LPTX_particle_set_delete(oset);
  if (p)
    LPTX_remote_replicator_delete(p);
  return r;
}

static int LPTX_replicate_scatter(LPTX_remote_replicator_data **outp,
                                  int *irank_out, const LPTX_particle_set *set,
                                  const LPTX_idtype *numbers_of_particles,
                                  int scatter_root, int set_root, MPI_Comm comm)
{
  LPTX_remote_replicator_data *p;
  LPTX_idtype nv;
  LPTX_idtype np;
  int r;
  int irank;

  p = NULL;
  r = MPI_SUCCESS;
  nv = -1;
  irank = -1;

  LPTX_MPI_coll_sync (comm, &r) {
    r = MPI_Comm_rank(comm, &irank);
    if (r != MPI_SUCCESS)
      break;
  }

  if (r == MPI_SUCCESS) {
    do {
      if (set_root == MPI_ANY_SOURCE) {
        r = LPTX_particle_set_root_nproot(&set_root, irank, scatter_root, set,
                                          comm);
        if (r != MPI_SUCCESS)
          break;
      }

      r = MPI_Scatter(numbers_of_particles, 1, LPTX_MPI_TYPE_ID, &np, 1,
                      LPTX_MPI_TYPE_ID, scatter_root, comm);
      if (r != MPI_SUCCESS)
        break;

      if (irank == set_root)
        nv = LPTX_particle_set_number_of_vectors(set);

      r = MPI_Bcast(&nv, 1, LPTX_MPI_TYPE_ID, set_root, comm);
      if (r != MPI_SUCCESS)
        break;
    } while (0);
  }

  LPTX_MPI_coll_sync (comm, &r) {
    if (irank == set_root) {
      p = LPTX_remote_replicator_data_new_set(1, set, LPTX_true);
    } else {
      p = LPTX_remote_replicator_data_new_nv(1, nv);
    }
    if (!p) {
      r = LPTX_MPI_ERR_NO_MEM;
      break;
    }

    p->number_of_particles[0] = np;
  }

  if (r == MPI_SUCCESS) {
    do {
      r = MPI_Bcast(p->vector_sizes_w, nv, LPTX_MPI_TYPE_ID, set_root, comm);
      if (r != MPI_SUCCESS)
        break;
    } while (0);
  }

  if (r == MPI_SUCCESS) {
    *outp = p;
    p = NULL;

    if (irank_out)
      *irank_out = irank;
  }

  if (p)
    LPTX_remote_replicator_delete(p);
  return r;
}

int LPTX_particle_set_replicate_scatter(LPTX_particle_set **outset,
                                        const LPTX_particle_set *set,
                                        const LPTX_idtype *numbers_of_particles,
                                        int scatter_root, int set_root,
                                        MPI_Comm comm)
{
  LPTX_particle_set *oset;
  LPTX_remote_replicator_data *p;
  int irank;
  int r;

  oset = NULL;
  p = NULL;
  r = MPI_SUCCESS;

  r = LPTX_replicate_scatter(&p, &irank, set, numbers_of_particles,
                             scatter_root, set_root, comm);

  LPTX_MPI_coll_sync (comm, &r) {
    r = LPTX_particle_set_new_from_remote_replicator_data(&oset, p);
    if (r != MPI_SUCCESS)
      break;
  }

  if (r == MPI_SUCCESS) {
    *outset = oset;
    oset = NULL;
  }

  if (oset)
    LPTX_particle_set_delete(oset);
  if (p)
    LPTX_remote_replicator_delete(p);
  return r;
}

static int LPTX_replicate_alltoall(LPTX_remote_replicator_data **outp,
                                   int *irank_out, const LPTX_particle_set *set,
                                   const LPTX_idtype *numbers_of_particles,
                                   int set_root, MPI_Comm comm)
{
  LPTX_remote_replicator_data *p;
  LPTX_idtype nv;
  int r;
  int nproc;
  int irank;

  p = NULL;
  irank = -1;
  r = MPI_SUCCESS;
  nv = -1;

  LPTX_MPI_coll_sync (comm, &r) {
    r = MPI_Comm_rank(comm, &irank);
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Comm_size(comm, &nproc);
    if (r != MPI_SUCCESS)
      break;
  }

  if (r == MPI_SUCCESS) {
    do {
      if (set_root == MPI_ANY_SOURCE) {
        r = LPTX_particle_set_root_noroot(&set_root, irank, set, comm);
        if (r != MPI_SUCCESS)
          break;
      }

      if (set_root < 0)
        break;

      if (irank == set_root)
        nv = LPTX_particle_set_number_of_vectors(set);

      r = MPI_Bcast(&nv, 1, LPTX_MPI_TYPE_ID, set_root, comm);
      if (r != MPI_SUCCESS)
        break;
    } while (0);
  }

  LPTX_MPI_coll_sync (comm, &r) {
    if (set_root < 0) {
      p = LPTX_remote_replicator_data_new_set(nproc, set, LPTX_false);
    } else if (irank == set_root) {
      p = LPTX_remote_replicator_data_new_set(nproc, set, LPTX_true);
    } else {
      p = LPTX_remote_replicator_data_new_nv(nproc, nv);
    }
    if (!p) {
      r = LPTX_MPI_ERR_NO_MEM;
      break;
    }
  }

  if (r == MPI_SUCCESS) {
    do {
      r = MPI_Alltoall(numbers_of_particles, 1, LPTX_MPI_TYPE_ID,
                       p->number_of_particles, 1, LPTX_MPI_TYPE_ID, comm);
      if (r != MPI_SUCCESS)
        break;

      if (set_root < 0)
        break;

      r = MPI_Bcast(p->vector_sizes_w, nv, LPTX_MPI_TYPE_ID, set_root, comm);
      if (r != MPI_SUCCESS)
        break;
    } while (0);
  }

  if (r == MPI_SUCCESS) {
    *outp = p;
    p = NULL;

    if (irank_out)
      *irank_out = irank;
  }

  if (p)
    LPTX_remote_replicator_delete(p);
  return r;
}

int LPTX_particle_set_replicate_alltoall(
  LPTX_particle_set **outset, const LPTX_particle_set *set,
  const LPTX_idtype *numbers_of_particles, int set_root, MPI_Comm comm)
{
  LPTX_particle_set *oset;
  LPTX_remote_replicator_data *p;
  int r;
  int irank;

  oset = NULL;
  p = NULL;
  r = MPI_SUCCESS;

  r = LPTX_replicate_alltoall(&p, &irank, set, numbers_of_particles, set_root,
                              comm);

  LPTX_MPI_coll_sync (comm, &r) {
    r = LPTX_particle_set_new_from_remote_replicator_data(&oset, p);
    if (r != MPI_SUCCESS)
      break;
  }

  if (r == MPI_SUCCESS) {
    *outset = oset;
    oset = NULL;
  }

  if (oset)
    LPTX_particle_set_delete(oset);
  if (p)
    LPTX_remote_replicator_delete(p);
  return r;
}

//---

/**
 * Creates for counts and displacements for p->number_of_particles
 */
static int LPTX_create_counts_and_displacements(LPTX_remote_replicator_data *p,
                                                int **counts,
                                                int **displacements)
{
  int nproc;
  int *cnts;
  int *displs;
  int r;

  LPTX_assert(!!p);
  LPTX_assert(p->number_procs > 0 && p->number_procs <= INT_MAX);

  nproc = p->number_procs;
  cnts = NULL;
  displs = NULL;
  r = MPI_SUCCESS;

  do {
    cnts = (int *)malloc(sizeof(int) * nproc);
    if (!cnts) {
      r = LPTX_MPI_ERR_NO_MEM;
      break;
    }

    displs = (int *)malloc(sizeof(int) * nproc);
    if (!displs) {
      r = LPTX_MPI_ERR_NO_MEM;
      break;
    }

    for (int i = 0; i < nproc; ++i) {
      LPTX_idtype np;

      np = p->number_of_particles[i];
      if (np < 0 || np > INT_MAX) {
        r = LPTX_MPI_ERR_OVERFLOW;
        break;
      }

      cnts[i] = np;
      if (i > 0) {
        if (LPTX_i_add_overflow(displs[i - 1], cnts[i - 1], &displs[i])) {
          r = LPTX_MPI_ERR_OVERFLOW;
          break;
        }
      } else {
        displs[i] = 0;
      }
    }
  } while (0);

  if (r == MPI_SUCCESS) {
    *counts = cnts;
    *displacements = displs;
    cnts = NULL;
    displs = NULL;
  }

  if (cnts)
    free(cnts);
  if (displs)
    free(displs);

  return r;
}

//---

static LPTX_bool LPTX_particle_is_collectable_base(const LPTX_particle_set *set,
                                                   LPTX_particle_set *other)
{
  if (set->number_vectors != other->number_vectors)
    return LPTX_false;
  if (set->number_data != other->number_data)
    return LPTX_false;
  return LPTX_true;
}

static LPTX_bool
LPTX_particle_is_collectable_sizes(const LPTX_particle_set *set,
                                   const LPTX_idtype *other_sizes)
{
  for (LPTX_idtype jj = 0; jj < set->number_vectors; ++jj)
    if (set->vector_sizes[jj] != other_sizes[jj])
      return LPTX_false;
  return LPTX_true;
}

LPTX_bool LPTX_particle_set_is_communicatable(const LPTX_particle_set *set,
                                              int peer, int tag, MPI_Comm comm,
                                              int *ier)
{
  int r;
  LPTX_bool res;
  MPI_Datatype stype;
  LPTX_particle_set tester;
  LPTX_idtype *sizes;

  sizes = NULL;
  stype = MPI_DATATYPE_NULL;
  res = LPTX_true;
  r = MPI_SUCCESS;

  do {
    r = LPTX_particle_set_mpi_type(&stype);
    if (!LPTX_MPI_p2pall(set && r == MPI_SUCCESS, peer, tag, comm, &r)) {
      res = LPTX_false;
      break;
    }

    r = MPI_Sendrecv(set, 1, stype, peer, tag, &tester, 1, stype, peer, tag,
                     comm, MPI_STATUS_IGNORE);
    if (r != MPI_SUCCESS) {
      res = LPTX_false;
      break;
    }

    res = LPTX_particle_is_collectable_base(set, &tester);
    if (!res)
      break;

    sizes = (LPTX_idtype *)calloc(set->number_vectors, sizeof(LPTX_idtype));
    if (!LPTX_MPI_p2pall(!!sizes, peer, tag, comm, &r)) {
      if (r == MPI_SUCCESS)
        r = LPTX_MPI_ERR_NO_MEM;
      res = LPTX_false;
      break;
    }

    r = MPI_Sendrecv(set->vector_sizes, set->number_vectors, LPTX_MPI_TYPE_ID,
                     peer, tag, sizes, set->number_vectors, LPTX_MPI_TYPE_ID,
                     peer, tag, comm, MPI_STATUS_IGNORE);
    if (r != MPI_SUCCESS) {
      res = LPTX_false;
      break;
    }

    res = LPTX_particle_is_collectable_sizes(set, sizes);
    if (!res)
      break;
  } while (0);

  if (ier)
    *ier = r;
  if (sizes)
    free(sizes);
  if (stype != MPI_DATATYPE_NULL)
    MPI_Type_free(&stype);

  return res;
}

LPTX_bool LPTX_particle_set_is_collectable(const LPTX_particle_set *set,
                                           MPI_Comm comm, int *ier)
{
  int r;
  LPTX_bool res;
  MPI_Datatype stype;
  LPTX_particle_set tester;
  LPTX_idtype *sizes;
  int rank, nproc;
  int source, dest;

  sizes = NULL;
  stype = MPI_DATATYPE_NULL;
  res = LPTX_true;
  r = MPI_SUCCESS;

  rank = -1;
  nproc = -1;

  MPI_Comm_rank(comm, &rank);
  MPI_Comm_size(comm, &nproc);
  LPTX_assert(rank >= 0);
  LPTX_assert(nproc > 0);

  source = rank - 1;
  if (source < 0)
    source = nproc - 1;

  dest = rank + 1;
  if (dest >= nproc)
    dest = 0;

  do {
    r = LPTX_particle_set_mpi_type(&stype);
    if (!LPTX_MPI_forall(set && r == MPI_SUCCESS, comm, &r)) {
      res = LPTX_false;
      break;
    }

    r = MPI_Sendrecv(set, 1, stype, dest, 1, &tester, 1, stype, source, 1, comm,
                     MPI_STATUS_IGNORE);
    if (!LPTX_MPI_forall(r == MPI_SUCCESS, comm, &r)) {
      if (r == MPI_SUCCESS)
        r = LPTX_MPI_ERR_REMOTE;
      res = LPTX_false;
      break;
    }

    res = LPTX_particle_is_collectable_base(set, &tester);
    if (!LPTX_MPI_forall(res, comm, &r)) {
      res = LPTX_false;
      break;
    }

    sizes = (LPTX_idtype *)calloc(set->number_vectors, sizeof(LPTX_idtype));
    if (!LPTX_MPI_forall(!!sizes, comm, &r)) {
      if (r == MPI_SUCCESS)
        r = LPTX_MPI_ERR_NO_MEM;
      res = LPTX_false;
      break;
    }

    r = MPI_Sendrecv(set->vector_sizes, set->number_vectors, LPTX_MPI_TYPE_ID,
                     dest, 1, sizes, set->number_vectors, LPTX_MPI_TYPE_ID,
                     source, 1, comm, MPI_STATUS_IGNORE);
    if (!LPTX_MPI_forall(r == MPI_SUCCESS, comm, &r)) {
      if (r == MPI_SUCCESS)
        r = LPTX_MPI_ERR_REMOTE;
      res = LPTX_false;
      break;
    }

    res = LPTX_particle_is_collectable_sizes(set, sizes);
    if (!LPTX_MPI_forall(res, comm, &r)) {
      res = LPTX_false;
      break;
    }
  } while (0);

  if (ier)
    *ier = r;
  if (sizes)
    free(sizes);
  if (stype != MPI_DATATYPE_NULL)
    MPI_Type_free(&stype);
  return res;
}

//---

int LPTX_particle_send(const LPTX_particle_set *set, LPTX_idtype start,
                       int count, int dest, int tag, MPI_Comm comm)
{
  MPI_Datatype ctype;
  int r;

  LPTX_assert(!!set);
  LPTX_assert(start >= 0);
  LPTX_assert(count >= 0);
  LPTX_assert(count < set->number_particles);
  LPTX_assert(start < set->number_particles - count);

  r = MPI_SUCCESS;
  if (!LPTX_particle_set_is_communicatable(set, dest, tag, comm, &r)) {
    if (r == MPI_SUCCESS)
      r = LPTX_MPI_ERR_VECTOR_SIZE_MISMATCH;
    return r;
  }

  ctype = MPI_DATATYPE_NULL;

  do {
    r = LPTX_composite_particle_range_type(&ctype, set, start, count);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  if (!LPTX_MPI_p2pall(r == MPI_SUCCESS, dest, tag, comm, &r)) {
    if (r == MPI_SUCCESS)
      r = LPTX_MPI_ERR_REMOTE;
  }

  do {
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Send(set, 1, ctype, dest, tag, comm);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  if (ctype != MPI_DATATYPE_NULL)
    MPI_Type_free(&ctype);
  return r;
}

int LPTX_particle_recv(LPTX_particle_set *set, LPTX_idtype start, int count,
                       int source, int tag, MPI_Comm comm)
{
  MPI_Datatype ctype;
  int r;

  LPTX_assert(!!set);
  LPTX_assert(start >= 0);
  LPTX_assert(count >= 0);
  LPTX_assert(count < set->number_particles);
  LPTX_assert(start < set->number_particles - count);

  r = MPI_SUCCESS;
  if (!LPTX_particle_set_is_communicatable(set, source, tag, comm, &r)) {
    if (r == MPI_SUCCESS)
      r = LPTX_MPI_ERR_VECTOR_SIZE_MISMATCH;
    return r;
  }

  ctype = MPI_DATATYPE_NULL;

  do {
    r = LPTX_composite_particle_range_type(&ctype, set, start, count);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  if (!LPTX_MPI_p2pall(r == MPI_SUCCESS, source, tag, comm, &r)) {
    if (r == MPI_SUCCESS)
      r = LPTX_MPI_ERR_REMOTE;
  }

  do {
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Recv(set, 1, ctype, source, tag, comm, MPI_STATUS_IGNORE);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  if (ctype != MPI_DATATYPE_NULL)
    MPI_Type_free(&ctype);
  return r;
}

//---

int LPTX_particle_sendv(const LPTX_particle_set *set,
                        LPTX_idtype size_of_indices, const LPTX_idtype *indices,
                        int dest, int tag, MPI_Comm comm)
{
  MPI_Datatype ctype;
  int r;

  LPTX_assert(!!set);

  r = MPI_SUCCESS;
  if (!LPTX_particle_set_is_communicatable(set, dest, tag, comm, &r)) {
    if (r == MPI_SUCCESS)
      r = LPTX_MPI_ERR_VECTOR_SIZE_MISMATCH;
    return r;
  }

  ctype = MPI_DATATYPE_NULL;

  do {
    r =
      LPTX_composite_particle_index_type(&ctype, set, size_of_indices, indices);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  if (!LPTX_MPI_p2pall(r == MPI_SUCCESS, dest, tag, comm, &r)) {
    if (r == MPI_SUCCESS)
      r = LPTX_MPI_ERR_REMOTE;
    return r;
  }

  do {
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Send(set, 1, ctype, dest, tag, comm);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  if (ctype != MPI_DATATYPE_NULL)
    MPI_Type_free(&ctype);
  return r;
}

int LPTX_particle_recvv(LPTX_particle_set *set, LPTX_idtype size_of_indices,
                        const LPTX_idtype *indices, int source, int tag,
                        MPI_Comm comm)
{
  MPI_Datatype ctype;
  int r;

  LPTX_assert(!!set);

  r = MPI_SUCCESS;
  if (!LPTX_particle_set_is_communicatable(set, source, tag, comm, &r)) {
    if (r == MPI_SUCCESS)
      r = LPTX_MPI_ERR_VECTOR_SIZE_MISMATCH;
    return r;
  }

  ctype = MPI_DATATYPE_NULL;

  do {
    r =
      LPTX_composite_particle_index_type(&ctype, set, size_of_indices, indices);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  if (!LPTX_MPI_p2pall(r == MPI_SUCCESS, source, tag, comm, &r)) {
    if (r == MPI_SUCCESS)
      r = LPTX_MPI_ERR_REMOTE;
    return r;
  }

  do {
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Recv(set, 1, ctype, source, tag, comm, MPI_STATUS_IGNORE);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  if (ctype != MPI_DATATYPE_NULL)
    MPI_Type_free(&ctype);
  return r;
}

//----

/**
 * @return number of indices
 * @retval -1 Allocation failed
 * @retval  0 No indices to send
 *
 * @p outindices will be allocated when returning positive number.
 */
static LPTX_idtype
LPTX_build_send_indices(LPTX_idtype **outindices, LPTX_particle_set *set,
                        LPTX_bool skip_unused, LPTX_bool send_sorted,
                        LPTX_cb_particle_if *cond, void *cond_arg)
{
  LPTX_idtype np;
  LPTX_idtype jt;
  LPTX_idtype *indices;
  LPTX_idtype *work;

  np = set->number_particles;
  if (np <= 0)
    return 0;

  indices = (LPTX_idtype *)malloc(sizeof(LPTX_idtype) * np);
  if (!indices)
    return -1;

  jt = -1;
  work = NULL;

#ifdef _OPENMP
#pragma omp parallel if (np > LPTX_omp_small_threshold)
#endif
  {
    LPTX_idtype is, ie, id, jb;
    int nt, it;
    LPTX_idtype *wk;
    LPTX_omp_distribute(&is, &ie, &nt, &it, 0, np);

    do {
      if (nt > 1) {
#ifdef _OPENMP
#pragma omp single
#endif
        work = (LPTX_idtype *)malloc(sizeof(LPTX_idtype) * (np + nt));
#ifdef _OPENMP
#pragma omp barrier
#endif
        if (!work)
          break;
        wk = work;
      } else {
        wk = indices;
      }

      id = is;
      for (LPTX_idtype jj = is; jj < ie; ++jj) {
        const LPTX_particle_data *p;
        LPTX_idtype idx;
        p = send_sorted ? set->sorted[jj] : &set->particles[jj];
        idx = p - set->particles;

        if (skip_unused) {
          if (!LPTX_particle_is_used(&p->base))
            idx = -1;
        }
        if (idx >= 0 && cond) {
          if (!cond(p, cond_arg))
            idx = -1;
        }
        if (idx >= 0)
          wk[id++] = idx;
      }
      if (nt <= 1) {
        jt = id;
        break;
      }

      wk[np + it] = id;

#ifdef _OPENMP
#pragma omp barrier
#endif

      jb = 0;
      for (int itt = 0; itt < it; ++itt)
        jb += wk[np + itt];

      for (LPTX_idtype jj = is; jj < id; ++jj)
        indices[jj - is + jb] = wk[jj];

      if (it == 0) {
        jt = 0;
        for (int itt = 0; itt < nt; ++itt)
          jt += wk[np + itt];
      }
    } while (0);
  }

  LPTX_assert(jt >= 0);

  if (jt > 0) {
    *outindices = indices;
  } else {
    free(indices);
  }
  if (work)
    free(work);
  return jt;
}

//---

int LPTX_particle_set_send(LPTX_particle_set *set, int dest, int tag,
                           MPI_Comm comm)
{
  MPI_Datatype ctype;
  int r;

  LPTX_assert(!!set);
  LPTX_assert(set->number_particles >= 0);
  LPTX_assert(set->number_particles <= INT_MAX); /* not supported yet */

  ctype = MPI_DATATYPE_NULL;
  r = MPI_SUCCESS;

  LPTX_MPI_p2p_sync (dest, tag, comm, &r) {
    /* nop */
  }

  if (r == MPI_SUCCESS) {
    do {
      r = LPTX_particle_set_replicate_send(set, set->number_particles, dest,
                                           tag, comm);
      if (r != MPI_SUCCESS)
        break;
    } while (0);
  }

  LPTX_MPI_p2p_sync (dest, tag, comm, &r) {
    if (set->number_particles <= 0)
      break;

    r =
      LPTX_composite_particle_range_type(&ctype, set, 0, set->number_particles);
    if (r != MPI_SUCCESS)
      break;
  }

  if (r == MPI_SUCCESS) {
    do {
      if (set->number_particles <= 0)
        break;

      r = MPI_Send(set, 1, ctype, dest, tag, comm);
      if (r != MPI_SUCCESS)
        break;
    } while (0);
  }

  if (ctype != MPI_DATATYPE_NULL)
    MPI_Type_free(&ctype);
  return r;
}

int LPTX_particle_set_sendc(LPTX_particle_set *set, LPTX_bool skip_unused,
                            LPTX_bool send_sorted, LPTX_cb_particle_if *cond,
                            void *cond_arg, int dest, int tag, MPI_Comm comm)
{
  MPI_Datatype ctype;
  LPTX_idtype size_of_indices;
  LPTX_idtype *indices;
  int r, sr;

  LPTX_assert(!!set);
  LPTX_assert(set->number_particles >= 0);

  ctype = MPI_DATATYPE_NULL;
  indices = NULL;
  r = MPI_SUCCESS;

  LPTX_MPI_p2p_sync (dest, tag, comm, &r) {
    size_of_indices = LPTX_build_send_indices(&indices, set, skip_unused,
                                              send_sorted, cond, cond_arg);
    if (size_of_indices < 0) {
      r = LPTX_MPI_ERR_NO_MEM;
      break;
    }
    if (size_of_indices <= 0)
      indices = NULL;
  }

  if (r == MPI_SUCCESS) {
    do {
      r =
        LPTX_particle_set_replicate_send(set, size_of_indices, dest, tag, comm);
      if (r != MPI_SUCCESS)
        break;
    } while (0);
  }

  LPTX_MPI_p2p_sync (dest, tag, comm, &r) {
    if (size_of_indices <= 0)
      break;

    r =
      LPTX_composite_particle_index_type(&ctype, set, size_of_indices, indices);
    if (r != MPI_SUCCESS)
      break;
  }

  if (r == MPI_SUCCESS) {
    do {
      if (size_of_indices > 0) {
        r = MPI_Send(set, 1, ctype, dest, tag, comm);
        if (r != MPI_SUCCESS)
          break;
      }
    } while (0);
  }

  if (indices)
    free(indices);
  if (ctype != MPI_DATATYPE_NULL)
    MPI_Type_free(&ctype);
  return r;
}

int LPTX_particle_set_sendv(LPTX_particle_set *set, LPTX_idtype size_of_indices,
                            LPTX_idtype *indices, int dest, int tag,
                            MPI_Comm comm)
{
  MPI_Datatype ctype;
  int r;

  LPTX_assert(!!set);
  LPTX_assert(set->number_particles >= 0);
  LPTX_assert(size_of_indices >= 0);

  ctype = MPI_DATATYPE_NULL;
  r = MPI_SUCCESS;

  LPTX_MPI_p2p_sync (dest, tag, comm, &r) {
    /* nop */
  }

  if (r == MPI_SUCCESS) {
    do {
      r =
        LPTX_particle_set_replicate_send(set, size_of_indices, dest, tag, comm);
      if (r != MPI_SUCCESS)
        break;
    } while (0);
  }

  LPTX_MPI_p2p_sync (dest, tag, comm, &r) {
    if (size_of_indices <= 0)
      break;

    r =
      LPTX_composite_particle_index_type(&ctype, set, size_of_indices, indices);
    if (r != MPI_SUCCESS)
      break;
  }

  if (r == MPI_SUCCESS) {
    do {
      if (size_of_indices > 0) {
        r = MPI_Send(set, 1, ctype, dest, tag, comm);
        if (r != MPI_SUCCESS)
          break;
      }
    } while (0);
  }

  if (indices)
    free(indices);
  if (ctype == MPI_DATATYPE_NULL)
    MPI_Type_free(&ctype);
  return r;
}

int LPTX_particle_set_recv(LPTX_particle_set **set, int source, int tag,
                           MPI_Comm comm)
{
  MPI_Datatype ctype;
  LPTX_particle_set *oset;
  LPTX_idtype np;
  int r;

  LPTX_assert(!!set);

  ctype = MPI_DATATYPE_NULL;
  oset = NULL;
  r = MPI_SUCCESS;
  np = 0;

  LPTX_MPI_p2p_sync (source, tag, comm, &r) {
    /* nop */
  }

  if (r == MPI_SUCCESS) {
    do {
      r = LPTX_particle_set_replicate_recv(&oset, source, tag, comm);
      if (r != MPI_SUCCESS)
        break;
    } while (0);
  }

  LPTX_MPI_p2p_sync (source, tag, comm, &r) {
    if (oset)
      np = LPTX_particle_set_number_of_particles(oset);

    if (np > 0) {
      r = LPTX_composite_particle_range_type(&ctype, oset, 0, np);
      if (r != MPI_SUCCESS)
        break;
    }
  }

  if (r == MPI_SUCCESS) {
    do {
      if (np > 0) {
        r = MPI_Recv(oset, 1, ctype, source, tag, comm, MPI_STATUS_IGNORE);
        if (r != MPI_SUCCESS)
          break;
      }
    } while (0);
  }

  if (r == MPI_SUCCESS) {
    *set = oset;
    oset = NULL;
  }

  if (oset)
    LPTX_particle_set_delete(oset);
  if (ctype != MPI_DATATYPE_NULL)
    MPI_Type_free(&ctype);
  return r;
}

int LPTX_particle_set_gather(LPTX_particle_set **out, LPTX_particle_set *inp,
                             int root, MPI_Comm comm)
{
  LPTX_remote_replicator_data *p;
  LPTX_particle_set *oset;
  MPI_Datatype btype, vtype, dtype;
  int *counts;
  int *displacements;
  LPTX_idtype np;
  LPTX_idtype nv, nd;
  int irank;
  int nproc;
  int r;

  p = NULL;
  oset = NULL;
  btype = MPI_DATATYPE_NULL;
  vtype = MPI_DATATYPE_NULL;
  dtype = MPI_DATATYPE_NULL;
  counts = NULL;
  displacements = NULL;
  nproc = -1;
  irank = -1;
  nv = 0;
  nd = 0;
  r = MPI_SUCCESS;

  np = 0;
  if (inp)
    np = LPTX_particle_set_number_of_particles(inp);

  r = LPTX_replicate_gather(&p, &irank, &nproc, inp, np, root, MPI_ANY_SOURCE,
                            comm);

  LPTX_MPI_coll_sync (comm, &r) {
    if (irank == root) {
      r = LPTX_particle_set_new_from_remote_replicator_data(&oset, p);
      if (r != MPI_SUCCESS)
        break;

      r = LPTX_create_counts_and_displacements(p, &counts, &displacements);
      if (r != MPI_SUCCESS)
        break;

      nv = LPTX_particle_set_number_of_vectors(oset);
      nd = LPTX_particle_set_number_of_data(oset);
    } else {
      if (inp) {
        nv = LPTX_particle_set_number_of_vectors(inp);
        nd = LPTX_particle_set_number_of_data(inp);
      }
    }

    r = LPTX_particle_data_range_type(&btype, 1);
    if (r != MPI_SUCCESS)
      break;

    if (nv > 0) {
      r = LPTX_particle_vector_range_type(&vtype, 1, nv);
      if (r != MPI_SUCCESS)
        break;
    }

    if (nd > 0) {
      r = LPTX_particle_vector_data_range_type(&dtype, 1, nd);
      if (r != MPI_SUCCESS)
        break;
    }
  }

  if (r == MPI_SUCCESS) {
    do {
      const void *ibp, *ivp, *idp;
      void *obp, *ovp, *odp;

      ibp = ivp = idp = NULL;
      obp = ovp = odp = NULL;
      if (irank == root) {
        obp = oset->particles;
        if (nv > 0)
          ovp = oset->vectors;
        if (nd > 0)
          odp = oset->data;
      }

      if (inp) {
        ibp = inp->particles;
        if (nv > 0)
          ivp = inp->vectors;
        if (nd > 0)
          idp = inp->data;
      }

      r = MPI_Gatherv(ibp, np, btype, obp, counts, displacements, btype, root,
                      comm);
      if (r != MPI_SUCCESS)
        break;

      if (nv > 0) {
        r = MPI_Gatherv(ivp, np, vtype, ovp, counts, displacements, vtype, root,
                        comm);
        if (r != MPI_SUCCESS)
          break;
      }

      if (nd > 0) {
        r = MPI_Gatherv(idp, np, dtype, odp, counts, displacements, dtype, root,
                        comm);
        if (r != MPI_SUCCESS)
          break;
      }
    } while (0);
  }

  if (r == MPI_SUCCESS && irank == root) {
    *out = oset;
    oset = NULL;
  }

  if (counts)
    free(counts);
  if (displacements)
    free(displacements);
  if (btype != MPI_DATATYPE_NULL)
    MPI_Type_free(&btype);
  if (vtype != MPI_DATATYPE_NULL)
    MPI_Type_free(&vtype);
  if (dtype != MPI_DATATYPE_NULL)
    MPI_Type_free(&dtype);
  if (p)
    LPTX_remote_replicator_delete(p);
  if (oset)
    LPTX_particle_set_delete(oset);

  return r;
}

int LPTX_particle_set_allgather(LPTX_particle_set **out, LPTX_particle_set *inp,
                                MPI_Comm comm)
{
  LPTX_remote_replicator_data *p;
  LPTX_particle_set *oset;
  MPI_Datatype btype, vtype, dtype;
  int *counts;
  int *displacements;
  LPTX_idtype np;
  LPTX_idtype nv, nd;
  int r;

  p = NULL;
  oset = NULL;
  btype = MPI_DATATYPE_NULL;
  vtype = MPI_DATATYPE_NULL;
  dtype = MPI_DATATYPE_NULL;
  counts = NULL;
  displacements = NULL;
  nv = 0;
  nd = 0;
  r = MPI_SUCCESS;

  np = 0;
  if (inp)
    np = LPTX_particle_set_number_of_particles(inp);

  r = LPTX_replicate_allgather(&p, NULL, inp, np, MPI_ANY_SOURCE, comm);

  LPTX_MPI_coll_sync (comm, &r) {
    LPTX_idtype st;
    r = LPTX_particle_set_new_from_remote_replicator_data(&oset, p);
    if (r != MPI_SUCCESS)
      break;

    r = LPTX_create_counts_and_displacements(p, &counts, &displacements);
    if (r != MPI_SUCCESS)
      break;

    nv = LPTX_particle_set_number_of_vectors(oset);
    nd = LPTX_particle_set_number_of_data(oset);

    r = LPTX_particle_data_range_type(&btype, 1);
    if (r != MPI_SUCCESS)
      break;

    if (nv > 0) {
      r = LPTX_particle_vector_range_type(&vtype, 1, nv);
      if (r != MPI_SUCCESS)
        break;
    }

    if (nd > 0) {
      r = LPTX_particle_vector_data_range_type(&dtype, 1, nd);
      if (r != MPI_SUCCESS)
        break;
    }
  }

  if (r == MPI_SUCCESS) {
    do {
      const void *ibp, *ivp, *idp;
      void *obp, *ovp, *odp;

      ibp = ivp = idp = NULL;
      obp = ovp = odp = NULL;
      obp = oset->particles;
      if (nv > 0)
        ovp = oset->vectors;
      if (nd > 0)
        odp = oset->data;

      if (inp) {
        ibp = inp->particles;
        if (nv > 0)
          ivp = inp->vectors;
        if (nd > 0)
          idp = inp->data;
      }

      r =
        MPI_Allgatherv(ibp, np, btype, obp, counts, displacements, btype, comm);
      if (r != MPI_SUCCESS)
        break;

      if (nv > 0) {
        r = MPI_Allgatherv(ivp, np, vtype, ovp, counts, displacements, vtype,
                           comm);
        if (r != MPI_SUCCESS)
          break;
      }

      if (nd > 0) {
        r = MPI_Allgatherv(idp, np, dtype, odp, counts, displacements, dtype,
                           comm);
        if (r != MPI_SUCCESS)
          break;
      }
    } while (0);
  }

  if (r == MPI_SUCCESS) {
    *out = oset;
    oset = NULL;
  }

  if (counts)
    free(counts);
  if (displacements)
    free(displacements);
  if (btype != MPI_DATATYPE_NULL)
    MPI_Type_free(&btype);
  if (vtype != MPI_DATATYPE_NULL)
    MPI_Type_free(&vtype);
  if (dtype != MPI_DATATYPE_NULL)
    MPI_Type_free(&dtype);
  if (p)
    LPTX_remote_replicator_delete(p);
  if (oset)
    LPTX_particle_set_delete(oset);

  return r;
}

//---

static void LPTX_count_for_dest_ranks_impl(int nproc,
                                           LPTX_idtype *number_of_particles,
                                           int size_of_rank, const int *ranks,
                                           int *r)
{
  LPTX_idtype *lnps;
  int lr, nt;

  lr = MPI_SUCCESS;
  nt = 1;
  lnps = NULL;

#ifdef _OPENMP
#pragma omp single
#endif
  *r = MPI_SUCCESS;

  do {
#ifdef _OPENMP
    nt = omp_get_num_threads();
#endif

#ifdef _OPENMP
    if (nt > 1 && nproc < LPTX_omp_small_threshold) {
      lnps = (LPTX_idtype *)malloc(sizeof(LPTX_idtype) * nproc);
      if (!lnps) {
        lr = LPTX_MPI_ERR_NO_MEM;
#pragma omp atomic write
        *r = lr;
      }
#pragma omp barrier
#pragma omp atomic read
      lr = *r;
      if (lr != MPI_SUCCESS)
        break;
    }
#endif

#ifdef _OPENMP
#pragma omp for
#endif
    for (int ip = 0; ip < nproc; ++ip)
      number_of_particles[ip] = 0;

    if (lnps) {
      for (int ip = 0; ip < nproc; ++ip)
        lnps[ip] = 0;
    }

#ifdef _OPENMP
#pragma omp for nowait
#endif
    for (int jj = 0; jj < size_of_rank; ++jj) {
      int ir;
      ir = ranks[jj];
      if (ir < 0 || ir >= nproc)
        continue;

      if (lnps) {
        lnps[ir] += 1;
      } else {
#ifdef _OPENMP
#pragma omp atomic update
#endif
        number_of_particles[ir] += 1;
      }
    }

    if (lnps) {
      for (int ip = 0; ip < nproc; ++ip) {
#ifdef _OPENMP
#pragma omp atomic update
#endif
        number_of_particles[ip] += lnps[ip];
      }
    }

  } while (0);

  if (lnps)
    free(lnps);

  if (lr != MPI_SUCCESS) {
#ifdef _OPENMP
#pragma omp atomic write
#endif
    *r = lr;
  }
}

static int LPTX_count_for_dest_ranks(int *nproc_out,
                                     LPTX_idtype **numbers_of_particles,
                                     int size_of_rank, const int *ranks,
                                     MPI_Comm comm)
{
  int r;
  int nproc;
  LPTX_idtype *nps;

  nps = NULL;
  r = MPI_SUCCESS;

  do {
    r = MPI_Comm_size(comm, &nproc);
    if (r != MPI_SUCCESS)
      break;

    nps = (LPTX_idtype *)malloc(sizeof(LPTX_idtype) * nproc);
    if (!nps) {
      r = LPTX_MPI_ERR_NO_MEM;
      break;
    }

#ifdef _OPENMP
#pragma omp parallel if (size_of_rank > LPTX_omp_unsafe_threshold)
#endif
    LPTX_count_for_dest_ranks_impl(nproc, nps, size_of_rank, ranks, &r);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  if (r == MPI_SUCCESS) {
    *numbers_of_particles = nps;
    nps = NULL;

    if (nproc_out)
      *nproc_out = nproc;
  }

  if (nps)
    free(nps);

  return r;
}

static int LPTX_create_type_for_dest_ranks(LPTX_idtype number_of_particles,
                                           MPI_Datatype *datatype,
                                           const LPTX_particle_set *set,
                                           LPTX_idtype start, int size_of_rank,
                                           const int *rank, int rank_to_extract)
{
  LPTX_idtype *indices;
  LPTX_idtype np;
  int r;

  r = MPI_SUCCESS;
  indices = NULL;
  np = number_of_particles;

#ifdef _OPENMP
#pragma omp parallel if (np != 0 && size_of_rank > LPTX_omp_small_threshold)
#endif
  do {
    LPTX_idtype is, ie, id, ls;
    int it, nt, lr;

#ifdef _OPENMP
#pragma omp atomic read
#endif
    id = np;
    if (id < 0) {
#ifdef _OPENMP
#pragma omp barrier
#pragma omp atomic write
#endif
      np = 0;
#ifdef _OPENMP
#pragma omp for reduction(+ : np)
#endif
      for (int jj = 0; jj < size_of_rank; ++jj) {
        if (rank[jj] == rank_to_extract)
          ++np;
      }
#ifdef _OPENMP
#pragma omp atomic read
#endif
      id = np;
      if (id < 0) {
        r = LPTX_MPI_ERR_OVERFLOW;
        break;
      }
    }
    if (id == 0)
      break;

    LPTX_omp_distribute(&is, &ie, &nt, &it, 0, size_of_rank);

#ifdef _OPENMP
#pragma omp single
#endif
    {
      LPTX_idtype na;
      na = (id > nt) ? id : nt;
      indices = (LPTX_idtype *)malloc(sizeof(LPTX_idtype) * na);
      if (!indices)
        r = LPTX_MPI_ERR_NO_MEM;
    }
#ifdef _OPENMP
#pragma omp atomic read
#endif
    lr = r;
    if (lr != MPI_SUCCESS)
      break;

    ls = 0;
    if (nt > 1) {
      for (int jj = is; jj < ie; ++jj) {
        if (rank[jj] == rank_to_extract)
          ++ls;
      }
      indices[it] = ls;
#ifdef _OPENMP
#pragma omp barrier
#endif
      ls = 0;
      for (int jj = 0; jj < it; ++jj)
        ls += indices[jj];

#ifdef _OPENMP
#pragma omp barrier
#endif
    }

    for (int jj = is; jj < ie; ++jj) {
      if (rank[jj] == rank_to_extract)
        indices[ls++] = start + jj;
    }
  } while (0);

  do {
    if (r != MPI_SUCCESS)
      break;

    r = LPTX_composite_particle_index_type(datatype, set, np, indices);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  if (indices)
    free(indices);
  return r;
}

int LPTX_particle_set_scatter(LPTX_particle_set **out,
                              const LPTX_particle_set *inp, LPTX_idtype start,
                              int root, int size_of_rank, const int *rank,
                              MPI_Comm comm)
{
  int nproc;
  int irank;
  int r;
  int nreq;
  LPTX_particle_set *oset;
  LPTX_remote_replicator_data *p;
  LPTX_idtype *number_of_particles;
  MPI_Request *requests;
  MPI_Datatype rtype;

  r = MPI_SUCCESS;
  rtype = MPI_DATATYPE_NULL;
  number_of_particles = NULL;
  requests = NULL;
  p = NULL;
  oset = NULL;
  nproc = 0;
  nreq = 0;

  LPTX_MPI_coll_sync (comm, &r) {
    r = MPI_Comm_rank(comm, &irank);
    if (r != MPI_SUCCESS)
      break;

    if (irank == root) {
      r = LPTX_count_for_dest_ranks(&nproc, &number_of_particles, size_of_rank,
                                    rank, comm);
      if (r != MPI_SUCCESS)
        break;

      requests = (MPI_Request *)malloc(sizeof(MPI_Request) * nproc);
      if (!requests) {
        r = LPTX_MPI_ERR_NO_MEM;
        break;
      }

      for (int ir = 0; ir < nproc; ++ir)
        requests[ir] = MPI_REQUEST_NULL;
    }
  }

  do {
    if (r != MPI_SUCCESS)
      break;

    r = LPTX_replicate_scatter(&p, NULL, inp, number_of_particles, root,
                               MPI_ANY_SOURCE, comm);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  LPTX_MPI_coll_sync (comm, &r) {
    LPTX_idtype np;

    r = LPTX_particle_set_new_from_remote_replicator_data(&oset, p);
    if (r != MPI_SUCCESS)
      break;

    np = 0;
    if (oset)
      np = LPTX_particle_set_number_of_particles(oset);
    if (np > 0) {
      r = LPTX_composite_particle_range_type(&rtype, oset, 0, np);
      if (r != MPI_SUCCESS)
        break;
    }
  }

  LPTX_MPI_coll_sync (comm, &r) {
    if (irank != root)
      break;

    for (int ir = 0; ir < nproc; ++ir) {
      MPI_Datatype stype;
      LPTX_idtype np = number_of_particles[ir];
      if (np <= 0)
        continue;

      r = LPTX_create_type_for_dest_ranks(np, &stype, inp, start, size_of_rank,
                                          rank, ir);
      if (r != MPI_SUCCESS)
        break;

      if (ir == root) {
        r = MPI_Sendrecv(inp, 1, stype, ir, 0, oset, 1, rtype, ir, 0, comm,
                         MPI_STATUS_IGNORE);
      } else {
        r = MPI_Isend(inp, 1, stype, ir, 0, comm, &requests[nreq++]);
      }
      MPI_Type_free(&stype);
      if (r != MPI_SUCCESS)
        break;
    }
  }

  LPTX_MPI_coll_sync (comm, &r) {
    LPTX_idtype np;
    np = 0;

    if (oset)
      np = LPTX_particle_set_number_of_particles(oset);

    if (irank != root && np > 0) {
      r = MPI_Recv(oset, 1, rtype, root, 0, comm, MPI_STATUS_IGNORE);
      if (r != MPI_SUCCESS)
        break;
    }

    if (irank == root && nreq > 0) {
      r = MPI_Waitall(nreq, requests, MPI_STATUSES_IGNORE);
      if (r != MPI_SUCCESS)
        break;
    }
  }

  if (r == MPI_SUCCESS) {
    *out = oset;
    oset = NULL;
  }

  if (rtype != MPI_DATATYPE_NULL)
    MPI_Type_free(&rtype);
  if (p)
    LPTX_remote_replicator_delete(p);
  if (oset)
    LPTX_particle_set_delete(oset);
  if (number_of_particles)
    free(number_of_particles);
  if (requests) {
    for (int ir = 0; ir < nreq; ++ir) {
      if (requests[ir] != MPI_REQUEST_NULL)
        MPI_Cancel(&requests[ir]);
    }
    free(requests);
  }
  return r;
}

int LPTX_particle_set_alltoall(LPTX_particle_set **out,
                               const LPTX_particle_set *inp, LPTX_idtype start,
                               int size_of_rank, const int *rank, MPI_Comm comm)
{
  int nproc;
  int irank;
  int r;
  LPTX_particle_set *oset;
  LPTX_remote_replicator_data *p;
  LPTX_idtype *number_of_particles;
  MPI_Datatype *stypes;
  MPI_Datatype *rtypes;
  int *scounts;
  int *rcounts;
  int *sdispls;
  int *rdispls;

  r = MPI_SUCCESS;
  stypes = NULL;
  rtypes = NULL;
  scounts = NULL;
  rcounts = NULL;
  sdispls = NULL;
  rdispls = NULL;
  number_of_particles = NULL;
  p = NULL;
  oset = NULL;
  nproc = 0;

  LPTX_MPI_coll_sync (comm, &r) {
    LPTX_idtype sz;

    r = LPTX_count_for_dest_ranks(&nproc, &number_of_particles, size_of_rank,
                                  rank, comm);
    if (r != MPI_SUCCESS)
      break;

    stypes = (MPI_Datatype *)malloc(sizeof(MPI_Datatype) * nproc);
    if (!stypes) {
      r = LPTX_MPI_ERR_NO_MEM;
      break;
    }

    for (int ir = 0; ir < nproc; ++ir)
      stypes[ir] = MPI_DATATYPE_NULL;

    rtypes = (MPI_Datatype *)malloc(sizeof(MPI_Datatype) * nproc);
    if (!rtypes) {
      r = LPTX_MPI_ERR_NO_MEM;
      break;
    }

    for (int ir = 0; ir < nproc; ++ir)
      rtypes[ir] = MPI_DATATYPE_NULL;

    scounts = (int *)malloc(sizeof(int) * nproc);
    if (!scounts) {
      r = LPTX_MPI_ERR_NO_MEM;
      break;
    }

    rcounts = (int *)malloc(sizeof(int) * nproc);
    if (!rcounts) {
      r = LPTX_MPI_ERR_NO_MEM;
      break;
    }

    sdispls = (int *)malloc(sizeof(int) * nproc);
    if (!sdispls) {
      r = LPTX_MPI_ERR_NO_MEM;
      break;
    }

    rdispls = (int *)malloc(sizeof(int) * nproc);
    if (!rdispls) {
      r = LPTX_MPI_ERR_NO_MEM;
      break;
    }

    for (int ir = 0; ir < nproc; ++ir) {
      scounts[ir] = 1;
      rcounts[ir] = 1;
      sdispls[ir] = 0;
      rdispls[ir] = 0;
    }
  }

  do {
    if (r != MPI_SUCCESS)
      break;

    r = LPTX_replicate_alltoall(&p, &irank, inp, number_of_particles,
                                MPI_ANY_SOURCE, comm);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  LPTX_MPI_coll_sync (comm, &r) {
    LPTX_idtype ost;

    r = LPTX_particle_set_new_from_remote_replicator_data(&oset, p);
    if (r != MPI_SUCCESS)
      break;

    ost = 0;
    for (int ir = 0; ir < nproc; ++ir) {
      LPTX_idtype nps, npr;

      nps = number_of_particles[ir];
      if (nps > 0) {
        scounts[ir] = 1;
        sdispls[ir] = 0;

        r = LPTX_create_type_for_dest_ranks(nps, &stypes[ir], inp, start,
                                            size_of_rank, rank, ir);
        if (r != MPI_SUCCESS)
          break;
      } else {
        scounts[ir] = 0;
        sdispls[ir] = 0;

        r = MPI_Type_dup(MPI_INT, &stypes[ir]);
        if (r != MPI_SUCCESS)
          break;
      }

      npr = p->number_of_particles[ir];
      if (npr > 0) {
        rcounts[ir] = 1;
        rdispls[ir] = 0;


        r = LPTX_composite_particle_range_type(&rtypes[ir], oset, ost, npr);
        if (r != MPI_SUCCESS)
          break;
      } else {
        rcounts[ir] = 0;
        rdispls[ir] = 0;

        r = MPI_Type_dup(MPI_INT, &rtypes[ir]);
        if (r != MPI_SUCCESS)
          break;
      }
      ost += npr;
    }
  }

  do {
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Alltoallw(inp, scounts, sdispls, stypes, oset, rcounts, rdispls,
                      rtypes, comm);
  } while (0);

  if (r == MPI_SUCCESS) {
    *out = oset;
    oset = NULL;
  }

  if (number_of_particles)
    free(number_of_particles);
  if (p)
    LPTX_remote_replicator_delete(p);
  if (oset)
    LPTX_particle_set_delete(oset);
  if (stypes) {
    for (int ir = 0; ir < nproc; ++ir) {
      if (stypes[ir] != MPI_DATATYPE_NULL)
        MPI_Type_free(&stypes[ir]);
    }
    free(stypes);
  }
  if (rtypes) {
    for (int ir = 0; ir < nproc; ++ir) {
      if (rtypes[ir] != MPI_DATATYPE_NULL)
        MPI_Type_free(&rtypes[ir]);
    }
    free(rtypes);
  }
  if (scounts)
    free(scounts);
  if (rcounts)
    free(rcounts);
  if (sdispls)
    free(sdispls);
  if (rdispls)
    free(rdispls);

  return r;
}

#endif
