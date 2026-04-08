
#ifndef JUPITER_CONTROL_STRUCT_DATA_H
#define JUPITER_CONTROL_STRUCT_DATA_H

#include "defs.h"
#include "shared_object_priv.h"
#include "data_array_data.h"

#include <jupiter/geometry/list.h>
#include <jupiter/geometry/defs.h>
#include <jupiter/geometry/svector.h>

JUPITER_CONTROL_DECL_BEGIN

struct jcntrl_generic_data_array
{
  jcntrl_data_array data;
  jcntrl_size_type number_of_tuples;
  void *allocated_data;
  const void *readonly_data;
};
#define jcntrl_generic_data_array__ancestor jcntrl_data_array
#define jcntrl_generic_data_array__dnmem data.jcntrl_data_array__dnmem
JCNTRL_VTABLE_NONE(jcntrl_generic_data_array);

struct jcntrl_char_array
{
  struct jcntrl_generic_data_array data;
};
#define jcntrl_char_array__ancestor jcntrl_generic_data_array
#define jcntrl_char_array__dnmem data.jcntrl_generic_data_array__dnmem
JCNTRL_VTABLE_NONE(jcntrl_char_array);

struct jcntrl_bool_array
{
  struct jcntrl_generic_data_array data;
};
#define jcntrl_bool_array__ancestor jcntrl_generic_data_array
#define jcntrl_bool_array__dnmem data.jcntrl_generic_data_array__dnmem
JCNTRL_VTABLE_NONE(jcntrl_bool_array);

struct jcntrl_int_array
{
  struct jcntrl_generic_data_array data;
};
#define jcntrl_int_array__ancestor jcntrl_generic_data_array
#define jcntrl_int_array__dnmem data.jcntrl_generic_data_array__dnmem
JCNTRL_VTABLE_NONE(jcntrl_int_array);

struct jcntrl_aint_array
{
  struct jcntrl_generic_data_array data;
};
#define jcntrl_aint_array__ancestor jcntrl_generic_data_array
#define jcntrl_aint_array__dnmem data.jcntrl_generic_data_array__dnmem
JCNTRL_VTABLE_NONE(jcntrl_aint_array);

struct jcntrl_float_array
{
  struct jcntrl_generic_data_array data;
};
#define jcntrl_float_array__ancestor jcntrl_generic_data_array
#define jcntrl_float_array__dnmem data.jcntrl_generic_data_array__dnmem
JCNTRL_VTABLE_NONE(jcntrl_float_array);

struct jcntrl_double_array
{
  struct jcntrl_generic_data_array data;
};
#define jcntrl_double_array__ancestor jcntrl_generic_data_array
#define jcntrl_double_array__dnmem data.jcntrl_generic_data_array__dnmem
JCNTRL_VTABLE_NONE(jcntrl_double_array);

struct jcntrl_size_array
{
  struct jcntrl_generic_data_array data;
};
#define jcntrl_size_array__ancestor jcntrl_generic_data_array
#define jcntrl_size_array__dnmem data.jcntrl_generic_data_array__dnmem
JCNTRL_VTABLE_NONE(jcntrl_size_array);

struct jcntrl_cell_data_entry
{
  struct geom_list list;
  jcntrl_data_array *array;
};

struct jcntrl_cell_data
{
  jcntrl_shared_object object;
  struct jcntrl_cell_data_entry head;
};
#define jcntrl_cell_data__ancestor jcntrl_shared_object
#define jcntrl_cell_data__dnmem object
JCNTRL_VTABLE_NONE(jcntrl_cell_data);

struct jcntrl_data_object
{
  jcntrl_shared_object object;
};
#define jcntrl_data_object__ancestor jcntrl_shared_object
#define jcntrl_data_object__dnmem object
JCNTRL_VTABLE_NONE(jcntrl_data_object);

