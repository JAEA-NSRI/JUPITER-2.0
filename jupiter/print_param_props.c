#include "print_param_props.h"
#include "component_data.h"
#include "component_data_defs.h"
#include "component_info_defs.h"
#include "csvutil.h"
#include "geometry/defs.h"
#include "geometry/infomap.h"
#include "print_param_basic.h"
#include "print_param_comp.h"
#include "print_param_comps.h"
#include "print_param_core.h"
#include "print_param_infomap.h"
#include "print_param_keywords.h"
#include "table/table.h"
#include "tempdep_properties.h"
#include "tmcalc.h"

#include <inttypes.h>
#include <string.h>
#include <stdlib.h>

struct PP_tempdep_property_info_map_ext_data
{
  int typeval;
  struct pp_format_value_data *v;
  union PP_tempdep_property_info_map_ext_set
  {
    struct PP_tempdep_property_type_value_data tempdep_type;
  } data;
};

static void PP_tempdep_property_info_map_ext_delete(void *arg)
{
  struct PP_tempdep_property_info_map_ext_data *p;
  p = (struct PP_tempdep_property_info_map_ext_data *)arg;

  if (p->v)
    pp_format_value_clean(p->v);
  p->v = NULL;
}

static struct pp_format_value_data *
PP_tempdep_property_info_map_ext_format(int typeval, void *data, void *arg)
{
  struct PP_tempdep_property_info_map_ext_data *p;
  p = (struct PP_tempdep_property_info_map_ext_data *)arg;

  PP_tempdep_property_info_map_ext_delete(arg);

  p->typeval = typeval;
  p->v = NULL;
  switch (typeval) {
  case TEMPDEP_VARTYPE_ENUM_TYPE:
    p->v = PP_tempdep_property_type_value_init(&p->data.tempdep_type,
                                               *(int *)data, NULL, NULL);
  }
  return p->v;
}

static void PP_tempdep_property_info_map_ext_init(
  struct PP_tempdep_property_info_map_ext_data *d)
{
  memset(d, 0, sizeof(struct PP_tempdep_property_info_map_ext_data));
  d->v = NULL;
}

void PPtempdep_property(flags *flg, int indent, const char *title,
                        char head_char, const char *base_unit,
                        const char *temperature_unit, tempdep_property *prop,
                        int *nogood)
{
  struct PP_tempdep_property_info_map_ext_data extd;
  geom_error err;
  geom_info_map *map;

  if (title)
    print_param_header(flg, head_char, indent, 3, 3, "%s", title);

  PP_tempdep_property_type(flg, indent, "Temperature dependency type",
                           prop->type, "", 1, nogood);

  err = GEOM_SUCCESS;
  map = tempdep_property_create_info_map(prop, &err);
  if (!map)
    return;

  PP_tempdep_property_info_map_ext_init(&extd);

  PPgeom_info_map(flg, indent, base_unit, temperature_unit, map, NULL, nogood,
                  PP_tempdep_property_info_map_ext_format,
                  PP_tempdep_property_info_map_ext_delete, &extd, NULL, NULL);

  geom_info_map_delete_all(map);
}

