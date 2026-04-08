#include "jupiter/cxxoptparse/parsers.h"
#include "jupiter/cxxoptparse/value_printer.h"
#include <jupiter/cxxoptparse/common.h>
#include <jupiter/cxxoptparse/enum.h>
#include <jupiter/cxxoptparse/exceptions.h>
#include <jupiter/cxxoptparse/help.h>
#include <jupiter/cxxoptparse/match.h>
#include <jupiter/cxxoptparse/optparse.h>
#include <jupiter/cxxoptparse/sequence.h>
#include <jupiter/cxxoptparse/stop.h>
#include <jupiter/cxxoptparse/traits.h>
#include <jupiter/cxxoptparse/types.h>

#include <test-util.hpp>

#include <array>
#include <iostream>
#include <ostream>
#include <tuple>
#include <type_traits>
#include <utility>

using namespace JUPITER;

class hv : public option_parser::help_value
{
public:
  std::ostream &print_help(std::ostream &os) const override { return os; }
};

class h : public option_parser::help
{
public:
  using value_type = hv;
  static value_type default_value() { return hv(); }
};

class s : public option_parser::stop
{
};

class m : public option_parser::bool_option
{
public:
  static constexpr auto &long_option_name = "te";
  static constexpr bool has_no_long_option = true;
  static bool default_value() { return false; }
  static std::ostream &description(std::ostream &os) { return os << "test"; }
};

class v : public option_parser::bool_option
{
public:
  static constexpr auto &long_option_name = "verbose";
  static constexpr auto &no_long_option_name = "quiet";
  static constexpr char short_option_name = 'v';
  static constexpr char no_short_option_name = 'q';

  static bool default_value() { return false; }
  static std::ostream &description(std::ostream &os) { return os << "v"; }
};

class vv : public option_parser::bool_option
{
public:
  static constexpr char short_option_name = 'a';
  static constexpr char no_short_option_name = 'b';

  static bool default_value() { return false; }
  static std::ostream &description(std::ostream &os) { return os << "v"; }
};

class l : public option_parser::bool_option
{
public:
  static constexpr char short_option_name = 'e';
  static constexpr auto &long_option_name = "le";
  static constexpr bool has_no_long_option = true;
  static bool default_value() { return false; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "test yes";
  }

  static std::ostream &no_description(std::ostream &os)
  {
    return os << "test no";
  }
};

class ll : public option_parser::bool_option
{
public:
  static constexpr auto &long_option_name = "ll";
  static constexpr bool has_no_long_option = true;
  static bool default_value() { return false; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "test yes";
  }

  static std::ostream &no_description(std::ostream &os)
  {
    return os << "test no";
  }
};

class l2 : public option_parser::bool_option
{
public:
  static constexpr auto &long_option_name = "l2";
  static constexpr auto &no_long_option_name = "l2n";
  static bool default_value() { return false; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "test yes";
  }

  static std::ostream &no_description(std::ostream &os)
  {
    return os << "test no";
  }
};

class u : public option_parser::bool_option
{
public:
  static constexpr auto &long_option_name = "uu";
  static constexpr bool has_no_long_option = true;
  static constexpr bool no_option_accept_value = true;
  static bool default_value() { return false; }
  static std::string value_name() { return "VALUE"; }

  static std::ostream &description(std::ostream &os) { return os << "test u"; }

  static void parse_value(value_type &value, bool p)
  {
    option_parser::bool_option::parse_value(value, p);
  }

  template <typename InIt>
  static void parse_value(value_type &value, bool p, InIt beg, InIt end)
  {
    option_parser::bool_option::parse_value(value, p);
  }
};

class iv : public option_parser::int_option<int>
{
public:
  static constexpr auto &long_option_name = "iv";
  static int default_value() { return 10; }
  static std::string value_name() { return "VALUE"; }
  static std::ostream &description(std::ostream &os) { return os << "test iv"; }
};

class uv : public option_parser::int_option<unsigned int>
{
public:
  static constexpr auto &long_option_name = "uv";
  static unsigned int default_value() { return 100; }
  static std::string value_name() { return "VALUE"; }
  static std::ostream &description(std::ostream &os) { return os << "test uv"; }
};

