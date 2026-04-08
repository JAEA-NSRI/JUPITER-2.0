
#include "jupiter/control/data_array.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/extent.h"
#include "control_test_util.h"
#include "jupiter/control/shared_object.h"
#include "jupiter/control/static_array.h"
#include "test-util.h"

#include <stdint.h>

int test_jcntrl_irange_cmp(void *got, void *exp, void *arg)
{
  jcntrl_irange *pgot, *pexp;
  pgot = (jcntrl_irange *)got;
  pexp = (jcntrl_irange *)exp;
  return pgot->is == pexp->is && pgot->ie == pexp->ie;
}

int test_jcntrl_irange_prn(char **buf, void *val, void *arg)
{
  jcntrl_irange *p = (jcntrl_irange *)val;
  return test_compare_asprintf(buf, "(%d...%d)", p->is, p->ie);
}

int test_jcntrl_extent_cmp(void *got, void *exp, void *arg)
{
  jcntrl_extent *pgot, *pexp;
  pgot = (jcntrl_extent *)got;
  pexp = (jcntrl_extent *)exp;
  for (int i = 0; i < 6; ++i) {
    if (pgot->extent[i] != pexp->extent[i])
      return 0;
  }
  return 1;
}

int test_jcntrl_extent_prn(char **buf, void *val, void *arg)
{
  jcntrl_extent *p = (jcntrl_extent *)val;
  return test_compare_asprintf(buf, "(%d...%d, %d...%d, %d...%d)", //
                               p->extent[0], p->extent[1], p->extent[2],
                               p->extent[3], p->extent[4], p->extent[5]);
}

struct test_jcntrl_chstr_cmp_data
{
  void *exp;
  void *arg;
  int ret;
};

static void test_jcntrl_chstr_cmp_wrk(char *str, jcntrl_size_type len,
                                      void *arg)
{
  struct test_jcntrl_chstr_cmp_data *p = arg;
  p->ret = test_compare_ss_cmp(&str, p->exp, p->arg);
}

int test_jcntrl_chstr_cmp(void *got, void *exp, void *arg)
{
  struct test_jcntrl_chstr_cmp_data darg = { .exp = exp, .arg = arg, .ret = 0 };
  jcntrl_data_array *d = got;
  if (!d)
    return 0;

  if (!jcntrl_char_array_copyable(d))
    return 0;

  jcntrl_char_array_work_cstr(d, test_jcntrl_chstr_cmp_wrk, &darg);
  return darg.ret;
}

struct test_jcntrl_chstr_prn_data
{
  char **buf;
  int ret;
};

static void test_jcntrl_chstr_prn_wrk(char *str, jcntrl_size_type len,
                                      void *arg)
{
  struct test_jcntrl_chstr_prn_data *p = arg;
  p->ret = test_compare_s_prn(p->buf, &str, NULL);
}

int test_jcntrl_chstr_prn(char **buf, void *val, void *arg)
{
  struct test_jcntrl_chstr_prn_data darg = { .buf = buf, .ret = -1 };
  jcntrl_data_array *d = val;
  if (!d)
    return test_compare_asprintf(buf, "%p", d);

  if (!jcntrl_char_array_copyable(d)) {
    jcntrl_shared_object *o;
    o = jcntrl_data_array_object(d);
    return test_compare_asprintf(buf, "%p (%s)",
                                 d, jcntrl_shared_object_class_name(o));
  }

  jcntrl_char_array_work_cstr(d, test_jcntrl_chstr_prn_wrk, &darg);
  return darg.ret;
}

int test_jcntrl_chi_cmp(void *got, void *exp, void *arg)
{
  intmax_t v = jcntrl_char_array_strtoll((jcntrl_data_array *)got, 0, NULL);
  return test_compare_ii_cmp(&v, exp, arg);
}

int test_jcntrl_chd_cmp(void *got, void *exp, void *arg)
{
  double v = jcntrl_char_array_strtod((jcntrl_data_array *)got, NULL);
  return test_compare_dd_cmp(&v, exp, arg);
}

int test_jcntrl_cheps_cmp(void *got, void *exp, void *arg)
{
  double v = jcntrl_char_array_strtod((jcntrl_data_array *)got, NULL);
  return test_compare_eps_cmp(&v, exp, arg);
}

int test_control_util(void)
{
  int ret;
  jcntrl_irange x;
  jcntrl_extent a;
  jcntrl_static_int_array id;
  jcntrl_static_cstr_array chd;
  jcntrl_data_array *d;

  ret = 0;

  x = (jcntrl_irange){1, 2};
  if (!test_compare_ii(test_compare_jcntrl_irange(x, ((jcntrl_irange){1, 2})),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(test_compare_jcntrl_irange(x, ((jcntrl_irange){1, 1})),
                       ==, 0))
    ret = 1;

  if (!test_compare_ii(test_compare_jcntrl_irange(x, ((jcntrl_irange){2, 2})),
                       ==, 0))
    ret = 1;

  a = (jcntrl_extent){.extent = {1, 2, 3, 4, 5, 6}};
  if (!test_compare_ii(test_compare_jcntrl_extent(a,
                                                  ((jcntrl_extent){1, 2, 3, 4,
                                                                   5, 6})),
                       ==, 1))
    ret = 1;

  if (!test_compare_ii(test_compare_jcntrl_extent(a,
                                                  ((jcntrl_extent){2, 2, 3, 4,
                                                                   5, 6})),
                       ==, 0))
    ret = 1;

  if (!test_compare_ii(test_compare_jcntrl_extent(a,
                                                  ((jcntrl_extent){1, 2, 3, 4,
                                                                   5, 7})),
                       ==, 0))
    ret = 1;

  jcntrl_static_int_array_init_b(&id, 0, 1, 2);
  jcntrl_static_cstr_array_init_base(&chd, "12.375", 6);
  d = jcntrl_static_cstr_array_data(&chd);
  if (d) {
    if (!test_compare_ii(test_compare_jcntrl_chstr(d, "12.375"), ==, 1))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_chstr(d, "12.374"), ==, 0))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_chstr(d, "12.37"), ==, 0))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_chstr(d, "12.3756"), ==, 0))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_chii(d, ==, 12), ==, 1))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_chii(d, ==, 11), ==, 0))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_chii(d, >, 11), ==, 1))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_chii(d, >, 13), ==, 0))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_chdd(d, ==, 12.375), ==, 1))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_chdd(d, ==, 11.375), ==, 0))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_chdd(d, >, 11.375), ==, 1))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_chdd(d, >, 13.375), ==, 0))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_cheps(d, 12.375, 1.0), ==, 1))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_cheps(d, 13.375, 1.0), ==, 0))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_cheps(d, 11.375, 1.0), ==, 0))
      ret = 1;
  } else {
    ret = 1;
  }

  d = jcntrl_static_int_array_data(&id);
  if (d) {
    if (!test_compare_ii(test_compare_jcntrl_chstr(d, "12.375"), ==, 0))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_chii(d, ==, 12), ==, 0))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_chdd(d, ==, 12.375), ==, 0))
      ret = 1;

    if (!test_compare_ii(test_compare_jcntrl_cheps(d, 12.375, 1.0), ==, 0))
      ret = 1;
  } else {
    ret = 1;
  }

  return ret;
}
