#include "jupiter/lptx/defs.h"
#include "jupiter/lptx/overflow.h"
#include "test-util.h"
#include "test/lptx/lptx-test.h"

#include <limits.h>
#include <stdint.h>

int test_lptx_overflow(void)
{
  int ir;
  LPTX_idtype sr;
  intmax_t mr;
  int ret;
  ret = 0;

  if (!test_compare_ii(LPTX__add_overflow(1, 2, -4, 4, &ir), ==, 3))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__add_overflow(1, 3, -4, 4, &ir), ==, 4))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__add_overflow(1, 4, -4, 4, &ir), ==, 5))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX__add_overflow(4, 1, -4, 4, &ir), ==, 5))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX__add_overflow(1, -4, -4, 4, &ir), ==, -3))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__add_overflow(-4, 1, -4, 4, &ir), ==, -3))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__add_overflow(-1, -4, -4, 4, &ir), ==, -5))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX__add_overflow(-4, -1, -4, 4, &ir), ==, -5))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  ir = 0;
  /*
   * Any results accepted; UndefinedBehaviorSanitizer will complain about
   * integer overflow, but that's expected.
   */
  if (!test_compare_ii((LPTX__add_overflow(INTMAX_MAX, 1, INTMAX_MIN,
                                             INTMAX_MAX, &ir),
                        ir),
                       ==, 1))
    ret = 1;

  ir = 0;
  if (!test_compare_ii((LPTX__add_overflow(INTMAX_MIN, -1, INTMAX_MIN,
                                             INTMAX_MAX, &ir),
                        ir),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX__sub_overflow(1, 4, -4, 4, &ir), ==, -3))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__sub_overflow(1, -4, -4, 4, &ir), ==, 5))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX__sub_overflow(-4, 1, -4, 4, &ir), ==, -5))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX__sub_overflow(-4, -1, -4, 4, &ir), ==, -3))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__sub_overflow(-4, -1, -4, 4, &ir), ==, -3))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  ir = 0;
  if (!test_compare_ii((LPTX__sub_overflow(INTMAX_MAX, -1, INTMAX_MIN,
                                             INTMAX_MAX, &ir),
                        ir),
                       ==, 1))
    ret = 1;

  ir = 0;
  if (!test_compare_ii((LPTX__sub_overflow(INTMAX_MIN, 1, INTMAX_MIN,
                                             INTMAX_MAX, &ir),
                        ir),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(0, 0, -10, 10, &ir), ==, 0))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(1, 1, -10, 10, &ir), ==, 1))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(-1, -1, -10, 10, &ir), ==, 1))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(2, 5, -10, 10, &ir), ==, 10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(5, 2, -10, 10, &ir), ==, 10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(-2, -5, -10, 10, &ir), ==, 10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(-5, -2, -10, 10, &ir), ==, 10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(2, -5, -10, 10, &ir), ==, -10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(5, -2, -10, 10, &ir), ==, -10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(-2, 5, -10, 10, &ir), ==, -10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(-5, 2, -10, 10, &ir), ==, -10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(-10, -1, -10, 10, &ir), ==, 10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(-1, -10, -10, 10, &ir), ==, 10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(-10, 1, -10, 10, &ir), ==, -10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(1, -10, -10, 10, &ir), ==, -10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(10, -1, -10, 10, &ir), ==, -10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(-1, 10, -10, 10, &ir), ==, -10))
    ret = 1;
  if (!test_compare_ii(ir, ==, 0))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(2, 6, -10, 10, &ir), ==, 12))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(6, 2, -10, 10, &ir), ==, 12))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(-2, -6, -10, 10, &ir), ==, 12))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(-6, -2, -10, 10, &ir), ==, 12))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(-2, 6, -10, 10, &ir), ==, -12))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(-6, 2, -10, 10, &ir), ==, -12))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(2, -6, -10, 10, &ir), ==, -12))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX__mul_overflow(6, -2, -10, 10, &ir), ==, -12))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_i_add_overflow(0, 1, &ir), ==, 0))
    ret = 1;
  if (!test_compare_ii(ir, ==, 1))
    ret = 1;

  //--- i
  if (!test_compare_ii(LPTX_i_add_overflow(INT_MAX, 1, &ir), ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_i_add_overflow(INT_MIN, -1, &ir), ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_i_sub_overflow(0, 1, &ir), ==, 0))
    ret = 1;
  if (!test_compare_ii(ir, ==, -1))
    ret = 1;

  if (!test_compare_ii(LPTX_i_sub_overflow(INT_MAX, -1, &ir), ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_i_sub_overflow(INT_MIN, 1, &ir), ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_i_mul_overflow(2, 5, &ir), ==, 0))
    ret = 1;
  if (!test_compare_ii(ir, ==, 10))
    ret = 1;

  if (!test_compare_ii(LPTX_i_mul_overflow(INT_MAX, INT_MAX, &ir), ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_i_mul_overflow(INT_MIN, INT_MIN, &ir), ==, 1))
    ret = 1;

  //--- s
  if (!test_compare_ii(LPTX_s_add_overflow(0, 1, &sr), ==, 0))
    ret = 1;
  if (!test_compare_ii(sr, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_s_add_overflow(LPTX_IDTYPE_MAX, 1, &sr), ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_s_add_overflow(LPTX_IDTYPE_MIN, -1, &sr), ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_s_sub_overflow(0, 1, &sr), ==, 0))
    ret = 1;
  if (!test_compare_ii(sr, ==, -1))
    ret = 1;

  if (!test_compare_ii(LPTX_s_sub_overflow(LPTX_IDTYPE_MAX, -1, &sr), ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_s_sub_overflow(LPTX_IDTYPE_MIN, 1, &sr), ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_s_mul_overflow(2, 5, &sr), ==, 0))
    ret = 1;
  if (!test_compare_ii(sr, ==, 10))
    ret = 1;

  if (!test_compare_ii(LPTX_s_mul_overflow(LPTX_IDTYPE_MAX, LPTX_IDTYPE_MAX,
                                             &sr),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_s_mul_overflow(LPTX_IDTYPE_MIN, LPTX_IDTYPE_MIN,
                                             &sr),
                       ==, 1))
    ret = 1;

  //--- m
  if (!test_compare_ii(LPTX_m_add_overflow(0, 1, &mr), ==, 0))
    ret = 1;
  if (!test_compare_ii(mr, ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_m_add_overflow(INTMAX_MAX, 1, &mr), ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_m_add_overflow(INTMAX_MIN, -1, &mr), ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_m_sub_overflow(0, 1, &mr), ==, 0))
    ret = 1;
  if (!test_compare_ii(mr, ==, -1))
    ret = 1;

  if (!test_compare_ii(LPTX_m_sub_overflow(INTMAX_MAX, -1, &mr), ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_m_sub_overflow(INTMAX_MIN, 1, &mr), ==, 1))
    ret = 1;

  if (!test_compare_ii(LPTX_m_mul_overflow(2, 5, &mr), ==, 0))
    ret = 1;
  if (!test_compare_ii(mr, ==, 10))
    ret = 1;

  if (!test_compare_ii(LPTX_m_mul_overflow(INTMAX_MAX, INTMAX_MAX, &mr), ==,
                       1))
    ret = 1;

  if (!test_compare_ii(LPTX_m_mul_overflow(INTMAX_MIN, INTMAX_MIN, &mr), ==,
                       1))
    ret = 1;

  return ret;
}
