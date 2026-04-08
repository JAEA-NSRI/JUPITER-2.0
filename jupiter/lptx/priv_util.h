/**
 * @file lptx/priv_util.h
 */

#ifndef JUPITER_LPTX_PRIV_UTIL_H
#define JUPITER_LPTX_PRIV_UTIL_H

#include "defs.h"
#include "particle.h"
#include "priv_struct_defs.h"

#include "jupiter/geometry/list.h"

#include <stdarg.h>
#include <stdlib.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef JUPITER_LPTX_MPI
#include <mpi.h>
#endif

typedef char *LPTX_asprintf_alloc(int size, void *arg);

/**
 * asprintf with custom allocator
 */
#ifdef __GNUC__
__attribute__((format(printf, 3, 4)))
#endif
JUPITER_LPTX_DECL int
LPTX_aaprintf(LPTX_asprintf_alloc *allocator, void *arg, const char *fmt, ...);
JUPITER_LPTX_DECL int LPTX_vaaprintf(LPTX_asprintf_alloc *allocator, void *arg,
                                     const char *fmt, va_list ap);

#ifdef __GNUC__
__attribute__((format(printf, 2, 3)))
#endif
JUPITER_LPTX_DECL int
LPTX_asprintf(char **buf, const char *fmt, ...);
JUPITER_LPTX_DECL int LPTX_vasprintf(char **buf, const char *fmt, va_list ap);

#ifdef JUPITER_LPTX_MPI
/**
 * Returns LPTX_true if @p cond evaluates true on both @p peer and this rank in
 * @p comm
 *
 * @note This function returns false if communication has been failed,
 *       if error handle is set to MPI_ERRORS_RETURN.
 */
static inline LPTX_bool LPTX_MPI_p2pall(LPTX_bool cond, int peer, int tag,
                                        MPI_Comm comm, int *r)
{
  int fr, sr, rr;
  sr = cond;
  fr = MPI_Sendrecv(&sr, 1, MPI_INT, peer, tag, &rr, 1, MPI_INT, peer, tag,
                    comm, MPI_STATUS_IGNORE);
  if (fr != MPI_SUCCESS) {
    if (r)
      *r = fr;
    return LPTX_false;
  }
  return (sr && rr) ? LPTX_true : LPTX_false;
}

/**
 * Returns LPTX_true if @p cond evaluates true on either @p peer or this rank in
 * @p comm
 *
 * @note This function returns false if communication has been failed,
 *       if error handle is set to MPI_ERRORS_RETURN.
 */
static inline LPTX_bool LPTX_MPI_p2pany(LPTX_bool cond, int peer, int tag,
                                        MPI_Comm comm, int *r)
{
  int fr, sr, rr;
  sr = cond;
  fr = MPI_Sendrecv(&sr, 1, MPI_INT, peer, tag, &rr, 1, MPI_INT, peer, tag,
                    comm, MPI_STATUS_IGNORE);
  if (fr != MPI_SUCCESS) {
    if (r)
      *r = fr;
    return LPTX_false;
  }
  return (sr || rr) ? LPTX_true : LPTX_false;
}

/**
 * Returns LPTX_true if @p cond evaluates true on all ranks in @p comm
 *
 * @note This function returns false if communication has been failed,
 *       if error handle is set to MPI_ERRORS_RETURN.
 *
 * ```c
 * int r = MPI_SUCCESS;
 * if (LPTX_MPI_forall(..., comm, &r)) {
 *   // normal true
 * } else {
 *   if (r != MPI_SUCCESS) {
 *     // MPI error (possibly fatal, but reachable when MPI_ERRORS_RETURN is
 *     // set as error handler)
 *   } else {
 *     // normal false
 *   }
 * }
 * ```
 *
 * Since `LPTX_MPI_forall(!cond, ...)` is same as `!LPTX_MPI_forany(cond, ...)`
 * logically and both functions always return false for communication
 * failure. So you can choose functions to process communication failure in
 * single path. i.e.:
 *
 * ```
 * ret = some_call(...);
 * if (LPTX_MPI_forany((ret is failure), ..., NULL)) {
 *   // failure on some rank but not communication failure
 * }
 * ```
 * ```
 * ret = some_call(...);
 * if (!LPTX_MPI_forall(!(ret is failure), ..., NULL)) {
 *   // failure on some rank or communication failure
 * }
 * ```
 */
