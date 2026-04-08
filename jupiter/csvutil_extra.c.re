/* This is -*- c -*- source. */
/* This is vim: set ft=c: source. */

#include <string.h>

#include "csvutil.h"
#include "csvutil_extra.h"
#include "dccalc.h"
#include "if_binary.h"

#include "geometry/defs.h"
#include "control/defs.h"
#include "control/fv_table.h"
#include "control/write_fv_csv.h"
#include "jupiter/csv.h"
#include "struct.h"
#include "tempdep_properties.h"
#include "tmcalc.h"

#ifdef LPT
#include "lpt/LPTdefs.h"
#endif

#ifdef LPTX
#include "lptx/defs.h"
#endif

/* Convenient utility which uses re2c */
/*!re2c
  end = "\x00";
*/

#define EXACT_MATCH_FLAG 0        /* Set 1 for EXACT MATCH */

/**
 * Use strncasecmp (in POSIX) for case insensitive compare but it is
 * not in C-standard.
 */
#define COMPARE_FUNC     strncmp

/**
 * @brief text to constant converter.
 *
 * If text in con (string), set var to val.
 * If it does not match, var is stay unchanged.
 *
 * con should be constant string expression.
 */
#define TXT2CON_X(var, text, con, val)                              \
  do {                                                              \
    if ((COMPARE_FUNC)(text, con,                                   \
                       (strlen(con) + (EXACT_MATCH_FLAG))) == 0) {  \
      var = val;                                                    \
    }                                                               \
  } while(0)

/**
 * @brief text to constant converter.
 */
#define TXT2CON(var, text, con, prefix)         \
  TXT2CON_X(var, text, #con, prefix##con)

/**
 * @brief text to constant converter
 *
 * Same as `TXT2CON`, but expand argument before stringify con(c).
 *
 * Prefix argument is mandatory because con will be expanded to an
 * actual value and stringify it when prefix is not present.
 * (ex. where `C` is `0`, `TXT2CON_E(A, B, C, )` treated as
 * `TXT2CON(A, B, 0, ))`
 *
 * Usage of TXT2CON:
 *
 *     #define P_AAA 1
 *     TXT2CON(var, text, AAA, P_)
 *
 *
 * Usage of TXT2CON_E:
 *
 *     #define P_AAA 1
 *     #define P_BBB 2
 *     #if CONDITION_A
 *     #define P_MODE AAA  // Use P_AAA in this case.
 *     #else
 *     #define P_MODE BBB  // Use P_BBB otherwise.
 *     #endif
 *
 * and very later:
 *
 *     TXT2CON_E(var, text, P_MODE, P_)
 *
 * will parses `P_AAA` for `CONDITION_A` is (was) true, `P_BBB` otherwise.
 */
#define TXT2CON_E(a,b,c,d) TXT2CON(a,b,c,d)

/* Keyword based matching functions */
/*!re2c
  re2c:define:YYCTYPE = "unsigned char";
  re2c:define:YYMARKER = "m";
  re2c:define:YYCURSOR = "p";
  re2c:yyfill:enable = 0;
  re2c:indent:string = "  ";

  // requires re2c 1.0 or later.

  // If you prefer case insensitive matching, set to 1.
  re2c:flags:case-insensitive = 0;

  // Common mandatory postfix (using '\x00' to make exact match)
  // If you prefer not to be exact match for keyword, set to empty string "".
  kend = "\x00";
*/

int str_to_bool(const char *p)
{
  const char *m;

  /*
   * It's highly recommended to use re2c to add new keyword (because
   * POSIX-only function, `strncasecmp`, is not required for
   * case-insensitive comparison), but if you are going to add new
   * keyword without re2c, you can use old text coversion macro
   * here. For example:
   *
   * ```
   *  int r;
   *  r = -1;
   *  TXT2CON(r, p, SOMETHING, );
   *  if (r >= 0) return r;
   * ```
   */

  /*!re2c
    re2c:indent:top = 1;

    *          { return -1; }
    "ON"/kend  { return ON; }
    "OFF"/kend { return OFF; }
  */
  CSVUNREACHABLE();
  return -1;
}

int str_to_interface_capturing_scheme(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return -1; }
    "PLIC"/kend   { return PLIC; }
    "THINC"/kend   { return THINC; }
    "THINC_WLIC"/kend    { return THINC_WLIC; }
    "THINC_AWLIC"/kend     { return THINC_AWLIC; }
  */
  CSVUNREACHABLE();
  return -1;
}

