#ifndef JUPITER_GEOMETRY_ENUMUTIL_H
#define JUPITER_GEOMETRY_ENUMUTIL_H

#include "defs.h"

JUPITER_GEOMETRY_DECL_START

/*
 * These functions returns NULL for unexpeced or uneumerated values
 * (e.g. INVALID, user-installed models)
 */

JUPITER_GEOMETRY_DECL
const char *geom_variant_type_to_str(geom_variant_type value);

JUPITER_GEOMETRY_DECL
const char *geom_vof_phase_to_str(geom_vof_phase value);

JUPITER_GEOMETRY_DECL
const char *geom_data_operator_to_str(geom_data_operator value);

JUPITER_GEOMETRY_DECL
const char *geom_shape_operator_to_str(geom_shape_operator value);

JUPITER_GEOMETRY_DECL
const char *geom_shape_to_str(geom_shape value);

JUPITER_GEOMETRY_DECL
const char *geom_shape_type_to_str(geom_shape_type value);

JUPITER_GEOMETRY_DECL
const char *geom_surface_shape_to_str(geom_surface_shape value);

JUPITER_GEOMETRY_DECL
const char *geom_2d_path_element_type_to_str(geom_2d_path_element_type value);

JUPITER_GEOMETRY_DECL
const char *geom_init_func_to_str(geom_init_func value);

JUPITER_GEOMETRY_DECL_END

#endif
