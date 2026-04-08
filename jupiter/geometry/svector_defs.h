#ifndef JUPITER_GEOMETRY_SVECTOR_DEFS_H
#define JUPITER_GEOMETRY_SVECTOR_DEFS_H

#include "defs.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @ingroup Geometry
 * @brief Size as vector.
 */
struct geom_svec3
{
  geom_size_type x; ///< X size
  geom_size_type y; ///< Y size
  geom_size_type z; ///< Z size
};
typedef struct geom_svec3 geom_svec3;

JUPITER_GEOMETRY_DECL_END

#endif
