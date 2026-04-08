#include <string.h>
#include <stdarg.h>

#include "control_test.h"

#include <jupiter/control/error.h>
#include <jupiter/control/information.h>
#include <jupiter/control/data_array.h>
#include <jupiter/control/defs.h>
#include <jupiter/control/grid_data.h>

#include "jupiter/control/cell_data.h"
#include "jupiter/control/executive.h"
#include "jupiter/control/output.h"
#include "jupiter/control/shared_object.h"
#include "jupiter/control/struct_grid.h"
#include "test-util.h"
#include "control_test_expect_raise.h"
#include "test/control/control_test_grid_data_feeder.h"

int test_control_grid_data(void)
{
  struct grid_data_feeder feeder;
  jcntrl_grid_data *grid, *g2;
  jcntrl_data_array *d;
  jcntrl_double_array *array;
  int ret;

  if (!test_compare_ii(grid_data_feeder_init(&feeder), ==, 1))
    return 1;

  ret = 0;
  grid = NULL;
  g2 = NULL;
  array = NULL;

  do {
    if (!test_compare_pp((grid = jcntrl_grid_data_new()), !=, NULL)) {
      ret = 1;
      break;
    }
    if (!test_compare_pp((array = jcntrl_double_array_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    control_test_use_expect_raise();
    begin_expected_raise();

    if (!test_compare_pp(jcntrl_double_array_resize(array, 4), ==, array))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    jcntrl_grid_data_set_extent(grid, (int[]){0, 3, 2, 4, 1, 6});
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp((d = jcntrl_double_array_data(array)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_data_array_set_name(d, "name", 1), ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_grid_data_extent(grid)[0], ==, 0))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_ii(jcntrl_grid_data_extent(grid)[1], ==, 3))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_ii(jcntrl_grid_data_extent(grid)[2], ==, 2))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_ii(jcntrl_grid_data_extent(grid)[3], ==, 4))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_ii(jcntrl_grid_data_extent(grid)[4], ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_ii(jcntrl_grid_data_extent(grid)[5], ==, 6))
      ret = 1;

    jcntrl_grid_data_set_x_coords(grid, d);
    if (test_expect_not_raised())
      ret = 1;
    jcntrl_grid_data_set_y_coords(grid, d);
    if (test_expect_not_raised())
      ret = 1;
    jcntrl_grid_data_set_z_coords(grid, d);
    if (test_expect_not_raised())
      ret = 1;

    /* Keep ownership for feeder.cell_arrays. */
    jcntrl_data_array_take_ownership(d);
    feeder.cell_arrays = &d;
    feeder.ncell_arrays = 1;
    grid_data_feeder_set_local_extent(&feeder, (int[]){0, 0, 1, 1, 3, 3});
    grid_data_feeder_set_whole_extent(&feeder, (int[]){-2, 1, 0, 5, 3, 4});
    jcntrl_struct_grid_set_extent(&feeder.struct_grid,
                                  (int[]){0, 1, 2, 3, 3, 4});
    jcntrl_struct_grid_set_x_coords(&feeder.struct_grid, d);
    jcntrl_struct_grid_set_y_coords(&feeder.struct_grid, d);
    jcntrl_struct_grid_set_z_coords(&feeder.struct_grid, d);

    if (!test_compare_ii(jcntrl_executive_update_information(&feeder.executive),
                         ==, 1))
      ret = 1;

    do {
      jcntrl_output *outp;
      jcntrl_information *info;

      outp = jcntrl_executive_get_output(&feeder.executive);
      if (!test_compare_pp((outp = jcntrl_output_next_port(outp)), !=, NULL)) {
        ret = 1;
        break;
      }

      if (!test_compare_pp((info = jcntrl_output_information(outp)), !=,
                           NULL)) {
        ret = 1;
        break;
      }

      do {
        if (!test_compare_pp(
              jcntrl_information_get_extent(info, JCNTRL_INFO_WHOLE_EXTENT), ==,
              NULL)) {
          ret = 1;
          break;
        }
      } while (0);
    } while (0);

    if (!test_compare_ii(jcntrl_executive_update_extent(&feeder.executive), ==,
                         1))
      ret = 1;

    do {
      jcntrl_output *outp;
      jcntrl_information *info;
      const int *ext;

      outp = jcntrl_executive_get_output(&feeder.executive);
      if (!test_compare_pp((outp = jcntrl_output_next_port(outp)), !=, NULL)) {
        ret = 1;
        break;
      }

      if (!test_compare_pp((info = jcntrl_output_information(outp)), !=,
                           NULL)) {
        ret = 1;
        break;
      }

      do {
        if (!test_compare_pp((ext = jcntrl_information_get_extent(
                                info, JCNTRL_INFO_WHOLE_EXTENT)),
                             !=, NULL)) {
          ret = 1;
          break;
        }

        if (!test_compare_ii(ext[0], ==, -2))
          ret = 1;
        if (!test_compare_ii(ext[1], ==, 1))
          ret = 1;
        if (!test_compare_ii(ext[2], ==, 0))
          ret = 1;
        if (!test_compare_ii(ext[3], ==, 5))
          ret = 1;
        if (!test_compare_ii(ext[4], ==, 3))
          ret = 1;
        if (!test_compare_ii(ext[5], ==, 4))
          ret = 1;
      } while (0);

      do {
        if (!test_compare_pp((ext = jcntrl_information_get_extent(
                                info, JCNTRL_INFO_DATA_EXTENT)),
                             !=, NULL)) {
          ret = 1;
          break;
        }

        if (!test_compare_ii(ext[0], ==, 0))
          ret = 1;
        if (!test_compare_ii(ext[1], ==, 1))
          ret = 1;
        if (!test_compare_ii(ext[2], ==, 2))
          ret = 1;
        if (!test_compare_ii(ext[3], ==, 3))
          ret = 1;
        if (!test_compare_ii(ext[4], ==, 3))
          ret = 1;
        if (!test_compare_ii(ext[5], ==, 4))
          ret = 1;
      } while (0);

      do {
        if (!test_compare_pp((ext = jcntrl_information_get_extent(
                                info, JCNTRL_INFO_PIECE_EXTENT)),
                             !=, NULL)) {
          ret = 1;
          break;
        }

        if (!test_compare_ii(ext[0], ==, 0))
          ret = 1;
        if (!test_compare_ii(ext[1], ==, 0))
          ret = 1;
        if (!test_compare_ii(ext[2], ==, 1))
          ret = 1;
        if (!test_compare_ii(ext[3], ==, 1))
          ret = 1;
        if (!test_compare_ii(ext[4], ==, 3))
          ret = 1;
        if (!test_compare_ii(ext[5], ==, 3))
          ret = 1;
      } while (0);
    } while (0);

    if (!test_compare_ii(jcntrl_executive_update(&feeder.executive), ==, 1))
      ret = 1;

    do {
      jcntrl_output *outp;
      jcntrl_information *info;
      jcntrl_grid_data *gp;
      const int *ext;

      outp = jcntrl_executive_get_output(&feeder.executive);
      if (!test_compare_pp((outp = jcntrl_output_next_port(outp)), !=, NULL)) {
        ret = 1;
        break;
      }

      if (!test_compare_pp((info = jcntrl_output_information(outp)), !=,
                           NULL)) {
        ret = 1;
        break;
      }

      do {
        if (!test_compare_pp((ext = jcntrl_information_get_extent(
                                info, JCNTRL_INFO_WHOLE_EXTENT)),
                             !=, NULL)) {
          ret = 1;
          break;
        }

        if (!test_compare_ii(ext[0], ==, -2))
          ret = 1;
        if (!test_compare_ii(ext[1], ==, 1))
          ret = 1;
        if (!test_compare_ii(ext[2], ==, 0))
          ret = 1;
        if (!test_compare_ii(ext[3], ==, 5))
          ret = 1;
        if (!test_compare_ii(ext[4], ==, 3))
          ret = 1;
        if (!test_compare_ii(ext[5], ==, 4))
          ret = 1;
      } while (0);

      do {
        if (!test_compare_pp((ext = jcntrl_information_get_extent(
                                info, JCNTRL_INFO_DATA_EXTENT)),
                             !=, NULL)) {
          ret = 1;
          break;
        }

        if (!test_compare_ii(ext[0], ==, 0))
          ret = 1;
        if (!test_compare_ii(ext[1], ==, 1))
          ret = 1;
        if (!test_compare_ii(ext[2], ==, 2))
          ret = 1;
        if (!test_compare_ii(ext[3], ==, 3))
          ret = 1;
        if (!test_compare_ii(ext[4], ==, 3))
          ret = 1;
        if (!test_compare_ii(ext[5], ==, 4))
          ret = 1;
      } while (0);

      do {
        if (!test_compare_pp((ext = jcntrl_information_get_extent(
                                info, JCNTRL_INFO_PIECE_EXTENT)),
                             !=, NULL)) {
          ret = 1;
          break;
        }

        if (!test_compare_ii(ext[0], ==, 0))
          ret = 1;
        if (!test_compare_ii(ext[1], ==, 0))
          ret = 1;
        if (!test_compare_ii(ext[2], ==, 1))
          ret = 1;
        if (!test_compare_ii(ext[3], ==, 1))
          ret = 1;
        if (!test_compare_ii(ext[4], ==, 3))
          ret = 1;
        if (!test_compare_ii(ext[5], ==, 3))
          ret = 1;
      } while (0);

      do {
        jcntrl_cell_data *cdata;
        jcntrl_data_array *a;
        jcntrl_shared_object *obj;
        if (!test_compare_pp((obj = jcntrl_information_get_object(
                                info, JCNTRL_INFO_DATA_OBJECT)),
                             !=, NULL)) {
          ret = 1;
          break;
        }

        if (!test_compare_pp((gp = jcntrl_grid_data_downcast(obj)), !=, NULL)) {
          ret = 1;
          break;
        }

        if (!test_compare_ii(jcntrl_grid_data_extent(gp)[0], ==, 0))
          ret = 1;
        if (!test_compare_ii(jcntrl_grid_data_extent(gp)[1], ==, 1))
          ret = 1;
        if (!test_compare_ii(jcntrl_grid_data_extent(gp)[2], ==, 2))
          ret = 1;
        if (!test_compare_ii(jcntrl_grid_data_extent(gp)[3], ==, 3))
          ret = 1;
        if (!test_compare_ii(jcntrl_grid_data_extent(gp)[4], ==, 3))
          ret = 1;
        if (!test_compare_ii(jcntrl_grid_data_extent(gp)[5], ==, 4))
          ret = 1;

        if (!test_compare_pp((cdata = jcntrl_grid_data_cell_data(gp)), !=,
                             NULL)) {
          ret = 1;
          break;
        }

        if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(cdata), ==,
                             1))
          ret = 1;

        if (!test_compare_pp(jcntrl_cell_data_get_array(cdata, 0), ==, d))
          ret = 1;
      } while (0);
    } while (0);
  } while (0);

  do {
    const int *ext1, *ext2;
    jcntrl_cell_data *c1, *c2;
    jcntrl_data_array *d1, *d2;

    if (ret)
      break;

    if (!test_compare_pp((g2 = jcntrl_grid_data_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    do {
      jcntrl_char_array *ch1;
      jcntrl_data_array *d1;
      jcntrl_size_type ntuple;

      if (!test_compare_pp((ch1 = jcntrl_char_array_new()), !=, NULL)) {
        ret = 1;
        break;
      }

      d1 = NULL;
      c1 = NULL;
      if (!test_compare_pp((d1 = jcntrl_char_array_data(ch1)), !=, NULL))
        ret = 1;
      if (!test_compare_pp((c1 = jcntrl_grid_data_cell_data(grid)), !=, NULL))
        ret = 1;
      if (ret) {
        jcntrl_char_array_delete(ch1);
        break;
      }

      if (!test_compare_ii(jcntrl_data_array_set_name(d1, "test", 4), ==, 1))
        ret = 1;
      if (!test_compare_ii(jcntrl_cell_data_add_array(c1, d1), ==, 1))
        ret = 1;

      jcntrl_shared_object_release_ownership(jcntrl_data_array_object(d1));
    } while (0);
    if (ret)
      break;

    d1 = NULL;

    if (!test_compare_ii(jcntrl_grid_data_shallow_copy(g2, grid), ==, 1))
      ret = 1;
    if (ret)
      break;

    ext1 = NULL;
    ext2 = NULL;
    if (!test_compare_pp((ext1 = jcntrl_grid_data_extent(grid)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((ext2 = jcntrl_grid_data_extent(g2)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(ext2[0], ==, ext1[0]))
      ret = 1;
    if (!test_compare_ii(ext2[1], ==, ext1[1]))
      ret = 1;
    if (!test_compare_ii(ext2[2], ==, ext1[2]))
      ret = 1;
    if (!test_compare_ii(ext2[3], ==, ext1[3]))
      ret = 1;
    if (!test_compare_ii(ext2[4], ==, ext1[4]))
      ret = 1;
    if (!test_compare_ii(ext2[5], ==, ext1[5]))
      ret = 1;

    c1 = c2 = NULL;
    if (!test_compare_pp((c1 = jcntrl_grid_data_cell_data(grid)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((c2 = jcntrl_grid_data_cell_data(g2)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(c1), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(c2), ==, 1))
      ret = 1;

    if (!test_compare_pp((d1 = jcntrl_cell_data_get_array(c1, 0)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((d2 = jcntrl_cell_data_get_array(c2, 0)), !=, NULL))
      ret = 1;

    if (!test_compare_pp(d2, ==, d1))
      ret = 1;

    if (!test_compare_pp((d1 = jcntrl_grid_data_x_coords(grid)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((d2 = jcntrl_grid_data_x_coords(g2)), !=, NULL))
      ret = 1;

    if (!test_compare_pp(d2, ==, d1))
      ret = 1;

    if (!test_compare_pp((d1 = jcntrl_grid_data_y_coords(grid)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((d2 = jcntrl_grid_data_y_coords(g2)), !=, NULL))
      ret = 1;

    if (!test_compare_pp(d2, ==, d1))
      ret = 1;

    if (!test_compare_pp((d1 = jcntrl_grid_data_z_coords(grid)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((d2 = jcntrl_grid_data_z_coords(g2)), !=, NULL))
      ret = 1;

    if (!test_compare_pp(d2, ==, d1))
      ret = 1;

    // deep copy
    if (!test_compare_ii(jcntrl_grid_data_deep_copy(g2, grid), ==, 1))
      ret = 1;
    if (ret)
      break;

    ext1 = NULL;
    ext2 = NULL;
    if (!test_compare_pp((ext1 = jcntrl_grid_data_extent(grid)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((ext2 = jcntrl_grid_data_extent(g2)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(ext2[0], ==, ext1[0]))
      ret = 1;
    if (!test_compare_ii(ext2[1], ==, ext1[1]))
      ret = 1;
    if (!test_compare_ii(ext2[2], ==, ext1[2]))
      ret = 1;
    if (!test_compare_ii(ext2[3], ==, ext1[3]))
      ret = 1;
    if (!test_compare_ii(ext2[4], ==, ext1[4]))
      ret = 1;
    if (!test_compare_ii(ext2[5], ==, ext1[5]))
      ret = 1;

    c1 = c2 = NULL;
    if (!test_compare_pp((c1 = jcntrl_grid_data_cell_data(grid)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((c2 = jcntrl_grid_data_cell_data(g2)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(c1), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(c2), ==, 1))
      ret = 1;

    if (!test_compare_pp((d1 = jcntrl_cell_data_get_array(c1, 0)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((d2 = jcntrl_cell_data_get_array(c2, 0)), !=, NULL))
      ret = 1;

    if (!test_compare_pp(d2, !=, d1))
      ret = 1;
    if (!test_compare_pp(jcntrl_data_array_element_type(d2), ==,
                         jcntrl_data_array_element_type(d1)))
      ret = 1;
    if (!test_compare_ii(jcntrl_data_array_get_ntuple(d2), ==,
                         jcntrl_data_array_get_ntuple(d1)))
      ret = 1;

    if (!test_compare_pp((d1 = jcntrl_grid_data_x_coords(grid)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((d2 = jcntrl_grid_data_x_coords(g2)), !=, NULL))
      ret = 1;

    if (!test_compare_pp(d2, !=, d1))
      ret = 1;
    if (!test_compare_pp(jcntrl_data_array_element_type(d2), ==,
                         jcntrl_data_array_element_type(d1)))
      ret = 1;
    if (!test_compare_ii(jcntrl_data_array_get_ntuple(d2), ==,
                         jcntrl_data_array_get_ntuple(d1)))
      ret = 1;

    if (!test_compare_pp((d1 = jcntrl_grid_data_y_coords(grid)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((d2 = jcntrl_grid_data_y_coords(g2)), !=, NULL))
      ret = 1;

    if (!test_compare_pp(d2, !=, d1))
      ret = 1;
    if (!test_compare_pp(jcntrl_data_array_element_type(d2), ==,
                         jcntrl_data_array_element_type(d1)))
      ret = 1;
    if (!test_compare_ii(jcntrl_data_array_get_ntuple(d2), ==,
                         jcntrl_data_array_get_ntuple(d1)))
      ret = 1;

    if (!test_compare_pp((d1 = jcntrl_grid_data_z_coords(grid)), !=, NULL))
      ret = 1;
    if (!test_compare_pp((d2 = jcntrl_grid_data_z_coords(g2)), !=, NULL))
      ret = 1;

    if (!test_compare_pp(d2, !=, d1))
      ret = 1;
    if (!test_compare_pp(jcntrl_data_array_element_type(d2), ==,
                         jcntrl_data_array_element_type(d1)))
      ret = 1;
    if (!test_compare_ii(jcntrl_data_array_get_ntuple(d2), ==,
                         jcntrl_data_array_get_ntuple(d1)))
      ret = 1;
  } while (0);

  if (grid)
    jcntrl_grid_data_delete(grid);
  if (g2)
    jcntrl_grid_data_delete(g2);
  if (array)
    jcntrl_double_array_delete(array);

  grid_data_feeder_clean(&feeder);
  return ret;
}
