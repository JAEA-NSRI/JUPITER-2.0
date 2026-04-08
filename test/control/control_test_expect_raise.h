#ifndef JUPITER_TEST_CONTROL_TEST_EXPECT_RAISE_H
#define JUPITER_TEST_CONTROL_TEST_EXPECT_RAISE_H

#include <jupiter/control/defs.h>

#include "test-util.h"

struct control_test_expected_raise_check_data;

struct control_test_expected_raise_check_data *
control_test_expect_raise_data(void);

/**
 * Set up expect raise test.
 */
void control_test_use_expect_raise(void);

/**
 * Error handler for expect raise
 *
 * You can call this function from your custom error handler.
 */
int control_test_check_expected_raise(void *d, jcntrl_information *info);

/**
 * Initialize expect raise
 *
 * This function is control_test_use_expect_raise() except for registering error
 * handler function. You can use this function to reset called flag.
 */
void begin_expected_raise(void);

int test_expect_raise_one(void);
int test_expect_raise_exactly_one(void);
int test_expect_raise_code(int code);
int test_expect_not_raised(void);
int control_expect_last_error(void);

/**
 * Checkes whether at least one error was raised. Error value is not considered
 * here.
 */
#define test_expect_raise_one(code)        \
  test_compare_cf(test_expect_raise_one(), \
                  "Expected to raise " #code " but nothing raised")

/**
 * Checkes whether exactly one error was raised. Error value is not considered
 * here.
 *
 * The error message is that for combining with test_expect_raise_one(). The
 * test will also fail if no errors are raised.
 */
#define test_expect_raise_exactly_one(code)        \
  test_compare_cf(test_expect_raise_exactly_one(), \
                  "Expected to raise one " #code   \
                  " but raised two or more errors")

/**
 * Checks for the error code. This function simply checks the code value only,
 * so this function does not checks whether errors are raised.
 */
#define test_expect_raise_code(code)                    \
  test_compare_cf(test_expect_raise_code(code),         \
                  "Expected to raise " #code            \
                  " but different error %d has raised", \
                  control_expect_last_error())

/**
 * Checks whether the error of @p code has been raised (after
 * begin_expect_raise()).
 */
#define test_expect_raise(code)                                          \
  (test_expect_raise_one(code) || test_expect_raise_exactly_one(code) || \
   test_expect_raise_code(code))

/**
 * Checks whether no errors are raised.
 */
#define test_expect_not_raised()                              \
  (test_compare_cf(test_expect_not_raised(),                  \
                   "Expected not to raise but %d has raised", \
                   control_expect_last_error()))

#endif
