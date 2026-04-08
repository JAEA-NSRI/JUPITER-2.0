
#include "input.h"
#include "defs.h"
#include "output.h"
#include "executive.h"
#include "connection.h"
#include "information.h"
#include "error.h"
#include "executive_data.h"
#include "shared_object.h"

int jcntrl_input_is_head(jcntrl_input *item)
{
  return jcntrl_input_rewind(item) == item;
}

jcntrl_input *jcntrl_input_next_port(jcntrl_input *item)
{
  jcntrl_input *next;
  next = jcntrl_input_entry(geom_list_next(&item->list));
  if (!jcntrl_input_is_head(next)) {
    return next;
  }
  return NULL;
}

jcntrl_input *jcntrl_input_prev_port(jcntrl_input *item)
{
  jcntrl_input *prev;
  prev = jcntrl_input_entry(geom_list_prev(&item->list));
  if (!jcntrl_input_is_head(prev)) {
    return prev;
  }
  return NULL;
}

jcntrl_input *jcntrl_input_rewind(jcntrl_input *item)
{
  jcntrl_executive *owner;

  owner = jcntrl_input_owner(item);
  return jcntrl_executive_get_input(owner);
}

jcntrl_input *jcntrl_input_at(jcntrl_input *item, int index)
{
  item = jcntrl_input_rewind(item);
  if (index >= 0) {
    item = jcntrl_input_next_port(item);
    while (item && index > 0) {
      item = jcntrl_input_next_port(item);
      --index;
    }
  } else {
    while (item && index < 0) {
      item = jcntrl_input_prev_port(item);
      ++index;
    }
  }
  return item;
}

static jcntrl_connection *jcntrl_input_connection(jcntrl_input *input)
{
  JCNTRL_ASSERT(input);
  return &input->port;
}

static jcntrl_output *jcntrl_input_get_upstream_port(jcntrl_input *input)
{
  jcntrl_connection *conn;
  conn = jcntrl_input_connection(input);
  return jcntrl_connection_get_upstream_port(conn);
}

jcntrl_executive *jcntrl_input_upstream_executive(jcntrl_input *input)
{
  jcntrl_output *upstream;

  upstream = jcntrl_input_get_upstream_port(input);
  if (!upstream) return NULL;

  return jcntrl_output_owner(upstream);
}

jcntrl_executive *jcntrl_input_owner(jcntrl_input *input)
{
  JCNTRL_ASSERT(input);
  return input->owner;
}

jcntrl_information *jcntrl_input_upstream_information(jcntrl_input *input)
{
  jcntrl_output *upstream;

  upstream = jcntrl_input_get_upstream_port(input);
  if (!upstream) return NULL;

  return jcntrl_output_information(upstream);
}

jcntrl_information *jcntrl_input_information(jcntrl_input *input)
{
  JCNTRL_ASSERT(input);
  return input->information;
}

jcntrl_shared_object *jcntrl_input_get_data_object_at(jcntrl_input *input_head,
                                                      int index)
{
  jcntrl_input *input;

  input = jcntrl_input_at(input_head, index);
  if (!input)
    return NULL;

  return jcntrl_input_get_data_object(input);
}

jcntrl_shared_object *jcntrl_input_get_data_object(jcntrl_input *input)
{
  jcntrl_information *info;

  info = jcntrl_input_upstream_information(input);
  if (!info)
    return NULL;

  return jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
}


void *(jcntrl_input_get_data_object_as)(jcntrl_input *input,
                                        const jcntrl_shared_object_data *cls)
{
  jcntrl_shared_object *obj;
  obj = jcntrl_input_get_data_object(input);
  if (!obj)
    return NULL;

  return jcntrl_shared_object_downcast_by_meta(cls, obj);
}

void *(jcntrl_input_get_data_object_at_as)(jcntrl_input *input, int index,
                                           const jcntrl_shared_object_data *cls)
{
  jcntrl_shared_object *obj;
  obj = jcntrl_input_get_data_object_at(input, index);
  if (!obj)
    return NULL;

  return jcntrl_shared_object_downcast_by_meta(cls, obj);
}
