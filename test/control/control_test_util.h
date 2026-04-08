#ifndef CONTROL_TEST_UTIL_H
#define CONTROL_TEST_UTIL_H

#include "jupiter/control/defs.h"
#include "jupiter/control/extent.h"
#include "test-util.h"

int test_jcntrl_irange_cmp(void *got, void *exp, void *arg);
int test_jcntrl_irange_prn(char **buf, void *val, void *arg);

int test_jcntrl_extent_cmp(void *got, void *exp, void *arg);
int test_jcntrl_extent_prn(char **buf, void *val, void *arg);

int test_jcntrl_chstr_cmp(void *got, void *exp, void *arg);
int test_jcntrl_chstr_prn(char **buf, void *val, void *arg);

int test_jcntrl_chi_cmp(void *got, void *exp, void *arg);
int test_jcntrl_chd_cmp(void *got, void *exp, void *arg);
int test_jcntrl_cheps_cmp(void *got, void *exp, void *arg);

static inline void *test_jcntrl_irange_wrap(jcntrl_irange r, jcntrl_irange *w)
{
  *w = r;
  return w;
}

static inline void *test_jcntrl_extent_wrap(jcntrl_extent r, jcntrl_extent *w)
{
  *w = r;
  return w;
}

#define test_compare_jcntrl_irange_x(got, exp, desc, file, line)          \
  test_compare_typed(test_jcntrl_irange_wrap((got), &(jcntrl_irange){0}), \
                     test_jcntrl_irange_wrap((exp), &(jcntrl_irange){0}), \
                     NULL, desc, (file), (line), test_jcntrl_irange_cmp,  \
                     test_jcntrl_irange_prn, test_jcntrl_irange_prn, NULL)

#define test_compare_jcntrl_irange_l(got, exp, file, line) \
  test_compare_jcntrl_irange_x(got, exp, #got " == " #exp, file, line)

#define test_compare_jcntrl_irange(got, exp) \
  test_compare_jcntrl_irange_x(got, exp, #got " == " #exp, __FILE__, __LINE__)

#define test_compare_jcntrl_extent_x(got, exp, desc, file, line)          \
  test_compare_typed(test_jcntrl_extent_wrap((got), &(jcntrl_extent){0}), \
                     test_jcntrl_extent_wrap((exp), &(jcntrl_extent){0}), \
                     NULL, desc, (file), (line), test_jcntrl_extent_cmp,  \
                     test_jcntrl_extent_prn, test_jcntrl_extent_prn, NULL)

#define test_compare_jcntrl_extent_l(got, exp, file, line) \
  test_compare_jcntrl_extent_x(got, exp, #got " == " #exp, file, line)

#define test_compare_jcntrl_extent(got, exp) \
  test_compare_jcntrl_extent_x(got, exp, #got " == " #exp, __FILE__, __LINE__)

#define test_compare_jcntrl_chstr_x(got, exp, desc, file, line)               \
  test_compare_typed(*((jcntrl_data_array **){&got}), &((const char *){exp}), \
                     NULL, desc, (file), (line), test_jcntrl_chstr_cmp,       \
                     test_jcntrl_chstr_prn, test_compare_s_prn, NULL)

#define test_compare_jcntrl_chstr_l(got, op, exp, file, line)                 \
  test_compare_jcnrtl_chstr_x_ii(got, exp, "strcmp(" #got ", " #exp ") == 0", \
                                 file, line)

#define test_compare_jcntrl_chstr(got, exp)                                \
  test_compare_jcntrl_chstr_x(got, exp, "strcmp(" #got ", " #exp ") == 0", \
                              __FILE__, __LINE__)

#define test_compare_jcntrl_ch_x_ii(got, op, exp, desc, file, line)            \
  (check_op_str((#op "\0\0\0")) &&                                             \
   test_compare_typed(*((jcntrl_data_array **){&got}), &((intmax_t){exp}),     \
                      &((enum test_compare_op){test_compare_op_s(#op)}), desc, \
                      (file), (line), test_jcntrl_chi_cmp,                     \
                      test_jcntrl_chstr_prn, test_compare_i_prn, NULL))

#define test_compare_jcntrl_ch_l_ii(got, op, exp, file, line)                  \
  test_compare_jcnrtl_ch_x_ii(got, op, exp, "strtoll(" #got ") " #op " " #exp, \
                              file, line)

#define test_compare_jcntrl_chii(got, op, exp)                                 \
  test_compare_jcntrl_ch_x_ii(got, op, exp, "strtoll(" #got ") " #op " " #exp, \
                              __FILE__, __LINE__)

#define test_compare_jcntrl_ch_x_dd(got, op, exp, desc, file, line)            \
  (check_op_str((#op "\0\0\0")) &&                                             \
   test_compare_typed(*((jcntrl_data_array **){&got}), &((double){exp}),       \
                      &((enum test_compare_op){test_compare_op_s(#op)}), desc, \
                      (file), (line), test_jcntrl_chd_cmp,                     \
                      test_jcntrl_chstr_prn, test_compare_d_prn, NULL))

#define test_compare_jcntrl_ch_l_dd(got, op, exp, file, line)                 \
  test_compare_jcntrl_ch_x_dd(got, op, exp, "strtod(" #got ") " #op " " #exp, \
                              file, line)

#define test_compare_jcntrl_chdd(got, op, exp)                                \
  test_compare_jcntrl_ch_x_dd(got, op, exp, "strtod(" #got ") " #op " " #exp, \
                              __FILE__, __LINE__)

#define test_compare_jcntrl_ch_x_eps(got, exp, eps, desc, file, line)      \
  test_compare_typed(*((jcntrl_data_array **){&got}), &((double){exp}),    \
                     &((struct test_compare_eps_d){eps}), desc, (file),    \
                     (line), test_jcntrl_cheps_cmp, test_jcntrl_chstr_prn, \
                     test_compare_d_prn, NULL)

#define test_compare_jcntrl_ch_l_eps(got, exp, eps, file, line)             \
  test_compare_jcntrl_ch_x_eps(got, exp, eps,                               \
                               "fabs(strtod(" #got ") - " #exp ") < " #eps, \
                               file, line)

#define test_compare_jcntrl_cheps(got, exp, eps)                            \
  test_compare_jcntrl_ch_x_eps(got, exp, eps,                               \
                               "fabs(strtod(" #got ") - " #exp ") < " #eps, \
                               __FILE__, __LINE__)

#endif
