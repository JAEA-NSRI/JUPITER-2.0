#ifndef JUPITER_GRID_DATA_FEEDER_DATA_H
#define JUPITER_GRID_DATA_FEEDER_DATA_H

#include "control/defs.h"
#include "control/executive_data.h"
#include "control/shared_object_priv.h"
#include "struct.h"

struct jupiter_grid_data_feeder
{
  jcntrl_executive executive;
  parameter *prm;
  variable *val;
  material *mtl;
};
#define jupiter_grid_data_feeder__ancestor jcntrl_executive
#define jupiter_grid_data_feeder__dnmem executive.jcntrl_executive__dnmem
JCNTRL_VTABLE_NONE(jupiter_grid_data_feeder);

#endif
