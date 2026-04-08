#ifndef JUPITER_PRINT_PARAM_INFOMAP_H
#define JUPITER_PRINT_PARAM_INFOMAP_H

/* print param for geom_infomap */

#include "common.h"
#include "geometry/defs.h"
#include "print_param_core.h"
#include "print_param_variant.h"
#include "strlist.h"
#include "struct.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

//---- print single node
//--- name (description)

struct PP_info_map_name_data
{
  struct pp_format_name_data data;
  geom_info_map *node;
};

JUPITER_DECL
const char *PP_info_map_name_format(void *a);

static inline struct pp_format_name_data *
PP_info_map_name_init(struct PP_info_map_name_data *a, geom_info_map *node)
{
  a->node = node;
  return pp_format_name_init(&a->data, PP_info_map_name_format, NULL, a);
}

//--- value

typedef int PP_info_map_ok_func(geom_info_map *value, void *arg);

struct PP_info_map_value_data
{
  struct pp_format_value_data data;
  struct PP_geom_variant_value_data var;
  geom_info_map *node;
  controllable_geometry_entry *control;
  jupiter_strlist_head lsh;
  const char *cbuf;
  PP_info_map_ok_func *okfunc;
  void *okarg;
};

JUPITER_DECL
void PP_info_map_value_format(int *argc, const char ***argv, void *a);
JUPITER_DECL
const char *PP_info_map_value_null_format(void *a);
JUPITER_DECL
int PP_info_map_value_custom_ok_func(void *a);
JUPITER_DECL
void PP_info_map_value_delete(void *a);

static inline struct pp_format_value_data *PP_info_map_value_init(
  struct PP_info_map_value_data *a, geom_info_map *node,
  controllable_geometry_entry *control_entry, PP_info_map_ok_func *okfunc,
  void *okarg, PP_geom_variant_extenumd_format *extenum_format,
  PP_geom_variant_extenumd_delete *extenum_delete, void *extenum_arg)
{
  a->node = node;
  a->cbuf = NULL;
  a->control = control_entry;
  a->okfunc = okfunc;
  a->okarg = okarg;
  jupiter_strlist_head_init(&a->lsh);
  PP_geom_variant_value_init(&a->var, NULL, NULL, NULL, extenum_format,
                             extenum_delete, extenum_arg);
  return pp_format_value_init_vec(&a->data, PP_info_map_value_format,
                                  PP_info_map_value_null_format,
                                  PP_info_map_value_custom_ok_func,
                                  PP_info_map_value_delete, a);
}

//--- unit

struct PP_info_map_unit_data
{
  struct pp_format_unit_data data;
  geom_info_map *node;
  char *buf;
  const char *base_unit;
  const char *length_unit;
};

JUPITER_DECL
const char *PP_info_map_unit_format(void *a);
JUPITER_DECL
void PP_info_map_unit_delete(void *a);

/**
 * @param base_unit Replacement unit for symbol `I`.
 * @param length_unit Replacement unit for symbol `L`.
 */
static inline struct pp_format_unit_data *
PP_info_map_unit_init(struct PP_info_map_unit_data *a, geom_info_map *node,
                      const char *base_unit, const char *length_unit)
{
  a->node = node;
  a->base_unit = base_unit;
  a->length_unit = length_unit;
  a->buf = NULL;
  return pp_format_unit_init(&a->data, PP_info_map_unit_format,
                             PP_info_map_unit_delete, a);
}

//--- Print all list

/**
 * @brief value checker prototype for PPgeom_info_map
 * @param index Index in the map
 * @param value Value that is assigned
 * @param handler Data provided for PPgeom_info_map
 * @retval 1 ok
 * @ratvel 0 not ok
 */
typedef int PPgeom_info_map_ok_func(geom_size_type index,
                                    const geom_variant *var, void *arg);

JUPITER_DECL
void PPgeom_info_map(flags *flg, int indent, const char *base_unit,
                     const char *length_unit, geom_info_map *map_head,
                     controllable_geometry_entry *start, int *nogood,
                     PP_geom_variant_extenumd_format *ext_format,
                     PP_geom_variant_extenumd_delete *ext_delete,
                     void *ext_format_data,
                     PPgeom_info_map_ok_func *ok_handler,
                     void *ok_handler_data);

#ifdef __cplusplus
}
#endif

#endif
