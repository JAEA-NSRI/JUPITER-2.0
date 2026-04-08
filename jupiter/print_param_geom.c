#include "print_param_geom.h"

#include "boundary_util.h"
#include "geometry/defs.h"
#include "geometry/infomap.h"
#include "geometry/init.h"
#include "geometry/shape.h"
#include "geometry/surface_shape.h"
#include "geometry/udata.h"
#include "geometry/variant.h"
#include "print_param_comp.h"
#include "print_param_comps.h"
#include "print_param_core.h"
#include "print_param_infomap.h"
#include "print_param_keywords.h"
#include "struct.h"

#include <stdlib.h>
#include <string.h>

struct PP_geom_data_info_map_ext_data
{
  int typeval;
  struct pp_format_value_data *v;
  union PP_geom_data_info_map_ext_set
  {
    struct PP_binary_output_mode_value_data output_mode;
    struct PP_bnd_value_data bnd_data;
    struct PP_tbnd_value_data tbnd_data;
  } data;
};

static void PP_geom_data_info_map_ext_delete(void *arg)
{
  struct PP_geom_data_info_map_ext_data *p;
  p = (struct PP_geom_data_info_map_ext_data *)arg;

  if (p->v)
    pp_format_value_clean(p->v);
  p->v = NULL;
}

static struct pp_format_value_data *
PP_geom_data_info_map_ext_format(int typeval, void *data, void *arg)
{
  struct PP_geom_data_info_map_ext_data *p;
  p = (struct PP_geom_data_info_map_ext_data *)arg;

  PP_geom_data_info_map_ext_delete(arg);

  p->typeval = typeval;
  p->v = NULL;
  switch (typeval) {
  case JUPITER_VARTYPE_OUTPUT_MODE:
    p->v = PP_binary_output_mode_value_init(&p->data.output_mode, *(int *)data,
                                            NULL, NULL);
    break;
  case JUPITER_VARTYPE_BOUNDARY:
    p->v = PP_bnd_value_init(&p->data.bnd_data, *(int *)data, NULL, NULL);
    break;
  case JUPITER_VARTYPE_TBOUNDARY:
    p->v = PP_tbnd_value_init(&p->data.tbnd_data, *(int *)data, NULL, NULL);
    break;
  }
  return p->v;
}

static void
PP_geom_data_info_map_ext_init(struct PP_geom_data_info_map_ext_data *d)
{
  memset(d, 0, sizeof(struct PP_geom_data_info_map_ext_data));
  d->v = NULL;
}

//---- init_func

struct PP_geom_info_map_init_func_ok_data
{
  geom_init_element *el;
  geom_variant *einfo;
};

static int PP_geom_info_map_geom_init_func_ok(geom_size_type index,
                                              const geom_variant *value,
                                              void *data)
{
  geom_error err;
  struct PP_geom_info_map_init_func_ok_data *d;

  d = (struct PP_geom_info_map_init_func_ok_data *)data;
  geom_variant_nullify(d->einfo);
  err = geom_init_element_set_parameter(d->el, index, value, d->einfo);
  if (err != GEOM_SUCCESS || !geom_variant_is_null(d->einfo))
    return 0;
  return 1;
}

void PPgeom_init_func_info(flags *flg, int indent, const char *base_unit,
                           const char *length_unit, geom_init_element *init_el,
                           int *nogood)
{
  struct PP_geom_data_info_map_ext_data extd;
  struct PP_geom_info_map_init_func_ok_data okd;
  const geom_user_defined_data *ud;
  controllable_geometry_entry *control_it;
  jupiter_geom_ext_init_eldata *ext_data;
  geom_info_map *map;

  map = geom_init_element_func_info(init_el);
  if (!map)
    return;

  PP_geom_data_info_map_ext_init(&extd);

  okd.el = init_el;
  okd.einfo = geom_variant_new(NULL);

  ud = geom_init_element_get_extra_data(init_el);
  ext_data = (jupiter_geom_ext_init_eldata *)geom_user_defined_data_get(ud);

  control_it = NULL;
  if (ext_data)
    control_it = &ext_data->control_entry_head;

  PPgeom_info_map(flg, indent, base_unit, length_unit, map, control_it, nogood,
                  PP_geom_data_info_map_ext_format,
                  PP_geom_data_info_map_ext_delete, &extd,
                  PP_geom_info_map_geom_init_func_ok, &okd);
  geom_info_map_delete_all(map);
  if (okd.einfo)
    geom_variant_delete(okd.einfo);
}

//---- shape

struct PP_geom_info_map_shape_ok_data
{
  geom_shape_element *el;
  geom_variant *einfo;
};

static int PP_geom_info_map_geom_shape_ok(geom_size_type index,
                                          const geom_variant *value, void *data)
{
  geom_error err;
  struct PP_geom_info_map_shape_ok_data *d;

  d = (struct PP_geom_info_map_shape_ok_data *)data;
  geom_variant_nullify(d->einfo);
  err = geom_shape_element_set_parameter(d->el, index, value, d->einfo);
  if (err != GEOM_SUCCESS || !geom_variant_is_null(d->einfo))
    return 0;
  return 1;
}

