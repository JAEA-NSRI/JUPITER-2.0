#ifndef JUPITER_CONTROL_ABSTRACT_STATS_DATA_H
#define JUPITER_CONTROL_ABSTRACT_STATS_DATA_H

#include "defs.h"
#include "executive_data.h"
#include "abstract_stats.h"
#include "shared_object_priv.h"

JUPITER_CONTROL_DECL_BEGIN

/**
 * Abstract class for generating stat result.
 */
struct jcntrl_abstract_stat
{
  jcntrl_executive exec;
  int root_rank;
};
#define jcntrl_abstract_stat__ancestor jcntrl_executive
#define jcntrl_abstract_stat__dnmem exec.jcntrl_executive__dnmem
enum jcntrl_abstract_stat_vtable_names
{
  // Override in result shape coordinator (stat along some axis or all etc).
  // jcntrl_abstract_stat does not consider result shape. Please override
  // update_extent it in those classes.
  jcntrl_abstract_stat_init_output_grid_id =
    JCNTRL_VTABLE_START(jcntrl_abstract_stat),
  jcntrl_abstract_stat_get_controller_id,

  // Override in statistic calculation
  jcntrl_abstract_stat_number_of_work_arrays_id,
  jcntrl_abstract_stat_work_array_types_id,
  jcntrl_abstract_stat_calc_local_stat_id,
  jcntrl_abstract_stat_calc_global_stat_id,
  jcntrl_abstract_stat_postprocess_id,
  JCNTRL_VTABLE_SIZE(jcntrl_abstract_stat),
};

struct jcntrl_abstract_stat_all
{
  jcntrl_abstract_stat stat;
  const jcntrl_mpi_controller *controller;
};
#define jcntrl_abstract_stat_all__ancestor jcntrl_abstract_stat
#define jcntrl_abstract_stat_all__dnmem stat.jcntrl_abstract_stat__dnmem
JCNTRL_VTABLE_NONE(jcntrl_abstract_stat_all);

//---- Initialize output grid

typedef int jcntrl_abstract_stat_init_output_grid_func(
  jcntrl_shared_object *obj, jcntrl_grid_data *input, jcntrl_grid_data *output,
  jcntrl_information *input_info, jcntrl_information *output_info);

struct jcntrl_abstract_stat_init_output_grid_args
{
  jcntrl_grid_data *input;
  jcntrl_grid_data *output;
  jcntrl_information *input_info;
  jcntrl_information *output_info;
  int ret;
};

static inline void jcntrl_abstract_stat_init_output_grid__wrapper(
  jcntrl_shared_object *obj, void *args,
  jcntrl_abstract_stat_init_output_grid_func *func)
{
  struct jcntrl_abstract_stat_init_output_grid_args *a = args;
  a->ret = func(obj, a->input, a->output, a->input_info, a->output_info);
}

static inline int jcntrl_abstract_stat_init_output_grid(
  jcntrl_abstract_stat *s, jcntrl_grid_data *input, jcntrl_grid_data *output,
  jcntrl_information *input_info, jcntrl_information *output_info)
{
  struct jcntrl_abstract_stat_init_output_grid_args a = {
    .input = input,
    .output = output,
    .input_info = input_info,
    .output_info = output_info,
    .ret = 0,
  };
  jcntrl_shared_object_call_virtual(jcntrl_abstract_stat_object(s),
                                    jcntrl_abstract_stat, init_output_grid, &a);
  return a.ret;
}

//---- communication controller getter

typedef int jcntrl_abstract_stat_get_controller_func(
  jcntrl_shared_object *obj, const jcntrl_mpi_controller **controller);

struct jcntrl_abstract_stat_get_controller_args
{
  const jcntrl_mpi_controller **controller;
  int ret;
};

static inline void jcntrl_abstract_stat_get_controller__wrapper(
  jcntrl_shared_object *obj, void *args,
  jcntrl_abstract_stat_get_controller_func *func)
{
  struct jcntrl_abstract_stat_get_controller_args *a = args;
  a->ret = func(obj, a->controller);
}

