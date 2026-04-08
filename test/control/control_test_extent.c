#include "control_test.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/extent.h"
#include "jupiter/control/overflow.h"
#include "test-util.h"
#include "test/control/control_test_util.h"
#include <limits.h>

int test_control_extent(void)
{
  int ret = 0;

  if (!test_compare_jcntrl_irange(jcntrl_irange_c(0, 0),
                                  ((jcntrl_irange){0, 0})))
    ret = 1;

  if (!test_compare_jcntrl_irange(jcntrl_irange_c(0, 2),
                                  ((jcntrl_irange){0, 2})))
    ret = 1;

  if (!test_compare_jcntrl_irange(jcntrl_irange_c(2, -1),
                                  ((jcntrl_irange){2, -1})))
    ret = 1;

  if (!test_compare_ii(jcntrl_irange_empty(jcntrl_irange_c(0, 0)), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_irange_empty(jcntrl_irange_c(0, 1)), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_irange_empty(jcntrl_irange_c(1, 1)), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_irange_empty(jcntrl_irange_c(0, -1)), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_irange_empty(jcntrl_irange_c(0, 3)), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_irange_include(jcntrl_irange_c(0, 0), -1), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_irange_include(jcntrl_irange_c(0, 0), 0), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_irange_include(jcntrl_irange_c(0, 0), 1), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_irange_include(jcntrl_irange_c(0, 3), -1), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_irange_include(jcntrl_irange_c(0, 3), 0), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_irange_include(jcntrl_irange_c(0, 3), 1), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_irange_include(jcntrl_irange_c(0, 3), 2), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_irange_include(jcntrl_irange_c(0, 3), 3), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_irange_n(jcntrl_irange_c(0, 0)), ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_irange_n(jcntrl_irange_c(0, 1)), ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_irange_n(jcntrl_irange_c(0, 5)), ==, 5))
    ret = 1;
  if (!test_compare_ii(jcntrl_irange_n(jcntrl_irange_c(0, 9)), ==, 9))
    ret = 1;
  if (!test_compare_ii(jcntrl_irange_n(jcntrl_irange_c(3, 9)), ==, 6))
    ret = 1;
  if (!test_compare_ii(jcntrl_irange_n(jcntrl_irange_c(0, -1)), ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_irange_n(jcntrl_irange_c(INT_MIN, INT_MAX)), ==,
                       -1))
    ret = 1;

  if (!test_compare_jcntrl_irange(jcntrl_irange_overlap(jcntrl_irange_c(0, 2),
                                                        jcntrl_irange_c(1, 3)),
                                  jcntrl_irange_c(1, 2)))
    ret = 1;
  if (!test_compare_jcntrl_irange(jcntrl_irange_overlap(jcntrl_irange_c(0, 5),
                                                        jcntrl_irange_c(3, 3)),
                                  jcntrl_irange_c(0, -1)))
    ret = 1;
  if (!test_compare_jcntrl_irange(jcntrl_irange_overlap(jcntrl_irange_c(5, 5),
                                                        jcntrl_irange_c(0, 0)),
                                  jcntrl_irange_c(0, -1)))
    ret = 1;
  if (!test_compare_jcntrl_irange(jcntrl_irange_overlap(jcntrl_irange_c(4, 7),
                                                        jcntrl_irange_c(0, 9)),
                                  jcntrl_irange_c(4, 7)))
    ret = 1;
  if (!test_compare_jcntrl_irange(jcntrl_irange_overlap(jcntrl_irange_c(4, 9),
                                                        jcntrl_irange_c(0, 7)),
                                  jcntrl_irange_c(4, 7)))
    ret = 1;
  if (!test_compare_jcntrl_irange(jcntrl_irange_overlap(jcntrl_irange_c(0, 3),
                                                        jcntrl_irange_c(5, 8)),
                                  jcntrl_irange_c(5, 3)))
    ret = 1;
  if (!test_compare_jcntrl_irange(jcntrl_irange_overlap(jcntrl_irange_c(0, 5),
                                                        jcntrl_irange_c(5, 8)),
                                  jcntrl_irange_c(5, 5)))
    ret = 1;

  if (!test_compare_jcntrl_irange(jcntrl_irange_cover(jcntrl_irange_c(0, 2),
                                                      jcntrl_irange_c(1, 3)),
                                  jcntrl_irange_c(0, 3)))
    ret = 1;
  if (!test_compare_jcntrl_irange(jcntrl_irange_cover(jcntrl_irange_c(0, 5),
                                                      jcntrl_irange_c(3, 3)),
                                  jcntrl_irange_c(0, 5)))
    ret = 1;
  if (!test_compare_jcntrl_irange(jcntrl_irange_cover(jcntrl_irange_c(5, 5),
                                                      jcntrl_irange_c(0, 0)),
                                  jcntrl_irange_c(0, 0)))
    ret = 1;
  if (!test_compare_jcntrl_irange(jcntrl_irange_cover(jcntrl_irange_c(4, 7),
                                                      jcntrl_irange_c(0, 9)),
                                  jcntrl_irange_c(0, 9)))
    ret = 1;
  if (!test_compare_jcntrl_irange(jcntrl_irange_cover(jcntrl_irange_c(4, 9),
                                                      jcntrl_irange_c(0, 7)),
                                  jcntrl_irange_c(0, 9)))
    ret = 1;
  if (!test_compare_jcntrl_irange(jcntrl_irange_cover(jcntrl_irange_c(0, 3),
                                                      jcntrl_irange_c(5, 8)),
                                  jcntrl_irange_c(0, 8)))
    ret = 1;
  if (!test_compare_jcntrl_irange(jcntrl_irange_cover(jcntrl_irange_c(0, 5),
                                                      jcntrl_irange_c(5, 8)),
                                  jcntrl_irange_c(0, 8)))
    ret = 1;

  if (!test_compare_jcntrl_extent(jcntrl_extent_empty(),
                                  ((jcntrl_extent){0, -1, 0, -1, 0, -1})))
    ret = 1;

  if (!test_compare_jcntrl_extent(jcntrl_extent_c((int[]){0, 1, 2, 3, 4, 5}),
                                  ((jcntrl_extent){0, 1, 2, 3, 4, 5})))
    ret = 1;

  if (!test_compare_jcntrl_extent(jcntrl_extent_i(0, 1, 2, 3, 4, 5, 0),
                                  ((jcntrl_extent){0, 1, 2, 3, 4, 5})))
    ret = 1;
  if (!test_compare_jcntrl_extent(jcntrl_extent_i(0, 1, 2, 3, 4, 5, 1),
                                  ((jcntrl_extent){0, 2, 2, 4, 4, 6})))
    ret = 1;
  if (!test_compare_jcntrl_extent(jcntrl_extent_i(0, 1, 2, 3, 4, 5, 3),
                                  ((jcntrl_extent){0, 2, 2, 4, 4, 6})))
    ret = 1;
  if (!test_compare_jcntrl_extent(jcntrl_extent_i(0, 1, 2, 3, 4, 5, -1),
                                  ((jcntrl_extent){0, 2, 2, 4, 4, 6})))
    ret = 1;

  if (!test_compare_jcntrl_extent(jcntrl_extent_r(jcntrl_irange_c(0, 1),
                                                  jcntrl_irange_c(2, 3),
                                                  jcntrl_irange_c(4, 5)),
                                  ((jcntrl_extent){0, 1, 2, 3, 4, 5})))
    ret = 1;

  if (!test_compare_jcntrl_irange(jcntrl_extent_xrange(
                                    jcntrl_extent_i(0, 1, 2, 3, 4, 5, 0)),
                                  jcntrl_irange_c(0, 1)))
    ret = 1;

  if (!test_compare_jcntrl_irange(jcntrl_extent_xrange(
                                    jcntrl_extent_i(0, -1, 2, -3, 4, -5, 0)),
                                  jcntrl_irange_c(0, -1)))
    ret = 1;

  if (!test_compare_jcntrl_irange(jcntrl_extent_xrange(jcntrl_extent_empty()),
                                  jcntrl_irange_c(0, -1)))
    ret = 1;

  if (!test_compare_jcntrl_irange(jcntrl_extent_yrange(
                                    jcntrl_extent_i(0, 1, 2, 3, 4, 5, 0)),
                                  jcntrl_irange_c(2, 3)))
    ret = 1;

  if (!test_compare_jcntrl_irange(jcntrl_extent_yrange(
                                    jcntrl_extent_i(0, -1, 2, -3, 4, -5, 0)),
                                  jcntrl_irange_c(2, -3)))
    ret = 1;

  if (!test_compare_jcntrl_irange(jcntrl_extent_yrange(jcntrl_extent_empty()),
                                  jcntrl_irange_c(0, -1)))
    ret = 1;

  if (!test_compare_jcntrl_irange(jcntrl_extent_zrange(
                                    jcntrl_extent_i(0, 1, 2, 3, 4, 5, 0)),
                                  jcntrl_irange_c(4, 5)))
    ret = 1;

  if (!test_compare_jcntrl_irange(jcntrl_extent_zrange(
                                    jcntrl_extent_i(0, -1, 2, -3, 4, -5, 0)),
                                  jcntrl_irange_c(4, -5)))
    ret = 1;

  if (!test_compare_jcntrl_irange(jcntrl_extent_zrange(jcntrl_extent_empty()),
                                  jcntrl_irange_c(0, -1)))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_nx(jcntrl_extent_i(0, 9, 2, 8, 3, 6, 0)),
                       ==, 9))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_nx(
                         jcntrl_extent_i(0, -1, 2, -3, 4, -5, 0)),
                       ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_ny(jcntrl_extent_i(0, 9, 2, 8, 3, 6, 0)),
                       ==, 6))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_ny(
                         jcntrl_extent_i(0, -1, 2, -3, 4, -5, 0)),
                       ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_nz(jcntrl_extent_i(0, 9, 2, 8, 3, 6, 0)),
                       ==, 3))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_nz(
                         jcntrl_extent_i(0, -1, 2, -3, 4, -5, 0)),
                       ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_size(jcntrl_extent_i(0, 9, 2, 8, 3, 6, 0)),
                       ==, 162))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_size(jcntrl_extent_i(0, 3, 2, 5, 8, 9, 0)),
                       ==, 9))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_size(jcntrl_extent_i(0, 1, 2, 3, 4, 5, 0)),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_size(
                         jcntrl_extent_i(0, -1, 2, 8, 3, 6, 0)),
                       ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_size(
                         jcntrl_extent_i(0, 9, 2, -3, 3, 6, 0)),
                       ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_size(
                         jcntrl_extent_i(0, 9, 2, 8, 4, -5, 0)),
                       ==, 0))
    ret = 1;

  /**
   * @note If int has 16-bit width and jcntrl_size_type has 64-bit width,
   * this test will fail (because calculation will not overflow).
   */
  if (!test_compare_ii(jcntrl_extent_size(jcntrl_extent_i(0, INT_MAX, //
                                                          0, INT_MAX, //
                                                          0, INT_MAX, 0)),
                       ==, -1)) {
    if (sizeof(jcntrl_size_type) < 3 * sizeof(int))
      ret = 1;
  }

  /**
   * @note Requires jcntrl_size_type to be 48-bit width and more. Otherwise,
   * this test will fail (because calculation will overflow).
   */
  if (!test_compare_ii(jcntrl_extent_size(jcntrl_extent_i(0, 65521, //
                                                          0, 65519, //
                                                          0, 65497, 0)),
                       ==, 281170132523303LL)) {
    if (sizeof(jcntrl_size_type) >= ((48 - 1) / CHAR_BIT + 1))
      ret = 1;
  }

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 0, 2, 8, 3, 9, 0),
                                          0, 2, 3),
                       ==, -1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 2, 3, 9, 0),
                                          0, 2, 3),
                       ==, -1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 3, 3, 3, 0),
                                          0, 2, 3),
                       ==, -1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          -1, 2, 3),
                       ==, -1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          0, 1, 3),
                       ==, -1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          0, 2, 2),
                       ==, -1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          5, 2, 3),
                       ==, -1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          0, 8, 3),
                       ==, -1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          0, 2, 9),
                       ==, -1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          0, 2, 3),
                       ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          1, 2, 3),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          4, 2, 3),
                       ==, 4))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          0, 3, 3),
                       ==, 5))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          1, 3, 3),
                       ==, 6))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          0, 7, 3),
                       ==, 25))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          4, 7, 3),
                       ==, 29))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          0, 2, 4),
                       ==, 30))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          1, 2, 4),
                       ==, 31))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          0, 3, 4),
                       ==, 35))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          0, 2, 8),
                       ==, 150))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          4, 2, 8),
                       ==, 154))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          4, 6, 8),
                       ==, 174))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          3, 7, 8),
                       ==, 178))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, 5, 2, 8, 3, 9, 0),
                                          4, 7, 8),
                       ==, 179))
    ret = 1;

  /**
   * @note If int has 16-bit width and jcntrl_size_type has 64-bit width,
   * this test will fail (because calculation will not overflow).
   */
  if (!test_compare_ii(jcntrl_extent_addr(jcntrl_extent_i(0, INT_MAX, //
                                                          0, INT_MAX, //
                                                          0, INT_MAX, 0),
                                          INT_MAX - 1, INT_MAX - 1,
                                          INT_MAX - 1),
                       ==, -2)) {
    if (sizeof(jcntrl_size_type) < 3 * sizeof(int))
      ret = 1;
  }

  {
    int i, j, k;

    i = j = k = -1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 0, 2, 8, 3, 9,
                                                             0),
                                             0, &i, &j, &k),
                         ==, 0))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 2, 3, 9,
                                                             0),
                                             0, &i, &j, &k),
                         ==, 0))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 3,
                                                             0),
                                             0, &i, &j, &k),
                         ==, 0))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             -1, &i, &j, &k),
                         ==, 0))
      ret = 1;

    i = j = k = -1;
    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             0, &i, &j, &k),
                         ==, 1))
      ret = 1;
    if (!test_compare_ii(i, ==, 0))
      ret = 1;
    if (!test_compare_ii(j, ==, 2))
      ret = 1;
    if (!test_compare_ii(k, ==, 3))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             1, &i, &j, &k),
                         ==, 1))
      ret = 1;
    if (!test_compare_ii(i, ==, 1))
      ret = 1;
    if (!test_compare_ii(j, ==, 2))
      ret = 1;
    if (!test_compare_ii(k, ==, 3))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             4, &i, &j, &k),
                         ==, 1))
      ret = 1;
    if (!test_compare_ii(i, ==, 4))
      ret = 1;
    if (!test_compare_ii(j, ==, 2))
      ret = 1;
    if (!test_compare_ii(k, ==, 3))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             5, &i, &j, &k),
                         ==, 1))
      ret = 1;
    if (!test_compare_ii(i, ==, 0))
      ret = 1;
    if (!test_compare_ii(j, ==, 3))
      ret = 1;
    if (!test_compare_ii(k, ==, 3))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             6, &i, &j, &k),
                         ==, 1))
      ret = 1;
    if (!test_compare_ii(i, ==, 1))
      ret = 1;
    if (!test_compare_ii(j, ==, 3))
      ret = 1;
    if (!test_compare_ii(k, ==, 3))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             25, &i, &j, &k),
                         ==, 1))
      ret = 1;
    if (!test_compare_ii(i, ==, 0))
      ret = 1;
    if (!test_compare_ii(j, ==, 7))
      ret = 1;
    if (!test_compare_ii(k, ==, 3))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             29, &i, &j, &k),
                         ==, 1))
      ret = 1;
    if (!test_compare_ii(i, ==, 4))
      ret = 1;
    if (!test_compare_ii(j, ==, 7))
      ret = 1;
    if (!test_compare_ii(k, ==, 3))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             30, &i, &j, &k),
                         ==, 1))
      ret = 1;
    if (!test_compare_ii(i, ==, 0))
      ret = 1;
    if (!test_compare_ii(j, ==, 2))
      ret = 1;
    if (!test_compare_ii(k, ==, 4))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             31, &i, &j, &k),
                         ==, 1))
      ret = 1;
    if (!test_compare_ii(i, ==, 1))
      ret = 1;
    if (!test_compare_ii(j, ==, 2))
      ret = 1;
    if (!test_compare_ii(k, ==, 4))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             35, &i, &j, &k),
                         ==, 1))
      ret = 1;
    if (!test_compare_ii(i, ==, 0))
      ret = 1;
    if (!test_compare_ii(j, ==, 3))
      ret = 1;
    if (!test_compare_ii(k, ==, 4))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             150, &i, &j, &k),
                         ==, 1))
      ret = 1;
    if (!test_compare_ii(i, ==, 0))
      ret = 1;
    if (!test_compare_ii(j, ==, 2))
      ret = 1;
    if (!test_compare_ii(k, ==, 8))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             154, &i, &j, &k),
                         ==, 1))
      ret = 1;
    if (!test_compare_ii(i, ==, 4))
      ret = 1;
    if (!test_compare_ii(j, ==, 2))
      ret = 1;
    if (!test_compare_ii(k, ==, 8))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             174, &i, &j, &k),
                         ==, 1))
      ret = 1;
    if (!test_compare_ii(i, ==, 4))
      ret = 1;
    if (!test_compare_ii(j, ==, 6))
      ret = 1;
    if (!test_compare_ii(k, ==, 8))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             178, &i, &j, &k),
                         ==, 1))
      ret = 1;
    if (!test_compare_ii(i, ==, 3))
      ret = 1;
    if (!test_compare_ii(j, ==, 7))
      ret = 1;
    if (!test_compare_ii(k, ==, 8))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             179, &i, &j, &k),
                         ==, 1))
      ret = 1;
    if (!test_compare_ii(i, ==, 4))
      ret = 1;
    if (!test_compare_ii(j, ==, 7))
      ret = 1;
    if (!test_compare_ii(k, ==, 8))
      ret = 1;

    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 5, 2, 8, 3, 9,
                                                             0),
                                             180, &i, &j, &k),
                         ==, 0))
      ret = 1;

    /**
     * If (jcntrl_size_type)INT_MAX + 1 overflows, the test will pass in most
     * cases, but the behavior is not guessable.
     */
    if (!test_compare_ii(jcntrl_extent_index(jcntrl_extent_i(0, 1, 2, 3, 3, 9,
                                                             0),
                                             (jcntrl_size_type)INT_MAX + 1, &i,
                                             &j, &k),
                         ==, 0)) {
      jcntrl_size_type t;
      if (!jcntrl_s_add_overflow(INT_MAX, 1, &t))
        ret = 1;
    }
  }

  if (!test_compare_jcntrl_extent(
        jcntrl_extent_overlap(jcntrl_extent_c((int[]){0, 1, 4, 5, 7, 8}),
                              jcntrl_extent_c((int[]){0, 1, 4, 5, 7, 8})),
        jcntrl_extent_c((int[]){0, 1, 4, 5, 7, 8})))
    ret = 1;

  if (!test_compare_jcntrl_extent(
        jcntrl_extent_overlap(jcntrl_extent_empty(), //
                              jcntrl_extent_c((int[]){0, 1, 4, 5, 7, 8})),
        jcntrl_extent_empty()))
    ret = 1;

  if (!test_compare_jcntrl_extent( //
        jcntrl_extent_overlap(jcntrl_extent_c((int[]){0, 1, 4, 5, 7, 8}),
                              jcntrl_extent_empty()),
        jcntrl_extent_empty()))
    ret = 1;

  if (!test_compare_jcntrl_extent(
        jcntrl_extent_overlap(jcntrl_extent_c((int[]){0, 5, 0, 8, 1, 6}),
                              jcntrl_extent_c((int[]){2, 3, 4, 5, 3, 9})),
        jcntrl_extent_c((int[]){2, 3, 4, 5, 3, 6})))
    ret = 1;

  if (!test_compare_jcntrl_extent(
        jcntrl_extent_overlap(jcntrl_extent_c((int[]){2, 7, 3, 6, 0, 8}),
                              jcntrl_extent_c((int[]){4, 9, 0, 4, 3, 4})),
        jcntrl_extent_c((int[]){4, 7, 3, 4, 3, 4})))
    ret = 1;

  if (!test_compare_jcntrl_extent(
        jcntrl_extent_overlap(jcntrl_extent_c((int[]){0, 2, 3, 5, 4, 7}),
                              jcntrl_extent_c((int[]){2, 4, 5, 6, 3, 4})),
        jcntrl_extent_c((int[]){2, 2, 5, 5, 4, 4})))
    ret = 1;

  if (!test_compare_jcntrl_extent(
        jcntrl_extent_cover(jcntrl_extent_c((int[]){0, 1, 4, 5, 7, 8}),
                            jcntrl_extent_c((int[]){0, 1, 4, 5, 7, 8})),
        jcntrl_extent_c((int[]){0, 1, 4, 5, 7, 8})))
    ret = 1;

  if (!test_compare_jcntrl_extent(                 //
        jcntrl_extent_cover(jcntrl_extent_empty(), //
                            jcntrl_extent_c((int[]){0, 1, 4, 5, 7, 8})),
        jcntrl_extent_c((int[]){0, 1, 4, 5, 7, 8})))
    ret = 1;

  if (!test_compare_jcntrl_extent( //
        jcntrl_extent_cover(jcntrl_extent_c((int[]){0, 1, 4, 5, 7, 8}),
                            jcntrl_extent_empty()),
        jcntrl_extent_c((int[]){0, 1, 4, 5, 7, 8})))
    ret = 1;

  if (!test_compare_jcntrl_extent(
        jcntrl_extent_cover(jcntrl_extent_c((int[]){0, 5, 0, 8, 1, 6}),
                            jcntrl_extent_c((int[]){2, 3, 4, 5, 3, 9})),
        jcntrl_extent_c((int[]){0, 5, 0, 8, 1, 9})))
    ret = 1;

  if (!test_compare_jcntrl_extent(
        jcntrl_extent_cover(jcntrl_extent_c((int[]){2, 7, 3, 6, 0, 8}),
                            jcntrl_extent_c((int[]){4, 9, 0, 4, 3, 4})),
        jcntrl_extent_c((int[]){2, 9, 0, 6, 0, 8})))
    ret = 1;

  if (!test_compare_jcntrl_extent(
        jcntrl_extent_cover(jcntrl_extent_c((int[]){0, 2, 3, 5, 4, 7}),
                            jcntrl_extent_c((int[]){2, 4, 5, 6, 3, 4})),
        jcntrl_extent_c((int[]){0, 4, 3, 6, 3, 7})))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_include(jcntrl_extent_c(
                                               (int[]){0, 5, 2, 8, 3, 9}),
                                             0, 2, 3),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_include(jcntrl_extent_c(
                                               (int[]){0, 5, 2, 8, 3, 9}),
                                             3, 2, 3),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_include(jcntrl_extent_c(
                                               (int[]){0, 5, 2, 8, 3, 9}),
                                             -1, 2, 3),
                       ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_include(jcntrl_extent_c(
                                               (int[]){0, 5, 2, 8, 3, 9}),
                                             5, 2, 3),
                       ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_include(jcntrl_extent_c(
                                               (int[]){0, 5, 2, 8, 3, 9}),
                                             0, 2, 3),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_include(jcntrl_extent_c(
                                               (int[]){0, 5, 2, 8, 3, 9}),
                                             0, 7, 3),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_include(jcntrl_extent_c(
                                               (int[]){0, 5, 2, 8, 3, 9}),
                                             0, 1, 3),
                       ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_include(jcntrl_extent_c(
                                               (int[]){0, 5, 2, 8, 3, 9}),
                                             0, 8, 3),
                       ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_include(jcntrl_extent_c(
                                               (int[]){0, 5, 2, 8, 3, 9}),
                                             5, 2, 3),
                       ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_include(jcntrl_extent_c(
                                               (int[]){0, 5, 2, 8, 3, 9}),
                                             0, 2, 3),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_include(jcntrl_extent_c(
                                               (int[]){0, 5, 2, 8, 3, 9}),
                                             0, 2, 8),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_include(jcntrl_extent_c(
                                               (int[]){0, 5, 2, 8, 3, 9}),
                                             0, 2, 2),
                       ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl_extent_include(jcntrl_extent_c(
                                               (int[]){0, 5, 2, 8, 3, 9}),
                                             0, 2, 9),
                       ==, 0))
    ret = 1;

  return ret;
}
