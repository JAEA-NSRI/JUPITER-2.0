#include "abstract_stats.h"
#include "abstract_stats_data.h"
#include "cell_data.h"
#include "data_array.h"
#include "defs.h"
#include "error.h"
#include "executive.h"
#include "extent.h"
#include "grid_data.h"
#include "information.h"
#include "input.h"
#include "mask_data.h"
#include "mpi_controller.h"
#include "output.h"
#include "overflow.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "static_array.h"
#include "struct_grid.h"
#include "subarray.h"

#include <math.h>
#include <stdlib.h>
#include <string.h>

static jcntrl_abstract_stat *
jcntrl_abstract_stat_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_abstract_stat, obj);
}

static void *jcntrl_abstract_stat_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_abstract_stat_downcast_impl(obj);
}

static int jcntrl_abstract_stat_initializer(jcntrl_shared_object *obj)
{
  jcntrl_abstract_stat *s = jcntrl_abstract_stat_downcast_impl(obj);
  jcntrl_input *input;
  jcntrl_output *output;
  jcntrl_executive *exec;

  s->root_rank = 0;

  exec = jcntrl_abstract_stat_executive(s);
  input = jcntrl_executive_get_input(exec);
  output = jcntrl_executive_get_output(exec);

  if (!jcntrl_input_set_number_of_ports(input, 1))
    return 0;
  if (!jcntrl_output_set_number_of_ports(output, 1))
    return 0;
  return 1;
}

static void jcntrl_abstract_stat_destructor(jcntrl_shared_object *obj)
{
  /* nop */
}

static int jcntrl_abstract_stat_fill_input_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_input *input)
{
  jcntrl_information *info;
  info = jcntrl_input_information(input);
  JCNTRL_ASSERT(info);

  if (!jcntrl_information_set_datatype(info, JCNTRL_INFO_REQUIRED_DATATYPE,
                                       JCNTRL_DATATYPE_GRID))
    return 0;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_abstract_stat, jcntrl_executive,
                    fill_input_port_information)

static int jcntrl_abstract_stat_fill_output_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_output *output)
{
  jcntrl_information *info;
  info = jcntrl_output_information(output);
  JCNTRL_ASSERT(info);

  if (!jcntrl_information_set_objecttype(info, JCNTRL_INFO_DATATYPE,
                                         jcntrl_grid_data))
    return 0;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_abstract_stat, jcntrl_executive,
                    fill_output_port_information)

static int jcntrl_abstract_stat_calc_result_size(
  jcntrl_abstract_stat *s, jcntrl_size_type *szout, jcntrl_grid_data *g,
  const jcntrl_mpi_controller *controller)
{
  jcntrl_size_type sz;
  sz = jcntrl_extent_size(jcntrl_extent_c(jcntrl_grid_data_extent(g)));

  if (controller) {
    int r;
    jcntrl_data_array *d;
    jcntrl_static_size_array sza;
    jcntrl_static_size_array_init_base(&sza, &sz, 1);

    d = jcntrl_static_size_array_data(&sza);
    r = 1;

    do {
      if (!(jcntrl_mpi_controller_rank(controller) == s->root_rank ||
            sz == 0)) {
        jcntrl_raise_argument_error(
          __FILE__, __LINE__, "Collect for multiple ranks are not supported");
        r = 0;
        break;
      }
    } while (0);

    if (!jcntrl_mpi_controller_all(controller, r))
      r = 0;

    do {
      if (!r)
        break;

      if (!jcntrl_mpi_controller_bcast(controller, d, 1, s->root_rank))
        r = 0;
    } while (0);

    if (!r)
      return 0;

    sz = jcntrl_static_size_array_get(&sza)[0];
  }

  *szout = sz;
  return 1;
}

