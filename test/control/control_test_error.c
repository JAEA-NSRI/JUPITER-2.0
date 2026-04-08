
#include <stdlib.h>
#include <string.h>

#include "control_test.h"

#include <jupiter/control/error.h>
#include <jupiter/control/defs.h>
#include <jupiter/control/information.h>
#include "jupiter/control/data_array.h"
#include "jupiter/control/geometry.h"
#include "jupiter/control/shared_object.h"
#include "test-util.h"

struct error_callback_test_data
{
  int called;
  int *ret;
  int (*checker_function)(jcntrl_information *info, int *ret);
};

static int test_callback_function(void *data, jcntrl_information *info)
{
  struct error_callback_test_data *edata;
  edata = (struct error_callback_test_data *)data;
  if (!edata)
    return -2;
  if (!edata->checker_function)
    return -1;
  edata->called = 1;
  return edata->checker_function(info, edata->ret);
}

static int allocation_failed_checker(jcntrl_information *info, int *ret);
static int information_locked_error_checker(jcntrl_information *info, int *ret);
static int overflow_error_checker(jcntrl_information *info, int *ret);
static int argument_error_checker(jcntrl_information *info, int *ret);
static int index_error_checker(jcntrl_information *info, int *ret);
static int information_type_error_checker(jcntrl_information *info, int *ret);
static int element_type_error_checker(jcntrl_information *info, int *ret);
static int loop_detected_error_checker(jcntrl_information *info, int *ret);
static int datatype_error_checker(jcntrl_information *info, int *ret);
static int pure_virtual_error_checker(jcntrl_information *info, int *ret);

enum checker_func_retid
{
  allocation_failed_checker_ret = 10,
  information_locked_error_checker_ret,
  overflow_error_checker_ret,
  argument_error_checker_ret,
  index_error_checker_ret,
  information_type_error_checker_ret,
  element_type_error_checker_ret,
  loop_detected_error_checker_ret,
  datatype_error_checker_ret,
  pure_virtual_error_checker_ret,
};

