
#ifndef JUPITER_CONTROL_FIELD_VARIABLE_H
#define JUPITER_CONTROL_FIELD_VARIABLE_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_field_variable);

JUPITER_CONTROL_DECL
jcntrl_field_variable *jcntrl_field_variable_new(void);

JUPITER_CONTROL_DECL
void jcntrl_field_variable_delete(jcntrl_field_variable *fvar);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_field_variable_object(jcntrl_field_variable *fvar);

JUPITER_CONTROL_DECL
jcntrl_field_variable *
jcntrl_field_variable_downcast(jcntrl_shared_object *object);

JUPITER_CONTROL_DECL
double jcntrl_field_variable_value(jcntrl_field_variable *fvar, int *error);
JUPITER_CONTROL_DECL
int jcntrl_field_variable_set_value(jcntrl_field_variable *fvar, double value);

JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_field_variable_array(jcntrl_field_variable *fvar);
JUPITER_CONTROL_DECL
int jcntrl_field_variable_set_array(jcntrl_field_variable *fvar,
                                    jcntrl_data_array *array);

JUPITER_CONTROL_DECL_END

#endif
