#include "jupiter/geometry/defs.h"
#include "jupiter/lptx/comm.h"
#include "jupiter/lptx/defs.h"
#include "jupiter/lptx/overflow.h"
#include "jupiter/lptx/param.h"
#include "jupiter/lptx/particle.h"
#include "jupiter/lptx/priv_util.h"
#include "jupiter/lptx/ptflags.h"
#include "jupiter/lptx/pvector.h"
#include "jupiter/lptx/vector.h"
#include "jupiter/random/random.h"
#include "lptx-test.h"
#include "test-util.h"

#include <inttypes.h>
#include <stdint.h>
#include <stdio.h>

#ifdef JUPTIER_LPTX_MPI
#include <mpi.h>
#endif

static int mpi_rank_s(void *args, const LPTX_particle *p)
{
  if (p->particle_id == 902)
    return 1;
  return 0;
}

static int mpi_rank_r(void *args, const LPTX_particle *p) { return 0; }

int test_lptx_comm(void)
{
  LPTX_param *param = NULL;
  LPTX_particle_set *set = NULL;
  LPTX_particle_set *s1 = NULL;
  LPTX_particle_set *s2 = NULL;
  LPTX_particle_set *s3 = NULL;
  LPTX_particle_set *s4 = NULL;
  LPTX_particle_set *s5 = NULL;
  int *sends, *dests, *recvs;
  LPTX_idtype *recvids;
  int r = 0;
  int irank = -1, nproc = -1;

  sends = NULL;
  dests = NULL;
  recvs = NULL;
  recvids = NULL;

#ifdef JUPITER_LPTX_MPI
  nproc = test_util_mpi_nproc();
  irank = test_util_mpi_rank();

  do {
    if (nproc > 1) {
      if (!test_compare_ii(LPTX_MPI_forall(irank == 0, MPI_COMM_WORLD, NULL),
                           ==, LPTX_false))
        r = 1;

      if (!test_compare_ii(LPTX_MPI_forall(LPTX_true, MPI_COMM_WORLD, NULL), ==,
                           LPTX_true))
        r = 1;

      if (!test_compare_ii(LPTX_MPI_forany(irank == 0, MPI_COMM_WORLD, NULL),
                           ==, LPTX_true))
        r = 1;

      if (!test_compare_ii(LPTX_MPI_forany(irank == -1, MPI_COMM_WORLD, NULL),
                           ==, LPTX_false))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_MPI_forall(irank == 0, MPI_COMM_WORLD, NULL),
                           ==, LPTX_true))
        r = 1;

      if (!test_compare_ii(LPTX_MPI_forall(LPTX_true, MPI_COMM_WORLD, NULL), ==,
                           LPTX_true))
        r = 1;

      if (!test_compare_ii(LPTX_MPI_forany(irank == 0, MPI_COMM_WORLD, NULL),
                           ==, LPTX_true))
        r = 1;

      if (!test_compare_ii(LPTX_MPI_forany(irank == -1, MPI_COMM_WORLD, NULL),
                           ==, LPTX_false))
        r = 1;
    }

    if (nproc < 2)
      break;

    switch (irank) {
    case 0:
      if (!test_compare_ii(LPTX_MPI_p2pall(irank == 0, 1, 0, MPI_COMM_WORLD,
                                           NULL),
                           ==, LPTX_false))
        r = 1;

      if (!test_compare_ii(LPTX_MPI_p2pall(irank == 0 || irank == 1, 1, 0,
                                           MPI_COMM_WORLD, NULL),
                           ==, LPTX_true))
        r = 1;

      if (!test_compare_ii(LPTX_MPI_p2pany(irank == 0, 1, 0, MPI_COMM_WORLD,
                                           NULL),
                           ==, LPTX_true))
        r = 1;

      if (!test_compare_ii(LPTX_MPI_p2pany(irank == -1, 1, 0, MPI_COMM_WORLD,
                                           NULL),
                           ==, LPTX_false))
        r = 1;
      break;
    case 1:
      if (!test_compare_ii(LPTX_MPI_p2pall(irank == 0, 0, 0, MPI_COMM_WORLD,
                                           NULL),
                           ==, LPTX_false))
        r = 1;

      if (!test_compare_ii(LPTX_MPI_p2pall(irank == 0 || irank == 1, 0, 0,
                                           MPI_COMM_WORLD, NULL),
                           ==, LPTX_true))
        r = 1;

      if (!test_compare_ii(LPTX_MPI_p2pany(irank == 0, 0, 0, MPI_COMM_WORLD,
                                           NULL),
                           ==, LPTX_true))
        r = 1;

      if (!test_compare_ii(LPTX_MPI_p2pany(irank == -1, 0, 0, MPI_COMM_WORLD,
                                           NULL),
                           ==, LPTX_false))
        r = 1;
      break;
    }

    MPI_Allreduce(MPI_IN_PLACE, &r, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  } while (0);

  if (r)
    return 1;

  do {
    int ier = MPI_SUCCESS;
    int tval = 0;

    LPTX_MPI_coll_sync (MPI_COMM_WORLD, &ier) {
      ++tval;
    }

    if (!test_compare_ii(ier, ==, MPI_SUCCESS))
      r = 1;
    if (!test_compare_ii(tval, ==, 1))
      r = 1;

    tval = 0;
    ier = LPTX_MPI_ERR_NO_MEM;
    LPTX_MPI_coll_sync (MPI_COMM_WORLD, &ier) {
      ++tval;
    }

    if (!test_compare_ii(ier, ==, LPTX_MPI_ERR_NO_MEM))
      r = 1;
    if (!test_compare_ii(tval, ==, 0))
      r = 1;

    ier = MPI_SUCCESS;
    LPTX_MPI_coll_sync (MPI_COMM_WORLD, &ier) {
      if (irank == 0)
        ier = LPTX_MPI_ERR_NO_MEM;
    }

    if (irank == 0) {
      if (!test_compare_ii(ier, ==, LPTX_MPI_ERR_NO_MEM))
        r = 1;
    } else {
      if (!test_compare_ii(ier, ==, LPTX_MPI_ERR_REMOTE))
        r = 1;
    }

    ier = MPI_SUCCESS;
    LPTX_MPI_coll_sync (MPI_COMM_WORLD, &ier) {
      if (irank == 0) {
        ier = LPTX_MPI_ERR_NO_MEM;
        continue;
      }

      if (irank == 1) {
        ier = LPTX_MPI_ERR_OVERFLOW;
        continue;
      }
    }

    if (irank == 0) {
      if (!test_compare_ii(ier, ==, LPTX_MPI_ERR_NO_MEM))
        r = 1;
    } else if (irank == 1) {
      if (!test_compare_ii(ier, ==, LPTX_MPI_ERR_OVERFLOW))
        r = 1;
    } else {
      if (!test_compare_ii(ier, ==, LPTX_MPI_ERR_REMOTE))
        r = 1;
    }

    ier = MPI_SUCCESS;
    LPTX_MPI_coll_sync (MPI_COMM_WORLD, &ier) {
      if (irank == 0) {
        ier = LPTX_MPI_ERR_NO_MEM;
        break;
      }

      break;
    }

    if (irank == 0) {
      if (!test_compare_ii(ier, ==, LPTX_MPI_ERR_NO_MEM))
        r = 1;
    } else {
      if (!test_compare_ii(ier, ==, LPTX_MPI_ERR_REMOTE))
        r = 1;
    }

    if (nproc < 2)
      break;

    if (irank < 2) {
      int peer;
      peer = irank == 1 ? 0 : 1;
      ier = MPI_SUCCESS;
      tval = 0;

      LPTX_MPI_p2p_sync (peer, 0, MPI_COMM_WORLD, &ier) {
        ++tval;
      }

      if (!test_compare_ii(ier, ==, MPI_SUCCESS))
        r = 1;
      if (!test_compare_ii(tval, ==, 1))
        r = 1;

      tval = 0;
      ier = LPTX_MPI_ERR_NO_MEM;
      LPTX_MPI_p2p_sync (peer, 0, MPI_COMM_WORLD, &ier) {
        ++tval;
      }

      if (!test_compare_ii(ier, ==, LPTX_MPI_ERR_NO_MEM))
        r = 1;
      if (!test_compare_ii(tval, ==, 0))
        r = 1;

      ier = MPI_SUCCESS;
      LPTX_MPI_p2p_sync (peer, 0, MPI_COMM_WORLD, &ier) {
        if (irank == 0)
          ier = LPTX_MPI_ERR_NO_MEM;
      }

      if (irank == 0) {
        if (!test_compare_ii(ier, ==, LPTX_MPI_ERR_NO_MEM))
          r = 1;
      } else {
        if (!test_compare_ii(ier, ==, LPTX_MPI_ERR_REMOTE))
          r = 1;
      }

      ier = MPI_SUCCESS;
      LPTX_MPI_p2p_sync (peer, 0, MPI_COMM_WORLD, &ier) {
        if (irank == 0) {
          ier = LPTX_MPI_ERR_NO_MEM;
          continue;
        }
      }

      if (irank == 0) {
        if (!test_compare_ii(ier, ==, LPTX_MPI_ERR_NO_MEM))
          r = 1;
      } else {
        if (!test_compare_ii(ier, ==, LPTX_MPI_ERR_REMOTE))
          r = 1;
      }

      ier = MPI_SUCCESS;
      LPTX_MPI_p2p_sync (peer, 0, MPI_COMM_WORLD, &ier) {
        if (irank == 0) {
          ier = LPTX_MPI_ERR_NO_MEM;
          break;
        }

        break;
      }

      if (irank == 0) {
        if (!test_compare_ii(ier, ==, LPTX_MPI_ERR_NO_MEM))
          r = 1;
      } else {
        if (!test_compare_ii(ier, ==, LPTX_MPI_ERR_REMOTE))
          r = 1;
      }
    }

    MPI_Allreduce(MPI_IN_PLACE, &r, 1, MPI_INT, MPI_MAX, MPI_COMM_WORLD);
  } while (0);

  do {
    if (r)
      break;

    if (!test_compare_ii(MPI_Comm_set_errhandler(MPI_COMM_WORLD,
                                                 MPI_ERRORS_RETURN),
                         ==, MPI_SUCCESS)) {
      r = 1;
      break;
    }
  } while (0);

  if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
    r = 1;

  do {
    LPTX_particle_set *lset;

    if (r)
      break;

    if (nproc < 2 || nproc > 8)
      break;

    if (s2)
      LPTX_particle_set_delete(s2);
    s2 = NULL;

    switch (irank) {
    case 0:
      if (!test_compare_pp((s1 = LPTX_particle_set_new(7, 0, NULL)), !=, NULL))
        r = 1;
      break;
    case 1:
      if (!test_compare_pp((s1 = LPTX_particle_set_new(3, 4,
                                                       (LPTX_idtype[]){1, 1, 5,
                                                                       4})),
                           !=, NULL))
        r = 1;
      break;
    case 2:
      if (!test_compare_pp((s1 = LPTX_particle_set_new(0, 2,
                                                       (LPTX_idtype[]){3, 3})),
                           !=, NULL))
        r = 1;
      break;
    case 3:
      if (!test_compare_pp((s1 =
                              LPTX_particle_set_new(7, 1, (LPTX_idtype[]){1})),
                           !=, NULL))
        r = 1;
      break;
    case 4:
      if (!test_compare_pp((s1 = LPTX_particle_set_new(9, 0, NULL)), !=, NULL))
        r = 1;
      break;
    case 5:
      if (!test_compare_pp((s1 =
                              LPTX_particle_set_new(0, 3,
                                                    (LPTX_idtype[]){1, 2, 2})),
                           !=, NULL))
        r = 1;
      break;
    case 6:
      if (!test_compare_pp((s1 = LPTX_particle_set_new(2, 2,
                                                       (LPTX_idtype[]){3, 5})),
                           !=, NULL))
        r = 1;
      break;
    default:
      if (!test_compare_pp((s1 = LPTX_particle_set_new(4, 2,
                                                       (LPTX_idtype[]){1, 3})),
                           !=, NULL))
        r = 1;
    }

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_replicate_send(s1, 6, 1, 0,
                                                            MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else if (irank == 1) {
      if (!test_compare_ii(LPTX_particle_set_replicate_recv(&s2, 0, 0,
                                                            MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;

      do {
        if (r)
          break;

        if (!test_compare_pp(s2, !=, NULL))
          r = 1;

        if (r)
          break;

        if (!test_compare_ii(s2->number_particles, ==, 6))
          r = 1;
        if (!test_compare_ii(s2->number_vectors, ==, 0))
          r = 1;
        if (!test_compare_pp(s2->vector_sizes, ==, NULL))
          r = 1;

        LPTX_particle_set_delete(s2);
        s2 = NULL;
      } while (0);
    }
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (irank == 1) {
      if (!test_compare_ii(LPTX_particle_set_replicate_send(s1, 11, 0, 0,
                                                            MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_replicate_recv(&s2, 1, 0,
                                                            MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;

      do {
        if (r)
          break;

        if (!test_compare_pp(s2, !=, NULL))
          r = 1;

        if (r)
          break;

        if (!test_compare_ii(s2->number_particles, ==, 11))
          r = 1;
        if (!test_compare_ii(s2->number_vectors, ==, 4))
          r = 1;
        if (!test_compare_pp(s2->vector_sizes, !=, NULL))
          r = 1;
        if (s2->vector_sizes) {
          if (!test_compare_ii(s2->vector_sizes[0], ==, 1))
            r = 1;
          if (!test_compare_ii(s2->vector_sizes[1], ==, 1))
            r = 1;
          if (!test_compare_ii(s2->vector_sizes[2], ==, 5))
            r = 1;
          if (!test_compare_ii(s2->vector_sizes[3], ==, 4))
            r = 1;
        }

        LPTX_particle_set_delete(s2);
        s2 = NULL;
      } while (0);
    }
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_replicate_bcast(&s2, s1, 5, 0, 0,
                                                             LPTX_true,
                                                             MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_replicate_bcast(&s2, s1, 0, 0, 0,
                                                             LPTX_true,
                                                             MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (!test_compare_pp(s2, !=, NULL))
        r = 1;

      if (r)
        break;

      if (!test_compare_ii(s2->number_particles, ==, 5))
        r = 1;
      if (!test_compare_ii(s2->number_vectors, ==, 0))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, ==, NULL))
        r = 1;

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_replicate_bcast(&s2, s1, 7, 0, 1,
                                                             LPTX_true,
                                                             MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_replicate_bcast(&s2, s1, 0, 0, 1,
                                                             LPTX_true,
                                                             MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (!test_compare_pp(s2, !=, NULL))
        r = 1;

      if (r)
        break;

      if (!test_compare_ii(s2->number_particles, ==, 7))
        r = 1;
      if (!test_compare_ii(s2->number_vectors, ==, 4))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, !=, NULL))
        r = 1;
      if (s2->vector_sizes) {
        if (!test_compare_ii(s2->vector_sizes[0], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[1], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[2], ==, 5))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[3], ==, 4))
          r = 1;
      }

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_replicate_bcast(&s2, NULL, 9, 0,
                                                             MPI_ANY_SOURCE,
                                                             LPTX_true,
                                                             MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_replicate_bcast(&s2, s1, 9, 0,
                                                             MPI_ANY_SOURCE,
                                                             LPTX_true,
                                                             MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (!test_compare_pp(s2, !=, NULL))
        r = 1;

      if (r)
        break;

      if (!test_compare_ii(s2->number_particles, ==, 9))
        r = 1;
      if (!test_compare_ii(s2->number_vectors, ==, 4))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, !=, NULL))
        r = 1;
      if (s2->vector_sizes) {
        if (!test_compare_ii(s2->vector_sizes[0], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[1], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[2], ==, 5))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[3], ==, 4))
          r = 1;
      }

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_replicate_bcast(&s2, s1, 0, 1,
                                                             MPI_ANY_SOURCE,
                                                             LPTX_true,
                                                             MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_replicate_bcast(&s2, NULL, 8, 1,
                                                             MPI_ANY_SOURCE,
                                                             LPTX_true,
                                                             MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (!test_compare_pp(s2, !=, NULL))
        r = 1;

      if (r)
        break;

      if (!test_compare_ii(s2->number_particles, ==, 8))
        r = 1;
      if (!test_compare_ii(s2->number_vectors, ==, 0))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, ==, NULL))
        r = 1;

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);

    if (!test_compare_ii(LPTX_particle_set_replicate_bcast(&s2, s1, 11, 0, 1,
                                                           LPTX_false,
                                                           MPI_COMM_WORLD),
                         ==, MPI_SUCCESS))
      r = 1;
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (irank == 1) {
        /*
         * s2 is allowed to be return any value, but should be guaranteed not to
         * cause memory leaks. In current behavior, s2 is left unchanged.
         */
        if (!test_compare_pp(s2, ==, NULL))
          r = 1;
        s2 = NULL;
        break;
      }

      if (!test_compare_pp(s2, !=, NULL))
        r = 1;

      if (r)
        break;

      if (!test_compare_ii(s2->number_particles, ==, 11))
        r = 1;
      if (!test_compare_ii(s2->number_vectors, ==, 4))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, !=, NULL))
        r = 1;
      if (s2->vector_sizes) {
        if (!test_compare_ii(s2->vector_sizes[0], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[1], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[2], ==, 5))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[3], ==, 4))
          r = 1;
      }

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(LPTX_particle_set_replicate_gather(&s2, s1, 4, 0, 0,
                                                            MPI_COMM_WORLD),
                         ==, MPI_SUCCESS))
      r = 1;
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (irank != 0) {
        if (!test_compare_pp(s2, ==, NULL))
          r = 1;
        break;
      }

      if (!test_compare_pp(s2, !=, NULL))
        r = 1;
      if (r)
        break;

      if (!test_compare_ii(s2->number_particles, ==, 4 * nproc))
        r = 1;
      if (!test_compare_ii(s2->number_vectors, ==, 0))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, ==, NULL))
        r = 1;

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(LPTX_particle_set_replicate_gather(&s2, s1, 3, 0, 1,
                                                            MPI_COMM_WORLD),
                         ==, MPI_SUCCESS))
      r = 1;
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (irank != 0) {
        if (!test_compare_pp(s2, ==, NULL))
          r = 1;
        break;
      }

      if (!test_compare_pp(s2, !=, NULL))
        r = 1;
      if (r)
        break;

      if (!test_compare_ii(s2->number_particles, ==, 3 * nproc))
        r = 1;
      if (!test_compare_ii(s2->number_vectors, ==, 4))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, !=, NULL))
        r = 1;
      if (s2->vector_sizes) {
        if (!test_compare_ii(s2->vector_sizes[0], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[1], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[2], ==, 5))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[3], ==, 4))
          r = 1;
      }

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_replicate_gather(&s2, NULL, 5, 0,
                                                              MPI_ANY_SOURCE,
                                                              MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_replicate_gather(&s2, s1, 5, 0,
                                                              MPI_ANY_SOURCE,
                                                              MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (irank != 0) {
        if (!test_compare_pp(s2, ==, NULL))
          r = 1;
        break;
      }

      if (!test_compare_pp(s2, !=, NULL))
        r = 1;
      if (r)
        break;

      if (!test_compare_ii(s2->number_particles, ==, 5 * nproc))
        r = 1;
      if (!test_compare_ii(s2->number_vectors, ==, 4))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, !=, NULL))
        r = 1;
      if (s2->vector_sizes) {
        if (!test_compare_ii(s2->vector_sizes[0], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[1], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[2], ==, 5))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[3], ==, 4))
          r = 1;
      }

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_replicate_gather(&s2, s1, 6, 1,
                                                              MPI_ANY_SOURCE,
                                                              MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_replicate_gather(&s2, NULL, 2, 1,
                                                              MPI_ANY_SOURCE,
                                                              MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (irank != 1) {
        if (!test_compare_pp(s2, ==, NULL))
          r = 1;
        break;
      }

      if (!test_compare_pp(s2, !=, NULL))
        r = 1;
      if (r)
        break;

      if (!test_compare_ii(s2->number_particles, ==, 2 * nproc + 4))
        r = 1;
      if (!test_compare_ii(s2->number_vectors, ==, 0))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, ==, NULL))
        r = 1;

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_replicate_gather(&s2, s1, 6, 1,
                                                              MPI_ANY_SOURCE,
                                                              MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_replicate_gather(&s2, NULL, 2, 1,
                                                              MPI_ANY_SOURCE,
                                                              MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (irank != 1) {
        if (!test_compare_pp(s2, ==, NULL))
          r = 1;
        break;
      }

      if (!test_compare_pp(s2, !=, NULL))
        r = 1;
      if (r)
        break;

      if (!test_compare_ii(s2->number_particles, ==, 2 * nproc + 4))
        r = 1;
      if (!test_compare_ii(s2->number_vectors, ==, 0))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, ==, NULL))
        r = 1;

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(LPTX_particle_set_replicate_allgather(&s2, s1, 3, 0,
                                                               MPI_COMM_WORLD),
                         ==, MPI_SUCCESS))
      r = 1;
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (!test_compare_pp(s2, !=, NULL))
        r = 1;
      if (r)
        break;

      if (!test_compare_ii(s2->number_particles, ==, 3 * nproc))
        r = 1;
      if (!test_compare_ii(s2->number_vectors, ==, 0))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, ==, NULL))
        r = 1;

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (nproc > 2) {
      if (irank == 1) {
        if (!test_compare_ii(LPTX_particle_set_replicate_allgather(
                               &s2, NULL, 3, 2, MPI_COMM_WORLD),
                             ==, MPI_SUCCESS))
          r = 1;
      } else {
        if (!test_compare_ii(LPTX_particle_set_replicate_allgather(
                               &s2, s1, 3, 2, MPI_COMM_WORLD),
                             ==, MPI_SUCCESS))
          r = 1;
      }
      if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
        r = 1;
      if (r)
        break;

      do {
        if (!test_compare_pp(s2, !=, NULL))
          r = 1;
        if (r)
          break;

        if (!test_compare_ii(s2->number_particles, ==, 3 * nproc))
          r = 1;
        if (!test_compare_ii(s2->number_vectors, ==, 2))
          r = 1;
        if (!test_compare_pp(s2->vector_sizes, !=, NULL))
          r = 1;
        if (s2->vector_sizes) {
          if (!test_compare_ii(s2->vector_sizes[0], ==, 3))
            r = 1;
          if (!test_compare_ii(s2->vector_sizes[1], ==, 3))
            r = 1;
        }

        LPTX_particle_set_delete(s2);
        s2 = NULL;
      } while (0);
      if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
        r = 1;
      if (r)
        break;
    }

    if (!test_compare_ii(LPTX_particle_set_replicate_allgather(&s2, s1, 9,
                                                               MPI_ANY_SOURCE,
                                                               MPI_COMM_WORLD),
                         ==, MPI_SUCCESS))
      r = 1;
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (!test_compare_pp(s2, !=, NULL))
        r = 1;
      if (r)
        break;

      /*
       * If all processes passes non-NULL particle set and set_root is
       * MPI_ANY_SOURCE, given particle set is used as vector information
       * and does not check consistency
       */
      if (!test_compare_ii(s2->number_particles, ==, 9 * nproc))
        r = 1;
      if (!test_compare_ii(s2->number_vectors, ==, s1->number_vectors))
        r = 1;
      if (!test_compare_ii(!!s2->vector_sizes, ==, !!s1->vector_sizes))
        r = 1;
      for (LPTX_idtype i = 0; i < s1->number_vectors && i < s2->number_vectors;
           ++i) {
        if (!test_compare_ii(s2->vector_sizes[i], ==, s1->vector_sizes[i]))
          r = 1;
      }

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_replicate_allgather(
                             &s2, NULL, 0, MPI_ANY_SOURCE, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_replicate_allgather(
                             &s2, s1, irank, MPI_ANY_SOURCE, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (!test_compare_pp(s2, !=, NULL))
        r = 1;
      if (r)
        break;

      if (!test_compare_ii(s2->number_particles, ==, nproc * (nproc - 1) / 2))
        r = 1;
      if (!test_compare_ii(s2->number_vectors, ==, 4))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, !=, NULL))
        r = 1;
      if (s2->vector_sizes) {
        if (!test_compare_ii(s2->vector_sizes[0], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[1], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[2], ==, 5))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[3], ==, 4))
          r = 1;
      }

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_replicate_scatter(
                             &s2, s1, (LPTX_idtype[]){8, 0, 3, 8, 5, 0, 5, 5},
                             0, 0, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_replicate_scatter(&s2, s1, NULL, 0,
                                                               0,
                                                               MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (!test_compare_pp(s2, !=, NULL))
        r = 1;
      if (r)
        break;

      switch (irank) {
      case 0:
        if (!test_compare_ii(s2->number_particles, ==, 8))
          r = 1;
        break;
      case 1:
        if (!test_compare_ii(s2->number_particles, ==, 0))
          r = 1;
        break;
      case 2:
        if (!test_compare_ii(s2->number_particles, ==, 3))
          r = 1;
        break;
      case 3:
        if (!test_compare_ii(s2->number_particles, ==, 8))
          r = 1;
        break;
      case 4:
        if (!test_compare_ii(s2->number_particles, ==, 5))
          r = 1;
        break;
      case 5:
        if (!test_compare_ii(s2->number_particles, ==, 0))
          r = 1;
        break;
      case 6:
        if (!test_compare_ii(s2->number_particles, ==, 5))
          r = 1;
        break;
      case 7:
        if (!test_compare_ii(s2->number_particles, ==, 5))
          r = 1;
        break;
      default:
        fprintf(stderr, "..... Unreachable reached: %s:%d\n", __FILE__,
                __LINE__);
        r = 1;
        break;
      }
      if (!test_compare_ii(s2->number_vectors, ==, 0))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, ==, NULL))
        r = 1;

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_replicate_scatter(
                             &s2, NULL, (LPTX_idtype[]){2, 9, 8, 9, 8, 0, 8, 2},
                             0, MPI_ANY_SOURCE, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_replicate_scatter(&s2, s1, NULL, 0,
                                                               MPI_ANY_SOURCE,
                                                               MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (!test_compare_pp(s2, !=, NULL))
        r = 1;
      if (r)
        break;

      switch (irank) {
      case 0:
        if (!test_compare_ii(s2->number_particles, ==, 2))
          r = 1;
        break;
      case 1:
        if (!test_compare_ii(s2->number_particles, ==, 9))
          r = 1;
        break;
      case 2:
        if (!test_compare_ii(s2->number_particles, ==, 8))
          r = 1;
        break;
      case 3:
        if (!test_compare_ii(s2->number_particles, ==, 9))
          r = 1;
        break;
      case 4:
        if (!test_compare_ii(s2->number_particles, ==, 8))
          r = 1;
        break;
      case 5:
        if (!test_compare_ii(s2->number_particles, ==, 0))
          r = 1;
        break;
      case 6:
        if (!test_compare_ii(s2->number_particles, ==, 8))
          r = 1;
        break;
      case 7:
        if (!test_compare_ii(s2->number_particles, ==, 2))
          r = 1;
        break;
      default:
        fprintf(stderr, "..... Unreachable reached: %s:%d\n", __FILE__,
                __LINE__);
        r = 1;
        break;
      }
      if (!test_compare_ii(s2->number_vectors, ==, 4))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, !=, NULL))
        r = 1;
      if (s2->vector_sizes) {
        if (!test_compare_ii(s2->vector_sizes[0], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[1], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[2], ==, 5))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[3], ==, 4))
          r = 1;
      }

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_replicate_scatter(
                             &s2, NULL, (LPTX_idtype[]){2, 9, 8, 9, 8, 0, 8, 2},
                             0, MPI_ANY_SOURCE, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_replicate_scatter(&s2, s1, NULL, 0,
                                                               MPI_ANY_SOURCE,
                                                               MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (!test_compare_pp(s2, !=, NULL))
        r = 1;
      if (r)
        break;

      switch (irank) {
      case 0:
        if (!test_compare_ii(s2->number_particles, ==, 2))
          r = 1;
        break;
      case 1:
        if (!test_compare_ii(s2->number_particles, ==, 9))
          r = 1;
        break;
      case 2:
        if (!test_compare_ii(s2->number_particles, ==, 8))
          r = 1;
        break;
      case 3:
        if (!test_compare_ii(s2->number_particles, ==, 9))
          r = 1;
        break;
      case 4:
        if (!test_compare_ii(s2->number_particles, ==, 8))
          r = 1;
        break;
      case 5:
        if (!test_compare_ii(s2->number_particles, ==, 0))
          r = 1;
        break;
      case 6:
        if (!test_compare_ii(s2->number_particles, ==, 8))
          r = 1;
        break;
      case 7:
        if (!test_compare_ii(s2->number_particles, ==, 2))
          r = 1;
        break;
      default:
        fprintf(stderr, "..... Unreachable reached: %s:%d\n", __FILE__,
                __LINE__);
        r = 1;
        break;
      }
      if (!test_compare_ii(s2->number_vectors, ==, 4))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, !=, NULL))
        r = 1;
      if (s2->vector_sizes) {
        if (!test_compare_ii(s2->vector_sizes[0], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[1], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[2], ==, 5))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[3], ==, 4))
          r = 1;
      }

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    switch (irank) {
    case 0:
      if (!test_compare_ii(LPTX_particle_set_replicate_alltoall(
                             &s2, s1, (LPTX_idtype[]){2, 8, 1, 1, 0, 3, 9, 5},
                             0, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
      break;
    case 1:
      if (!test_compare_ii(LPTX_particle_set_replicate_alltoall(
                             &s2, s1, (LPTX_idtype[]){8, 5, 1, 2, 7, 3, 9, 7},
                             0, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
      break;
    case 2:
      if (!test_compare_ii(LPTX_particle_set_replicate_alltoall(
                             &s2, s1, (LPTX_idtype[]){4, 4, 6, 6, 3, 2, 4, 1},
                             0, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
      break;
    case 3:
      if (!test_compare_ii(LPTX_particle_set_replicate_alltoall(
                             &s2, s1, (LPTX_idtype[]){6, 9, 1, 5, 0, 6, 7, 4},
                             0, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
      break;
    case 4:
      if (!test_compare_ii(LPTX_particle_set_replicate_alltoall(
                             &s2, s1, (LPTX_idtype[]){2, 7, 4, 8, 8, 9, 0, 1},
                             0, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
      break;
    case 5:
      if (!test_compare_ii(LPTX_particle_set_replicate_alltoall(
                             &s2, s1, (LPTX_idtype[]){2, 4, 6, 4, 8, 6, 1, 0},
                             0, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
      break;
    case 6:
      if (!test_compare_ii(LPTX_particle_set_replicate_alltoall(
                             &s2, s1, (LPTX_idtype[]){6, 2, 4, 4, 4, 6, 9, 7},
                             0, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
      break;
    default:
      if (!test_compare_ii(LPTX_particle_set_replicate_alltoall(
                             &s2, s1, (LPTX_idtype[]){1, 9, 6, 7, 3, 6, 0, 1},
                             0, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
      break;
    }
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (!test_compare_pp(s2, !=, NULL))
        r = 1;
      if (r)
        break;

      switch (nproc) {
      case 2:
        switch (irank) {
        case 0:
          if (!test_compare_ii(s2->number_particles, ==, 2 + 8))
            r = 1;
          break;
        default: /* 1 */
          if (!test_compare_ii(s2->number_particles, ==, 8 + 5))
            r = 1;
          break;
        }
        break;

      case 3:
        switch (irank) {
        case 0:
          if (!test_compare_ii(s2->number_particles, ==, 2 + 8 + 4))
            r = 1;
          break;
        case 1:
          if (!test_compare_ii(s2->number_particles, ==, 8 + 5 + 4))
            r = 1;
          break;
        default: /* 2 */
          if (!test_compare_ii(s2->number_particles, ==, 1 + 1 + 6))
            r = 1;
          break;
        }
        break;

      case 4:
        switch (irank) {
        case 0:
          if (!test_compare_ii(s2->number_particles, ==, 2 + 8 + 4 + 6))
            r = 1;
          break;
        case 1:
          if (!test_compare_ii(s2->number_particles, ==, 8 + 5 + 4 + 9))
            r = 1;
          break;
        case 2:
          if (!test_compare_ii(s2->number_particles, ==, 1 + 1 + 6 + 1))
            r = 1;
          break;
        default: /* 3 */
          if (!test_compare_ii(s2->number_particles, ==, 1 + 2 + 6 + 5))
            r = 1;
          break;
        }
        break;

      case 5:
        switch (irank) {
        case 0:
          if (!test_compare_ii(s2->number_particles, ==, 2 + 8 + 4 + 6 + 2))
            r = 1;
          break;
        case 1:
          if (!test_compare_ii(s2->number_particles, ==, 8 + 5 + 4 + 9 + 7))
            r = 1;
          break;
        case 2:
          if (!test_compare_ii(s2->number_particles, ==, 1 + 1 + 6 + 1 + 4))
            r = 1;
          break;
        case 3:
          if (!test_compare_ii(s2->number_particles, ==, 1 + 2 + 6 + 5 + 8))
            r = 1;
          break;
        default: /* 4 */
          if (!test_compare_ii(s2->number_particles, ==, 0 + 7 + 3 + 0 + 8))
            r = 1;
          break;
        }
        break;

      case 6:
        switch (irank) {
        case 0:
          if (!test_compare_ii(s2->number_particles, ==, 2 + 8 + 4 + 6 + 2 + 2))
            r = 1;
          break;
        case 1:
          if (!test_compare_ii(s2->number_particles, ==, 8 + 5 + 4 + 9 + 7 + 4))
            r = 1;
          break;
        case 2:
          if (!test_compare_ii(s2->number_particles, ==, 1 + 1 + 6 + 1 + 4 + 6))
            r = 1;
          break;
        case 3:
          if (!test_compare_ii(s2->number_particles, ==, 1 + 2 + 6 + 5 + 8 + 4))
            r = 1;
          break;
        case 4:
          if (!test_compare_ii(s2->number_particles, ==, 0 + 7 + 3 + 0 + 8 + 8))
            r = 1;
          break;
        default: /* 5 */
          if (!test_compare_ii(s2->number_particles, ==, 3 + 3 + 2 + 6 + 9 + 6))
            r = 1;
          break;
        }

      case 7:
        switch (irank) {
        case 0:
          if (!test_compare_ii(s2->number_particles, ==,
                               2 + 8 + 4 + 6 + 2 + 2 + 6))
            r = 1;
          break;
        case 1:
          if (!test_compare_ii(s2->number_particles, ==,
                               8 + 5 + 4 + 9 + 7 + 4 + 2))
            r = 1;
          break;
        case 2:
          if (!test_compare_ii(s2->number_particles, ==,
                               1 + 1 + 6 + 1 + 4 + 6 + 4))
            r = 1;
          break;
        case 3:
          if (!test_compare_ii(s2->number_particles, ==,
                               1 + 2 + 6 + 5 + 8 + 4 + 4))
            r = 1;
          break;
        case 4:
          if (!test_compare_ii(s2->number_particles, ==,
                               0 + 7 + 3 + 0 + 8 + 8 + 4))
            r = 1;
          break;
        case 5:
          if (!test_compare_ii(s2->number_particles, ==,
                               3 + 3 + 2 + 6 + 9 + 6 + 6))
            r = 1;
          break;
        default: /* 6 */
          if (!test_compare_ii(s2->number_particles, ==,
                               9 + 9 + 4 + 7 + 0 + 1 + 9))
            r = 1;
          break;
        }
        break;

      default:
        if (!test_compare_ii(nproc, ==, 8)) {
          r = 1;
          break;
        }

        switch (irank) {
        case 0:
          if (!test_compare_ii(s2->number_particles, ==,
                               2 + 8 + 4 + 6 + 2 + 2 + 6 + 1))
            r = 1;
          break;
        case 1:
          if (!test_compare_ii(s2->number_particles, ==,
                               8 + 5 + 4 + 9 + 7 + 4 + 2 + 9))
            r = 1;
          break;
        case 2:
          if (!test_compare_ii(s2->number_particles, ==,
                               1 + 1 + 6 + 1 + 4 + 6 + 4 + 6))
            r = 1;
          break;
        case 3:
          if (!test_compare_ii(s2->number_particles, ==,
                               1 + 2 + 6 + 5 + 8 + 4 + 4 + 7))
            r = 1;
          break;
        case 4:
          if (!test_compare_ii(s2->number_particles, ==,
                               0 + 7 + 3 + 0 + 8 + 8 + 4 + 3))
            r = 1;
          break;
        case 5:
          if (!test_compare_ii(s2->number_particles, ==,
                               3 + 3 + 2 + 6 + 9 + 6 + 6 + 6))
            r = 1;
          break;
        case 6:
          if (!test_compare_ii(s2->number_particles, ==,
                               9 + 9 + 4 + 7 + 0 + 1 + 9 + 0))
            r = 1;
          break;
        default: /* 7 */
          if (!test_compare_ii(s2->number_particles, ==,
                               5 + 7 + 1 + 4 + 1 + 0 + 7 + 1))
            r = 1;
          break;
        }
        break;
      }
      if (!test_compare_ii(s2->number_vectors, ==, 0))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, ==, NULL))
        r = 1;

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_replicate_alltoall(
                             &s2, NULL, (LPTX_idtype[]){0, 1, 2, 3, 4, 5, 6, 7},
                             MPI_ANY_SOURCE, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_replicate_alltoall(
                             &s2, s1, (LPTX_idtype[]){0, 1, 2, 3, 4, 5, 6, 7},
                             MPI_ANY_SOURCE, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (!test_compare_pp(s2, !=, NULL))
        r = 1;
      if (r)
        break;

      if (!test_compare_ii(s2->number_particles, ==, irank * nproc))
        r = 1;
      if (!test_compare_ii(s2->number_vectors, ==, 4))
        r = 1;
      if (!test_compare_pp(s2->vector_sizes, !=, NULL))
        r = 1;
      if (s2->vector_sizes) {
        if (!test_compare_ii(s2->vector_sizes[0], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[1], ==, 1))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[2], ==, 5))
          r = 1;
        if (!test_compare_ii(s2->vector_sizes[3], ==, 4))
          r = 1;
      }

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    if (!test_compare_ii(LPTX_particle_set_replicate_alltoall(
                           &s2, s1, (LPTX_idtype[]){0, 1, 2, 3, 4, 5, 6, 7},
                           MPI_ANY_SOURCE, MPI_COMM_WORLD),
                         ==, MPI_SUCCESS))
      r = 1;
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

    do {
      if (!test_compare_pp(s2, !=, NULL))
        r = 1;
      if (r)
        break;

      /*
       * If all processes passes non-NULL particle set and set_root is
       * MPI_ANY_SOURCE, given particle set is used as vector information
       * and does not check consistency
       */
      if (!test_compare_ii(s2->number_particles, ==, irank * nproc))
        r = 1;
      if (!test_compare_ii(s2->number_vectors, ==, s1->number_vectors))
        r = 1;
      if (!test_compare_ii(!!s2->vector_sizes, ==, !!s1->vector_sizes))
        r = 1;
      for (LPTX_idtype i = 0; i < s1->number_vectors && i < s2->number_vectors;
           ++i) {
        if (!test_compare_ii(s2->vector_sizes[i], ==, s1->vector_sizes[i]))
          r = 1;
      }

      LPTX_particle_set_delete(s2);
      s2 = NULL;
    } while (0);
    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
      r = 1;
    if (r)
      break;

  } while (0);

  do {
    if (r)
      break;

    if (set)
      LPTX_particle_set_delete(set);
    if (s1)
      LPTX_particle_set_delete(s1);
    if (s2)
      LPTX_particle_set_delete(s2);
    if (s3)
      LPTX_particle_set_delete(s3);
    if (s4)
      LPTX_particle_set_delete(s4);

    set = NULL;
    s1 = NULL;
    s2 = NULL;
    s3 = NULL;
    s4 = NULL;

    if (!test_compare_pp((set = LPTX_particle_set_new(10, 2,
                                                      (LPTX_idtype[]){2, 3})),
                         !=, NULL))
      r = 1;

    if (!test_compare_pp((s1 = LPTX_particle_set_new(5, 0, NULL)), !=, NULL))
      r = 1;

    if (!test_compare_pp((s2 = LPTX_particle_set_new(4, 3,
                                                     (LPTX_idtype[]){1, 2, 3})),
                         !=, NULL))
      r = 1;

    if (!test_compare_pp((s3 =
                            LPTX_particle_set_new(2, 2, (LPTX_idtype[]){3, 2})),
                         !=, NULL))
      r = 1;
  } while (0);

  if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
    r = 1;

  do {
    int ier = MPI_SUCCESS;
    if (r)
      break;

    if (irank >= 2)
      break;

    if (nproc <= 1)
      break;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_is_communicatable(set, 1, 0,
                                                               MPI_COMM_WORLD,
                                                               &ier),
                           ==, LPTX_true))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_is_communicatable(set, 0, 0,
                                                               MPI_COMM_WORLD,
                                                               &ier),
                           ==, LPTX_true))
        r = 1;
    }
    if (!test_compare_ii(ier, ==, MPI_SUCCESS))
      r = 1;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_is_communicatable(s1, 1, 0,
                                                               MPI_COMM_WORLD,
                                                               &ier),
                           ==, LPTX_true))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_is_communicatable(s1, 0, 0,
                                                               MPI_COMM_WORLD,
                                                               &ier),
                           ==, LPTX_true))
        r = 1;
    }
    if (!test_compare_ii(ier, ==, MPI_SUCCESS))
      r = 1;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_is_communicatable(set, 1, 0,
                                                               MPI_COMM_WORLD,
                                                               &ier),
                           ==, LPTX_false))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_is_communicatable(s1, 0, 0,
                                                               MPI_COMM_WORLD,
                                                               &ier),
                           ==, LPTX_false))
        r = 1;
    }
    if (!test_compare_ii(ier, ==, MPI_SUCCESS))
      r = 1;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_is_communicatable(set, 1, 0,
                                                               MPI_COMM_WORLD,
                                                               &ier),
                           ==, LPTX_false))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_is_communicatable(s2, 0, 0,
                                                               MPI_COMM_WORLD,
                                                               &ier),
                           ==, LPTX_false))
        r = 1;
    }
    if (!test_compare_ii(ier, ==, MPI_SUCCESS))
      r = 1;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_is_communicatable(s3, 1, 0,
                                                               MPI_COMM_WORLD,
                                                               &ier),
                           ==, LPTX_false))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_is_communicatable(set, 0, 0,
                                                               MPI_COMM_WORLD,
                                                               &ier),
                           ==, LPTX_false))
        r = 1;
    }
    if (!test_compare_ii(ier, ==, MPI_SUCCESS))
      r = 1;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_is_communicatable(s3, 1, 0,
                                                               MPI_COMM_WORLD,
                                                               &ier),
                           ==, LPTX_false))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_is_communicatable(NULL, 0, 0,
                                                               MPI_COMM_WORLD,
                                                               &ier),
                           ==, LPTX_false))
        r = 1;
    }
    if (!test_compare_ii(ier, ==, MPI_SUCCESS))
      r = 1;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_is_communicatable(NULL, 1, 0,
                                                               MPI_COMM_WORLD,
                                                               &ier),
                           ==, LPTX_false))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_is_communicatable(NULL, 0, 0,
                                                               MPI_COMM_WORLD,
                                                               &ier),
                           ==, LPTX_false))
        r = 1;
    }
    if (!test_compare_ii(ier, ==, MPI_SUCCESS))
      r = 1;
  } while (0);

  if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
    r = 1;

  do {
    int ier = MPI_SUCCESS;

    if (r)
      break;

    if (!test_compare_ii(LPTX_particle_set_is_collectable(set, MPI_COMM_WORLD,
                                                          &ier),
                         ==, LPTX_true))
      r = 1;
    if (!test_compare_ii(ier, ==, MPI_SUCCESS))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_is_collectable(s1, MPI_COMM_WORLD,
                                                          &ier),
                         ==, LPTX_true))
      r = 1;
    if (!test_compare_ii(ier, ==, MPI_SUCCESS))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_is_collectable(s2, MPI_COMM_WORLD,
                                                          &ier),
                         ==, LPTX_true))
      r = 1;
    if (!test_compare_ii(ier, ==, MPI_SUCCESS))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_is_collectable(NULL, MPI_COMM_WORLD,
                                                          &ier),
                         ==, LPTX_false))
      r = 1;
    if (!test_compare_ii(ier, ==, MPI_SUCCESS))
      r = 1;

    if (nproc < 2)
      break;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_is_collectable(set, MPI_COMM_WORLD,
                                                            &ier),
                           ==, LPTX_false))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_is_collectable(s1, MPI_COMM_WORLD,
                                                            &ier),
                           ==, LPTX_false))
        r = 1;
    }
    if (!test_compare_ii(ier, ==, MPI_SUCCESS))
      r = 1;

    if (irank == 1) {
      if (!test_compare_ii(LPTX_particle_set_is_collectable(s3, MPI_COMM_WORLD,
                                                            &ier),
                           ==, LPTX_false))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_is_collectable(set, MPI_COMM_WORLD,
                                                            &ier),
                           ==, LPTX_false))
        r = 1;
    }
    if (!test_compare_ii(ier, ==, MPI_SUCCESS))
      r = 1;

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_is_collectable(set, MPI_COMM_WORLD,
                                                            &ier),
                           ==, LPTX_false))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_is_collectable(s2, MPI_COMM_WORLD,
                                                            &ier),
                           ==, LPTX_false))
        r = 1;
    }
    if (!test_compare_ii(ier, ==, MPI_SUCCESS))
      r = 1;

    if (irank == 1) {
      if (!test_compare_ii(LPTX_particle_set_is_collectable(set, MPI_COMM_WORLD,
                                                            &ier),
                           ==, LPTX_false))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_is_collectable(s1, MPI_COMM_WORLD,
                                                            &ier),
                           ==, LPTX_false))
        r = 1;
    }
    if (!test_compare_ii(ier, ==, MPI_SUCCESS))
      r = 1;
  } while (0);

  if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
    r = 1;

  do {
    if (r)
      break;

    if (nproc < 2)
      break;

    if (irank >= 2)
      break;

    if (irank == 0) {
      set->particles[0].base.particle_id = 11;
      set->particles[1].base.particle_id = 12;
      set->particles[2].base.particle_id = 13;
      set->particles[0].base.position = LPTX_vector_c(11.0, 14.0, 17.0);
      set->particles[1].base.position = LPTX_vector_c(12.0, 15.0, 18.0);
      set->particles[2].base.position = LPTX_vector_c(13.0, 16.0, 19.0);
      *(LPTX_type *)&set->particles[0].vectors[0].v[0] = 44.0;
      *(LPTX_type *)&set->particles[0].vectors[0].v[1] = 45.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[0] = 46.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[1] = 47.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[2] = 48.0;
      *(LPTX_type *)&set->particles[1].vectors[0].v[0] = 49.0;
      *(LPTX_type *)&set->particles[1].vectors[0].v[1] = 50.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[0] = 51.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[1] = 52.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[2] = 53.0;
      *(LPTX_type *)&set->particles[2].vectors[0].v[0] = 54.0;
      *(LPTX_type *)&set->particles[2].vectors[0].v[1] = 55.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[0] = 56.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[1] = 57.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[2] = 58.0;

      if (!test_compare_ii(LPTX_particle_send(set, 0, 3, 1, 0, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(set->particles[1].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[2].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[3].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[4].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[5].base.particle_id, ==, -1))
        r = 1;

      *(LPTX_idtype *)&set->particles[3].vectors[0].length = 99;
      if (!test_compare_ii(set->particles[3].vectors[0].length, ==, 99))
        r = 1;

      if (!test_compare_ii(LPTX_particle_recv(set, 2, 3, 0, 0, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;

      if (!test_compare_ii(set->particles[1].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[2].base.particle_id, ==, 11))
        r = 1;
      if (!test_compare_ii(set->particles[3].base.particle_id, ==, 12))
        r = 1;
      if (!test_compare_ii(set->particles[4].base.particle_id, ==, 13))
        r = 1;
      if (!test_compare_ii(set->particles[5].base.particle_id, ==, -1))
        r = 1;

      if (!test_compare_dd(LPTX_vector_x(set->particles[1].base.position), ==,
                           0.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(set->particles[2].base.position), ==,
                           11.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(set->particles[3].base.position), ==,
                           12.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(set->particles[4].base.position), ==,
                           13.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(set->particles[5].base.position), ==,
                           0.0))
        r = 1;

      if (!test_compare_dd(LPTX_vector_y(set->particles[1].base.position), ==,
                           0.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(set->particles[2].base.position), ==,
                           14.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(set->particles[3].base.position), ==,
                           15.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(set->particles[4].base.position), ==,
                           16.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(set->particles[5].base.position), ==,
                           0.0))
        r = 1;

      if (!test_compare_dd(LPTX_vector_z(set->particles[1].base.position), ==,
                           0.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(set->particles[2].base.position), ==,
                           17.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(set->particles[3].base.position), ==,
                           18.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(set->particles[4].base.position), ==,
                           19.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(set->particles[5].base.position), ==,
                           0.0))
        r = 1;

      if (!test_compare_ii(set->particles[1].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(set->particles[1].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(set->particles[1].vectors[0].v[0], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[1].vectors[0].v[1], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[1].vectors[1].v[0], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[1].vectors[1].v[1], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[1].vectors[1].v[2], ==, 0.0))
        r = 1;

      if (!test_compare_ii(set->particles[2].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(set->particles[2].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(set->particles[2].vectors[0].v[0], ==, 44.0))
        r = 1;
      if (!test_compare_dd(set->particles[2].vectors[0].v[1], ==, 45.0))
        r = 1;
      if (!test_compare_dd(set->particles[2].vectors[1].v[0], ==, 46.0))
        r = 1;
      if (!test_compare_dd(set->particles[2].vectors[1].v[1], ==, 47.0))
        r = 1;
      if (!test_compare_dd(set->particles[2].vectors[1].v[2], ==, 48.0))
        r = 1;

      if (!test_compare_ii(set->particles[3].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(set->particles[3].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(set->particles[3].vectors[0].v[0], ==, 49.0))
        r = 1;
      if (!test_compare_dd(set->particles[3].vectors[0].v[1], ==, 50.0))
        r = 1;
      if (!test_compare_dd(set->particles[3].vectors[1].v[0], ==, 51.0))
        r = 1;
      if (!test_compare_dd(set->particles[3].vectors[1].v[1], ==, 52.0))
        r = 1;
      if (!test_compare_dd(set->particles[3].vectors[1].v[2], ==, 53.0))
        r = 1;

      if (!test_compare_ii(set->particles[4].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(set->particles[4].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(set->particles[4].vectors[0].v[0], ==, 54.0))
        r = 1;
      if (!test_compare_dd(set->particles[4].vectors[0].v[1], ==, 55.0))
        r = 1;
      if (!test_compare_dd(set->particles[4].vectors[1].v[0], ==, 56.0))
        r = 1;
      if (!test_compare_dd(set->particles[4].vectors[1].v[1], ==, 57.0))
        r = 1;
      if (!test_compare_dd(set->particles[4].vectors[1].v[2], ==, 58.0))
        r = 1;

      if (!test_compare_ii(set->particles[5].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(set->particles[5].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(set->particles[5].vectors[0].v[0], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[5].vectors[0].v[1], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[5].vectors[1].v[0], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[5].vectors[1].v[1], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[5].vectors[1].v[2], ==, 0.0))
        r = 1;
    }

    if (irank == 1) {
      s1->particles[0].base.particle_id = 14;
      s1->particles[1].base.particle_id = 15;
      s1->particles[0].base.velocity = LPTX_vector_c(20.0, 22.0, 24.0);
      s1->particles[1].base.velocity = LPTX_vector_c(21.0, 23.0, 25.0);

      if (!test_compare_pp(s1->particles[0].vectors, ==, NULL))
        r = 1;
      if (!test_compare_pp(s1->particles[1].vectors, ==, NULL))
        r = 1;

      if (!test_compare_ii(LPTX_particle_send(s1, 0, 2, 0, 1, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_recv(s1, 0, 2, 1, 1, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;

      if (!test_compare_ii(s1->particles[0].base.particle_id, ==, 14))
        r = 1;
      if (!test_compare_ii(s1->particles[1].base.particle_id, ==, 15))
        r = 1;
      if (!test_compare_ii(s1->particles[2].base.particle_id, ==, -1))
        r = 1;

      if (!test_compare_dd(LPTX_vector_x(s1->particles[0].base.velocity), ==,
                           20.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(s1->particles[1].base.velocity), ==,
                           21.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(s1->particles[2].base.velocity), ==,
                           0.0))
        r = 1;

      if (!test_compare_dd(LPTX_vector_y(s1->particles[0].base.velocity), ==,
                           22.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(s1->particles[1].base.velocity), ==,
                           23.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(s1->particles[2].base.velocity), ==,
                           0.0))
        r = 1;

      if (!test_compare_dd(LPTX_vector_z(s1->particles[0].base.velocity), ==,
                           24.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(s1->particles[1].base.velocity), ==,
                           25.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(s1->particles[2].base.velocity), ==,
                           0.0))
        r = 1;

      if (!test_compare_pp(s1->particles[0].vectors, ==, NULL))
        r = 1;
      if (!test_compare_pp(s1->particles[1].vectors, ==, NULL))
        r = 1;
      if (!test_compare_pp(s1->particles[2].vectors, ==, NULL))
        r = 1;
    }

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_send(set, 0, 3, 1, 0, MPI_COMM_WORLD),
                           ==, LPTX_MPI_ERR_VECTOR_SIZE_MISMATCH))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_recv(s1, 0, 3, 0, 0, MPI_COMM_WORLD),
                           ==, LPTX_MPI_ERR_VECTOR_SIZE_MISMATCH))
        r = 1;
    }

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_send(set, 0, 3, 1, 0, MPI_COMM_WORLD),
                           ==, LPTX_MPI_ERR_VECTOR_SIZE_MISMATCH))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_recvv(s1, 3, (LPTX_idtype[]){0, 1, 2},
                                               0, 0, MPI_COMM_WORLD),
                           ==, LPTX_MPI_ERR_VECTOR_SIZE_MISMATCH))
        r = 1;
    }

    if (irank == 1) {
      if (!test_compare_ii(LPTX_particle_sendv(set, 3, (LPTX_idtype[]){0, 1, 2},
                                               0, 0, MPI_COMM_WORLD),
                           ==, LPTX_MPI_ERR_VECTOR_SIZE_MISMATCH))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_recv(s1, 0, 3, 1, 0, MPI_COMM_WORLD),
                           ==, LPTX_MPI_ERR_VECTOR_SIZE_MISMATCH))
        r = 1;
    }
  } while (0);

  if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
    r = 1;

  do {
    if (r)
      break;

    if (nproc < 2)
      break;

    if (irank >= 2)
      break;

    if (irank == 0) {
      set->particles[5].base.particle_id = 14;
      set->particles[6].base.particle_id = 15;
      set->particles[8].base.particle_id = 16;
      set->particles[5].base.position = LPTX_vector_c(21.0, 24.0, 27.0);
      set->particles[6].base.position = LPTX_vector_c(22.0, 25.0, 28.0);
      set->particles[8].base.position = LPTX_vector_c(23.0, 26.0, 29.0);
      *(LPTX_type *)&set->particles[5].vectors[0].v[0] = 54.0;
      *(LPTX_type *)&set->particles[5].vectors[0].v[1] = 55.0;
      *(LPTX_type *)&set->particles[5].vectors[1].v[0] = 56.0;
      *(LPTX_type *)&set->particles[5].vectors[1].v[1] = 57.0;
      *(LPTX_type *)&set->particles[5].vectors[1].v[2] = 58.0;
      *(LPTX_type *)&set->particles[6].vectors[0].v[0] = 59.0;
      *(LPTX_type *)&set->particles[6].vectors[0].v[1] = 60.0;
      *(LPTX_type *)&set->particles[6].vectors[1].v[0] = 61.0;
      *(LPTX_type *)&set->particles[6].vectors[1].v[1] = 62.0;
      *(LPTX_type *)&set->particles[6].vectors[1].v[2] = 63.0;
      *(LPTX_type *)&set->particles[8].vectors[0].v[0] = 64.0;
      *(LPTX_type *)&set->particles[8].vectors[0].v[1] = 65.0;
      *(LPTX_type *)&set->particles[8].vectors[1].v[0] = 66.0;
      *(LPTX_type *)&set->particles[8].vectors[1].v[1] = 67.0;
      *(LPTX_type *)&set->particles[8].vectors[1].v[2] = 68.0;

      if (!test_compare_ii(LPTX_particle_sendv(set, 3, (LPTX_idtype[]){5, 8, 6},
                                               1, 0, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      for (int i = 0; i < 9; ++i) {
        set->particles[i].base.particle_id = -1;
        set->particles[i].base.position = LPTX_vector_c(0.0, 0.0, 0.0);
        *(LPTX_type *)&set->particles[i].vectors[0].v[0] = 0.0;
        *(LPTX_type *)&set->particles[i].vectors[0].v[1] = 0.0;
        *(LPTX_type *)&set->particles[i].vectors[1].v[0] = 0.0;
        *(LPTX_type *)&set->particles[i].vectors[1].v[1] = 0.0;
        *(LPTX_type *)&set->particles[i].vectors[1].v[2] = 0.0;
      }

      if (!test_compare_ii(set->particles[4].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[5].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[6].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[7].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[8].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[9].base.particle_id, ==, -1))
        r = 1;

      *(LPTX_idtype *)&set->particles[5].vectors[0].length = 99;
      if (!test_compare_ii(set->particles[5].vectors[0].length, ==, 99))
        r = 1;

      if (!test_compare_ii(LPTX_particle_recvv(set, 3, (LPTX_idtype[]){8, 6, 5},
                                               0, 0, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;

      if (!test_compare_ii(set->particles[4].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[5].base.particle_id, ==, 15))
        r = 1;
      if (!test_compare_ii(set->particles[6].base.particle_id, ==, 16))
        r = 1;
      if (!test_compare_ii(set->particles[7].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[8].base.particle_id, ==, 14))
        r = 1;
      if (!test_compare_ii(set->particles[9].base.particle_id, ==, -1))
        r = 1;

      if (!test_compare_dd(LPTX_vector_x(set->particles[4].base.position), ==,
                           0.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(set->particles[5].base.position), ==,
                           22.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(set->particles[6].base.position), ==,
                           23.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(set->particles[7].base.position), ==,
                           0.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(set->particles[8].base.position), ==,
                           21.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(set->particles[9].base.position), ==,
                           0.0))
        r = 1;

      if (!test_compare_dd(LPTX_vector_y(set->particles[4].base.position), ==,
                           0.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(set->particles[5].base.position), ==,
                           25.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(set->particles[6].base.position), ==,
                           26.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(set->particles[7].base.position), ==,
                           0.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(set->particles[8].base.position), ==,
                           24.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(set->particles[9].base.position), ==,
                           0.0))
        r = 1;

      if (!test_compare_dd(LPTX_vector_z(set->particles[4].base.position), ==,
                           0.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(set->particles[5].base.position), ==,
                           28.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(set->particles[6].base.position), ==,
                           29.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(set->particles[7].base.position), ==,
                           0.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(set->particles[8].base.position), ==,
                           27.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(set->particles[9].base.position), ==,
                           0.0))
        r = 1;

      if (!test_compare_ii(set->particles[4].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(set->particles[4].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(set->particles[4].vectors[0].v[0], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[4].vectors[0].v[1], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[4].vectors[1].v[0], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[4].vectors[1].v[1], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[4].vectors[1].v[2], ==, 0.0))
        r = 1;

      if (!test_compare_ii(set->particles[5].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(set->particles[5].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(set->particles[5].vectors[0].v[0], ==, 59.0))
        r = 1;
      if (!test_compare_dd(set->particles[5].vectors[0].v[1], ==, 60.0))
        r = 1;
      if (!test_compare_dd(set->particles[5].vectors[1].v[0], ==, 61.0))
        r = 1;
      if (!test_compare_dd(set->particles[5].vectors[1].v[1], ==, 62.0))
        r = 1;
      if (!test_compare_dd(set->particles[5].vectors[1].v[2], ==, 63.0))
        r = 1;

      if (!test_compare_ii(set->particles[6].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(set->particles[6].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(set->particles[6].vectors[0].v[0], ==, 64.0))
        r = 1;
      if (!test_compare_dd(set->particles[6].vectors[0].v[1], ==, 65.0))
        r = 1;
      if (!test_compare_dd(set->particles[6].vectors[1].v[0], ==, 66.0))
        r = 1;
      if (!test_compare_dd(set->particles[6].vectors[1].v[1], ==, 67.0))
        r = 1;
      if (!test_compare_dd(set->particles[6].vectors[1].v[2], ==, 68.0))
        r = 1;

      if (!test_compare_ii(set->particles[7].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(set->particles[7].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(set->particles[7].vectors[0].v[0], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[7].vectors[0].v[1], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[7].vectors[1].v[0], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[7].vectors[1].v[1], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[7].vectors[1].v[2], ==, 0.0))
        r = 1;

      if (!test_compare_ii(set->particles[8].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(set->particles[8].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(set->particles[8].vectors[0].v[0], ==, 54.0))
        r = 1;
      if (!test_compare_dd(set->particles[8].vectors[0].v[1], ==, 55.0))
        r = 1;
      if (!test_compare_dd(set->particles[8].vectors[1].v[0], ==, 56.0))
        r = 1;
      if (!test_compare_dd(set->particles[8].vectors[1].v[1], ==, 57.0))
        r = 1;
      if (!test_compare_dd(set->particles[8].vectors[1].v[2], ==, 58.0))
        r = 1;

      if (!test_compare_ii(set->particles[9].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(set->particles[9].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(set->particles[9].vectors[0].v[0], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[9].vectors[0].v[1], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[9].vectors[1].v[0], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[9].vectors[1].v[1], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[9].vectors[1].v[2], ==, 0.0))
        r = 1;
    }

    /* send with sendv and receive with recv */
    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_sendv(set, 3, (LPTX_idtype[]){5, 8, 6},
                                               1, 0, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      for (int i = 0; i < 9; ++i) {
        set->particles[i].base.particle_id = -1;
        set->particles[i].base.position = LPTX_vector_c(0.0, 0.0, 0.0);
        *(LPTX_type *)&set->particles[i].vectors[0].v[0] = 0.0;
        *(LPTX_type *)&set->particles[i].vectors[0].v[1] = 0.0;
        *(LPTX_type *)&set->particles[i].vectors[1].v[0] = 0.0;
        *(LPTX_type *)&set->particles[i].vectors[1].v[1] = 0.0;
        *(LPTX_type *)&set->particles[i].vectors[1].v[2] = 0.0;
      }

      if (!test_compare_ii(set->particles[4].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[5].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[6].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[7].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[8].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[9].base.particle_id, ==, -1))
        r = 1;

      *(LPTX_idtype *)&set->particles[5].vectors[0].length = 99;
      if (!test_compare_ii(set->particles[5].vectors[0].length, ==, 99))
        r = 1;

      if (!test_compare_ii(LPTX_particle_recv(set, 5, 3, 0, 0, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;

      if (!test_compare_ii(set->particles[4].base.particle_id, ==, -1))
        r = 1;
      if (!test_compare_ii(set->particles[5].base.particle_id, ==, 14))
        r = 1;
      if (!test_compare_ii(set->particles[6].base.particle_id, ==, 16))
        r = 1;
      if (!test_compare_ii(set->particles[7].base.particle_id, ==, 15))
        r = 1;
      if (!test_compare_ii(set->particles[8].base.particle_id, ==, -1))
        r = 1;

      if (!test_compare_dd(LPTX_vector_x(set->particles[4].base.position), ==,
                           0.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(set->particles[5].base.position), ==,
                           21.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(set->particles[6].base.position), ==,
                           23.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(set->particles[7].base.position), ==,
                           22.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(set->particles[8].base.position), ==,
                           0.0))
        r = 1;

      if (!test_compare_dd(LPTX_vector_y(set->particles[4].base.position), ==,
                           0.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(set->particles[5].base.position), ==,
                           24.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(set->particles[6].base.position), ==,
                           26.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(set->particles[7].base.position), ==,
                           25.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(set->particles[8].base.position), ==,
                           0.0))
        r = 1;

      if (!test_compare_dd(LPTX_vector_z(set->particles[4].base.position), ==,
                           0.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(set->particles[5].base.position), ==,
                           27.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(set->particles[6].base.position), ==,
                           29.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(set->particles[7].base.position), ==,
                           28.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(set->particles[8].base.position), ==,
                           0.0))
        r = 1;

      if (!test_compare_ii(set->particles[4].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(set->particles[4].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(set->particles[4].vectors[0].v[0], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[4].vectors[0].v[1], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[4].vectors[1].v[0], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[4].vectors[1].v[1], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[4].vectors[1].v[2], ==, 0.0))
        r = 1;

      if (!test_compare_ii(set->particles[5].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(set->particles[5].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(set->particles[5].vectors[0].v[0], ==, 54.0))
        r = 1;
      if (!test_compare_dd(set->particles[5].vectors[0].v[1], ==, 55.0))
        r = 1;
      if (!test_compare_dd(set->particles[5].vectors[1].v[0], ==, 56.0))
        r = 1;
      if (!test_compare_dd(set->particles[5].vectors[1].v[1], ==, 57.0))
        r = 1;
      if (!test_compare_dd(set->particles[5].vectors[1].v[2], ==, 58.0))
        r = 1;

      if (!test_compare_ii(set->particles[6].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(set->particles[6].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(set->particles[6].vectors[0].v[0], ==, 64.0))
        r = 1;
      if (!test_compare_dd(set->particles[6].vectors[0].v[1], ==, 65.0))
        r = 1;
      if (!test_compare_dd(set->particles[6].vectors[1].v[0], ==, 66.0))
        r = 1;
      if (!test_compare_dd(set->particles[6].vectors[1].v[1], ==, 67.0))
        r = 1;
      if (!test_compare_dd(set->particles[6].vectors[1].v[2], ==, 68.0))
        r = 1;

      if (!test_compare_ii(set->particles[7].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(set->particles[7].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(set->particles[7].vectors[0].v[0], ==, 59.0))
        r = 1;
      if (!test_compare_dd(set->particles[7].vectors[0].v[1], ==, 60.0))
        r = 1;
      if (!test_compare_dd(set->particles[7].vectors[1].v[0], ==, 61.0))
        r = 1;
      if (!test_compare_dd(set->particles[7].vectors[1].v[1], ==, 62.0))
        r = 1;
      if (!test_compare_dd(set->particles[7].vectors[1].v[2], ==, 63.0))
        r = 1;

      if (!test_compare_ii(set->particles[8].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(set->particles[8].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(set->particles[8].vectors[0].v[0], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[8].vectors[0].v[1], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[8].vectors[1].v[0], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[8].vectors[1].v[1], ==, 0.0))
        r = 1;
      if (!test_compare_dd(set->particles[8].vectors[1].v[2], ==, 0.0))
        r = 1;
    }

    /** @todo Add send with send and receive with recvv */

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_sendv(set, 1, (LPTX_idtype[]){0}, 1, 0,
                                               MPI_COMM_WORLD),
                           ==, LPTX_MPI_ERR_VECTOR_SIZE_MISMATCH))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_recvv(s1, 1, (LPTX_idtype[]){0}, 0, 0,
                                               MPI_COMM_WORLD),
                           ==, LPTX_MPI_ERR_VECTOR_SIZE_MISMATCH))
        r = 1;
    }
  } while (0);

  if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
    r = 1;

  do {
    if (r)
      break;

    if (nproc < 2)
      break;

    if (irank >= 2)
      break;

    if (irank == 0) {
      s2->particles[0].base.particle_id = 200;
      s2->particles[1].base.particle_id = 201;
      s2->particles[2].base.particle_id = 202;
      s2->particles[3].base.particle_id = 203;
      *(LPTX_type *)&s2->particles[0].vectors[0].v[0] = 100.0;
      *(LPTX_type *)&s2->particles[0].vectors[1].v[0] = 101.0;
      *(LPTX_type *)&s2->particles[0].vectors[1].v[1] = 102.0;
      *(LPTX_type *)&s2->particles[0].vectors[2].v[0] = 103.0;
      *(LPTX_type *)&s2->particles[0].vectors[2].v[1] = 104.0;
      *(LPTX_type *)&s2->particles[0].vectors[2].v[2] = 105.0;
      *(LPTX_type *)&s2->particles[1].vectors[0].v[0] = 110.0;
      *(LPTX_type *)&s2->particles[1].vectors[1].v[0] = 111.0;
      *(LPTX_type *)&s2->particles[1].vectors[1].v[1] = 112.0;
      *(LPTX_type *)&s2->particles[1].vectors[2].v[0] = 113.0;
      *(LPTX_type *)&s2->particles[1].vectors[2].v[1] = 114.0;
      *(LPTX_type *)&s2->particles[1].vectors[2].v[2] = 115.0;
      *(LPTX_type *)&s2->particles[2].vectors[0].v[0] = 120.0;
      *(LPTX_type *)&s2->particles[2].vectors[1].v[0] = 121.0;
      *(LPTX_type *)&s2->particles[2].vectors[1].v[1] = 122.0;
      *(LPTX_type *)&s2->particles[2].vectors[2].v[0] = 123.0;
      *(LPTX_type *)&s2->particles[2].vectors[2].v[1] = 124.0;
      *(LPTX_type *)&s2->particles[2].vectors[2].v[2] = 125.0;
      *(LPTX_type *)&s2->particles[3].vectors[0].v[0] = 130.0;
      *(LPTX_type *)&s2->particles[3].vectors[1].v[0] = 131.0;
      *(LPTX_type *)&s2->particles[3].vectors[1].v[1] = 132.0;
      *(LPTX_type *)&s2->particles[3].vectors[2].v[0] = 133.0;
      *(LPTX_type *)&s2->particles[3].vectors[2].v[1] = 134.0;
      *(LPTX_type *)&s2->particles[3].vectors[2].v[2] = 135.0;

      if (!test_compare_ii(LPTX_particle_set_send(s2, 1, 0, MPI_COMM_WORLD), ==,
                           MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_recv(&s4, 0, 0, MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;

      if (!test_compare_pp(s4, !=, NULL)) {
        r = 1;
        break;
      }

      if (!test_compare_ii(s4->number_particles, ==, 4))
        r = 1;
      if (!test_compare_ii(s4->number_data, ==, 6))
        r = 1;
      if (!test_compare_ii(s4->number_vectors, ==, 3))
        r = 1;
      if (r)
        break;

      if (!test_compare_ii(s4->particles[0].base.particle_id, ==, 200))
        r = 1;
      if (!test_compare_ii(s4->particles[1].base.particle_id, ==, 201))
        r = 1;
      if (!test_compare_ii(s4->particles[2].base.particle_id, ==, 202))
        r = 1;
      if (!test_compare_ii(s4->particles[3].base.particle_id, ==, 203))
        r = 1;

      if (!test_compare_ii(s4->particles[0].vectors[0].length, ==, 1))
        r = 1;
      if (!test_compare_ii(s4->particles[0].vectors[1].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s4->particles[0].vectors[2].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s4->particles[1].vectors[0].length, ==, 1))
        r = 1;
      if (!test_compare_ii(s4->particles[1].vectors[1].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s4->particles[1].vectors[2].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s4->particles[2].vectors[0].length, ==, 1))
        r = 1;
      if (!test_compare_ii(s4->particles[2].vectors[1].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s4->particles[2].vectors[2].length, ==, 3))
        r = 1;

      if (!test_compare_dd(s4->particles[0].vectors[0].v[0], ==, 100.0))
        r = 1;
      if (!test_compare_dd(s4->particles[0].vectors[1].v[0], ==, 101.0))
        r = 1;
      if (!test_compare_dd(s4->particles[0].vectors[1].v[1], ==, 102.0))
        r = 1;
      if (!test_compare_dd(s4->particles[0].vectors[2].v[0], ==, 103.0))
        r = 1;
      if (!test_compare_dd(s4->particles[0].vectors[2].v[1], ==, 104.0))
        r = 1;
      if (!test_compare_dd(s4->particles[0].vectors[2].v[2], ==, 105.0))
        r = 1;
      if (!test_compare_dd(s4->particles[1].vectors[0].v[0], ==, 110.0))
        r = 1;
      if (!test_compare_dd(s4->particles[1].vectors[1].v[0], ==, 111.0))
        r = 1;
      if (!test_compare_dd(s4->particles[1].vectors[1].v[1], ==, 112.0))
        r = 1;
      if (!test_compare_dd(s4->particles[1].vectors[2].v[0], ==, 113.0))
        r = 1;
      if (!test_compare_dd(s4->particles[1].vectors[2].v[1], ==, 114.0))
        r = 1;
      if (!test_compare_dd(s4->particles[1].vectors[2].v[2], ==, 115.0))
        r = 1;
      if (!test_compare_dd(s4->particles[2].vectors[0].v[0], ==, 120.0))
        r = 1;
      if (!test_compare_dd(s4->particles[2].vectors[1].v[0], ==, 121.0))
        r = 1;
      if (!test_compare_dd(s4->particles[2].vectors[1].v[1], ==, 122.0))
        r = 1;
      if (!test_compare_dd(s4->particles[2].vectors[2].v[0], ==, 123.0))
        r = 1;
      if (!test_compare_dd(s4->particles[2].vectors[2].v[1], ==, 124.0))
        r = 1;
      if (!test_compare_dd(s4->particles[2].vectors[2].v[2], ==, 125.0))
        r = 1;
      if (!test_compare_dd(s4->particles[3].vectors[0].v[0], ==, 130.0))
        r = 1;
      if (!test_compare_dd(s4->particles[3].vectors[1].v[0], ==, 131.0))
        r = 1;
      if (!test_compare_dd(s4->particles[3].vectors[1].v[1], ==, 132.0))
        r = 1;
      if (!test_compare_dd(s4->particles[3].vectors[2].v[0], ==, 133.0))
        r = 1;
      if (!test_compare_dd(s4->particles[3].vectors[2].v[1], ==, 134.0))
        r = 1;
      if (!test_compare_dd(s4->particles[3].vectors[2].v[2], ==, 135.0))
        r = 1;
    }
  } while (0);

  if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL))
    r = 1;

  do {
    LPTX_idtype npt[] = {3, 4, 0, 1};
    LPTX_idtype np;

    if (r)
      break;

    if (s4)
      LPTX_particle_set_delete(s4);

    s4 = NULL;
    np = npt[irank % 4] + irank / 4;
    // 3,  4,  0,  1,  4,  5,  1,  2,  5,  6,  2,  3,  6, ...
    // 3,  7,  7,  8, 12, 17, 18, 20, 25, 31, 33, 36, 42, ...
    // n = nproc / 4
    // S = n * (2 * 3 + (n - 1) + 2 * 4 + (n - 1) + (n - 1) + 2 * 1 + (n - 1))
    //     / 2
    // S = n * (6 + 8 + 2 + 4 * (n - 1)) / 2
    //   = n * (6 + 2 * n)
    // S0 = n * (6 + 2 * n)
    // S1 = n * (6 + 2 * n) + (n + 3)
    //    = n * (7 + 2 * n) + 3
    // S2 = n * (8 + 2 * n) + 7
    // S3 = n * (9 + 2 * n) + 7

    if (np > 0) {
      if (!test_compare_pp((s4 = LPTX_particle_set_new(np, 2,
                                                       (LPTX_idtype[]){2, 3})),
                           !=, NULL))
        r = 1;
    }

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
      r = 1;
      break;
    }

    switch (irank) {
    case 0:
      if (!test_compare_ii(LPTX_particle_set_number_of_particles(s4), ==, 3))
        r = 1;
      break;
    case 1:
      if (!test_compare_ii(LPTX_particle_set_number_of_particles(s4), ==, 4))
        r = 1;
      break;
    case 2:
      if (!test_compare_pp(s4, ==, NULL))
        r = 1;
      break;
    case 3:
      if (!test_compare_ii(LPTX_particle_set_number_of_particles(s4), ==, 1))
        r = 1;
      break;
    default:
      switch (irank % 4) {
      case 0:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s4), ==,
                             3 + irank / 4))
          r = 1;
        break;
      case 1:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s4), ==,
                             4 + irank / 4))
          r = 1;
        break;
      case 2:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s4), ==,
                             0 + irank / 4))
          r = 1;
        break;
      case 3:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s4), ==,
                             1 + irank / 4))
          r = 1;
        break;
      }
      break;
    }

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
      r = 1;
      break;
    }

    if (s4) {
      for (LPTX_idtype jj = 0; jj < np; ++jj) {
        s4->particles[jj].base.particle_id = (irank + 1) * 100 + jj;
        *(LPTX_type *)&s4->particles[jj].vectors[0].v[0] =
          (irank + 1) * 1000 + jj * 5 + 0;
        *(LPTX_type *)&s4->particles[jj].vectors[0].v[1] =
          (irank + 1) * 1000 + jj * 5 + 1;
        *(LPTX_type *)&s4->particles[jj].vectors[1].v[0] =
          (irank + 1) * 1000 + jj * 5 + 2;
        *(LPTX_type *)&s4->particles[jj].vectors[1].v[1] =
          (irank + 1) * 1000 + jj * 5 + 3;
        *(LPTX_type *)&s4->particles[jj].vectors[1].v[2] =
          (irank + 1) * 1000 + jj * 5 + 4;
      }
    }

    if (s5)
      LPTX_particle_set_delete(s5);
    s5 = NULL;

    if (!test_compare_ii(LPTX_particle_set_gather(&s5, s4, 0, MPI_COMM_WORLD),
                         ==, MPI_SUCCESS)) {
      r = 1;
      break;
    }

    do {
      if (irank != 0)
        break;

      if (!test_compare_pp(s5, !=, NULL)) {
        r = 1;
        break;
      }

      switch (nproc) {
        int n;
      case 1:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 3))
          r = 1;
        break;
      case 2:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 7))
          r = 1;
        break;
      case 3:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 7))
          r = 1;
        break;
      case 4:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 8))
          r = 1;
        break;
      default:
        n = nproc / 4;
        switch (nproc % 4) {
        case 1:
          if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==,
                               n * (7 + 2 * n) + 3))
            r = 1;
          break;
        case 2:
          if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==,
                               n * (8 + 2 * n) + 7))
            r = 1;
          break;
        case 3:
          if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==,
                               n * (9 + 2 * n) + 7))
            r = 1;
          break;
        case 0:
          if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==,
                               n * (6 + 2 * n)))
            r = 1;
          break;
        }
        break;
      }
    } while (0);

    do {
      LPTX_idtype n;
      if (r)
        break;
      if (irank != 0)
        break;
      if (!s5) {
        r = 1;
        break;
      }

      n = LPTX_particle_set_number_of_particles(s5);

      if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 100))
        r = 1;
      if (!test_compare_ii(s5->particles[1].base.particle_id, ==, 101))
        r = 1;
      if (!test_compare_ii(s5->particles[2].base.particle_id, ==, 102))
        r = 1;

      if (!test_compare_ii(s5->particles[0].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[0].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[1].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[1].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[2].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[2].vectors[1].length, ==, 3))
        r = 1;

      if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1000.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1001.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1002.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1003.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1004.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[0].v[0], ==, 1005.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[0].v[1], ==, 1006.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[1].v[0], ==, 1007.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[1].v[1], ==, 1008.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[1].v[2], ==, 1009.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[0].v[0], ==, 1010.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[0].v[1], ==, 1011.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[1].v[0], ==, 1012.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[1].v[1], ==, 1013.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[1].v[2], ==, 1014.0))
        r = 1;

      if (nproc < 2)
        break;

      if (!test_compare_ii(s5->particles[3].base.particle_id, ==, 200))
        r = 1;
      if (!test_compare_ii(s5->particles[4].base.particle_id, ==, 201))
        r = 1;
      if (!test_compare_ii(s5->particles[5].base.particle_id, ==, 202))
        r = 1;
      if (!test_compare_ii(s5->particles[6].base.particle_id, ==, 203))
        r = 1;

      if (!test_compare_ii(s5->particles[3].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[3].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[4].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[4].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[5].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[5].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[6].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[6].vectors[1].length, ==, 3))
        r = 1;

      if (!test_compare_dd(s5->particles[3].vectors[0].v[0], ==, 2000.0))
        r = 1;
      if (!test_compare_dd(s5->particles[3].vectors[0].v[1], ==, 2001.0))
        r = 1;
      if (!test_compare_dd(s5->particles[3].vectors[1].v[0], ==, 2002.0))
        r = 1;
      if (!test_compare_dd(s5->particles[3].vectors[1].v[1], ==, 2003.0))
        r = 1;
      if (!test_compare_dd(s5->particles[3].vectors[1].v[2], ==, 2004.0))
        r = 1;
      if (!test_compare_dd(s5->particles[4].vectors[0].v[0], ==, 2005.0))
        r = 1;
      if (!test_compare_dd(s5->particles[4].vectors[0].v[1], ==, 2006.0))
        r = 1;
      if (!test_compare_dd(s5->particles[4].vectors[1].v[0], ==, 2007.0))
        r = 1;
      if (!test_compare_dd(s5->particles[4].vectors[1].v[1], ==, 2008.0))
        r = 1;
      if (!test_compare_dd(s5->particles[4].vectors[1].v[2], ==, 2009.0))
        r = 1;
      if (!test_compare_dd(s5->particles[5].vectors[0].v[0], ==, 2010.0))
        r = 1;
      if (!test_compare_dd(s5->particles[5].vectors[0].v[1], ==, 2011.0))
        r = 1;
      if (!test_compare_dd(s5->particles[5].vectors[1].v[0], ==, 2012.0))
        r = 1;
      if (!test_compare_dd(s5->particles[5].vectors[1].v[1], ==, 2013.0))
        r = 1;
      if (!test_compare_dd(s5->particles[5].vectors[1].v[2], ==, 2014.0))
        r = 1;
      if (!test_compare_dd(s5->particles[6].vectors[0].v[0], ==, 2015.0))
        r = 1;
      if (!test_compare_dd(s5->particles[6].vectors[0].v[1], ==, 2016.0))
        r = 1;
      if (!test_compare_dd(s5->particles[6].vectors[1].v[0], ==, 2017.0))
        r = 1;
      if (!test_compare_dd(s5->particles[6].vectors[1].v[1], ==, 2018.0))
        r = 1;
      if (!test_compare_dd(s5->particles[6].vectors[1].v[2], ==, 2019.0))
        r = 1;

      if (nproc < 4)
        break;

      if (!test_compare_ii(s5->particles[7].base.particle_id, ==, 400))
        r = 1;

      if (!test_compare_ii(s5->particles[7].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[7].vectors[1].length, ==, 3))
        r = 1;

      if (nproc < 5)
        break;

      for (LPTX_idtype ir = 4, jj = 8; ir < nproc; ++ir) {
        LPTX_idtype lp = npt[ir % 4] + ir / 4;
        for (LPTX_idtype jk = 0; jk < lp; ++jk, ++jj) {
          if (!test_compare_ii(s5->particles[jj].base.particle_id, ==,
                               (ir + 1) * 100 + jk))
            r = 1;

          if (!test_compare_ii(s5->particles[jj].vectors[0].length, ==, 2))
            r = 1;
          if (!test_compare_ii(s5->particles[jj].vectors[1].length, ==, 3))
            r = 1;

          if (!test_compare_dd(s5->particles[jj].vectors[0].v[0], ==,
                               (ir + 1) * 1000 + jk * 5 + 0))
            r = 1;
          if (!test_compare_dd(s5->particles[jj].vectors[0].v[1], ==,
                               (ir + 1) * 1000 + jk * 5 + 1))
            r = 1;
          if (!test_compare_dd(s5->particles[jj].vectors[1].v[0], ==,
                               (ir + 1) * 1000 + jk * 5 + 2))
            r = 1;
          if (!test_compare_dd(s5->particles[jj].vectors[1].v[1], ==,
                               (ir + 1) * 1000 + jk * 5 + 3))
            r = 1;
          if (!test_compare_dd(s5->particles[jj].vectors[1].v[2], ==,
                               (ir + 1) * 1000 + jk * 5 + 4))
            r = 1;
        }
      }
    } while (0);

    if (s5)
      LPTX_particle_set_delete(s5);
    s5 = NULL;

    if (nproc < 3)
      break;

    if (s5)
      LPTX_particle_set_delete(s5);
    s5 = NULL;

    if (!test_compare_ii(LPTX_particle_set_gather(&s5, s4, 2, MPI_COMM_WORLD),
                         ==, MPI_SUCCESS)) {
      r = 1;
      break;
    }

    do {
      if (irank != 2)
        break;

      if (!test_compare_pp(s5, !=, NULL)) {
        r = 1;
        break;
      }

      switch (nproc) {
        int n;
      case 1:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 3))
          r = 1;
        break;
      case 2:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 7))
          r = 1;
        break;
      case 3:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 7))
          r = 1;
        break;
      case 4:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 8))
          r = 1;
        break;
      default:
        n = nproc / 4;
        switch (nproc % 4) {
        case 1:
          if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==,
                               n * (7 + 2 * n) + 3))
            r = 1;
          break;
        case 2:
          if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==,
                               n * (8 + 2 * n) + 7))
            r = 1;
          break;
        case 3:
          if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==,
                               n * (9 + 2 * n) + 7))
            r = 1;
          break;
        case 0:
          if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==,
                               n * (6 + 2 * n)))
            r = 1;
          break;
        }
        break;
      }
    } while (0);

    do {
      LPTX_idtype n;
      if (r)
        break;
      if (irank != 2)
        break;
      if (!s5) {
        r = 1;
        break;
      }

      n = LPTX_particle_set_number_of_particles(s5);

      if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 100))
        r = 1;
      if (!test_compare_ii(s5->particles[1].base.particle_id, ==, 101))
        r = 1;
      if (!test_compare_ii(s5->particles[2].base.particle_id, ==, 102))
        r = 1;

      if (!test_compare_ii(s5->particles[0].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[0].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[1].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[1].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[2].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[2].vectors[1].length, ==, 3))
        r = 1;

      if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1000.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1001.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1002.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1003.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1004.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[0].v[0], ==, 1005.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[0].v[1], ==, 1006.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[1].v[0], ==, 1007.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[1].v[1], ==, 1008.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[1].v[2], ==, 1009.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[0].v[0], ==, 1010.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[0].v[1], ==, 1011.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[1].v[0], ==, 1012.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[1].v[1], ==, 1013.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[1].v[2], ==, 1014.0))
        r = 1;

      if (nproc < 2)
        break;

      if (!test_compare_ii(s5->particles[3].base.particle_id, ==, 200))
        r = 1;
      if (!test_compare_ii(s5->particles[4].base.particle_id, ==, 201))
        r = 1;
      if (!test_compare_ii(s5->particles[5].base.particle_id, ==, 202))
        r = 1;
      if (!test_compare_ii(s5->particles[6].base.particle_id, ==, 203))
        r = 1;

      if (!test_compare_ii(s5->particles[3].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[3].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[4].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[4].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[5].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[5].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[6].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[6].vectors[1].length, ==, 3))
        r = 1;

      if (!test_compare_dd(s5->particles[3].vectors[0].v[0], ==, 2000.0))
        r = 1;
      if (!test_compare_dd(s5->particles[3].vectors[0].v[1], ==, 2001.0))
        r = 1;
      if (!test_compare_dd(s5->particles[3].vectors[1].v[0], ==, 2002.0))
        r = 1;
      if (!test_compare_dd(s5->particles[3].vectors[1].v[1], ==, 2003.0))
        r = 1;
      if (!test_compare_dd(s5->particles[3].vectors[1].v[2], ==, 2004.0))
        r = 1;
      if (!test_compare_dd(s5->particles[4].vectors[0].v[0], ==, 2005.0))
        r = 1;
      if (!test_compare_dd(s5->particles[4].vectors[0].v[1], ==, 2006.0))
        r = 1;
      if (!test_compare_dd(s5->particles[4].vectors[1].v[0], ==, 2007.0))
        r = 1;
      if (!test_compare_dd(s5->particles[4].vectors[1].v[1], ==, 2008.0))
        r = 1;
      if (!test_compare_dd(s5->particles[4].vectors[1].v[2], ==, 2009.0))
        r = 1;
      if (!test_compare_dd(s5->particles[5].vectors[0].v[0], ==, 2010.0))
        r = 1;
      if (!test_compare_dd(s5->particles[5].vectors[0].v[1], ==, 2011.0))
        r = 1;
      if (!test_compare_dd(s5->particles[5].vectors[1].v[0], ==, 2012.0))
        r = 1;
      if (!test_compare_dd(s5->particles[5].vectors[1].v[1], ==, 2013.0))
        r = 1;
      if (!test_compare_dd(s5->particles[5].vectors[1].v[2], ==, 2014.0))
        r = 1;
      if (!test_compare_dd(s5->particles[6].vectors[0].v[0], ==, 2015.0))
        r = 1;
      if (!test_compare_dd(s5->particles[6].vectors[0].v[1], ==, 2016.0))
        r = 1;
      if (!test_compare_dd(s5->particles[6].vectors[1].v[0], ==, 2017.0))
        r = 1;
      if (!test_compare_dd(s5->particles[6].vectors[1].v[1], ==, 2018.0))
        r = 1;
      if (!test_compare_dd(s5->particles[6].vectors[1].v[2], ==, 2019.0))
        r = 1;

      if (nproc < 4)
        break;

      if (!test_compare_ii(s5->particles[7].base.particle_id, ==, 400))
        r = 1;

      if (!test_compare_ii(s5->particles[7].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[7].vectors[1].length, ==, 3))
        r = 1;

      if (nproc < 5)
        break;

      for (LPTX_idtype ir = 4, jj = 8; ir < nproc; ++ir) {
        LPTX_idtype lp = npt[ir % 4] + ir / 4;
        for (LPTX_idtype jk = 0; jk < lp; ++jk, ++jj) {
          if (!test_compare_ii(s5->particles[jj].base.particle_id, ==,
                               (ir + 1) * 100 + jk))
            r = 1;

          if (!test_compare_ii(s5->particles[jj].vectors[0].length, ==, 2))
            r = 1;
          if (!test_compare_ii(s5->particles[jj].vectors[1].length, ==, 3))
            r = 1;

          if (!test_compare_dd(s5->particles[jj].vectors[0].v[0], ==,
                               (ir + 1) * 1000 + jk * 5 + 0))
            r = 1;
          if (!test_compare_dd(s5->particles[jj].vectors[0].v[1], ==,
                               (ir + 1) * 1000 + jk * 5 + 1))
            r = 1;
          if (!test_compare_dd(s5->particles[jj].vectors[1].v[0], ==,
                               (ir + 1) * 1000 + jk * 5 + 2))
            r = 1;
          if (!test_compare_dd(s5->particles[jj].vectors[1].v[1], ==,
                               (ir + 1) * 1000 + jk * 5 + 3))
            r = 1;
          if (!test_compare_dd(s5->particles[jj].vectors[1].v[2], ==,
                               (ir + 1) * 1000 + jk * 5 + 4))
            r = 1;
        }
      }
    } while (0);
  } while (0);

  do {
    LPTX_idtype npt[] = {3, 4, 0, 1};
    LPTX_idtype np;

    if (r)
      break;

    if (s4)
      LPTX_particle_set_delete(s4);

    s4 = NULL;
    np = npt[irank % 4] + irank / 4;
    // 3,  4,  0,  1,  4,  5,  1,  2,  5,  6,  2,  3,  6, ...
    // 3,  7,  7,  8, 12, 17, 18, 20, 25, 31, 33, 36, 42, ...
    // n = nproc / 4
    // S = n * (2 * 3 + (n - 1) + 2 * 4 + (n - 1) + (n - 1) + 2 * 1 + (n - 1))
    //     / 2
    // S = n * (6 + 8 + 2 + 4 * (n - 1)) / 2
    //   = n * (6 + 2 * n)
    // S0 = n * (6 + 2 * n)
    // S1 = n * (6 + 2 * n) + (n + 3)
    //    = n * (7 + 2 * n) + 3
    // S2 = n * (8 + 2 * n) + 7
    // S3 = n * (9 + 2 * n) + 7

    if (np > 0) {
      if (!test_compare_pp((s4 = LPTX_particle_set_new(np, 2,
                                                       (LPTX_idtype[]){2, 3})),
                           !=, NULL))
        r = 1;
    }

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
      r = 1;
      break;
    }

    switch (irank) {
    case 0:
      if (!test_compare_ii(LPTX_particle_set_number_of_particles(s4), ==, 3))
        r = 1;
      break;
    case 1:
      if (!test_compare_ii(LPTX_particle_set_number_of_particles(s4), ==, 4))
        r = 1;
      break;
    case 2:
      if (!test_compare_pp(s4, ==, NULL))
        r = 1;
      break;
    case 3:
      if (!test_compare_ii(LPTX_particle_set_number_of_particles(s4), ==, 1))
        r = 1;
      break;
    default:
      switch (irank % 4) {
      case 0:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s4), ==,
                             3 + irank / 4))
          r = 1;
        break;
      case 1:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s4), ==,
                             4 + irank / 4))
          r = 1;
        break;
      case 2:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s4), ==,
                             0 + irank / 4))
          r = 1;
        break;
      case 3:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s4), ==,
                             1 + irank / 4))
          r = 1;
        break;
      }
      break;
    }

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
      r = 1;
      break;
    }

    if (s4) {
      for (LPTX_idtype jj = 0; jj < np; ++jj) {
        s4->particles[jj].base.particle_id = (irank + 1) * 100 + jj;
        *(LPTX_type *)&s4->particles[jj].vectors[0].v[0] =
          (irank + 1) * 1000 + jj * 5 + 0;
        *(LPTX_type *)&s4->particles[jj].vectors[0].v[1] =
          (irank + 1) * 1000 + jj * 5 + 1;
        *(LPTX_type *)&s4->particles[jj].vectors[1].v[0] =
          (irank + 1) * 1000 + jj * 5 + 2;
        *(LPTX_type *)&s4->particles[jj].vectors[1].v[1] =
          (irank + 1) * 1000 + jj * 5 + 3;
        *(LPTX_type *)&s4->particles[jj].vectors[1].v[2] =
          (irank + 1) * 1000 + jj * 5 + 4;
      }
    }

    if (s5)
      LPTX_particle_set_delete(s5);
    s5 = NULL;

    if (!test_compare_ii(LPTX_particle_set_allgather(&s5, s4, MPI_COMM_WORLD),
                         ==, MPI_SUCCESS)) {
      r = 1;
      break;
    }

    do {
      if (!test_compare_pp(s5, !=, NULL)) {
        r = 1;
        break;
      }

      switch (nproc) {
        int n;
      case 1:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 3))
          r = 1;
        break;
      case 2:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 7))
          r = 1;
        break;
      case 3:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 7))
          r = 1;
        break;
      case 4:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 8))
          r = 1;
        break;
      default:
        n = nproc / 4;
        switch (nproc % 4) {
        case 1:
          if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==,
                               n * (7 + 2 * n) + 3))
            r = 1;
          break;
        case 2:
          if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==,
                               n * (8 + 2 * n) + 7))
            r = 1;
          break;
        case 3:
          if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==,
                               n * (9 + 2 * n) + 7))
            r = 1;
          break;
        case 0:
          if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==,
                               n * (6 + 2 * n)))
            r = 1;
          break;
        }
        break;
      }
    } while (0);

    do {
      LPTX_idtype n;
      if (r)
        break;
      if (!s5) {
        r = 1;
        break;
      }

      n = LPTX_particle_set_number_of_particles(s5);

      if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 100))
        r = 1;
      if (!test_compare_ii(s5->particles[1].base.particle_id, ==, 101))
        r = 1;
      if (!test_compare_ii(s5->particles[2].base.particle_id, ==, 102))
        r = 1;

      if (!test_compare_ii(s5->particles[0].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[0].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[1].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[1].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[2].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[2].vectors[1].length, ==, 3))
        r = 1;

      if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1000.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1001.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1002.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1003.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1004.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[0].v[0], ==, 1005.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[0].v[1], ==, 1006.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[1].v[0], ==, 1007.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[1].v[1], ==, 1008.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[1].v[2], ==, 1009.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[0].v[0], ==, 1010.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[0].v[1], ==, 1011.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[1].v[0], ==, 1012.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[1].v[1], ==, 1013.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[1].v[2], ==, 1014.0))
        r = 1;

      if (nproc < 2)
        break;

      if (!test_compare_ii(s5->particles[3].base.particle_id, ==, 200))
        r = 1;
      if (!test_compare_ii(s5->particles[4].base.particle_id, ==, 201))
        r = 1;
      if (!test_compare_ii(s5->particles[5].base.particle_id, ==, 202))
        r = 1;
      if (!test_compare_ii(s5->particles[6].base.particle_id, ==, 203))
        r = 1;

      if (!test_compare_ii(s5->particles[3].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[3].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[4].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[4].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[5].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[5].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[6].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[6].vectors[1].length, ==, 3))
        r = 1;

      if (!test_compare_dd(s5->particles[3].vectors[0].v[0], ==, 2000.0))
        r = 1;
      if (!test_compare_dd(s5->particles[3].vectors[0].v[1], ==, 2001.0))
        r = 1;
      if (!test_compare_dd(s5->particles[3].vectors[1].v[0], ==, 2002.0))
        r = 1;
      if (!test_compare_dd(s5->particles[3].vectors[1].v[1], ==, 2003.0))
        r = 1;
      if (!test_compare_dd(s5->particles[3].vectors[1].v[2], ==, 2004.0))
        r = 1;
      if (!test_compare_dd(s5->particles[4].vectors[0].v[0], ==, 2005.0))
        r = 1;
      if (!test_compare_dd(s5->particles[4].vectors[0].v[1], ==, 2006.0))
        r = 1;
      if (!test_compare_dd(s5->particles[4].vectors[1].v[0], ==, 2007.0))
        r = 1;
      if (!test_compare_dd(s5->particles[4].vectors[1].v[1], ==, 2008.0))
        r = 1;
      if (!test_compare_dd(s5->particles[4].vectors[1].v[2], ==, 2009.0))
        r = 1;
      if (!test_compare_dd(s5->particles[5].vectors[0].v[0], ==, 2010.0))
        r = 1;
      if (!test_compare_dd(s5->particles[5].vectors[0].v[1], ==, 2011.0))
        r = 1;
      if (!test_compare_dd(s5->particles[5].vectors[1].v[0], ==, 2012.0))
        r = 1;
      if (!test_compare_dd(s5->particles[5].vectors[1].v[1], ==, 2013.0))
        r = 1;
      if (!test_compare_dd(s5->particles[5].vectors[1].v[2], ==, 2014.0))
        r = 1;
      if (!test_compare_dd(s5->particles[6].vectors[0].v[0], ==, 2015.0))
        r = 1;
      if (!test_compare_dd(s5->particles[6].vectors[0].v[1], ==, 2016.0))
        r = 1;
      if (!test_compare_dd(s5->particles[6].vectors[1].v[0], ==, 2017.0))
        r = 1;
      if (!test_compare_dd(s5->particles[6].vectors[1].v[1], ==, 2018.0))
        r = 1;
      if (!test_compare_dd(s5->particles[6].vectors[1].v[2], ==, 2019.0))
        r = 1;

      if (nproc < 4)
        break;

      if (!test_compare_ii(s5->particles[7].base.particle_id, ==, 400))
        r = 1;

      if (!test_compare_ii(s5->particles[7].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[7].vectors[1].length, ==, 3))
        r = 1;

      if (nproc < 5)
        break;

      for (LPTX_idtype ir = 4, jj = 8; ir < nproc; ++ir) {
        LPTX_idtype lp = npt[ir % 4] + ir / 4;
        for (LPTX_idtype jk = 0; jk < lp; ++jk, ++jj) {
          if (!test_compare_ii(s5->particles[jj].base.particle_id, ==,
                               (ir + 1) * 100 + jk))
            r = 1;

          if (!test_compare_ii(s5->particles[jj].vectors[0].length, ==, 2))
            r = 1;
          if (!test_compare_ii(s5->particles[jj].vectors[1].length, ==, 3))
            r = 1;

          if (!test_compare_dd(s5->particles[jj].vectors[0].v[0], ==,
                               (ir + 1) * 1000 + jk * 5 + 0))
            r = 1;
          if (!test_compare_dd(s5->particles[jj].vectors[0].v[1], ==,
                               (ir + 1) * 1000 + jk * 5 + 1))
            r = 1;
          if (!test_compare_dd(s5->particles[jj].vectors[1].v[0], ==,
                               (ir + 1) * 1000 + jk * 5 + 2))
            r = 1;
          if (!test_compare_dd(s5->particles[jj].vectors[1].v[1], ==,
                               (ir + 1) * 1000 + jk * 5 + 3))
            r = 1;
          if (!test_compare_dd(s5->particles[jj].vectors[1].v[2], ==,
                               (ir + 1) * 1000 + jk * 5 + 4))
            r = 1;
        }
      }
    } while (0);

    if (s5)
      LPTX_particle_set_delete(s5);
    s5 = NULL;
  } while (0);

  /* Scatter test for nproc == 1 */
  do {
    if (r)
      break;

    if (nproc != 1)
      break;

    if (s5) {
      LPTX_particle_set_delete(s5);
      s5 = NULL;
    }

    set->particles[0].base.particle_id = 400;
    set->particles[1].base.particle_id = 401;
    set->particles[2].base.particle_id = 402;
    set->particles[3].base.particle_id = 403;
    set->particles[0].base.position = LPTX_vector_c(20.0, 24.0, 28.0);
    set->particles[1].base.position = LPTX_vector_c(21.0, 25.0, 29.0);
    set->particles[2].base.position = LPTX_vector_c(22.0, 26.0, 30.0);
    set->particles[3].base.position = LPTX_vector_c(23.0, 27.0, 31.0);
    *(LPTX_type *)&set->particles[0].vectors[0].v[0] = 1000.0;
    *(LPTX_type *)&set->particles[0].vectors[0].v[1] = 1001.0;
    *(LPTX_type *)&set->particles[0].vectors[1].v[0] = 1002.0;
    *(LPTX_type *)&set->particles[0].vectors[1].v[1] = 1003.0;
    *(LPTX_type *)&set->particles[0].vectors[1].v[2] = 1004.0;
    *(LPTX_type *)&set->particles[1].vectors[0].v[0] = 1005.0;
    *(LPTX_type *)&set->particles[1].vectors[0].v[1] = 1006.0;
    *(LPTX_type *)&set->particles[1].vectors[1].v[0] = 1007.0;
    *(LPTX_type *)&set->particles[1].vectors[1].v[1] = 1008.0;
    *(LPTX_type *)&set->particles[1].vectors[1].v[2] = 1009.0;
    *(LPTX_type *)&set->particles[2].vectors[0].v[0] = 1010.0;
    *(LPTX_type *)&set->particles[2].vectors[0].v[1] = 1011.0;
    *(LPTX_type *)&set->particles[2].vectors[1].v[0] = 1012.0;
    *(LPTX_type *)&set->particles[2].vectors[1].v[1] = 1013.0;
    *(LPTX_type *)&set->particles[2].vectors[1].v[2] = 1014.0;
    *(LPTX_type *)&set->particles[3].vectors[0].v[0] = 1015.0;
    *(LPTX_type *)&set->particles[3].vectors[0].v[1] = 1016.0;
    *(LPTX_type *)&set->particles[3].vectors[1].v[0] = 1017.0;
    *(LPTX_type *)&set->particles[3].vectors[1].v[1] = 1018.0;
    *(LPTX_type *)&set->particles[3].vectors[1].v[2] = 1019.0;

    if (!test_compare_ii(LPTX_particle_set_scatter(&s5, set, 0, 0, 4,
                                                   (int[]){0, 0, -1, 0},
                                                   MPI_COMM_WORLD),
                         ==, MPI_SUCCESS))
      r = 1;

    if (!test_compare_pp(s5, !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 3))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_number_of_vectors(s5), ==, 2))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_number_of_data(s5), ==, 5))
      r = 1;

    if (r)
      break;

    if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 400))
      r = 1;
    if (!test_compare_ii(s5->particles[1].base.particle_id, ==, 401))
      r = 1;
    if (!test_compare_ii(s5->particles[2].base.particle_id, ==, 403))
      r = 1;
    if (!test_compare_dd(LPTX_vector_x(s5->particles[0].base.position), ==,
                         20.0))
      r = 1;
    if (!test_compare_dd(LPTX_vector_x(s5->particles[1].base.position), ==,
                         21.0))
      r = 1;
    if (!test_compare_dd(LPTX_vector_x(s5->particles[2].base.position), ==,
                         23.0))
      r = 1;
    if (!test_compare_dd(LPTX_vector_y(s5->particles[0].base.position), ==,
                         24.0))
      r = 1;
    if (!test_compare_dd(LPTX_vector_y(s5->particles[1].base.position), ==,
                         25.0))
      r = 1;
    if (!test_compare_dd(LPTX_vector_y(s5->particles[2].base.position), ==,
                         27.0))
      r = 1;
    if (!test_compare_dd(LPTX_vector_z(s5->particles[0].base.position), ==,
                         28.0))
      r = 1;
    if (!test_compare_dd(LPTX_vector_z(s5->particles[1].base.position), ==,
                         29.0))
      r = 1;
    if (!test_compare_dd(LPTX_vector_z(s5->particles[2].base.position), ==,
                         31.0))
      r = 1;
    if (!test_compare_ii(s5->particles[0].vectors[0].length, ==, 2))
      r = 1;
    if (!test_compare_ii(s5->particles[1].vectors[0].length, ==, 2))
      r = 1;
    if (!test_compare_ii(s5->particles[2].vectors[0].length, ==, 2))
      r = 1;
    if (!test_compare_ii(s5->particles[0].vectors[1].length, ==, 3))
      r = 1;
    if (!test_compare_ii(s5->particles[1].vectors[1].length, ==, 3))
      r = 1;
    if (!test_compare_ii(s5->particles[2].vectors[1].length, ==, 3))
      r = 1;
    if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1000.0))
      r = 1;
    if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1001.0))
      r = 1;
    if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1002.0))
      r = 1;
    if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1003.0))
      r = 1;
    if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1004.0))
      r = 1;
    if (!test_compare_dd(s5->particles[1].vectors[0].v[0], ==, 1005.0))
      r = 1;
    if (!test_compare_dd(s5->particles[1].vectors[0].v[1], ==, 1006.0))
      r = 1;
    if (!test_compare_dd(s5->particles[1].vectors[1].v[0], ==, 1007.0))
      r = 1;
    if (!test_compare_dd(s5->particles[1].vectors[1].v[1], ==, 1008.0))
      r = 1;
    if (!test_compare_dd(s5->particles[1].vectors[1].v[2], ==, 1009.0))
      r = 1;
    if (!test_compare_dd(s5->particles[2].vectors[0].v[0], ==, 1015.0))
      r = 1;
    if (!test_compare_dd(s5->particles[2].vectors[0].v[1], ==, 1016.0))
      r = 1;
    if (!test_compare_dd(s5->particles[2].vectors[1].v[0], ==, 1017.0))
      r = 1;
    if (!test_compare_dd(s5->particles[2].vectors[1].v[1], ==, 1018.0))
      r = 1;
    if (!test_compare_dd(s5->particles[2].vectors[1].v[2], ==, 1019.0))
      r = 1;

    if (s5) {
      LPTX_particle_set_delete(s5);
      s5 = NULL;
    }

    if (!test_compare_ii(LPTX_particle_set_scatter(&s5, set, 0, 0, 4,
                                                   (int[]){-1, -1, -1, -1},
                                                   MPI_COMM_WORLD),
                         ==, MPI_SUCCESS))
      r = 1;

    if (!test_compare_pp(s5, !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 0))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_number_of_vectors(s5), ==, 2))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_number_of_data(s5), ==, 5))
      r = 1;
  } while (0);

  /* Scatter test for nproc == 2 */
  do {
    if (r)
      break;

    if (nproc != 2)
      break;

    if (s5) {
      LPTX_particle_set_delete(s5);
      s5 = NULL;
    }

    if (irank == 0) {
      set->particles[0].base.particle_id = 500;
      set->particles[1].base.particle_id = 501;
      set->particles[2].base.particle_id = 502;
      set->particles[3].base.particle_id = 503;
      set->particles[0].base.position = LPTX_vector_c(40.0, 44.0, 48.0);
      set->particles[1].base.position = LPTX_vector_c(41.0, 45.0, 49.0);
      set->particles[2].base.position = LPTX_vector_c(42.0, 46.0, 50.0);
      set->particles[3].base.position = LPTX_vector_c(43.0, 47.0, 51.0);
      *(LPTX_type *)&set->particles[0].vectors[0].v[0] = 1100.0;
      *(LPTX_type *)&set->particles[0].vectors[0].v[1] = 1101.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[0] = 1102.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[1] = 1103.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[2] = 1104.0;
      *(LPTX_type *)&set->particles[1].vectors[0].v[0] = 1105.0;
      *(LPTX_type *)&set->particles[1].vectors[0].v[1] = 1106.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[0] = 1107.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[1] = 1108.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[2] = 1109.0;
      *(LPTX_type *)&set->particles[2].vectors[0].v[0] = 1110.0;
      *(LPTX_type *)&set->particles[2].vectors[0].v[1] = 1111.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[0] = 1112.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[1] = 1113.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[2] = 1114.0;
      *(LPTX_type *)&set->particles[3].vectors[0].v[0] = 1115.0;
      *(LPTX_type *)&set->particles[3].vectors[0].v[1] = 1116.0;
      *(LPTX_type *)&set->particles[3].vectors[1].v[0] = 1117.0;
      *(LPTX_type *)&set->particles[3].vectors[1].v[1] = 1118.0;
      *(LPTX_type *)&set->particles[3].vectors[1].v[2] = 1119.0;

      if (!test_compare_ii(LPTX_particle_set_scatter(&s5, set, 0, 0, 4,
                                                     (int[]){0, 1, -1, 0},
                                                     MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_scatter(&s5, NULL, 0, 0, 0, NULL,
                                                     MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }

    do {
      if (!test_compare_pp(s5, !=, NULL)) {
        r = 1;
        break;
      }

      switch (irank) {
      case 0:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 2))
          r = 1;
        break;
      case 1:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 1))
          r = 1;
        break;
      default:
        r = 1;
        break;
      }

      if (!test_compare_ii(LPTX_particle_set_number_of_vectors(s5), ==, 2))
        r = 1;

      if (!test_compare_ii(LPTX_particle_set_number_of_data(s5), ==, 5))
        r = 1;

      if (r)
        break;

      switch (irank) {
      case 0:
        if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 500))
          r = 1;
        if (!test_compare_ii(s5->particles[1].base.particle_id, ==, 503))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[0].base.position), ==,
                             40.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[1].base.position), ==,
                             43.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[0].base.position), ==,
                             44.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[1].base.position), ==,
                             47.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[0].base.position), ==,
                             48.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[1].base.position), ==,
                             51.0))
          r = 1;
        if (!test_compare_ii(s5->particles[0].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[1].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[0].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_ii(s5->particles[1].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1100.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1101.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1102.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1103.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1104.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].v[0], ==, 1115.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].v[1], ==, 1116.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[0], ==, 1117.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[1], ==, 1118.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[2], ==, 1119.0))
          r = 1;
        break;
      case 1:
        if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 501))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[0].base.position), ==,
                             41.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[0].base.position), ==,
                             45.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[0].base.position), ==,
                             49.0))
          r = 1;
        if (!test_compare_ii(s5->particles[0].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[0].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1105.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1106.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1107.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1108.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1109.0))
          r = 1;
        break;
      }
    } while (0);

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
      r = 1;
      break;
    }

    if (s5) {
      LPTX_particle_set_delete(s5);
      s5 = NULL;
    }

    if (irank == 1) {
      set->particles[0].base.particle_id = 600;
      set->particles[1].base.particle_id = 601;
      set->particles[2].base.particle_id = 602;
      set->particles[3].base.particle_id = 603;
      set->particles[0].base.position = LPTX_vector_c(60.0, 64.0, 68.0);
      set->particles[1].base.position = LPTX_vector_c(61.0, 65.0, 69.0);
      set->particles[2].base.position = LPTX_vector_c(62.0, 66.0, 70.0);
      set->particles[3].base.position = LPTX_vector_c(63.0, 67.0, 71.0);
      *(LPTX_type *)&set->particles[0].vectors[0].v[0] = 1200.0;
      *(LPTX_type *)&set->particles[0].vectors[0].v[1] = 1201.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[0] = 1202.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[1] = 1203.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[2] = 1204.0;
      *(LPTX_type *)&set->particles[1].vectors[0].v[0] = 1205.0;
      *(LPTX_type *)&set->particles[1].vectors[0].v[1] = 1206.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[0] = 1207.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[1] = 1208.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[2] = 1209.0;
      *(LPTX_type *)&set->particles[2].vectors[0].v[0] = 1210.0;
      *(LPTX_type *)&set->particles[2].vectors[0].v[1] = 1211.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[0] = 1212.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[1] = 1213.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[2] = 1214.0;
      *(LPTX_type *)&set->particles[3].vectors[0].v[0] = 1215.0;
      *(LPTX_type *)&set->particles[3].vectors[0].v[1] = 1216.0;
      *(LPTX_type *)&set->particles[3].vectors[1].v[0] = 1217.0;
      *(LPTX_type *)&set->particles[3].vectors[1].v[1] = 1218.0;
      *(LPTX_type *)&set->particles[3].vectors[1].v[2] = 1219.0;

      if (!test_compare_ii(LPTX_particle_set_scatter(&s5, set, 0, 1, 4,
                                                     (int[]){0, 0, -1, 0},
                                                     MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_scatter(&s5, NULL, 0, 1, 0, NULL,
                                                     MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }

    do {
      if (!test_compare_pp(s5, !=, NULL)) {
        r = 1;
        break;
      }

      switch (irank) {
      case 0:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 3))
          r = 1;
        break;
      case 1:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 0))
          r = 1;
        break;
      default:
        r = 1;
        break;
      }

      if (!test_compare_ii(LPTX_particle_set_number_of_vectors(s5), ==, 2))
        r = 1;

      if (!test_compare_ii(LPTX_particle_set_number_of_data(s5), ==, 5))
        r = 1;

      if (r)
        break;

      if (irank != 0)
        break;

      if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 600))
        r = 1;
      if (!test_compare_ii(s5->particles[1].base.particle_id, ==, 601))
        r = 1;
      if (!test_compare_ii(s5->particles[2].base.particle_id, ==, 603))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(s5->particles[0].base.position), ==,
                           60.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(s5->particles[1].base.position), ==,
                           61.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(s5->particles[2].base.position), ==,
                           63.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(s5->particles[0].base.position), ==,
                           64.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(s5->particles[1].base.position), ==,
                           65.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(s5->particles[2].base.position), ==,
                           67.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(s5->particles[0].base.position), ==,
                           68.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(s5->particles[1].base.position), ==,
                           69.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(s5->particles[2].base.position), ==,
                           71.0))
        r = 1;
      if (!test_compare_ii(s5->particles[0].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[1].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[2].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[0].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[1].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[2].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1200.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1201.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1202.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1203.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1204.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[0].v[0], ==, 1205.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[0].v[1], ==, 1206.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[1].v[0], ==, 1207.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[1].v[1], ==, 1208.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[1].v[2], ==, 1209.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[0].v[0], ==, 1215.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[0].v[1], ==, 1216.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[1].v[0], ==, 1217.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[1].v[1], ==, 1218.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[1].v[2], ==, 1219.0))
        r = 1;
    } while (0);

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
      r = 1;
      break;
    }

    if (s5) {
      LPTX_particle_set_delete(s5);
      s5 = NULL;
    }

    if (irank == 0) {
      set->particles[0].base.particle_id = 700;
      set->particles[1].base.particle_id = 701;
      set->particles[2].base.particle_id = 702;
      set->particles[3].base.particle_id = 703;
      set->particles[0].base.position = LPTX_vector_c(80.0, 84.0, 88.0);
      set->particles[1].base.position = LPTX_vector_c(81.0, 85.0, 89.0);
      set->particles[2].base.position = LPTX_vector_c(82.0, 86.0, 90.0);
      set->particles[3].base.position = LPTX_vector_c(83.0, 87.0, 91.0);
      *(LPTX_type *)&set->particles[0].vectors[0].v[0] = 1300.0;
      *(LPTX_type *)&set->particles[0].vectors[0].v[1] = 1301.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[0] = 1302.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[1] = 1303.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[2] = 1304.0;
      *(LPTX_type *)&set->particles[1].vectors[0].v[0] = 1305.0;
      *(LPTX_type *)&set->particles[1].vectors[0].v[1] = 1306.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[0] = 1307.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[1] = 1308.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[2] = 1309.0;
      *(LPTX_type *)&set->particles[2].vectors[0].v[0] = 1310.0;
      *(LPTX_type *)&set->particles[2].vectors[0].v[1] = 1311.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[0] = 1312.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[1] = 1313.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[2] = 1314.0;
      *(LPTX_type *)&set->particles[3].vectors[0].v[0] = 1315.0;
      *(LPTX_type *)&set->particles[3].vectors[0].v[1] = 1316.0;
      *(LPTX_type *)&set->particles[3].vectors[1].v[0] = 1317.0;
      *(LPTX_type *)&set->particles[3].vectors[1].v[1] = 1318.0;
      *(LPTX_type *)&set->particles[3].vectors[1].v[2] = 1319.0;

      if (!test_compare_ii(LPTX_particle_set_scatter(&s5, set, 0, 0, 4,
                                                     (int[]){0, 0, -1, 0},
                                                     MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_scatter(&s5, NULL, 0, 0, 0, NULL,
                                                     MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }

    do {
      if (!test_compare_pp(s5, !=, NULL)) {
        r = 1;
        break;
      }

      switch (irank) {
      case 0:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 3))
          r = 1;
        break;
      case 1:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 0))
          r = 1;
        break;
      default:
        r = 1;
        break;
      }

      if (!test_compare_ii(LPTX_particle_set_number_of_vectors(s5), ==, 2))
        r = 1;

      if (!test_compare_ii(LPTX_particle_set_number_of_data(s5), ==, 5))
        r = 1;

      if (r)
        break;

      if (irank != 0)
        break;

      if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 700))
        r = 1;
      if (!test_compare_ii(s5->particles[1].base.particle_id, ==, 701))
        r = 1;
      if (!test_compare_ii(s5->particles[2].base.particle_id, ==, 703))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(s5->particles[0].base.position), ==,
                           80.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(s5->particles[1].base.position), ==,
                           81.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_x(s5->particles[2].base.position), ==,
                           83.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(s5->particles[0].base.position), ==,
                           84.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(s5->particles[1].base.position), ==,
                           85.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_y(s5->particles[2].base.position), ==,
                           87.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(s5->particles[0].base.position), ==,
                           88.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(s5->particles[1].base.position), ==,
                           89.0))
        r = 1;
      if (!test_compare_dd(LPTX_vector_z(s5->particles[2].base.position), ==,
                           91.0))
        r = 1;
      if (!test_compare_ii(s5->particles[0].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[1].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[2].vectors[0].length, ==, 2))
        r = 1;
      if (!test_compare_ii(s5->particles[0].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[1].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_ii(s5->particles[2].vectors[1].length, ==, 3))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1300.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1301.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1302.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1303.0))
        r = 1;
      if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1304.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[0].v[0], ==, 1305.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[0].v[1], ==, 1306.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[1].v[0], ==, 1307.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[1].v[1], ==, 1308.0))
        r = 1;
      if (!test_compare_dd(s5->particles[1].vectors[1].v[2], ==, 1309.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[0].v[0], ==, 1315.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[0].v[1], ==, 1316.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[1].v[0], ==, 1317.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[1].v[1], ==, 1318.0))
        r = 1;
      if (!test_compare_dd(s5->particles[2].vectors[1].v[2], ==, 1319.0))
        r = 1;
    } while (0);

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
      r = 1;
      break;
    }
  } while (0);

  /* Scatter test for nproc == 4 */
  do {
    if (r)
      break;

    if (nproc != 4)
      break;

    if (s5) {
      LPTX_particle_set_delete(s5);
      s5 = NULL;
    }

    if (irank == 0) {
      set->particles[0].base.particle_id = 800;
      set->particles[1].base.particle_id = 801;
      set->particles[2].base.particle_id = 802;
      set->particles[3].base.particle_id = 803;
      set->particles[4].base.particle_id = 804;
      set->particles[5].base.particle_id = 805;
      set->particles[6].base.particle_id = 806;
      set->particles[7].base.particle_id = 807;
      set->particles[0].base.position = LPTX_vector_c(100.0, 108.0, 116.0);
      set->particles[1].base.position = LPTX_vector_c(101.0, 109.0, 117.0);
      set->particles[2].base.position = LPTX_vector_c(102.0, 110.0, 118.0);
      set->particles[3].base.position = LPTX_vector_c(103.0, 111.0, 119.0);
      set->particles[4].base.position = LPTX_vector_c(104.0, 112.0, 120.0);
      set->particles[5].base.position = LPTX_vector_c(105.0, 113.0, 121.0);
      set->particles[6].base.position = LPTX_vector_c(106.0, 114.0, 122.0);
      set->particles[7].base.position = LPTX_vector_c(107.0, 115.0, 123.0);
      *(LPTX_type *)&set->particles[0].vectors[0].v[0] = 1400.0;
      *(LPTX_type *)&set->particles[0].vectors[0].v[1] = 1401.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[0] = 1402.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[1] = 1403.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[2] = 1404.0;
      *(LPTX_type *)&set->particles[1].vectors[0].v[0] = 1405.0;
      *(LPTX_type *)&set->particles[1].vectors[0].v[1] = 1406.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[0] = 1407.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[1] = 1408.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[2] = 1409.0;
      *(LPTX_type *)&set->particles[2].vectors[0].v[0] = 1410.0;
      *(LPTX_type *)&set->particles[2].vectors[0].v[1] = 1411.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[0] = 1412.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[1] = 1413.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[2] = 1414.0;
      *(LPTX_type *)&set->particles[3].vectors[0].v[0] = 1415.0;
      *(LPTX_type *)&set->particles[3].vectors[0].v[1] = 1416.0;
      *(LPTX_type *)&set->particles[3].vectors[1].v[0] = 1417.0;
      *(LPTX_type *)&set->particles[3].vectors[1].v[1] = 1418.0;
      *(LPTX_type *)&set->particles[3].vectors[1].v[2] = 1419.0;
      *(LPTX_type *)&set->particles[4].vectors[0].v[0] = 1420.0;
      *(LPTX_type *)&set->particles[4].vectors[0].v[1] = 1421.0;
      *(LPTX_type *)&set->particles[4].vectors[1].v[0] = 1422.0;
      *(LPTX_type *)&set->particles[4].vectors[1].v[1] = 1423.0;
      *(LPTX_type *)&set->particles[4].vectors[1].v[2] = 1424.0;
      *(LPTX_type *)&set->particles[5].vectors[0].v[0] = 1425.0;
      *(LPTX_type *)&set->particles[5].vectors[0].v[1] = 1426.0;
      *(LPTX_type *)&set->particles[5].vectors[1].v[0] = 1427.0;
      *(LPTX_type *)&set->particles[5].vectors[1].v[1] = 1428.0;
      *(LPTX_type *)&set->particles[5].vectors[1].v[2] = 1429.0;
      *(LPTX_type *)&set->particles[6].vectors[0].v[0] = 1430.0;
      *(LPTX_type *)&set->particles[6].vectors[0].v[1] = 1431.0;
      *(LPTX_type *)&set->particles[6].vectors[1].v[0] = 1432.0;
      *(LPTX_type *)&set->particles[6].vectors[1].v[1] = 1433.0;
      *(LPTX_type *)&set->particles[6].vectors[1].v[2] = 1434.0;
      *(LPTX_type *)&set->particles[7].vectors[0].v[0] = 1435.0;
      *(LPTX_type *)&set->particles[7].vectors[0].v[1] = 1436.0;
      *(LPTX_type *)&set->particles[7].vectors[1].v[0] = 1437.0;
      *(LPTX_type *)&set->particles[7].vectors[1].v[1] = 1438.0;
      *(LPTX_type *)&set->particles[7].vectors[1].v[2] = 1439.0;

      if (!test_compare_ii(LPTX_particle_set_scatter(&s5, set, 0, 0, 8,
                                                     (int[]){0, 1, -1, 0, 2, 3,
                                                             2, 1},
                                                     MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_scatter(&s5, NULL, 0, 0, 0, NULL,
                                                     MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }

    do {
      if (!test_compare_pp(s5, !=, NULL)) {
        r = 1;
        break;
      }

      switch (irank) {
      case 0:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 2))
          r = 1;
        break;
      case 1:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 2))
          r = 1;
        break;
      case 2:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 2))
          r = 1;
        break;
      case 3:
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 1))
          r = 1;
        break;
      default:
        r = 1;
        break;
      }

      if (!test_compare_ii(LPTX_particle_set_number_of_vectors(s5), ==, 2))
        r = 1;

      if (!test_compare_ii(LPTX_particle_set_number_of_data(s5), ==, 5))
        r = 1;

      if (r)
        break;

      switch (irank) {
      case 0:
        if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 800))
          r = 1;
        if (!test_compare_ii(s5->particles[1].base.particle_id, ==, 803))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[0].base.position), ==,
                             100.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[1].base.position), ==,
                             103.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[0].base.position), ==,
                             108.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[1].base.position), ==,
                             111.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[0].base.position), ==,
                             116.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[1].base.position), ==,
                             119.0))
          r = 1;
        if (!test_compare_ii(s5->particles[0].number_of_vectors, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[1].number_of_vectors, ==, 2))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1400.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1401.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1402.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1403.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1404.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].v[0], ==, 1415.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].v[1], ==, 1416.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[0], ==, 1417.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[1], ==, 1418.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[2], ==, 1419.0))
          r = 1;
        break;
      case 1:
        if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 801))
          r = 1;
        if (!test_compare_ii(s5->particles[1].base.particle_id, ==, 807))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[0].base.position), ==,
                             101.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[1].base.position), ==,
                             107.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[0].base.position), ==,
                             109.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[1].base.position), ==,
                             115.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[0].base.position), ==,
                             117.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[1].base.position), ==,
                             123.0))
          r = 1;
        if (!test_compare_ii(s5->particles[0].number_of_vectors, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[1].number_of_vectors, ==, 2))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1405.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1406.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1407.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1408.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1409.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].v[0], ==, 1435.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].v[1], ==, 1436.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[0], ==, 1437.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[1], ==, 1438.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[2], ==, 1439.0))
          r = 1;
        break;
      case 2:
        if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 804))
          r = 1;
        if (!test_compare_ii(s5->particles[1].base.particle_id, ==, 806))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[0].base.position), ==,
                             104.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[1].base.position), ==,
                             106.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[0].base.position), ==,
                             112.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[1].base.position), ==,
                             114.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[0].base.position), ==,
                             120.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[1].base.position), ==,
                             122.0))
          r = 1;
        if (!test_compare_ii(s5->particles[0].number_of_vectors, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[1].number_of_vectors, ==, 2))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1420.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1421.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1422.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1423.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1424.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].v[0], ==, 1430.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].v[1], ==, 1431.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[0], ==, 1432.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[1], ==, 1433.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[2], ==, 1434.0))
          r = 1;
        break;
      case 3:
        if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 805))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[0].base.position), ==,
                             105.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[0].base.position), ==,
                             113.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[0].base.position), ==,
                             121.0))
          r = 1;
        if (!test_compare_ii(s5->particles[0].number_of_vectors, ==, 2))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1425.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1426.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1427.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1428.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1429.0))
          r = 1;
        break;
      default:
        r = 1;
        break;
      }
    } while (0);

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
      r = 1;
      break;
    }

  } while (0);

  /* Alltoall test for nproc == 1 */
  do {
    if (r)
      break;

    if (nproc != 1)
      break;

    set->particles[0].base.particle_id = 900;
    set->particles[1].base.particle_id = 901;
    set->particles[2].base.particle_id = 902;
    set->particles[3].base.particle_id = 903;
    set->particles[0].base.position = LPTX_vector_c(200.0, 204.0, 208.0);
    set->particles[1].base.position = LPTX_vector_c(201.0, 205.0, 209.0);
    set->particles[2].base.position = LPTX_vector_c(202.0, 206.0, 210.0);
    set->particles[3].base.position = LPTX_vector_c(203.0, 207.0, 211.0);
    *(LPTX_type *)&set->particles[0].vectors[0].v[0] = 1500.0;
    *(LPTX_type *)&set->particles[0].vectors[0].v[1] = 1501.0;
    *(LPTX_type *)&set->particles[0].vectors[1].v[0] = 1502.0;
    *(LPTX_type *)&set->particles[0].vectors[1].v[1] = 1503.0;
    *(LPTX_type *)&set->particles[0].vectors[1].v[2] = 1504.0;
    *(LPTX_type *)&set->particles[1].vectors[0].v[0] = 1505.0;
    *(LPTX_type *)&set->particles[1].vectors[0].v[1] = 1506.0;
    *(LPTX_type *)&set->particles[1].vectors[1].v[0] = 1507.0;
    *(LPTX_type *)&set->particles[1].vectors[1].v[1] = 1508.0;
    *(LPTX_type *)&set->particles[1].vectors[1].v[2] = 1509.0;
    *(LPTX_type *)&set->particles[2].vectors[0].v[0] = 1510.0;
    *(LPTX_type *)&set->particles[2].vectors[0].v[1] = 1511.0;
    *(LPTX_type *)&set->particles[2].vectors[1].v[0] = 1512.0;
    *(LPTX_type *)&set->particles[2].vectors[1].v[1] = 1513.0;
    *(LPTX_type *)&set->particles[2].vectors[1].v[2] = 1514.0;
    *(LPTX_type *)&set->particles[3].vectors[0].v[0] = 1515.0;
    *(LPTX_type *)&set->particles[3].vectors[0].v[1] = 1516.0;
    *(LPTX_type *)&set->particles[3].vectors[1].v[0] = 1517.0;
    *(LPTX_type *)&set->particles[3].vectors[1].v[1] = 1518.0;
    *(LPTX_type *)&set->particles[3].vectors[1].v[2] = 1519.0;

    if (s5) {
      LPTX_particle_set_delete(s5);
      s5 = NULL;
    }

    if (!test_compare_ii(LPTX_particle_set_alltoall(&s5, set, 0, 4,
                                                    (int[]){0, -1, 0, 0},
                                                    MPI_COMM_WORLD),
                         ==, MPI_SUCCESS))
      r = 1;

    if (!test_compare_pp(s5, !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 3))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_number_of_vectors(s5), ==, 2))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_number_of_data(s5), ==, 5))
      r = 1;

    if (r)
      break;

    if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 900))
      r = 1;
    if (!test_compare_ii(s5->particles[1].base.particle_id, ==, 902))
      r = 1;
    if (!test_compare_ii(s5->particles[2].base.particle_id, ==, 903))
      r = 1;
    if (!test_compare_dd(LPTX_vector_x(s5->particles[0].base.position), ==,
                         200.0))
      r = 1;
    if (!test_compare_dd(LPTX_vector_x(s5->particles[1].base.position), ==,
                         202.0))
      r = 1;
    if (!test_compare_dd(LPTX_vector_x(s5->particles[2].base.position), ==,
                         203.0))
      r = 1;
    if (!test_compare_dd(LPTX_vector_y(s5->particles[0].base.position), ==,
                         204.0))
      r = 1;
    if (!test_compare_dd(LPTX_vector_y(s5->particles[1].base.position), ==,
                         206.0))
      r = 1;
    if (!test_compare_dd(LPTX_vector_y(s5->particles[2].base.position), ==,
                         207.0))
      r = 1;
    if (!test_compare_dd(LPTX_vector_z(s5->particles[0].base.position), ==,
                         208.0))
      r = 1;
    if (!test_compare_dd(LPTX_vector_z(s5->particles[1].base.position), ==,
                         210.0))
      r = 1;
    if (!test_compare_dd(LPTX_vector_z(s5->particles[2].base.position), ==,
                         211.0))
      r = 1;
    if (!test_compare_ii(s5->particles[0].vectors[0].length, ==, 2))
      r = 1;
    if (!test_compare_ii(s5->particles[1].vectors[0].length, ==, 2))
      r = 1;
    if (!test_compare_ii(s5->particles[2].vectors[0].length, ==, 2))
      r = 1;
    if (!test_compare_ii(s5->particles[0].vectors[1].length, ==, 3))
      r = 1;
    if (!test_compare_ii(s5->particles[1].vectors[1].length, ==, 3))
      r = 1;
    if (!test_compare_ii(s5->particles[2].vectors[1].length, ==, 3))
      r = 1;
    if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1500.0))
      r = 1;
    if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1501.0))
      r = 1;
    if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1502.0))
      r = 1;
    if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1503.0))
      r = 1;
    if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1504.0))
      r = 1;
    if (!test_compare_dd(s5->particles[1].vectors[0].v[0], ==, 1510.0))
      r = 1;
    if (!test_compare_dd(s5->particles[1].vectors[0].v[1], ==, 1511.0))
      r = 1;
    if (!test_compare_dd(s5->particles[1].vectors[1].v[0], ==, 1512.0))
      r = 1;
    if (!test_compare_dd(s5->particles[1].vectors[1].v[1], ==, 1513.0))
      r = 1;
    if (!test_compare_dd(s5->particles[1].vectors[1].v[2], ==, 1514.0))
      r = 1;
    if (!test_compare_dd(s5->particles[2].vectors[0].v[0], ==, 1515.0))
      r = 1;
    if (!test_compare_dd(s5->particles[2].vectors[0].v[1], ==, 1516.0))
      r = 1;
    if (!test_compare_dd(s5->particles[2].vectors[1].v[0], ==, 1517.0))
      r = 1;
    if (!test_compare_dd(s5->particles[2].vectors[1].v[1], ==, 1518.0))
      r = 1;
    if (!test_compare_dd(s5->particles[2].vectors[1].v[2], ==, 1519.0))
      r = 1;

    if (s5) {
      LPTX_particle_set_delete(s5);
      s5 = NULL;
    }

    if (!test_compare_ii(LPTX_particle_set_alltoall(&s5, set, 0, 4,
                                                    (int[]){-1, -1, -1, -1},
                                                    MPI_COMM_WORLD),
                         ==, MPI_SUCCESS))
      r = 1;

    if (!test_compare_pp(s5, !=, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 0))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_number_of_vectors(s5), ==, 2))
      r = 1;

    if (!test_compare_ii(LPTX_particle_set_number_of_data(s5), ==, 5))
      r = 1;
  } while (0);

  /* alltoall test for nproc == 2 (with NULL set) */
  do {
    if (r)
      break;

    if (nproc != 2)
      break;

    set->particles[0].base.particle_id = 900;
    set->particles[1].base.particle_id = 901;
    set->particles[2].base.particle_id = 902;
    set->particles[3].base.particle_id = 903;
    set->particles[0].base.position = LPTX_vector_c(200.0, 204.0, 208.0);
    set->particles[1].base.position = LPTX_vector_c(201.0, 205.0, 209.0);
    set->particles[2].base.position = LPTX_vector_c(202.0, 206.0, 210.0);
    set->particles[3].base.position = LPTX_vector_c(203.0, 207.0, 211.0);
    *(LPTX_type *)&set->particles[0].vectors[0].v[0] = 1500.0;
    *(LPTX_type *)&set->particles[0].vectors[0].v[1] = 1501.0;
    *(LPTX_type *)&set->particles[0].vectors[1].v[0] = 1502.0;
    *(LPTX_type *)&set->particles[0].vectors[1].v[1] = 1503.0;
    *(LPTX_type *)&set->particles[0].vectors[1].v[2] = 1504.0;
    *(LPTX_type *)&set->particles[1].vectors[0].v[0] = 1505.0;
    *(LPTX_type *)&set->particles[1].vectors[0].v[1] = 1506.0;
    *(LPTX_type *)&set->particles[1].vectors[1].v[0] = 1507.0;
    *(LPTX_type *)&set->particles[1].vectors[1].v[1] = 1508.0;
    *(LPTX_type *)&set->particles[1].vectors[1].v[2] = 1509.0;
    *(LPTX_type *)&set->particles[2].vectors[0].v[0] = 1510.0;
    *(LPTX_type *)&set->particles[2].vectors[0].v[1] = 1511.0;
    *(LPTX_type *)&set->particles[2].vectors[1].v[0] = 1512.0;
    *(LPTX_type *)&set->particles[2].vectors[1].v[1] = 1513.0;
    *(LPTX_type *)&set->particles[2].vectors[1].v[2] = 1514.0;
    *(LPTX_type *)&set->particles[3].vectors[0].v[0] = 1515.0;
    *(LPTX_type *)&set->particles[3].vectors[0].v[1] = 1516.0;
    *(LPTX_type *)&set->particles[3].vectors[1].v[0] = 1517.0;
    *(LPTX_type *)&set->particles[3].vectors[1].v[1] = 1518.0;
    *(LPTX_type *)&set->particles[3].vectors[1].v[2] = 1519.0;

    if (s5) {
      LPTX_particle_set_delete(s5);
      s5 = NULL;
    }

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_alltoall(&s5, set, 0, 4,
                                                      (int[]){0, -1, 1, 0},
                                                      MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_alltoall(&s5, NULL, 0, 0, NULL,
                                                      MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }

    do {
      if (!test_compare_pp(s5, !=, NULL)) {
        r = 1;
        break;
      }

      if (irank == 0) {
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 2))
          r = 1;
      } else {
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 1))
          r = 1;
      }

      if (!test_compare_ii(LPTX_particle_set_number_of_vectors(s5), ==, 2))
        r = 1;

      if (!test_compare_ii(LPTX_particle_set_number_of_data(s5), ==, 5))
        r = 1;

      if (r)
        break;

      if (irank == 0) {
        if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 900))
          r = 1;
        if (!test_compare_ii(s5->particles[1].base.particle_id, ==, 903))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[0].base.position), ==,
                             200.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[1].base.position), ==,
                             203.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[0].base.position), ==,
                             204.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[1].base.position), ==,
                             207.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[0].base.position), ==,
                             208.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[1].base.position), ==,
                             211.0))
          r = 1;
        if (!test_compare_ii(s5->particles[0].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[1].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[0].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_ii(s5->particles[1].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1500.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1501.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1502.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1503.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1504.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].v[0], ==, 1515.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].v[1], ==, 1516.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[0], ==, 1517.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[1], ==, 1518.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[2], ==, 1519.0))
          r = 1;

      } else {
        if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 902))
          r = 1;

        if (!test_compare_dd(LPTX_vector_x(s5->particles[0].base.position), ==,
                             202.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[0].base.position), ==,
                             206.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[0].base.position), ==,
                             210.0))
          r = 1;

        if (!test_compare_ii(s5->particles[0].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[0].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1510.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1511.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1512.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1513.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1514.0))
          r = 1;
      }
    } while (0);

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
      r = 1;
      break;
    }

    if (s5) {
      LPTX_particle_set_delete(s5);
      s5 = NULL;
    }

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_alltoall(&s5, set, 0, 4,
                                                      (int[]){1, -1, 1, 1},
                                                      MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_alltoall(&s5, NULL, 0, 0, NULL,
                                                      MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }

    do {
      if (!test_compare_pp(s5, !=, NULL)) {
        r = 1;
        break;
      }

      if (irank == 0) {
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 0))
          r = 1;
      } else {
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 3))
          r = 1;
      }

      if (!test_compare_ii(LPTX_particle_set_number_of_vectors(s5), ==, 2))
        r = 1;

      if (!test_compare_ii(LPTX_particle_set_number_of_data(s5), ==, 5))
        r = 1;

      if (r)
        break;

      if (irank == 1) {
        if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 900))
          r = 1;
        if (!test_compare_ii(s5->particles[1].base.particle_id, ==, 902))
          r = 1;
        if (!test_compare_ii(s5->particles[2].base.particle_id, ==, 903))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[0].base.position), ==,
                             200.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[1].base.position), ==,
                             202.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[2].base.position), ==,
                             203.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[0].base.position), ==,
                             204.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[1].base.position), ==,
                             206.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[2].base.position), ==,
                             207.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[0].base.position), ==,
                             208.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[1].base.position), ==,
                             210.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[2].base.position), ==,
                             211.0))
          r = 1;
        if (!test_compare_ii(s5->particles[0].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[1].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[2].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[0].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_ii(s5->particles[1].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_ii(s5->particles[2].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1500.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1501.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1502.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1503.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1504.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].v[0], ==, 1510.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].v[1], ==, 1511.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[0], ==, 1512.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[1], ==, 1513.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[2], ==, 1514.0))
          r = 1;
        if (!test_compare_dd(s5->particles[2].vectors[0].v[0], ==, 1515.0))
          r = 1;
        if (!test_compare_dd(s5->particles[2].vectors[0].v[1], ==, 1516.0))
          r = 1;
        if (!test_compare_dd(s5->particles[2].vectors[1].v[0], ==, 1517.0))
          r = 1;
        if (!test_compare_dd(s5->particles[2].vectors[1].v[1], ==, 1518.0))
          r = 1;
        if (!test_compare_dd(s5->particles[2].vectors[1].v[2], ==, 1519.0))
          r = 1;
      }
    } while (0);

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
      r = 1;
      break;
    }

    if (s5) {
      LPTX_particle_set_delete(s5);
      s5 = NULL;
    }

    if (irank == 0) {
      if (!test_compare_ii(LPTX_particle_set_alltoall(&s5, set, 0, 4,
                                                      (int[]){0, -1, 0, 0},
                                                      MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    } else {
      if (!test_compare_ii(LPTX_particle_set_alltoall(&s5, NULL, 0, 0, NULL,
                                                      MPI_COMM_WORLD),
                           ==, MPI_SUCCESS))
        r = 1;
    }

    do {
      if (!test_compare_pp(s5, !=, NULL)) {
        r = 1;
        break;
      }

      if (irank == 0) {
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 3))
          r = 1;
      } else {
        if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, 0))
          r = 1;
      }

      if (!test_compare_ii(LPTX_particle_set_number_of_vectors(s5), ==, 2))
        r = 1;

      if (!test_compare_ii(LPTX_particle_set_number_of_data(s5), ==, 5))
        r = 1;

      if (r)
        break;

      if (irank == 0) {
        if (!test_compare_ii(s5->particles[0].base.particle_id, ==, 900))
          r = 1;
        if (!test_compare_ii(s5->particles[1].base.particle_id, ==, 902))
          r = 1;
        if (!test_compare_ii(s5->particles[2].base.particle_id, ==, 903))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[0].base.position), ==,
                             200.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[1].base.position), ==,
                             202.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[2].base.position), ==,
                             203.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[0].base.position), ==,
                             204.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[1].base.position), ==,
                             206.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[2].base.position), ==,
                             207.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[0].base.position), ==,
                             208.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[1].base.position), ==,
                             210.0))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[2].base.position), ==,
                             211.0))
          r = 1;
        if (!test_compare_ii(s5->particles[0].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[1].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[2].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[0].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_ii(s5->particles[1].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_ii(s5->particles[2].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[0], ==, 1500.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[0].v[1], ==, 1501.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[0], ==, 1502.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[1], ==, 1503.0))
          r = 1;
        if (!test_compare_dd(s5->particles[0].vectors[1].v[2], ==, 1504.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].v[0], ==, 1510.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[0].v[1], ==, 1511.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[0], ==, 1512.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[1], ==, 1513.0))
          r = 1;
        if (!test_compare_dd(s5->particles[1].vectors[1].v[2], ==, 1514.0))
          r = 1;
        if (!test_compare_dd(s5->particles[2].vectors[0].v[0], ==, 1515.0))
          r = 1;
        if (!test_compare_dd(s5->particles[2].vectors[0].v[1], ==, 1516.0))
          r = 1;
        if (!test_compare_dd(s5->particles[2].vectors[1].v[0], ==, 1517.0))
          r = 1;
        if (!test_compare_dd(s5->particles[2].vectors[1].v[1], ==, 1518.0))
          r = 1;
        if (!test_compare_dd(s5->particles[2].vectors[1].v[2], ==, 1519.0))
          r = 1;
      }
    } while (0);

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
      r = 1;
      break;
    }
  } while (0);

  /* alltoall test for nproc > 2 */
  do {
    LPTX_idtype ptsum;
    int ns, nr;
    int *ldests;
    LPTX_idtype *lrecvs;
    LPTX_idtype npdigs;
    LPTX_idtype npoffs;

    if (r)
      break;

    npdigs = 1;
    npoffs = nproc;
    for (; npoffs > 10; npoffs /= 10)
      npdigs++;

    npoffs = 1;
    for (; npdigs > 0; --npdigs)
      npoffs *= 10;

    do {
      jupiter_random_seed seed = {
        .seed = {UINT64_C(0x853d7caa65de091b), UINT64_C(0x96a6525f60a56c08),
                 UINT64_C(0xd1a943a123402478), UINT64_C(0xd3779192f413906b)},
      };

      sends = malloc(sizeof(int) * nproc);
      if (!sends) {
        fprintf(stderr, "ERROR Memory allocation failed\n");
        r = 1;
        break;
      }

      recvs = malloc(sizeof(int) * nproc);
      if (!recvs) {
        fprintf(stderr, "ERROR Memory allocation failed\n");
        r = 1;
        break;
      }

      if (irank != 0)
        break;

      /* random example: 2, 4, 1, 3, 4, 4, 2, 4, ... */
      for (int ir = 0; ir < nproc; ++ir) {
        sends[ir] = jupiter_random_nextn(&seed, 8);
        if (sends[ir] > 4)
          sends[ir] = 4;
        fprintf(stderr, "..... Sends %d pts from rank %d\n", sends[ir], ir);
      }

      ptsum = 0;
      for (int ir = 0; ir < nproc; ++ir) {
        if (LPTX_s_add_overflow(ptsum, sends[ir], &ptsum)) {
          fprintf(stderr, "ERROR Arithmetic overflow\n");
          r = 1;
          break;
        }
      }
      if (r)
        break;

      dests = malloc(sizeof(int) * ptsum);
      if (!dests) {
        fprintf(stderr, "ERROR Memory allocation failed\n");
        r = 1;
        break;
      }

      recvids = malloc(sizeof(LPTX_idtype) * ptsum);
      if (!recvids) {
        fprintf(stderr, "ERROR Memory allocation failed\n");
        r = 1;
        break;
      }

      /*
       * nproc == 1: [0 -1]
       * nproc == 2: [-1, 0], [1, 0, -1, 1]
       * nproc == 3: [0, 1], [1, 0, 2, 1], [0]
       * nproc == 4: [2, 2], [0, 3, 1, 0], [1], [2, 2, 2]
       * nproc == 6: [1, 5], [2, 1, 2, 4], [3], [3, 3, 2], [5, 2, 2, 0],
       *             [-1, 4, -1, -1]
       * nproc == 8: [3, 2], [3, 6, 5, 4], [5], [3, 7, 4], [3, 0, -1, 5],
       *             [-1, -1, 0, 1], [-1, -1], [3, 5, 7, 3]
       */
      for (LPTX_idtype ir = 0, is = 0, ii = 0; ir < ptsum; ++ir) {
        dests[ir] = (int32_t)jupiter_random_nextn(&seed, nproc + 1) - 1;
        fprintf(stderr,
                "..... Sends pt [%" PRIdMAX "] in rank %" PRIdMAX
                " to rank %d\n",
                (intmax_t)ii, (intmax_t)is, dests[ir]);
        ++ii;
        while (is < nproc && ii >= sends[is]) {
          ++is;
          ii = 0;
        }
      }

      for (int ir = 0; ir < nproc; ++ir)
        recvs[ir] = 0;
      for (LPTX_idtype ir = 0; ir < ptsum; ++ir) {
        int it = dests[ir];
        if (it >= 0)
          recvs[it] += 1;
      }

      /*
       * nproc == 1: 1
       * nproc == 2: 2, 2
       * nproc == 3: 3, 3, 1
       * nproc == 4: 2, 2, 5, 1
       * nproc == 6: 1, 2, 5, 3, 2, 2
       * nproc == 8: 2, 1, 1, 6, 2, 4, 1, 2
       */
      for (int ir = 0; ir < nproc; ++ir)
        fprintf(stderr, "..... Rank %d receives %d particles\n", ir, recvs[ir]);

      /*
       * nproc == 1 ... rank 0: 10
       * nrpoc == 2 ... rank 0: 20 21, rank 1: 11 41
       * nproc == 3 ... rank 0: 10 21 12, rank 1: 20 11 41, rank 2: 31
       */
      for (LPTX_idtype jj = 0, ir = 0, ii = 0; jj < ptsum && ir < nproc; ++jj) {
        LPTX_idtype id, ic, sr, si;
        ic = 0, sr = 0, si = 0;
        for (id = 0; id < ptsum; ++id) {
          if (dests[id] == ir) {
            if (ic == ii)
              break;
            ++ic;
          }
          ++si;
          while (sr < nproc && si >= sends[sr]) {
            ++sr;
            si = 0;
          }
        }
        recvids[jj] = (si + 1) * npoffs + sr;
        fprintf(stderr,
                "..... Recvs pt [%" PRIdMAX "] in rank %" PRIdMAX
                " should have ID %" PRIdMAX "\n",
                ii, ir, recvids[jj]);
        ++ii;
        while (ir < nproc && ii >= recvs[ir]) {
          ++ir;
          ii = 0;
        }
      }
    } while (0);

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_ii(MPI_Bcast(sends, nproc, MPI_INT, 0, MPI_COMM_WORLD),
                         ==, MPI_SUCCESS)) {
      r = 1;
      break;
    }

    if (!test_compare_ii(MPI_Bcast(recvs, nproc, MPI_INT, 0, MPI_COMM_WORLD),
                         ==, MPI_SUCCESS)) {
      r = 1;
      break;
    }

    if (!test_compare_ii(MPI_Bcast(&ptsum, 1, LPTX_MPI_TYPE_ID, 0,
                                   MPI_COMM_WORLD),
                         ==, MPI_SUCCESS)) {
      r = 1;
      break;
    }

    do {
      if (irank == 0)
        break;

      dests = malloc(sizeof(int) * ptsum);
      if (!dests) {
        fprintf(stderr, "ERROR Memory allocation failed\n");
        r = 1;
        break;
      }

      recvids = malloc(sizeof(LPTX_idtype) * ptsum);
      if (!recvids) {
        fprintf(stderr, "ERROR Memory allocation failed\n");
        r = 1;
        break;
      }
    } while (0);

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
      r = 1;
      break;
    }

    if (!test_compare_ii(MPI_Bcast(dests, ptsum, MPI_INT, 0, MPI_COMM_WORLD),
                         ==, MPI_SUCCESS)) {
      r = 1;
      break;
    }

    if (!test_compare_ii(MPI_Bcast(recvids, ptsum, LPTX_MPI_TYPE_ID, 0,
                                   MPI_COMM_WORLD),
                         ==, MPI_SUCCESS)) {
      r = 1;
      break;
    }

    do {
      ns = sends[irank];
      nr = recvs[irank];
      ldests = dests;
      lrecvs = recvids;

      for (int ir = 0; ir < irank; ++ir) {
        ldests += sends[ir];
        lrecvs += recvs[ir];
      }

      for (int i = 0; i < ns; ++i) {
        set->particles[i].base.particle_id = npoffs * (i + 1) + irank;
        set->particles[i].base.velocity =
          LPTX_vector_c(npoffs * (i * 3 + 1) + irank,
                        npoffs * (i * 3 + 2) + irank,
                        npoffs * (i * 3 + 3) + irank);
        *(LPTX_type *)&set->particles[i].vectors[0].v[0] =
          npoffs * (i * 5 + 1) + irank;
        *(LPTX_type *)&set->particles[i].vectors[0].v[1] =
          npoffs * (i * 5 + 2) + irank;
        *(LPTX_type *)&set->particles[i].vectors[1].v[0] =
          npoffs * (i * 5 + 3) + irank;
        *(LPTX_type *)&set->particles[i].vectors[1].v[1] =
          npoffs * (i * 5 + 4) + irank;
        *(LPTX_type *)&set->particles[i].vectors[1].v[2] =
          npoffs * (i * 5 + 5) + irank;
      }
    } while (0);

    if (s5) {
      LPTX_particle_set_delete(s5);
      s5 = NULL;
    }

    if (!test_compare_ii(LPTX_particle_set_alltoall(&s5, set, 0, ns, ldests,
                                                    MPI_COMM_WORLD),
                         ==, MPI_SUCCESS)) {
      r = 1;
      break;
    }

    do {
      if (!test_compare_pp(s5, !=, NULL)) {
        r = 1;
        break;
      }

      if (!test_compare_ii(LPTX_particle_set_number_of_particles(s5), ==, nr))
        r = 1;

      if (!test_compare_ii(LPTX_particle_set_number_of_vectors(s5), ==, 2))
        r = 1;

      if (!test_compare_ii(LPTX_particle_set_number_of_data(s5), ==, 5))
        r = 1;

      if (r)
        break;

      for (int ir = 0; ir < nr; ++ir) {
        LPTX_idtype rid = lrecvs[ir];
        int rir = rid % npoffs;
        int idx = rid / npoffs - 1;
        if (!test_compare_ii(s5->particles[ir].base.particle_id, ==, rid))
          r = 1;
        if (!test_compare_dd(LPTX_vector_x(s5->particles[ir].base.velocity), ==,
                             (LPTX_type)(idx * 3 + 1) * npoffs + rir))
          r = 1;
        if (!test_compare_dd(LPTX_vector_y(s5->particles[ir].base.velocity), ==,
                             (LPTX_type)(idx * 3 + 2) * npoffs + rir))
          r = 1;
        if (!test_compare_dd(LPTX_vector_z(s5->particles[ir].base.velocity), ==,
                             (LPTX_type)(idx * 3 + 3) * npoffs + rir))
          r = 1;

        if (!test_compare_ii(s5->particles[ir].number_of_vectors, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[ir].vectors[0].length, ==, 2))
          r = 1;
        if (!test_compare_ii(s5->particles[ir].vectors[1].length, ==, 3))
          r = 1;
        if (!test_compare_dd(s5->particles[ir].vectors[0].v[0], ==,
                             (LPTX_type)(idx * 5 + 1) * npoffs + rir))
          r = 1;
        if (!test_compare_dd(s5->particles[ir].vectors[0].v[1], ==,
                             (LPTX_type)(idx * 5 + 2) * npoffs + rir))
          r = 1;
        if (!test_compare_dd(s5->particles[ir].vectors[1].v[0], ==,
                             (LPTX_type)(idx * 5 + 3) * npoffs + rir))
          r = 1;
        if (!test_compare_dd(s5->particles[ir].vectors[1].v[1], ==,
                             (LPTX_type)(idx * 5 + 4) * npoffs + rir))
          r = 1;
        if (!test_compare_dd(s5->particles[ir].vectors[1].v[2], ==,
                             (LPTX_type)(idx * 5 + 5) * npoffs + rir))
          r = 1;
      }

    } while (0);

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
      r = 1;
      break;
    }
  } while (0);

  do {
    if (r)
      break;

    if (param)
      LPTX_param_delete(param);
    if (!test_compare_pp((param = LPTX_param_new()), !=, NULL))
      r = 1;

    if (!test_compare_ii(LPTX_param_set_mpi_comm(param, MPI_COMM_WORLD), ==,
                         MPI_SUCCESS))
      r = 1;

    if (set)
      LPTX_particle_set_delete(set);
    set = NULL;
    if (irank == 0) {
      if (!test_compare_pp((set = LPTX_particle_set_new(4, 2,
                                                        (LPTX_idtype[]){2, 3})),
                           !=, NULL))
        r = 1;

      set->particles[0].base.particle_id = 900;
      set->particles[1].base.particle_id = 901;
      set->particles[2].base.particle_id = 902;
      set->particles[3].base.particle_id = 903;
      set->particles[0].base.position = LPTX_vector_c(200.0, 204.0, 208.0);
      set->particles[1].base.position = LPTX_vector_c(201.0, 205.0, 209.0);
      set->particles[2].base.position = LPTX_vector_c(202.0, 206.0, 210.0);
      set->particles[3].base.position = LPTX_vector_c(203.0, 207.0, 211.0);
      LPTX_particle_set_used(&set->particles[0].base, LPTX_true);
      LPTX_particle_set_used(&set->particles[1].base, LPTX_false);
      LPTX_particle_set_used(&set->particles[2].base, LPTX_true);
      LPTX_particle_set_used(&set->particles[3].base, LPTX_true);
      *(LPTX_type *)&set->particles[0].vectors[0].v[0] = 1500.0;
      *(LPTX_type *)&set->particles[0].vectors[0].v[1] = 1501.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[0] = 1502.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[1] = 1503.0;
      *(LPTX_type *)&set->particles[0].vectors[1].v[2] = 1504.0;
      *(LPTX_type *)&set->particles[1].vectors[0].v[0] = 1505.0;
      *(LPTX_type *)&set->particles[1].vectors[0].v[1] = 1506.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[0] = 1507.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[1] = 1508.0;
      *(LPTX_type *)&set->particles[1].vectors[1].v[2] = 1509.0;
      *(LPTX_type *)&set->particles[2].vectors[0].v[0] = 1510.0;
      *(LPTX_type *)&set->particles[2].vectors[0].v[1] = 1511.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[0] = 1512.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[1] = 1513.0;
      *(LPTX_type *)&set->particles[2].vectors[1].v[2] = 1514.0;
      *(LPTX_type *)&set->particles[3].vectors[0].v[0] = 1515.0;
      *(LPTX_type *)&set->particles[3].vectors[0].v[1] = 1516.0;
      *(LPTX_type *)&set->particles[3].vectors[1].v[0] = 1517.0;
      *(LPTX_type *)&set->particles[3].vectors[1].v[1] = 1518.0;
      *(LPTX_type *)&set->particles[3].vectors[1].v[2] = 1519.0;

      LPTX_particle_set_append(param, set);
      set = NULL;
    }

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
      r = 1;
      break;
    }

    switch (irank) {
    case 0:
      if (!test_compare_ii(LPTX_param_get_total_particles(param), ==, 4))
        r = 1;
      break;
    case 1:
      if (!test_compare_ii(LPTX_param_get_total_particles(param), ==, 0))
        r = 1;
      break;
    default:
      if (!test_compare_ii(LPTX_param_get_total_particles(param), ==, 0))
        r = 1;
      break;
    }

    if (nproc > 1) {
      if (!test_compare_ii(LPTX_param_redistribute_particles(param, mpi_rank_s,
                                                             NULL),
                           ==, 0))
        r = 1;

      switch (irank) {
      case 0:
        if (!test_compare_ii(LPTX_param_get_total_particles(param), ==, 2))
          r = 1;
        if (!r) {
          LPTX_idtype i[3];
          if (!test_compare_ii(LPTX_get_particle_particle_id(param, 0, i, 3),
                               ==, 2))
            r = 1;
          if (r)
            break;
          if (!test_compare_ii(i[0], ==, 900))
            r = 1;
          if (!test_compare_ii(i[1], ==, 903))
            r = 1;
        }
        break;
      case 1:
        if (!test_compare_ii(LPTX_param_get_total_particles(param), ==, 1))
          r = 1;
        if (!r) {
          LPTX_idtype i[3];
          if (!test_compare_ii(LPTX_get_particle_particle_id(param, 0, i, 3),
                               ==, 1))
            r = 1;
          if (r)
            break;
          if (!test_compare_ii(i[0], ==, 902))
            r = 1;
        }
        break;
      default:
        if (!test_compare_ii(LPTX_param_get_total_particles(param), ==, 0))
          r = 1;
        break;
      }

      if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
        r = 1;
        break;
      }
    }

    if (!test_compare_ii(LPTX_param_redistribute_particles(param, mpi_rank_r,
                                                           NULL),
                         ==, 0))
      r = 1;

    switch (irank) {
    case 0:
      if (!test_compare_ii(LPTX_param_get_total_particles(param), ==, 3))
        r = 1;
      if (!r) {
        LPTX_idtype i[3];
        if (!test_compare_ii(LPTX_get_particle_particle_id(param, 0, i, 3), ==,
                             3))
          r = 1;
        if (r)
          break;
        if (!test_compare_ii(i[0], ==, 900))
          r = 1;
        if (nproc > 1) {
          if (!test_compare_ii(i[1], ==, 903))
            r = 1;
          if (!test_compare_ii(i[2], ==, 902))
            r = 1;
        } else {
          if (!test_compare_ii(i[1], ==, 902))
            r = 1;
          if (!test_compare_ii(i[2], ==, 903))
            r = 1;
        }
      }
      break;
    case 1:
      if (!test_compare_ii(LPTX_param_get_total_particles(param), ==, 0))
        r = 1;
      break;
    default:
      if (!test_compare_ii(LPTX_param_get_total_particles(param), ==, 0))
        r = 1;
      break;
    }

    if (!LPTX_MPI_forall(!r, MPI_COMM_WORLD, NULL)) {
      r = 1;
      break;
    }
  } while (0);
#endif

  if (set)
    LPTX_particle_set_delete(set);
  if (s1)
    LPTX_particle_set_delete(s1);
  if (s2)
    LPTX_particle_set_delete(s2);
  if (s3)
    LPTX_particle_set_delete(s3);
  if (s4)
    LPTX_particle_set_delete(s4);
  if (s5)
    LPTX_particle_set_delete(s5);
  if (param)
    LPTX_param_delete(param);
  free(sends);
  free(dests);
  free(recvs);
  free(recvids);

  return r;
}