static int jcntrl_abstract_stat_allocate_work_arrays_on(
  jcntrl_abstract_stat *s, jcntrl_size_type narys, jcntrl_size_type rsz,
  int rank, int nproc, jcntrl_data_array **work_arrays,
  jcntrl_data_array **local_work_subarrays)
{
  jcntrl_size_type rszg;
  const jcntrl_shared_object_data **work_array_types;

  work_array_types = NULL;
  if (!jcntrl_abstract_stat_work_array_types(s, &work_array_types))
    return 0;

  JCNTRL_ASSERT(work_array_types);

  if (jcntrl_s_mul_overflow(rsz, nproc, &rszg))
    return 0;

  for (jcntrl_size_type i = 0; i < narys; ++i) {
    jcntrl_shared_object *obj;
    jcntrl_data_array *d;
    jcntrl_data_subarray *s;

    obj = jcntrl_shared_object_new_by_meta(work_array_types[i]);
    if (!obj)
      return 0;

    d = jcntrl_data_array_downcast(obj);
    if (!d) {
      jcntrl_shared_object_delete(obj);
      return 0;
    }

    work_arrays[i] = d;
    if (!jcntrl_data_array_resize(d, rszg))
      return 0;

    s = jcntrl_data_subarray_new(d, 0, rsz);
    if (!s)
      return 0;

    d = jcntrl_data_subarray_data(s);
    JCNTRL_ASSERT(d);

    local_work_subarrays[i] = d;
  }

  return 1;
}

static void jcntrl_abstract_stat_delete_work_arrays(
  jcntrl_size_type narys, jcntrl_data_array **work_arrays,
  jcntrl_data_array **local_work_subarrays)
{
  for (jcntrl_size_type i = 0; i < narys; ++i) {
    if (work_arrays[i])
      jcntrl_data_array_delete(work_arrays[i]);
    if (local_work_subarrays[i])
      jcntrl_data_array_delete(local_work_subarrays[i]);
  }
  free(work_arrays);
}

static int jcntrl_abstract_stat_allocate_work_arrays(
  jcntrl_abstract_stat *s, jcntrl_size_type *narys, jcntrl_size_type rsz,
  int rank, int nproc, jcntrl_data_array ***work_arrays,
  jcntrl_data_array ***local_work_subarrays)
{
  jcntrl_size_type nary, narysz;
  jcntrl_data_array **warys, **larys;

  nary = jcntrl_abstract_stat_number_of_work_arrays(s);
  if (nary <= 0)
    return 0;

  if (jcntrl_s_mul_overflow(nary, 2, &narysz))
    return 0;

  warys = calloc(narysz, sizeof(jcntrl_data_array *));
  if (!warys) {
    jcntrl_raise_allocation_failed(__FILE__, __LINE__);
    return 0;
  }

  larys = warys + nary;

  if (jcntrl_abstract_stat_allocate_work_arrays_on(s, nary, rsz, rank, nproc,
                                                   warys, larys)) {
    *narys = nary;
    *work_arrays = warys;
    *local_work_subarrays = larys;
    return 1;
  }

  jcntrl_abstract_stat_delete_work_arrays(nary, warys, larys);
  return 0;
}