int str_to_boundary(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return -1; }
    "WALL"/kend   { return WALL; }
    "SLIP"/kend   { return SLIP; }
    "OUT"/kend    { return OUT; }
    "IN"/kend     { return INLET; }
  */
  CSVUNREACHABLE();
  return -1;
}

int str_to_tboundary(const char *p)
{
  const char *m;

  /*!re2c
    re2c:indent:top = 1;

    * { return -1; }
    "INSULATION"/kend  { return INSULATION; }
    "ISOTHERMAL"/kend  { return ISOTHERMAL; }
    "DIFFUSION"/kend   { return DIFFUSION; }
  */
  CSVUNREACHABLE();
  return -1;
}

out_p_cond str_to_out_p_cond(const char *p)
{
  const char *m;

  /*!re2c
    re2c:indent:top = 1;

    * { return OUT_P_COND_INVALID; }
    "NEUMANN"/kend { return OUT_P_COND_NEUMANN; }
    "CONST"/kend   { return OUT_P_COND_CONST; }
   */
  CSVUNREACHABLE();
  return -1;
}

geom_vof_phase str_to_vof_phase(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return GEOM_PHASE_INVALID; }
    "SOLID"/kend  { return GEOM_PHASE_SOLID; }
    "LIQUID"/kend { return GEOM_PHASE_LIQUID; }
    "GAS"/kend    { return GEOM_PHASE_GAS; }
  */
  CSVUNREACHABLE();
  return -1;
}

geom_data_operator str_to_geom_data_op(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return GEOM_OP_INVALID; }
    "SET"/kend  { return GEOM_OP_SET; }
    "ADD"/kend  { return GEOM_OP_ADD; }
    "MUL"/kend  { return GEOM_OP_MUL; }
    "SUB"/kend  { return GEOM_OP_SUB; }
    "NONE"/kend { return GEOM_OP_NONE; }
  */
  CSVUNREACHABLE();
  return -1;
}

geom_shape_operator str_to_geom_shape_op(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return GEOM_SOP_INVALID; }
    "SET"/kend  { return GEOM_SOP_SET; }
    "PUSH"/kend { return GEOM_SOP_PUSH; }
    "ADD"/kend  { return GEOM_SOP_ADD; }
    "OR"/kend   { return GEOM_SOP_OR; }
    "MUL"/kend  { return GEOM_SOP_MUL; }
    "AND"/kend  { return GEOM_SOP_AND; }
    "SUB"/kend  { return GEOM_SOP_SUB; }
    "XOR"/kend  { return GEOM_SOP_XOR; }
  */
  CSVUNREACHABLE();
  return -1;
}

geom_shape str_to_geom_shape(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return GEOM_SHAPE_INVALID; }

    // bodies
    "BOX"/kend  { return GEOM_SHAPE_BOX; }
    "PLA"/kend  { return GEOM_SHAPE_PLA; }
    "WED"/kend  { return GEOM_SHAPE_WED; }
    "ARB"/kend  { return GEOM_SHAPE_ARB; }
    "APP"/kend  { return GEOM_SHAPE_APP; }
    "RPR"/kend  { return GEOM_SHAPE_RPR; }
    "TRP"/kend  { return GEOM_SHAPE_TRP; }
    "RCC"/kend  { return GEOM_SHAPE_RCC; }
    "SPH"/kend  { return GEOM_SHAPE_SPH; }
    "TRC"/kend  { return GEOM_SHAPE_TRC; }
    "REC"/kend  { return GEOM_SHAPE_REC; }
    "TEC"/kend  { return GEOM_SHAPE_TEC; }
    "ELL"/kend  { return GEOM_SHAPE_ELL; }
    "TOR"/kend  { return GEOM_SHAPE_TOR; }
    "ETO"/kend  { return GEOM_SHAPE_ETO; }
    "PL1"/kend  { return GEOM_SHAPE_PL1; }
    "PL2"/kend  { return GEOM_SHAPE_PL2; }
    "PL3"/kend  { return GEOM_SHAPE_PL3; }
    "PL4"/kend  { return GEOM_SHAPE_PL4; }
    "PLN"/kend  { return GEOM_SHAPE_PLN; }

    // transformations
    "TRA"/kend  { return GEOM_SHAPE_TRA; }
    "ROT"/kend  { return GEOM_SHAPE_ROT; }
    "SCA"/kend  { return GEOM_SHAPE_SCA; }
    "MAT"/kend  { return GEOM_SHAPE_MAT; }

    // special commands
    "COMB"/kend { return GEOM_SHAPE_COMB; }
    "GST"/kend  { return GEOM_SHAPE_GST; }
    "GED"/kend  { return GEOM_SHAPE_GED; }
  */
  CSVUNREACHABLE();
  return GEOM_SHAPE_INVALID;
}