class iz : public option_parser::no_option<option_parser::int_option<>>
{
public:
  static constexpr auto &long_option_name = "iz";
  static value_type default_value() { return 10; }
  static std::string value_name() { return "VALUE"; }
  static std::ostream &description(std::ostream &os) { return os << "test iz"; }
};

class im : public option_parser::no_option<option_parser::int_option<>>
{
public:
  static constexpr auto &long_option_name = "im";
  static value_type default_value() { return value_type(true, 10); }
  static std::string value_name() { return "VALUE"; }
  static std::ostream &description(std::ostream &os) { return os << "test iz"; }
};

class in : public option_parser::no_option<option_parser::int_option<>,
                                           option_parser::int_option<>>
{
public:
  static constexpr auto &long_option_name = "in";
  static value_type default_value() { return value_type(true, 10, 20); }
  static std::string value_name() { return "VALUE"; }
  static std::ostream &description(std::ostream &os) { return os << "test iz"; }
};

class arr
  : public option_parser::array_option<option_parser::double_option<double>, 3>
{
public:
  static constexpr auto &long_option_name = "arr";
  static std::string value_name() { return "A,B,C"; }
  static std::ostream &description(std::ostream &os)
  {
    return os << "test arr";
  }
};

enum x
{
  one,
  two,
  three,
  four,
};

template <x> class enum_tr;

JUPITER_CXXOPTPARSE_ENUM2STR(enum_tr, x, one, "one");
JUPITER_CXXOPTPARSE_ENUM2STR(enum_tr, x, two, "two", "description 2");
JUPITER_CXXOPTPARSE_ENUM2STR(enum_tr, x, three, "three",
                             "description 3\nwith nl");
JUPITER_CXXOPTPARSE_ENUM2STR(enum_tr, x, four, "four", "d4");

using enum_tr_tuple =
  option_parser::enum_trait_tuple_type<x, enum_tr, one, two, three, four>;

template <>
class option_parser::value_print<x>
{
public:
  static std::ostream &print(std::ostream &os, const x &value)
  {
    return os << enum2str<enum_tr_tuple>{}(value);
  }
};

class ex : public option_parser::enum_option<enum_tr_tuple>
{
public:
  static constexpr auto &long_option_name = "ex";
  static std::string value_name() { return "VALUE"; }
  static x default_value() { return one; }
  static std::ostream &description(std::ostream &os)
  {
    return os << "test ex";
  }
};

struct t
{
  int a;
  double b;
  std::string c;
};

class t_a_parser
{
  using option = option_parser::int_option<int>;

public:
  template <typename InIt>
  void operator()(t &value, bool p, InIt beg, InIt end) const
  {
    option::parse_value(value.a, p, beg, end);
  }

  static std::ostream &option_value_help(std::ostream &os)
  {
    return option::option_value_help(os);
  }
};

class t_b_parser
{
  using option = option_parser::double_option<double>;

public:
  template <typename InIt>
  void operator()(t &value, bool p, InIt beg, InIt end) const
  {
    option::parse_value(value.b, p, beg, end);
  }

  static std::ostream &option_value_help(std::ostream &os)
  {
    return option::option_value_help(os);
  }
};

class t_c_parser
{
  using option = option_parser::string_option<std::string>;

public:
  template <typename InIt>
  void operator()(t &value, bool p, InIt beg, InIt end) const
  {
    option::parse_value(value.c, p, beg, end);
  }
};

class t_option : public option_parser::composite_option<t, ',', t_a_parser,
                                                        t_b_parser, t_c_parser>
{
public:
  static constexpr auto &long_option_name = "tac";
  static t default_value() { return t{0, 0.0, ""}; }
  static std::string value_name() { return "A,B,C"; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "tac";
  }
};

template <>
class option_parser::value_print<t>
{
public:
  static std::ostream &print(std::ostream &os, const t &value)
  {
    return os << value_printer(value.a) << "," << value_printer(value.b) << ","
              << value_printer(value.c);
  }
};

class z
{
};

