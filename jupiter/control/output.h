
#ifndef JUPITER_CONTROL_OUTPUT_H
#define JUPITER_CONTROL_OUTPUT_H

#include "defs.h"

JUPITER_CONTROL_DECL_BEGIN

JUPITER_CONTROL_DECL
jcntrl_output *jcntrl_output_add(jcntrl_output *next);
JUPITER_CONTROL_DECL
jcntrl_output *jcntrl_output_next_port(jcntrl_output *item);
JUPITER_CONTROL_DECL
jcntrl_output *jcntrl_output_prev_port(jcntrl_output *item);
JUPITER_CONTROL_DECL
int jcntrl_output_is_head(jcntrl_output *item);
JUPITER_CONTROL_DECL
jcntrl_output *jcntrl_output_rewind(jcntrl_output *item);
JUPITER_CONTROL_DECL
jcntrl_output *jcntrl_output_at(jcntrl_output *item, int index);
JUPITER_CONTROL_DECL
jcntrl_output *jcntrl_output_set_number_of_ports(jcntrl_output *item, int num);
JUPITER_CONTROL_DECL
void jcntrl_output_delete(jcntrl_output *item);

JUPITER_CONTROL_DECL
jcntrl_connection *jcntrl_output_downstreams(jcntrl_output *item);
JUPITER_CONTROL_DECL
jcntrl_executive *jcntrl_output_owner(jcntrl_output *item);
JUPITER_CONTROL_DECL
jcntrl_information *jcntrl_output_information(jcntrl_output *item);

JUPITER_CONTROL_DECL
void jcntrl_output_disconnect_all(jcntrl_output *item);

/**
 * @brief Get output data object
 * @param output Output head
 * @param index index of port to get
 * @return object
 *
 * Shorthand for
 * ```
 * jcntrl_information_get_object(
 *  jcntrl_output_information(jcntrl_output_at(output_head, index)),
 *  JCNTRL_INFO_DATA_OBJECT);
 * ```
 */
JUPITER_CONTROL_DECL
jcntrl_shared_object *
jcntrl_output_get_data_object_at(jcntrl_output *output_head, int index);

/**
 * @brief Get output data object
 * @param output Output port
 * @return object
 *
 * Shorthand for
 * ```
 * jcntrl_information_get_object(
 *  jcntrl_output_information(output), JCNTRL_INFO_DATA_OBJECT);
 * ```
 */
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_output_get_data_object(jcntrl_output *output);

JUPITER_CONTROL_DECL
void *jcntrl_output_get_data_object_as(jcntrl_output *output,
                                       const jcntrl_shared_object_data *cls);

#define jcntrl_output_get_data_object_as_T(output, sttype, clstype)         \
  ((sttype *)jcntrl_output_get_data_object_as(output, JCNTRL_METADATA_INIT( \
                                                        clstype)()))

#define jcntrl_output_get_data_object_as(output, type) \
  jcntrl_output_get_data_object_as_T(output, type, type)

JUPITER_CONTROL_DECL
void *jcntrl_output_get_data_object_at_as(jcntrl_output *output, int index,
                                          const jcntrl_shared_object_data *cls);

#define jcntrl_output_get_data_object_at_as_T(output, index, sttype, clstype) \
  ((sttype *)jcntrl_output_get_data_object_at_as(output, index,               \
                                                 JCNTRL_METADATA_INIT(        \
                                                   clstype)()))

#define jcntrl_output_get_data_object_at_as(output, index, type) \
  jcntrl_output_get_data_object_at_as_T(output, index, type, type)

JUPITER_CONTROL_DECL_END

#endif
