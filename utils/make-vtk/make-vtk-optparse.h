/* This is -*- c++ -*- source. */
/* This is vim: set ft=cpp: source. */

#ifndef make_vtk_optparse_H
#define make_vtk_optparse_H

#ifndef __cplusplus
#error This header is only for C++.
#endif

#include <string>
#include <vector>

class MakeVTKOptions
{
public:
  std::string input_directory;
  std::string output_directory;
  bool fluid;
  bool particle;
  bool read_geom_dump;
  bool use_raw_name;
  bool include_ext_info;
  std::vector<int> numbers;

  MakeVTKOptions(const char *idir = "",
                 const char *odir = ".") :
    input_directory(idir), output_directory(odir),
    fluid(true), particle(true), read_geom_dump(false), use_raw_name(false) {
  }

  bool parse(int argc, char **argv);
};

#endif
