#ifndef LPTX_TEST_H
#define LPTX_TEST_H

#include "jupiter/lptx/vector.h"

int test_lptx_param(void);
int test_lptx_particle(void);
int test_lptx_overflow(void);
int test_lptx_util(void);
int test_lptx_comm(void);
int test_lptx_collision(void);

int test_compare_lptxvec_cmp(void *got, void *exp, void *arg);
int test_compare_lptxvec_prn(char **buf, void *d, void *a);

#define test_compare_x_lptx_u(...) __VA_ARGS__

/**
 * Equivalent to test_compare_dd for each vector components
 *
 * This macro is for @p exp that is not literal
 */
#define test_compare_x_lptxvec(got, op, exp, desc, file, line)                 \
  (check_op_str((#op "\0\0\0")) &&                                             \
   test_compare_typed(((LPTX_vector *)(const LPTX_vector *){&(got)}),          \
                      ((LPTX_vector *)(const LPTX_vector *){&(exp)}),          \
                      &((enum test_compare_op){test_compare_op_s(#op)}), desc, \
                      (file), (line), test_compare_lptxvec_cmp,                \
                      test_compare_lptxvec_prn, test_compare_lptxvec_prn,      \
                      NULL))

/**
 * Equivalent to test_compare_dd for each vector components
 *
 * This macro is for literal @p exp (Pass `(x, y, z)` for @p exp)
 */
#define test_compare_x_lptxvecl(got, op, exp, desc, file, line)             \
  test_compare_x_lptxvec(got, op, (LPTX_vector){test_compare_x_lptx_u exp}, \
                         desc, file, line)

#define test_compare_lptxvec(got, op, exp)                              \
  test_compare_x_lptxvec(got, op, exp, #got " " #op " " #exp, __FILE__, \
                         __LINE__)

#define test_compare_lptxvecl(got, op, exp)                              \
  test_compare_x_lptxvecl(got, op, exp, #got " " #op " " #exp, __FILE__, \
                          __LINE__)

#endif
