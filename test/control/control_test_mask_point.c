
#include "control_test.h"
#include "control_test_field_value_feeder.h"
#include "control_test_grid_data_feeder.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/cell.h"
#include "jupiter/control/executive.h"
#include "jupiter/control/grid_data.h"
#include "jupiter/control/information.h"
#include "jupiter/control/input.h"
#include "jupiter/control/mask_function.h"
#include "jupiter/control/mask_point.h"
#include "jupiter/control/output.h"
#include "jupiter/control/static_array.h"
#include "test-util.h"
#include <string.h>

int test_control_mask_point(void)
{
  struct field_value_feeder xfeeder, yfeeder, zfeeder;
  struct grid_data_feeder feeder;
  jcntrl_mask_point *mask_point;
  int ret;
  ret = 0;

  mask_point = NULL;
  grid_data_feeder_init(&feeder);
  field_value_feeder_init(&xfeeder);
  field_value_feeder_init(&yfeeder);
  field_value_feeder_init(&zfeeder);

  do {
    jcntrl_shared_object *obj;
    jcntrl_mask_function *f;
    jcntrl_information *info;
    jcntrl_executive *exec;
    jcntrl_input *input;
    jcntrl_output *output;
    jcntrl_grid_data *grid;
    jcntrl_cell_hex h;

    if (!test_compare_ii(grid_data_feeder_set(
                           &feeder, jcntrl_extent_c((int[]){0, 3, 0, 3, 0, 3}),
                           d(({0, 1, 2, 3})), d(({0, 1, 2, 3})),
                           d(({0, 1, 2, 3}))),
                         ==, 1)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_executive_update(&feeder.executive), ==, 1)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((output =
                            jcntrl_executive_output_port(&feeder.executive, 0)),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((info = jcntrl_output_information(output)), !=,
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

    if (!test_compare_pp((grid = jcntrl_grid_data_downcast(obj)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((mask_point = jcntrl_mask_point_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((exec = jcntrl_mask_point_executive(mask_point)), !=,
                         NULL)) {
      ret = 1;
      break;
    }

    jcntrl_mask_point_set_default_point_x(mask_point, 1.0);
    jcntrl_mask_point_set_default_point_y(mask_point, 1.0);
    jcntrl_mask_point_set_default_point_z(mask_point, 1.0);

    if (!test_compare_ii(jcntrl_executive_update(exec), ==, 1)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((output = jcntrl_executive_output_port(exec, 0)), !=,
                         NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((info = jcntrl_output_information(output)), !=,
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

    if (!test_compare_pp((f = jcntrl_mask_function_downcast(obj)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 0, 0, 0, NULL), ==,
                         1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(f, jcntrl_cell_hex_cell(&h),
                                                   0, 0, 0),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 1, 0, 0, NULL), ==,
                         1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(f, jcntrl_cell_hex_cell(&h),
                                                   0, 0, 0),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 0, 1, 0, NULL), ==,
                         1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(f, jcntrl_cell_hex_cell(&h),
                                                   0, 0, 0),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 1, 1, 0, NULL), ==,
                         1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(f, jcntrl_cell_hex_cell(&h),
                                                   0, 0, 0),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 0, 0, 1, NULL), ==,
                         1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(f, jcntrl_cell_hex_cell(&h),
                                                   0, 0, 0),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 1, 0, 1, NULL), ==,
                         1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(f, jcntrl_cell_hex_cell(&h),
                                                   0, 0, 0),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 0, 1, 1, NULL), ==,
                         1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(f, jcntrl_cell_hex_cell(&h),
                                                   0, 0, 0),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 1, 1, 1, NULL), ==,
                         1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(f, jcntrl_cell_hex_cell(&h),
                                                   0, 0, 0),
                         ==, 0))
      ret = 1;

    jcntrl_static_double_array_init_n(&xfeeder.array, 2, 0.0);
    if (!test_compare_pp((output =
                            jcntrl_executive_output_port(&xfeeder.executive,
                                                         0)),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((input = jcntrl_mask_point_x_point_port(mask_point)),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_input_connect(input, output), ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_executive_update(exec), ==, 0)) {
      ret = 1;
      break;
    }

    jcntrl_static_double_array_init_n(&xfeeder.array, 1, 2.0);
    jcntrl_static_double_array_init_n(&yfeeder.array, 1, 3.0);
    jcntrl_static_double_array_init_n(&zfeeder.array, 1, 1.0);

    if (!test_compare_pp((output =
                            jcntrl_executive_output_port(&yfeeder.executive,
                                                         0)),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((input = jcntrl_mask_point_y_point_port(mask_point)),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_input_connect(input, output), ==, 1))
      ret = 1;

    if (!test_compare_pp((output =
                            jcntrl_executive_output_port(&zfeeder.executive,
                                                         0)),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((input = jcntrl_mask_point_z_point_port(mask_point)),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_input_connect(input, output), ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_executive_update(exec), ==, 1)) {
      ret = 1;
      break;
    }


    if (!test_compare_pp((output = jcntrl_executive_output_port(exec, 0)), !=,
                         NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((info = jcntrl_output_information(output)), !=,
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

    if (!test_compare_pp((f = jcntrl_mask_function_downcast(obj)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 2, 3, 1, NULL), ==,
                         0))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 2, 2, 1, NULL), ==,
                         1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(f, jcntrl_cell_hex_cell(&h),
                                                   0, 0, 0),
                         ==, 0))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 1, 2, 1, NULL), ==,
                         1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(f, jcntrl_cell_hex_cell(&h),
                                                   0, 0, 0),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 2, 2, 2, NULL), ==,
                         1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(f, jcntrl_cell_hex_cell(&h),
                                                   0, 0, 0),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 1, 2, 2, NULL), ==,
                         1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(f, jcntrl_cell_hex_cell(&h),
                                                   0, 0, 0),
                         ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_hex_init_g(&h, grid, 1, 1, 1, NULL), ==,
                         1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(f, jcntrl_cell_hex_cell(&h),
                                                   0, 0, 0),
                         ==, 1))
      ret = 1;

  } while (0);

  if (mask_point)
    jcntrl_mask_point_delete(mask_point);

  field_value_feeder_clean(&xfeeder);
  field_value_feeder_clean(&yfeeder);
  field_value_feeder_clean(&zfeeder);
  grid_data_feeder_clean(&feeder);
  return ret;
}
