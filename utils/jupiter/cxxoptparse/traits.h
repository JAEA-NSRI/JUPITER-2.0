/* This is -*- c++ -*- source */
/* This is vim: set ft=cpp: source */

#ifndef JUPITER_CXXOPTPARSE_TRAITS_H
#define JUPITER_CXXOPTPARSE_TRAITS_H

#include <jupiter/cxxoptparse/common.h>
#include <jupiter/cxxoptparse/sequence.h>

#include <cstddef>
#include <ostream>
#include <tuple>
#include <type_traits>

namespace JUPITER
{
namespace option_parser
{
/**
 * @brief Tester for option metadata class
 */
namespace traits
{
namespace common
{
template <typename T>
using remove_ref_t = typename std::remove_reference<T>::type;

template <typename T> using remove_cv_t = typename std::remove_cv<T>::type;

template <typename T> using remove_cvref_t = remove_cv_t<remove_ref_t<T>>;

template <typename T>
using remove_all_extents_t = typename std::remove_all_extents<T>::type;

template <typename X, typename Y>
using element_type_eq =
  std::is_same<remove_all_extents_t<remove_cvref_t<X>>, Y>;

template <typename U> using is_string_or_char = element_type_eq<U, char>;
template <typename U> using any = std::is_same<U, U>;

template <bool B>
using true_if = typename std::enable_if<B, std::true_type>::type;

template <bool B>
using false_if = typename std::enable_if<B, std::false_type>::type;

template <bool B>
using to_bool_type =
  typename std::conditional<B, std::true_type, std::false_type>::type;

/* Tests whether tester<O>::value can be deducible (exists) and return true */
template <typename O, template <typename M> class tester,
          typename deduce_fail_type = std::false_type>
class test_type
{
  template <typename M> static true_if<tester<M>::value> impl(int);
  template <typename M> static false_if<!tester<M>::value> impl(int);
  template <typename M> static deduce_fail_type impl(long);

public:
  static constexpr bool value = decltype(impl<O>(1))::value;
};

template <typename U> class is_tuple
{
public:
  static constexpr bool value = false;
};

template <typename... U> class is_tuple<std::tuple<U...>>
{
public:
  static constexpr bool value = true;
};

template <typename U, template <typename O> class tester> class is_tuple_of
{
public:
  static constexpr bool value = false;
};

template <typename... U, template <typename O> class tester>
class is_tuple_of<std::tuple<U...>, tester>
{
  using seq = core::sequence<bool, tester<U>::value...>;

public:
  static constexpr bool value = core::sequence_all(seq{});
};
} // namespace common

/**
 * @brief Test whether option `O` has long option.
 */
template <typename O> class has_long_option
{
  template <typename U> using is_char = common::is_string_or_char<U>;
  template <typename M> using tester = is_char<decltype(M::long_option_name)>;

public:
  static constexpr bool value = common::test_type<O, tester>::value;
};

/**
 * @brief Test whether option `O` has short option.
 */
template <typename O> class has_short_option
{
  template <typename U> using is_char = common::is_string_or_char<U>;
  template <typename M> using tester = is_char<decltype(M::short_option_name)>;

public:
  static constexpr bool value = common::test_type<O, tester>::value;
};

/**
 * @brief Test whether option `O` accepts `--no-` form long option
 */
template <typename O> class has_no_long_option
{
  template <typename M>
  using tester = common::to_bool_type<M::has_no_long_option>;

public:
  static constexpr bool value = common::test_type<O, tester>::value;
};

/**
 * @brief Test whether option `O` accepts custom negative long option
 */
template <typename O>
class has_no_long_option_name
{
  template <typename U> using is_char = common::is_string_or_char<U>;
  template <typename M>
  using tester = is_char<decltype(M::no_long_option_name)>;

public:
  static constexpr bool value = common::test_type<O, tester>::value;
};

/**
 * @brief Test whether option `O` accept negative short option
 */
template <typename O>
class has_no_short_option
{
  template <typename U> using is_char = common::is_string_or_char<U>;
  template <typename M>
  using tester = is_char<decltype(M::no_short_option_name)>;

public:
  static constexpr bool value = common::test_type<O, tester>::value;
};

/**
 * @brief Test whether option `O` accept in any negative form
 */
template <typename O>
class has_negative
{
public:
  static constexpr bool value = has_no_long_option<O>::value ||
                                has_no_long_option_name<O>::value ||
                                has_no_short_option<O>::value;
};

/**
 * @brief Test whether option `O` has default value
 */
template <typename O> class has_default_value
{
  template <typename U> using any = common::any<U>;
  template <typename M> using tester = any<decltype(M::default_value())>;

public:
  static constexpr bool value = common::test_type<O, tester>::value;
};

/**
 * @brief Test whether option `O` provides a help for (possible) values
 */
template <typename O, bool positive> class has_option_value_help
{
  template <typename U> using any = common::any<U>;
  template <typename M>
  using tester =
    any<decltype(M::option_value_help(std::declval<std::ostream &>()))>;

public:
  static constexpr bool value = common::test_type<O, tester>::value;
};

/**
 * @brief Same as has_value_option_help but for negative option
 */
template <typename O> class has_option_value_help<O, false>
{
  template <typename U> using any = common::any<U>;
  template <typename M>
  using tester =
    any<decltype(M::no_option_value_help(std::declval<std::ostream &>()))>;

public:
  static constexpr bool value = common::test_type<O, tester>::value;
};

/**
 * @brief Test whether (negative) option `O` require a option value
 */
template <typename O, bool positive> class requires_value
{
  template <typename M> using tester = common::to_bool_type<M::require_value>;

public:
  static constexpr bool value = common::test_type<O, tester>::value;
};

template <typename O> class requires_value<O, false>
{
  template <typename M>
  using tester = common::to_bool_type<M::no_option_require_value>;

public:
  static constexpr bool value = common::test_type<O, tester>::value;
};

template <typename O>
using positive_option_requires_value = requires_value<O, true>;

template <typename O>
using negative_option_requires_value = requires_value<O, false>;

/**
 * @brief Test whether (negative) option `O` accepts a option value
 *
 * Value is optional.
 */
template <typename O, bool positive> class accepts_value
{
  template <typename M> using tester = common::to_bool_type<M::accept_value>;

public:
  static constexpr bool value = common::test_type<O, tester>::value;
};

template <typename O> class accepts_value<O, false>
{
  template <typename M>
  using tester = common::to_bool_type<M::no_option_accept_value>;

public:
  static constexpr bool value = common::test_type<O, tester>::value;
};

template <typename O>
using positive_option_accepts_value = accepts_value<O, true>;

template <typename O>
using negative_option_accepts_value = accepts_value<O, false>;

/**
 * Short option cannot omit value when accepts value
 */
template <typename O, bool positive>
using short_option_require_value =
  typename std::conditional<accepts_value<O, positive>::value ||
                              requires_value<O, positive>::value,
                            std::true_type, std::false_type>::type;

/**
 * @brief Test whether short option for option `O` can be combined with other
 * options.
 *
 * The value is `true` when `allow_combine_short_option` is not in a member
 * of or wrongly defined in `O`.
 */
template <typename O> class is_allow_combine_short_option
{
  template <typename M>
  using tester = common::to_bool_type<M::allow_combine_short_option>;

public:
  static constexpr bool value =
    common::test_type<O, tester, std::true_type>::value;
};

/**
 * Has description (for option, description is mandatory). This trait is used
 * for enum value description, but can also be used for options.
 */
template <typename O> class has_description
{
  template <typename U> using is_stream = std::is_same<U, std::ostream &>;

  template <typename M>
  using tester =
    is_stream<decltype(M::description(std::declval<std::ostream &>()))>;

public:
  static constexpr bool value = common::test_type<O, tester>::value;
};

/**
 * Has separate description for negative option
 */
template <typename O> class has_no_description
{
  template <typename U> using is_stream = std::is_same<U, std::ostream &>;

  template <typename M>
  using tester =
    is_stream<decltype(M::no_description(std::declval<std::ostream &>()))>;

public:
  static constexpr bool value = common::test_type<O, tester>::value;
};

/**
 * @brief Find matching option from `tuple<Options...>`.
 *
 * `type` will be `void` if not found.
 */
template <typename OptionsTuple, template <typename T> class Comp>
class find_option;

template <template <typename T> class Comp, typename O, typename... Os>
class find_option<std::tuple<O, Os...>, Comp>
{
  using next = find_option<std::tuple<Os...>, Comp>;

