/**
 * @file struct_data.h
 * @ingroup Geometry
 * @private
 * @brief Definition of struct data.
 *
 * You should not include this header outside of Geometry library, use
 * access functions defined for each structures.
 */

#ifndef JUPITER_GEOMETRY_STRUCT_DATA_H
#define JUPITER_GEOMETRY_STRUCT_DATA_H

#include "defs.h"
#include "func_data.h"
#include "list.h"
#include "vector.h"
#include "svector.h"
#include "mat43.h"
#include "func_defs.h"
#include "rbtree.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @ingroup Geometry
 * @brief Storage for any user defined data
 *
 * The user can a piece of extra data to extend data in geometry
 * library.
 */
struct geom_user_defined_data
{
  void *data;    ///< Pointer to actual data
  geom_deallocator *deallocator; ///< Deallocator of the `data`.
};

/**
 * @ingroup Geometry
 * @brief Surface shape path element single entry
 */
union geom_2d_path_element_entry
{
  geom_2d_path_element_type t;
  int i;
  double d;
};

/**
 * @ingroup Geometry
 * @brief Surface shape path data
 */
struct geom_2d_path_data
{
  geom_size_type number_entry;
  union geom_2d_path_element_entry entry[];
};

/**
 * @ingroup Geometry
 * @brief Compound 2D shape data element
 */
struct geom_surface_shape_element
{
  struct geom_list list; ///< List entry
  geom_surface_shape_data *parent; ///< Parent data
  const geom_surface_shape_funcs *funcs; ///< Set fo shape function
  geom_shape_operator op; ///< Shape combine operator
  void *data; ///< Extra shape-specific data
  geom_user_defined_data extra_data; ///< Extra user defined data
};

/**
 * @ingroup Geometry
 * @brief Compound 2D shape data
 */
struct geom_surface_shape_data
{
  geom_data_element *parant; ///< Parent data element
  geom_surface_shape_element head; ///< Head of 2D shape element
  geom_user_defined_data extra_data; ///< Extra user defined data
};

/**
 * @ingroup Geometry
 * @brief Shape element
 *
 * Stores single shape entry.
 */
struct geom_shape_element
{
  struct geom_list list;  ///< List entry
  geom_shape_data *parent; ///< Parent shape collection data
  const geom_shape_funcs *funcs; ///< Set of shape function
  geom_shape_operator op_effec; ///< Effective shape combine operator
  geom_shape_operator op_orig;  ///< Original shape combine operator
  geom_mat43 transform;    ///< Shape tranformation
  int copied;             ///< Generated copy by transformation
  void *data;             ///< Extra shape-specific data
  geom_user_defined_data extra_data;  ///< Extra user defined data.
};

/**
 * @ingroup Geometry
 * @brief Collection of shape elements
 */
struct geom_shape_data
{
  geom_data_element *parent;   ///< Parent data element
  geom_vec3  origin; ///< Global origin
  geom_vec3  offset; ///< Global offset
  geom_svec3 repeat; ///< Global repetition
  geom_shape_element  element; ///< Head of shape element
  int need_update_transform;   ///< Flag to requirement of update transform
                               ///  matrix
  int nsub_cell;     ///< Number of subcells while
                     ///  calculating VOF from shape
  geom_user_defined_data extra_data; ///< Extra user defined data.
};

/**
 * @ingroup Geometry
 * @brief Initialization Element item.
 *
 * Defines a single item of component initialization definitions.
 *
 * @sa geom_init_data
 */
struct geom_init_element
{
  struct geom_list list;
  geom_init_data *parent;
  const geom_init_funcs *funcs;
  int component_id;
  geom_user_defined_data component_data;
  const char *component_name;
  geom_data_operator op;
  double threshold;
  void *data;
  geom_user_defined_data extra_data; ///< Extra user defined data.
};

/**
 * @ingroup Geometry
 * @brief Initialization Data.
 *
 * Defines a collection of component intialization definitions.
 *
 * @sa geom_init_element
 */
struct geom_init_data
{
  /* This structure separated for expanding it in the future. */
  geom_data_element *parent;
  geom_init_element  element; ///< head of init element
  geom_user_defined_data extra_data;
};

/**
 * @ingroup Geometry
 * @brief File data
 *
 * Keep file name(s) and basic structured data
 *
 * A single file data can be stored in a data element, storing
 * multiple files are not supported.
 *
 * Geometry library does nothing with files specified here, so you
 * must implement to handle these data.
 *
 * If this does not match your needs, you can add an extra data to
 * `geom_data_element` with `geom_data_element_set_extra_data()`, for
 * keeping file information.
 *
 * `geom_file_data` can also have extra data to extend this structure.
 */
struct geom_file_data
{
  geom_data_element *parent;

  const char *file_name;
  const char *alt_file_name;

  geom_svec3 origin;  // Originating point (in cell)
  geom_svec3 offset;  // Offset for repeating (in cell)
  geom_svec3 size;    // Size of file.
  geom_svec3 repeat;  // Repititon

  geom_user_defined_data extra_data;
};

/**
 * @ingroup Geometry
 * @brief Data element
 */
struct geom_data_element
{
  struct geom_list list;

  const char *name;                    // Name of geometry
  geom_file_data *file;
  geom_shape_data *shape;
  geom_surface_shape_data *surface_shape;

  geom_init_data *init_data;
  geom_data *parent;

  geom_user_defined_data extra_data;
};

/**
 * @ingroup Geometry
 * @brief Geomtry master data
 */
struct geom_data
{
  geom_data_element *elements;
  geom_alloc_list   *allocs;          // allocations
  geom_user_defined_data extra_data;
};

/**
 * @typedef struct geom_shape_data geom_shape_data
 * @copybrief geom_shape_data
 */
/**
 * @typedef struct geom_args_builder geom_args_builder
 * @copybrief geom_args_builder
 */

JUPITER_GEOMETRY_DECL_END

#endif
