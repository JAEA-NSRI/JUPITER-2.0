
#include "jupiter/control/defs.h"
#include "jupiter/control/executive.h"
#include "jupiter/control/input.h"
#include "jupiter/control/mask_extent.h"
#include "jupiter/control/mask_function.h"
#include "jupiter/control/mask_lop.h"
#include "control_test.h"
#include "jupiter/control/shared_object_priv.h"
#include "test-util.h"

#include <string.h>

int test_control_mask_lop(void)
{
  jcntrl_mask_extent *p1, *p2;
  jcntrl_mask_lop *mask_lop;
  int ret;
  ret = 0;

  p1 = NULL;
  p2 = NULL;
  mask_lop = NULL;

  do {
    jcntrl_executive *exec;
    jcntrl_input *input;
    jcntrl_output *output;
    jcntrl_mask_lop_function *f;
    jcntrl_mask_function *fun;

    if (!test_compare_pp((p1 = jcntrl_mask_extent_new()), !=, NULL))
      ret = 1;
    if (!test_compare_pp((p2 = jcntrl_mask_extent_new()), !=, NULL))
      ret = 1;
    if (!test_compare_pp((mask_lop = jcntrl_mask_lop_new()), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((exec = jcntrl_mask_lop_executive(mask_lop)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((input = jcntrl_executive_get_input(exec)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp(jcntrl_input_set_number_of_ports(input, 2), !=, NULL))
      ret = 1;

    if (!test_compare_pp((exec = jcntrl_mask_extent_executive(p1)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((output = jcntrl_executive_output_port(exec, 0)), !=,
                         NULL))
      ret = 1;
    if (!test_compare_pp((input = jcntrl_input_next_port(input)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_input_connect(input, output), ==, 1))
      ret = 1;

    if (!test_compare_pp((exec = jcntrl_mask_extent_executive(p2)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((output = jcntrl_executive_output_port(exec, 0)), !=,
                         NULL))
      ret = 1;
    if (!test_compare_pp((input = jcntrl_input_next_port(input)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_input_connect(input, output), ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_mask_lop_set_op(mask_lop, JCNTRL_LOP_INVALID),
                         ==, 0))
      ret = 1;

    if (!test_compare_ii(jcntrl_mask_lop_set_op(mask_lop, JCNTRL_LOP_OR), ==,
                         1))
      ret = 1;

    jcntrl_mask_extent_set_extent(p1, (int[]){0, 5, 2, 8, 3, 11});
    jcntrl_mask_extent_set_extent(p2, (int[]){3, 7, 5, 15, 8, 19});

    if (!test_compare_pp((exec = jcntrl_mask_lop_executive(mask_lop)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_executive_update(exec), ==, 1))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((f = jcntrl_executive_get_output_data_object_as(
                            exec, 0, jcntrl_mask_lop_function)),
                         !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((fun = jcntrl_mask_lop_function_function(f)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_mask_function_eval(fun, NULL, 0, 0, 0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(fun, NULL, 0, 2, 3), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(fun, NULL, 4, 7, 9), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(fun, NULL, 6, 9, 13), ==, 1))
      ret = 1;


    if (!test_compare_ii(jcntrl_mask_lop_set_op(mask_lop, JCNTRL_LOP_AND), ==,
                         1))
      ret = 1;

    jcntrl_mask_extent_set_extent(p1, (int[]){0, 5, 2, 8, 3, 11});
    jcntrl_mask_extent_set_extent(p2, (int[]){3, 7, 5, 15, 8, 19});

    if (!test_compare_pp((exec = jcntrl_mask_lop_executive(mask_lop)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_executive_update(exec), ==, 1))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((f = jcntrl_executive_get_output_data_object_as(
                            exec, 0, jcntrl_mask_lop_function)),
                         !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((fun = jcntrl_mask_lop_function_function(f)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_mask_function_eval(fun, NULL, 0, 0, 0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(fun, NULL, 0, 2, 3), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(fun, NULL, 4, 7, 9), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(fun, NULL, 6, 9, 13), ==, 0))
      ret = 1;

    if (!test_compare_ii(jcntrl_mask_lop_set_op(mask_lop, JCNTRL_LOP_EQV), ==,
                         1))
      ret = 1;

    jcntrl_mask_extent_set_extent(p1, (int[]){0, 5, 2, 8, 3, 11});
    jcntrl_mask_extent_set_extent(p2, (int[]){3, 7, 5, 15, 8, 19});

    if (!test_compare_pp((exec = jcntrl_mask_lop_executive(mask_lop)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_executive_update(exec), ==, 1))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((f = jcntrl_executive_get_output_data_object_as(
                            exec, 0, jcntrl_mask_lop_function)),
                         !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((fun = jcntrl_mask_lop_function_function(f)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_mask_function_eval(fun, NULL, 0, 0, 0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(fun, NULL, 0, 2, 3), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(fun, NULL, 4, 7, 9), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(fun, NULL, 6, 9, 13), ==, 0))
      ret = 1;


    if (!test_compare_ii(jcntrl_mask_lop_set_op(mask_lop, JCNTRL_LOP_SUB), ==,
                         1))
      ret = 1;

    jcntrl_mask_extent_set_extent(p1, (int[]){0, 5, 2, 8, 3, 11});
    jcntrl_mask_extent_set_extent(p2, (int[]){3, 7, 5, 15, 8, 19});

    if (!test_compare_pp((exec = jcntrl_mask_lop_executive(mask_lop)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_executive_update(exec), ==, 1))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((f = jcntrl_executive_get_output_data_object_as(
                            exec, 0, jcntrl_mask_lop_function)),
                         !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((fun = jcntrl_mask_lop_function_function(f)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_mask_function_eval(fun, NULL, 0, 0, 0), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(fun, NULL, 0, 2, 3), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(fun, NULL, 4, 7, 9), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(fun, NULL, 6, 9, 13), ==, 1))
      ret = 1;

  } while (0);

  if (p1)
    jcntrl_mask_extent_delete(p1);
  if (p2)
    jcntrl_mask_extent_delete(p2);
  if (mask_lop)
    jcntrl_mask_lop_delete(mask_lop);
  return ret;
}