struct jcntrl_mask_object
{
  jcntrl_data_object data_object;
};
#define jcntrl_mask_object__ancestor jcntrl_data_object
#define jcntrl_mask_object__dnmem data_object.jcntrl_data_object__dnmem
JCNTRL_VTABLE_NONE(jcntrl_mask_object);

struct jcntrl_mask_function
{
  jcntrl_mask_object mask_object;
};
#define jcntrl_mask_function__ancestor jcntrl_mask_object
#define jcntrl_mask_function__dnmem mask_object.jcntrl_mask_object__dnmem
enum jcntrl_mask_function_vtable_names
{
  jcntrl_mask_function_eval_id = JCNTRL_VTABLE_START(jcntrl_mask_function),
  JCNTRL_VTABLE_SIZE(jcntrl_mask_function)
};

struct jcntrl_mask_data
{
  jcntrl_mask_object mask_object;
  jcntrl_bool_array *array;
};
#define jcntrl_mask_data__ancestor jcntrl_mask_object
#define jcntrl_mask_data__dnmem mask_object.jcntrl_mask_object__dnmem
JCNTRL_VTABLE_NONE(jcntrl_mask_data);

struct jcntrl_struct_grid
{
  int extent[6];
  jcntrl_data_array *x_coords;
  jcntrl_data_array *y_coords;
  jcntrl_data_array *z_coords;
};

struct jcntrl_grid_data
{
  jcntrl_data_object data_object;
  jcntrl_struct_grid coords;
  jcntrl_mask_data *mask;
  jcntrl_cell_data *cell_data;
};
#define jcntrl_grid_data__ancestor jcntrl_data_object
#define jcntrl_grid_data__dnmem data_object.jcntrl_data_object__dnmem
JCNTRL_VTABLE_NONE(jcntrl_grid_data);

struct jcntrl_field_object
{
  jcntrl_data_object data_object;
};
#define jcntrl_field_object__ancestor jcntrl_data_object
#define jcntrl_field_object__dnmem data_object.jcntrl_data_object__dnmem
JCNTRL_VTABLE_NONE(jcntrl_field_object);

struct jcntrl_field_variable
{
  jcntrl_field_object field_object;
  jcntrl_data_array *data;
};
#define jcntrl_field_variable__ancestor jcntrl_field_object
#define jcntrl_field_variable__dnmem field_object.jcntrl_field_object__dnmem
JCNTRL_VTABLE_NONE(jcntrl_field_variable);

struct jcntrl_field_function
{
  jcntrl_field_object field_object;
  jcntrl_scalar_field_function *sfunc;
  jcntrl_vector_field_function *vfunc;
  void *data;
};
#define jcntrl_field_function__ancestor jcntrl_field_object
#define jcntrl_field_function__dnmem field_object.jcntrl_field_object__dnmem
JCNTRL_VTABLE_NONE(jcntrl_field_function);

struct jcntrl_logical_variable
{
  jcntrl_data_object data_object;
  int cond;
};
#define jcntrl_logical_variable__ancestor jcntrl_data_object
#define jcntrl_logical_variable__dnmem data_object.jcntrl_data_object__dnmem
JCNTRL_VTABLE_NONE(jcntrl_logical_variable);

struct jcntrl_geometry
{
  jcntrl_data_object data_object;
  geom_data_element *data;
  jcntrl_data_array *file_data;
  int file_extent[6];
  int data_extent[6];
};
#define jcntrl_geometry__ancestor jcntrl_data_object
#define jcntrl_geometry__dnmem data_object.jcntrl_data_object__dnmem
enum jcntrl_geometry_vtable_names
{
  jcntrl_geometry_has_shape_id = JCNTRL_VTABLE_START(jcntrl_geometry),
  jcntrl_geometry_has_file_id,
  jcntrl_geometry_file_load_id,
  JCNTRL_VTABLE_SIZE(jcntrl_geometry),
};

JUPITER_CONTROL_DECL_END

#endif
