#include "rect3d_relp_test.h"
#include "test-util.h"

int main(int argc, char **argv)
{
  return run_test_main(argc, argv, test_entry(make_cdo), test_entry(shuffle),

                       test_entry(rect3d_index_X), test_entry(rect3d_index_Y),
                       test_entry(rect3d_index_Z), test_entry(rect3d_index_C),

                       test_entry(rect3d_axis_X), test_entry(rect3d_axis_Y),
                       test_entry(rect3d_axis_Z),

                       test_entry(rect3d1_relp), test_entry(rect3d2_relp),

                       test_entry(rect3d1_boundary));
}
