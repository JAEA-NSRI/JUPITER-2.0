#ifndef JUPITER_INLET_COMPONENT_DEFS_H
#define JUPITER_INLET_COMPONENT_DEFS_H

#include "common.h"
#include "component_info_defs.h"
#include "controllable_type.h"

JUPITER_DECL_START

/**
 * @brief Inlet data to be used with BOUNDARY data.
 */
struct inlet_component_element
{
  struct component_info_data comp; /*!< Material ID that inlet */
  controllable_type ratio;         /*!< Ratio of `comp_id` */
};

struct inlet_component_data
{
  int ncomp;                              /*!< Number of components */
  struct inlet_component_element array[]; /*!< Array of data */
};

JUPITER_DECL_END

#endif
