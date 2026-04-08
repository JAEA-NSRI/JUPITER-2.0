/**
 * @file   variant.h
 * @brief  Variable typed data.
 */

#ifndef JUPITER_GEOMETRY_VARIANT_H
#define JUPITER_GEOMETRY_VARIANT_H

#include "defs.h"

JUPITER_GEOMETRY_DECL_START

typedef void geom_variant_ext_deallocator(void *);

/**
 * @memberof geom_variant
 * @brief Create new variant data.
 * @param e If not NULL, set errors if occured
 * @return Created variant, NULL if error occured
 */
JUPITER_GEOMETRY_DECL
geom_variant *geom_variant_new(geom_error *e);

/**
 * @memberof geom_variant
 * @brief Delete variant data.
 * @param v Data to delete
 */
JUPITER_GEOMETRY_DECL
void geom_variant_delete(geom_variant *v);

/**
 * @memberof geom_variant
 * @brief Copy variant to new variant
 * @param to Variant data to copy to
 * @param from Variant data to copy from
 *
 * This function creates the shallow copy of `from` for extended
 * data.
 *
 * This function will not fail.
 */
JUPITER_GEOMETRY_DECL
void geom_variant_copy(geom_variant *to, const geom_variant *from);

/**
 * @memberof geom_variant
 * @brief Clone a variant
 * @param from Variant data to copy from
 * @param e If not NULL, sets error if occuered any.
 * @return Created new variant.
 */
JUPITER_GEOMETRY_DECL
geom_variant *geom_variant_dup(const geom_variant *from, geom_error *e);

/**
 * @memberof geom_variant
 * @brief Nullify variant
 * @param v
 */
JUPITER_GEOMETRY_DECL
void geom_variant_nullify(geom_variant *v);

/**
 * @memberof geom_variant
 * @brief Test variant is null.
 * @param v Variant to test
 * @retval 0 if v is not null
 * @retval non-0 if v is null
 */
JUPITER_GEOMETRY_DECL
int geom_variant_is_null(const geom_variant *v);

/**
 * @memberof geom_variant
 * @brief Get current type of the variant
 * @param v Variant to get from
 * @return variant type of v
 */
JUPITER_GEOMETRY_DECL
int geom_variant_get_type(const geom_variant *v);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_enum(geom_variant *v, int type, int val);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_ext(geom_variant *v, int type, void *p,
                                geom_variant_ext_deallocator *dealloc);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_char(geom_variant *v, char ch);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_uchar(geom_variant *v, unsigned char uch);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_int(geom_variant *v, int ival);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_long_int(geom_variant *v, long int lval);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_double(geom_variant *v, double dval);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_size_value(geom_variant *v, geom_size_type sval);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_string(geom_variant *v, const char *str, size_t n);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_data_op(geom_variant *v, geom_data_operator op);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_shape_op(geom_variant *v, geom_shape_operator op);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_init_func(geom_variant *v, geom_init_func f);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_shape(geom_variant *v, geom_shape s);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_phase(geom_variant *v, geom_vof_phase vf);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_error(geom_variant *v, geom_error error);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_set_surface_shape(geom_variant *v,
                                          geom_surface_shape s);

JUPITER_GEOMETRY_DECL
int geom_variant_get_enum(const geom_variant *v, int type, int invalid_val,
                          geom_error *e);

JUPITER_GEOMETRY_DECL
void *geom_variant_get_ext(const geom_variant *v, int type, geom_error *e);

JUPITER_GEOMETRY_DECL
const char *geom_variant_get_string(const geom_variant *v, geom_error *err);

JUPITER_GEOMETRY_DECL
char geom_variant_get_char(const geom_variant *v, geom_error *err);

JUPITER_GEOMETRY_DECL
unsigned char geom_variant_get_uchar(const geom_variant *v, geom_error *err);

JUPITER_GEOMETRY_DECL
int geom_variant_get_int(const geom_variant *v, geom_error *err);

JUPITER_GEOMETRY_DECL
long int geom_variant_get_long_int(const geom_variant *v, geom_error *err);

JUPITER_GEOMETRY_DECL
geom_size_type geom_variant_get_size_value(const geom_variant *v,
                                           geom_error *err);

JUPITER_GEOMETRY_DECL
double geom_variant_get_double(const geom_variant *v, geom_error *err);

JUPITER_GEOMETRY_DECL
geom_data_operator geom_variant_get_data_op(const geom_variant *v,
                                            geom_error *err);

JUPITER_GEOMETRY_DECL
geom_shape_operator geom_variant_get_shape_op(const geom_variant *v,
                                              geom_error *err);

JUPITER_GEOMETRY_DECL
geom_init_func geom_variant_get_init_func(const geom_variant *v,
                                          geom_error *err);

JUPITER_GEOMETRY_DECL
geom_shape geom_variant_get_shape(const geom_variant *v, geom_error *err);

JUPITER_GEOMETRY_DECL
geom_vof_phase geom_variant_get_phase(const geom_variant *v, geom_error *err);

