#include "particle.h"
#include "defs.h"
#include "error.h"
#include "interpolator.h"
#include "jupiter/geometry/bitarray.h"
#include "jupiter/geometry/util.h"
#include "jupiter/random/random.h"
#include "overflow.h"
#include "param.h"
#include "priv_array_data.h"
#include "priv_array_types_a.h"
#include "priv_array_types_p.h"
#include "priv_struct_defs.h"
#include "priv_util.h"
#include "ptflags.h"
#include "pvector.h"
#include "struct_defs.h"
#include "type_math.h"
#include "util.h"
#include "vector.h"

#include <inttypes.h>
#include <jupiter/geometry/list.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef JUPITER_LPTX_MPI
#include <mpi.h>
#endif

static int LPTX_particle_set_list_is_head(struct LPTX_particle_set_list *p)
{
  return p == &p->param->sets_head;
}

static LPTX_particle_set *
LPTX_get_particle_set_from_list(struct geom_list *list)
{
  struct LPTX_particle_set_list *lp;
  lp = LPTX_particle_set_list_entry(list);
  if (LPTX_particle_set_list_is_head(lp))
    return NULL;
  return LPTX_particle_set_entry(&lp->list);
}

LPTX_param *LPTX_particle_set_param(LPTX_particle_set *set)
{
  return set->list.param;
}

LPTX_particle_set *LPTX_particle_set_next(LPTX_particle_set *set)
{
  return LPTX_get_particle_set_from_list(geom_list_next(&set->list.list));
}

LPTX_particle_set *LPTX_particle_set_prev(LPTX_particle_set *set)
{
  return LPTX_get_particle_set_from_list(geom_list_prev(&set->list.list));
}

LPTX_idtype LPTX_particle_set_number_of_particles(const LPTX_particle_set *set)
{
  return set->number_particles;
}

LPTX_idtype LPTX_particle_set_number_of_vectors(const LPTX_particle_set *set)
{
  return set->number_vectors;
}

LPTX_idtype LPTX_particle_set_number_of_data(const LPTX_particle_set *set)
{
  return set->number_data;
}

const LPTX_idtype *LPTX_particle_set_vector_sizes(const LPTX_particle_set *set)
{
  return set->vector_sizes;
}

LPTX_particle_set *LPTX_particle_set_insert_next(LPTX_particle_set *prev,
                                                 LPTX_particle_set *set)
{
  if (!prev->list.param)
    return NULL;

  geom_list_insert_next(&prev->list.list, &set->list.list);
  set->list.param = prev->list.param;
  return set;
}

LPTX_particle_set *LPTX_particle_set_insert_prev(LPTX_particle_set *next,
                                                 LPTX_particle_set *set)
{
  if (!next->list.param)
    return NULL;

  geom_list_insert_prev(&next->list.list, &set->list.list);
  set->list.param = next->list.param;
  return set;
}

LPTX_particle_set *LPTX_particle_set_append(LPTX_param *param,
                                            LPTX_particle_set *set)
{
  geom_list_insert_prev(&param->sets_head.list, &set->list.list);
  set->list.param = param;
  return set;
}

LPTX_particle_set *LPTX_particle_set_prepend(LPTX_param *param,
                                             LPTX_particle_set *set)
{
  geom_list_insert_next(&param->sets_head.list, &set->list.list);
  set->list.param = param;
  return set;
}

/**
 * Sort particles by insertion sort
 */
static void LPTX_particle_set_isort(LPTX_particle_data **sorted, LPTX_idtype is,
                                    LPTX_idtype ie,
                                    LPTX_cb_particle_compn *comp, void *arg)
{
  LPTX_idtype i = is;
  while (i <= ie) {
    LPTX_idtype j = i;
    for (; j > 0; --j) {
      LPTX_particle_data *a, *b;
      a = sorted[j - 1];
      b = sorted[j];
      if (comp(a, b, arg) <= 0)
        break;

      sorted[j - 1] = b;
      sorted[j] = a;
    }
    ++i;
  }
}

static LPTX_idtype
LPTX_particle_set_qsort_partition(LPTX_particle_data **sorted, LPTX_idtype is,
                                  LPTX_idtype ie, LPTX_cb_particle_compn *comp,
                                  void *arg)
{
  LPTX_particle_data *pivot;
  LPTX_idtype i;

  pivot = sorted[ie];

  i = is;
  for (LPTX_idtype j = is; j < ie; ++j) {
    LPTX_particle_data *a;
    a = sorted[j];
    if (comp(a, pivot, arg) <= 0) {
      sorted[j] = sorted[i];
      sorted[i] = a;
      ++i;
    }
  }

  sorted[ie] = sorted[i];
  sorted[i] = pivot;
  return i;
}

/**
 * Sort particles by quick sort
 */
static void LPTX_particle_set_qsort(LPTX_particle_data **sorted, LPTX_idtype is,
                                    LPTX_idtype ie,
                                    LPTX_cb_particle_compn *comp, void *arg)
{
  LPTX_idtype p;

  if (is >= ie || is < 0)
    return;

  p = LPTX_particle_set_qsort_partition(sorted, is, ie, comp, arg);

  LPTX_particle_set_qsort(sorted, is, p - 1, comp, arg);
  LPTX_particle_set_qsort(sorted, p + 1, ie, comp, arg);
}

static void LPTX_particle_set_update_sorted_indices(LPTX_particle_set *set)
{
  LPTX_idtype np;

  np = LPTX_particle_set_number_of_particles(set);

#ifdef _OPENMP
#pragma omp parallel for if (np >= LPTX_omp_small_threshold)
#endif
  for (LPTX_idtype i = 0; i < np; ++i)
    set->sorted_indices[set->sorted[i] - set->particles] = i;
}

enum LPTX_particle_set_sort_large
{
#ifdef _OPENMP
  LPTX_particle_set_sort_small_threshold = LPTX_omp_small_threshold,
#else
  LPTX_particle_set_sort_small_threshold = 100,
#endif
};

void LPTX_particle_set_sort_particles(LPTX_particle_set *set,
                                      LPTX_cb_particle_compn *comp, void *arg)
{
  LPTX_idtype np;
  np = LPTX_particle_set_number_of_particles(set);

  if (np < LPTX_particle_set_sort_small_threshold) {
    LPTX_particle_set_isort(set->sorted, 0, np - 1, comp, arg);
  } else {
    LPTX_particle_set_qsort(set->sorted, 0, np - 1, comp, arg);
  }

  LPTX_particle_set_update_sorted_indices(set);
}

void LPTX_particle_set_isort_particles(LPTX_particle_set *set,
                                       LPTX_cb_particle_compn *comp, void *arg)
{
  LPTX_idtype np;

  np = LPTX_particle_set_number_of_particles(set);
  LPTX_particle_set_isort(set->sorted, 0, np - 1, comp, arg);
  LPTX_particle_set_update_sorted_indices(set);
}

void LPTX_particle_set_qsort_particles(LPTX_particle_set *set,
                                       LPTX_cb_particle_compn *comp, void *arg)
{
  LPTX_idtype np;

  np = LPTX_particle_set_number_of_particles(set);
  LPTX_particle_set_qsort(set->sorted, 0, np - 1, comp, arg);
  LPTX_particle_set_update_sorted_indices(set);
}

static LPTX_idtype LPTX_particle_set_bsearch(LPTX_particle_data **sorted,
                                             LPTX_idtype is, LPTX_idtype ie,
                                             LPTX_cb_particle_comp1 *comp,
                                             void *arg)
{
  while (is <= ie) {
    LPTX_idtype m = (is + ie) / 2;
    int r = comp(sorted[m], arg);
    if (r > 0) {
      ie = m - 1;
    } else if (r < 0) {
      is = m + 1;
    } else {
      return m;
    }
  }
  return -1;
}

LPTX_idtype LPTX_particle_set_binary_search(LPTX_particle_set *set,
                                            LPTX_cb_particle_comp1 *comp,
                                            void *arg)
{
  LPTX_idtype np = LPTX_particle_set_number_of_particles(set);
  return LPTX_particle_set_bsearch(set->sorted, 0, np - 1, comp, arg);
}

static LPTX_idtype LPTX_particle_set_lbound(LPTX_particle_data **sorted,
                                            LPTX_idtype is, LPTX_idtype ie,
                                            LPTX_cb_particle_comp1 *comp,
                                            void *arg)
{
  LPTX_idtype mid, lb;

  lb = -1;
  while (is <= ie) {
    int r;

    mid = is + (ie - is) / 2;
    r = comp(sorted[mid], arg);
    if (r >= 0) {
      ie = mid - 1;
    } else {
      lb = mid;
      is = mid + 1;
    }
  }
  return lb + 1;
}

static LPTX_idtype LPTX_particle_set_ubound(LPTX_particle_data **sorted,
                                            LPTX_idtype is, LPTX_idtype ie,
                                            LPTX_cb_particle_comp1 *comp,
                                            void *arg)
{
  LPTX_idtype mid, ub;

  ub = -1;
  while (is <= ie) {
    int r;

    mid = is + (ie - is) / 2;
    r = comp(sorted[mid], arg);
    if (r <= 0) {
      is = mid + 1;
    } else {
      ub = mid;
      ie = mid - 1;
    }
  }
  return ub;
}

LPTX_idtype LPTX_particle_set_binary_lbound(LPTX_particle_set *set,
                                            LPTX_cb_particle_comp1 *comp,
                                            void *arg)
{
  LPTX_idtype np;
  np = LPTX_particle_set_number_of_particles(set);

  return LPTX_particle_set_lbound(set->sorted, 0, np - 1, comp, arg);
}

LPTX_idtype LPTX_particle_set_binary_ubound(LPTX_particle_set *set,
                                            LPTX_cb_particle_comp1 *comp,
                                            void *arg)
{
  LPTX_idtype np, ub;

  np = LPTX_particle_set_number_of_particles(set);
  ub = LPTX_particle_set_ubound(set->sorted, 0, np - 1, comp, arg);
  if (ub < 0)
    ub = np;
  return ub;
}

void LPTX_particle_set_binary_partition(LPTX_particle_set *set,
                                        LPTX_idtype *first, LPTX_idtype *last,
                                        LPTX_cb_particle_comp1 *comp, void *arg)
{
  LPTX_idtype np;
  np = LPTX_particle_set_number_of_particles(set);

  if (first)
    *first = LPTX_particle_set_lbound(set->sorted, 0, np - 1, comp, arg);

  if (last) {
    *last = LPTX_particle_set_ubound(set->sorted, 0, np - 1, comp, arg);
    if (*last < 0)
      *last = np;
  }
}

LPTX_idtype LPTX_particle_set_array_index_from_sorted(LPTX_particle_set *set,
                                                      LPTX_idtype sorted_index)
{
  LPTX_idtype np = LPTX_particle_set_number_of_particles(set);
  if (sorted_index < 0 || sorted_index >= np)
    return -1;
  return set->sorted[sorted_index] - set->particles;
}

LPTX_idtype LPTX_particle_set_sorted_index_from_array(LPTX_particle_set *set,
                                                      LPTX_idtype array_index)
{
  LPTX_idtype np = LPTX_particle_set_number_of_particles(set);
  if (array_index < 0 || array_index >= np)
    return -1;
  return set->sorted_indices[array_index];
}

static int LPTX_particle_sorter(const LPTX_particle_data *a,
                                const LPTX_particle_data *b, void *arg)
{
  const LPTX_particle *pa = &a->base;
  const LPTX_particle *pb = &b->base;
  int pa_used = LPTX_particle_is_used(pa);
  int pb_used = LPTX_particle_is_used(pb);

  if (!pa_used && !pb_used)
    return 0;

  if (!pa_used)
    return 1;
  if (!pb_used)
    return -1;

  if (pa->particle_id == pb->particle_id)
    return 0;
  return (pa->particle_id - pb->particle_id > 0) ? 1 : -1;
}

void LPTX_particle_set_sort_particles_id(LPTX_particle_set *set)
{
  LPTX_particle_set_sort_particles(set, LPTX_particle_sorter, NULL);
}

static int LPTX_particle_active_sorter(const LPTX_particle_data *a,
                                       const LPTX_particle_data *b, void *arg)
{
  const LPTX_particle *pa = &a->base;
  const LPTX_particle *pb = &b->base;
  int pa_exited, pb_exited;

  if (!(LPTX_particle_is_used(pa) && LPTX_particle_is_used(pb)))
    return LPTX_particle_sorter(a, b, NULL);

  pa_exited = LPTX_particle_is_exited(pa);
  pb_exited = LPTX_particle_is_exited(pb);
  if (pa_exited == pb_exited)
    return (pa->particle_id - pb->particle_id > 0) ? 1 : -1;

  if (pa_exited && !pb_exited)
    return -1;
  return 1;
}

void LPTX_particle_set_sort_particles_active(LPTX_particle_set *set)
{
  LPTX_particle_set_sort_particles(set, LPTX_particle_active_sorter, NULL);
}

static int LPTX_particle_used_sorted_comp(const LPTX_particle_data *p,
                                          void *arg)
{
  if (!LPTX_particle_data_is_used(p))
    return 1;
  return 0;
}

LPTX_idtype LPTX_particle_set_used_particles_sorted(LPTX_particle_set *set)
{
  return LPTX_particle_set_binary_ubound(set, LPTX_particle_used_sorted_comp,
                                         NULL);
}

static int LPTX_particle_set_find_index_comp(const LPTX_particle_data *p,
                                             void *arg)
{
  LPTX_idtype id;

  if (LPTX_particle_used_sorted_comp(p, NULL))
    return 1;

  id = *(LPTX_idtype *)arg;
  if (p->base.particle_id == id)
    return 0;

  return (p->base.particle_id - id > 0) ? 1 : -1;
}

LPTX_idtype LPTX_particle_set_find_index(LPTX_particle_set *set, LPTX_idtype id)
{
  return LPTX_particle_set_binary_search(set, LPTX_particle_set_find_index_comp,
                                         &id);
}

LPTX_bool LPTX_particle_set_are_mergeable(LPTX_particle_set *a,
                                          LPTX_particle_set *b)
{
  /* a or b must not be NULL */
  if (!(a || b))
    return LPTX_false;

  /* a or b allowed to be NULL, no further check is required */
  if (!(a && b))
    return LPTX_true;

  if (a->number_vectors != b->number_vectors)
    return LPTX_false;
  if (a->number_data != b->number_data)
    return LPTX_false;

  for (LPTX_idtype jj = 0; jj < b->number_vectors; ++jj)
    if (a->vector_sizes[jj] != b->vector_sizes[jj])
      return LPTX_false;

  return LPTX_true;
}

void LPTX_assert_particle_set_are_mergeable_impl(LPTX_particle_set *a,
                                                 LPTX_particle_set *b,
                                                 const char *aname,
                                                 const char *bname,
                                                 const char *file, long line)
{
  LPTX_assert_Xf(a || b, file, line, "%s or %s must not be NULL", aname, bname);

  if (!(a && b))
    return;

  LPTX_assert_Xf(a->number_vectors == b->number_vectors, file, line,
                 "number of vectors in %s and %s differ", aname, bname);
  LPTX_assert_Xf(a->number_data == b->number_data, file, line,
                 "number of vector data in %s and %s differ", aname, bname);
  for (LPTX_idtype jj = 0; jj < a->number_vectors; ++jj) {
    LPTX_assert_Xf(a->vector_sizes[jj] == b->vector_sizes[jj], file, line,
                   "number of elements in vector %" PRIdMAX " in %s and %s "
                   "differ",
                   (intmax_t)jj, aname, bname);
  }
}

LPTX_bool LPTX_particle_set_merge(LPTX_particle_set **outp,
                                  LPTX_particle_set *a, LPTX_particle_set *b,
                                  LPTX_cb_particle_set_merge *func, void *arg)
{
  LPTX_particle_set *set = NULL;
  LPTX_idtype npa = 0;
  LPTX_idtype npb = 0;
  LPTX_idtype npo = 0;
  LPTX_idtype ja, jb, jo;

  LPTX_assert(!!outp);
  LPTX_assert(!!func);
  LPTX_assert(a || b);
  LPTX_assert(a != b);
  LPTX_assert_particle_set_are_mergeable(a, b);

  if (a) {
    LPTX_particle_set_sort_particles_id(a);
    npa = LPTX_particle_set_used_particles_sorted(a);
  }
  if (b) {
    LPTX_particle_set_sort_particles_id(b);
    npb = LPTX_particle_set_used_particles_sorted(b);
  }
  if (LPTX_s_add_overflow(npa, npb, &npo))
    return LPTX_false;

  if (npo <= 0) {
    *outp = NULL;
    return LPTX_true;
  }

  set = LPTX_particle_set_replicate(npo, a ? a : b);
  if (!set)
    return LPTX_false;

  ja = 0;
  jb = 0;
  jo = 0;
  while (1) {
    LPTX_idtype id;
    LPTX_particle_data *po;
    const LPTX_particle_data *p1;

    if (ja < npa) {
      if (jb < npb) {
        LPTX_idtype ida, idb;
        ida = a->sorted[ja]->base.particle_id;
        idb = b->sorted[jb]->base.particle_id;
        id = (ida < idb) ? ida : idb;
      } else {
        id = a->sorted[ja]->base.particle_id;
      }
    } else if (jb < npb) {
      id = b->sorted[jb]->base.particle_id;
    } else {
      break;
    }

    p1 = NULL;
    po = &set->particles[jo++];

    for (; ja < npa; ++ja) {
      if (a->sorted[ja]->base.particle_id != id)
        break;

      if (p1) {
        func(po, p1, a->sorted[ja], arg);
        p1 = po;
      } else {
        p1 = a->sorted[ja];
      }
    }
    for (; jb < npb; ++jb) {
      if (b->sorted[jb]->base.particle_id != id)
        break;

      if (p1) {
        func(po, p1, b->sorted[jb], arg);
        p1 = po;
      } else {
        p1 = b->sorted[jb];
      }
    }
    if (p1 != po)
      LPTX_particle_copy(po, p1);
  }

  *outp = set;
  return LPTX_true;
}

LPTX_bool LPTX_particle_set_filter(LPTX_particle_set **outp,
                                   LPTX_particle_set **stripped,
                                   LPTX_particle_set *set,
                                   LPTX_cb_particle_if *cond, void *arg)
{
  LPTX_particle_set *toutp, *tstripped;
  LPTX_idtype ninp;
  LPTX_idtype nout;
  LPTX_idtype nstripped;
  LPTX_idtype nindices;
  LPTX_idtype *indices;
  LPTX_bool *condc;
  LPTX_bool r;

  ninp = LPTX_particle_set_number_of_particles(set);
  nindices = ninp;
  nout = 0;
  nstripped = 0;
  indices = NULL;
  r = LPTX_true;
  toutp = NULL;
  tstripped = NULL;

#ifdef _OPENMP
#pragma omp parallel if (ninp > LPTX_omp_small_threshold)
#endif
  do {
    LPTX_bool lr;
    LPTX_idtype is, ie;
    int it, nt;
    LPTX_omp_distribute(&is, &ie, &nt, &it, 0, ninp);

#ifdef _OPENMP
#pragma omp master
#endif
    {
      if (outp || stripped) {
        /* Temporary shared memory */
        if (LPTX_s_mul_overflow(nt, 2, &nindices))
          r = LPTX_false;
      } else {
        nindices = 0; /* No new particle allocation */
      }

      if (r && nindices > 0) {
        indices = (LPTX_idtype *)malloc(sizeof(LPTX_idtype) * nindices);
        if (!indices)
          r = LPTX_false;

        condc = (LPTX_bool *)malloc(sizeof(LPTX_bool) * ninp);
        if (!condc)
          r = LPTX_false;
      }
    }
#ifdef _OPENMP
#pragma omp barrier
#pragma omp atomic read
#endif
    lr = r;
    if (!lr)
      break;

    if (indices) {
      LPTX_idtype lno, lns;
      lno = 0;
      lns = 0;
      for (LPTX_idtype i = is; i < ie; ++i) {
        LPTX_bool used;
        LPTX_bool cond_res;

        cond_res = LPTX_false;
        used = LPTX_particle_is_used(&set->particles[i].base);

        if (used)
          cond_res = cond(&set->particles[i], arg);
        condc[i] = cond_res;

        if (used) {
          if (cond_res) {
            if (outp)
              lno += 1;
          } else if (used) {
            if (stripped)
              lns += 1;
          }
        }
      }

      indices[it] = lno;
      indices[it + nt] = lns;

      if (outp) {
#ifdef _OPENMP
#pragma omp atomic update
#endif
        nout += lno;
      }

      if (stripped) {
#ifdef _OPENMP
#pragma omp atomic update
#endif
        nstripped += lns;
      }

#ifdef _OPENMP
#pragma omp barrier
#pragma omp master
#endif
      {
        if (outp) {
          toutp = LPTX_particle_set_replicate(nout, set);
          if (!toutp)
            r = LPTX_false;
        }

        if (stripped) {
          tstripped = LPTX_particle_set_replicate(nstripped, set);
          if (!tstripped)
            r = LPTX_false;
        }
      }

#ifdef _OPENMP
#pragma omp barrier
#pragma omp atomic read
#endif
      lr = r;
      if (!lr)
        break;

      lno = 0;
      lns = 0;
      for (int i = 0; i < it; ++i) {
        lno += indices[i];
        lns += indices[i + nt];
      }

      for (LPTX_idtype i = is; i < ie; ++i) {
        if (!LPTX_particle_is_used(&set->particles[i].base))
          continue;

        if (condc[i]) {
          if (toutp)
            LPTX_particle_copy(&toutp->particles[lno++], &set->particles[i]);

          if (!stripped)
            LPTX_particle_set_used(&set->particles[i].base, LPTX_false);
        } else {
          if (tstripped)
            LPTX_particle_copy(&tstripped->particles[lns++],
                               &set->particles[i]);
        }
      }

    } else {
      /* Inplace with no allocation */
      for (LPTX_idtype i = is; i < ie; ++i) {
        if (!LPTX_particle_is_used(&set->particles[i].base))
          continue;

        if (cond(&set->particles[i], arg))
          LPTX_particle_set_used(&set->particles[i].base, LPTX_false);
      }
    }

  } while (0);

  if (r) {
    if (outp && toutp) {
      *outp = toutp;
      toutp = NULL;
    }

    if (stripped && tstripped) {
      *stripped = tstripped;
      tstripped = NULL;
    }
  }

  if (indices)
    free(indices);
  if (condc)
    free(condc);
  if (toutp)
    LPTX_particle_set_delete(toutp);
  if (tstripped)
    LPTX_particle_set_delete(tstripped);
  return r;
}

const LPTX_particle *
LPTX_particle_set_get_particle_at(LPTX_particle_set *set, LPTX_idtype index,
                                  const LPTX_particle_vector **const vectors,
                                  LPTX_idtype *number_of_vectors)
{
  if (index < 0 || index >= LPTX_particle_set_number_of_particles(set))
    return NULL;

  if (vectors)
    *vectors = set->particles[index].vectors;
  if (number_of_vectors)
    *number_of_vectors = set->number_vectors;
  return &set->particles[index].base;
}

void LPTX_particle_set_set_particle_at(LPTX_particle_set *set,
                                       LPTX_idtype index,
                                       const LPTX_particle *p,
                                       LPTX_idtype number_of_vectors,
                                       const LPTX_particle_vectors *vector_vars,
                                       const LPTX_particle_vector *vectors)
{
  LPTX_particle_data *pd;
  pd = &set->particles[index];

  if (p)
    pd->base = *p;

  for (LPTX_idtype jj = 0; jj < number_of_vectors; ++jj) {
    const LPTX_particle_vector *cdest;
    cdest = &pd->vectors[vector_vars[jj]];
    LPTX_particle_vector_copy((LPTX_particle_vector *)cdest, &vectors[jj]);
  }
}

LPTX_idtype LPTX_particle_set_get_particle_id_at(LPTX_particle_set *set,
                                                 LPTX_idtype index)
{
  return set->particles[index].base.particle_id;
}

void LPTX_particle_set_set_particle_id_at(LPTX_particle_set *set,
                                          LPTX_idtype index, LPTX_idtype value)
{
  set->particles[index].base.particle_id = value;
}

//---- bulk property setter and getter to plain array

#undef LPTX_DECL_SETGET_DECL
#define LPTX_DECL_SETGET_DECL

/* set/get particle id */
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_particle_id, particle_id)
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_particle_id, particle_id)
#define LPTX_get_particle_id() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_particle_id)
#define LPTX_set_particle_id() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_particle_id)
#define LPTX_dtype_LPTX_set_particle_id() LPTX_PTMEM_SET_S_DTYPE(particle_id)
#define LPTX_dtype_LPTX_get_particle_id() LPTX_PTMEM_GET_S_DTYPE(particle_id)

LPTX_DECL_SET_FN(LPTX_particle_set_set_particle_id, SNGL, (LPTX_idtype, ids))
{
  return LPTX_SNGL_SCALAR_SET(ids, LPTX_array_set_spp(),
                              LPTX_set_particle_id());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_particle_id, SNGL, (LPTX_idtype, ids))
{
  return LPTX_SNGL_SCALAR_GET(ids, LPTX_array_get_spp(),
                              LPTX_get_particle_id());
}

LPTX_DECL_SET_FN(LPTX_set_particle_particle_id, PACK, (LPTX_idtype, ids))
{
  return LPTX_PACK_SCALAR_SET(ids, LPTX_array_set_spp(),
                              LPTX_set_particle_id());
}