geom_surface_shape str_to_geom_surface_shape(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return GEOM_SURFACE_SHAPE_INVALID; }

    // bodies
    "PARALLELOGRAM"/kend  { return GEOM_SURFACE_SHAPE_PARALLELOGRAM; }

    // transformations

    // special commands
    "COMB"/kend { return GEOM_SURFACE_SHAPE_COMB; }
  */
  CSVUNREACHABLE();
  return GEOM_SURFACE_SHAPE_INVALID;
}

geom_init_func str_to_init_func(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return GEOM_INIT_FUNC_INVALID; }
    "CONST"/kend   { return GEOM_INIT_FUNC_CONST; }
    "LINEAR"/kend  { return GEOM_INIT_FUNC_LINEAR; }
    "POLY_N"/kend  { return GEOM_INIT_FUNC_POLY_N; }
    "POLY"/kend    { return GEOM_INIT_FUNC_POLY; }
    "EXP_POLY"/kend { return GEOM_INIT_FUNC_EXP_POLY; }
    "BINARY"/kend  { return jupiter_init_func_binary_data_id(); }
  */
  CSVUNREACHABLE();
  return GEOM_INIT_FUNC_INVALID;
}

trip_control str_to_trip_control(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return TRIP_CONTROL_INVALID; }
    "CONST"/kend   { return TRIP_CONTROL_CONST; }
    "CONTROL"/kend { return TRIP_CONTROL_CONTROL; }
  */
  CSVUNREACHABLE();
  return TRIP_CONTROL_INVALID;
}

boundary_direction str_to_boundary_dir(const char *p)
{
  const char *m;
  const char *op;
  boundary_direction dir;
  op = p;

  for (;;) {
    /*!re2c
      re2c:indent:top = 2;

      *             { break; }
      "ALL"/kend    { return BOUNDARY_DIR_ALL; }
      "WEST"/kend   { return BOUNDARY_DIR_WEST; }
      "EAST"/kend   { return BOUNDARY_DIR_EAST; }
      "SOUTH"/kend  { return BOUNDARY_DIR_SOUTH; }
      "NORTH"/kend  { return BOUNDARY_DIR_NORTH; }
      "BOTTOM"/kend { return BOUNDARY_DIR_BOTTOM; }
      "TOP"/kend    { return BOUNDARY_DIR_TOP; }
    */
  }

  p = op;
  dir = BOUNDARY_DIR_NONE;
  for (;;) {
    /*!re2c
      re2c:indent:top = 2;

      *    { return BOUNDARY_DIR_NONE; }
      kend { break; }
      "W"  { dir |= BOUNDARY_DIR_WEST; continue; }
      "E"  { dir |= BOUNDARY_DIR_EAST; continue; }
      "S"  { dir |= BOUNDARY_DIR_SOUTH; continue; }
      "N"  { dir |= BOUNDARY_DIR_NORTH; continue; }
      "B"  { dir |= BOUNDARY_DIR_BOTTOM; continue; }
      "T"  { dir |= BOUNDARY_DIR_TOP; continue; }
      "X"  { dir |= BOUNDARY_DIR_X; continue; }
      "Y"  { dir |= BOUNDARY_DIR_Y; continue; }
      "Z"  { dir |= BOUNDARY_DIR_Z; continue; }
     */
  }
  return dir;
}

