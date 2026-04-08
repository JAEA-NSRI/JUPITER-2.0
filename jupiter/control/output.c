
#include "output.h"
#include "defs.h"
#include "error.h"
#include "information.h"
#include "input.h"

#include "connection.h"
#include "executive.h"

#include "executive_data.h"
#include "shared_object.h"

jcntrl_output *jcntrl_output_next_port(jcntrl_output *item)
{
  jcntrl_output *next;

  JCNTRL_ASSERT(item);

  next = jcntrl_output_entry(geom_list_next(&item->list));
  if (jcntrl_output_is_head(next)) {
    return NULL;
  }
  return next;
}

jcntrl_output *jcntrl_output_prev_port(jcntrl_output *item)
{
  jcntrl_output *prev;

  JCNTRL_ASSERT(item);

  prev = jcntrl_output_entry(geom_list_prev(&item->list));
  if (jcntrl_output_is_head(prev)) {
    return NULL;
  }
  return prev;
}

int jcntrl_output_is_head(jcntrl_output *item)
{
  return jcntrl_output_rewind(item) == item;
}

jcntrl_output *jcntrl_output_rewind(jcntrl_output *item)
{
  jcntrl_executive *owner;

  owner = jcntrl_output_owner(item);
  if (owner) {
    return jcntrl_executive_get_output(owner);
  }
  return NULL;
}

jcntrl_executive *jcntrl_output_owner(jcntrl_output *item)
{
  JCNTRL_ASSERT(item);

  return item->owner;
}

jcntrl_connection *jcntrl_output_downstreams(jcntrl_output *item)
{
  JCNTRL_ASSERT(item);

  return &item->port;
}

jcntrl_information *jcntrl_output_information(jcntrl_output *item)
{
  JCNTRL_ASSERT(item);

  return item->information;
}

jcntrl_output *jcntrl_output_at(jcntrl_output *item, int index)
{
  item = jcntrl_output_rewind(item);
  if (index >= 0) {
    item = jcntrl_output_next_port(item);
    while (item && index > 0) {
      item = jcntrl_output_next_port(item);
      --index;
    }
  } else {
    while (item && index < 0) {
      item = jcntrl_output_prev_port(item);
      ++index;
    }
  }
  return item;
}

void jcntrl_output_disconnect_all(jcntrl_output *item)
{
  jcntrl_connection *conn, *next;
  conn = jcntrl_output_downstreams(item);
  conn = jcntrl_connection_next(conn);
  for (next = (conn ? jcntrl_connection_next(conn) : NULL); conn;
       conn = next, next = (conn ? jcntrl_connection_next(conn) : NULL)) {
    jcntrl_input *downstream;

    downstream = jcntrl_connection_get_downstream_port(conn);
    jcntrl_input_disconnect(downstream);
  }
}

jcntrl_shared_object *
jcntrl_output_get_data_object_at(jcntrl_output *output_head, int index)
{
  jcntrl_output *output;

  output = jcntrl_output_at(output_head, index);
  if (!output)
    return NULL;

  return jcntrl_output_get_data_object(output);
}

jcntrl_shared_object *jcntrl_output_get_data_object(jcntrl_output *output)
{
  jcntrl_information *info;
  info = jcntrl_output_information(output);
  if (!info)
    return NULL;

  return jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
}

void *(jcntrl_output_get_data_object_as)(jcntrl_output *output,
                                         const jcntrl_shared_object_data *cls)
{
  jcntrl_shared_object *obj;
  obj = jcntrl_output_get_data_object(output);
  if (!obj)
    return NULL;

  return jcntrl_shared_object_downcast_by_meta(cls, obj);
}

void *(jcntrl_output_get_data_object_at_as)(jcntrl_output *output, int index,
                                          const jcntrl_shared_object_data *cls)
{
  jcntrl_shared_object *obj;
  obj = jcntrl_output_get_data_object_at(output, index);
  if (!obj)
    return NULL;

  return jcntrl_shared_object_downcast_by_meta(cls, obj);
}
