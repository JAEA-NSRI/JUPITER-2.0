/* This is -*- c++ -*- source */
/* This is vim: set ft=cpp: source */

#ifndef JUPITER_CXXOPTPARSE_EXCEPTIONS_H
#define JUPITER_CXXOPTPARSE_EXCEPTIONS_H

#include <stdexcept>
#include <string>

namespace JUPITER
{
namespace option_parser
{
/**
 * @brief Common invalid option exception
 */
class invalid_option_error : public std::runtime_error
{
public:
  explicit invalid_option_error(const char *str) : std::runtime_error(str) {}

  explicit invalid_option_error(const std::string &str)
    : std::runtime_error(str)
  {
  }
};

/**
 * @brief Invalid short option exception
 */
class invalid_short_option : public invalid_option_error
{
  static std::string format(char v)
  {
    return std::string("Invalid option: -") + v;
  }

public:
  invalid_short_option(char v) : invalid_option_error(format(v)) {}
};

/**
 * @brief Invalid long option exception
 */
class invalid_long_option : public invalid_option_error
{
  template <typename It> static std::string format(It &&b, It &&e)
  {
    return std::string("Invalid option: --") + std::string(b, e);
  }

public:
  template <typename It>
  invalid_long_option(It &&b, It &&e) : invalid_option_error(format(b, e))
  {
  }
};

/**
 * @brief Excess value exception
 */
class excess_value_error : public invalid_option_error
{
  static std::string format(const std::string &arg)
  {
    return std::string("Excess value found for option: ") + arg;
  }

public:
  excess_value_error(const std::string &long_option)
    : invalid_option_error(format(long_option))
  {
  }

  excess_value_error(char short_option)
    : invalid_option_error(format(std::string("-") + short_option))
  {
  }
};

/**
 * @brief Missing value exception
 */
class missing_value_error : public invalid_option_error
{
  static std::string format(const std::string &arg)
  {
    return std::string("Missing value for option: ") + arg;
  }

public:
  missing_value_error(const std::string &long_option)
    : invalid_option_error(format(long_option))
  {
  }

  missing_value_error(char short_option)
    : invalid_option_error(format(std::string("-") + short_option))
  {
  }
};

/**
 * @brief Invalid value error exception
 */
class invalid_value_error : public invalid_option_error
{
  std::string format(const std::string &optname,
                     const std::string &value_or_what)
  {
    return std::string("Invalid value for option `") + optname +
           "`: " + value_or_what;
  }

  std::string format(const std::string &optname, const std::string &value,
                     const std::string &what)
  {
    return std::string("Invalid value for option `") + optname + "`: " + value +
           ": " + what;
  }

public:
  invalid_value_error(const std::string &optname, const std::string &what)
    : invalid_option_error(format(optname, what))
  {
  }

  invalid_value_error(const std::string &optname, const std::string &value,
                      const std::string &what)
    : invalid_option_error(format(optname, value, what))
  {
  }

  template <size_t L>
  invalid_value_error(const char (&optname)[L], const std::string &what)
    : invalid_value_error(std::string("--") + optname, what)
  {
  }

  template <size_t L>
  invalid_value_error(const char (&optname)[L], const std::string &value,
                      const std::string &what)
    : invalid_value_error(std::string("--") + optname, value, what)
  {
  }

  invalid_value_error(const char *optname, const std::string &what)
    : invalid_value_error(std::string("--") + optname, what)
  {
  }

  invalid_value_error(const char *optname, const std::string &value,
                      const std::string &what)
    : invalid_value_error(std::string("--") + optname, value, what)
  {
  }

  invalid_value_error(const char s, const std::string &what)
    : invalid_value_error(std::string("-") + s, what)
  {
  }

  invalid_value_error(const char s, const std::string &value,
                      const std::string &what)
    : invalid_value_error(std::string("-") + s, what)
  {
  }
};

} // namespace option_parser
} // namespace JUPITER

#endif
