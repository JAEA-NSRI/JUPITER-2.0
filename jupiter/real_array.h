#ifndef JUPITER_REAL_ARRAY_H
#define JUPITER_REAL_ARRAY_H

#include "common.h"
#include "control/defs.h"
#include "control/shared_object.h"
#include "struct.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Real array used for control library with `type` (in struct.h)
 *
 * jupiter_real_array inherits jcntrl_float_array or jcntrl_double_array on
 * `type` definition.
 */
struct jupiter_real_array;
typedef struct jupiter_real_array jupiter_real_array;

JUPITER_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jupiter_real_array);

JUPITER_DECL
jupiter_real_array *jupiter_real_array_new(void);
JUPITER_DECL
void jupiter_real_array_delete(jupiter_real_array *ary);

JUPITER_DECL
jcntrl_data_array *jupiter_real_array_data_array(jupiter_real_array *ary);
JUPITER_DECL
jcntrl_shared_object *jupiter_real_array_object(jupiter_real_array *ary);
JUPITER_DECL
jupiter_real_array *jupiter_real_array_downcast(jcntrl_shared_object *obj);

JUPITER_DECL
jupiter_real_array *jupiter_real_array_bind(jupiter_real_array *ary,
                                            const type *data,
                                            jcntrl_size_type n);

JUPITER_DECL
int jupiter_real_array_is_float(void);
JUPITER_DECL
int jupiter_real_array_is_double(void);

JUPITER_DECL
jcntrl_float_array *jupiter_real_array_as_float(jupiter_real_array *ary);
JUPITER_DECL
jcntrl_double_array *jupiter_real_array_as_double(jupiter_real_array *ary);

#ifdef __cplusplus
}
#endif

#endif