LPTX_DECL_GET_FN(LPTX_get_particle_particle_id, PACK, (LPTX_idtype, ids))
{
  return LPTX_PACK_SCALAR_GET(ids, LPTX_array_get_spp(),
                              LPTX_get_particle_id());
}

/* set/get origin id */
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_origin_id, origin_id)
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_origin_id, origin_id)
#define LPTX_get_origin_id() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_origin_id)
#define LPTX_set_origin_id() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_origin_id)
#define LPTX_dtype_LPTX_set_origin_id() LPTX_PTMEM_SET_S_DTYPE(origin_id)
#define LPTX_dtype_LPTX_get_origin_id() LPTX_PTMEM_GET_S_DTYPE(origin_id)

LPTX_DECL_SET_FN(LPTX_particle_set_set_origin_id, SNGL, (LPTX_idtype, ids))
{
  return LPTX_SNGL_SCALAR_SET(ids, LPTX_array_set_spp(), LPTX_set_origin_id());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_origin_id, SNGL, (LPTX_idtype, ids))
{
  return LPTX_SNGL_SCALAR_GET(ids, LPTX_array_get_spp(), LPTX_get_origin_id());
}

LPTX_DECL_SET_FN(LPTX_set_particle_origin_id, PACK, (LPTX_idtype, ids))
{
  return LPTX_PACK_SCALAR_SET(ids, LPTX_array_set_spp(), LPTX_set_origin_id());
}

LPTX_DECL_GET_FN(LPTX_get_particle_origin_id, PACK, (LPTX_idtype, ids))
{
  return LPTX_PACK_SCALAR_GET(ids, LPTX_array_get_spp(), LPTX_get_origin_id());
}

/* set/get fluid velocity */
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_fluid_velocity, fluid_velocity)
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_fluid_velocity, fluid_velocity)
#define LPTX_get_fluid_velocity() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_fluid_velocity)
#define LPTX_set_fluid_velocity() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_fluid_velocity)
#define LPTX_dtype_LPTX_get_fluid_velocity() \
  LPTX_PTMEM_GET_S_DTYPE(fluid_velocity)
#define LPTX_dtype_LPTX_set_fluid_velocity() \
  LPTX_PTMEM_SET_S_DTYPE(fluid_velocity)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity, SNGL,
                 (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_SET(vector, LPTX_array_set_svv(),
                              LPTX_set_fluid_velocity());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_velocity, SNGL,
                 (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_GET(vector, LPTX_array_get_svv(),
                              LPTX_get_fluid_velocity());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_SET(vector, LPTX_array_set_svv(),
                              LPTX_set_fluid_velocity());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_velocity, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_GET(vector, LPTX_array_get_svv(),
                              LPTX_get_fluid_velocity());
}

LPTX_DEFINE_ARRAY_GET_V_PTMEM(LPTX_get_fluid_velocity_v, fluid_velocity)
LPTX_DEFINE_ARRAY_SET_V_PTMEM(LPTX_set_fluid_velocity_v, fluid_velocity)
#define LPTX_get_fluid_velocity_v() \
  LPTX_PARTICLE_GET_V_PTMEM_INIT(LPTX_get_fluid_velocity_v)
#define LPTX_set_fluid_velocity_v() \
  LPTX_PARTICLE_SET_V_PTMEM_INIT(LPTX_set_fluid_velocity_v)
#define LPTX_dtype_LPTX_get_fluid_velocity_v() \
  LPTX_PTMEM_GET_V_DTYPE(fluid_velocity)
#define LPTX_dtype_LPTX_set_fluid_velocity_v() \
  LPTX_PTMEM_SET_V_DTYPE(fluid_velocity)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_t_aos, SNGL,
                 (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvt(),
                              LPTX_set_fluid_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_velocity_t_aos, SNGL,
                 (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvt(),
                              LPTX_get_fluid_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_t_aos, PACK,
                 (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvt(),
                              LPTX_set_fluid_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_velocity_t_aos, PACK,
                 (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvt(),
                              LPTX_get_fluid_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_f_aos, SNGL,
                 (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvf(),
                              LPTX_set_fluid_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_velocity_f_aos, SNGL,
                 (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvf(),
                              LPTX_get_fluid_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_f_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvf(),
                              LPTX_set_fluid_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_velocity_f_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvf(),
                              LPTX_get_fluid_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_d_aos, SNGL,
                 (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvd(),
                              LPTX_set_fluid_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_velocity_d_aos, SNGL,
                 (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvd(),
                              LPTX_get_fluid_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_d_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvd(),
                              LPTX_set_fluid_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_velocity_d_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvd(),
                              LPTX_get_fluid_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_fluid_velocity_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_velocity_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_fluid_velocity_v(),
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_fluid_velocity_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_velocity_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_fluid_velocity_v(),
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_f_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_fluid_velocity_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_velocity_f_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_fluid_velocity_v(),
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_f_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_fluid_velocity_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_velocity_f_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_fluid_velocity_v(),
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_d_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_fluid_velocity_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_velocity_d_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_fluid_velocity_v(),
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_d_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_fluid_velocity_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_velocity_d_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_fluid_velocity_v(),
                              x, y, z);
}

// #define JUPITER_LPTX_EMULATE_LPT_BEHAVIOR

static LPTX_vector
LPTX_set_fluid_velocity_st(const LPTX_rectilinear_grid *grid,
                           const struct LPTX_fluid_rect_args *d,
                           LPTX_vector position, LPTX_idtype icfpt,
                           LPTX_idtype jcfpt, LPTX_idtype kcfpt, void *arg)
{
  const LPTX_rectilinear_vector *vect;
  LPTX_assert(d->number_of_scalars == 0);
  LPTX_assert(d->number_of_vectors == 1);

  vect = d->vectors[0];

#ifndef JUPITER_LPTX_EMULATE_LPT_BEHAVIOR
  return LPTX_interp_staggered_vector_linear(grid, vect, icfpt, jcfpt, kcfpt,
                                             position);
#else
  /* Noticed that LPT just the velocity at corner */
  return LPTX_interp_staggered_corner(grid, vect, icfpt, jcfpt, kcfpt);
#endif
}

LPTX_DEFINE_FLUID_CB_SET_V_PTMEM(LPTX_set_fluid_velocity_cb, fluid_velocity)

#define LPTX_set_fluid_velocity_cb(func, arg) \
  LPTX_PARTICLE_FLUID_CB_SET_V_INIT(LPTX_set_fluid_velocity_cb, func, arg)

#define LPTX_set_fluid_velocity_rect(velocity)                       \
  LPTX_PARTICLE_FLUID_RECT_CV_INIT(LPTX_set_fluid_velocity_cb, grid, \
                                   LPTX_FRECT_IV(velocity),          \
                                   LPTX_set_fluid_velocity_st, NULL)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_struct, SRECT,
                 (LPTX_rectilinear_vector, velocity))
{
  LPTX_SNGL_VECTOR_FLUID_CB_SET(LPTX_set_fluid_velocity_rect(velocity));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_struct, PRECT,
                 (LPTX_rectilinear_vector, velocity))
{
  LPTX_PACK_VECTOR_FLUID_CB_SET(LPTX_set_fluid_velocity_rect(velocity));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_t_struct, SRECTT, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  LPTX_particle_set_set_fluid_velocity_struct(set, LPTX_RECT_GRID_T(),
                                              LPTX_RECT_VECTOR_T(x, y, z));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_t_struct, PRECTT, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  LPTX_set_particle_fluid_velocity_struct(param, LPTX_RECT_GRID_T(),
                                          LPTX_RECT_VECTOR_T(x, y, z));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_f_struct, SRECTF, //
                 (float, x), (float, y), (float, z))
{
  LPTX_particle_set_set_fluid_velocity_struct(set, LPTX_RECT_GRID_F(),
                                              LPTX_RECT_VECTOR_F(x, y, z));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_f_struct, PRECTF, //
                 (float, x), (float, y), (float, z))
{
  LPTX_set_particle_fluid_velocity_struct(param, LPTX_RECT_GRID_F(),
                                          LPTX_RECT_VECTOR_F(x, y, z));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_d_struct, SRECTD,
                 (double, x), (double, y), (double, z))
{
  LPTX_particle_set_set_fluid_velocity_struct(set, LPTX_RECT_GRID_D(),
                                              LPTX_RECT_VECTOR_D(x, y, z));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_d_struct, PRECTD, //
                 (double, x), (double, y), (double, z))
{
  LPTX_set_particle_fluid_velocity_struct(param, LPTX_RECT_GRID_D(),
                                          LPTX_RECT_VECTOR_D(x, y, z));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_velocity_cb, SFLCV)
{
  LPTX_SNGL_VECTOR_FLUID_CB_SET(LPTX_set_fluid_velocity_cb(func, arg));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_velocity_cb, PFLCV)
{
  LPTX_PACK_VECTOR_FLUID_CB_SET(LPTX_set_fluid_velocity_cb(func, arg));
}

/* set/get fluid density */
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_fluid_density, fluid_density)
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_fluid_density, fluid_density)
#define LPTX_get_fluid_density() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_fluid_density)
#define LPTX_set_fluid_density() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_fluid_density)
#define LPTX_dtype_LPTX_set_fluid_density() \
  LPTX_PTMEM_SET_S_DTYPE(fluid_density)
#define LPTX_dtype_LPTX_get_fluid_density() \
  LPTX_PTMEM_GET_S_DTYPE(fluid_density)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_density, SNGL,
                 (LPTX_type, density))
{
  return LPTX_SNGL_SCALAR_SET(density, LPTX_array_set_stt(),
                              LPTX_set_fluid_density());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_density, SNGL,
                 (LPTX_type, density))
{
  return LPTX_SNGL_SCALAR_GET(density, LPTX_array_get_stt(),
                              LPTX_get_fluid_density());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_density, PACK, (LPTX_type, density))
{
  return LPTX_PACK_SCALAR_SET(density, LPTX_array_set_stt(),
                              LPTX_set_fluid_density());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_density, PACK, (LPTX_type, density))
{
  return LPTX_PACK_SCALAR_GET(density, LPTX_array_get_stt(),
                              LPTX_get_fluid_density());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_density_f, SNGL, (float, density))
{
  return LPTX_SNGL_SCALAR_SET(density, LPTX_array_set_stf(),
                              LPTX_set_fluid_density());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_density_f, SNGL, (float, density))
{
  return LPTX_SNGL_SCALAR_GET(density, LPTX_array_get_stf(),
                              LPTX_get_fluid_density());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_density_f, PACK, (float, density))
{
  return LPTX_PACK_SCALAR_SET(density, LPTX_array_set_stf(),
                              LPTX_set_fluid_density());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_density_f, PACK, (float, density))
{
  return LPTX_PACK_SCALAR_GET(density, LPTX_array_get_stf(),
                              LPTX_get_fluid_density());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_density_d, SNGL, (double, density))
{
  return LPTX_SNGL_SCALAR_SET(density, LPTX_array_set_std(),
                              LPTX_set_fluid_density());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_density_d, SNGL, (double, density))
{
  return LPTX_SNGL_SCALAR_GET(density, LPTX_array_get_std(),
                              LPTX_get_fluid_density());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_density_d, PACK, (double, density))
{
  return LPTX_PACK_SCALAR_SET(density, LPTX_array_set_std(),
                              LPTX_set_fluid_density());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_density_d, PACK, (double, density))
{
  return LPTX_PACK_SCALAR_GET(density, LPTX_array_get_std(),
                              LPTX_get_fluid_density());
}

static LPTX_type LPTX_set_fluid_density_st(const LPTX_rectilinear_grid *grid,
                                           const struct LPTX_fluid_rect_args *d,
                                           LPTX_vector position,
                                           LPTX_idtype icfpt, LPTX_idtype jcfpt,
                                           LPTX_idtype kcfpt, void *arg)
{
  LPTX_assert(d->number_of_vectors == 0);
  LPTX_assert(d->number_of_scalars == 1);

  /* no interpolation */
  return LPTX_rectilinear_scalar_at(d->scalars[0], icfpt, jcfpt, kcfpt);
}

LPTX_DEFINE_FLUID_CB_SET_S_PTMEM(LPTX_set_fluid_density_cb, fluid_density)

#define LPTX_set_fluid_density_cb(func, arg) \
  LPTX_PARTICLE_FLUID_CB_SET_S_INIT(LPTX_set_fluid_density_cb, func, arg)

#define LPTX_set_fluid_density_rect(density)                        \
  LPTX_PARTICLE_FLUID_RECT_CS_INIT(LPTX_set_fluid_density_cb, grid, \
                                   LPTX_FRECT_IS(density),          \
                                   LPTX_set_fluid_density_st, NULL)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_density_struct_s, SRECT,
                 (LPTX_rectilinear_scalar, density))
{
  LPTX_SNGL_SCALAR_FLUID_CB_SET(LPTX_set_fluid_density_rect(density));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_density_struct_s, PRECT,
                 (LPTX_rectilinear_scalar, density))
{
  LPTX_PACK_SCALAR_FLUID_CB_SET(LPTX_set_fluid_density_rect(density));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_density_struct, SRECTT,
                 (LPTX_type, density))
{
  LPTX_particle_set_set_fluid_density_struct_s(set, LPTX_RECT_GRID_T(),
                                               LPTX_RECT_SCALAR_T(density));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_density_struct, PRECTT,
                 (LPTX_type, density))
{
  LPTX_set_particle_fluid_density_struct_s(param, LPTX_RECT_GRID_T(),
                                           LPTX_RECT_SCALAR_T(density));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_density_f_struct, SRECTF,
                 (float, density))
{
  LPTX_particle_set_set_fluid_density_struct_s(set, LPTX_RECT_GRID_F(),
                                               LPTX_RECT_SCALAR_F(density));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_density_f_struct, PRECTF,
                 (float, density))
{
  LPTX_set_particle_fluid_density_struct_s(param, LPTX_RECT_GRID_F(),
                                           LPTX_RECT_SCALAR_F(density));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_density_d_struct, SRECTD,
                 (double, density))
{
  LPTX_particle_set_set_fluid_density_struct_s(set, LPTX_RECT_GRID_D(),
                                               LPTX_RECT_SCALAR_D(density));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_density_d_struct, PRECTD,
                 (double, density))
{
  LPTX_set_particle_fluid_density_struct_s(param, LPTX_RECT_GRID_D(),
                                           LPTX_RECT_SCALAR_D(density));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_density_cb, SFLCF)
{
  LPTX_SNGL_SCALAR_FLUID_CB_SET(LPTX_set_fluid_density_cb(func, arg));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_density_cb, PFLCF)
{
  LPTX_PACK_SCALAR_FLUID_CB_SET(LPTX_set_fluid_density_cb(func, arg));
}

/* set/get fluid viscosity */
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_fluid_viscosity, fluid_viscosity)
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_fluid_viscosity, fluid_viscosity)
#define LPTX_get_fluid_viscosity() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_fluid_viscosity)
#define LPTX_set_fluid_viscosity() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_fluid_viscosity)
#define LPTX_dtype_LPTX_set_fluid_viscosity() \
  LPTX_PTMEM_SET_S_DTYPE(fluid_viscosity)
#define LPTX_dtype_LPTX_get_fluid_viscosity() \
  LPTX_PTMEM_GET_S_DTYPE(fluid_viscosity)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_viscosity, SNGL,
                 (LPTX_type, viscosity))
{
  return LPTX_SNGL_SCALAR_SET(viscosity, LPTX_array_set_stt(),
                              LPTX_set_fluid_viscosity());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_viscosity, SNGL,
                 (LPTX_type, viscosity))
{
  return LPTX_SNGL_SCALAR_GET(viscosity, LPTX_array_get_stt(),
                              LPTX_get_fluid_viscosity());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_viscosity, PACK,
                 (LPTX_type, viscosity))
{
  return LPTX_PACK_SCALAR_SET(viscosity, LPTX_array_set_stt(),
                              LPTX_set_fluid_viscosity());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_viscosity, PACK,
                 (LPTX_type, viscosity))
{
  return LPTX_PACK_SCALAR_GET(viscosity, LPTX_array_get_stt(),
                              LPTX_get_fluid_viscosity());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_viscosity_f, SNGL,
                 (float, viscosity))
{
  return LPTX_SNGL_SCALAR_SET(viscosity, LPTX_array_set_stf(),
                              LPTX_set_fluid_viscosity());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_viscosity_f, SNGL,
                 (float, viscosity))
{
  return LPTX_SNGL_SCALAR_GET(viscosity, LPTX_array_get_stf(),
                              LPTX_get_fluid_viscosity());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_viscosity_f, PACK, (float, viscosity))
{
  return LPTX_PACK_SCALAR_SET(viscosity, LPTX_array_set_stf(),
                              LPTX_set_fluid_viscosity());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_viscosity_f, PACK, (float, viscosity))
{
  return LPTX_PACK_SCALAR_GET(viscosity, LPTX_array_get_stf(),
                              LPTX_get_fluid_viscosity());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_viscosity_d, SNGL,
                 (double, viscosity))
{
  return LPTX_SNGL_SCALAR_SET(viscosity, LPTX_array_set_std(),
                              LPTX_set_fluid_viscosity());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_viscosity_d, SNGL,
                 (double, viscosity))
{
  return LPTX_SNGL_SCALAR_GET(viscosity, LPTX_array_get_std(),
                              LPTX_get_fluid_viscosity());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_viscosity_d, PACK, (double, viscosity))
{
  return LPTX_PACK_SCALAR_SET(viscosity, LPTX_array_set_std(),
                              LPTX_set_fluid_viscosity());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_viscosity_d, PACK, (double, viscosity))
{
  return LPTX_PACK_SCALAR_GET(viscosity, LPTX_array_get_std(),
                              LPTX_get_fluid_viscosity());
}

static LPTX_type
LPTX_set_fluid_viscosity_st(const LPTX_rectilinear_grid *grid,
                            const struct LPTX_fluid_rect_args *d,
                            LPTX_vector position, LPTX_idtype icfpt,
                            LPTX_idtype jcfpt, LPTX_idtype kcfpt, void *arg)
{
  LPTX_assert(d->number_of_vectors == 0);
  LPTX_assert(d->number_of_scalars == 1);

  /* no interpolation */
  return LPTX_rectilinear_scalar_at(d->scalars[0], icfpt, jcfpt, kcfpt);
}

LPTX_DEFINE_FLUID_CB_SET_S_PTMEM(LPTX_set_fluid_viscosity_cb, fluid_viscosity)

#define LPTX_set_fluid_viscosity_cb(func, arg) \
  LPTX_PARTICLE_FLUID_CB_SET_S_INIT(LPTX_set_fluid_viscosity_cb, func, arg)

#define LPTX_set_fluid_viscosity_rect(viscosity)                      \
  LPTX_PARTICLE_FLUID_RECT_CS_INIT(LPTX_set_fluid_viscosity_cb, grid, \
                                   LPTX_FRECT_IS(viscosity),          \
                                   LPTX_set_fluid_viscosity_st, NULL)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_viscosity_struct_s, SRECT,
                 (LPTX_rectilinear_scalar, viscosity))
{
  LPTX_SNGL_SCALAR_FLUID_CB_SET(LPTX_set_fluid_viscosity_rect(viscosity));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_viscosity_struct_s, PRECT,
                 (LPTX_rectilinear_scalar, viscosity))
{
  LPTX_PACK_SCALAR_FLUID_CB_SET(LPTX_set_fluid_viscosity_rect(viscosity));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_viscosity_struct, SRECTT,
                 (LPTX_type, viscosity))
{
  LPTX_particle_set_set_fluid_viscosity_struct_s(set, LPTX_RECT_GRID_T(),
                                                 LPTX_RECT_SCALAR_T(viscosity));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_viscosity_struct, PRECTT,
                 (LPTX_type, viscosity))
{
  LPTX_set_particle_fluid_viscosity_struct_s(param, LPTX_RECT_GRID_T(),
                                             LPTX_RECT_SCALAR_T(viscosity));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_viscosity_f_struct, SRECTF,
                 (float, viscosity))
{
  LPTX_particle_set_set_fluid_viscosity_struct_s(set, LPTX_RECT_GRID_F(),
                                                 LPTX_RECT_SCALAR_F(viscosity));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_viscosity_f_struct, PRECTF,
                 (float, viscosity))
{
  LPTX_set_particle_fluid_viscosity_struct_s(param, LPTX_RECT_GRID_F(),
                                             LPTX_RECT_SCALAR_F(viscosity));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_viscosity_d_struct, SRECTD,
                 (double, viscosity))
{
  LPTX_particle_set_set_fluid_viscosity_struct_s(set, LPTX_RECT_GRID_D(),
                                                 LPTX_RECT_SCALAR_D(viscosity));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_viscosity_d_struct, PRECTD,
                 (double, viscosity))
{
  LPTX_set_particle_fluid_viscosity_struct_s(param, LPTX_RECT_GRID_D(),
                                             LPTX_RECT_SCALAR_D(viscosity));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_viscosity_cb, SFLCF)
{
  LPTX_SNGL_SCALAR_FLUID_CB_SET(LPTX_set_fluid_viscosity_cb(func, arg));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_viscosity_cb, PFLCF)
{
  LPTX_PACK_SCALAR_FLUID_CB_SET(LPTX_set_fluid_viscosity_cb(func, arg));
}

/* set/get fluid specific heat */
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_fluid_specific_heat, fluid_specific_heat)
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_fluid_specific_heat, fluid_specific_heat)
#define LPTX_get_fluid_specific_heat() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_fluid_specific_heat)
#define LPTX_set_fluid_specific_heat() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_fluid_specific_heat)
#define LPTX_dtype_LPTX_set_fluid_specific_heat() \
  LPTX_PTMEM_SET_S_DTYPE(fluid_specific_heat)
#define LPTX_dtype_LPTX_get_fluid_specific_heat() \
  LPTX_PTMEM_GET_S_DTYPE(fluid_specific_heat)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_specific_heat, SNGL,
                 (LPTX_type, specific_heat))
{
  return LPTX_SNGL_SCALAR_SET(specific_heat, LPTX_array_set_stt(),
                              LPTX_set_fluid_specific_heat());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_specific_heat, SNGL,
                 (LPTX_type, specific_heat))
{
  return LPTX_SNGL_SCALAR_GET(specific_heat, LPTX_array_get_stt(),
                              LPTX_get_fluid_specific_heat());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_specific_heat, PACK,
                 (LPTX_type, specific_heat))
{
  return LPTX_PACK_SCALAR_SET(specific_heat, LPTX_array_set_stt(),
                              LPTX_set_fluid_specific_heat());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_specific_heat, PACK,
                 (LPTX_type, specific_heat))
{
  return LPTX_PACK_SCALAR_GET(specific_heat, LPTX_array_get_stt(),
                              LPTX_get_fluid_specific_heat());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_specific_heat_f, SNGL,
                 (float, specific_heat))
{
  return LPTX_SNGL_SCALAR_SET(specific_heat, LPTX_array_set_stf(),
                              LPTX_set_fluid_specific_heat());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_specific_heat_f, SNGL,
                 (float, specific_heat))
{
  return LPTX_SNGL_SCALAR_GET(specific_heat, LPTX_array_get_stf(),
                              LPTX_get_fluid_specific_heat());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_specific_heat_f, PACK,
                 (float, specific_heat))
{
  return LPTX_PACK_SCALAR_SET(specific_heat, LPTX_array_set_stf(),
                              LPTX_set_fluid_specific_heat());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_specific_heat_f, PACK,
                 (float, specific_heat))
{
  return LPTX_PACK_SCALAR_GET(specific_heat, LPTX_array_get_stf(),
                              LPTX_get_fluid_specific_heat());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_specific_heat_d, SNGL,
                 (double, specific_heat))
{
  return LPTX_SNGL_SCALAR_SET(specific_heat, LPTX_array_set_std(),
                              LPTX_set_fluid_specific_heat());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_specific_heat_d, SNGL,
                 (double, specific_heat))
{
  return LPTX_SNGL_SCALAR_GET(specific_heat, LPTX_array_get_std(),
                              LPTX_get_fluid_specific_heat());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_specific_heat_d, PACK,
                 (double, specific_heat))
{
  return LPTX_PACK_SCALAR_SET(specific_heat, LPTX_array_set_std(),
                              LPTX_set_fluid_specific_heat());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_specific_heat_d, PACK,
                 (double, specific_heat))
{
  return LPTX_PACK_SCALAR_GET(specific_heat, LPTX_array_get_std(),
                              LPTX_get_fluid_specific_heat());
}

static LPTX_type
LPTX_set_fluid_specific_heat_st(const LPTX_rectilinear_grid *grid,
                                const struct LPTX_fluid_rect_args *d,
                                LPTX_vector position, LPTX_idtype icfpt,
                                LPTX_idtype jcfpt, LPTX_idtype kcfpt, void *arg)
{
  LPTX_assert(d->number_of_vectors == 0);
  LPTX_assert(d->number_of_scalars == 1);

  /* no interpolation */
  return LPTX_rectilinear_scalar_at(d->scalars[0], icfpt, jcfpt, kcfpt);
}

LPTX_DEFINE_FLUID_CB_SET_S_PTMEM(LPTX_set_fluid_specific_heat_cb,
                                 fluid_specific_heat)

