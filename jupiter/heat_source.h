#ifndef JUPITER_HEAT_SOURCE_H
#define JUPITER_HEAT_SOURCE_H

#include "common.h"
#include "component_data_defs.h"
#include "component_info_defs.h"
#include "controllable_type.h"
#include "geometry/list.h"
#include "trip_control.h"

#ifdef __cplusplus
extern "C" {
#endif

/* YSE: Add Heat source setting info structure */
/**
 * @brief heat source parameters
 */
struct heat_source_param
{
  struct geom_list list;           /*!< list */
  struct component_info_data comp; /*!< Component ID data */
  trip_control control;            /*!< Trip control data */
  controllable_type q_s;           /*!< Volumetric heat source for solid or gas
                                       (or gas if compo == -1) [W.m-3] */
  controllable_type q_l; /*!< Volumetric heat source for liquid [W.m-3] */
};
typedef struct heat_source_param heat_source_param;

#define heat_source_param_entry(ptr) \
  geom_list_entry(ptr, struct heat_source_param, list)

JUPITER_DECL
void heat_source_param_init(heat_source_param *p);
JUPITER_DECL
heat_source_param *heat_source_param_new(void);

JUPITER_DECL
void heat_source_param_clean(heat_source_param *p);
JUPITER_DECL
void heat_source_param_delete(heat_source_param *p);
JUPITER_DECL
void heat_source_param_delete_all(heat_source_param *head);

typedef int heat_source_param_find_func(heat_source_param *p, void *arg);

JUPITER_DECL
heat_source_param *
heat_source_param_find_base(heat_source_param *head,
                            heat_source_param_find_func *func, void *arg);

JUPITER_DECL
heat_source_param *heat_source_param_find(heat_source_param *head,
                                          int comp_index);

JUPITER_DECL
heat_source_param *heat_source_param_find_by_id(heat_source_param *head,
                                                int jupiter_id);

JUPITER_DECL
heat_source_param *heat_source_param_find_by_data(heat_source_param *head,
                                                  component_data *data);

static inline int heat_source_param_has_any(heat_source_param *head)
{
  return !geom_list_empty(&head->list);
}

#ifdef __cplusplus
}
#endif

#endif
