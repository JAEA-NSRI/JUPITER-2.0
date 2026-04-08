/**
 * @file geometry/udata-priv.h
 * @ingroup Geometry
 * @brief Private functions for `geom_user_defined_data`
 */

#ifndef JUPITER_GEOMETRY_UDATA_PRIV_H
#define JUPITER_GEOMETRY_UDATA_PRIV_H

#include <stdlib.h>

#include "defs.h"
#include "struct_data.h"
#include "data.h"
#include "geom_assert.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @private
 * @memberof geom_user_defined_data
 * @brief Initialize user defined data
 * @param data Location to init
 */
static inline void
geom_user_defined_data_init(geom_user_defined_data *data)
{
  data->data = NULL;
  data->deallocator = NULL;
}

/**
 * @private
 * @memberof geom_user_defined_data
 * @brief Set user defined data
 * @param master Master data of geometry
 * @param data Location to set
 * @param p Pointer to user defined data
 * @param dealloc Deallocator of `p`
 * @retval GEOM_SUCCESS   No Error
 * @retval GEOM_ERR_NOMEM Memory allocation failure.
 *
 * Sets pointer `p` to `data` and register `p` to be managed by
 * `master`.
 */
static inline geom_error
geom_user_defined_data_set(geom_data *master,
                           geom_user_defined_data *data,
                           void *p, geom_deallocator *dealloc)
{
  geom_error e;

  GEOM_ASSERT(master);
  GEOM_ASSERT(data);

  e = geom_data_add_pointer(master, p, dealloc);
  if (e == GEOM_ERR_NOMEM) return e;

  data->data = p;
  data->deallocator = dealloc;

  return GEOM_SUCCESS;
}

/**
 * @private
 * @memberof geom_user_defined_data
 * @brief Deallocates user defined data
 * @param master Master data of geometry
 * @param data pointer to be freed
 */
static inline void
geom_user_defined_data_free(geom_data *master, geom_user_defined_data *data)
{
  geom_error e;

  GEOM_ASSERT(master);
  GEOM_ASSERT(data);

  e = geom_data_del_pointer(master, data->data);
  if (e != GEOM_SUCCESS && data->deallocator) {
    data->deallocator(data->data);
  }
  geom_user_defined_data_init(data);
}

JUPITER_GEOMETRY_DECL_END

#endif
