
#ifndef JUPITER_GEOMETRY_FUNC_DATA_H
#define JUPITER_GEOMETRY_FUNC_DATA_H

#include "defs.h"
#include "func_defs.h"
#include "rbtree.h"

JUPITER_GEOMETRY_DECL_START

struct geom_funcs_common
{
  struct geom_rbtree tree;
  geom_args_nextf  *args_next;
  geom_args_checkf *args_check;
  geom_allocator   *allocator;
  geom_deallocator *deallocator;
  geom_get_n_params *n_params;
  geom_get_value   *get_value;
  geom_set_value   *set_value;
  geom_copy_data   *copy;
  geom_infomap_gen *infomap_gen;
};

struct geom_init_funcs
{
  struct geom_funcs_common c;
  geom_init_func enum_val;
  geom_initf *func;
};

#define geom_init_funcs_common_tree_entry(ptr) \
  geom_rbtree_entry(ptr, struct geom_funcs_common, tree)

#define geom_init_funcs_tree_entry(ptr) \
  geom_container_of(geom_init_funcs_common_tree_entry(ptr), \
                    struct geom_init_funcs, c)

struct geom_shape_funcs
{
  struct geom_funcs_common c;
  geom_shape enum_val;
  geom_shape_type shape_type;
  geom_shape_testf *body_testf;
  geom_shape_bboxf *body_bboxf;
  geom_shape_unwrapf *body_unwrapf;
  geom_shape_wrapf *body_wrapf;
  geom_shape_has_surff *body_has_surff;
  geom_shape_nsurff *body_nsurff;
  geom_shape_surface_uvpathf *body_surff;
  geom_shape_transformf *transform_func;
};

#define geom_shape_funcs_tree_entry(ptr)                     \
  geom_container_of(geom_init_funcs_common_tree_entry(ptr),  \
                    struct geom_shape_funcs, c)

struct geom_surface_shape_funcs
{
  struct geom_funcs_common c;
  geom_surface_shape enum_val;
  geom_shape_type shape_type;
  geom_surface_shape_testf *body_testf;
  geom_surface_shape_bboxf *body_bboxf;
  geom_surface_shape_to_pathf *to_pathf;
};

#define geom_surface_shape_funcs_tree_entry(ptr) \
  geom_container_of(geom_init_funcs_common_tree_entry(ptr), \
                    struct geom_surface_shape_funcs, c)

JUPITER_GEOMETRY_DECL_END

#endif
