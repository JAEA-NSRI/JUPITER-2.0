#include "control_test.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/overflow.h"
#include "test-util.h"
#include "control_test_expect_raise.h"

#include <stdint.h>
#include <limits.h>

int test_control_overflow(void)
{
  int ir;
  jcntrl_size_type sr;
  jcntrl_aint_type ar;
  intmax_t mr;
  int ret;
  ret = 0;

  control_test_use_expect_raise();

  if (!test_compare_ii(jcntrl__add_overflow(1, 2, -4, 4, &ir), ==, 3))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__add_overflow(1, 3, -4, 4, &ir), ==, 4))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__add_overflow(1, 4, -4, 4, &ir), ==, 5))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl__add_overflow(4, 1, -4, 4, &ir), ==, 5))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl__add_overflow(1, -4, -4, 4, &ir), ==, -3))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__add_overflow(-4, 1, -4, 4, &ir), ==, -3))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__add_overflow(-1, -4, -4, 4, &ir), ==, -5))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl__add_overflow(-4, -1, -4, 4, &ir), ==, -5))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  ir = 0;
  /*
   * Any results accepted; UndefinedBehaviorSanitizer will complain about
   * integer overflow, but that's expected.
   */
  if (!test_compare_ii((jcntrl__add_overflow(INTMAX_MAX, 1, INTMAX_MIN,
                                             INTMAX_MAX, &ir),
                        ir),
                       ==, 1))
    ret = 1;

  ir = 0;
  if (!test_compare_ii((jcntrl__add_overflow(INTMAX_MIN, -1, INTMAX_MIN,
                                             INTMAX_MAX, &ir),
                        ir),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl__sub_overflow(1, 4, -4, 4, &ir), ==, -3))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__sub_overflow(1, -4, -4, 4, &ir), ==, 5))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl__sub_overflow(-4, 1, -4, 4, &ir), ==, -5))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl__sub_overflow(-4, -1, -4, 4, &ir), ==, -3))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__sub_overflow(-4, -1, -4, 4, &ir), ==, -3))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  ir = 0;
  if (!test_compare_ii((jcntrl__sub_overflow(INTMAX_MAX, -1, INTMAX_MIN,
                                             INTMAX_MAX, &ir),
                        ir),
                       ==, 1))
    ret = 1;

  ir = 0;
  if (!test_compare_ii((jcntrl__sub_overflow(INTMAX_MIN, 1, INTMAX_MIN,
                                             INTMAX_MAX, &ir),
                        ir),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(0, 0, -10, 10, &ir), ==, 0))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(1, 1, -10, 10, &ir), ==, 1))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(-1, -1, -10, 10, &ir), ==, 1))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(2, 5, -10, 10, &ir), ==, 10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(5, 2, -10, 10, &ir), ==, 10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(-2, -5, -10, 10, &ir), ==, 10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(-5, -2, -10, 10, &ir), ==, 10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(2, -5, -10, 10, &ir), ==, -10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(5, -2, -10, 10, &ir), ==, -10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(-2, 5, -10, 10, &ir), ==, -10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(-5, 2, -10, 10, &ir), ==, -10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(-10, -1, -10, 10, &ir), ==, 10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(-1, -10, -10, 10, &ir), ==, 10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(-10, 1, -10, 10, &ir), ==, -10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(1, -10, -10, 10, &ir), ==, -10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(10, -1, -10, 10, &ir), ==, -10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(-1, 10, -10, 10, &ir), ==, -10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(2, 6, -10, 10, &ir), ==, 12))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(6, 2, -10, 10, &ir), ==, 12))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(-2, -6, -10, 10, &ir), ==, 12))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(-6, -2, -10, 10, &ir), ==, 12))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(-2, 6, -10, 10, &ir), ==, -12))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(-6, 2, -10, 10, &ir), ==, -12))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(2, -6, -10, 10, &ir), ==, -12))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(jcntrl__mul_overflow(6, -2, -10, 10, &ir), ==, -12))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_i_add_overflow(0, 1, &ir), ==, 0))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  //--- i
  begin_expected_raise();
  if (!test_compare_ii(jcntrl_i_add_overflow(INT_MAX, 1, &ir), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_i_add_overflow(INT_MIN, -1, &ir), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_i_sub_overflow(0, 1, &ir), ==, 0))
    ret = 1;
  if (!test_compare_ii(ir, ==, -1))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_i_sub_overflow(INT_MAX, -1, &ir), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_i_sub_overflow(INT_MIN, 1, &ir), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_i_mul_overflow(2, 5, &ir), ==, 0))
    ret = 1;
  if (!test_compare_ii(ir, ==, 10))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_i_mul_overflow(INT_MAX, INT_MAX, &ir), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_i_mul_overflow(INT_MIN, INT_MIN, &ir), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  //--- s
  begin_expected_raise();
  if (!test_compare_ii(jcntrl_s_add_overflow(0, 1, &sr), ==, 0))
    ret = 1;
  if (!test_compare_ii(sr, ==, 1))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_s_add_overflow(JCNTRL_SIZE_MAX, 1, &sr), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_s_add_overflow(JCNTRL_SIZE_MIN, -1, &sr), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_s_sub_overflow(0, 1, &sr), ==, 0))
    ret = 1;
  if (!test_compare_ii(sr, ==, -1))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_s_sub_overflow(JCNTRL_SIZE_MAX, -1, &sr), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_s_sub_overflow(JCNTRL_SIZE_MIN, 1, &sr), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_s_mul_overflow(2, 5, &sr), ==, 0))
    ret = 1;
  if (!test_compare_ii(sr, ==, 10))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_s_mul_overflow(JCNTRL_SIZE_MAX, JCNTRL_SIZE_MAX,
                                             &sr),
                       ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_s_mul_overflow(JCNTRL_SIZE_MIN, JCNTRL_SIZE_MIN,
                                             &sr),
                       ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  //--- a
  begin_expected_raise();
  if (!test_compare_ii(jcntrl_a_add_overflow(0, 1, &ar), ==, 0))
    ret = 1;
  if (!test_compare_ii(ar, ==, 1))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_a_add_overflow(INTPTR_MAX, 1, &ar), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_a_add_overflow(INTPTR_MIN, -1, &ar), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_a_sub_overflow(0, 1, &ar), ==, 0))
    ret = 1;
  if (!test_compare_ii(ar, ==, -1))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_a_sub_overflow(INTPTR_MAX, -1, &ar), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_a_sub_overflow(INTPTR_MIN, 1, &ar), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_a_mul_overflow(2, 5, &ar), ==, 0))
    ret = 1;
  if (!test_compare_ii(ar, ==, 10))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_a_mul_overflow(INTPTR_MAX, INTPTR_MAX, &ar), ==,
                       1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_a_mul_overflow(INTPTR_MIN, INTPTR_MIN, &ar), ==,
                       1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  //--- m
  begin_expected_raise();
  if (!test_compare_ii(jcntrl_m_add_overflow(0, 1, &mr), ==, 0))
    ret = 1;
  if (!test_compare_ii(mr, ==, 1))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_m_add_overflow(INTMAX_MAX, 1, &mr), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_m_add_overflow(INTMAX_MIN, -1, &mr), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_m_sub_overflow(0, 1, &mr), ==, 0))
    ret = 1;
  if (!test_compare_ii(mr, ==, -1))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_m_sub_overflow(INTMAX_MAX, -1, &mr), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_m_sub_overflow(INTMAX_MIN, 1, &mr), ==, 1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_m_mul_overflow(2, 5, &mr), ==, 0))
    ret = 1;
  if (!test_compare_ii(mr, ==, 10))
    ret = 1;
  if (test_expect_not_raised())
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_m_mul_overflow(INTMAX_MAX, INTMAX_MAX, &mr), ==,
                       1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  begin_expected_raise();
  if (!test_compare_ii(jcntrl_m_mul_overflow(INTMAX_MIN, INTMAX_MIN, &mr), ==,
                       1))
    ret = 1;
  if (test_expect_raise(JCNTRL_ERROR_OVERFLOW))
    ret = 1;

  return ret;
}
