
#include "control_test.h"
#include "jupiter/control/data_array.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/executive.h"
#include "jupiter/control/grid_data.h"
#include "jupiter/control/information.h"
#include "jupiter/control/input.h"
#include "jupiter/control/mask_data.h"
#include "jupiter/control/mask_extent.h"
#include "jupiter/control/output.h"
#include "jupiter/control/postp_mask.h"
#include "test-util.h"
#include "control_test_grid_data_feeder.h"

#include <string.h>

int test_control_postp_mask(void)
{
  jcntrl_postp_mask *m;
  jcntrl_mask_extent *mext;
  struct grid_data_feeder feeder;
  int ret;

  ret = 0;
  m = NULL;
  mext = NULL;

  grid_data_feeder_init(&feeder);

  do {
    jcntrl_executive *exe;
    jcntrl_input *inp;
    jcntrl_output *outp;
    jcntrl_information *info;
    jcntrl_shared_object *obj;
    jcntrl_grid_data *g;
    jcntrl_mask_data *mdata;
    jcntrl_bool_array *b;

    if (!test_compare_pp((m = jcntrl_postp_mask_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((mext = jcntrl_mask_extent_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(grid_data_feeder_set(&feeder,
                                              ((jcntrl_extent){0, 4, 0, 4, 0,
                                                               3}),
                                              d(({0.0, 0.1, 0.2, 0.3, 0.4})),
                                              d(({0.0, 0.1, 0.2, 0.3, 0.4})),
                                              d(({0.0, 0.1, 0.2, 0.3}))),
                         ==, 1))
      ret = 1;

    jcntrl_mask_extent_set_extent(mext, (int []){0, 2, 0, 1, 2, 3});

    if (!test_compare_pp((inp = jcntrl_postp_mask_get_grid_input(m)), !=, NULL))
      ret = 1;

    exe = &feeder.executive;
    if (!test_compare_pp((outp = jcntrl_executive_output_port(exe, 0)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_input_connect(inp, outp), ==, 1))
      ret = 1;

    if (!test_compare_pp((inp = jcntrl_postp_mask_get_mask_input(m)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((exe = jcntrl_mask_extent_executive(mext)), !=, NULL))
      ret = 1;
    if (ret)
      break;
    if (!test_compare_pp((outp = jcntrl_executive_output_port(exe, 0)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_input_connect(inp, outp), ==, 1))
      ret = 1;

    if (!test_compare_pp((exe = jcntrl_postp_mask_executive(m)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_executive_update(exe), ==, 1)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((outp = jcntrl_executive_output_port(exe, 0)), !=,
                         NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((info = jcntrl_output_information(outp)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp(
          (obj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT)),
          !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((g = jcntrl_grid_data_downcast(obj)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((mdata = jcntrl_grid_data_get_mask(g)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((b = jcntrl_mask_data_array(mdata)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_bool_array_get_ntuple(b), ==, 48)) {
      ret = 1;
      break;
    }

    {
      const char *bb;
      if (!test_compare_pp((bb = jcntrl_bool_array_get(b)), !=, NULL)) {
        ret = 1;
        break;
      }

      if (!test_compare_ii(bb[0 + 4 * (0 + 4 * 0)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[1 + 4 * (0 + 4 * 0)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[2 + 4 * (0 + 4 * 0)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[3 + 4 * (0 + 4 * 0)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[0 + 4 * (1 + 4 * 0)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[1 + 4 * (1 + 4 * 0)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[2 + 4 * (1 + 4 * 0)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[3 + 4 * (1 + 4 * 0)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[0 + 4 * (2 + 4 * 0)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[1 + 4 * (2 + 4 * 0)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[2 + 4 * (2 + 4 * 0)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[3 + 4 * (2 + 4 * 0)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[0 + 4 * (3 + 4 * 0)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[1 + 4 * (3 + 4 * 0)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[2 + 4 * (3 + 4 * 0)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[3 + 4 * (3 + 4 * 0)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[0 + 4 * (0 + 4 * 1)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[1 + 4 * (0 + 4 * 1)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[2 + 4 * (0 + 4 * 1)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[3 + 4 * (0 + 4 * 1)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[0 + 4 * (1 + 4 * 1)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[1 + 4 * (1 + 4 * 1)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[2 + 4 * (1 + 4 * 1)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[3 + 4 * (1 + 4 * 1)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[0 + 4 * (2 + 4 * 1)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[1 + 4 * (2 + 4 * 1)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[2 + 4 * (2 + 4 * 1)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[3 + 4 * (2 + 4 * 1)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[0 + 4 * (3 + 4 * 1)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[1 + 4 * (3 + 4 * 1)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[2 + 4 * (3 + 4 * 1)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[3 + 4 * (3 + 4 * 1)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[0 + 4 * (0 + 4 * 2)], ==, 0)) ret = 1;
      if (!test_compare_ii(bb[1 + 4 * (0 + 4 * 2)], ==, 0)) ret = 1;
      if (!test_compare_ii(bb[2 + 4 * (0 + 4 * 2)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[3 + 4 * (0 + 4 * 2)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[0 + 4 * (1 + 4 * 2)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[1 + 4 * (1 + 4 * 2)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[2 + 4 * (1 + 4 * 2)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[3 + 4 * (1 + 4 * 2)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[0 + 4 * (2 + 4 * 2)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[1 + 4 * (2 + 4 * 2)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[2 + 4 * (2 + 4 * 2)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[3 + 4 * (2 + 4 * 2)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[0 + 4 * (3 + 4 * 2)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[1 + 4 * (3 + 4 * 2)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[2 + 4 * (3 + 4 * 2)], ==, 1)) ret = 1;
      if (!test_compare_ii(bb[3 + 4 * (3 + 4 * 2)], ==, 1)) ret = 1;
    }
  } while (0);

  if (m)
    jcntrl_postp_mask_delete(m);
  if (mext)
    jcntrl_mask_extent_delete(mext);

  grid_data_feeder_clean(&feeder);
  return ret;
}
