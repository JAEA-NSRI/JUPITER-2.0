#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <inttypes.h>
#include <stdlib.h>

#include "jupiter/component_data.h"
#include "jupiter/geometry/list.h"
#include "test-util.h"

#include "jupiter/csv.h"
#include "jupiter/csvutil.h"
#include "jupiter/component_info.h"

int main(int argc, char **argv)
{
  int r = 0;
  int i, icomm;
  int diff, base;
  ptrdiff_t addr;
  struct component_data d, ary[15];
  struct component_info c1, c2, c3;
  component_info_init(&c1);
  component_info_init(&c2);
  component_info_init(&c3);

  component_data_init(&d);
  for (int i = 0; i < 15; ++i) {
    component_data_init(&ary[i]);
    if (i >= 1) {
      if (i > 1) {
        ary[i].jupiter_id = i - 1;
        ary[i].comp_index = i - 1;
      } else {
        ary[i].jupiter_id = -1;
        ary[i].comp_index = -1;
      }
      geom_list_insert_next(&ary[i - 1].list, &ary[i].list);
    }
  }

  if (!component_info_resize(&c1, 1, 0)) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return EXIT_FAILURE;
  }

  if (test_compare(component_info_ncompo(&c1), 1))
    r = 1;
  if (test_compare(component_info_ncompo(&c2), 0))
    r = 1;

  component_info_clear(&c2);
  if (test_compare(component_info_ncompo(&c2), 0))
    r = 1;

  component_info_seti(&c1, 0, 0);
  if (test_compare(component_info_geti(&c1, 0), 0))
    r = 1;

  component_info_seti(&c1, 0, 1);
  if (test_compare(component_info_geti(&c1, 0), 1))
    r = 1;

  component_info_setc(&c1, 0, &d);
  if (test_compare(component_info_getc(&c1, 0), &d))
    r = 1;

  component_info_setc(&c1, 0, NULL);
  if (test_compare(component_info_getc(&c1, 0), NULL))
    r = 1;

  component_info_setc(&c1, 0, &d);
  component_info_seti(&c1, 0, 1);
  if (test_compare(component_info_getc(&c1, 0), NULL))
    r = 1;

  if (!component_info_resize(&c1, 10, 1)) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return EXIT_FAILURE;
  }

  if (test_compare(component_info_geti(&c1, 0), 1))
    r = 1;

  component_info_seti(&c1, 0, 0);
  component_info_seti(&c1, 1, 22);
  component_info_seti(&c1, 2, 1);
  component_info_seti(&c1, 3, 7);
  component_info_seti(&c1, 4, 1);
  component_info_seti(&c1, 5, 2);
  component_info_seti(&c1, 6, 3);
  component_info_seti(&c1, 7, 7);
  component_info_seti(&c1, 8, 8);
  component_info_seti(&c1, 9, 2);

  component_info_sort(&c1);
  if (test_compare(component_info_geti(&c1, 0), 0))
    r = 1;
  if (test_compare(component_info_geti(&c1, 1), 1))
    r = 1;
  if (test_compare(component_info_geti(&c1, 2), 1))
    r = 1;
  if (test_compare(component_info_geti(&c1, 3), 2))
    r = 1;
  if (test_compare(component_info_geti(&c1, 4), 2))
    r = 1;
  if (test_compare(component_info_geti(&c1, 5), 3))
    r = 1;
  if (test_compare(component_info_geti(&c1, 6), 7))
    r = 1;
  if (test_compare(component_info_geti(&c1, 7), 7))
    r = 1;
  if (test_compare(component_info_geti(&c1, 8), 8))
    r = 1;
  if (test_compare(component_info_geti(&c1, 9), 22))
    r = 1;

  component_info_resize(&c2, 2, 0);
  component_info_setc(&c2, 0, &ary[7]);
  component_info_setc(&c2, 1, &ary[4]);

  if (test_compare(component_info_getc(&c2, 0)->comp_index, 6))
    r = 1;
  if (test_compare(component_info_getc(&c2, 1)->comp_index, 3))
    r = 1;

  component_info_merge(&c1, &c1, &c2);
  if (test_compare(component_info_ncompo(&c1), 8))
    r = 1;

  if (test_compare(component_info_getc(&c1, 0), NULL))
    r = 1;
  if (test_compare(component_info_getc(&c1, 1), NULL))
    r = 1;
  if (test_compare(component_info_getc(&c1, 2), NULL))
    r = 1;
  if (test_compare(component_info_getc(&c1, 3), NULL))
    r = 1;
  if (test_compare(component_info_getc(&c1, 4), NULL))
    r = 1;
  if (test_compare(component_info_getc(&c1, 5), NULL))
    r = 1;
  if (test_compare(component_info_getc(&c1, 6), &ary[7]))
    r = 1;
  if (test_compare(component_info_getc(&c1, 7), &ary[4]))
    r = 1;

  if (test_compare(component_info_geti(&c1, 0), 0))
    r = 1;
  if (test_compare(component_info_geti(&c1, 1), 1))
    r = 1;
  if (test_compare(component_info_geti(&c1, 2), 2))
    r = 1;
  if (test_compare(component_info_geti(&c1, 3), 7))
    r = 1;
  if (test_compare(component_info_geti(&c1, 4), 8))
    r = 1;
  if (test_compare(component_info_geti(&c1, 5), 22))
    r = 1;

  component_info_resize(&c1, 1, 1);
  if (test_compare(component_info_geti(&c1, 0), 0))
    r = 1;

  component_info_merge(&c3, &c1, &c2);
  if (test_compare(component_info_ncompo(&c3), 3))
    r = 1;

  if (test_compare(component_info_getc(&c3, 0), NULL))
    r = 1;
  if (test_compare(component_info_getc(&c3, 1), &ary[7]))
    r = 1;
  if (test_compare(component_info_getc(&c3, 2), &ary[4]))
    r = 1;

  if (test_compare(component_info_geti(&c3, 0), 0))
    r = 1;

  component_info_setc(&c1, 0, &ary[11]);
  component_info_merge(&c3, &c1, &c2);
  if (test_compare(component_info_ncompo(&c3), 3))
    r = 1;

  if (test_compare(component_info_getc(&c3, 0), &ary[11]))
    r = 1;
  if (test_compare(component_info_getc(&c3, 1), &ary[7]))
    r = 1;
  if (test_compare(component_info_getc(&c3, 2), &ary[4]))
    r = 1;

  component_info_resize(&c1, 1, 1);
  component_info_resize(&c2, 0, 1);
  component_info_seti(&c1, 0, 4);

  component_info_merge(&c2, &c2, &c1);
  if (test_compare(component_info_ncompo(&c2), 1))
    r = 1;

  if (test_compare(component_info_getc(&c2, 0), NULL))
    r = 1;

  if (test_compare(component_info_geti(&c2, 0), 4))
    r = 1;

  component_info_clear(&c1);
  component_info_clear(&c2);
  component_info_clear(&c3);

  if (r)
    return EXIT_FAILURE;
  return EXIT_SUCCESS;
}
