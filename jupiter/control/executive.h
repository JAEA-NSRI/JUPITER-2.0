/**
 * @brief Executive class
 *
 * Executive defines excutive and connect each other.
 */

#ifndef JUPITER_CONTROL_EXECUTIVE_H
#define JUPITER_CONTROL_EXECUTIVE_H

#include "defs.h"
#include "shared_object.h"

JUPITER_CONTROL_DECL_BEGIN

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_executive);

/**
 * @brief Executive creater for executive of specific class by ID
 * @param executive_id ID that value returned by jcntrl_executive_install.
 * @return New executive class, or NULL if executive_id is invalid,
 * or, allocatoin failed.
 */
JUPITER_CONTROL_DECL
jcntrl_executive *jcntrl_executive_new(int exective_id);

/**
 * @brief Test whether given executive is of a specific class.
 * @param exe Executive to test
 * @param cls Executive Class to test
 * @return 1 if the given executive is the specific class, 0 otherwise
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_is_a(jcntrl_executive *exe,
                          const jcntrl_shared_object_data *cls);

/**
 * @brief Calls deleter of subclass to delete executive
 * @param exe Executive to delete
 *
 * @note for who implements a new executive class: You must call this
 * function instead of your deleter function given to
 * jcntrl_executive_class callbacks, if you are going to implement
 * delete function for your class. Also, this function does not manage
 * its memory, because jcntrl_executive should be used in embedded
 * into your class struct. Memory management is renponsible to your
 * deleter function set to jcntrl_executive_class.
 */
JUPITER_CONTROL_DECL
void jcntrl_executive_delete(jcntrl_executive *exe);

JUPITER_CONTROL_DECL
jcntrl_executive *jcntrl_executive_downcast(jcntrl_shared_object *object);
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_executive_object(jcntrl_executive *exe);

/**
 * @brief Set name of an executive
 * @param exe Executive to set.
 * @param name Name to set
 * @return Pointer to copy of @p name, `NULL` if allocation failed.
 *
 * This function allocates a space store name. If an allocation
 * failed, the name will not be changed.
 */
JUPITER_CONTROL_DECL
const char *jcntrl_executive_set_name(jcntrl_executive *exe, const char *name);

/**
 * @brief Get name of an executive
 * @param exe Executive to get.
 * @return the name set to the given executive
 *
 * @note Return value can be `NULL`.
 */
JUPITER_CONTROL_DECL
const char *jcntrl_executive_get_name(jcntrl_executive *exe);

/**
 * @brief Get input ring of an exectuive.
 * @param exe Executive to get.
 * @return Pointer to head of the input ring.
 */
JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_executive_get_input(jcntrl_executive *exe);

/**
 * @brief Get output ring of an executive.
 * @param exe Executive to get.
 * @return Pointer to head of the output ring.
 */
JUPITER_CONTROL_DECL
jcntrl_output *jcntrl_executive_get_output(jcntrl_executive *exe);

/**
 * @brief Get class name of an executive
 * @param exe Executive to get.
 * @return Pointer to the name, if avialable.
 */
JUPITER_CONTROL_DECL
const char *jcntrl_executive_get_class_name(jcntrl_executive *exe);

/**
 * @brief Process request for specified executive.
 * @param exe Executive to process.
 * @param request Request information
 * @return 0 if failed, otherwise success.
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_process_request(jcntrl_executive *exe,
                                     jcntrl_information *request);

/**
 * @brief Process update request of output information for given executive.
 * @param exe Executive to process
 * @param request Request information
 * @return 0 if failed, otherwise success.
 *
 * This function is considered to be an internal.
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_process_update_information(jcntrl_executive *exe,
                                                jcntrl_information *request);

/**
 * @brief Process update request of extent information for given executive.
 * @param exe Executive to process
 * @param request Request information
 * @return 0 if failed, otherwise success.
 *
 * This function is considered to be an internal.
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_process_update_extent(jcntrl_executive *exe,
                                           jcntrl_information *request);

/**
 * @brief Process update request of output data for given executive
 * @param exe Executive to process
 * @param request Request information
 * @return 0 if failed, otherwise success.
 *
 * This function is considered to be an internal.
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_process_update_data(jcntrl_executive *exe,
                                         jcntrl_information *request);

/**
 * @brief Update any information by specified request information
 * @param exe Executive to update
 * @return 0 if failed, 1 if success
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_update_by_request(jcntrl_executive *exe,
                                       jcntrl_information *request);

/* Following functions defined in information.c */

