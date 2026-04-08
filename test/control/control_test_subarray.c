
#include "jupiter/control/data_array.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/subarray.h"
#include "test-util.h"
#include "test/control/control_test.h"
#include "test/control/control_test_expect_raise.h"

#include <stddef.h>

int test_control_subarray(void)
{
  int ret = 0;
  jcntrl_double_array *d;
  jcntrl_data_array *a;
  jcntrl_data_subarray *p;

  d = NULL;
  a = NULL;
  p = NULL;

  control_test_use_expect_raise();

  do {
    jcntrl_data_subarray lp;
    do {
      if (!test_compare_pp((d = jcntrl_double_array_new()), !=, NULL)) {
        ret = 1;
        break;
      }

      if (!test_compare_pp((a = jcntrl_double_array_data(d)), !=, NULL))
        ret = 1;

      if (!test_compare_pp(jcntrl_double_array_resize(d, 100), ==, d))
        ret = 1;
    } while (0);
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_data_subarray_static_init(&lp, a, 10, 5), ==,
                         1)) {
      ret = 1;
      break;
    }

    do {
      begin_expected_raise();
      if (!test_compare_ii(jcntrl_data_array_get_ntuple(a), ==, 100))
        ret = 1;
      if (test_expect_not_raised())
        ret = 1;

      if (!test_compare_ii(jcntrl_data_subarray_get_ntuple(&lp), ==, 5))
        ret = 1;
      if (test_expect_not_raised())
        ret = 1;

      if (!test_compare_ii(jcntrl_data_subarray_element_size(&lp), ==,
                           jcntrl_data_array_element_size(a)))
        ret = 1;
      if (test_expect_not_raised())
        ret = 1;

      if (!test_compare_pp(jcntrl_data_subarray_element_type(&lp), ==,
                           jcntrl_double_array_metadata_init()))
        ret = 1;
      if (test_expect_not_raised())
        ret = 1;

      if (!test_compare_poff((const double *)jcntrl_data_subarray_get(&lp),
                             jcntrl_double_array_get(d), 10))
        ret = 1;
      if (test_expect_not_raised())
        ret = 1;

      if (!test_compare_ii(jcntrl_data_array_set_value(a, 12, 5.0), ==, 1))
        ret = 1;
      if (test_expect_not_raised())
        ret = 1;

      if (!test_compare_dd(jcntrl_data_subarray_get_value(&lp, 2), ==, 5.0))
        ret = 1;
      if (test_expect_not_raised())
        ret = 1;

      if (!test_compare_dd(jcntrl_data_subarray_get_value(&lp, 99), ==, 0.0))
        ret = 1;
      if (test_expect_raise(JCNTRL_ERROR_INDEX))
        ret = 1;

      begin_expected_raise();
      if (!test_compare_pp((p = jcntrl_data_subarray_new(a, 999, 1)), ==, NULL))
        ret = 1;
      if (test_expect_raise(JCNTRL_ERROR_INDEX))
        ret = 1;
      if (p)
        break;

      begin_expected_raise();
      if (!test_compare_pp((p = jcntrl_data_subarray_new(a, 0, 999)), ==, NULL))
        ret = 1;
      if (test_expect_raise(JCNTRL_ERROR_INDEX))
        ret = 1;
      if (p)
        break;

      begin_expected_raise();
      if (!test_compare_pp((p = jcntrl_data_subarray_new(a, 7, 20)), !=, NULL))
        ret = 1;
      if (test_expect_not_raised())
        ret = 1;
      if (!p)
        break;

      if (!test_compare_pp((a = jcntrl_data_subarray_data(p)), !=, NULL)) {
        ret = 1;
        break;
      }
      if (test_expect_not_raised())
        ret = 1;

      jcntrl_data_subarray_delete(&lp);

      begin_expected_raise();
      if (!test_compare_ii(jcntrl_data_subarray_static_init(&lp, a, 25, 1), ==,
                           0))
        ret = 1;
      if (test_expect_raise(JCNTRL_ERROR_INDEX))
        ret = 1;

      begin_expected_raise();
      if (!test_compare_ii(jcntrl_data_subarray_static_init(&lp, a, 1, 100), ==,
                           0))
        ret = 1;
      if (test_expect_raise(JCNTRL_ERROR_INDEX))
        ret = 1;

      begin_expected_raise();
      if (!test_compare_ii(jcntrl_data_subarray_static_init(&lp, a, 5, 10), ==,
                           1))
        ret = 1;
      if (test_expect_not_raised())
        ret = 1;

      if (!test_compare_poff((const double *)jcntrl_data_subarray_get(&lp),
                             (const double *)jcntrl_data_subarray_get(p), 5))
        ret = 1;

      if (!test_compare_poff((const double *)jcntrl_data_subarray_get(&lp),
                             jcntrl_double_array_get(d), 12))
        ret = 1;
    } while (0);

    jcntrl_data_subarray_delete(&lp);
  } while (0);

  if (d)
    jcntrl_double_array_delete(d);
  if (p)
    jcntrl_data_subarray_delete(p);

  return ret;
}
