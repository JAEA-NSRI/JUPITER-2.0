/* This is -*- c++ -*- source */
/* This is vim: set ft=cpp: source */

#ifndef JUPITER_CXXOPTPARSE_HELP_H
#define JUPITER_CXXOPTPARSE_HELP_H

#include <jupiter/cxxoptparse/value_printer.h>
#include <jupiter/cxxoptparse/common.h>
#include <jupiter/cxxoptparse/help_printer.h>

#include <iostream>
#include <ostream>
#include <string>

namespace JUPITER
{
namespace option_parser
{
class help_value;
std::ostream &operator<<(std::ostream &os, const help_value &h);

/**
 * @brief Value data for handling printing help.
 */
class help_value
{
  bool help_printed = false;

public:
  bool want_help = false;

  /**
   * Called for printing help. Example implementation:
   *
   * ```c++
   * return print_options<...>(print_banner(os) << "\n");
   * ```
   */
  virtual std::ostream &print_help(std::ostream &os) const = 0;

  /**
   * Application name, used for printing (in default banner)
   */
  virtual std::string progname() const { return ""; }

  virtual std::ostream &print_banner(std::ostream &os) const
  {
    return os << "Usage: " << progname() << " [options...]";
  }

  /**
   * This does not include `help` option. Include it explicitly.
   */
  template <typename... Options>
  static std::ostream &print_options(std::ostream &os)
  {
    return os << "Options:\n"
              << help_printer::print_option_help_all<Options...>();
  }

  bool get_help_printed() const { return help_printed; }

  /**
   * Prints help to std::cerr, if not yet printed.
   */
  bool print_help_if_needed()
  {
    if (help_printed)
      return help_printed;

    help_printed = true;
    std::cerr << *this;
    return help_printed;
  }
};

/**
 * Sending to ostream always prints help.
 */
inline std::ostream &operator<<(std::ostream &os, const help_value &h)
{
  return h.print_help(os);
}

/**
 * @brief Base option data for `help` option
 */
class help
{
public:
  constexpr static auto &long_option_name = "help";
  constexpr static char short_option_name = 'h';
  constexpr static bool has_no_long_option = false;
  constexpr static bool require_value = false;
  /* help_value is abstract type. please override. */
  using value_type = help_value;

  static std::ostream &description(std::ostream &os)
  {
    return os << "Show this help";
  }

  static void parse_value(help_value &value, bool p)
  {
    value.want_help = p;
    /*
     * This is for printing after all parse process. To print immediately use
     * this in parse_value() of inherited class:
     *
     * ```c++
     * if (p)
     *   value.print_help_if_needed();
     * ```
     */
  }
};

template <typename... Options>
class help_generator
{
public:
  class option;

  class value : public help_value
  {
    std::string progname_value;

  public:
    std::string &progname() { return progname_value; }
    std::string progname() const override { return progname_value; }

    template <typename help_option = option>
    std::ostream &print_options(std::ostream &os) const
    {
      return help_value::print_options<Options..., help_option>(os);
    }

    std::ostream &print_help(std::ostream &os) const override
    {
      return print_options<>(print_banner(os) << "\n\n");
    }
  };

  class option : public help
  {
  public:
    using value_type = value;
  };
};

template <typename... Options>
using help_option = typename help_generator<Options...>::option;

} // namespace option_parser
} // namespace JUPITER

#endif
