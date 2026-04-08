#include "print_param_keywords.h"
#include "component_data_defs.h"
#include "geometry/enumutil.h"
#include "if_binary.h"
#include "struct.h"
#include "table/table.h"

#ifdef LPTX
#include "lptx/defs.h"
#endif

const char *pp_kwtype_value_null(void *arg) { return "(invalid)"; }

/**
 * @brief Make case label for enum to string conversion
 * @parem txt Variable to assign converted string.
 * @param px Prefix for getting right enum value
 * @param con Value to be converted to string
 *
 * @note this macro contains a statement that assigns string to @p txt.
 * This means you must add `break` for every `case` statements.
 */
#define PPvarcon(txt, px, con)     \
  px##con : txt = #con;            \
  goto PPvarcon_##px##con##_label; \
  PPvarcon_##px##con##_label

#define pp_format_get_value_data(str, a, value_data_ptr) \
  struct PP_##str##_value_data *value_data_ptr;          \
  value_data_ptr = (struct PP_##str##_value_data *)a

#define DEFINE_PP_FORMATTER(str)                \
  const char *PP_##str##_value_format(void *a)  \
  {                                             \
    pp_format_get_value_data(str, a, d);        \
    return PP_##str##_value_format_v(d->value); \
  }

#define DEFINE_PP_NULL_FORMATTER(str)          \
  const char *PP_##str##_value_null(void *arg) \
  {                                            \
    return pp_kwtype_value_null(arg);          \
  }

#define DEFINE_PP_OKFUNC(str)             \
  int PP_##str##_value_custom_ok(void *a) \
  {                                       \
    pp_format_get_value_data(str, a, d);  \
    if (!PP_##str##_value_format(a))      \
      return 0;                           \
    if (d->okfunc)                        \
      return d->okfunc(d->value, d->arg); \
    return 1;                             \
  }

const char *PP_bool_value_format_v(int value)
{
  const char *txt;

  txt = NULL;
  switch (value) {
  case PPvarcon(txt, , ON):
    break;
  case PPvarcon(txt, , OFF):
    break;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(bool)
DEFINE_PP_FORMATTER(bool)
DEFINE_PP_OKFUNC(bool)

const char *PP_bool_yn_value_format_v(int value)
{
  if (value) {
    return "YES";
  } else {
    return "NO";
  }
}

DEFINE_PP_NULL_FORMATTER(bool_yn)
DEFINE_PP_FORMATTER(bool_yn)
DEFINE_PP_OKFUNC(bool_yn)

const char *PP_ics_value_format_v(int value)
{
  const char *txt;
  txt = NULL;

  switch (value) {
  case PPvarcon(txt, , PLIC):
    break;
  case PPvarcon(txt, , THINC):
    break;
  case PPvarcon(txt, , THINC_WLIC):
    break;
  case PPvarcon(txt, , THINC_AWLIC):
    break;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(ics)
DEFINE_PP_FORMATTER(ics)
DEFINE_PP_OKFUNC(ics)

const char *PP_bnd_value_format_v(int value)
{
  const char *txt;
  txt = NULL;

  switch (value) {
  case PPvarcon(txt, , WALL):
    break;
  case PPvarcon(txt, , SLIP):
    break;
  case PPvarcon(txt, , OUT):
    break;
  case INLET:
    txt = "IN";
    break;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(bnd)
DEFINE_PP_FORMATTER(bnd)
DEFINE_PP_OKFUNC(bnd)

const char *PP_tbnd_value_format_v(int value)
{
  const char *txt;
  txt = NULL;

  switch (value) {
  case PPvarcon(txt, , INSULATION):
    break;
  case PPvarcon(txt, , ISOTHERMAL):
    break;
  case PPvarcon(txt, , DIFFUSION):
    break;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(tbnd)
DEFINE_PP_FORMATTER(tbnd)
DEFINE_PP_OKFUNC(tbnd)

const char *PP_out_p_cond_value_format_v(out_p_cond value)
{
  const char *txt;
  txt = NULL;

  switch (value) {
  case PPvarcon(txt, OUT_P_COND_, CONST):
    break;
  case PPvarcon(txt, OUT_P_COND_, NEUMANN):
    break;
  case OUT_P_COND_INVALID:
    break;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(out_p_cond)
DEFINE_PP_FORMATTER(out_p_cond)
DEFINE_PP_OKFUNC(out_p_cond)

const char *PP_control_value_format_v(trip_control value)
{
  const char *txt;
  txt = NULL;

  switch (value) {
  case PPvarcon(txt, TRIP_CONTROL_, CONST):
    break;
  case PPvarcon(txt, TRIP_CONTROL_, CONTROL):
    break;
  case PPvarcon(txt, TRIP_CONTROL_, PULSE):
    break;
  case TRIP_CONTROL_INVALID:
    break;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(control)
DEFINE_PP_FORMATTER(control)
DEFINE_PP_OKFUNC(control)

/**
 * Least 8 byte buffer is required for making string.
 */
const char *
PP_boundary_dir_value_format_v(boundary_direction value,
                               char buf[PP_boundary_dir_value_maxlen])
{
  const char *txt;
  int i;
  i = 0;

  txt = NULL;
  switch (value) {
  case PPvarcon(txt, BOUNDARY_DIR_, ALL):
    break;
  case PPvarcon(txt, BOUNDARY_DIR_, WEST):
    break;
  case PPvarcon(txt, BOUNDARY_DIR_, EAST):
    break;
  case PPvarcon(txt, BOUNDARY_DIR_, SOUTH):
    break;
  case PPvarcon(txt, BOUNDARY_DIR_, NORTH):
    break;
  case PPvarcon(txt, BOUNDARY_DIR_, BOTTOM):
    break;
  case PPvarcon(txt, BOUNDARY_DIR_, TOP):
    break;
  default:
    if (value & BOUNDARY_DIR_Z) {
      buf[i++] = 'Z';
    } else {
      if (value & BOUNDARY_DIR_BOTTOM) {
        buf[i++] = 'B';
      }
      if (value & BOUNDARY_DIR_TOP) {
        buf[i++] = 'T';
      }
    }
    if (value & BOUNDARY_DIR_Y) {
      buf[i++] = 'Y';
    } else {
      if (value & BOUNDARY_DIR_SOUTH) {
        buf[i++] = 'S';
      }
      if (value & BOUNDARY_DIR_NORTH) {
        buf[i++] = 'N';
      }
    }
    if (value & BOUNDARY_DIR_X) {
      buf[i++] = 'X';
    } else {
      if (value & BOUNDARY_DIR_WEST) {
        buf[i++] = 'W';
      }
      if (value & BOUNDARY_DIR_EAST) {
        buf[i++] = 'E';
      }
    }
  }
  if (i > 0) {
    buf[i] = '\0';
    txt = buf;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(boundary_dir)

const char *PP_boundary_dir_value_format(void *arg)
{
  pp_format_get_value_data(boundary_dir, arg, d);
  return PP_boundary_dir_value_format_v(d->value, d->buf);
}

DEFINE_PP_OKFUNC(boundary_dir)

const char *PP_surface_inlet_dir_value_format_v(surface_inlet_dir value)
{
  const char *txt;

  txt = NULL;
  switch (value) {
  case PPvarcon(txt, SURFACE_INLET_DIR_, NORMAL):
    break;
  case SURFACE_INLET_DIR_INVALID:
    break;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(surface_inlet_dir)
DEFINE_PP_FORMATTER(surface_inlet_dir)
DEFINE_PP_OKFUNC(surface_inlet_dir)

const char *PP_tm_func2_model_value_format_v(tm_func2_model value)
{
  const char *txt;

  txt = NULL;
  switch (value) {
  case PPvarcon(txt, TM_FUNC_, LIQUIDUS_FE_ZR):
    break;
  case PPvarcon(txt, TM_FUNC_, LIQUIDUS_FE_B):
    break;
  case TM_FUNC_INVALID:
    break;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(tm_func2_model)
DEFINE_PP_FORMATTER(tm_func2_model)
DEFINE_PP_OKFUNC(tm_func2_model)

const char *PP_dc_func2_model_value_format_v(dc_func2_model value)
{
  const char *txt;

  txt = NULL;
  switch (value) {
  case PPvarcon(txt, DC_FUNCS_, SUS304_ZIRCALOY):
    break;
  case PPvarcon(txt, DC_FUNCS_, ZIRCALOY_SUS304):
    break;
  case PPvarcon(txt, DC_FUNCS_, SUS304_B4C):
    break;
  case PPvarcon(txt, DC_FUNCS_, B4C_SUS304):
    break;
  case PPvarcon(txt, DC_FUNCS_, B4C_ZIRCALOY):
    break;
  case PPvarcon(txt, DC_FUNCS_, ZIRCALOY_B4C):
    break;
  case PPvarcon(txt, DC_FUNCS_, UO2_ZIRCALOY):
    break;
  case PPvarcon(txt, DC_FUNCS_, ZIRCALOY_UO2):
    break;
  case DC_FUNCS_TEMPDEP_PROPERTY:
    txt = "(by regular material)";
    break;
  case DC_FUNCS_INVALID:
    break;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(dc_func2_model)
DEFINE_PP_FORMATTER(dc_func2_model)
DEFINE_PP_OKFUNC(dc_func2_model)

const char *PP_solid_form_value_format_v(enum solid_form value)
{
  const char *txt;

  txt = NULL;
  switch (value) {
  case PPvarcon(txt, SOLID_FORM_, IBM):
    break;
  case PPvarcon(txt, SOLID_FORM_, POROUS):
    break;
  case SOLID_FORM_UNUSED:
    txt = "(no solid)";
    break;
  case SOLID_FORM_INVALID:
    break;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(solid_form)
DEFINE_PP_FORMATTER(solid_form)
DEFINE_PP_OKFUNC(solid_form)

const char *PP_binary_output_mode_value_format_v(binary_output_mode value)
{
  const char *txt;

  txt = NULL;
  switch (value) {
  case PPvarcon(txt, BINARY_OUTPUT_, BYPROCESS):
    break;
  case PPvarcon(txt, BINARY_OUTPUT_, UNIFY_MPI):
    break;
  case PPvarcon(txt, BINARY_OUTPUT_, UNIFY_GATHER):
    break;
  case BINARY_OUTPUT_INVALID:
    break;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(binary_output_mode)
DEFINE_PP_FORMATTER(binary_output_mode)
DEFINE_PP_OKFUNC(binary_output_mode)

const char *PP_tempdep_property_type_value_format_v(tempdep_property_type value)
{
  const char *txt;

  txt = NULL;
  switch (value) {
  case PPvarcon(txt, TEMPDEP_PROPERTY_, CONST):
    break;
  case PPvarcon(txt, TEMPDEP_PROPERTY_, POLY):
    break;
  case PPvarcon(txt, TEMPDEP_PROPERTY_, POLY_L):
    break;
  case PPvarcon(txt, TEMPDEP_PROPERTY_, TABLE):
    break;
  case PPvarcon(txt, TEMPDEP_PROPERTY_, ARRHENIUS):
    break;
  case PPvarcon(txt, TEMPDEP_PROPERTY_, PIECEWISE):
    break;
  case PPvarcon(txt, TEMPDEP_PROPERTY_, OX_URBANIC_HEIDRICK):
    break;
  case PPvarcon(txt, TEMPDEP_PROPERTY_, OX_BAKER_JUST):
    break;
  case PPvarcon(txt, TEMPDEP_PROPERTY_, OX_CATHCART_PAWEL):
    break;
  case PPvarcon(txt, TEMPDEP_PROPERTY_, OX_LEISTIKOW_SCHANZ):
    break;
  case PPvarcon(txt, TEMPDEP_PROPERTY_, OX_PRATER_COURTRIGHT):
    break;
  case PPvarcon(txt, TEMPDEP_PROPERTY_, OX_RECESSION):
    break;
  case TEMPDEP_PROPERTY_INVALID:
    break;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(tempdep_property_type)
DEFINE_PP_FORMATTER(tempdep_property_type)
DEFINE_PP_OKFUNC(tempdep_property_type)

const char *PP_LPTts_value_format_v(int value)
{
  const char *txt;
#ifdef LPT
  LPT_time_scheme lv;
#endif
#ifdef LPTX
  LPTX_time_scheme lv;
#endif

  txt = NULL;

#ifdef LPT
  lv = value;
  switch (lv) {
  case PPvarcon(txt, LPT_TIME_SCHEME_, ADAMS_BASHFORTH_2):
    break;
  case PPvarcon(txt, LPT_TIME_SCHEME_, RUNGE_KUTTA_2):
    break;
  case PPvarcon(txt, LPT_TIME_SCHEME_, RUNGE_KUTTA_3):
    break;
  case LPT_TIME_SCHEME_INVALID:
    break;
  }
#endif
#ifdef LPTX
  lv = value;
  switch (lv) {
  case PPvarcon(txt, LPTX_TIME_SCHEME_, ADAMS_BASHFORTH_2):
    break;
  case PPvarcon(txt, LPTX_TIME_SCHEME_, RUNGE_KUTTA_2):
    break;
  case PPvarcon(txt, LPTX_TIME_SCHEME_, RUNGE_KUTTA_3):
    break;
  case LPTX_TIME_SCHEME_INVALID:
    break;
  }
#endif

  return txt;
}

const char *PP_LPTts_value_null(void *arg)
{
#ifdef HAVE_LPT
  return pp_kwtype_value_null(arg);
#else
  return "(LPT is not complied in)";
#endif
}

DEFINE_PP_FORMATTER(LPTts)
DEFINE_PP_OKFUNC(LPTts)

#ifdef LPTX
static const char *PP_LPTht_value_format_v_LPTX(int value)
{
  const char *txt = NULL;
  LPTX_heat_scheme lv = value;
  switch (lv) {
  case PPvarcon(txt, LPTX_HEAT_, OFF):
    break;
  case PPvarcon(txt, LPTX_HEAT_, FOLLOW_FLUID):
    break;
  case PPvarcon(txt, LPTX_HEAT_, RANZ_MARSHALL):
    break;
  case LPTX_HEAT_INVALID:
    break;
  }
  return txt;
}
#endif

const char *PP_LPTht_value_format_v(int value)
{
#ifdef LPTX
  return PP_LPTht_value_format_v_LPTX(value);
#endif

  if (value == OFF)
    return "OFF";
  return NULL;
}

const char *PP_LPTht_value_null(void *arg)
{
#ifdef HAVE_LPT
  return pp_kwtype_value_null(arg);
#else
  return "(LPT is not complied in)";
#endif
}

DEFINE_PP_FORMATTER(LPTht)
DEFINE_PP_OKFUNC(LPTht)

const char *PP_ox_kp_model_value_format_v(enum ox_reaction_rate_model value)
{
  const char *txt;

  txt = NULL;
  switch (value) {
  case PPvarcon(txt, OX_RRMODEL_, URBANIC_HEIDRICK):
    break;
  case PPvarcon(txt, OX_RRMODEL_, BAKER_JUST):
    break;
  case PPvarcon(txt, OX_RRMODEL_, CATHCART_PAWEL):
    break;
  case PPvarcon(txt, OX_RRMODEL_, LEISTIKOW_SCHANZ):
    break;
  case PPvarcon(txt, OX_RRMODEL_, PRATER_COURTRIGHT):
    break;
  case OX_RRMODEL_TEMPDEP:
    txt = "AS_MATERIAL";
    break;
  case OX_RRMODEL_INVALID:
    break;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(ox_kp_model)
DEFINE_PP_FORMATTER(ox_kp_model)
DEFINE_PP_OKFUNC(ox_kp_model)

const char *PP_table_geom_value_format_v(table_geometry value)
{
  const char *txt;

  txt = NULL;
  switch (value) {
  case PPvarcon(txt, TABLE_GEOMETRY_, RECTILINEAR):
    break;
  case PPvarcon(txt, TABLE_GEOMETRY_, SUM_CONSTANT):
    break;
  case TABLE_GEOMETRY_INVALID:
    break;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(table_geom)
DEFINE_PP_FORMATTER(table_geom)
DEFINE_PP_OKFUNC(table_geom)

const char *PP_component_phase_value_format_v(component_phase_name value)
{
  const char *txt;

  txt = NULL;
  switch (value) {
  case PPvarcon(txt, COMPONENT_PHASE_, GAS):
    break;
  case PPvarcon(txt, COMPONENT_PHASE_, SOLID):
    break;
  case PPvarcon(txt, COMPONENT_PHASE_, LIQUID):
    break;
  case COMPONENT_PHASE_MAX:
    break;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(component_phase)
DEFINE_PP_FORMATTER(component_phase)
DEFINE_PP_OKFUNC(component_phase)

const char *
PP_non_uniform_grid_func_value_format_v(non_uniform_grid_function value)
{
  const char *txt;

  txt = NULL;
  switch (value) {
  case PPvarcon(txt, NON_UNIFORM_GRID_FUNC_, CONST):
    break;
  case PPvarcon(txt, NON_UNIFORM_GRID_FUNC_, CONST_RATIO_INC):
    break;
  case PPvarcon(txt, NON_UNIFORM_GRID_FUNC_, CONST_RATIO_DEC):
    break;
  case PPvarcon(txt, NON_UNIFORM_GRID_FUNC_, SINE):
    break;
  case PPvarcon(txt, NON_UNIFORM_GRID_FUNC_, QSINE_B):
    break;
  case PPvarcon(txt, NON_UNIFORM_GRID_FUNC_, QSINE_E):
    break;
  case NON_UNIFORM_GRID_FUNC_INVALID:
    break;
  }
  return txt;
}

DEFINE_PP_NULL_FORMATTER(non_uniform_grid_func)
DEFINE_PP_FORMATTER(non_uniform_grid_func)
DEFINE_PP_OKFUNC(non_uniform_grid_func)

const char *PP_geom_op_value_format_v(geom_data_operator value)
{
  return geom_data_operator_to_str(value);
}

DEFINE_PP_NULL_FORMATTER(geom_op)
DEFINE_PP_FORMATTER(geom_op)
DEFINE_PP_OKFUNC(geom_op)

const char *PP_geom_sop_value_format_v(geom_shape_operator value)
{
  return geom_shape_operator_to_str(value);
}

DEFINE_PP_NULL_FORMATTER(geom_sop)
DEFINE_PP_FORMATTER(geom_sop)
DEFINE_PP_OKFUNC(geom_sop)

const char *PP_init_func_value_format_v(geom_init_func value)
{
  if (value == jupiter_init_func_binary_data_id())
    return "BINARY";
  return geom_init_func_to_str(value);
}

DEFINE_PP_NULL_FORMATTER(init_func)
DEFINE_PP_FORMATTER(init_func)
DEFINE_PP_OKFUNC(init_func)

const char *PP_vphase_value_format_v(geom_vof_phase value)
{
  return geom_vof_phase_to_str(value);
}

DEFINE_PP_NULL_FORMATTER(vphase)
DEFINE_PP_FORMATTER(vphase)
DEFINE_PP_OKFUNC(vphase)

const char *PP_gshape_value_format_v(geom_shape value)
{
  return geom_shape_to_str(value);
}

DEFINE_PP_NULL_FORMATTER(gshape)
DEFINE_PP_FORMATTER(gshape)
DEFINE_PP_OKFUNC(gshape)

const char *PP_gsshape_value_format_v(geom_surface_shape value)
{
  return geom_surface_shape_to_str(value);
}

DEFINE_PP_NULL_FORMATTER(gsshape)
DEFINE_PP_FORMATTER(gsshape)
DEFINE_PP_OKFUNC(gsshape)
