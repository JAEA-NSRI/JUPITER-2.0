#ifndef JUPITER_CONTROL_SHARED_OBJECT_H
#define JUPITER_CONTROL_SHARED_OBJECT_H

#include "defs.h"

JUPITER_CONTROL_DECL_BEGIN

/**
 * @todo Make cls##_metadata_init() thread-safe. Currently,
 * cls##_metadata_init() functions are not thread-safe, when first
 * initialization.
 */
#define JCNTRL_SHARED_METADATA_INIT_DECL(cls) \
  const jcntrl_shared_object_data *cls##_metadata_init(void)

/**
 * Root shared object metadata (metadata for jcntrl_shared_object).
 */
JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_shared_object);

/**
 * Get shared object meta data by class name.
 *
 * @note The specific entry needs to be installed before this function is
 * called. This function only works when the given class contextually assumed to
 * already be added.
 */
JUPITER_CONTROL_DECL
const jcntrl_shared_object_data *
jcntrl_shared_object_data_for(const char *name);

JUPITER_CONTROL_DECL
void *jcntrl_shared_object_new_by_meta(const jcntrl_shared_object_data *p);

/**
 * Creates by class name
 *
 * @note The specific entry needs to be installed before this function is
 * called. This function only works when the given class contextually assumed to
 * already be added.
 */
static inline void *jcntrl_shared_object_new_by_name(const char *type)
{
  return jcntrl_shared_object_new_by_meta(jcntrl_shared_object_data_for(type));
}

#define jcntrl_shared_object_new(type) \
  ((type *)jcntrl_shared_object_new_by_meta(type##_metadata_init()))

JUPITER_CONTROL_DECL
void *jcntrl_shared_object_downcast_by_meta(const jcntrl_shared_object_data *p,
                                            jcntrl_shared_object *obj);

static inline void *
jcntrl_shared_object_downcast_by_name(const char *type,
                                      jcntrl_shared_object *obj)
{
  const jcntrl_shared_object_data *p;
  p = jcntrl_shared_object_data_for(type);
  return jcntrl_shared_object_downcast_by_meta(p, obj);
}

#define jcntrl_shared_object_downcast(type, obj) \
  ((type *)jcntrl_shared_object_downcast_by_meta(type##_metadata_init(), obj))

JUPITER_CONTROL_DECL
int jcntrl_shared_object_is_a_by_meta(const jcntrl_shared_object_data *p,
                                      jcntrl_shared_object *obj);

static inline int jcntrl_shared_object_is_a_by_name(const char *name,
                                                    jcntrl_shared_object *obj)
{
  const jcntrl_shared_object_data *p;
  p = jcntrl_shared_object_data_for(name);
  return jcntrl_shared_object_is_a_by_meta(p, obj);
}

#define jcntrl_shared_object_is_a(type, obj) \
  jcntrl_shared_object_is_a_by_meta(type##_metadata_init(), obj)

/**
 * Deletes the object (if object is no more referenced)
 *
 * @retval obj  Just decrement reference counter (still exist)
 * @retval NULL Object deallocated
 *
 * This function is exactly same behavior for
 * jcntrl_shared_object_release_ownership().
 */
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_shared_object_delete(jcntrl_shared_object *obj);

/**
 * Takes ownership for the @p obj (increments the reference count)
 *
 * @retval obj  Success
 * @retval NULL Could not take ownership (overflow or statically allocated)
 */
JUPITER_CONTROL_DECL
jcntrl_shared_object *
jcntrl_shared_object_take_ownership(jcntrl_shared_object *obj);

/**
 * Releases ownership for @p obj (decrements the reference count)
 *
 * @retval obj  Decremented reference count
 * @retval NULL Object has been deallocated (i.e., anymore referenced)
 *
 * This function is exactly same behavior for jcntrl_shared_object_delete().
 * Use this function to indicate the deallocation is not expected.
 */
JUPITER_CONTROL_DECL
jcntrl_shared_object *
jcntrl_shared_object_release_ownership(jcntrl_shared_object *obj);

/**
 * @return pointer to class.
 */
JUPITER_CONTROL_DECL
const jcntrl_shared_object_data *
jcntrl_shared_object_class(jcntrl_shared_object *obj);

/**
 * Return class name of shared_object.
 */
JUPITER_CONTROL_DECL
const char *jcntrl_shared_object_class_name(jcntrl_shared_object *obj);

/**
 * Return reference counter
 */
JUPITER_CONTROL_DECL
int jcntrl_shared_object_refcount(jcntrl_shared_object *obj);

/**
 * Return 1 if the object is shared (ownership taken by multiple objects or
 * contexts)
 */
JUPITER_CONTROL_DECL
int jcntrl_shared_object_is_shared(jcntrl_shared_object *obj);

/**
 * Return 1 if the object is static (not allocated dynamically or automatic)
 */
JUPITER_CONTROL_DECL
int jcntrl_shared_object_is_static(jcntrl_shared_object *obj);

JUPITER_CONTROL_DECL_END

#endif
