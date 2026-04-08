#include "common_util.h"
#include "csv.h"
#include "csvutil.h"
#include "func.h"
#include "geometry/bitarray.h"
#include "geometry/list.h"
#include "strlist.h"
#include "struct.h"
#include <limits.h>
#include <stdio.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#include <inttypes.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifdef _OPENMP
#include <omp.h>
#endif

#ifdef JUPITER_MPI
typedef MPI_Count validate_VOF_count;
typedef MPI_Offset validate_VOF_coords;
#define validate_VOF_count_MPITYPE MPI_COUNT
#define validate_VOF_coords_MPITYPE MPI_OFFSET
#else
typedef ptrdiff_t validate_VOF_count;
typedef ptrdiff_t validate_VOF_coords;
#endif

/**
 * Caveat: Errors will be printed in this order, if multiple conditions are
 * matched.
 */
enum validate_VOF_error
{
  VALIDATE_VOF_SUM_SOLID_VOF_GT1,        // sum(fs) > 1
  VALIDATE_VOF_SUM_LIQUID_VOF_GT1,       // sum(fl) > 1
  VALIDATE_VOF_SUM_VOFS_GE1,             // sum(fs) >= 1 and sum(fl) >= 1
  VALIDATE_VOF_SUM_SOLID_VOF_PARTS_GT1,  // sum(fs * Y) > 1
  VALIDATE_VOF_SUM_LIQUID_VOF_PARTS_GT1, // sum(fl * Y) > 1
  VALIDATE_VOF_SUM_VOF_GT1,              // fs + fl > 1

  VALIDATE_VOF_ERROR_MAX
};

struct validate_VOF_errors
{
  geom_bitarray_n(errors, VALIDATE_VOF_ERROR_MAX);
};

static void validate_VOF_errors_set(struct validate_VOF_errors *e,
                                    enum validate_VOF_error f, int value)
{
  geom_bitarray_element_set(e->errors, f, value);
}

static int validate_VOF_errors_get(const struct validate_VOF_errors *e,
                                   enum validate_VOF_error f)
{
  return geom_bitarray_element_get(e->errors, f);
}

static struct validate_VOF_errors validate_VOF_errors_zero(void)
{
  struct validate_VOF_errors e;
  geom_bitarray_element_setall(e.errors, VALIDATE_VOF_ERROR_MAX, 0);
  return e;
}

static int validate_VOF_errors_any(const struct validate_VOF_errors *e)
{
  return geom_bitarray_element_getany(e->errors, VALIDATE_VOF_ERROR_MAX);
}

/*
 * Validate VOF at cell jj
 */
static struct validate_VOF_errors validate_VOF_c(flags *flg, domain *cdo,
                                                 validate_VOF_coords jj,
                                                 type *Y, type *fs, type *fl)
{
  struct validate_VOF_errors e;
  int m = cdo->m;

  e = validate_VOF_errors_zero();

  if (flg->solute_diff == ON) {
    type fssum = 0.0, flsum = 0.0;
    type fsj = fs[jj], flj = fl[jj];

    for (int i = 0; i < cdo->NumberOfComponent; ++i) {
      type y = Y[jj + i * m];
      fssum += y * fsj;
      flsum += y * flj;
    }
    if (fssum > 1.0)
      validate_VOF_errors_set(&e, VALIDATE_VOF_SUM_SOLID_VOF_PARTS_GT1, 1);
    if (flsum > 1.0)
      validate_VOF_errors_set(&e, VALIDATE_VOF_SUM_LIQUID_VOF_PARTS_GT1, 1);
    if (fsj + flj > 1.0)
      validate_VOF_errors_set(&e, VALIDATE_VOF_SUM_VOF_GT1, 1);

  } else {
    type fssum = 0.0, flsum = 0.0;

    for (int i = 0; i < cdo->NumberOfComponent; ++i) {
      fssum += fs[jj + i * m];
      flsum += fl[jj + i * m];
    }
    if (fssum > 1.0)
      validate_VOF_errors_set(&e, VALIDATE_VOF_SUM_SOLID_VOF_GT1, 1);
    if (flsum > 1.0)
      validate_VOF_errors_set(&e, VALIDATE_VOF_SUM_LIQUID_VOF_GT1, 1);
    if (fssum >= 1.0 && flsum >= 1.0)
      validate_VOF_errors_set(&e, VALIDATE_VOF_SUM_VOFS_GE1, 1);
  }
  return e;
}

struct validate_VOF_init_link_data
{
  struct geom_list list;
  validate_VOF_count nout;
  validate_VOF_count maxout;
  struct validate_VOF_errors *eout;
  validate_VOF_coords *coords;
};

static struct validate_VOF_init_link_data *
validate_VOF_init_link_entry(struct geom_list *ptr)
{
  return geom_list_entry(ptr, struct validate_VOF_init_link_data, list);
}

static void
validate_VOF_init_link_data_init(struct validate_VOF_init_link_data *p)
{
  geom_list_init(&p->list);
  p->nout = 0;
  p->maxout = 0;
  p->eout = NULL;
  p->coords = NULL;
}