static int jcntrl_abstract_stat_calc_for_array(
  jcntrl_abstract_stat *s, jcntrl_information *info, jcntrl_grid_data *grid,
  const int *data_extent, jcntrl_data_array *input, const char *name,
  jcntrl_size_type nlen, jcntrl_bool_array *mask,
  jcntrl_cell_data *output_celldata, jcntrl_size_type rsz,
  const jcntrl_mpi_controller *controller, int rank, jcntrl_size_type nary,
  jcntrl_data_array **work_arrays, jcntrl_data_array **local_work_arrays)
{
  int r;
  const int *piece_extent;
  jcntrl_double_array *d;
  jcntrl_data_array *outp;

  piece_extent = jcntrl_information_get_extent(info, JCNTRL_INFO_PIECE_EXTENT);

  r = jcntrl_abstract_stat_calc_local_stat(s, grid, piece_extent, data_extent,
                                           input, mask, local_work_arrays);
  if (!(controller ? jcntrl_mpi_controller_all(controller, r) : r))
    return 0;

  if (controller) {
    for (jcntrl_size_type i = 0; i < nary; ++i) {
      if (!jcntrl_mpi_controller_gather(controller, work_arrays[i],
                                        local_work_arrays[i], rsz,
                                        s->root_rank))
        return 0;
    }
  }

  d = jcntrl_double_array_new();
  outp = NULL;
  if (d)
    outp = jcntrl_double_array_data(d);
  if (outp) {
    if (!jcntrl_data_array_set_name(outp, name, nlen))
      outp = NULL;
  }
  if (outp) {
    if (!jcntrl_data_array_resize(outp, rsz))
      outp = NULL;
  }
  if (!(controller ? jcntrl_mpi_controller_all(controller, !!outp) : !!outp)) {
    if (d)
      jcntrl_double_array_delete(d);
    return 0;
  }

  if (s->root_rank == rank) {
    r = jcntrl_abstract_stat_calc_global_stat(s, work_arrays, outp);
  }

  r = jcntrl_cell_data_add_array(output_celldata, outp);
  if (controller)
    r = jcntrl_mpi_controller_all(controller, r);

  // delete if failed to add array, otherwise release ownership
  jcntrl_data_array_delete(outp);
  return r;
}

static int jcntrl_abstract_stat_calc_for_grid(
  jcntrl_abstract_stat *s, jcntrl_information *info, jcntrl_grid_data *input,
  jcntrl_grid_data *output, jcntrl_size_type rsz, int rank, int nproc,
  const jcntrl_mpi_controller *controller, jcntrl_size_type nwork_array,
  jcntrl_data_array **work_arrays, jcntrl_data_array **local_work_arrays)
{
  int iret = 1;
  jcntrl_cell_data *icdata, *ocdata;
  const int *data_extent = jcntrl_grid_data_extent(input);
  jcntrl_mask_data *m;
  jcntrl_bool_array *mask;

  icdata = jcntrl_grid_data_cell_data(input);
  ocdata = jcntrl_grid_data_cell_data(output);

  m = jcntrl_grid_data_get_mask(input);
  mask = NULL;
  if (m)
    mask = jcntrl_mask_data_array(m);

  jcntrl_cell_data_remove_all_arrays(ocdata);

  for (int master = 0; master < nproc; ++master) {
    jcntrl_data_array *aryp;
    int iend = 0;
    jcntrl_static_int_array ienda;
    jcntrl_data_array *iendd;
    jcntrl_static_int_array_init_base(&ienda, &iend, 1);
    iendd = jcntrl_static_int_array_data(&ienda);

    if (rank == master) {
      if (controller) {
        // name array allocation on slave
        if (!jcntrl_mpi_controller_all(controller, 1))
          return 0;
      }

      jcntrl_cell_data_array_foreach (icdata, aryp, iter) {
        const char *name;
        jcntrl_size_type nlen;

        name = jcntrl_data_array_name(aryp, &nlen);
        if (!name || nlen == 0)
          continue;

        if (jcntrl_cell_data_get_array_by_name(ocdata, name, nlen))
          continue;

        if (controller) {
          jcntrl_static_cstr_array name_s;
          jcntrl_data_array *name_sd;
          jcntrl_static_cstr_array_init_base(&name_s, name, nlen);
          name_sd = jcntrl_static_cstr_array_data(&name_s);
          JCNTRL_ASSERT(name_sd);

          if (!jcntrl_mpi_controller_bcast(controller, iendd, 1, master))
            return 0;

          if (!jcntrl_mpi_controller_bcasta(controller, name_sd, master))
            return 0;
        }

        if (!jcntrl_abstract_stat_calc_for_array(s, info, input, data_extent,
                                                 aryp, name, nlen, mask, ocdata,
                                                 rsz, controller, rank,
                                                 nwork_array, work_arrays,
                                                 local_work_arrays)) {
          return 0;
        }
      }

      iend = 1;
      if (controller) {
        if (!jcntrl_mpi_controller_bcast(controller, iendd, 1, master))
          return 0;
      }

    } else { // slave
      jcntrl_char_array *name;
      jcntrl_data_array *named;

      JCNTRL_ASSERT(controller);

      name = jcntrl_char_array_new();
      if (!jcntrl_mpi_controller_all(controller, !!name)) {
        if (name)
          jcntrl_char_array_delete(name);
        return 0;
      }

      named = jcntrl_char_array_data(name);

      while (!iend) {
        const char *namep;
        jcntrl_size_type nlen;
        jcntrl_data_array *iary;

        if (!jcntrl_mpi_controller_bcast(controller, iendd, 1, master)) {
          iret = 0;
          break;
        }
        if (iend)
          break;

        if (!jcntrl_mpi_controller_bcasta(controller, named, master)) {
          iret = 0;
          break;
        }

        namep = jcntrl_char_array_get(name);
        nlen = jcntrl_char_array_get_ntuple(name);

        iary = jcntrl_cell_data_get_array_by_name(icdata, namep, nlen);
        if (!jcntrl_abstract_stat_calc_for_array(s, info, input, data_extent,
                                                 iary, namep, nlen, mask,
                                                 ocdata, rsz, controller, rank,
                                                 nwork_array, work_arrays,
                                                 local_work_arrays)) {
          iret = 0;
          break;
        }
      }

      jcntrl_char_array_delete(name);
    }
  }
  return iret;
}