class c
{
public:
  static constexpr auto &u = "abc";
  static_assert(u[2] == 'c', "");
};

template <typename parser, typename... Args> void parse(parser &p, Args... args)
{
  option_parser::core::argv_type l = {args...};
  p.parse(l.cbegin(), l.cend());
};

int main(int argc, char **argv)
{
  {
    option_parser::parser_base<h> parser;
    parse(parser, "--help");
    test_expect(parser.get<h>().want_help).should(eq(true));

    parser.reset();
    parse(parser, "a", "b", "c");
    test_expect(parser.argv[0]).should(eq("a"));

    expect_throw(option_parser::invalid_short_option) = [&]() {
      parse(parser, "--");
    };
  }

  {
    option_parser::parser<m, l, ll, l2, u, iv, uv, iz, im, in, ex, arr,
                          t_option, v, vv>
      parser;
    parse(parser, "abc", "--help");

    parser.reset();

    test_expect(parser.get<m>()).should(eq(false));
    test_expect(parser.get<l>()).should(eq(false));
    test_expect(parser.get<u>()).should(eq(false));
    test_expect(parser.get<v>()).should(eq(false));

    parse(parser, "abc", "--te");

    test_expect(parser.get<m>()).should(eq(true));
    test_expect(parser.get<l>()).should(eq(false));
    test_expect(parser.get<u>()).should(eq(false));
    test_expect(parser.get<v>()).should(eq(false));

    parse(parser, "--no-te");

    test_expect(parser.get<m>()).should(eq(false));
    test_expect(parser.get<l>()).should(eq(false));
    test_expect(parser.get<u>()).should(eq(false));
    test_expect(parser.get<v>()).should(eq(false));

    parse(parser, "--verbose");

    test_expect(parser.get<m>()).should(eq(false));
    test_expect(parser.get<l>()).should(eq(false));
    test_expect(parser.get<u>()).should(eq(false));
    test_expect(parser.get<v>()).should(eq(true));

    parse(parser, "--quiet");

    test_expect(parser.get<m>()).should(eq(false));
    test_expect(parser.get<l>()).should(eq(false));
    test_expect(parser.get<u>()).should(eq(false));
    test_expect(parser.get<v>()).should(eq(false));

    parse(parser, "--verbose", "--quiet");

    test_expect(parser.get<m>()).should(eq(false));
    test_expect(parser.get<l>()).should(eq(false));
    test_expect(parser.get<u>()).should(eq(false));
    test_expect(parser.get<v>()).should(eq(false));

    parse(parser, "-vq");

    test_expect(parser.get<v>()).should(eq(false));

    parse(parser, "-qv");

    test_expect(parser.get<v>()).should(eq(true));

    parser.get<arr>()[0] = 1.0;

    test_expect(parser.get<arr>()).should(eq((std::array<double, 3>{1, 0, 0})));

    parse(parser, "--arr=0,0,0");

    test_expect(parser.get<arr>()).should(eq((std::array<double, 3>{0, 0, 0})));

    parse(parser, "--arr=1,2,0");

    test_expect(parser.get<arr>()).should(eq((std::array<double, 3>{1, 2, 0})));

    parse(parser, "--ex", "three");

    test_expect(parser.get<ex>()).should(eq(three));

    parse(parser, "--ex=two");

    test_expect(parser.get<ex>()).should(eq(two));

    parse(parser, "--tac=4,1,abc");

    test_expect(parser.get<t_option>().a).should(eq(4));
    test_expect(parser.get<t_option>().b).should(eq(1.0));
    test_expect(parser.get<t_option>().c).should(eq("abc"));

    parse(parser, "--tac=2,1.5,efg");

    test_expect(parser.get<t_option>().a).should(eq(2));
    test_expect(parser.get<t_option>().b).should(eq(1.5));
    test_expect(parser.get<t_option>().c).should(eq("efg"));

    expect_throw(option_parser::invalid_long_option) = [&]() {
      parse(parser, "--tet");
    };

    expect_throw(option_parser::invalid_short_option) = [&]() {
      parse(parser, "-p");
    };

    expect_throw(option_parser::invalid_short_option) = [&]() {
      parse(parser, "-ep");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--iv=-999999999999999999999999999999999999999999");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--iv=a");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--iv=0a");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--uv=-1");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--uv=a");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--uv=0a");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--arr=0,1,2,3");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--arr=0,1");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--arr=0,1,");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--arr=0,1,2,");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--arr=a,0,0");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--arr=1e+9999,0,0");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--arr=1e-9999,0,0");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--tac=");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--tac=0,aa,1");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--tac=0,1,aaa,");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--tac=0,1,aaa,t");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--ex=ona");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--ex=thr");
    };

    expect_throw(option_parser::invalid_value_error) = [&]() {
      parse(parser, "--ex=");
    };

    expect_throw(option_parser::missing_value_error) = [&]() {
      parse(parser, "--ex");
    };

    expect_throw(option_parser::excess_value_error) = [&]() {
      parse(parser, "--help=value");
    };

    parse(parser, "--", "--te");
    test_expect(*--parser.argv.cend()).should(eq(std::string("--te")));
  }

  static_assert(
    option_parser::traits::common::is_string_or_char<decltype(c::u)>::value,
    "");

  static_assert(
    option_parser::traits::data_index<h, std::tuple<h, s, z>>::value == 0, "");
  static_assert(
    option_parser::traits::data_index<s, std::tuple<h, s, z>>::value == 1, "");
  static_assert(
    option_parser::traits::data_index<z, std::tuple<h, s, z>>::value == 2, "");
  static_assert(option_parser::traits::data_index<h, std::tuple<h>>::value == 0,
                "");

  static_assert(option_parser::traits::has_long_option<h>::value, "");
  static_assert(!option_parser::traits::has_long_option<s>::value, "");
  static_assert(!option_parser::traits::has_long_option<z>::value, "");

  static_assert(option_parser::traits::has_no_long_option<m>::value, "");
  static_assert(!option_parser::traits::has_no_long_option<h>::value, "");
  static_assert(!option_parser::traits::has_no_long_option<z>::value, "");

  static_assert(
    std::is_same<option_parser::core::make_sequence<int, 0, 5>,
                 option_parser::core::sequence<int, 0, 1, 2, 3, 4, 5>>::value,
    "");

  static_assert(std::is_same<option_parser::core::make_sequence<int, 0, 0>,
                             option_parser::core::sequence<int, 0>>::value,
                "");

  static_assert(std::is_same<option_parser::core::make_sequence<int, 0, -1>,
                             option_parser::core::sequence<int>>::value,
                "");

  static_assert(
    std::is_same<option_parser::core::make_sequence<size_t, 0, 1>,
                 option_parser::core::sequence<size_t, 0, 1>>::value,
    "");

  static_assert(std::is_same<option_parser::core::make_sequence<size_t, 0, 0>,
                             option_parser::core::sequence<size_t, 0>>::value,
                "");

  static_assert(std::is_same<option_parser::core::make_sequence<size_t, 1, 0>,
                             option_parser::core::sequence<size_t>>::value,
                "");

  return 0;
}

