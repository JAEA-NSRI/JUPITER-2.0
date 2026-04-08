
#include "jupiter/control/cell_data.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/executive.h"
#include "jupiter/control/extent.h"
#include "jupiter/control/input.h"
#include "jupiter/control/postp_pass_arrays.h"
#include "jupiter/control/grid_data.h"
#include "test-util.h"
#include "test/control/control_test.h"
#include "test/control/control_test_grid_data_feeder.h"
#include <string.h>

int test_control_postp_pass_arrays(void)
{
  int ret;
  struct grid_data_feeder feeder;
  jcntrl_postp_pass_arrays *pass_arys;

  ret = 0;
  pass_arys = NULL;
  grid_data_feeder_init(&feeder);

  do {
    jcntrl_executive *exec;
    jcntrl_input *input;
    jcntrl_output *output;
    jcntrl_grid_data *igrid, *ogrid;
    jcntrl_cell_data *ocdata;

    if (!test_compare_ii(grid_data_feeder_set(
                           &feeder, jcntrl_extent_c((int[]){0, 1, 0, 1, 0, 1}),
                           d(({0, 1})), d(({0, 1})), d(({0, 1})), 0,
                           d(({1}), "a"), d(({2}), "b"), d(({3}), "c")),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_executive_update(&feeder.executive), ==, 1))
      ret = 1;

    if (!test_compare_pp((igrid = jcntrl_executive_get_output_data_object_as(
                            &feeder.executive, 0, jcntrl_grid_data)),
                         !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((pass_arys = jcntrl_postp_pass_arrays_new()), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    jcntrl_postp_pass_arrays_set_mode(pass_arys, JCNTRL_POSTP_PASS_DELETE);
    if (!test_compare_ii(
          jcntrl_postp_pass_arrays_set_number_of_variables(pass_arys, 1), ==,
          1))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_postp_pass_arrays_set_variable_c(pass_arys, 0,
                                                                 "b", 1),
                         ==, 1))
      ret = 1;

    if (!test_compare_pp((exec = jcntrl_postp_pass_arrays_executive(pass_arys)),
                         !=, NULL))
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

    if (!test_compare_ii(jcntrl_executive_update(exec), ==, 1))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((ogrid = jcntrl_executive_get_output_data_object_as(
                            exec, 0, jcntrl_grid_data)),
                         !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((ocdata = jcntrl_grid_data_cell_data(ogrid)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(ocdata), ==, 2))
      ret = 1;
    if (!test_compare_pp(jcntrl_cell_data_get_array_by_name(ocdata, "a", 1), !=,
                         NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_cell_data_get_array_by_name(ocdata, "b", 1), ==,
                         NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_cell_data_get_array_by_name(ocdata, "c", 1), !=,
                         NULL))
      ret = 1;

    jcntrl_postp_pass_arrays_set_mode(pass_arys, JCNTRL_POSTP_PASS_KEEP);

    if (!test_compare_ii(jcntrl_executive_update(exec), ==, 1))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((ogrid = jcntrl_executive_get_output_data_object_as(
                            exec, 0, jcntrl_grid_data)),
                         !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((ocdata = jcntrl_grid_data_cell_data(ogrid)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(ocdata), ==, 1))
      ret = 1;
    if (!test_compare_pp(jcntrl_cell_data_get_array_by_name(ocdata, "a", 1), ==,
                         NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_cell_data_get_array_by_name(ocdata, "b", 1), !=,
                         NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_cell_data_get_array_by_name(ocdata, "c", 1), ==,
                         NULL))
      ret = 1;

  } while (0);

  grid_data_feeder_clean(&feeder);
  if (pass_arys)
    jcntrl_postp_pass_arrays_delete(pass_arys);

  return ret;
}