#define LPTX_set_fluid_specific_heat_cb(func, arg) \
  LPTX_PARTICLE_FLUID_CB_SET_S_INIT(LPTX_set_fluid_specific_heat_cb, func, arg)

#define LPTX_set_fluid_specific_heat_rect(specific_heat)                  \
  LPTX_PARTICLE_FLUID_RECT_CS_INIT(LPTX_set_fluid_specific_heat_cb, grid, \
                                   LPTX_FRECT_IS(specific_heat),          \
                                   LPTX_set_fluid_specific_heat_st, NULL)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_specific_heat_struct_s, SRECT,
                 (LPTX_rectilinear_scalar, specific_heat))
{
  LPTX_SNGL_SCALAR_FLUID_CB_SET(
    LPTX_set_fluid_specific_heat_rect(specific_heat));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_specific_heat_struct_s, PRECT,
                 (LPTX_rectilinear_scalar, specific_heat))
{
  LPTX_PACK_SCALAR_FLUID_CB_SET(
    LPTX_set_fluid_specific_heat_rect(specific_heat));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_specific_heat_struct, SRECTT,
                 (LPTX_type, specific_heat))
{
  const LPTX_rectilinear_grid *grid = LPTX_RECT_GRID_T();
  const LPTX_rectilinear_scalar *cp = LPTX_RECT_SCALAR_T(specific_heat);
  LPTX_particle_set_set_fluid_specific_heat_struct_s(set, grid, cp);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_specific_heat_struct, PRECTT,
                 (LPTX_type, specific_heat))
{
  const LPTX_rectilinear_grid *grid = LPTX_RECT_GRID_T();
  const LPTX_rectilinear_scalar *cp = LPTX_RECT_SCALAR_T(specific_heat);
  LPTX_set_particle_fluid_specific_heat_struct_s(param, grid, cp);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_specific_heat_f_struct, SRECTF,
                 (float, specific_heat))
{
  const LPTX_rectilinear_grid *grid = LPTX_RECT_GRID_F();
  const LPTX_rectilinear_scalar *cp = LPTX_RECT_SCALAR_F(specific_heat);
  LPTX_particle_set_set_fluid_specific_heat_struct_s(set, grid, cp);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_specific_heat_f_struct, PRECTF,
                 (float, specific_heat))
{
  const LPTX_rectilinear_grid *grid = LPTX_RECT_GRID_F();
  const LPTX_rectilinear_scalar *cp = LPTX_RECT_SCALAR_F(specific_heat);
  LPTX_set_particle_fluid_specific_heat_struct_s(param, grid, cp);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_specific_heat_d_struct, SRECTD,
                 (double, specific_heat))
{
  const LPTX_rectilinear_grid *grid = LPTX_RECT_GRID_D();
  const LPTX_rectilinear_scalar *cp = LPTX_RECT_SCALAR_D(specific_heat);
  LPTX_particle_set_set_fluid_specific_heat_struct_s(set, grid, cp);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_specific_heat_d_struct, PRECTD,
                 (double, specific_heat))
{
  const LPTX_rectilinear_grid *grid = LPTX_RECT_GRID_D();
  const LPTX_rectilinear_scalar *cp = LPTX_RECT_SCALAR_D(specific_heat);
  LPTX_set_particle_fluid_specific_heat_struct_s(param, grid, cp);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_specific_heat_cb, SFLCF)
{
  LPTX_SNGL_SCALAR_FLUID_CB_SET(LPTX_set_fluid_specific_heat_cb(func, arg));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_specific_heat_cb, PFLCF)
{
  LPTX_PACK_SCALAR_FLUID_CB_SET(LPTX_set_fluid_specific_heat_cb(func, arg));
}

/* set/get fluid thermal conductivity */
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_fluid_thermal_conductivity,
                              fluid_thermal_conductivity)
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_fluid_thermal_conductivity,
                              fluid_thermal_conductivity)
#define LPTX_get_fluid_thermal_conductivity() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_fluid_thermal_conductivity)
#define LPTX_set_fluid_thermal_conductivity() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_fluid_thermal_conductivity)
#define LPTX_dtype_LPTX_get_fluid_thermal_conductivity() \
  LPTX_PTMEM_GET_S_DTYPE(fluid_thermal_conductivity)
#define LPTX_dtype_LPTX_set_fluid_thermal_conductivity() \
  LPTX_PTMEM_SET_S_DTYPE(fluid_thermal_conductivity)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_thermal_conductivity, SNGL,
                 (LPTX_type, thermal_conductivity))
{
  return LPTX_SNGL_SCALAR_SET(thermal_conductivity, LPTX_array_set_stt(),
                              LPTX_set_fluid_thermal_conductivity());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_thermal_conductivity, SNGL,
                 (LPTX_type, thermal_conductivity))
{
  return LPTX_SNGL_SCALAR_GET(thermal_conductivity, LPTX_array_get_stt(),
                              LPTX_get_fluid_thermal_conductivity());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_thermal_conductivity, PACK,
                 (LPTX_type, thermal_conductivity))
{
  return LPTX_PACK_SCALAR_SET(thermal_conductivity, LPTX_array_set_stt(),
                              LPTX_set_fluid_thermal_conductivity());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_thermal_conductivity, PACK,
                 (LPTX_type, thermal_conductivity))
{
  return LPTX_PACK_SCALAR_GET(thermal_conductivity, LPTX_array_get_stt(),
                              LPTX_get_fluid_thermal_conductivity());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_thermal_conductivity_f, SNGL,
                 (float, thermal_conductivity))
{
  return LPTX_SNGL_SCALAR_SET(thermal_conductivity, LPTX_array_set_stf(),
                              LPTX_set_fluid_thermal_conductivity());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_thermal_conductivity_f, SNGL,
                 (float, thermal_conductivity))
{
  return LPTX_SNGL_SCALAR_GET(thermal_conductivity, LPTX_array_get_stf(),
                              LPTX_get_fluid_thermal_conductivity());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_thermal_conductivity_f, PACK,
                 (float, thermal_conductivity))
{
  return LPTX_PACK_SCALAR_SET(thermal_conductivity, LPTX_array_set_stf(),
                              LPTX_set_fluid_thermal_conductivity());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_thermal_conductivity_f, PACK,
                 (float, thermal_conductivity))
{
  return LPTX_PACK_SCALAR_GET(thermal_conductivity, LPTX_array_get_stf(),
                              LPTX_get_fluid_thermal_conductivity());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_thermal_conductivity_d, SNGL,
                 (double, thermal_conductivity))
{
  return LPTX_SNGL_SCALAR_SET(thermal_conductivity, LPTX_array_set_std(),
                              LPTX_set_fluid_thermal_conductivity());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_thermal_conductivity_d, SNGL,
                 (double, thermal_conductivity))
{
  return LPTX_SNGL_SCALAR_GET(thermal_conductivity, LPTX_array_get_std(),
                              LPTX_get_fluid_thermal_conductivity());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_thermal_conductivity_d, PACK,
                 (double, thermal_conductivity))
{
  return LPTX_PACK_SCALAR_SET(thermal_conductivity, LPTX_array_set_std(),
                              LPTX_set_fluid_thermal_conductivity());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_thermal_conductivity_d, PACK,
                 (double, thermal_conductivity))
{
  return LPTX_PACK_SCALAR_GET(thermal_conductivity, LPTX_array_get_std(),
                              LPTX_get_fluid_thermal_conductivity());
}

static LPTX_type LPTX_set_fluid_thermal_conductivity_st(
  const LPTX_rectilinear_grid *grid, const struct LPTX_fluid_rect_args *d,
  LPTX_vector position, LPTX_idtype icfpt, LPTX_idtype jcfpt, LPTX_idtype kcfpt,
  void *arg)
{
  LPTX_assert(d->number_of_vectors == 0);
  LPTX_assert(d->number_of_scalars == 1);

  /* no interpolation */
  return LPTX_rectilinear_scalar_at(d->scalars[0], icfpt, jcfpt, kcfpt);
}

LPTX_DEFINE_FLUID_CB_SET_S_PTMEM(LPTX_set_fluid_thermal_conductivity_cb,
                                 fluid_thermal_conductivity)

#define LPTX_set_fluid_thermal_conductivity_cb(func, arg) \
  LPTX_PARTICLE_FLUID_CB_SET_S_INIT(LPTX_set_fluid_specific_heat_cb, func, arg)

#define LPTX_set_fluid_thermal_conductivity_rect(thermal_conductivity)        \
  LPTX_PARTICLE_FLUID_RECT_CS_INIT(LPTX_set_fluid_thermal_conductivity_cb,    \
                                   grid, LPTX_FRECT_IS(thermal_conductivity), \
                                   LPTX_set_fluid_thermal_conductivity_st,    \
                                   NULL)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_thermal_conductivity_struct_s,
                 SRECT, (LPTX_rectilinear_scalar, thermal_conductivity))
{
  LPTX_SNGL_SCALAR_FLUID_CB_SET(
    LPTX_set_fluid_thermal_conductivity_rect(thermal_conductivity));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_thermal_conductivity_struct_s, PRECT,
                 (LPTX_rectilinear_scalar, thermal_conductivity))
{
  LPTX_PACK_SCALAR_FLUID_CB_SET(
    LPTX_set_fluid_thermal_conductivity_rect(thermal_conductivity));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_thermal_conductivity_struct,
                 SRECTT, (LPTX_type, thermal_conductivity))
{
  LPTX_particle_set_set_fluid_thermal_conductivity_struct_s(
    set, LPTX_RECT_GRID_T(), LPTX_RECT_SCALAR_T(thermal_conductivity));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_thermal_conductivity_struct, PRECTT,
                 (LPTX_type, thermal_conductivity))
{
  LPTX_set_particle_fluid_thermal_conductivity_struct_s(
    param, LPTX_RECT_GRID_T(), LPTX_RECT_SCALAR_T(thermal_conductivity));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_thermal_conductivity_f_struct,
                 SRECTF, (float, thermal_conductivity))
{
  LPTX_particle_set_set_fluid_thermal_conductivity_struct_s(
    set, LPTX_RECT_GRID_F(), LPTX_RECT_SCALAR_F(thermal_conductivity));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_thermal_conductivity_f_struct, PRECTF,
                 (float, thermal_conductivity))
{
  LPTX_set_particle_fluid_thermal_conductivity_struct_s(
    param, LPTX_RECT_GRID_F(), LPTX_RECT_SCALAR_F(thermal_conductivity));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_thermal_conductivity_d_struct,
                 SRECTD, (double, thermal_conductivity))
{
  LPTX_particle_set_set_fluid_thermal_conductivity_struct_s(
    set, LPTX_RECT_GRID_D(), LPTX_RECT_SCALAR_D(thermal_conductivity));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_thermal_conductivity_d_struct, PRECTD,
                 (double, thermal_conductivity))
{
  LPTX_set_particle_fluid_thermal_conductivity_struct_s(
    param, LPTX_RECT_GRID_D(), LPTX_RECT_SCALAR_D(thermal_conductivity));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_thermal_conductivity_cb, SFLCF)
{
  LPTX_SNGL_SCALAR_FLUID_CB_SET(
    LPTX_set_fluid_thermal_conductivity_cb(func, arg));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_thermal_conductivity_cb, PFLCF)
{
  LPTX_PACK_SCALAR_FLUID_CB_SET(
    LPTX_set_fluid_thermal_conductivity_cb(func, arg));
}

/* set/get fluid tempretaure */
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_fluid_temperature, fluid_temperature)
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_fluid_temperature, fluid_temperature)
#define LPTX_get_fluid_temperature() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_fluid_temperature)
#define LPTX_set_fluid_temperature() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_fluid_temperature)
#define LPTX_dtype_LPTX_get_fluid_temperature() \
  LPTX_PTMEM_GET_S_DTYPE(fluid_temperature)
#define LPTX_dtype_LPTX_set_fluid_temperature() \
  LPTX_PTMEM_SET_S_DTYPE(fluid_temperature)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature, SNGL,
                 (LPTX_type, temperature))
{
  return LPTX_SNGL_SCALAR_SET(temperature, LPTX_array_set_stt(),
                              LPTX_set_fluid_temperature());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature, SNGL,
                 (LPTX_type, temperature))
{
  return LPTX_SNGL_SCALAR_GET(temperature, LPTX_array_get_stt(),
                              LPTX_get_fluid_temperature());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature, PACK,
                 (LPTX_type, temperature))
{
  return LPTX_PACK_SCALAR_SET(temperature, LPTX_array_set_stt(),
                              LPTX_set_fluid_temperature());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature, PACK,
                 (LPTX_type, temperature))
{
  return LPTX_PACK_SCALAR_GET(temperature, LPTX_array_get_stt(),
                              LPTX_get_fluid_temperature());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_f, SNGL,
                 (float, temperature))
{
  return LPTX_SNGL_SCALAR_SET(temperature, LPTX_array_set_stf(),
                              LPTX_set_fluid_temperature());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_f, SNGL,
                 (float, temperature))
{
  return LPTX_SNGL_SCALAR_GET(temperature, LPTX_array_get_stf(),
                              LPTX_get_fluid_temperature());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_f, PACK,
                 (float, temperature))
{
  return LPTX_PACK_SCALAR_SET(temperature, LPTX_array_set_stf(),
                              LPTX_set_fluid_temperature());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_f, PACK,
                 (float, temperature))
{
  return LPTX_PACK_SCALAR_GET(temperature, LPTX_array_get_stf(),
                              LPTX_get_fluid_temperature());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_d, SNGL,
                 (double, temperature))
{
  return LPTX_SNGL_SCALAR_SET(temperature, LPTX_array_set_std(),
                              LPTX_set_fluid_temperature());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_d, SNGL,
                 (double, temperature))
{
  return LPTX_SNGL_SCALAR_GET(temperature, LPTX_array_get_std(),
                              LPTX_get_fluid_temperature());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_d, PACK,
                 (double, temperature))
{
  return LPTX_PACK_SCALAR_SET(temperature, LPTX_array_set_std(),
                              LPTX_set_fluid_temperature());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_d, PACK,
                 (double, temperature))
{
  return LPTX_PACK_SCALAR_GET(temperature, LPTX_array_get_std(),
                              LPTX_get_fluid_temperature());
}

static LPTX_type
LPTX_set_fluid_temperature_st(const LPTX_rectilinear_grid *grid,
                              const struct LPTX_fluid_rect_args *d,
                              LPTX_vector position, LPTX_idtype icfpt,
                              LPTX_idtype jcfpt, LPTX_idtype kcfpt, void *arg)
{
  LPTX_assert(d->number_of_vectors == 0);
  LPTX_assert(d->number_of_scalars == 1 || d->number_of_scalars == 2);

  /* Currently thermal conductivity is ignored (just reserved) */
  return LPTX_interp_scalar_linear(grid, d->scalars[0], icfpt, jcfpt, kcfpt,
                                   position);
}

LPTX_DEFINE_FLUID_CB_SET_S_PTMEM(LPTX_set_fluid_temperature_cb,
                                 fluid_temperature)

#define LPTX_set_fluid_temperature_cb(func, arg) \
  LPTX_PARTICLE_FLUID_CB_SET_S_INIT(LPTX_set_fluid_temperature_cb, func, arg)

#define LPTX_set_fluid_temperature_rect_T(temperature)                  \
  LPTX_PARTICLE_FLUID_RECT_CS_INIT(LPTX_set_fluid_temperature_cb, grid, \
                                   LPTX_FRECT_IS(temperature),          \
                                   LPTX_set_fluid_temperature_st, NULL)

#define LPTX_set_fluid_temperature_rect_T_thc(temperature,              \
                                              thermal_conductivity)     \
  LPTX_PARTICLE_FLUID_RECT_CS_INIT(LPTX_set_fluid_temperature_cb, grid, \
                                   LPTX_FRECT_IS(temperature,           \
                                                 thermal_conductivity), \
                                   LPTX_set_fluid_temperature_st, NULL)

#define LPTX_set_fluid_temperature_rect(temperature, thermal_conductivity) \
  ((thermal_conductivity && thermal_conductivity->scalar)                  \
     ? LPTX_set_fluid_temperature_rect_T_thc(temperature,                  \
                                             thermal_conductivity)         \
     : LPTX_set_fluid_temperature_rect_T(temperature))

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_struct_s, SRECT,
                 (LPTX_rectilinear_scalar, temperature),
                 (LPTX_rectilinear_scalar, thermal_conductivity))
{
  LPTX_SNGL_SCALAR_FLUID_CB_SET(
    LPTX_set_fluid_temperature_rect(temperature, thermal_conductivity));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_struct_s, PRECT,
                 (LPTX_rectilinear_scalar, temperature),
                 (LPTX_rectilinear_scalar, thermal_conductivity))
{
  LPTX_PACK_SCALAR_FLUID_CB_SET(
    LPTX_set_fluid_temperature_rect(temperature, thermal_conductivity));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_struct, SRECTT,
                 (LPTX_type, temperature), (LPTX_type, thermal_conductivity))
{
  const LPTX_rectilinear_scalar *t, *thc;
  t = LPTX_RECT_SCALAR_T(temperature);
  thc = thermal_conductivity ? LPTX_RECT_SCALAR_T(thermal_conductivity) : NULL;
  LPTX_particle_set_set_fluid_temperature_struct_s(set, LPTX_RECT_GRID_T(), t,
                                                   thc);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_struct, PRECTT,
                 (LPTX_type, temperature), (LPTX_type, thermal_conductivity))
{
  const LPTX_rectilinear_scalar *t, *thc;
  t = LPTX_RECT_SCALAR_T(temperature);
  thc = thermal_conductivity ? LPTX_RECT_SCALAR_T(thermal_conductivity) : NULL;
  LPTX_set_particle_fluid_temperature_struct_s(param, LPTX_RECT_GRID_T(), t,
                                               thc);
}
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_f_struct, SRECTF,
                 (float, temperature), (float, thermal_conductivity))
{
  const LPTX_rectilinear_scalar *t, *thc;
  t = LPTX_RECT_SCALAR_F(temperature);
  thc = thermal_conductivity ? LPTX_RECT_SCALAR_F(thermal_conductivity) : NULL;
  LPTX_particle_set_set_fluid_temperature_struct_s(set, LPTX_RECT_GRID_F(), t,
                                                   thc);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_f_struct, PRECTF,
                 (float, temperature), (float, thermal_conductivity))
{
  const LPTX_rectilinear_scalar *t, *thc;
  t = LPTX_RECT_SCALAR_F(temperature);
  thc = thermal_conductivity ? LPTX_RECT_SCALAR_F(thermal_conductivity) : NULL;
  LPTX_set_particle_fluid_temperature_struct_s(param, LPTX_RECT_GRID_F(), t,
                                               thc);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_d_struct, SRECTD,
                 (double, temperature), (double, thermal_conductivity))
{
  const LPTX_rectilinear_scalar *t, *thc;
  t = LPTX_RECT_SCALAR_D(temperature);
  thc = thermal_conductivity ? LPTX_RECT_SCALAR_D(thermal_conductivity) : NULL;
  LPTX_particle_set_set_fluid_temperature_struct_s(set, LPTX_RECT_GRID_D(), t,
                                                   thc);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_d_struct, PRECTD,
                 (double, temperature), (double, thermal_conductivity))
{
  const LPTX_rectilinear_scalar *t, *thc;
  t = LPTX_RECT_SCALAR_D(temperature);
  thc = thermal_conductivity ? LPTX_RECT_SCALAR_D(thermal_conductivity) : NULL;
  LPTX_set_particle_fluid_temperature_struct_s(param, LPTX_RECT_GRID_D(), t,
                                               thc);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_cb, SFLCF)
{
  LPTX_SNGL_SCALAR_FLUID_CB_SET(LPTX_set_fluid_temperature_cb(func, arg));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_cb, PFLCF)
{
  LPTX_PACK_SCALAR_FLUID_CB_SET(LPTX_set_fluid_temperature_cb(func, arg));
}

/* set/get fluid temperature gradient */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_fluid_temperature_grad,
                              fluid_temperature_grad)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_fluid_temperature_grad,
                              fluid_temperature_grad)
#define LPTX_set_fluid_temperature_grad() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_fluid_temperature_grad)
#define LPTX_get_fluid_temperature_grad() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_fluid_temperature_grad)
#define LPTX_dtype_LPTX_set_fluid_temperature_grad() \
  LPTX_PTMEM_SET_S_DTYPE(fluid_temperature_grad)