surface_inlet_dir str_to_inlet_dir(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return SURFACE_INLET_DIR_INVALID; }
    "NORMAL"/kend { return SURFACE_INLET_DIR_NORMAL; }
   */
  CSVUNREACHABLE();
  return SURFACE_INLET_DIR_INVALID;
}

tm_func2_model str_to_tm_func2_model(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return TM_FUNC_INVALID; }
    "LIQUIDUS_FE_ZR"/kend  { return TM_FUNC_LIQUIDUS_FE_ZR; }
    "LIQUIDUS_FE_B"/kend   { return TM_FUNC_LIQUIDUS_FE_B; }
  */
  CSVUNREACHABLE();
  return TM_FUNC_INVALID;
}

dc_func2_model str_to_dc_func2_model(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return DC_FUNCS_TEMPDEP_PROPERTY; }
    "SUS304_B4C"/kend { return DC_FUNCS_SUS304_B4C; }
    "B4C_SUS304"/kend { return DC_FUNCS_B4C_SUS304; }
    "SUS304_ZIRCALOY"/kend { return DC_FUNCS_SUS304_ZIRCALOY; }
    "ZIRCALOY_SUS304"/kend { return DC_FUNCS_ZIRCALOY_SUS304; }
    "B4C_ZIRCALOY"/kend { return DC_FUNCS_B4C_ZIRCALOY; }
    "ZIRCALOY_B4C"/kend { return DC_FUNCS_ZIRCALOY_B4C; }
    "UO2_ZIRCALOY"/kend { return DC_FUNCS_UO2_ZIRCALOY; }
    "ZIRCALOY_UO2"/kend { return DC_FUNCS_ZIRCALOY_UO2; }
  */
  CSVUNREACHABLE();
  return DC_FUNCS_INVALID;
}

enum solid_form str_to_solid_form(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;
    * { return SOLID_FORM_INVALID; }
    "IBM"/kend { return SOLID_FORM_IBM; }
    "POROUS"/kend { return SOLID_FORM_POROUS; }
  */
  CSVUNREACHABLE();
  return SOLID_FORM_INVALID;
}

binary_output_mode str_to_binary_output_mode(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return BINARY_OUTPUT_INVALID; }
    "BYPROCESS"/kend     { return BINARY_OUTPUT_BYPROCESS; }
    "UNIFY""_MPI"?/kend  { return BINARY_OUTPUT_UNIFY_MPI; }
    "UNIFY_GATHER"/kend  { return BINARY_OUTPUT_UNIFY_GATHER; }
  */
  CSVUNREACHABLE();
  return BINARY_OUTPUT_INVALID;
}

tempdep_property_type str_to_tempdep_property_type(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return TEMPDEP_PROPERTY_INVALID; }
    "CONST"/kend { return TEMPDEP_PROPERTY_CONST; }
    "POLY_L"/kend { return TEMPDEP_PROPERTY_POLY_L; }
    "POLY"/kend { return TEMPDEP_PROPERTY_POLY; }
    "TABLE"/kend { return TEMPDEP_PROPERTY_TABLE; }
    "ARRHENIUS"/kend { return TEMPDEP_PROPERTY_ARRHENIUS; }
    "PIECEWISE"/kend { return TEMPDEP_PROPERTY_PIECEWISE; }
   */
  CSVUNREACHABLE();
  return TEMPDEP_PROPERTY_INVALID;
}

enum ox_reaction_rate_model str_to_ox_kp_model(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return OX_RRMODEL_INVALID; }
    ("UH"|"URBANIC""_HEIDRICK"?|"HEIDRICK")/kend {
      return OX_RRMODEL_URBANIC_HEIDRICK;
    }

    ("BJ"|"BAKER""_JUST"?|"JUST")/kend {
      return OX_RRMODEL_BAKER_JUST;
    }

    ("CP"|"CATHCART""_PAWEL"?|"PAWEL")/kend {
      return OX_RRMODEL_CATHCART_PAWEL;
    }

    ("LS"|"LEISTIKOW""_SCHANZ"?|"SCHANZ")/kend {
      return OX_RRMODEL_LEISTIKOW_SCHANZ;
    }

    ("PC"|"PRATER""_COURTRIGHT"?|"COURTRIGHT")/kend {
      return OX_RRMODEL_PRATER_COURTRIGHT;
    }

    ("AS_"?"MATERIAL")/kend {
      return OX_RRMODEL_TEMPDEP;
    }
  */
  CSVUNREACHABLE();
  return OX_RRMODEL_INVALID;
}

