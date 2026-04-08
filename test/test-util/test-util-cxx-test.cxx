#include "test-util.hpp"

#include <new>
#include <stdexcept>
#include <utility>
#include <vector>

using namespace JUPITER_Test;

class f
{
};

class g : public std::runtime_error
{
public:
  template <typename... Arg>
  g(Arg &&...args) : std::runtime_error(std::forward<Arg>(args)...)
  {
  }
};

class h : public g
{
public:
  template <typename... Arg>
  h(Arg &&...args) : g(std::forward<Arg>(args)...)
  {
  }
};

int main(int, char **)
{
  expect_throw(std::runtime_error) = [] { throw std::runtime_error("test"); };

  expect_throw(expect_throw_mismatch) = [] {
    expect_throw(std::runtime_error) = [] { throw f{}; };
  };

  expect_throw(expect_throw_mismatch) = [] {
    expect_throw(h) = [] { throw g(""); };
  };

  expect_throw(expect_throw_mismatch) = [] {
    expect_throw(std::runtime_error) = [] { /* nop */ };
  };

  try {
    expect_throw(std::runtime_error) = [] { throw std::bad_alloc(); };
    print_fail("Expected rethrow bad_alloc");
    return 1;
  } catch (std::bad_alloc &) {
    print_pass("expect rethrow bad_alloc");
  }

  test_expect(1).should(eq(1));
  test_expect(1).should(ne(0));
  test_expect(1).should(le(1));
  test_expect(1).should(le(2));
  test_expect(1).should(lt(2));
  test_expect(1).should(ge(1));
  test_expect(1).should(ge(0));
  test_expect(1).should(gt(0));

  test_expect(1).should_not(eq(0));
  test_expect(1).should_not(ne(1));
  test_expect(1).should_not(le(0));
  test_expect(1).should_not(lt(0));
  test_expect(1).should_not(lt(1));
  test_expect(1).should_not(ge(2));
  test_expect(1).should_not(gt(2));
  test_expect(1).should_not(gt(1));

  expect_throw(test_util_fail) = [] { test_expect(1).should(eq(2)); };
  expect_throw(test_util_fail) = [] { test_expect(1).should(ne(1)); };
  expect_throw(test_util_fail) = [] { test_expect(1).should(lt(1)); };
  expect_throw(test_util_fail) = [] { test_expect(1).should(le(0)); };
  expect_throw(test_util_fail) = [] { test_expect(1).should(gt(1)); };
  expect_throw(test_util_fail) = [] { test_expect(1).should(ge(2)); };

  expect_throw(test_util_fail) = [] { test_expect(1).should_not(eq(1)); };
  expect_throw(test_util_fail) = [] { test_expect(1).should_not(ne(0)); };
  expect_throw(test_util_fail) = [] { test_expect(1).should_not(lt(2)); };
  expect_throw(test_util_fail) = [] { test_expect(1).should_not(le(1)); };
  expect_throw(test_util_fail) = [] { test_expect(1).should_not(gt(0)); };
  expect_throw(test_util_fail) = [] { test_expect(1).should_not(ge(1)); };

  test_expect(std::vector<int>{0}).should(eq(std::vector<int>{0}));

  expect_throw(test_util_fail) = [] {
    test_expect(std::vector<int>{0}).should(eq(std::vector<int>{}));
  };

  expect_throw(test_util_fail) = [] {
    test_expect(std::vector<int>{0}).should(eq(std::vector<int>{2}));
  };

  return 0;
}
