/* This is -*- c++ -*- source */
/* This is vim: set ft=cpp: source */

#ifndef JUPITER_CXXOPTPARSE_HELP_PRINTER_H
#define JUPITER_CXXOPTPARSE_HELP_PRINTER_H

#include <jupiter/cxxoptparse/traits.h>
#include <jupiter/cxxoptparse/value_printer.h>
#include <jupiter/os/os.h>

#include <algorithm>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <string>
#include <type_traits>

namespace JUPITER
{
namespace option_parser
{
namespace help_printer
{
static constexpr int indent = 25;

using traits::accepts_value;
using traits::has_default_value;
using traits::has_description;
using traits::has_long_option;
using traits::has_negative;
using traits::has_no_description;
using traits::has_no_long_option;
using traits::has_no_long_option_name;
using traits::has_no_short_option;
using traits::has_option_value_help;
using traits::has_short_option;
using traits::requires_value;

static void add_nl_if_not(std::ostringstream &os)
{
  if (*(os.str().cend() - 1) != '\n')
    os << "\n";
}

/**
 * Whether separated descriptions or option value help for negative option
 * provided.
 *
 * Whether prints documents in
 *
 * ```
 *  -a --[no-]abc       Shared description
 *                      Shared option value help
 * ```
 *
 * or
 *
 * ```
 *  -a --abc            Yes description
 *                      Yes option value help
 *  -b --no-abc         No description
 *                      No option value help
 * ```
 */
template <typename O>
using needs_separated =
  typename std::conditional<has_no_description<O>::value ||
                              has_option_value_help<O, false>::value,
                            std::true_type, std::false_type>::type;

template <typename O, bool positive>
using share_value_format_impl = typename std::conditional<
  requires_value<O, positive>::value, std::integral_constant<int, 2>,
  typename std::conditional<accepts_value<O, positive>::value,
                            std::integral_constant<int, 1>,
                            std::integral_constant<int, 0>>::type>::type;

/**
 * Tests whether negative option needs to be separated because of
 * value name printing (required, optional or nothing).
 */
template <typename O>
using share_value_format =
  typename std::conditional<share_value_format_impl<O, true>::value ==
                              share_value_format_impl<O, false>::value,
                            std::true_type, std::false_type>::type;

/**
 * Whether prints options in
 *
 * ```
 *  -a --[no-]abc       ...
 * ```
 *
 * or
 *
 * ```
 *  -a --abc
 *     --no-abc         ...
 * ```
 */
template <typename O>
using share_no_option = typename std::conditional<
  !needs_separated<O>::value && !has_no_long_option_name<O>::value &&
    !has_no_short_option<O>::value && share_value_format<O>::value,
  std::true_type, std::false_type>::type;

class print_nop
{
};

inline std::ostream &operator<<(std::ostream &os, const print_nop &)
{
  return os;
}

template <typename O, bool B = has_short_option<O>::value>
class print_positive_short_option
{
};

template <typename O>
std::ostream &operator<<(std::ostream &os,
                         const print_positive_short_option<O, true> &)
{
  return os << "-" << O::short_option_name;
}

template <typename O>
std::ostream &operator<<(std::ostream &os,
                         const print_positive_short_option<O, false> &)
{
  return os << "  ";
}

template <typename O, bool B = has_no_short_option<O>::value>
class print_negative_short_option
{
};

template <typename O>
std::ostream &operator<<(std::ostream &os,
                         const print_negative_short_option<O, true> &)
{
  return os << "-" << O::no_short_option_name;
}

template <typename O>
std::ostream &operator<<(std::ostream &os,
                         const print_negative_short_option<O, false> &)
{
  return os << "  ";
}

template <typename O, bool positive> class print_short_option;

template <typename O>
class print_short_option<O, true> : public print_positive_short_option<O>
{
};

template <typename O>
class print_short_option<O, false> : public print_negative_short_option<O>
{
};

template <typename O> class print_shared_no_option
{
};

template <typename O>
std::ostream &operator<<(std::ostream &os, const print_shared_no_option<O> &)
{
  return os << " --[no-]" << O::long_option_name;
}

template <typename O> class print_positive_option
{
};

template <typename O>
std::ostream &operator<<(std::ostream &os, const print_positive_option<O> &)
{
  return os << " --" << O::long_option_name;
}

template <typename O, bool has_no_name = has_no_long_option_name<O>::value>
class print_negative_option
{
};

template <typename O>
std::ostream &operator<<(std::ostream &os,
                         const print_negative_option<O, true> &)
{
  return os << " --" << O::no_long_option_name;
}

template <typename O>
std::ostream &operator<<(std::ostream &os,
                         const print_negative_option<O, false> &)
{
  return os << " --no-" << O::long_option_name;
}

template <typename O, bool positive, bool has_long = has_long_option<O>::value,
          bool has_no =
            has_no_long_option<O>::value || has_no_long_option_name<O>::value,
          bool share_no = share_no_option<O>::value>
class print_long_option;

template <typename O, bool positive, bool has_no, bool share_no>
class print_long_option<O, positive, false, has_no, share_no> : public print_nop
{
};

template <typename O>
class print_long_option<O, true, true, true, true>
  : public print_shared_no_option<O>
{
};

template <typename O>
class print_long_option<O, true, true, true, false>
  : public print_positive_option<O>
{
};

template <typename O, bool share_no>
class print_long_option<O, true, true, false, share_no>
  : public print_positive_option<O>
{
};

template <typename O>
class print_long_option<O, false, true, true, false>
  : public print_negative_option<O>
{
};

template <typename O>
class print_long_option<O, false, true, true, true> : public print_nop
{
};

template <typename O, bool share_no>
class print_long_option<O, false, true, false, share_no> : public print_nop
{
};

template <typename O, bool positive, bool L = has_long_option<O>::value,
          bool S = has_short_option<O>::value,
          bool R = requires_value<O, positive>::value,
          bool A = accepts_value<O, positive>::value>
class print_value_name
{
};

template <typename O, bool p, bool S, bool A>
std::ostream &operator<<(std::ostream &os,
                         const print_value_name<O, p, true, S, true, A> &)
{
  return os << "=" << O::value_name();
}

template <typename O, bool p, bool S>
std::ostream &operator<<(std::ostream &os,
                         const print_value_name<O, p, true, S, false, true> &)
{
  return os << "[=" << O::value_name() << "]";
}

template <typename O, bool p, bool A>
std::ostream &operator<<(std::ostream &os,
                         const print_value_name<O, p, false, true, true, A> &)
{
  return os << " " << O::value_name();
}

template <typename O, bool p>
std::ostream &
operator<<(std::ostream &os,
           const print_value_name<O, p, false, true, false, true> &)
{
  return os << " " << O::value_name();
}

template <typename O, bool p, bool L, bool S>
std::ostream &operator<<(std::ostream &os,
                         const print_value_name<O, p, L, S, false, false> &)
{
  return os;
}

template <typename O, bool positive> class print_description
{
};

template <typename O>
std::ostream &operator<<(std::ostream &os, const print_description<O, true> &)
{
  return O::description(os);
}

template <typename O>
std::ostream &operator<<(std::ostream &os, const print_description<O, false> &)

{
  return O::no_description(os);
}

template <typename O, bool positive, bool shared_format,
          bool has_value_help = has_option_value_help<O, positive>::value,
          bool has_no_value_help = has_option_value_help<O, !positive>::value>
class print_value_help
{
};

// shared format

template <typename O, bool positive, bool no_value_help>
std::ostream &
operator<<(std::ostream &os,
           const print_value_help<O, positive, true, true, no_value_help> &)
{
  return O::option_value_help(os);
}

template <typename O, bool positive, bool no_value_help>
std::ostream &
operator<<(std::ostream &os,
           const print_value_help<O, positive, true, false, no_value_help> &)
{
  return os;
}

// separated format

template <typename O, bool positive>
std::ostream &
operator<<(std::ostream &os,
           const print_value_help<O, positive, false, false, false> &)
{
  /* No value help */
  return os;
}

template <typename O>
std::ostream &operator<<(std::ostream &os,
                         const print_value_help<O, true, false, true, true> &)
{
  /* Has separate help, print for positive */
  return O::option_value_help(os);
}

template <typename O>
std::ostream &operator<<(std::ostream &os,
                         const print_value_help<O, false, false, true, true> &)
{
  /* Has separate help, print for negative */
  return O::no_option_value_help(os);
}

template <typename O>
std::ostream &operator<<(std::ostream &os,
                         const print_value_help<O, true, false, true, false> &)
{
  /* Has common help, nothing print for positive */
  return os;
}

template <typename O>
std::ostream &operator<<(std::ostream &os,
                         const print_value_help<O, false, false, false, true> &)
{
  /* Has common help, print for negative */
  return O::option_value_help(os);
}

template <typename O, bool D = has_default_value<O>::value>
class print_default_value
{
};

template <typename O>
std::ostream &operator<<(std::ostream &os, const print_default_value<O, true> &)
{
  typename O::value_type value = O::default_value();
  return os << "(default: " << value_printer(value) << ")\n";
}

template <typename O>
std::ostream &operator<<(std::ostream &os,
                         const print_default_value<O, false> &)
{
  return os;
}

template <typename InIt>
static void print_description_wrapped(std::ostream &os, int indent, int width,
                                      InIt beg, InIt end, bool first = true)
{
  static constexpr decltype(*beg) nl = '\n';
  int tw = width - indent;
  InIt nlp = std::find(beg, end, nl);
  if (tw > 0 && nlp - beg > tw) {
    InIt wp = beg + tw;
    for (; wp > beg && *wp != ' '; --wp)
      ; // nop
    if (wp != beg)
      nlp = wp;
  }
  os << std::setw(first ? 0 : indent) << "" << std::string(beg, nlp) << "\n";
  if (nlp != end) {
    ++nlp;
    if (nlp != end)
      print_description_wrapped(os, indent, width, nlp, end, false);
  }
}

static void print_description_wrapped(std::ostream &os, int indent, int width,
                                      const std::string &str)
{
  print_description_wrapped(os, indent, width, str.cbegin(), str.cend());
}

template <typename O> class print_option_help_shared
{
};

template <typename O>
std::ostream &operator<<(std::ostream &os, const print_option_help_shared<O> &)
{
  std::string::size_type ist = 0;
  std::ostringstream ostr;
  ostr << "  " << print_short_option<O, true>() << print_long_option<O, true>()
       << print_value_name<O, true>();
  if (has_negative<O>::value && !share_no_option<O>::value) {
    const std::string &str = ostr.str();
    ist = str.length();
    os << str << "\n";
    ostr.str("");
    ostr << "  " << print_short_option<O, false>()
         << print_long_option<O, false>() << print_value_name<O, false>();
  }
  if (ist >= indent || ostr.str().length() >= indent) {
    os << ostr.str() << "\n" << std::setw(indent) << " ";
  } else {
    os << std::setw(indent) << std::left << ostr.str();
  }
  ostr.str("");
  ostr << print_description<O, true>();
  add_nl_if_not(ostr);
  ostr << print_value_help<O, true, true>();
  ostr << print_default_value<O>();
  print_description_wrapped(os, indent, jupiter_get_terminal_width(),
                            ostr.str());
  return os;
}

template <typename O> class print_option_help_separated
{
};

template <typename O>
std::ostream &operator<<(std::ostream &os,
                         const print_option_help_separated<O> &)
{
  static_assert(has_negative<O>::value, "Expected to have negative");
  static_assert(has_no_description<O>::value, "Expected to have no desc");

  std::ostringstream ostr;
  ostr << "  " << print_short_option<O, true>() << print_long_option<O, true>()
       << print_value_name<O, true>();
  if (ostr.str().length() >= indent) {
    os << ostr.str() << "\n" << std::setw(indent) << " ";
  } else {
    os << std::setw(indent) << std::left << ostr.str();
  }
  ostr.str("");
  ostr << print_description<O, true>();
  add_nl_if_not(ostr);
  ostr << print_value_help<O, true, false>();
  print_description_wrapped(os, indent, jupiter_get_terminal_width(),
                            ostr.str());

  ostr.str("");
  ostr << "    " << print_long_option<O, false>()
       << print_value_name<O, false>();
  if (ostr.str().length() >= indent) {
    os << ostr.str() << "\n" << std::setw(indent) << " ";
  } else {
    os << std::setw(indent) << std::left << ostr.str();
  }
  ostr.str("");
  ostr << print_description<O, false>();
  add_nl_if_not(ostr);
  ostr << print_value_help<O, false, false>();
  ostr << print_default_value<O>();
  print_description_wrapped(os, indent, jupiter_get_terminal_width(),
                            ostr.str());
  return os;
}

template <typename O, bool shared = !needs_separated<O>::value>
class print_option_help
{
};

template <typename O>
std::ostream &operator<<(std::ostream &os, const print_option_help<O, true> &)
{
  return os << print_option_help_shared<O>();
}

template <typename O>
std::ostream &operator<<(std::ostream &os, const print_option_help<O, false> &)
{
  return os << print_option_help_separated<O>();
}

template <typename... Os> class print_option_help_all
{
};

template <typename O, typename... Os>
std::ostream &operator<<(std::ostream &os,
                         const print_option_help_all<O, Os...> &)
{
  return os << print_option_help<O>() << print_option_help_all<Os...>();
}

inline std::ostream &operator<<(std::ostream &os,
                                const print_option_help_all<> &)
{
  return os;
}

template <typename enum_metadata,
          bool has_description = has_description<enum_metadata>::value>
class print_enum_help
{
};

template <typename T>
inline std::ostream &operator<<(std::ostream &os,
                                const print_enum_help<T, true> &)
{
  std::ostringstream ostr;
  ostr << " * " << T::name << ": ";
  T::description(ostr);
  add_nl_if_not(ostr);
  print_description_wrapped(os, 3, jupiter_get_terminal_width(), ostr.str());
  return os;
}

template <typename T>
inline std::ostream &operator<<(std::ostream &os,
                                const print_enum_help<T, false> &)
{
  return os << " * " << T::name << "\n";
}

template <typename enum_metadata_tuple> class enum_help_printer
{
};

static inline std::ostream &operator<<(std::ostream &os,
                                       const enum_help_printer<std::tuple<>> &)
{
  return os;
}

template <typename T, typename... Ts>
inline std::ostream &operator<<(std::ostream &os,
                                const enum_help_printer<std::tuple<T, Ts...>> &)
{
  return os << print_enum_help<T>() << enum_help_printer<std::tuple<Ts...>>();
}

} // namespace help_printer
} // namespace option_parser
} // namespace JUPITER

#endif