static inline LPTX_bool LPTX_MPI_forall(LPTX_bool cond, MPI_Comm comm, int *r)
{
  int rr;
  int c = cond;

  rr = MPI_Allreduce(MPI_IN_PLACE, &c, 1, MPI_INT, MPI_LAND, comm);
  if (rr != MPI_SUCCESS) {
    if (r)
      *r = rr;
    return LPTX_false;
  }

  return c ? LPTX_true : LPTX_false;
}

/**
 * Returns LPTX_true if @p cond evaluates true on least one of ranks in @p comm
 *
 * @note This function returns false if communication has been failed,
 *       if error handle is set to MPI_ERRORS_RETURN.
 *
 * ```c
 * int r = MPI_SUCCESS;
 * if (LPTX_MPI_forany(..., comm, &r)) {
 *   // normal true
 * } else {
 *   if (r != MPI_SUCCESS) {
 *     // MPI error (possibly fatal, but reachable when MPI_ERRORS_RETURN is
 *     // set as error handler)
 *   } else {
 *     // normal false
 *   }
 * }
 * ```
 */
static inline LPTX_bool LPTX_MPI_forany(LPTX_bool cond, MPI_Comm comm, int *r)
{
  int rr;
  int c = cond;

  rr = MPI_Allreduce(MPI_IN_PLACE, &c, 1, MPI_INT, MPI_LOR, comm);
  if (rr != MPI_SUCCESS) {
    if (r)
      *r = rr;
    return LPTX_false;
  }

  return c ? LPTX_true : LPTX_false;
}

static inline LPTX_bool LPTX_MPI_coll_sync_impl(int cnt, MPI_Comm comm, int *r)
{
  if (cnt) {
    if (!LPTX_MPI_forall(*r == MPI_SUCCESS, comm, r)) {
      if (*r == MPI_SUCCESS)
        *r = LPTX_MPI_ERR_REMOTE;
    }
    return LPTX_false;
  }

  if (*r == MPI_SUCCESS)
    return LPTX_true;
  return LPTX_false;
}

/**
 * @brief Synchronization for collective processing
 * @param comm Communicator to sync
 * @param r Pointer to MPI error value [in/out]
 *
 * If @p *r is already not MPI_SUCCESS at entry, the following block will not be
 * executed at all.
 *
 * If @p *r is MPI_SUCCESS on entry, executes the following block and then
 * checks whether all processes still keep *r == MPI_SUCCESS. If this rank keeps
 * *r == MPI_SUCCESS and any of other rank does not, sets *r to
 * LPTX_MPI_ERR_REMOTE.
 *
 * @note the value of *r is assumed to be already synchronized at entry.
 * Set *r to MPI_SUCCESS for the first use.
 *
 * ```
 * // While prior processing...
 * r = MPI_ERR_COMM;
 *
 * LPTX_MPI_coll_sync(comm, &r) {
 *   // This block will not be executed
 * }
 * ```
 *
 * ```
 * r = MPI_SUCCESS;
 *
 * LPTX_MPI_coll_sync(comm, &r) {
 *   if (some error at a specific rank and then)
 *     r = MPI_ERR_NO_MEM;
 * }
 * // r == LPTX_MPI_ERR_REMOTE at ranks in no error occurred
 * // r == MPI_ERR_NO_MEM      at ranks in the error occurred
 * ```
 *
 * The macro uses `for` loop. Executing `continue` or `break` synchronize
 * immediately.
 *
 * ```
 * r = MPI_SUCCESS;
 * LPTX_MPI_coll_sync(comm, &r) {
 *   r = MPI_...(...); // not collective MPI call.
 *   if (r != MPI_SUCCESS)
 *     break; // abort remaining processing and sync immediately
 *
 *   ...
 * }
 *
 * LPTX_MPI_coll_sync(comm, *r) {
 *   r = MPI_...(...); // not collective MPI call.
 *   if (r != MPI_SUCCESS)
 *     continue; // continue is also ok
 *
 *   ...
 * }
 * ```
 *
 * LPTX_MPI_coll_sync() cannot be nested.
 *
 * ```
 * LPTX_MPI_coll_sync(comm, &r) {
 *   LPTX_MPI_coll_sync(comm, &r) { // compilation error
 *     ...
 *   }
 *   ...
 * }
 * ```
 */