void PPphase_value_component(flags *flg, int indent, int tm_sol_from_table,
                             int tm_liq_from_table, phase_value_component *f,
                             int *nogood)
{
  PP_table_or_double(flg, indent, "Solidus Temperature", tm_sol_from_table,
                     "(From Table)", f->tm_soli, "", "K", f->tm_soli > 0.0,
                     nogood);
  PP_table_or_double(flg, indent, "Liquidus Temperature", tm_liq_from_table,
                     "(From Table)", f->tm_liq, "", "K", f->tm_liq > 0.0,
                     nogood);

  PP_double(flg, indent, "Vaporization Temperature", f->tb, "K", f->tb >= 0.0,
            nogood);
  PP_double(flg, indent, "Latent Heat (Solid--Liquid)", f->lh, "J/kg",
            f->lh > 0.0, nogood);
  PP_double(flg, indent, "Latent Heat (Liquid--Gas)", f->lhv, "J/kg",
            f->lhv >= 0.0, nogood);

  PP_double(flg, indent, "Molar Mass", f->molar_mass, "g/mol",
            f->molar_mass > 0.0, nogood);

  if (flg->IBM == ON || flg->porous == ON) {
    PP_solid_form(flg, indent, "Solid Form", f->sform, "",
                  !((flg->IBM != ON && f->sform == SOLID_FORM_IBM) ||
                    (flg->porous != ON && f->sform == SOLID_FORM_POROUS)),
                  nogood);
  }

  if (flg->porous == ON && f->sform == SOLID_FORM_POROUS) {
    PP_double(flg, indent, "Porosity", f->poros, "", f->poros > 0.0, nogood);
    PP_double(flg, indent, "Permeability", f->permea, "m^2", f->permea > 0.0,
              nogood);
  }

  PPtempdep_property(flg, indent, "Volumetric Expansion Coefficient", '.', "/K",
                     "K", &f->beta, nogood);

  PPtempdep_property(flg, indent, "Surface Tension Coefficient", '.', "N/m",
                     "K", &f->sigma, nogood);

  PPtempdep_property(flg, indent, "Density, Solid", '.', "kg/m3", "K",
                     &f->rho_s, nogood);
  PPtempdep_property(flg, indent, "Density, Liquid", '.', "kg/m3", "K",
                     &f->rho_l, nogood);

  PPtempdep_property(flg, indent, "Viscousity, Solid", '.', "Pa.s", "K",
                     &f->mu_s, nogood);
  PPtempdep_property(flg, indent, "Viscousity, Liquid", '.', "Pa.s", "K",
                     &f->mu_l, nogood);

  PPtempdep_property(flg, indent, "Specific Heat, Solid", '.', "J/kg.K", "K",
                     &f->specht_s, nogood);
  PPtempdep_property(flg, indent, "Specific Heat, Liquid", '.', "J/kg.K", "K",
                     &f->specht_l, nogood);

  PPtempdep_property(flg, indent, "Thermal Conductivity, Solid", '.', "W/m.K",
                     "K", &f->thc_s, nogood);
  PPtempdep_property(flg, indent, "Thermal Conductivity, Liquid", '.', "W/m.K",
                     "K", &f->thc_l, nogood);

  if (flg->radiation == ON) {
    PPtempdep_property(flg, indent, "Emissivity, Solid", '.', "", "K",
                       &f->emi_s, nogood);
    PPtempdep_property(flg, indent, "Emissivity, Liquid", '.', "", "K",
                       &f->emi_l, nogood);
    PPtempdep_property(flg, indent, "Emissivity, Gas", '.', "", "K", &f->emi_g,
                       nogood);
  }
}

void PPphase_value_component_g(flags *flg, int indent, phase_value_component *f,
                               int *nogood)
{
  PP_double(flg, indent, "Latent Heat (Liquid--Gas)", f->lhv, "J/kg",
            f->lhv > 0.0, nogood);

  PP_double(flg, indent, "Molar Mass", f->molar_mass, "g/mol",
            f->molar_mass > 0.0, nogood);

  PPtempdep_property(flg, indent, "Density", '.', "kg/m3", "K", &f->rho_g,
                     nogood);

  PPtempdep_property(flg, indent, "Viscousity", '.', "Pa.s", "K", &f->mu_g,
                     nogood);

  PPtempdep_property(flg, indent, "Specific Heat", '.', "J/kg.K", "K",
                     &f->specht_g, nogood);

  PPtempdep_property(flg, indent, "Thermal Conductivity", '.', "W/m.K", "K",
                     &f->thc_g, nogood);

  PPtempdep_property(flg, indent, "Saturation Pressure", '.', "Pa", "K",
                     &f->saturated_pressure, nogood);

  PPtempdep_property(flg, indent, "Internal Energy (for energy source)", '.',
                     "J/kg", "K", &f->condensation_E, nogood);

  PPtempdep_property(flg, indent, "Surface tension coeffient (as liquid)", '.',
                     "N/m", "K", &f->sigma, nogood);
}

void PPbinary_diffusivity(flags *flg, int indent, const char *title,
                          char head_char, int len,
                          struct dc_calc_param **dc_funcs,
                          struct dc_calc_param_input *input_head, int ncompo,
                          int commutative, int *nogood)
{
  ptrdiff_t npar;
  const char *conjunction;
  struct geom_list *lp, *lh;
  ptrdiff_t i;
  ptrdiff_t nmiss;

  if (commutative) {
    npar = dc_calc_binary_size_commutative(ncompo);
    conjunction = " and ";
  } else {
    npar = dc_calc_binary_size(ncompo);
    conjunction = " in ";
  }
  if (npar < 0) {
    CSVUNREACHABLE();
    if (nogood)
      *nogood = ON;
    return;
  }

  i = 1;
  nmiss = 0;
  lh = &input_head->list;
  geom_list_foreach (lp, lh) {
    struct dc_calc_param_input *p;
    struct dc_calc_param *dc_func;

    p = dc_calc_param_input_entry(lp);
    if (p->missing) {
      nmiss++;
      continue;
    }

    print_param_header(flg, head_char, indent, 3, len,
                       "%s Definition %" PRIdMAX, title, (intmax_t)i);
    if (commutative) {
      PP_component_info_data(flg, indent, "Material ID of Left Hand",
                             &p->diffusing.id, "", !!p->diffusing.id.d, nogood);
      PP_component_info_data(flg, indent, "Material ID of Right Hand",
                             &p->base.id, "", !!p->base.id.d, nogood);
    } else {
      PP_component_info_data(flg, indent, "Material ID for Diffusing",
                             &p->diffusing.id, "", !!p->diffusing.id.d, nogood);
      PP_component_info_data(flg, indent, "Material ID for Base",
                             &p->base.id, "", !!p->base.id.d, nogood);
    }

    dc_func = &p->data;
    CSVASSERT(dc_func);
    if (dc_func->model == DC_FUNCS_TEMPDEP_PROPERTY) {
      PPtempdep_property(flg, indent, NULL, ' ', "m2/s", "K", &dc_func->prop,
                         nogood);
    } else {
      PP_dc_func2_model(flg, indent, "Predefined Function", dc_func->model, "",
                        1, nogood);
    }
    i += 1;
  }
  if (nmiss > 0) {
    print_param_header(flg, head_char, indent, 3, len,
                       "%s of Following Combinations are not specified", title);
    geom_list_foreach (lp, lh) {
      struct dc_calc_param_input *p;
      int idiff, ibase;
      int alloc;
      char *c;

      p = dc_calc_param_input_entry(lp);
      if (!p->missing) {
        continue;
      }

      idiff = p->diffusing.id.d->jupiter_id;
      ibase = p->base.id.d->jupiter_id;

      PP_int_pair(flg, indent, "Material ID combination", idiff, ibase,
                  conjunction, "", 1, nogood);
    }
  }
}

