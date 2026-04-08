
#ifndef JUPITER_GEOMETRY_SHP_SPECIALS_H
#define JUPITER_GEOMETRY_SHP_SPECIALS_H

#include "defs.h"
#include "func_defs.h"

JUPITER_GEOMETRY_DECL_START

JUPITER_GEOMETRY_DECL
geom_error geom_install_shape_specials(void);

/*
 * These declaration is to refer from shape.c.
 *
 * You should not do this for regular shapes because the API does not
 * interact to geom_shape_funcs directly.
 */
JUPITER_GEOMETRY_DECL_PRIVATE
extern const geom_shape_funcs *geom_shape_gst;

JUPITER_GEOMETRY_DECL_PRIVATE
extern const geom_shape_funcs *geom_shape_ged;

JUPITER_GEOMETRY_DECL_PRIVATE
extern const geom_shape_funcs *geom_shape_comb;

JUPITER_GEOMETRY_DECL_END

#endif