component_phase_name str_to_component_phase(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return COMPONENT_PHASE_MAX; }
    "SOLID"/kend  { return COMPONENT_PHASE_SOLID; }
    "LIQUID"/kend { return COMPONENT_PHASE_LIQUID; }
    "GAS"/kend    { return COMPONENT_PHASE_GAS; }
  */
  CSVUNREACHABLE();
  return -1;
}

#ifdef LPT
#define LPTPREFIX(F, X, N, C) F##C
#endif
#ifdef LPTX
#define LPTPREFIX(F, X, N, C) X##C
#endif
#ifndef LPTPREFIX
#define LPTPREFIX(F, X, N, C) N##C
#endif

#ifndef HAVE_LPT
enum NoLPTts
{
  NoLPT_TIME_SCHEME_INVALID = -1,
  NoLPT_TIME_SCHEME_ADAMS_BASHFORTH_2 = -1,
  NoLPT_TIME_SCHEME_RUNGE_KUTTA_2 = -1,
  NoLPT_TIME_SCHEME_RUNGE_KUTTA_3 = -1,
};
#endif

#define LPTts(X) LPTPREFIX(LPT_, LPTX_, NoLPT_, TIME_SCHEME_##X)
int str_to_LPTts(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return LPTts(INVALID); }
    "ADAMS_BASHFORTH_2"/kend { return LPTts(ADAMS_BASHFORTH_2); }
    "RUNGE_KUTTA_2"/kend { return LPTts(RUNGE_KUTTA_2); }
    "RUNGE_KUTTA_3"/kend { return LPTts(RUNGE_KUTTA_3); }
  */
  CSVUNREACHABLE();
  return LPTts(INVALID);
}

int LPTts_invalid(void)
{
  return LPTts(INVALID);
}
#undef LPTts

#if !defined(HAVE_LPT) || defined(LPT)
enum NoLPTht
{
  NoLPT_HEAT_INVALID = -1,
  NoLPT_HEAT_OFF = OFF, /* OFF is always valid */
  NoLPT_HEAT_FOLLOW_FLUID = -1,
  NoLPT_HEAT_RANZ_MARSHALL = -1,
};
#endif

#define LPTht(X) LPTPREFIX(NoLPT_, LPTX_, NoLPT_, HEAT_##X)
int str_to_LPTht(const char *p)
{

  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return LPTht(INVALID); }
    "OFF"/kend           { return LPTht(OFF); }
    "FOLLOW_FLUID"/kend  { return LPTht(FOLLOW_FLUID); }
    "RANZ_MARSHALL"/kend { return LPTht(RANZ_MARSHALL); }
  */
  CSVUNREACHABLE();
  return LPTht(INVALID);
}

int LPTht_invalid(void)
{
  return LPTht(INVALID);
}
#undef LPTht

