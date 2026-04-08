/**
 * @ingroup Geometry
 * @file   abuilder.h
 *
 * @brief variable length, dynamically typed arguments list builder
 */

#ifndef JUPITER_GEOMETRY_ABUILDER_H
#define JUPITER_GEOMETRY_ABUILDER_H

#include "defs.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @memberof geom_args_builder
 * @brief Goto next parameter for set
 * @param b Builder to move next
 * @return Next required parameter type
 */
JUPITER_GEOMETRY_DECL
geom_variant_type geom_args_builder_next(geom_args_builder *b);

/**
 * @memberof geom_args_builder
 * @brief Get requested type for current processing parameter.
 * @param b Builder to get
 * @return Requested type
 */
JUPITER_GEOMETRY_DECL
geom_variant_type geom_args_builder_get_type(geom_args_builder *b);

/**
 * @memberof geom_args_builder
 * @brief Check whether optional or mandatory for current processing parameter
 * @param b Builder to get
 * @retval 0 The parameter is mandatory
 * @retval non-0 The parameter is optional
 */
JUPITER_GEOMETRY_DECL
int geom_args_builder_is_optional(geom_args_builder *b);

/**
 * @memberof geom_args_builder
 * @brief Get current index value of the location
 * @param b Builder to get
 * @return Location (first item is 0)
 */
JUPITER_GEOMETRY_DECL
geom_size_type geom_args_builder_get_loc(geom_args_builder *b);

/**
 * @memberof geom_args_builder
 * @brief Get description of the parameter currently setting for.
 * @param b Builder to get
 * @return Description variant
 *
 * Returning variant should be a string.
 *
 * This function does not filter out variants typed others, but these
 * variants are not usable, so you can assume a string and you can
 * treat as "no description provided" if geom_variant_get_string()
 * returned NULL.
 */
JUPITER_GEOMETRY_DECL
const geom_variant *geom_args_builder_get_description(geom_args_builder *b);

/**
 * @memberof geom_args_builder
 * @brief Check argument value
 * @param b Builder
 * @param v Variant to check
 * @param errinfo If given, sets description of error.
 * @retval ::GEOM_SUCCESS No error
 * @retval ::GEOM_ERR_RANGE Current location or value invalid
 * @retval ::GEOM_ERR_NOMEM Cannot allocate memory
 * @retval ::GEOM_ERR_VARIANT_TYPE Invalid type
 */
JUPITER_GEOMETRY_DECL
geom_error geom_args_builder_check(geom_args_builder *b, const geom_variant *v,
                                   geom_variant *errinfo);

/**
 * @memberof geom_args_builder
 * @brief Set argument value
 * @param b Builder
 * @param v Variant to set
 * @retval ::GEOM_SUCCESS No error
 * @retval ::GEOM_ERR_NOMEM Cannot allocate memory
 */
JUPITER_GEOMETRY_DECL
geom_error geom_args_builder_set_value(geom_args_builder *b,
                                       const geom_variant *v);

/**
 * @memberof geom_args_builder
 * @brief Get current arguments list in the builder
 * @param b Builder to get
 * @return List of the variant
 *
 * Modification of the list is not allowed.
 */
JUPITER_GEOMETRY_DECL
geom_variant_list *geom_args_builder_get_list(geom_args_builder *b);

/**
 * @memberof geom_args_builder
 * @brief Get current argument value in the builder at specific location
 * @param b Builder to get
 * @param l Location to get for.
 *
 * @return Variant data for positioned at \p l, or NULL if not present.
 */
JUPITER_GEOMETRY_DECL
const geom_variant *geom_args_builder_value_at(geom_args_builder *b,
                                               geom_size_type l);

/**
 * @memberof geom_args_builder
 * @brief Set argumets from list
 * @param b Builder to set
 * @param l List to set
 * @param retcur If non-NULL given sets the location of error occured
 * @param errinfo Sets infomation of error if given
 *
 * @return First occured error.
 */
JUPITER_GEOMETRY_DECL
geom_error geom_args_builder_vlist(geom_args_builder *b, geom_variant_list *l,
                                   geom_variant_list **retcur,
                                   geom_variant *errinfo);

JUPITER_GEOMETRY_DECL_END

#endif
