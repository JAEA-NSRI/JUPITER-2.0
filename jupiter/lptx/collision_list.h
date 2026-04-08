#ifndef JUPITER_LPTX_COLLISION_LIST_H
#define JUPITER_LPTX_COLLISION_LIST_H

/*
 * This header file is public, but collision list is used in internal only.
 */

#include "defs.h"

JUPITER_LPTX_DECL_START

JUPITER_LPTX_DECL
LPTX_collision_list_set *LPTX_collision_list_set_new(LPTX_idtype num_entry);

JUPITER_LPTX_DECL
void LPTX_collision_list_set_delete(LPTX_collision_list_set *list);

JUPITER_LPTX_DECL
void LPTX_collision_list_set_delete_all(LPTX_param *param);

/**
 * @brief Update collision list
 * @param param Parameter to update
 * @param binary_only Makes list for binary collision only
 * @param cond Collision condition function
 * @param arg Argument for @p cond
 * @return Size of resulting list
 * @retval -1 Memory allocation failed
 *
 * @p cond will only called for same fluid cell index (icfpt, jcfpt and kcfpt).
 */
JUPITER_LPTX_DECL
LPTX_idtype LPTX_collision_list_update(LPTX_param *param, LPTX_bool binary_only,
                                       LPTX_cb_collision_if *cond, void *arg);

JUPITER_LPTX_DECL_END

#endif
