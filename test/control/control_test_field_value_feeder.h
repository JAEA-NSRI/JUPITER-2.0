#ifndef JUPITER_TEST_CONTROL_TEST_FIELD_VALUE_FEEDER_H
#define JUPITER_TEST_CONTROL_TEST_FIELD_VALUE_FEEDER_H

#include "jupiter/control/static_array.h"
#include <jupiter/control/shared_object.h>
#include <jupiter/control/shared_object_priv.h>
#include <jupiter/control/defs.h>
#include <jupiter/control/executive_data.h>

JUPITER_CONTROL_DECL_BEGIN

/**
 * To obtain the pointer of jcntrl_executive or getting or setting
 * input value, Directly use it.
 */
struct field_value_feeder
{
  jcntrl_executive executive;
  jcntrl_static_double_array array;
};
#define field_value_feeder__ancestor jcntrl_executive
#define field_value_feeder__dnmem executive.jcntrl_executive__dnmem
JCNTRL_VTABLE_NONE(field_value_feeder);

JCNTRL_SHARED_METADATA_INIT_DECL(field_value_feeder);

int field_value_feeder_init(struct field_value_feeder *exe);
void field_value_feeder_clean(struct field_value_feeder *exe);

JUPITER_CONTROL_DECL_END

#endif
