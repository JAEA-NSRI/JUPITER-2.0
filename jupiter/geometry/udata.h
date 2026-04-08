/**
 * @file geometry/udata.h
 * @brief Handle additinal data to keep with `geom_data`.
 * @ingroup Geometry
 *
 * This header provides reader only.
 *
 * To set additional data, use functions given by destination structure.
 * (ex. `geom_data_set_extra_data()` for `geom_data`)
 */

#ifndef JUPITER_GEOMETRY_UDATA_H
#define JUPITER_GEOMETRY_UDATA_H

#include "defs.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @memberof geom_user_defined_data
 * @brief Get actual data for extra user defined data.
 * @param d User defined data structure to get
 * @return Pointer to actual data.
 *
 * The data will be cast to non-const pointer.
 */
JUPITER_GEOMETRY_DECL
void *geom_user_defined_data_get(const geom_user_defined_data *d);

JUPITER_GEOMETRY_DECL_END

#endif
