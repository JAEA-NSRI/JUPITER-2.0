
#include "jupiter/lptx/defs.h"
#include "jupiter/lptx/param.h"
#include "jupiter/lptx/particle.h"
#include "test-util.h"
#include "test/lptx/lptx-test.h"

#include <stdio.h>
#include <string.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

struct test_lptx_foreach_set_data
{
  LPTX_particle_set *sets[6];
  LPTX_particle_set **lp;
};

static void
test_lptx_foreach_set_data_init(struct test_lptx_foreach_set_data *p)
{
  for (size_t i = 0; i < sizeof(p->sets) / sizeof(p->sets[0]); ++i)
    p->sets[i] = NULL;
  p->lp = p->sets;
}

static LPTX_bool test_lptx_foreach_set1(LPTX_particle_set *p, void *a);
static LPTX_bool test_lptx_foreach_set2(LPTX_particle_set *p, void *a);

struct test_lptx_foreach_set_range_data
{
  struct test_lptx_foreach_set_range_data_entry
  {
    LPTX_particle_set *set;
    LPTX_idtype start;
    LPTX_idtype last;
  } sets[6];
  struct test_lptx_foreach_set_range_data_entry *lp;
};

static void test_lptx_foreach_set_range_data_init(
  struct test_lptx_foreach_set_range_data *p)
{
  for (size_t i = 0; i < sizeof(p->sets) / sizeof(p->sets[0]); ++i) {
    p->sets[i].set = NULL;
    p->sets[i].start = -1;
    p->sets[i].last = -1;
  }
  p->lp = p->sets;
}

static LPTX_bool test_lptx_foreach_set_range1(LPTX_particle_set *p,
                                              LPTX_idtype start,
                                              LPTX_idtype *last, void *a);
static LPTX_bool test_lptx_foreach_set_range2(LPTX_particle_set *p,
                                              LPTX_idtype start,
                                              LPTX_idtype *last, void *a);
static LPTX_bool test_lptx_foreach_set_range3(LPTX_particle_set *p,
                                              LPTX_idtype start,
                                              LPTX_idtype *last, void *a);

