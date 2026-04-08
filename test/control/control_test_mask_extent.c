#include "jupiter/control/defs.h"
#include "jupiter/control/executive.h"
#include "jupiter/control/information.h"
#include "jupiter/control/mask_extent.h"
#include "jupiter/control/mask_function.h"
#include "jupiter/control/output.h"
#include "jupiter/control/shared_object.h"
#include "test-util.h"
#include "test/control/control_test.h"

#include <stddef.h>

int test_control_mask_extent(void)
{
  jcntrl_mask_extent *ext;
  jcntrl_mask_extent_function *extf;
  jcntrl_mask_function *mf;
  int ret = 0;

  ext = jcntrl_mask_extent_new();
  if (!test_compare_pp(ext, !=, NULL))
    return 1;

  do {
    jcntrl_output *output;
    jcntrl_shared_object *obj;
    jcntrl_information *info;
    jcntrl_executive *exe;
    const int *extent;

    if (!test_compare_pp((extent = jcntrl_mask_extent_get_extent(ext)), !=,
                         NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(extent[0], ==, 0))
      ret = 1;
    if (!test_compare_ii(extent[1], ==, -1))
      ret = 1;
    if (!test_compare_ii(extent[2], ==, 0))
      ret = 1;
    if (!test_compare_ii(extent[3], ==, -1))
      ret = 1;
    if (!test_compare_ii(extent[4], ==, 0))
      ret = 1;
    if (!test_compare_ii(extent[5], ==, -1))
      ret = 1;

    jcntrl_mask_extent_set_extent(ext, (int[6]){1, 4, 2, 5, 3, 8});
    if (!test_compare_ii(extent[0], ==, 1))
      ret = 1;
    if (!test_compare_ii(extent[1], ==, 4))
      ret = 1;
    if (!test_compare_ii(extent[2], ==, 2))
      ret = 1;
    if (!test_compare_ii(extent[3], ==, 5))
      ret = 1;
    if (!test_compare_ii(extent[4], ==, 3))
      ret = 1;
    if (!test_compare_ii(extent[5], ==, 8))
      ret = 1;

    exe = jcntrl_mask_extent_executive(ext);
    if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
      ret = 1;

    if (!test_compare_pp((output = jcntrl_executive_get_output(exe)), !=,
                         NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((output = jcntrl_output_next_port(output)), !=,
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

    if (!test_compare_ii(jcntrl_shared_object_is_a(jcntrl_mask_extent_function,
                                                   obj),
                         ==, 1))
      ret = 1;

    if (!test_compare_pp((mf =
                            jcntrl_shared_object_downcast(jcntrl_mask_function,
                                                          obj)),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((extf = jcntrl_shared_object_downcast(
                            jcntrl_mask_extent_function, obj)),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp(
          (extent = jcntrl_mask_extent_function_get_extent(extf)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(extent[0], ==, 1))
      ret = 1;
    if (!test_compare_ii(extent[1], ==, 4))
      ret = 1;
    if (!test_compare_ii(extent[2], ==, 2))
      ret = 1;
    if (!test_compare_ii(extent[3], ==, 5))
      ret = 1;
    if (!test_compare_ii(extent[4], ==, 3))
      ret = 1;
    if (!test_compare_ii(extent[5], ==, 8))
      ret = 1;

    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 0, 0, 0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 0, 1, 2), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 1, 2, 3), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 3, 4, 7), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 4, 5, 8), ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 0, 2, 3), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 4, 2, 3), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 3, 1, 7), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 3, 5, 7), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 3, 2, 2), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 3, 2, 8), ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 0, 1, 3), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 4, 5, 3), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 0, 5, 3), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 4, 1, 3), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 0, 2, 2), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 4, 2, 8), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 0, 2, 8), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 4, 2, 2), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 3, 1, 2), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 3, 5, 8), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 3, 1, 8), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 3, 5, 2), ==, 1))
      ret = 1;

    jcntrl_mask_extent_set_extent(ext, (int [6]){2, 3, 4, 5, 6, 7});

    if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
      ret = 1;

    if (!test_compare_pp((extent =
                            jcntrl_mask_extent_function_get_extent(extf)),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(extent[0], ==, 2))
      ret = 1;
    if (!test_compare_ii(extent[1], ==, 3))
      ret = 1;
    if (!test_compare_ii(extent[2], ==, 4))
      ret = 1;
    if (!test_compare_ii(extent[3], ==, 5))
      ret = 1;
    if (!test_compare_ii(extent[4], ==, 6))
      ret = 1;
    if (!test_compare_ii(extent[5], ==, 7))
      ret = 1;

    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 0, 0, 0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 1, 3, 5), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 2, 4, 6), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 3, 5, 7), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 1, 4, 6), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 3, 4, 6), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 2, 3, 6), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 2, 5, 6), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 2, 4, 5), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 2, 4, 7), ==, 1))
      ret = 1;

    jcntrl_mask_extent_set_extent(ext, (int[6]){0, 0, 0, 0, 0, 0});

    if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1))
      ret = 1;

    if (!test_compare_pp((extent =
                            jcntrl_mask_extent_function_get_extent(extf)),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(extent[0], ==, 0))
      ret = 1;
    if (!test_compare_ii(extent[1], ==, 0))
      ret = 1;
    if (!test_compare_ii(extent[2], ==, 0))
      ret = 1;
    if (!test_compare_ii(extent[3], ==, 0))
      ret = 1;
    if (!test_compare_ii(extent[4], ==, 0))
      ret = 1;
    if (!test_compare_ii(extent[5], ==, 0))
      ret = 1;

    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 0, 0, 0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 1, 0, 0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 0, 1, 0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_mask_function_eval(mf, NULL, 0, 0, 1), ==, 1))
      ret = 1;

  } while (0);
  jcntrl_mask_extent_delete(ext);

  return ret;
}
