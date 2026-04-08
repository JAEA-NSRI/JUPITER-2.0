
#include <string.h>

#include "control_test.h"

#include <jupiter/control/error.h>
#include <jupiter/control/information.h>
#include <jupiter/control/executive.h>
#include <jupiter/control/input.h>
#include <jupiter/control/executive_data.h>

#include "jupiter/control/defs.h"
#include "jupiter/control/shared_object_priv.h"
#include "jupiter/geometry/util.h"
#include "test-util.h"
#include "control_test_empty_exec.h"

int test_control_input(void)
{
  struct empty_exec test_executive;
  jcntrl_input *input, *fp;
  int ret;
  int i;
  ret = 0;

  empty_exec_init(&test_executive);
  input = jcntrl_executive_get_input(&test_executive.executive);
  JCNTRL_ASSERT(input);

  if (!test_compare_pp(input->owner, ==, &test_executive.executive))
    ret = 1;

  if (!test_compare_pp(jcntrl_input_set_number_of_ports(input, 10), ==, input))
    ret = 1;
  for (fp = jcntrl_input_next_port(input), i = 0; fp;
       fp = jcntrl_input_next_port(fp), ++i)
    /* no body */;
  if (!test_compare_ii(i, ==, 10))
    ret = 1;

  if (!test_compare_pp(jcntrl_input_set_number_of_ports(input, 2), ==, input))
    ret = 1;
  for (fp = jcntrl_input_next_port(input), i = 0; fp;
       fp = jcntrl_input_next_port(fp), ++i)
    /* no body */;
  if (!test_compare_ii(i, ==, 2))
    ret = 1;

  if (!test_compare_pp(jcntrl_input_set_number_of_ports(input, 7), ==, input))
    ret = 1;
  for (fp = jcntrl_input_next_port(input), i = 0; fp;
       fp = jcntrl_input_next_port(fp), ++i)
    /* no body */;
  if (!test_compare_ii(i, ==, 7))
    ret = 1;

  if (!test_compare_pp(jcntrl_input_set_number_of_ports(input, 0), ==, input))
    ret = 1;
  for (fp = jcntrl_input_next_port(input), i = 0; fp;
       fp = jcntrl_input_next_port(fp), ++i)
    /* no body */;
  if (!test_compare_ii(i, ==, 0))
    ret = 1;

  if (!test_compare_pp(jcntrl_input_set_number_of_ports(input, 1), !=, NULL))
    goto cleanup;

  fp = jcntrl_input_next_port(input);
  if ((!test_compare_pp(fp, !=, input)) ||
      (!test_compare_pp(jcntrl_input_rewind(fp), ==, input)))
    ret = 1;

  jcntrl_input_delete(fp);

  for (fp = jcntrl_input_next_port(input), i = 0; fp;
       fp = jcntrl_input_next_port(fp), ++i)
    /* no body */;
  if (!test_compare_ii(i, ==, 0))
    ret = 1;

  if (!test_compare_pp(jcntrl_input_at(input, 0), ==, NULL))
    ret = 1;

  if (!test_compare_pp((fp = jcntrl_input_add(input)), !=, NULL))
    goto cleanup;

  if (!test_compare_pp(jcntrl_input_at(input, 0), ==, fp))
    ret = 1;

  if (!test_compare_pp(jcntrl_input_add(input), !=, NULL)) {
    ret = 1;
    goto cleanup;
  }
  if (!test_compare_pp(jcntrl_input_add(input), !=, NULL)) {
    ret = 1;
    goto cleanup;
  }
  if (!test_compare_pp((fp = jcntrl_input_add(input)), !=, NULL)) {
    ret = 1;
    goto cleanup;
  }

  if (!test_compare_pp(jcntrl_input_at(input, 3), ==, fp))
    ret = 1;

  fp = jcntrl_input_prev_port(fp);
  if ((!test_compare_pp(fp, !=, NULL)) || (!test_compare_pp(fp, !=, input)))
    ret = 1;
  if (!test_compare_pp(jcntrl_input_at(input, 2), ==, fp))
    ret = 1;

cleanup:
  jcntrl_input_set_number_of_ports(input, 0);
  return ret;
}
