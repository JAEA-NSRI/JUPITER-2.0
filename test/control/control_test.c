#include "control_test.h"
#include "test-util.h"

#include <jupiter/control/defs.h>
#include <jupiter/control/error.h>
#include <jupiter/control/information.h>

#define add_test(case_name) \
  {.name = #case_name, .func = test_control_##case_name}

static const char *progname = "control_test";

int main(int argc, char **argv)
{
  jcntrl_error_callback_set(error_handler, NULL);
  return run_test_main(argc, argv, //
                       add_test(util), add_test(grid_data_feeder),

                       add_test(object), add_test(error), add_test(overflow),
                       add_test(extent), add_test(logical_operator),
                       add_test(comparator), add_test(cell),
                       add_test(information), add_test(input), add_test(output),
                       add_test(connection), add_test(data_array),
                       add_test(executive), add_test(manager),
                       add_test(grid_data), add_test(cell_data),
                       add_test(subarray), add_test(static_array),
                       add_test(string_array), add_test(csvparser),
                       add_test(mpi_controller),

                       add_test(postp_mask), add_test(postp_volume_integral),
                       add_test(postp_sum), add_test(postp_pass_arrays),

                       add_test(mask_extent), add_test(mask_point),
                       add_test(mask_lop),

                       add_test(fv_table), add_test(fv_get),

                       add_test(write_fv_csv));
}

int error_handler(void *data, jcntrl_information *errinfo)
{
  const char *src;
  int lno;
  const char *mesg;

  src = jcntrl_information_get_string(errinfo, JCNTRL_INFO_ERROR_SOURCE_FILE);
  lno = jcntrl_information_get_integer(errinfo, JCNTRL_INFO_ERROR_SOURCE_LINE);
  mesg = jcntrl_information_get_string(errinfo, JCNTRL_INFO_ERROR_MESSAGE);
  fprintf(stderr, "ERROR %s\n", mesg);
  fprintf(stderr, "..... from %s (line %d)\n", src, lno);
  return 0;
}