#define LPTX_MPI_coll_sync(comm, r)                                    \
  for (int _lptx_cnt = 0; LPTX_MPI_coll_sync_impl(_lptx_cnt, comm, r); \
       ++_lptx_cnt)                                                    \
    for (int _lptx_cnt_int = 0; !_lptx_cnt_int; ++_lptx_cnt_int)

static inline LPTX_bool LPTX_MPI_p2p_sync_impl(int cnt, int peer, int tag,
                                               MPI_Comm comm, int *r)
{
  if (cnt) {
    if (!LPTX_MPI_p2pall(*r == MPI_SUCCESS, peer, tag, comm, r)) {
      if (*r == MPI_SUCCESS)
        *r = LPTX_MPI_ERR_REMOTE;
    }
    return LPTX_false;
  }

  if (*r == MPI_SUCCESS)
    return LPTX_true;
  return LPTX_false;
}

/**
 * @brief Synchronization for P2P processing
 * @param peer Peer rank
 * @param tag  Tag to be used for communication
 * @param comm Communicator to sync
 * @param r Pointer to MPI error value [in/out]
 *
 * If @p *r is already not MPI_SUCCESS at entry, the following block will not be
 * executed at all.
 *
 * If @p *r is MPI_SUCCESS on entry, executes the following block and then
 * checks whether all processes still keep *r == MPI_SUCCESS. If this rank keeps
 * *r == MPI_SUCCESS and any of other rank does not, sets *r to
 * LPTX_MPI_ERR_REMOTE.
 *
 * @note the value of *r is assumed to be already synchronized at entry.
 * Set *r to MPI_SUCCESS for the first use.
 *
 * ```
 * // While prior processing...
 * r = MPI_ERR_COMM;
 *
 * LPTX_MPI_p2p_sync(comm, &r) {
 *   // This block will not be executed
 * }
 * ```
 *
 * ```
 * r = MPI_SUCCESS;
 *
 * LPTX_MPI_p2p_sync(comm, &r) {
 *   if (some error at a specific rank and then)
 *     r = MPI_ERR_NO_MEM;
 * }
 * // r == LPTX_MPI_ERR_REMOTE if error occurred at peer rank only
 * // r == MPI_ERR_NO_MEM      if error occurred at this rank
 * ```
 *
 * The macro uses `for` loop. Executing `continue` or `break` synchronize
 * immediately.
 *
 * ```
 * r = MPI_SUCCESS;
 * LPTX_MPI_p2p_sync(comm, &r) {
 *   r = MPI_...(...); // not communication MPI call.
 *   if (r != MPI_SUCCESS)
 *     break; // abort remaining process and sync immediately
 *
 *   ...
 * }
 *
 * LPTX_MPI_p2p_sync(comm, *r) {
 *   r = MPI_...(...); // not communication MPI call.
 *   if (r != MPI_SUCCESS)
 *     continue; // continue is also ok
 *
 *   ...
 * }
 * ```
 *
 * LPTX_MPI_p2p_sync() cannot be nested.
 *
 * ```
 * LPTX_MPI_p2p_sync(comm, &r) {
 *   LPTX_MPI_p2p_sync(comm, &r) { // compilation error
 *     ...
 *   }
 *   ...
 * }
 * ```
 */
#define LPTX_MPI_p2p_sync(peer, tag, comm, r)                              \
  for (int _lptx_cnt = 0;                                                  \
       LPTX_MPI_p2p_sync_impl(_lptx_cnt, peer, tag, comm, r); ++_lptx_cnt) \
    for (int _lptx_cnt_int = 0; !_lptx_cnt_int; ++_lptx_cnt_int)

#endif

