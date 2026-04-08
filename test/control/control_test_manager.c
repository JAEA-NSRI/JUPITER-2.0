#include <string.h>
#include <stdarg.h>

#include "control_test.h"

#include <jupiter/control/defs.h>
#include <jupiter/control/manager.h>
#include <jupiter/control/information.h>
#include <jupiter/control/executive.h>
#include <jupiter/control/executive_data.h>

#include "test-util.h"
#include "control_test_empty_exec.h"
#include "test/control/control_test_expect_raise.h"

int test_control_manager(void)
{
  struct empty_exec test_executive[5];
  jcntrl_executive_manager *manager;
  jcntrl_executive_manager_entry *entry;

  int ret;
  int i;
  ret = 0;

  manager = jcntrl_executive_manager_new();
  if (!test_compare_pp(manager, !=, NULL))
    return 1;

  for (i = 0; i < 5; ++i) {
    empty_exec_init(&test_executive[i]);
  }

  control_test_use_expect_raise();

  begin_expected_raise();
  if (!test_compare_pp(jcntrl_executive_manager_add(manager, &test_executive[0]
                                                                .executive),
                       ==, 0))
    ret = 1;
  if (test_expect_raise_one(JCNTRL_ERROR_ARGUMENT))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_pp(jcntrl_executive_set_name(&test_executive[0].executive,
                                                 "Kiara"),
                       !=, NULL))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_pp(jcntrl_executive_manager_add(manager, &test_executive[0]
                                                                .executive),
                       ==, 1))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_pp(jcntrl_executive_manager_has(manager, "Kiara"), !=,
                       NULL))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_pp(jcntrl_executive_manager_disown(manager, "Kiara"), ==,
                       &test_executive[0].executive))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_pp(jcntrl_executive_manager_has(manager, "Kiara"), ==,
                       NULL))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_pp((entry = jcntrl_executive_manager_reserve(manager,
                                                                 "Kikkeriki")),
                       !=, NULL))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_pp(jcntrl_executive_get_name(&test_executive[1].executive),
                       ==, NULL))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_pp(jcntrl_executive_manager_bind(entry, &test_executive[1]
                                                               .executive),
                       ==, 1))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_ss(jcntrl_executive_get_name(&test_executive[1].executive),
                       "Kikkeriki"))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_pp((entry = jcntrl_executive_manager_reserve(manager,
                                                                 "Dankeschon")),
                       !=, NULL))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_pp(jcntrl_executive_set_name(&test_executive[2].executive,
                                                 "Dankeschon"),
                       !=, NULL))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_ii(jcntrl_executive_manager_add(manager, &test_executive[2]
                                                                .executive),
                       ==, 1))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_pp(jcntrl_executive_set_name(&test_executive[3].executive,
                                                 "Dankeschon"),
                       !=, NULL))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_executive_manager_add(manager, &test_executive[3]
                                                                .executive),
                       ==, 0))
    ret = 1;
  if (test_expect_raise_one(JCNTRL_ERROR_ARGUMENT_ERROR))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_pp((entry =
                          jcntrl_executive_manager_reserve(manager, "...")),
                       !=, NULL))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  if (!test_compare_ii(jcntrl_executive_manager_bind(entry, &test_executive[3]
                                                               .executive),
                       ==, 1))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  /* Use with leak checker (ex. valgrind or AddressSanitizer) */
  if (!test_compare_pp(jcntrl_executive_manager_reserve(manager,
                                                        "Auf Wiedersehen"),
                       !=, NULL))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  goto cleanup;
cleanup:
  jcntrl_executive_manager_delete(manager);
  for (i = 0; i < 5; ++i) {
    jcntrl_executive_delete(&test_executive[i].executive);
  }
  return ret;
}
