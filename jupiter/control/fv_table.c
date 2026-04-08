
#include <math.h>
#include <stdlib.h>

#include "fv_table.h"
#include "defs.h"
#include "global.h"
#include "executive_data.h"
#include "executive.h"
#include "error.h"
#include "input.h"
#include "output.h"
#include "information.h"
#include "field_variable.h"
#include "shared_object.h"
#include "shared_object_priv.h"

#include <jupiter/table/table.h>

struct jcntrl_fv_table
{
  jcntrl_executive executive;
  table_data *table;
  double default_x;
  double default_y;
  jcntrl_fv_table_extend_mode extend_mode;
};
#define jcntrl_fv_table__ancestor jcntrl_executive
#define jcntrl_fv_table__dnmem executive.jcntrl_executive__dnmem
JCNTRL_VTABLE_NONE(jcntrl_fv_table);

static jcntrl_fv_table *jcntrl_fv_table_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jcntrl_fv_table, obj);
}

static void *jcntrl_fv_table_downcast_v(jcntrl_shared_object *obj)
{
  return jcntrl_fv_table_downcast_impl(obj);
}

static int jcntrl_fv_table_initialize(jcntrl_shared_object *obj)
{
  jcntrl_fv_table *ptr;

  ptr = jcntrl_fv_table_downcast_impl(obj);
  ptr->table = table_alloc();
  if (!ptr->table) {
    jcntrl_raise_allocation_failed(__FILE__, __LINE__);
    return 0;
  }

  ptr->extend_mode = JCNTRL_FV_TABLE_EXTEND_NEAREST;
  ptr->default_x = 0.0;
  ptr->default_y = 0.0;

  {
    jcntrl_input *input;
    jcntrl_output *output;

    input = jcntrl_executive_get_input(jcntrl_fv_table_executive(ptr));
    output = jcntrl_executive_get_output(jcntrl_fv_table_executive(ptr));
    if (!jcntrl_input_set_number_of_ports(input, 2))
      return 0;
    if (!jcntrl_output_set_number_of_ports(output, 1))
      return 0;
  }
  return 1;
}

static void jcntrl_fv_table_desctructor(jcntrl_shared_object *obj)
{
  jcntrl_fv_table *ptr;

  ptr = jcntrl_fv_table_downcast_impl(obj);
  if (ptr->table)
    table_free(ptr->table);
  ptr->table = NULL;
}

static jcntrl_shared_object *jcntrl_fv_table_allocator(void)
{
  jcntrl_fv_table *ptr;
  ptr = jcntrl_shared_object_default_allocator(jcntrl_fv_table);
  return ptr ? jcntrl_fv_table_object(ptr) : NULL;
}

static void jcntrl_fv_table_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

jcntrl_fv_table *jcntrl_fv_table_new(void)
{
  return jcntrl_shared_object_new(jcntrl_fv_table);
}

void jcntrl_fv_table_delete(jcntrl_fv_table *fv)
{
  JCNTRL_ASSERT(fv);
  jcntrl_shared_object_delete(jcntrl_fv_table_object(fv));
}

jcntrl_fv_table *jcntrl_fv_table_downcast(jcntrl_shared_object *object)
{
  return jcntrl_shared_object_downcast(jcntrl_fv_table, object);
}

jcntrl_executive *jcntrl_fv_table_executive(jcntrl_fv_table *fv)
{
  return &fv->executive;
}

jcntrl_shared_object *jcntrl_fv_table_object(jcntrl_fv_table *fv)
{
  return jcntrl_executive_object(jcntrl_fv_table_executive(fv));
}

void jcntrl_fv_table_set_default_x(jcntrl_fv_table *fv, double x)
{
  JCNTRL_ASSERT(fv);

  fv->default_x = x;
}

void jcntrl_fv_table_set_default_y(jcntrl_fv_table *fv, double y)
{
  JCNTRL_ASSERT(fv);

  fv->default_y = y;
}

