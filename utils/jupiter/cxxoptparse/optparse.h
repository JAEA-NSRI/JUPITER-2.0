/* This is -*- c++ -*- source */
/* This is vim: set ft=cpp: source */

#ifndef JUPITER_CXXOPTPARSE_H
#define JUPITER_CXXOPTPARSE_H

#include <jupiter/cxxoptparse/common.h>
#include <jupiter/cxxoptparse/enum.h>
#include <jupiter/cxxoptparse/exceptions.h>
#include <jupiter/cxxoptparse/help.h>
#include <jupiter/cxxoptparse/help_printer.h>
#include <jupiter/cxxoptparse/match.h>
#include <jupiter/cxxoptparse/parsers.h>
#include <jupiter/cxxoptparse/sequence.h>
#include <jupiter/cxxoptparse/stop.h>
#include <jupiter/cxxoptparse/traits.h>
#include <jupiter/cxxoptparse/types.h>
#include <jupiter/cxxoptparse/value_printer.h>

#include <stdexcept>
#include <tuple>
#include <type_traits>

namespace JUPITER
{
namespace option_parser
{
namespace core
{
template <typename Option, typename D, typename OptName>
inline void parse_value(D &data, const OptName &optname, bool positive)
{
  try {
    Option::parse_value(data, positive);
  } catch (const std::runtime_error &r) {
    throw invalid_value_error(optname, r.what());
  }
}

template <typename Option, typename D, typename OptName, typename It>
inline void parse_value(D &data, const OptName &optname, bool positive, It beg,
                        It end)
{
  try {
    Option::parse_value(data, positive, beg, end);
  } catch (const std::runtime_error &r) {
    throw invalid_value_error(optname, string_type(beg, end), r.what());
  }
}

//--- long option processor

template <typename Option, bool positive, size_t index,
          bool require_value = traits::requires_value<Option, positive>::value,
          bool accept_value = traits::accepts_value<Option, positive>::value>
class process_long_option
{
public:
  /* does not accept value */
  template <typename D>
  void operator()(string_iter &strit, const string_iter &optend,
                  const string_iter &strend, argv_iter &vecit,
                  const argv_iter &vecend, D &data)
  {
    if (strend != optend)
      throw excess_value_error(*vecit);
    parse_value<Option>(std::get<index>(data), Option::long_option_name,
                        positive);
  }
};

template <typename Option, bool positive, size_t index, bool accept>
class process_long_option<Option, positive, index, true, accept>
{
public:
  /* requires value */
  template <typename D>
  void operator()(string_iter &strit, const string_iter &optend,
                  const string_iter &strend, argv_iter &vecit,
                  const argv_iter &vecend, D &data)
  {
    if (strend == optend) {
      ++vecit; /* swallow next argument as value */
      if (vecit == vecend)
        throw missing_value_error(Option::long_option_name);
      parse_value<Option>(std::get<index>(data), Option::long_option_name,
                          positive, vecit->cbegin(), vecit->cend());
    } else {
      strit = optend;
      ++strit;
      parse_value<Option>(std::get<index>(data), Option::long_option_name,
                          positive, strit, strend);
    }
  }
};

template <typename Option, bool positive, size_t index>
class process_long_option<Option, positive, index, false, true>
{
public:
  /* accepts value */
  template <typename D>
  void operator()(string_iter &strit, const string_iter &optend,
                  const string_iter &strend, argv_iter &vecit,
                  const argv_iter &vecend, D &data)
  {
    if (strend == optend) {
      parse_value<Option>(std::get<index>(data), Option::long_option_name,
                          positive);
    } else {
      strit = optend;
      ++strit;
      parse_value<Option>(std::get<index>(data), Option::long_option_name,
                          positive, strit, strend);
    }
  }
};

template <typename OptionsTuple> class process_long_option_all
{
  template <typename O> using data_index = traits::data_index<O, OptionsTuple>;
  template <typename D> class processor;

  template <typename O, bool positive>
  class processor<option_match_tag<O, positive>>
    : public process_long_option<O, positive, data_index<O>::value>
  {
  };

  using tags = typename generate_match_tags<OptionsTuple>::tags;
  using tree = typename generate_match_tree<tags>::type;
  using match = generate_match<processor, tree>;

public:
  template <typename D>
  static void parse(D &data, string_iter &strit, const string_iter &optend,
                    const string_iter &strend, argv_iter &vecit,
                    const argv_iter &vecend)
  {
    string_iter beg = strit;
    if (match::match(strit, optend, strend, vecit, vecend, data))
      return;

    /* C++11 requires strictly same type for template instanciation */
    throw invalid_long_option(static_cast<const string_iter &>(beg), optend);
  }
};

template <typename OptionsTuple, typename D>
void parse_long_option(D &data, string_iter &strit, const string_iter &strend,
                       argv_iter &vecit, const argv_iter &vecend)
{
  static constexpr auto equal = decltype(*strit)('=');

  string_iter cur = strit;
  while (cur != strend && *cur != equal)
    ++cur;

  using processor = process_long_option_all<OptionsTuple>;
  processor::parse(data, strit, cur, strend, vecit, vecend);
  ++vecit;
}

//--- short option processor

template <typename Option, bool positive> class short_option_name
{
public:
  static constexpr char name = Option::short_option_name;
};

template <typename Option> class short_option_name<Option, false>
{
public:
  static constexpr char name = Option::no_short_option_name;
};

template <typename Option, bool positive, size_t data_index,
          bool require_value =
            traits::short_option_require_value<Option, positive>::value>
class process_short_option_value
{
public:
  static constexpr bool value_parsed = require_value;
  static constexpr char name()
  {
    return short_option_name<Option, positive>::name;
  }