static struct validate_VOF_init_link_data *
validate_VOF_init_link_data_alloc(struct validate_VOF_init_link_data *head,
                                  validate_VOF_count allocate_size)
{
  struct validate_VOF_init_link_data *p;
  struct validate_VOF_errors *e;
  validate_VOF_coords *chrds;
  int err = 0;

  p = NULL;
  e = NULL;
  chrds = NULL;
  do {
    p = (struct validate_VOF_init_link_data *)malloc(
      sizeof(struct validate_VOF_init_link_data));
    if (!p) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      err = 1;
      break;
    }

    e = (struct validate_VOF_errors *)malloc(
      sizeof(struct validate_VOF_errors) * allocate_size);
    if (!e) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      err = 1;
      break;
    }

    chrds = (validate_VOF_coords *)malloc(sizeof(validate_VOF_coords) *
                                          allocate_size);
    if (!chrds) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      err = 1;
      break;
    }
  } while (0);
  if (err) {
    free(p);
    free(e);
    free(chrds);
    return NULL;
  }

  validate_VOF_init_link_data_init(p);
  p->maxout = allocate_size;
  p->eout = e;
  p->coords = chrds;
  geom_list_insert_prev(&head->list, &p->list);
  return p;
}

static int validate_VOF_init_link_data_add(
  struct validate_VOF_init_link_data *head, validate_VOF_count allocate_size,
  struct validate_VOF_errors e, validate_VOF_coords jj)
{
  struct geom_list *lp;
  struct validate_VOF_init_link_data *p;

  lp = geom_list_prev(&head->list);
  p = NULL;
  if (lp != &head->list)
    p = validate_VOF_init_link_entry(lp);
  if (!p || p->nout >= p->maxout) {
    p = validate_VOF_init_link_data_alloc(head, allocate_size);
    if (!p)
      return 1;
  }

  p->eout[p->nout] = e;
  p->coords[p->nout] = jj;
  p->nout++;
  return 0;
}

static void
validate_VOF_init_link_data_reset(struct validate_VOF_init_link_data *e)
{
  geom_list_delete(&e->list);
  if (e->eout)
    free(e->eout);
  if (e->coords)
    free(e->coords);
  e->eout = NULL;
  e->coords = NULL;
}

static void
validate_VOF_init_link_data_delete(struct validate_VOF_init_link_data *e)
{
  validate_VOF_init_link_data_reset(e);
  free(e);
}

static void
validate_VOF_init_link_data_delete_all(struct validate_VOF_init_link_data *head)
{
  struct geom_list *lp, *ln, *lh;

  lh = &head->list;
  geom_list_foreach_safe (lp, ln, lh) {
    struct validate_VOF_init_link_data *p;
    p = validate_VOF_init_link_entry(lp);
    validate_VOF_init_link_data_delete(p);
  }
}

static validate_VOF_coords validate_VOF_init_local2global(mpi_param *mpi,
                                                          domain *cdo, int jx,
                                                          int jy, int jz)
{
  jx += mpi->rank_x * cdo->nx;
  jy += mpi->rank_y * cdo->ny;
  jz += mpi->rank_z * cdo->nz;
  return calc_address(jx, jy, jz, cdo->gnx, cdo->gny, cdo->gnz);
}

static validate_VOF_coords
validate_VOF_init_localjj2global(mpi_param *mpi, domain *cdo,
                                 validate_VOF_coords jj)
{
  int jx, jy, jz;
  calc_struct_index(jj, cdo->mx, cdo->my, cdo->mz, &jx, &jy, &jz);
  jx -= cdo->stm;
  jy -= cdo->stm;
  jz -= cdo->stm;
  return validate_VOF_init_local2global(mpi, cdo, jx, jy, jz);
}

struct validate_VOF_init_local_find_data
{
  mpi_param *mpi;
  domain *cdo;
  flags *flg;
  validate_VOF_count maxout;
  validate_VOF_count nout;
  struct validate_VOF_errors *eout;
  validate_VOF_coords *coords;
  type *Y;
  type *fs;
  type *fl;
};

static struct validate_VOF_init_local_find_data
validate_VOF_init_local_find_data_init(mpi_param *mpi, domain *cdo, flags *flg,
                                       validate_VOF_count maxout,
                                       struct validate_VOF_errors *eout,
                                       validate_VOF_coords *coords, type *Y,
                                       type *fs, type *fl)
{
  return (struct validate_VOF_init_local_find_data){
    .mpi = mpi,
    .cdo = cdo,
    .flg = flg,
    .nout = 0,
    .maxout = maxout,
    .eout = eout,
    .coords = coords,
    .Y = Y,
    .fs = fs,
    .fl = fl,
  };
}

static int validate_VOF_init_find_func(ptrdiff_t jj, void *arg)
{
  struct validate_VOF_errors e;
  struct validate_VOF_init_local_find_data *p;
  validate_VOF_count n;
  p = (struct validate_VOF_init_local_find_data *)arg;

  e = validate_VOF_c(p->flg, p->cdo, jj, p->Y, p->fs, p->fl);
  if (!validate_VOF_errors_any(&e))
    return 0;

  if (p->maxout > 0) {
#ifdef _OPENMP
#pragma omp atomic read
#endif
    n = p->nout;

    if (n < p->maxout) {
#ifdef _OPENMP
#pragma omp critical
#endif
      {
        if (p->nout < p->maxout) {
          jj = validate_VOF_init_localjj2global(p->mpi, p->cdo, jj);
          p->eout[p->nout] = e;
          p->coords[p->nout] = jj;
          p->nout += 1;
        }
      }
    }
  }
  return 1;
}