int test_control_error(void)
{
  int ret;
  struct error_callback_test_data test_data;

  ret = 0;
  test_data.ret = &ret;
  test_data.checker_function = NULL;

  jcntrl_error_callback_set(test_callback_function, NULL);
  if (!test_compare_pp(jcntrl_error_callback_get(), ==, test_callback_function))
    ret = 1;

  jcntrl_error_callback_set(NULL, NULL);
  if (!test_compare_ii(jcntrl_raise_allocation_failed(__FILE__, __LINE__), ==,
                       0))
    ret = 1;

  if (!test_compare_pp(jcntrl_error_callback_set(test_callback_function, NULL),
                       ==, NULL))
    ret = 1;

  if (!test_compare_ii(jcntrl_raise_allocation_failed(__FILE__, __LINE__), ==,
                       -2))
    ret = 1;

  jcntrl_error_callback_set(test_callback_function, &test_data);
  if (!test_compare_ii(jcntrl_raise_allocation_failed(__FILE__, __LINE__), ==,
                       -1))
    ret = 1;

  test_data.called = 0;
  test_data.checker_function = allocation_failed_checker;
  if (!test_compare_ii(jcntrl_raise_allocation_failed("test_file.c", 1000), ==,
                       allocation_failed_checker_ret))
    ret = 1;
  if (!test_compare_ii(test_data.called, ==, 1))
    ret = 1;

  test_data.called = 0;
  test_data.checker_function = information_locked_error_checker;
  if (!test_compare_ii(jcntrl_raise_information_locked_error("boom.c", 1001),
                       ==, information_locked_error_checker_ret))
    ret = 1;
  if (!test_compare_ii(test_data.called, ==, 1))
    ret = 1;

  test_data.called = 0;
  test_data.checker_function = overflow_error_checker;
  if (!test_compare_ii(jcntrl_raise_overflow_error("bang.c", 1002), ==,
                       overflow_error_checker_ret))
    ret = 1;
  if (!test_compare_ii(test_data.called, ==, 1))
    ret = 1;

  test_data.called = 0;
  test_data.checker_function = argument_error_checker;
  if (!test_compare_ii(jcntrl_raise_argument_error("dong.c", 1003,
                                                   "How am I wrong?"),
                       ==, argument_error_checker_ret))
    ret = 1;
  if (!test_compare_ii(test_data.called, ==, 1))
    ret = 1;

  test_data.called = 0;
  test_data.checker_function = index_error_checker;
  if (!test_compare_ii(jcntrl_raise_index_error("ding.c", 1004, 104), ==,
                       index_error_checker_ret))
    ret = 1;
  if (!test_compare_ii(test_data.called, ==, 1))
    ret = 1;

  test_data.called = 0;
  test_data.checker_function = information_type_error_checker;
  if (!test_compare_ii(
        jcntrl_raise_information_type_error("moo.c", 1005, JCNTRL_INFO_DATATYPE,
                                            JCNTRL_IDATATYPE_OBJECT),
        ==, information_type_error_checker_ret))
    ret = 1;
  if (!test_compare_ii(test_data.called, ==, 1))
    ret = 1;

  test_data.called = 0;
  test_data.checker_function = element_type_error_checker;
  if (!test_compare_ii(jcntrl_raise_element_type_error("meow.c", 1006,
                                                       JCNTRL_EL_CHAR,
                                                       JCNTRL_EL_INT),
                       ==, element_type_error_checker_ret))
    ret = 1;
  if (!test_compare_ii(test_data.called, ==, 1))
    ret = 1;

  test_data.called = 0;
  test_data.checker_function = loop_detected_error_checker;
  if (!test_compare_ii(jcntrl_raise_loop_detected_error("nyan.c", 1007, "ups",
                                                        "dps"),
                       ==, loop_detected_error_checker_ret))
    ret = 1;
  if (!test_compare_ii(test_data.called, ==, 1))
    ret = 1;

  test_data.called = 0;
  test_data.checker_function = datatype_error_checker;
  if (!test_compare_ii(jcntrl_raise_upstream_type_error(
                         "luna.c", 1008, "matsuri", "fubuki",
                         JCNTRL_DATATYPE_FIELD_FUN, JCNTRL_DATATYPE_GEOMETRY,
                         jcntrl_geometry_metadata_init()),
                       ==, datatype_error_checker_ret))
    ret = 1;
  if (!test_compare_ii(test_data.called, ==, 1))
    ret = 1;

  test_data.called = 0;
  test_data.checker_function = pure_virtual_error_checker;
  if (!test_compare_ii(jcntrl_raise_pure_virtual_error(
                         "bow.c", 999, jcntrl_shared_object_metadata_init(),
                         jcntrl_data_array_metadata_init(), "test"),
                       ==, pure_virtual_error_checker_ret))
    ret = 1;
  if (!test_compare_ii(test_data.called, ==, 1))
    ret = 1;

  jcntrl_error_callback_set(error_handler, NULL);
  return ret;
}

#define test_compare_info_i(info, key, op, exp)                          \
  (test_compare_x_ii(jcntrl_information_get_required_type(key), ==,      \
                     JCNTRL_IDATATYPE_INTEGER,                           \
                     "jcntrl_information_get_required_type(" #key        \
                     ") == JCNTRL_IDATATYPE_INTEGER",                    \
                     __FILE__, __LINE__) &&                              \
   test_compare_x_ii(jcntrl_information_get_integer(info, key), op, exp, \
                     "jcntrl_information_get_integer(" #info ", " #key   \
                     ") " #op " " #exp,                                  \
                     __FILE__, __LINE__))