static inline void LPTX_param_foreach_particle_sets_i(
  LPTX_param *param, LPTX_cb_foreach_particle_sets *func, void *arg)
{
  struct geom_list *lp, *lh;
  lh = &param->sets_head.list;
  geom_list_foreach (lp, lh) {
    LPTX_particle_set *set;
    set = LPTX_particle_set_entry(lp);

    if (func(set, arg))
      break;
  }
}

static inline void LPTX_param_foreach_particle_sets_si(
  LPTX_param *param, LPTX_cb_foreach_particle_sets *func, void *arg)
{
  struct geom_list *lp, *ln, *lh;
  lh = &param->sets_head.list;
  geom_list_foreach_safe (lp, ln, lh) {
    LPTX_particle_set *set;
    set = LPTX_particle_set_entry(lp);

    if (func(set, arg))
      break;
  }
}

static inline LPTX_bool
LPTX_call_cb_ptrange(LPTX_particle_set *set, LPTX_idtype start,
                     LPTX_idtype *last, LPTX_cb_foreach_particle_range *func,
                     void *arg)
{
  if (!last) {
    LPTX_idtype last = LPTX_particle_set_number_of_particles(set);
    return func(set, start, &last, arg);
  }
  return func(set, start, last, arg);
}

/**
 * Iterate over particle sets at given indices of sets.
 *
 * @p func will be called by range of all particles in the set.
 */
static inline void LPTX_param_foreach_particle_set_range_i(
  LPTX_param *param, LPTX_idtype start, LPTX_idtype last,
  LPTX_cb_foreach_particle_range *func, void *arg)
{
  LPTX_idtype i = 0;
  struct geom_list *lp, *lh;
  lh = &param->sets_head.list;
  geom_list_foreach (lp, lh) {
    if (start <= i && i < last) {
      LPTX_particle_set *set;
      set = LPTX_particle_set_entry(lp);
      if (LPTX_call_cb_ptrange(set, 0, NULL, func, arg))
        break;
    }
    ++i;
  }
}

static inline void LPTX_param_foreach_particle_set_range_si(
  LPTX_param *param, LPTX_idtype start, LPTX_idtype last,
  LPTX_cb_foreach_particle_range *func, void *arg)
{
  LPTX_idtype i = 0;
  struct geom_list *lp, *ln, *lh;
  lh = &param->sets_head.list;
  geom_list_foreach_safe (lp, ln, lh) {
    if (start <= i && i < last) {
      LPTX_particle_set *set;
      set = LPTX_particle_set_entry(lp);
      if (LPTX_call_cb_ptrange(set, 0, NULL, func, arg))
        break;
    }
    ++i;
  }
}

static inline LPTX_bool
LPTX_call_cb_ptrange_p(LPTX_particle_set *set, LPTX_idtype *start,
                       LPTX_idtype *last, LPTX_cb_foreach_particle_range *func,
                       void *arg)
{
  LPTX_idtype np;
  LPTX_idtype pstart, plast_save, plast;
  np = LPTX_particle_set_number_of_particles(set);
  if (*start >= np) {
    pstart = 0;
    plast = 0;
    plast_save = plast;
    if (func(set, pstart, &plast, arg))
      return LPTX_true;

    *start -= np;
    *last += plast - plast_save - np;
    return LPTX_false;
  }

  pstart = *start;
  plast = *last;
  if (plast > np)
    plast = np;
  if (plast < pstart)
    plast = pstart;
  plast_save = plast;
  if (func(set, pstart, &plast, arg))
    return LPTX_true;

  *start = 0;
  *last += plast - plast_save - np;
  return LPTX_false;
}

/**
 * Iterate over particle sets at given indices of particles
 *
 * @p func will be called for all particle sets even if its range is empty.
 */
static inline void LPTX_param_foreach_particle_set_range_pi(
  LPTX_param *param, LPTX_idtype start, LPTX_idtype last,
  LPTX_cb_foreach_particle_range *func, void *arg)
{
  struct geom_list *lp, *lh;
  lh = &param->sets_head.list;
  geom_list_foreach (lp, lh) {
    LPTX_particle_set *set;
    set = LPTX_particle_set_entry(lp);
    if (LPTX_call_cb_ptrange_p(set, &start, &last, func, arg))
      break;
  }
}