/**
 * Find maximum of 1 (regardless of @p maxout parameters) error in local rank
 * efficiently.
 */
static validate_VOF_count validate_VOF_init_local_fast(
  mpi_param *mpi, domain *cdo, flags *flg, validate_VOF_count maxout,
  struct validate_VOF_errors *eout, validate_VOF_coords *coords, type *Y,
  type *fs, type *fl)
{
  struct validate_VOF_init_local_find_data d;
  d = validate_VOF_init_local_find_data_init(mpi, cdo, flg, maxout, eout,
                                             coords, Y, fs, fl);

  struct_domain_find_if(cdo->mx, cdo->my, cdo->mz, cdo->stm, cdo->stm, cdo->stm,
                        cdo->stp, cdo->stp, cdo->stp,
                        validate_VOF_init_find_func, &d);
  return d.nout;
}

/**
 * Find maximum of @p maxout error(s) in local rank.
 */
static validate_VOF_count validate_VOF_init_local_n(
  mpi_param *mpi, domain *cdo, flags *flg, validate_VOF_count maxout,
  struct validate_VOF_errors *eout, validate_VOF_coords *coords, type *Y,
  type *fs, type *fl)
{
  struct validate_VOF_init_local_find_data d;
  d = validate_VOF_init_local_find_data_init(mpi, cdo, flg, maxout, eout,
                                             coords, Y, fs, fl);

  struct_domain_find_n_if(cdo->mx, cdo->my, cdo->mz, cdo->stm, cdo->stm,
                          cdo->stm, cdo->stp, cdo->stp, cdo->stp, maxout,
                          validate_VOF_init_find_func, &d);
  return d.nout;
}

/**
 * Find all errors in local rank.
 */
static validate_VOF_count
validate_VOF_init_local_all(mpi_param *mpi, domain *cdo, flags *flg,
                            struct validate_VOF_init_link_data *head, type *Y,
                            type *fs, type *fl)
{
  const ptrdiff_t alloc_size = 128;
  validate_VOF_count sout = 0;

#ifdef _OPENMP
#pragma omp parallel
#endif
  do {
    ptrdiff_t lsout, lecnt;
    ptrdiff_t is, ie;
    struct validate_VOF_init_link_data lhead;

    geom_list_init(&lhead.list);

    distribute_thread_1d(0, cdo->n, NULL, NULL, &is, &ie, NULL, NULL, NULL);

    for (ptrdiff_t i = is; i < ie; ++i) {
      int jx, jy, jz, jlx, jly, jlz;
      ptrdiff_t jj;
      struct validate_VOF_errors e;

      if (i > is) {
#ifdef _OPENMP
#pragma omp atomic read
#endif
        lsout = sout;
        if (lsout < 0)
          break;
      }

      calc_struct_index(i, cdo->nx, cdo->ny, cdo->nz, &jx, &jy, &jz);
      jlx = jx + cdo->stm;
      jly = jy + cdo->stm;
      jlz = jz + cdo->stm;
      jj = calc_address(jlx, jly, jlz, cdo->mx, cdo->my, cdo->mz);

      e = validate_VOF_c(flg, cdo, jj, Y, fs, fl);
      if (validate_VOF_errors_any(&e)) {
        ptrdiff_t jg;
        jg = validate_VOF_init_local2global(mpi, cdo, jx, jy, jz);
        if (validate_VOF_init_link_data_add(&lhead, alloc_size, e, jg)) {
#ifdef _OPENMP
#pragma omp atomic write
#endif
          sout = -1;
          break;
        }
      }
    }

#ifdef _OPENMP
#pragma omp barrier
#pragma omp atomic read
#endif
    lsout = sout;
    if (lsout < 0) {
      validate_VOF_init_link_data_delete_all(&lhead);
      break;
    }

    lecnt = 0;
    {
      struct geom_list *lp, *lh;
      lh = &lhead.list;
      geom_list_foreach (lp, lh) {
        struct validate_VOF_init_link_data *p;
        p = validate_VOF_init_link_entry(lp);
        lecnt += p->nout;
      }
    }

#ifdef _OPENMP
#pragma omp atomic update
#endif
    sout += lecnt;

    if (!geom_list_empty(&lhead.list)) {
#ifdef _OPENMP
#pragma omp critical
#endif
      {
        geom_list_insert_list_prev(&head->list, &lhead.list);
        geom_list_delete(&lhead.list);
      }
    }

    validate_VOF_init_link_data_delete_all(&lhead);
  } while (0);

  return sout;
}

//-----

static validate_VOF_count
validate_VOF_count_nout(struct validate_VOF_init_link_data *lhead,
                        int *nentry_out)
{
  int nentry;
  validate_VOF_count nout;
  struct geom_list *lp, *lh;
  nout = 0;
  nentry = 0;
  lh = &lhead->list;
  geom_list_foreach (lp, lh) {
    struct validate_VOF_init_link_data *l;
    l = validate_VOF_init_link_entry(lp);
    nout += l->nout;
    nentry += 1;
  }
  if (nentry_out)
    *nentry_out = nentry;
  return nout;
}

