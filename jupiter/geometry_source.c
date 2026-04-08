#include "geometry_source.h"
#include "control/data_array.h"
#include "control/defs.h"
#include "control/error.h"
#include "control/executive.h"
#include "control/executive_data.h"
#include "control/extent.h"
#include "control/global.h"
#include "control/geometry.h"
#include "control/geometry_data.h"
#include "control/information.h"
#include "control/input.h"
#include "control/output.h"
#include "control/manager.h"
#include "control/shared_object.h"
#include "control/shared_object_priv.h"
#include "control/struct_data.h"
#include "csvutil.h"
#include "field_control.h"
#include "func.h"
#include "geometry/data.h"
#include "geometry/defs.h"
#include "geometry/file.h"
#include "geometry/shape.h"
#include "geometry/svector.h"
#include "geometry/udata-priv.h"
#include "geometry/udata.h"
#include "real_array.h"
#include "struct.h"

#include <stdlib.h>
#include <string.h>

struct jupiter_geometry_object
{
  jcntrl_geometry data;
  domain *cdo;
  mpi_param *mpi;
};
typedef struct jupiter_geometry_object jupiter_geometry_object;
#define jupiter_geometry_object__ancestor jcntrl_geometry
#define jupiter_geometry_object__dnmem data.jcntrl_geometry__dnmem
JCNTRL_VTABLE_NONE(jupiter_geometry_object);

static jupiter_geometry_object *
jupiter_geometry_object_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jupiter_geometry_object, obj);
}

static void *jupiter_geometry_object_downcast_v(jcntrl_shared_object *obj)
{
  return jupiter_geometry_object_downcast_impl(obj);
}

static jcntrl_shared_object *jupiter_geometry_object_allocator(void)
{
  jupiter_geometry_object *p;
  p = jcntrl_shared_object_default_allocator(jupiter_geometry_object);
  return p ? jcntrl_geometry_object(&p->data) : NULL;
}

static void jupiter_geometry_object_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int jupiter_geometry_object_initializer(jcntrl_shared_object *obj)
{
  jupiter_geometry_object *o = jupiter_geometry_object_downcast_impl(obj);
  o->mpi = NULL;
  o->cdo = NULL;
  return 1;
}

static void jupiter_geometry_object_destructor(jcntrl_shared_object *obj)
{
  /* nop */
}