  template <typename T>
  static typename std::enable_if<Comp<T>::value, T>::type found_option(int);

  template <typename T> static typename next::type found_option(long);

public:
  using type = decltype(found_option<O>(1));
};

template <template <typename T> class Comp>
class find_option<std::tuple<>, Comp>
{
public:
  using type = void;
};

/**
 * @brief Find matching options' index from `tuple<Options...>`.
 *
 * @note Finding option must exist. Otherwise it raises compilation
 * error.
 */
template <typename OptionsTuple, template <typename T> class Comp, size_t I = 0>
class find_index;

template <template <typename T> class Comp, size_t I, typename O,
          typename... Os>
class find_index<std::tuple<O, Os...>, Comp, I>
{
  template <typename T>
  static typename std::enable_if<Comp<T>::value,
                                 std::integral_constant<size_t, I>>::type
  found_index(int);

  template <typename T>
  static find_index<std::tuple<Os...>, Comp, I + 1> found_index(long);

public:
  static constexpr size_t value = decltype(found_index<O>(1))::value;
};

template <template <typename T> class Comp, size_t I>
class find_index<std::tuple<>, Comp, I>
{
};

/**
 * @brief Find data index of given option in options.
 */
template <typename Option, typename OptionsTuple> class data_index
{
  template <typename O> using comp = std::is_same<O, Option>;

public:
  static constexpr size_t value = find_index<OptionsTuple, comp>::value;
};

/**
 * @brief Find help option (an option that derives `help`)
 */
template <typename OptionsTuple, typename help = help> class find_help
{
  template <typename O> using comp = std::is_base_of<help, O>;

public:
  using type = typename find_option<OptionsTuple, comp>::type;
};

/**
 * @brief Find stop option (an option that derives `stop`)
 */
template <typename OptionsTuple, typename stop = stop> class find_stop
{
  template <typename O> using comp = std::is_base_of<stop, O>;

public:
  using type = typename find_option<OptionsTuple, comp>::type;
};

template <typename Option> class validate_option
{
public:
  constexpr bool operator()() const
  {
    static_assert(has_long_option<Option>::value ||
                    has_short_option<Option>::value,
                  "Option must have a long or short option");
    return true;
  }
};

template <typename... Options>
class validate_options
{
public:
  constexpr bool operator()() const
  {
    using U = core::sequence<bool, validate_option<Options>{}()...>;
    return core::sequence_all(U{});
  }
};

} // namespace traits
} // namespace option_parser
} // namespace JUPITER

#endif
