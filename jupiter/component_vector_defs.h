#ifndef JUPITER_COMPONENT_VECTOR_DEFS_H
#define JUPITER_COMPONENT_VECTOR_DEFS_H

#include "component_data_defs.h"
#include "general_vector_node.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief General array of components (not for user's input entry)
 *
 * See struct component_data for more info.
 */
struct component_vector
{
  struct component_data **const d;
  struct general_vector_node node;
};
typedef struct component_vector component_vector;

#ifdef __cplusplus
}
#endif

#endif