static void validate_VOF_concat_raw(struct validate_VOF_init_link_data *lhead,
                                    struct validate_VOF_init_link_data *lskip,
                                    struct validate_VOF_errors **eout,
                                    validate_VOF_coords **coordsout)
{
#ifdef _OPENMP
#pragma omp single
#endif
  {
    struct geom_list *lp, *lh;
    struct validate_VOF_errors *e;
    validate_VOF_coords *c;

    lh = &lhead->list;
    geom_list_foreach (lp, lh) {
      struct validate_VOF_init_link_data *l;
      l = validate_VOF_init_link_entry(lp);
      if (lskip && l == lskip)
        continue;

      e = *eout;
      c = *coordsout;
      *eout += l->nout;
      *coordsout += l->nout;

      /*
       * note: Assumes l->nout is small enough not to need to parallelize
       * copying elements
       */
#ifdef _OPENMP
#pragma omp task
#endif
      {
        memcpy(e, l->eout, l->nout * sizeof(struct validate_VOF_errors));
        memcpy(c, l->coords, l->nout * sizeof(validate_VOF_coords));
      }
    }
  }
}

static int validate_VOF_concat(struct validate_VOF_init_link_data *lhead,
                               struct validate_VOF_init_link_data **lout,
                               int *status)
{
  struct validate_VOF_init_link_data *lo;
  validate_VOF_count noutalloc;
  int nent;

  noutalloc = validate_VOF_count_nout(lhead, &nent);
  if (noutalloc <= 0) {
    *lout = NULL;
    return 0;
  }
  if (nent <= 1) {
    *lout = validate_VOF_init_link_entry(geom_list_next(&lhead->list));
    return 0;
  }

  lo = validate_VOF_init_link_data_alloc(lhead, noutalloc);
  if (!lo) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    if (status)
      *status = ON;
    return 1;
  }

#ifdef _OPENMP
#pragma omp parallel
#endif
  {
    struct validate_VOF_errors *eout = lo->eout;
    validate_VOF_count *cout = lo->coords;

    validate_VOF_concat_raw(lhead, lo, &eout, &cout);
  }

  lo->nout = noutalloc;
  *lout = lo;
  return 0;
}

#ifdef JUPITER_MPI
static int validate_VOF_create_error_mpitype(MPI_Datatype *outp)
{
  const int count = geom_bitarray_n_word(VALIDATE_VOF_ERROR_MAX);
  const MPI_Aint displs = offsetof(struct validate_VOF_errors, errors);
  const MPI_Datatype baset = GEOM_BITARRAY_ELEMENT_MPI_BASE_TYPE;
  MPI_Datatype t;
  int r;

  r = MPI_Type_create_struct(1, &count, &displs, &baset, &t);
  if (r != MPI_SUCCESS) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_MPI, 0, r,
              NULL);
    *outp = MPI_DATATYPE_NULL;
    return r;
  }

  r = MPI_Type_create_resized(t, 0, sizeof(struct validate_VOF_errors), outp);
  MPI_Type_free(&t);
  if (r != MPI_SUCCESS) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_MPI, 0, r,
              NULL);
    return r;
  }

  r = MPI_Type_commit(outp);
  if (r != MPI_SUCCESS) {
    MPI_Type_free(outp);
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_MPI, 0, r,
              NULL);
    return r;
  }

  return r;
}