double jcntrl_fv_table_get_default_x(jcntrl_fv_table *fv)
{
  JCNTRL_ASSERT(fv);
  return fv->default_x;
}

double jcntrl_fv_table_get_default_y(jcntrl_fv_table *fv)
{
  JCNTRL_ASSERT(fv);
  return fv->default_y;
}

void jcntrl_fv_table_set_extend_mode(jcntrl_fv_table *fv,
                                     jcntrl_fv_table_extend_mode mode)
{
  JCNTRL_ASSERT(fv);
  fv->extend_mode = mode;
}

jcntrl_fv_table_extend_mode jcntrl_fv_table_get_extend_mode(jcntrl_fv_table *fv)
{
  JCNTRL_ASSERT(fv);
  return fv->extend_mode;
}

int jcntrl_fv_table_set_table(jcntrl_fv_table *fv, table_data *table)
{
  table_size nx, ny, nv;
  table_error e;
  table_geometry geom;
  const double *x, *y, *v;

  JCNTRL_ASSERT(fv);
  JCNTRL_ASSERT(table);
  JCNTRL_ASSERT(table_inited(table));
  JCNTRL_ASSERT(table_get_geometry(table) == TABLE_GEOMETRY_RECTILINEAR);

  if (!table_inited(table)) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Given table is not initialized");
    return 1;
  }

  geom = table_get_geometry(table);
  if (geom != TABLE_GEOMETRY_RECTILINEAR) {
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Given table must be rectilinear shaped");
    return 1;
  }

  nx = table_get_nx(table);
  ny = table_get_ny(table);
  nv = table_calc_data_size(geom, nx, ny);

  x = table_get_xdata(table);
  y = table_get_ydata(table);
  v = table_get_data(table);

  e = table_init(fv->table, "", geom, nx, ny, TABLE_INTERP_LINEAR, x, y, v);
  if (e != TABLE_SUCCESS) {
    jcntrl_raise_table_error(__FILE__, __LINE__, e, NULL);
  }
  return 0;
}

int jcntrl_fv_table_set_table_1d(jcntrl_fv_table *fv, int npt, const double *x,
                                 const double *v)
{
  double y;
  y = 0.0;

  return jcntrl_fv_table_set_table_2d(fv, npt, 1, x, &y, v);
}

int jcntrl_fv_table_set_table_2d(jcntrl_fv_table *fv, int nx, int ny,
                                 const double *x, const double *y,
                                 const double *v)
{
  table_error e;

  JCNTRL_ASSERT(fv);
  JCNTRL_ASSERT(fv->table);
  JCNTRL_ASSERT(nx > 0);
  JCNTRL_ASSERT(ny > 0);
  JCNTRL_ASSERT(x);
  JCNTRL_ASSERT(y);
  JCNTRL_ASSERT(v);

  e = table_init(fv->table, "", TABLE_GEOMETRY_RECTILINEAR, nx, ny,
                 TABLE_INTERP_LINEAR, x, y, v);
  if (e != TABLE_SUCCESS) {
    jcntrl_raise_table_error(__FILE__, __LINE__, e, NULL);
    return 0;
  }

  e = table_set_algorithm(fv->table, TABLE_SALG_BIN_TREE_MAX);
  if (e != TABLE_SUCCESS) {
    jcntrl_raise_table_error(__FILE__, __LINE__, e, NULL);
    return 0;
  }

  return 1;
}

static double jcntrl_fv_table_adjust_extrapolate(double min, double max,
                                                 double x)
{
  return x;
}

static double jcntrl_fv_table_adjust_circular(double min, double max, double x)
{
  double d;
  d = max - min;
  if (d > 0.0) {
    if (x < min) {
      x = x + floor((max - x) / d) * d;
    } else if (x > max) {
      x = x - floor((x - min) / d) * d;
    }
  } else {
    x = min;
  }
  return x;
}

