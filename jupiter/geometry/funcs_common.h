/**
 * @file   funcs_common.h
 *
 * @brief
 */

#ifndef JUPITER_GEOMETRY_FUNCS_COMMON_H
#define JUPITER_GEOMETRY_FUNCS_COMMON_H

#include "defs.h"

JUPITER_GEOMETRY_DECL_START

struct geom_funcs_common;

/**
 * @memberof geom_funcs_common
 * @brief Allocate data.
 * @param funcs function set
 * @param master Geometry data master
 * @param e If non-NULL value given, sets errors there.
 * @return Allocated pointer, NULL if allocation failed or **`funcs`
 *         does not define allocation**
 *
 * If allocation succeeded, sets `e` to ::GEOM_SUCCESS.
 *
 * If allocation failed, sets `e` to ::GEOM_ERR_NOMEM.
 *
 * If `funcs` does not define allocation, sets `e` to ::GEOM_SUCCESS,
 * and returns NULL.
 */
JUPITER_GEOMETRY_DECL
void *geom_funcs_common_allocate(const struct geom_funcs_common *funcs,
                                 geom_data *master, geom_error *e);

/**
 * @memberof geom_funcs_common
 * @brief Deallocate data.
 * @param funcs function set
 * @param data data to deallocate
 * @param master Geometry data master
 *
 * If `funcs` does not define deallocator, this function does nothing.
 */
JUPITER_GEOMETRY_DECL
void geom_funcs_common_deallocate(const struct geom_funcs_common *funcs,
                                  void *data, geom_data *master);

/**
 * @memberof geom_funcs_common
 * @brief Set data from builder
 * @param funcs function set
 * @param data  data set to.
 * @param ab    builder argument.
 * @retval ::GEOM_SUCCESS no error occured.
 * @retval (others) Each `init_set` function may define errors.
 *
 * If `data` is NULL, this function does nothing and returns `GEOM_SUCCESS`.
 *
 * If `funcs` does not define init_set, this function will abort with
 * `GEOM_ASSERT()` (Note that this will not occur if `NDEBUG` is
 * defined at compile time).
 */
JUPITER_GEOMETRY_DECL
geom_error geom_funcs_common_set_data(const struct geom_funcs_common *funcs,
                                      void *data, geom_args_builder *ab);

/**
 * @memberof geom_funcs_common
 * @brief Copy data
 * @param funcs function set.
 * @param a data to copy
 * @param master Geometry master data.
 * @param e if non-NULL value given, set errors
 * @return copied pointer, NULL if error occured
 * @error ::GEOM_ERR_NOMEM Cannot allocate memory
 *
 * If `funcs` does not define copy, this function will abort with
 * `GEOM_ASSERT()` (Note that this will not occur if `NDEBUG` is
 * defined at compile time).
 */
JUPITER_GEOMETRY_DECL
void *geom_funcs_common_copy_data(const struct geom_funcs_common *funcs,
                                  void *a, geom_data *master, geom_error *e);

JUPITER_GEOMETRY_DECL_END

#endif
