#include "control_test.h"
#include "control_test_grid_data_feeder.h"
#include "jupiter/control/data_array.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/executive.h"
#include "jupiter/control/extent.h"
#include "jupiter/control/field_variable.h"
#include "jupiter/control/fv_get.h"
#include "jupiter/control/information.h"
#include "jupiter/control/input.h"
#include "jupiter/control/mpi_controller.h"
#include "jupiter/control/output.h"
#include "test-util.h"

#include <stddef.h>
#include <time.h>

static double xcoords[][6] = {
  {0, 1, 2, 3, 4, 5},
  {4, 5, 6, 7, 8, 9},
  {8, 9, 10, 11, 12, 13},
  {12, 13, 14, 15, 16, 17},
};

static double source_ary[][5 * 3 * 2] = {
  {
    237.5,  148.0, 49.25,  38.75,  8.75, //
    236.0,  233.5, 176.0,  179.25, 56.0, //
    71.5,   61.0,  245.75, 215.25, 41.0, //

    108.75, 151.5, 21.75,  73.5,   15.0,   //
    147.75, 74.75, 128.75, 27.5,   167.75, //
    64.0,   143.0, 133.25, 156.75, 123.0,  //
  },
  {
    207.5,  67.25,  90.25,  118.0,  75.0,   //
    83.25,  243.25, 70.0,   114.25, 138.75, //
    182.25, 219.75, 240.25, 122.25, 204.75, //

    118.25, 97.75,  164.0,  235.5,  17.0,   //
    118.5,  114.75, 235.0,  33.0,   248.5,  //
    244.0,  238.25, 44.25,  47.75,  189.25, //
  },
  {
    54.25,  102.25, 152.5,  174.25, 29.25,  //
    100.5,  175.0,  80.0,   89.75,  207.0,  //
    214.75, 122.0,  135.75, 115.5,  239.75, //

    99.0,   44.5,   84.5,   2.0,    104.25, //
    106.75, 76.75,  111.0,  116.75, 153.0,  //
    51.25,  125.5,  38.5,   205.25, 65.5,   //
  },
  {
    61.5,   113.0,  171.0,  228.5,  234.5, //
    225.25, 17.25,  74.25,  26.0,   120.0, //
    161.75, 165.75, 247.25, 233.75, 35.25, //

    50.25,  223.25, 39.0,   193.25, 4.0,    //
    134.75, 19.0,   159.75, 214.0,  105.5,  //
    58.75,  31.75,  161.0,  183.75, 138.25, //
  },
};

int test_control_fv_get(void)
{
  int ret;
  struct grid_data_feeder feeder;
  jcntrl_fv_get *getter;
  const jcntrl_mpi_controller *world_comm;
  jcntrl_data_array *arys[1] = {NULL};
  const jcntrl_size_type nary = sizeof(arys) / sizeof(arys[0]);
  jcntrl_extent data_extent;
  int nproc, irank;
  int is, ie;
  double *pd, *xd;

  if (!test_compare_ii(grid_data_feeder_init(&feeder), ==, 1))
    return 1;

  if (!test_compare_pp((world_comm = jcntrl_mpi_controller_world()), !=, NULL))
    return 1;

  nproc = jcntrl_mpi_controller_nproc(world_comm);
  irank = jcntrl_mpi_controller_rank(world_comm);

  is = irank * 4;
  ie = is + 5;
  data_extent = jcntrl_extent_i(is, ie, 0, 3, 0, 2, 0);
  pd = source_ary[irank % 4];
  xd = xcoords[irank % 4];

  is = irank * 4 + 1;
  ie = is + 4;
  feeder.local_data_extent = jcntrl_extent_i(is, ie, 0, 3, 0, 2, 0);

  if (!test_compare_ii(grid_data_feeder_set(&feeder, data_extent, dp(6, xd),
                                            d(({0, 1, 2, 3})), d(({0, 1, 2})),
                                            0, dp(30, pd, "name")),
                       ==, 1))
    return 1;

  if (!test_compare_pp((getter = jcntrl_fv_get_new()), !=, NULL))
    return 1;

  ret = 0;

  do {
    jcntrl_executive *exe;
    jcntrl_input *input;
    jcntrl_output *output, *output_getter;
    jcntrl_shared_object *obj;
    jcntrl_field_variable *fv;
    jcntrl_information *info;

    jcntrl_fv_get_set_controller(getter, world_comm);
    jcntrl_fv_get_set_varname_c(getter, "name");
    jcntrl_fv_get_set_extent(getter, (int[]){3, 8, 0, 2, 0, 1});
    jcntrl_fv_get_set_extract_extent(getter, 1);

    if (!test_compare_pp((output =
                            jcntrl_executive_get_output(&feeder.executive)),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((exe = jcntrl_fv_get_executive(getter)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((input = jcntrl_executive_get_input(exe)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((output = jcntrl_output_next_port(output)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((input = jcntrl_input_next_port(input)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_input_connect(input, output), ==, 1)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
      ret = 1;

    if (!test_compare_pp((output_getter = jcntrl_executive_output_port(exe, 0)),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((info = jcntrl_output_information(output_getter)), !=,
                         NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp(
          (obj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT)),
          !=, NULL)) {
      ret = 1;
      break;
    }

    fv = NULL;
    if (!test_compare_pp((fv = jcntrl_field_variable_downcast(obj)), !=, NULL))
      ret = 1;

    if (fv) {
      jcntrl_data_array *d = jcntrl_field_variable_array(fv);

      if (nproc == 1) {
        /* (3..4, 0..1, 0..0) => 4 */
        if (!test_compare_ii(jcntrl_data_array_get_ntuple(d), ==, 4))
          ret = 1;
      } else {
        /* (3..7, 0..1, 0..0) => 10 */
        if (!test_compare_ii(jcntrl_data_array_get_ntuple(d), ==, 10))
          ret = 1;
      }
    }
    /// @todo Add masked extraction test.
  } while (0);

  jcntrl_fv_get_delete(getter);
  grid_data_feeder_clean(&feeder);
  for (int i = 0; i < nary; ++i) {
    if (arys[i])
      jcntrl_data_array_delete(arys[i]);
  }
  return ret;
}
