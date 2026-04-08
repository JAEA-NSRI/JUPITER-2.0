
#ifndef JUPITER_CONTROL_DATA_ARRAY_H
#define JUPITER_CONTROL_DATA_ARRAY_H

#include "defs.h"
#include "shared_object.h"

#include <jupiter/geometry/vector.h>

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_data_array);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_generic_data_array);

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_char_array);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_bool_array);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_int_array);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_aint_array);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_float_array);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_double_array);
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_size_array);

/**
 * @brief Create a new data array with specified type.
 * @return Pointer to new data array, NULL if allocation failed
 */
JUPITER_CONTROL_DECL
jcntrl_char_array *jcntrl_char_array_new(void);
JUPITER_CONTROL_DECL
jcntrl_bool_array *jcntrl_bool_array_new(void);
JUPITER_CONTROL_DECL
jcntrl_int_array *jcntrl_int_array_new(void);
JUPITER_CONTROL_DECL
jcntrl_aint_array *jcntrl_aint_array_new(void);
JUPITER_CONTROL_DECL
jcntrl_float_array *jcntrl_float_array_new(void);
JUPITER_CONTROL_DECL
jcntrl_double_array *jcntrl_double_array_new(void);
JUPITER_CONTROL_DECL
jcntrl_size_array *jcntrl_size_array_new(void);

/**
 * Duplicate the array @p inp.
 *
 * @note the returing array will be the type of
 * jcntrl_data_array_element_type(inp), not the type of class itself,
 * jcntrl_shared_object_class(jcntrl_data_array_object(inp)). So, for example,
 * if you duplicate jcntrl_static_char_array, you will get jcntrl_char_array
 * instead, and, if you duplicate jcntrl_data_subarray, you will get the array
 * of source type.
 */
JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_data_array_dup(jcntrl_data_array *inp);

/**
 * @return The size of array element
 */
JUPITER_CONTROL_DECL
jcntrl_size_type jcntrl_data_array_element_size(jcntrl_data_array *ary);

/**
 * @return The type of array element
 *
 * E.g., This function return jcntrl_int_array_metadata_init() (or may be
 * subclassing it) for array of `int`. The classes that
 * jcntrl_char_array_metadat_init() and jcntrl_bool_array_metadata_init() are
 * both array of `char`, but they are considered incompatible.
 */
JUPITER_CONTROL_DECL
const jcntrl_shared_object_data *
jcntrl_data_array_element_type(jcntrl_data_array *ary);

/**
 * @brief Create a new data ary bound to specified raw array data
 * @param type Type to be used with
 * @param data Raw array data
 * @param ntuple Number of elements of @p data
 * @return Pointer to new data array, NULL if allocation failed
 *
 * Given @p data will be shared with returning data array
 */
