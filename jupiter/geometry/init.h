/**
 * @file init.h
 * @ingroup Geometry
 * @brief Functions to define component initializations
 */

#ifndef JUPITER_GEOMETRY_INIT_H
#define JUPITER_GEOMETRY_INIT_H

#include "defs.h"

JUPITER_GEOMETRY_DECL_START

/**
 * @memberof geom_init_data
 * @brief Create new component initialization definition set.
 * @param parent Parent data element
 * @param e If not NULL, sets error to given location.
 * @return Newly allocated pointer, NULL if allocation failed.
 */
JUPITER_GEOMETRY_DECL
geom_init_data *geom_init_data_new(geom_data_element *parent, geom_error *e);

/**
 * @memberof geom_init_data
 * @brief Remove all initialization elements and data set.
 * @param data Data to delete.
 */
JUPITER_GEOMETRY_DECL
void geom_init_data_delete(geom_init_data *data);

/**
 * @memberof geom_init_data
 * @brief Parent data element of initialization set.
 * @param data Data to get.
 * @return Pointer to parent data element.
 */
JUPITER_GEOMETRY_DECL
geom_data_element *geom_init_data_parent(geom_init_data *data);

/**
 * @memberof geom_init_data
 * @brief Master data of initialization set.
 * @param data Data to get
 * @return Pointer to master data.
 */
JUPITER_GEOMETRY_DECL
geom_data *geom_init_data_master(geom_init_data *data);

/**
 * @memberof geom_init_data
 * @brief Add initizaition element to set.
 * @param el Initialization element to add.
 *
 * This function will not fail.
 */
JUPITER_GEOMETRY_DECL
void geom_init_data_add_element(geom_init_element *el);

/**
 * @memberof geom_init_data
 * @brief Get first initialization element
 * @param data Initialization data to get from.
 * @return First item, or NULL if no elements have been set.
 */
JUPITER_GEOMETRY_DECL
geom_init_element *geom_init_data_get_element(geom_init_data *data);

/**
 * @memberof geom_init_element
 * @brief Create new initialization definition element.
 * @param data parent initialization data set.
 * @param e If not NULL, sets error to given location.
 * @return Allocatad pointer, NULL if failed.
 */
JUPITER_GEOMETRY_DECL
geom_init_element *geom_init_element_new(geom_init_data *data, geom_error *e);

/**
 * @memberof geom_init_element
 * @brief Get parent initialization definition set
 * @param e Initialazaition element to get from.
 * @return Pointer to parent initialization definition set.
 */
JUPITER_GEOMETRY_DECL
geom_init_data *geom_init_element_parent(geom_init_element *e);

/**
 * @memberof geom_init_element
 * @brief Master data of initialization element
 * @param e Initialization element to get from.
 * @return Pointer to master data.
 */
JUPITER_GEOMETRY_DECL
geom_data *geom_init_element_master(geom_init_element *e);

/**
 * @memberof geom_init_element
 * @brief Delete initialization element
 * @param e Element to delete.
 */
JUPITER_GEOMETRY_DECL
void geom_init_element_delete(geom_init_element *e);

/**
 * @memberof geom_init_element
 * @brief Set component data to initialization data element
 * @param e element to set
 * @param comp_id Component ID number
 * @param comp_data Component data.
 * @param comp_data_dealloc Deallocator function to free component data.
 * @param comp_name NUL-terminated, name of component.
 *
 * Geometry library does not define the meaning of component IDs.
 * This parameter should be used to determine what comp_data means.
 *
 * The `comp_data` gets part of the initialization data element `e`.
 * The `comp_data` will also be freed when new `comp_data` is set, `e`
 * gets freed, or parent `data` freed.
 *
 * The `comp_name` will be copied on set.
 */
JUPITER_GEOMETRY_DECL
geom_error geom_init_element_set_component(geom_init_element *e, int comp_id,
                                           void *comp_data,
                                           geom_deallocator *comp_data_dealloc,
                                           const char *comp_name);

/**
 * @memberof geom_init_element
 * @brief Set threshold value on VOF given from shapes, file, etc.
 * @param e Element to set.
 * @param threshold The threshold value.
 *
 * If `threshold` is positive, selects the region where its VOF value
 * is greater than or equal to specified value.
 *
 * If `threshold` is negative, selects the region where its VOF value is
 * less than absolute value of specified value.
 *
 * If `threshold` is 0, selects the region where its VOF value is
 * greater than 0 (note that 0 is not included).
 *
 * If the aboslute value of `threshold` is greater than 1 or NaN,
 * `threshold` will be no be affect and VOF value will act as factor
 * to the initializing value.
 */
