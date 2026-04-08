/**
 * @file data.h
 * @brief Geometry over-all data
 */

#ifndef JUPITER_GEOMETRY_DATA_H
#define JUPITER_GEOMETRY_DATA_H

#include "defs.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @brief create new data
 * @return created data, NULL if allocation failed
 * @memberof geom_data
 */
JUPITER_GEOMETRY_DECL
geom_data *geom_data_new(geom_error *e);

/**
 * @brief delete data
 * @param d data to delete
 * @memberof geom_data
 *
 * Deletes all of d, its subsidiaries and managed pointers which is
 * added by `geom_data_add_pointer`.
 */
JUPITER_GEOMETRY_DECL
void geom_data_delete(geom_data *d);

/**
 * @brief Set deallocation responsibility to geom_data.
 * @param d geom_data which has deallocation responsibility of p
 * @param p data pointer to be deallocated if d is deallocated
 * @param dealloc dellocator function to be used for p
 * @return `GEOM_SUCCESS` if success, `GEOM_ERR_HAS_POINTER` if `d`
 *         already has pointer `p`, `GEOM_ERR_NOMEM` if allocation failed.
 * @memberof geom_data
 */
JUPITER_GEOMETRY_DECL
geom_error geom_data_add_pointer(geom_data *d, void *p,
                                 geom_deallocator *dealloc);

/**
 * @brief Delete pointer p which is contained in d.
 * @param d geom_data which has deallocation responsibility of p
 * @param p pointer deallocate.
 * @param dealloc dellocator function to be used for p
 * @return GEOM_SUCCESS if success, GEOM_ERR_POINTER_NOT_FOUND if p
 *         is not managed by d.
 * @memberof geom_data
 */
JUPITER_GEOMETRY_DECL
geom_error geom_data_del_pointer(geom_data *d, void *p);

/**
 * @brief Create new data element
 * @param parent data item belongs to
 * @param e if not NULL, set error info if happen.
 * @memberof geom_data_element
 * @return Pointer to created data element
 */
JUPITER_GEOMETRY_DECL
geom_data_element *geom_data_element_new(geom_data *parent, geom_error *e);

/**
 * @brief Add element elem to data.
 * @param elem element to add
 * @memberof geom_data
 */
JUPITER_GEOMETRY_DECL
geom_error geom_data_add_element(geom_data_element *elem);

/**
 * @brief Get list start of data elements
 * @param data data to be obtained from
 * @memberof geom_data
 */
JUPITER_GEOMETRY_DECL
geom_data_element *geom_data_get_element(geom_data *data);

/**
 * @brief get parent data of data element
 * @param element Data element to get from
 * @return data which element belongs to
 * @memberof geom_data_element
 */
JUPITER_GEOMETRY_DECL
geom_data *geom_data_element_parent(geom_data_element *element);

/**
 * @brief get master data of data element
 * @param element Data element to get from
 * @return master data which element belongs to
 * @memberof geom_data_element
 *
 * This function is equivalent to `geom_data_element_parent`.
 *
 * This function provides same program-readability with other
 * structures.
 */
JUPITER_GEOMETRY_DECL
geom_data *geom_data_element_master(geom_data_element *element);

JUPITER_GEOMETRY_DECL
geom_file_data *geom_data_element_get_file(geom_data_element *element);

JUPITER_GEOMETRY_DECL
geom_shape_data *geom_data_element_get_shape(geom_data_element *element);

JUPITER_GEOMETRY_DECL
geom_init_data *geom_data_element_get_init(geom_data_element *element);

JUPITER_GEOMETRY_DECL
geom_surface_shape_data *
geom_data_element_get_surface_shape(geom_data_element *element);

/**
 * @brief Remove data element from data
 * @param element element to remove.
 * @memberof geom_data
 *
 * Data pointing at element will be freed.
 */
JUPITER_GEOMETRY_DECL
void geom_data_element_delete(geom_data_element *element);

/**
 * @brief Get next data element of elem
 * @return next item, if elem is last element, returns NULL.
 * @memberof geom_data_element
 */
JUPITER_GEOMETRY_DECL
geom_data_element *geom_data_element_next(geom_data_element *elem);

/**
 * @brief Get previous data element of elem
 * @return previous item, if elem is first element, returns NULL.
 * @memberof geom_data_element
 */
JUPITER_GEOMETRY_DECL
geom_data_element *geom_data_element_prev(geom_data_element *elem);

/**
 * @brief Set name of geometry data element.
 * @param elem data element to be set
 * @param name NUL-terminated string to set
 * @memberof geom_data_element
 *
 * This function will not fail.
 *
 * If name is allocated, consider adding it to the data to be freed
 * on `geom_data_delete`.
 */
JUPITER_GEOMETRY_DECL
void geom_data_element_set_name(geom_data_element *elem, const char *name);

/**
 * @brief Get name of geometry data element.
 * @param elem data element to be get from.
 * @return name data
 * @memberof geom_data_element
 *
 * The default name is NULL (not set). Also,
 * `geom_data_element_set_name` allows set name to NULL. This function
 * returns that value as-is.
 */
JUPITER_GEOMETRY_DECL
const char *geom_data_element_get_name(geom_data_element *elem);

/**
 * @brief Set custom data.
 * @memberof geom_data
 * @param data Destination data
 * @param extra_data Data to be set
 * @param dealloc deallocator function for given `data`.
 * @retval GEOM_SUCCESS   No error
 * @retval GEOM_ERR_NOMEM Cannot allocate memory.
 *
 * If `dealloc` is not `NULL`, the `data` will be disposed by calling
 * `dealloc` when `geom_data_delete()` is called (in other words, the
 * `data` will be managed by the `geom_data`).
 */
JUPITER_GEOMETRY_DECL
geom_error geom_data_set_extra_data(geom_data *data, void *extra_data,
                                    geom_deallocator *dealloc);

/**
 * @brief Get custom data.
 * @memberof geom_data
 * @param data Data to get from.
 * @return user-defined custom data
 */
JUPITER_GEOMETRY_DECL
const geom_user_defined_data *geom_data_get_extra_data(geom_data *data);

/**
 * @brief Set custom data
 * @memberof geom_data_element
 * @param el Destination data element
 * @param extra_data Data to be set
 * @param dealloc deallocator function for given `data`
 * @retval GEOM_SUCCESS   No Error
 * @retval GEOM_ERR_NOMEM Cannot allocate memory
 *
 * Same as `geom_data_set_extra_data()` but set data for contained by
 * `geom_data_element`.
 *
 * @sa geom_data_set_extra_data()
 */
JUPITER_GEOMETRY_DECL
geom_error geom_data_element_set_extra_data(geom_data_element *el,
                                            void *extra_data,
                                            geom_deallocator *dealloc);

/**
 * @brief Get custom data
 * @memberof geom_data_element
 * @param el Data element to get from.
 * @return user-defined custom data.
 */
JUPITER_GEOMETRY_DECL
const geom_user_defined_data *
geom_data_element_get_extra_data(geom_data_element *el);

JUPITER_GEOMETRY_DECL_END

#endif
