#ifndef JUPITER_CONTROLLABLE_TYPE_H
#define JUPITER_CONTROLLABLE_TYPE_H

#include "control/defs.h"
#include "geometry/list.h"

#ifdef __cplusplus
extern "C" {
#endif

/* YSE: Add dynamic parameter with field variable */
/**
 * @brief Controllable type
 *
 * Use this type instead of regular `type` to make a variable
 * controllable by a field variable.
 */
struct controllable_type {
  double current_value;     ///< Current value
  jcntrl_executive_manager_entry *exec;
                          ///< Controlling field variable executive
                          ///  (NULL means not controlled)
  struct geom_list list;  ///< Link with another controlled controls
                          ///  to iterate over. Non-controlled
                          ///  (aka. constant entries) are not in
                          ///  member of this list.
};
typedef struct controllable_type controllable_type;
#define controllable_type_entry(ptr) \
  geom_list_entry(ptr, struct controllable_type, list)

/**
 * @brief Controllable entry for geometry data
 */
struct controllable_geometry_entry {
  controllable_type control[3]; ///< Control data.
  geom_size_type index;         ///< Index of the parameter to set
  geom_variant_type type;       ///< Type of assignment data
                                ///  (DOUBLE, VECTOR2 or VECTOR3)
  struct geom_list list;    ///< Link with another controllable_geometry_entry
};
typedef struct controllable_geometry_entry controllable_geometry_entry;
#define controllable_geometry_list_entry(ptr) \
  geom_list_entry(ptr, struct controllable_geometry_entry, list)


#ifdef __cplusplus
}
#endif

#endif
