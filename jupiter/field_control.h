
#ifndef FIELD_CONTROL_H
#define FIELD_CONTROL_H

#include "common.h"
#include "control/defs.h"
#include "geometry/list.h"
#include "serializer/defs.h"
#include "csv.h"
#include "struct.h"

#ifdef __cplusplus
extern "C" {
#endif

static inline void write_field_variables_init(write_field_variables *p)
{
  geom_list_init(&p->list);
  p->writer = NULL;
}

JUPITER_DECL
write_field_variables *write_field_variables_add(domain *cdo);
JUPITER_DECL
void write_field_variables_delete(write_field_variables *p);
JUPITER_DECL
void write_field_variables_delete_all(write_field_variables *head);

/**
 * @brief Return type of name
 * @param name Name to test
 * @return Type of given name.
 */
JUPITER_DECL
control_nametype field_control_nametype(const char *name);

/**
 * @brief Read names used by control data file and add them to manager
 * @param manager manager to add
 * @param ctrl_csv Control CSV data
 * @param ctrl_file File name of @p ctrl_csv
 * @param stat if given, sets to `ON` on error
 */
JUPITER_DECL
void read_control_names(jcntrl_executive_manager *manager, csv_data *ctrl_csv,
                        const char *ctrl_file, int *stat);

/**
 * @brief Initialize controllable type
 * @param obj controllable type object to initialize
 */
JUPITER_DECL
void controllable_type_init(controllable_type *obj);

/**
 * @brief Copy the content of the controllable type
 * @param to destination
 * @param from source
 *
 * @p to must not be a member of list.
 */
JUPITER_DECL
void controllable_type_copy(controllable_type *to, controllable_type *from);

/**
 * @brief Remove controllable type item from the process chain list
 * @param item Item to remove
 *
 * Removing @p item that is not in any list is safe.
 */
JUPITER_DECL
void controllable_type_remove_from_list(controllable_type *item);

/**
 * @brief Add controllable type item to the process chain list
 * @param head Head item of the list
 * @param item Item to add
 *
 * @p item must not to be a member of any lists (including @p head).
 */
JUPITER_DECL
void controllable_type_add_to_list(controllable_type *head,
                                   controllable_type *item);

/**
 * @brief Add controllable type item to the process chain list if
 *        an executive is pointed
 * @param head Head item of the list
 * @param item Item to add
 * @return 1 @p item has been added to the list of @p head, 0 if not.
 *
 * @p item must not to be a member of any lists (including @p head).
 */
JUPITER_DECL
int controllable_type_add_to_list_if_needed(controllable_type *head,
                                            controllable_type *item);

/**
 * @brief Update current value of the item
 * @param item Item to update
 * @return 1 if success, 0 if any error(s) occured
 *
 * Run the executive assinged to the @p item, and update the value.
 *
 * For updating multiple items, form a list of items to update and use
 * `controllable_type_update_all()`.
 */
JUPITER_DECL
int controllable_type_update(controllable_type *item);

JUPITER_DECL
int controllable_type_update_all(jcntrl_executive_manager *manager,
                                 controllable_type *head);

JUPITER_DECL
int controllable_type_format(char **buf, char fp_type,
                             int width, int fp_precision,
                             controllable_type *item);
JUPITER_DECL
int controllable_type_format_vec3(char **buf, char fp_type,
                                  int width, int fp_precision,
                                  controllable_type *x_item,
                                  controllable_type *y_item,
                                  controllable_type *z_item);

/**
 * @brief Serialize controllable_type data
 * @param item Item to serialize
 * @return result map head
 */
JUPITER_DECL
msgpackx_map_node *controllable_type_to_msgpackx(controllable_type *item);

/**
 * @brief Deserialize controllable_type data
 * @param dest Destnatio controllable_type data
 * @param mdata Serialized data to deselialize
 * @retval 0 Sucess
 * @retval 1 Error
 *
 * @note @p dest must be initialized before the call.
 * @note current value in @p dest is not up-to-date.
 */
JUPITER_DECL
int controllable_type_from_msgpackx(controllable_type *dest,
                                    msgpackx_map_node *mdata,
                                    jcntrl_executive_manager *manager);

/**
 * @brief Create new controllable geometry entry.
 * @param oldp Reusable old pointer to be discard data.
 * @return initialized oldp if oldp given, new pointer otherwise, NULL
 *         if allocation failed.
 */
JUPITER_DECL
controllable_geometry_entry *
controllable_geometry_entry_new(controllable_geometry_entry *oldp);

JUPITER_DECL
void controllable_geometry_entry_delete(controllable_geometry_entry *p);
JUPITER_DECL
void controllable_geometry_entry_add(controllable_geometry_entry *head,
                                     controllable_geometry_entry *p);
JUPITER_DECL
geom_error
controllable_geometry_entry_set_to_variant(controllable_geometry_entry *p,
                                           geom_variant *v);

JUPITER_DECL
void controllable_geometry_entry_sort(controllable_geometry_entry *head);

static inline controllable_geometry_entry *
controllable_geometry_entry_prev(controllable_geometry_entry *entry)
{
  return controllable_geometry_list_entry(geom_list_prev(&entry->list));
}

static inline controllable_geometry_entry *
controllable_geometry_entry_next(controllable_geometry_entry *entry)
{
  return controllable_geometry_list_entry(geom_list_next(&entry->list));
}

JUPITER_DECL
void set_controls(csv_data *control_csv, const char *control_file,
                  jcntrl_executive_manager *data, int *status, flags *flg,
                  domain *cdo, mpi_param *mpi);

JUPITER_DECL
int field_control_errorhandler(void *data, jcntrl_information *info);

#ifdef __cplusplus
}
#endif

#endif
