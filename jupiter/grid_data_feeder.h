#ifndef JUPITER_GRID_DATA_FEEDER_H
#define JUPITER_GRID_DATA_FEEDER_H

#include "control/defs.h"
#include "control/shared_object.h"
#include "struct.h"

#ifdef __cplusplus
extern "C" {
#endif

JUPITER_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jupiter_grid_data_feeder);

JUPITER_DECL
jupiter_grid_data_feeder *jupiter_grid_data_feeder_new(void);
JUPITER_DECL
void jupiter_grid_data_feeder_delete(jupiter_grid_data_feeder *feeder);

JUPITER_DECL
parameter *jupiter_grid_data_feeder_prm(jupiter_grid_data_feeder *feeder);
JUPITER_DECL
void jupiter_grid_data_feeder_set_prm(jupiter_grid_data_feeder *feeder,
                                      parameter *prm);

JUPITER_DECL
variable *jupiter_grid_data_feeder_val(jupiter_grid_data_feeder *feeder);
JUPITER_DECL
void jupiter_grid_data_feeder_set_val(jupiter_grid_data_feeder *feeder,
                                      variable *val);

JUPITER_DECL
material *jupiter_grid_data_feeder_mtl(jupiter_grid_data_feeder *feeder);
JUPITER_DECL
void jupiter_grid_data_feeder_set_mtl(jupiter_grid_data_feeder *feeder,
                                      material *mtl);

JUPITER_DECL
jcntrl_executive *
jupiter_grid_data_feeder_executive(jupiter_grid_data_feeder *feeder);

JUPITER_DECL
jupiter_grid_data_feeder *
jupiter_grid_data_feeder_downcast(jcntrl_shared_object *object);

#ifdef __cplusplus
}
#endif

#endif