#define LPTX_dtype_LPTX_get_fluid_temperature_grad() \
  LPTX_PTMEM_GET_S_DTYPE(fluid_temperature_grad)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad, SNGL,
                 (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_SET(vector, LPTX_array_set_svv(),
                              LPTX_set_fluid_temperature_grad());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_grad, SNGL,
                 (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_GET(vector, LPTX_array_get_svv(),
                              LPTX_get_fluid_temperature_grad());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad, PACK,
                 (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_SET(vector, LPTX_array_set_svv(),
                              LPTX_set_fluid_temperature_grad());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_grad, PACK,
                 (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_GET(vector, LPTX_array_get_svv(),
                              LPTX_get_fluid_temperature_grad());
}

LPTX_DEFINE_ARRAY_SET_V_PTMEM(LPTX_set_fluid_temperature_grad_v,
                              fluid_temperature_grad)
LPTX_DEFINE_ARRAY_GET_V_PTMEM(LPTX_get_fluid_temperature_grad_v,
                              fluid_temperature_grad)
#define LPTX_set_fluid_temperature_grad_v() \
  LPTX_PARTICLE_SET_V_PTMEM_INIT(LPTX_set_fluid_temperature_grad_v)
#define LPTX_get_fluid_temperature_grad_v() \
  LPTX_PARTICLE_GET_V_PTMEM_INIT(LPTX_get_fluid_temperature_grad_v)
#define LPTX_dtype_LPTX_set_fluid_temperature_grad_v() \
  LPTX_PTMEM_SET_V_DTYPE(fluid_temperature_grad)
#define LPTX_dtype_LPTX_get_fluid_temperature_grad_v() \
  LPTX_PTMEM_GET_V_DTYPE(fluid_temperature_grad)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_t_aos, SNGL,
                 (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvt(),
                              LPTX_set_fluid_temperature_grad_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_grad_t_aos, SNGL,
                 (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvt(),
                              LPTX_get_fluid_temperature_grad_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_t_aos, PACK,
                 (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvt(),
                              LPTX_set_fluid_temperature_grad_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_grad_t_aos, PACK,
                 (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvt(),
                              LPTX_get_fluid_temperature_grad_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_f_aos, SNGL,
                 (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvf(),
                              LPTX_set_fluid_temperature_grad_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_grad_f_aos, SNGL,
                 (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvf(),
                              LPTX_get_fluid_temperature_grad_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_f_aos, PACK,
                 (float, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvf(),
                              LPTX_set_fluid_temperature_grad_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_grad_f_aos, PACK,
                 (float, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvf(),
                              LPTX_get_fluid_temperature_grad_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_d_aos, SNGL,
                 (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvd(),
                              LPTX_set_fluid_temperature_grad_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_grad_d_aos, SNGL,
                 (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvd(),
                              LPTX_get_fluid_temperature_grad_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_d_aos, PACK,
                 (double, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvd(),
                              LPTX_set_fluid_temperature_grad_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_grad_d_aos, PACK,
                 (double, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvd(),
                              LPTX_get_fluid_temperature_grad_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvt(),
                              LPTX_set_fluid_temperature_grad_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_grad_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvt(),
                              LPTX_get_fluid_temperature_grad_v(), //
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvt(),
                              LPTX_set_fluid_temperature_grad_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_grad_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvt(),
                              LPTX_get_fluid_temperature_grad_v(), //
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_f_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvf(),
                              LPTX_set_fluid_temperature_grad_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_grad_f_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvf(),
                              LPTX_get_fluid_temperature_grad_v(), //
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_f_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvf(),
                              LPTX_set_fluid_temperature_grad_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_grad_f_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvf(),
                              LPTX_get_fluid_temperature_grad_v(), //
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_d_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvd(),
                              LPTX_set_fluid_temperature_grad_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_temperature_grad_d_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvd(),
                              LPTX_get_fluid_temperature_grad_v(), //
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_d_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvd(),
                              LPTX_set_fluid_temperature_grad_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_temperature_grad_d_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvd(),
                              LPTX_get_fluid_temperature_grad_v(), //
                              x, y, z);
}

static LPTX_vector LPTX_set_fluid_temperature_grad_vt(
  const LPTX_rectilinear_grid *grid, const struct LPTX_fluid_rect_args *d,
  LPTX_vector position, LPTX_idtype icfpt, LPTX_idtype jcfpt, LPTX_idtype kcfpt,
  void *arg)
{
  LPTX_assert(d->number_of_vectors == 1);
  LPTX_assert(d->number_of_scalars == 0);

  return LPTX_interp_centered_vector_linear(grid, d->vectors[0], icfpt, jcfpt,
                                            kcfpt, position);
}

LPTX_DEFINE_FLUID_CB_SET_V_PTMEM(LPTX_set_fluid_temperature_grad_cb,
                                 fluid_temperature_grad)

#define LPTX_set_fluid_temperature_grad_cb(func, arg)                         \
  LPTX_PARTICLE_FLUID_CB_SET_V_INIT(LPTX_set_fluid_temperature_grad_cb, func, \
                                    arg)

#define LPTX_set_fluid_temperature_grad_rect(temperature_grad)               \
  LPTX_PARTICLE_FLUID_RECT_CV_INIT(LPTX_set_fluid_temperature_grad_cb, grid, \
                                   LPTX_FRECT_IV(temperature_grad),          \
                                   LPTX_set_fluid_temperature_grad_vt, NULL)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_struct, SRECT,
                 (LPTX_rectilinear_vector, temperature_grad))
{
  LPTX_SNGL_VECTOR_FLUID_CB_SET(
    LPTX_set_fluid_temperature_grad_rect(temperature_grad));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_struct, PRECT,
                 (LPTX_rectilinear_vector, temperature_grad))
{
  LPTX_PACK_VECTOR_FLUID_CB_SET(
    LPTX_set_fluid_temperature_grad_rect(temperature_grad));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_t_struct, SRECTT,
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  const LPTX_rectilinear_grid *g = LPTX_RECT_GRID_T();
  const LPTX_rectilinear_vector *v = LPTX_RECT_VECTOR_T(x, y, z);
  LPTX_particle_set_set_fluid_temperature_grad_struct(set, g, v);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_t_struct, PRECTT,
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  const LPTX_rectilinear_grid *g = LPTX_RECT_GRID_T();
  const LPTX_rectilinear_vector *v = LPTX_RECT_VECTOR_T(x, y, z);
  LPTX_set_particle_fluid_temperature_grad_struct(param, g, v);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_f_struct, SRECTF,
                 (float, x), (float, y), (float, z))
{
  const LPTX_rectilinear_grid *g = LPTX_RECT_GRID_F();
  const LPTX_rectilinear_vector *v = LPTX_RECT_VECTOR_F(x, y, z);
  LPTX_particle_set_set_fluid_temperature_grad_struct(set, g, v);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_f_struct, PRECTF,
                 (float, x), (float, y), (float, z))
{
  const LPTX_rectilinear_grid *g = LPTX_RECT_GRID_F();
  const LPTX_rectilinear_vector *v = LPTX_RECT_VECTOR_F(x, y, z);
  LPTX_set_particle_fluid_temperature_grad_struct(param, g, v);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_d_struct, SRECTD,
                 (double, x), (double, y), (double, z))
{
  const LPTX_rectilinear_grid *g = LPTX_RECT_GRID_D();
  const LPTX_rectilinear_vector *v = LPTX_RECT_VECTOR_D(x, y, z);
  LPTX_particle_set_set_fluid_temperature_grad_struct(set, g, v);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_d_struct, PRECTD,
                 (double, x), (double, y), (double, z))
{
  const LPTX_rectilinear_grid *g = LPTX_RECT_GRID_D();
  const LPTX_rectilinear_vector *v = LPTX_RECT_VECTOR_D(x, y, z);
  LPTX_set_particle_fluid_temperature_grad_struct(param, g, v);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_temperature_grad_cb, SFLCV)
{
  LPTX_SNGL_VECTOR_FLUID_CB_SET(LPTX_set_fluid_temperature_grad_cb(func, arg));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_temperature_grad_cb, PFLCV)
{
  LPTX_PACK_VECTOR_FLUID_CB_SET(LPTX_set_fluid_temperature_grad_cb(func, arg));
}

/* set/get mean free path (of fluid) */
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_mean_free_path, mean_free_path)
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_mean_free_path, mean_free_path)
#define LPTX_get_mean_free_path() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_mean_free_path)
#define LPTX_set_mean_free_path() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_mean_free_path)
#define LPTX_dtype_LPTX_get_mean_free_path() \
  LPTX_PTMEM_GET_S_DTYPE(mean_free_path)
#define LPTX_dtype_LPTX_set_mean_free_path() \
  LPTX_PTMEM_SET_S_DTYPE(mean_free_path)

LPTX_DECL_SET_FN(LPTX_particle_set_set_mean_free_path, SNGL,
                 (LPTX_type, lambda))
{
  return LPTX_SNGL_SCALAR_SET(lambda, LPTX_array_set_stt(),
                              LPTX_set_mean_free_path());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_mean_free_path, SNGL,
                 (LPTX_type, lambda))
{
  return LPTX_SNGL_SCALAR_GET(lambda, LPTX_array_get_stt(),
                              LPTX_get_mean_free_path());
}

LPTX_DECL_SET_FN(LPTX_set_particle_mean_free_path, PACK, (LPTX_type, lambda))
{
  return LPTX_PACK_SCALAR_SET(lambda, LPTX_array_set_stt(),
                              LPTX_set_mean_free_path());
}

LPTX_DECL_GET_FN(LPTX_get_particle_mean_free_path, PACK, (LPTX_type, lambda))
{
  return LPTX_PACK_SCALAR_GET(lambda, LPTX_array_get_stt(),
                              LPTX_get_mean_free_path());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_mean_free_path_f, SNGL, (float, lambda))
{
  return LPTX_SNGL_SCALAR_SET(lambda, LPTX_array_set_stf(),
                              LPTX_set_mean_free_path());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_mean_free_path_f, SNGL, (float, lambda))
{
  return LPTX_SNGL_SCALAR_GET(lambda, LPTX_array_get_stf(),
                              LPTX_get_mean_free_path());
}

LPTX_DECL_SET_FN(LPTX_set_particle_mean_free_path_f, PACK, (float, lambda))
{
  return LPTX_PACK_SCALAR_SET(lambda, LPTX_array_set_stf(),
                              LPTX_set_mean_free_path());
}

LPTX_DECL_GET_FN(LPTX_get_particle_mean_free_path_f, PACK, (float, lambda))
{
  return LPTX_PACK_SCALAR_GET(lambda, LPTX_array_get_stf(),
                              LPTX_get_mean_free_path());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_mean_free_path_d, SNGL, (double, lambda))
{
  return LPTX_SNGL_SCALAR_SET(lambda, LPTX_array_set_std(),
                              LPTX_set_mean_free_path());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_mean_free_path_d, SNGL, (double, lambda))
{
  return LPTX_SNGL_SCALAR_GET(lambda, LPTX_array_get_std(),
                              LPTX_get_mean_free_path());
}

LPTX_DECL_SET_FN(LPTX_set_particle_mean_free_path_d, PACK, (double, lambda))
{
  return LPTX_PACK_SCALAR_SET(lambda, LPTX_array_set_std(),
                              LPTX_set_mean_free_path());
}

LPTX_DECL_GET_FN(LPTX_get_particle_mean_free_path_d, PACK, (double, lambda))
{
  return LPTX_PACK_SCALAR_GET(lambda, LPTX_array_get_std(),
                              LPTX_get_mean_free_path());
}

static LPTX_type
LPTX_set_mean_free_path_st(const LPTX_rectilinear_grid *grid,
                           const struct LPTX_fluid_rect_args *d,
                           LPTX_vector position, LPTX_idtype icfpt,
                           LPTX_idtype jcfpt, LPTX_idtype kcfpt, void *arg)
{
  LPTX_assert(d->number_of_vectors == 0);
  LPTX_assert(d->number_of_scalars == 1);

  return LPTX_interp_scalar_linear(grid, d->scalars[0], icfpt, jcfpt, kcfpt,
                                   position);
}

LPTX_DEFINE_FLUID_CB_SET_S_PTMEM(LPTX_set_mean_free_path_cb, mean_free_path)

#define LPTX_set_mean_free_path_cb(func, arg) \
  LPTX_PARTICLE_FLUID_CB_SET_S_INIT(LPTX_set_mean_free_path_cb, func, arg)

#define LPTX_set_mean_free_path_rect(lambda)                         \
  LPTX_PARTICLE_FLUID_RECT_CS_INIT(LPTX_set_mean_free_path_cb, grid, \
                                   LPTX_FRECT_IS(lambda),            \
                                   LPTX_set_mean_free_path_st, NULL)

LPTX_DECL_SET_FN(LPTX_particle_set_set_mean_free_path_struct_s, SRECT,
                 (LPTX_rectilinear_scalar, lambda))
{
  LPTX_SNGL_SCALAR_FLUID_CB_SET(LPTX_set_mean_free_path_rect(lambda));
}

LPTX_DECL_SET_FN(LPTX_set_particle_mean_free_path_struct_s, PRECT,
                 (LPTX_rectilinear_scalar, lambda))
{
  LPTX_PACK_SCALAR_FLUID_CB_SET(LPTX_set_mean_free_path_rect(lambda));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_mean_free_path_struct, SRECTT,
                 (LPTX_type, lambda))
{
  LPTX_particle_set_set_mean_free_path_struct_s(set, LPTX_RECT_GRID_T(),
                                                LPTX_RECT_SCALAR_T(lambda));
}

LPTX_DECL_SET_FN(LPTX_set_particle_mean_free_path_struct, PRECTT,
                 (LPTX_type, lambda))
{
  LPTX_set_particle_mean_free_path_struct_s(param, LPTX_RECT_GRID_T(),
                                            LPTX_RECT_SCALAR_T(lambda));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_mean_free_path_f_struct, SRECTF,
                 (float, lambda))
{
  LPTX_particle_set_set_mean_free_path_struct_s(set, LPTX_RECT_GRID_F(),
                                                LPTX_RECT_SCALAR_F(lambda));
}

LPTX_DECL_SET_FN(LPTX_set_particle_mean_free_path_f_struct, PRECTF,
                 (float, lambda))
{
  LPTX_set_particle_mean_free_path_struct_s(param, LPTX_RECT_GRID_F(),
                                            LPTX_RECT_SCALAR_F(lambda));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_mean_free_path_d_struct, SRECTD,
                 (double, lambda))
{
  LPTX_particle_set_set_mean_free_path_struct_s(set, LPTX_RECT_GRID_D(),
                                                LPTX_RECT_SCALAR_D(lambda));
}

LPTX_DECL_SET_FN(LPTX_set_particle_mean_free_path_d_struct, PRECTD,
                 (double, lambda))
{
  LPTX_set_particle_mean_free_path_struct_s(param, LPTX_RECT_GRID_D(),
                                            LPTX_RECT_SCALAR_D(lambda));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_mean_free_path_cb, SFLCF)
{
  LPTX_SNGL_SCALAR_FLUID_CB_SET(LPTX_set_mean_free_path_cb(func, arg));
}

LPTX_DECL_SET_FN(LPTX_set_particle_mean_free_path_cb, PFLCF)
{
  LPTX_PACK_SCALAR_FLUID_CB_SET(LPTX_set_mean_free_path_cb(func, arg));
}

/* set/get fluid molecular weight */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_fluid_molecular_weight,
                              fluid_molecular_weight)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_fluid_molecular_weight,
                              fluid_molecular_weight)
#define LPTX_set_fluid_molecular_weight() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_fluid_molecular_weight)
#define LPTX_get_fluid_molecular_weight() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_fluid_molecular_weight)
#define LPTX_dtype_LPTX_set_fluid_molecular_weight() \
  LPTX_PTMEM_SET_S_DTYPE(fluid_molecular_weight)
#define LPTX_dtype_LPTX_get_fluid_molecular_weight() \
  LPTX_PTMEM_GET_S_DTYPE(fluid_molecular_weight)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_molecular_weight, SNGL,
                 (LPTX_type, molecular_weight))
{
  return LPTX_SNGL_SCALAR_SET(molecular_weight, LPTX_array_set_stt(),
                              LPTX_set_fluid_molecular_weight());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_molecular_weight, SNGL,
                 (LPTX_type, molecular_weight))
{
  return LPTX_SNGL_SCALAR_GET(molecular_weight, LPTX_array_get_stt(),
                              LPTX_get_fluid_molecular_weight());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_molecular_weight, PACK,
                 (LPTX_type, molecular_weight))
{
  return LPTX_PACK_SCALAR_SET(molecular_weight, LPTX_array_set_stt(),
                              LPTX_set_fluid_molecular_weight());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_molecular_weight, PACK,
                 (LPTX_type, molecular_weight))
{
  return LPTX_PACK_SCALAR_GET(molecular_weight, LPTX_array_get_stt(),
                              LPTX_get_fluid_molecular_weight());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_molecular_weight_f, SNGL,
                 (float, molecular_weight))
{
  return LPTX_SNGL_SCALAR_SET(molecular_weight, LPTX_array_set_stf(),
                              LPTX_set_fluid_molecular_weight());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_molecular_weight_f, SNGL,
                 (float, molecular_weight))
{
  return LPTX_SNGL_SCALAR_GET(molecular_weight, LPTX_array_get_stf(),
                              LPTX_get_fluid_molecular_weight());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_molecular_weight_f, PACK,
                 (float, molecular_weight))
{
  return LPTX_PACK_SCALAR_SET(molecular_weight, LPTX_array_set_stf(),
                              LPTX_set_fluid_molecular_weight());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_molecular_weight_f, PACK,
                 (float, molecular_weight))
{
  return LPTX_PACK_SCALAR_GET(molecular_weight, LPTX_array_get_stf(),
                              LPTX_get_fluid_molecular_weight());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_molecular_weight_d, SNGL,
                 (double, molecular_weight))
{
  return LPTX_SNGL_SCALAR_SET(molecular_weight, LPTX_array_set_std(),
                              LPTX_set_fluid_molecular_weight());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fluid_molecular_weight_d, SNGL,
                 (double, molecular_weight))
{
  return LPTX_SNGL_SCALAR_GET(molecular_weight, LPTX_array_get_std(),
                              LPTX_get_fluid_molecular_weight());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_molecular_weight_d, PACK,
                 (double, molecular_weight))
{
  return LPTX_PACK_SCALAR_SET(molecular_weight, LPTX_array_set_std(),
                              LPTX_set_fluid_molecular_weight());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fluid_molecular_weight_d, PACK,
                 (double, molecular_weight))
{
  return LPTX_PACK_SCALAR_GET(molecular_weight, LPTX_array_get_std(),
                              LPTX_get_fluid_molecular_weight());
}

static LPTX_type LPTX_set_fluid_molecular_weight_st(
  const LPTX_rectilinear_grid *grid, const struct LPTX_fluid_rect_args *d,
  LPTX_vector position, LPTX_idtype icfpt, LPTX_idtype jcfpt, LPTX_idtype kcfpt,
  void *arg)
{
  LPTX_assert(d->number_of_vectors == 0);
  LPTX_assert(d->number_of_scalars == 1);

  return LPTX_interp_scalar_linear(grid, d->scalars[0], icfpt, jcfpt, kcfpt,
                                   position);
}

LPTX_DEFINE_FLUID_CB_SET_S_PTMEM(LPTX_set_fluid_molecular_weight_cb,
                                 fluid_molecular_weight)

#define LPTX_set_fluid_molecular_weight_cb(func, arg)                         \
  LPTX_PARTICLE_FLUID_CB_SET_S_INIT(LPTX_set_fluid_molecular_weight_cb, func, \
                                    arg)

#define LPTX_set_fluid_molecular_weight_rect(molecular_weight)               \
  LPTX_PARTICLE_FLUID_RECT_CS_INIT(LPTX_set_fluid_molecular_weight_cb, grid, \
                                   LPTX_FRECT_IS(molecular_weight),          \
                                   LPTX_set_fluid_molecular_weight_st, NULL)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_molecular_weight_struct_s, SRECT,
                 (LPTX_rectilinear_scalar, molecular_weight))
{
  LPTX_SNGL_SCALAR_FLUID_CB_SET(
    LPTX_set_fluid_molecular_weight_rect(molecular_weight));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_molecular_weight_struct_s, PRECT,
                 (LPTX_rectilinear_scalar, molecular_weight))
{
  LPTX_PACK_SCALAR_FLUID_CB_SET(
    LPTX_set_fluid_molecular_weight_rect(molecular_weight));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_molecular_weight_struct, SRECTT,
                 (LPTX_type, molecular_weight))
{
  const LPTX_rectilinear_grid *grid = LPTX_RECT_GRID_T();
  const LPTX_rectilinear_scalar *mw = LPTX_RECT_SCALAR_T(molecular_weight);
  LPTX_particle_set_set_fluid_molecular_weight_struct_s(set, grid, mw);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_molecular_weight_struct, PRECTT,
                 (LPTX_type, molecular_weight))
{
  const LPTX_rectilinear_grid *grid = LPTX_RECT_GRID_T();
  const LPTX_rectilinear_scalar *mw = LPTX_RECT_SCALAR_T(molecular_weight);
  LPTX_set_particle_fluid_molecular_weight_struct_s(param, grid, mw);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_molecular_weight_f_struct, SRECTF,
                 (float, molecular_weight))
{
  const LPTX_rectilinear_grid *grid = LPTX_RECT_GRID_F();
  const LPTX_rectilinear_scalar *mw = LPTX_RECT_SCALAR_F(molecular_weight);
  LPTX_particle_set_set_fluid_molecular_weight_struct_s(set, grid, mw);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_molecular_weight_f_struct, PRECTF,
                 (float, molecular_weight))
{
  const LPTX_rectilinear_grid *grid = LPTX_RECT_GRID_F();
  const LPTX_rectilinear_scalar *mw = LPTX_RECT_SCALAR_F(molecular_weight);
  LPTX_set_particle_fluid_molecular_weight_struct_s(param, grid, mw);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_molecular_weight_d_struct, SRECTD,
                 (double, molecular_weight))
{
  const LPTX_rectilinear_grid *grid = LPTX_RECT_GRID_D();
  const LPTX_rectilinear_scalar *mw = LPTX_RECT_SCALAR_D(molecular_weight);
  LPTX_particle_set_set_fluid_molecular_weight_struct_s(set, grid, mw);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_molecular_weight_d_struct, PRECTD,
                 (double, molecular_weight))
{
  const LPTX_rectilinear_grid *grid = LPTX_RECT_GRID_D();
  const LPTX_rectilinear_scalar *mw = LPTX_RECT_SCALAR_D(molecular_weight);
  LPTX_set_particle_fluid_molecular_weight_struct_s(param, grid, mw);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_molecular_weight_cb, SFLCF)
{
  LPTX_SNGL_SCALAR_FLUID_CB_SET(LPTX_set_fluid_molecular_weight_cb(func, arg));
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_molecular_weight_cb, PFLCF)
{
  LPTX_PACK_SCALAR_FLUID_CB_SET(LPTX_set_fluid_molecular_weight_cb(func, arg));
}

/* set/get particle position */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_position, position)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_position, position)
#define LPTX_set_position() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_position)
#define LPTX_get_position() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_position)
#define LPTX_dtype_LPTX_set_position() LPTX_PTMEM_SET_S_DTYPE(position)
#define LPTX_dtype_LPTX_get_position() LPTX_PTMEM_GET_S_DTYPE(position)

LPTX_DECL_SET_FN(LPTX_particle_set_set_position, SNGL, (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_SET(vector, LPTX_array_set_svv(),
                              LPTX_set_position());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_position, SNGL, (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_GET(vector, LPTX_array_get_svv(),
                              LPTX_get_position());
}

LPTX_DECL_SET_FN(LPTX_set_particle_position, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_SET(vector, LPTX_array_set_svv(),
                              LPTX_set_position());
}

LPTX_DECL_GET_FN(LPTX_get_particle_position, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_GET(vector, LPTX_array_get_svv(),
                              LPTX_get_position());
}

LPTX_DEFINE_ARRAY_SET_V_PTMEM(LPTX_set_position_v, position)
LPTX_DEFINE_ARRAY_GET_V_PTMEM(LPTX_get_position_v, position)
#define LPTX_set_position_v() \
  LPTX_PARTICLE_SET_V_PTMEM_INIT(LPTX_set_position_v)
#define LPTX_get_position_v() \
  LPTX_PARTICLE_GET_V_PTMEM_INIT(LPTX_get_position_v)
#define LPTX_dtype_LPTX_set_position_v() LPTX_PTMEM_SET_V_DTYPE(position)
#define LPTX_dtype_LPTX_get_position_v() LPTX_PTMEM_GET_V_DTYPE(position)

LPTX_DECL_SET_FN(LPTX_particle_set_set_position_t_aos, SNGL,
                 (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvt(),
                              LPTX_set_position_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_position_t_aos, SNGL,
                 (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvt(),
                              LPTX_get_position_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_position_t_aos, PACK, (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvt(),
                              LPTX_set_position_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_position_t_aos, PACK, (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvt(),
                              LPTX_get_position_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_position_f_aos, SNGL, (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvf(),
                              LPTX_set_position_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_position_f_aos, SNGL, (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvf(),
                              LPTX_get_position_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_position_f_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvf(),
                              LPTX_set_position_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_position_f_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvf(),
                              LPTX_get_position_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_position_d_aos, SNGL, (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvd(),
                              LPTX_set_position_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_position_d_aos, SNGL, (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvd(),
                              LPTX_get_position_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_position_d_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvd(),
                              LPTX_set_position_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_position_d_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvd(),
                              LPTX_get_position_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_position_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_position_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_position_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_position_v(), //
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_position_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_position_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_position_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_position_v(), //
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_position_f_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_position_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_position_f_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_position_v(), //
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_position_f_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_position_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_position_f_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_position_v(), //
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_position_d_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_position_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_position_d_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_position_v(), //
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_position_d_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_position_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_position_d_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_position_v(), //
                              x, y, z);
}

/* set/get particle velocity */
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_velocity, velocity)
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_velocity, velocity)
#define LPTX_set_velocity() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_velocity)
#define LPTX_get_velocity() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_velocity)
#define LPTX_dtype_LPTX_set_velocity() LPTX_PTMEM_SET_S_DTYPE(velocity)
#define LPTX_dtype_LPTX_get_velocity() LPTX_PTMEM_GET_S_DTYPE(velocity)

LPTX_DECL_SET_FN(LPTX_particle_set_set_velocity, SNGL, (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_SET(vector, LPTX_array_set_svv(),
                              LPTX_set_velocity());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_velocity, SNGL, (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_GET(vector, LPTX_array_get_svv(),
                              LPTX_get_velocity());
}

LPTX_DECL_SET_FN(LPTX_set_particle_velocity, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_SET(vector, LPTX_array_set_svv(),
                              LPTX_set_velocity());
}

LPTX_DECL_GET_FN(LPTX_get_particle_velocity, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_GET(vector, LPTX_array_get_svv(),
                              LPTX_get_velocity());
}

LPTX_DEFINE_ARRAY_GET_V_PTMEM(LPTX_get_velocity_v, velocity)
LPTX_DEFINE_ARRAY_SET_V_PTMEM(LPTX_set_velocity_v, velocity)
#define LPTX_set_velocity_v() \
  LPTX_PARTICLE_SET_V_PTMEM_INIT(LPTX_set_velocity_v)
#define LPTX_get_velocity_v() \
  LPTX_PARTICLE_GET_V_PTMEM_INIT(LPTX_get_velocity_v)
#define LPTX_dtype_LPTX_set_velocity_v() LPTX_PTMEM_SET_V_DTYPE(velocity)
#define LPTX_dtype_LPTX_get_velocity_v() LPTX_PTMEM_GET_V_DTYPE(velocity)

LPTX_DECL_SET_FN(LPTX_particle_set_set_velocity_t_aos, SNGL,
                 (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvt(),
                              LPTX_set_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_velocity_t_aos, SNGL,
                 (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvt(),
                              LPTX_get_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_velocity_t_aos, PACK, (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvt(),
                              LPTX_set_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_velocity_t_aos, PACK, (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvt(),
                              LPTX_get_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_velocity_f_aos, SNGL, (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvf(),
                              LPTX_set_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_velocity_f_aos, SNGL, (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvf(),
                              LPTX_get_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_velocity_f_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvf(),
                              LPTX_set_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_velocity_f_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvf(),
                              LPTX_get_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_velocity_d_aos, SNGL, (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvd(),
                              LPTX_set_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_velocity_d_aos, SNGL, (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvd(),
                              LPTX_get_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_velocity_d_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvd(),
                              LPTX_set_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_velocity_d_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvd(),
                              LPTX_get_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_velocity_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_velocity_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_velocity_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_velocity_v(), //
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_velocity_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_velocity_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_velocity_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_velocity_v(), //
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_velocity_f_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_velocity_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_velocity_f_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_velocity_v(), //
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_velocity_f_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_velocity_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_velocity_f_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_velocity_v(), //
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_velocity_d_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_velocity_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_velocity_d_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_velocity_v(), //
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_velocity_d_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_velocity_v(), //
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_velocity_d_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_velocity_v(), //
                              x, y, z);
}

/* set/get particle current time */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_current_time, current_time)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_current_time, current_time)
#define LPTX_set_current_time() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_current_time)
#define LPTX_get_current_time() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_current_time)
#define LPTX_dtype_LPTX_set_current_time() LPTX_PTMEM_SET_S_DTYPE(current_time)
#define LPTX_dtype_LPTX_get_current_time() LPTX_PTMEM_GET_S_DTYPE(current_time)

LPTX_DECL_SET_FN(LPTX_particle_set_set_current_time, SNGL, (LPTX_type, time))
{
  return LPTX_SNGL_SCALAR_SET(time, LPTX_array_set_stt(),
                              LPTX_set_current_time());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_current_time, SNGL, (LPTX_type, time))
{
  return LPTX_SNGL_SCALAR_GET(time, LPTX_array_get_stt(),
                              LPTX_get_current_time());
}

LPTX_DECL_SET_FN(LPTX_set_particle_current_time, PACK, (LPTX_type, time))
{
  return LPTX_PACK_SCALAR_SET(time, LPTX_array_set_stt(),
                              LPTX_set_current_time());
}

LPTX_DECL_GET_FN(LPTX_get_particle_current_time, PACK, (LPTX_type, time))
{
  return LPTX_PACK_SCALAR_GET(time, LPTX_array_get_stt(),
                              LPTX_get_current_time());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_current_time_f, SNGL, (float, time))
{
  return LPTX_SNGL_SCALAR_SET(time, LPTX_array_set_stf(),
                              LPTX_set_current_time());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_current_time_f, SNGL, (float, time))
{
  return LPTX_SNGL_SCALAR_GET(time, LPTX_array_get_stf(),
                              LPTX_get_current_time());
}

LPTX_DECL_SET_FN(LPTX_set_particle_current_time_f, PACK, (float, time))
{
  return LPTX_PACK_SCALAR_SET(time, LPTX_array_set_stf(),
                              LPTX_set_current_time());
}

LPTX_DECL_GET_FN(LPTX_get_particle_current_time_f, PACK, (float, time))
{
  return LPTX_PACK_SCALAR_GET(time, LPTX_array_get_stf(),
                              LPTX_get_current_time());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_current_time_d, SNGL, (double, time))
{
  return LPTX_SNGL_SCALAR_SET(time, LPTX_array_set_std(),
                              LPTX_set_current_time());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_current_time_d, SNGL, (double, time))
{
  return LPTX_SNGL_SCALAR_GET(time, LPTX_array_get_std(),
                              LPTX_get_current_time());
}

LPTX_DECL_SET_FN(LPTX_set_particle_current_time_d, PACK, (double, time))
{
  return LPTX_PACK_SCALAR_SET(time, LPTX_array_set_std(),
                              LPTX_set_current_time());
}

LPTX_DECL_GET_FN(LPTX_get_particle_current_time_d, PACK, (double, time))
{
  return LPTX_PACK_SCALAR_GET(time, LPTX_array_get_std(),
                              LPTX_get_current_time());
}

/* set/get particle start time */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_start_time, start_time)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_start_time, start_time)
#define LPTX_set_start_time() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_start_time)
#define LPTX_get_start_time() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_start_time)
#define LPTX_dtype_LPTX_set_start_time() LPTX_PTMEM_SET_S_DTYPE(start_time)
#define LPTX_dtype_LPTX_get_start_time() LPTX_PTMEM_GET_S_DTYPE(start_time)

LPTX_DECL_SET_FN(LPTX_particle_set_set_start_time, SNGL, (LPTX_type, time))
{
  return LPTX_SNGL_SCALAR_SET(time, LPTX_array_set_stt(),
                              LPTX_set_start_time());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_start_time, SNGL, (LPTX_type, time))
{
  return LPTX_SNGL_SCALAR_GET(time, LPTX_array_get_stt(),
                              LPTX_get_start_time());
}

LPTX_DECL_SET_FN(LPTX_set_particle_start_time, PACK, (LPTX_type, time))
{
  return LPTX_PACK_SCALAR_SET(time, LPTX_array_set_stt(),
                              LPTX_set_start_time());
}

LPTX_DECL_GET_FN(LPTX_get_particle_start_time, PACK, (LPTX_type, time))
{
  return LPTX_PACK_SCALAR_GET(time, LPTX_array_get_stt(),
                              LPTX_get_start_time());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_start_time_f, SNGL, (float, time))
{
  return LPTX_SNGL_SCALAR_SET(time, LPTX_array_set_stf(),
                              LPTX_set_start_time());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_start_time_f, SNGL, (float, time))
{
  return LPTX_SNGL_SCALAR_GET(time, LPTX_array_get_stf(),
                              LPTX_get_start_time());
}

LPTX_DECL_SET_FN(LPTX_set_particle_start_time_f, PACK, (float, time))
{
  return LPTX_PACK_SCALAR_SET(time, LPTX_array_set_stf(),
                              LPTX_set_start_time());
}

LPTX_DECL_GET_FN(LPTX_get_particle_start_time_f, PACK, (float, time))
{
  return LPTX_PACK_SCALAR_GET(time, LPTX_array_get_stf(),
                              LPTX_get_start_time());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_start_time_d, SNGL, (double, time))
{
  return LPTX_SNGL_SCALAR_SET(time, LPTX_array_set_std(),
                              LPTX_set_start_time());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_start_time_d, SNGL, (double, time))
{
  return LPTX_SNGL_SCALAR_GET(time, LPTX_array_get_std(),
                              LPTX_get_start_time());
}

LPTX_DECL_SET_FN(LPTX_set_particle_start_time_d, PACK, (double, time))
{
  return LPTX_PACK_SCALAR_SET(time, LPTX_array_set_std(),
                              LPTX_set_start_time());
}

LPTX_DECL_GET_FN(LPTX_get_particle_start_time_d, PACK, (double, time))
{
  return LPTX_PACK_SCALAR_GET(time, LPTX_array_get_std(),
                              LPTX_get_start_time());
}

/* set/get particle density */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_density, density)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_density, density)
#define LPTX_set_density() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_density)
#define LPTX_get_density() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_density)
#define LPTX_dtype_LPTX_set_density() LPTX_PTMEM_SET_S_DTYPE(density)
#define LPTX_dtype_LPTX_get_density() LPTX_PTMEM_GET_S_DTYPE(density)

LPTX_DECL_SET_FN(LPTX_particle_set_set_density, SNGL, (LPTX_type, density))
{
  return LPTX_SNGL_SCALAR_SET(density, LPTX_array_set_stt(),
                              LPTX_set_density());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_density, SNGL, (LPTX_type, density))
{
  return LPTX_SNGL_SCALAR_GET(density, LPTX_array_get_stt(),
                              LPTX_get_density());
}

LPTX_DECL_SET_FN(LPTX_set_particle_density, PACK, (LPTX_type, density))
{
  return LPTX_PACK_SCALAR_SET(density, LPTX_array_set_stt(),
                              LPTX_set_density());
}

LPTX_DECL_GET_FN(LPTX_get_particle_density, PACK, (LPTX_type, density))
{
  return LPTX_PACK_SCALAR_GET(density, LPTX_array_get_stt(),
                              LPTX_get_density());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_density_f, SNGL, (float, density))
{
  return LPTX_SNGL_SCALAR_SET(density, LPTX_array_set_stf(),
                              LPTX_set_density());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_density_f, SNGL, (float, density))
{
  return LPTX_SNGL_SCALAR_GET(density, LPTX_array_get_stf(),
                              LPTX_get_density());
}

LPTX_DECL_SET_FN(LPTX_set_particle_density_f, PACK, (float, density))
{
  return LPTX_PACK_SCALAR_SET(density, LPTX_array_set_stf(),
                              LPTX_set_density());
}

LPTX_DECL_GET_FN(LPTX_get_particle_density_f, PACK, (float, density))
{
  return LPTX_PACK_SCALAR_GET(density, LPTX_array_get_stf(),
                              LPTX_get_density());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_density_d, SNGL, (double, density))
{
  return LPTX_SNGL_SCALAR_SET(density, LPTX_array_set_std(),
                              LPTX_set_density());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_density_d, SNGL, (double, density))
{
  return LPTX_SNGL_SCALAR_GET(density, LPTX_array_get_std(),
                              LPTX_get_density());
}

LPTX_DECL_SET_FN(LPTX_set_particle_density_d, PACK, (double, density))
{
  return LPTX_PACK_SCALAR_SET(density, LPTX_array_set_std(),
                              LPTX_set_density());
}

LPTX_DECL_GET_FN(LPTX_get_particle_density_d, PACK, (double, density))
{
  return LPTX_PACK_SCALAR_GET(density, LPTX_array_get_std(),
                              LPTX_get_density());
}

/* set/get particle speicfic heat */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_specific_heat, specific_heat)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_specific_heat, specific_heat)
#define LPTX_set_specific_heat() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_specific_heat)
#define LPTX_get_specific_heat() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_specific_heat)
#define LPTX_dtype_LPTX_set_specific_heat() \
  LPTX_PTMEM_SET_S_DTYPE(specific_heat)
#define LPTX_dtype_LPTX_get_specific_heat() \
  LPTX_PTMEM_GET_S_DTYPE(specific_heat)

LPTX_DECL_SET_FN(LPTX_particle_set_set_specific_heat, SNGL,
                 (LPTX_type, specific_heat))
{
  return LPTX_SNGL_SCALAR_SET(specific_heat, LPTX_array_set_stt(),
                              LPTX_set_specific_heat());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_specific_heat, SNGL,
                 (LPTX_type, specific_heat))
{
  return LPTX_SNGL_SCALAR_GET(specific_heat, LPTX_array_get_stt(),
                              LPTX_get_specific_heat());
}

LPTX_DECL_SET_FN(LPTX_set_particle_specific_heat, PACK,
                 (LPTX_type, specific_heat))
{
  return LPTX_PACK_SCALAR_SET(specific_heat, LPTX_array_set_stt(),
                              LPTX_set_specific_heat());
}

LPTX_DECL_GET_FN(LPTX_get_particle_specific_heat, PACK,
                 (LPTX_type, specific_heat))
{
  return LPTX_PACK_SCALAR_GET(specific_heat, LPTX_array_get_stt(),
                              LPTX_get_specific_heat());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_specific_heat_f, SNGL,
                 (float, specific_heat))
{
  return LPTX_SNGL_SCALAR_SET(specific_heat, LPTX_array_set_stf(),
                              LPTX_set_specific_heat());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_specific_heat_f, SNGL,
                 (float, specific_heat))
{
  return LPTX_SNGL_SCALAR_GET(specific_heat, LPTX_array_get_stf(),
                              LPTX_get_specific_heat());
}

LPTX_DECL_SET_FN(LPTX_set_particle_specific_heat_f, PACK,
                 (float, specific_heat))
{
  return LPTX_PACK_SCALAR_SET(specific_heat, LPTX_array_set_stf(),
                              LPTX_set_specific_heat());
}

LPTX_DECL_GET_FN(LPTX_get_particle_specific_heat_f, PACK,
                 (float, specific_heat))
{
  return LPTX_PACK_SCALAR_GET(specific_heat, LPTX_array_get_stf(),
                              LPTX_get_specific_heat());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_specific_heat_d, SNGL,
                 (double, specific_heat))
{
  return LPTX_SNGL_SCALAR_SET(specific_heat, LPTX_array_set_std(),
                              LPTX_set_specific_heat());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_specific_heat_d, SNGL,
                 (double, specific_heat))
{
  return LPTX_SNGL_SCALAR_GET(specific_heat, LPTX_array_get_std(),
                              LPTX_get_specific_heat());
}

LPTX_DECL_SET_FN(LPTX_set_particle_specific_heat_d, PACK,
                 (double, specific_heat))
{
  return LPTX_PACK_SCALAR_SET(specific_heat, LPTX_array_set_std(),
                              LPTX_set_specific_heat());
}

LPTX_DECL_GET_FN(LPTX_get_particle_specific_heat_d, PACK,
                 (double, specific_heat))
{
  return LPTX_PACK_SCALAR_GET(specific_heat, LPTX_array_get_std(),
                              LPTX_get_specific_heat());
}

/* set/get particle thermal conductivity */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_thermal_conductivity,
                              thermal_conductivity)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_thermal_conductivity,
                              thermal_conductivity)
#define LPTX_set_thermal_conductivity() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_thermal_conductivity)
#define LPTX_get_thermal_conductivity() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_thermal_conductivity)
#define LPTX_dtype_LPTX_set_thermal_conductivity() \
  LPTX_PTMEM_SET_S_DTYPE(thermal_conductivity)
#define LPTX_dtype_LPTX_get_thermal_conductivity() \
  LPTX_PTMEM_GET_S_DTYPE(thermal_conductivity)

LPTX_DECL_SET_FN(LPTX_particle_set_set_thermal_conductivity, SNGL,
                 (LPTX_type, thermal_conductivity))
{
  return LPTX_SNGL_SCALAR_SET(thermal_conductivity, LPTX_array_set_stt(),
                              LPTX_set_thermal_conductivity());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_thermal_conductivity, SNGL,
                 (LPTX_type, thermal_conductivity))
{
  return LPTX_SNGL_SCALAR_GET(thermal_conductivity, LPTX_array_get_stt(),
                              LPTX_get_thermal_conductivity());
}

LPTX_DECL_SET_FN(LPTX_set_particle_thermal_conductivity, PACK,
                 (LPTX_type, thermal_conductivity))
{
  return LPTX_PACK_SCALAR_SET(thermal_conductivity, LPTX_array_set_stt(),
                              LPTX_set_thermal_conductivity());
}

LPTX_DECL_GET_FN(LPTX_get_particle_thermal_conductivity, PACK,
                 (LPTX_type, thermal_conductivity))
{
  return LPTX_PACK_SCALAR_GET(thermal_conductivity, LPTX_array_get_stt(),
                              LPTX_get_thermal_conductivity());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_thermal_conductivity_f, SNGL,
                 (float, thermal_conductivity))
{
  return LPTX_SNGL_SCALAR_SET(thermal_conductivity, LPTX_array_set_stf(),
                              LPTX_set_thermal_conductivity());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_thermal_conductivity_f, SNGL,
                 (float, thermal_conductivity))
{
  return LPTX_SNGL_SCALAR_GET(thermal_conductivity, LPTX_array_get_stf(),
                              LPTX_get_thermal_conductivity());
}

LPTX_DECL_SET_FN(LPTX_set_particle_thermal_conductivity_f, PACK,
                 (float, thermal_conductivity))
{
  return LPTX_PACK_SCALAR_SET(thermal_conductivity, LPTX_array_set_stf(),
                              LPTX_set_thermal_conductivity());
}

LPTX_DECL_GET_FN(LPTX_get_particle_thermal_conductivity_f, PACK,
                 (float, thermal_conductivity))
{
  return LPTX_PACK_SCALAR_GET(thermal_conductivity, LPTX_array_get_stf(),
                              LPTX_get_thermal_conductivity());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_thermal_conductivity_d, SNGL,
                 (double, thermal_conductivity))
{
  return LPTX_SNGL_SCALAR_SET(thermal_conductivity, LPTX_array_set_std(),
                              LPTX_set_thermal_conductivity());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_thermal_conductivity_d, SNGL,
                 (double, thermal_conductivity))
{
  return LPTX_SNGL_SCALAR_GET(thermal_conductivity, LPTX_array_get_std(),
                              LPTX_get_thermal_conductivity());
}

LPTX_DECL_SET_FN(LPTX_set_particle_thermal_conductivity_d, PACK,
                 (double, thermal_conductivity))
{
  return LPTX_PACK_SCALAR_SET(thermal_conductivity, LPTX_array_set_std(),
                              LPTX_set_thermal_conductivity());
}

LPTX_DECL_GET_FN(LPTX_get_particle_thermal_conductivity_d, PACK,
                 (double, thermal_conductivity))
{
  return LPTX_PACK_SCALAR_GET(thermal_conductivity, LPTX_array_get_std(),
                              LPTX_get_thermal_conductivity());
}

/* set/get heat transfer rate */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_heat_transfer_rate, heat_transfer_rate)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_heat_transfer_rate, heat_transfer_rate)
#define LPTX_set_heat_transfer_rate() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_heat_transfer_rate)
#define LPTX_get_heat_transfer_rate() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_heat_transfer_rate)
#define LPTX_dtype_LPTX_set_heat_transfer_rate() \
  LPTX_PTMEM_SET_S_DTYPE(heat_transfer_rate)
#define LPTX_dtype_LPTX_get_heat_transfer_rate() \
  LPTX_PTMEM_GET_S_DTYPE(heat_transfer_rate)

LPTX_DECL_SET_FN(LPTX_particle_set_set_heat_transfer_rate, SNGL,
                 (LPTX_type, heat_transfer_rate))
{
  return LPTX_SNGL_SCALAR_SET(heat_transfer_rate, LPTX_array_set_stt(),
                              LPTX_set_heat_transfer_rate());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_heat_transfer_rate, SNGL,
                 (LPTX_type, heat_transfer_rate))
{
  return LPTX_SNGL_SCALAR_GET(heat_transfer_rate, LPTX_array_get_stt(),
                              LPTX_get_heat_transfer_rate());
}

LPTX_DECL_SET_FN(LPTX_set_particle_heat_transfer_rate, PACK,
                 (LPTX_type, heat_transfere_rate))
{
  return LPTX_PACK_SCALAR_SET(heat_transfere_rate, LPTX_array_set_stt(),
                              LPTX_set_heat_transfer_rate());
}

LPTX_DECL_GET_FN(LPTX_get_particle_heat_transfer_rate, PACK,
                 (LPTX_type, heat_transfer_rate))
{
  return LPTX_PACK_SCALAR_GET(heat_transfer_rate, LPTX_array_get_stt(),
                              LPTX_get_heat_transfer_rate());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_heat_transfer_rate_f, SNGL,
                 (float, heat_transfer_rate))
{
  return LPTX_SNGL_SCALAR_SET(heat_transfer_rate, LPTX_array_set_stf(),
                              LPTX_set_heat_transfer_rate());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_heat_transfer_rate_f, SNGL,
                 (float, heat_transfer_rate))
{
  return LPTX_SNGL_SCALAR_GET(heat_transfer_rate, LPTX_array_get_stf(),
                              LPTX_get_heat_transfer_rate());
}

LPTX_DECL_SET_FN(LPTX_set_particle_heat_transfer_rate_f, PACK,
                 (float, heat_transfere_rate))
{
  return LPTX_PACK_SCALAR_SET(heat_transfere_rate, LPTX_array_set_stf(),
                              LPTX_set_heat_transfer_rate());
}

LPTX_DECL_GET_FN(LPTX_get_particle_heat_transfer_rate_f, PACK,
                 (float, heat_transfer_rate))
{
  return LPTX_PACK_SCALAR_GET(heat_transfer_rate, LPTX_array_get_stf(),
                              LPTX_get_heat_transfer_rate());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_heat_transfer_rate_d, SNGL,
                 (double, heat_transfer_rate))
{
  return LPTX_SNGL_SCALAR_SET(heat_transfer_rate, LPTX_array_set_std(),
                              LPTX_set_heat_transfer_rate());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_heat_transfer_rate_d, SNGL,
                 (double, heat_transfer_rate))
{
  return LPTX_SNGL_SCALAR_GET(heat_transfer_rate, LPTX_array_get_std(),
                              LPTX_get_heat_transfer_rate());
}

LPTX_DECL_SET_FN(LPTX_set_particle_heat_transfer_rate_d, PACK,
                 (double, heat_transfere_rate))
{
  return LPTX_PACK_SCALAR_SET(heat_transfere_rate, LPTX_array_set_std(),
                              LPTX_set_heat_transfer_rate());
}

LPTX_DECL_GET_FN(LPTX_get_particle_heat_transfer_rate_d, PACK,
                 (double, heat_transfer_rate))
{
  return LPTX_PACK_SCALAR_GET(heat_transfer_rate, LPTX_array_get_std(),
                              LPTX_get_heat_transfer_rate());
}

/* set/get total heat transfer */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_total_heat_transfer, total_heat_transfer)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_total_heat_transfer, total_heat_transfer)
#define LPTX_set_total_heat_transfer() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_total_heat_transfer)
#define LPTX_get_total_heat_transfer() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_total_heat_transfer)
#define LPTX_dtype_LPTX_set_total_heat_transfer() \
  LPTX_PTMEM_SET_S_DTYPE(total_heat_transfer)
#define LPTX_dtype_LPTX_get_total_heat_transfer() \
  LPTX_PTMEM_GET_S_DTYPE(total_heat_transfer)

LPTX_DECL_SET_FN(LPTX_particle_set_set_total_heat_transfer, SNGL,
                 (LPTX_type, total_heat_transfer))
{
  return LPTX_SNGL_SCALAR_SET(total_heat_transfer, LPTX_array_set_stt(),
                              LPTX_set_total_heat_transfer());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_total_heat_transfer, SNGL,
                 (LPTX_type, total_heat_transfer))
{
  return LPTX_SNGL_SCALAR_GET(total_heat_transfer, LPTX_array_get_stt(),
                              LPTX_get_total_heat_transfer());
}

LPTX_DECL_SET_FN(LPTX_set_particle_total_heat_transfer, PACK,
                 (LPTX_type, total_heat_transfer))
{
  return LPTX_PACK_SCALAR_SET(total_heat_transfer, LPTX_array_set_stt(),
                              LPTX_set_total_heat_transfer());
}

LPTX_DECL_GET_FN(LPTX_get_particle_total_heat_transfer, PACK,
                 (LPTX_type, total_heat_transfer))
{
  return LPTX_PACK_SCALAR_GET(total_heat_transfer, LPTX_array_get_stt(),
                              LPTX_get_total_heat_transfer());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_total_heat_transfer_f, SNGL,
                 (float, total_heat_transfer))
{
  return LPTX_SNGL_SCALAR_SET(total_heat_transfer, LPTX_array_set_stf(),
                              LPTX_set_total_heat_transfer());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_total_heat_transfer_f, SNGL,
                 (float, total_heat_transfer))
{
  return LPTX_SNGL_SCALAR_GET(total_heat_transfer, LPTX_array_get_stf(),
                              LPTX_get_total_heat_transfer());
}

LPTX_DECL_SET_FN(LPTX_set_particle_total_heat_transfer_f, PACK,
                 (float, total_heat_transfer))
{
  return LPTX_PACK_SCALAR_SET(total_heat_transfer, LPTX_array_set_stf(),
                              LPTX_set_total_heat_transfer());
}

LPTX_DECL_GET_FN(LPTX_get_particle_total_heat_transfer_f, PACK,
                 (float, total_heat_transfer))
{
  return LPTX_PACK_SCALAR_GET(total_heat_transfer, LPTX_array_get_stf(),
                              LPTX_get_total_heat_transfer());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_total_heat_transfer_d, SNGL,
                 (double, total_heat_transfer))
{
  return LPTX_SNGL_SCALAR_SET(total_heat_transfer, LPTX_array_set_std(),
                              LPTX_set_total_heat_transfer());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_total_heat_transfer_d, SNGL,
                 (double, total_heat_transfer))
{
  return LPTX_SNGL_SCALAR_GET(total_heat_transfer, LPTX_array_get_std(),
                              LPTX_get_total_heat_transfer());
}

LPTX_DECL_SET_FN(LPTX_set_particle_total_heat_transfer_d, PACK,
                 (double, total_heat_transfer))
{
  return LPTX_PACK_SCALAR_SET(total_heat_transfer, LPTX_array_set_std(),
                              LPTX_set_total_heat_transfer());
}

LPTX_DECL_GET_FN(LPTX_get_particle_total_heat_transfer_d, PACK,
                 (double, total_heat_transfer))
{
  return LPTX_PACK_SCALAR_GET(total_heat_transfer, LPTX_array_get_std(),
                              LPTX_get_total_heat_transfer());
}

/* set/get particle diameter */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_diameter, diameter)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_diameter, diameter)
#define LPTX_set_diameter() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_diameter)
#define LPTX_get_diameter() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_diameter)
#define LPTX_dtype_LPTX_set_diameter() LPTX_PTMEM_SET_S_DTYPE(diameter)
#define LPTX_dtype_LPTX_get_diameter() LPTX_PTMEM_GET_S_DTYPE(diameter)

LPTX_DECL_SET_FN(LPTX_particle_set_set_diameter, SNGL, (LPTX_type, diameter))
{
  return LPTX_SNGL_SCALAR_SET(diameter, LPTX_array_set_stt(),
                              LPTX_set_diameter());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_diameter, SNGL, (LPTX_type, diameter))
{
  return LPTX_SNGL_SCALAR_GET(diameter, LPTX_array_get_stt(),
                              LPTX_get_diameter());
}

LPTX_DECL_SET_FN(LPTX_set_particle_diameter, PACK, (LPTX_type, diameter))
{
  return LPTX_PACK_SCALAR_SET(diameter, LPTX_array_set_stt(),
                              LPTX_set_diameter());
}

LPTX_DECL_GET_FN(LPTX_get_particle_diameter, PACK, (LPTX_type, diameter))
{
  return LPTX_PACK_SCALAR_GET(diameter, LPTX_array_get_stt(),
                              LPTX_get_diameter());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_diameter_f, SNGL, (float, diameter))
{
  return LPTX_SNGL_SCALAR_SET(diameter, LPTX_array_set_stf(),
                              LPTX_set_diameter());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_diameter_f, SNGL, (float, diameter))
{
  return LPTX_SNGL_SCALAR_GET(diameter, LPTX_array_get_stf(),
                              LPTX_get_diameter());
}

LPTX_DECL_SET_FN(LPTX_set_particle_diameter_f, PACK, (float, diameter))
{
  return LPTX_PACK_SCALAR_SET(diameter, LPTX_array_set_stf(),
                              LPTX_set_diameter());
}

LPTX_DECL_GET_FN(LPTX_get_particle_diameter_f, PACK, (float, diameter))
{
  return LPTX_PACK_SCALAR_GET(diameter, LPTX_array_get_stf(),
                              LPTX_get_diameter());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_diameter_d, SNGL, (double, diameter))
{
  return LPTX_SNGL_SCALAR_SET(diameter, LPTX_array_set_std(),
                              LPTX_set_diameter());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_diameter_d, SNGL, (double, diameter))
{
  return LPTX_SNGL_SCALAR_GET(diameter, LPTX_array_get_std(),
                              LPTX_get_diameter());
}

LPTX_DECL_SET_FN(LPTX_set_particle_diameter_d, PACK, (double, diameter))
{
  return LPTX_PACK_SCALAR_SET(diameter, LPTX_array_set_std(),
                              LPTX_set_diameter());
}

LPTX_DECL_GET_FN(LPTX_get_particle_diameter_d, PACK, (double, diameter))
{
  return LPTX_PACK_SCALAR_GET(diameter, LPTX_array_get_std(),
                              LPTX_get_diameter());
}

/* set/get particle fupt */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_fupt, fupt)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_fupt, fupt)
#define LPTX_set_fupt() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_fupt)
#define LPTX_get_fupt() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_fupt)
#define LPTX_dtype_LPTX_set_fupt() LPTX_PTMEM_SET_S_DTYPE(fupt)
#define LPTX_dtype_LPTX_get_fupt() LPTX_PTMEM_GET_S_DTYPE(fupt)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fupt, SNGL, (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_SET(vector, LPTX_array_set_svv(), LPTX_set_fupt());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fupt, SNGL, (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_GET(vector, LPTX_array_get_svv(), LPTX_get_fupt());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fupt, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_SET(vector, LPTX_array_set_svv(), LPTX_set_fupt());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fupt, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_GET(vector, LPTX_array_get_svv(), LPTX_get_fupt());
}

LPTX_DEFINE_ARRAY_SET_V_PTMEM(LPTX_set_fupt_v, fupt)
LPTX_DEFINE_ARRAY_GET_V_PTMEM(LPTX_get_fupt_v, fupt)
#define LPTX_set_fupt_v() LPTX_PARTICLE_SET_V_PTMEM_INIT(LPTX_set_fupt_v)
#define LPTX_get_fupt_v() LPTX_PARTICLE_GET_V_PTMEM_INIT(LPTX_get_fupt_v)
#define LPTX_dtype_LPTX_set_fupt_v() LPTX_PTMEM_SET_V_DTYPE(fupt)
#define LPTX_dtype_LPTX_get_fupt_v() LPTX_PTMEM_GET_V_DTYPE(fupt)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fuptt_aos, SNGL, (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvt(), LPTX_set_fupt_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fuptt_aos, SNGL, (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvt(), LPTX_get_fupt_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fuptt_aos, PACK, (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvt(), LPTX_set_fupt_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fuptt_aos, PACK, (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvt(), LPTX_get_fupt_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fuptf_aos, SNGL, (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvf(), LPTX_set_fupt_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fuptf_aos, SNGL, (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvf(), LPTX_get_fupt_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fuptf_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvf(), LPTX_set_fupt_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fuptf_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvf(), LPTX_get_fupt_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fuptd_aos, SNGL, (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvd(), LPTX_set_fupt_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fuptd_aos, SNGL, (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvd(), LPTX_get_fupt_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fuptd_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvd(), LPTX_set_fupt_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fuptd_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvd(), LPTX_get_fupt_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fuptt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_fupt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fuptt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_fupt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fuptt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_fupt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fuptt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_fupt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fuptf_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_fupt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fuptf_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_fupt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fuptf_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_fupt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fuptf_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_fupt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fuptd_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_fupt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fuptd_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_fupt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fuptd_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_fupt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fuptd_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_fupt_v(), x, y, z);
}

