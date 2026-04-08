#ifndef JUPITER_COMPONENT_INFO_DEFS_H
#define JUPITER_COMPONENT_INFO_DEFS_H

#include "component_data_defs.h"
#include "general_vector_node.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Data of general array of component IDs
 *
 * @p d->comp_index shall be used for index on component-based arrays, such as
 * fs, fl and Y.
 *
 * @p id exists for fill @p d later (as value of @p d->jupiter_id).
 */
struct component_info_data
{
  struct component_data *d;
  int id; /* for d is NULL */
};

/**
 * @brief General array of component ids
 *
 * `component_info_getc(info, index)->comp_index` shall be used for index on
 * component-based arrays, such as fs, fl and Y. See struct component_data for
 * more info.
 */
struct component_info
{
  struct component_info_data *const d;
  struct general_vector_node node;
};
typedef struct component_info component_info;

#ifdef __cplusplus
}
#endif

#endif
