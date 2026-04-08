/**
 * @file alloc_list.h
 * @brief Memory allocation manager
 *
 * This is not what they say 'garbage collector', all allocated memory
 * objects are kept until `geom_alloc_free` (for single object) or
 * `geom_alloc_free_all` (for all objects) is called explicitly.
 */

#ifndef JUPITER_GEOMETRY_ALLOC_LIST_H
#define JUPITER_GEOMETRY_ALLOC_LIST_H

#include "defs.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @memberof geom_alloc_list
 * @brief Create new allocation list
 * @return new list or `NULL` if allocation failed.
 */
JUPITER_GEOMETRY_DECL
geom_alloc_list *geom_alloc_list_new(void);

/**
 * @memberof geom_alloc_list
 * @brief Add a pointer n to list p
 * @param p Pointer to list
 * @param n Pointer to add
 * @param d deallocator function for n
 * @retval ::GEOM_SUCCESS No error
 * @retval ::GEOM_ERR_NOMEM Allocation failed
 * @retval ::GEOM_ERR_HAS_POINTER Already registered pointer
 *
 * If \p d is NULL, nothing will be done when free \p n.
 */
JUPITER_GEOMETRY_DECL
geom_error geom_alloc_add(geom_alloc_list *p, void *n, geom_deallocator *d);

/**
 * @memberof geom_alloc_list
 * @brief Add a pointer n to list p
 * @param p Pointer to list
 * @param n Pointer to remove
 * @retval ::GEOM_SUCCESS No error
 * @retval ::GEOM_ERR_POINTER_NOT_FOUND \p n is not registered.
 */
JUPITER_GEOMETRY_DECL
geom_error geom_alloc_free(geom_alloc_list *p, void *n);

/**
 * @memberof geom_alloc_list
 * @brief free all allocated pointers
 * @param allocs allocation lists
 */
JUPITER_GEOMETRY_DECL
void geom_alloc_free_all(geom_alloc_list *allocs);

/**
 * @memberof geom_alloc_list
 * @brief find if pointer n is registered
 * @return \p n if found, NULL if not found
 */
JUPITER_GEOMETRY_DECL
void *geom_alloc_find(geom_alloc_list *allocs, void *n);

JUPITER_GEOMETRY_DECL_END

#endif
