
#include "control_test.h"
#include "jupiter/control/data_array.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/mpi_controller.h"
#include "jupiter/control/reduce_op.h"
#include "jupiter/control/overflow.h"
#include "jupiter/control/shared_object.h"
#include "jupiter/control/shared_object_priv.h"
#include "jupiter/control/static_array.h"
#include "jupiter/control/subarray.h"
#include "test-util.h"

#include <stdlib.h>
#include <string.h>

#define test_alloc(expr) \
  ((test_compare_pp((expr), !=, NULL)) ? 1 : (ret = 1, 0))

struct reduce_op_tester
{
  jcntrl_reduce_op op;
  int ret;
};
#define reduce_op_tester__ancestor jcntrl_reduce_op
#define reduce_op_tester__dnmem op.jcntrl_reduce_op__dnmem
JCNTRL_VTABLE_NONE(reduce_op_tester);

JCNTRL_SHARED_METADATA_INIT_DECL(reduce_op_tester);

static struct reduce_op_tester *
reduce_op_tester_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL_T(reduce_op_tester, struct reduce_op_tester, obj);
}

static void *reduce_op_tester_downcast_v(jcntrl_shared_object *obj)
{
  return reduce_op_tester_downcast_impl(obj);
}

static int reduce_op_tester_initializer(jcntrl_shared_object *obj)
{
  struct reduce_op_tester *t = reduce_op_tester_downcast_impl(obj);
  t->ret = 1;
  return 1;
}

static int test_ret = 0;
static void reduce_op_tester_calc_impl(jcntrl_data_array *inpvec,
                                       jcntrl_data_array *outvec,
                                       jcntrl_shared_object *obj)
{
  struct reduce_op_tester *p;
  p = jcntrl_shared_object_downcast_by_meta(reduce_op_tester_metadata_init(),
                                            obj);
  if (!test_compare_pp(p, !=, NULL)) {
    test_ret = 0;
    return;
  }

  p->ret = 0;
  if (!test_compare_ii(jcntrl_data_array_get_ntuple(inpvec), ==, 10))
    test_ret = 1;
  if (!test_compare_ii(jcntrl_data_array_get_ntuple(outvec), ==, 10))
    test_ret = 1;
}

JCNTRL_VIRTUAL_WRAP(reduce_op_tester, jcntrl_reduce_op, calc)

static void reduce_op_tester_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = reduce_op_tester_downcast_v;
  p->initializer = reduce_op_tester_initializer;
  JCNTRL_VIRTUAL_WRAP_SET(p, reduce_op_tester, jcntrl_reduce_op, calc);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(reduce_op_tester, reduce_op_tester_init_func)

