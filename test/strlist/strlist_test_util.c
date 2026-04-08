#include "strlist_test_util.h"
#include "jupiter/strlist.h"
#include "test-util.h"

#include <string.h>

int test_compare_sls_cmp(void *got, void *exp, void *arg)
{
  jupiter_strlist *g = *(jupiter_strlist **)got;
  const char *e = *(const char **)exp;

  if (!e) {
    if (!g)
      return 1;
    return 0;
  }
  if (!g)
    return 0;
  return strncmp(g->buf, e, g->node.len) == 0;
}

int test_compare_sl_prn(char **buf, void *d, void *a)
{
  jupiter_strlist *base = *(jupiter_strlist **)d;
  if (base) {
    const char *str = base->buf;
    struct test_compare_ssn_d n = { .n = base->node.len };
    return test_compare_sn_prn(buf, &str, &n);
  } else {
    const char *nullp = NULL;
    return test_compare_s_prn(buf, &nullp, NULL);
  }
}