static int PPtabfun_req_compid_test(const struct component_info_data *info,
                                    void *arg)
{
  component_phases ph;
  component_phases ls = *(component_phases *)arg;

  if (!info->d)
    return 0;

  ph = info->d->phases;
  if (!component_phases_eql(ls, component_phases_band(ph, ls)))
    return 0;
  return 1;
}

static int PPtabfun_opt_compid_test(const struct component_info_data *info,
                                    void *arg)
{
  if (!info->d) {
    if (info->id == -1)
      return 1;
  }

  return PPtabfun_req_compid_test(info, arg);
}

static void PPtabfunc_compid(flags *f, int indent, const char *name,
                             const struct component_info_data *value,
                             const char *unit, int opt, component_phases phases,
                             int *nogood)
{
  PP_component_info_data_ok_func *okfunc;
  okfunc = opt ? PPtabfun_opt_compid_test : PPtabfun_req_compid_test;
  PP_component_info_data_fulltest_f(f, indent, name, value, unit, okfunc,
                                    &phases, nogood);
}

void PPtm_table_param(flags *flg, int indent, const char *title, char head_char,
                      int len, struct tm_table_param *start,
                      component_phases phases, int *nogood)
{
  int i = 1;
  for (struct tm_table_param *tmp = start; tmp; tmp = tmp->next, ++i) {
    table_geometry tabg;
    table_size nx, ny;

    print_param_header(flg, head_char, indent, indent + 3, len,
                       "%s Table %d", title, i);
    PP_fname(flg, 0, "Table file", tmp->table_file, "",
             tmp->table_file && tmp->table, nogood);

    tabg = TABLE_GEOMETRY_INVALID;
    if (tmp->table) {
      tabg = table_get_geometry(tmp->table);
    }
    PP_table_geom(flg, indent, "Table Geometry", tabg, "",
                  tmp->yid.d ? (tabg == TABLE_GEOMETRY_SUM_CONSTANT) : 1,
                  nogood);

    nx = tmp->table ? table_get_nx(tmp->table) : 0;
    ny = tmp->table ? table_get_ny(tmp->table) : 0;
    PP_int(flg, 0, "Table nx", nx, "", 1, nogood);
    PP_int(flg, 0, "Table ny", ny, "", 1, nogood);
    PPtabfunc_compid(flg, 0, "Material ID for Remainder", &tmp->rid, "", 0,
                     phases, nogood);
    PPtabfunc_compid(flg, 0, "Material ID for X", &tmp->xid, "", 0, phases,
                     nogood);
    PPtabfunc_compid(flg, 0, "Material ID for Y", &tmp->yid, "", 1, phases,
                     nogood);
  }
}

void PPtm_funcs_param(flags *flg, int indent, const char *title, char head_char,
                      int len, struct tm_func2_param *start,
                      component_phases phases, int *nogood)
{
  int i = 1;
  for (struct tm_func2_param *tmp = start; tmp; tmp = tmp->next, ++i) {
    print_param_header(flg, head_char, indent, indent + 3, len,
                       "%s Built-in Function Usage %d", title, i);
    PP_tm_func2_model(flg, indent, "Function", tmp->model, "", 1, nogood);
    PPtabfunc_compid(flg, 0, "Material ID for Remainder", &tmp->yid, "",
                     1, phases, nogood);
    PPtabfunc_compid(flg, 0, "Material ID for function input", &tmp->xid, "",
                     0, phases, nogood);
  }
}