int test_control_mpi_controller(void)
{
  int ret;
  const jcntrl_mpi_controller *world_comm;
  const jcntrl_mpi_controller *self_comm;
  jcntrl_mpi_controller *comm;
  jcntrl_double_array *d1, *d2, *d3;
  jcntrl_int_array *i1, *i2;
  jcntrl_size_array *s1, *s2;
  jcntrl_data_array **dstidx;
  const int nproc = test_util_mpi_nproc();
  const int rank = test_util_mpi_rank();

  ret = 0;
  comm = NULL;
  d1 = NULL;
  d2 = NULL;
  d3 = NULL;
  i1 = NULL;
  i2 = NULL;
  s1 = NULL;
  s2 = NULL;
  dstidx = NULL;
  world_comm = NULL;
  self_comm = NULL;

  do {
    if (!test_compare_pp(jcntrl_reduce_op_sum(), !=, NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_reduce_op_prod(), !=, NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_reduce_op_min(), !=, NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_reduce_op_max(), !=, NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_reduce_op_band(), !=, NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_reduce_op_bor(), !=, NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_reduce_op_bxor(), !=, NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_reduce_op_land(), !=, NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_reduce_op_lor(), !=, NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_reduce_op_lxor(), !=, NULL))
      ret = 1;

    if (!test_compare_pp((world_comm = jcntrl_mpi_controller_world()), !=,
                         NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((self_comm = jcntrl_mpi_controller_self()), !=,
                         NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_mpi_controller_nproc(world_comm), ==,
                         test_util_mpi_nproc()))
      ret = 1;

    if (!test_compare_ii(jcntrl_mpi_controller_rank(world_comm), ==,
                         test_util_mpi_rank()))
      ret = 1;

    if (!test_compare_ii(jcntrl_mpi_controller_any(world_comm, 1), ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_mpi_controller_any(world_comm, 0), ==, 0))
      ret = 1;

    if (!test_compare_ii(jcntrl_mpi_controller_any(world_comm, rank == 0), ==,
                         1))
      ret = 1;

    if (!test_compare_ii(jcntrl_mpi_controller_any(world_comm, rank == 1), ==,
                         (nproc > 1) ? 1 : 0))
      ret = 1;

    if (!test_compare_ii(jcntrl_mpi_controller_all(world_comm, 1), ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_mpi_controller_all(world_comm, 0), ==, 0))
      ret = 1;

    if (!test_compare_ii(jcntrl_mpi_controller_all(world_comm, rank == 0), ==,
                         (nproc > 1) ? 0 : 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_mpi_controller_all(world_comm, rank == 1), ==,
                         0))
      ret = 1;

    if (!test_compare_ii(jcntrl_mpi_controller_nproc(self_comm), ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_mpi_controller_rank(self_comm), ==, 0))
      ret = 1;

  } while (0);

  if (!world_comm)
    return 1;
  if (jcntrl_mpi_controller_any(world_comm, ret))
    return 1;

  do {
    if (!test_compare_ii(nproc, >, 0)) {
      ret = 1;
      break;
    }

    if (!test_alloc(d1 = jcntrl_double_array_new()))
      break;
    if (!test_alloc(d2 = jcntrl_double_array_new()))
      break;
    if (!test_alloc(i1 = jcntrl_int_array_new()))
      break;
    if (!test_alloc(i2 = jcntrl_int_array_new()))
      break;
    if (!test_alloc(s1 = jcntrl_size_array_new()))
      break;
    if (!test_alloc(s2 = jcntrl_size_array_new()))
      break;

    if (!test_compare_pp(jcntrl_double_array_resize(d1, 10), ==, d1)) {
      ret = 1;
      break;
    }
    if (!test_compare_pp(jcntrl_double_array_resize(d2, nproc * 10), ==, d2)) {
      ret = 1;
      break;
    }
    if (!test_compare_pp(jcntrl_int_array_resize(i1, 10), ==, i1)) {
      ret = 1;
      break;
    }
    if (!test_compare_pp(jcntrl_int_array_resize(i2, nproc * 10), ==, i2)) {
      ret = 1;
      break;
    }
    if (!test_compare_pp(jcntrl_size_array_resize(s1, 10), ==, s1)) {
      ret = 1;
      break;
    }
    if (!test_compare_pp(jcntrl_size_array_resize(s2, nproc * 10), ==, s2)) {
      ret = 1;
      break;
    }

    if (!test_alloc(comm = jcntrl_mpi_controller_new()))
      break;
  } while (0);

  do {
    if (jcntrl_mpi_controller_any(world_comm, ret))
      break;

#ifdef JCNTRL_USE_MPI
    if (!test_compare_ii(jcntrl_mpi_controller_set_comm(comm, MPI_COMM_WORLD),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_mpi_controller_set_comm(comm, MPI_COMM_SELF),
                         ==, 1))
      ret = 1;

    {
      int r;
      int compres;
      MPI_Comm mpicomm = jcntrl_mpi_controller_get_comm(comm);
      if (test_compare_c(mpicomm != MPI_COMM_SELF))
        ret = 1;

      compres = 999;
      r = MPI_Comm_compare(mpicomm, MPI_COMM_SELF, &compres);
      if (!test_compare_ii(r, ==, MPI_SUCCESS))
        ret = 1;

      if (!(test_compare_ii(compres, ==, MPI_CONGRUENT) ||
            test_compare_ii(compres, ==, MPI_IDENT)))
        ret = 1;
    }
#endif

    jcntrl_mpi_controller_delete(comm);
    comm = NULL;

    if (!test_compare_pp((comm = jcntrl_mpi_controller_split(world_comm,
                                                             rank % 2, rank)),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_mpi_controller_rank(comm), ==, rank / 2))
      ret = 1;
    if (!test_compare_ii(jcntrl_mpi_controller_nproc(comm), ==,
                         nproc / 2 +
                           ((nproc % 2 == 1 && rank % 2 == 0) ? 1 : 0)))
      ret = 1;

    jcntrl_mpi_controller_delete(comm);
    comm = NULL;
  } while (0);

  do {
    if (jcntrl_mpi_controller_any(world_comm, ret))
      break;

    /* This test require exactly 2 process */
    if (nproc < 2 || rank >= 2)
      break;

    if (rank == 0) {
      jcntrl_data_array *a1;
      double *p;

      p = NULL;
      if (!test_compare_pp((p = jcntrl_double_array_get_writable(d1)), !=,
                           NULL))
        ret = 1;

      p[0] = 1.139414310e-01;
      p[1] = 5.827307701e-02;
      p[2] = 1.476147771e-01;
      p[3] = 1.798378229e-01;
      p[4] = 3.925675154e-01;
      p[5] = 5.001095533e-01;
      p[6] = 2.073279023e-01;
      p[7] = 9.228192568e-01;
      p[8] = 9.640464187e-01;
      p[9] = 5.610931516e-01;

      if (!test_compare_pp((a1 = jcntrl_double_array_data(d1)), !=, NULL))
        ret = 1;

      if (!test_compare_ii(jcntrl_mpi_controller_send(world_comm, a1, 5, 1, 1),
                           ==, 1))
        ret = 1;
    } else {
      jcntrl_data_array *a2;
      const double *p;
      a2 = jcntrl_double_array_data(d2);

      if (!test_compare_ii(jcntrl_mpi_controller_recv(world_comm, a2, 5, 0, 1),
                           ==, 1))
        ret = 1;

      if (!test_compare_pp((p = jcntrl_double_array_get(d2)), !=, NULL))
        ret = 1;

      if (!test_compare_dd(p[0], ==, 1.139414310e-01))
        ret = 1;
      if (!test_compare_dd(p[1], ==, 5.827307701e-02))
        ret = 1;
      if (!test_compare_dd(p[2], ==, 1.476147771e-01))
        ret = 1;
      if (!test_compare_dd(p[3], ==, 1.798378229e-01))
        ret = 1;
      if (!test_compare_dd(p[4], ==, 3.925675154e-01))
        ret = 1;
      if (!test_compare_dd(p[5], ==, 0.0))
        ret = 1;
      if (!test_compare_dd(p[6], ==, 0.0))
        ret = 1;
      if (!test_compare_dd(p[7], ==, 0.0))
        ret = 1;
      if (!test_compare_dd(p[8], ==, 0.0))
        ret = 1;
    }
  } while (0);

  do {
    if (jcntrl_mpi_controller_any(world_comm, ret))
      break;

    /* This test require exactly 2 process */
    if (nproc < 2 || rank >= 2)
      break;

    if (rank == 0) {
      jcntrl_data_array *a1;
      double *p;

      p = NULL;
      if (!test_compare_pp((p = jcntrl_double_array_get_writable(d1)), !=,
                           NULL))
        ret = 1;

      p[0] = 5.098438263e-01;
      p[1] = 3.762477636e-02;
      p[2] = 8.553281426e-01;
      p[3] = 1.278406382e-01;
      p[4] = 5.660974979e-01;
      p[5] = 2.497373819e-01;
      p[6] = 2.664548159e-02;
      p[7] = 7.875890732e-01;
      p[8] = 3.411549330e-02;
      p[9] = 3.208561540e-01;

      if (!test_compare_pp((a1 = jcntrl_double_array_data(d1)), !=, NULL))
        ret = 1;

      if (!test_compare_ii(jcntrl_mpi_controller_senda(world_comm, a1, 1, 2),
                           ==, 1))
        ret = 1;
    } else {
      jcntrl_data_array *a2;
      const double *p;
      a2 = jcntrl_double_array_data(d2);

      if (!test_compare_ii(jcntrl_mpi_controller_recva(world_comm, a2, 0, 2),
                           ==, 1))
        ret = 1;

      if (!test_compare_ii(jcntrl_double_array_get_ntuple(d2), ==, 10))
        ret = 1;

      if (!test_compare_pp((p = jcntrl_double_array_get(d2)), !=, NULL))
        ret = 1;

      if (!test_compare_dd(p[0], ==, 5.098438263e-01))
        ret = 1;
      if (!test_compare_dd(p[1], ==, 3.762477636e-02))
        ret = 1;
      if (!test_compare_dd(p[2], ==, 8.553281426e-01))
        ret = 1;
      if (!test_compare_dd(p[3], ==, 1.278406382e-01))
        ret = 1;
      if (!test_compare_dd(p[4], ==, 5.660974979e-01))
        ret = 1;
      if (!test_compare_dd(p[5], ==, 2.497373819e-01))
        ret = 1;
      if (!test_compare_dd(p[6], ==, 2.664548159e-02))
        ret = 1;
      if (!test_compare_dd(p[7], ==, 7.875890732e-01))
        ret = 1;
      if (!test_compare_dd(p[8], ==, 3.411549330e-02))
        ret = 1;
      if (!test_compare_dd(p[9], ==, 3.208561540e-01))
        ret = 1;
    }
  } while (0);

  do {
    jcntrl_data_array *d1, *d2;
    int *pi;
    const int *po;

    if (jcntrl_mpi_controller_any(world_comm, ret))
      break;

    pi = NULL;
    po = NULL;

    if (!test_compare_pp((pi = jcntrl_int_array_get_writable(i1)), !=, NULL))
      ret = 1;
    if (rank == 0) {
      pi[0] = 11;
      pi[1] = 74;
      pi[2] = 103;
      pi[3] = 4;
      pi[4] = 24;
      pi[5] = 25;
      pi[6] = 58;
      pi[7] = 40;
      pi[8] = 16;
      pi[9] = 22;
    } else {
      pi[0] = 0;
      pi[1] = 0;
      pi[2] = 0;
      pi[3] = 0;
      pi[4] = 0;
      pi[5] = 0;
      pi[6] = 0;
      pi[7] = 0;
      pi[8] = 0;
      pi[9] = 0;
    }

    if (!test_compare_pp((d1 = jcntrl_int_array_data(i1)), !=, NULL))
      ret = 1;
    if (!test_compare_ii(jcntrl_data_array_get_ntuple(d1), >, 2))
      ret = 1;
    if (jcntrl_mpi_controller_any(world_comm, ret))
      break;

    if (!test_compare_ii(jcntrl_mpi_controller_bcast(world_comm, d1, 2, 0), ==,
                         1))
      ret = 1;

    if (!test_compare_pp((po = jcntrl_int_array_get(i1)), !=, NULL))
      ret = 1;
    if (rank != 0) {
      if (!test_compare_ii(po[0], ==, 11))
        ret = 1;
      if (!test_compare_ii(po[1], ==, 74))
        ret = 1;
      if (!test_compare_ii(po[2], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[3], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[4], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[5], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[6], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[7], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[8], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[9], ==, 0))
        ret = 1;
    }

    if (rank != 0) {
      if (!test_compare_pp(jcntrl_int_array_resize(i1, 0), !=, NULL))
        ret = 1;
    }
    if (jcntrl_mpi_controller_any(world_comm, ret))
      break;

    if (!test_compare_ii(jcntrl_mpi_controller_bcasta(world_comm, d1, 0), ==,
                         1))
      ret = 1;

    if (!test_compare_pp((po = jcntrl_int_array_get(i1)), !=, NULL))
      ret = 1;
    if (!test_compare_ii(jcntrl_int_array_get_ntuple(i1), ==, 10))
      ret = 1;
    if (rank != 0) {
      if (!test_compare_ii(po[0], ==, 11))
        ret = 1;
      if (!test_compare_ii(po[1], ==, 74))
        ret = 1;
      if (!test_compare_ii(po[2], ==, 103))
        ret = 1;
      if (!test_compare_ii(po[3], ==, 4))
        ret = 1;
      if (!test_compare_ii(po[4], ==, 24))
        ret = 1;
      if (!test_compare_ii(po[5], ==, 25))
        ret = 1;
      if (!test_compare_ii(po[6], ==, 58))
        ret = 1;
      if (!test_compare_ii(po[7], ==, 40))
        ret = 1;
      if (!test_compare_ii(po[8], ==, 16))
        ret = 1;
      if (!test_compare_ii(po[9], ==, 22))
        ret = 1;
    }
  } while (0);

  do {
    jcntrl_data_array *a1, *a2;
    int *pi;
    const int *po;

    if (jcntrl_mpi_controller_any(world_comm, ret))
      break;

    pi = NULL;
    po = NULL;

    if (!test_compare_pp((pi = jcntrl_int_array_get_writable(i1)), !=, NULL))
      ret = 1;

    if (rank % 4 == 0) {
      pi[0] = 138;
      pi[1] = 88;
      pi[2] = 215;
      pi[3] = 44;
      pi[4] = 182;
      pi[5] = 17;
      pi[6] = 238;
      pi[7] = 200;
      pi[8] = 198;
      pi[9] = 79;
    } else if (rank % 4 == 1) {
      pi[0] = 39;
      pi[1] = 81;
      pi[2] = 161;
      pi[3] = 7;
      pi[4] = 91;
      pi[5] = 254;
      pi[6] = 209;
      pi[7] = 235;
      pi[8] = 62;
      pi[9] = 186;
    } else if (rank % 4 == 2) {
      pi[0] = 81;
      pi[1] = 138;
      pi[2] = 160;
      pi[3] = 255;
      pi[4] = 117;
      pi[5] = 105;
      pi[6] = 166;
      pi[7] = 237;
      pi[8] = 128;
      pi[9] = 139;
    } else if (rank % 4 == 3) {
      pi[0] = 17;
      pi[1] = 228;
      pi[2] = 32;
      pi[3] = 188;
      pi[4] = 38;
      pi[5] = 158;
      pi[6] = 163;
      pi[7] = 171;
      pi[8] = 163;
      pi[9] = 194;
    }

    a1 = NULL;
    a2 = NULL;

    if (!test_compare_pp((a1 = jcntrl_int_array_data(i1)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((a2 = jcntrl_int_array_data(i2)), !=, NULL))
      ret = 1;
    if (!test_compare_ii(jcntrl_mpi_controller_gather(world_comm, a2, a1, 5, 0),
                         ==, 1))
      ret = 1;

    if (!test_compare_pp((po = jcntrl_int_array_get(i2)), !=, NULL))
      ret = 1;

    if (rank == 0) {
      for (int i = 0; i < nproc; i += 4) {
        int n0 = i;
        int n1 = i + 1;
        int n2 = i + 2;
        int n3 = i + 3;
        if (!test_compare_ii(po[5 * n0 + 0], ==, 138))
          ret = 1;
        if (!test_compare_ii(po[5 * n0 + 1], ==, 88))
          ret = 1;
        if (!test_compare_ii(po[5 * n0 + 2], ==, 215))
          ret = 1;
        if (!test_compare_ii(po[5 * n0 + 3], ==, 44))
          ret = 1;
        if (!test_compare_ii(po[5 * n0 + 4], ==, 182))
          ret = 1;
        if (n1 < nproc) {
          if (!test_compare_ii(po[5 * n1 + 0], ==, 39))
            ret = 1;
          if (!test_compare_ii(po[5 * n1 + 1], ==, 81))
            ret = 1;
          if (!test_compare_ii(po[5 * n1 + 2], ==, 161))
            ret = 1;
          if (!test_compare_ii(po[5 * n1 + 3], ==, 7))
            ret = 1;
          if (!test_compare_ii(po[5 * n1 + 4], ==, 91))
            ret = 1;
        }
        if (n2 < nproc) {
          if (!test_compare_ii(po[5 * n2 + 0], ==, 81))
            ret = 1;
          if (!test_compare_ii(po[5 * n2 + 1], ==, 138))
            ret = 1;
          if (!test_compare_ii(po[5 * n2 + 2], ==, 160))
            ret = 1;
          if (!test_compare_ii(po[5 * n2 + 3], ==, 255))
            ret = 1;
          if (!test_compare_ii(po[5 * n2 + 4], ==, 117))
            ret = 1;
        }
        if (n3 < nproc) {
          if (!test_compare_ii(po[5 * n3 + 0], ==, 17))
            ret = 1;
          if (!test_compare_ii(po[5 * n3 + 1], ==, 228))
            ret = 1;
          if (!test_compare_ii(po[5 * n3 + 2], ==, 32))
            ret = 1;
          if (!test_compare_ii(po[5 * n3 + 3], ==, 188))
            ret = 1;
          if (!test_compare_ii(po[5 * n3 + 4], ==, 38))
            ret = 1;
        }
      }
    }
  } while (0);

  do {
    jcntrl_data_array *a1, *a2;
    int *pi;
    const int *po;

    if (jcntrl_mpi_controller_any(world_comm, ret))
      break;

    pi = NULL;
    po = NULL;

    if (!test_compare_pp((pi = jcntrl_int_array_get_writable(i1)), !=, NULL))
      ret = 1;

    if (rank % 4 == 0) {
      pi[0] = 138;
      pi[1] = 88;
      pi[2] = 215;
      pi[3] = 44;
      pi[4] = 182;
      pi[5] = 17;
      pi[6] = 238;
      pi[7] = 200;
      pi[8] = 198;
      pi[9] = 79;
    } else if (rank % 4 == 1) {
      pi[0] = 39;
      pi[1] = 81;
      pi[2] = 161;
      pi[3] = 7;
      pi[4] = 91;
      pi[5] = 254;
      pi[6] = 209;
      pi[7] = 235;
      pi[8] = 62;
      pi[9] = 186;
    } else if (rank % 4 == 2) {
      pi[0] = 81;
      pi[1] = 138;
      pi[2] = 160;
      pi[3] = 255;
      pi[4] = 117;
      pi[5] = 105;
      pi[6] = 166;
      pi[7] = 237;
      pi[8] = 128;
      pi[9] = 139;
    } else if (rank % 4 == 3) {
      pi[0] = 17;
      pi[1] = 228;
      pi[2] = 32;
      pi[3] = 188;
      pi[4] = 38;
      pi[5] = 158;
      pi[6] = 163;
      pi[7] = 171;
      pi[8] = 163;
      pi[9] = 194;
    }

    a1 = NULL;
    a2 = NULL;

    if (!test_compare_pp((a1 = jcntrl_int_array_data(i1)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((a2 = jcntrl_int_array_data(i2)), !=, NULL))
      ret = 1;
    if (!test_compare_ii(jcntrl_mpi_controller_allgather(world_comm, a2, a1, 5),
                         ==, 1))
      ret = 1;

    if (!test_compare_pp((po = jcntrl_int_array_get(i2)), !=, NULL))
      ret = 1;

    for (int i = 0; i < nproc; i += 4) {
      int n0 = i;
      int n1 = i + 1;
      int n2 = i + 2;
      int n3 = i + 3;
      if (!test_compare_ii(po[5 * n0 + 0], ==, 138))
        ret = 1;
      if (!test_compare_ii(po[5 * n0 + 1], ==, 88))
        ret = 1;
      if (!test_compare_ii(po[5 * n0 + 2], ==, 215))
        ret = 1;
      if (!test_compare_ii(po[5 * n0 + 3], ==, 44))
        ret = 1;
      if (!test_compare_ii(po[5 * n0 + 4], ==, 182))
        ret = 1;
      if (n1 < nproc) {
        if (!test_compare_ii(po[5 * n1 + 0], ==, 39))
          ret = 1;
        if (!test_compare_ii(po[5 * n1 + 1], ==, 81))
          ret = 1;
        if (!test_compare_ii(po[5 * n1 + 2], ==, 161))
          ret = 1;
        if (!test_compare_ii(po[5 * n1 + 3], ==, 7))
          ret = 1;
        if (!test_compare_ii(po[5 * n1 + 4], ==, 91))
          ret = 1;
      }
      if (n2 < nproc) {
        if (!test_compare_ii(po[5 * n2 + 0], ==, 81))
          ret = 1;
        if (!test_compare_ii(po[5 * n2 + 1], ==, 138))
          ret = 1;
        if (!test_compare_ii(po[5 * n2 + 2], ==, 160))
          ret = 1;
        if (!test_compare_ii(po[5 * n2 + 3], ==, 255))
          ret = 1;
        if (!test_compare_ii(po[5 * n2 + 4], ==, 117))
          ret = 1;
      }
      if (n3 < nproc) {
        if (!test_compare_ii(po[5 * n3 + 0], ==, 17))
          ret = 1;
        if (!test_compare_ii(po[5 * n3 + 1], ==, 228))
          ret = 1;
        if (!test_compare_ii(po[5 * n3 + 2], ==, 32))
          ret = 1;
        if (!test_compare_ii(po[5 * n3 + 3], ==, 188))
          ret = 1;
        if (!test_compare_ii(po[5 * n3 + 4], ==, 38))
          ret = 1;
      }
    }
  } while (0);

  do {
    int *pi;
    const int *po;
    jcntrl_data_array *a1, *a2, *as1;
    jcntrl_size_type *si, *so;
    jcntrl_size_type ns, nr;
    jcntrl_size_type idxsidx;
    jcntrl_static_size_array stb;

    const jcntrl_size_type nidxs[] = {
      /* for nproc == 1 */
      5,

      /* for nproc == 2 */
      3,
      9,

      /* for nproc == 3 */
      0,
      7,
      1,

      /* for nproc >= 4 */
      10,
      8,
      8,
      3,
    };
    const jcntrl_size_type *nidxsp;

    jcntrl_size_type sidxs[][10] = {
      /* for nproc == 1 */
      {3, 2, 0, 1, 6},

      /* for nproc == 2 */
      {1, 2, 8},                   /* from rank 0 */
      {5, 1, 3, 8, 6, 4, 2, 7, 0}, /* from rank 1 */

      /* for nproc == 3 */
      {0},                   /* from rank 0 (dummy) */
      {7, 9, 0, 1, 3, 6, 8}, /* from rank 1 */
      {6},                   /* from rank 2 */

      /* for nproc >= 4 */
      {2, 4, 9, 3, 6, 7, 1, 8, 0, 5}, /* from rank 0 */
      {8, 9, 7, 0, 2, 6, 4, 1},       /* from rank 1 */
      {1, 6, 4, 2, 9, 8, 5, 3},       /* from rank 2 */
      {8, 4, 9},                      /* from rank 3 */
    };

    jcntrl_size_type ridxs[][10] = {
      /* for nproc == 1 */
      {1, 0, 4, 9, 8},

      /* for nproc == 2 */
      {11, 18, 19},                    /* from rank 0 */
      {6, 4, 10, 13, 0, 15, 9, 5, 17}, /* from rank 1 */

      /* for nproc == 3 */
      {0},                        /* from rank 0 (dummy) */
      {25, 28, 18, 24, 7, 1, 26}, /* from rank 1 */
      {15},                       /* from rank 2 */

      /* for nproc >= 4 (40-based) */
      {19, 18, 7, 38, 22, 6, 10, 9, 3, 36}, /* from rank 0 */
      {12, 4, 17, 11, 28, 37, 14, 15},      /* from rank 1 */
      {21, 33, 25, 39, 35, 0, 31, 1},       /* from rank 2 */
      {34, 16, 32},                         /* from rank 3 */
    };

    if (jcntrl_mpi_controller_any(world_comm, ret))
      break;

    if (!test_compare_pp(jcntrl_int_array_resize(i1, 10), ==, i1))
      ret = 1;

    if (!test_compare_pp(jcntrl_int_array_resize(i2, 0), ==, i2))
      ret = 1;

    if (!test_compare_pp(jcntrl_int_array_resize(i2,
                                                 ((nproc - 1) / 4 + 1) * 40),
                         ==, i2))
      ret = 1;

    idxsidx = (nproc > 4) ? 4 : nproc;
    idxsidx = idxsidx * (idxsidx - 1) / 2;
    nidxsp = &nidxs[idxsidx];

    ns = nidxsp[rank % 4];
    nr = 0;
    for (int i = 0; i < nproc; ++i) {
      if (jcntrl_s_add_overflow(nr, nidxsp[i % 4], &nr)) {
        ret = 1;
        break;
      }
    }

    if (!test_compare_pp(jcntrl_size_array_resize(s1, ns), ==, s1))
      ret = 1;

    if (!test_compare_pp(jcntrl_size_array_resize(s2, nr), ==, s2))
      ret = 1;

    if (!test_compare_pp((dstidx = calloc(nproc, sizeof(jcntrl_data_array *))),
                         !=, NULL))
      ret = 1;

    if (!test_compare_pp((as1 = jcntrl_size_array_data(s2)), !=, NULL))
      ret = 1;

    if (dstidx && as1) {
      jcntrl_size_type ib = 0;
      jcntrl_size_type nb;

      for (int i = 0; i < nproc; ++i, ib += nb) {
        jcntrl_data_subarray *p;
        nb = nidxsp[i % 4];
        if (nb <= 0)
          continue;

        if (!test_compare_pp((p = jcntrl_data_subarray_new(as1, ib, nb)), !=,
                             NULL)) {
          ret = 1;
          break;
        }

        if (!test_compare_pp((dstidx[i] = jcntrl_data_subarray_data(p)), !=,
                             NULL)) {
          jcntrl_data_subarray_delete(p);
          ret = 1;
          break;
        }
      }
    }

    pi = NULL;
    po = NULL;

    if (!test_compare_pp((pi = jcntrl_int_array_get_writable(i1)), !=, NULL))
      ret = 1;

    if (!test_compare_pp((po = jcntrl_int_array_get(i2)), !=, NULL))
      ret = 1;

    if (jcntrl_mpi_controller_any(world_comm, ret))
      break;

    if (rank % 4 == 0) {
      pi[0] = 555;
      pi[1] = 352;
      pi[2] = 861;
      pi[3] = 177;
      pi[4] = 728;
      pi[5] = 70;
      pi[6] = 952;
      pi[7] = 801;
      pi[8] = 793;
      pi[9] = 318;
    } else if (rank % 4 == 1) {
      pi[0] = 156;
      pi[1] = 327;
      pi[2] = 644;
      pi[3] = 31;
      pi[4] = 365;
      pi[5] = 1016;
      pi[6] = 838;
      pi[7] = 941;
      pi[8] = 250;
      pi[9] = 747;
    } else if (rank % 4 == 2) {
      pi[0] = 324;
      pi[1] = 552;
      pi[2] = 643;
      pi[3] = 1020;
      pi[4] = 471;
      pi[5] = 423;
      pi[6] = 667;
      pi[7] = 951;
      pi[8] = 515;
      pi[9] = 556;
    } else {
      pi[0] = 68;
      pi[1] = 914;
      pi[2] = 130;
      pi[3] = 752;
      pi[4] = 154;
      pi[5] = 632;
      pi[6] = 652;
      pi[7] = 684;
      pi[8] = 654;
      pi[9] = 779;
    }

    jcntrl_static_size_array_init_base(&stb, sidxs[idxsidx + rank % 4], ns);

    as1 = NULL;
    if (!test_compare_pp((as1 = jcntrl_static_size_array_data(&stb)), !=, NULL))
      ret = 1;

    if (!test_compare_ii(jcntrl_size_array_copy(s1, as1, ns, 0, 0), ==, 1))
      ret = 1;

    for (int i = 0; i < nproc; ++i) {
      if (dstidx[i]) {
        jcntrl_size_type nb;

        nb = jcntrl_data_array_get_ntuple(dstidx[i]);
        jcntrl_static_size_array_init_base(&stb, ridxs[idxsidx + i % 4], nb);

        if (!test_compare_ii(jcntrl_data_array_copy(dstidx[i], as1, nb, 0, 0),
                             ==, 1))
          ret = 1;

        if (!test_compare_pp((so = jcntrl_data_array_get_writable_sizes(
                                dstidx[i])),
                             !=, NULL))
          ret = 1;

        if (so) {
          jcntrl_size_type offset;
          if (!test_compare_ii((offset = (i / 4) * 40) % 40, ==, 0))
            ret = 1;

          for (jcntrl_size_type jj = 0; jj < nb; ++jj)
            so[jj] += offset;
        }
      }
    }

    if (!test_compare_pp((as1 = jcntrl_size_array_data(s1)), !=, NULL))
      ret = 1;

    if (!test_compare_pp((a1 = jcntrl_int_array_data(i1)), !=, NULL))
      ret = 1;

    if (!test_compare_pp((a2 = jcntrl_int_array_data(i2)), !=, NULL))
      ret = 1;

    if (jcntrl_mpi_controller_any(world_comm, ret))
      break;

    if (!test_compare_ii(jcntrl_mpi_controller_gathervi(world_comm, a2, dstidx,
                                                        a1, as1, 0),
                         ==, 1))
      ret = 1;

    if (rank == 0) {
      switch (nproc) {
      case 1:
        /* (3, 2, 0, 1, 6) -> (1, 0, 4, 9, 8) */
        if (!test_compare_ii(po[0], ==, 861))
          ret = 1;
        if (!test_compare_ii(po[1], ==, 177))
          ret = 1;
        if (!test_compare_ii(po[2], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[3], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[4], ==, 555))
          ret = 1;
        if (!test_compare_ii(po[5], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[6], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[7], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[8], ==, 952))
          ret = 1;
        if (!test_compare_ii(po[9], ==, 352))
          ret = 1;
        break;
      case 2:
        /*
         * (1, 2, 8) -> (11, 18, 19)
         * (5, 1, 3, 8, 6, 4, 2, 7, 0) -> (6, 4, 10, 13, 0, 15, 9, 5, 17)
         */
        if (!test_compare_ii(po[0], ==, 838))
          ret = 1;
        if (!test_compare_ii(po[1], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[2], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[3], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[4], ==, 327))
          ret = 1;
        if (!test_compare_ii(po[5], ==, 941))
          ret = 1;
        if (!test_compare_ii(po[6], ==, 1016))
          ret = 1;
        if (!test_compare_ii(po[7], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[8], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[9], ==, 644))
          ret = 1;
        if (!test_compare_ii(po[10], ==, 31))
          ret = 1;
        if (!test_compare_ii(po[11], ==, 352))
          ret = 1;
        if (!test_compare_ii(po[12], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[13], ==, 250))
          ret = 1;
        if (!test_compare_ii(po[14], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[15], ==, 365))
          ret = 1;
        if (!test_compare_ii(po[16], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[17], ==, 156))
          ret = 1;
        if (!test_compare_ii(po[18], ==, 861))
          ret = 1;
        if (!test_compare_ii(po[19], ==, 793))
          ret = 1;
        break;
      case 3:
        /*
         * () -> ()
         * (7, 9, 0, 1, 3, 6, 8) -> (25, 28, 18, 24, 7, 1, 26)
         * (6) -> (15)
         */
        if (!test_compare_ii(po[0], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[1], ==, 838))
          ret = 1;
        if (!test_compare_ii(po[2], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[3], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[4], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[5], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[6], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[7], ==, 31))
          ret = 1;
        if (!test_compare_ii(po[8], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[9], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[10], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[11], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[12], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[13], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[14], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[15], ==, 667))
          ret = 1;
        if (!test_compare_ii(po[16], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[17], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[18], ==, 156))
          ret = 1;
        if (!test_compare_ii(po[19], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[20], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[21], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[22], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[23], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[24], ==, 327))
          ret = 1;
        if (!test_compare_ii(po[25], ==, 941))
          ret = 1;
        if (!test_compare_ii(po[26], ==, 250))
          ret = 1;
        if (!test_compare_ii(po[27], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[28], ==, 747))
          ret = 1;
        if (!test_compare_ii(po[29], ==, 0))
          ret = 1;
        break;
      default:
        /*
         * (2, 4, 9, 3, 6, 7, 1, 8, 0, 5) ->
         *   (19, 18, 7, 38, 22, 6, 10, 9, 3, 36)
         * (8, 9, 7, 0, 2, 6, 4, 1) -> (12, 4, 17, 11, 28, 37, 14, 15)
         * (1, 6, 4, 2, 9, 8, 5, 3) -> (21, 33, 25, 39, 35, 0, 31, 1)
         * (8, 4, 9) -> (34, 16, 32)
         */
        for (int i = 0; i < nproc; i += 4) {
          jcntrl_size_type ib = i * 10;
#define N0(x) x
#define N1(x) ((i + 1 >= nproc) ? 0 : x)
#define N2(x) ((i + 2 >= nproc) ? 0 : x)
#define N3(x) ((i + 3 >= nproc) ? 0 : x)
          if (!test_compare_ii(po[ib + 0], ==, N2(515)))
            ret = 1;
          if (!test_compare_ii(po[ib + 1], ==, N2(1020)))
            ret = 1;
          if (!test_compare_ii(po[ib + 2], ==, 0))
            ret = 1;
          if (!test_compare_ii(po[ib + 3], ==, N0(555)))
            ret = 1;
          if (!test_compare_ii(po[ib + 4], ==, N1(747)))
            ret = 1;
          if (!test_compare_ii(po[ib + 5], ==, 0))
            ret = 1;
          if (!test_compare_ii(po[ib + 6], ==, N0(801)))
            ret = 1;
          if (!test_compare_ii(po[ib + 7], ==, N0(318)))
            ret = 1;
          if (!test_compare_ii(po[ib + 8], ==, 0))
            ret = 1;
          if (!test_compare_ii(po[ib + 9], ==, N0(793)))
            ret = 1;
          if (!test_compare_ii(po[ib + 10], ==, N0(352)))
            ret = 1;
          if (!test_compare_ii(po[ib + 11], ==, N1(156)))
            ret = 1;
          if (!test_compare_ii(po[ib + 12], ==, N1(250)))
            ret = 1;
          if (!test_compare_ii(po[ib + 13], ==, 0))
            ret = 1;
          if (!test_compare_ii(po[ib + 14], ==, N1(365)))
            ret = 1;
          if (!test_compare_ii(po[ib + 15], ==, N1(327)))
            ret = 1;
          if (!test_compare_ii(po[ib + 16], ==, N3(154)))
            ret = 1;
          if (!test_compare_ii(po[ib + 17], ==, N1(941)))
            ret = 1;
          if (!test_compare_ii(po[ib + 18], ==, N0(728)))
            ret = 1;
          if (!test_compare_ii(po[ib + 19], ==, N0(861)))
            ret = 1;
          if (!test_compare_ii(po[ib + 20], ==, 0))
            ret = 1;
          if (!test_compare_ii(po[ib + 21], ==, N2(552)))
            ret = 1;
          if (!test_compare_ii(po[ib + 22], ==, N0(952)))
            ret = 1;
          if (!test_compare_ii(po[ib + 23], ==, 0))
            ret = 1;
          if (!test_compare_ii(po[ib + 24], ==, 0))
            ret = 1;
          if (!test_compare_ii(po[ib + 25], ==, N2(471)))
            ret = 1;
          if (!test_compare_ii(po[ib + 26], ==, 0))
            ret = 1;
          if (!test_compare_ii(po[ib + 27], ==, 0))
            ret = 1;
          if (!test_compare_ii(po[ib + 28], ==, N1(644)))
            ret = 1;
          if (!test_compare_ii(po[ib + 29], ==, 0))
            ret = 1;
          if (!test_compare_ii(po[ib + 30], ==, 0))
            ret = 1;
          if (!test_compare_ii(po[ib + 31], ==, N2(423)))
            ret = 1;
          if (!test_compare_ii(po[ib + 32], ==, N3(779)))
            ret = 1;
          if (!test_compare_ii(po[ib + 33], ==, N2(667)))
            ret = 1;
          if (!test_compare_ii(po[ib + 34], ==, N3(654)))
            ret = 1;
          if (!test_compare_ii(po[ib + 35], ==, N2(556)))
            ret = 1;
          if (!test_compare_ii(po[ib + 36], ==, N0(70)))
            ret = 1;
          if (!test_compare_ii(po[ib + 37], ==, N1(838)))
            ret = 1;
          if (!test_compare_ii(po[ib + 38], ==, N0(177)))
            ret = 1;
          if (!test_compare_ii(po[ib + 39], ==, N2(643)))
            ret = 1;
#undef N0
#undef N1
#undef N2
#undef N3
        }
        break;
      }
    }
  } while (0);
  if (dstidx) {
    for (int i = 0; i < nproc; ++i) {
      if (dstidx[i])
        jcntrl_data_array_delete(dstidx[i]);
    }
    free(dstidx);
    dstidx = NULL;
  }

  do {
    int *pi;
    const int *po;
    jcntrl_data_array *a1, *a2, *as1;
    jcntrl_size_type *si, *so;
    jcntrl_size_type ns, nr;
    jcntrl_size_type idxsidx;
    jcntrl_static_size_array stb;

    const jcntrl_size_type nidxs[] = {
      /* for nproc == 1 */
      6,

      /* for nproc == 2 */
      4,
      9,

      /* for nproc == 3 */
      5,
      8,
      9,

      /* for nproc >= 4 */
      10,
      0,
      6,
      3,
    };
    const jcntrl_size_type *nidxsp;

    jcntrl_size_type sidxs[][10] = {
      /* for nproc == 1 */
      {9, 2, 5, 4, 8, 0},

      /* for nproc == 2 */
      {8, 3, 1, 2},                /* from rank 0 */
      {5, 4, 0, 2, 3, 1, 8, 9, 7}, /* from rank 1 */

      /* for nproc == 3 */
      {6, 0, 3, 2, 8},             /* from rank 0 */
      {0, 6, 7, 5, 2, 8, 9, 4},    /* from rank 1 */
      {4, 9, 8, 0, 5, 3, 1, 2, 7}, /* from rank 2 */

      /* for nproc >= 4 */
      {6, 7, 2, 9, 5, 3, 1, 8, 4, 0}, /* from rank 0 */
      {0},                            /* from rank 1 (dummy) */
      {5, 6, 8, 3, 0, 7},             /* from rank 2 */
      {3, 0, 6},                      /* from rank 3 */
    };

    jcntrl_size_type ridxs[][10] = {
      /* for nproc == 1 */
      {8, 2, 1, 7, 5, 4},

      /* for nproc == 2 */
      {10, 8, 14, 2},                   /* from rank 0 */
      {19, 17, 6, 9, 7, 15, 4, 13, 18}, /* from rank 1 */

      /* for nproc == 3 */
      {7, 24, 16, 11, 9},               /* from rank 0 */
      {5, 1, 25, 20, 19, 21, 13, 29},   /* from rank 1 */
      {12, 2, 0, 10, 14, 26, 27, 8, 6}, /* from rank 2 */

      /* for nproc >= 4 (40-based) */
      {15, 7, 2, 30, 28, 4, 8, 23, 27, 11}, /* from rank 0 */
      {0},                                  /* from rank 1 (dummy) */
      {3, 34, 25, 36, 29, 13},              /* from rank 2 */
      {26, 31, 33},                         /* from rank 3 */
    };

    if (jcntrl_mpi_controller_any(world_comm, ret))
      break;

    if (!test_compare_pp(jcntrl_int_array_resize(i1, 10), ==, i1))
      ret = 1;

    if (!test_compare_pp(jcntrl_int_array_resize(i2, 0), ==, i2))
      ret = 1;

    if (!test_compare_pp(jcntrl_int_array_resize(i2,
                                                 ((nproc - 1) / 4 + 1) * 40),
                         ==, i2))
      ret = 1;

    idxsidx = (nproc > 4) ? 4 : nproc;
    idxsidx = idxsidx * (idxsidx - 1) / 2;
    nidxsp = &nidxs[idxsidx];

    ns = nidxsp[rank % 4];
    nr = 0;
    for (int i = 0; i < nproc; ++i) {
      if (jcntrl_s_add_overflow(nr, nidxsp[i % 4], &nr)) {
        ret = 1;
        break;
      }
    }

    if (!test_compare_pp(jcntrl_size_array_resize(s1, ns), ==, s1))
      ret = 1;

    if (!test_compare_pp(jcntrl_size_array_resize(s2, nr), ==, s2))
      ret = 1;

    if (!test_compare_pp((dstidx = calloc(nproc, sizeof(jcntrl_data_array *))),
                         !=, NULL))
      ret = 1;

    if (!test_compare_pp((as1 = jcntrl_size_array_data(s2)), !=, NULL))
      ret = 1;

    if (dstidx && as1) {
      jcntrl_size_type ib = 0;
      jcntrl_size_type nb;

      for (int i = 0; i < nproc; ++i, ib += nb) {
        jcntrl_data_subarray *p;
        nb = nidxsp[i % 4];
        if (nb <= 0)
          continue;

        if (!test_compare_pp((p = jcntrl_data_subarray_new(as1, ib, nb)), !=,
                             NULL)) {
          ret = 1;
          break;
        }

        if (!test_compare_pp((dstidx[i] = jcntrl_data_subarray_data(p)), !=,
                             NULL)) {
          jcntrl_data_subarray_delete(p);
          ret = 1;
          break;
        }
      }
    }

    pi = NULL;
    po = NULL;

    if (!test_compare_pp((pi = jcntrl_int_array_get_writable(i1)), !=, NULL))
      ret = 1;

    if (!test_compare_pp((po = jcntrl_int_array_get(i2)), !=, NULL))
      ret = 1;

    if (jcntrl_mpi_controller_any(world_comm, ret))
      break;

    if (rank % 4 == 0) {
      pi[0] = 858;
      pi[1] = 313;
      pi[2] = 1594;
      pi[3] = 1053;
      pi[4] = 116;
      pi[5] = 1469;
      pi[6] = 826;
      pi[7] = 185;
      pi[8] = 107;
      pi[9] = 360;
    } else if (rank % 4 == 1) {
      pi[0] = 181;
      pi[1] = 1574;
      pi[2] = 836;
      pi[3] = 347;
      pi[4] = 1208;
      pi[5] = 1587;
      pi[6] = 1899;
      pi[7] = 295;
      pi[8] = 1013;
      pi[9] = 359;
    } else if (rank % 4 == 2) {
      pi[0] = 1232;
      pi[1] = 1701;
      pi[2] = 164;
      pi[3] = 268;
      pi[4] = 132;
      pi[5] = 41;
      pi[6] = 1618;
      pi[7] = 1381;
      pi[8] = 1873;
      pi[9] = 1110;
    } else {
      pi[0] = 460;
      pi[1] = 1354;
      pi[2] = 795;
      pi[3] = 1595;
      pi[4] = 558;
      pi[5] = 1100;
      pi[6] = 831;
      pi[7] = 713;
      pi[8] = 632;
      pi[9] = 1504;
    }

    jcntrl_static_size_array_init_base(&stb, sidxs[idxsidx + rank % 4], ns);

    as1 = NULL;
    if (!test_compare_pp((as1 = jcntrl_static_size_array_data(&stb)), !=, NULL))
      ret = 1;

    if (!test_compare_ii(jcntrl_size_array_copy(s1, as1, ns, 0, 0), ==, 1))
      ret = 1;

    for (int i = 0; i < nproc; ++i) {
      if (dstidx[i]) {
        jcntrl_size_type nb;

        nb = jcntrl_data_array_get_ntuple(dstidx[i]);
        jcntrl_static_size_array_init_base(&stb, ridxs[idxsidx + i % 4], nb);

        if (!test_compare_ii(jcntrl_data_array_copy(dstidx[i], as1, nb, 0, 0),
                             ==, 1))
          ret = 1;

        if (!test_compare_pp((so = jcntrl_data_array_get_writable_sizes(
                                dstidx[i])),
                             !=, NULL))
          ret = 1;

        if (so) {
          jcntrl_size_type offset;
          if (!test_compare_ii((offset = (i / 4) * 40) % 40, ==, 0))
            ret = 1;

          for (jcntrl_size_type jj = 0; jj < nb; ++jj)
            so[jj] += offset;
        }
      }
    }

    if (!test_compare_pp((as1 = jcntrl_size_array_data(s1)), !=, NULL))
      ret = 1;

    if (!test_compare_pp((a1 = jcntrl_int_array_data(i1)), !=, NULL))
      ret = 1;

    if (!test_compare_pp((a2 = jcntrl_int_array_data(i2)), !=, NULL))
      ret = 1;

    if (jcntrl_mpi_controller_any(world_comm, ret))
      break;

    if (!test_compare_ii(jcntrl_mpi_controller_allgathervi(world_comm, a2,
                                                           dstidx, a1, as1),
                         ==, 1))
      ret = 1;

    switch (nproc) {
    case 1:
      /* {9, 2, 5, 4, 8, 0} -> {8, 2, 1, 7, 5, 4} */
      if (!test_compare_ii(po[0], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[1], ==, 1469))
        ret = 1;
      if (!test_compare_ii(po[2], ==, 1594))
        ret = 1;
      if (!test_compare_ii(po[3], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[4], ==, 858))
        ret = 1;
      if (!test_compare_ii(po[5], ==, 107))
        ret = 1;
      if (!test_compare_ii(po[6], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[7], ==, 116))
        ret = 1;
      if (!test_compare_ii(po[8], ==, 360))
        ret = 1;
      if (!test_compare_ii(po[9], ==, 0))
        ret = 1;
      break;
    case 2:
      /*
       * {8, 3, 1, 2} -> {10, 8, 14, 2}
       * {5, 4, 0, 2, 3, 1, 8, 9, 7} -> {19, 17, 6, 9, 7, 15, 4, 13, 18}
       */
      if (!test_compare_ii(po[0], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[1], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[2], ==, 1594))
        ret = 1;
      if (!test_compare_ii(po[3], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[4], ==, 1013))
        ret = 1;
      if (!test_compare_ii(po[5], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[6], ==, 181))
        ret = 1;
      if (!test_compare_ii(po[7], ==, 347))
        ret = 1;
      if (!test_compare_ii(po[8], ==, 1053))
        ret = 1;
      if (!test_compare_ii(po[9], ==, 836))
        ret = 1;
      if (!test_compare_ii(po[10], ==, 107))
        ret = 1;
      if (!test_compare_ii(po[11], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[12], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[13], ==, 359))
        ret = 1;
      if (!test_compare_ii(po[14], ==, 313))
        ret = 1;
      if (!test_compare_ii(po[15], ==, 1574))
        ret = 1;
      if (!test_compare_ii(po[16], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[17], ==, 1208))
        ret = 1;
      if (!test_compare_ii(po[18], ==, 295))
        ret = 1;
      if (!test_compare_ii(po[19], ==, 1587))
        ret = 1;
      break;
    case 3:
      /*
       * {6, 0, 3, 2, 8} -> {7, 24, 16, 11, 9}
       * {0, 6, 7, 5, 2, 8, 9, 4} -> {5, 1, 25, 20, 19, 21, 13, 29}
       * {4, 9, 8, 0, 5, 3, 1, 2, 7} -> {12, 2, 0, 10, 14, 26, 27, 8, 6}
       */
      if (!test_compare_ii(po[0], ==, 1873))
        ret = 1;
      if (!test_compare_ii(po[1], ==, 1899))
        ret = 1;
      if (!test_compare_ii(po[2], ==, 1110))
        ret = 1;
      if (!test_compare_ii(po[3], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[4], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[5], ==, 181))
        ret = 1;
      if (!test_compare_ii(po[6], ==, 1381))
        ret = 1;
      if (!test_compare_ii(po[7], ==, 826))
        ret = 1;
      if (!test_compare_ii(po[8], ==, 164))
        ret = 1;
      if (!test_compare_ii(po[9], ==, 107))
        ret = 1;
      if (!test_compare_ii(po[10], ==, 1232))
        ret = 1;
      if (!test_compare_ii(po[11], ==, 1594))
        ret = 1;
      if (!test_compare_ii(po[12], ==, 132))
        ret = 1;
      if (!test_compare_ii(po[13], ==, 359))
        ret = 1;
      if (!test_compare_ii(po[14], ==, 41))
        ret = 1;
      if (!test_compare_ii(po[15], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[16], ==, 1053))
        ret = 1;
      if (!test_compare_ii(po[17], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[18], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[19], ==, 836))
        ret = 1;
      if (!test_compare_ii(po[20], ==, 1587))
        ret = 1;
      if (!test_compare_ii(po[21], ==, 1013))
        ret = 1;
      if (!test_compare_ii(po[22], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[23], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[24], ==, 858))
        ret = 1;
      if (!test_compare_ii(po[25], ==, 295))
        ret = 1;
      if (!test_compare_ii(po[26], ==, 268))
        ret = 1;
      if (!test_compare_ii(po[27], ==, 1701))
        ret = 1;
      if (!test_compare_ii(po[28], ==, 0))
        ret = 1;
      if (!test_compare_ii(po[29], ==, 1208))
        ret = 1;
      break;
    default:
      /*
       * {6, 7, 2, 9, 5, 3, 1, 8, 4, 0} -> {15, 7, 2, 30, 28, 4, 8, 23, 27, 11}
       * {} -> {}
       * {5, 6, 8, 3, 0, 7} -> {3, 34, 25, 36, 29, 13}
       * {3, 0, 6} -> {26, 31, 33}
       */
      for (int i = 0; i < nproc; i += 4) {
        jcntrl_size_type ib = i * 10;
#define N0(x) x
#define N1(x) ((i + 1 >= nproc) ? 0 : x)
#define N2(x) ((i + 2 >= nproc) ? 0 : x)
#define N3(x) ((i + 3 >= nproc) ? 0 : x)
        if (!test_compare_ii(po[ib + 0], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 1], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 2], ==, N0(1594)))
          ret = 1;
        if (!test_compare_ii(po[ib + 3], ==, N2(41)))
          ret = 1;
        if (!test_compare_ii(po[ib + 4], ==, N0(1053)))
          ret = 1;
        if (!test_compare_ii(po[ib + 5], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 6], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 7], ==, N0(185)))
          ret = 1;
        if (!test_compare_ii(po[ib + 8], ==, N0(313)))
          ret = 1;
        if (!test_compare_ii(po[ib + 9], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 10], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 11], ==, N0(858)))
          ret = 1;
        if (!test_compare_ii(po[ib + 12], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 13], ==, N2(1381)))
          ret = 1;
        if (!test_compare_ii(po[ib + 14], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 15], ==, N0(826)))
          ret = 1;
        if (!test_compare_ii(po[ib + 16], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 17], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 18], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 19], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 20], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 21], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 22], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 23], ==, N0(107)))
          ret = 1;
        if (!test_compare_ii(po[ib + 24], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 25], ==, N2(1873)))
          ret = 1;
        if (!test_compare_ii(po[ib + 26], ==, N3(1595)))
          ret = 1;
        if (!test_compare_ii(po[ib + 27], ==, N0(116)))
          ret = 1;
        if (!test_compare_ii(po[ib + 28], ==, N0(1469)))
          ret = 1;
        if (!test_compare_ii(po[ib + 29], ==, N2(1232)))
          ret = 1;
        if (!test_compare_ii(po[ib + 30], ==, N0(360)))
          ret = 1;
        if (!test_compare_ii(po[ib + 31], ==, N3(460)))
          ret = 1;
        if (!test_compare_ii(po[ib + 32], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 33], ==, N3(831)))
          ret = 1;
        if (!test_compare_ii(po[ib + 34], ==, N2(1618)))
          ret = 1;
        if (!test_compare_ii(po[ib + 35], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 36], ==, N2(268)))
          ret = 1;
        if (!test_compare_ii(po[ib + 37], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 38], ==, 0))
          ret = 1;
        if (!test_compare_ii(po[ib + 39], ==, 0))
          ret = 1;
