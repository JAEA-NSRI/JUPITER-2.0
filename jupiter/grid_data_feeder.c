#include "grid_data_feeder.h"
#include "control/cell_data.h"
#include "control/data_array.h"
#include "control/defs.h"
#include "control/error.h"
#include "control/grid_data.h"
#include "control/executive.h"
#include "control/information.h"
#include "control/input.h"
#include "control/output.h"
#include "control/shared_object_priv.h"
#include "control/struct_grid.h"
#include "csvutil.h"
#include "grid_data_feeder_data.h"
#include "control/shared_object.h"
#include "real_array.h"
#include "strlist.h"
#include "struct.h"

#include <stdarg.h>
#include <string.h>

static jupiter_grid_data_feeder *
jupiter_grid_data_feeder_downcast_impl(jcntrl_shared_object *obj)
{
  return JCNTRL_DOWNCAST_IMPL(jupiter_grid_data_feeder, obj);
}

jupiter_grid_data_feeder *jupiter_grid_data_feeder_new(void)
{
  return jcntrl_shared_object_new(jupiter_grid_data_feeder);
}

void jupiter_grid_data_feeder_delete(jupiter_grid_data_feeder *feeder)
{
  jcntrl_executive_delete(&feeder->executive);
}

parameter *jupiter_grid_data_feeder_prm(jupiter_grid_data_feeder *feeder)
{
  CSVASSERT(feeder);
  return feeder->prm;
}

void jupiter_grid_data_feeder_set_prm(jupiter_grid_data_feeder *feeder,
                                      parameter *prm)
{
  CSVASSERT(feeder);
  feeder->prm = prm;
}

variable *jupiter_grid_data_feeder_val(jupiter_grid_data_feeder *feeder)
{
  CSVASSERT(feeder);
  return feeder->val;
}

void jupiter_grid_data_feeder_set_val(jupiter_grid_data_feeder *feeder,
                                      variable *val)
{
  CSVASSERT(feeder);
  feeder->val = val;
}

material *jupiter_grid_data_feeder_mtl(jupiter_grid_data_feeder *feeder)
{
  CSVASSERT(feeder);
  return feeder->mtl;
}

void jupiter_grid_data_feeder_set_mtl(jupiter_grid_data_feeder *feeder,
                                      material *mtl)
{
  CSVASSERT(feeder);
  feeder->mtl = mtl;
}

static void *jupiter_grid_data_feeder_downcast_v(jcntrl_shared_object *obj)
{
  return jupiter_grid_data_feeder_downcast_impl(obj);
}

jcntrl_executive *
jupiter_grid_data_feeder_executive(jupiter_grid_data_feeder *feeder)
{
  return &feeder->executive;
}

jupiter_grid_data_feeder *
jupiter_grid_data_feeder_downcast(jcntrl_shared_object *object)
{
  return jcntrl_shared_object_downcast(jupiter_grid_data_feeder, object);
}

static jcntrl_shared_object *jupiter_grid_data_feeder_allocator(void)
{
  jupiter_grid_data_feeder *p;
  p = jcntrl_shared_object_default_allocator(jupiter_grid_data_feeder);
  return p ? &p->executive.object : NULL;
}

static void jupiter_grid_data_feeder_deleter(jcntrl_shared_object *obj)
{
  jcntrl_shared_object_default_deleter(obj);
}

static int jupiter_grid_data_feeder_initializer(jcntrl_shared_object *obj)
{
  jcntrl_input *input;
  jcntrl_output *output;
  jupiter_grid_data_feeder *p;
  p = jupiter_grid_data_feeder_downcast_impl(obj);
  p->prm = NULL;
  p->mtl = NULL;
  p->val = NULL;

  input = jcntrl_executive_get_input(&p->executive);
  output = jcntrl_executive_get_output(&p->executive);

  if (!jcntrl_input_set_number_of_ports(input, 0))
    return 0;
  if (!jcntrl_output_set_number_of_ports(output, 1))
    return 0;

  if (!jcntrl_executive_set_name(&p->executive, CONTROL_KEYCHAR_GRID "all"))
    return 0;

  return 1;
}

static void jupiter_grid_data_feeder_destructor(jcntrl_shared_object *obj)
{
  /* nop */
}

static int jupiter_grid_data_feeder_fill_input_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_input *input)
{
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jupiter_grid_data_feeder, jcntrl_executive,
                    fill_input_port_information)

static int jupiter_grid_data_feeder_fill_output_port_information_impl(
  jcntrl_shared_object *obj, int index, jcntrl_output *output)
{
  jcntrl_information *info;
  info = jcntrl_output_information(output);
  if (!jcntrl_information_set_objecttype(info, JCNTRL_INFO_DATATYPE,
                                         jcntrl_grid_data))
    return 0;
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jupiter_grid_data_feeder, jcntrl_executive,
                    fill_output_port_information)

