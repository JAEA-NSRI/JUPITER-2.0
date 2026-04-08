/* This is -*- c++ -*- source */
/* This is vim: set ft=cpp: source */

#ifndef JUPITER_CXXOPTPARSE_VALUE_PRINTER_H
#define JUPITER_CXXOPTPARSE_VALUE_PRINTER_H

#include <array>
#include <list>
#include <ostream>
#include <set>
#include <vector>

namespace JUPITER
{
namespace option_parser
{
template <typename T> class value_print;

template <typename T> struct value_print_wrapper
{
  const T &value;
};

template <typename T> value_print_wrapper<T> value_printer(const T &value)
{
  return value_print_wrapper<T>{value};
}

/**
 * `os << value_printer(value);`
 */
template <typename T>
std::ostream &operator<<(std::ostream &os, const value_print_wrapper<T> &t)
{
  return value_print<T>::print(os, t.value);
}

/**
 * @brief Value printer (used for printing default values)
 */
template <typename T> class value_print
{
public:
  static std::ostream &print(std::ostream &os, const T &value)
  {
    return os << value;
  }
};

template <> class value_print<std::string>
{
public:
  static std::ostream &print(std::ostream &os, const std::string &value)
  {
    return os << "\"" << value << "\"";
  }
};

template <> class value_print<bool>
{
public:
  static std::ostream &print(std::ostream &os, const bool &value)
  {
    return os << (value ? "yes" : "no");
  }
};

template <typename T, size_t N> class value_print<std::array<T, N>>
{
public:
  static std::ostream &print(std::ostream &os, const std::array<T, N> &value)
  {
    if (N > 0) {
      for (size_t i = 0; i < N; ++i) {
        if (i > 0)
          os << ",";
        value_print<T>::print(os, value[i]);
      }
    }
    return os;
  }
};

class value_print_list
{
public:
  template <typename L>
  static std::ostream &print(std::ostream &os, const L &list)
  {
    using T = typename L::value_type;
    if (list.empty()) {
      os << "{}";
    } else {
      bool first = true;
      for (auto &value : list) {
        if (!first)
          os << ",";
        value_print<T>::print(os, value);
        first = false;
      }
    }
    return os;
  }
};

template <typename T, typename A>
class value_print<std::vector<T, A>> : public value_print_list
{
};

template <typename T, typename A>
class value_print<std::list<T, A>> : public value_print_list
{
};

template <typename T, typename A>
class value_print<std::set<T, A>> : public value_print_list
{
};
} // namespace option_parser
} // namespace JUPITER

#endif