JUPITER_GEOMETRY_DECL
void geom_init_element_set_threshold(geom_init_element *e, double threshold);

/**
 * @memberof geom_init_element
 * @brief Set data operator
 * @param e Element to set.
 * @param op The operator value.
 */
JUPITER_GEOMETRY_DECL
void geom_init_element_set_operator(geom_init_element *e,
                                    geom_data_operator op);

/**
 * @memberof geom_init_element
 * @brief Get user provided component data
 * @param e Element to get.
 * @return Pointer user provided component data.
 *
 * Returning pointer is always *not* NULL.
 *
 * Use `geom_user_defined_data_get()` to get actual pointer.
 */
JUPITER_GEOMETRY_DECL
const geom_user_defined_data *
geom_init_element_get_comp_data(geom_init_element *e);

/**
 * @memberof geom_init_element
 * @brief Get name of component
 * @param e Element to get.
 * @return Component name which is provided at
 * `geom_init_element_set_component()` function.
 *
 * Do not `free()` returned pointer.
 */
JUPITER_GEOMETRY_DECL
const char *geom_init_element_get_comp_name(geom_init_element *e);

/**
 * @memberof geom_init_element
 * @brief Get componend ID set to the element
 * @param e Element to get from.
 * @return Component ID which is provided at
 * `geom_init_element_set_component()` function.
 */
JUPITER_GEOMETRY_DECL
int geom_init_element_get_comp_id(geom_init_element *e);

/**
 * @memberof geom_init_element
 * @brief Get threshold value set to the element.
 * @param e Element to get from.
 * @return Threshold value which is set to the element.
 */
JUPITER_GEOMETRY_DECL
double geom_init_element_get_threshold(geom_init_element *e);

/**
 * @memberof geom_init_element
 * @brief Get operator value set to the element.
 * @param e Element to get from.
 * @return operator value which is set to the element.
 */
JUPITER_GEOMETRY_DECL
geom_data_operator geom_init_element_get_operator(geom_init_element *e);

/**
 * @memberof geom_init_element
 * @brief Get function which will be used for the element.
 * @param e Element to get from.
 * @return initialization function enumeretor which is set.
 */
JUPITER_GEOMETRY_DECL
geom_init_func geom_init_element_get_func(geom_init_element *e);

/**
 * @memberof geom_init_element
 * @brief Get function data information, set to the element.
 * @param e Element to get from.
 * @return Information map of function data set in the specified element
 *
 * After use, use `geom_info_map_delete()` to free allocated data.
 */
JUPITER_GEOMETRY_DECL
geom_info_map *geom_init_element_func_info(geom_init_element *e);

/**
 * @memberof geom_init_element
 * @brief Set initialization function to element.
 * @param e Element to set
 * @param init_func
 * @return `GEOM_SUCCESS` if success, `GEOM_ERR_NOMEM` if allocation failed.

 * @relates geom_init_args_builder_new
 */
JUPITER_GEOMETRY_DECL
geom_error geom_init_element_set_func(geom_init_element *e,
                                      geom_init_args_builder *init_func);

/**
 * @memberof geom_init_element
 * @brief Next initialization element
 * @param e Element to get of next.
 * @return Next element of \p e
 * @retval NULL \p e is the last element.
 * @retval e    \p e is not added to the data yet.
 */
JUPITER_GEOMETRY_DECL
geom_init_element *geom_init_element_next(geom_init_element *e);

/**
 * @memberof geom_init_element
 * @brief Previous initialization element
 * @param e Element to get of previous.
 * @return Previous elemetn of \p e.
 * @retval NULL \p e is the first element.
 * @retval e    \p e is not added to the data yet.
 */
JUPITER_GEOMETRY_DECL
geom_init_element *geom_init_element_prev(geom_init_element *e);

JUPITER_GEOMETRY_DECL
int geom_init_element_is_head(geom_init_element *e);

/**
 * @brief setup initialize function with varargs.
 * @param e init element to set function
 * @param einfo pointer to set extended error infomation
 * @param f function to set
 * @param ... arguments for f
 *
 * The 'argument building system' is reliable, but it is painful to
 * implement when you are writing small application which is used only
 * once.
 *
 * If the number of argument is fixed for f (note: function f may have
 * variadic number of arguments) and its types are known and fixed,
 * this function provides the shortcut that avoid using argument
 * builder functions.
 *
 * This function supports `int`, `char`, `double`, `long int`,
 * `size_t`, `string` (i.e., `char *`), and any enum types (if passing
 * enum variable, please cast to `int`).
 *
 * @warning This function does not check the type at all. `f` should
 *          be literal value of one of predefined function at least.
 */
