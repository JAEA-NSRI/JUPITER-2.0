#ifndef JUPITER_CONTROL_MASK_DATA_H
#define JUPITER_CONTROL_MASK_DATA_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_mask_data);

JUPITER_CONTROL_DECL
jcntrl_mask_data *jcntrl_mask_data_new(void);
JUPITER_CONTROL_DECL
void jcntrl_mask_data_delete(jcntrl_mask_data *mask);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_mask_data_object(jcntrl_mask_data *mask);
JUPITER_CONTROL_DECL
jcntrl_mask_data *jcntrl_mask_data_downcast(jcntrl_shared_object *object);

JUPITER_CONTROL_DECL
jcntrl_bool_array *jcntrl_mask_data_array(jcntrl_mask_data *mask);
JUPITER_CONTROL_DECL
void jcntrl_mask_data_set_array(jcntrl_mask_data *mask,
                                jcntrl_bool_array *array);

JUPITER_CONTROL_DECL_END

#endif
