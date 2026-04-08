#ifndef JUPITER_CONTROL_DATA_OBJECT_H
#define JUPITER_CONTROL_DATA_OBJECT_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_data_object);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_data_object_object(jcntrl_data_object *obj);
JUPITER_CONTROL_DECL
jcntrl_data_object *jcntrl_data_object_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL_END

#endif
