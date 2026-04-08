#ifndef JUPITER_TEST_STRLIST_TEST_UTIL_H
#define JUPITER_TEST_STRLIST_TEST_UTIL_H

#include <jupiter/strlist.h>

#include "test-util.h"

int test_compare_sls_cmp(void *got, void *exp, void *arg);
int test_compare_sl_prn(char **buf, void *d, void *a);

/**
 * Compare jupiter_strlist *got and const char *exp.
 *
 * null @p got is considered to be equal null @p exp.
 */
#define test_compare_x_sls(got, exp, desc, file, line)                    \
  test_compare_typed(&((jupiter_strlist *){got}), &((const char *){exp}), \
                     NULL, desc, (file), (line), test_compare_sls_cmp,    \
                     test_compare_sl_prn, test_compare_s_prn, NULL)

#define test_compare_l_sls(got, exp, file, line) \
  test_compare_x_sls(got, exp, "strcmp(" #got ", " #exp ") == 0", file, line)

#define test_compare_sls(got, exp)                                          \
  test_compare_x_sls(got, exp, "strcmp(" #got ", " #exp ") == 0", __FILE__, \
                     __LINE__)

#endif