/**
 * @brief Update all information of executive
 * @param exe Executive to update
 * @return 0 if failed, 1 if success
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_update(jcntrl_executive *exe);

/**
 * @brief Update output information of executive
 * @param exe Executive to update
 * @return 0 if failed, 1 if success
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_update_information(jcntrl_executive *exe);

/**
 * @brief Update extent information of executive
 * @param exe Executive to update
 * @return 0 if failed, 1 if success
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_update_extent(jcntrl_executive *exe);

/**
 * @brief Update output data of executive
 * @param exe Executive to update
 * @return 0 if failed, 1 if success
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_update_data(jcntrl_executive *exe);

/**
 * @brief Run loop check start from given executive
 * @param exe Executive to check start
 * @return 0 if failed, 1 if pass
 *
 * Unrelated branches to @p exe and downstreams of @p exe won't be tested.
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_check_loop(jcntrl_executive *exe);

/**
 * @brief Get n-th input port of given executive
 * @param exe Executive to get
 * @param nth index of port
 * @return pointer to the specified index, NULL if does not exist.
 *
 * @note The index of the first port is 0. This function never return
 *       'head' port.
 *
 * This function is just short hand for
 * `jcntrl_input_at(jcntrl_executive_get_input(exe), nth)`
 */
JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_executive_input_port(jcntrl_executive *exe, int nth);

/**
 * @brief Get n-th input data object of given executive
 * @param exe Executive to get
 * @param nth index of port
 * @return data object from upstream of nth port
 *
 * This function is just short hand for
 * `jcntrl_input_object_at(jcntrl_executive_get_input(exe), nth)`
 */
JUPITER_CONTROL_DECL
jcntrl_shared_object *
jcntrl_executive_get_input_data_object(jcntrl_executive *exec, int nth);

JUPITER_CONTROL_DECL
void *
jcntrl_executive_get_input_data_object_as(jcntrl_executive *exec, int nth,
                                          const jcntrl_shared_object_data *cls);

#define jcntrl_executive_get_input_data_object_as_T(exec, nth, sttype,       \
                                                    clstype)                 \
  ((sttype *)jcntrl_executive_get_input_data_object_as(exec, nth,            \
                                                       JCNTRL_METADATA_INIT( \
                                                         clstype)()))

#define jcntrl_executive_get_input_data_object_as(exec, nth, type) \
  jcntrl_executive_get_input_data_object_as_T(exec, nth, type, type)

/**
 * @brief Get n-th output port of given executive
 * @param exe Executive to get
 * @param nth index of port
 * @return pointer to the specified index, NULL if does not exist.
 *
 * @note The index of the first port is 0. This function never return
 *       'head' port.
 *
 * This function is just short hand for
 * `jcntrl_output_at(jcntrl_executive_get_input(exe), nth)`
 */
JUPITER_CONTROL_DECL
jcntrl_output *jcntrl_executive_output_port(jcntrl_executive *exe, int nth);

/**
 * @brief Get n-th output data object of given executive
 * @param exe Executive to get
 * @param nth index of port
 * @return data object from upstream of nth port
 *
 * This function is just short hand for
 * `jcntrl_output_object_at(jcntrl_executive_get_output(exe), nth)`
 */
JUPITER_CONTROL_DECL
jcntrl_shared_object *
jcntrl_executive_get_output_data_object(jcntrl_executive *exec, int nth);

JUPITER_CONTROL_DECL
void *jcntrl_executive_get_output_data_object_as(
  jcntrl_executive *exec, int nth, const jcntrl_shared_object_data *cls);

#define jcntrl_executive_get_output_data_object_as_T(exec, nth, sttype,       \
                                                     clstype)                 \
  ((sttype *)jcntrl_executive_get_output_data_object_as(exec, nth,            \
                                                        JCNTRL_METADATA_INIT( \
                                                          clstype)()))

#define jcntrl_executive_get_output_data_object_as(exec, nth, type) \
  jcntrl_executive_get_output_data_object_as_T(exec, nth, type, type)

JUPITER_CONTROL_DECL_END

#endif