int test_lptx_param(void)
{
  int ret = 0;
  LPTX_param *param;

  if (!test_compare_pp((param = LPTX_param_new()), !=, NULL))
    return 1;

#if defined(JUPITER_MPI) && defined(JUPITER_LPTX_MPI)
  if (!test_compare_ii(LPTX_param_set_mpi_comm(param, MPI_COMM_WORLD), ==,
                       MPI_SUCCESS))
    ret = 1;

  if (!test_compare_ii(LPTX_param_set_mpi_errhandler(param, MPI_ERRORS_RETURN),
                       ==, MPI_SUCCESS))
    ret = 1;
#endif

  do {
    LPTX_particle_set *set;

    if (ret)
      break;

    for (int i = 0; i < 5; ++i) {
      if (!test_compare_pp((set = LPTX_particle_set_new(100 * i, 0, NULL)), !=,
                           NULL))
        ret = 1;
      if (ret)
        break;

      if (!test_compare_pp(LPTX_particle_set_append(param, set), ==, set))
        ret = 1;
      if (ret)
        break;
    }

    if (!test_compare_pp((set = LPTX_param_get_particle_set(param, 0)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(LPTX_particle_set_number_of_particles(set), ==, 0))
      ret = 1;

    if (!test_compare_pp((set = LPTX_param_get_particle_set(param, -1)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(LPTX_particle_set_number_of_particles(set), ==, 400))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(LPTX_param_get_total_particles(param), ==, 1000))
      ret = 1;
  } while (0);

  do {
    struct test_lptx_foreach_set_data d;
    if (ret)
      break;

    test_lptx_foreach_set_data_init(&d);
    LPTX_param_foreach_particle_sets(param, test_lptx_foreach_set1, &d);

    if (!test_compare_pp(d.sets[0], ==, LPTX_param_get_particle_set(param, 0)))
      ret = 1;
    if (!test_compare_pp(d.sets[1], ==, LPTX_param_get_particle_set(param, 1)))
      ret = 1;
    if (!test_compare_pp(d.sets[2], ==, LPTX_param_get_particle_set(param, 2)))
      ret = 1;
    if (!test_compare_pp(d.sets[3], ==, LPTX_param_get_particle_set(param, 3)))
      ret = 1;
    if (!test_compare_pp(d.sets[4], ==, LPTX_param_get_particle_set(param, 4)))
      ret = 1;
    if (!test_compare_pp(d.sets[5], ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.lp, ==, &d.sets[5]))
      ret = 1;

    test_lptx_foreach_set_data_init(&d);
    LPTX_param_foreach_particle_sets(param, test_lptx_foreach_set2, &d);

    if (!test_compare_pp(d.sets[0], ==, LPTX_param_get_particle_set(param, 0)))
      ret = 1;
    if (!test_compare_pp(d.sets[1], ==, LPTX_param_get_particle_set(param, 1)))
      ret = 1;
    if (!test_compare_pp(d.sets[2], ==, LPTX_param_get_particle_set(param, 2)))
      ret = 1;
    if (!test_compare_pp(d.sets[3], ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[4], ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[5], ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.lp, ==, &d.sets[3]))
      ret = 1;

    LPTX_particle_set_delete(d.sets[2]);

    test_lptx_foreach_set_data_init(&d);
    LPTX_param_foreach_particle_sets(param, test_lptx_foreach_set1, &d);

    if (!test_compare_pp(d.sets[0], ==, LPTX_param_get_particle_set(param, 0)))
      ret = 1;
    if (!test_compare_pp(d.sets[1], ==, LPTX_param_get_particle_set(param, 1)))
      ret = 1;
    if (!test_compare_pp(d.sets[2], ==, LPTX_param_get_particle_set(param, 2)))
      ret = 1;
    if (!test_compare_pp(d.sets[3], ==, LPTX_param_get_particle_set(param, 3)))
      ret = 1;
    if (!test_compare_pp(d.sets[4], ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[5], ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.lp, ==, &d.sets[4]))
      ret = 1;
  } while (0);

  do {
    struct test_lptx_foreach_set_range_data d;
    /*
     * 0: [0, 0)
     * 1: [0, 100)
     * 3: [100, 400)
     * 4: [400, 800)
     */
    if (ret)
      break;

    if (!test_compare_ii(LPTX_param_get_total_particles(param), ==, 800))
      ret = 1;

    test_lptx_foreach_set_range_data_init(&d);
    LPTX_param_foreach_particle_set_range(param, 0, 100, LPTX_false,
                                          test_lptx_foreach_set_range1, &d);

    if (!test_compare_pp(d.sets[0].set, ==,
                         LPTX_param_get_particle_set(param, 0)))
      ret = 1;
    if (!test_compare_pp(d.sets[1].set, ==,
                         LPTX_param_get_particle_set(param, 1)))
      ret = 1;
    if (!test_compare_pp(d.sets[2].set, ==,
                         LPTX_param_get_particle_set(param, 2)))
      ret = 1;
    if (!test_compare_pp(d.sets[3].set, ==,
                         LPTX_param_get_particle_set(param, 3)))
      ret = 1;
    if (!test_compare_pp(d.sets[4].set, ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[5].set, ==, NULL))
      ret = 1;

    if (!test_compare_ii(d.sets[0].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[2].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[3].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[4].start, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].start, ==, -1))
      ret = 1;

    if (!test_compare_ii(d.sets[0].last, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].last, ==, 100))
      ret = 1;
    if (!test_compare_ii(d.sets[2].last, ==, 300))
      ret = 1;
    if (!test_compare_ii(d.sets[3].last, ==, 400))
      ret = 1;
    if (!test_compare_ii(d.sets[4].last, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].last, ==, -1))
      ret = 1;

    if (!test_compare_pp(d.lp, ==, &d.sets[4]))
      ret = 1;

    test_lptx_foreach_set_range_data_init(&d);
    LPTX_param_foreach_particle_set_range(param, 0, 2, LPTX_false,
                                          test_lptx_foreach_set_range1, &d);

    if (!test_compare_pp(d.sets[0].set, ==,
                         LPTX_param_get_particle_set(param, 0)))
      ret = 1;
    if (!test_compare_pp(d.sets[1].set, ==,
                         LPTX_param_get_particle_set(param, 1)))
      ret = 1;
    if (!test_compare_pp(d.sets[2].set, ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[3].set, ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[4].set, ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[5].set, ==, NULL))
      ret = 1;

    if (!test_compare_ii(d.sets[0].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[2].start, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[3].start, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[4].start, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].start, ==, -1))
      ret = 1;

    if (!test_compare_ii(d.sets[0].last, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].last, ==, 100))
      ret = 1;
    if (!test_compare_ii(d.sets[2].last, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[3].last, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[4].last, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].last, ==, -1))
      ret = 1;

    if (!test_compare_pp(d.lp, ==, &d.sets[2]))
      ret = 1;

    test_lptx_foreach_set_range_data_init(&d);
    LPTX_param_foreach_particle_set_range(param, 2, 4, LPTX_false,
                                          test_lptx_foreach_set_range1, &d);

    if (!test_compare_pp(d.sets[0].set, ==,
                         LPTX_param_get_particle_set(param, 2)))
      ret = 1;
    if (!test_compare_pp(d.sets[1].set, ==,
                         LPTX_param_get_particle_set(param, 3)))
      ret = 1;
    if (!test_compare_pp(d.sets[2].set, ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[3].set, ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[4].set, ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[5].set, ==, NULL))
      ret = 1;

    if (!test_compare_ii(d.sets[0].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[2].start, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[3].start, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[4].start, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].start, ==, -1))
      ret = 1;

    if (!test_compare_ii(d.sets[0].last, ==, 300))
      ret = 1;
    if (!test_compare_ii(d.sets[1].last, ==, 400))
      ret = 1;
    if (!test_compare_ii(d.sets[2].last, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[3].last, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[4].last, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].last, ==, -1))
      ret = 1;

    if (!test_compare_pp(d.lp, ==, &d.sets[2]))
      ret = 1;

    test_lptx_foreach_set_range_data_init(&d);
    LPTX_param_foreach_particle_set_range(param, 0, 100, LPTX_true,
                                          test_lptx_foreach_set_range1, &d);

    if (!test_compare_pp(d.sets[0].set, ==,
                         LPTX_param_get_particle_set(param, 0)))
      ret = 1;
    if (!test_compare_pp(d.sets[1].set, ==,
                         LPTX_param_get_particle_set(param, 1)))
      ret = 1;
    if (!test_compare_pp(d.sets[2].set, ==,
                         LPTX_param_get_particle_set(param, 2)))
      ret = 1;
    if (!test_compare_pp(d.sets[3].set, ==,
                         LPTX_param_get_particle_set(param, 3)))
      ret = 1;
    if (!test_compare_pp(d.sets[4].set, ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[5].set, ==, NULL))
      ret = 1;

    if (!test_compare_ii(d.sets[0].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[2].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[3].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[4].start, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].start, ==, -1))
      ret = 1;

    if (!test_compare_ii(d.sets[0].last, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].last, ==, 100))
      ret = 1;
    if (!test_compare_ii(d.sets[2].last, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[3].last, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[4].last, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].last, ==, -1))
      ret = 1;

    if (!test_compare_pp(d.lp, ==, &d.sets[4]))
      ret = 1;

    test_lptx_foreach_set_range_data_init(&d);
    LPTX_param_foreach_particle_set_range(param, 50, 600, LPTX_true,
                                          test_lptx_foreach_set_range1, &d);

    if (!test_compare_pp(d.sets[0].set, ==,
                         LPTX_param_get_particle_set(param, 0)))
      ret = 1;
    if (!test_compare_pp(d.sets[1].set, ==,
                         LPTX_param_get_particle_set(param, 1)))
      ret = 1;
    if (!test_compare_pp(d.sets[2].set, ==,
                         LPTX_param_get_particle_set(param, 2)))
      ret = 1;
    if (!test_compare_pp(d.sets[3].set, ==,
                         LPTX_param_get_particle_set(param, 3)))
      ret = 1;
    if (!test_compare_pp(d.sets[4].set, ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[5].set, ==, NULL))
      ret = 1;

    if (!test_compare_ii(d.sets[0].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].start, ==, 50))
      ret = 1;
    if (!test_compare_ii(d.sets[2].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[3].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[4].start, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].start, ==, -1))
      ret = 1;

    if (!test_compare_ii(d.sets[0].last, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].last, ==, 100))
      ret = 1;
    if (!test_compare_ii(d.sets[2].last, ==, 300))
      ret = 1;
    if (!test_compare_ii(d.sets[3].last, ==, 200))
      ret = 1;
    if (!test_compare_ii(d.sets[4].last, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].last, ==, -1))
      ret = 1;

    if (!test_compare_pp(d.lp, ==, &d.sets[4]))
      ret = 1;

    test_lptx_foreach_set_range_data_init(&d);
    LPTX_param_foreach_particle_set_range(param, 50, 400, LPTX_true,
                                          test_lptx_foreach_set_range1, &d);

    if (!test_compare_pp(d.sets[0].set, ==,
                         LPTX_param_get_particle_set(param, 0)))
      ret = 1;
    if (!test_compare_pp(d.sets[1].set, ==,
                         LPTX_param_get_particle_set(param, 1)))
      ret = 1;
    if (!test_compare_pp(d.sets[2].set, ==,
                         LPTX_param_get_particle_set(param, 2)))
      ret = 1;
    if (!test_compare_pp(d.sets[3].set, ==,
                         LPTX_param_get_particle_set(param, 3)))
      ret = 1;
    if (!test_compare_pp(d.sets[4].set, ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[5].set, ==, NULL))
      ret = 1;

    if (!test_compare_ii(d.sets[0].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].start, ==, 50))
      ret = 1;
    if (!test_compare_ii(d.sets[2].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[3].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[4].start, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].start, ==, -1))
      ret = 1;

    if (!test_compare_ii(d.sets[0].last, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].last, ==, 100))
      ret = 1;
    if (!test_compare_ii(d.sets[2].last, ==, 300))
      ret = 1;
    if (!test_compare_ii(d.sets[3].last, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[4].last, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].last, ==, -1))
      ret = 1;

    /* extend last 10 particles par set */
    test_lptx_foreach_set_range_data_init(&d);
    LPTX_param_foreach_particle_set_range(param, 50, 600, LPTX_true,
                                          test_lptx_foreach_set_range2, &d);

    if (!test_compare_pp(d.sets[0].set, ==,
                         LPTX_param_get_particle_set(param, 0)))
      ret = 1;
    if (!test_compare_pp(d.sets[1].set, ==,
                         LPTX_param_get_particle_set(param, 1)))
      ret = 1;
    if (!test_compare_pp(d.sets[2].set, ==,
                         LPTX_param_get_particle_set(param, 2)))
      ret = 1;
    if (!test_compare_pp(d.sets[3].set, ==,
                         LPTX_param_get_particle_set(param, 3)))
      ret = 1;
    if (!test_compare_pp(d.sets[4].set, ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[5].set, ==, NULL))
      ret = 1;

    if (!test_compare_ii(d.sets[0].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].start, ==, 50))
      ret = 1;
    if (!test_compare_ii(d.sets[2].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[3].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[4].start, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].start, ==, -1))
      ret = 1;

    if (!test_compare_ii(d.sets[0].last, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].last, ==, 100))
      ret = 1;
    if (!test_compare_ii(d.sets[2].last, ==, 300))
      ret = 1;
    if (!test_compare_ii(d.sets[3].last, ==, 230))
      ret = 1;
    if (!test_compare_ii(d.sets[4].last, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].last, ==, -1))
      ret = 1;

    if (!test_compare_pp(d.lp, ==, &d.sets[4]))
      ret = 1;

    test_lptx_foreach_set_range_data_init(&d);
    LPTX_param_foreach_particle_set_range(param, 50, 400, LPTX_true,
                                          test_lptx_foreach_set_range2, &d);

    if (!test_compare_pp(d.sets[0].set, ==,
                         LPTX_param_get_particle_set(param, 0)))
      ret = 1;
    if (!test_compare_pp(d.sets[1].set, ==,
                         LPTX_param_get_particle_set(param, 1)))
      ret = 1;
    if (!test_compare_pp(d.sets[2].set, ==,
                         LPTX_param_get_particle_set(param, 2)))
      ret = 1;
    if (!test_compare_pp(d.sets[3].set, ==,
                         LPTX_param_get_particle_set(param, 3)))
      ret = 1;
    if (!test_compare_pp(d.sets[4].set, ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[5].set, ==, NULL))
      ret = 1;

    if (!test_compare_ii(d.sets[0].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].start, ==, 50))
      ret = 1;
    if (!test_compare_ii(d.sets[2].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[3].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[4].start, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].start, ==, -1))
      ret = 1;

    if (!test_compare_ii(d.sets[0].last, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].last, ==, 100))
      ret = 1;
    if (!test_compare_ii(d.sets[2].last, ==, 300))
      ret = 1;
    if (!test_compare_ii(d.sets[3].last, ==, 30))
      ret = 1;
    if (!test_compare_ii(d.sets[4].last, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].last, ==, -1))
      ret = 1;

    if (!test_compare_pp(d.lp, ==, &d.sets[4]))
      ret = 1;

    /* shrink last 10 particles par set */
    test_lptx_foreach_set_range_data_init(&d);
    LPTX_param_foreach_particle_set_range(param, 50, 600, LPTX_true,
                                          test_lptx_foreach_set_range3, &d);

    if (!test_compare_pp(d.sets[0].set, ==,
                         LPTX_param_get_particle_set(param, 0)))
      ret = 1;
    if (!test_compare_pp(d.sets[1].set, ==,
                         LPTX_param_get_particle_set(param, 1)))
      ret = 1;
    if (!test_compare_pp(d.sets[2].set, ==,
                         LPTX_param_get_particle_set(param, 2)))
      ret = 1;
    if (!test_compare_pp(d.sets[3].set, ==,
                         LPTX_param_get_particle_set(param, 3)))
      ret = 1;
    if (!test_compare_pp(d.sets[4].set, ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[5].set, ==, NULL))
      ret = 1;

    if (!test_compare_ii(d.sets[0].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].start, ==, 50))
      ret = 1;
    if (!test_compare_ii(d.sets[2].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[3].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[4].start, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].start, ==, -1))
      ret = 1;

    if (!test_compare_ii(d.sets[0].last, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].last, ==, 100))
      ret = 1;
    if (!test_compare_ii(d.sets[2].last, ==, 300))
      ret = 1;
    if (!test_compare_ii(d.sets[3].last, ==, 170))
      ret = 1;
    if (!test_compare_ii(d.sets[4].last, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].last, ==, -1))
      ret = 1;

    if (!test_compare_pp(d.lp, ==, &d.sets[4]))
      ret = 1;

    test_lptx_foreach_set_range_data_init(&d);
    LPTX_param_foreach_particle_set_range(param, 50, 400, LPTX_true,
                                          test_lptx_foreach_set_range3, &d);

    if (!test_compare_pp(d.sets[0].set, ==,
                         LPTX_param_get_particle_set(param, 0)))
      ret = 1;
    if (!test_compare_pp(d.sets[1].set, ==,
                         LPTX_param_get_particle_set(param, 1)))
      ret = 1;
    if (!test_compare_pp(d.sets[2].set, ==,
                         LPTX_param_get_particle_set(param, 2)))
      ret = 1;
    if (!test_compare_pp(d.sets[3].set, ==,
                         LPTX_param_get_particle_set(param, 3)))
      ret = 1;
    if (!test_compare_pp(d.sets[4].set, ==, NULL))
      ret = 1;
    if (!test_compare_pp(d.sets[5].set, ==, NULL))
      ret = 1;

    if (!test_compare_ii(d.sets[0].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].start, ==, 50))
      ret = 1;
    if (!test_compare_ii(d.sets[2].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[3].start, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[4].start, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].start, ==, -1))
      ret = 1;

    if (!test_compare_ii(d.sets[0].last, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[1].last, ==, 100))
      ret = 1;
    if (!test_compare_ii(d.sets[2].last, ==, 280))
      ret = 1;
    if (!test_compare_ii(d.sets[3].last, ==, 0))
      ret = 1;
    if (!test_compare_ii(d.sets[4].last, ==, -1))
      ret = 1;
    if (!test_compare_ii(d.sets[5].last, ==, -1))
      ret = 1;

    if (!test_compare_pp(d.lp, ==, &d.sets[4]))
      ret = 1;
  } while (0);

  do {
    LPTX_particle_set *s;
    const LPTX_particle_set *sc;
    struct test_lptx_foreach_set_data d, dn;
    if (ret)
      break;

    test_lptx_foreach_set_data_init(&d);
    LPTX_param_foreach_particle_sets(param, test_lptx_foreach_set1, &d);
    if (!test_compare_pp(d.lp, ==, &d.sets[4]))
      ret = 1;

    if (!test_compare_ii(LPTX_param_get_total_particles(param), ==, 800))
      ret = 1;

    if (!test_compare_pp((s = LPTX_param_get_particle_set(param, 1)), !=, NULL))
      ret = 1;
    if (s) {
      LPTX_particle_set_set_particle_id_at(s, 0, 0);
      LPTX_particle_set_set_particle_id_at(s, 5, 5);
    }

    if (!test_compare_pp((s = LPTX_param_get_particle_set(param, 2)), !=, NULL))
      ret = 1;
    if (s) {
      LPTX_particle_set_set_particle_id_at(s, 0, 100);
      LPTX_particle_set_set_particle_id_at(s, 199, 299);
    }

    if (!test_compare_pp((s = LPTX_param_join_particle_sets(param)), !=, NULL))
      ret = 1;
    if (s) {
      if (!test_compare_pp(LPTX_particle_set_param(s), ==, NULL))
        ret = 1;
      if (!test_compare_ii(LPTX_particle_set_number_of_particles(s), ==, 800))
        ret = 1;
      do {
        if (ret)
          break;

        if (!test_compare_ii(LPTX_particle_set_get_particle_id_at(s, 0), ==, 0))
          ret = 1;
        if (!test_compare_ii(LPTX_particle_set_get_particle_id_at(s, 5), ==, 5))
          ret = 1;
        if (!test_compare_ii(LPTX_particle_set_get_particle_id_at(s, 100), ==,
                             100))
          ret = 1;
        if (!test_compare_ii(LPTX_particle_set_get_particle_id_at(s, 299), ==,
                             299))
          ret = 1;
      } while (0);
      LPTX_particle_set_delete(s);
    }

    test_lptx_foreach_set_data_init(&dn);
    LPTX_param_foreach_particle_sets(param, test_lptx_foreach_set1, &dn);
    if (!test_compare_pp(dn.lp, ==, &dn.sets[4]))
      ret = 1;

    /* Confirmation with sets before join */
    if (!test_compare_pp(d.sets[0], ==, LPTX_param_get_particle_set(param, 0)))
      ret = 1;
    if (!test_compare_pp(d.sets[1], ==, LPTX_param_get_particle_set(param, 1)))
      ret = 1;
    if (!test_compare_pp(d.sets[2], ==, LPTX_param_get_particle_set(param, 2)))
      ret = 1;
    if (!test_compare_pp(d.sets[3], ==, LPTX_param_get_particle_set(param, 3)))
      ret = 1;

    if (ret)
      break;

    if (!test_compare_pp((sc = LPTX_param_join_particle_sets_inplace(param)),
                         !=, NULL))
      ret = 1;
    if (sc) {
      if (!test_compare_ii(LPTX_particle_set_number_of_particles(sc), ==, 800))
        ret = 1;

      do {
        if (ret)
          break;

        s = (LPTX_particle_set *)sc;
        if (!test_compare_pp(LPTX_particle_set_param(s), ==, param))
          ret = 1;

        if (!test_compare_ii(LPTX_particle_set_get_particle_id_at(s, 0), ==, 0))
          ret = 1;
        if (!test_compare_ii(LPTX_particle_set_get_particle_id_at(s, 5), ==, 5))
          ret = 1;
        if (!test_compare_ii(LPTX_particle_set_get_particle_id_at(s, 100), ==,
                             100))
          ret = 1;
        if (!test_compare_ii(LPTX_particle_set_get_particle_id_at(s, 299), ==,
                             299))
          ret = 1;
      } while (0);
      s = NULL;
    }

    test_lptx_foreach_set_data_init(&dn);
    LPTX_param_foreach_particle_sets(param, test_lptx_foreach_set1, &dn);
    if (!test_compare_pp(dn.lp, ==, &dn.sets[1]))
      ret = 1;

    if (!test_compare_pp((s = LPTX_param_join_particle_sets(param)), !=, NULL))
      ret = 1;
    if (s) {
      if (!test_compare_pp(LPTX_particle_set_param(s), ==, param))
        ret = 1;
      if (!test_compare_ii(LPTX_particle_set_number_of_particles(s), ==, 800))
        ret = 1;
      do {
        if (ret)
          break;

        if (!test_compare_ii(LPTX_particle_set_get_particle_id_at(s, 0), ==, 0))
          ret = 1;
        if (!test_compare_ii(LPTX_particle_set_get_particle_id_at(s, 5), ==, 5))
          ret = 1;
        if (!test_compare_ii(LPTX_particle_set_get_particle_id_at(s, 100), ==,
                             100))
          ret = 1;
        if (!test_compare_ii(LPTX_particle_set_get_particle_id_at(s, 299), ==,
                             299))
          ret = 1;
      } while (0);
    }

    test_lptx_foreach_set_data_init(&dn);
    LPTX_param_foreach_particle_sets(param, test_lptx_foreach_set1, &dn);
    if (!test_compare_pp(dn.lp, ==, &dn.sets[1]))
      ret = 1;

    if (!test_compare_pp((sc = LPTX_param_join_particle_sets_inplace(param)),
                         !=, NULL))
      ret = 1;
    if (sc) {
      if (!test_compare_ii(LPTX_particle_set_number_of_particles(sc), ==, 800))
        ret = 1;
      do {
        if (ret)
          break;

        s = (LPTX_particle_set *)sc;
        if (!test_compare_pp(LPTX_particle_set_param(s), ==, param))
          ret = 1;

        if (!test_compare_ii(LPTX_particle_set_get_particle_id_at(s, 0), ==, 0))
          ret = 1;
        if (!test_compare_ii(LPTX_particle_set_get_particle_id_at(s, 5), ==, 5))
          ret = 1;
        if (!test_compare_ii(LPTX_particle_set_get_particle_id_at(s, 100), ==,
                             100))
          ret = 1;
        if (!test_compare_ii(LPTX_particle_set_get_particle_id_at(s, 299), ==,
                             299))
          ret = 1;
      } while (0);
    }

    test_lptx_foreach_set_data_init(&dn);
    LPTX_param_foreach_particle_sets(param, test_lptx_foreach_set1, &dn);
    if (!test_compare_pp(dn.lp, ==, &dn.sets[1]))
      ret = 1;

    LPTX_param_delete_all_particles(param);
    if (!test_compare_ii(LPTX_param_get_total_particles(param), ==, 0))
      ret = 1;

    test_lptx_foreach_set_data_init(&dn);
    LPTX_param_foreach_particle_sets(param, test_lptx_foreach_set1, &dn);
    if (!test_compare_pp(dn.lp, ==, &dn.sets[0]))
      ret = 1;

    if (!test_compare_pp((s = LPTX_param_join_particle_sets(param)), !=, NULL))
      ret = 1;
    if (s) {
      if (!test_compare_pp(LPTX_particle_set_param(s), ==, NULL))
        ret = 1;
      if (!test_compare_ii(LPTX_particle_set_number_of_particles(s), ==, 0))
        ret = 1;
      LPTX_particle_set_delete(s);
    }

    if (!test_compare_pp((sc = LPTX_param_join_particle_sets_inplace(param)),
                         !=, NULL))
      ret = 1;
    if (sc) {
      if (!test_compare_ii(LPTX_particle_set_number_of_particles(sc), ==, 0))
        ret = 1;
    }

    if (!test_compare_pp((s = LPTX_param_new_particle_set(param, 0)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    LPTX_particle_set_append(param, s);
    if (!test_compare_ii(LPTX_param_get_total_particles(param), ==, 0))
      ret = 1;

    test_lptx_foreach_set_data_init(&dn);
    LPTX_param_foreach_particle_sets(param, test_lptx_foreach_set1, &dn);
    if (!test_compare_pp(dn.lp, ==, &dn.sets[1]))
      ret = 1;

    if (!test_compare_pp((s = LPTX_param_join_particle_sets(param)), !=, NULL))
      ret = 1;
    if (s) {
      if (!test_compare_pp(LPTX_particle_set_param(s), ==, param))
        ret = 1;
      if (!test_compare_ii(LPTX_particle_set_number_of_particles(s), ==, 0))
        ret = 1;
    }

    if (!test_compare_pp((sc = LPTX_param_join_particle_sets_inplace(param)),
                         !=, NULL))
      ret = 1;
    if (sc) {
      if (!test_compare_ii(LPTX_particle_set_number_of_particles(sc), ==, 0))
        ret = 1;
    }

  } while (0);

  LPTX_param_delete(param);
  return ret;
}

static LPTX_bool test_lptx_foreach_set1(LPTX_particle_set *p, void *a)
{
  struct test_lptx_foreach_set_data *pp = a;
  *(pp->lp)++ = p;
  return LPTX_false;
}

static LPTX_bool test_lptx_foreach_set2(LPTX_particle_set *p, void *a)
{
  struct test_lptx_foreach_set_data *pp = a;
  *(pp->lp)++ = p;
  if (pp->lp > &pp->sets[2])
    return LPTX_true;
  return LPTX_false;
}

static LPTX_bool test_lptx_foreach_set_range1(LPTX_particle_set *p,
                                              LPTX_idtype start,
                                              LPTX_idtype *last, void *a)
{
  struct test_lptx_foreach_set_range_data *pp = a;
  struct test_lptx_foreach_set_range_data_entry *e;
  e = pp->lp++;
  e->set = p;
  e->start = start;
  e->last = *last;
  return LPTX_false;
}

static LPTX_bool test_lptx_foreach_set_range2(LPTX_particle_set *p,
                                              LPTX_idtype start,
                                              LPTX_idtype *last, void *a)
{
  struct test_lptx_foreach_set_range_data *pp = a;
  test_lptx_foreach_set_range1(p, start, last, a);
  *last += 10;
  return LPTX_false;
}

static LPTX_bool test_lptx_foreach_set_range3(LPTX_particle_set *p,
                                              LPTX_idtype start,
                                              LPTX_idtype *last, void *a)
{
  struct test_lptx_foreach_set_range_data *pp = a;
  test_lptx_foreach_set_range1(p, start, last, a);
  *last -= 10;
  return LPTX_false;
}
