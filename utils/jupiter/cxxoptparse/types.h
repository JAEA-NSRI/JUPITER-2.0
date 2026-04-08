/* This is -*- c++ -*- source */
/* This is vim: set ft=cpp: source */

#ifndef JUPITER_CXXOPTPARSE_TYPES_H
#define JUPITER_CXXOPTPARSE_TYPES_H

#include "jupiter/cxxoptparse/value_printer.h"
#include <jupiter/cxxoptparse/enum.h>
#include <jupiter/cxxoptparse/exceptions.h>
#include <jupiter/cxxoptparse/help_printer.h>
#include <jupiter/cxxoptparse/parsers.h>
#include <jupiter/cxxoptparse/sequence.h>
#include <jupiter/cxxoptparse/traits.h>

#include <algorithm>
#include <array>
#include <cstddef>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>
#include <utility>
#include <vector>

namespace JUPITER
{
namespace option_parser
{
class bool_option
{
public:
  using value_type = bool;
  static constexpr bool has_no_long_option = true;
  static constexpr bool require_value = false;

  static void parse_value(bool &value, bool p) { value = p; }
};

template <typename string_type = std::string> class string_option
{
public:
  using value_type = string_type;
  static constexpr bool has_no_long_option = false;
  static constexpr bool require_value = true;

  template <typename InIt>
  static void parse_value(string_type &value, bool p, InIt beg, InIt end)
  {
    string_parser{}(value, p, beg, end);
  }
};

template <typename itype> class default_int_option_property
{
public:
  static constexpr itype min = std::numeric_limits<itype>::min();
  static constexpr itype max = std::numeric_limits<itype>::max();
  static constexpr int base = 10;
};

template <typename itype = int,
          typename props = default_int_option_property<itype>>
class int_option
{
public:
  using value_type = itype;
  static constexpr bool has_no_long_option = false;
  static constexpr bool require_value = true;
  static constexpr int base = props::base;
  static constexpr itype min_value = props::min;
  static constexpr itype max_value = props::max;

  template <typename InIt>
  static void parse_value(itype &value, bool p, InIt beg, InIt end)
  {
    int_parser{}(value, p, beg, end, base, min_value, max_value);
  }

  static std::ostream &option_value_help(std::ostream &os)
  {
    return os << " * min (inclusive): " << min_value
              << "\n * max (inclusive): " << max_value << "\n";
  }
};

template <typename real_type> class default_double_option_property
{
public:
  static constexpr real_type min = std::numeric_limits<real_type>::lowest();
  static constexpr real_type max = std::numeric_limits<real_type>::max();
  static constexpr bool include_min = true;
  static constexpr bool include_max = true;
  static constexpr bool allow_positive_inf = true;
  static constexpr bool allow_negative_inf = true;
  static constexpr bool allow_nan = true;
};

template <typename real_type>
class finite_double_option_property
  : public default_double_option_property<real_type>
{
public:
  static constexpr bool allow_positive_inf = false;
  static constexpr bool allow_negative_inf = false;
  static constexpr bool allow_nan = false;
};

template <typename real_type = double,
          typename props = default_double_option_property<real_type>>
class double_option
{
public:
  using value_type = real_type;
  const static bool has_no_long_option = false;
  const static bool require_value = true;
  static constexpr real_type min_value = props::min;
  static constexpr real_type max_value = props::max;
  static constexpr bool include_min = props::include_min;
  static constexpr bool include_max = props::include_max;
  static constexpr bool allow_positive_inf = props::allow_positive_inf;
  static constexpr bool allow_negative_inf = props::allow_negative_inf;
  static constexpr bool allow_nan = props::allow_nan;

  template <typename InIt>
  static void parse_value(real_type &value, bool p, InIt beg, InIt end)
  {
    real_parser{}(value, p, beg, end, min_value, max_value, include_min,
                  include_max, allow_positive_inf, allow_negative_inf,
                  allow_nan);
  }