static int jcntrl_abstract_stat_process_update_data_impl(
  jcntrl_information *request, jcntrl_input *input, jcntrl_output *output,
  jcntrl_shared_object *obj)
{
  jcntrl_information *iinfo, *oinfo;
  jcntrl_shared_object *iobj, *oobj;
  jcntrl_grid_data *ig, *og;
  jcntrl_abstract_stat *s;
  const jcntrl_mpi_controller *controller;
  jcntrl_size_type result_size;
  jcntrl_size_type nary;
  jcntrl_data_array **work_arrays;
  jcntrl_data_array **local_work_subarrays;
  int ret;
  int nproc, rank;

  s = jcntrl_abstract_stat_downcast_impl(obj);
  ret = jcntrl_abstract_stat_get_controller(s, &controller);
  if (!ret)
    return 0;

  nproc = 1;
  rank = 0;
  if (controller) {
    nproc = jcntrl_mpi_controller_nproc(controller);
    rank = jcntrl_mpi_controller_rank(controller);
  }

  input = jcntrl_input_next_port(input);
  output = jcntrl_output_next_port(output);

  JCNTRL_ASSERT(input);
  JCNTRL_ASSERT(output);

  iinfo = jcntrl_input_upstream_information(input);
  oinfo = jcntrl_output_information(output);

  JCNTRL_ASSERT(iinfo);
  JCNTRL_ASSERT(oinfo);

  iobj = jcntrl_information_get_object(iinfo, JCNTRL_INFO_DATA_OBJECT);
  oobj = jcntrl_information_get_object(oinfo, JCNTRL_INFO_DATA_OBJECT);

  JCNTRL_ASSERT(iobj);
  JCNTRL_ASSERT(oobj);

  ig = jcntrl_grid_data_downcast(iobj);
  og = jcntrl_grid_data_downcast(oobj);

  ret = jcntrl_abstract_stat_init_output_grid(s, ig, og, iinfo, oinfo);

  if (!(controller ? jcntrl_mpi_controller_all(controller, ret) : ret))
    return 0;

  if (!jcntrl_abstract_stat_calc_result_size(s, &result_size, og, controller))
    return 0;

  if (result_size <= 0)
    return 1;

  ret = jcntrl_abstract_stat_allocate_work_arrays(s, &nary, result_size, rank,
                                                  nproc, &work_arrays,
                                                  &local_work_subarrays);

  if (!(controller ? jcntrl_mpi_controller_all(controller, ret) : ret)) {
    jcntrl_abstract_stat_delete_work_arrays(nary, work_arrays,
                                            local_work_subarrays);
    return 0;
  }

  ret = jcntrl_abstract_stat_calc_for_grid(s, iinfo, ig, og, result_size, rank,
                                           nproc, controller, nary, work_arrays,
                                           local_work_subarrays);

  if (ret) {
    if (rank == s->root_rank)
      ret =
        jcntrl_abstract_stat_postprocess(s, oinfo, result_size, work_arrays);
    if (controller)
      ret = jcntrl_mpi_controller_all(controller, ret);
  }

  jcntrl_abstract_stat_delete_work_arrays(nary, work_arrays,
                                          local_work_subarrays);
  return ret;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_abstract_stat, jcntrl_executive, process_update_data)

