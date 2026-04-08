
#include <string.h>

#include "control_test.h"
#include "jupiter/control/defs.h"
#include "jupiter/geometry/util.h"
#include "test-util.h"

#include <jupiter/control/error.h>
#include <jupiter/control/information.h>
#include <jupiter/control/shared_object.h>
#include <jupiter/control/shared_object_priv.h>

static int callback_test_locked(void *data, jcntrl_information *info);
static int callback_test_locked_loop(void *data, jcntrl_information *info);

struct dummy_object
{
  jcntrl_shared_object object;
};
#define dummy_object__ancestor jcntrl_shared_object
#define dummy_object__dnmem object
JCNTRL_VTABLE_NONE(dummy_object);

static void *dummy_object_downcast(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL_T(dummy_object, struct dummy_object, obj);
}

static void dummy_object_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = dummy_object_downcast;
}

static JCNTRL_SHARED_METADATA_INIT_DEFINE(dummy_object, dummy_object_init_func)

  int test_control_information(void)
{
  struct dummy_object so;
  int ret;
  jcntrl_information *info;
  ret = 0;

  jcntrl_shared_object_static_init(&so.object, dummy_object_metadata_init());

  info = jcntrl_information_new();
  if (!info)
    return 1;

  if (!test_compare_ii(jcntrl_information_set_const_string(
                         info, JCNTRL_INFO_ERROR_SOURCE_FILE, "test"),
                       ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_information_has(info,
                                              JCNTRL_INFO_ERROR_SOURCE_FILE),
                       ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_information_has(info,
                                              JCNTRL_INFO_ERROR_SOURCE_LINE),
                       ==, 0))
    ret = 1;
  if (!test_compare_ss(
        jcntrl_information_get_string(info, JCNTRL_INFO_ERROR_SOURCE_FILE),
        "test"))
    ret = 1;
  if (!test_compare_ii(jcntrl_information_set_const_string(
                         info, JCNTRL_INFO_ERROR_SOURCE_FILE, "another"),
                       ==, 1))
    ret = 1;
  if (!test_compare_ss(
        jcntrl_information_get_string(info, JCNTRL_INFO_ERROR_SOURCE_FILE),
        "another"))
    ret = 1;

  /*
   * information key should be a literal value. It's not allowed to
   * use different type other than expected one for specified key, at
   * compile time. So, such test has not been run.
   *
   * if (!test_compare_ii(jcntrl_information_set_const_string(info,
   *     JCNTRL_INFO_ERROR_SOURCE_LINE, "this key expects integer"), ==, 0))
   *   ret = 1;
   */

  jcntrl_error_callback_set(NULL, NULL);
  if (!test_compare_ii(jcntrl_information_set_bool(info, JCNTRL_INFO_LOCKED, 1),
                       ==, 1))
    ret = 1;
  if (!test_compare_ii(jcntrl_information_set_integer(
                         info, JCNTRL_INFO_ERROR_SOURCE_LINE, 100),
                       ==, 0))
    ret = 1;
  if (!test_compare_ii(jcntrl_information_has(info,
                                              JCNTRL_INFO_ERROR_SOURCE_LINE),
                       ==, 0))
    ret = 1;

  {
    int data;
    data = 0;

    jcntrl_error_callback_set(callback_test_locked, &data);
    if (!test_compare_ii(jcntrl_information_set_bool(info, JCNTRL_INFO_LOCKED,
                                                     0),
                         ==, 0))
      ret = 1;
    if (!test_compare_ii(data, ==, 1))
      ret = 1;
    jcntrl_error_callback_set(NULL, NULL);
  }

  {
    int data;
    data = 1024;
    jcntrl_error_callback_set(callback_test_locked_loop, &data);
    if (!test_compare_ii(jcntrl_information_set_bool(info, JCNTRL_INFO_LOCKED,
                                                     0),
                         ==, 0))
      ret = 1;
    if (!test_compare_ii(data, ==, 0))
      ret = 1;
    jcntrl_error_callback_set(NULL, NULL);
  }

  jcntrl_error_callback_set(NULL, NULL);

  if (test_compare_c(!jcntrl_information_unlink(info)))
    ret = 1;
  return ret;
}

static int callback_test_locked(void *data, jcntrl_information *info)
{
  int v;
  v = jcntrl_information_get_integer(info, JCNTRL_INFO_ERROR_NUMBER);
  if (v == JCNTRL_ERROR_INFORMATION_LOCKED) {
    *(int *)data = 1;
  }
  return 0;
}

static int callback_test_locked_loop(void *data, jcntrl_information *info)
{
  static int lv = 0;
  int dval;
  if (lv + 1 > 20)
    return 0;
  lv++;
  dval = 0;
  if (!test_compare_ii(
        jcntrl_information_set_integer(info, JCNTRL_INFO_ERROR_SOURCE_LINE, lv),
        ==, 0))
    dval = -1;
  lv--;
  if (!dval) {
    dval = lv;
  }
  *(int *)data = dval;
  return 0;
}
