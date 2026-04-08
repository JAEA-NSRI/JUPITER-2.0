#ifndef CONTROL_TEST_EMPTY_EXEC_H
#define CONTROL_TEST_EMPTY_EXEC_H

/* Empty executive for just using jcntrl_executive */

#include "jupiter/control/executive.h"
#include "jupiter/control/executive_data.h"
#include "jupiter/control/shared_object.h"
#include "jupiter/control/shared_object_priv.h"

struct empty_exec
{
  jcntrl_executive executive;
};
#define empty_exec__ancestor jcntrl_executive
#define empty_exec__dnmem executive.jcntrl_executive__dnmem
JCNTRL_VTABLE_NONE(empty_exec);

JCNTRL_SHARED_METADATA_INIT_DECL(empty_exec);

static inline void empty_exec_init(struct empty_exec *e)
{
  jcntrl_executive_init(&e->executive, empty_exec_metadata_init());
}

#endif