static int jupiter_geometry_object_file_load_impl(jcntrl_shared_object *obj,
                                                  jcntrl_data_array **ret_array,
                                                  int *ret_extent)
{
  jupiter_geometry_object *p = jupiter_geometry_object_downcast_impl(obj);
  int ret;
  int alloc;
  type *f;
  binary_output_mode unif;
  geom_data_element *el;
  geom_file_data *fdata;
  const geom_user_defined_data *ud;
  jupiter_geom_ext_file_data *extf;
  geom_svec3 size, offs, rep, orig, gsize, goffs, grep, gorig;
  const char *afname;

  el = jcntrl_geometry_get_data_element(&p->data);
  if (!el)
    return 0;

  fdata = geom_data_element_get_file(el);
  if (!fdata)
    return 0;

  afname = geom_file_data_get_alt_file_path(fdata);
  if (!afname || strlen(afname) == 0)
    return 0;

  ud = geom_file_data_get_extra_data(fdata);
  extf = geom_user_defined_data_get(ud);
  CSVASSERT(extf);

  alloc = 0;
  if (*ret_array) {
#ifdef JUPITER_DOUBLE
    alloc = !jcntrl_data_array_get_double(*ret_array);
#else
    alloc = !jcntrl_data_array_get_float(*ret_array);
#endif
  } else {
    alloc = 1;
  }
  if (alloc) {
    jupiter_real_array *f;
    f = jupiter_real_array_new();
    if (!f)
      return 0;

    *ret_array = jupiter_real_array_data_array(f);
  }

  if (!jcntrl_data_array_resize(*ret_array, p->cdo->n))
    return 0;

#ifdef JUPITER_DOUBLE
  f = jcntrl_data_array_get_writable_double(*ret_array);
#else
  f = jcntrl_data_array_get_writable_float(*ret_array);
#endif
  if (!f)
    return 0;

  gsize = geom_svec3_c(p->cdo->gnx, p->cdo->gny, p->cdo->gnz);
  goffs = geom_svec3_c(0, 0, 0);
  grep = geom_svec3_c(1, 1, 1);
  gorig = geom_svec3_c(0, 0, 0);

  unif = extf->read_mode;
  if (unif != BINARY_OUTPUT_BYPROCESS) {
    size = geom_file_data_get_size(fdata);
    offs = geom_file_data_get_offset(fdata);
    rep = geom_file_data_get_repeat(fdata);
    orig = geom_file_data_get_origin(fdata);
  } else {
    size = geom_svec3_c(p->cdo->gnx, p->cdo->gny, p->cdo->gnz);
    offs = geom_svec3_c(0, 0, 0);
    rep = geom_svec3_c(1, 1, 1);
    orig = geom_svec3_c(0, 0, 0);
  }
  if (unif != BINARY_OUTPUT_BYPROCESS &&
      !(geom_svec3_eql(size, gsize) && geom_svec3_eql(offs, goffs) &&
        geom_svec3_eql(rep, grep) && geom_svec3_eql(orig, gorig))) {
    ret = input_geometry_binary_unified(p->mpi, f, 0, 0, 0, 0, 0, 0, p->cdo->nx,
                                        p->cdo->ny, p->cdo->nz, 1, afname, 1,
                                        orig, size, rep, offs);
  } else {
    ret = input_binary(p->mpi, f, NULL, 0, 0, 0, 0, 0, 0, p->cdo->nx,
                       p->cdo->ny, p->cdo->nz, 1, afname, unif, 1);
  }
  if (ret == 0) {
    ret_extent[0] = p->mpi->rank_x * p->cdo->nx;
    ret_extent[1] = ret_extent[0] + p->cdo->nx;
    ret_extent[2] = p->mpi->rank_y * p->cdo->ny;
    ret_extent[3] = ret_extent[2] + p->cdo->ny;
    ret_extent[4] = p->mpi->rank_z * p->cdo->nz;
    ret_extent[5] = ret_extent[4] + p->cdo->nz;
  }

  /*
   * input_binary() and input_geometry_binary_unified() is positive error.
   */
  return ret == 0;
}

JCNTRL_VIRTUAL_WRAP(jupiter_geometry_object, jcntrl_geometry, file_load)

static void jupiter_geometry_object_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jupiter_geometry_object_downcast_v;
  p->allocator = jupiter_geometry_object_allocator;
  p->deleter = jupiter_geometry_object_deleter;
  p->initializer = jupiter_geometry_object_initializer;
  p->destructor = jupiter_geometry_object_destructor;
  JCNTRL_VIRTUAL_WRAP_SET(p, jupiter_geometry_object, jcntrl_geometry,
                          file_load);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jupiter_geometry_object,
                                   jupiter_geometry_object_init_func)

//--- jupiter_geometry_source

struct jupiter_geometry_source
{
  jcntrl_executive executive;
  geom_data_element *element;
  domain *cdo;
  mpi_param *mpi;
};
#define jupiter_geometry_source__ancestor jcntrl_executive
#define jupiter_geometry_source__dnmem executive.jcntrl_executive__dnmem
JCNTRL_VTABLE_NONE(jupiter_geometry_source);

static jupiter_geometry_source *
jupiter_geometry_source_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jupiter_geometry_source, obj);
}

static void *jupiter_geometry_source_downcast_v(jcntrl_shared_object *obj)
{
  return jupiter_geometry_source_downcast_impl(obj);
}

static jcntrl_shared_object *jupiter_geometry_source_allocator(void)
{
  jupiter_geometry_source *p;
  p = jcntrl_shared_object_default_allocator(jupiter_geometry_source);
  return p ? &p->executive.object : NULL;
}

static void jupiter_geometry_source_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int jupiter_geometry_source_initializer(jcntrl_shared_object *obj)
{
  jcntrl_input *input;
  jcntrl_output *output;
  jupiter_geometry_source *ptr;

  ptr = jupiter_geometry_source_downcast_impl(obj);
  ptr->cdo = NULL;
  ptr->mpi = NULL;
  ptr->element = NULL;

  input = jcntrl_executive_get_input(&ptr->executive);
  output = jcntrl_executive_get_output(&ptr->executive);

  if (!jcntrl_input_set_number_of_ports(input, 0))
    return 0;
  if (!jcntrl_output_set_number_of_ports(output, 1))
    return 0;
  return 1;
}