static inline void LPTX_param_foreach_particle_set_range_spi(
  LPTX_param *param, LPTX_idtype start, LPTX_idtype last,
  LPTX_cb_foreach_particle_range *func, void *arg)
{
  struct geom_list *lp, *ln, *lh;
  lh = &param->sets_head.list;
  geom_list_foreach_safe (lp, ln, lh) {
    LPTX_particle_set *set;
    set = LPTX_particle_set_entry(lp);
    if (LPTX_call_cb_ptrange_p(set, &start, &last, func, arg))
      break;
  }
}

static inline void
LPTX_param_foreach_init_sets_i(LPTX_param *param,
                               LPTX_cb_foreach_init_sets *func, void *arg)
{
  struct geom_list *lp, *lh;
  lh = &param->init_sets_head.list;
  geom_list_foreach (lp, lh) {
    LPTX_particle_init_set *set;
    set = LPTX_particle_init_set_entry(lp);

    if (func(set, arg))
      break;
  }
}

static inline void
LPTX_param_foreach_init_sets_si(LPTX_param *param,
                                LPTX_cb_foreach_init_sets *func, void *arg)
{
  struct geom_list *lp, *ln, *lh;
  lh = &param->init_sets_head.list;
  geom_list_foreach_safe (lp, ln, lh) {
    LPTX_particle_init_set *set;
    set = LPTX_particle_init_set_entry(lp);

    if (func(set, arg))
      break;
  }
}

static inline void LPTX_param_foreach_collision_data_i(
  LPTX_param *param, LPTX_cb_foreach_collision_data *func, void *arg)
{
  LPTX_idtype b;
  LPTX_idtype n;
  struct geom_list *lp, *lh;
  n = param->number_of_collisions;
  lh = &param->collision_list_head.list;

  b = LPTX_false;
  geom_list_foreach (lp, lh) {
    LPTX_collision_list_set *set;
    LPTX_idtype nl;
    set = LPTX_collision_list_set_entry(lp);
    nl = (n < set->number_of_entries) ? n : set->number_of_entries;

    for (LPTX_idtype jj = 0; jj < nl; ++jj) {
      b = func(&set->entries[jj], arg);
      if (b)
        break;
    }
    if (b)
      break;

    n -= nl;
    if (n <= 0)
      break;
  }
}

/**
 * @brief Update global variable according thread-local value
 * @param global Pointer to global variable
 * @param local The value to update (from thread-local variable)
 * @param func Condition to update the global variable
 *
 * @note If OpenMP is not enabled, @p func is not used and this function always
 * sets @p global to @p local value.
 *
 * For, summation, product reduction you can use atomic operation:
 *
 * ```c
 * #pragma omp atomic update
 *   *global += *local;
 * ```
 */
static inline void LPTX_type_update(LPTX_type *global, LPTX_type local,
                                    LPTX_bool (*func)(LPTX_type g, LPTX_type l))
{
#ifdef _OPENMP
  LPTX_type g;
#pragma omp atomic read
  g = *global;
  if (func(g, local)) {
#pragma omp critical
    {
      if (func(*global, local))
        *global = local;
    }
  }
#else
  *global = local;
#endif
}

static inline LPTX_bool LPTX_type_update__max(LPTX_type g, LPTX_type l)
{
  return g < l;
}

/**
 * @brief set *global to *local if *local is greater.
 */
static inline void LPTX_type_update_max(LPTX_type *global, LPTX_type local)
{
  LPTX_type_update(global, local, LPTX_type_update__max);
}

static inline LPTX_bool LPTX_type_update__min(LPTX_type g, LPTX_type l)
{
  return g > l;
}

/**
 * @brief set *global to *local if *local is less.
 */
static inline void LPTX_type_update_min(LPTX_type *global, LPTX_type local)
{
  LPTX_type_update(global, local, LPTX_type_update__min);
}

static inline void LPTX_collision_list_data_init(LPTX_collision_list_data *p)
{
  geom_list_init(&p->list);
  p->a = NULL;
  p->b = NULL;
  p->new_p = 0;
  p->processed = LPTX_false;
}

static inline void LPTX_collision_list_init(LPTX_collision_list *l)
{
  geom_list_init(&l->list);
}

#endif