int str_to_postp(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return JCNTRL_EXE_INVALID; }

    "CALCULATOR"/kend          { return JCNTRL_POSTP_CALCULATOR; }
    "ADD_VARIABLE"/kend        { return JCNTRL_POSTP_ADD_VARIABLE; }
    "DEL_VARIABLE"/kend        { return JCNTRL_POSTP_DEL_VARIABLE; }
    "DEL_VARIABLE_EXCEPT"/kend { return JCNTRL_POSTP_DEL_VARIABLE_EXCEPT; }
    "SUM"/kend                 { return JCNTRL_POSTP_SUM; }
    "SUM_X"/kend               { return JCNTRL_POSTP_SUM_X; }
    "SUM_Y"/kend               { return JCNTRL_POSTP_SUM_Y; }
    "SUM_Z"/kend               { return JCNTRL_POSTP_SUM_Z; }
    "NEIGHBOR_SUM"/kend        { return JCNTRL_POSTP_NEIGHBOR_SUM; }
    "NEIGHBOR_SUM_X"/kend      { return JCNTRL_POSTP_NEIGHBOR_SUM_X; }
    "NEIGHBOR_SUM_Y"/kend      { return JCNTRL_POSTP_NEIGHBOR_SUM_Y; }
    "NEIGHBOR_SUM_Z"/kend      { return JCNTRL_POSTP_NEIGHBOR_SUM_Z; }
    "AVERAGE"/kend             { return JCNTRL_POSTP_AVERAGE; }
    "AVERAGE_X"/kend           { return JCNTRL_POSTP_AVERAGE_X; }
    "AVERAGE_Y"/kend           { return JCNTRL_POSTP_AVERAGE_Y; }
    "AVERAGE_Z"/kend           { return JCNTRL_POSTP_AVERAGE_Z; }
    "AVERAGE_XY"/kend          { return JCNTRL_POSTP_AVERAGE_XY; }
    "AVERAGE_YZ"/kend          { return JCNTRL_POSTP_AVERAGE_YZ; }
    "AVERAGE_XZ"/kend          { return JCNTRL_POSTP_AVERAGE_XZ; }
    "AVERAGE_T"/kend           { return JCNTRL_POSTP_AVERAGE_T; }
    "NEIGHBOR_AVERAGE"/kend    { return JCNTRL_POSTP_NEIGHBOR_AVERAGE; }
    "NEIGHBOR_AVERAGE_X"/kend  { return JCNTRL_POSTP_NEIGHBOR_AVERAGE_X; }
    "NEIGHBOR_AVERAGE_Y"/kend  { return JCNTRL_POSTP_NEIGHBOR_AVERAGE_Y; }
    "NEIGHBOR_AVERAGE_Z"/kend  { return JCNTRL_POSTP_NEIGHBOR_AVERAGE_Z; }
    "NEIGHBOR_AVERAGE_XY"/kend { return JCNTRL_POSTP_NEIGHBOR_AVERAGE_XY; }
    "NEIGHBOR_AVERAGE_YZ"/kend { return JCNTRL_POSTP_NEIGHBOR_AVERAGE_YZ; }
    "NEIGHBOR_AVERAGE_XZ"/kend { return JCNTRL_POSTP_NEIGHBOR_AVERAGE_XZ; }
    "VORTICITY_XY"/kend        { return JCNTRL_POSTP_VORTICITY_XY; }
    "VORTICITY_YZ"/kend        { return JCNTRL_POSTP_VORTICITY_YZ; }
    "VORTICITY_XZ"/kend        { return JCNTRL_POSTP_VORTICITY_XZ; }
    "VORTICITY2"/kend          { return JCNTRL_POSTP_VORTICITY2; }
    "VOLUME_INTEGRAL"/kend     { return JCNTRL_POSTP_VOLUME_INTEGRAL; }
    "MASK"/kend                { return JCNTRL_POSTP_MASK; }
    "UNMASK"/kend              { return JCNTRL_POSTP_UNMASK; }
    "NEGATE_MASK"/kend         { return JCNTRL_POSTP_NEGATE_MASK; }
  */
  CSVUNREACHABLE();
  return JCNTRL_EXE_INVALID;
}

int str_to_maskp(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return JCNTRL_EXE_INVALID; }

    "GEOMETRY"/kend { return JCNTRL_MASK_GEOMETRY; }
    "GRID"/kend     { return JCNTRL_MASK_GRID; }
    "ADD"/kend      { return JCNTRL_MASK_ADD; }
    "OR"/kend       { return JCNTRL_MASK_OR; }
    "SUB"/kend      { return JCNTRL_MASK_SUB; }
    "MUL"/kend      { return JCNTRL_MASK_MUL; }
    "AND"/kend      { return JCNTRL_MASK_AND; }
    "XOR"/kend      { return JCNTRL_MASK_XOR; }
    "EQV"/kend      { return JCNTRL_MASK_EQV; }
    "NOR"/kend      { return JCNTRL_MASK_NOR; }
    "NAND"/kend     { return JCNTRL_MASK_NAND; }
    "XNOR"/kend     { return JCNTRL_MASK_XNOR; }
    "NEQV"/kend     { return JCNTRL_MASK_NEQV; }
    "POINT"/kend    { return JCNTRL_MASK_POINT; }
    "EXTENT"/kend   { return JCNTRL_MASK_EXTENT; }
    "GET"/kend      { return JCNTRL_MASK_GET; }

    "ALL"/kend      { return JCNTRL_MASK_ALL; }
    "NONE"/kend     { return JCNTRL_MASK_NONE; }
  */
  CSVUNREACHABLE();
  return JCNTRL_EXE_INVALID;
}