static void jupiter_geometry_source_destructor(jcntrl_shared_object *obj) {}

jupiter_geometry_source *jupiter_geometry_source_new(void)
{
  return jcntrl_shared_object_new(jupiter_geometry_source);
}

void jupiter_geometry_source_delete(jupiter_geometry_source *source)
{
  CSVASSERT(source);
  jcntrl_executive_delete(jupiter_geometry_source_executive(source));
}

jcntrl_executive *
jupiter_geometry_source_executive(jupiter_geometry_source *source)
{
  CSVASSERT(source);
  return &source->executive;
}

jupiter_geometry_source *
jupiter_geometry_source_downcast(jcntrl_executive *executive)
{
  CSVASSERT(executive);
  return jcntrl_shared_object_downcast(jupiter_geometry_source,
                                       &executive->object);
}

static jcntrl_input *jupiter_geometry_source_create_upstream_for_shape(
  jcntrl_input *input, jcntrl_input *head, geom_data_element *element,
  int *stat)
{
  geom_shape_data *shape;
  geom_shape_element *shp_el;

  CSVASSERT(element);

  shape = geom_data_element_get_shape(element);
  if (!shape)
    return input;

  shp_el = geom_shape_data_get_element(shape);
  for (; shp_el; shp_el = geom_shape_element_next(shp_el)) {
    const geom_user_defined_data *ud;
    jupiter_geom_ext_shp_eldata *ext_data;
    controllable_geometry_entry *lh, *le;

    ud = geom_shape_element_get_extra_data(shp_el);
    ext_data = (jupiter_geom_ext_shp_eldata *)geom_user_defined_data_get(ud);
    if (!ext_data)
      continue;

    lh = &ext_data->control_entry_head;
    le = controllable_geometry_entry_next(lh);
    for (; le != lh; le = controllable_geometry_entry_next(le)) {
      int n = 0;
      switch (le->type) {
      case GEOM_VARTYPE_DOUBLE:
        n = 1;
        break;
      case GEOM_VARTYPE_VECTOR2:
        n = 2;
        break;
      case GEOM_VARTYPE_VECTOR3:
        n = 3;
        break;
      default:
        CSVUNREACHABLE();
        break;
      }
      for (int i = 0; i < n; ++i) {
        jcntrl_executive *exe;
        jcntrl_output *outp;

        exe = jcntrl_executive_manager_entry_get(le->control[i].exec);
        if (!exe)
          continue;

        if (!input) {
          jcntrl_information *info;

          input = jcntrl_input_add(head);
          if (!input) {
            if (stat)
              *stat = 1;
            return NULL;
          }

          info = jcntrl_input_information(input);
          jcntrl_information_set_datatype(info, JCNTRL_INFO_REQUIRED_DATATYPE,
                                          JCNTRL_DATATYPE_FIELD_VAR);
        }

        outp = jcntrl_executive_get_output(exe);
        outp = jcntrl_output_at(outp, 0);
        if (!outp || !jcntrl_input_connect(input, outp)) {
          if (stat)
            *stat = 1;
          return NULL;
        }

        input = jcntrl_input_next_port(input);
      }
    }
  }
  return input;
}

int jupiter_geometry_source_set_geometry(jupiter_geometry_source *source,
                                         geom_data_element *element)
{
  geom_size_type ccnt;
  geom_shape_data *shape;
  jcntrl_input *head, *input, *n;
  int r;

  CSVASSERT(source);
  CSVASSERT(element);

  r = 0;
  head = jcntrl_executive_get_input(jupiter_geometry_source_executive(source));
  input = jcntrl_input_next_port(head);
  input =
    jupiter_geometry_source_create_upstream_for_shape(input, head, element, &r);
  if (r) {
    if (source->element && source->element != element) {
      /* Reset */
      int r;
      r = jupiter_geometry_source_set_geometry(source, source->element);
      CSVASSERT(r);
    } else {
      /* If element is not changed, reversion does not make sense */
      jcntrl_input_set_number_of_ports(head, 0);
    }
    return 0;
  }

  if (input) {
    n = jcntrl_input_next_port(input);
    for (; input; input = n, n = (n ? jcntrl_input_next_port(n) : n)) {
      jcntrl_input_delete(input);
    }
  }

  source->element = element;
  return 1;
}

