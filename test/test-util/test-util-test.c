
#include "test-util.h"

#include <math.h>
#include <stdint.h>
#include <stdlib.h>

#define test_expect(expr, exp)                                                \
  do {                                                                        \
    int _x = 0;                                                               \
    if ((_x = (expr)) != exp) {                                               \
      fprintf(stderr, "===== failed: %s\n===== expect: %d, got: %d\n", #expr, \
              exp, _x);                                                       \
      r = EXIT_FAILURE;                                                       \
    }                                                                         \
  } while (0)

int main(int argc, char **argv)
{
  int r;

  r = EXIT_SUCCESS;

  test_expect(1 == 1, 1);
  test_expect(1 == 0, 0);
  test_expect(0 != 1, 1);

  test_expect(test_compare_base(0, "test1", 0, "descrip1", "faildesc1"), 0);
  test_expect(test_compare_base(1, "test2", 1, "descrip2", "faildesc2"), 1);
  test_expect(test_compare_base(0, "test3", 2,
                                "descrip3\nnext line\nmore lines\ntoo many",
                                "faildesc3\naaz\nbb\n\nccc\n"),
              0);
  test_expect(test_compare_base(1, "test4", 3,
                                "descrip3\nnext line\nmore lines\ntoo many",
                                "faildesc3\naaz\nbb\n\nccc\n"),
              1);
  test_expect(test_compare_base(0, "test5", 4, "descrip4", NULL), 0);
  test_expect(test_compare_base(1, "test6", 5, "descrip4", NULL), 1);

  test_expect(test_compare(1, 1), 0);
  test_expect(test_compare(1, 2), 1);
  test_expect(test_compare_f(1, 2, "test1"), 1);
  test_expect(test_compare_f(1, 2, "foo\nbar\n"), 1);
  test_expect(test_compare_c(1 < 2), 0);
  test_expect(test_compare_c(2 < 2), 1);
  test_expect(test_compare_cf(2 < 2, "test2"), 1);

  test_expect(test_compare_bytes(((char[]){'a', 'b', 'c', '\0'}), "abc"), 0);
  test_expect(test_compare_bytes(((char[]){'a', 'b', 'c'}), "abc"), 0);
  test_expect(test_compare_bytes(((char[]){'a', 'b', 'e', '\0'}), "abc"), 1);
  test_expect(test_compare_bytes("abc", "abc"), 0);
  test_expect(test_compare_bytes("abc", "abcd"), 1);
  test_expect(test_compare_bytes("abcd", "abc"), 1);
  test_expect(test_compare_bytes("abc", "abe"), 1);

  test_print_bytes((char[]){0}, 1);
  test_print_bytes((char[]){0, 2, 6, 33, 96}, 5);
  test_print_bytes("ABCDEFGHIJKLMNOPQRSTUVWXYZ012", 29);
  test_print_bytes(NULL, 0);

  test_expect(test_compare_ii(2, ==, 2), 1);
  test_expect(test_compare_ii(1, ==, 2), 0);

  test_expect(test_compare_ii(3, <=, 2), 0);
  test_expect(test_compare_ii(2, <=, 2), 1);
  test_expect(test_compare_ii(1, <=, 2), 1);

  test_expect(test_compare_ii(3, <, 2), 0);
  test_expect(test_compare_ii(2, <, 2), 0);
  test_expect(test_compare_ii(1, <, 2), 1);

  test_expect(test_compare_ii(3, >=, 2), 1);
  test_expect(test_compare_ii(2, >=, 2), 1);
  test_expect(test_compare_ii(1, >=, 2), 0);

  test_expect(test_compare_ii(3, >, 2), 1);
  test_expect(test_compare_ii(2, >, 2), 0);
  test_expect(test_compare_ii(1, >, 2), 0);

  test_expect(test_compare_ii(2, !=, 2), 0);
  test_expect(test_compare_ii(1, !=, 2), 1);

  test_expect(test_compare_iu(2, ==, 2), 1);
  test_expect(test_compare_iu(1, ==, 2), 0);
  test_expect(test_compare_iu(-1, ==, (uintmax_t)-1), 0);

  test_expect(test_compare_iu(3, <=, 2), 0);
  test_expect(test_compare_iu(2, <=, 2), 1);
  test_expect(test_compare_iu(1, <=, 2), 1);
  test_expect(test_compare_iu(-1, <=, 2), 1);

  test_expect(test_compare_iu(3, <, 2), 0);
  test_expect(test_compare_iu(2, <, 2), 0);
  test_expect(test_compare_iu(1, <, 2), 1);
  test_expect(test_compare_iu(-1, <, 2), 1);

  test_expect(test_compare_iu(3, >=, 2), 1);
  test_expect(test_compare_iu(2, >=, 2), 1);
  test_expect(test_compare_iu(1, >=, 2), 0);
  test_expect(test_compare_iu(-1, >=, 2), 0);

  test_expect(test_compare_iu(3, >, 2), 1);
  test_expect(test_compare_iu(2, >, 2), 0);
  test_expect(test_compare_iu(1, >, 2), 0);
  test_expect(test_compare_iu(-1, >, 2), 0);

  test_expect(test_compare_iu(2, !=, 2), 0);
  test_expect(test_compare_iu(1, !=, 2), 1);
  test_expect(test_compare_iu(-1, !=, (uintmax_t)-1), 0);

  test_expect(test_compare_ui(2, ==, 2), 1);
  test_expect(test_compare_ui(1, ==, 2), 0);
  test_expect(test_compare_ui((uintmax_t)-1, ==, -1), 0);

  test_expect(test_compare_ui(3, <=, 2), 0);
  test_expect(test_compare_ui(2, <=, 2), 1);
  test_expect(test_compare_ui(1, <=, 2), 1);
  test_expect(test_compare_ui(2, <=, -1), 0);

  test_expect(test_compare_ui(3, <, 2), 0);
  test_expect(test_compare_ui(2, <, 2), 0);
  test_expect(test_compare_ui(1, <, 2), 1);
  test_expect(test_compare_ui(2, <, -1), 0);

  test_expect(test_compare_ui(3, >=, 2), 1);
  test_expect(test_compare_ui(2, >=, 2), 1);
  test_expect(test_compare_ui(1, >=, 2), 0);
  test_expect(test_compare_ui(2, >=, -1), 1);

  test_expect(test_compare_ui(3, >, 2), 1);
  test_expect(test_compare_ui(2, >, 2), 0);
  test_expect(test_compare_ui(1, >, 2), 0);
  test_expect(test_compare_ui(2, >, -1), 1);

  test_expect(test_compare_ui(2, !=, 2), 0);
  test_expect(test_compare_ui(1, !=, 2), 1);
  test_expect(test_compare_ui((uintmax_t)-1, !=, -1), 0);

  test_expect(test_compare_uu(2, ==, 2), 1);
  test_expect(test_compare_uu(1, ==, 2), 0);

  test_expect(test_compare_uu(3, <=, 2), 0);
  test_expect(test_compare_uu(2, <=, 2), 1);
  test_expect(test_compare_uu(1, <=, 2), 1);

  test_expect(test_compare_uu(3, <, 2), 0);
  test_expect(test_compare_uu(2, <, 2), 0);
  test_expect(test_compare_uu(1, <, 2), 1);

  test_expect(test_compare_uu(3, >=, 2), 1);
  test_expect(test_compare_uu(2, >=, 2), 1);
  test_expect(test_compare_uu(1, >=, 2), 0);

  test_expect(test_compare_uu(3, >, 2), 1);
  test_expect(test_compare_uu(2, >, 2), 0);
  test_expect(test_compare_uu(1, >, 2), 0);

  test_expect(test_compare_uu(2, !=, 2), 0);
  test_expect(test_compare_uu(1, !=, 2), 1);

  test_expect(test_compare_dd(2., ==, 2.), 1);
  test_expect(test_compare_dd(1., ==, 2.), 0);

  test_expect(test_compare_dd(3., <=, 2.), 0);
  test_expect(test_compare_dd(nexttoward(2., HUGE_VAL), <=, 2.), 0);
  test_expect(test_compare_dd(2., <=, 2.), 1);
  test_expect(test_compare_dd(1., <=, 2.), 1);

  test_expect(test_compare_dd(3., <, 2.), 0);
  test_expect(test_compare_dd(2., <, 2.), 0);
  test_expect(test_compare_dd(nexttoward(2., -HUGE_VAL), <, 2.), 1);
  test_expect(test_compare_dd(1., <, 2.), 1);

  test_expect(test_compare_dd(3., >=, 2.), 1);
  test_expect(test_compare_dd(2., >=, 2.), 1);
  test_expect(test_compare_dd(nexttoward(2., -HUGE_VAL), >=, 2.), 0);
  test_expect(test_compare_dd(1., >=, 2.), 0);

  test_expect(test_compare_dd(3., >, 2.), 1);
  test_expect(test_compare_dd(nexttoward(2., HUGE_VAL), >, 2.), 1);
  test_expect(test_compare_dd(2., >, 2.), 0);
  test_expect(test_compare_dd(1., >, 2.), 0);

  test_expect(test_compare_dd(2., !=, 2.), 0);
  test_expect(test_compare_dd(1., !=, 2.), 1);

  test_expect(test_compare_eps(2., 3., 1.), 0);
  test_expect(test_compare_eps(nexttoward(2., HUGE_VAL), 3., 1.), 1);
  test_expect(test_compare_eps(2.2, 3., 1.), 1);
  test_expect(test_compare_eps(3.8, 3., 1.), 1);
  test_expect(test_compare_eps(nexttoward(4., -HUGE_VAL), 3., 1.), 1);
  test_expect(test_compare_eps(4., 3., 1.), 0);

  test_expect(test_compare_ss("abc", "abc"), 1);
  test_expect(test_compare_ss("abc", "abcd"), 0);
  test_expect(test_compare_ss("abcd", "abc"), 0);
  test_expect(test_compare_ss("abt", "abc"), 0);
  test_expect(test_compare_ss("abt", "ab\ta\vv\f\a\bc\ntt vv"), 0);
  test_expect(test_compare_ss("abt", "ab\344\x03 vv"), 0);
  test_print_bytes("ab\344\x03 vv", 8);

  test_expect(test_compare_ssn("abc", "abc", 3), 1);
  test_expect(test_compare_ssn("abc", "abd", 3), 0);
  test_expect(test_compare_ssn("abc", "abd", 2), 1);
  test_expect(test_compare_ssn("ab", "abd", 2), 1);
  test_expect(test_compare_ssn("abt", "ab", 2), 1);
  test_expect(test_compare_ssn("abt", "ac", 1), 1);
  test_expect(test_compare_ssn("abcd", "abc", 4), 0);
  test_expect(test_compare_ssn("abc", "abcdefghijkl", 4), 0);

  test_expect(test_compare_pp(NULL, ==, NULL), 1);

  {
    int a;
    int b;
    int c[10];
    test_expect(test_compare_pp(&a, ==, &a), 1);
    test_expect(test_compare_pp(&b, ==, &a), 0);
    test_expect(test_compare_pp(&c[5], ==, c + 5), 1);
    test_expect(test_compare_pp(c + 5, ==, &c[5]), 1);

    test_expect(test_compare_pp(&c[3], <=, &c[2]), 0);
    test_expect(test_compare_pp(&c[2], <=, &c[2]), 1);
    test_expect(test_compare_pp(&c[1], <=, &c[2]), 1);
    test_expect(test_compare_pp(&c[0], <=, &c[2]), 1);

    test_expect(test_compare_pp(&c[3], <, &c[2]), 0);
    test_expect(test_compare_pp(&c[2], <, &c[2]), 0);
    test_expect(test_compare_pp(&c[1], <, &c[2]), 1);
    test_expect(test_compare_pp(&c[0], <, &c[2]), 1);

    test_expect(test_compare_pp(&c[3], >=, &c[2]), 1);
    test_expect(test_compare_pp(&c[2], >=, &c[2]), 1);
    test_expect(test_compare_pp(&c[1], >=, &c[2]), 0);
    test_expect(test_compare_pp(&c[0], >=, &c[2]), 0);

    test_expect(test_compare_pp(&c[3], >, &c[2]), 1);
    test_expect(test_compare_pp(&c[2], >, &c[2]), 0);
    test_expect(test_compare_pp(&c[1], >, &c[2]), 0);
    test_expect(test_compare_pp(&c[0], >, &c[2]), 0);

    test_expect(test_compare_pp(&a, !=, &b), 1);
    test_expect(test_compare_pp(&a, !=, &a), 0);

    test_expect(test_compare_poff(&c[0], &c[0], 0), 1);
    test_expect(test_compare_poff(&c[2], &c[0], 2), 1);
    test_expect(test_compare_poff(&c[0], &c[2], -2), 1);

    test_expect(test_compare_poff(&c[3], &c[0], 2), 0);

    double llllllllllllll;
    double mmmmmmmmmmmmmmmm;

    test_expect(test_compare_poff(&llllllllllllll, &mmmmmmmmmmmmmmmm, 0), 0);
  }

  return r;
}
