#ifndef JUPITER_TEST_TEST_UTIL_HPP
#define JUPITER_TEST_TEST_UTIL_HPP

#include <array>
#include <exception>
#include <ios>
#include <iostream>
#include <list>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <typeinfo>
#include <vector>

#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#if __has_include(<source_location>)
#include <source_location>
#endif
#if __has_include(<cxxabi.h>)
#include <cxxabi.h>
#define have_cxxabi 1
#endif
#endif

/*
 * We use macro for improving usablity not to specify source location explicitly
 * (this was solved in C++20).
 *
 * The value of macro is from
 * https://en.cppreference.com/w/cpp/feature_test.html
 */
namespace JUPITER_Test
{
#if defined(__cpp_lib_source_location) && __cpp_lib_source_location >= 201907L
using source_location = std::source_location;
#define current_source_location() ::JUPITER_Test::source_location::current()
#else
struct source_location
{
  const char *const file;
  const long line;

  const char *file_name() const { return file; }
  static source_location current() { return source_location{"<unknown>", 0}; }
};
#define current_source_location() \
  ::JUPITER_Test::source_location { __FILE__, __LINE__ }
#endif
#define with_location(f, ...) f(__VA_ARGS__, current_source_location())

class get_class_name
{
  static std::string demangle(const char *cls_name)
  {
#ifdef have_cxxabi
    int status;
    char *name = abi::__cxa_demangle(cls_name, 0, 0, &status);
    std::string str_name = std::string(name);
    free(name);
    return str_name;
#else
    return std::string(cls_name);
#endif
  }

public:
  template <typename C> static std::string of(C &&value)
  {
    return demangle(typeid(value).name());
  }

  template <typename C> static std::string of()
  {
    return demangle(typeid(C).name());
  }
};

class test_util_fail : public std::runtime_error
{
public:
  const source_location location;

private:
  template <typename F> static std::string format(F &&func)
  {
    std::ostringstream ostr;
    func(ostr);
    return ostr.str();
  }

public:
  test_util_fail(const std::string &message, source_location loc)
    : std::runtime_error(message), location(loc)
  {
  }
};

class expect_throw_mismatch : public test_util_fail
{
public:
  expect_throw_mismatch(const std::string &message, const source_location loc)
    : test_util_fail(message, loc)
  {
  }

  template <typename Expect, typename Catch>
  static expect_throw_mismatch caught(Catch &&caught, const source_location loc)
  {
    std::ostringstream ostr;
    ostr << "Expected to throw " << get_class_name::of<Expect>()
         << ", but caught " << get_class_name::of(caught) << ".";
    return expect_throw_mismatch(ostr.str(), loc);
  }

  template <typename Catch>
  static expect_throw_mismatch caught(const source_location loc)
  {
    std::ostringstream ostr;
    ostr << "It threw an exception, but it could not be caught with "
         << get_class_name::of<Catch>() << ".";
    return expect_throw_mismatch(ostr.str(), loc);
  }

  template <typename Expect>
  static expect_throw_mismatch not_threw(const source_location loc)
  {
    std::ostringstream ostr;
    ostr << "Expected to throw " << get_class_name::of<Expect>()
         << ", but no exception has been thrown.";
    return expect_throw_mismatch(ostr.str(), loc);
  }
};

template <typename Expect>
class expect_throw_mismatch_caught : public expect_throw_mismatch
{
  template <typename C> std::string format(C &&caught) const
  {
    return test_util_fail::format([&](std::ostream &os) {
      os << "Expected to throw " << get_class_name::of<Expect>()
         << ", but caught " << get_class_name::of(caught);
    });
  }

  std::string format() const
  {
    return test_util_fail::format([&](std::ostream &os) {
      os << "Expected to throw " << get_class_name::of<Expect>()
         << ", but unknown type object caught";
    });
  }

  template <typename C>
  expect_throw_mismatch_caught(C &&caught, const source_location loc)
    : test_util_fail(format(caught), loc)
  {
  }

  expect_throw_mismatch_caught(const source_location loc)
    : test_util_fail(format(), loc)
  {
  }

public:
  template <typename C>
  static expect_throw_mismatch_caught<Expect> caught(C &&caught,
                                                     const source_location loc)
  {
    expect_throw_mismatch_caught<Expect> c(caught, loc);
    return c;
  }