#undef N0
#undef N1
#undef N2
#undef N3
      }
      break;
    }
  } while (0);

  do {
    struct reduce_op_tester t;
    int *pi;
    const int *po;
    jcntrl_data_array *a1, *a2;

    if (jcntrl_mpi_controller_any(world_comm, ret)) {
      ret = 1;
      break;
    }

    jcntrl_shared_object_static_init(jcntrl_reduce_op_object(&t.op),
                                     reduce_op_tester_metadata_init());

    pi = NULL;
    a1 = NULL;
    a2 = NULL;

    if (!test_compare_pp(jcntrl_int_array_resize(i1, 10), ==, i1))
      ret = 1;
    if (!test_compare_pp(jcntrl_int_array_resize(i2, 10), ==, i2))
      ret = 1;

    if (!test_compare_pp((pi = jcntrl_int_array_get_writable(i1)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((a1 = jcntrl_int_array_data(i1)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((a2 = jcntrl_int_array_data(i2)), !=, NULL))
      ret = 1;

    if (jcntrl_mpi_controller_any(world_comm, ret)) {
      ret = 1;
      break;
    }

    if (rank % 4 == 0) {
      pi[0] = 7432;
      pi[1] = 7358;
      pi[2] = 5916;
      pi[3] = 9402;
      pi[4] = 1680;
      pi[5] = 201;
      pi[6] = 5893;
      pi[7] = 540;
      pi[8] = 7874;
      pi[9] = 865;
    } else if (rank % 4 == 1) {
      pi[0] = 3864;
      pi[1] = 804;
      pi[2] = 6665;
      pi[3] = 392;
      pi[4] = 8631;
      pi[5] = 9889;
      pi[6] = 4436;
      pi[7] = 1996;
      pi[8] = 6487;
      pi[9] = 6372;
    } else if (rank % 4 == 2) {
      pi[0] = 5382;
      pi[1] = 947;
      pi[2] = 9003;
      pi[3] = 844;
      pi[4] = 4910;
      pi[5] = 5099;
      pi[6] = 4055;
      pi[7] = 253;
      pi[8] = 5281;
      pi[9] = 6552;
    } else if (rank % 4 == 3) {
      pi[0] = 9072;
      pi[1] = 8533;
      pi[2] = 6189;
      pi[3] = 4998;
      pi[4] = 7164;
      pi[5] = 5671;
      pi[6] = 2577;
      pi[7] = 3444;
      pi[8] = 714;
      pi[9] = 9497;
    }

    if (!test_compare_ii(jcntrl_mpi_controller_reduce(world_comm, a2, a1,
                                                      jcntrl_reduce_op_sum(),
                                                      0),
                         ==, 1))
      ret = 1;

    po = NULL;
    if (rank == 0) {
      if (!test_compare_pp((po = jcntrl_int_array_get(i2)), !=, NULL))
        ret = 1;
    }
    if (po) {
#define N(a, b, c, d)                                         \
  (a + b + c + d) * (nproc / 4) + ((nproc % 4 > 0) ? a : 0) + \
    ((nproc % 4 > 1) ? b : 0) + ((nproc % 4 > 2) ? c : 0)

      if (!test_compare_ii(po[0], ==, N(7432, 3864, 5382, 9072)))
        ret = 1;
      if (!test_compare_ii(po[1], ==, N(7358, 804, 947, 8533)))
        ret = 1;
      if (!test_compare_ii(po[2], ==, N(5916, 6665, 9003, 6189)))
        ret = 1;
      if (!test_compare_ii(po[3], ==, N(9402, 392, 844, 4998)))
        ret = 1;
      if (!test_compare_ii(po[4], ==, N(1680, 8631, 4910, 7164)))
        ret = 1;
      if (!test_compare_ii(po[5], ==, N(201, 9889, 5099, 5671)))
        ret = 1;
      if (!test_compare_ii(po[6], ==, N(5893, 4436, 4055, 2577)))
        ret = 1;
      if (!test_compare_ii(po[7], ==, N(540, 1996, 253, 3444)))
        ret = 1;
      if (!test_compare_ii(po[8], ==, N(7874, 6487, 5281, 714)))
        ret = 1;
      if (!test_compare_ii(po[9], ==, N(865, 6372, 6552, 9497)))
        ret = 1;
    }

    if (rank % 4 == 0) {
      pi[0] = 4515;
      pi[1] = 8329;
      pi[2] = 1623;
      pi[3] = 9602;
      pi[4] = 5170;
      pi[5] = 5091;
      pi[6] = 866;
      pi[7] = 8982;
      pi[8] = 4373;
      pi[9] = 8900;
    } else if (rank % 4 == 1) {
      pi[0] = 914;
      pi[1] = 9980;
      pi[2] = 2074;
      pi[3] = 477;
      pi[4] = 4923;
      pi[5] = 4283;
      pi[6] = 2066;
      pi[7] = 2494;
      pi[8] = 9081;
      pi[9] = 9171;
    } else if (rank % 4 == 2) {
      pi[0] = 1735;
      pi[1] = 9510;
      pi[2] = 82;
      pi[3] = 5571;
      pi[4] = 1731;
      pi[5] = 6012;
      pi[6] = 5986;
      pi[7] = 2493;
      pi[8] = 7556;
      pi[9] = 1055;
    } else if (rank % 4 == 3) {
      pi[0] = 1824;
      pi[1] = 7423;
      pi[2] = 8951;
      pi[3] = 4040;
      pi[4] = 4409;
      pi[5] = 5553;
      pi[6] = 9880;
      pi[7] = 6426;
      pi[8] = 8971;
      pi[9] = 5182;
    }

    if (!test_compare_ii(jcntrl_mpi_controller_allreduce(
                           world_comm, a2, a1, jcntrl_reduce_op_sum()),
                         ==, 1))
      ret = 1;

    po = NULL;
    if (!test_compare_pp((po = jcntrl_int_array_get(i2)), !=, NULL))
      ret = 1;
    if (po) {
      if (!test_compare_ii(po[0], ==, N(4515, 914, 1735, 1824)))
        ret = 1;
      if (!test_compare_ii(po[1], ==, N(8329, 9980, 9510, 7423)))
        ret = 1;
      if (!test_compare_ii(po[2], ==, N(1623, 2074, 82, 8951)))
        ret = 1;
      if (!test_compare_ii(po[3], ==, N(9602, 477, 5571, 4040)))
        ret = 1;
      if (!test_compare_ii(po[4], ==, N(5170, 4923, 1731, 4409)))
        ret = 1;
      if (!test_compare_ii(po[5], ==, N(5091, 4283, 6012, 5553)))
        ret = 1;
      if (!test_compare_ii(po[6], ==, N(866, 2066, 5986, 9880)))
        ret = 1;
      if (!test_compare_ii(po[7], ==, N(8982, 2494, 2493, 6426)))
        ret = 1;
      if (!test_compare_ii(po[8], ==, N(4373, 9081, 7556, 8971)))
        ret = 1;
      if (!test_compare_ii(po[9], ==, N(8900, 9171, 1055, 5182)))
        ret = 1;
    }

    if (!test_compare_ii(jcntrl_mpi_controller_allreduce(
                           world_comm, a2, a1, &t.op),
                         ==, 1))
      ret = 1;
    if (nproc > 1) {
      /* Ranks where the reduce function is called are implementation-defined */
      if (!test_compare_ii(jcntrl_mpi_controller_any(world_comm, t.ret == 0),
                           ==, 1))
        ret = 1;
    }
  } while (0);

  if (d1)
    jcntrl_double_array_delete(d1);
  if (d2)
    jcntrl_double_array_delete(d2);
  if (d3)
    jcntrl_double_array_delete(d3);
  if (i1)
    jcntrl_int_array_delete(i1);
  if (i2)
    jcntrl_int_array_delete(i2);
  if (s1)
    jcntrl_size_array_delete(s1);
  if (s2)
    jcntrl_size_array_delete(s2);
  if (dstidx) {
    for (int i = 0; i < nproc; ++i) {
      if (dstidx[i])
        jcntrl_data_array_delete(dstidx[i]);
    }
    free(dstidx);
  }
  if (comm)
    jcntrl_mpi_controller_delete(comm);

  return ret || test_ret;
}
