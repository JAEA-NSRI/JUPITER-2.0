/* This is -*- c++ -*- source */
/* This is vim: set ft=cpp: source */

#ifndef JUPITER_CXXOPTPARSE_MATCH_H
#define JUPITER_CXXOPTPARSE_MATCH_H

#include <jupiter/cxxoptparse/sequence.h>
#include <jupiter/cxxoptparse/traits.h>

#include <tuple>
#include <type_traits>
#include <utility>

namespace JUPITER
{
namespace option_parser
{
namespace core
{
template <typename Option, bool positive> class option_match_tag
{
};

template <typename DataType> class match_tag
{
};

template <typename tag, typename name_seq> class match_tag_name
{
};

/* tags may be nested */
template <char ch, typename... tags> class match_tags_by_ch
{
};

template <typename tag, typename name_seq>
static name_seq get_tag_sequence_impl(match_tag_name<tag, name_seq>);

template <typename tag_name>
using get_tag_sequence =
  decltype(get_tag_sequence_impl(std::declval<tag_name>()));

template <typename Option, bool positive,
          bool has_no_name = traits::has_no_long_option_name<Option>::value>
class match_tag_gen_sequence
{
  static constexpr auto &name = Option::long_option_name;

  template <int I> struct getter
  {
    static constexpr auto value = name[I];
  };

public:
  using type = typename make_sequence_from_str<decltype(name), getter>::type;
};

template <typename Option> class match_tag_gen_sequence<Option, false, true>
{
  static constexpr auto &name = Option::no_long_option_name;

  template <int I> struct getter
  {
    static constexpr auto value = name[I];
  };

public:
  using type = typename make_sequence_from_str<decltype(name), getter>::type;
};

template <typename Option> class match_tag_gen_sequence<Option, false, false>
{
  using no_sequence = sequence<char, 'n', 'o', '-'>;
  using positive_sequence = typename match_tag_gen_sequence<Option, true>::type;

public:
  using type = typename concat_sequence<no_sequence, positive_sequence>::type;
};

template <typename Option, bool positive> class match_tag_gen_impl
{
  using sequence = typename match_tag_gen_sequence<Option, positive>::type;
  using tag = match_tag<option_match_tag<Option, positive>>;

public:
  using type = match_tag_name<tag, sequence>;
};

template <typename Option,
          bool has_long = traits::has_long_option<Option>::value,
          bool has_no = traits::has_no_long_option<Option>::value,
          bool has_no_name = traits::has_no_long_option_name<Option>::value>
class match_tag_gen
{
public:
  using type = std::tuple<>;
};

template <typename Option, bool has_no_name>
class match_tag_gen<Option, true, true, has_no_name>
{
public:
  using positive = typename match_tag_gen_impl<Option, true>::type;
  using negative = typename match_tag_gen_impl<Option, false>::type;
  using type = std::tuple<positive, negative>;
};

template <typename Option> class match_tag_gen<Option, true, false, true>
{
public:
  using positive = typename match_tag_gen_impl<Option, true>::type;
  using negative = typename match_tag_gen_impl<Option, false>::type;
  using type = std::tuple<positive, negative>;
};

template <typename Option> class match_tag_gen<Option, true, false, false>
{
public:
  using positive = typename match_tag_gen_impl<Option, true>::type;
  using type = std::tuple<positive>;
};

template <typename Option, bool has_no>
class match_tag_gen<Option, false, has_no, true>
{
public:
  using negative = typename match_tag_gen_impl<Option, false>::type;
  using type = std::tuple<negative>;
};

/**
 * Extracts long option name and generates tuple of `match_tag_name`s for given
 * tuple of options.
 */
template <typename OptionsTuple> class generate_match_tags;

template <typename O, typename... Options>
class generate_match_tags<std::tuple<O, Options...>>
{
  using next = typename generate_match_tags<std::tuple<Options...>>::tags;
  using tag = typename match_tag_gen<O>::type;

public:
  using tags = typename concat_tuples<tag, next>::type;
};

template <> class generate_match_tags<std::tuple<>>
{
public:
  using tags = std::tuple<>;
};

template <typename C> class generate_match_tree_recurse;

template <typename D> class generate_match_tree_recurse<match_tag<D>>
{
public:
  using type = match_tag<D>;
};

template <char ch, typename N, typename I> class match_tag_add;

template <char ch, typename N, char chX, typename... tags>
class match_tag_add<ch, N, match_tags_by_ch<chX, tags...>>
{
  using F = match_tags_by_ch<chX, tags...>;
  using T = match_tags_by_ch<chX, tags..., N>;

public:
  static constexpr bool matched = ch == chX;
  using type = typename std::conditional<matched, T, F>::type;
};

template <char ch, typename N, typename D>
class match_tag_add<ch, N, match_tag<D>>
{
public:
  static constexpr bool matched = false;
  using type = match_tag<D>;
};

template <char ch, typename N, typename... added>
class match_tag_add<ch, N, std::tuple<added...>>
{
  using F = std::tuple<typename added::type..., match_tags_by_ch<ch, N>>;
  using T = std::tuple<typename added::type...>;
  using seq = sequence<bool, added::matched...>;
  static constexpr bool cond = sequence_any(seq());

public:
  using type = typename std::conditional<cond, T, F>::type;
};

template <typename T, typename I> class generate_match_tree_single;

template <typename tag, char ch, char... rem, typename... Inputs>
class generate_match_tree_single<
  match_tag_name<tag, sequence<char, ch, rem...>>, std::tuple<Inputs...>>
{
  using N = match_tag_name<tag, sequence<char, rem...>>;
  using A = std::tuple<match_tag_add<ch, N, Inputs>...>;

public:
  using type = typename match_tag_add<ch, N, A>::type;
};

template <typename tag, typename... Inputs>
class generate_match_tree_single<match_tag_name<tag, sequence<char>>,
                                 std::tuple<Inputs...>>
{
public:
  using type = std::tuple<Inputs..., tag>;
};

/**
 * Generate comparison tree for tags std::tuple<match_tag_name<>...>
 */
template <typename tags, typename R = std::tuple<>> class generate_match_tree;

template <char ch, typename... tags>
class generate_match_tree_recurse<match_tags_by_ch<ch, tags...>>
{
  using N = typename generate_match_tree<std::tuple<tags...>>::type;