  static expect_throw_mismatch_caught<Expect> caught(const source_location loc)
  {
    expect_throw_mismatch_caught<Expect> c(loc);
    return c;
  }
};

inline void print_log(bool is_pass, const std::string &str)
{
  bool first = true;
  std::string::size_type pos = 0, np;
  do {
    np = str.find('\n', pos);
    if (first) {
      if (is_pass) {
        std::cerr << "[PASS]: ";
      } else {
        std::cerr << "[FAIL]: ";
      }
    } else {
      std::cerr << "      | ";
    }
    std::string sstr =
      str.substr(pos, (np == std::string::npos ? np : (np - pos)));
    std::cerr << sstr << "\n";
    first = false;
    pos = np + 1;
  } while (np != std::string::npos);
}

inline void print_pass(const std::string &str) { print_log(true, str); }
inline void print_fail(const std::string &str) { print_log(false, str); }
inline void print_fail(const test_util_fail &e)
{
  std::ostringstream ostr;
  ostr << e.what() << "\n\n"
       << "(defined at " << e.location.file_name() << ", line "
       << e.location.line << ")";
  print_fail(ostr.str());
}

template <typename Expect> class expect_throw_catch
{
public:
  using type = typename std::conditional<
    std::is_base_of<std::runtime_error, Expect>::value, std::runtime_error,
    Expect>::type;
};

template <typename Expect,
          typename Catch = typename expect_throw_catch<Expect>::type>
class expect_throw
{
  template <typename F> void exec(F &&func) const
  {
    try {
      func();
    } catch (const Catch &e) {
      try {
        const Expect &ex = dynamic_cast<const Expect &>(e);
      } catch (const std::bad_cast &) {
        throw expect_throw_mismatch::caught<Expect>(e, location);
      }
      return;
    } catch (const std::exception &e) {
      throw;
    } catch (...) {
      throw expect_throw_mismatch::caught<Catch>(location);
    }
    throw expect_throw_mismatch::not_threw<Expect>(location);
  }

public:
  const source_location location;

  expect_throw(const source_location loc = source_location::current())
    : location(loc)
  {
  }

  template <typename F> void operator=(F &&func) const
  {
    try {
      exec(func);
      print_pass("expect_throw()...");
    } catch (const test_util_fail &e) {
      print_fail(e);
      throw;
    }
  }
};

#define expect_throw_f(e, ...) ::JUPITER_Test::expect_throw<e>(__VA_ARGS__)
#define expect_throw(e) with_location(expect_throw_f, e)

template <typename T> class value_printer
{
public:
  static std::ostream &print(std::ostream &os, const T &value)
  {
    return os << value;
  }
};

class iterable_value_printer
{
public:
  template <typename InIt>
  static std::ostream &print(std::ostream &os, const InIt &beg, const InIt &end)
  {
    bool first = true;
    os << "{";
    for (InIt iter = beg; iter != end; first = false, ++iter) {
      if (!first)
        os << ", ";
      value_printer<decltype(*iter)>::print(os, *iter);
    }
    return os << "}";
  }

  template <typename T>
  static std::ostream &print(std::ostream &os, const T &value)
  {
    return print(os, value.cbegin(), value.cend());
  }
};

template <typename T, size_t N> class value_printer<std::array<T, N>>
  : public iterable_value_printer
{
};

template <typename T, typename A>
class value_printer<std::vector<T, A>> : public iterable_value_printer
{
};

template <typename T, typename A>
class value_printer<std::list<T, A>> : public iterable_value_printer
{
};

class expect_verb;

template <typename L> class expect_data
{
  const source_location location_data;
  const char *value_str;
  const L &value_data;

public:
  expect_data(const L &value, const char *value_str,
              const source_location location)
    : value_data(value), value_str(value_str), location_data(location)
  {
  }

  template <typename verb>
  typename std::enable_if<std::is_base_of<expect_verb, verb>::value>::type
  should(const verb &v) const
  {
    try {
      v.comp(*this);
    } catch (test_util_fail &e) {
      print_fail(e);
      throw;
    }
  }

  template <typename verb>
  typename std::enable_if<std::is_base_of<expect_verb, verb>::value>::type
  should_not(const verb &v) const
  {
    try {
      v.comp_not(*this);
    } catch (test_util_fail &e) {
      print_fail(e);
      throw;
    }
  }

  const L &value() const { return value_data; }
  const char *name() const { return value_str; }
  const source_location &location() const { return location_data; }
};

class expect_verb
{
};

template <typename R, template <typename LL, typename RR> class compfunc>
class expect_verb_binary : public expect_verb
{
  const char *value_str;
  const R &value_data;

public:
  expect_verb_binary(const R &value, const char *value_str)
    : value_data(value), value_str(value_str)
  {
  }

