/**
 * @brief Stores information of control processing
 */

#ifndef JUPITER_CONTROL_INFORMATION_H
#define JUPITER_CONTROL_INFORMATION_H

#include "defs.h"
#include "shared_object.h"

#include <jupiter/geometry/vector.h>

JUPITER_CONTROL_DECL_BEGIN

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_information);

/**
 * @brief Create new information object
 * @return Created information object, NULL if allocation failed
 */
JUPITER_CONTROL_DECL
jcntrl_information *jcntrl_information_new(void);

/**
 * @brief Unlink information object
 * @param info Information object to discard reference
 * @return info itself, or NULL if no references remained
 */
JUPITER_CONTROL_DECL
jcntrl_information *jcntrl_information_unlink(jcntrl_information *info);

/**
 * @brief Required type for specified information key
 * @param key Key value to test.
 * @return matching information datatype, or JCNTRL_IDATATYPE_INVALID if invalid key value given
 */
JUPITER_CONTROL_DECL
jcntrl_information_datatype
jcntrl_information_get_required_type(jcntrl_info key);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_information_object(jcntrl_information *info);
JUPITER_CONTROL_DECL
jcntrl_information *jcntrl_information_downcast(jcntrl_shared_object *object);

/**
 * @brief Check specified key exists
 * @param info information object to check.
 * @param key key value to check.
 * @return 1 if exists, 0 if not
 */
JUPITER_CONTROL_DECL
int jcntrl_information_has(jcntrl_information *info, jcntrl_info key);

/**
 * @brief Remove specified key entry
 * @param info information object to operate
 * @param key key to remove
 * @return 1 if success, 0 if failed (ex. locked)
 *
 * This function returns 1 if the entry already does not exist.
 */
JUPITER_CONTROL_DECL
int jcntrl_information_remove(jcntrl_information *info, jcntrl_info key);

/**
 * @brief Remove all entries in info
 * @param info information object to clear all entry.
 * @return 1 if success, 0 if failed (ex. locked)
 */
JUPITER_CONTROL_DECL
int jcntrl_information_clear(jcntrl_information *info);

/**
 * @brief Check specified value is true
 * @param info information object
 * @param key key value to check.
 * @return 1 if true, 0 otherwise
 */
JUPITER_CONTROL_DECL
int jcntrl_information_is_true(jcntrl_information *info, jcntrl_info key);

/**
 * @brief Check specified value is false
 * @param info information object
 * @param key key value to check.
 * @return 1 if false, 0 otherwise
 */
JUPITER_CONTROL_DECL
int jcntrl_information_is_false(jcntrl_information *info, jcntrl_info key);

/**
 * @brief Check specified value is falsey
 * @param info information object
 * @param key key value to check.
 * @return 1 if false or *not found*, 0 otherwise
 */
JUPITER_CONTROL_DECL
int jcntrl_information_is_falsey(jcntrl_information *info, jcntrl_info key);

/**
 * @brief Get integer value from integer key
 * @param info information object to get
 * @param key Key value to get from
 * @return stored value, 0 if not found
 *
 * @p key must be one of integer keys.
 */
JUPITER_CONTROL_DECL
int jcntrl_information_get_integer(jcntrl_information *info, jcntrl_info key);

JUPITER_CONTROL_DECL
int jcntrl_information_get_integer_nf(jcntrl_information *info, jcntrl_info key,
                                      int (*func_if_notfound)(void *),
                                      void *arg, int value_notfound);

JUPITER_CONTROL_DECL
const int *jcntrl_information_get_range(jcntrl_information *info,
                                        jcntrl_info key);

JUPITER_CONTROL_DECL
const int *jcntrl_information_get_extent(jcntrl_information *info,
                                         jcntrl_info key);

JUPITER_CONTROL_DECL
const int *jcntrl_information_get_index(jcntrl_information *info,
                                        jcntrl_info key);

JUPITER_CONTROL_DECL
double jcntrl_information_get_float(jcntrl_information *info, jcntrl_info key);

JUPITER_CONTROL_DECL
geom_vec3 jcntrl_information_get_vector(jcntrl_information *info,
                                        jcntrl_info key);

JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_information_get_object(jcntrl_information *info,
                                                    jcntrl_info key);

JUPITER_CONTROL_DECL
const char *jcntrl_information_get_string(jcntrl_information *info,
                                          jcntrl_info key);

JUPITER_CONTROL_DECL
jcntrl_datatype jcntrl_information_get_datatype(jcntrl_information *info,
                                                jcntrl_info key);

JUPITER_CONTROL_DECL
const jcntrl_shared_object_data *
jcntrl_information_get_objecttype(jcntrl_information *info, jcntrl_info key);

/**
 * @brief Set boolean value
 * @param info information object
 * @param key key value to set
 * @param value value to set
 * @return
 *
 * @p value will be normalized to 0/1.
 */
JUPITER_CONTROL_DECL
int jcntrl_information_set_bool(jcntrl_information *info, jcntrl_info key,
                                int value);

/**
 * @brief Set integer value
 * @param info information object
 * @paarm key key value to set
 * @param value value to set
 * @return
 */
JUPITER_CONTROL_DECL
int jcntrl_information_set_integer(jcntrl_information *info, jcntrl_info key,
                                   int value);

JUPITER_CONTROL_DECL
int jcntrl_information_set_range(jcntrl_information *info, jcntrl_info key,
                                 const int range[2]);

JUPITER_CONTROL_DECL
int jcntrl_information_set_range2(jcntrl_information *info, jcntrl_info key,
                                  int begin, int end);

JUPITER_CONTROL_DECL
int jcntrl_information_set_extent(jcntrl_information *info, jcntrl_info key,
                                  const int extent[6]);

JUPITER_CONTROL_DECL
int jcntrl_information_set_extent6(jcntrl_information *info, jcntrl_info key,
                                   int x1, int x2, int x3, int x4, int x5,
                                   int x6);

JUPITER_CONTROL_DECL
int jcntrl_information_set_index(jcntrl_information *info, jcntrl_info key,
                                 const int index[3]);

JUPITER_CONTROL_DECL
int jcntrl_information_set_index3(jcntrl_information *info, jcntrl_info key,
                                  int x1, int x2, int x3);

JUPITER_CONTROL_DECL
int jcntrl_information_set_float(jcntrl_information *info, jcntrl_info key,
                                 double value);

JUPITER_CONTROL_DECL
int jcntrl_information_set_vector(jcntrl_information *info, jcntrl_info key,
                                  geom_vec3 vec);

JUPITER_CONTROL_DECL
int jcntrl_information_set_object(jcntrl_information *info, jcntrl_info key,
                                  jcntrl_shared_object *object);

JUPITER_CONTROL_DECL
int jcntrl_information_set_const_string(jcntrl_information *info,
                                        jcntrl_info key, const char *str);

/**
 * Copy and set NUL-terminated string.
 */
JUPITER_CONTROL_DECL
int jcntrl_information_set_string(jcntrl_information *info, jcntrl_info key,
                                  const char *str);

JUPITER_CONTROL_DECL
int jcntrl_information_set_datatype(jcntrl_information *info, jcntrl_info key,
                                    jcntrl_datatype value);

JUPITER_CONTROL_DECL
int jcntrl_information_set_objecttype(jcntrl_information *info, jcntrl_info key,
                                      const jcntrl_shared_object_data *value);

#define jcntrl_information_set_objecttype(info, key, type) \
  jcntrl_information_set_objecttype(info, key, type##_metadata_init())

JUPITER_CONTROL_DECL_END

#endif
