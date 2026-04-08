
#ifndef JUPITER_GEOMETRY_FUNC_DEFS_H
#define JUPITER_GEOMETRY_FUNC_DEFS_H

#include "defs.h"
#include "vector.h"
#include "mat43.h"

JUPITER_GEOMETRY_DECL_START

/* common callbacks */
typedef geom_variant_type geom_args_nextf(geom_args_builder *b,
                                          geom_variant *description,
                                          int *optional);
typedef geom_error geom_args_checkf(void *p, geom_args_builder *b,
                                    geom_size_type index, const geom_variant *v,
                                    geom_variant *error_info);
typedef geom_error geom_get_value(void *p, geom_size_type index,
                                  geom_variant *out_variable);
typedef geom_error geom_set_value(void *p, geom_size_type index,
                                  const geom_variant *value);
typedef geom_size_type geom_get_n_params(void *p, geom_args_builder *b);
typedef geom_error geom_infomap_gen(void *p, geom_info_map *list);
typedef void *geom_copy_data(void *p);

/* init functions */
typedef double geom_initf(void *p, double x, double y, double z, void *arg);

/* shape functions (for body) */
typedef int geom_shape_testf(void *p, double x, double y, double z);
typedef void geom_shape_bboxf(void *p, geom_vec3 *start, geom_vec3 *end);
typedef int geom_shape_unwrapf(void *p, geom_vec3 xyz, int surfid,
                               geom_vec2 *uv);
typedef int geom_shape_wrapf(void *p, geom_vec2 uv, int surfid, geom_vec3 *xyz,
                             geom_vec3 *norm);
typedef int geom_shape_nsurff(void *p);
typedef int geom_shape_has_surff(void *p, int surfid);
typedef geom_error geom_shape_surface_uvpathf(void *p, int surfid,
                                              geom_2d_path_data **surface);

/* shape functions (for transform) */
typedef geom_mat43 geom_shape_transformf(void *p);

/* surface shape functions */
typedef int geom_surface_shape_testf(void *p, double x, double y);
typedef void geom_surface_shape_bboxf(void *p, geom_vec2 *start,
                                      geom_vec2 *end);
typedef geom_error geom_surface_shape_to_pathf(void *p,
                                               geom_2d_path_data **path);

struct geom_init_funcs;
typedef struct geom_init_funcs geom_init_funcs;

struct geom_shape_funcs;
typedef struct geom_shape_funcs geom_shape_funcs;

struct geom_surface_shape_funcs;
typedef struct geom_surface_shape_funcs geom_surface_shape_funcs;

JUPITER_GEOMETRY_DECL_END

#endif
