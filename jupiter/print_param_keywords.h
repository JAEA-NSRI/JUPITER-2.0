#ifndef JUPITER_PRINT_PARAM_KEYWORDS_H
#define JUPITER_PRINT_PARAM_KEYWORDS_H

#include "common.h"
#include "struct.h"

#include "dccalc.h"
#include "geometry/defs.h"
#include "print_param_basic.h"
#include "print_param_core.h"
#include "table/table.h"
#include "tempdep_properties.h"
#include "tmcalc.h"

#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* static enum values (does not require memory allocation) */

/**
 * Returns `(invalid)`.
 */
const char *pp_kwtype_value_null(void *arg);

static inline struct pp_format_value_data *
pp_kwtype_value_init_base(struct pp_format_value_data *p, pp_format_func *value,
                          pp_format_func *value_null, pp_format_okfunc *okfunc,
                          void *data)
{
  return pp_format_value_init(p,value, value_null, okfunc, NULL, data);
}

#define pp_s_value_type(str) PP_##str##_value_type

#define pp_s_value_okfunc_typedef(str) \
  typedef int PP_##str##_ok_func(pp_s_value_type(str) value, void *arg)

#define pp_s_value_data(str, xstr)    \
  struct xstr                         \
  {                                   \
    struct pp_format_value_data data; \
    pp_s_value_type(str) value;       \
    PP_##str##_ok_func *okfunc;       \
    void *arg;                        \
  }

#define PP_S_VALUE_DATA_DEF(str) pp_s_value_data(str, PP_##str##_value_data)

#define pp_s_value_init_sig(str, xstr)                                 \
  struct pp_format_value_data                                          \
    *PP_##str##_value_init(struct xstr *p, pp_s_value_type(str) value, \
                           PP_##str##_ok_func *okfunc, void *arg)

