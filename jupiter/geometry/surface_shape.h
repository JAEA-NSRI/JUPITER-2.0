#ifndef JUPITER_GEOMETRY_SURFACE_SHAPE_H
#define JUPITER_GEOMETRY_SURFACE_SHAPE_H

#include "defs.h"

JUPITER_GEOMETRY_DECL_START

JUPITER_GEOMETRY_DECL
geom_surface_shape_data *geom_surface_shape_data_new(geom_data_element *parent,
                                                     geom_error *e);

JUPITER_GEOMETRY_DECL
void geom_surface_shape_data_delete(geom_surface_shape_data *data);

JUPITER_GEOMETRY_DECL
geom_data_element *
geom_surface_shape_data_parent(geom_surface_shape_data *data);

JUPITER_GEOMETRY_DECL
geom_data *geom_surface_shape_data_master(geom_surface_shape_data *data);

typedef int geom_surface_shape_data_error_callback(
  geom_error err, geom_surface_shape_data *data,
  geom_surface_shape_element *error_el, void *extp);

JUPITER_GEOMETRY_DECL
geom_error
geom_surface_shape_data_check(geom_surface_shape_data *data,
                              geom_surface_shape_data_error_callback *callback,
                              void *ext_data,
                              geom_surface_shape_element **error_element);

JUPITER_GEOMETRY_DECL
int geom_surface_shape_data_inout_test_at(geom_surface_shape_data *data,
                                          double x, double y);

JUPITER_GEOMETRY_DECL
geom_surface_shape_args_builder *
geom_surface_shape_args_builder_new(geom_surface_shape shape, geom_error *e);

JUPITER_GEOMETRY_DECL
void geom_surface_shape_args_builder_delete(geom_surface_shape_args_builder *b);

JUPITER_GEOMETRY_DECL
geom_args_builder *
geom_surface_shape_args_get_builder(geom_surface_shape_args_builder *b);

JUPITER_GEOMETRY_DECL
geom_error
geom_surface_shape_element_set_shape(geom_surface_shape_element *e,
                                     geom_shape_operator op,
                                     geom_surface_shape_args_builder *shape);

JUPITER_GEOMETRY_DECL
geom_surface_shape_element *
geom_surface_shape_element_new(geom_surface_shape_data *data, geom_error *e);

JUPITER_GEOMETRY_DECL
void geom_surface_shape_element_delete(geom_surface_shape_element *element);

JUPITER_GEOMETRY_DECL
geom_surface_shape_data *
geom_surface_shape_element_parent(geom_surface_shape_element *element);

JUPITER_GEOMETRY_DECL
geom_data *
geom_surface_shape_element_master(geom_surface_shape_element *element);

JUPITER_GEOMETRY_DECL
geom_surface_shape_element *
geom_surface_shape_data_get_element(geom_surface_shape_data *data);

JUPITER_GEOMETRY_DECL
geom_surface_shape_element *
geom_surface_shape_element_next(geom_surface_shape_element *element);

JUPITER_GEOMETRY_DECL
geom_surface_shape_element *
geom_surface_shape_element_prev(geom_surface_shape_element *element);

JUPITER_GEOMETRY_DECL
void geom_surface_shape_data_add_element(geom_surface_shape_element *element);

JUPITER_GEOMETRY_DECL
int geom_surface_shape_element_is_head(geom_surface_shape_element *element);

JUPITER_GEOMETRY_DECL
geom_shape_operator
geom_surface_shape_element_shape_operator(geom_surface_shape_element *element);

JUPITER_GEOMETRY_DECL
geom_surface_shape
geom_surface_shape_element_get_shape(geom_surface_shape_element *element);

JUPITER_GEOMETRY_DECL
geom_shape_type
geom_surface_shape_element_get_shape_type(geom_surface_shape_element *element);

JUPITER_GEOMETRY_DECL
geom_shape_type geom_shape_type_for_surface_shape(geom_surface_shape shape);

JUPITER_GEOMETRY_DECL
geom_info_map *
geom_surface_shape_element_shape_info(geom_surface_shape_element *element);

JUPITER_GEOMETRY_DECL
geom_error geom_surface_shape_element_set_parameter(
  geom_surface_shape_element *element, geom_size_type index,
  const geom_variant *var, geom_variant *einfo);

JUPITER_GEOMETRY_DECL
geom_error
geom_surface_shape_element_get_parameter(geom_surface_shape_element *element,
                                         geom_size_type index,
                                         geom_variant *out_variable);

JUPITER_GEOMETRY_DECL
const geom_user_defined_data *
geom_surface_shape_data_get_extra_data(geom_surface_shape_data *data);

geom_error geom_surface_shape_data_set_extra_data(geom_surface_shape_data *data,
                                                  void *extra_data,
                                                  geom_deallocator *dealloc);

JUPITER_GEOMETRY_DECL
const geom_user_defined_data *
geom_surface_shape_element_get_extra_data(geom_surface_shape_element *element);

JUPITER_GEOMETRY_DECL
geom_error
geom_surface_shape_element_set_extra_data(geom_surface_shape_element *element,
                                          void *extra_data,
                                          geom_deallocator *dealloc);

JUPITER_GEOMETRY_DECL
int geom_surface_shape_element_inout_test_at(
  geom_surface_shape_element *element, double x, double y);

JUPITER_GEOMETRY_DECL_END

#endif
