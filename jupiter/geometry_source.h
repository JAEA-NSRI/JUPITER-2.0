/**
 * Control executive that converts geometry (may controlled) to geometry data
 */

#ifndef JUPITER_GEOMETRY_SOURCE_H
#define JUPITER_GEOMETRY_SOURCE_H

#include "common.h"
#include "control/defs.h"
#include "control/shared_object.h"
#include "geometry/defs.h"
#include "struct.h"

#ifdef __cplusplus
extern "C" {
#endif

struct jupiter_geometry_source;
typedef struct jupiter_geometry_source jupiter_geometry_source;

JUPITER_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jupiter_geometry_source);

JUPITER_DECL
jupiter_geometry_source *jupiter_geometry_source_new(void);
JUPITER_DECL
void jupiter_geometry_source_delete(jupiter_geometry_source *source);

JUPITER_DECL
jcntrl_executive *
jupiter_geometry_source_executive(jupiter_geometry_source *source);

JUPITER_DECL
jupiter_geometry_source *
jupiter_geometry_source_downcast(jcntrl_executive *executive);

JUPITER_DECL
int jupiter_geometry_source_set_geometry(jupiter_geometry_source *source,
                                         geom_data_element *element);
JUPITER_DECL
int jupiter_geometry_source_set_domain(jupiter_geometry_source *source,
                                       domain *cdo);

JUPITER_DECL
geom_data_element *
jupiter_geometry_source_get_geometry(jupiter_geometry_source *source);

JUPITER_DECL
domain *jupiter_geometry_source_get_domain(jupiter_geometry_source *source);

JUPITER_DECL
int jupiter_geometry_source_install(void);

#ifdef __cplusplus
}
#endif

#endif
