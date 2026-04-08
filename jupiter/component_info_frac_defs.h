#ifndef JUPITER_COMPONENT_INFO_FRAC_DEFS_H
#define JUPITER_COMPONENT_INFO_FRAC_DEFS_H

#include "component_info_defs.h"
#include "general_vector_node.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef JUPITER_DOUBLE
typedef double component_info_frac_type;
#else
typedef float component_info_frac_type;
#endif

struct component_info_frac_data
{
  struct component_info_data comp;
  component_info_frac_type fraction;
};

struct component_info_frac
{
  struct component_info_frac_data *const d;
  struct general_vector_node node;
};

#ifdef __cplusplus
}
#endif

#endif
