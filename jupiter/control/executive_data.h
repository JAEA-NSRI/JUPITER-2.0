#ifndef JUPITER_CONTROL_EXECUTIVE_DATA_H
#define JUPITER_CONTROL_EXECUTIVE_DATA_H

#include "defs.h"
#include "shared_object_priv.h"

#include <jupiter/geometry/list.h>

JUPITER_CONTROL_DECL_BEGIN

/**
 * @brief Stores information of connection between executives
 */
struct jcntrl_connection
{
  struct geom_list list;
  struct jcntrl_output *upstream;
};

#define jcntrl_connection_entry(ptr) \
  geom_list_entry(ptr, struct jcntrl_connection, list)

/**
 * @brief Input port of jcntrl executive
 */
struct jcntrl_input
{
  struct geom_list list;
  jcntrl_connection port;
  jcntrl_information *information;
  jcntrl_executive *owner;
};

#define jcntrl_input_entry(ptr) geom_list_entry(ptr, struct jcntrl_input, list)

#define jcntrl_input_connection_entry(ptr) \
  geom_container_of(ptr, struct jcntrl_input, port)

/**
 * @brief Output port of jcntrl executive
 *
 * This is head of the output chain list of jcntrl_connection.
 */
struct jcntrl_output
{
  struct geom_list list;
  jcntrl_connection port;
  jcntrl_information *information;
  jcntrl_executive *owner;
};

#define jcntrl_output_entry(ptr) \
  geom_list_entry(ptr, struct jcntrl_output, list)

#define jcntrl_output_connection_entry(ptr) \
  geom_container_of(ptr, struct jcntrl_output, port)

/**
 * @brief Executive class
 */
struct jcntrl_executive
{
  jcntrl_shared_object object;
  char *name;                              ///< Name of executive
  int lname;                               ///< Allocated name length
  jcntrl_input input_head;                 ///< Input port ring
  jcntrl_output output_head;               ///< Output port ring
};
#define jcntrl_executive__ancestor jcntrl_shared_object
#define jcntrl_executive__dnmem object

enum jcntrl_executive_vtable_names
{
  jcntrl_executive_process_request_id = JCNTRL_VTABLE_START(jcntrl_executive),
  jcntrl_executive_process_update_information_id,
  jcntrl_executive_process_update_extent_id,
  jcntrl_executive_process_update_data_id,
  jcntrl_executive_fill_input_port_information_id,
  jcntrl_executive_fill_output_port_information_id,
  jcntrl_executive_class_name_id,
  jcntrl_executive_restart_read_id,
  jcntrl_executive_restart_write_id,
  JCNTRL_VTABLE_SIZE(jcntrl_executive)
};

JUPITER_CONTROL_DECL
void jcntrl_executive_process_request__wrapper(jcntrl_shared_object *obj,
                                               void *p,
                                               jcntrl_process_request *func);

JUPITER_CONTROL_DECL
void jcntrl_executive_process_update_information__wrapper(
  jcntrl_shared_object *obj, void *p, jcntrl_update_information *func);

JUPITER_CONTROL_DECL
void jcntrl_executive_process_update_extent__wrapper(
  jcntrl_shared_object *obj, void *p, jcntrl_update_extent *func);

JUPITER_CONTROL_DECL
void jcntrl_executive_process_update_data__wrapper(jcntrl_shared_object *obj,
                                                   void *p,
                                                   jcntrl_update_data *func);

JUPITER_CONTROL_DECL
void jcntrl_executive_fill_input_port_information__wrapper(
  jcntrl_shared_object *obj, void *p, jcntrl_fill_input_port_information *func);

JUPITER_CONTROL_DECL
void jcntrl_executive_fill_output_port_information__wrapper(
  jcntrl_shared_object *obj, void *p,
  jcntrl_fill_output_port_information *func);

JUPITER_CONTROL_DECL
int jcntrl_executive_process_request__super(
  const jcntrl_shared_object_data *ancestor, jcntrl_information *request);

JUPITER_CONTROL_DECL
int jcntrl_executive_process_update_information__super(
  const jcntrl_shared_object_data *ancestor, jcntrl_information *request);

JUPITER_CONTROL_DECL
int jcntrl_executive_process_update_extent__super(
  const jcntrl_shared_object_data *ancestor, jcntrl_information *request);

JUPITER_CONTROL_DECL
int jcntrl_executive_process_update_data__super(
  const jcntrl_shared_object_data *ancestor, jcntrl_information *request);

JUPITER_CONTROL_DECL
int jcntrl_executive_process_fill_input_port_information__super(
  const jcntrl_shared_object_data *ancestor, int index, jcntrl_input *port);

JUPITER_CONTROL_DECL
int jcntrl_executive_process_fill_output_port_information__super(
  const jcntrl_shared_object_data *ancestor, int index, jcntrl_output *port);

/**
 * @brief Initalize given executive class (for function-local object).
 * @param exe Executive to initialize
 * @param cls Executive class.
 *
 * jcntrl_excutive contain allocations in some processes. You have to call
 * jcntrl_shared_object_delete (or jcntrl_executive_delete) to clear allocated
 * data.
 */
JUPITER_CONTROL_DECL
int jcntrl_executive_init(jcntrl_executive *exe,
                          const jcntrl_shared_object_data *cls);

JUPITER_CONTROL_DECL_END

#endif