/* set/get particle fdut */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_fdut, fdut)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_fdut, fdut)
#define LPTX_set_fdut() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_fdut)
#define LPTX_get_fdut() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_fdut)
#define LPTX_dtype_LPTX_set_fdut() LPTX_PTMEM_SET_S_DTYPE(fdut)
#define LPTX_dtype_LPTX_get_fdut() LPTX_PTMEM_GET_S_DTYPE(fdut)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fdut, SNGL, (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_SET(vector, LPTX_array_set_svv(), LPTX_set_fdut());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fdut, SNGL, (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_GET(vector, LPTX_array_get_svv(), LPTX_get_fdut());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fdut, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_SET(vector, LPTX_array_set_svv(), LPTX_set_fdut());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fdut, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_GET(vector, LPTX_array_get_svv(), LPTX_get_fdut());
}

LPTX_DEFINE_ARRAY_SET_V_PTMEM(LPTX_set_fdut_v, fdut)
LPTX_DEFINE_ARRAY_GET_V_PTMEM(LPTX_get_fdut_v, fdut)
#define LPTX_set_fdut_v() LPTX_PARTICLE_SET_V_PTMEM_INIT(LPTX_set_fdut_v)
#define LPTX_get_fdut_v() LPTX_PARTICLE_GET_V_PTMEM_INIT(LPTX_get_fdut_v)
#define LPTX_dtype_LPTX_set_fdut_v() LPTX_PTMEM_SET_V_DTYPE(fdut)
#define LPTX_dtype_LPTX_get_fdut_v() LPTX_PTMEM_GET_V_DTYPE(fdut)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fdutt_aos, SNGL, (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvt(), LPTX_set_fdut_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fdutt_aos, SNGL, (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvt(), LPTX_get_fdut_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fdutt_aos, PACK, (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvt(), LPTX_set_fdut_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fdutt_aos, PACK, (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvt(), LPTX_get_fdut_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fdutf_aos, SNGL, (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvf(), LPTX_set_fdut_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fdutf_aos, SNGL, (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvf(), LPTX_get_fdut_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fdutf_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvf(), LPTX_set_fdut_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fdutf_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvf(), LPTX_get_fdut_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fdutd_aos, SNGL, (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvd(), LPTX_set_fdut_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fdutd_aos, SNGL, (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvd(), LPTX_get_fdut_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fdutd_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvd(), LPTX_set_fdut_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fdutd_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvd(), LPTX_get_fdut_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fdutt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_fdut_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fdutt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_fdut_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fdutt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_fdut_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fdutt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_fdut_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fdutf_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_fdut_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fdutf_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_fdut_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fdutf_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_fdut_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fdutf_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_fdut_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fdutd_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_fdut_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fdutd_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_fdut_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fdutd_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_fdut_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fdutd_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_fdut_v(), x, y, z);
}