  /* values are not accepted */
  template <typename D>
  static void parse(D &data, string_iter &strit, const string_iter &strend,
                    argv_iter &vecit, const argv_iter &vecend)
  {
    parse_value<Option>(std::get<data_index>(data), name(), positive);
  }
};

template <typename Option, bool positive, size_t data_index>
class process_short_option_value<Option, positive, data_index, true>
{
public:
  static constexpr bool value_parsed = true;
  static constexpr char name()
  {
    return short_option_name<Option, positive>::name;
  }

  /* values are required (cannot omit value for short option) */
  template <typename D>
  static void parse(D &data, string_iter &strit, const string_iter &strend,
                    argv_iter &vecit, const argv_iter &vecend)
  {

    ++strit;
    if (strit == strend) {
      ++vecit;
      if (vecit == vecend)
        throw missing_value_error(*--strit);
      parse_value<Option>(std::get<data_index>(data), name(), positive,
                          vecit->cbegin(), vecit->cend());
    } else {
      parse_value<Option>(std::get<data_index>(data), name(), positive, strit,
                          strend);
      strit = strend;
    }
  }
};

template <typename Option, bool positive, bool first, size_t data_index,
          bool allow_combine =
            traits::is_allow_combine_short_option<Option>::value>
class process_short_option_combine
{
public:
  using processor = process_short_option_value<Option, positive, data_index>;
  static constexpr bool value_parsed = processor::value_parsed;

  template <typename D>
  static bool parse(D &data, string_iter &strit, const string_iter &strend,
                    argv_iter &vecit, const argv_iter &vecend)
  {
    if (*strit == processor::name()) {
      processor::parse(data, strit, strend, vecit, vecend);
      return true;
    }
    return false;
  }
};

template <typename Option, bool positive, bool first, size_t data_index>
class process_short_option_combine<Option, positive, first, data_index, false>
{
public:
  using processor = process_short_option_value<Option, positive, data_index>;
  static constexpr bool value_parsed = processor::value_parsed;

