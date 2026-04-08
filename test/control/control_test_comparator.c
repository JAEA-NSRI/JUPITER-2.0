
#include "control_test.h"
#include "jupiter/control/comparator.h"
#include "jupiter/control/defs.h"
#include "test-util.h"

int test_control_comparator(void)
{
  int ret;
  ret = 0;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_LESS, 2.0, 3.0), ==,
                      (2.0 < 3.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_LESS, 3.0, 2.0), ==,
                      (3.0 < 2.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_LESS, 3.0, 3.0), ==,
                      (3.0 < 3.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_LESS_EQ, 2.0, 3.0), ==,
                      (2.0 <= 3.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_LESS_EQ, 3.0, 2.0), ==,
                      (3.0 <= 2.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_LESS_EQ, 3.0, 3.0), ==,
                      (3.0 <= 3.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_EQUAL, 2.0, 3.0), ==,
                      (2.0 == 3.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_EQUAL, 3.0, 2.0), ==,
                      (3.0 == 2.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_EQUAL, 3.0, 3.0), ==,
                      (3.0 == 3.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_NOT_EQ, 2.0, 3.0), ==,
                      (2.0 != 3.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_NOT_EQ, 3.0, 2.0), ==,
                      (3.0 != 2.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_NOT_EQ, 3.0, 3.0), ==,
                      (3.0 != 3.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_GREATER, 2.0, 3.0), ==,
                      (2.0 > 3.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_GREATER, 3.0, 2.0), ==,
                      (3.0 > 2.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_GREATER, 3.0, 3.0), ==,
                      (3.0 > 3.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_GREATER_EQ, 2.0, 3.0), ==,
                      (2.0 >= 3.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_GREATER_EQ, 3.0, 2.0), ==,
                      (3.0 >= 2.0)))
    ret = 1;

  if (!test_compare_ii(jcntrl_compare_d(JCNTRL_COMP_GREATER_EQ, 3.0, 3.0), ==,
                      (3.0 >= 3.0)))
    ret = 1;

  return ret;
}
