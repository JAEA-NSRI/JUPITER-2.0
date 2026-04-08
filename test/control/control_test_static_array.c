#include "control_test.h"
#include "jupiter/control/data_array.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/static_array.h"
#include "test-util.h"

int test_control_static_array(void)
{
  int ret;
  jcntrl_data_array *d1, *d2;
  jcntrl_char_array *achary1;
  jcntrl_int_array *aintary1;
  jcntrl_static_char_array chary1, chary2;
  jcntrl_static_cstr_array cstrary1;
  jcntrl_static_int_array iary1;
  int tmpdata[10] = {0, 1, 5, 9, 2, 8, 3, 4, 6, 7};

  ret = 0;
  achary1 = NULL;
  aintary1 = NULL;

  do {
    char *p1, *p2;
    const char *cp1;
    int *ip1;
    jcntrl_size_type n;

    if (!test_compare_pp((achary1 = jcntrl_char_array_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp((aintary1 = jcntrl_int_array_new()), !=, NULL)) {
      ret = 1;
      break;
    }

    jcntrl_static_char_array_init_cstr(&chary1, "Lipsum");
    jcntrl_static_char_array_init_n(&chary2, 100, 0);

    if (!test_compare_ii(jcntrl_static_char_array_get_ntuple(&chary1), ==,
                         strlen("Lipsum")))
      ret = 1;
    if (!test_compare_ii(jcntrl_static_char_array_get_ntuple(&chary2), ==, 100))
      ret = 1;

    if (!test_compare_pp((p1 = jcntrl_static_char_array_get(&chary1)), !=,
                         NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_static_char_array_get_ntuple(&chary2), >=,
                         jcntrl_static_char_array_get_ntuple(&chary1))) {
      ret = 1;
    }

    if (!test_compare_pp((d1 = jcntrl_static_char_array_data(&chary1)), !=,
                         NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp(jcntrl_char_array_resize(achary1, strlen("Lipsum")),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_char_array_copy(achary1, d1, strlen("Lipsum"),
                                                0, 0),
                         ==, 1))
      ret = 1;

    if (!test_compare_pp((cp1 = jcntrl_char_array_get(achary1)), !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ssn(p1, "Lipsum", strlen("Lipsum")))
      ret = 1;

    if (!test_compare_ssn(cp1, "Lipsum", strlen("Lipsum")))
      ret = 1;

    if (!test_compare_pp(p1, !=, cp1))
      ret = 1;

    if (!test_compare_dd(jcntrl_data_array_get_value(d1, 2), !=, 0.0))
      ret = 1;

    if (!test_compare_ii(p1[2], ==, 'p'))
      ret = 1;

    if (!test_compare_ii(jcntrl_data_array_set_value(d1, 2, 0), ==, 1))
      ret = 1;

    if (!test_compare_dd(jcntrl_data_array_get_value(d1, 2), ==, 0.0))
      ret = 1;

    if (!test_compare_ii(p1[2], ==, 0))
      ret = 1;

    jcntrl_static_cstr_array_init_cstr(&cstrary1, "This boom!");

    if (!test_compare_ii((n = jcntrl_static_cstr_array_get_ntuple(&cstrary1)),
                         ==, 10))
      ret = 1;

    if (!test_compare_pp((d1 = jcntrl_static_cstr_array_data(&cstrary1)),
                         !=, NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_pp(jcntrl_data_array_get_ntuple(d1), ==, 10))
      ret = 1;

    if (!test_compare_pp(jcntrl_data_array_get(d1), !=, NULL))
      ret = 1;

    if (!test_compare_pp(jcntrl_data_array_get_writable(d1), ==, NULL))
      ret = 1;

    if (!test_compare_pp(jcntrl_char_array_resize(achary1, n), ==, achary1)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(jcntrl_char_array_copy(achary1, d1, 4, 0, 0), ==, 1))
      ret = 1;

    if (!test_compare_ssn(jcntrl_char_array_get(achary1), "This", 4))
      ret = 1;


    jcntrl_static_int_array_init_b(&iary1, 0, 9, 2, 4, 8);
    if (!test_compare_ii(jcntrl_static_int_array_get_ntuple(&iary1), ==, 5))
      ret = 1;

    if (!test_compare_pp((ip1 = jcntrl_static_int_array_get(&iary1)), !=,
                         NULL)) {
      ret = 1;
      break;
    }

    if (!test_compare_ii(ip1[0], ==, 0)) ret = 1;
    if (!test_compare_ii(ip1[1], ==, 9)) ret = 1;
    if (!test_compare_ii(ip1[2], ==, 2)) ret = 1;
    if (!test_compare_ii(ip1[3], ==, 4)) ret = 1;
    if (!test_compare_ii(ip1[4], ==, 8)) ret = 1;

    jcntrl_static_int_array_init_base(&iary1, tmpdata, 10);

    if (!test_compare_ii(jcntrl_static_int_array_get_ntuple(&iary1), ==, 10))
      ret = 1;

    if (!test_compare_pp(jcntrl_static_int_array_get(&iary1), ==,
                         (int *)tmpdata))
      ret = 1;

  } while (0);

  if (achary1)
    jcntrl_char_array_delete(achary1);
  if (aintary1)
    jcntrl_int_array_delete(aintary1);

  return ret;
}
