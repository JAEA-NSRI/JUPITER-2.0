#include "postp_mask.h"
#include "cell.h"
#include "data_array.h"
#include "defs.h"
#include "error.h"
#include "executive.h"
#include "executive_data.h"
#include "extent.h"
#include "information.h"
#include "input.h"
#include "logical_operator.h"
#include "mask_data.h"
#include "mask_function.h"
#include "output.h"
#include "shared_object.h"
#include "shared_object_priv.h"
#include "grid_data.h"

#include <string.h>

struct jcntrl_postp_mask
{
  jcntrl_executive exec;
  jcntrl_logical_operator op;
};
#define jcntrl_postp_mask__ancestor jcntrl_executive
#define jcntrl_postp_mask__dnmem exec.jcntrl_executive__dnmem
JCNTRL_VTABLE_NONE(jcntrl_postp_mask);

static jcntrl_postp_mask *
jcntrl_postp_mask_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_postp_mask, obj);
}

static void *jcntrl_postp_mask_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_postp_mask_downcast_impl(obj);
}

static int jcntrl_postp_mask_initializer(jcntrl_shared_object *obj)
{
  jcntrl_postp_mask *m = jcntrl_postp_mask_downcast_impl(obj);
  jcntrl_executive *exe;
  jcntrl_input *input;
  jcntrl_output *output;

  m->op = JCNTRL_LOP_ADD;

  exe = jcntrl_postp_mask_executive(m);
  input = jcntrl_executive_get_input(exe);
  output = jcntrl_executive_get_output(exe);

  if (!jcntrl_input_set_number_of_ports(input, 2))
    return 0;
  if (!jcntrl_output_set_number_of_ports(output, 1))
    return 0;
  return 1;
}

static void jcntrl_postp_mask_destructor(jcntrl_shared_object *obj)
{
  /* nop */
}

static jcntrl_shared_object *jcntrl_postp_mask_allocator(void)
{
  jcntrl_postp_mask *m;
  m = jcntrl_shared_object_default_allocator(jcntrl_postp_mask);
  return m ? jcntrl_postp_mask_object(m) : NULL;
}

static void jcntrl_postp_mask_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int jcntrl_postp_mask_fill_input_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_input *port)
{
  jcntrl_information *info;

  info = jcntrl_input_information(port);
  switch (index) {
  case 0:
    if (!jcntrl_information_set_datatype(info, JCNTRL_INFO_REQUIRED_DATATYPE,
                                         JCNTRL_DATATYPE_GRID))
      return 0;
    break;
  case 1:
    /* Mask array (and extent) is not supported yet */
    if (!jcntrl_information_set_datatype(info, JCNTRL_INFO_REQUIRED_DATATYPE,
                                         JCNTRL_DATATYPE_MASK_FUN))
      return 0;
    break;
  }
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_mask, jcntrl_executive,
                    fill_input_port_information)

static int jcntrl_postp_mask_fill_output_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_output *port)
{
  jcntrl_information *info;

  info = jcntrl_output_information(port);

  jcntrl_information_set_objecttype(info, JCNTRL_INFO_DATATYPE,
                                    jcntrl_grid_data);
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_mask, jcntrl_executive,
                    fill_output_port_information)

static int jcntrl_postp_mask_process_update_information_impl(
  jcntrl_information *request, jcntrl_input *input, jcntrl_output *output,
  jcntrl_shared_object *obj)
{
  /* nop */
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_mask, jcntrl_executive,
                    process_update_information)

static int jcntrl_postp_mask_process_update_extent_impl(
  jcntrl_information *request, jcntrl_input *input, jcntrl_output *output,
  jcntrl_shared_object *obj)
{
  jcntrl_input *gport;
  jcntrl_postp_mask *m;
  jcntrl_information *iinfo, *oinfo;
  const int *ext;

  m = jcntrl_postp_mask_downcast_impl(obj);
  gport = jcntrl_postp_mask_get_grid_input(m);
  if (!gport)
    return 0;

  output = jcntrl_output_next_port(output);

  iinfo = jcntrl_input_upstream_information(gport);
  oinfo = jcntrl_output_information(output);

  if (!iinfo)
    return 0;
  if (!oinfo)
    return 0;

  ext = jcntrl_information_get_extent(iinfo, JCNTRL_INFO_WHOLE_EXTENT);
  if (ext)
    if (!jcntrl_information_set_extent(oinfo, JCNTRL_INFO_WHOLE_EXTENT, ext))
      return 0;

  ext = jcntrl_information_get_extent(iinfo, JCNTRL_INFO_DATA_EXTENT);
  if (ext)
    if (!jcntrl_information_set_extent(oinfo, JCNTRL_INFO_DATA_EXTENT, ext))
      return 0;

  ext = jcntrl_information_get_extent(iinfo, JCNTRL_INFO_PIECE_EXTENT);
  if (ext)
    if (!jcntrl_information_set_extent(oinfo, JCNTRL_INFO_PIECE_EXTENT, ext))
      return 0;

  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_mask, jcntrl_executive, process_update_extent)

