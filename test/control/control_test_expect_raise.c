#include "control_test_expect_raise.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/error.h"
#include "jupiter/control/information.h"
#include "test/control/control_test.h"

struct control_test_expected_raise_check_data
{
  jcntrl_error_code errcode;
  int called;
};

/*
 * @p data is only thread-private for OpenMP manner.
 */
static struct control_test_expected_raise_check_data data;
#ifdef _OPENMP
#pragma omp threadprivate(data)
#endif

struct control_test_expected_raise_check_data *
control_test_expect_raise_data(void)
{
  return &data;
}

int control_test_check_expected_raise(void *d, jcntrl_information *info)
{
  int err;

  data.called++;
  err = jcntrl_information_get_integer(info, JCNTRL_INFO_ERROR_NUMBER);
  data.errcode = err;

  return error_handler(d, info);
}

void control_test_use_expect_raise(void)
{
  jcntrl_error_callback_set(control_test_check_expected_raise, NULL);
  begin_expected_raise();
}

void begin_expected_raise(void)
{
  data.errcode = JCNTRL_ERROR_UNKNOWN;
  data.called = 0;
}

int (test_expect_raise_one)(void) { return data.called > 0; }
int (test_expect_raise_exactly_one)(void) { return data.called < 2; }
int (test_expect_raise_code)(int code) { return data.errcode == code; }
int (test_expect_not_raised)(void) { return data.called == 0; }
int control_expect_last_error(void) { return data.errcode; }