static int validate_VOF_gather(mpi_param *mpi, int root, int num_print,
                               struct validate_VOF_init_link_data *lhead,
                               struct validate_VOF_init_link_data **lout,
                               int *status)
{
  MPI_Datatype etype = MPI_DATATYPE_NULL;
  struct geom_list *lp, *lh;
  struct validate_VOF_init_link_data *lo;
  validate_VOF_count *noutproc;
  validate_VOF_count noutalloc;
  int r;

  CSVASSERT(num_print != 0);
  CSVASSERT(mpi->npe > 1); /* Use validate_VOF_concat() if mpi->npe == 1 */

  noutproc = (validate_VOF_count *)calloc(mpi->npe, sizeof(validate_VOF_count));
  if (!mpi_for_all_rank(mpi, !!noutproc)) {
    if (!noutproc) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
    }
    free(noutproc);
    return 1;
  }

  do {
    noutproc[mpi->rank] = validate_VOF_count_nout(lhead, NULL);

    r = MPI_Allgather(MPI_IN_PLACE, 1, validate_VOF_count_MPITYPE, noutproc, 1,
                      validate_VOF_count_MPITYPE, mpi->CommJUPITER);
    if (r != MPI_SUCCESS) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL, CSV_ERR_MPI, 0, r,
                NULL);
      if (status)
        *status = ON;
      break;
    }

    if (num_print > 0) {
      int nprem = num_print;

      for (int i = 0; i < mpi->npe; ++i) {
        int ir = i + root;
        if (ir >= mpi->npe)
          ir -= mpi->npe;

        if (nprem >= noutproc[ir]) {
          nprem -= noutproc[ir];
        } else {
          noutproc[ir] = nprem;
          nprem = 0;
        }
      }
    }

    noutalloc = 0;
    for (int ir = 0; ir < mpi->npe; ++ir)
      noutalloc += noutproc[ir];

    lo = NULL;
    if (mpi->rank == root) {
      if (num_print < 0) {
        lo = validate_VOF_init_link_data_alloc(lhead, noutalloc);
        if (!lo) {
          csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                    0, NULL);
        }
      } else {
        /* Assumes that 1 (stack-)allocated entry stores the enough storage */
        lo = validate_VOF_init_link_entry(geom_list_next(&lhead->list));
        CSVASSERT_X(lo->maxout >= noutalloc,
                    "Failed to compute the number of entries for gathering "
                    "entries from other ranks");
      }
    }
  } while (0);
  if (!for_all_rank(mpi, mpi->rank != root || lo)) {
    if (status)
      *status = ON;
    free(noutproc);
    return 1;
  }

  do {
    struct validate_VOF_errors *eout;
    validate_VOF_coords *cout;
    int r = MPI_SUCCESS;

    if (mpi->rank == root) {
      eout = lo->eout;
      cout = lo->coords;
      if (num_print < 0) {
#ifdef _OPENMP
#pragma omp parallel
#endif
        validate_VOF_concat_raw(lhead, lo, &eout, &cout);
      } else {
        eout += lo->nout;
        cout += lo->nout;
      }
    }

    for (int i = 1; i < mpi->npe; ++i) {
      int ir = i + root;
      if (ir >= mpi->npe)
        ir -= mpi->npe;

      if (noutproc[ir] > 0) {
        if (etype == MPI_DATATYPE_NULL) {
          CSVASSERT(r == MPI_SUCCESS);
          r = validate_VOF_create_error_mpitype(&etype);
        }
        if (!mpi_for_all_rank(mpi, r == MPI_SUCCESS)) /* (a) */
          break;

        if (mpi->rank == ir) { /* sender */
          struct geom_list *lp, *lh;
          validate_VOF_count lsend = noutproc[mpi->rank];

          lh = &lhead->list;
          geom_list_foreach (lp, lh) {
            struct validate_VOF_init_link_data *l;
            struct validate_VOF_errors *e;
            validate_VOF_coords *c;
            validate_VOF_count ln;

            l = validate_VOF_init_link_entry(lp);
            if (l->nout <= 0)
              continue; /* skip */

            ln = l->nout;
            e = l->eout;
            c = l->coords;

            if (lsend < ln)
              ln = lsend;

            while (ln > 0) {
              int n;
              if (ln > INT_MAX) {
                n = INT_MAX;
              } else {
                n = ln;
              }

              r = MPI_Send(&n, 1, MPI_INT, root, 0, mpi->CommJUPITER);
              if (r != MPI_SUCCESS)
                break;

              r = MPI_Send(e, n, etype, root, 0, mpi->CommJUPITER);
              if (r != MPI_SUCCESS)
                break;

              r = MPI_Send(c, n, validate_VOF_coords_MPITYPE, root, 0,
                           mpi->CommJUPITER);
              if (r != MPI_SUCCESS)
                break;

              e += n;
              c += n;
              ln -= n;
            }
            if (r != MPI_SUCCESS)
              break;

            lsend -= ln;
          }
          if (r != MPI_SUCCESS)
            continue;

          r = MPI_Send((int[]){0}, 1, MPI_INT, root, 0, mpi->CommJUPITER);
          if (r != MPI_SUCCESS)
            continue;

        } else if (mpi->rank == root) { /* receiver */
          struct validate_VOF_errors *eend = lo->eout + lo->maxout;
          while (1) {
            int n;

            r = MPI_Recv(&n, 1, MPI_INT, ir, 0, mpi->CommJUPITER,
                         MPI_STATUS_IGNORE);
            if (r != MPI_SUCCESS)
              break;
            if (n <= 0)
              break;

            CSVASSERT(eout + n <= eend);

            r = MPI_Recv(eout, n, etype, ir, 0, mpi->CommJUPITER,
                         MPI_STATUS_IGNORE);
            if (r != MPI_SUCCESS)
              break;

            r = MPI_Recv(cout, n, validate_VOF_coords_MPITYPE, ir, 0,
                         mpi->CommJUPITER, MPI_STATUS_IGNORE);
            if (r != MPI_SUCCESS)
              break;

            eout += n;
            cout += n;
          }
        }
      }
    }

    if (mpi->rank == root)
      lo->nout = noutalloc;
  } while (0);

  free(noutproc);
  if (etype != MPI_DATATYPE_NULL)
    MPI_Type_free(&etype);

  *lout = lo;
  return 0;
}
#endif

//-----

enum validate_VOF_init_msgsep_wrap
{
  VALIDATE_VOF_INIT_MSGSEP_WRAP,
  VALIDATE_VOF_INIT_MSGSEP_TITLE,
  VALIDATE_VOF_INIT_MSGSEP_CONTINUE,
};

static int validate_VOF_init_msgsep_create(
  jupiter_strlist *l, int num_print, const char *title, validate_VOF_count nout,
  validate_VOF_count tout, enum validate_VOF_init_msgsep_wrap i, int *status)
{
  const char *m = "";
  jupiter_strlist *n;

  switch (i) {
  case VALIDATE_VOF_INIT_MSGSEP_TITLE:
    if (num_print > 0 && tout >= num_print) {
      m = "least ";
    }

    n = jupiter_strlist_asprintf(" * %s found at %s%" PRIdMAX " cells:\n    ",
                                 title, m, nout);
    break;
  case VALIDATE_VOF_INIT_MSGSEP_WRAP:
    n = jupiter_strlist_dup_s(",\n    ");
    break;
  case VALIDATE_VOF_INIT_MSGSEP_CONTINUE:
    n = jupiter_strlist_dup_s(", ");
    break;
  }

  if (n) {
#ifdef _OPENMP
#pragma omp critical
#endif
    jupiter_strlist_insert_prev(l, n);
  } else {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    if (status) {
#ifdef _OPENMP
#pragma omp atomic write
#endif
      *status = ON;
    }
    return 1;
  }

  return 0;
}

