
#include "connection.h"
#include "output.h"
#include "input.h"
#include "error.h"

#include "executive_data.h"

int jcntrl_connection_is_head(jcntrl_connection *conn)
{
  JCNTRL_ASSERT(conn);

  return jcntrl_connection_rewind(conn) == conn;
}

jcntrl_connection *jcntrl_connection_next(jcntrl_connection *conn)
{
  jcntrl_connection *next;

  JCNTRL_ASSERT(conn);
  next = jcntrl_connection_entry(geom_list_next(&conn->list));
  if (jcntrl_connection_is_head(next)) {
    return NULL;
  }
  return next;
}

jcntrl_connection *jcntrl_connection_prev(jcntrl_connection *conn)
{
  jcntrl_connection *prev;

  JCNTRL_ASSERT(conn);
  prev = jcntrl_connection_entry(geom_list_prev(&conn->list));
  if (jcntrl_connection_is_head(prev)) {
    return NULL;
  }
  return prev;
}

jcntrl_connection *jcntrl_connection_rewind(jcntrl_connection *conn)
{
  jcntrl_output *upstream;

  JCNTRL_ASSERT(conn);

  upstream = jcntrl_connection_get_upstream_port(conn);
  if (upstream) {
    return jcntrl_output_downstreams(upstream);
  }
  return NULL;
}

jcntrl_input *jcntrl_connection_get_downstream_port(jcntrl_connection *conn)
{
  JCNTRL_ASSERT(conn);

  if (!jcntrl_connection_is_head(conn)) {
    return jcntrl_input_connection_entry(conn);
  }
  return NULL;
}

jcntrl_output *jcntrl_connection_get_upstream_port(jcntrl_connection *conn)
{
  JCNTRL_ASSERT(conn);

  return conn->upstream;
}
