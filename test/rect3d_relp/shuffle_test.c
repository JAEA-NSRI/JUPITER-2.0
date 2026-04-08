
#include <jupiter/random/random.h>

#include "rect3d_relp_test.h"
#include "test-util.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

#define test_fail_if(expr) \
  do {                     \
    if (expr)              \
      r = 1;               \
  } while (0)

static int type_sort(const void *a, const void *b)
{
  type ta, tb;
  ta = *(const type *)a;
  tb = *(const type *)b;
  if (ta < tb)
    return -1;
  if (ta > tb)
    return 1;
  return 0;
}

int shuffle_test(void)
{
  int r = 0;
  type *a, *b;
  jupiter_random_seed seed = {0x6573bbef816103ec, 0xf255631cee2c3bc6,
                              0xe769115a4e5a9aea, 0x7da8d65c8c396ce3};

  a = random_type_seq(10, 0);
  test_fail_if(!test_compare_dd(a[0], ==, 0.0));
  test_fail_if(!test_compare_dd(a[1], ==, 1.0));
  test_fail_if(!test_compare_dd(a[2], ==, 2.0));
  test_fail_if(!test_compare_dd(a[3], ==, 3.0));
  test_fail_if(!test_compare_dd(a[4], ==, 4.0));
  test_fail_if(!test_compare_dd(a[5], ==, 5.0));
  test_fail_if(!test_compare_dd(a[6], ==, 6.0));
  test_fail_if(!test_compare_dd(a[7], ==, 7.0));
  test_fail_if(!test_compare_dd(a[8], ==, 8.0));
  test_fail_if(!test_compare_dd(a[9], ==, 9.0));

  b = random_type_seq(10, -1);
  test_fail_if(!test_compare_dd(b[0], ==, 0.0));
  test_fail_if(!test_compare_dd(b[1], ==, 0.5));
  test_fail_if(!test_compare_dd(b[2], ==, 1.0));
  test_fail_if(!test_compare_dd(b[3], ==, 1.5));
  test_fail_if(!test_compare_dd(b[4], ==, 2.0));
  test_fail_if(!test_compare_dd(b[5], ==, 2.5));
  test_fail_if(!test_compare_dd(b[6], ==, 3.0));
  test_fail_if(!test_compare_dd(b[7], ==, 3.5));
  test_fail_if(!test_compare_dd(b[8], ==, 4.0));
  test_fail_if(!test_compare_dd(b[9], ==, 4.5));

  memcpy(b, a, sizeof(type) * 10);
  test_fail_if(!test_compare_dd(b[0], ==, 0.0));
  test_fail_if(!test_compare_dd(b[1], ==, 1.0));
  test_fail_if(!test_compare_dd(b[2], ==, 2.0));
  test_fail_if(!test_compare_dd(b[3], ==, 3.0));
  test_fail_if(!test_compare_dd(b[4], ==, 4.0));
  test_fail_if(!test_compare_dd(b[5], ==, 5.0));
  test_fail_if(!test_compare_dd(b[6], ==, 6.0));
  test_fail_if(!test_compare_dd(b[7], ==, 7.0));
  test_fail_if(!test_compare_dd(b[8], ==, 8.0));
  test_fail_if(!test_compare_dd(b[9], ==, 9.0));

  rect3d_relp_test_shuffle(b, 10, 1, 1, &seed);
  test_fail_if(!test_compare_dd(b[0], ==, 2.0));
  test_fail_if(!test_compare_dd(b[1], ==, 8.0));
  test_fail_if(!test_compare_dd(b[2], ==, 4.0));
  test_fail_if(!test_compare_dd(b[3], ==, 9.0));
  test_fail_if(!test_compare_dd(b[4], ==, 3.0));
  test_fail_if(!test_compare_dd(b[5], ==, 7.0));
  test_fail_if(!test_compare_dd(b[6], ==, 5.0));
  test_fail_if(!test_compare_dd(b[7], ==, 0.0));
  test_fail_if(!test_compare_dd(b[8], ==, 6.0));
  test_fail_if(!test_compare_dd(b[9], ==, 1.0));

  rect3d_relp_test_shuffle(b, 1, 2, 5, &seed);
  test_fail_if(!test_compare_dd(b[0], ==, 7.0));
  test_fail_if(!test_compare_dd(b[1], ==, 3.0));
  test_fail_if(!test_compare_dd(b[2], ==, 1.0));
  test_fail_if(!test_compare_dd(b[3], ==, 5.0));
  test_fail_if(!test_compare_dd(b[4], ==, 0.0));
  test_fail_if(!test_compare_dd(b[5], ==, 4.0));
  test_fail_if(!test_compare_dd(b[6], ==, 9.0));
  test_fail_if(!test_compare_dd(b[7], ==, 6.0));
  test_fail_if(!test_compare_dd(b[8], ==, 8.0));
  test_fail_if(!test_compare_dd(b[9], ==, 2.0));

  rect3d_relp_test_shuffle(b, 1, 2, 5, NULL);

  for (int k = 0; k < 10; ++k) {
    int j = 0, ji = -1;
    for (int i = 0; i < 10; ++i) {
      if (b[i] == a[k]) {
        ji = i;
        j++;
      }
    }
    test_compare_printf("..... ", "..... ", "for k = %d, a[k] = %g, ji = %d", k,
                        a[k], ji);
    test_fail_if(!test_compare_ii(j, ==, 1));
  }

  a = random_type_value(12, 13, 14, -4, &seed);
  test_fail_if(!test_compare_dd(a[0], ==, 50.5625)); // 905 * (2 ** -4)
  test_fail_if(!test_compare_dd(a[1], ==, 18.875));  // 301 * (2 ** -4)
  test_fail_if(!test_compare_dd(a[2], ==, 123.0));   // 1968 * (2 ** -4)
  test_fail_if(!test_compare_dd(a[3], ==, 37.1875)); // 595 * (2 ** -4)

  qsort(a, 12 * 13 * 14, sizeof(type), type_sort);
  for (int i = 0; i < 12 * 13 * 14; ++i) {
    test_compare_printf("..... ", "..... ", "for i = %d, a[i] = %.7g", i, a[i]);
#ifdef JUPITER_DOUBLE
    test_fail_if(!test_compare_dd(a[i], ==, ldexp(i, -4)));
#else
    test_fail_if(!test_compare_dd(a[i], ==, ldexpf(i, -4)));
#endif
  }

  return r;
}