static int validate_VOF_init_msgsep_create_end(jupiter_strlist_head *head,
                                               int *status)
{
  jupiter_strlist *l = jupiter_strlist_dup_s("\n\n");
  if (l) {
#ifdef _OPENMP
#pragma omp critical
#endif
    jupiter_strlist_append(head, l);
  } else {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);

    if (status) {
#ifdef _OPENMP
#pragma omp atomic write
#endif
      *status = ON;
    }
    return 1;
  }
  return 0;
}

struct validate_VOF_init_msgsep_data
{
  const char *title;
  int *status;
  int num_print;
  validate_VOF_count n, nout, tout;
  validate_VOF_count ch;
};

static int validate_VOF_init_add_msgsep(jupiter_strlist *l, void *arg)
{
  struct validate_VOF_init_msgsep_data *p;
  const validate_VOF_count wrap_factor = 55;
  enum validate_VOF_init_msgsep_wrap i;

  p = (struct validate_VOF_init_msgsep_data *)arg;

  i = VALIDATE_VOF_INIT_MSGSEP_CONTINUE;
  p->ch += l->node.len;
  if (p->ch > wrap_factor) {
    p->ch = l->node.len;
    i = VALIDATE_VOF_INIT_MSGSEP_WRAP;
  }
  if (p->n++ == 0)
    i = VALIDATE_VOF_INIT_MSGSEP_TITLE;

#ifdef _OPENMP
#pragma omp task
  validate_VOF_init_msgsep_create(l, p->num_print, p->title, p->nout, p->tout,
                                  i, p->status);
#else
  if (validate_VOF_init_msgsep_create(l, p->num_print, p->title, p->nout,
                                      p->tout, i, p->status))
    return 1;
#endif
  return 0;
}

struct validate_VOF_message_sort_node
{
  validate_VOF_coords jj;
  jupiter_strlist *l;
};

static int validate_VOF_message_sort_comp(const void *a, const void *b)
{
  struct validate_VOF_message_sort_node *pa, *pb;
  pa = (struct validate_VOF_message_sort_node *)a;
  pb = (struct validate_VOF_message_sort_node *)b;
  if (pa->l && pb->l)
    return pa->jj - pb->jj;
  if (pa->l)
    return -1; /* pa->jj - MAX */
  if (pb->l)
    return 1; /* MAX - pb->jj */
  return 0;
}

static int validate_VOF_init_build_message(
  jupiter_strlist_head *shead, domain *cdo, flags *flg, validate_VOF_count nout,
  struct validate_VOF_errors *errs, validate_VOF_coords *coords, int num_print,
  enum validate_VOF_error e, const char *title,
  struct validate_VOF_message_sort_node *sortbuf, int *status)
{
  int xstatus = OFF;
  validate_VOF_count n = 0, si = 0;
  jupiter_strlist_head ghead;
  jupiter_strlist_head_init(&ghead);

  CSVASSERT(nout > 0);

#ifdef _OPENMP
#pragma omp parallel if (nout >= 100)
#endif
  do {
    validate_VOF_count ln = 0;
    ptrdiff_t is, ie;
    int gnx, gny, gnz;
    jupiter_strlist_head lhead;
    jupiter_strlist_head_init(&lhead);

    gnx = cdo->gnx;
    gny = cdo->gny;
    gnz = cdo->gnz;

    distribute_thread_1d(0, nout, NULL, NULL, &is, &ie, NULL, NULL, NULL);

    for (ptrdiff_t i = is; i < ie; ++i) {
      sortbuf[i].jj = -1;
      sortbuf[i].l = NULL;
    }
#ifdef _OPENMP
#pragma omp barrier
#endif

    for (ptrdiff_t i = is; i < ie; ++i) {
      int jx, jy, jz;
      validate_VOF_coords jj;
      jupiter_strlist *l;

      if (i > is) {
        validate_VOF_count xn;
#ifdef _OPENMP
#pragma omp atomic read
#endif
        xn = n;
        if (xn < 0)
          break;
      }

      if (!validate_VOF_errors_get(&errs[i], e))
        continue;

      ln += 1;
      jj = coords[i];
      calc_struct_index(jj, gnx, gny, gnz, &jx, &jy, &jz);
      l = jupiter_strlist_asprintf("(%d, %d, %d)", jx, jy, jz);
      if (!l) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
#ifdef _OPENMP
#pragma omp atomic write
#endif
        n = -1;
        break;
      }

      sortbuf[i].jj = jj;
      sortbuf[i].l = l;
      jupiter_strlist_append(&lhead, l);
    }

    {
      ptrdiff_t xn;
#ifdef _OPENMP
#pragma omp atomic read
#endif
      xn = n;
      if (xn < 0) {
        jupiter_strlist_free_all(&lhead);
        break;
      }
    }

#ifdef _OPENMP
#pragma omp barrier
#pragma omp atomic update
#endif
    n += ln;

