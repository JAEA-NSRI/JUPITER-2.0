#ifndef JUPITER_CXXOPTPARSE_ENUM_H
#define JUPITER_CXXOPTPARSE_ENUM_H

#include <jupiter/cxxoptparse/match.h>
#include <jupiter/cxxoptparse/sequence.h>

#include <stdexcept>
#include <string>
#include <tuple>
#include <type_traits>

namespace JUPITER
{
namespace option_parser
{
namespace core
{
//--- enum2str

template <typename enum_metadata_tuple> class enum2str;

template <typename T, typename... Ts> class enum2str<std::tuple<T, Ts...>>
{
  using next = enum2str<std::tuple<Ts...>>;

public:
  template <typename U> std::string operator()(U v) const
  {
    static_assert(std::is_same<typename T::enum_type, U>::value,
                  "Enum type does not match");

    if (v == T::value)
      return std::string(T::name);
    return next{}(v);
  }
};

template <> class enum2str<std::tuple<>>
{
public:
  template <typename U> [[noreturn]] std::string operator()(U v) const
  {
    throw std::runtime_error("Enum value did not match in tuple");
  }
};

//--- str2enum

template <typename enum_metadata_tuple> class enum_match_tags;

template <typename T, typename... Ts>
class enum_match_tags<std::tuple<T, Ts...>>
{
  static constexpr auto &name = T::name;

  template <int I> struct getter
  {
    static constexpr auto value = name[I];
  };

  using name_sequence =
    typename make_sequence_from_str<decltype(name), getter>::type;

  using next_tags = typename enum_match_tags<std::tuple<Ts...>>::tags;
  using type = match_tag_name<match_tag<T>, name_sequence>;

  template <typename C = next_tags> struct gen_tags;

  template <typename... R> struct gen_tags<std::tuple<R...>>
  {
    using tags = std::tuple<type, R...>;
  };

public:
  using tags = typename gen_tags<>::tags;
};

template <> class enum_match_tags<std::tuple<>>
{
public:
  using tags = std::tuple<>;
};

template <typename enum_metadata_tuple> class str2enum;

template <typename T, typename... Ts> class str2enum<std::tuple<T, Ts...>>
{
  using enum_type = typename T::enum_type;

  template <typename trait> class func
  {
  public:
    template <typename InIt>
    void operator()(InIt &strit, const InIt &strend, enum_type &value) const
    {
      value = trait::value;
    }
  };

  using tags = typename enum_match_tags<std::tuple<T, Ts...>>::tags;
  using tree = typename generate_match_tree<tags>::type;
  using match = generate_match<func, tree>;

public:
  template <typename InIt> enum_type operator()(InIt &strit, const InIt &strend)
  {
    enum_type value;
    if (!match::match(strit, strend, value))
      throw std::runtime_error("Enum value did not match");
    return value;
  }

  enum_type operator()(const std::string &str)
  {
    auto strit = str.cbegin();
    return operator()(strit, str.cend());
  }
};
} // namespace core

/*
 * Theses symbols are available for the users' use.
 */

using core::enum2str;
using core::str2enum;

#define JUPITER_CXXOPTPARSE_ENUM2STR_R(template_class_name, type, enum_value, \
                                       namestr, description)                  \
  template <> class template_class_name<enum_value>                           \
  {                                                                           \
  public:                                                                     \
    using enum_type = type;                                                   \
    static constexpr enum_type value = enum_value;                            \
    static constexpr auto &name = "" namestr;                                 \
    description                                                               \
  };

#define JUPITER_CXXOPTPARSE_ENUM2STR_D(description_str) \
  static std::ostream &description(std::ostream &os)    \
  {                                                     \
    return os << "" description_str;                    \
  }

#define JUPITER_CXXOPTPARSE_ENUM2STR_P(n, d, E, ...) E
#define JUPITER_CXXOPTPARSE_ENUM2STR_N(...) \
  JUPITER_CXXOPTPARSE_ENUM2STR_P(__VA_ARGS__, 2, 1, 0)

#define JUPITER_CXXOPTPARSE_ENUM2STR_1(t, e, v, n) \
  JUPITER_CXXOPTPARSE_ENUM2STR_R(t, e, v, n, )
#define JUPITER_CXXOPTPARSE_ENUM2STR_2(t, e, v, n, d) \
  JUPITER_CXXOPTPARSE_ENUM2STR_R(t, e, v, n, JUPITER_CXXOPTPARSE_ENUM2STR_D(d))

#define JUPITER_CXXOPTPARSE_ENUM2STR_X(t, e, v, n, ...) \
  JUPITER_CXXOPTPARSE_ENUM2STR_##n(t, e, v, __VA_ARGS__)
#define JUPITER_CXXOPTPARSE_ENUM2STR_E(t, e, v, n, ...) \
  JUPITER_CXXOPTPARSE_ENUM2STR_X(t, e, v, n, __VA_ARGS__)

#define JUPITER_CXXOPTPARSE_ENUM2STR(template_class_name, enum_type,          \
                                     enum_value, ...)                         \
  JUPITER_CXXOPTPARSE_ENUM2STR_E(template_class_name, enum_type, enum_value,  \
                                 JUPITER_CXXOPTPARSE_ENUM2STR_N(__VA_ARGS__), \
                                 __VA_ARGS__)

template <typename E, template <E> class trait, typename Sequence>
class to_enum_trait_tuple;

template <typename E, template <E> class trait, E... values>
class to_enum_trait_tuple<E, trait, core::sequence<E, values...>>
{
public:
  using type = std::tuple<trait<values>...>;
};

/**
 * Make tuple of metadata trait of enum values
 */
template <typename E, template <E> class trait, E... values>
using enum_trait_tuple_type =
  typename to_enum_trait_tuple<E, trait, core::sequence<E, values...>>::type;

} // namespace option_parser
} // namespace JUPITER

#endif