static inline int
jcntrl_abstract_stat_get_controller(jcntrl_abstract_stat *s,
                                    const jcntrl_mpi_controller **controller)
{
  struct jcntrl_abstract_stat_get_controller_args a = {
    .controller = controller,
    .ret = 0,
  };
  jcntrl_shared_object_call_virtual(jcntrl_abstract_stat_object(s),
                                    jcntrl_abstract_stat, get_controller, &a);
  return a.ret;
}

//---- Return number of required work arrays

typedef jcntrl_size_type
jcntrl_abstract_stat_number_of_work_arrays_func(jcntrl_shared_object *obj);

struct jcntrl_abstract_stat_number_of_work_arrays_args
{
  jcntrl_size_type ret;
};

static inline void jcntrl_abstract_stat_number_of_work_arrays__wrapper(
  jcntrl_shared_object *obj, void *args,
  jcntrl_abstract_stat_number_of_work_arrays_func *func)
{
  struct jcntrl_abstract_stat_number_of_work_arrays_args *a = args;
  a->ret = func(obj);
}

static inline jcntrl_size_type
jcntrl_abstract_stat_number_of_work_arrays(jcntrl_abstract_stat *s)
{
  struct jcntrl_abstract_stat_number_of_work_arrays_args a = {.ret = 0};
  jcntrl_shared_object_call_virtual(jcntrl_abstract_stat_object(s),
                                    jcntrl_abstract_stat, number_of_work_arrays,
                                    &a);
  return a.ret;
}

//---- Return type of requried array types

typedef int jcntrl_abstract_stat_work_array_types_func(
  jcntrl_shared_object *obj, const jcntrl_shared_object_data **types[]);

struct jcntrl_abstract_stat_work_array_types_args
{
  const jcntrl_shared_object_data ***types;
  int ret;
};

static inline void jcntrl_abstract_stat_work_array_types__wrapper(
  jcntrl_shared_object *obj, void *args,
  jcntrl_abstract_stat_work_array_types_func *func)
{
  struct jcntrl_abstract_stat_work_array_types_args *a = args;
  a->ret = func(obj, a->types);
}

static inline int
jcntrl_abstract_stat_work_array_types(jcntrl_abstract_stat *s,
                                      const jcntrl_shared_object_data **types[])
{
  struct jcntrl_abstract_stat_work_array_types_args a = {
    .ret = 0,
    .types = types,
  };
  jcntrl_shared_object_call_virtual(jcntrl_abstract_stat_object(s),
                                    jcntrl_abstract_stat, work_array_types, &a);
  return a.ret;
}

//---- Calculate local reduction and store results in work_arrays

typedef int jcntrl_abstract_stat_calc_local_stat_func(
  jcntrl_shared_object *obj, jcntrl_grid_data *grid, const int piece_extent[6],
  const int data_extent[6], jcntrl_data_array *input, jcntrl_bool_array *mask,
  jcntrl_data_array **work_arrays);

struct jcntrl_abstract_stat_calc_local_stat_args
{
  jcntrl_grid_data *grid;
  const int *piece_extent;
  const int *data_extent;
  jcntrl_data_array *input;
  jcntrl_bool_array *mask;
  jcntrl_data_array **work_arrays;
  int ret;
};

static inline void jcntrl_abstract_stat_calc_local_stat__wrapper(
  jcntrl_shared_object *obj, void *args,
  jcntrl_abstract_stat_calc_local_stat_func *func)
{
  struct jcntrl_abstract_stat_calc_local_stat_args *a = args;
  a->ret = func(obj, a->grid, a->piece_extent, a->data_extent, a->input,
                a->mask, a->work_arrays);
}