/// TODO: Currently nothing returns JCNTRL_IDATATYPE_STRING values.
#define test_compare_info_s(info, key, exp)                                  \
  (test_compare_x_ii(jcntrl_information_get_required_type(key), ==,          \
                     JCNTRL_IDATATYPE_CSTRING,                               \
                     "jcntrl_information_get_required_type(" #key            \
                     ") == JCNTRL_IDATATYPE_CSTRING",                        \
                     __FILE__, __LINE__) &&                                  \
   test_compare_x_ss(jcntrl_information_get_string(info, key), exp,          \
                     "strcmp(jcntrl_information_get_string(" #info ", " #key \
                     "), " #exp ") == 0",                                    \
                     __FILE__, __LINE__))

#define test_compare_info_d(info, key, op, exp)                           \
  (test_compare_x_ii(jcntrl_information_get_required_type(key), ==,       \
                     JCNTRL_IDATATYPE_DATATYPE,                           \
                     "jcntrl_information_get_required_type(" #key         \
                     ") == JCNTRL_IDATATYPE_DATATYPE",                    \
                     __FILE__, __LINE__) &&                               \
   test_compare_x_ii(jcntrl_information_get_datatype(info, key), op, exp, \
                     "jcntrl_information_get_datatype(" #info ", " #key   \
                     ") " #op " " #exp,                                   \
                     __FILE__, __LINE__))

#define test_compare_info_o(info, key, op, exp)                             \
  (test_compare_x_ii(jcntrl_information_get_required_type(key), ==,         \
                     JCNTRL_IDATATYPE_OBJECTTYPE,                           \
                     "jcntrl_information_get_required_type(" #key           \
                     ") == JCNTRL_IDATATYPE_OBJECTTYPE",                    \
                     __FILE__, __LINE__) &&                                 \
   test_compare_x_pp(jcntrl_information_get_objecttype(info, key), op, exp, \
                     "jcntrl_information_get_objecttype(" #info ", " #key   \
                     ") " #op " " #exp,                                     \
                     __FILE__, __LINE__))

static int allocation_failed_checker(jcntrl_information *info, int *ret)
{
  if (!ret)
    return 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_NUMBER, ==,
                           JCNTRL_ERROR_ALLOCATE))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_SOURCE_FILE, "test_file.c"))
    *ret = 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_SOURCE_LINE, ==, 1000))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_MESSAGE,
                           "Allocation Failed"))
    *ret = 1;

  return allocation_failed_checker_ret;
}

static int information_locked_error_checker(jcntrl_information *info, int *ret)
{
  if (!ret)
    return 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_NUMBER, ==,
                           JCNTRL_ERROR_INFORMATION_LOCKED))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_SOURCE_FILE, "boom.c"))
    *ret = 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_SOURCE_LINE, ==, 1001))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_MESSAGE,
                           "Attempted to modify a locked information"))
    *ret = 1;

  return information_locked_error_checker_ret;
}

static int overflow_error_checker(jcntrl_information *info, int *ret)
{
  if (!ret)
    return 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_NUMBER, ==,
                           JCNTRL_ERROR_OVERFLOW))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_SOURCE_FILE, "bang.c"))
    *ret = 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_SOURCE_LINE, ==, 1002))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_MESSAGE,
                           "Arithmetic overflow detected"))
    *ret = 1;

  return overflow_error_checker_ret;
}

static int argument_error_checker(jcntrl_information *info, int *ret)
{
  if (!ret)
    return 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_NUMBER, ==,
                           JCNTRL_ERROR_ARGUMENT))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_SOURCE_FILE, "dong.c"))
    *ret = 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_SOURCE_LINE, ==, 1003))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_MESSAGE, "How am I wrong?"))
    *ret = 1;

  return argument_error_checker_ret;
}

static int index_error_checker(jcntrl_information *info, int *ret)
{
  if (!ret)
    return 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_NUMBER, ==,
                           JCNTRL_ERROR_INDEX))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_SOURCE_FILE, "ding.c"))
    *ret = 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_SOURCE_LINE, ==, 1004))
    *ret = 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_INDEX, ==, 104))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_MESSAGE,
                           "Index out-of-range"))
    *ret = 1;

  return index_error_checker_ret;
}