struct ary_evaluator_data
{
  const char *ary;
  jcntrl_size_type jj;
};

static int jcntrl_postp_mask_ary_evaluator(const void *arg)
{
  const struct ary_evaluator_data *p = arg;
  return p->ary[p->jj];
}

struct func_evaluator_data
{
  jcntrl_mask_function *func;
  const int *whole_extent;
  jcntrl_grid_data *d;
  jcntrl_size_type jj;
  int *ret;
};

static int
jcntrl_postp_mask_evaluator_index(const struct func_evaluator_data *p, //
                                  int *i, int *j, int *k)
{
  if (!jcntrl_extent_index(jcntrl_extent_c(jcntrl_grid_data_extent(p->d)),
                           p->jj, i, j, k)) {
    if (p->ret) {
#ifdef _OPENMP
#pragma omp atomic write
      *p->ret = 0;
#endif
    }
    return 0;
  }
  return 1;
}

static int jcntrl_postp_mask_evaluator(const void *arg)
{
  int i, j, k;
  const struct func_evaluator_data *p = arg;
  if (jcntrl_postp_mask_evaluator_index(p, &i, &j, &k)) {
    jcntrl_cell_hex h;
    jcntrl_cell_hex_init_g(&h, p->d, i, j, k, p->whole_extent);
    return jcntrl_mask_function_eval(p->func, jcntrl_cell_hex_cell(&h), i, j,
                                     k);
  }
  return 0;
}

static int jcntrl_postp_mask_process_update_data_impl(
  jcntrl_information *request, jcntrl_input *input, jcntrl_output *output,
  jcntrl_shared_object *obj)
{
  jcntrl_postp_mask *m;
  jcntrl_input *gport;
  jcntrl_input *mport;
  jcntrl_information *iginfo, *iminfo;
  jcntrl_information *oinfo;
  jcntrl_shared_object *objp;
  jcntrl_mask_data *imdata, *igmdata, *ogmdata;
  jcntrl_mask_function *imfunc;
  jcntrl_grid_data *igrid, *ogrid;
  jcntrl_bool_array *ibary, *imary, *obary;
  const char *ibptr, *imptr;
  char *obptr;
  jcntrl_size_type ncells;
  jcntrl_extent ext;
  const int *whole_extent;
  int imfunc_index;
  int r;

  m = jcntrl_postp_mask_downcast_impl(obj);

  output = jcntrl_output_next_port(output);
  JCNTRL_ASSERT(output);

  gport = jcntrl_postp_mask_get_grid_input(m);
  mport = jcntrl_postp_mask_get_mask_input(m);

  iginfo = jcntrl_input_upstream_information(gport);
  iminfo = jcntrl_input_upstream_information(mport);
  oinfo = jcntrl_output_information(output);

  if (!iginfo)
    return 0;
  if (!iminfo)
    return 0;
  if (!oinfo)
    return 0;

  whole_extent = NULL;
  igrid = NULL;
  ogrid = NULL;
  imdata = NULL;
  imfunc = NULL;

  objp = jcntrl_information_get_object(iginfo, JCNTRL_INFO_DATA_OBJECT);
  if (objp)
    igrid = jcntrl_grid_data_downcast(objp);

  whole_extent =
    jcntrl_information_get_extent(iginfo, JCNTRL_INFO_WHOLE_EXTENT);

  objp = jcntrl_information_get_object(iminfo, JCNTRL_INFO_DATA_OBJECT);
  if (objp) {
    imdata = jcntrl_mask_data_downcast(objp);
    if (!imdata)
      imfunc = jcntrl_mask_function_downcast(objp);
  }

  objp = jcntrl_information_get_object(oinfo, JCNTRL_INFO_DATA_OBJECT);
  if (objp)
    ogrid = jcntrl_grid_data_downcast(objp);

  if (!igrid)
    return 0;
  if (!ogrid)
    return 0;
  if (!(imdata || imfunc))
    return 0;

  ext = jcntrl_extent_c(jcntrl_grid_data_extent(igrid));
  ncells = jcntrl_extent_size(ext);

  if (!jcntrl_grid_data_shallow_copy(ogrid, igrid))
    return 0;

  if (ncells <= 0) {
    jcntrl_grid_data_set_mask(ogrid, NULL);
    return 1;
  }

  imary = NULL;
  if (imdata) {
    imary = jcntrl_mask_data_array(imdata);
    if (jcntrl_bool_array_get_ntuple(imary) != ncells) {
      jcntrl_raise_argument_error(__FILE__, __LINE__,
                                  "Mask data to apply does not match the size");
      return 0;
    }
  }

  igmdata = jcntrl_grid_data_get_mask(igrid);
  ogmdata = jcntrl_grid_data_get_mask(ogrid);
  if (!ogmdata) {
    ogmdata = jcntrl_mask_data_new();
    if (!ogmdata)
      return 0;

    jcntrl_grid_data_set_mask(ogrid, ogmdata);
    jcntrl_shared_object_release_ownership(jcntrl_mask_data_object(ogmdata));
  }
  JCNTRL_ASSERT(igmdata != ogmdata);

  ibary = NULL;
  if (igmdata)
    ibary = jcntrl_mask_data_array(igmdata);
  if (ibary)
    JCNTRL_ASSERT(jcntrl_bool_array_get_ntuple(ibary) == ncells);

  /* No masks can be applied */
  if (!(imary || imfunc)) {
    jcntrl_mask_data_set_array(ogmdata, ibary);
    return 1;
  }

  obary = jcntrl_bool_array_new();
  if (!obary)
    return 0;

  if (!jcntrl_bool_array_resize(obary, ncells)) {
    jcntrl_bool_array_delete(obary);
    return 0;
  }

  jcntrl_mask_data_set_array(ogmdata, obary);
  jcntrl_shared_object_release_ownership(jcntrl_bool_array_object(obary));

  obptr = jcntrl_bool_array_get_writable(obary);
  if (!obptr)
    return 0;

  imptr = NULL;
  if (imary)
    imptr = jcntrl_bool_array_get(imary);

  ibptr = NULL;
  if (ibary)
    ibptr = jcntrl_bool_array_get(ibary);

  jcntrl_cell_hex_metadata_init();
  r = 1;
#ifdef _OPENMP
#pragma omp parallel for
#endif
  for (jcntrl_size_type ii = 0; ii < ncells; ++ii) {
    jcntrl_logical_evaluator ev[2];
    struct ary_evaluator_data ae[2];
    struct func_evaluator_data f;
    int n = 0;
    int ival = 0;

    if (imptr) {
      ae[n] = (struct ary_evaluator_data){.ary = imptr, .jj = ii};
      ev[n] = (jcntrl_logical_evaluator){
        .f = jcntrl_postp_mask_ary_evaluator,
        .arg = &ae[n],
      };
      ++n;
    }
    if (ibptr) {
      ae[n] = (struct ary_evaluator_data){.ary = ibptr, .jj = ii};
      ev[n] = (jcntrl_logical_evaluator){
        .f = jcntrl_postp_mask_ary_evaluator,
        .arg = &ae[n],
      };
      ++n;
    } else if (imfunc) {
      f = (struct func_evaluator_data){
        .func = imfunc,
        .whole_extent = whole_extent,
        .d = igrid,
        .jj = ii,
        .ret = &r,
      };
      ev[n] = (jcntrl_logical_evaluator){
        .f = jcntrl_postp_mask_evaluator,
        .arg = &f,
      };
      ++n;
    }

    if (n > 0)
      ival = jcntrl_logical_calc_n(m->op, n, ev);

    obptr[ii] = ival;

#ifndef _OPENMP
    if (!r)
      break;
#endif
  }
  if (!r)
    return 0;

  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_postp_mask, jcntrl_executive, process_update_data)