    if (ln > 0) {
#ifdef _OPENMP
#pragma omp critical
#endif
      jupiter_strlist_append_list(&ghead, &lhead);
    }
    jupiter_strlist_free_all(&lhead);
  } while (0);
  if (n < 0) {
    if (status)
      *status = ON;
    return 1;
  }
  if (n <= 0)
    return 0;

  qsort(sortbuf, nout, sizeof(struct validate_VOF_message_sort_node),
        validate_VOF_message_sort_comp);

  for (validate_VOF_count i = 0; i < nout; ++i) {
    if (!sortbuf[i].l)
      break;

    jupiter_strlist_delete(sortbuf[i].l);
    jupiter_strlist_append(&ghead, sortbuf[i].l);
  }

  {
    struct validate_VOF_init_msgsep_data d = {
      .status = &xstatus,
      .n = 0,
      .ch = 0,
      .num_print = num_print,
      .nout = n,
      .tout = nout,
      .title = title,
    };

#ifdef _OPENMP
#pragma omp parallel if (nout >= 100)
#pragma omp single
#endif
    jupiter_strlist_foreach_all_safe(&ghead, validate_VOF_init_add_msgsep, &d);
  }

  validate_VOF_init_msgsep_create_end(&ghead, &xstatus);
  if (xstatus == ON) {
    if (status)
      *status = ON;
    jupiter_strlist_free_all(&ghead);
    return 1;
  }

  jupiter_strlist_append_list(shead, &ghead);
  jupiter_strlist_free_all(&ghead);
  return 0;
}

static int validate_VOF_init_print(domain *cdo, flags *flg, int num_print,
                                   validate_VOF_count nout,
                                   struct validate_VOF_errors *errs,
                                   validate_VOF_coords *coords, int *status)
{
  jupiter_strlist_head shead;
  struct validate_VOF_message_sort_node *sortbuf;
  jupiter_strlist *s;
  static const char *const message_data[VALIDATE_VOF_ERROR_MAX] = {
    [VALIDATE_VOF_SUM_SOLID_VOF_GT1] = "Sum of Solid VOFs is greater than 1",
    [VALIDATE_VOF_SUM_LIQUID_VOF_GT1] = "Sum of Liquid VOFs is greater than 1",

    [VALIDATE_VOF_SUM_VOFS_GE1] =
      "Sum of Solid VOFs and sum of Liquid VOFs are greater than or equal to 1",

    [VALIDATE_VOF_SUM_SOLID_VOF_PARTS_GT1] =
      "Sum of Solid amounts (VOF * mass fraction) is greater than 1",

    [VALIDATE_VOF_SUM_LIQUID_VOF_PARTS_GT1] =
      "Sum of Liquid amounts (VOF * mass fraction) is greater than 1",

    [VALIDATE_VOF_SUM_VOF_GT1] =
      "Sum of Solid and Liquid VOFs is greater than 1",
  };

  CSVASSERT(nout > 0);

  sortbuf = (struct validate_VOF_message_sort_node *)malloc(
    nout * sizeof(struct validate_VOF_message_sort_node));
  if (!sortbuf) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    if (status)
      *status = ON;
    return 1;
  }

  jupiter_strlist_head_init(&shead);

  for (int i = 0; i < VALIDATE_VOF_ERROR_MAX; ++i) {
    if (validate_VOF_init_build_message(&shead, cdo, flg, nout, errs, coords,
                                        num_print, (enum validate_VOF_error)i,
                                        message_data[i], sortbuf, status)) {
      jupiter_strlist_free_all(&shead);
      free(sortbuf);
      if (status)
        *status = ON;
      return 1;
    }
  }
  free(sortbuf);

  if (jupiter_strlist_is_empty(&shead)) {
    jupiter_strlist_free_all(&shead);
    return 0;
  }

  s =
    jupiter_strlist_dup_s("Following error(s) found while VOF validation:\n\n");
  if (!s) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    jupiter_strlist_free_all(&shead);
    if (status)
      *status = ON;
    return 1;
  }

  jupiter_strlist_prepend(&shead, s);
  csvperrorl(NULL, 0, 0, CSV_EL_ERROR, NULL, &shead);

  jupiter_strlist_free_all(&shead);
  return 0;
}

/*
 * find and print all errors
 */
static int validate_VOF_init_all(mpi_param *mpi, domain *cdo, flags *flg,
                                 type *Y, type *fs, type *fl, int *status)
{
  struct validate_VOF_init_link_data lhead;
  struct validate_VOF_init_link_data *lout;
  validate_VOF_count nout;

  validate_VOF_init_link_data_init(&lhead);
  nout = validate_VOF_init_local_all(mpi, cdo, flg, &lhead, Y, fs, fl);

  do {
    int r;

#ifdef JUPITER_MPI
    if (mpi->npe > 1) {
      r = validate_VOF_gather(mpi, 0, -1, &lhead, &lout, status);
      if (r)
        break;
    } else {
      if (validate_VOF_concat(&lhead, &lout, status))
        break;
    }
#else
    if (validate_VOF_concat(&lhead, &lout, status))
      break;
#endif

    if (mpi->rank == 0) {
      if (for_any_rank(mpi, lout->nout > 0)) {
        validate_VOF_init_print(cdo, flg, -1, lout->nout, lout->eout,
                                lout->coords, status);
        if (status)
          *status = ON;
      }
    } else {
      if (for_any_rank(mpi, 0))
        if (status)
          *status = ON;
    }
  } while (0);

  validate_VOF_init_link_data_delete_all(&lhead);
  return 0;
}

