#ifndef JUPITER_PRINT_PARAM_PROPS_H
#define JUPITER_PRINT_PARAM_PROPS_H

/*
 * print param for tempdep_property, phase_value_component, dc_calc_param,
 * tm_table_param and tm_funcs_param
 */

#include "component_data_defs.h"
#include "struct.h"
#include "dccalc.h"
#include "print_param_basic.h"
#include "print_param_core.h"
#include "tempdep_properties.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

JUPITER_DECL
void PPtempdep_property(flags *flg, int indent, const char *title,
                        char head_char, const char *base_unit,
                        const char *temperature_unit, tempdep_property *prop,
                        int *nogood);

/**
 * @param tm_sol_from_table Whether solidus temperature is calculated by
 * table.
 * @param tm_liq_from_table Whether liquidus temperature is calculated by
 * table.
 */
JUPITER_DECL
void PPphase_value_component(flags *flg, int indent, int tm_sol_from_table,
                             int tm_liq_from_table, phase_value_component *f,
                             int *nogood);

JUPITER_DECL
void PPphase_value_component_g(flags *flg, int indent, phase_value_component *f,
                               int *nogood);

JUPITER_DECL
void PPbinary_diffusivity(flags *flg, int indent, const char *title,
                          char head_char, int len,
                          struct dc_calc_param **dc_funcs,
                          struct dc_calc_param_input *input_head, int ncompo,
                          int commutative, int *nogood);

JUPITER_DECL
void PPtm_table_param(flags *flg, int indent, const char *title, char head_char,
                      int len, struct tm_table_param *start,
                      component_phases phases, int *nogood);

JUPITER_DECL
void PPtm_funcs_param(flags *flg, int indent, const char *title, char head_char,
                      int len, struct tm_func2_param *start,
                      component_phases phases, int *nogood);

/**
 * Prints "(From Table)" (or other string) when flagged, otherwise print double
 * value.
 */
struct PP_table_or_double_value_data
{
  struct pp_format_value_data *value;
  union PP_table_or_double_value_set
  {
    struct PP_charp_value_data table;
    struct PP_double_value_data value;
  } set;
};

static inline struct pp_format_value_data *
PP_table_or_double_velue_init(struct PP_table_or_double_value_data *data,
                              int flag, const char *text_for_flag, double value,
                              PP_double_ok_func *okfunc, void *okarg)
{
  if (flag) {
    data->value = PP_charp_value_init(&data->set.table, text_for_flag,
                                      "(From Table)", NULL, NULL);
  } else {
    data->value = PP_double_value_init(&data->set.value, value, okfunc, okarg);
  }
  return data->value;
}

static inline void PP_table_or_double_f(flags *f, int indent, const char *name,
                                        int flag, const char *text_for_flag,
                                        double value, const char *unit_table,
                                        const char *unit_value,
                                        PP_double_ok_func *okfunc, void *okarg,
                                        int *nogood)
{
  struct pp_format_value_data *p;
  struct PP_charp_name_data n;
  struct PP_table_or_double_value_data v;
  struct PP_charp_unit_data u;
  const char *unit;

  PP_charp_name_init(&n, name);
  p = PP_table_or_double_velue_init(&v, flag, text_for_flag, value, okfunc,
                                    okarg);
  unit = flag ? unit_table : unit_value;
  unit = unit ? unit : "";
  PP_charp_unit_init(&u, unit);

  pp_m_line_f(f, indent, PP_BASELEN, &n.data, p, &u.data, nogood);
}

static inline void PP_table_or_double(flags *f, int indent, const char *name,
                                      int flag, const char *text_for_flag,
                                      double value, const char *unit_table,
                                      const char *unit_value, int ok,
                                      int *nogood)
{
  struct PP_double_ok_precalc_data d = {.ok = ok};
  PP_table_or_double_f(f, indent, name, flag, text_for_flag, value, unit_table,
                       unit_value, PP_double_ok_precalc, &d, nogood);
}

#ifdef __cplusplus
}
#endif

#endif