static int jupiter_grid_data_feeder_process_update_information_impl(
  jcntrl_information *request, jcntrl_input *input, jcntrl_output *output,
  jcntrl_shared_object *obj)
{
  jcntrl_information *info;
  jupiter_grid_data_feeder *f;

  f = jupiter_grid_data_feeder_downcast_impl(obj);

  output = jcntrl_output_next_port(output);
  if (!output)
    return 0;

  info = jcntrl_output_information(output);
  if (!info)
    return 0;

  if (f->prm && f->prm->cdo) {
    domain *cdo = f->prm->cdo;

    if (!jcntrl_information_set_float(info, JCNTRL_INFO_UPDATE_TIME, cdo->time))
      return 0;
  }
  return 1;
}

JCNTRL_VIRTUAL_WRAP(jupiter_grid_data_feeder, jcntrl_executive,
                    process_update_information)

static void jupiter_grid_data_feeder_get_local_extent(int extent[6],
                                                      mpi_param *mpi,
                                                      domain *cdo)
{
  extent[0] = mpi->rank_x * cdo->nx;
  extent[1] = extent[0] + cdo->nx;
  extent[2] = mpi->rank_y * cdo->ny;
  extent[3] = extent[2] + cdo->ny;
  extent[4] = mpi->rank_z * cdo->nz;
  extent[5] = extent[4] + cdo->nz;
}

static void jupiter_grid_data_feeder_get_whole_extent(int extent[6],
                                                      domain *cdo)
{
  extent[0] = 0;
  extent[1] = cdo->gnx;
  extent[2] = 0;
  extent[3] = cdo->gny;
  extent[4] = 0;
  extent[5] = cdo->gnz;
}

static void jupiter_grid_data_feeder_get_data_extent(int extent[6],
                                                     mpi_param *mpi,
                                                     domain *cdo)
{
  jupiter_grid_data_feeder_get_local_extent(extent, mpi, cdo);
  extent[0] -= cdo->stm;
  extent[1] += cdo->stp;
  extent[2] -= cdo->stm;
  extent[3] += cdo->stp;
  extent[4] -= cdo->stm;
  extent[5] += cdo->stp;
}

static int jupiter_grid_data_feeder_process_update_extent_impl(
  jcntrl_information *request, jcntrl_input *input, jcntrl_output *output,
  jcntrl_shared_object *obj)
{
  jcntrl_information *info;
  jupiter_grid_data_feeder *f;

  f = jupiter_grid_data_feeder_downcast_impl(obj);

  output = jcntrl_output_next_port(output);
  if (!output)
    return 0;

  info = jcntrl_output_information(output);
  if (!info)
    return 0;

  if (f->prm && f->prm->mpi && f->prm->cdo) {
    mpi_param *mpi = f->prm->mpi;
    domain *cdo = f->prm->cdo;
    int lextent[6], dextent[6], wextent[6];

    jupiter_grid_data_feeder_get_data_extent(dextent, mpi, cdo);
    jupiter_grid_data_feeder_get_local_extent(lextent, mpi, cdo);
    jupiter_grid_data_feeder_get_whole_extent(wextent, cdo);

    if (!jcntrl_information_set_extent(info, JCNTRL_INFO_WHOLE_EXTENT,
                                       wextent))
      return 0;

    if (!jcntrl_information_set_extent(info, JCNTRL_INFO_PIECE_EXTENT,
                                       lextent))
      return 0;

    if (!jcntrl_information_set_extent(info, JCNTRL_INFO_DATA_EXTENT,
                                       dextent))
      return 0;
  }

  return 1;
}

JCNTRL_VIRTUAL_WRAP(jupiter_grid_data_feeder, jcntrl_executive,
                    process_update_extent)

static jupiter_real_array *
jupiter_grid_data_feeder_wrap_array(jcntrl_data_array **orig, const type *src,
                                    jcntrl_size_type len, const char *name,
                                    jcntrl_size_type namelen)
{
  jcntrl_data_array *dary;
  jupiter_real_array *ary, *aary;

  aary = NULL;
  ary = NULL;
  if (orig && *orig)
    ary = jupiter_real_array_downcast(jcntrl_data_array_object(*orig));

  if (!ary) {
    ary = aary = jupiter_real_array_new();
    if (!ary)
      return NULL;
  }

  if (!jupiter_real_array_bind(ary, src, len)) {
    if (aary)
      jupiter_real_array_delete(aary);
    return NULL;
  }

  dary = jupiter_real_array_data_array(ary);
  if (!jcntrl_data_array_set_name(dary, name, namelen)) {
    if (aary)
      jupiter_real_array_delete(aary);
    return NULL;
  }

  if (orig)
    *orig = dary;
  return ary;
}

