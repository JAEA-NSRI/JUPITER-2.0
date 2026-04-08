
#ifndef JUPITER_GEOMETRY_ABUILDER_PRIV_H
#define JUPITER_GEOMETRY_ABUILDER_PRIV_H

#include <stdarg.h>

#include "defs.h"
#include "abuilder.h"
#include "func_defs.h"

JUPITER_GEOMETRY_DECL_START

JUPITER_GEOMETRY_DECL
geom_args_builder *geom_args_builder_new(geom_args_nextf *nextf,
                                         geom_args_checkf *checker,
                                         geom_error *e);

JUPITER_GEOMETRY_DECL
void geom_args_builder_free(geom_args_builder *b);

JUPITER_GEOMETRY_DECL
geom_error geom_args_builder_vargs(geom_args_builder *bb, geom_variant *errinfo,
                                   va_list ap);

JUPITER_GEOMETRY_DECL_END

#endif
