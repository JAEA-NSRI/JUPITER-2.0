#ifndef JUPITER_GEOMETRY_2D_PATH_H
#define JUPITER_GEOMETRY_2D_PATH_H

#include "defs.h"
#include "vector.h"

JUPITER_GEOMETRY_DECL_START

struct geom_2d_path_element_iterator
{
  const union geom_2d_path_element_entry *const entry; ///< Iterator
  const geom_vec2 origin_point;    ///< Origin point for next element
  const geom_vec2 last_move_point; ///< Start point of current path
};

JUPITER_GEOMETRY_DECL
geom_2d_path_data *geom_2d_path_data_new(geom_size_type nentry);

JUPITER_GEOMETRY_DECL
geom_2d_path_data *geom_2d_path_data_resize(geom_2d_path_data *data,
                                            geom_size_type nentry);

JUPITER_GEOMETRY_DECL
void geom_2d_path_data_delete(geom_2d_path_data *data);

JUPITER_GEOMETRY_DECL
geom_2d_path_element_iterator
geom_2d_path_data_get_element_iterator(const geom_2d_path_data *data);

/**
 * @brief Convert next path element
 * @param entry Entry point to parse
 * @param element Element to store the result.
 *
 * `#include "2d_path_element.h"` to get the definition of struct
 * `geom_2d_path_element`. It's public.
 */
JUPITER_GEOMETRY_DECL
geom_2d_path_element_iterator
geom_2d_path_element_next(geom_2d_path_element_iterator entry,
                          geom_2d_path_element *element);

/**
 * @brief Assign array of path elements to path data
 * @param data Path data assign
 * @param elements Array of path element data
 * @param number_of_elements Number of elements in @p elements
 * @return (number_of_elements + 1) if successfully assigned, the
 *         number of elements that could be assigned if partially
 *         assigned, and -1 if other error occuered.
 */
JUPITER_GEOMETRY_DECL
geom_size_type geom_2d_path_from_elements(geom_2d_path_data *data,
                                          const geom_2d_path_element *elements,
                                          geom_size_type number_of_elements);

JUPITER_GEOMETRY_DECL_END

#endif
