/**
 * @file global.h
 *
 * Global storage or initialization of control library
 */

#ifndef JUPITER_CONTROL_GLOBAL_H
#define JUPITER_CONTROL_GLOBAL_H

#include "defs.h"

JUPITER_CONTROL_DECL_BEGIN

/**
 * @brief Initialize control module
 * @return 0 if success, 1 if failed with any error.
 */
JUPITER_CONTROL_DECL
int jcntrl_initialize(void);

/**
 * @brief Install new executive to make viable to create from
 * jcntrl_executive_new(id)
 * @param want_id ID number to be needed
 * @param cls executive class to be used for callbacks
 * @return Actually registered ID, JCNTRL_EXE_INVALID if too many classes
 * registered.
 *
 * Use `JCNTRL_EXE_USER` for adding user-defined executive.
 *
 * Dev note: Traversing cls->ancestor must reach
 * `jcntrl_executive_metadata_init()` (i.e., must inherit jcntrl_executive).
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_install(int want_id, const jcntrl_shared_object_data *cls);

#define jcntrl_executive_install(want_id, cls) \
  jcntrl_executive_install(want_id, JCNTRL_METADATA_INIT(cls)())

/**
 * @brief Get executive class from ID.
 * @param id ID to search
 *
 * NULL if not found, or invalid ID range.
 */
JUPITER_CONTROL_DECL
const jcntrl_shared_object_data *jcntrl_executive_get_from_id(int id);

JUPITER_CONTROL_DECL_END

#endif