  static std::ostream &option_value_help(std::ostream &os)
  {
    const real_type zero = static_cast<real_type>(0.0);
    bool f = true;
    os << " * min " << (include_min ? "(inclusive)" : "(exclusive)") << ": "
       << min_value;
    os << "\n * max " << (include_max ? "(inclusive)" : "(exclusive)") << ": "
       << max_value;
    if (min_value <= zero && max_value >= zero && min_value != max_value) {
      os << "\n * absolute no-zero min: ";
      if (min_value < zero && zero < max_value) {
        os << "+/- ";
      } else if (max_value == zero) {
        os << "-";
      }
      os << std::numeric_limits<real_type>::min();
    }
    if ((min_value < zero || (include_min && min_value == zero)) &&
        (max_value > zero || (include_max && max_value == zero))) {
      os << (f ? "\n * " : ", ") << zero;
      f = false;
    }
    if (std::numeric_limits<real_type>::has_infinity) {
      if (allow_positive_inf) {
        os << (f ? "\n * " : ", ") << "+inf";
        f = false;
      }
      if (allow_negative_inf) {
        os << (f ? "\n * " : ", ") << "-inf";
        f = false;
      }
    }
    if ((std::numeric_limits<real_type>::has_quiet_NaN ||
         std::numeric_limits<real_type>::has_signaling_NaN) &&
        allow_nan) {
      os << (f ? "\n * " : ", ") << "nan";
    }
    os << "\n";
    return os;
  }
};

template <typename enum_metadata_tuple> class enum_option;

/*
 * First parameter is used for getting enum info
 */
template <typename T, typename... Ts> class enum_option<std::tuple<T, Ts...>>
{
public:
  using tuple = std::tuple<T, Ts...>;
  using str2enum = core::str2enum<tuple>;

  using value_type = typename T::enum_type;
  const static bool has_no_long_option = false;
  const static bool require_value = true;

  static_assert(
    sizeof...(Ts) == 0 ||
      core::sequence_all(
        core::sequence<
          bool, std::is_same<value_type, typename Ts::enum_type>::value...>{}),
    "enum value type is not consistent");

  template <typename InIt>
  static void parse_value(value_type &value, bool p, InIt beg, InIt end)
  {
    value = str2enum{}(beg, end);
  }

  static std::ostream &option_value_help(std::ostream &os)
  {
    return os << help_printer::enum_help_printer<tuple>();
  }
};

template <typename P, typename N> class no_option_value
{
public:
  bool flag;
  P positive;
  N negative;

  no_option_value() = default;
  no_option_value(no_option_value<P, N> &) = default;
  no_option_value(const no_option_value<P, N> &) = default;

  no_option_value(const bool &f, const P &p, const N &n)
    : flag(f), positive(p), negative(n)
  {
  }
  no_option_value(bool &&f, P &&p, N &&n) : flag(f), positive(p), negative(n) {}
};

template <typename P> class no_option_value<P, void>
{
public:
  bool flag;
  P positive;

  no_option_value() = default;
  no_option_value(no_option_value<P, void> &) = default;
  no_option_value(const no_option_value<P, void> &) = default;

  no_option_value(const bool &f, const P &p) : flag(f), positive(p) {}
  no_option_value(bool &&f, P &&p) : flag(f), positive(p) {}
  no_option_value(const P &p) : flag(false), positive(p) {}
  no_option_value(P &&p) : flag(false), positive(p) {}
};

template <typename T, typename U> class value_print<no_option_value<T, U>>
{
public:
  template <typename X>
  static
    typename std::enable_if<!std::is_same<X, void>::value, std::ostream &>::type
    print(std::ostream &os, const no_option_value<T, X> &value)
  {
    value_print<bool>::print(os, value.flag) << ", ";
    if (value.flag) {
      return value_print<T>::print(os, value.positive);
    } else {
      return value_print<X>::print(os, value.negative);
    }
  }

  template <typename X>
  static
    typename std::enable_if<std::is_same<X, void>::value, std::ostream &>::type
    print(std::ostream &os, const no_option_value<T, X> &value)
  {
    if (value.flag) {
      return value_print<T>::print(os, value.positive);
    } else {
      return value_print<bool>::print(os, value.flag);
    }
  }
};

/**
 * Add `--no-` option flag to base_option with value
 *
 * You can use different value type for positive and negative (but not
 * recommended). If you pass `void` to `base_no_option` (this is default), no
 * values accepted for negative form.
 */
template <typename base_option, typename base_no_option = void> class no_option
{
public:
  using positive_type = typename base_option::value_type;
  using negative_type = typename base_no_option::value_type;
  using value_type = no_option_value<positive_type, negative_type>;
  const static bool has_no_long_option = true;
  const static bool require_value = true;
  const static bool no_option_require_value = true;

  static_assert(base_option::require_value,
                "Positive base option does not state values are mandatory");

  static_assert(base_no_option::require_value,
                "Negative base option does not state values are mandatory");

  template <typename InIt>
  static void parse_value(value_type &value, bool p, InIt beg, InIt end)
  {
    value.flag = p;
    if (p) {
      base_option::parse_value(value.positive, p, beg, end);
    } else {
      base_no_option::parse_value(value.negative, p, beg, end);
    }
  }
};

template <typename base_option> class no_option<base_option, void>
{
public:
  using positive_type = typename base_option::value_type;
  using negative_type = void;
  using value_type = no_option_value<positive_type, negative_type>;
  const static bool has_no_long_option = true;
  const static bool require_value = true;
  const static bool no_option_require_value = false;
  const static bool no_option_accept_value = false;

  static_assert(base_option::require_value,
                "The base option does not state values are mandatory");

  template <typename InIt>
  static void parse_value(value_type &value, bool p, InIt beg, InIt end)
  {
    value.flag = p;
    base_option::parse_value(value.positive, p, beg, end);
  }

  static void parse_value(value_type &value, bool p) { value.flag = p; }
};

template <typename base_option, size_t N, char sep = ',',
          template <typename T, size_t Np> class array = std::array>
class array_option
{
public:
  using value_type = array<typename base_option::value_type, N>;

