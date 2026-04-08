#include <limits.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <inttypes.h>

#include "control_test.h"

#include <jupiter/control/error.h>
#include <jupiter/control/information.h>
#include <jupiter/control/data_array.h>
#include <time.h>

#include "jupiter/control/defs.h"
#include "jupiter/control/shared_object.h"
#include "test-util.h"
#include "control_test_expect_raise.h"

int test_control_data_array(void)
{
  int ret;
  jcntrl_char_array *chary1;
  jcntrl_char_array *chary2;
  jcntrl_int_array *iary1;
  jcntrl_int_array *iary2;
  jcntrl_bool_array *b1, *b2;
  jcntrl_size_array *s1;
  jcntrl_aint_array *a1;
  jcntrl_double_array *d1;
  jcntrl_data_array *array;
  jcntrl_size_type len;

  ret = 0;
  chary1 = NULL;
  chary2 = NULL;
  iary1 = NULL;
  iary2 = NULL;
  b1 = NULL;
  b2 = NULL;
  s1 = NULL;
  a1 = NULL;
  d1 = NULL;

  control_test_use_expect_raise();

  do {
    if (!test_compare_ii(
          jcntrl_data_array_copyable_tt(jcntrl_data_array_metadata_init(),
                                        jcntrl_size_array_metadata_init()),
          ==, 0))
      ret = 1;

    if (!test_compare_ii(
          jcntrl_data_array_copyable_tt(jcntrl_size_array_metadata_init(),
                                        jcntrl_data_array_metadata_init()),
          ==, 0))
      ret = 1;

    if (!test_compare_ii(jcntrl_data_array_copyable_tt(
                           jcntrl_generic_data_array_metadata_init(),
                           jcntrl_size_array_metadata_init()),
                         ==, 0))
      ret = 1;

    if (!test_compare_ii(jcntrl_data_array_copyable_tt(
                           jcntrl_size_array_metadata_init(),
                           jcntrl_generic_data_array_metadata_init()),
                         ==, 0))
      ret = 1;

    if (!test_compare_ii(
          jcntrl_data_array_copyable_tt(jcntrl_size_array_metadata_init(),
                                        jcntrl_size_array_metadata_init()),
          ==, 1))
      ret = 1;

    if (!test_compare_ii(
          jcntrl_data_array_copyable_tt(jcntrl_char_array_metadata_init(),
                                        jcntrl_size_array_metadata_init()),
          ==, 0))
      ret = 1;
  } while (0);

  do {
    int ierr;
    double *p1;

    if (!test_compare_pp((d1 = jcntrl_double_array_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp(jcntrl_double_array_resize(d1, 15), ==, d1)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((p1 = jcntrl_double_array_get_writable(d1)), !=,
                         NULL)) {
      ret = 1;
      break;
    }

    p1[0] = 5.828493834e-02;
    p1[1] = 7.928552032e-01;
    p1[2] = 1.626710892e-01;
    p1[3] = 8.948800564e-01;
    p1[4] = 6.945621967e-02;
    p1[5] = 9.363824129e-01;
    p1[6] = 8.418110609e-01;
    p1[7] = 1.952597499e-01;
    p1[8] = 3.378502727e-01;
    p1[9] = 2.034163475e-02;
    p1[10] = 8.215856552e-01;
    p1[11] = 8.559100032e-01;
    p1[12] = 3.077151775e-01;
    p1[13] = 8.235719204e-01;
    p1[14] = 3.937567472e-01;

    begin_expected_raise();
    if (!test_compare_pp(jcntrl_double_array_get(d1), ==, p1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp((array = jcntrl_double_array_data(d1)), !=, NULL))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(jcntrl_char_array_copyable(array), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_bool_array_copyable(array), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_int_array_copyable(array), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_double_array_copyable(array), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_size_array_copyable(array), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_aint_array_copyable(array), ==, 0))
      ret = 1;

    if (!test_compare_dd(jcntrl_data_array_get_value(array, 0), ==,
                         5.828493834e-02))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_dd(jcntrl_data_array_get_value(array, 5), ==,
                         9.363824129e-01))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_dd(jcntrl_data_array_get_value(array, 14), ==,
                         3.937567472e-01))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    begin_expected_raise();
    if (!test_compare_dd(jcntrl_data_array_get_value(array, 15), ==, 0.0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_dd(jcntrl_data_array_get_value(array, -1), ==, 0.0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;

    if (!test_compare_dd(jcntrl_double_array_get(d1)[11], ==, 8.559100032e-01))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_data_array_set_value(array, 11,
                                                     4.577016234e-01),
                         ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_dd(jcntrl_double_array_get(d1)[11], ==, 4.577016234e-01))
      ret = 1;

    begin_expected_raise();
    ierr = 0;
    if (!test_compare_ii(jcntrl_data_array_get_ivalue(array, 0, &ierr), ==, 0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_ARGUMENT))
      ret = 1;
    if (!test_compare_ii(ierr, ==, 1))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_data_array_set_ivalue(array, 0, 0), ==, 0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_ARGUMENT))
      ret = 1;

    if (!test_compare_pp((a1 = jcntrl_aint_array_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    do {
      jcntrl_data_array *idxd;
      jcntrl_aint_type iidx[4] = {5, 6, 8, 1};
      if (!test_compare_pp(jcntrl_aint_array_bind(a1, iidx, 4), ==, a1)) {
        ret = 1;
        break;
      }

      if (!test_compare_pp((idxd = jcntrl_aint_array_data(a1)), !=, NULL)) {
        ret = 1;
        break;
      }

      if (!test_compare_ii(jcntrl_data_array_copyidx(array, array, NULL, idxd),
                           ==, 1))
        ret = 1;

      if (!test_compare_dd(p1[0], ==, p1[5]))
        ret = 1;
      if (!test_compare_dd(p1[1], ==, p1[6]))
        ret = 1;
      if (!test_compare_dd(p1[2], ==, p1[8]))
        ret = 1;

      // Note: copy performed in order of specified indices so copyies p1[6] to
      // p1[3] via p1[1].
      if (!test_compare_dd(p1[3], ==, p1[1]))
        ret = 1;

      iidx[3] = -1;
      begin_expected_raise();
      if (!test_compare_ii(jcntrl_data_array_copyidx(array, array, NULL, idxd),
                           ==, 0))
        ret = 1;
      if (test_expect_raise(JCNTRL_ERROR_INDEX))
        ret = 1;
    } while (0);

  } while (0);

  if (a1) {
    jcntrl_aint_array_delete(a1);
    a1 = NULL;
  }

  do {
    int ierr;

    if (!test_compare_pp((s1 = jcntrl_size_array_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp(jcntrl_size_array_resize(s1, 10), ==, s1)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((array = jcntrl_size_array_data(s1)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_char_array_copyable(array), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_bool_array_copyable(array), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_int_array_copyable(array), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_double_array_copyable(array), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_size_array_copyable(array), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_aint_array_copyable(array), ==, 0))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_data_array_set_ivalue(array, 0, 11), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_data_array_set_ivalue(array, -1, 1), ==, 0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_data_array_set_ivalue(array, 99, 1), ==, 0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;

    if (!test_compare_ii(jcntrl_size_array_get(s1)[0], ==, 11))
      ret = 1;
    if (!test_compare_ii(jcntrl_size_array_get(s1)[1], ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_size_array_get(s1)[8], ==, 0))
      ret = 1;

    begin_expected_raise();
    /* In most 64bit architectures, this condition will be false */
    if (JCNTRL_SIZE_MAX < INTMAX_MAX) {
      if (!test_compare_ii(jcntrl_data_array_set_ivalue(array, 8, INTMAX_MAX),
                           ==, 0))
        ret = 1;
      if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
        ret = 1;
    } else {
      if (!test_compare_ii(jcntrl_data_array_set_ivalue(array, 8, INTMAX_MAX),
                           ==, 1))
        ret = 1;
      if (test_expect_not_raised())
        ret = 1;
    }

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_data_array_get_ivalue(array, 0, NULL), ==, 11))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    ierr = 99;
    if (!test_compare_ii(jcntrl_data_array_get_ivalue(array, 8, &ierr), ==,
                         ((JCNTRL_SIZE_MAX < INTMAX_MAX) ? 0 : INTMAX_MAX)))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_ii(ierr, ==, 0))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_data_array_get_ivalue(array, -1, &ierr), ==, 0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;
    if (!test_compare_ii(ierr, ==, 1))
      ret = 1;
  } while (0);

  do {
    if (!test_compare_pp((chary1 = jcntrl_char_array_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((chary2 = jcntrl_char_array_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((array = jcntrl_char_array_data(chary1)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_char_array_copyable(array), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_bool_array_copyable(array), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_int_array_copyable(array), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_double_array_copyable(array), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_size_array_copyable(array), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_aint_array_copyable(array), ==, 0))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_pp(jcntrl_char_array_bind(chary1, "Ninon Joubert", 5), ==,
                         chary1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(jcntrl_char_array_get_ntuple(chary1), ==, 5))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(jcntrl_data_array_get_ntuple(array), ==, 5))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ss(jcntrl_char_array_get(chary1), "Ninon Joubert"))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ss((const char *)jcntrl_data_array_get(array),
                         "Ninon Joubert"))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_char_array_resize(chary1, 15), ==, chary1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ss(jcntrl_char_array_get(chary1), "Ninon"))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_char_array_bind(chary2, "Kurumi", 6), ==,
                         chary2))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp((array = jcntrl_char_array_data(chary2)), !=, NULL))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(jcntrl_char_array_copy(chary1, array, 6, 8, 0), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

#define NINON 'N', 'i', 'n', 'o', 'n'
#define KURUMI 'K', 'u', 'r', 'u', 'm', 'i'
    if (test_compare_bytes(bytes(NINON, 0, 0, 0, KURUMI, 0),
                           jcntrl_char_array_get(chary1)))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_char_array_bind(chary2, "Muimi", 5), ==,
                         chary2))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(jcntrl_char_array_copy(chary1, array, 5, 13, 0), ==,
                         1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (test_compare_bytes(bytes(NINON, 0, 0, 0, 'K', 'u', 'r', 'u', 'm', 'M',
                                 'u'),
                           jcntrl_char_array_get(chary1)))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp((array = jcntrl_char_array_data(chary1)), !=, NULL))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(jcntrl_char_array_copy(chary1, array, 6, 7, 3), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (test_compare_bytes(bytes(NINON, 0, 0, 'o', 'n', 0, 0, 0, 'K', 'M', 'u'),
                           jcntrl_char_array_get(chary1)))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(jcntrl_char_array_copy(chary1, array, 4, 0, 3), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (test_compare_bytes(bytes('o', 'n', 0, 0, 'n', 0, 0, 'o', 'n', 0, 0, 0,
                                 'K', 'M', 'u'),
                           jcntrl_char_array_get(chary1)))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_char_array_resize(chary1, 5), ==, chary1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (test_compare_bytes(bytes('o', 'n', 0, 0, 'n'),
                           jcntrl_char_array_get(chary1)))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_char_array_bind(chary2, "Saren", 5), ==,
                         chary2))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp((array = jcntrl_char_array_data(chary2)), !=, NULL))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_char_array_copy(chary1, array, 5, 11, 0), ==,
                         0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;

    begin_expected_raise();

    if (!test_compare_pp((array = jcntrl_char_array_data(chary1)), !=, NULL))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(jcntrl_data_array_set_name(array, "test", -1), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_data_array_name(array, &len), !=, NULL))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(len, ==, 4))
      ret = 1;
    if (!test_compare_ssn(jcntrl_data_array_name(array, &len), "test", 4))
      ret = 1;

    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(jcntrl_data_array_set_name(array, "test", 2), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_data_array_name(array, &len), !=, NULL))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_ii(len, ==, 2))
      ret = 1;
    if (!test_compare_ssn(jcntrl_data_array_name(array, &len), "te", 2))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_ii(jcntrl_data_array_set_name(array, "tester", 0), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_pp(jcntrl_data_array_name(array, &len), ==, NULL))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_ii(len, ==, 0))
      ret = 1;

    if (!test_compare_pp(jcntrl_char_array_downcast(
                           jcntrl_data_array_object(array)),
                         ==, chary1)) {
      ret = 1;
      break;
    }

    begin_expected_raise();
    if (!test_compare_pp(jcntrl_data_array_resize(array, 15), ==, array))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_data_array_get_ntuple(array), ==, 15))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    begin_expected_raise();

    jcntrl_data_array_delete(array);
    if (test_expect_not_raised())
      ret = 1;

    chary1 = NULL;

    jcntrl_char_array_delete(chary2);
    if (test_expect_not_raised())
      ret = 1;

    chary2 = NULL;
  } while (0);

  do {
    const int *raw_array;
    int rval;
    int tmp[4];
    tmp[0] = 1;
    tmp[1] = 2;

    begin_expected_raise();
    if (!test_compare_pp((iary1 = jcntrl_int_array_new()), !=, NULL)) {
      ret = 1;
      break;
    }
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_pp(jcntrl_int_array_resize(iary1, 10), ==, iary1)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((iary2 = jcntrl_int_array_for(tmp, 2)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((array = jcntrl_int_array_data(iary2)), !=, NULL))
      ret = 1;

    if (!test_compare_ii(jcntrl_int_array_copy(iary1, array, 2, 2, 0), ==, 1))
      ret = 1;

    if (!test_compare_pp((raw_array = jcntrl_int_array_get(iary1)), !=, NULL))
      ret = 1;
    if (!test_compare_ii(raw_array[0], ==, 0))
      ret = 1;
    if (!test_compare_ii(raw_array[1], ==, 0))
      ret = 1;
    if (!test_compare_ii(raw_array[2], ==, 1))
      ret = 1;
    if (!test_compare_ii(raw_array[3], ==, 2))
      ret = 1;
    if (!test_compare_ii(raw_array[4], ==, 0))
      ret = 1;
    if (!test_compare_ii(raw_array[5], ==, 0))
      ret = 1;
    if (!test_compare_ii(raw_array[6], ==, 0))
      ret = 1;
    if (!test_compare_ii(raw_array[7], ==, 0))
      ret = 1;
    if (!test_compare_ii(raw_array[8], ==, 0))
      ret = 1;
    if (!test_compare_ii(raw_array[9], ==, 0))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_dd(jcntrl_data_array_get_value(array, 0), ==, 1.0))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_dd(jcntrl_data_array_get_value(array, 1), ==, 2.0))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    if (!test_compare_dd(jcntrl_data_array_get_value(array, 3), ==, 0.0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_int_array_get(iary2)[0], ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_data_array_set_value(array, 0, 0.0), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_ii(jcntrl_int_array_get(iary2)[0], ==, 0))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_int_array_get(iary2)[1], ==, 2))
      ret = 1;
    if (!test_compare_ii(jcntrl_data_array_set_value(array, 1, 4.0), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_ii(jcntrl_int_array_get(iary2)[1], ==, 4))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_data_array_set_value(array, 3, 99.0), ==, 0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_data_array_get_ivalue(array, 0, NULL), ==, 0))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    rval = 99;
    if (!test_compare_ii(jcntrl_data_array_get_ivalue(array, 1, &rval), ==, 4))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;
    if (!test_compare_ii(rval, ==, 0))
      ret = 1;

    begin_expected_raise();
    rval = 99;
    if (!test_compare_ii(jcntrl_data_array_get_ivalue(array, 2, &rval), ==, 0))
      ret = 1;
    if (test_expect_raise(JCNTRL_ERROR_INDEX))
      ret = 1;
    if (!test_compare_ii(rval, ==, 1))
      ret = 1;

    begin_expected_raise();
    if (!test_compare_ii(jcntrl_data_array_set_ivalue(array, 1, 0), ==, 1))
      ret = 1;
    if (test_expect_not_raised())
      ret = 1;

    begin_expected_raise();
    /*
     * This condition can be assumed to be always true; however, the C standard
     * allows all integral types to be the same bit width, and such
     * architectures or OS may still exist. However, such a system may not be
     * POSIX since POSIX needs char to be 8 bits, and the C standard requires
     * int to be more than 16 bits.
     */
    if (SCHAR_MAX < INTMAX_MAX) {
      if (!test_compare_ii(jcntrl_data_array_set_ivalue(array, 1, INTMAX_MAX),
                           ==, 0))
        ret = 1;
      if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
        ret = 1;
    } else {
      if (!test_compare_ii(jcntrl_data_array_set_ivalue(array, 1, INTMAX_MAX),
                           ==, 1))
        ret = 1;
      if (test_expect_not_raised())
        ret = 1;
    }

  } while (0);

  do {
    jcntrl_data_array *d;
    const char *t1;
    char *t3;
    char t2[10];

    if (!test_compare_pp((b1 = jcntrl_bool_array_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((b2 = jcntrl_bool_array_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp(jcntrl_bool_array_get(b1), ==, NULL))
      ret = 1;

    if (!test_compare_pp(jcntrl_bool_array_resize(b1, 10), ==, b1)) {
      ret = 1;
      break;
    }

    for (int i = 0; i < 10; ++i)
      t2[i] = i;

    if (!test_compare_pp(jcntrl_bool_array_bind(b2, t2, 10), ==, b2))
      ret = 1;

    if (!test_compare_pp((d = jcntrl_bool_array_data(b2)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_bool_array_copy(b1, d, 10, 0, 0), ==, 1))
      ret = 1;

    if (!test_compare_pp((t1 = jcntrl_bool_array_get(b1)), !=, NULL))
      ret = 1;

    if (!test_compare_ii(t1[0], ==, 0))
      ret = 1;
    if (!test_compare_ii(t1[1], ==, 1))
      ret = 1;
    if (!test_compare_ii(t1[2], ==, 2))
      ret = 1;
    if (!test_compare_ii(t1[3], ==, 3))
      ret = 1;
    if (!test_compare_ii(t1[4], ==, 4))
      ret = 1;
    if (!test_compare_ii(t1[5], ==, 5))
      ret = 1;
    if (!test_compare_ii(t1[6], ==, 6))
      ret = 1;
    if (!test_compare_ii(t1[7], ==, 7))
      ret = 1;
    if (!test_compare_ii(t1[8], ==, 8))
      ret = 1;
    if (!test_compare_ii(t1[9], ==, 9))
      ret = 1;

    if (!test_compare_pp((d = jcntrl_bool_array_data(b1)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_bool_array_copy(b1, d, 6, 4, 1), ==, 1))
      ret = 1;

    if (!test_compare_pp((t1 = jcntrl_bool_array_get(b1)), !=, NULL))
      ret = 1;

    if (!test_compare_ii(t1[0], ==, 0))
      ret = 1;
    if (!test_compare_ii(t1[1], ==, 1))
      ret = 1;
    if (!test_compare_ii(t1[2], ==, 2))
      ret = 1;
    if (!test_compare_ii(t1[3], ==, 3))
      ret = 1;
    if (!test_compare_ii(t1[4], ==, 1))
      ret = 1;
    if (!test_compare_ii(t1[5], ==, 2))
      ret = 1;
    if (!test_compare_ii(t1[6], ==, 3))
      ret = 1;
    if (!test_compare_ii(t1[7], ==, 4))
      ret = 1;
    if (!test_compare_ii(t1[8], ==, 5))
      ret = 1;
    if (!test_compare_ii(t1[9], ==, 6))
      ret = 1;

    if (!test_compare_pp((t1 = t2), !=, NULL))
      ret = 1;

    if (!test_compare_ii(t1[0], ==, 0))
      ret = 1;
    if (!test_compare_ii(t1[1], ==, 1))
      ret = 1;
    if (!test_compare_ii(t1[2], ==, 2))
      ret = 1;
    if (!test_compare_ii(t1[3], ==, 3))
      ret = 1;
    if (!test_compare_ii(t1[4], ==, 4))
      ret = 1;
    if (!test_compare_ii(t1[5], ==, 5))
      ret = 1;
    if (!test_compare_ii(t1[6], ==, 6))
      ret = 1;
    if (!test_compare_ii(t1[7], ==, 7))
      ret = 1;
    if (!test_compare_ii(t1[8], ==, 8))
      ret = 1;
    if (!test_compare_ii(t1[9], ==, 9))
      ret = 1;

    if (!test_compare_pp(jcntrl_bool_array_bind(b2, t1, 10), ==, b2))
      ret = 1;

    if (!test_compare_pp(jcntrl_bool_array_get(b2), ==, t1))
      ret = 1;

    if (!test_compare_ii(t1[2], ==, 2))
      ret = 1;
    if (!test_compare_ii(t1[5], ==, 5))
      ret = 1;

    if (!test_compare_pp((t3 = jcntrl_bool_array_get_writable(b2)), !=, t1))
      ret = 1;

    if (!test_compare_ii(t3[2], ==, 2))
      ret = 1;

    if (!test_compare_ii((t3[4] = 1), ==, 1))
      ret = 1;

    if (!test_compare_ii(t2[4], ==, 4))
      ret = 1;

    if (!test_compare_ii(jcntrl_bool_array_get_ntuple(b2), ==, 10))
      ret = 1;

    if (!test_compare_pp((t1 = jcntrl_bool_array_get(b2)), !=, NULL))
      ret = 1;

    if (!test_compare_ii(t1[5], ==, 5))
      ret = 1;

    if (!test_compare_pp(jcntrl_bool_array_get_writable(b2), ==, t1))
      ret = 1;

    if (!test_compare_pp((d = jcntrl_bool_array_data(b2)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((array = jcntrl_data_array_dup(d)), !=, NULL))
      ret = 1;

    if (array) {
      do {
        const char *t3;

        if (!test_compare_ii(jcntrl_data_array_get_ntuple(array), ==, 10))
          ret = 1;

        t3 = NULL;
        if (!test_compare_pp((t3 = jcntrl_data_array_get_bool(array)),
                             !=, NULL))
          ret = 1;

        if (t3) {
          if (!test_compare_ii(memcmp(t1, t3, sizeof(char) * 10), ==, 0))
            ret = 1;
        }

        if (!test_compare_pp(jcntrl_data_array_name(array, NULL), ==, NULL))
          ret = 1;
      } while (0);
      jcntrl_data_array_delete(array);
    }

    if (!test_compare_ii(jcntrl_data_array_set_name(d, "ary1", 4), ==, 1))
      ret = 1;

    if (!test_compare_pp((array = jcntrl_data_array_dup(d)), !=, NULL))
      ret = 1;

    if (array) {
      do {
        jcntrl_size_type l;
        const char *t3;

        if (!test_compare_ii(jcntrl_data_array_get_ntuple(array), ==, 10))
          ret = 1;

        t3 = NULL;
        if (!test_compare_pp((t3 = jcntrl_data_array_get_bool(array)), !=,
                             NULL))
          ret = 1;

        if (t3) {
          if (!test_compare_ii(memcmp(t1, t3, sizeof(char) * 10), ==, 0))
            ret = 1;
        }

        if (!test_compare_pp((t3 = jcntrl_data_array_name(array, &l)), !=,
                             NULL))
          ret = 1;

        if (!test_compare_ssn(t3, "ary1", 4))
          ret = 1;
      } while (0);
      jcntrl_data_array_delete(array);
    }

    if (!test_compare_pp((array = jcntrl_bool_array_data(b2)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_char_array_copyable(array), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_bool_array_copyable(array), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_int_array_copyable(array), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_double_array_copyable(array), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_size_array_copyable(array), ==, 0))
      ret = 1;
    if (!test_compare_ii(jcntrl_aint_array_copyable(array), ==, 0))
      ret = 1;

    if (!test_compare_pp(jcntrl_data_array_get_bool(array), ==, t1))
      ret = 1;

    if (!test_compare_pp(jcntrl_data_array_get_char(array), ==, NULL))
      ret = 1;

    if (!test_compare_pp(jcntrl_data_array_get_writable_bool(array), ==, t1))
      ret = 1;

    if (!test_compare_pp(jcntrl_data_array_get_writable_char(array), ==, NULL))
      ret = 1;

    if (!test_compare_pp(jcntrl_data_array_get_by_meta(
                           array, jcntrl_shared_object_metadata_init()),
                         ==, NULL))
      ret = 1;

    if (!test_compare_pp(jcntrl_data_array_get_by_meta(
                           array, jcntrl_bool_array_metadata_init()),
                         ==, t1))
      ret = 1;
  } while (0);

  do {
    jcntrl_data_array *d;
    const char str[] = "0.21456011111 1xxx 0x4444.333 ";
    char *p;
    jcntrl_size_type l;

    if (!chary1) {
      test_compare_pp((chary1 = jcntrl_char_array_new()), !=, NULL);
      if (!chary1) {
        ret = 1;
        break;
      }
    }

    if (!test_compare_pp(jcntrl_char_array_bind(chary1, str, 4), ==, chary1)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((d = jcntrl_char_array_data(chary1)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((p = jcntrl_char_array_make_cstr(d)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(sizeof("0.21"), ==, 5))
      ret = 1;
    if (test_compare_bytes("0.21", p))
      ret = 1;
    free(p);

    if (!test_compare_pp(jcntrl_char_array_bind(chary1, str, 7), ==, chary1)) {
      ret = 1;
      break;
    }

    l = 0;
    if (!test_compare_eps(jcntrl_char_array_strtod(d, &l), 0.21456, 0.00001))
      ret = 1;
    if (!test_compare_ii(l, ==, 7))
      ret = 1;

    l = 0;
    if (!test_compare_eps(jcntrl_char_array_strtof(d, &l), 0.21456f, 0.00001f))
      ret = 1;
    if (!test_compare_ii(l, ==, 7))
      ret = 1;

    l = 0;
    if (!test_compare_ii(jcntrl_char_array_strtol(d, 0, &l), ==, 0))
      ret = 1;
    if (!test_compare_ii(l, ==, 1))
      ret = 1;

    l = 0;
    if (!test_compare_ii(jcntrl_char_array_strtoll(d, 0, &l), ==, 0))
      ret = 1;
    if (!test_compare_ii(l, ==, 1))
      ret = 1;

    if (!test_compare_pp(jcntrl_char_array_resize(chary1, 0), ==, chary1)) {
      ret = 1;
      break;
    }

    l = 1;
    if (!test_compare_dd(jcntrl_char_array_strtod(d, &l), ==, 0.0))
      ret = 1;
    if (!test_compare_ii(l, ==, 0))
      ret = 1;

    l = 1;
    if (!test_compare_dd(jcntrl_char_array_strtof(d, &l), ==, 0.0f))
      ret = 1;
    if (!test_compare_ii(l, ==, 0))
      ret = 1;

    l = 1;
    if (!test_compare_ii(jcntrl_char_array_strtol(d, 0, &l), ==, 0))
      ret = 1;
    if (!test_compare_ii(l, ==, 0))
      ret = 1;

    l = 1;
    if (!test_compare_ii(jcntrl_char_array_strtoll(d, 0, &l), ==, 0))
      ret = 1;
    if (!test_compare_ii(l, ==, 0))
      ret = 1;

    if (!test_compare_ii(str[15], ==, 'x'))
      ret = 1;

    if (!test_compare_pp(jcntrl_char_array_bind(chary1, &str[14], 3), ==,
                         chary1)) {
      ret = 1;
      break;
    }

    l = 1;
    if (!test_compare_dd(jcntrl_char_array_strtod(d, &l), ==, 1.0))
      ret = 1;
    if (!test_compare_ii(l, ==, 1))
      ret = 1;

    l = 1;
    if (!test_compare_dd(jcntrl_char_array_strtof(d, &l), ==, 1.0f))
      ret = 1;
    if (!test_compare_ii(l, ==, 1))
      ret = 1;

    l = 1;
    if (!test_compare_ii(jcntrl_char_array_strtol(d, 0, &l), ==, 1))
      ret = 1;
    if (!test_compare_ii(l, ==, 1))
      ret = 1;

    l = 1;
    if (!test_compare_ii(jcntrl_char_array_strtoll(d, 0, &l), ==, 1))
      ret = 1;
    if (!test_compare_ii(l, ==, 1))
      ret = 1;

    if (!test_compare_pp(jcntrl_char_array_bind(chary1, &str[18], 12), ==,
                         chary1)) {
      ret = 1;
      break;
    }

    l = 1;
    if (!test_compare_dd(jcntrl_char_array_strtod(d, &l), ==, 0x4444.333p0))
      ret = 1;
    if (!test_compare_ii(l, ==, 11))
      ret = 1;

    l = 1;
    if (!test_compare_dd(jcntrl_char_array_strtof(d, &l), ==, 0x4444.333p0f))
      ret = 1;
    if (!test_compare_ii(l, ==, 11))
      ret = 1;

    l = 1;
    if (!test_compare_ii(jcntrl_char_array_strtol(d, 0, &l), ==, 0x4444))
      ret = 1;
    if (!test_compare_ii(l, ==, 7))
      ret = 1;

    l = 1;
    if (!test_compare_ii(jcntrl_char_array_strtoll(d, 0, &l), ==, 0x4444))
      ret = 1;
    if (!test_compare_ii(l, ==, 7))
      ret = 1;

    l = 1;
    if (!test_compare_ii(jcntrl_char_array_strtoll(d, 10, &l), ==, 0))
      ret = 1;
    if (!test_compare_ii(l, ==, 2))
      ret = 1;

    l = 1;
    if (!test_compare_ii(jcntrl_char_array_strtoll(d, 16, &l), ==, 0x4444))
      ret = 1;
    if (!test_compare_ii(l, ==, 7))
      ret = 1;

  } while (0);

  do {
    jcntrl_data_array *ary1;

    if (chary1)
      jcntrl_char_array_delete(chary1);

    chary1 = NULL;

    if (!test_compare_ii(jcntrl_set_string_c(&chary1, NULL, 0), ==, 1))
      ret = 1;
    if (!test_compare_pp(chary1, ==, NULL))
      ret = 1;

    if (!test_compare_ii(jcntrl_set_string_c(&chary1, "abcd", 4), ==, 1))
      ret = 1;
    if (!test_compare_pp(chary1, !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_char_array_get_ntuple(chary1), ==, 4))
      ret = 1;
    if (!test_compare_ssn(jcntrl_char_array_get(chary1), "abcd", 4))
      ret = 1;

    if (!test_compare_ii(jcntrl_set_string_c(&chary1, "12345", 5), ==, 1))
      ret = 1;
    if (!test_compare_pp(chary1, !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_char_array_get_ntuple(chary1), ==, 5))
      ret = 1;
    if (!test_compare_ssn(jcntrl_char_array_get(chary1), "12345", 5))
      ret = 1;

    if (!test_compare_ii(jcntrl_set_string_c(&chary1, "12345", 0), ==, 1))
      ret = 1;
    if (!test_compare_pp(chary1, !=, NULL))
      ret = 1;
    if (ret)
      break;

    if (!test_compare_ii(jcntrl_char_array_get_ntuple(chary1), ==, 0))
      ret = 1;

    if (!chary2) {
      if (!test_compare_pp((chary2 = jcntrl_char_array_new()), !=, NULL)) {
        ret = 1;
        break;
      }
    }

    if (!test_compare_pp(jcntrl_char_array_resize(chary2, 0), ==, chary2)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((ary1 = jcntrl_char_array_data(chary2)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp(jcntrl_char_array_resize(chary1, 5), ==, chary1))
      ret = 1;

    if (!test_compare_ii(jcntrl_set_string(&chary1, ary1), ==, 1))
      ret = 1;
    if (!test_compare_ii(jcntrl_char_array_get_ntuple(chary1), ==, 0))
      ret = 1;

  } while (0);

  if (chary1)
    jcntrl_char_array_delete(chary1);
  if (chary2)
    jcntrl_char_array_delete(chary2);
  if (iary1)
    jcntrl_int_array_delete(iary1);
  if (iary2)
    jcntrl_int_array_delete(iary2);
  if (b1)
    jcntrl_bool_array_delete(b1);
  if (b2)
    jcntrl_bool_array_delete(b2);
  if (a1)
    jcntrl_aint_array_delete(a1);
  if (s1)
    jcntrl_size_array_delete(s1);
  if (d1)
    jcntrl_double_array_delete(d1);

  jcntrl_error_callback_set(NULL, NULL);
  return ret;
}
