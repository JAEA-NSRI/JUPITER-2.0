#ifndef JUPITER_CONTROL_SUBARRAY_H
#define JUPITER_CONTROL_SUBARRAY_H

#include "defs.h"
#include "data_array_data.h"
#include "shared_object.h"
#include "shared_object_priv.h"

JUPITER_CONTROL_DECL_BEGIN

/**
 * Represents subarray (aka. array view)
 *
 * Datatype of subarray inherits @p source.
 */
struct jcntrl_data_subarray
{
  jcntrl_data_array data;
  jcntrl_data_array *const source;
  jcntrl_size_type offset;
  jcntrl_size_type viewsize;
};
#define jcntrl_data_subarray__ancestor jcntrl_data_array
#define jcntrl_data_subarray__dnmem data.jcntrl_data_array__dnmem
JCNTRL_VTABLE_NONE(jcntrl_data_subarray);

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_data_subarray);

/**
 * @return 1 success, 0 failed (view out-of-range)
 *
 * @note This function increment reference count of @p source.
 */
JUPITER_CONTROL_DECL
int jcntrl_data_subarray_static_init(jcntrl_data_subarray *ary,
                                     jcntrl_data_array *source,
                                     jcntrl_size_type offset,
                                     jcntrl_size_type viewsize);

JUPITER_CONTROL_DECL
jcntrl_data_subarray *jcntrl_data_subarray_new(jcntrl_data_array *source,
                                               jcntrl_size_type offset,
                                               jcntrl_size_type viewsize);

JUPITER_CONTROL_DECL
void jcntrl_data_subarray_delete(jcntrl_data_subarray *ary);

/**
 * This function is upcast. Use jcntrl_data_subarray_source() to obtain
 * the source array.
 */
JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_data_subarray_data(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_data_subarray_object(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
jcntrl_data_subarray *jcntrl_data_subarray_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_data_subarray_source(jcntrl_data_subarray *ary);

JUPITER_CONTROL_DECL
const void *jcntrl_data_subarray_get(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
void *jcntrl_data_subarray_get_writable(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
jcntrl_size_type jcntrl_data_subarray_get_ntuple(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
jcntrl_size_type jcntrl_data_subarray_element_size(jcntrl_data_subarray *ary);

JUPITER_CONTROL_DECL
double jcntrl_data_subarray_get_value(jcntrl_data_subarray *ary,
                                      jcntrl_size_type index);

JUPITER_CONTROL_DECL
const jcntrl_shared_object_data *
jcntrl_data_subarray_element_type(jcntrl_data_subarray *ary);

JUPITER_CONTROL_DECL
int jcntrl_data_subarray_set_view(jcntrl_data_subarray *ary,
                                  jcntrl_size_type offset,
                                  jcntrl_size_type viewsize);

JUPITER_CONTROL_DECL
const char *jcntrl_data_subarray_get_char(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
const char *jcntrl_data_subarray_get_bool(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
const int *jcntrl_data_subarray_get_int(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
const jcntrl_aint_type *
jcntrl_data_subarray_get_aint(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
const float *jcntrl_data_subarray_get_float(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
const double *jcntrl_data_subarray_get_double(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
const jcntrl_size_type *
jcntrl_data_subarray_get_size(jcntrl_data_subarray *ary);

JUPITER_CONTROL_DECL
char *jcntrl_data_subarray_get_writable_char(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
char *jcntrl_data_subarray_get_writable_bool(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
int *jcntrl_data_subarray_get_writable_int(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
jcntrl_aint_type *
jcntrl_data_subarray_get_writable_aint(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
float *jcntrl_data_subarray_get_writable_float(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
double *jcntrl_data_subarray_get_writable_double(jcntrl_data_subarray *ary);
JUPITER_CONTROL_DECL
jcntrl_size_type *
jcntrl_data_subarray_get_writable_size(jcntrl_data_subarray *ary);

JUPITER_CONTROL_DECL_END

#endif
