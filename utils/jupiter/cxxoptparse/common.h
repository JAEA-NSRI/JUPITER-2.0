/* This is -*- c++ -*- source */
/* This is vim: set ft=cpp: source */

#ifndef JUPITER_CXXOPTPARSE_COMMON_H
#define JUPITER_CXXOPTPARSE_COMMON_H

#include <vector>
#include <string>

namespace JUPITER
{
namespace option_parser
{
class help;
class help_value;

class stop;

namespace core
{
using string_type = std::string;
using argv_type = std::vector<string_type>;
using string_iter = string_type::const_iterator;
using argv_iter = argv_type::const_iterator;

template <typename... Options>
using data_type = std::tuple<typename Options::value_type...>;
} // namespace core

} // namespace option_parser
} // namespace JUPITER

#endif
