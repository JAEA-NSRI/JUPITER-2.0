#ifndef JUPITER_CONTROL_MASK_EXTENT_H
#define JUPITER_CONTROL_MASK_EXTENT_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_mask_extent;
typedef struct jcntrl_mask_extent jcntrl_mask_extent;

struct jcntrl_mask_extent_function;
typedef struct jcntrl_mask_extent_function jcntrl_mask_extent_function;

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_extent);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_extent_function);

JUPITER_CONTROL_DECL
int jcntrl_install_mask_extent(void);

JUPITER_CONTROL_DECL
jcntrl_mask_extent *jcntrl_mask_extent_new(void);
JUPITER_CONTROL_DECL
void jcntrl_mask_extent_delete(jcntrl_mask_extent *e);

JUPITER_CONTROL_DECL
jcntrl_executive *jcntrl_mask_extent_executive(jcntrl_mask_extent *e);
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_mask_extent_object(jcntrl_mask_extent *e);
JUPITER_CONTROL_DECL
jcntrl_mask_extent *jcntrl_mask_extent_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
const int *jcntrl_mask_extent_get_extent(jcntrl_mask_extent *extent);
JUPITER_CONTROL_DECL
void jcntrl_mask_extent_set_extent(jcntrl_mask_extent *extent,
                                   const int value[6]);

JUPITER_CONTROL_DECL
const int *
jcntrl_mask_extent_function_get_extent(jcntrl_mask_extent_function *extent);

JUPITER_CONTROL_DECL
void jcntrl_mask_extent_function_set_extent(jcntrl_mask_extent_function *extent,
                                            const int value[6]);

JUPITER_CONTROL_DECL_END

#endif
