/**
 * @file   infomap.h
 * @brief  Infomation map.
 */

#ifndef JUPITER_GEOMETRY_INFOMAP_H
#define JUPITER_GEOMETRY_INFOMAP_H

#include "defs.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @memberof geom_info_map
 * @brief Create new information map.
 * @param e If not NULL set error informations
 * @return Created map, NULL if failed.
 *
 * Returning pointer is map head.
 */
JUPITER_GEOMETRY_DECL
geom_info_map *geom_info_map_new(geom_error *e);

/**
 * @memberof geom_info_map
 * @brief Remove a single map entry
 * @param map Map entry to remove.
 */
JUPITER_GEOMETRY_DECL
void geom_info_map_delete_item(geom_info_map *map);

/**
 * @memberof geom_info_map
 * @brief Delete all map entries
 * @param head Map head to remove.
 */
JUPITER_GEOMETRY_DECL
void geom_info_map_delete_all(geom_info_map *head);

/**
 * @memberof geom_info_map
 * @brief Add parameter information to map
 * @param head Head of map
 * @param value Value of parameter
 * @param description Description of parameter
 * @param unit Unit of parameter
 * @param e If non-NULL value, sets error info.
 *
 * Geometry library has no concept on unit of length.
 * Geometry library uses 'degree' for unit of angular and
 * pre-defined shapes puts 'deg'.
 *
 * Pre-defined shapes and initialization function sets 'L' for length
 * and 'I' for initialization base unit. You might replace it with
 * your appropriate unit (such as 'm' (meter) for 'L' and 'K' (kelvin)
 * for 'I') using `geom_info_map_convert_unit()`.
 *
 * Specified value, descpription and unit will be copied.
 */
JUPITER_GEOMETRY_DECL
geom_info_map *geom_info_map_append(geom_info_map *head,
                                    const geom_variant *value,
                                    const char *description, const char *unit,
                                    geom_error *e);

/**
 * @memberof geom_info_map
 * @brief Test specified entry is map head
 * @param map information map entry to test.
 * @retval non-0 map is head
 * @retval 0     map is not head
 */
JUPITER_GEOMETRY_DECL
int geom_info_map_is_head(geom_info_map *map);

/**
 * @memberof geom_info_map
 * @brief Get next entry of `p`
 * @param p Map entry to get
 * @return Next entry, 'head' entry if `p` is the last entry.
 */
JUPITER_GEOMETRY_DECL
geom_info_map *geom_info_map_next(geom_info_map *p);

/**
 * @memberof geom_info_map
 * @brief Get Previous entry of `p`
 * @param p Map entry to get
 * @return Previous entry, 'head' entry if `p` is the first entry.
 */
JUPITER_GEOMETRY_DECL
geom_info_map *geom_info_map_prev(geom_info_map *p);

/**
 * @memberof geom_info_map
 * @brief Get description string for specified information map entry.
 * @param p information map entry to get from
 * @return Description data
 */
JUPITER_GEOMETRY_DECL
const char *geom_info_map_get_description(geom_info_map *p);

/**
 * @memberof geom_info_map
 * @brief Get unit string for specified information map entry
 * @param p information map entry to get from
 * @return Unit data
 */
JUPITER_GEOMETRY_DECL
const char *geom_info_map_get_unit(geom_info_map *p);

/**
 * @memberof geom_info_map
 * @brief get value part of information map entry
 * @param p information map entry to get from
 * @return Value data
 */
JUPITER_GEOMETRY_DECL
const geom_variant *geom_info_map_get_value(geom_info_map *p);

/**
 * @memberof geom_info_map
 * @brief Replace and join the units used in the infomap.
 * @param buf Pointer to buffer to allocate and store the result.
 * @param base Base unit if present.
 * @param length_unit Unit of length
 * @param geometry_expr In Geometry library expression
 *        (used in geom_info_map_get_unit)
 * @return Number of bytes to written, -1 if allocation failed.
 *
 * If @p base is NULL, "" or "1" (exact) replaces "T" with
 * "1" if content of numerator will become empty, "" otherwise.
 *
 * If @p base is "" or "1" (exact), treat it has no unit.
 *
 * @p length_unit may be "m" for meter, "in" for inches for example.
 *
 * If @p length_unit is not given, replace will not be done.
 *
 * Replaces 'L' in @p geometry_expr to content of length_unit.
 *
 * @note Do not forget to free buf after use.
 *
 * @note Content of `buf` is undefined if any errors occured.
 *       (You must not expect something)
 */
JUPITER_GEOMETRY_DECL
int geom_info_map_convert_unit(char **buf, const char *base,
                               const char *length_unit,
                               const char *geometry_expr);

JUPITER_GEOMETRY_DECL_END

#endif