JUPITER_GEOMETRY_DECL
geom_error geom_variant_get_error(const geom_variant *v, geom_error *err);

JUPITER_GEOMETRY_DECL
geom_surface_shape geom_variant_get_surface_shape(const geom_variant *v,
                                                  geom_error *err);

/**
 * @brief Convert variant to a string
 * @param buf pointer to store the result
 * @param v variant to convert
 * @return GEOM_SUCCESS if ok, otherwise an error occured.
 *
 * Use `free()` to deallocate `buf`.
 *
 * Length of the result is not defined.
 *
 * For predefined enums, values will be formatted to its string
 * expression.
 */
JUPITER_GEOMETRY_DECL
geom_error geom_variant_to_string(char **buf, const geom_variant *v);

/**
 * @brief create new list
 * @param e set error
 * @return New list, or NULL if error.
 */
JUPITER_GEOMETRY_DECL
geom_variant_list *geom_variant_list_new(geom_error *e);

/**
 * @brief Insert item v next to p
 * @param p list pointer
 * @param v item to insert
 * @param e set error status if given
 * @return the list item contains the value v. NULL if error.
 */
JUPITER_GEOMETRY_DECL
geom_variant_list *geom_variant_list_insert_next(geom_variant_list *p,
                                                 const geom_variant *v,
                                                 geom_error *e);

/**
 * @brief Insert item v previous to p
 * @param p list pointer
 * @param v item to insert
 * @param e set error status if given
 * @return the list item contains the value v. NULL if error.
 */
JUPITER_GEOMETRY_DECL
geom_variant_list *geom_variant_list_insert_prev(geom_variant_list *p,
                                                 const geom_variant *v,
                                                 geom_error *e);

/**
 * @brief Test p is head of list
 * @param p Test item
 * @return 1 if head, 0 if item.
 */
JUPITER_GEOMETRY_DECL
int geom_variant_list_is_head(geom_variant_list *p);

/**
 * @brief Test p is empty
 * @param p Test head
 * @return 1 if p is head and empty, 0 otherwise.
 */
JUPITER_GEOMETRY_DECL
int geom_variant_list_is_empty(geom_variant_list *p);

/**
 * @brief delete item p from list
 * @param p List item to delete
 */
JUPITER_GEOMETRY_DECL
void geom_variant_list_delete(geom_variant_list *p);

/**
 * @brief delete whole items from list p
 * @param p List to delete
 *
 * p is not required to be head (but recommended).
 */
JUPITER_GEOMETRY_DECL
void geom_variant_list_delete_all(geom_variant_list *p);

/**
 * @brief clear all items in the list.
 * @param p List head to clear
 */
JUPITER_GEOMETRY_DECL
void geom_variant_list_clear(geom_variant_list *p);

/**
 * @brief list next of p
 * @param p List item to get.
 * @return next pointer, head if p is end of the list.
 */
JUPITER_GEOMETRY_DECL
geom_variant_list *geom_variant_list_next(geom_variant_list *p);

/**
 * @brief list prev of p
 * @param p List item to get.
 * @return prev pointer, head if p is begin of the list.
 */
JUPITER_GEOMETRY_DECL
geom_variant_list *geom_variant_list_prev(geom_variant_list *p);

/**
 * @brief copy the variant data in p to dest.
 * @param dest copy destination.
 * @param p copy from
 *
 * If p is head of the list, returns GEOM_ERR_LIST_HEAD.
 */
JUPITER_GEOMETRY_DECL
geom_error geom_variant_list_get_copy(geom_variant *dest, geom_variant_list *p);

/**
 * @brief get the variant data
 * @param p get
 * @return pointer to the variant if ok, NULL if failed.
 *
 * If p is head of the list, returns NULL.
 *
 * Data is read-only.
 */
JUPITER_GEOMETRY_DECL
const geom_variant *geom_variant_list_get(geom_variant_list *p);

/**
 * @brief set the variant data
 * @param p list item to set
 * @return GEOM_SUCCESS if ok, otherwise failed.
 *
 * If p is head of the list, returns GEOM_ERR_LIST_HEAD.
 */
JUPITER_GEOMETRY_DECL
geom_error geom_variant_list_set(geom_variant_list *p, const geom_variant *v);

/**
 * @brief Copy whole list item of p.
 * @param p list to copy.
 * @param e if not NULL, sets error info
 * @return copied list, NULL if error.
 *
 * If p is not head, rewinds to head and then make a copy.
 *
 * Returning pointer is head.
 */
JUPITER_GEOMETRY_DECL
geom_variant_list *geom_variant_list_copy(geom_variant_list *p, geom_error *e);

JUPITER_GEOMETRY_DECL
int geom_variant_is_exttype(int t);

JUPITER_GEOMETRY_DECL
int geom_variant_has_exttype(const geom_variant *v);

JUPITER_GEOMETRY_DECL
int geom_variant_is_enumtype(int t);

JUPITER_GEOMETRY_DECL
int geom_variant_has_enumtype(const geom_variant *v);

JUPITER_GEOMETRY_DECL_END

#endif