void PPgeom_shape_info(flags *flg, int indent, const char *base_unit,
                       const char *length_unit, geom_shape_element *shape_el,
                       int *nogood)
{
  struct PP_geom_data_info_map_ext_data extd;
  struct PP_geom_info_map_shape_ok_data okd;
  const geom_user_defined_data *ud;
  controllable_geometry_entry *control_it;
  jupiter_geom_ext_shp_eldata *ext_data;
  geom_info_map *map;

  map = geom_shape_element_shape_info(shape_el);
  if (!map)
    return;

  PP_geom_data_info_map_ext_init(&extd);

  okd.el = shape_el;
  okd.einfo = geom_variant_new(NULL);

  ud = geom_shape_element_get_extra_data(shape_el);
  ext_data = (jupiter_geom_ext_shp_eldata *)geom_user_defined_data_get(ud);

  control_it = NULL;
  if (ext_data)
    control_it = &ext_data->control_entry_head;

  PPgeom_info_map(flg, indent, base_unit, length_unit, map, control_it, nogood,
                  PP_geom_data_info_map_ext_format,
                  PP_geom_data_info_map_ext_delete, &extd,
                  PP_geom_info_map_geom_shape_ok, &okd);
  geom_info_map_delete_all(map);
  if (okd.einfo)
    geom_variant_delete(okd.einfo);
}

//---- surface shape

struct PP_geom_info_map_sshape_ok_data
{
  geom_surface_shape_element *el;
  geom_variant *einfo;
};

static int PP_geom_info_map_geom_sshape_ok(geom_size_type index,
                                           const geom_variant *value,
                                           void *data)
{
  geom_error err;
  struct PP_geom_info_map_sshape_ok_data *d;

  d = (struct PP_geom_info_map_sshape_ok_data *)data;
  geom_variant_nullify(d->einfo);
  err = geom_surface_shape_element_set_parameter(d->el, index, value, d->einfo);
  if (err != GEOM_SUCCESS || !geom_variant_is_null(d->einfo))
    return 0;
  return 1;
}

void PPgeom_surface_shape_info(flags *flg, int indent, const char *base_unit,
                               const char *length_unit,
                               geom_surface_shape_element *surface_shape_el,
                               int *nogood)
{
  struct PP_geom_data_info_map_ext_data extd;
  struct PP_geom_info_map_sshape_ok_data okd;
  const geom_user_defined_data *ud;
  controllable_geometry_entry *control_it;
  jupiter_geom_ext_sshp_eldata *ext_data;
  geom_info_map *map;

  map = geom_surface_shape_element_shape_info(surface_shape_el);
  if (!map)
    return;

  PP_geom_data_info_map_ext_init(&extd);

  okd.el = surface_shape_el;
  okd.einfo = geom_variant_new(NULL);

  ud = geom_surface_shape_element_get_extra_data(surface_shape_el);
  ext_data = (jupiter_geom_ext_sshp_eldata *)geom_user_defined_data_get(ud);

  control_it = NULL;
  if (ext_data)
    control_it = &ext_data->control_entry_head;

  PPgeom_info_map(flg, indent, base_unit, length_unit, map, control_it, nogood,
                  PP_geom_data_info_map_ext_format,
                  PP_geom_data_info_map_ext_delete, &extd,
                  PP_geom_info_map_geom_sshape_ok, &okd);
  geom_info_map_delete_all(map);
  if (okd.einfo)
    geom_variant_delete(okd.einfo);
}

//--- Combined data types

void PPgeom_init_vof_data(flags *flg, int indent, struct init_vof_data *data,
                          int *nogood)
{
  int good_phase = 0;
  PP_component_info_data(flg, indent, "Material ID", &data->comp, "", 1,
                         nogood);

  if (data->comp.d) {
    if (data->comp.d->jupiter_id != -1) {
      component_phases ph = data->comp.d->phases;
      switch (data->phase) {
      case GEOM_PHASE_GAS:
        good_phase = component_phases_has_gas(ph);
        break;
      case GEOM_PHASE_SOLID:
        good_phase = component_phases_has_solid(ph);
        break;
      case GEOM_PHASE_LIQUID:
        good_phase = component_phases_has_liquid(ph);
        break;
      case GEOM_PHASE_INVALID:
        good_phase = 0;
        break;
      }
    } else {
      good_phase = 1;
    }
  } else {
    good_phase = 0;
  }
  PP_vphase(flg, indent, "Phase to set", data->phase, "", good_phase, nogood);
}

void PPgeom_init_lpt_pewall_data(flags *flg, int indent,
                                 struct init_lpt_pewall_data *data, int *nogood)
{
  if (data->is_boundary) {
    PP_boundary_dir(flg, indent, "Direction", data->dir, "", 1, nogood);
  }
}