JUPITER_GEOMETRY_DECL
geom_error geom_init_element_set_func_vargs(geom_init_element *elem,
                                            geom_variant *einfo,
                                            geom_init_func f, ...);

JUPITER_GEOMETRY_DECL
geom_error geom_init_element_set_func_vlist(geom_init_element *elem,
                                            geom_variant *einfo,
                                            geom_init_func f,
                                            geom_variant_list *vl,
                                            geom_variant_list **retcur);

/**
 * @brief Calculate function result at (x, y, z)
 * @param element initialize element data
 * @param x x-axis value
 * @param y y-axis value
 * @param z z-axis value
 * @param arg User-defined initialization argument
 * @param e set error info if exist.
 * @return calculated value, HUGE_VAL if e is NULL or no function defined.
 *
 * Geometry predefined initializations does not use @p arg. You should
 * not use multiple types for @p arg.
 */
JUPITER_GEOMETRY_DECL
double geom_init_element_calc_func_at(geom_init_element *element, double x,
                                      double y, double z, void *arg,
                                      geom_error *e);

/**
 * @brief Do initialization work for element at (x, y, z)
 * @param element initialize element data
 * @param old_val Previous component value at (x, y, z)
 * @param vof_val Geometry VOF value at (x, y, z)
 * @param x x-axis value
 * @param y y-axis value
 * @param z z-axis value
 * @param arg User-defined initialization argument
 * @param e set error info if exist.
 * @return calculated value, HUGE_VAL if e is NULL or no function defined.
 *
 * Geometry predefined initializations does not use @p arg.
 * You should not use multiple types for @p arg.
 */
JUPITER_GEOMETRY_DECL
double geom_init_element_calc_at(geom_init_element *element, double old_val,
                                 double vof_val, double x, double y, double z,
                                 void *arg, geom_error *e);

/**
 * @brief Modify initialization specific parameter
 * @param e Initialization element to modify
 * @param index Parameter index to set.
 * @param var Value to set.
 * @param einfo Sets error information if error occurs.
 * @retval GEOM_SUCCESS no error
 * @retval GEOM_ERR_RANGE @p index or value out-of-range
 * @retval GEOM_ERR_NOMEM Allocation failed (some parameters may allocate
 *                        memory to set the parameter)
 * @retval GEOM_ERR_DEPENDENCY Some of parameters are missing to check
 *                             or set the given parameter.
 * @retval ...
 *
 * Some initialization may set @p einfo even if it return GEOM_SUCCESS.
 */
JUPITER_GEOMETRY_DECL
geom_error geom_init_element_set_parameter(geom_init_element *element,
                                           geom_size_type index,
                                           const geom_variant *value,
                                           geom_variant *errinfo);

/**
 * @brief Retrieve initialization specific parameter
 * @param e Initialization element to retrieve
 * @param index Parameter index to retrieve
 * @param var Denstination variant to retrieve
 * @retval GEOM_SUCCESS no error
 * @retval GEOM_ERR_NOMEM Allocation failed (some parameters may allocate
 *                        memory to get the parameter)
 */
JUPITER_GEOMETRY_DECL
geom_error geom_init_element_get_parameter(geom_init_element *element,
                                           geom_size_type index,
                                           geom_variant *out_variable);

/**
 * @brief Get function data from element
 * @param element initialize element data
 * @param f Function ID
 * @return data (or NULL if @p f does not match)
 *
 * Do not free returned pointer (even if with deallocator function)
 *
 * This function is for adding extra works in user-defined functions.
 */
JUPITER_GEOMETRY_DECL
void *geom_init_element_get_func_data(geom_init_element *element,
                                      geom_init_func f);

JUPITER_GEOMETRY_DECL
geom_init_args_builder *geom_init_args_builder_new(geom_init_func f,
                                                   geom_error *e);

JUPITER_GEOMETRY_DECL
void geom_init_args_builder_delete(geom_init_args_builder *b);

JUPITER_GEOMETRY_DECL
geom_args_builder *geom_init_args_get_builder(geom_init_args_builder *b);

JUPITER_GEOMETRY_DECL
geom_init_func geom_init_args_get_func(geom_init_args_builder *b);

JUPITER_GEOMETRY_DECL
geom_error geom_init_element_set_extra_data(geom_init_element *e, void *data,
                                            geom_deallocator *dealloc);

JUPITER_GEOMETRY_DECL
const geom_user_defined_data *
geom_init_element_get_extra_data(geom_init_element *element);

JUPITER_GEOMETRY_DECL_END

#endif