JUPITER_CONTROL_DECL
jcntrl_char_array *jcntrl_char_array_for(const char *data,
                                         jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_bool_array *jcntrl_bool_array_for(const char *data,
                                         jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_int_array *jcntrl_int_array_for(const int *data,
                                       jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_aint_array *jcntrl_aint_array_for(const jcntrl_aint_type *data,
                                         jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_float_array *jcntrl_float_array_for(const float *data,
                                           jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_double_array *jcntrl_double_array_for(const double *data,
                                             jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_size_array *jcntrl_size_array_for(const jcntrl_size_type *data,
                                         jcntrl_size_type ntuple);

/**
 * @brief Bind specified raw array data to existing data array
 * @param type The type (e.g. metadata of jcntrl_char_array) of @p data
 *
 * This function checks type by @p type. Use with carefully.
 */
JUPITER_CONTROL_DECL
jcntrl_generic_data_array *
jcntrl_generic_data_array_bind(jcntrl_generic_data_array *ary, const void *data,
                               const jcntrl_shared_object_data *type,
                               jcntrl_size_type ntuple);

/**
 * @brief Bind specified raw array data to existing data array
 * @param ary Array data to be bound
 * @param data Raw binary data
 * @param size Bytesize of @p data
 * @return @p ary, or NULL if type does not match
 */
JUPITER_CONTROL_DECL
jcntrl_char_array *jcntrl_char_array_bind(jcntrl_char_array *ary,
                                          const char *data,
                                          jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_bool_array *jcntrl_bool_array_bind(jcntrl_bool_array *ary,
                                          const char *data,
                                          jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_int_array *jcntrl_int_array_bind(jcntrl_int_array *ary, const int *data,
                                        jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_aint_array *jcntrl_aint_array_bind(jcntrl_aint_array *ary,
                                          const jcntrl_aint_type *data,
                                          jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_float_array *jcntrl_float_array_bind(jcntrl_float_array *ary,
                                            const float *data,
                                            jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_double_array *jcntrl_double_array_bind(jcntrl_double_array *ary,
                                              const double *data,
                                              jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_size_array *jcntrl_size_array_bind(jcntrl_size_array *ary,
                                          const jcntrl_size_type *data,
                                          jcntrl_size_type ntuple);

/**
 * @brief Take ownership of array
 * @param ary array to take ownership
 */
JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_data_array_take_ownership(jcntrl_data_array *ary);

JUPITER_CONTROL_DECL
jcntrl_char_array *jcntrl_char_array_take_ownership(jcntrl_char_array *ary);
JUPITER_CONTROL_DECL
jcntrl_bool_array *jcntrl_bool_array_take_ownership(jcntrl_bool_array *ary);
JUPITER_CONTROL_DECL
jcntrl_int_array *jcntrl_int_array_take_ownership(jcntrl_int_array *ary);
JUPITER_CONTROL_DECL
jcntrl_aint_array *jcntrl_aint_array_take_ownership(jcntrl_aint_array *ary);
JUPITER_CONTROL_DECL
jcntrl_float_array *jcntrl_float_array_take_ownership(jcntrl_float_array *ary);
JUPITER_CONTROL_DECL
jcntrl_double_array *
jcntrl_double_array_take_ownership(jcntrl_double_array *ary);
JUPITER_CONTROL_DECL
jcntrl_size_array *jcntrl_size_array_take_ownership(jcntrl_size_array *ary);

JUPITER_CONTROL_DECL
void jcntrl_data_array_delete(jcntrl_data_array *ary);

JUPITER_CONTROL_DECL
void jcntrl_char_array_delete(jcntrl_char_array *ary);
JUPITER_CONTROL_DECL
void jcntrl_bool_array_delete(jcntrl_bool_array *ary);
JUPITER_CONTROL_DECL
void jcntrl_int_array_delete(jcntrl_int_array *ary);
JUPITER_CONTROL_DECL
void jcntrl_aint_array_delete(jcntrl_aint_array *ary);
JUPITER_CONTROL_DECL
void jcntrl_float_array_delete(jcntrl_float_array *ary);
JUPITER_CONTROL_DECL
void jcntrl_double_array_delete(jcntrl_double_array *ary);
JUPITER_CONTROL_DECL
void jcntrl_size_array_delete(jcntrl_size_array *ary);

JUPITER_CONTROL_DECL
jcntrl_abstract_array *jcntrl_data_array_abstract(jcntrl_data_array *ary);

JUPITER_CONTROL_DECL
jcntrl_data_array *
jcntrl_generic_data_array_data(jcntrl_generic_data_array *ary);

JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_char_array_data(jcntrl_char_array *ary);
JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_bool_array_data(jcntrl_bool_array *ary);
JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_int_array_data(jcntrl_int_array *ary);
JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_aint_array_data(jcntrl_aint_array *ary);
JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_float_array_data(jcntrl_float_array *ary);
JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_double_array_data(jcntrl_double_array *ary);
JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_size_array_data(jcntrl_size_array *ary);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_data_array_object(jcntrl_data_array *ary);

JUPITER_CONTROL_DECL
jcntrl_shared_object *
jcntrl_generic_data_array_object(jcntrl_generic_data_array *ary);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_char_array_object(jcntrl_char_array *ary);
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_bool_array_object(jcntrl_bool_array *ary);
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_int_array_object(jcntrl_int_array *ary);
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_aint_array_object(jcntrl_aint_array *ary);
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_float_array_object(jcntrl_float_array *ary);
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_double_array_object(jcntrl_double_array *ary);
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_size_array_object(jcntrl_size_array *ary);

JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_data_array_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
jcntrl_generic_data_array *
jcntrl_generic_data_array_downcast(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL
jcntrl_char_array *jcntrl_char_array_downcast(jcntrl_shared_object *obj);
JUPITER_CONTROL_DECL
jcntrl_bool_array *jcntrl_bool_array_downcast(jcntrl_shared_object *obj);
JUPITER_CONTROL_DECL
jcntrl_int_array *jcntrl_int_array_downcast(jcntrl_shared_object *obj);
JUPITER_CONTROL_DECL
jcntrl_aint_array *jcntrl_aint_array_downcast(jcntrl_shared_object *obj);
JUPITER_CONTROL_DECL
jcntrl_float_array *jcntrl_float_array_downcast(jcntrl_shared_object *obj);
JUPITER_CONTROL_DECL
jcntrl_double_array *jcntrl_double_array_downcast(jcntrl_shared_object *obj);
JUPITER_CONTROL_DECL
jcntrl_size_array *jcntrl_size_array_downcast(jcntrl_shared_object *obj);

/**
 * @brief Resize array (can shrink)
 * @param ary array to get from
 * @param ntuple New number of tuples
 * @retval ary Success
 * @retval NULL if not supported
 * @retval NULL if bytesize overflowed
 * @retval NULL if allocation failed
 */
JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_data_array_resize(jcntrl_data_array *ary,
                                            jcntrl_size_type ntuple);

/**
 * @brief Resize array (can shrink)
 * @param ary array to get from
 * @param ntuple New number of tuples
 * @return @p ary itself, or NULL if allocation failed or bytesize for
 *         @p ntuple is too large (overflow)
 */
JUPITER_CONTROL_DECL
jcntrl_generic_data_array *
jcntrl_generic_data_array_resize(jcntrl_generic_data_array *ary,
                                 jcntrl_size_type ntuple);

JUPITER_CONTROL_DECL
jcntrl_char_array *jcntrl_char_array_resize(jcntrl_char_array *ary,
                                            jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_bool_array *jcntrl_bool_array_resize(jcntrl_bool_array *ary,
                                            jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_int_array *jcntrl_int_array_resize(jcntrl_int_array *ary,
                                          jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_aint_array *jcntrl_aint_array_resize(jcntrl_aint_array *ary,
                                            jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_float_array *jcntrl_float_array_resize(jcntrl_float_array *ary,
                                              jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_double_array *jcntrl_double_array_resize(jcntrl_double_array *ary,
                                                jcntrl_size_type ntuple);
JUPITER_CONTROL_DECL
jcntrl_size_array *jcntrl_size_array_resize(jcntrl_size_array *ary,
                                            jcntrl_size_type ntuple);

/**
 * Get array name.
 *
 * @warning Returning string is **not** NUL-terminated.
 */
JUPITER_CONTROL_DECL
const char *jcntrl_data_array_name(jcntrl_data_array *ary,
                                   jcntrl_size_type *len);

JUPITER_CONTROL_DECL
jcntrl_char_array *jcntrl_data_array_name_d(jcntrl_data_array *ary);

/**
 * Set name with null terminated string @p name
 */
JUPITER_CONTROL_DECL
int jcntrl_data_array_set_name(jcntrl_data_array *ary, const char *name,
                               jcntrl_size_type len);

JUPITER_CONTROL_DECL
int jcntrl_data_array_set_name_d(jcntrl_data_array *ary,
                                 jcntrl_data_array *name);

JUPITER_CONTROL_DECL
jcntrl_size_type jcntrl_data_array_get_ntuple(jcntrl_data_array *ary);

JUPITER_CONTROL_DECL
jcntrl_size_type jcntrl_char_array_get_ntuple(jcntrl_char_array *ary);
JUPITER_CONTROL_DECL
jcntrl_size_type jcntrl_bool_array_get_ntuple(jcntrl_bool_array *ary);
JUPITER_CONTROL_DECL
jcntrl_size_type jcntrl_int_array_get_ntuple(jcntrl_int_array *ary);
JUPITER_CONTROL_DECL
jcntrl_size_type jcntrl_aint_array_get_ntuple(jcntrl_aint_array *ary);
JUPITER_CONTROL_DECL
jcntrl_size_type jcntrl_float_array_get_ntuple(jcntrl_float_array *ary);
JUPITER_CONTROL_DECL
jcntrl_size_type jcntrl_double_array_get_ntuple(jcntrl_double_array *ary);
JUPITER_CONTROL_DECL
jcntrl_size_type jcntrl_size_array_get_ntuple(jcntrl_size_array *ary);

/**
 * Get raw pointer
 */
JUPITER_CONTROL_DECL
const void *jcntrl_data_array_get(jcntrl_data_array *ary);

/**
 * Get raw pointer, but returns NULL if element type is not @p meta.
 */
JUPITER_CONTROL_DECL
const void *
jcntrl_data_array_get_by_meta(jcntrl_data_array *ary,
                              const jcntrl_shared_object_data *meta);

JUPITER_CONTROL_DECL
const char *jcntrl_data_array_get_char(jcntrl_data_array *ary);
JUPITER_CONTROL_DECL
const char *jcntrl_data_array_get_bool(jcntrl_data_array *ary);
JUPITER_CONTROL_DECL
const int *jcntrl_data_array_get_int(jcntrl_data_array *ary);
JUPITER_CONTROL_DECL
const jcntrl_aint_type *jcntrl_data_array_get_aint(jcntrl_data_array *ary);
JUPITER_CONTROL_DECL
const float *jcntrl_data_array_get_float(jcntrl_data_array *ary);
JUPITER_CONTROL_DECL
const double *jcntrl_data_array_get_double(jcntrl_data_array *ary);
JUPITER_CONTROL_DECL
const jcntrl_size_type *jcntrl_data_array_get_sizes(jcntrl_data_array *ary);

JUPITER_CONTROL_DECL
const char *jcntrl_char_array_get(jcntrl_char_array *ary);
JUPITER_CONTROL_DECL
const char *jcntrl_bool_array_get(jcntrl_bool_array *ary);
JUPITER_CONTROL_DECL
const int *jcntrl_int_array_get(jcntrl_int_array *ary);
JUPITER_CONTROL_DECL
const jcntrl_aint_type *jcntrl_aint_array_get(jcntrl_aint_array *ary);
JUPITER_CONTROL_DECL
const float *jcntrl_float_array_get(jcntrl_float_array *ary);
JUPITER_CONTROL_DECL
const double *jcntrl_double_array_get(jcntrl_double_array *ary);
JUPITER_CONTROL_DECL
const jcntrl_size_type *jcntrl_size_array_get(jcntrl_size_array *ary);

/**
 * Get raw writable pointer
 *
 * - Returns NULL if completely readonly or empty.
 * - If the array can generate the writable portion from read-only data,
 *   allocate writable portion and returns it.
 */
JUPITER_CONTROL_DECL
void *jcntrl_data_array_get_writable(jcntrl_data_array *ary);

/**
 * Get raw pointer, but returns NULL if element type is not @p meta.
 */
JUPITER_CONTROL_DECL
void *
jcntrl_data_array_get_writable_by_meta(jcntrl_data_array *ary,
                                       const jcntrl_shared_object_data *meta);

JUPITER_CONTROL_DECL
char *jcntrl_data_array_get_writable_char(jcntrl_data_array *ary);
JUPITER_CONTROL_DECL
char *jcntrl_data_array_get_writable_bool(jcntrl_data_array *ary);
JUPITER_CONTROL_DECL
int *jcntrl_data_array_get_writable_int(jcntrl_data_array *ary);
JUPITER_CONTROL_DECL
jcntrl_aint_type *jcntrl_data_array_get_writable_aint(jcntrl_data_array *ary);
JUPITER_CONTROL_DECL
float *jcntrl_data_array_get_writable_float(jcntrl_data_array *ary);
JUPITER_CONTROL_DECL
double *jcntrl_data_array_get_writable_double(jcntrl_data_array *ary);
JUPITER_CONTROL_DECL
jcntrl_size_type *jcntrl_data_array_get_writable_sizes(jcntrl_data_array *ary);

/**
 * Get raw writable pointer
 *
 * - Detach from externally bounded array.
 */
JUPITER_CONTROL_DECL
char *jcntrl_char_array_get_writable(jcntrl_char_array *ary);
JUPITER_CONTROL_DECL
char *jcntrl_bool_array_get_writable(jcntrl_bool_array *ary);
JUPITER_CONTROL_DECL
int *jcntrl_int_array_get_writable(jcntrl_int_array *ary);
JUPITER_CONTROL_DECL
jcntrl_aint_type *jcntrl_aint_array_get_writable(jcntrl_aint_array *ary);
JUPITER_CONTROL_DECL
float *jcntrl_float_array_get_writable(jcntrl_float_array *ary);
JUPITER_CONTROL_DECL
double *jcntrl_double_array_get_writable(jcntrl_double_array *ary);
JUPITER_CONTROL_DECL
jcntrl_size_type *jcntrl_size_array_get_writable(jcntrl_size_array *ary);

/**
 * Get value of given index as double
 */
JUPITER_CONTROL_DECL
double jcntrl_data_array_get_value(jcntrl_data_array *ary,
                                   jcntrl_size_type index);

/**
 * Get value of given index as int (intmax_t)
 *
 * If you pass non-NULL pointer to @p err, the status will be set
 * (sets 0 for success, non-0 for any errors).
 */
JUPITER_CONTROL_DECL
intmax_t jcntrl_data_array_get_ivalue(jcntrl_data_array *ary,
                                      jcntrl_size_type index, int *err);

/**
 * Set value at given index as double
 */
JUPITER_CONTROL_DECL
int jcntrl_data_array_set_value(jcntrl_data_array *ary, jcntrl_size_type index,
                                double value);

/**
 * Set value at given index as int (intmax_t)
 */
JUPITER_CONTROL_DECL
int jcntrl_data_array_set_ivalue(jcntrl_data_array *ary, jcntrl_size_type index,
                                 intmax_t value);

/**
 * Test for copy compatible types.
 *
 * @note The underlying types of jcntrl_char_array and jcntrl_bool_array are the
 * same, but they are used with different meanings. So they are copy
 * incompatible.
 *
 * @note The underlying type of jcntrl_size_array and jcntrl_aint_array could be
 * the same type by the meaning of MPI specification, but they are always
 * treated as copy incompatible. MPI specification does not state a specific
 * type defined by the C-language standard. Without MPI-linkage, the underlying
 * type of jcntrl_size_array and jcntrl_aint_array are the same but still copy
 * incompatible.
 */
JUPITER_CONTROL_DECL
int jcntrl_data_array_copyable_tt(const jcntrl_shared_object_data *desttype,
                                  const jcntrl_shared_object_data *srctype);

JUPITER_CONTROL_DECL
int jcntrl_data_array_copyable_ta(const jcntrl_shared_object_data *desttype,
                                  jcntrl_data_array *src);

JUPITER_CONTROL_DECL
int jcntrl_data_array_copyable_at(jcntrl_data_array *dest,
                                  const jcntrl_shared_object_data *srctype);

JUPITER_CONTROL_DECL
int jcntrl_data_array_copyable(jcntrl_data_array *dest, jcntrl_data_array *src);

/**
 * Test for copy compatible for copy to specific type.
 */
JUPITER_CONTROL_DECL
int jcntrl_char_array_copyable(jcntrl_data_array *src);

JUPITER_CONTROL_DECL
int jcntrl_bool_array_copyable(jcntrl_data_array *src);

JUPITER_CONTROL_DECL
int jcntrl_int_array_copyable(jcntrl_data_array *src);

JUPITER_CONTROL_DECL
int jcntrl_double_array_copyable(jcntrl_data_array *src);

JUPITER_CONTROL_DECL
int jcntrl_size_array_copyable(jcntrl_data_array *src);

JUPITER_CONTROL_DECL
int jcntrl_aint_array_copyable(jcntrl_data_array *src);

/**
 * @brief Copy raw data to data array
 * @param dest destination array
 * @param src source array
 * @param ntuple Number of tuples to copy
 * @param idest Start index for @p dest
 * @param iskip Start index for @p src
 * @retval 0 failed (type error or allocation error)
 * @retval 1 success
 *
 * Overflowing portion will be silently discarded.
 */
JUPITER_CONTROL_DECL
int jcntrl_data_array_copy(jcntrl_data_array *dest, jcntrl_data_array *src,
                           jcntrl_size_type ntuple, jcntrl_size_type idest,
                           jcntrl_size_type skip);

JUPITER_CONTROL_DECL
int jcntrl_char_array_copy(jcntrl_char_array *dest, jcntrl_data_array *src,
                           jcntrl_size_type ntuple, jcntrl_size_type idest,
                           jcntrl_size_type iskip);

JUPITER_CONTROL_DECL
int jcntrl_bool_array_copy(jcntrl_bool_array *dest, jcntrl_data_array *src,
                           jcntrl_size_type ntuple, jcntrl_size_type idest,
                           jcntrl_size_type iskip);

JUPITER_CONTROL_DECL
int jcntrl_int_array_copy(jcntrl_int_array *dest, jcntrl_data_array *src,
                          jcntrl_size_type ntuple, jcntrl_size_type idest,
                          jcntrl_size_type iskip);

JUPITER_CONTROL_DECL
int jcntrl_aint_array_copy(jcntrl_aint_array *dest, jcntrl_data_array *src,
                           jcntrl_size_type ntuple, jcntrl_size_type idest,
                           jcntrl_size_type iskip);

JUPITER_CONTROL_DECL
int jcntrl_float_array_copy(jcntrl_float_array *dest, jcntrl_data_array *src,
                            jcntrl_size_type ntuple, jcntrl_size_type idest,
                            jcntrl_size_type iskip);

JUPITER_CONTROL_DECL
int jcntrl_double_array_copy(jcntrl_double_array *dest, jcntrl_data_array *src,
                             jcntrl_size_type ntuple, jcntrl_size_type idest,
                             jcntrl_size_type iskip);

JUPITER_CONTROL_DECL
int jcntrl_size_array_copy(jcntrl_size_array *dest, jcntrl_data_array *src,
                           jcntrl_size_type ntuple, jcntrl_size_type idest,
                           jcntrl_size_type iskip);

/**
 * Copies list of indices in @p srcidx to @p dstidx
 * @param dest Destination array
 * @param src Source array
 * @param dstidx Indices of @p dest
 * @param srcidx Indices of @p src
 * @retval 0 failed
 * @retval 1 success
 *
 * If indices are NULL, they are treated as packed.
 *
 * Although this function is equivalent to jcntrl_data_array_copy(dest, src,
 * jcntrl_data_array_get_ntuple(src), 0, 0) when both of @p srcidx and @p dstidx
 * are NULL, we recommend to use jcntrl_data_array_copy().
 *
 * This function may not process correctly if @p dest and @p src overlaps.
 * This function can be slow for large @p srcidx and @p dstidx.
 */
JUPITER_CONTROL_DECL
int jcntrl_data_array_copyidx(jcntrl_data_array *dest, jcntrl_data_array *src,
                              jcntrl_data_array *dstidx,
                              jcntrl_data_array *srcidx);

/*
 * Type specific utilities
 */

/**
 * Make NUL-terminated C-style string from jcntrl_char_array (or arrays with
 * type of it).
 *
 * @note This function always make a copy of the contents even if the content
 * already ends with '\0'. Please deallocate the returned pointer with free().
 */
JUPITER_CONTROL_DECL
char *jcntrl_char_array_make_cstr(jcntrl_data_array *array);

typedef void jcntrl_char_array_work_cstr_func(char *str, jcntrl_size_type len,
                                              void *arg);

/**
 * @retval 0 Failed to call @p func (by prior error)
 * @retval 1 Function called
 *
 * Work with temporal NUL-terminated C-style string from jcntrl_char_array (or
 * arrays with type of it).
 *
 * In this function, @p str is not heap. If @p array is too large to store in
 * stack or somewhere VLA supports, the application tends to cause SEGV. Also do
 * not free @p str.
 */
JUPITER_CONTROL_DECL
int jcntrl_char_array_work_cstr(jcntrl_data_array *array,
                                jcntrl_char_array_work_cstr_func *func,
                                void *arg);

/**
 * strtod() for jcntrl_char_array (or array with type of it)
 *
 * Stores the corresponding index of endptr of strtod() returns to @p len.
 *
 * Stores -1 to @p len if allocation error occured
 */
JUPITER_CONTROL_DECL
double jcntrl_char_array_strtod(jcntrl_data_array *array,
                                jcntrl_size_type *len);

/**
 * strtof() for jcntrl_char_array (or array with type of it)
 *
 * Stores the corresponding index of endptr of strtof() returns to @p len.
 *
 * Stores -1 to @p len if allocation error occured
 */
JUPITER_CONTROL_DECL
float jcntrl_char_array_strtof(jcntrl_data_array *array, jcntrl_size_type *len);

/**
 * strtol() for jcntrl_char_array (or array with type of it)
 *
 * Stores the corresponding index of endptr of strtol() returns to @p len.
 *
 * Stores -1 to @p len if allocation error occured
 */
JUPITER_CONTROL_DECL
long jcntrl_char_array_strtol(jcntrl_data_array *array, int base,
                              jcntrl_size_type *len);

/**
 * strtoll() for jcntrl_char_array (or array with type of it)
 *
 * Stores the corresponding index of endptr of strtoll() returns to @p len.
 *
 * Stores -1 to @p len if allocation error occured
 */
JUPITER_CONTROL_DECL
long long jcntrl_char_array_strtoll(jcntrl_data_array *array, int base,
                                    jcntrl_size_type *len);

/**
 * Helper for get string from jcntrl_char_array to raw string and length
 *
 * @note Just return pointer of jcntrl_char_array for the 'get' counterpart of
 * jcntrl_set_string().
 */
JUPITER_CONTROL_DECL
const char *jcntrl_get_string_c(jcntrl_char_array *src, jcntrl_size_type *len);

/**
 * Helper for set string to jcntrl_char_array (with copying).
 */
JUPITER_CONTROL_DECL
int jcntrl_set_string(jcntrl_char_array **dest, jcntrl_data_array *src);

/**
 * Helper for set string to jcntrl_char_array (with copying).
 */
JUPITER_CONTROL_DECL
int jcntrl_set_string_c(jcntrl_char_array **dest, const char *str,
                        jcntrl_size_type len);

#endif