  template <typename D>
  static bool parse(D &data, string_iter &strit, const string_iter &strend,
                    argv_iter &vecit, const argv_iter &vecend)
  {
    /* Skip if value not accepted and not solely used */
    if (!first || (!processor::value_parsed && strit + 1 != strend))
      return false;
    if (*strit == processor::name()) {
      processor::parse(data, strit, strend, vecit, vecend);
      return true;
    }
    return false;
  }
};

template <typename Tag, bool first, template <typename O> class index>
class process_short_option;

template <typename Option, bool positive, bool first,
          template <typename O> class index>
class process_short_option<option_match_tag<Option, positive>, first, index>
{
public:
  static constexpr size_t data_index = index<Option>::value;
  using processor =
    process_short_option_combine<Option, positive, first, data_index>;
};

template <typename MatchTags, bool first, template <typename O> class index>
class process_short_option_tags;

template <typename Tag, bool first, typename... Tags,
          template <typename O> class index>
class process_short_option_tags<std::tuple<Tag, Tags...>, first, index>
{
  using processor = typename process_short_option<Tag, first, index>::processor;
  using next = process_short_option_tags<std::tuple<Tags...>, first, index>;

public:
  template <typename D>
  static bool parse(D &data, string_iter &strit, const string_iter &strend,
                    argv_iter &vecit, const argv_iter &vecend)
  {
    if (processor::parse(data, strit, strend, vecit, vecend))
      return processor::value_parsed;
    return next::parse(data, strit, strend, vecit, vecend);
  }
};

template <bool first, template <typename O> class index>
class process_short_option_tags<std::tuple<>, first, index>
{
public:
  template <typename D>
  static bool parse(D &data, string_iter &strit, const string_iter &strend,
                    argv_iter &vecit, const argv_iter &vecend)
  {
    throw invalid_short_option(*strit);
  }
};

template <typename Option, bool P = traits::has_short_option<Option>::value,
          bool N = traits::has_no_short_option<Option>::value>
class process_short_option_to_tags_impl
{
public:
  using type = std::tuple<>;
};

template <typename Option>
class process_short_option_to_tags_impl<Option, true, true>
{
public:
  using positive = option_match_tag<Option, true>;
  using negative = option_match_tag<Option, false>;
  using type = std::tuple<positive, negative>;
};

template <typename Option>
class process_short_option_to_tags_impl<Option, true, false>
{
public:
  using positive = option_match_tag<Option, true>;
  using type = std::tuple<positive>;
};

template <typename Option>
class process_short_option_to_tags_impl<Option, false, true>
{
public:
  using negative = option_match_tag<Option, false>;
  using type = std::tuple<negative>;
};

template <typename Options> class process_short_option_to_tags;

template <typename Option, typename... Options>
class process_short_option_to_tags<std::tuple<Option, Options...>>
{
  using tag = typename process_short_option_to_tags_impl<Option>::type;
  using next =
    typename process_short_option_to_tags<std::tuple<Options...>>::type;

public:
  using type = typename concat_tuples<tag, next>::type;
};

template <> class process_short_option_to_tags<std::tuple<>>
{
public:
  using type = std::tuple<>;
};

template <typename OptionsTuple, bool first> class process_short_option_all
{
public:
  template <typename O> using data_index = traits::data_index<O, OptionsTuple>;
  using tags = typename process_short_option_to_tags<OptionsTuple>::type;
  using processor = process_short_option_tags<tags, first, data_index>;
};

template <typename OptionsTuple, typename D>
void parse_short_options(D &data, string_iter &strit, const string_iter &strend,
                         argv_iter &vecit, const argv_iter &vecend)
{
  using processor_1 =
    typename process_short_option_all<OptionsTuple, true>::processor;
  using processor_r =
    typename process_short_option_all<OptionsTuple, false>::processor;

  if (!processor_1::parse(data, strit, strend, vecit, vecend)) {
    ++strit;
    for (; strit != strend; ++strit) {
      if (processor_r::parse(data, strit, strend, vecit, vecend))
        break;
    }
  }
  ++vecit;
}

template <typename OptionsTuple, typename D>
void parse_list(argv_iter beg, argv_iter end, argv_type &argv_out, D &data,
                const bool &stopped)
{
  static const auto hyphen = decltype(*beg->cbegin())('-');

  while (beg != end) {
    auto stred = beg->cend();
    auto strit = beg->cbegin();
    if (stopped || strit == stred || *strit++ != hyphen) {
      argv_out.push_back(*beg);
      ++beg;
      continue;
    }

    if (*strit == hyphen && strit + 1 != stred) {
      strit++;
      parse_long_option<OptionsTuple>(data, strit, stred, beg, end);
    } else {
      parse_short_options<OptionsTuple>(data, strit, stred, beg, end);
    }
  }
}

template <typename OptionsTuple,
          typename help_option = typename traits::find_help<OptionsTuple>::type,
          bool has_help = !std::is_same<help_option, void>::value,
          typename stop_option = typename traits::find_stop<OptionsTuple>::type,
          bool has_stop = !std::is_same<stop_option, void>::value>
class parser;

template <typename OptionsTuple, typename help_option, typename stop_option>
class parser<OptionsTuple, help_option, true, stop_option, true>
{
public:
  template <typename O> using data_index = traits::data_index<O, OptionsTuple>;

  template <typename D>
  static void parse(argv_iter beg, argv_iter end, argv_type &argv_out, D &data)
  {
    constexpr auto help_index = data_index<help_option>::value;
    constexpr auto stop_index = data_index<stop_option>::value;
    help_value &h = std::get<help_index>(data);
    bool &stopped = std::get<stop_index>(data);

    parse_list<OptionsTuple>(beg, end, argv_out, data, stopped);
    if (h.want_help)
      h.print_help_if_needed();
  }
};

template <typename OptionsTuple, typename help_option, typename stop_option>
class parser<OptionsTuple, help_option, false, stop_option, true>
{
public:
  template <typename O> using data_index = traits::data_index<O, OptionsTuple>;

  template <typename D>
  static void parse(argv_iter beg, argv_iter end, argv_type &argv_out, D &data)
  {
    constexpr auto stop_index = data_index<stop_option>::value;
    bool &stopped = std::get<stop_index>(data);

    parse_list<OptionsTuple>(beg, end, argv_out, data, stopped);
  }
};

template <typename OptionsTuple, typename help_option, typename stop_option>
class parser<OptionsTuple, help_option, true, stop_option, false>
{
public:
  template <typename O> using data_index = traits::data_index<O, OptionsTuple>;