#define PP_S_VALUE_INIT_DEF(str)                                        \
  static inline pp_s_value_init_sig(str, PP_##str##_value_data)         \
  {                                                                     \
    p->value = value;                                                   \
    p->okfunc = okfunc;                                                 \
    p->arg = arg;                                                       \
    return pp_kwtype_value_init_base(&p->data, PP_##str##_value_format, \
                                     PP_##str##_value_null,             \
                                     PP_##str##_value_custom_ok, p);    \
  }

#define pp_s_precalc_ok_sig(str) \
  int PP_##str##_precalc_ok(pp_s_value_type(str) value, void *arg)

#define pp_s_f_sig(str)                                           \
  void PP_##str##_f(flags *f, int indent, const char *name,       \
                    pp_s_value_type(str) value, const char *unit, \
                    PP_##str##_ok_func *okfunc, void *arg, int *nogood)

#define pp_s_c_sig(str)                                               \
  void PP_##str(flags *f, int indent, const char *name,               \
                pp_s_value_type(str) value, const char *unit, int ok, \
                int *nogood)

#define PP_F_FUNC_DEF(str)                                            \
  static inline pp_s_f_sig(str)                                       \
  {                                                                   \
    struct PP_charp_name_data n;                                      \
    struct PP_##str##_value_data v;                                   \
    struct PP_charp_unit_data u;                                      \
    struct pp_format_value_data *vp;                                  \
    PP_charp_name_init(&n, name);                                     \
    vp = PP_##str##_value_init(&v, value, okfunc, arg);               \
    PP_charp_unit_init(&u, unit);                                     \
                                                                      \
    pp_m_line_f(f, indent, PP_BASELEN, &n.data, vp, &u.data, nogood); \
  }

#define PP_C_FUNC_DEF(str)                                                \
  struct PP_##str##_precalc_data                                          \
  {                                                                       \
    int ok;                                                               \
  };                                                                      \
                                                                          \
  static inline pp_s_precalc_ok_sig(str)                                  \
  {                                                                       \
    return ((struct PP_##str##_precalc_data *)arg)->ok;                   \
  }                                                                       \
                                                                          \
  static inline pp_s_c_sig(str)                                           \
  {                                                                       \
    struct PP_##str##_precalc_data d = {.ok = ok};                        \
    PP_##str##_f(f, indent, name, value, unit, PP_##str##_precalc_ok, &d, \
                 nogood);                                                 \
  }

/**
 * Redefine for the use of PP_KW_FUNC_ALL in external library/executable
 */
#define PP_KW_FUNC_DECL JUPITER_DECL

/**
 * Following functions will be declared for keyword type @p str.
 * Please implement them in print_param_keywords.c.
 *
 * - const char *PP_##str##_value_format_v(value_type value);
 *   - Native converter
 *
 * - const char *PP_##str##_value_format(void *arg);
 *   - PP_* formatter
 *
 * - const char *PP_##str##_value_null(void *arg);
 *   - PP_* null value formatter
 *
 * - int PP_##str##_value_custom_ok(void *arg);
 *   - Aggregator for user-provided PP_##str##_ok_func function
 *
 * @note `void *` argument will be passed pointer to `struct
 * pp_format_value_data`. The pointer to `struct PP_##str##_value_data`
 * is stored in `data` member of it.
 *
 * Following functions will be defined for keyword type @p str,
 * as compatibility function.
 *
 * - void PP_##str##_f(flags *f, int indent, const char *name, value_type value,
 *   const char *unit, PP_##str##_ok_func *okfunc, void *arg, int *nogood)
 *
 * - void PP_##str##(flags *f, int indent, const char *name, value_type value,
 *   const char *unit, int ok, int *nogood)
 */
#define PP_KW_FUNC_ALL(str)                                                  \
  PP_KW_FUNC_DECL const char *PP_##str##_value_format_v(pp_s_value_type(str) \
                                                          value);            \
  PP_KW_FUNC_DECL const char *PP_##str##_value_format(void *arg);            \
  PP_KW_FUNC_DECL const char *PP_##str##_value_null(void *arg);              \
  PP_KW_FUNC_DECL int PP_##str##_value_custom_ok(void *arg);                 \
  pp_s_value_okfunc_typedef(str);                                            \
  PP_S_VALUE_DATA_DEF(str);                                                  \
  PP_S_VALUE_INIT_DEF(str)                                                   \
  PP_F_FUNC_DEF(str)                                                         \
  PP_C_FUNC_DEF(str)

/* ON/OFF bool */
#define PP_bool_value_type int
PP_KW_FUNC_ALL(bool)

/* Print boolean value as YES/NO */
#define PP_bool_yn_value_type int
PP_KW_FUNC_ALL(bool_yn)

/* interface_capturing_scheme */
#define PP_ics_value_type int
PP_KW_FUNC_ALL(ics)

/* Boundary */
#define PP_bnd_value_type int
PP_KW_FUNC_ALL(bnd)

/* Thermal boundary */
#define PP_tbnd_value_type int
PP_KW_FUNC_ALL(tbnd)

/* OUT pressure conditon type */
#define PP_out_p_cond_value_type out_p_cond
PP_KW_FUNC_ALL(out_p_cond)

/* Control type */
#define PP_control_value_type trip_control
PP_KW_FUNC_ALL(control)

/* Boundary direction */
#define PP_boundary_dir_value_maxlen 8
#define PP_boundary_dir_value_type boundary_direction
JUPITER_DECL
const char *
PP_boundary_dir_value_format_v(boundary_direction value,
                               char buf[PP_boundary_dir_value_maxlen]);
JUPITER_DECL
const char *PP_boundary_dir_value_format(void *);
JUPITER_DECL
const char *PP_boundary_dir_value_null(void *);
JUPITER_DECL
int PP_boundary_dir_value_custom_ok(void *);
pp_s_value_okfunc_typedef(boundary_dir);
struct PP_boundary_dir_value_data
{
  struct pp_format_value_data data;
  boundary_direction value;
  PP_boundary_dir_ok_func *okfunc;
  void *arg;
  char buf[PP_boundary_dir_value_maxlen];
};
PP_S_VALUE_INIT_DEF(boundary_dir)
PP_F_FUNC_DEF(boundary_dir)
PP_C_FUNC_DEF(boundary_dir)

/* Surface inlet direction */
#define PP_surface_inlet_dir_value_type surface_inlet_dir
PP_KW_FUNC_ALL(surface_inlet_dir)

/* Predefined liqudus temperature function */
#define PP_tm_func2_model_value_type tm_func2_model
PP_KW_FUNC_ALL(tm_func2_model)

/* Predefined solute diffusivity function */
#define PP_dc_func2_model_value_type dc_func2_model
PP_KW_FUNC_ALL(dc_func2_model)

/* Solid form */
#define PP_solid_form_value_type enum solid_form
PP_KW_FUNC_ALL(solid_form)

/* Binary output mode */
#define PP_binary_output_mode_value_type binary_output_mode
PP_KW_FUNC_ALL(binary_output_mode)

/* Temperature dependency function */
#define PP_tempdep_property_type_value_type tempdep_property_type
PP_KW_FUNC_ALL(tempdep_property_type)

/* LPT time scheme */
#define PP_LPTts_value_type int
PP_KW_FUNC_ALL(LPTts)

/* LPT heat scheme */
#define PP_LPTht_value_type int
PP_KW_FUNC_ALL(LPTht)

/* Oxidation kp model */
#define PP_ox_kp_model_value_type enum ox_reaction_rate_model
PP_KW_FUNC_ALL(ox_kp_model)

/* Table geometry */
#define PP_table_geom_value_type table_geometry
PP_KW_FUNC_ALL(table_geom)

/* Component phase */
#define PP_component_phase_value_type component_phase_name
PP_KW_FUNC_ALL(component_phase)

/* non-uniform-grid func */
#define PP_non_uniform_grid_func_value_type non_uniform_grid_function
PP_KW_FUNC_ALL(non_uniform_grid_func)

/* Geometry data operator */
#define PP_geom_op_value_type geom_data_operator
PP_KW_FUNC_ALL(geom_op)

/* Geometry shape operator */
#define PP_geom_sop_value_type geom_shape_operator
PP_KW_FUNC_ALL(geom_sop)

/* Geometry init functon */
#define PP_init_func_value_type geom_init_func
PP_KW_FUNC_ALL(init_func)

/* Phase Name */
#define PP_vphase_value_type geom_vof_phase
PP_KW_FUNC_ALL(vphase)

/* Geometry shape */
#define PP_gshape_value_type geom_shape
PP_KW_FUNC_ALL(gshape)

/* Geometry surface shape */
#define PP_gsshape_value_type geom_surface_shape
PP_KW_FUNC_ALL(gsshape)

#undef PP_KW_FUNC_DECL

#ifdef __cplusplus
}
#endif

#endif
