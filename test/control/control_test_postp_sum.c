#include "jupiter/control/cell_data.h"
#include "jupiter/control/data_array.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/executive.h"
#include "jupiter/control/extent.h"
#include "jupiter/control/grid_data.h"
#include "jupiter/control/information.h"
#include "jupiter/control/input.h"
#include "jupiter/control/mpi_controller.h"
#include "jupiter/control/output.h"
#include "jupiter/control/postp_sum.h"
#include "test-util.h"
#include "test/control/control_test.h"
#include "test/control/control_test_grid_data_feeder.h"
#include "test/control/control_test_util.h"
#include <string.h>

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

int test_control_postp_sum(void)
{
  const jcntrl_mpi_controller *world_comm;
  jcntrl_postp_sum *psum;
  struct grid_data_feeder feeder;
  int ret;
  jcntrl_data_array *arys[1] = {NULL};
  const jcntrl_size_type nary = sizeof(arys) / sizeof(arys[0]);
  jcntrl_extent data_extent;
  int nproc, irank;
  int is, ie;
  double *pd, *xd;

  ret = 0;

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
                                            d(({0, 2, 4, 6})), d(({0, 3, 6})),
                                            0, dp(30, pd, "name")),
                       ==, 1))
    return 1;

  psum = NULL;
  if (!test_compare_pp((psum = jcntrl_postp_sum_new()), !=, NULL))
    ret = 1;

  do {
    jcntrl_executive *exec;
    jcntrl_input *input;
    jcntrl_output *output;

    if (ret)
      break;

    if (!test_compare_pp((exec = jcntrl_postp_sum_executive(psum)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((input = jcntrl_executive_input_port(exec, 0)), !=,
                         NULL))
      ret = 1;

    if (!test_compare_pp((output =
                            jcntrl_executive_output_port(&feeder.executive, 0)),
                         !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_input_connect(input, output), ==, 1))
      ret = 1;
    if (ret)
      break;

    jcntrl_postp_sum_set_controller(psum, world_comm);
  } while (0);

  ret = jcntrl_mpi_controller_any(world_comm, ret);

  do {
    jcntrl_executive *exec;
    if (ret)
      break;

    exec = jcntrl_postp_sum_executive(psum);
    if (!test_compare_ii(jcntrl_executive_update(exec), ==, 1))
      ret = 1;
    if (ret)
      break;
  } while (0);

  ret = jcntrl_mpi_controller_any(world_comm, ret);

  do {
    jcntrl_executive *exec;
    jcntrl_output *output;
    jcntrl_information *info;
    jcntrl_shared_object *obj;
    jcntrl_grid_data *g;
    jcntrl_cell_data *cdata;
    jcntrl_data_array *d;
    const double *dval;

    if (ret)
      break;

    if (irank != 0)
      break;

    exec = NULL;
    if (!test_compare_pp((exec = jcntrl_postp_sum_executive(psum)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((output = jcntrl_executive_output_port(exec, 0)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((info = jcntrl_output_information(output)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp(
          (obj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT)),
          !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((g = jcntrl_grid_data_downcast(obj)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_jcntrl_extent(jcntrl_extent_c(jcntrl_grid_data_extent(g)),
                                    jcntrl_extent_i(0, 1, 0, 1, 0, 1, 0)))
      ret = 1;

    if (!test_compare_pp((d = jcntrl_grid_data_x_coords(g)), !=, NULL))
      ret = 1;
    if (ret)
      break;
    if (!test_compare_ii(jcntrl_data_array_get_ntuple(d), ==, 2))
      ret = 1;

    if (!test_compare_pp((dval = jcntrl_data_array_get_double(d)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_dd(dval[0], ==, 1.0))
      ret = 1;
    switch (nproc) {
    case 1:
      if (!test_compare_dd(dval[1], ==, 5.0))
        ret = 1;
      break;
    case 2:
      if (!test_compare_dd(dval[1], ==, 9.0))
        ret = 1;
      break;
    case 3:
      if (!test_compare_dd(dval[1], ==, 13.0))
        ret = 1;
      break;
    case 4:
      if (!test_compare_dd(dval[1], ==, 17.0))
        ret = 1;
      break;
    }

    if (!test_compare_pp((d = jcntrl_grid_data_y_coords(g)), !=, NULL))
      ret = 1;
    if (ret)
      break;
    if (!test_compare_ii(jcntrl_data_array_get_ntuple(d), ==, 2))
      ret = 1;

    if (!test_compare_pp((dval = jcntrl_data_array_get_double(d)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_dd(dval[0], ==, 0.0))
      ret = 1;
    if (!test_compare_dd(dval[1], ==, 6.0))
      ret = 1;

    if (!test_compare_pp((d = jcntrl_grid_data_z_coords(g)), !=, NULL))
      ret = 1;
    if (ret)
      break;
    if (!test_compare_ii(jcntrl_data_array_get_ntuple(d), ==, 2))
      ret = 1;

    if (!test_compare_pp((dval = jcntrl_data_array_get_double(d)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_dd(dval[0], ==, 0.0))
      ret = 1;
    if (!test_compare_dd(dval[1], ==, 6.0))
      ret = 1;

    if (!test_compare_pp((cdata = jcntrl_grid_data_cell_data(g)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((d = jcntrl_cell_data_get_array_by_name(cdata, "name",
                                                                 4)),
                         !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_data_array_get_ntuple(d), ==, 1))
      ret = 1;
    if (!test_compare_pp((dval = jcntrl_data_array_get_double(d)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    switch (nproc) {
    case 1:
      if (!test_compare_dd(dval[0], ==, 2669.0))
        ret = 1;
      break;
    case 2:
      if (!test_compare_dd(dval[0], ==, 2669.0 + 3368.75))
        ret = 1;
      break;
    case 3:
      if (!test_compare_dd(dval[0], ==, 2669.0 + 3368.75 + 2750.5))
        ret = 1;
      break;
    case 4:
      if (!test_compare_dd(dval[0], ==, 2669.0 + 3368.75 + 2750.5 + 3139.0))
        ret = 1;
      break;
    }
  } while (0);

  ret = jcntrl_mpi_controller_any(world_comm, ret);
  do {
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_postp_sum_set_cardinality_varname_c(psum, "car",
                                                                    3),
                         ==, 1))
      ret = 1;

    ret = jcntrl_mpi_controller_any(world_comm, ret);
  } while (0);

  do {
    jcntrl_executive *exec;
    jcntrl_output *output;
    jcntrl_information *info;
    jcntrl_shared_object *obj;
    jcntrl_grid_data *g;
    jcntrl_cell_data *cdata;
    jcntrl_data_array *d;
    const double *dval;
    const jcntrl_size_type *sval;

    if (ret)
      break;

    exec = jcntrl_postp_sum_executive(psum);
    if (!test_compare_ii(jcntrl_executive_update(exec), ==, 1))
      ret = 1;

    ret = jcntrl_mpi_controller_any(world_comm, ret);
    if (ret)
      break;

    if (irank != 0)
      break;

    if (!test_compare_pp((output = jcntrl_executive_output_port(exec, 0)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((info = jcntrl_output_information(output)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp(
          (obj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT)),
          !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((g = jcntrl_grid_data_downcast(obj)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((cdata = jcntrl_grid_data_cell_data(g)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((d = jcntrl_cell_data_get_array_by_name(cdata, "car",
                                                                 3)),
                         !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_data_array_get_ntuple(d), ==, 1))
      ret = 1;
    if (!test_compare_pp((sval = jcntrl_data_array_get_sizes(d)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_dd(sval[0], ==, nproc * 24))
      ret = 1;

  } while (0);

  if (psum)
    jcntrl_postp_sum_delete(psum);
  grid_data_feeder_clean(&feeder);
  return ret;
}