static inline int jcntrl_abstract_stat_postprocess_impl(
  jcntrl_shared_object *obj, jcntrl_information *output_info,
  jcntrl_size_type result_size, jcntrl_data_array **work_arrays)
{
  /* nop */
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_abstract_stat, jcntrl_abstract_stat, postprocess)

static void jcntrl_abstract_stat_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_abstract_stat_downcast_v;
  p->initializer = jcntrl_abstract_stat_initializer;
  p->destructor = jcntrl_abstract_stat_destructor;
  p->allocator = NULL;
  p->deleter = NULL;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_abstract_stat, jcntrl_executive,
                          fill_input_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_abstract_stat, jcntrl_executive,
                          fill_output_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_abstract_stat, jcntrl_executive,
                          process_update_data);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_abstract_stat, jcntrl_abstract_stat,
                          postprocess);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_abstract_stat,
                                   jcntrl_abstract_stat_init_func)

jcntrl_shared_object *jcntrl_abstract_stat_object(jcntrl_abstract_stat *s)
{
  return jcntrl_executive_object(jcntrl_abstract_stat_executive(s));
}

jcntrl_executive *jcntrl_abstract_stat_executive(jcntrl_abstract_stat *s)
{
  return &s->exec;
}

jcntrl_abstract_stat *jcntrl_abstract_stat_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_abstract_stat, obj);
}

int jcntrl_abstract_stat_root_rank(jcntrl_abstract_stat *stat)
{
  JCNTRL_ASSERT(stat);
  return stat->root_rank;
}

void jcntrl_abstract_stat_set_root_rank(jcntrl_abstract_stat *stat, int rank)
{
  JCNTRL_ASSERT(stat);
  stat->root_rank = rank;
}

//----

static jcntrl_abstract_stat_all *
jcntrl_abstract_stat_all_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_abstract_stat_all, obj);
}

static void *jcntrl_abstract_stat_all_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_abstract_stat_all_downcast_impl(obj);
}

static int jcntrl_abstract_stat_all_initializer(jcntrl_shared_object *obj)
{
  jcntrl_abstract_stat_all *a = jcntrl_abstract_stat_all_downcast_impl(obj);
  a->controller = NULL;
  return 1;
}

static void jcntrl_abstract_stat_all_destructor(jcntrl_shared_object *obj)
{
  /* nop */
}

static int jcntrl_abstract_stat_all_is_root(jcntrl_abstract_stat_all *a)
{
  int root_rank;

  if (!a->controller) {
    jcntrl_abstract_stat_set_root_rank(&a->stat, 0);
    return 1;
  }

  root_rank = jcntrl_abstract_stat_root_rank(&a->stat);
  return jcntrl_mpi_controller_rank(a->controller) == root_rank;
}

