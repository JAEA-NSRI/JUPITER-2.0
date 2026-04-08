
#include <string.h>

#include "control_test.h"

#include <jupiter/control/error.h>
#include <jupiter/control/information.h>
#include <jupiter/control/executive.h>
#include <jupiter/control/output.h>
#include <jupiter/control/executive_data.h>

#include "test-util.h"
#include "control_test_empty_exec.h"

int test_control_output(void)
{
  struct empty_exec test_executive;
  jcntrl_output *output, *fp;
  int ret;
  int i;
  ret = 0;

  empty_exec_init(&test_executive);
  output = jcntrl_executive_get_output(&test_executive.executive);
  JCNTRL_ASSERT(output);

  if (!test_compare_pp(output->owner, ==, &test_executive.executive))
    ret = 1;

  if (!test_compare_pp(jcntrl_output_set_number_of_ports(output, 10), ==,
                       output))
    ret = 1;
  for (fp = jcntrl_output_next_port(output), i = 0; fp;
       fp = jcntrl_output_next_port(fp), ++i)
    /* no body */;
  if (!test_compare_ii(i, ==, 10))
    ret = 1;

  if (!test_compare_pp(jcntrl_output_set_number_of_ports(output, 2), ==,
                       output))
    ret = 1;
  for (fp = jcntrl_output_next_port(output), i = 0; fp;
       fp = jcntrl_output_next_port(fp), ++i)
    /* no body */;
  if (!test_compare_ii(i, ==, 2))
    ret = 1;

  if (!test_compare_pp(jcntrl_output_set_number_of_ports(output, 7), ==,
                       output))
    ret = 1;
  for (fp = jcntrl_output_next_port(output), i = 0; fp;
       fp = jcntrl_output_next_port(fp), ++i)
    /* no body */;
  if (!test_compare_ii(i, ==, 7))
    ret = 1;

  if (!test_compare_pp(jcntrl_output_set_number_of_ports(output, 0), ==,
                       output))
    ret = 1;
  for (fp = jcntrl_output_next_port(output), i = 0; fp;
       fp = jcntrl_output_next_port(fp), ++i)
    /* no body */;
  if (!test_compare_ii(i, ==, 0))
    ret = 1;

  if (!test_compare_pp(jcntrl_output_set_number_of_ports(output, 1), !=, NULL))
    goto cleanup;

  fp = jcntrl_output_next_port(output);
  if ((!test_compare_pp(fp, !=, output)) ||
      (!test_compare_pp(jcntrl_output_rewind(fp), ==, output)))
    ret = 1;

  jcntrl_output_delete(fp);

  for (fp = jcntrl_output_next_port(output), i = 0; fp;
       fp = jcntrl_output_next_port(fp), ++i)
    /* no body */;
  if (!test_compare_ii(i, ==, 0))
    ret = 1;

  if (!test_compare_pp(jcntrl_output_at(output, 0), ==, NULL))
    ret = 1;

  if (!test_compare_pp((fp = jcntrl_output_add(output)), !=, NULL))
    goto cleanup;

  if (!test_compare_pp(jcntrl_output_at(output, 0), ==, fp))
    ret = 1;

  if (!test_compare_pp(jcntrl_output_add(output), !=, NULL)) {
    ret = 1;
    goto cleanup;
  }
  if (!test_compare_pp(jcntrl_output_add(output), !=, NULL)) {
    ret = 1;
    goto cleanup;
  }
  if (!test_compare_pp((fp = jcntrl_output_add(output)), !=, NULL)) {
    ret = 1;
    goto cleanup;
  }

  if (!test_compare_pp(jcntrl_output_at(output, 3), ==, fp))
    ret = 1;

  fp = jcntrl_output_prev_port(fp);
  if ((!test_compare_pp(fp, !=, NULL)) || (!test_compare_pp(fp, !=, output)))
    ret = 1;

  if (!test_compare_pp(jcntrl_output_at(output, 2), ==, fp))
    ret = 1;

cleanup:
  jcntrl_output_set_number_of_ports(output, 0);
  return ret;
}
