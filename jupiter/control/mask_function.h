#ifndef JUPITER_CONTROL_MASK_FUNCTION_H
#define JUPITER_CONTROL_MASK_FUNCTION_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

/**
 * jcntrl_mask_function does not define any function.
 *
 * Override required function to implement.
 */
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_function);

JUPITER_CONTROL_DECL
jcntrl_mask_function *jcntrl_mask_function_downcast(jcntrl_shared_object *p);
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_mask_function_object(jcntrl_mask_function *func);

JUPITER_CONTROL_DECL
void jcntrl_mask_function_delete(jcntrl_mask_function *func);

JUPITER_CONTROL_DECL
int jcntrl_mask_function_eval(jcntrl_mask_function *func, //
                              jcntrl_cell *cell, int i, int j, int k);

JUPITER_CONTROL_DECL_END

#endif