static inline int jcntrl_abstract_stat_calc_local_stat(
  jcntrl_abstract_stat *s, jcntrl_grid_data *grid, const int piece_extent[6],
  const int data_extent[6], jcntrl_data_array *input, jcntrl_bool_array *mask,
  jcntrl_data_array **work_arrays)
{
  struct jcntrl_abstract_stat_calc_local_stat_args a = {
    .grid = grid,
    .piece_extent = piece_extent,
    .data_extent = data_extent,
    .input = input,
    .mask = mask,
    .work_arrays = work_arrays,
    .ret = 0,
  };
  jcntrl_shared_object_call_virtual(jcntrl_abstract_stat_object(s),
                                    jcntrl_abstract_stat, calc_local_stat, &a);
  return a.ret;
}

//---- Calculate global reduction and store results in work_arrays

typedef int
jcntrl_abstract_stat_calc_global_stat_func(jcntrl_shared_object *obj,
                                           jcntrl_data_array **work_arrays,
                                           jcntrl_data_array *output);

struct jcntrl_abstract_stat_calc_global_stat_args
{
  jcntrl_data_array **work_arrays;
  jcntrl_data_array *output;
  int ret;
};

static inline void jcntrl_abstract_stat_calc_global_stat__wrapper(
  jcntrl_shared_object *obj, void *args,
  jcntrl_abstract_stat_calc_global_stat_func *func)
{
  struct jcntrl_abstract_stat_calc_global_stat_args *a = args;
  a->ret = func(obj, a->work_arrays, a->output);
}

static inline int
jcntrl_abstract_stat_calc_global_stat(jcntrl_abstract_stat *s,
                                      jcntrl_data_array **work_arrays,
                                      jcntrl_data_array *output)
{
  struct jcntrl_abstract_stat_calc_global_stat_args a = {
    .work_arrays = work_arrays,
    .output = output,
    .ret = 0,
  };
  jcntrl_shared_object_call_virtual(jcntrl_abstract_stat_object(s),
                                    jcntrl_abstract_stat, calc_global_stat, &a);
  return a.ret;
}

//---- Postprocess function (for implement work before deallocate work_arrays)
//
//     If work_arrays are not required, you can override process_update_data
//     directly and use jcntrl_executive_process_update_data__super().

typedef int jcntrl_abstract_stat_postprocess_func(
  jcntrl_shared_object *obj, jcntrl_information *output_info,
  jcntrl_size_type result_size, jcntrl_data_array **work_arrays);

struct jcntrl_abstract_stat_postprocess_args
{
  jcntrl_information *output_info;
  jcntrl_size_type result_size;
  jcntrl_data_array **work_arrays;
  int ret;
};

static inline void jcntrl_abstract_stat_postprocess__wrapper(
  jcntrl_shared_object *obj, void *args,
  jcntrl_abstract_stat_postprocess_func *func)
{
  struct jcntrl_abstract_stat_postprocess_args *a = args;
  a->ret = func(obj, a->output_info, a->result_size, a->work_arrays);
}

static inline int jcntrl_abstract_stat_postprocess(
  jcntrl_abstract_stat *s, jcntrl_information *output_info,
  jcntrl_size_type result_size, jcntrl_data_array **work_arrays)
{
  struct jcntrl_abstract_stat_postprocess_args a = {
    .output_info = output_info,
    .result_size = result_size,
    .work_arrays = work_arrays,
    .ret = 0,
  };
  jcntrl_shared_object_call_virtual(jcntrl_abstract_stat_object(s),
                                    jcntrl_abstract_stat, postprocess, &a);
  return a.ret;
}

static inline int jcntrl_abstract_stat_postprocess__super(
  const jcntrl_shared_object_data *ancestor, jcntrl_information *output_info,
  jcntrl_size_type result_size, jcntrl_data_array **work_arrays)
{
  struct jcntrl_abstract_stat_postprocess_args a = {
    .output_info = output_info,
    .result_size = result_size,
    .work_arrays = work_arrays,
    .ret = 0,
  };
  jcntrl_shared_object_call_super(ancestor, jcntrl_abstract_stat, postprocess,
                                  &a);
  return a.ret;
}

#endif