int str_to_fieldp(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return JCNTRL_EXE_INVALID; }
    "CALCULATOR"/kend { return JCNTRL_FV_CALCULATOR; }
    "GET"/kend        { return JCNTRL_FV_GET; }
    "COND"/kend       { return JCNTRL_FV_COND; }
    "ON_TRIGGER"/kend { return JCNTRL_FV_ON_TRIGGER; }
    "RAND"/kend       { return JCNTRL_FV_RAND; }
    "SUM"/kend        { return JCNTRL_FV_SUM; }
    "AVERAGE"/kend    { return JCNTRL_FV_AVERAGE; }
    "TABLE"/kend      { return JCNTRL_FV_TABLE; }
  */
  CSVUNREACHABLE();
  return JCNTRL_EXE_INVALID;
}

int str_to_fv_tabbnd(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return JCNTRL_FV_TABLE_EXTEND_INVALID; }
    "EXTRAPOLATE"/kend { return JCNTRL_FV_TABLE_EXTEND_EXTRAPOLATE; }
    "NEAREST"/kend     { return JCNTRL_FV_TABLE_EXTEND_NEAREST; }
    "CIRCULAR"/kend    { return JCNTRL_FV_TABLE_EXTEND_CIRCULAR; }
    "MIRROR"/kend      { return JCNTRL_FV_TABLE_EXTEND_MIRROR; }
  */
  CSVUNREACHABLE();
  return JCNTRL_FV_TABLE_EXTEND_INVALID;
}

int str_to_jcntrl_lop(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return JCNTRL_LOP_INVALID; }
    "SET"/kend  { return JCNTRL_LOP_SET; }
    "ADD"/kend  { return JCNTRL_LOP_ADD; }
    "OR"/kend   { return JCNTRL_LOP_OR; }
    "SUB"/kend  { return JCNTRL_LOP_SUB; }
    "MUL"/kend  { return JCNTRL_LOP_MUL; }
    "AND"/kend  { return JCNTRL_LOP_AND; }
    "EQV"/kend  { return JCNTRL_LOP_EQV; }
    "XOR"/kend  { return JCNTRL_LOP_XOR; }
    "NOR"/kend  { return JCNTRL_LOP_NOR; }
    "NAND"/kend { return JCNTRL_LOP_NAND; }
    "XNOR"/kend { return JCNTRL_LOP_XNOR; }
    "NEQV"/kend { return JCNTRL_LOP_NEQV; }
  */
  CSVUNREACHABLE();
  return JCNTRL_LOP_INVALID;
}

int str_to_jcntrl_compp(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return JCNTRL_COMP_INVALID; }
    "LESS"/kend       { return JCNTRL_COMP_LESS; }
    "LESS_EQ"/kend    { return JCNTRL_COMP_LESS_EQ; }
    "EQUAL"/kend      { return JCNTRL_COMP_EQUAL; }
    "NOT_EQ"/kend     { return JCNTRL_COMP_NOT_EQ; }
    "GREATER"/kend    { return JCNTRL_COMP_GREATER; }
    "GREATER_EQ"/kend { return JCNTRL_COMP_GREATER_EQ; }
  */
  CSVUNREACHABLE();
  return JCNTRL_COMP_INVALID;
}

int str_to_write_fv_csv_mode(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return JCNTRL_WRITE_FV_OUTPUT_BY_NONE; }
    "NSTEP"/kend { return JCNTRL_WRITE_FV_OUTPUT_BY_NSTEP; }
    "TIME"/kend  { return JCNTRL_WRITE_FV_OUTPUT_BY_TIME; }
  */
  CSVUNREACHABLE();
  return JCNTRL_WRITE_FV_OUTPUT_BY_NONE;
}

int str_to_write_fv_form(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return WRITE_FIELD_VARIABLES_FORMAT_INVALID; }
    "CSV"/kend { return WRITE_FIELD_VARIABLES_FORMAT_CSV; }
  */
  CSVUNREACHABLE();
  return WRITE_FIELD_VARIABLES_FORMAT_INVALID;
}