static double jcntrl_fv_table_adjust_mirror(double min, double max, double x)
{
  double d;
  d = max - min;
  if (d > 0.0) {
    x = jcntrl_fv_table_adjust_circular(min, max + d, x);
    if (x > max) {
      x = max - (x - max);
    }
  } else {
    x = min;
  }
  return x;
}

static double jcntrl_fv_table_adjust_nearest(double min, double max, double x)
{
  if (x < min)
    return min;
  if (x > max)
    return max;
  return x;
}

static int jcntrl_fv_table_adjust_range(jcntrl_fv_table *fv, double *rx,
                                        double *ry)
{
  table_size nx, ny;
  const double *xdata, *ydata;
  double xmin, xmax, ymin, ymax;
  double x, y;

  JCNTRL_ASSERT(fv);
  JCNTRL_ASSERT(fv->table);
  JCNTRL_ASSERT(rx);
  JCNTRL_ASSERT(ry);

  x = *rx;
  y = *ry;

  nx = table_get_nx(fv->table);
  ny = table_get_ny(fv->table);
  xdata = table_get_xdata(fv->table);
  ydata = table_get_ydata(fv->table);

  JCNTRL_ASSERT(nx >= 1);
  JCNTRL_ASSERT(ny >= 1);

  xmin = xdata[0];
  xmax = xdata[nx - 1];
  ymin = ydata[0];
  ymax = ydata[ny - 1];

  switch (fv->extend_mode) {
  case JCNTRL_FV_TABLE_EXTEND_EXTRAPOLATE:
    x = jcntrl_fv_table_adjust_extrapolate(xmin, xmax, x);
    y = jcntrl_fv_table_adjust_extrapolate(ymin, ymax, y);
    break;
  case JCNTRL_FV_TABLE_EXTEND_CIRCULAR:
    x = jcntrl_fv_table_adjust_circular(xmin, xmax, x);
    y = jcntrl_fv_table_adjust_circular(ymin, ymax, y);
    break;
  case JCNTRL_FV_TABLE_EXTEND_MIRROR:
    x = jcntrl_fv_table_adjust_mirror(xmin, xmax, x);
    y = jcntrl_fv_table_adjust_mirror(ymin, ymax, y);
    break;
  case JCNTRL_FV_TABLE_EXTEND_NEAREST:
    x = jcntrl_fv_table_adjust_nearest(xmin, xmax, x);
    y = jcntrl_fv_table_adjust_nearest(ymin, ymax, y);
    break;
  case JCNTRL_FV_TABLE_EXTEND_INVALID:
  default:
    JCNTRL_ASSERT_X(0,
                    "Unreachable reached (Invalid table extension mode used)");
    jcntrl_raise_argument_error(__FILE__, __LINE__,
                                "Invalid table extension mode used "
                                "(This error will occur even if the input "
                                "is within the valid range)");
    return 0;
  }

  *rx = x;
  *ry = y;
  return 1;
}

double jcntrl_fv_table_get_interpolated(jcntrl_fv_table *fv, double x, double y,
                                        int *stat)
{
  double ret;
  struct table_node node;
  table_error e;

  JCNTRL_ASSERT(fv);
  JCNTRL_ASSERT(fv->table);

  if (!jcntrl_fv_table_adjust_range(fv, &x, &y)) {
    if (stat)
      *stat = 1;
    return 0.0;
  }

  e = TABLE_SUCCESS;
  ret = table_search(fv->table, x, y, &e);
  if (e != TABLE_SUCCESS) {
    if (e != TABLE_ERR_TABLE_RANGE ||
        fv->extend_mode != JCNTRL_FV_TABLE_EXTEND_EXTRAPOLATE) {
      jcntrl_raise_table_error(__FILE__, __LINE__, e, NULL);
    }
  }

  return ret;
}

