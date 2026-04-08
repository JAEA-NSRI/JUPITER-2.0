/* This is -*- c++ -*- source. */
/* This is vim: set ft=cpp: source. */

#include "make-vtk-optparse.h"

#include "jupiter/cxxoptparse/help.h"
#include "jupiter/cxxoptparse/help_printer.h"
#include "jupiter/cxxoptparse/optparse.h"
#include "jupiter/cxxoptparse/types.h"
#include "jupiter/os/os.h"

#include <cstring>
#include <iostream>
#include <new>

#include <ostream>
#include <stdexcept>
#include <string>

namespace options
{
using namespace JUPITER::option_parser;

class input_directory : public string_option<>
{
public:
  static constexpr char short_option_name = 'i';
  static constexpr auto &long_option_name = "input-directory";
  static std::string value_name() { return "NAME"; }
  static std::string default_value() { return ""; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "Input directory name";
  }
};

class output_directory : public string_option<>
{
public:
  static constexpr char short_option_name = 'o';
  static constexpr auto &long_option_name = "output-directory";
  static std::string value_name() { return "NAME"; }
  static std::string default_value() { return "."; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "Output directory name";
  }
};

class read_restart : public bool_option
{
public:
  static constexpr char short_option_name = '1';

  static std::ostream &description(std::ostream &os)
  {
    return os << "Convert restart data";
  }
};

class convert_fluid : public bool_option
{
public:
  static constexpr auto &long_option_name = "fluid";
  static bool default_value() { return true; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "(Don't) convert continium data";
  }
};

class convert_particle : public bool_option
{
public:
  static constexpr auto &long_option_name = "particle";
  static bool default_value() { return true; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "(Don't) convert particle data";
  }
};

class read_geom_dump : public bool_option
{
public:
  static constexpr auto &long_option_name = "read-geom-dump";
  static bool default_value() { return false; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "(Don't) read the output of Geom_dump";
  }
};

class use_data_name : public bool_option
{
public:
  static constexpr auto &long_option_name = "use-data-name";
  static bool default_value() { return false; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "Use raw data name for variable names, or use descriptive "
                 "variable names";
  }
};

class include_ext_info : public bool_option
{
public:
  static constexpr auto &long_option_name = "include-ext-info";
  static bool default_value() { return false; }

  static std::ostream &description(std::ostream &os)
  {
    return os << "Include raw data name and component index information as "
                 "custom information";
  }
};

class show_license : public bool_option
{
public:
  static constexpr auto &long_option_name = "license";
  static constexpr bool has_no_long_option = false;

  static std::ostream &description(std::ostream &os)
  {
    return os << "Show license quotes of VTK";
  }
};

template <typename... Options> class help_generator
{
public:
  using base = JUPITER::option_parser::help_generator<Options...>;

  class option;

  class value : public base::value
  {
  public:
    std::ostream &print_banner(std::ostream &os) const override
    {
      return os << "Usage: " << this->progname()
                << " -input params.txt -flags flags.txt -- [options...] [--] "
                   "[NUMBERS...]";
    }

    std::ostream &print_jupiter_option(std::ostream &os) const
    {
      return os << "JUPITER options:\n"
                   "  -input FILE            Parameter input file name\n"
                   "  -flags FILE            Flags input file name\n"
                   "  -geom FILE             Geometry input file name (if read "
                   "Geom_dump)\n"
                   "  -plist FILE            Ignored\n"
                   "  -control FILE          Control input file name (if read "
                   "Geom_dump)\n"
                   "  --                     Separator for JUPITER and "
                   "Converter\n\n";
    }

    std::ostream &print_postnote(std::ostream &os) const
    {
      help_printer::print_description_wrapped(
        os, 6, jupiter_get_terminal_width(),
        "BUG:  You'll see the error message which referenced field variables "
        "in your parameter input file are not defined. This is normal because "
        "the parameters required by `make-vtk` are NOT controllable by field "
        "variable. We are still improving this.");
      os << "\n";
      help_printer::print_description_wrapped(
        os, 6, jupiter_get_terminal_width(),
        "NOTE: JUPITER options starts with single hyphen and long options. "
        "Converter options starts with single hyphen and short options or two "
        "hyphens and long options.");
      os << "\n";
      return os;
    }

    std::ostream &print_option(std::ostream &os) const
    {
      return base::value::template print_options<option>(os);
    }

    std::ostream &print_help(std::ostream &os) const override
    {
      return print_postnote(
        print_option(print_jupiter_option(print_banner(os) << "\n\n"))
        << "\n\n");
    }
  };

  class option : public base::option
  {
  public:
    using value_type = value;
  };
};

template <typename... Options>
using help_option = typename help_generator<Options...>::option;

using parser = JUPITER::option_parser::parser_with_help<
  std::tuple<input_directory, output_directory, read_restart, convert_fluid,
             convert_particle, read_geom_dump, use_data_name, include_ext_info,
             show_license>,
  help_option>;
} // namespace options

static void show_license()
{
  std::cerr
    << "This program is linked against VTK (Visualization Toolkit).\n"
       "The license term of VTK is following:\n"
       "\n"
       "Copyright (c) 1993-2015 Ken Martin, Will Schroeder, Bill Lorensen\n"
       "All rights reserved.\n"
       "\n"
       "Redistribution and use in source and binary forms, with or without\n"
       "modification, are permitted provided that the following conditions are "
       "met:\n"
       "\n"
       " * Redistributions of source code must retain the above copyright "
       "notice,\n"
       "   this list of conditions and the following disclaimer.\n"
       "\n"
       " * Redistributions in binary form must reproduce the above copyright "
       "notice,\n"
       "   this list of conditions and the following disclaimer in the "
       "documentation\n"
       "   and/or other materials provided with the distribution.\n"
       "\n"
       " * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor "
       "the names\n"
       "   of any contributors may be used to endorse or promote products "
       "derived\n"
       "   from this software without specific prior written permission.\n"
       "\n"
       "THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "
       "``AS IS''\n"
       "AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, "
       "THE\n"
       "IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR "
       "PURPOSE\n"
       "ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE "
       "LIABLE FOR\n"
       "ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR "
       "CONSEQUENTIAL\n"
       "DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE "
       "GOODS OR\n"
       "SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) "
       "HOWEVER\n"
       "CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT "
       "LIABILITY,\n"
       "OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF "
       "THE USE\n"
       "OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.\n";
}

bool MakeVTKOptions::parse(int argc, char **argv)
{
  options::parser p;

  p.parse(argc, argv);
  if (p.get_help_value().get_help_printed())
    return false;
  if (p.get<options::show_license>()) {
    show_license();
    return false;
  }

  for (auto &it : p.argv) {
    char *s;
    int iout = std::strtol(it.c_str(), &s, 10);
    if (*s != '\0') {
      std::cerr << "Invalid Number \"" << it << "\"\n";
      continue;
    }

    numbers.push_back(iout);
  }

  if (p.get<options::read_restart>())
    numbers.push_back(-1);

  input_directory = p.get<options::input_directory>();
  output_directory = p.get<options::output_directory>();
  fluid = p.get<options::convert_fluid>();
  particle = p.get<options::convert_particle>();
  read_geom_dump = p.get<options::read_geom_dump>();
  use_raw_name = p.get<options::use_data_name>();
  include_ext_info = p.get<options::include_ext_info>();

  return true;
}
