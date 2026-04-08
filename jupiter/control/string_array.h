#ifndef JUPITER_CONTROL_STRING_ARRAY_H
#define JUPITER_CONTROL_STRING_ARRAY_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_string_array);

JUPITER_CONTROL_DECL
jcntrl_string_array *jcntrl_string_array_new(void);
JUPITER_CONTROL_DECL
void jcntrl_string_array_delete(jcntrl_string_array *p);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_string_array_object(jcntrl_string_array *p);
JUPITER_CONTROL_DECL
jcntrl_abstract_array *jcntrl_string_array_abstract(jcntrl_string_array *p);
JUPITER_CONTROL_DECL
jcntrl_string_array *jcntrl_string_array_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
jcntrl_size_type jcntrl_string_array_get_ntuple(jcntrl_string_array *p);
JUPITER_CONTROL_DECL
int jcntrl_string_array_resize(jcntrl_string_array *p, jcntrl_size_type ntuple);

/**
 * @note modification of returned array also modifies the content of array.
 *
 * To get a copy of array, use jcntrl_char_array_copy() to existing
 * jcntrl_char_array or jcntrl_string_array_get_copy() to create a new array.
 */
JUPITER_CONTROL_DECL
jcntrl_char_array *jcntrl_string_array_get(jcntrl_string_array *p,
                                           jcntrl_size_type index);

/**
 * Returns copied array of specified index
 */
JUPITER_CONTROL_DECL
jcntrl_char_array *jcntrl_string_array_get_copy(jcntrl_string_array *p,
                                                jcntrl_size_type index);

JUPITER_CONTROL_DECL
const char *jcntrl_string_array_get_cstr(jcntrl_string_array *p,
                                         jcntrl_size_type index,
                                         jcntrl_size_type *len);

/**
 * This function does not copy the contents of @p data. Instead, increases
 * reference counter (shallow copy) and take ownership of @p data. Modification
 * of @p data later also affects the content of array @p p.
 *
 * To make a copy or to set data from contents which requires copy (e.g.,
 * subarray or static array), use jcntrl_string_array_set_copy().
 *
 * To set from a C-style string, use jcntrl_string_array_set_cstr().
 */
JUPITER_CONTROL_DECL
int jcntrl_string_array_set(jcntrl_string_array *p, jcntrl_size_type index,
                            jcntrl_char_array *data);

JUPITER_CONTROL_DECL
int jcntrl_string_array_set_copy(jcntrl_string_array *p, jcntrl_size_type index,
                                 jcntrl_data_array *data);

JUPITER_CONTROL_DECL
int jcntrl_string_array_set_cstr(jcntrl_string_array *p, jcntrl_size_type index,
                                 const char *string, jcntrl_size_type len);

/**
 * This function copies string references (shares string data)
 */
JUPITER_CONTROL_DECL
int jcntrl_string_array_copy(jcntrl_string_array *dest,
                             jcntrl_string_array *src, jcntrl_size_type ntuple,
                             jcntrl_size_type idest, jcntrl_size_type iskip);

/**
 * This function copies string contents
 */
JUPITER_CONTROL_DECL
int jcntrl_string_array_deep_copy(jcntrl_string_array *dest,
                                  jcntrl_string_array *src,
                                  jcntrl_size_type ntuple,
                                  jcntrl_size_type idest,
                                  jcntrl_size_type iskip);

JUPITER_CONTROL_DECL_END

#endif