/*
 * find and print maximum of N errors
 */
static int validate_VOF_init_n(mpi_param *mpi, domain *cdo, flags *flg,
                               int num_print, type *Y, type *fs, type *fl,
                               int *status)
{
  struct validate_VOF_init_link_data lhead, le;
  struct validate_VOF_init_link_data *lout;
  struct validate_VOF_errors *e = NULL;
  validate_VOF_coords *coords = NULL;

  validate_VOF_init_link_data_init(&lhead);
  validate_VOF_init_link_data_init(&le);
  geom_list_insert_prev(&le.list, &lhead.list);

  do {
    e = (struct validate_VOF_errors *)malloc(
      sizeof(struct validate_VOF_errors) * num_print);
    if (!e) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      break;
    }

    coords =
      (validate_VOF_coords *)malloc(sizeof(validate_VOF_coords) * num_print);
    if (!coords) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      break;
    }
  } while (0);
  if (!mpi_for_all_rank(mpi, e && coords)) {
    if (e)
      free(e);
    if (coords)
      free(coords);
    return 1;
  }

  le.maxout = num_print;
  le.eout = e;
  le.coords = coords;
  le.nout =
    validate_VOF_init_local_n(mpi, cdo, flg, num_print, e, coords, Y, fs, fl);

  do {
    lout = &le;

#ifdef JUPITER_MPI
    if (mpi->npe > 1) {
      int r;
      r = validate_VOF_gather(mpi, 0, num_print, &lhead, &lout, status);
      if (r)
        break;

      if (mpi->rank == 0)
        CSVASSERT(lout == &le);
    }
#endif

    if (mpi->rank == 0) {
      if (for_any_rank(mpi, lout->nout > 0)) {
        validate_VOF_init_print(cdo, flg, num_print, lout->nout, lout->eout,
                                lout->coords, status);
        if (status)
          *status = ON;
      }
    } else {
      if (for_any_rank(mpi, 0))
        if (status)
          *status = ON;
    }
  } while (0);

  free(e);
  free(coords);
  return 0;
}

/*
 * find and print maximum 1 error
 */
static int validate_VOF_init_1(mpi_param *mpi, domain *cdo, flags *flg,
                               int num_print, type *Y, type *fs, type *fl,
                               int *status)
{
  struct validate_VOF_init_link_data lhead, le;
  struct validate_VOF_errors e;
  validate_VOF_coords coords;

  CSVASSERT(num_print <= 1);

  validate_VOF_init_link_data_init(&lhead);
  validate_VOF_init_link_data_init(&le);
  geom_list_insert_prev(&le.list, &lhead.list);

  le.coords = &coords;
  le.eout = &e;
  le.maxout = 1;
  le.nout =
    validate_VOF_init_local_fast(mpi, cdo, flg, 1, &e, &coords, Y, fs, fl);

  if (num_print > 0) {
    struct validate_VOF_init_link_data *lout = &le;

    do {
#ifdef JUPITER_MPI
      if (mpi->npe > 1) {
        int r;
        r = validate_VOF_gather(mpi, 0, 1, &lhead, &lout, status);
        if (r)
          break;

        if (mpi->rank == 0) CSVASSERT(lout == &le);
      }
#endif

      if (mpi->rank == 0) {
        if (for_any_rank(mpi, lout->nout > 0)) {
          validate_VOF_init_print(cdo, flg, 1, lout->nout, lout->eout,
                                  lout->coords, status);
          if (status)
            *status = ON;
        }
      } else {
        if (for_any_rank(mpi, 0))
          if (status)
            *status = ON;
      }
    } while (0);

  } else {
    if (for_any_rank(mpi, le.nout > 0)) {
      if (mpi->rank == 0) {
        csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                   "Found VOF value error (details omitted)");
        if (status)
          *status = ON;
      }
    }
  }

  return 0;
}

int validate_VOF_init(mpi_param *mpi, domain *cdo, flags *flg, int num_print,
                      type *Y, type *fs, type *fl, int *status)
{
  CSVASSERT(mpi);
  CSVASSERT(cdo);
  CSVASSERT(flg);
  CSVASSERT(fs);
  CSVASSERT(fl);
  if (flg->solute_diff == ON) {
    CSVASSERT(Y);
  }

  /* Use num_print value passed in rank == 0 */
#ifdef JUPITER_MPI
  if (mpi->npe > 1) {
    int r;
    r = MPI_Bcast(&num_print, 1, MPI_INT, 0, mpi->CommJUPITER);
    if (r != MPI_SUCCESS) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_MPI, 0, r,
                NULL);
      return 1;
    }
  }
#endif

  if (num_print < 0) {
    return validate_VOF_init_all(mpi, cdo, flg, Y, fs, fl, status);
  } else if (num_print > 1) {
    return validate_VOF_init_n(mpi, cdo, flg, num_print, Y, fs, fl, status);
  } else {
    return validate_VOF_init_1(mpi, cdo, flg, num_print, Y, fs, fl, status);
  }
}