void PPgeom_boundary_data(flags *flg, int indent,
                          struct boundary_init_data *data, char sub_header,
                          int *nogood)
{
  fluid_boundary_data *fdata;
  fdata = data->data;

  PP_boundary_dir(flg, indent, "Direction", data->dir, "", 1, nogood);
  PP_bnd(flg, indent, "Condition", fdata->cond, "", 1, nogood);
  PP_double(flg, indent, "Geometry Threshold", data->threshold, "", 1, nogood);
  if (fdata->cond == INLET) {
    int ncomp, raw_ncomp;
    PP_control(flg, indent, "Trip control method", fdata->control, "", 1,
               nogood);
    PP_controllable_type(flg, indent, "Inlet velocity U", &fdata->inlet_vel_u,
                         "m/s", 1, nogood);
    PP_controllable_type(flg, indent, "Inlet velocity V", &fdata->inlet_vel_v,
                         "m/s", 1, nogood);
    PP_controllable_type(flg, indent, "Inlet velocity W", &fdata->inlet_vel_w,
                         "m/s", 1, nogood);

    ncomp = 0;
    raw_ncomp = 0;
    if (fdata->comps) {
      ncomp = inlet_component_data_ncomp(fdata->comps);
      raw_ncomp = fdata->comps->ncomp;
    }
    PP_int(flg, indent, "Number of parts of inlet mixture", raw_ncomp, "",
           ncomp > 0, nogood);
    for (int i = 0; i < ncomp; ++i) {
      struct inlet_component_element *e;

      e = inlet_component_data_get(fdata->comps, i);
      print_param_header(flg, sub_header, indent + 2, 3, 3,
                         "Part %d of inlet mixture", i + 1);
      PP_component_info_data(flg, indent + 2, "Material ID", &e->comp, "", 1,
                             nogood);
      PP_controllable_type(flg, indent + 2, "Ratio of inlet", &e->ratio, "", 1,
                           nogood);
    }

  } else if (fdata->cond == OUT) {
    PP_out_p_cond(flg, indent, "Pressure Condition", fdata->out_p_cond, "", 1,
                  nogood);

    if (fdata->out_p_cond == OUT_P_COND_CONST) {
      PP_controllable_type(flg, indent, "Pressure value", &fdata->const_p, "Pa",
                           1, nogood);
    }
  }
}

void PPgeom_tboundary_data(flags *flg, int indent,
                           struct tboundary_init_data *data, char sub_header,
                           int *nogood)
{
  thermal_boundary_data *tdata;
  tdata = data->data;

  PP_boundary_dir(flg, indent, "Direction", data->dir, "", 1, nogood);
  PP_tbnd(flg, indent, "Condition", tdata->cond, "", 1, nogood);
  PP_double(flg, indent, "Geometry Threshold", data->threshold, "", 1, nogood);
  if (tdata->cond == ISOTHERMAL) {
    PP_control(flg, indent, "Trip control method", tdata->control, "", 1,
               nogood);
    PP_controllable_type(flg, indent, "Temperature", &tdata->temperature, "K",
                         1, nogood);
  }
  if (tdata->cond == DIFFUSION) {
    PP_double(flg, indent, "Lower Limit of Temperature", tdata->diffusion_limit,
              "K", 1, nogood);
  }
}

void PPgeom_surface_boundary_data(flags *flg, int indent,
                                  struct surface_boundary_init_data *data,
                                  char sub_header, int *nogood)
{
  surface_boundary_data *sdata;
  sdata = data->data;

  PP_bnd(flg, indent, "Condition", sdata->cond, "", 1, nogood);
  PP_double(flg, indent, "Geometry threshold", data->threshold, "", 1, nogood);
  if (sdata->cond == INLET) {
    int ncomp;
    int raw_ncomp;
    PP_surface_inlet_dir(flg, indent, "Inlet direction", sdata->inlet_dir, "",
                         1, nogood);
    PP_control(flg, indent, "Trip control method", sdata->control, "", 1,
               nogood);

    if (sdata->inlet_dir == SURFACE_INLET_DIR_NORMAL) {
      PP_controllable_type(flg, indent, "Normal inlet velocity",
                           &sdata->normal_inlet_vel, "m/s", 1, nogood);
    }

    ncomp = 0;
    raw_ncomp = 0;
    if (sdata->comps) {
      ncomp = inlet_component_data_ncomp(sdata->comps);
      raw_ncomp = sdata->comps->ncomp;
    }

    PP_int(flg, indent, "Number of parts of inlet mixture", raw_ncomp, "",
           ncomp > 0, nogood);
    for (int i = 0; i < ncomp; ++i) {
      struct inlet_component_element *e;

      e = inlet_component_data_get(sdata->comps, i);
      print_param_header(flg, sub_header, indent + 2, 3, 3,
                         "Part %d of inlet mixture", i + 1);
      PP_component_info_data(flg, indent + 2, "Material ID", &e->comp, "", 1,
                             nogood);
      PP_controllable_type(flg, indent + 2, "Ratio of inlet", &e->ratio, "", 1,
                           nogood);
    }
  }
}