/* set/get particle fxpt */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_fxpt, fxpt)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_fxpt, fxpt)
#define LPTX_set_fxpt() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_fxpt)
#define LPTX_get_fxpt() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_fxpt)
#define LPTX_dtype_LPTX_set_fxpt() LPTX_PTMEM_SET_S_DTYPE(fxpt)
#define LPTX_dtype_LPTX_get_fxpt() LPTX_PTMEM_GET_S_DTYPE(fxpt)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fxpt, SNGL, (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_SET(vector, LPTX_array_set_svv(), LPTX_set_fxpt());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fxpt, SNGL, (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_GET(vector, LPTX_array_get_svv(), LPTX_get_fxpt());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fxpt, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_SET(vector, LPTX_array_set_svv(), LPTX_set_fxpt());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fxpt, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_GET(vector, LPTX_array_get_svv(), LPTX_get_fxpt());
}

LPTX_DEFINE_ARRAY_SET_V_PTMEM(LPTX_set_fxpt_v, fxpt)
LPTX_DEFINE_ARRAY_GET_V_PTMEM(LPTX_get_fxpt_v, fxpt)
#define LPTX_set_fxpt_v() LPTX_PARTICLE_SET_V_PTMEM_INIT(LPTX_set_fxpt_v)
#define LPTX_get_fxpt_v() LPTX_PARTICLE_GET_V_PTMEM_INIT(LPTX_get_fxpt_v)
#define LPTX_dtype_LPTX_set_fxpt_v() LPTX_PTMEM_SET_V_DTYPE(fxpt)
#define LPTX_dtype_LPTX_get_fxpt_v() LPTX_PTMEM_GET_V_DTYPE(fxpt)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fxptt_aos, SNGL, (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvt(), LPTX_set_fxpt_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fxptt_aos, SNGL, (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvt(), LPTX_get_fxpt_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fxptt_aos, PACK, (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvt(), LPTX_set_fxpt_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fxptt_aos, PACK, (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvt(), LPTX_get_fxpt_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fxptf_aos, SNGL, (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvf(), LPTX_set_fxpt_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fxptf_aos, SNGL, (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvf(), LPTX_get_fxpt_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fxptf_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvf(), LPTX_set_fxpt_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fxptf_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvf(), LPTX_get_fxpt_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fxptd_aos, SNGL, (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvd(), LPTX_set_fxpt_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fxptd_aos, SNGL, (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvd(), LPTX_get_fxpt_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fxptd_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvd(), LPTX_set_fxpt_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fxptd_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvd(), LPTX_get_fxpt_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fxptt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_fxpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fxptt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_fxpt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fxptt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_fxpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fxptt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_fxpt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fxptf_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_fxpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fxptf_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_fxpt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fxptf_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_fxpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fxptf_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_fxpt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fxptd_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_fxpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fxptd_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_fxpt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fxptd_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_fxpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fxptd_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_fxpt_v(), x, y, z);
}

/* set/get particle dTdt */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_dTdt, dTdt)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_dTdt, dTdt)
#define LPTX_set_dTdt() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_dTdt)
#define LPTX_get_dTdt() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_dTdt)
#define LPTX_dtype_LPTX_set_dTdt() LPTX_PTMEM_SET_S_DTYPE(dTdt)
#define LPTX_dtype_LPTX_get_dTdt() LPTX_PTMEM_GET_S_DTYPE(dTdt)

LPTX_DECL_SET_FN(LPTX_particle_set_set_dTdt, SNGL, (LPTX_type, dTdt))
{
  return LPTX_SNGL_SCALAR_SET(dTdt, LPTX_array_set_stt(), LPTX_set_dTdt());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_dTdt, SNGL, (LPTX_type, dTdt))
{
  return LPTX_SNGL_SCALAR_GET(dTdt, LPTX_array_get_stt(), LPTX_get_dTdt());
}

LPTX_DECL_SET_FN(LPTX_set_particle_dTdt, PACK, (LPTX_type, dTdt))
{
  return LPTX_PACK_SCALAR_SET(dTdt, LPTX_array_set_stt(), LPTX_set_dTdt());
}

LPTX_DECL_GET_FN(LPTX_get_particle_dTdt, PACK, (LPTX_type, dTdt))
{
  return LPTX_PACK_SCALAR_GET(dTdt, LPTX_array_get_stt(), LPTX_get_dTdt());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_dTdtf, SNGL, (float, dTdt))
{
  return LPTX_SNGL_SCALAR_SET(dTdt, LPTX_array_set_stf(), LPTX_set_dTdt());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_dTdtf, SNGL, (float, dTdt))
{
  return LPTX_SNGL_SCALAR_GET(dTdt, LPTX_array_get_stf(), LPTX_get_dTdt());
}

LPTX_DECL_SET_FN(LPTX_set_particle_dTdtf, PACK, (float, dTdt))
{
  return LPTX_PACK_SCALAR_SET(dTdt, LPTX_array_set_stf(), LPTX_set_dTdt());
}

LPTX_DECL_GET_FN(LPTX_get_particle_dTdtf, PACK, (float, dTdt))
{
  return LPTX_PACK_SCALAR_GET(dTdt, LPTX_array_get_stf(), LPTX_get_dTdt());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_dTdtd, SNGL, (double, dTdt))
{
  return LPTX_SNGL_SCALAR_SET(dTdt, LPTX_array_set_std(), LPTX_set_dTdt());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_dTdtd, SNGL, (double, dTdt))
{
  return LPTX_SNGL_SCALAR_GET(dTdt, LPTX_array_get_std(), LPTX_get_dTdt());
}

LPTX_DECL_SET_FN(LPTX_set_particle_dTdtd, PACK, (double, dTdt))
{
  return LPTX_PACK_SCALAR_SET(dTdt, LPTX_array_set_std(), LPTX_set_dTdt());
}

LPTX_DECL_GET_FN(LPTX_get_particle_dTdtd, PACK, (double, dTdt))
{
  return LPTX_PACK_SCALAR_GET(dTdt, LPTX_array_get_std(), LPTX_get_dTdt());
}

/* set/get particle fbpt */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_fbpt, fbpt)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_fbpt, fbpt)
#define LPTX_set_fbpt() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_fbpt)
#define LPTX_get_fbpt() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_fbpt)
#define LPTX_dtype_LPTX_set_fbpt() LPTX_PTMEM_SET_S_DTYPE(fbpt)
#define LPTX_dtype_LPTX_get_fbpt() LPTX_PTMEM_GET_S_DTYPE(fbpt)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fbpt, SNGL, (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_SET(vector, LPTX_array_set_svv(), LPTX_set_fbpt());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fbpt, SNGL, (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_GET(vector, LPTX_array_get_svv(), LPTX_get_fbpt());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fbpt, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_SET(vector, LPTX_array_set_svv(), LPTX_set_fbpt());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fbpt, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_GET(vector, LPTX_array_get_svv(), LPTX_get_fbpt());
}

LPTX_DEFINE_ARRAY_SET_V_PTMEM(LPTX_set_fbpt_v, fbpt)
LPTX_DEFINE_ARRAY_GET_V_PTMEM(LPTX_get_fbpt_v, fbpt)
#define LPTX_set_fbpt_v() LPTX_PARTICLE_SET_V_PTMEM_INIT(LPTX_set_fbpt_v)
#define LPTX_get_fbpt_v() LPTX_PARTICLE_GET_V_PTMEM_INIT(LPTX_get_fbpt_v)
#define LPTX_dtype_LPTX_set_fbpt_v() LPTX_PTMEM_SET_V_DTYPE(fbpt)
#define LPTX_dtype_LPTX_get_fbpt_v() LPTX_PTMEM_GET_V_DTYPE(fbpt)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fbptt_aos, SNGL, (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvt(), LPTX_set_fbpt_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fbptt_aos, SNGL, (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvt(), LPTX_get_fbpt_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fbptt_aos, PACK, (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvt(), LPTX_set_fbpt_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fbptt_aos, PACK, (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvt(), LPTX_get_fbpt_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fbptf_aos, SNGL, (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvf(), LPTX_set_fbpt_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fbptf_aos, SNGL, (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvf(), LPTX_get_fbpt_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fbptf_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvf(), LPTX_set_fbpt_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fbptf_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvf(), LPTX_get_fbpt_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fbptd_aos, SNGL, (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvd(), LPTX_set_fbpt_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fbptd_aos, SNGL, (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvd(), LPTX_get_fbpt_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fbptd_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvd(), LPTX_set_fbpt_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fbptd_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvd(), LPTX_get_fbpt_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fbptt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_fbpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fbptt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_fbpt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fbptt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_fbpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fbptt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_fbpt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fbptf_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_fbpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fbptf_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_fbpt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fbptf_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_fbpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fbptf_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_fbpt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fbptd_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_fbpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fbptd_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_fbpt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fbptd_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_fbpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fbptd_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_fbpt_v(), x, y, z);
}

/* set/get particle fTpt */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_fTpt, fTpt)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_fTpt, fTpt)
#define LPTX_set_fTpt() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_fTpt)
#define LPTX_get_fTpt() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_fTpt)
#define LPTX_dtype_LPTX_set_fTpt() LPTX_PTMEM_SET_S_DTYPE(fTpt)
#define LPTX_dtype_LPTX_get_fTpt() LPTX_PTMEM_GET_S_DTYPE(fTpt)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fTpt, SNGL, (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_SET(vector, LPTX_array_set_svv(), LPTX_set_fTpt());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fTpt, SNGL, (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_GET(vector, LPTX_array_get_svv(), LPTX_get_fTpt());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fTpt, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_SET(vector, LPTX_array_set_svv(), LPTX_set_fTpt());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fTpt, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_GET(vector, LPTX_array_get_svv(), LPTX_get_fTpt());
}

LPTX_DEFINE_ARRAY_SET_V_PTMEM(LPTX_set_fTpt_v, fTpt)
LPTX_DEFINE_ARRAY_GET_V_PTMEM(LPTX_get_fTpt_v, fTpt)
#define LPTX_set_fTpt_v() LPTX_PARTICLE_SET_V_PTMEM_INIT(LPTX_set_fTpt_v)
#define LPTX_get_fTpt_v() LPTX_PARTICLE_GET_V_PTMEM_INIT(LPTX_get_fTpt_v)
#define LPTX_dtype_LPTX_set_fTpt_v() LPTX_PTMEM_SET_V_DTYPE(fTpt)
#define LPTX_dtype_LPTX_get_fTpt_v() LPTX_PTMEM_GET_V_DTYPE(fTpt)

LPTX_DECL_SET_FN(LPTX_particle_set_set_fTptt_aos, SNGL, (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvt(), LPTX_set_fTpt_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fTptt_aos, SNGL, (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvt(), LPTX_get_fTpt_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fTptt_aos, PACK, (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvt(), LPTX_set_fTpt_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fTptt_aos, PACK, (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvt(), LPTX_get_fTpt_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fTptf_aos, SNGL, (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvf(), LPTX_set_fTpt_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fTptf_aos, SNGL, (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvf(), LPTX_get_fTpt_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fTptf_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvf(), LPTX_set_fTpt_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fTptf_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvf(), LPTX_get_fTpt_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fTptd_aos, SNGL, (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvd(), LPTX_set_fTpt_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fTptd_aos, SNGL, (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvd(), LPTX_get_fTpt_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fTptd_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvd(), LPTX_set_fTpt_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_fTptd_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvd(), LPTX_get_fTpt_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fTptt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_fTpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fTptt_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_fTpt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fTptt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_fTpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fTptt_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_fTpt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fTptf_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_fTpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fTptf_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_fTpt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fTptf_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_fTpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fTptf_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_fTpt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fTptd_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_fTpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_fTptd_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_fTpt_v(), x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fTptd_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_fTpt_v(), x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_fTptd_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_fTpt_v(), x, y, z);
}

/* set/get particle dlpin (distance to interface; unused in LPTX) */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_dlpin, dlpin)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_dlpin, dlpin)
#define LPTX_set_dlpin() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_dlpin)
#define LPTX_get_dlpin() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_dlpin)
#define LPTX_dtype_LPTX_set_dlpin() LPTX_PTMEM_SET_S_DTYPE(dlpin)
#define LPTX_dtype_LPTX_get_dlpin() LPTX_PTMEM_GET_S_DTYPE(dlpin)

LPTX_DECL_SET_FN(LPTX_particle_set_set_dlpin, SNGL, (LPTX_type, dlpin))
{
  return LPTX_SNGL_SCALAR_SET(dlpin, LPTX_array_set_stt(), LPTX_set_dlpin());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_dlpin, SNGL, (LPTX_type, dlpin))
{
  return LPTX_SNGL_SCALAR_GET(dlpin, LPTX_array_get_stt(), LPTX_get_dlpin());
}

LPTX_DECL_SET_FN(LPTX_set_particle_dlpin, PACK, (LPTX_type, dlpin))
{
  return LPTX_PACK_SCALAR_SET(dlpin, LPTX_array_set_stt(), LPTX_set_dlpin());
}

LPTX_DECL_GET_FN(LPTX_get_particle_dlpin, PACK, (LPTX_type, dlpin))
{
  return LPTX_PACK_SCALAR_GET(dlpin, LPTX_array_get_stt(), LPTX_get_dlpin());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_dlpinf, SNGL, (float, dlpin))
{
  return LPTX_SNGL_SCALAR_SET(dlpin, LPTX_array_set_stf(), LPTX_set_dlpin());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_dlpinf, SNGL, (float, dlpin))
{
  return LPTX_SNGL_SCALAR_GET(dlpin, LPTX_array_get_stf(), LPTX_get_dlpin());
}

LPTX_DECL_SET_FN(LPTX_set_particle_dlpinf, PACK, (float, dlpin))
{
  return LPTX_PACK_SCALAR_SET(dlpin, LPTX_array_set_stf(), LPTX_set_dlpin());
}

LPTX_DECL_GET_FN(LPTX_get_particle_dlpinf, PACK, (float, dlpin))
{
  return LPTX_PACK_SCALAR_GET(dlpin, LPTX_array_get_stf(), LPTX_get_dlpin());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_dlpind, SNGL, (double, dlpin))
{
  return LPTX_SNGL_SCALAR_SET(dlpin, LPTX_array_set_std(), LPTX_set_dlpin());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_dlpind, SNGL, (double, dlpin))
{
  return LPTX_SNGL_SCALAR_GET(dlpin, LPTX_array_get_std(), LPTX_get_dlpin());
}

LPTX_DECL_SET_FN(LPTX_set_particle_dlpind, PACK, (double, dlpin))
{
  return LPTX_PACK_SCALAR_SET(dlpin, LPTX_array_set_std(), LPTX_set_dlpin());
}

LPTX_DECL_GET_FN(LPTX_get_particle_dlpind, PACK, (double, dlpin))
{
  return LPTX_PACK_SCALAR_GET(dlpin, LPTX_array_get_std(), LPTX_get_dlpin());
}

/* set/get particle icfpt */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_icfpt, icfpt)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_icfpt, icfpt)
#define LPTX_set_icfpt() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_icfpt)
#define LPTX_get_icfpt() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_icfpt)
#define LPTX_dtype_LPTX_set_icfpt() LPTX_PTMEM_SET_S_DTYPE(icfpt)
#define LPTX_dtype_LPTX_get_icfpt() LPTX_PTMEM_GET_S_DTYPE(icfpt)

LPTX_DECL_SET_FN(LPTX_particle_set_set_icfpt, SNGL, (LPTX_idtype, icfpt))
{
  return LPTX_SNGL_SCALAR_SET(icfpt, LPTX_array_set_spp(), LPTX_set_icfpt());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_icfpt, SNGL, (LPTX_idtype, icfpt))
{
  return LPTX_SNGL_SCALAR_GET(icfpt, LPTX_array_get_spp(), LPTX_get_icfpt());
}

LPTX_DECL_SET_FN(LPTX_set_particle_icfpt, PACK, (LPTX_idtype, icfpt))
{
  return LPTX_PACK_SCALAR_SET(icfpt, LPTX_array_set_spp(), LPTX_set_icfpt());
}

LPTX_DECL_GET_FN(LPTX_get_particle_icfpt, PACK, (LPTX_idtype, icfpt))
{
  return LPTX_PACK_SCALAR_GET(icfpt, LPTX_array_get_spp(), LPTX_get_icfpt());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_icfpti, SNGL, (int, icfpt))
{
  return LPTX_SNGL_SCALAR_SET(icfpt, LPTX_array_set_spi(), LPTX_set_icfpt());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_icfpti, SNGL, (int, icfpt))
{
  return LPTX_SNGL_SCALAR_GET(icfpt, LPTX_array_get_spi(), LPTX_get_icfpt());
}

LPTX_DECL_SET_FN(LPTX_set_particle_icfpti, PACK, (int, icfpt))
{
  return LPTX_PACK_SCALAR_SET(icfpt, LPTX_array_set_spi(), LPTX_set_icfpt());
}

LPTX_DECL_GET_FN(LPTX_get_particle_icfpti, PACK, (int, icfpt))
{
  return LPTX_PACK_SCALAR_GET(icfpt, LPTX_array_get_spi(), LPTX_get_icfpt());
}

/* set/get particle jcfpt */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_jcfpt, jcfpt)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_jcfpt, jcfpt)
#define LPTX_set_jcfpt() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_jcfpt)
#define LPTX_get_jcfpt() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_get_jcfpt)
#define LPTX_dtype_LPTX_set_jcfpt() LPTX_PTMEM_SET_S_DTYPE(jcfpt)
#define LPTX_dtype_LPTX_get_jcfpt() LPTX_PTMEM_GET_S_DTYPE(jcfpt)

LPTX_DECL_SET_FN(LPTX_particle_set_set_jcfpt, SNGL, (LPTX_idtype, jcfpt))
{
  return LPTX_SNGL_SCALAR_SET(jcfpt, LPTX_array_set_spp(), LPTX_set_jcfpt());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_jcfpt, SNGL, (LPTX_idtype, jcfpt))
{
  return LPTX_SNGL_SCALAR_GET(jcfpt, LPTX_array_get_spp(), LPTX_get_jcfpt());
}

LPTX_DECL_SET_FN(LPTX_set_particle_jcfpt, PACK, (LPTX_idtype, jcfpt))
{
  return LPTX_PACK_SCALAR_SET(jcfpt, LPTX_array_set_spp(), LPTX_set_jcfpt());
}

LPTX_DECL_GET_FN(LPTX_get_particle_jcfpt, PACK, (LPTX_idtype, jcfpt))
{
  return LPTX_PACK_SCALAR_GET(jcfpt, LPTX_array_get_spp(), LPTX_get_jcfpt());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_jcfpti, SNGL, (int, jcfpt))
{
  return LPTX_SNGL_SCALAR_SET(jcfpt, LPTX_array_set_spi(), LPTX_set_jcfpt());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_jcfpti, SNGL, (int, jcfpt))
{
  return LPTX_SNGL_SCALAR_GET(jcfpt, LPTX_array_get_spi(), LPTX_get_jcfpt());
}

LPTX_DECL_SET_FN(LPTX_set_particle_jcfpti, PACK, (int, jcfpt))
{
  return LPTX_PACK_SCALAR_SET(jcfpt, LPTX_array_set_spi(), LPTX_set_jcfpt());
}

LPTX_DECL_GET_FN(LPTX_get_particle_jcfpti, PACK, (int, jcfpt))
{
  return LPTX_PACK_SCALAR_GET(jcfpt, LPTX_array_get_spi(), LPTX_get_jcfpt());
}

/* set/get particle kcfpt */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_kcfpt, kcfpt)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_kcfpt, kcfpt)
#define LPTX_set_kcfpt() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_kcfpt)
#define LPTX_get_kcfpt() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_kcfpt)
#define LPTX_dtype_LPTX_set_kcfpt() LPTX_PTMEM_SET_S_DTYPE(kcfpt)
#define LPTX_dtype_LPTX_get_kcfpt() LPTX_PTMEM_GET_S_DTYPE(kcfpt)