static jupiter_real_array *
jupiter_grid_data_feeder_wrap_arrayl(jcntrl_data_array **orig, const type *src,
                                     jcntrl_size_type len,
                                     jupiter_strlist *name)
{
  return jupiter_grid_data_feeder_wrap_array(orig, src, len, name->buf,
                                             strlen(name->buf));
}

static jupiter_real_array *
jupiter_grid_data_feeder_wrap_arrayv(jcntrl_data_array **orig, const type *src,
                                     jcntrl_size_type len, const char *fmt,
                                     va_list args)
{
  jupiter_real_array *p;
  jupiter_strlist *lp;

  lp = jupiter_strlist_vasprintf(fmt, args);
  if (!lp) {
    jcntrl_raise_allocation_failed(__FILE__, __LINE__);
    return NULL;
  }

  p = jupiter_grid_data_feeder_wrap_arrayl(orig, src, len, lp);
  jupiter_strlist_delete(lp);
  return p;
}

static jupiter_real_array *
#ifdef __GNUC__
  __attribute__((format(printf, 4, 5)))
#endif
  jupiter_grid_data_feeder_wrap_arrayf(jcntrl_data_array **orig,
                                       const type *src, jcntrl_size_type len,
                                       const char *fmt, ...)
{
  jupiter_real_array *p;
  va_list ap;

  va_start(ap, fmt);
  p = jupiter_grid_data_feeder_wrap_arrayv(orig, src, len, fmt, ap);
  va_end(ap);
  return p;
}

static jupiter_real_array *
jupiter_grid_data_feeder_wrap_cdata(jcntrl_cell_data *cdata, const type *src,
                                    jcntrl_size_type len, const char *name,
                                    jcntrl_size_type namelen)
{
  jupiter_real_array *j;
  jcntrl_data_array *d, *a;
  d = jcntrl_cell_data_get_array_by_name(cdata, name, namelen);
  a = d;

  j = jupiter_grid_data_feeder_wrap_array(&a, src, len, name, namelen);
  if (!j)
    return NULL;

  if (a == d)
    return j;

  if (!jcntrl_cell_data_add_array(cdata, a)) {
    jcntrl_data_array_delete(a);
    return NULL;
  }

  // Release ownership of j
  jcntrl_data_array_delete(a);
  return j;
}

static jupiter_real_array *
jupiter_grid_data_feeder_wrap_cdatal(jcntrl_cell_data *cdata, const type *src,
                                     jcntrl_size_type len,
                                     jupiter_strlist *name)
{
  return jupiter_grid_data_feeder_wrap_cdata(cdata, src, len, name->buf,
                                             strlen(name->buf));
}

static jupiter_real_array *
jupiter_grid_data_feeder_wrap_cdatav(jcntrl_cell_data *cdata, const type *src,
                                     jcntrl_size_type len, const char *fmt,
                                     va_list ap)
{
  jupiter_real_array *p;
  jupiter_strlist *lp;

  lp = jupiter_strlist_vasprintf(fmt, ap);
  if (!lp) {
    jcntrl_raise_allocation_failed(__FILE__, __LINE__);
    return NULL;
  }

  p = jupiter_grid_data_feeder_wrap_cdatal(cdata, src, len, lp);
  jupiter_strlist_free(lp);
  return p;
}

static jupiter_real_array *
#ifdef __GNUC__
  __attribute__((format(printf, 4, 5)))
#endif
  jupiter_grid_data_feeder_wrap_cdataf(jcntrl_cell_data *cdata, const type *src,
                                       jcntrl_size_type len, const char *fmt,
                                       ...)
{
  jupiter_real_array *p;
  va_list ap;
  va_start(ap, fmt);
  p = jupiter_grid_data_feeder_wrap_cdatav(cdata, src, len, fmt, ap);
  va_end(ap);
  return p;
}

