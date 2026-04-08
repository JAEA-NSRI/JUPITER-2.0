/* This is -*- c++ -*- source */
/* This is vim: set ft=cpp: source */

#ifndef JUPITER_CXXOPTPARSE_STOP_H
#define JUPITER_CXXOPTPARSE_STOP_H

#include <string>

namespace JUPITER
{
namespace option_parser
{
/**
 * @brief Base option data for 'stop' option (`--` by default)
 *
 * value_type must be `bool` and default value must be `false`. Other properties
 * can be customizeable.
 */
class stop
{
public:
  constexpr static char short_option_name = '-';
  constexpr static bool allow_combine_short_option = false;
  constexpr static bool require_value = false;
  static std::string description() { return "Stop parse arguemnts"; }
  using value_type = bool;
  constexpr static bool default_value() { return false; }

  static void parse_value(bool &value, bool p)
  {
    value = p;
  }
};
}
}

#endif
