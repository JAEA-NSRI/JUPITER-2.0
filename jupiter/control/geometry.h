#ifndef JUPITER_CONTROL_GEOMETRY_H
#define JUPITER_CONTROL_GEOMETRY_H

#include "defs.h"
#include "shared_object.h"

#include <jupiter/geometry/defs.h>

JUPITER_CONTROL_DECL_BEGIN

JUPITER_CONTROL_DECL
JCNTRL_SHARED_METADATA_INIT_DECL(jcntrl_geometry);

JUPITER_CONTROL_DECL
jcntrl_geometry *jcntrl_geometry_new(void);
JUPITER_CONTROL_DECL
void jcntrl_geometry_delete(jcntrl_geometry *geometry);

JUPITER_CONTROL_DECL
jcntrl_data_object *jcntrl_geometry_data_object(jcntrl_geometry *geometry);
JUPITER_CONTROL_DECL
jcntrl_shared_object *jcntrl_geometry_object(jcntrl_geometry *geometry);
JUPITER_CONTROL_DECL
jcntrl_geometry *jcntrl_geometry_downcast(jcntrl_shared_object *object);

/*
 * geom_data_element can store *both* of geom_shape_data and geom_file_data at
 * same time. You can override has_shape or has_file to downstream executive can
 * choose correct one.
 */
JUPITER_CONTROL_DECL
int jcntrl_geometry_has_shape(jcntrl_geometry *geometry);

JUPITER_CONTROL_DECL
int jcntrl_geometry_shape_insideout_at(jcntrl_geometry *geometry, //
                                       int *ret, double x, double y, double z);

JUPITER_CONTROL_DECL
int jcntrl_geometry_has_file(jcntrl_geometry *geometry);

/**
 * Extent of geom_file_data_origin() and geom_file_data_size().
 */
JUPITER_CONTROL_DECL
const int *jcntrl_geometry_file_extent(jcntrl_geometry *geometry);
JUPITER_CONTROL_DECL
int jcntrl_geometry_file_repeat_count(jcntrl_geometry *geometry, int out[3]);
JUPITER_CONTROL_DECL
int jcntrl_geometry_file_repeat_offset(jcntrl_geometry *geometry, int out[3]);

JUPITER_CONTROL_DECL
int jcntrl_geometry_file_value(jcntrl_geometry *geometry, //
                               double *ret, int i, int j, int k);

/**
 * Extent of loaded file data.
 */
JUPITER_CONTROL_DECL
const int *jcntrl_geometry_data_extent(jcntrl_geometry *geometry);

JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_geometry_file_data(jcntrl_geometry *geometry);
JUPITER_CONTROL_DECL
jcntrl_data_array *jcntrl_geometry_file_load(jcntrl_geometry *geometry);

/**
 * Update transform matrix for each shape.
 */
JUPITER_CONTROL_DECL
int jcntrl_geometry_update_transform(jcntrl_geometry *geometry);

/**
 * While geom_data_element does not support copying, given pointer will not be
 * managed by jcntrl_geometry. This might be improved later.
 */
JUPITER_CONTROL_DECL
void jcntrl_geometry_set_data_element(jcntrl_geometry *geometry,
                                      geom_data_element *el);

JUPITER_CONTROL_DECL
geom_data_element *jcntrl_geometry_get_data_element(jcntrl_geometry *geometry);

JUPITER_CONTROL_DECL_END

#endif