  const R &value() const { return value_data; }

  template <typename L>
  [[noreturn]]
  void raise(const char *type, const L &lval, const char *lname, const R &rval,
             const char *rname, const char *desc, const char *op,
             const source_location location) const
  {
    std::ostringstream ostr;
    ostr << lname << " should" << type << " " << desc << " " << rname << "\n\n";
    ostr << "  Expect: ";
    value_printer<R>::print(ostr, rval);
    ostr << "\n";
    ostr << "     Got: ";
    value_printer<L>::print(ostr, lval);
    throw test_util_fail(ostr.str(), location);
  }

  template <typename L> void comp(const expect_data<L> &data) const
  {
    using compar = compfunc<L, R>;
    if (compar{}(data.value(), value_data)) {
      print_pass(std::string(data.name()) + " " + compar::op + " " + value_str);
      return;
    }

    raise("", data.value(), data.name(), value_data, value_str, compar::name,
          compar::op, data.location());
  }

  template <typename L> void comp_not(const expect_data<L> &data) const
  {
    using compar = compfunc<L, R>;
    if (!compar{}(data.value(), value_data)) {
      print_pass(std::string("!(") + data.name() + " " + compar::op + " " +
                 value_str + ")");
      return;
    }

    raise(" not", data.value(), data.name(), value_data, value_str,
          compar::name, compar::op, data.location());
  }
};

template <template <typename RR> class klass, typename R>
inline klass<R> expect_be(const R &value, const char *value_str)
{
  return klass<R>(value, value_str);
}

template <typename L, typename R> struct expect_comp_eq
{
  bool operator()(const L &l, const R &r) { return l == r; }
  static constexpr auto &name = "be equal to";
  static constexpr auto &op = "==";
};

template <typename L, typename R> struct expect_comp_ne
{
  bool operator()(const L &l, const R &r) { return l != r; }
  static constexpr auto &name = "be unequal to";
  static constexpr auto &op = "!=";
};

template <typename L, typename R> struct expect_comp_lt
{
  bool operator()(const L &l, const R &r) { return l < r; }
  static constexpr auto &name = "be less than";
  static constexpr auto &op = "<";
};

template <typename L, typename R> struct expect_comp_le
{
  bool operator()(const L &l, const R &r) { return l <= r; }
  static constexpr auto &name = "be less than or equal to";
  static constexpr auto &op = "<=";
};

template <typename L, typename R> struct expect_comp_gt
{
  bool operator()(const L &l, const R &r) { return l > r; }
  static constexpr auto &name = "be greater than";
  static constexpr auto &op = ">";
};

template <typename L, typename R> struct expect_comp_ge
{
  bool operator()(const L &l, const R &r) { return l >= r; }
  static constexpr auto &name = "be greater than or equal to";
  static constexpr auto &op = ">=";
};

template <typename R>
using expect_verb_eq = expect_verb_binary<R, expect_comp_eq>;
#define eq(value) \
  ::JUPITER_Test::expect_be<::JUPITER_Test::expect_verb_eq>(value, #value)

template <typename R>
using expect_verb_ne = expect_verb_binary<R, expect_comp_ne>;
#define ne(value) \
  ::JUPITER_Test::expect_be<::JUPITER_Test::expect_verb_ne>(value, #value)

template <typename R>
using expect_verb_lt = expect_verb_binary<R, expect_comp_lt>;
#define lt(value) \
  ::JUPITER_Test::expect_be<::JUPITER_Test::expect_verb_lt>(value, #value)

template <typename R>
using expect_verb_le = expect_verb_binary<R, expect_comp_le>;
#define le(value) \
  ::JUPITER_Test::expect_be<::JUPITER_Test::expect_verb_le>(value, #value)

template <typename R>
using expect_verb_gt = expect_verb_binary<R, expect_comp_gt>;
#define gt(value) \
  ::JUPITER_Test::expect_be<::JUPITER_Test::expect_verb_gt>(value, #value)

template <typename R>
using expect_verb_ge = expect_verb_binary<R, expect_comp_ge>;
#define ge(value) \
  ::JUPITER_Test::expect_be<::JUPITER_Test::expect_verb_ge>(value, #value)

template <typename L>
expect_data<L>
test_expect(const L &value, const char *value_str,
            source_location location = source_location::current())
{
  return expect_data<L>(value, value_str, location);
}

#define test_expect(value) \
  ::JUPITER_Test::test_expect(value, #value, current_source_location())

} // namespace JUPITER_Test

#endif
