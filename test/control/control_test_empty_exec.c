#include "control_test_empty_exec.h"
#include "jupiter/control/defs.h"
#include "jupiter/control/shared_object_priv.h"

static void *empty_exec_dnv(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL_T(empty_exec, struct empty_exec, obj);
}

static void empty_exec_initf(jcntrl_shared_object_funcs *f)
{
  f->downcast = empty_exec_dnv;
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(empty_exec, empty_exec_initf)