  template <typename O> using has_default_value = traits::has_default_value<O>;

protected:
  using seq = core::make_sequence<size_t, 1, N>;

  template <typename option = base_option,
            bool B = has_default_value<option>::value>
  struct base_default_value;

  template <typename option> struct base_default_value<option, true>
  {
    constexpr typename option::value_type operator()() const
    {
      return option::default_value();
    }
  };

  template <typename option> struct base_default_value<option, false>
  {
    constexpr typename option::value_type operator()() const
    {
      return typename option::value_type{};
    }
  };

  template <typename D = seq> struct default_values;

  template <size_t... values>
  struct default_values<core::sequence<size_t, values...>>
  {
    constexpr value_type operator()() const
    {
      return value_type{((void)values, base_default_value<>{}())...};
    }
  };

public:
  const static bool has_no_long_option = false;
  const static bool require_value = true;
  static constexpr value_type default_value() { return default_values<>{}(); }

  template <typename InIt>
  static void parse_value(value_type &value, bool p, InIt beg, InIt end)
  {
    std::string str(beg, end);
    std::string::size_type pos = 0, np = 0;

    for (size_t i = 0; i < N; ++i, pos = np + 1) {
      if (np == std::string::npos)
        throw std::runtime_error("More values required");

      np = str.find(sep, pos);
      std::string sstr = str.substr(pos, np - pos);
      base_option::parse_value(value.at(i), p, sstr.cbegin(), sstr.cend());
    }
    if (np != std::string::npos)
      throw std::runtime_error("Excess values found");
  }

  template <bool B = traits::has_option_value_help<base_option, true>::value>
  static typename std::enable_if<B, std::ostream &>::type
  option_value_help(std::ostream &os)
  {
    return base_option::option_value_help(os);
  }

  template <bool B = traits::has_option_value_help<base_option, false>::value>
  static typename std::enable_if<B, std::ostream &>::type
  no_option_value_help(std::ostream &os)
  {
    return base_option::no_option_value_help(os);
  }
};

/**
 * Each appearance of option appends value.
 *
 * Required API
 *
 * * The element type (`typename base_option::value_type`) needs to be move
 *   constructible.
 * * For custom bector type, `vector_type::push_back()`
 */
template <typename base_option,
          typename vector_type = std::vector<typename base_option::value_type>>
class vector_option
{
public:
  static_assert(std::is_same<typename base_option::value_type,
                             typename vector_type::value_type>::value,
                "Value type does not match");

  using value_type = vector_type;
  const static bool has_no_long_option = false;
  const static bool require_value = true;
  static value_type default_value() { return value_type(); }

  template <typename InIt>
  static void parse_value(value_type &value, bool p, InIt beg, InIt end)
  {
    typename base_option::value_type v;
    base_option::parse_value(v, p, beg, end);
    value.push_back(std::move(v));
  }