static void jcntrl_postp_mask_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jcntrl_postp_mask_downcast_v;
  p->initializer = jcntrl_postp_mask_initializer;
  p->destructor = jcntrl_postp_mask_destructor;
  p->allocator = jcntrl_postp_mask_allocator;
  p->deleter = jcntrl_postp_mask_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_mask, jcntrl_executive,
                          fill_input_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_mask, jcntrl_executive,
                          fill_output_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_mask, jcntrl_executive,
                          process_update_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_mask, jcntrl_executive,
                          process_update_extent);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_postp_mask, jcntrl_executive,
                          process_update_data);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_postp_mask,
                                   jcntrl_postp_mask_init_func)

jcntrl_postp_mask *jcntrl_postp_mask_new(void)
{
  return jcntrl_shared_object_new(jcntrl_postp_mask);
}

void jcntrl_postp_mask_delete(jcntrl_postp_mask *m)
{
  jcntrl_shared_object_delete(jcntrl_postp_mask_object(m));
}

jcntrl_executive *jcntrl_postp_mask_executive(jcntrl_postp_mask *m)
{
  return &m->exec;
}

jcntrl_shared_object *jcntrl_postp_mask_object(jcntrl_postp_mask *m)
{
  return jcntrl_executive_object(jcntrl_postp_mask_executive(m));
}

jcntrl_postp_mask *jcntrl_postp_mask_downcast(jcntrl_shared_object *obj)
{
  return jcntrl_shared_object_downcast(jcntrl_postp_mask, obj);
}

jcntrl_input *jcntrl_postp_mask_get_grid_input(jcntrl_postp_mask *m)
{
  return jcntrl_executive_input_port(jcntrl_postp_mask_executive(m), 0);
}

jcntrl_input *jcntrl_postp_mask_get_mask_input(jcntrl_postp_mask *m)
{
  return jcntrl_executive_input_port(jcntrl_postp_mask_executive(m), 1);
}

jcntrl_logical_operator jcntrl_postp_mask_get_op(jcntrl_postp_mask *m)
{
  JCNTRL_ASSERT(m);
  return m->op;
}

void jcntrl_postp_mask_set_op(jcntrl_postp_mask *m, jcntrl_logical_operator op)
{
  JCNTRL_ASSERT(m);
  m->op = op;
}
