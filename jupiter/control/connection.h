
#ifndef JUPITER_CONTROL_CONNECTION_H
#define JUPITER_CONTROL_CONNECTION_H

#include "defs.h"

JUPITER_CONTROL_DECL_BEGIN

JUPITER_CONTROL_DECL
int jcntrl_connection_is_head(jcntrl_connection *conn);
JUPITER_CONTROL_DECL
jcntrl_connection *jcntrl_connection_next(jcntrl_connection *conn);
JUPITER_CONTROL_DECL
jcntrl_connection *jcntrl_connection_prev(jcntrl_connection *conn);
JUPITER_CONTROL_DECL
jcntrl_connection *jcntrl_connection_rewind(jcntrl_connection *conn);

JUPITER_CONTROL_DECL
jcntrl_input *jcntrl_connection_get_downstream_port(jcntrl_connection *conn);
JUPITER_CONTROL_DECL
jcntrl_output *jcntrl_connection_get_upstream_port(jcntrl_connection *conn);

JUPITER_CONTROL_DECL_END

#endif