  template <bool B = traits::has_option_value_help<base_option, true>::value>
  static typename std::enable_if<B, std::ostream &>::type
  option_value_help(std::ostream &os)
  {
    return base_option::option_value_help(os);
  }

  template <bool B = traits::has_option_value_help<base_option, false>::value>
  static typename std::enable_if<B, std::ostream &>::type
  no_option_value_help(std::ostream &os)
  {
    return base_option::no_option_value_help(os);
  }
};

/**
 * Same as vector_option, with specifying container template, instead of
 * fully-qualified type.
 */
template <typename base_option, template <typename T> class vector_like>
using vector_option_t =
  vector_option<base_option, vector_like<typename base_option::value_type>>;

/**
 * Parse separator-separated values to composite type
 *
 * Each delegates needs to be parsed for each elements.
 *
 * @p delegates must have implementation of `operator()` with same prototype of
 * regular `parse_value()` function. Note that passes only composite_type value
 * itself and being non-static function.
 *
 * ```c++
 * class a_delegate
 * {
 * public:
 *   template <typename InIt>
 *   void
 *   operator()(composite_type &value_type, bool p, InIt beg, InIt end)  const
 *   {
 *     // e.g. int_parser{}(value_type.member, p, beg, end);
 *   }
 * }
 * ```
 */
template <typename composite_type, char sep, typename... delegates>
class composite_option
{
public:
  static constexpr size_t N = sizeof...(delegates);
  using value_type = composite_type;
  const static bool has_no_long_option = false;
  const static bool require_value = true;
  /* default_value() is not defined */

protected:
  template <size_t I, typename D> class parse_value_tmpl
  {
  public:
    template <typename InIt>
    void operator()(value_type &value, bool p, InIt &beg, InIt end)
    {
      if (beg == end) {
        throw std::runtime_error("Missing value " + std::to_string(I));
      }

      auto next = std::find(beg, end, sep);
      D{}(value, p, beg, next);
      if (next != end)
        ++next;
      beg = next;
    }
  };

  template <typename D> class parse_value_tmpl<N, D>
  {
  public:
    template <typename InIt>
    void operator()(value_type &value, bool p, InIt &beg, InIt end)
    {
      if (beg == end) {
        throw std::runtime_error("Missing value " + std::to_string(N));
      }

      auto next = std::find(beg, end, sep);
      D{}(value, p, beg, next);
      if (next != end)
        throw std::runtime_error("Excess value found");
    }
  };

  using has_option_value_help_seq =
    core::sequence<bool,
                   traits::has_option_value_help<delegates, true>::value...>;
  using any_has_option_value_help =
    std::integral_constant<bool,
                           core::sequence_any(has_option_value_help_seq{})>;

  template <size_t I, typename D,
            bool b = traits::has_option_value_help<D, true>::value>
  class option_value_help_tmpl_impl
  {
  public:
    void operator()(std::ostream &os) const
    {
      os << "For value " << I << "\n";
      D::option_value_help(os);
    }
  };

  template <size_t I, typename D> class option_value_help_tmpl_impl<I, D, false>
  {
  public:
    std::ostream &operator()(std::ostream &os) const { return os; }
  };

  template <size_t I, typename D>
  using option_value_help_tmpl = option_value_help_tmpl_impl<I, D>;

  template <template <size_t I, typename D> class templ>
  class for_each_delegates
  {
    using indices = core::make_sequence<size_t, 1, sizeof...(delegates)>;

    template <typename T = indices> class impl;

    template <size_t... indices> class impl<core::sequence<size_t, indices...>>
    {
    public:
      template <typename... Args> void operator()(Args &&...args) const
      {
        int f[] = {
          (templ<indices, delegates>{}(std::forward<Args>(args)...), 0)...,
        };
      }
    };

  public:
    template <typename... Args> void operator()(Args &&...args) const
    {
      impl<>{}(std::forward<Args>(args)...);
    }
  };

public:
  template <typename InIt>
  static void parse_value(value_type &value, bool p, InIt beg, InIt end)
  {
    for_each_delegates<parse_value_tmpl>{}(value, p, (beg), end);
  }

  template <typename T = any_has_option_value_help>
  static typename std::enable_if<T::value, std::ostream &>::type
  option_value_help(std::ostream &os)
  {
    for_each_delegates<option_value_help_tmpl>{}(os);
    return os;
  }
};

} // namespace option_parser
} // namespace JUPITER

#endif