static int jcntrl_abstract_stat_all_process_update_extent_impl(
  jcntrl_information *request, jcntrl_input *input, jcntrl_output *output,
  jcntrl_shared_object *obj)
{
  jcntrl_information *oinfo;
  jcntrl_abstract_stat_all *a;
  jcntrl_extent piece_extent;
  jcntrl_extent whole_extent;

  a = jcntrl_abstract_stat_all_downcast_impl(obj);

  output = jcntrl_output_next_port(output);
  JCNTRL_ASSERT(output);

  oinfo = jcntrl_output_information(output);
  JCNTRL_ASSERT(oinfo);

  if (jcntrl_abstract_stat_all_is_root(a)) {
    piece_extent = jcntrl_extent_i(0, 1, 0, 1, 0, 1, 0);
  } else {
    piece_extent = jcntrl_extent_empty();
  }
  whole_extent = jcntrl_extent_i(0, 1, 0, 1, 0, 1, 0);

  if (!jcntrl_information_set_extent(oinfo, JCNTRL_INFO_WHOLE_EXTENT,
                                     whole_extent.extent))
    return 0;
  if (!jcntrl_information_set_extent(oinfo, JCNTRL_INFO_PIECE_EXTENT,
                                     piece_extent.extent))
    return 0;

  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_abstract_stat_all, jcntrl_executive,
                    process_update_extent)

static int jcntrl_abstract_stat_all_get_coord(
  const jcntrl_mpi_controller *controller, jcntrl_data_array *d[3],
  const int *piece_extent, const int *data_extent, int root, double result[6])
{
  int nx, ny, nz;
  int offs[6];
  jcntrl_static_double_array dary;
  jcntrl_data_array *dd;
  jcntrl_extent extent;
  double l[3];

  extent = jcntrl_extent_c(data_extent);
  nx = jcntrl_extent_nx(extent);
  ny = jcntrl_extent_ny(extent);
  nz = jcntrl_extent_nz(extent);

  for (int i = 0; i < 6; i += 2) {
    offs[i + 0] = piece_extent[i + 0] - data_extent[i];
    offs[i + 1] = piece_extent[i + 1] - data_extent[i];
  }

  result[0] = result[2] = result[4] = HUGE_VAL;
  result[1] = result[3] = result[5] = -HUGE_VAL;
  if (nx > 0) {
    result[0] = jcntrl_data_array_get_value(d[0], offs[0]);
    result[1] = jcntrl_data_array_get_value(d[0], offs[1]);
  }
  if (ny > 0) {
    result[2] = jcntrl_data_array_get_value(d[1], offs[2]);
    result[3] = jcntrl_data_array_get_value(d[1], offs[3]);
  }
  if (nz > 0) {
    result[4] = jcntrl_data_array_get_value(d[2], offs[4]);
    result[5] = jcntrl_data_array_get_value(d[2], offs[5]);
  }
  if (!controller)
    return 1;

  jcntrl_static_double_array_init_base(&dary, l, 3);
  dd = jcntrl_static_double_array_data(&dary);

  for (int i = 0; i < 2; ++i) {
    const jcntrl_reduce_op *op;
    op = (i == 0) ? jcntrl_reduce_op_min() : jcntrl_reduce_op_max();

    for (int j = 0; j < 3; ++j)
      l[j] = result[2 * j + i];
    if (!jcntrl_mpi_controller_reduce(controller, dd, dd, op, root))
      return 0;
    for (int j = 0; j < 3; ++j)
      result[2 * j + i] = l[j];
  }

  return 1;
}