LPTX_DECL_SET_FN(LPTX_particle_set_set_kcfpt, SNGL, (LPTX_idtype, kcfpt))
{
  return LPTX_SNGL_SCALAR_SET(kcfpt, LPTX_array_set_spp(), LPTX_set_kcfpt());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_kcfpt, SNGL, (LPTX_idtype, kcfpt))
{
  return LPTX_SNGL_SCALAR_GET(kcfpt, LPTX_array_get_spp(), LPTX_get_kcfpt());
}

LPTX_DECL_SET_FN(LPTX_set_particle_kcfpt, PACK, (LPTX_idtype, kcfpt))
{
  return LPTX_PACK_SCALAR_SET(kcfpt, LPTX_array_set_spp(), LPTX_set_kcfpt());
}

LPTX_DECL_GET_FN(LPTX_get_particle_kcfpt, PACK, (LPTX_idtype, kcfpt))
{
  return LPTX_PACK_SCALAR_GET(kcfpt, LPTX_array_get_spp(), LPTX_get_kcfpt());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_kcfpti, SNGL, (int, kcfpt))
{
  return LPTX_SNGL_SCALAR_SET(kcfpt, LPTX_array_set_spi(), LPTX_set_kcfpt());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_kcfpti, SNGL, (int, kcfpt))
{
  return LPTX_SNGL_SCALAR_GET(kcfpt, LPTX_array_get_spi(), LPTX_get_kcfpt());
}

LPTX_DECL_SET_FN(LPTX_set_particle_kcfpti, PACK, (int, kcfpt))
{
  return LPTX_PACK_SCALAR_SET(kcfpt, LPTX_array_set_spi(), LPTX_set_kcfpt());
}

LPTX_DECL_GET_FN(LPTX_get_particle_kcfpti, PACK, (int, kcfpt))
{
  return LPTX_PACK_SCALAR_GET(kcfpt, LPTX_array_get_spi(), LPTX_get_kcfpt());
}

/* set/get parceln (number of particles in parcel) */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_parceln, parceln)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_parceln, parceln)
#define LPTX_set_parceln() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_parceln)
#define LPTX_get_parceln() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_parceln)
#define LPTX_dtype_LPTX_set_parceln() LPTX_PTMEM_SET_S_DTYPE(parceln)
#define LPTX_dtype_LPTX_get_parceln() LPTX_PTMEM_GET_S_DTYPE(parceln)

LPTX_DECL_SET_FN(LPTX_particle_set_set_parceln, SNGL, (LPTX_type, parceln))
{
  return LPTX_SNGL_SCALAR_SET(parceln, LPTX_array_set_stt(),
                              LPTX_set_parceln());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_parceln, SNGL, (LPTX_type, parceln))
{
  return LPTX_SNGL_SCALAR_GET(parceln, LPTX_array_get_stt(),
                              LPTX_get_parceln());
}

LPTX_DECL_SET_FN(LPTX_set_particle_parceln, PACK, (LPTX_type, parceln))
{
  return LPTX_PACK_SCALAR_SET(parceln, LPTX_array_set_stt(),
                              LPTX_set_parceln());
}

LPTX_DECL_GET_FN(LPTX_get_particle_parceln, PACK, (LPTX_type, parceln))
{
  return LPTX_PACK_SCALAR_GET(parceln, LPTX_array_get_stt(),
                              LPTX_get_parceln());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_parceln_f, SNGL, (float, parceln))
{
  return LPTX_SNGL_SCALAR_SET(parceln, LPTX_array_set_stf(),
                              LPTX_set_parceln());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_parceln_f, SNGL, (float, parceln))
{
  return LPTX_SNGL_SCALAR_GET(parceln, LPTX_array_get_stf(),
                              LPTX_get_parceln());
}

LPTX_DECL_SET_FN(LPTX_set_particle_parceln_f, PACK, (float, parceln))
{
  return LPTX_PACK_SCALAR_SET(parceln, LPTX_array_set_stf(),
                              LPTX_set_parceln());
}

LPTX_DECL_GET_FN(LPTX_get_particle_parceln_f, PACK, (float, parceln))
{
  return LPTX_PACK_SCALAR_GET(parceln, LPTX_array_get_stf(),
                              LPTX_get_parceln());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_parceln_d, SNGL, (double, parceln))
{
  return LPTX_SNGL_SCALAR_SET(parceln, LPTX_array_set_std(),
                              LPTX_set_parceln());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_parceln_d, SNGL, (double, parceln))
{
  return LPTX_SNGL_SCALAR_GET(parceln, LPTX_array_get_std(),
                              LPTX_get_parceln());
}

LPTX_DECL_SET_FN(LPTX_set_particle_parceln_d, PACK, (double, parceln))
{
  return LPTX_PACK_SCALAR_SET(parceln, LPTX_array_set_std(),
                              LPTX_set_parceln());
}

LPTX_DECL_GET_FN(LPTX_get_particle_parceln_d, PACK, (double, parceln))
{
  return LPTX_PACK_SCALAR_GET(parceln, LPTX_array_get_std(),
                              LPTX_get_parceln());
}

/* set/get particle temperature */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_temperature, temperature)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_temperature, temperature)
#define LPTX_set_temperature() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_temperature)
#define LPTX_get_temperature() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_temperature)
#define LPTX_dtype_LPTX_set_temperature() LPTX_PTMEM_SET_S_DTYPE(temperature)
#define LPTX_dtype_LPTX_get_temperature() LPTX_PTMEM_GET_S_DTYPE(temperature)

LPTX_DECL_SET_FN(LPTX_particle_set_set_temperature, SNGL,
                 (LPTX_type, temperature))
{
  return LPTX_SNGL_SCALAR_SET(temperature, LPTX_array_set_stt(),
                              LPTX_set_temperature());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_temperature, SNGL,
                 (LPTX_type, temperature))
{
  return LPTX_SNGL_SCALAR_GET(temperature, LPTX_array_get_stt(),
                              LPTX_get_temperature());
}

LPTX_DECL_SET_FN(LPTX_set_particle_temperature, PACK, (LPTX_type, temperature))
{
  return LPTX_PACK_SCALAR_SET(temperature, LPTX_array_set_stt(),
                              LPTX_set_temperature());
}

LPTX_DECL_GET_FN(LPTX_get_particle_temperature, PACK, (LPTX_type, temperature))
{
  return LPTX_PACK_SCALAR_GET(temperature, LPTX_array_get_stt(),
                              LPTX_get_temperature());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_temperature_f, SNGL,
                 (float, temperature))
{
  return LPTX_SNGL_SCALAR_SET(temperature, LPTX_array_set_stf(),
                              LPTX_set_temperature());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_temperature_f, SNGL,
                 (float, temperature))
{
  return LPTX_SNGL_SCALAR_GET(temperature, LPTX_array_get_stf(),
                              LPTX_get_temperature());
}

LPTX_DECL_SET_FN(LPTX_set_particle_temperature_f, PACK, (float, temperature))
{
  return LPTX_PACK_SCALAR_SET(temperature, LPTX_array_set_stf(),
                              LPTX_set_temperature());
}

LPTX_DECL_GET_FN(LPTX_get_particle_temperature_f, PACK, (float, temperature))
{
  return LPTX_PACK_SCALAR_GET(temperature, LPTX_array_get_stf(),
                              LPTX_get_temperature());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_temperature_d, SNGL,
                 (double, temperature))
{
  return LPTX_SNGL_SCALAR_SET(temperature, LPTX_array_set_std(),
                              LPTX_set_temperature());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_temperature_d, SNGL,
                 (double, temperature))
{
  return LPTX_SNGL_SCALAR_GET(temperature, LPTX_array_get_std(),
                              LPTX_get_temperature());
}

LPTX_DECL_SET_FN(LPTX_set_particle_temperature_d, PACK, (double, temperature))
{
  return LPTX_PACK_SCALAR_SET(temperature, LPTX_array_set_std(),
                              LPTX_set_temperature());
}

LPTX_DECL_GET_FN(LPTX_get_particle_temperature_d, PACK, (double, temperature))
{
  return LPTX_PACK_SCALAR_GET(temperature, LPTX_array_get_std(),
                              LPTX_get_temperature());
}

/* set/get initial position */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_init_position, init_position)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_init_position, init_position)
#define LPTX_set_init_position() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_init_position)
#define LPTX_get_init_position() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_init_position)
#define LPTX_dtype_LPTX_set_init_position() \
  LPTX_PTMEM_SET_S_DTYPE(init_position)
