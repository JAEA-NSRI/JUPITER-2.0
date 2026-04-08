#ifndef JUPITER_CONRTOL_MASK_OBJECT_H
#define JUPITER_CONRTOL_MASK_OBJECT_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_object);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_mask_object_object(jcntrl_mask_object *object);
JUPITER_CONTROL_DECL
jcntrl_data_object *jcntrl_mask_object_data(jcntrl_mask_object *object);
JUPITER_CONTROL_DECL
jcntrl_mask_object *jcntrl_mask_object_downcast(jcntrl_shared_object *object);

JUPITER_CONTROL_DECL_END

#endif