static int information_type_error_checker(jcntrl_information *info, int *ret)
{
  if (!ret)
    return 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_NUMBER, ==,
                           JCNTRL_ERROR_INFORMATION_TYPE))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_SOURCE_FILE, "moo.c"))
    *ret = 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_SOURCE_LINE, ==, 1005))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_MESSAGE,
                           "Invalid type used for specified information key. "
                           "This error is considered bug"))
    *ret = 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_INFO_KEY, ==,
                           JCNTRL_INFO_DATATYPE))
    *ret = 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_INFO_TYPE, ==,
                           JCNTRL_IDATATYPE_OBJECT))
    *ret = 1;

  return information_type_error_checker_ret;
}

static int element_type_error_checker(jcntrl_information *info, int *ret)
{
  if (!ret)
    return 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_NUMBER, ==,
                           JCNTRL_ERROR_ELEMENT_TYPE))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_SOURCE_FILE, "meow.c"))
    *ret = 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_SOURCE_LINE, ==, 1006))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_MESSAGE,
                           "Requested array type does not match to data array. "
                           "This error is considered bug"))
    *ret = 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_ELTYPE_EXPECT, ==,
                           JCNTRL_EL_CHAR))
    *ret = 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_ELTYPE_REQUEST, ==,
                           JCNTRL_EL_INT))
    *ret = 1;

  return element_type_error_checker_ret;
}

static int loop_detected_error_checker(jcntrl_information *info, int *ret)
{
  if (!ret)
    return 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_NUMBER, ==,
                           JCNTRL_ERROR_LOOP_DETECTED))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_SOURCE_FILE, "nyan.c"))
    *ret = 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_SOURCE_LINE, ==, 1007))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_MESSAGE,
                           "Connection loop detected"))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_EXEC_UP, "ups"))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_EXEC_DOWN, "dps"))
    *ret = 1;

  return loop_detected_error_checker_ret;
}

static int datatype_error_checker(jcntrl_information *info, int *ret)
{
  if (!ret)
    return 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_NUMBER, ==,
                           JCNTRL_ERROR_DATATYPE_ERROR))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_SOURCE_FILE, "luna.c"))
    *ret = 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_SOURCE_LINE, ==, 1008))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_MESSAGE,
                           "Provided data type from upstream executive does "
                           "not fit the input requirement"))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_EXEC_UP, "matsuri"))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_EXEC_DOWN, "fubuki"))
    *ret = 1;

  if (!test_compare_info_d(info, JCNTRL_INFO_ERROR_TYPE_GIVEN, ==,
                           JCNTRL_DATATYPE_GEOMETRY))
    *ret = 1;

  if (!test_compare_info_o(info, JCNTRL_INFO_ERROR_OBJECT_GIVEN, ==,
                           jcntrl_geometry_metadata_init()))
    *ret = 1;

  if (!test_compare_info_d(info, JCNTRL_INFO_ERROR_TYPE_REQUIRED, ==,
                           JCNTRL_DATATYPE_FIELD_FUN))
    *ret = 1;

  return datatype_error_checker_ret;
}

static int pure_virtual_error_checker(jcntrl_information *info, int *ret)
{
  if (!ret)
    return 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_NUMBER, ==,
                           JCNTRL_ERROR_PURE_VIRTUAL))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_SOURCE_FILE, "bow.c"))
    *ret = 1;

  if (!test_compare_info_i(info, JCNTRL_INFO_ERROR_SOURCE_LINE, ==, 999))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_MESSAGE,
                           "Pure virtual (unbound function) called"))
    *ret = 1;

  if (!test_compare_info_o(info, JCNTRL_INFO_ERROR_VIRTUAL_BASE, ==,
                           jcntrl_shared_object_metadata_init()))
    *ret = 1;

  if (!test_compare_info_o(info, JCNTRL_INFO_ERROR_CLASS_CALLING, ==,
                           jcntrl_data_array_metadata_init()))
    *ret = 1;

  if (!test_compare_info_s(info, JCNTRL_INFO_ERROR_FUNCNAME, "test"))
    *ret = 1;

  return pure_virtual_error_checker_ret;
}
