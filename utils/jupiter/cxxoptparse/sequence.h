/* This is -*- c++ -*- source */
/* This is vim: set ft=cpp: source */

#ifndef JUPITER_CXXOPTPARSE_SEQUENCE_H
#define JUPITER_CXXOPTPARSE_SEQUENCE_H

#include <cstddef>
#include <tuple>
#include <type_traits>

#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#if __has_include(<utility>)
#include <utility>
#endif
#endif

namespace JUPITER
{
namespace option_parser
{
namespace core
{
#if defined(__cpp_lib_integer_sequence) && __cpp_lib_integer_sequence >= 201304L
template <typename T, T... seq>
using sequence = std::integer_sequence<T, seq...>;
#else
template <typename T, T... seq> class sequence
{
public:
  static constexpr size_t size() { return sizeof...(seq); }
  using value_type = T;
};
#endif

template <typename T, T f, T t, T m = (t + f) / 2, bool b = (t < f)>
struct make_sequence_q
{
  template <T... i, T... j>
  static sequence<T, i..., j...> c(sequence<T, i...>, sequence<T, j...>);

  static decltype(c(make_sequence_q<T, f, m>::g(),
                    make_sequence_q<T, m + 1, t>::g()))
  g();
};

template <typename T, T f, T m> struct make_sequence_q<T, f, m, m, false>
{
  static_assert(f + 1 == m, "Unexpected sequence");
  static sequence<T, f, m> g();
};

template <typename T, T t, T m> struct make_sequence_q<T, m, t, m, false>
{
  static_assert(m + 1 == t, "Unexpected sequence");
  static sequence<T, m, t> g();
};

template <typename T, T t> struct make_sequence_q<T, t, t, t, false>
{
  static sequence<T, t> g();
};

template <typename T, T f, T t, T m> struct make_sequence_q<T, f, t, m, true>
{
  static sequence<T> g();
};

/**
 * Generates `sequence<T, f, f + 1, ..., t - 1, t>`.
 */
template <typename T, T f, T t>
using make_sequence = decltype(make_sequence_q<T, f, t>::g());

/**
 * Concatenate sequence
 */
template <typename... Sequences> class concat_sequence;

template <typename S0, typename S1, typename... Sequences>
class concat_sequence<S0, S1, Sequences...>
{
  template <typename T, T... i, T... j>
  static sequence<T, i..., j...> one(sequence<T, i...>, sequence<T, j...>);

public:
  using next = typename concat_sequence<S1, Sequences...>::type;
  using type = decltype(one(std::declval<S0>(), std::declval<next>()));
};

template <typename T, T... i> class concat_sequence<sequence<T, i...>>
{
public:
  using type = sequence<T, i...>;
};

template <typename C, template <int I> class getter>
struct make_sequence_from_str;

template <typename C, int L, template <int I> class getter>
struct make_sequence_from_str<C (&)[L], getter>
{
  using seq = make_sequence<int, 0, L - 2>;
  using C_value = typename std::remove_cv<C>::type;

  template <typename seq = seq> class apply;

  template <int... seq> class apply<sequence<int, seq...>>
  {
  public:
    using type = sequence<C_value, getter<seq>::value...>;
  };

public:
  using type = typename apply<seq>::type;
};

template <size_t I, typename Seq> class get_sequence_impl;

template <typename T, T one, T... seq>
class get_sequence_impl<0, sequence<T, one, seq...>>
{
public:
  static constexpr T value = one;
};

template <size_t I, typename T, T one, T... seq>
class get_sequence_impl<I, sequence<T, one, seq...>>
{
  using next = get_sequence_impl<I - 1, sequence<T, seq...>>;

public:
  static constexpr T value = next::value;
};

template <size_t I, typename T> class get_sequence_impl<I, sequence<T>>
{
};

template <size_t I, typename T, T... seq>
static constexpr T get_sequence(sequence<T, seq...>)
{
  return get_sequence_impl<I, sequence<T, seq...>>::value;
}

static inline constexpr bool sequence_any(sequence<bool>) { return false; }

template <bool B, bool... seq>
static inline constexpr bool sequence_any(sequence<bool, B, seq...>)
{
  return B || sequence_any(sequence<bool, seq...>{});
}

static inline constexpr bool sequence_all(sequence<bool>) { return false; }

template <bool B, bool... seq>
static inline constexpr bool sequence_all(sequence<bool, B, seq...>)
{
  return B &&
         ((sizeof...(seq) > 0) ? sequence_any(sequence<bool, seq...>{}) : true);
}

//---

/**
 * Concat types of tuple. Use std::tuple_cat() for general use.
 */
template <typename... Tuples> class concat_tuples
{
public:
  using type = decltype(std::tuple_cat(std::declval<Tuples>()...));
};

} // namespace core
} // namespace option_parser
} // namespace JUPITER

#endif