  template <typename D>
  static void parse(argv_iter beg, argv_iter end, argv_type &argv_out, D &data)
  {
    constexpr auto help_index = data_index<help_option>::value;
    help_value &h = std::get<help_index>(data);
    const bool stopped = false;

    parse_list<OptionsTuple>(beg, end, argv_out, data, stopped);
    if (h.want_help)
      h.print_help_if_needed();
  }
};

template <typename OptionsTuple, typename help_option, typename stop_option>
class parser<OptionsTuple, help_option, false, stop_option, false>
{
public:
  template <typename O> using data_index = traits::data_index<O, OptionsTuple>;

  template <typename D>
  static void parse(argv_iter beg, argv_iter end, argv_type &argv_out, D &data)
  {
    const bool stopped = false;

    parse_list<OptionsTuple>(beg, end, argv_out, data, stopped);
  }
};

template <typename Option,
          bool has_default_value = traits::has_default_value<Option>::value>
class make_default_value_tuple_f
{
  using value_type = typename Option::value_type;

public:
  static constexpr value_type value() { return value_type{}; }
};

template <typename Option> class make_default_value_tuple_f<Option, true>
{
  using value_type = typename Option::value_type;

public:
  static constexpr value_type value() { return Option::default_value(); }
};

template <typename... Options>
static constexpr std::tuple<typename Options::value_type...>
make_default_value_tuple()
{
  return std::make_tuple(make_default_value_tuple_f<Options>::value()...);
}

} // namespace core

/**
 * Generic option parser
 *
 * - help needs to be included explicitly, but help will be printed if a `help`
 *   option included and used it in the command line.
 * - stop needs to be included explicitly, but stop will be acted if a `stop`
 *   option included and used it in the command line.
 */
template <typename... Options> class parser_base
{
public:
  using options_tuple = std::tuple<Options...>;
  using argv_type = core::argv_type;
  using data_type = core::data_type<Options...>;

  template <typename Option>
  using data_index = traits::data_index<Option, options_tuple>;

  argv_type argv;
  data_type data = core::make_default_value_tuple<Options...>();

  template <typename Option> const typename Option::value_type &get() const
  {
    return std::get<data_index<Option>::value>(data);
  }

  template <typename Option> typename Option::value_type &get()
  {
    return std::get<data_index<Option>::value>(data);
  }

protected:
  using parser = core::parser<options_tuple>;

public:
  virtual void parse(core::argv_iter beg, core::argv_iter end)
  {
    traits::validate_options<Options...>{}();
    parser::parse(beg, end, argv, data);
  }

  /**
   * @note all (i.e., including argv[0]) will be parsed as parameters.
   */
  virtual void parse(int argc, char **argv)
  {
    argv_type list;
    for (int i = 0; i < argc; ++i)
      list.push_back(argv[i]);
    parse(list.cbegin(), list.cend());
  }

  /**
   * Reset to default value and clear argv.
   */
  void reset()
  {
    argv.clear();
    data = core::make_default_value_tuple<Options...>();
  }
};

template <typename OptionsTuple,
          template <typename... Os> class help_option = help_option,
          typename stop_option = stop>
class parser_with_help;

template <typename... Options, template <typename... Os> class help_option,
          class stop_option>
class parser_with_help<std::tuple<Options...>, help_option, stop_option>
  : public parser_base<Options..., help_option<Options...>, stop_option>
{
public:
  using help = help_option<Options...>;
  using help_value = typename help::value_type;
  using base = parser_base<Options..., help, stop_option>;
  using argv_type = typename base::argv_type;
  using data_type = typename base::data_type;
  using options_tuple = typename base::options_tuple;

  template <typename Option>
  using data_index = typename base::template data_index<Option>;

  argv_type &argv = base::argv;
  data_type &data = base::data;

  template <typename Option> typename Option::value_type &get()
  {
    return base::template get<Option>();
  }

  template <typename Option> const typename Option::value_type &get() const
  {
    return base::template get<Option>();
  }

  help_value &get_help_value() { return get<help>(); }
  const help_value &get_help_value() const { return get<help>(); }

  virtual void parse(core::argv_iter beg, core::argv_iter end) override
  {
    help_value &help = get_help_value();
    if (help.progname().empty())
      help.progname() = *beg++;
    base::parser::parse(beg, end, argv, data);
  }

  virtual void parse(int argc, char **argv) override
  {
    base::parse(argc, argv);
  }
};

template <typename... Options>
using parser = parser_with_help<std::tuple<Options...>>;

} // namespace option_parser
} // namespace JUPITER

#endif
