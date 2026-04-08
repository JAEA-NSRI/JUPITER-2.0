/* This is -*- c++ -*- source */
/* This is vim: set ft=cpp: source */

#ifndef JUPITER_CXXOPTPARSE_PARSERS_H
#define JUPITER_CXXOPTPARSE_PARSERS_H

/* Type-specific predefined value string parsers */

#include <cmath>
#include <cstddef>
#include <limits>
#include <stdexcept>
#include <string>
#include <type_traits>

namespace JUPITER
{
namespace option_parser
{
/**
 * String parser. Just construct text as std::string().
 */
class string_parser
{
public:
  template <typename string_type, typename InIt>
  void operator()(string_type &value, bool p, InIt beg, InIt end) const
  {
    value = string_type(beg, end);
  }
};

/**
 * Signed integer type parser (note: `value_type` is not validated)
 */
class sint_parser
{
public:
  template <typename value_type, typename InIt>
  void operator()(value_type &value, bool p, InIt beg, InIt end,
                  value_type min = std::numeric_limits<value_type>::lowest(),
                  value_type max = std::numeric_limits<value_type>::max(),
                  int base = 10) const
  {
    size_t idx;
    long long llval;

    try {
      llval = std::stoll(std::string(beg, end), &idx, base);
    } catch (const std::invalid_argument &) {
      throw std::runtime_error("Invalid value");
    } catch (const std::out_of_range &) {
      throw std::runtime_error("Out of domain");
    }
    if (idx != end - beg)
      throw std::runtime_error("Invalid value");
    if (llval < min || llval > max)
      throw std::runtime_error("Out of domain");
    value = llval;
  }
};

/**
 * Unsigned integer type parser (note: `value_type` is not validated)
 */
class uint_parser
{
public:
  template <typename value_type, typename InIt>
  void operator()(value_type &value, bool p, InIt beg, InIt end,
                  value_type min = std::numeric_limits<value_type>::lowest(),
                  value_type max = std::numeric_limits<value_type>::max(),
                  int base = 10) const
  {
    size_t idx;
    unsigned long long llval;

    try {
      llval = std::stoull(std::string(beg, end), &idx, base);
    } catch (const std::invalid_argument &) {
      throw std::runtime_error("Invalid value");
    } catch (const std::out_of_range &) {
      throw std::runtime_error("Out of domain");
    }
    if (idx != end - beg)
      throw std::runtime_error("Invalid value");
    if (llval < min || llval > max)
      throw std::runtime_error("Out of domain");
    value = llval;
  }
};

/**
 * auto-detect signess for value_type
 */
class int_parser
{
public:
  template <typename value_type, typename InIt>
  typename std::enable_if<std::numeric_limits<value_type>::is_signed>::type
  operator()(value_type &value, bool p, InIt beg, InIt end, int base = 10,
             value_type min = std::numeric_limits<value_type>::lowest(),
             value_type max = std::numeric_limits<value_type>::max()) const
  {
    sint_parser{}(value, p, beg, end, min, max, base);
  }

  template <typename value_type, typename InIt>
  typename std::enable_if<!std::numeric_limits<value_type>::is_signed>::type
  operator()(value_type &value, bool p, InIt beg, InIt end, int base = 10,
             value_type min = std::numeric_limits<value_type>::lowest(),
             value_type max = std::numeric_limits<value_type>::max()) const
  {
    uint_parser{}(value, p, beg, end, min, max, base);
  }
};

/**
 * Real (float, double, long double) parser
 */
class real_parser
{
  template <typename value_type>
  typename std::enable_if<
    std::numeric_limits<value_type>::has_quiet_NaN ||
    std::numeric_limits<value_type>::has_signaling_NaN>::type
  check_nan(value_type value, bool allow_nan, int) const
  {
    if (!std::isnan(value))
      return;
    if (allow_nan)
      return;
    throw std::runtime_error("NaN is not allowed");
  }

  template <typename value_type>
  void check_nan(value_type value, bool allow_nan, long) const
  {
    // nop
  }

  template <typename value_type>
  typename std::enable_if<std::numeric_limits<value_type>::has_infinity>::type
  check_inf(value_type value, bool allow_positive, bool allow_negative,
            int) const
  {
    if (!std::isinf(value))
      return;

    if (value > static_cast<value_type>(0.0)) {
      if (allow_positive)
        return;
      throw std::runtime_error("Positive infinity is not allowed");
    } else {
      if (allow_negative)
        return;
      throw std::runtime_error("Negative infinity is not allowed");
    }
  }

  template <typename F, typename value_type, typename InIt>
  void base(value_type &value, InIt beg, InIt end, value_type min,
            value_type max, bool include_min, bool include_max,
            bool allow_positive_inf, bool allow_negative_inf,
            bool allow_nan) const
  {
    size_t idx;

    try {
      F{}(value, &idx, std::string(beg, end));
    } catch (const std::invalid_argument &) {
      throw std::runtime_error("Invalid value");
    } catch (const std::out_of_range &) {
      throw std::runtime_error("Value out of range");
    }

    if (idx != end - beg)
      throw std::runtime_error("Invalid value");

    check_nan(value, allow_nan, 1);
    check_inf(value, allow_positive_inf, allow_negative_inf, 1);

    if ((include_min ? min <= value : min < value) &&
        (include_max ? value <= max : value < max))
      return;
    throw std::runtime_error("Value out of range");
  }

  struct stof
  {
    void operator()(float &value, size_t *idx, const std::string &str) const
    {
      value = std::stof(str, idx);
    }
  };

  struct stod
  {
    void operator()(double &value, size_t *idx, const std::string &str) const
    {
      value = std::stod(str, idx);
    }
  };

  struct stold
  {
    void operator()(long double &value, size_t *idx,
                    const std::string &str) const
    {
      value = std::stold(str, idx);
    }
  };

public:
  template <typename InIt>
  void operator()(float &value, bool p, InIt beg, InIt end,
                  float min = std::numeric_limits<float>::lowest(),
                  float max = std::numeric_limits<float>::max(),
                  bool include_min = true, bool include_max = true,
                  bool allow_positive_inf = true,
                  bool allow_negative_inf = true, bool allow_nan = true) const
  {
    base<stof>(value, beg, end, min, max, include_min, include_max,
               allow_positive_inf, allow_negative_inf, allow_nan);
  }

  template <typename InIt>
  void operator()(double &value, bool p, InIt beg, InIt end,
                  double min = std::numeric_limits<double>::lowest(),
                  double max = std::numeric_limits<double>::max(),
                  bool include_min = true, bool include_max = true,
                  bool allow_positive_inf = true,
                  bool allow_negative_inf = true, bool allow_nan = true) const
  {
    base<stod>(value, beg, end, min, max, include_min, include_max,
               allow_positive_inf, allow_negative_inf, allow_nan);
  }

  template <typename InIt>
  void operator()(long double &value, bool p, InIt beg, InIt end,
                  long double min = std::numeric_limits<long double>::lowest(),
                  long double max = std::numeric_limits<long double>::max(),
                  bool include_min = true, bool include_max = true,
                  bool allow_positive_inf = true,
                  bool allow_negative_inf = true, bool allow_nan = true) const
  {
    base<stold>(value, beg, end, min, max, include_min, include_max,
                allow_positive_inf, allow_negative_inf, allow_nan);
  }
};


} // namespace option_parser
} // namespace JUPITER

#endif