static int jcntrl_abstract_stat_all_init_output_grid_impl(
  jcntrl_shared_object *obj, jcntrl_grid_data *input, jcntrl_grid_data *output,
  jcntrl_information *input_info, jcntrl_information *output_info)
{
  jcntrl_abstract_stat_all *a = jcntrl_abstract_stat_all_downcast_impl(obj);
  int is_root;
  jcntrl_struct_grid g;
  const int *piece_extent;
  const int *data_extent;
  int root;
  int r;
  double coords[6];
  jcntrl_data_array *icoords[3];

  root = jcntrl_abstract_stat_root_rank(&a->stat);
  is_root = jcntrl_abstract_stat_all_is_root(a);

  jcntrl_struct_grid_init(&g);
  if (is_root) {
    jcntrl_struct_grid_set_extent(&g, (int[6]){0, 1, 0, 1, 0, 1});
  } else {
    jcntrl_struct_grid_set_extent(&g, jcntrl_extent_empty().extent);
  }

  piece_extent =
    jcntrl_information_get_extent(input_info, JCNTRL_INFO_PIECE_EXTENT);
  data_extent = jcntrl_grid_data_extent(input);

  icoords[0] = jcntrl_grid_data_x_coords(input);
  icoords[1] = jcntrl_grid_data_y_coords(input);
  icoords[2] = jcntrl_grid_data_z_coords(input);
  if (!jcntrl_abstract_stat_all_get_coord(a->controller, icoords, piece_extent,
                                          data_extent, root, coords))
    return 0;

  r = 1;
  do {
    if (!is_root)
      break;

    for (int i = 0; i < 3; ++i)
      icoords[i] = NULL;

    for (int i = 0; i < 3; ++i) {
      jcntrl_double_array *dd;
      double *d;
      dd = jcntrl_double_array_new();
      if (!dd) {
        r = 0;
        break;
      }

      icoords[i] = jcntrl_double_array_data(dd);
      if (!jcntrl_data_array_resize(icoords[i], 2)) {
        r = 0;
        break;
      }

      d = jcntrl_double_array_get_writable(dd);
      d[0] = coords[2 * i + 0];
      d[1] = coords[2 * i + 1];
    }
    if (!r) {
      for (int j = 0; j < 3; ++j)
        if (icoords[j])
          jcntrl_data_array_delete(icoords[j]);
      break;
    }

    jcntrl_struct_grid_set_x_coords(&g, icoords[0]);
    jcntrl_struct_grid_set_y_coords(&g, icoords[1]);
    jcntrl_struct_grid_set_z_coords(&g, icoords[2]);

    for (int i = 0; i < 3; ++i) {
      jcntrl_shared_object *o;
      o = jcntrl_data_array_object(icoords[i]);
      jcntrl_shared_object_release_ownership(o);
    }
  } while (0);
  if (!(a->controller ? jcntrl_mpi_controller_all(a->controller, r) : r)) {
    jcntrl_struct_grid_clear(&g);
    return 0;
  }

  jcntrl_grid_data_set_struct_grid(output, &g);
  jcntrl_struct_grid_clear(&g);
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_abstract_stat_all, jcntrl_abstract_stat,
                    init_output_grid)

int jcntrl_abstract_stat_all_get_controller_impl(
  jcntrl_shared_object *obj, const jcntrl_mpi_controller **controller)
{
  jcntrl_abstract_stat_all *a;
  a = jcntrl_abstract_stat_all_downcast_impl(obj);
  *controller = a->controller;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_abstract_stat_all, jcntrl_abstract_stat,
                    get_controller)

static void jcntrl_abstract_stat_all_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_abstract_stat_all_downcast_v;
  p->initializer = jcntrl_abstract_stat_all_initializer;
  p->destructor = jcntrl_abstract_stat_all_destructor;
  p->allocator = NULL;
  p->deleter = NULL;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_abstract_stat_all, jcntrl_executive,
                          process_update_extent);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_abstract_stat_all, jcntrl_abstract_stat,
                          init_output_grid);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_abstract_stat_all, jcntrl_abstract_stat,
                          get_controller);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_abstract_stat_all,
                                   jcntrl_abstract_stat_all_init_func)

jcntrl_shared_object *
jcntrl_abstract_stat_all_object(jcntrl_abstract_stat_all *s)
{
  return jcntrl_abstract_stat_object(&s->stat);
}

jcntrl_executive *
jcntrl_abstract_stat_all_executive(jcntrl_abstract_stat_all *s)
{
  return jcntrl_abstract_stat_executive(&s->stat);
}

jcntrl_abstract_stat_all *
jcntrl_abstract_stat_all_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_abstract_stat_all, obj);
}

void jcntrl_abstract_stat_all_set_controller(
  jcntrl_abstract_stat_all *s, const jcntrl_mpi_controller *controller)
{
  s->controller = controller;
}

const jcntrl_mpi_controller *
jcntrl_abstract_stat_all_controller(jcntrl_abstract_stat_all *s)
{
  return s->controller;
}