int jupiter_geometry_source_set_domain(jupiter_geometry_source *source,
                                       domain *cdo)
{
  CSVASSERT(source);
  CSVASSERT(cdo);

  source->cdo = cdo;
  return 1;
}

geom_data_element *
jupiter_geometry_source_get_geometry(jupiter_geometry_source *source)
{
  CSVASSERT(source);

  return source->element;
}

domain *jupiter_geometry_source_get_domain(jupiter_geometry_source *source)
{
  CSVASSERT(source);

  return source->cdo;
}

static int jupiter_geometry_source_fill_input_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_input *input)
{
  jcntrl_information *info;
  jupiter_geometry_source *source;
  source = jupiter_geometry_source_downcast_impl(obj);

  info = jcntrl_input_information(input);
  if (!jcntrl_information_set_datatype(info, JCNTRL_INFO_REQUIRED_DATATYPE,
                                       JCNTRL_DATATYPE_FIELD_VAR)) {
    return 0;
  }
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jupiter_geometry_source, jcntrl_executive,
                    fill_input_port_information)

static int jupiter_geometry_source_fill_output_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_output *output)
{
  jcntrl_information *info;
  jupiter_geometry_source *source;
  source = jupiter_geometry_source_downcast_impl(obj);

  info = jcntrl_output_information(output);
  if (!jcntrl_information_set_objecttype(info, JCNTRL_INFO_DATATYPE,
                                         jupiter_geometry_object)) {
    return 0;
  }
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jupiter_geometry_source, jcntrl_executive,
                    fill_output_port_information)

static int jupiter_geometry_source_process_update_data_impl(
  jcntrl_information *request, jcntrl_input *input, jcntrl_output *output,
  jcntrl_shared_object *obj)
{
  jupiter_geometry_source *source;
  jupiter_geometry_object *geom;
  jcntrl_shared_object *oobj;
  jcntrl_information *info;

  source = jupiter_geometry_source_downcast_impl(obj);
  output = jcntrl_output_next_port(output);
  JCNTRL_ASSERT(output);

  info = jcntrl_output_information(output);
  JCNTRL_ASSERT(info);

  oobj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
  JCNTRL_ASSERT(oobj);

  geom = jcntrl_shared_object_downcast(jupiter_geometry_object, oobj);
  JCNTRL_ASSERT(geom);

  geom->cdo = source->cdo;
  geom->mpi = source->mpi;
  jcntrl_geometry_set_data_element(&geom->data, source->element);

  if (jcntrl_geometry_has_shape(&geom->data)) {
    if (!jcntrl_geometry_update_transform(&geom->data))
      return 0;
  } else {
    if (!jcntrl_geometry_file_load(&geom->data))
      return 0;
  }

  return 1;
}

JCNTRL_VIRTUAL_WRAP(jupiter_geometry_source, jcntrl_executive,
                    process_update_data)

static void jupiter_geometry_source_init_func(jcntrl_shared_object_funcs *p)
{
  p->initializer = jupiter_geometry_source_initializer;
  p->destructor = jupiter_geometry_source_destructor;
  p->allocator = jupiter_geometry_source_allocator;
  p->deleter = jupiter_geometry_source_deleter;
  p->downcast = jupiter_geometry_source_downcast_v;
  JCNTRL_VIRTUAL_WRAP_SET(p, jupiter_geometry_source, jcntrl_executive,
                          fill_input_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jupiter_geometry_source, jcntrl_executive,
                          fill_output_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jupiter_geometry_source, jcntrl_executive,
                          process_update_data);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jupiter_geometry_source,
                                   jupiter_geometry_source_init_func)

int jupiter_geometry_source_install(void)
{
  return jcntrl_executive_install(JCNTRL_EXE_USER, jupiter_geometry_source);
}