non_uniform_grid_function str_to_non_uniform_grid_func(const char *p)
{
  const char *m;
  /*!re2c
    re2c:indent:top = 1;

    * { return NON_UNIFORM_GRID_FUNC_INVALID; }
    "CONST"/kend            { return NON_UNIFORM_GRID_FUNC_CONST; }
    "CONST_RATIO_INC"/kend  { return NON_UNIFORM_GRID_FUNC_CONST_RATIO_INC; }
    "CONST_RATIO_DEC"/kend  { return NON_UNIFORM_GRID_FUNC_CONST_RATIO_DEC; }
    "SINE"/kend             { return NON_UNIFORM_GRID_FUNC_SINE; }
    "QSINE_B"/kend          { return NON_UNIFORM_GRID_FUNC_QSINE_B; }
    "QSINE_E"/kend          { return NON_UNIFORM_GRID_FUNC_QSINE_E; }
  */
  CSVUNREACHABLE();
  return NON_UNIFORM_GRID_FUNC_INVALID;
}

/*!re2c
  // Remaining part of matching should be case sensitive
  re2c:flags:case-insensitive = 0;
*/
jupiter_geom_key set_geom_get_keyname(const char *text)
{
  struct sp {
    const char *p;
    const char *m;
  } p;

  p.p = text;

  /*!re2c
    re2c:define:YYCTYPE = "unsigned char";
    re2c:define:YYCURSOR = "p.p";
    re2c:define:YYMARKER = "p.m";
    re2c:yyfill:enable = 0;
    re2c:indent:top = 1;

    *                      { return JUPITER_GEOM_KEY_INVALID; }
    "Geom_file"/end        { return JUPITER_GEOM_KEY_FILE; }
    "Geom_name"/end        { return JUPITER_GEOM_KEY_NAME; }
    "Geom_size"/end        { return JUPITER_GEOM_KEY_SIZE; }
    "Geom_repeat"/end      { return JUPITER_GEOM_KEY_REPEAT; }
    "Geom_offset"/end      { return JUPITER_GEOM_KEY_OFFSET; }
    "Geom_origin"/end      { return JUPITER_GEOM_KEY_ORIGIN; }
    "Geom_boundary"/end    { return JUPITER_GEOM_KEY_BOUNDARY; }
    "Geom_tboundary"/end   { return JUPITER_GEOM_KEY_TBOUNDARY; }
    "Geom_surface_boundary"/end { return JUPITER_GEOM_KEY_SURFACE_BOUNDARY; }
    "Geom_shape"/end       { return JUPITER_GEOM_KEY_SHAPE; }
    "Geom_surface_shape"/end { return JUPITER_GEOM_KEY_SURFACE_SHAPE; }
    "Geom_num_subcell"/end { return JUPITER_GEOM_KEY_NUM_SUBCELL; }
    "Geom_vof"/end         { return JUPITER_GEOM_KEY_VOF; }
    "Geom_velocity_u"/end  { return JUPITER_GEOM_KEY_VELOCITY_U; }
    "Geom_velocity_v"/end  { return JUPITER_GEOM_KEY_VELOCITY_V; }
    "Geom_velocity_w"/end  { return JUPITER_GEOM_KEY_VELOCITY_W; }
    "Geom_velocity"/end    { return JUPITER_GEOM_KEY_VELOCITY; }
    "Geom_temperature"/end { return JUPITER_GEOM_KEY_TEMPERATURE; }
    "Geom_pressure"/end    { return JUPITER_GEOM_KEY_PRESSURE; }
    "Geom_fixed_heat_source"/kend { return JUPITER_GEOM_KEY_FIXED_HSOURCE; }
    "Geom_LPT_boundary_wall_reflection"/kend
       { return JUPITER_GEOM_KEY_LPT_WALLREF_BN; }
    "Geom_LPT_wall_reflection_n"/kend
       { return JUPITER_GEOM_KEY_LPT_WALLREF_IN; }
    "Geom_dump"/end        { return JUPITER_GEOM_KEY_DUMP; }
  */
  CSVUNREACHABLE();
  return JUPITER_GEOM_KEY_INVALID;
}
