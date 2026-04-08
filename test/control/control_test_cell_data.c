#include "control_test.h"
#include "control_test_expect_raise.h"

#include "jupiter/control/cell_data.h"
#include "jupiter/control/data_array.h"
#include "jupiter/control/defs.h"
#include "test-util.h"

int test_control_cell_data(void)
{
  int ret;
  jcntrl_cell_data *cdata, *c2;
  jcntrl_data_array *a[5] = {NULL};
  const size_t na = sizeof(a) / sizeof(a[0]);

  ret = 0;
  cdata = jcntrl_cell_data_new();
  if (!cdata)
    return 1;

  control_test_use_expect_raise();
  begin_expected_raise();

  do {
    jcntrl_int_array *i;
    jcntrl_double_array *d;
    if (!test_compare_pp((d = jcntrl_double_array_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((a[0] = jcntrl_double_array_data(d)), !=, NULL)) {
      jcntrl_double_array_delete(d);
      ret = 1;
      break;
    }

    if (!test_compare_pp(jcntrl_double_array_resize(d, 4), !=, NULL))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(!!jcntrl_data_array_set_name(a[0], "t1", -1), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_data_add_array(cdata, a[0]), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp(jcntrl_cell_data_get_array_by_name(cdata, "t1", -1),
                        ==, a[0]))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_cell_data_get_array_by_name(cdata, "t1", 0), ==,
                         NULL))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_cell_data_get_array_by_name(cdata, "t1", 1), ==,
                         NULL))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_cell_data_get_array_by_name(cdata, "t1", 2), ==,
                         a[0]))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    /* This matches because length of name is 2. */
    if (!test_compare_pp(jcntrl_cell_data_get_array_by_name(cdata, "t111", 2),
                         ==, a[0]))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_cell_data_get_array_by_name(cdata, "t2", -1),
                        ==, NULL))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_cell_data_get_array(cdata, 0), ==, a[0]))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp((i = jcntrl_int_array_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((a[1] = jcntrl_int_array_data(i)), !=, NULL)) {
      jcntrl_int_array_delete(i);
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_data_array_set_name(a[1], "t2", -1), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_data_add_array(cdata, a[1]), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp(jcntrl_cell_data_get_array(cdata, 0), ==, a[0]))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_cell_data_get_array(cdata, 1), ==, a[1]))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_cell_data_get_array_by_name(cdata, "t2", -1),
                        ==, a[1]))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_cell_data_get_array_by_name(cdata, "t1", -1),
                        ==, a[0]))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp((i = jcntrl_int_array_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((a[2] = jcntrl_int_array_data(i)), !=, NULL)) {
      jcntrl_int_array_delete(i);
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_data_array_set_name(a[2], "t1", -1), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_data_add_array(cdata, a[2]), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp(jcntrl_cell_data_get_array(cdata, 0), ==, a[2]))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_cell_data_get_array(cdata, 1), ==, a[1]))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_cell_data_get_array_by_name(cdata, "t2", -1),
                        ==, a[1]))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_cell_data_get_array_by_name(cdata, "t1", -1),
                        ==, a[2]))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(cdata), ==, 2))
      ret = 1;

    {
      jcntrl_data_array *p;
      jcntrl_size_type n;

      n = 0;
      jcntrl_cell_data_array_foreach (cdata, p, iter) {
        switch (n) {
        case 0:
          if (!test_compare_pp(p, ==, a[2]))
            ret = 1;
          break;
        case 1:
          if (!test_compare_pp(p, ==, a[1]))
            ret = 1;
          break;
        }
        ++n;
      }
      if (!test_compare_ii(n, ==, 2))
        ret = 1;

      n = 0;
      jcntrl_cell_data_array_reverse_foreach (cdata, p, iter) {
        switch (n) {
        case 1:
          if (!test_compare_pp(p, ==, a[2]))
            ret = 1;
          break;
        case 0:
          if (!test_compare_pp(p, ==, a[1]))
            ret = 1;
          break;
        }
        ++n;
      }

      if (!test_compare_ii(n, ==, 2))
        ret = 1;
    }

    c2 = NULL;
    if (!test_compare_pp((c2 = jcntrl_cell_data_new()), !=, NULL))
      ret = 1;

    do {
      jcntrl_data_array *d1, *d2;
      const char *na1, *na2;
      jcntrl_size_type la1, la2;

      if (!test_compare_ii(jcntrl_cell_data_shallow_copy(c2, cdata), ==, 1))
        ret = 1;

      if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(c2), ==, 2))
        ret = 1;

      if (!test_compare_pp(jcntrl_cell_data_get_array(c2, 0), ==, a[2]))
        ret = 1;
      if (!test_compare_pp(jcntrl_cell_data_get_array(c2, 1), ==, a[1]))
        ret = 1;

      if (!test_compare_ii(jcntrl_cell_data_deep_copy(c2, cdata), ==, 1))
        ret = 1;

      if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(c2), ==, 2))
        ret = 1;

      if (!test_compare_pp((d1 = jcntrl_cell_data_get_array(c2, 0)), !=, a[2]))
        ret = 1;
      if (!test_compare_pp((d2 = jcntrl_cell_data_get_array(c2, 1)), !=, a[1]))
        ret = 1;

      if (!test_compare_pp(jcntrl_data_array_element_type(d1), ==,
                           jcntrl_data_array_element_type(a[2])))
        ret = 1;

      if (!test_compare_pp(jcntrl_data_array_element_type(d2), ==,
                           jcntrl_data_array_element_type(a[1])))
        ret = 1;

      if (!test_compare_ii(jcntrl_data_array_get_ntuple(d1), ==,
                           jcntrl_data_array_get_ntuple(a[2])))
        ret = 1;

      if (!test_compare_ii(jcntrl_data_array_get_ntuple(d2), ==,
                           jcntrl_data_array_get_ntuple(a[1])))
        ret = 1;

      na1 = NULL;
      if (!test_compare_pp((na1 = jcntrl_data_array_name(d1, &la1)), !=, NULL))
        ret = 1;
      if (!test_compare_ii(la1, ==, 2))
        ret = 1;
      if (na1) {
        if (!test_compare_ssn(na1, "t1", 2))
          ret = 1;
      }

      na2 = NULL;
      if (!test_compare_pp((na2 = jcntrl_data_array_name(d2, &la2)), !=, NULL))
        ret = 1;
      if (!test_compare_ii(la2, ==, 2))
        ret = 1;
      if (na1) {
        if (!test_compare_ssn(na2, "t2", 2))
          ret = 1;
      }

    } while (0);

    if (c2)
      jcntrl_cell_data_delete(c2);

    if (!test_compare_ii(jcntrl_cell_data_remove_array(cdata, 0), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_data_remove_array(cdata, 99), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(cdata), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_data_remove_array_by_name(cdata, "t2", -1),
                        ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_data_remove_array_by_name(cdata, "t1", -1),
                        ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(cdata), ==, 0))
      ret = 1;

    jcntrl_cell_data_remove_all_arrays(cdata);
    if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(cdata), ==, 0))
      ret = 1;

    jcntrl_cell_data_add_array(cdata, a[1]);
    jcntrl_cell_data_add_array(cdata, a[2]);
    if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(cdata), ==, 2))
      ret = 1;

    jcntrl_cell_data_remove_all_arrays(cdata);
    if (!test_compare_ii(jcntrl_cell_data_get_number_of_arrays(cdata), ==, 0))
      ret = 1;
  } while (0);

  jcntrl_cell_data_delete(cdata);
  for (size_t i = 0; i < na; ++i) {
    if (a[i])
      jcntrl_data_array_delete(a[i]);
  }
  return ret;
}