#define wrap_array(r, f, ...)                            \
  do {                                                   \
    if (!jupiter_grid_data_feeder_wrap_##f(__VA_ARGS__)) \
      r = 0;                                             \
  } while (0)

#define wrap_var_1(r, c, s, l, vn)                                      \
  wrap_array(r, cdataf, c, s, l, "%s%s", CONTROL_KEYCHAR_VARNAME, vn)

#define wrap_var_n(r, c, s, l, vn, n)                                   \
  wrap_array(r, cdataf, c, s, l, "%s%s%d", CONTROL_KEYCHAR_VARNAME, vn, n)

static int jupiter_grid_data_feeder_process_update_data_impl(
  jcntrl_information *request, jcntrl_input *input, jcntrl_output *output,
  jcntrl_shared_object *obj)
{
  jcntrl_shared_object *outobj;
  jcntrl_grid_data *g;
  jcntrl_cell_data *c;
  jcntrl_information *info;
  jupiter_grid_data_feeder *f;
  mpi_param *mpi;
  flags *flg;
  domain *cdo;
  variable *val;
  material *mtl;
  jcntrl_struct_grid grid;
  int r;

  f = jupiter_grid_data_feeder_downcast_impl(obj);

  output = jcntrl_output_next_port(output);
  if (!output)
    return 0;

  info = jcntrl_output_information(output);
  if (!info)
    return 0;

  outobj = jcntrl_information_get_object(info, JCNTRL_INFO_DATA_OBJECT);
  if (!outobj)
    return 0;

  g = jcntrl_grid_data_downcast(outobj);
  if (!g)
    return 0;

  c = jcntrl_grid_data_cell_data(g);
  if (!c)
    return 0;

  jcntrl_struct_grid_init(&grid);

  do {
    r = 1;

    if (!f->prm)
      r = 0;

    if (r && (!f->prm->flg || !f->prm->cdo || !f->prm->mpi))
      r = 0;

    if (!f->mtl || !f->val)
      r = 0;

    if (!r)
      break;

    flg = f->prm->flg;
    mpi = f->prm->mpi;
    cdo = f->prm->cdo;
    val = f->val;
    mtl = f->mtl;

    jcntrl_struct_grid_copy(&grid, jcntrl_grid_data_struct_grid(g));
    jupiter_grid_data_feeder_get_data_extent(grid.extent, mpi, cdo);

    r = 1;
    wrap_array(r, array, &grid.x_coords, cdo->x, cdo->mx + 1, "x", 1);
    wrap_array(r, array, &grid.y_coords, cdo->y, cdo->my + 1, "y", 1);
    wrap_array(r, array, &grid.z_coords, cdo->z, cdo->mz + 1, "z", 1);

    jcntrl_grid_data_set_struct_grid(g, &grid);
    jcntrl_struct_grid_clear(&grid);
    if (!r)
      break;

    {
      int ncompo;

      ncompo = (flg->solute_diff == ON) ? 1 : cdo->NBaseComponent;
      for (int ic = 0; ic < ncompo; ++ic) {
        wrap_var_n(r, c, val->fs + ic * cdo->m, cdo->m, "solid-vof-", ic);
        wrap_var_n(r, c, val->fl + ic * cdo->m, cdo->m, "liquid-vof-", ic);
      }

      ncompo = (flg->solute_diff == ON) ? cdo->NumberOfComponent : 0;
      for (int ic = 0; ic < ncompo; ++ic) {
        wrap_var_n(r, c, val->Y + ic * cdo->m, cdo->m, "solute-Y-", ic);
      }

      wrap_var_1(r, c, val->p, cdo->m, "pressure");
      wrap_var_1(r, c, val->u, cdo->m, "velocity-x");
      wrap_var_1(r, c, val->v, cdo->m, "velocity-y");
      wrap_var_1(r, c, val->w, cdo->m, "velocity-z");
      wrap_var_1(r, c, val->t, cdo->m, "temperature");
    }
  } while (0);

  if (!r) {
    return 0;
  }

  return 1;
}

JCNTRL_VIRTUAL_WRAP(jupiter_grid_data_feeder, jcntrl_executive,
                    process_update_data)

static void jupiter_grid_data_feeder_init_func(jcntrl_shared_object_funcs *p)
{
  p->downcast = jupiter_grid_data_feeder_downcast_v;
  p->initializer = jupiter_grid_data_feeder_initializer;
  p->destructor = jupiter_grid_data_feeder_destructor;
  p->allocator = jupiter_grid_data_feeder_allocator;
  p->deleter = jupiter_grid_data_feeder_deleter;
  JCNTRL_VIRTUAL_WRAP_SET(p, jupiter_grid_data_feeder, jcntrl_executive,
                          fill_input_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jupiter_grid_data_feeder, jcntrl_executive,
                          fill_output_port_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jupiter_grid_data_feeder, jcntrl_executive,
                          process_update_information);
  JCNTRL_VIRTUAL_WRAP_SET(p, jupiter_grid_data_feeder, jcntrl_executive,
                          process_update_extent);
  JCNTRL_VIRTUAL_WRAP_SET(p, jupiter_grid_data_feeder, jcntrl_executive,
                          process_update_data);
}

JCNTRL_SHARED_METADATA_INIT_DEFINE(jupiter_grid_data_feeder,
                                   jupiter_grid_data_feeder_init_func)
