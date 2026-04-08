#include "defs.h"

#ifndef JUPITER_CONTROL_MANAGER_H
#define JUPITER_CONTROL_MANAGER_H

JUPITER_CONTROL_DECL_BEGIN

/**
 * @brief Create new executive manager
 * @return Newly created manager, NULL if allocation failed.
 */
JUPITER_CONTROL_DECL
jcntrl_executive_manager *jcntrl_executive_manager_new(void);

/**
 * @brief Delete manager and all executives belongs to it.
 * @param manager manager to delete
 */
JUPITER_CONTROL_DECL
void jcntrl_executive_manager_delete(jcntrl_executive_manager *manager);

/**
 * @brief Get executive from manager
 * @param manager manager to get from
 * @param name Executive name to look for
 * @return Executive or NULL if no executive is bound with given name.
 */
JUPITER_CONTROL_DECL
jcntrl_executive *
jcntrl_executive_manager_get(jcntrl_executive_manager *manager,
                             const char *name);

/**
 * @brief Take ownership of executive
 * @param manager manager to get from
 * @return Executive or NULL if no executive is bound with given name.
 *
 * Deletes the entry of executive from the manager, but not delete the
 * executive itself.
 */
JUPITER_CONTROL_DECL
jcntrl_executive *
jcntrl_executive_manager_disown(jcntrl_executive_manager *manager,
                                const char *name);

/**
 * @brief Add executive be managed by executive manager
 * @param manager manager to add to
 * @param executive executive to add
 * @return 1 if success, 0 if error (including already exists or reserved)
 *
 * The key name is taken from the name set to @p executive.
 *
 * This function returns error (1) when the name is reserved but not
 * bound. Use jcntrl_executive_manager_bind() instead.
 *
 * @sa jcntrl_executive_set_name(), jcntrl_executive_get_name()
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_manager_add(jcntrl_executive_manager *manager,
                                 jcntrl_executive *executive);

/**
 * @brief Remove and delete an excutive from manager
 * @param manager manager to delete from
 * @param name Name to be deleted
 * @retval 1 Executive is removed
 * @retval 0 No executive is bound to given name
 *
 * Entry will also be removed (makes jcntrl_executive_manager_has() to
 * return 'NULL').
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_manager_remove(jcntrl_executive_manager *manager,
                                    const char *name);

/**
 * @brief Reserve a name for future use
 * @param manager manager to reserve a name
 * @param name name to reserve
 * @return Reserved entry or NULL if allocation failed.
 *
 * Given name will be copied. The name must be NUL-terminated.
 */
JUPITER_CONTROL_DECL
jcntrl_executive_manager_entry *
jcntrl_executive_manager_reserve(jcntrl_executive_manager *manager,
                                 const char *name);

/**
 * @brief Test a name is reserved, bound or vacant
 * @param manager manager to reserve a name
 * @param name name to check
 * @return Reserved (or used) entry or NULL if available for new one.
 */
JUPITER_CONTROL_DECL
jcntrl_executive_manager_entry *
jcntrl_executive_manager_has(jcntrl_executive_manager *manager,
                             const char *name);

/**
 * @brief Bind an executive to reserved entry
 * @param entry Entry to bind
 * @param executive Executive to bind
 * @return 1 if successfully bound, 0 if given entry is already bound.
 *
 * This function sets the name of @p executive to the name set to @p
 * entry.
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_manager_bind(jcntrl_executive_manager_entry *entry,
                                  jcntrl_executive *executive);

/**
 * @brief Bind an executive to reserved entry
 * @param entry Entry to get
 * @return bound executive, NULL if not.
 */
JUPITER_CONTROL_DECL
jcntrl_executive *
jcntrl_executive_manager_entry_get(jcntrl_executive_manager_entry *entry);

/**
 * @brief Get executive name from entry
 * @param entry Entry to get
 * @return name of entry.
 */
JUPITER_CONTROL_DECL
const char *
jcntrl_executive_manager_entry_name(jcntrl_executive_manager_entry *entry);

/**
 * @brief Mark a manager entry.
 * @param entry Entry to set
 * @param mark value to set
 *
 * The value is not used in manager.
 */
JUPITER_CONTROL_DECL
void jcntrl_executive_manager_set_mark(jcntrl_executive_manager_entry *entry,
                                       int mark);

/**
 * @brief Get mark value
 * @param entry Entry to get
 * @return marked value
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_manager_mark(jcntrl_executive_manager_entry *entry);

/**
 * @brief Set mark value to all
 * @param manager Manager to set
 * @param mark mark value to set
 */
JUPITER_CONTROL_DECL
void jcntrl_executive_manager_set_all_marks(jcntrl_executive_manager *manager,
                                            int mark);

JUPITER_CONTROL_DECL_END

#endif