  template <typename... R>
  static match_tags_by_ch<ch, R...> f(std::tuple<R...>);

public:
  using type = decltype(f(std::declval<N>()));
};

template <typename tag, typename... tags, typename R>
class generate_match_tree<std::tuple<tag, tags...>, R>
{
  using single = typename generate_match_tree_single<tag, R>::type;
  using next = generate_match_tree<std::tuple<tags...>, single>;

public:
  using type = typename next::type;
};

template <typename... R>
class generate_match_tree<std::tuple<>, std::tuple<R...>>
{
public:
  using type = std::tuple<typename generate_match_tree_recurse<R>::type...>;
};

/**
 * Generate code for matching to given match tree.
 */
template <template <typename Data> class func_template, typename Tree>
class generate_match;

template <template <typename Data> class func_template, char ch,
          typename... ctags, typename... tags>
class generate_match<func_template,
                     std::tuple<match_tags_by_ch<ch, ctags...>, tags...>>
{
public:
  template <typename InIt, typename... F>
  static bool match(InIt &strit, const InIt &strend, F &&...args)
  {
    if (strit != strend) {
      char chr = *strit;
      if (chr == ch) {
        using nextdep = generate_match<func_template, std::tuple<ctags...>>;

        strit++;
        return nextdep::match(strit, strend, std::forward<F>(args)...);
      }
    }

    using nexttag = generate_match<func_template, std::tuple<tags...>>;
    return nexttag::match(strit, strend, std::forward<F>(args)...);
  }
};

template <template <typename Data> class func_template, typename D,
          typename... tags>
class generate_match<func_template, std::tuple<match_tag<D>, tags...>>
{
public:
  template <typename InIt, typename... F>
  static bool match(InIt &strit, const InIt &strend, F &&...args)
  {
    if (strit == strend) {
      func_template<D>{}(strit, strend, std::forward<F>(args)...);
      return true;
    }

    using nexttag = generate_match<func_template, std::tuple<tags...>>;
    return nexttag::match(strit, strend, std::forward<F>(args)...);
  }
};

template <template <typename D> class func_template>
class generate_match<func_template, std::tuple<>>
{
public:
  template <typename InIt, typename... F>
  static bool match(InIt &strit, const InIt &strend, F &&...args)
  {
    return false;
  }
};

} // namespace core
} // namespace option_parser
} // namespace JUPITER

#endif
