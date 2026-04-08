/**
 * @brief Input connection handler
 *
 * Handles input connections of each executives. These have not to do
 * with I/O to external files.
 */

#ifndef JUPITER_CONTROL_INPUT_H
#define JUPITER_CONTROL_INPUT_H

#include "defs.h"

JUPITER_CONTROL_DECL_BEGIN

/**
 * @brief Add input port
 * @param prev Insertion point which would be next of new port.
 * @return new port, NULL if allocation failed
 *
 * Adding to the head port means adding port to last.
 */
JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_input_add(jcntrl_input *next);

/**
 * @brief Next input port
 * @param item Item to get
 *
 * If @p item's next port is the head port, this function returns
 * NULL. If @p item is the head port, returns the first port.
 */
JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_input_next_port(jcntrl_input *item);

/**
 * @brief Previous input port
 * @param item Item to get
 *
 * If @p item's previous port is the head port, this function returns
 * NULL. If @p item is the head port, returns the last port.
 */
JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_input_prev_port(jcntrl_input *item);

/**
 * @brief Test whether an item is head port or not
 * @param item item to test
 * @retval 0 item is not head
 * @retval 1 item is head item
 */
JUPITER_CONTROL_DECL
int jcntrl_input_is_head(jcntrl_input *item);

/**
 * @brief Returns head port
 */
JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_input_rewind(jcntrl_input *item);

/**
 * @brief Returns port at given index
 * @param item input port chain to get
 * @param index Index to get
 * @return Port on the given index, NULL if does not exist.
 *
 * Because the input port chain is stored as a list and not a vector
 * (or an array), this function has the cost in order of O(N).
 */
JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_input_at(jcntrl_input *item, int index);

/**
 * @brief Set number of input to given one
 * @param item input port to set
 * @param num Number of ports to allocate
 * @return head port of @p item, NULL if allocation failed.
 *
 * Allocate or deallocate ports to make @p num ports in total.
 */
JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_input_set_number_of_ports(jcntrl_input *item, int num);

/**
 * @brief Delete port
 * @param item Input port to delete
 *
 * Disconnect and delete given port. Deleting the head port is prohibited
 * (causes segmentation fault in normal mode).
 */
JUPITER_CONTROL_DECL
void jcntrl_input_delete(jcntrl_input *item);

/**
 * @brief Connect to output port
 * @param input Input port to connect
 * @param output Output port to be connected to
 * @return 1 if success, 0 if failed
 *
 * You can not connect 'head' ports.
 *
 * If @p input is already connected, that connection will be
 * disconnected.
 */
JUPITER_CONTROL_DECL
int jcntrl_input_connect(jcntrl_input *input, jcntrl_output *output);

/**
 * @brief Disconnect from output port
 * @param input Input port to disconnect
 * @return 1 if success, 0 if failed
 */
JUPITER_CONTROL_DECL
int jcntrl_input_disconnect(jcntrl_input *input);

/**
 * @brief Test input is connected
 * @param input Input port to test
 * @return 1 if conneted, 0 if not
 *
 * This is equivalent to testing whether the return value of
 * jcntrl_input_upstream_executive() is not NULL, but done in less
 * const, and provides more meaningful way.
 */
JUPITER_CONTROL_DECL
int jcntrl_input_is_connected(jcntrl_input *input);

/**
 * @brief Get upstream executive
 * @param input Input port to check.
 * @return executive of upstream, or NULL if not connected.
 */
JUPITER_CONTROL_DECL
jcntrl_executive *jcntrl_input_upstream_executive(jcntrl_input *input);

/**
 * @brief Get executive which owns this port
 * @param input Input port to obtain
 * @return executive of owner
 */
JUPITER_CONTROL_DECL
jcntrl_executive *jcntrl_input_owner(jcntrl_input *input);

/**
 * @brief Get upstream information
 * @param input Input port to obtain
 * @return shallow copy of upstream information, or NULL if not connected
 */
JUPITER_CONTROL_DECL
jcntrl_information *jcntrl_input_upstream_information(jcntrl_input *input);

/**
 * @brief Get input port information
 * @param input Input port to obtain
 * @retur shallow copy of upstream inforamation
 */
JUPITER_CONTROL_DECL
jcntrl_information *jcntrl_input_information(jcntrl_input *input);

/**
 * @brief Get input data object
 * @param input Input head
 * @param index index of port to get
 * @return object
 *
 * Shorthand for
 * ```
 * jcntrl_information_get_object(
 *  jcntrl_input_upstream_information(jcntrl_input_at(input_head, index)),
 *  JCNTRL_INFO_DATA_OBJECT);
 * ```
 */
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_input_get_data_object_at(jcntrl_input *input_head,
                                                      int index);

/**
 * @brief Get input data object
 * @param input Input port
 * @return object
 *
 * Shorthand for
 * ```
 * jcntrl_information_get_object(
 *  jcntrl_input_information(input), JCNTRL_INFO_DATA_OBJECT);
 * ```
 */
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_input_get_data_object(jcntrl_input *input);

JUPITER_CONTROL_DECL
void *jcntrl_input_get_data_object_as(jcntrl_input *input,
                                      const jcntrl_shared_object_data *cls);

#define jcntrl_input_get_data_object_as_T(input, sttype, clstype) \
  ((sttype *)jcntrl_input_get_data_object_as(input,               \
                                             JCNTRL_METADATA_INIT(clstype)()))

#define jcntrl_input_get_data_object_as(input, type) \
  jcntrl_input_get_data_object_as_T(input, type, type)

JUPITER_CONTROL_DECL
void *jcntrl_input_get_data_object_at_as(jcntrl_input *input, int index,
                                         const jcntrl_shared_object_data *cls);

#define jcntrl_input_get_data_object_at_as_T(input, index, sttype, clstype) \
  ((sttype *)jcntrl_input_get_data_object_at_as(input, index,               \
                                                JCNTRL_METADATA_INIT(       \
                                                  clstype)()))

#define jcntrl_input_get_data_object_at_as(input, index, type) \
  jcntrl_input_get_data_object_at_as_T(input, index, type, type)

JUPITER_CONTROL_DECL_END

#endif