#define LPTX_dtype_LPTX_get_init_position() \
  LPTX_PTMEM_GET_S_DTYPE(init_position)

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_position, SNGL,
                 (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_SET(vector, LPTX_array_set_svv(),
                              LPTX_set_init_position());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_position, SNGL,
                 (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_GET(vector, LPTX_array_get_svv(),
                              LPTX_get_init_position());
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_position, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_SET(vector, LPTX_array_set_svv(),
                              LPTX_set_init_position());
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_position, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_GET(vector, LPTX_array_get_svv(),
                              LPTX_get_init_position());
}

LPTX_DEFINE_ARRAY_SET_V_PTMEM(LPTX_set_init_position_v, init_position)
LPTX_DEFINE_ARRAY_GET_V_PTMEM(LPTX_get_init_position_v, init_position)
#define LPTX_set_init_position_v() \
  LPTX_PARTICLE_SET_V_PTMEM_INIT(LPTX_set_init_position_v)
#define LPTX_get_init_position_v() \
  LPTX_PARTICLE_GET_V_PTMEM_INIT(LPTX_get_init_position_v)
#define LPTX_dtype_LPTX_set_init_position_v() \
  LPTX_PTMEM_SET_V_DTYPE(init_position)
#define LPTX_dtype_LPTX_get_init_position_v() \
  LPTX_PTMEM_GET_V_DTYPE(init_position)

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_position_t_aos, SNGL,
                 (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvt(),
                              LPTX_set_init_position_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_position_t_aos, SNGL,
                 (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvt(),
                              LPTX_get_init_position_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_position_t_aos, PACK,
                 (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvt(),
                              LPTX_set_init_position_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_position_t_aos, PACK,
                 (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvt(),
                              LPTX_get_init_position_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_position_f_aos, SNGL,
                 (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvf(),
                              LPTX_set_init_position_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_position_f_aos, SNGL,
                 (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvf(),
                              LPTX_get_init_position_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_position_f_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvf(),
                              LPTX_set_init_position_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_position_f_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvf(),
                              LPTX_get_init_position_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_position_d_aos, SNGL,
                 (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvd(),
                              LPTX_set_init_position_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_position_d_aos, SNGL,
                 (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvd(),
                              LPTX_get_init_position_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_position_d_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvd(),
                              LPTX_set_init_position_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_position_d_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvd(),
                              LPTX_get_init_position_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_position_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_init_position_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_position_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_init_position_v(),
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_position_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_init_position_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_position_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_init_position_v(),
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_position_f_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_init_position_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_position_f_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_init_position_v(),
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_position_f_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_init_position_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_position_f_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_init_position_v(),
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_position_d_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_init_position_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_position_d_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_init_position_v(),
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_position_d_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_init_position_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_position_d_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_init_position_v(),
                              x, y, z);
}

/* set/get initial velocity */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_init_velocity, init_velocity)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_init_velocity, init_velocity)
#define LPTX_set_init_velocity() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_init_velocity)
#define LPTX_get_init_velocity() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_init_velocity)
#define LPTX_dtype_LPTX_set_init_velocity() \
  LPTX_PTMEM_SET_S_DTYPE(init_velocity)
#define LPTX_dtype_LPTX_get_init_velocity() \
  LPTX_PTMEM_GET_S_DTYPE(init_velocity)

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_velocity, SNGL,
                 (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_SET(vector, LPTX_array_set_svv(),
                              LPTX_set_init_velocity());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_velocity, SNGL,
                 (LPTX_vector, vector))
{
  return LPTX_SNGL_SCALAR_GET(vector, LPTX_array_get_svv(),
                              LPTX_get_init_velocity());
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_velocity, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_SET(vector, LPTX_array_set_svv(),
                              LPTX_set_init_velocity());
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_velocity, PACK, (LPTX_vector, vector))
{
  return LPTX_PACK_SCALAR_GET(vector, LPTX_array_get_svv(),
                              LPTX_get_init_velocity());
}

LPTX_DEFINE_ARRAY_SET_V_PTMEM(LPTX_set_init_velocity_v, init_velocity)
LPTX_DEFINE_ARRAY_GET_V_PTMEM(LPTX_get_init_velocity_v, init_velocity)
#define LPTX_set_init_velocity_v() \
  LPTX_PARTICLE_SET_V_PTMEM_INIT(LPTX_set_init_velocity_v)
#define LPTX_get_init_velocity_v() \
  LPTX_PARTICLE_GET_V_PTMEM_INIT(LPTX_get_init_velocity_v)
#define LPTX_dtype_LPTX_set_init_velocity_v() \
  LPTX_PTMEM_SET_V_DTYPE(init_velocity)
#define LPTX_dtype_LPTX_get_init_velocity_v() \
  LPTX_PTMEM_GET_V_DTYPE(init_velocity)

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_velocity_t_aos, SNGL,
                 (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvt(),
                              LPTX_set_init_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_velocity_t_aos, SNGL,
                 (LPTX_type, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvt(),
                              LPTX_get_init_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_velocity_t_aos, PACK,
                 (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvt(),
                              LPTX_set_init_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_velocity_t_aos, PACK,
                 (LPTX_type, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvt(),
                              LPTX_get_init_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_velocity_f_aos, SNGL,
                 (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvf(),
                              LPTX_set_init_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_velocity_f_aos, SNGL,
                 (float, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvf(),
                              LPTX_get_init_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_velocity_f_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvf(),
                              LPTX_set_init_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_velocity_f_aos, PACK, (float, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvf(),
                              LPTX_get_init_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_velocity_d_aos, SNGL,
                 (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_SET(aosvec, LPTX_array_set_vvd(),
                              LPTX_set_init_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_velocity_d_aos, SNGL,
                 (double, aosvec))
{
  return LPTX_SNGL_AOSVEC_GET(aosvec, LPTX_array_get_vvd(),
                              LPTX_get_init_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_velocity_d_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_SET(aosvec, LPTX_array_set_vvd(),
                              LPTX_set_init_velocity_v());
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_velocity_d_aos, PACK, (double, aosvec))
{
  return LPTX_PACK_AOSVEC_GET(aosvec, LPTX_array_get_vvd(),
                              LPTX_get_init_velocity_v());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_velocity_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_init_velocity_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_velocity_t_soa, SNGL, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_init_velocity_v(),
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_velocity_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvt(), LPTX_set_init_velocity_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_velocity_t_soa, PACK, //
                 (LPTX_type, x), (LPTX_type, y), (LPTX_type, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvt(), LPTX_get_init_velocity_v(),
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_velocity_f_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_init_velocity_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_velocity_f_soa, SNGL, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_init_velocity_v(),
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_velocity_f_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvf(), LPTX_set_init_velocity_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_velocity_f_soa, PACK, //
                 (float, x), (float, y), (float, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvf(), LPTX_get_init_velocity_v(),
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_velocity_d_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_init_velocity_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_velocity_d_soa, SNGL, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_SNGL_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_init_velocity_v(),
                              x, y, z);
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_velocity_d_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_SET(LPTX_array_set_vvd(), LPTX_set_init_velocity_v(),
                              x, y, z);
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_velocity_d_soa, PACK, //
                 (double, x), (double, y), (double, z))
{
  return LPTX_PACK_SOAVEC_GET(LPTX_array_get_vvd(), LPTX_get_init_velocity_v(),
                              x, y, z);
}

/* set/get initial temperature */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_init_temperature, init_temperature)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_init_temperature, init_temperature)
#define LPTX_set_init_temperature() \
  LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_init_temperature)
#define LPTX_get_init_temperature() \
  LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_init_temperature)
#define LPTX_dtype_LPTX_set_init_temperature() \
  LPTX_PTMEM_SET_S_DTYPE(init_temperature)
#define LPTX_dtype_LPTX_get_init_temperature() \
  LPTX_PTMEM_GET_S_DTYPE(init_temperature)

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_temperature, SNGL,
                 (LPTX_type, temperature))
{
  return LPTX_SNGL_SCALAR_SET(temperature, LPTX_array_set_stt(),
                              LPTX_set_init_temperature());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_temperature, SNGL,
                 (LPTX_type, temperature))
{
  return LPTX_SNGL_SCALAR_GET(temperature, LPTX_array_get_stt(),
                              LPTX_get_init_temperature());
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_temperature, PACK,
                 (LPTX_type, temperature))
{
  return LPTX_PACK_SCALAR_SET(temperature, LPTX_array_set_stt(),
                              LPTX_set_init_temperature());
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_temperature, PACK,
                 (LPTX_type, temperature))
{
  return LPTX_PACK_SCALAR_GET(temperature, LPTX_array_get_stt(),
                              LPTX_get_init_temperature());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_temperature_f, SNGL,
                 (float, temperature))
{
  return LPTX_SNGL_SCALAR_SET(temperature, LPTX_array_set_stf(),
                              LPTX_set_init_temperature());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_temperature_f, SNGL,
                 (float, temperature))
{
  return LPTX_SNGL_SCALAR_GET(temperature, LPTX_array_get_stf(),
                              LPTX_get_init_temperature());
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_temperature_f, PACK,
                 (float, temperature))
{
  return LPTX_PACK_SCALAR_SET(temperature, LPTX_array_set_stf(),
                              LPTX_set_init_temperature());
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_temperature_f, PACK,
                 (float, temperature))
{
  return LPTX_PACK_SCALAR_GET(temperature, LPTX_array_get_stf(),
                              LPTX_get_init_temperature());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_init_temperature_d, SNGL,
                 (double, temperature))
{
  return LPTX_SNGL_SCALAR_SET(temperature, LPTX_array_set_std(),
                              LPTX_set_init_temperature());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_init_temperature_d, SNGL,
                 (double, temperature))
{
  return LPTX_SNGL_SCALAR_GET(temperature, LPTX_array_get_std(),
                              LPTX_get_init_temperature());
}

LPTX_DECL_SET_FN(LPTX_set_particle_init_temperature_d, PACK,
                 (double, temperature))
{
  return LPTX_PACK_SCALAR_SET(temperature, LPTX_array_set_std(),
                              LPTX_set_init_temperature());
}

LPTX_DECL_GET_FN(LPTX_get_particle_init_temperature_d, PACK,
                 (double, temperature))
{
  return LPTX_PACK_SCALAR_GET(temperature, LPTX_array_get_std(),
                              LPTX_get_init_temperature());
}

/* set/get particle-bound random seed */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_ptseed, ptseed)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_ptseed, ptseed)
#define LPTX_set_ptseed() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_ptseed)
#define LPTX_get_ptseed() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_ptseed)
#define LPTX_dtype_LPTX_set_ptseed() LPTX_PTMEM_SET_S_DTYPE(ptseed)
#define LPTX_dtype_LPTX_get_ptseed() LPTX_PTMEM_GET_S_DTYPE(ptseed)

LPTX_DECL_SET_FN(LPTX_particle_set_set_ptseed, SNGL,
                 (jupiter_random_seed, seed))
{
  return LPTX_SNGL_SCALAR_SET(seed, LPTX_array_set_rndsv(), LPTX_set_ptseed());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_ptseed, SNGL,
                 (jupiter_random_seed, seed))
{
  return LPTX_SNGL_SCALAR_GET(seed, LPTX_array_get_rndsv(), LPTX_get_ptseed());
}

LPTX_DECL_SET_FN(LPTX_set_particle_ptseed, PACK, (jupiter_random_seed, seed))
{
  return LPTX_PACK_SCALAR_SET(seed, LPTX_array_set_rndsv(), LPTX_set_ptseed());
}

LPTX_DECL_GET_FN(LPTX_get_particle_ptseed, PACK, (jupiter_random_seed, seed))
{
  return LPTX_PACK_SCALAR_GET(seed, LPTX_array_get_rndsv(), LPTX_get_ptseed());
}

LPTX_DEFINE_ARRAY_SET_SEEDI_PTMEM(LPTX_set_ptseedi, ptseed)
LPTX_DEFINE_ARRAY_GET_SEEDI_PTMEM(LPTX_get_ptseedi, ptseed)
#define LPTX_set_ptseedi() LPTX_PARTICLE_SET_SEEDI_PTMEM_INIT(LPTX_set_ptseedi)
#define LPTX_get_ptseedi() LPTX_PARTICLE_GET_SEEDI_PTMEM_INIT(LPTX_get_ptseedi)
#define LPTX_dtype_LPTX_set_ptseedi() LPTX_PTMEM_SET_SEEDI_DTYPE(ptseed)
#define LPTX_dtype_LPTX_get_ptseedi() LPTX_PTMEM_GET_SEEDI_DTYPE(ptseed)

LPTX_DECL_SET_FN(LPTX_particle_set_set_ptseed_i, SNGL, (uint64_t, seed))
{
  return LPTX_SNGL_AOSVEC_SET(seed, LPTX_array_set_seedi(), LPTX_set_ptseedi());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_ptseed_i, SNGL, (uint64_t, seed))
{
  return LPTX_SNGL_AOSVEC_GET(seed, LPTX_array_get_seedi(), LPTX_get_ptseedi());
}

LPTX_DECL_SET_FN(LPTX_set_particle_ptseed_i, PACK, (uint64_t, seed))
{
  return LPTX_PACK_AOSVEC_SET(seed, LPTX_array_set_seedi(), LPTX_set_ptseedi());
}

LPTX_DECL_GET_FN(LPTX_get_particle_ptseed_i, PACK, (uint64_t, seed))
{
  return LPTX_PACK_AOSVEC_GET(seed, LPTX_array_get_seedi(), LPTX_get_ptseedi());
}

/* set/get particle flags */
LPTX_DEFINE_ARRAY_SET_S_PTMEM(LPTX_set_flagsall, flags)
LPTX_DEFINE_ARRAY_GET_S_PTMEM(LPTX_get_flagsall, flags)
#define LPTX_set_flagsall() LPTX_PARTICLE_SET_S_PTMEM_INIT(LPTX_set_flagsall)
#define LPTX_get_flagsall() LPTX_PARTICLE_GET_S_PTMEM_INIT(LPTX_get_flagsall)
#define LPTX_dtype_LPTX_set_flagsall() LPTX_PTMEM_SET_S_DTYPE(flags)
#define LPTX_dtype_LPTX_get_flagsall() LPTX_PTMEM_GET_S_DTYPE(flags)

LPTX_DECL_SET_FN(LPTX_particle_set_set_flagsall, SNGL,
                 (LPTX_particle_flags, flags))
{
  return LPTX_SNGL_SCALAR_SET(flags, LPTX_array_set_flagsv(),
                              LPTX_set_flagsall());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_flagsall, SNGL,
                 (LPTX_particle_flags, flags))
{
  return LPTX_SNGL_SCALAR_GET(flags, LPTX_array_get_flagsv(),
                              LPTX_get_flagsall());
}

LPTX_DECL_SET_FN(LPTX_set_particle_flagsall, PACK, (LPTX_particle_flags, flags))
{
  return LPTX_PACK_SCALAR_SET(flags, LPTX_array_set_flagsv(),
                              LPTX_set_flagsall());
}

LPTX_DECL_GET_FN(LPTX_get_particle_flagsall, PACK, (LPTX_particle_flags, flags))
{
  return LPTX_PACK_SCALAR_GET(flags, LPTX_array_get_flagsv(),
                              LPTX_get_flagsall());
}

LPTX_DEFINE_ARRAY_SET_FLAGSP_PTMEM(LPTX_set_flagsp, flags)
LPTX_DEFINE_ARRAY_GET_FLAGSP_PTMEM(LPTX_get_flagsp, flags)
#define LPTX_set_flagsp() LPTX_PARTICLE_SET_FLAGSP_PTMEM_INIT(LPTX_set_flagsp)
#define LPTX_get_flagsp() LPTX_PARTICLE_GET_FLAGSP_PTMEM_INIT(LPTX_get_flagsp)
#define LPTX_dtype_LPTX_set_flagsp() LPTX_PTMEM_SET_FLAGSP_DTYPE(flags)
#define LPTX_dtype_LPTX_get_flagsp() LPTX_PTMEM_GET_FLAGSP_DTYPE(flags)

LPTX_DECL_SET_FN(LPTX_particle_set_set_flagsall_g, SNGL,
                 (geom_bitarray_element_type, flags))
{
  return LPTX_SNGL_AOSVEC_SET(flags, LPTX_array_set_flagsp(),
                              LPTX_set_flagsp());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_flagsall_g, SNGL,
                 (geom_bitarray_element_type, flags))
{
  return LPTX_SNGL_AOSVEC_GET(flags, LPTX_array_get_flagsp(),
                              LPTX_get_flagsp());
}

LPTX_DECL_SET_FN(LPTX_set_particle_flagsall_g, PACK,
                 (geom_bitarray_element_type, flags))
{
  return LPTX_PACK_AOSVEC_SET(flags, LPTX_array_set_flagsp(),
                              LPTX_set_flagsp());
}

LPTX_DECL_GET_FN(LPTX_get_particle_flagsall_g, PACK,
                 (geom_bitarray_element_type, flags))
{
  return LPTX_PACK_AOSVEC_GET(flags, LPTX_array_get_flagsp(),
                              LPTX_get_flagsp());
}

LPTX_DEFINE_ARRAY_SET_FLAGS_PTMEM(LPTX_set_flags, flags)
LPTX_DEFINE_ARRAY_GET_FLAGS_PTMEM(LPTX_get_flags, flags)
#define LPTX_set_flags() LPTX_PARTICLE_SET_FLAGS_PTMEM_INIT(LPTX_set_flags)
#define LPTX_get_flags() LPTX_PARTICLE_GET_FLAGS_PTMEM_INIT(LPTX_get_flags)
#define LPTX_dtype_LPTX_set_flags() LPTX_PTMEM_SET_FLAGS_DTYPE(flags)
#define LPTX_dtype_LPTX_get_flags() LPTX_PTMEM_GET_FLAGS_DTYPE(flags)

LPTX_DECL_SET_FN(LPTX_particle_set_set_flagsall_c, SNGL, (char, flags))
{
  return LPTX_SNGL_AOSVEC_SET(flags, LPTX_array_set_flagsc(), LPTX_set_flags());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_flagsall_c, SNGL, (char, flags))
{
  return LPTX_SNGL_AOSVEC_GET(flags, LPTX_array_get_flagsc(), LPTX_get_flags());
}

LPTX_DECL_SET_FN(LPTX_set_particle_flagsall_c, PACK, (char, flags))
{
  return LPTX_PACK_AOSVEC_SET(flags, LPTX_array_set_flagsc(), LPTX_set_flags());
}

LPTX_DECL_GET_FN(LPTX_get_particle_flagsall_c, PACK, (char, flags))
{
  return LPTX_PACK_AOSVEC_GET(flags, LPTX_array_get_flagsc(), LPTX_get_flags());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_flag_b, SNGL,
                 (LPTX_particle_flag, SCL, bit), (LPTX_bool, flags))
{
  return LPTX_SNGL_VECCMP_SET(flags, bit, LPTX_array_set_flagsb(),
                              LPTX_set_flags());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_flag_b, SNGL,
                 (LPTX_particle_flag, SCL, bit), (LPTX_bool, flags))
{
  return LPTX_SNGL_VECCMP_GET(flags, bit, LPTX_array_get_flagsb(),
                              LPTX_get_flags());
}

LPTX_DECL_SET_FN(LPTX_set_particle_flag_b, PACK, (LPTX_particle_flag, SCL, bit),
                 (LPTX_bool, flags))
{
  return LPTX_PACK_VECCMP_SET(flags, bit, LPTX_array_set_flagsb(),
                              LPTX_set_flags());
}

LPTX_DECL_GET_FN(LPTX_get_particle_flag_b, PACK, (LPTX_particle_flag, SCL, bit),
                 (LPTX_bool, flags))
{
  return LPTX_PACK_VECCMP_GET(flags, bit, LPTX_array_get_flagsb(),
                              LPTX_get_flags());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_flag_c, SNGL,
                 (LPTX_particle_flag, SCL, bit), (char, flags))
{
  return LPTX_SNGL_VECCMP_SET(flags, bit, LPTX_array_set_flagsc(),
                              LPTX_set_flags());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_flag_c, SNGL,
                 (LPTX_particle_flag, SCL, bit), (char, flags))
{
  return LPTX_SNGL_VECCMP_GET(flags, bit, LPTX_array_get_flagsc(),
                              LPTX_get_flags());
}

LPTX_DECL_SET_FN(LPTX_set_particle_flag_c, PACK, (LPTX_particle_flag, SCL, bit),
                 (char, flags))
{
  return LPTX_PACK_VECCMP_SET(flags, bit, LPTX_array_set_flagsc(),
                              LPTX_set_flags());
}

LPTX_DECL_GET_FN(LPTX_get_particle_flag_c, PACK, (LPTX_particle_flag, SCL, bit),
                 (char, flags))
{
  return LPTX_PACK_VECCMP_GET(flags, bit, LPTX_array_get_flagsc(),
                              LPTX_get_flags());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_flag_i, SNGL,
                 (LPTX_particle_flag, SCL, bit), (int, flags))
{
  return LPTX_SNGL_VECCMP_SET(flags, bit, LPTX_array_set_flagsi(),
                              LPTX_set_flags());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_flag_i, SNGL,
                 (LPTX_particle_flag, SCL, bit), (int, flags))
{
  return LPTX_SNGL_VECCMP_GET(flags, bit, LPTX_array_get_flagsi(),
                              LPTX_get_flags());
}

LPTX_DECL_SET_FN(LPTX_set_particle_flag_i, PACK, (LPTX_particle_flag, SCL, bit),
                 (int, flags))
{
  return LPTX_PACK_VECCMP_SET(flags, bit, LPTX_array_set_flagsi(),
                              LPTX_set_flags());
}

LPTX_DECL_GET_FN(LPTX_get_particle_flag_i, PACK, (LPTX_particle_flag, SCL, bit),
                 (int, flags))
{
  return LPTX_PACK_VECCMP_GET(flags, bit, LPTX_array_get_flagsi(),
                              LPTX_get_flags());
}

/* set/get particle allocation flag */
LPTX_DECL_SET_FN(LPTX_particle_set_set_used, SNGL, (LPTX_bool, is_used))
{
  return LPTX_SNGL_VECCMP_SET(is_used, LPTX_PT_IS_USED, LPTX_array_set_flagsb(),
                              LPTX_set_flags());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_used, SNGL, (LPTX_bool, is_used))
{
  return LPTX_SNGL_VECCMP_GET(is_used, LPTX_PT_IS_USED, LPTX_array_get_flagsb(),
                              LPTX_get_flags());
}

LPTX_DECL_SET_FN(LPTX_set_particle_used, PACK, (LPTX_bool, is_used))
{
  return LPTX_PACK_VECCMP_SET(is_used, LPTX_PT_IS_USED, LPTX_array_set_flagsb(),
                              LPTX_set_flags());
}

LPTX_DECL_GET_FN(LPTX_get_particle_used, PACK, (LPTX_bool, is_used))
{
  return LPTX_PACK_VECCMP_GET(is_used, LPTX_PT_IS_USED, LPTX_array_get_flagsb(),
                              LPTX_get_flags());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_used_i, SNGL, (int, is_used))
{
  return LPTX_SNGL_VECCMP_SET(is_used, LPTX_PT_IS_USED, LPTX_array_set_flagsi(),
                              LPTX_set_flags());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_used_i, SNGL, (int, is_used))
{
  return LPTX_SNGL_VECCMP_GET(is_used, LPTX_PT_IS_USED, LPTX_array_get_flagsi(),
                              LPTX_get_flags());
}

LPTX_DECL_SET_FN(LPTX_set_particle_used_i, PACK, (int, is_used))
{
  return LPTX_PACK_VECCMP_SET(is_used, LPTX_PT_IS_USED, LPTX_array_set_flagsi(),
                              LPTX_set_flags());
}

LPTX_DECL_GET_FN(LPTX_get_particle_used_i, PACK, (int, is_used))
{
  return LPTX_PACK_VECCMP_GET(is_used, LPTX_PT_IS_USED, LPTX_array_get_flagsi(),
                              LPTX_get_flags());
}

/* set/get particle exit flag */
LPTX_DECL_SET_FN(LPTX_particle_set_set_exited, SNGL, (LPTX_bool, is_exited))
{
  return LPTX_SNGL_VECCMP_SET(is_exited, LPTX_PT_IS_EXITED,
                              LPTX_array_set_flagsb(), LPTX_set_flags());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_exited, SNGL, (LPTX_bool, is_exited))
{
  return LPTX_SNGL_VECCMP_GET(is_exited, LPTX_PT_IS_EXITED,
                              LPTX_array_get_flagsb(), LPTX_get_flags());
}

LPTX_DECL_SET_FN(LPTX_set_particle_exited, PACK, (LPTX_bool, is_exited))
{
  return LPTX_PACK_VECCMP_SET(is_exited, LPTX_PT_IS_EXITED,
                              LPTX_array_set_flagsb(), LPTX_set_flags());
}

LPTX_DECL_GET_FN(LPTX_get_particle_exited, PACK, (LPTX_bool, is_exited))
{
  return LPTX_PACK_VECCMP_GET(is_exited, LPTX_PT_IS_EXITED,
                              LPTX_array_get_flagsb(), LPTX_get_flags());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_exited_i, SNGL, (int, is_exited))
{
  return LPTX_SNGL_VECCMP_SET(is_exited, LPTX_PT_IS_EXITED,
                              LPTX_array_set_flagsi(), LPTX_set_flags());
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_exited_i, SNGL, (int, is_exited))
{
  return LPTX_SNGL_VECCMP_GET(is_exited, LPTX_PT_IS_EXITED,
                              LPTX_array_get_flagsi(), LPTX_get_flags());
}

LPTX_DECL_SET_FN(LPTX_set_particle_exited_i, PACK, (int, is_exited))
{
  return LPTX_PACK_VECCMP_SET(is_exited, LPTX_PT_IS_EXITED,
                              LPTX_array_set_flagsi(), LPTX_set_flags());
}

LPTX_DECL_GET_FN(LPTX_get_particle_exited_i, PACK, (int, is_exited))
{
  return LPTX_PACK_VECCMP_GET(is_exited, LPTX_PT_IS_EXITED,
                              LPTX_array_get_flagsi(), LPTX_get_flags());
}

/* set/get particle vector element (all components) */
#define LPTX_set_particle_vector(vindex) LPTX_PARTICLE_SET_PVEC_INIT(vindex)
#define LPTX_get_particle_vector(vindex) LPTX_PARTICLE_GET_PVEC_INIT(vindex)
#define LPTX_dtype_LPTX_set_particle_vector(vindex) LPTX_PVEC_SET_DTYPE()
#define LPTX_dtype_LPTX_get_particle_vector(vindex) LPTX_PVEC_GET_DTYPE()

LPTX_DECL_SET_FN(LPTX_particle_set_set_vector_t_aos, SNGL,
                 (LPTX_idtype, SCL, vector_index), (LPTX_type, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_SNGL_AOSVEC_SET(array, LPTX_array_set_pvt(number_of_components),
                              LPTX_set_particle_vector(vector_index));
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_vector_t_aos, SNGL,
                 (LPTX_idtype, SCL, vector_index), (LPTX_type, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_SNGL_AOSVEC_GET(array, LPTX_array_get_pvt(number_of_components),
                              LPTX_get_particle_vector(vector_index));
}

LPTX_DECL_SET_FN(LPTX_set_particle_vector_t_aos, PACK,
                 (LPTX_idtype, SCL, vector_index), (LPTX_type, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_PACK_AOSVEC_SET(array, LPTX_array_set_pvt(number_of_components),
                              LPTX_set_particle_vector(vector_index));
}

LPTX_DECL_GET_FN(LPTX_get_particle_vector_t_aos, PACK,
                 (LPTX_idtype, SCL, vector_index), (LPTX_type, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_PACK_AOSVEC_GET(array, LPTX_array_get_pvt(number_of_components),
                              LPTX_get_particle_vector(vector_index));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_vector_d_aos, SNGL,
                 (LPTX_idtype, SCL, vector_index), (double, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_SNGL_AOSVEC_SET(array, LPTX_array_set_pvd(number_of_components),
                              LPTX_set_particle_vector(vector_index));
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_vector_d_aos, SNGL,
                 (LPTX_idtype, SCL, vector_index), (double, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_SNGL_AOSVEC_GET(array, LPTX_array_get_pvd(number_of_components),
                              LPTX_get_particle_vector(vector_index));
}

LPTX_DECL_SET_FN(LPTX_set_particle_vector_d_aos, PACK,
                 (LPTX_idtype, SCL, vector_index), (double, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_PACK_AOSVEC_SET(array, LPTX_array_set_pvd(number_of_components),
                              LPTX_set_particle_vector(vector_index));
}

LPTX_DECL_GET_FN(LPTX_get_particle_vector_d_aos, PACK,
                 (LPTX_idtype, SCL, vector_index), (double, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_PACK_AOSVEC_GET(array, LPTX_array_get_pvd(number_of_components),
                              LPTX_get_particle_vector(vector_index));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_vector_f_aos, SNGL,
                 (LPTX_idtype, SCL, vector_index), (float, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_SNGL_AOSVEC_SET(array, LPTX_array_set_pvf(number_of_components),
                              LPTX_set_particle_vector(vector_index));
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_vector_f_aos, SNGL,
                 (LPTX_idtype, SCL, vector_index), (float, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_SNGL_AOSVEC_GET(array, LPTX_array_get_pvf(number_of_components),
                              LPTX_get_particle_vector(vector_index));
}

LPTX_DECL_SET_FN(LPTX_set_particle_vector_f_aos, PACK,
                 (LPTX_idtype, SCL, vector_index), (float, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_PACK_AOSVEC_SET(array, LPTX_array_set_pvf(number_of_components),
                              LPTX_set_particle_vector(vector_index));
}

LPTX_DECL_GET_FN(LPTX_get_particle_vector_f_aos, PACK,
                 (LPTX_idtype, SCL, vector_index), (float, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_PACK_AOSVEC_GET(array, LPTX_array_get_pvf(number_of_components),
                              LPTX_get_particle_vector(vector_index));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_vector_t_soa, SNGL,
                 (LPTX_idtype, SCL, vector_index), (LPTX_type *, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_SNGL_SOAVECV_SET(number_of_components, array,
                               LPTX_array_set_pvt(number_of_components),
                               LPTX_set_particle_vector(vector_index));
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_vector_t_soa, SNGL,
                 (LPTX_idtype, SCL, vector_index), (LPTX_type *, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_SNGL_SOAVECV_GET(number_of_components, array,
                               LPTX_array_get_pvt(number_of_components),
                               LPTX_get_particle_vector(vector_index));
}

LPTX_DECL_SET_FN(LPTX_set_particle_vector_t_soa, PACK,
                 (LPTX_idtype, SCL, vector_index), (LPTX_type *, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_PACK_SOAVECV_SET(number_of_components, array,
                               LPTX_array_set_pvt(number_of_components),
                               LPTX_set_particle_vector(vector_index));
}

LPTX_DECL_GET_FN(LPTX_get_particle_vector_t_soa, PACK,
                 (LPTX_idtype, SCL, vector_index), (LPTX_type *, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_PACK_SOAVECV_GET(number_of_components, array,
                               LPTX_array_get_pvt(number_of_components),
                               LPTX_get_particle_vector(vector_index));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_vector_d_soa, SNGL,
                 (LPTX_idtype, SCL, vector_index), (double *, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_SNGL_SOAVECV_SET(number_of_components, array,
                               LPTX_array_set_pvd(number_of_components),
                               LPTX_set_particle_vector(vector_index));
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_vector_d_soa, SNGL,
                 (LPTX_idtype, SCL, vector_index), (double *, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_SNGL_SOAVECV_GET(number_of_components, array,
                               LPTX_array_get_pvd(number_of_components),
                               LPTX_get_particle_vector(vector_index));
}

LPTX_DECL_SET_FN(LPTX_set_particle_vector_d_soa, PACK,
                 (LPTX_idtype, SCL, vector_index), (double *, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_PACK_SOAVECV_SET(number_of_components, array,
                               LPTX_array_set_pvd(number_of_components),
                               LPTX_set_particle_vector(vector_index));
}

LPTX_DECL_GET_FN(LPTX_get_particle_vector_d_soa, PACK,
                 (LPTX_idtype, SCL, vector_index), (double *, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_PACK_SOAVECV_GET(number_of_components, array,
                               LPTX_array_get_pvd(number_of_components),
                               LPTX_get_particle_vector(vector_index));
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_vector_f_soa, SNGL,
                 (LPTX_idtype, SCL, vector_index), (float *, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_SNGL_SOAVECV_SET(number_of_components, array,
                               LPTX_array_set_pvf(number_of_components),
                               LPTX_set_particle_vector(vector_index));
}

LPTX_DECL_GET_FN(LPTX_particle_set_get_vector_f_soa, SNGL,
                 (LPTX_idtype, SCL, vector_index), (float *, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_SNGL_SOAVECV_GET(number_of_components, array,
                               LPTX_array_get_pvf(number_of_components),
                               LPTX_get_particle_vector(vector_index));
}

LPTX_DECL_SET_FN(LPTX_set_particle_vector_f_soa, PACK,
                 (LPTX_idtype, SCL, vector_index), (float *, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_PACK_SOAVECV_SET(number_of_components, array,
                               LPTX_array_set_pvf(number_of_components),
                               LPTX_set_particle_vector(vector_index));
}

LPTX_DECL_GET_FN(LPTX_get_particle_vector_f_soa, PACK,
                 (LPTX_idtype, SCL, vector_index), (float *, array),
                 (LPTX_idtype, SCL, number_of_components))
{
  return LPTX_PACK_SOAVECV_GET(number_of_components, array,
                               LPTX_array_get_pvf(number_of_components),
                               LPTX_get_particle_vector(vector_index));
}

/* set fluid index from fluid grid */
LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_index, SRECTT)
{
  LPTX_particle_set_set_fluid_index_s(set, LPTX_RECT_GRID_T());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_index, PRECTT)
{
  LPTX_set_particle_fluid_index_s(param, LPTX_RECT_GRID_T());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_index_f, SRECTF)
{
  LPTX_particle_set_set_fluid_index_s(set, LPTX_RECT_GRID_F());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_index_f, PRECTF)
{
  LPTX_set_particle_fluid_index_s(param, LPTX_RECT_GRID_F());
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_index_d, SRECTD)
{
  LPTX_particle_set_set_fluid_index_s(set, LPTX_RECT_GRID_D());
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_index_d, PRECTD)
{
  LPTX_set_particle_fluid_index_s(param, LPTX_RECT_GRID_D());
}

static int LPTX_find_struct_index(const void *coords, LPTX_idtype n,
                                  LPTX_idtype m, LPTX_idtype stm, LPTX_type p,
                                  LPTX_cb_get_scalar_of *getter, LPTX_idtype *i)
{
  LPTX_idtype low, mid, high;

  if (getter(coords, 0) > p)
    return 0;
  if (getter(coords, m) < p)
    return 0;

  low = stm;
  high = stm + n + 1;
  while (low + 1 < high) {
    mid = (high - low) / 2 + low;
    if (getter(coords, mid) > p) {
      high = mid;
    } else {
      low = mid;
    }
  }
  *i = low - stm;
  return 1;
}

static LPTX_cb_fluid_index_flags
LPTX_particle_fluid_struct_index(void *args, LPTX_vector position,
                                 LPTX_idtype *i, LPTX_idtype *j, LPTX_idtype *k)
{
  const LPTX_rectilinear_grid *g;
  g = (const LPTX_rectilinear_grid *)args;

  if (LPTX_find_struct_index(g->coords_i, g->nx, g->mx, g->stmx,
                             LPTX_vector_x(position), g->get_coord_i, i) &&
      LPTX_find_struct_index(g->coords_j, g->ny, g->my, g->stmy,
                             LPTX_vector_y(position), g->get_coord_j, j) &&
      LPTX_find_struct_index(g->coords_k, g->nz, g->mz, g->stmz,
                             LPTX_vector_z(position), g->get_coord_k, k))
    return LPTX_FLUID_INDEX_OK;
  return LPTX_FLUID_INDEX_OUT_OF_DOMAIN;
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_index_s, SRECT)
{
  LPTX_particle_set_set_fluid_index_c(set, LPTX_particle_fluid_struct_index,
                                      (void *)grid);
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_index_s, PRECT)
{
  LPTX_set_particle_fluid_index_c(param, LPTX_particle_fluid_struct_index,
                                  (void *)grid);
}

LPTX_DECL_SET_FN(LPTX_particle_set_set_fluid_index_c, SFLCI)
{
  LPTX_SNGL_INDEX_FLUID_CB_SET();
}

LPTX_DECL_SET_FN(LPTX_set_particle_fluid_index_c, PFLCI)
{
  LPTX_PACK_INDEX_FLUID_CB_SET();
}

//---------------------------------

LPTX_idtype LPTX_particle_set_calc_mpirank_c(LPTX_particle_set *set,
                                             LPTX_idtype start,
                                             int size_of_rank, int *rank,
                                             LPTX_cb_mpirank *func, void *arg)
{
  LPTX_idtype np = LPTX_particle_set_number_of_particles(set);
  LPTX_idtype last = start + size_of_rank;
  if (last >= np)
    last = np;

#ifdef _OPENMP
#pragma omp for
#endif
  for (LPTX_idtype jj = start; jj < last; ++jj) {
    const LPTX_particle *dp;
    int r;
    r = -1;
    dp = &set->particles[jj].base;
    if (LPTX_particle_is_used(dp))
      r = func(arg, dp);
    rank[jj - start] = r;
  }

  return last - start;
}

struct LPTX_mpirank_struct_data
{
  int nproc;
  const LPTX_vector *lb;
  const LPTX_vector *ub;
  const int *flgs;
};

static int LPTX_mpirank_rect_cb(void *arg, const LPTX_particle *p)
{
  struct LPTX_mpirank_struct_data *d;
  LPTX_vector_rect_flags f;

  d = (struct LPTX_mpirank_struct_data *)arg;

  for (int ir = 0; ir < d->nproc; ++ir) {
    f = LPTX_vector_rect_in(p->position, d->lb[ir], d->ub[ir]);
    if (d->flgs) {
      if ((f & LPTX_VECTOR_RECT_P) && (f & d->flgs[ir]))
        return ir;
    } else {
      if (f == LPTX_VECTOR_RECT_P)
        return ir;
    }
  }
  return -1;
}

LPTX_idtype LPTX_particle_set_calc_mpirank_rect_v(
  LPTX_particle_set *set, LPTX_idtype start, int size_of_rank, int *rank,
  int nproc, const LPTX_vector *lb, const LPTX_vector *ut, const int *flgs)
{
  struct LPTX_mpirank_struct_data d;

  d.nproc = nproc;
  d.lb = lb;
  d.ub = ut;
  d.flgs = flgs;

  return LPTX_particle_set_calc_mpirank_c(set, start, size_of_rank, rank,
                                          LPTX_mpirank_rect_cb, &d);
}

int LPTX_param_redistribute_particles_rect_v(LPTX_param *param, LPTX_vector lb,
                                             LPTX_vector ub,
                                             LPTX_vector_rect_flags flg)
{
#ifdef JUPITER_LPTX_MPI
  int r;
  int nproc;
  MPI_Datatype vect, vectt;
  LPTX_vector *lba, *uba;
  int *flga;

  r = MPI_SUCCESS;
  vect = MPI_DATATYPE_NULL;
  vectt = MPI_DATATYPE_NULL;
  lba = NULL;
  uba = NULL;
  flga = NULL;

  do {
    const MPI_Aint vdispls[] = LPTX_vector_MPI_data(displs);
    const MPI_Datatype vtypes[] = LPTX_vector_MPI_data(types);
    const int vblls[] = LPTX_vector_MPI_data(len);
    const int vcnt = sizeof(vblls) / sizeof(vblls[0]);

    r = MPI_Comm_size(param->mpi_comm, &nproc);
    if (r != MPI_SUCCESS)
      break;

    lba = (LPTX_vector *)calloc(nproc * 2, sizeof(LPTX_vector));
    flga = (int *)calloc(nproc, sizeof(int));
    if (!lba || !flga) {
      r = MPI_ERR_NO_MEM;
      break;
    }

    uba = lba + nproc;

    r = MPI_Type_create_struct(vcnt, vblls, vdispls, vtypes, &vectt);
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Type_commit(&vectt);
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Type_create_resized(vectt, 0, sizeof(LPTX_vector), &vect);
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Type_commit(&vect);
    if (r != MPI_SUCCESS)
      break;
  } while (0);

  if (!LPTX_MPI_forall(r == MPI_SUCCESS, param->mpi_comm, &r)) {
    if (r == MPI_SUCCESS)
      r = MPI_ERR_NO_MEM;
  }

  do {
    struct LPTX_mpirank_struct_data d;
    int iflg = flg;

    if (r != MPI_SUCCESS)
      break;

    r = MPI_Allgather(&ub, 1, vect, uba, 1, vect, param->mpi_comm);
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Allgather(&lb, 1, vect, lba, 1, vect, param->mpi_comm);
    if (r != MPI_SUCCESS)
      break;

    r = MPI_Allgather(&iflg, 1, MPI_INT, flga, 1, MPI_INT, param->mpi_comm);
    if (r != MPI_SUCCESS)
      break;

    d.flgs = flga;
    d.lb = lba;
    d.ub = uba;
    d.nproc = nproc;
    r = LPTX_param_redistribute_particles(param, LPTX_mpirank_rect_cb, &d);
  } while (0);

  if (vectt != MPI_DATATYPE_NULL)
    MPI_Type_free(&vectt);
  if (vect != MPI_DATATYPE_NULL)
    MPI_Type_free(&vect);
  if (lba)
    free(lba);
  if (flga)
    free(flga);

  return r;
#else
  return 0;
#endif
}