static int
jcntrl_fv_table_fill_input_port_information_impl(jcntrl_shared_object *obj,
                                                 int index, jcntrl_input *input)
{
  jcntrl_information *info;
  jcntrl_fv_table *fv;
  fv = jcntrl_fv_table_downcast_impl(obj);

  info = jcntrl_input_information(input);
  if (!jcntrl_information_set_datatype(info, JCNTRL_INFO_REQUIRED_DATATYPE,
                                       JCNTRL_DATATYPE_FIELD_VAR)) {
    return 0;
  }
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_fv_table, jcntrl_executive,
                    fill_input_port_information)

static int jcntrl_fv_table_fill_output_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_output *output)
{
  jcntrl_information *info;
  jcntrl_fv_table *fv;
  fv = jcntrl_fv_table_downcast_impl(obj);

  info = jcntrl_output_information(output);
  if (!jcntrl_information_set_objecttype(info, JCNTRL_INFO_DATATYPE,
                                         jcntrl_field_variable)) {
    return 0;
  }
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_fv_table, jcntrl_executive,
                    fill_output_port_information)

static double jcntrl_fv_table_get_input_value(jcntrl_input *input,
                                              double errval)
{
  jcntrl_information *info;
  jcntrl_shared_object *object;
  jcntrl_field_variable *fvar;
  int err;
  double ret;

  info = jcntrl_input_upstream_information(input);
  if (!info)
    return errval;

  object = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
  if (!object)
    return errval;

  fvar = jcntrl_field_variable_downcast(object);
  if (!fvar)
    return errval;

  err = 0;
  ret = jcntrl_field_variable_value(fvar, &err);
  if (err)
    return errval;

  return ret;
}

static int jcntrl_fv_table_process_update_data_impl(jcntrl_information *request,
                                                    jcntrl_input *input,
                                                    jcntrl_output *output,
                                                    jcntrl_shared_object *obj)
{
  jcntrl_fv_table *fv;
  jcntrl_input *in1, *in2;
  double x, y;
  double r;
  int stat;

  fv = jcntrl_fv_table_downcast_impl(obj);

  in1 = jcntrl_input_next_port(input);
  in2 = NULL;
  if (in1) {
    in2 = jcntrl_input_next_port(in1);
  } else {
    return 0;
  }

  output = jcntrl_output_next_port(output);
  if (!output)
    return 0;

  x = fv->default_x;
  y = fv->default_y;
  if (in1) {
    x = jcntrl_fv_table_get_input_value(in1, fv->default_x);
  }
  if (in2) {
    y = jcntrl_fv_table_get_input_value(in2, fv->default_y);
  }

  stat = 0;
  r = jcntrl_fv_table_get_interpolated(fv, x, y, &stat);
  if (stat) {
    return 0;
  }

  {
    jcntrl_information *info;
    jcntrl_shared_object *object;
    jcntrl_field_variable *fvar;

    info = jcntrl_output_information(output);
    if (!info)
      return 0;

    object = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
    if (!object)
      return 0;

    fvar = jcntrl_field_variable_downcast(object);
    if (!fvar)
      return 0;

    if (!jcntrl_field_variable_set_value(fvar, r))
      return 0;
  }
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jcntrl_fv_table, jcntrl_executive, process_update_data)

static void jcntrl_fv_table_init_func(jcntrl_shared_object_funcs *p)
{
  p->initializer = jcntrl_fv_table_initialize;
  p->destructor = jcntrl_fv_table_desctructor;
  p->allocator = jcntrl_fv_table_allocator;
  p->deleter = jcntrl_fv_table_deleter;
  p->downcast = jcntrl_fv_table_downcast_v;
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_fv_table, jcntrl_executive,
                          fill_input_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_fv_table, jcntrl_executive,
                          fill_output_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jcntrl_fv_table, jcntrl_executive,
                          process_update_data);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jcntrl_fv_table, jcntrl_fv_table_init_func)

int jcntrl_install_fv_table(void)
{
  int rid = jcntrl_executive_install(JCNTRL_FV_TABLE, jcntrl_fv_table);
  return (rid == JCNTRL_FV_TABLE);
}
