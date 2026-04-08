#ifndef JUPITER_GEOMETRY_2D_PATH_ELEMENT_H
#define JUPITER_GEOMETRY_2D_PATH_ELEMENT_H

#include "defs.h"
#include "geom_assert.h"
#include "vector.h"

JUPITER_GEOMETRY_DECL_START

struct geom_2d_path_move_element
{
  geom_vec2 to; ///< Destination point (can be relative)
};

struct geom_2d_path_line_element
{
  geom_vec2 to; ///< Destination point (can be relative)
};

/**
 * @ingroup Geometry
 * @brief Circular arc path element
 *
 * The radius is allowed to be smaller than the distance between
 * origin and destination points. In this case, the radius will be
 * scaled to match to the distance, i.e., the distance will be used as
 * the diameter of drawing circle.
 */
struct geom_2d_path_circ_element
{
  double radius; ///< Radius
  int large_arc_flag; ///< If nonzero, trace larger arc (i.e., > 180 degree)
  int sweep_flag; ///< If nonzero, trace in clockwise direction
  geom_vec2 to; ///< Destination point
};

struct geom_2d_path_end_element
{
  geom_vec2 start_p; ///< Path start point (last move or (0, 0) otherwise)
};

union geom_2d_path_element_data
{
  geom_2d_path_move_element move;
  geom_2d_path_line_element line;
  geom_2d_path_circ_element circ;
  geom_2d_path_end_element end;
};

/*
 * Describes single path element
 */
struct geom_2d_path_element
{
  geom_2d_path_element_type type;
  geom_vec2 origin;
  union geom_2d_path_element_data data;
};

/**
 * @brief Get max number of required entries per path element.
 * @return Number of max entry
 *
 * The value is constant
 */
JUPITER_GEOMETRY_DECL
geom_size_type geom_2d_path_get_max_entry(void);

/**
 * @brief Get the number of required entries for given element type
 * @param element_type Element type to get for
 * @return The number of required entries for given element type
 * @retval 0 Error (invalid type)
 *
 * Any element requires least 1 entry. The return value 0 is error
 * (invalid type).
 */
JUPITER_GEOMETRY_DECL
geom_size_type geom_2d_path_get_nentry1(geom_2d_path_element_type element_type);

/**
 * @brief Get the number of required entries for given elemnt data array
 * @param elements Array of 2D path elements
 * @param number_of_elements Number of elements
 * @return The total number of required entries for given elements
 * @retval 0 Error (invalid type)
 */
JUPITER_GEOMETRY_DECL
geom_size_type geom_2d_path_get_nentry(const geom_2d_path_element *elements,
                                       geom_size_type number_of_elements);

static inline geom_2d_path_element_type
geom_2d_path_element_get_type(geom_2d_path_element *element)
{
  return element->type;
}

/**
 * @brief Get the move entry data if it stores
 * @param element Element data to get from
 * @return move element data if it stores that.
 *
 * This function does not have any purpose that improves readability
 * since the struct content is public. The user should have
 * responsibility for consistency.
 */
static inline geom_2d_path_move_element *
geom_2d_path_get_move_element(geom_2d_path_element *element)
{
  if (!element)
    return NULL;
  switch (geom_2d_path_element_get_type(element)) {
  case GEOM_2D_PATH_MOVE:
  case GEOM_2D_PATH_RELMOVE:
    return &element->data.move;
  default:
    return NULL;
  }
}

/**
 * @brief Get the line entry data if it stores
 * @param element Element data to get from
 * @return move element data if it stores that.
 *
 * This function does not have any purpose that improves readability
 * since the struct content is public. The user should have
 * responsibility for consistency.
 */
static inline geom_2d_path_line_element *
geom_2d_path_get_line_element(geom_2d_path_element *element)
{
  if (!element)
    return NULL;
  switch (element->type) {
  case GEOM_2D_PATH_LINE:
  case GEOM_2D_PATH_RELLINE:
    return &element->data.line;
  default:
    return NULL;
  }
}

/**
 * @brief Get the circ entry data if it stores
 * @param element Element data to get from
 * @return move element data if it stores that.
 *
 * This function does not have any purpose that improves readability
 * since the struct content is public. The user should have
 * responsibility for consistency.
 */
static inline geom_2d_path_circ_element *
geom_2d_path_get_circ_element(geom_2d_path_element *element)
{
  if (!element)
    return NULL;
  switch(element->type) {
  case GEOM_2D_PATH_CIRC:
  case GEOM_2D_PATH_RELCIRC:
    return &element->data.circ;
  default:
    return NULL;
  }
}

/**
 * @brief Get the circ entry data if it stores
 * @param element Element data to get from
 * @return move element data if it stores that.
 *
 * This function does not have any purpose that improves readability
 * since the struct content is public. The user should have
 * responsibility for consistency.
 */
static inline geom_2d_path_end_element *
geom_2d_path_get_end_element(geom_2d_path_element *element)
{
  if (!element)
    return NULL;
  switch(element->type) {
  case GEOM_2D_PATH_END:
    return &element->data.end;
  default:
    return NULL;
  }
}

static inline void
geom_2d_path_set_absmove_element(geom_2d_path_element *element, geom_vec2 to)
{
  GEOM_ASSERT(element);
  element->type = GEOM_2D_PATH_MOVE;
  element->data.move.to = to;
}

static inline void
geom_2d_path_set_relmove_element(geom_2d_path_element *element, geom_vec2 to)
{
  GEOM_ASSERT(element);
  element->type = GEOM_2D_PATH_RELMOVE;
  element->data.move.to = to;
}

static inline void
geom_2d_path_set_absline_element(geom_2d_path_element *element, geom_vec2 to)
{
  GEOM_ASSERT(element);
  element->type = GEOM_2D_PATH_LINE;
  element->data.line.to = to;
}

static inline void
geom_2d_path_set_relline_element(geom_2d_path_element *element, geom_vec2 to)
{
  GEOM_ASSERT(element);
  element->type = GEOM_2D_PATH_RELLINE;
  element->data.line.to = to;
}

static inline void
geom_2d_path_set_abscirc_element(geom_2d_path_element *element, double radius,
                                 int larger_arc, int sweep, geom_vec2 to)
{
  GEOM_ASSERT(element);
  element->type = GEOM_2D_PATH_CIRC;
  element->data.circ.radius = radius;
  element->data.circ.large_arc_flag = larger_arc;
  element->data.circ.sweep_flag = sweep;
  element->data.circ.to = to;
}

static inline void
geom_2d_path_set_relcirc_element(geom_2d_path_element *element, double radius,
                                 int larger_arc, int sweep, geom_vec2 to)
{
  GEOM_ASSERT(element);
  element->type = GEOM_2D_PATH_RELCIRC;
  element->data.circ.radius = radius;
  element->data.circ.large_arc_flag = larger_arc;
  element->data.circ.sweep_flag = sweep;
  element->data.circ.to = to;
}

static inline void
geom_2d_path_set_end_element(geom_2d_path_element *element)
{
  GEOM_ASSERT(element);
  element->type = GEOM_2D_PATH_END;
}

JUPITER_GEOMETRY_DECL_END

#endif
