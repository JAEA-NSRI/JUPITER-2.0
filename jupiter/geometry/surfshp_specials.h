#ifndef JUPITER_GEOMETRY_SURFSHP_SPECIALS_H
#define JUPITER_GEOMETRY_SURFSHP_SPECIALS_H

#include "defs.h"
#include "func_data.h"

JUPITER_GEOMETRY_DECL_START

JUPITER_GEOMETRY_DECL
geom_error geom_install_surface_shape_specials(void);

JUPITER_GEOMETRY_DECL_PRIVATE
extern const geom_surface_shape_funcs *geom_surface_shape_comb;

JUPITER_GEOMETRY_DECL_END

#endif