namespace
{
using namespace JUPITER::option_parser::core;

static constexpr auto &a = "abc";
static constexpr auto &b = "";

template <int I> struct g
{
  static constexpr auto value = a[I];
};

template <int I> struct f
{
  static constexpr auto value = b[I];
};

static_assert(std::is_same<make_sequence_from_str<decltype(a), g>::type,
                           sequence<char, 'a', 'b', 'c'>>::value,
              "");

static_assert(std::is_same<make_sequence_from_str<decltype(b), f>::type,
                           sequence<char>>::value,
              "");

static_assert(sequence_any(sequence<bool, true>()), "");
static_assert(!sequence_any(sequence<bool, false>()), "");
static_assert(sequence_any(sequence<bool, true, true>()), "");
static_assert(sequence_any(sequence<bool, true, false>()), "");
static_assert(sequence_any(sequence<bool, false, true>()), "");
static_assert(!sequence_any(sequence<bool, false, false>()), "");
static_assert(!sequence_any(sequence<bool>()), "");

static_assert(sequence_all(sequence<bool, true>()), "");
static_assert(!sequence_all(sequence<bool, false>()), "");
static_assert(sequence_all(sequence<bool, true, true>()), "");
static_assert(!sequence_all(sequence<bool, true, false>()), "");
static_assert(!sequence_all(sequence<bool, false, true>()), "");
static_assert(!sequence_all(sequence<bool, false, false>()), "");
static_assert(!sequence_all(sequence<bool>()), "");

static_assert(
  std::is_same<
    generate_match_tags<std::tuple<h>>::tags,
    std::tuple<match_tag_name<match_tag<option_match_tag<h, true>>,
                              sequence<char, 'h', 'e', 'l', 'p'>>>>::value,
  "");

static_assert(
  std::is_same<
    generate_match_tags<std::tuple<h, m>>::tags,
    std::tuple<match_tag_name<match_tag<option_match_tag<h, true>>,
                              sequence<char, 'h', 'e', 'l', 'p'>>,
               match_tag_name<match_tag<option_match_tag<m, true>>,
                              sequence<char, 't', 'e'>>,
               match_tag_name<match_tag<option_match_tag<m, false>>,
                              sequence<char, 'n', 'o', '-', 't', 'e'>>>>::value,
  "");

namespace t1
{
using t1 = match_tags_by_ch<'c', match_tag<int>>;
using t2 = match_tags_by_ch<'b', t1>;
using t3 = match_tags_by_ch<'a', t2>;

static_assert(
  std::is_same<generate_match_tree<std::tuple<match_tag_name<
                 match_tag<int>, sequence<char, 'a', 'b', 'c'>>>>::type,
               std::tuple<t3>>::value,
  "");
} // namespace t1

namespace t2
{
using t1c = match_tags_by_ch<'c', match_tag<int>>;
using t1d = match_tags_by_ch<'d', match_tag<long>>;
using t2 = match_tags_by_ch<'b', t1c, t1d>;
using t3 = match_tags_by_ch<'a', t2>;

static_assert(
  std::is_same<
    generate_match_tree<std::tuple<
      match_tag_name<match_tag<int>, sequence<char, 'a', 'b', 'c'>>,
      match_tag_name<match_tag<long>, sequence<char, 'a', 'b', 'd'>>>>::type,
    std::tuple<t3>>::value,
  "");
} // namespace t2

namespace t3
{
using t1 = match_tags_by_ch<'c', match_tag<int>>;
using t2 = match_tags_by_ch<'b', t1, match_tag<long>>;
using t3 = match_tags_by_ch<'a', t2>;

static_assert(
  std::is_same<
    generate_match_tree<std::tuple<
      match_tag_name<match_tag<int>, sequence<char, 'a', 'b', 'c'>>,
      match_tag_name<match_tag<long>, sequence<char, 'a', 'b'>>>>::type,
    std::tuple<t3>>::value,
  "");
} // namespace t3

namespace t4
{
using t1 = match_tags_by_ch<'c', match_tag<long>>;
using t2 = match_tags_by_ch<'b', match_tag<int>, t1>;
using t3 = match_tags_by_ch<'a', t2>;

static_assert(
  std::is_same<
    generate_match_tree<std::tuple<
      match_tag_name<match_tag<int>, sequence<char, 'a', 'b'>>,
      match_tag_name<match_tag<long>, sequence<char, 'a', 'b', 'c'>>>>::type,
    std::tuple<t3>>::value,
  "");
} // namespace t4

namespace t5
{
using t1 = match_tags_by_ch<'a', match_tag<int>>;
using t2 = match_tags_by_ch<'b', match_tag<long>>;
using t3 = match_tags_by_ch<'z', match_tag<char>>;

static_assert(
  std::is_same<generate_match_tree<std::tuple<
                 match_tag_name<match_tag<int>, sequence<char, 'a'>>,
                 match_tag_name<match_tag<long>, sequence<char, 'b'>>,
                 match_tag_name<match_tag<char>, sequence<char, 'z'>>>>::type,
               std::tuple<t1, t2, t3>>::value,
  "");
} // namespace t5

} // namespace
