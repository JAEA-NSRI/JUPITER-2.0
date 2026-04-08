
#include "jupiter/control/data_array.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/error.h"
#include "jupiter/control/string_array.h"
#include "test-util.h"
#include "test/control/control_test.h"
#include "test/control/control_test_expect_raise.h"

#include <stddef.h>
#include <time.h>

int test_control_string_array(void)
{
  jcntrl_char_array *c1, *c2;
  jcntrl_string_array *s1, *s2;
  int ret;
  ret = 0;

  control_test_use_expect_raise();

  c1 = c2 = NULL;
  s1 = s2 = NULL;

  do {
    if (!test_compare_pp((c1 = jcntrl_char_array_new()), !=, NULL))
      ret = 1;
    if (!test_compare_pp((c2 = jcntrl_char_array_new()), !=, NULL))
      ret = 1;
    if (!test_compare_pp((s1 = jcntrl_string_array_new()), !=, NULL))
      ret = 1;
    if (!test_compare_pp((s2 = jcntrl_string_array_new()), !=, NULL))
      ret = 1;
  } while (0);

  do {
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_string_array_resize(s1, 10), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_string_array_resize(s2, 15), ==, 1))
      ret = 1;
  } while (0);

  do {
    jcntrl_size_type l;

    if (ret)
      break;

    begin_expected_raise();
    if (!test_compare_pp(jcntrl_string_array_get(s1, 0), ==, NULL))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_pp(jcntrl_string_array_get(s1, 4), ==, NULL))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    begin_expected_raise();
    if (!test_compare_pp(jcntrl_string_array_get(s1, -1), ==, NULL))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_pp(jcntrl_string_array_get(s1, 99), ==, NULL))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_pp(jcntrl_char_array_bind(c1, "Ampere", 6), ==, c1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_ii(jcntrl_string_array_set(s1, 3, c1), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_ii(jcntrl_string_array_set(s1, 4, c2), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(jcntrl_string_array_set(s1, 4, NULL), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_string_array_get(s1, 3), ==, c1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    l = 0;
    begin_expected_raise();
    if (!test_compare_pp(jcntrl_string_array_get_cstr(s1, -1, &l), ==, NULL))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;

    l = 0;
    begin_expected_raise();
    if (!test_compare_pp(jcntrl_string_array_get_cstr(s1, 99, &l), ==, NULL))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;

    l = 0;
    begin_expected_raise();
    if (!test_compare_ssn(jcntrl_string_array_get_cstr(s1, 3, &l), "Ampere", 6))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(l, ==, 6))
      ret = 1;

    if (!test_compare_pp(jcntrl_string_array_get_cstr(s1, 0, &l), ==, NULL))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_string_array_set(s1, -1, c1), ==, 0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;
    begin_expected_raise();
    if (!test_compare_ii(jcntrl_string_array_set(s1, 99, c1), ==, 0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_pp(jcntrl_char_array_bind(c2, "Volta", 5), ==, c2))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
  } while (0);

  do {
    jcntrl_char_array *c3;

    if (ret)
      break;

    c3 = NULL;
    if (!test_compare_pp((c3 = jcntrl_string_array_get_copy(s1, 0)), ==, NULL))
      ret = 1;
    if (c3)
      jcntrl_char_array_delete(c3);

    if (!test_compare_pp((c3 = jcntrl_string_array_get_copy(s1, 3)), !=, NULL))
      ret = 1;
    if (!test_compare_pp(c3, !=, jcntrl_string_array_get(s1, 3)))
      ret = 1;

    if (ret)
      break;

    if (!test_compare_ii(jcntrl_char_array_get_ntuple(c3), ==, 6))
      ret = 1;
    if (!test_compare_ssn(jcntrl_char_array_get(c3), "Ampere", 6))
      ret = 1;

    jcntrl_char_array_delete(c3);
  } while (0);

  do {
    jcntrl_size_type l;
    jcntrl_char_array *t;
    jcntrl_data_array *d;
    char *p;
    const char *q, *r;

    if (ret)
      break;

    if (!test_compare_pp((p = jcntrl_char_array_get_writable(c2)), !=, NULL))
      ret = 1;

    d = NULL;
    if (!test_compare_pp((d = jcntrl_char_array_data(c2)), !=, NULL))
      ret = 1;

    if (ret)
      break;

    if (!test_compare_ii(jcntrl_string_array_set_copy(s1, 6, NULL), ==, 1))
      ret = 1;

    if (!test_compare_ii(jcntrl_string_array_set_copy(s1, 7, d), ==, 1))
      ret = 1;

    p[0] = 'Z';

    if (!test_compare_pp((t = jcntrl_string_array_get(s1, 7)), !=, NULL))
      ret = 1;
    if (!test_compare_pp(t, !=, c2))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_pp((q = jcntrl_char_array_get(t)), !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(q[0], ==, 'V'))
      ret = 1;

    if (!test_compare_ii(jcntrl_string_array_set_cstr(s1, 9, "Einstein", 8), ==,
                         1))
      ret = 1;

    if (!test_compare_pp((t = jcntrl_string_array_get(s1, 9)), !=, NULL))
      ret = 1;

    if (ret)
      break;

    if (!test_compare_ii(jcntrl_char_array_get_ntuple(t), ==, 8))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_string_array_copy(s2, s1, 1, 99, 0), ==, 0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_string_array_copy(s2, s1, 1, 0, 99), ==, 0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_string_array_copy(s2, s1, 1, 99, 99), ==, 0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;

    if (!test_compare_ii(jcntrl_string_array_copy(s2, s1, 10, 0, 3), ==, 1))
      ret = 1;

    if (!test_compare_pp(jcntrl_string_array_get(s2, 0), ==, c1))
      ret = 1;
    if (!test_compare_pp(jcntrl_string_array_get(s2, 3), ==, NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_string_array_get(s2, 4), !=, NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_string_array_get(s2, 4), ==,
                         jcntrl_string_array_get(s1, 7)))
      ret = 1;
    if (!test_compare_pp(jcntrl_string_array_get(s2, 5), ==, NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_string_array_get(s2, 6), !=, NULL))
      ret = 1;
    if (!test_compare_pp(jcntrl_string_array_get(s2, 6), ==,
                         jcntrl_string_array_get(s1, 9)))
      ret = 1;
    if (!test_compare_pp(jcntrl_string_array_get(s2, 7), ==, NULL))
      ret = 1;

    if (!test_compare_ii(jcntrl_string_array_deep_copy(s2, s1, 10, 3, 0), ==,
                         1))
      ret = 1;

    if (!test_compare_pp(jcntrl_string_array_get(s2, 3), ==, NULL))
      ret = 1;

    l = 0;
    if (!test_compare_pp(jcntrl_string_array_get(s2, 6), !=,
                         jcntrl_string_array_get(s1, 3)))
      ret = 1;
    if (!test_compare_ssn(jcntrl_string_array_get_cstr(s2, 6, &l), "Ampere", 6))
      ret = 1;
    if (!test_compare_ii(l, ==, 6))
      ret = 1;

    l = 0;
    if (!test_compare_pp(jcntrl_string_array_get(s2, 10), !=,
                         jcntrl_string_array_get(s1, 7)))
      ret = 1;
    if (!test_compare_ssn(jcntrl_string_array_get_cstr(s2, 10, &l), "Volta", 5))
      ret = 1;
    if (!test_compare_ii(l, ==, 5))
      ret = 1;

    if (!test_compare_pp(jcntrl_string_array_get(s2, 11), ==, NULL))
      ret = 1;

    l = 0;
    if (!test_compare_pp(jcntrl_string_array_get(s2, 12), !=,
                         jcntrl_string_array_get(s1, 9)))
      ret = 1;
    if (!test_compare_ssn(jcntrl_string_array_get_cstr(s2, 12, &l),
                          "Einstein", 8))
      ret = 1;
    if (!test_compare_ii(l, ==, 8))
      ret = 1;

  } while (0);

  do {
    if (ret)
      break;
    if (!test_compare_ii(jcntrl_string_array_resize(s2, 0), ==, 1))
      ret = 1;
  } while (0);

  if (c1)
    jcntrl_char_array_delete(c1);
  if (c2)
    jcntrl_char_array_delete(c2);
  if (s1)
    jcntrl_string_array_delete(s1);
  if (s2)
    jcntrl_string_array_delete(s2);
  return 0;
}
