#ifndef JUPITER_CONTROL_ABSTRACT_ARRAY_H
#define JUPITER_CONTROL_ABSTRACT_ARRAY_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_abstract_array);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_abstract_array_object(jcntrl_abstract_array *p);
JUPITER_CONTROL_DECL
jcntrl_abstract_array *jcntrl_abstract_array_downcast(jcntrl_shared_object *o);

JUPITER_CONTROL_DECL
jcntrl_size_type jcntrl_abstract_array_get_ntuple(jcntrl_abstract_array *p);
JUPITER_CONTROL_DECL
int jcntrl_abstract_array_resize(jcntrl_abstract_array *p, jcntrl_size_type n);

/**
 * Get array name.
 *
 * @warning Returning string is **not** NUL-terminated.
 */
JUPITER_CONTROL_DECL
const char *jcntrl_abstract_array_name(jcntrl_abstract_array *ary,
                                       jcntrl_size_type *len);

JUPITER_CONTROL_DECL
jcntrl_char_array *jcntrl_abstract_array_name_d(jcntrl_abstract_array *ary);

/**
 * Set name with null terminated string @p name
 */
JUPITER_CONTROL_DECL
int jcntrl_abstract_array_set_name(jcntrl_abstract_array *ary, const char *name,
                                   jcntrl_size_type len);

JUPITER_CONTROL_DECL
int jcntrl_abstract_array_set_name_d(jcntrl_abstract_array *ary,
                                     jcntrl_data_array *name);

JUPITER_CONTROL_DECL_END

#endif
