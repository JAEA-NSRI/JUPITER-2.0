#include "component_data_defs.h"
#include "component_info.h"
#include "component_info_defs.h"
#include "csvutil_extra.h"
#include "geometry/bitarrayv.h"
#include "geometry/defs.h"
#include "heat_source.h"
#include "lpt.h"
#include "print_param_keywords.h"

#include <limits.h>
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <inttypes.h>
#include <errno.h>
#include <time.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#include "component_info.h"
#include "control/defs.h"
#include "component_info.h"
#include "field_control.h"
#include "geometry/list.h"
#include "struct.h"
#include "init_component.h"
#include "component_data.h"
#include "non_uniform_grid.h"

/* YSE: Use CSV functions and utility */
#include "csv.h"
#include "csvutil.h"
#include "csvtmpl_format.h"
#include "optparse.h"
#include "common_util.h"
#include "os/asprintf.h"

/* YSE: Definitions for tm_table_param structure */
#include "tmcalc.h"

/* YSE: Definitions for dc_funcs2_id_list/param structure */
#include "dccalc.h"

/* YSE: Boundary utility */
#include "boundary_util.h"

/* YSE: Temperature dependent property data */
#include "tempdep_properties.h"

/* YSE: JUPITER-defined geometry shapes and initializations */
#include "if_binary.h"

/* Oxidation parameter setter */
#include "oxidation.h"

/* LPT */
#ifdef LPT
#include "lpt/LPTbnd.h"
#endif

#ifdef LPTX
#include "lptx/defs.h"
#include "lptx/init_set.h"
#include "lptx/param.h"
#include "lptx/ptflags.h"
#include "lptx/vector.h"
#endif

/* field variable utility */
#include "field_control.h"
#include "update_level_set_flags.h"

void aluminium(phase_value *f);
void water(phase_value *f);
void Fe(phase_value *f);
void UO2(phase_value *f);
void SUS(phase_value *f);

/* YSE: Heat source parameter setter */
static void set_heat_source_param(domain *cdo, csv_data *csv, const char *fname,
                                  component_data *comp_data_head,
                                  jcntrl_executive_manager *manager,
                                  controllable_type *control_head, int *stat);

/* YSE: Pass csv data and its filename */
void set_property(int i, phase_value_component *f, flags *flg,
                  csv_data *csv, const char *fname, int *stat);

/* YSE: Set physical properties for Gaseous-only phase */
void set_property_g(int i, phase_value_component *f, flags *flg, csv_data *csv,
                    const char *fname, int *stat);

/* YSE: Set physical properties for Gaseous-only phase of ID == -1 */
void set_property_gm1(phase_value *phv, flags *flg, csv_data *csv,
                      const char *fname, int *stat);

struct fname_template_metadata {
  const char *default_name;
  enum embed_requirement {
    required, optional, warn_if_not, warn_if_present,
  } compname, compid, rank, chrono_idx;
};

struct filename_template_set {
  const char *output;
  const char *restart;
  struct fname_template_metadata output_meta;
  struct fname_template_metadata restart_meta;
};

static int
fname_check_warn(const char *csv_fname,
                 csv_row *found_row, csv_column *found_col,
                 const char *fmt, const char *title,
                 enum embed_requirement req, int ret_format)
{
  int ret;
  SET_P_INIT(NULL, csv_fname, &found_row, &found_col);

  ret = 0;
  if (ret_format < 0) { /* Present */
    if (req == warn_if_present) {
      SET_P_PERROR(WARN, "%s is included", title);
    }
  } else {
    if (req == required) {
      SET_P_PERROR(ERROR, "%s must be included", title);
      ret = 1;
    } else if (req == warn_if_not) {
      SET_P_PERROR(WARN, "%s should be included", title);
    }
  }
  return ret;
}

static int
fname_template_check(const char *csv_fname,
                     csv_row *found_row, csv_column *found_col,
                     const char *fmt,
                     const struct fname_template_metadata *data)
{
  int r;
  int t;
  int ret;
  SET_P_INIT(NULL, csv_fname, &found_row, &found_col);

  ret = 0;
  r = format_integers(NULL, fmt, "irn[s]c", 0, 0, 0, "");
  if (r < 0) {
    SET_P_PERROR(ERROR, "Invalid template format");
    ret = 1;
    return ret;
  }

  r = format_integers(NULL, fmt, "irn", 0, 0, 0);
  t = fname_check_warn(csv_fname, found_row, found_col, fmt,
                       "Component name (%c)", data->compname, r);
  if (t) { ret = 1; }

  r = format_integers(NULL, fmt, "ir[s]c", 0, 0, "");
  t = fname_check_warn(csv_fname, found_row, found_col, fmt,
                       "Chronological index (%n)", data->chrono_idx, r);
  if (t) { ret = 1; }

  r = format_integers(NULL, fmt, "in[s]c", 0, 0, "");
  t = fname_check_warn(csv_fname, found_row, found_col, fmt,
                       "Rank number (%r)", data->rank, r);
  if (t) { ret = 1; }

  r = format_integers(NULL, fmt, "rn[s]c", 0, 0, "");
  t = fname_check_warn(csv_fname, found_row, found_col, fmt,
                       "Material ID (%i)", data->compid, r);
  if (t) { ret = 1; }

  return ret;
}

static void
set_fname_template(struct filename_template_data *p, int binary,
                   csv_data *csv, const char *fname, const char *key,
                   const struct fname_template_metadata *time,
                   const struct fname_template_metadata *comp_based,
                   const struct fname_template_metadata *others,
                   int *stat)
{
  int r;
  csv_row *found_row;
  csv_column *found_col;
  SET_P_INIT(csv, fname, &found_row, &found_col);

  if (binary) {
    SET_P(&p->time, charp, key, 1, NULL);
    if (p->time) {
      r = fname_template_check(fname, found_row, found_col, p->time, time);
      if (r && stat) *stat = ON;
    }

    SET_P_NEXT(&p->comp_based, charp, NULL);
    if (p->comp_based) {
      r = fname_template_check(fname, found_row, found_col, p->comp_based,
                               comp_based);
      if (r && stat) *stat = ON;
    }

    SET_P_NEXT(&p->others, charp, NULL);
    if (p->others) {
      r = fname_template_check(fname, found_row, found_col, p->others, others);
      if (r && stat) *stat = ON;
    }

    if (!p->time) {
      p->time = jupiter_strdup(time->default_name);
    }
    if (!p->comp_based) {
      p->comp_based = jupiter_strdup(comp_based->default_name);
    }
    if (!p->others) {
      p->others = jupiter_strdup(others->default_name);
    }
    if (!p->time || !p->comp_based || !p->others) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM,
                0, 0, NULL);
      if (stat) *stat = ON;
    }
  } else {
    p->time = NULL;
    p->comp_based = NULL;
    p->others = NULL;
  }
}

static int
get_whether_output_lpt_ctrl(struct data_output_spec *spec)
{
  if (spec->lpt_ptid.outf == ON) return ON;
  if (spec->lpt_oid.outf == ON) return ON;
  if (spec->lpt_xpt.outf == ON) return ON;
  if (spec->lpt_ypt.outf == ON) return ON;
  if (spec->lpt_zpt.outf == ON) return ON;
  if (spec->lpt_pospt.outf == ON) return ON;
  if (spec->lpt_uxpt.outf == ON) return ON;
  if (spec->lpt_uypt.outf == ON) return ON;
  if (spec->lpt_uzpt.outf == ON) return ON;
  if (spec->lpt_upt.outf == ON) return ON;
  if (spec->lpt_uf.outf == ON) return ON;
  if (spec->lpt_muf.outf == ON) return ON;
  if (spec->lpt_densf.outf == ON) return ON;
  if (spec->lpt_cpf.outf == ON) return ON;
  if (spec->lpt_thcf.outf == ON) return ON;
  if (spec->lpt_tempf.outf == ON) return ON;
  if (spec->lpt_gradTf.outf == ON) return ON;
  if (spec->lpt_pathf.outf == ON) return ON;
  if (spec->lpt_mwf.outf == ON) return ON;
  if (spec->lpt_fuxpt.outf == ON) return ON;
  if (spec->lpt_fuypt.outf == ON) return ON;
  if (spec->lpt_fuzpt.outf == ON) return ON;
  if (spec->lpt_fupt.outf == ON) return ON;
  if (spec->lpt_fduxt.outf == ON) return ON;
  if (spec->lpt_fduyt.outf == ON) return ON;
  if (spec->lpt_fduzt.outf == ON) return ON;
  if (spec->lpt_dTdt.outf == ON) return ON;
  if (spec->lpt_fdt.outf == ON) return ON;
  if (spec->lpt_timpt.outf == ON) return ON;
  if (spec->lpt_denspt.outf == ON) return ON;
  if (spec->lpt_cppt.outf == ON) return ON;
  if (spec->lpt_thcpt.outf == ON) return ON;
  if (spec->lpt_diapt.outf == ON) return ON;
  if (spec->lpt_temppt.outf == ON) return ON;
  if (spec->lpt_htrpt.outf == ON) return ON;
  if (spec->lpt_tothtpt.outf == ON) return ON;
  if (spec->lpt_inipospt.outf == ON) return ON;
  if (spec->lpt_iniupt.outf == ON) return ON;
  if (spec->lpt_initemppt.outf == ON) return ON;
  if (spec->lpt_exit.outf == ON) return ON;
  if (spec->lpt_flags.outf == ON) return ON;
  if (spec->lpt_parceln.outf == ON) return ON;
  if (spec->lpt_fbpt.outf == ON) return ON;
  if (spec->lpt_fTpt.outf == ON) return ON;
  if (spec->lpt_seed.outf == ON) return ON;
  if (spec->lpt_Y.outf == ON) return ON;
  return OFF;
}

static void
set_output_spec(struct data_spec *spec,
                int binary, csv_data *csv, const char *fname,
                const char *key, csv_row **row, csv_column **col, int read,
                const char *comp_name, int default_flg, const char *tmpl)
{
  SET_P_INIT(csv, fname, row, col);

  if (read) {
    if (key) {
      SET_P(&spec->outf, bool, key, 1, default_flg);
    } else {
      SET_P_NEXT(&spec->outf, bool, default_flg);
    }
  } else {
    spec->outf = default_flg;
  }
  spec->filename_template = tmpl;
  spec->name = comp_name;
}

static void
set_output_spec_b(struct data_spec *output_spec,
                  struct data_spec *restart_spec,
                  int binary, csv_data *csv, const char *fname,
                  csv_row **found_row, csv_column **found_col,
                  const char *name, const char *key,
                  int default_out, int default_restart,
                  int read_for_out, int read_for_restart,
                  const struct filename_template_set *set)
{
  set_output_spec(output_spec, binary, csv, fname, key, found_row, found_col,
                  read_for_out, name, default_out, set->output);
  set_output_spec(restart_spec, binary, csv, fname,
                  ((!read_for_out && read_for_restart) ? key : NULL),
                  found_row, found_col, read_for_restart, name,
                  default_restart, set->restart);
}

#define SET_OUTPUT_SPEC(name, def_out, def_restart, read_out, read_restart, \
                        key, template_set)                                  \
  set_output_spec_b(&flg->output_data.name, &flg->restart_data.name,        \
                    flg->binary, SET_P_SOURCE_VARNAME.csv,                  \
                    SET_P_SOURCE_VARNAME.fname,                             \
                    SET_P_SOURCE_VARNAME.found_row,                         \
                    SET_P_SOURCE_VARNAME.found_col, #name, key, def_out,    \
                    def_restart, read_out, read_restart, &(template_set))

/**
 * Allow to switch output both for restart and step
 */
#define SET_OUTPUT_SPEC_1(name, def_out, restart_out, key, template_set) \
  SET_OUTPUT_SPEC(name, def_out, restart_out, 1, 1, key, template_set)

/**
 * Always output for restart data, allowing to switch output for step
 */
#define SET_OUTPUT_SPEC_R(name, def_out, key, template_set) \
  SET_OUTPUT_SPEC(name, def_out, ON, 1, 0, key, template_set)

/**
 * Always no output
 */
#define SET_OUTPUT_SPEC_0(name, key, template_set) \
  SET_OUTPUT_SPEC(name, OFF, OFF, 0, 0, key, template_set)

/**
 * Output only if model_flag yields true.
 */
#define SET_OUTPUT_SPEC_M(model_flag, name, def_out, def_restart, read_out, \
                          read_restart, key, template_set)                  \
  ((model_flag) ? SET_OUTPUT_SPEC(name, def_out, def_restart, read_out,     \
                                  read_restart, key, template_set)          \
                : SET_OUTPUT_SPEC_0(name, key, template_set))

/**
 * Output only if model_flag yields true. Allow to switch output both for
 * restart and step
 */
#define SET_OUTPUT_SPEC_M1(model_flag, name, def_out, restart_out, key, \
                           template_set)                                \
  ((model_flag)                                                         \
     ? SET_OUTPUT_SPEC_1(name, def_out, restart_out, key, template_set) \
     : SET_OUTPUT_SPEC_0(name, key, template_set))

/**
 * Output only if model_flag yields true. Otherwise, always output for restart
 * data.
 */
#define SET_OUTPUT_SPEC_MR(model_flag, name, def_out, key, template_set) \
  ((model_flag) ? SET_OUTPUT_SPEC_R(name, def_out, key, template_set)    \
                : SET_OUTPUT_SPEC_0(name, key, template_set))

struct update_level_set_reason_printer_data_type
{
  flags *flg;
};

static struct update_level_set_reason_printer_data_type
update_level_set_reason_printer_data = { .flg = NULL  };

static void
update_level_set_reason_printer(void *data,
                                const update_level_set_flags *flags,
                                update_level_set_reason reason)
{
  const char *varname = "(unknown)";

  struct update_level_set_reason_printer_data_type *ud;
  ud = (struct update_level_set_reason_printer_data_type *)data;
  if (ud->flg) {
    if (flags == &ud->flg->update_level_set_ll) {
      varname = "ll";
    } else if (flags == &ud->flg->update_level_set_lls) {
      varname = "lls";
    } else if (flags == &ud->flg->update_level_set_ls) {
      varname = "ls";
    }
  }

  csvperrorf(NULL, 0, 0, CSV_EL_INFO, varname,
             "Updating level set function: %s",
             update_level_set_flags_get_reason_str(reason));
}

static void set_update_level_set_flags(update_level_set_flags *flags,
                                       const char *fname, csv_data *csv,
                                       const char *keyname, csv_row **start_row,
                                       csv_column **start_col, int *status,
                                       int describe_reason)
{
  csv_row *row;
  csv_column *col;
  SET_P_INIT(csv, fname, (start_row ? start_row : &row),
             (start_col ? start_col : &col));

  CSVASSERT(flags);

  /* Initially set to OFF and set to ON where required. */
  update_level_set_flags_init(flags, describe_reason);

  if (keyname) {
    SET_P(&flags->force_update, bool, keyname, 1, OFF);
  } else if (start_col || start_row) {
    CSVASSERT(start_col);
    CSVASSERT(start_row);

    SET_P_CURRENT(&flags->force_update, bool, OFF);
  }

  if (flags->force_update == ON) {
    update_level_set_flags_mark_update(flags, UPDATE_LEVEL_SET_BY_INPUT);
  }
}

/* YSE: Pass csv data for flags and argument data,
   return status of result. */
void set_flags(flags *flg,
               const char *flags_fname, csv_data *flags_csv,
               const char *param_fname, csv_data *param_csv,
               jupiter_options *exec_argument, int *stat)
{
  /* YSE: Use flags.txt for SET_P* macros */
  SET_P_INIT_NOLOC(flags_csv, flags_fname);

  /* YSE: Test arguments */
  CSVASSERT(flg);
  CSVASSERT(exec_argument);
  CSVASSERT(stat);

  /* YSE: Init command line parameters */
  flg->restart = -1;
  if (exec_argument->restart_job && exec_argument->restart > 0) {
    csvperrorf("<command line>", 0, 0, CSV_EL_ERROR, NULL,
               "Only one of -restart or -restart_job can be specifiable");
    *stat = ON;
  } else {
    if (exec_argument->restart_job) {
      flg->restart = 0;
    } else if (exec_argument->restart > 0) {
      flg->restart = exec_argument->restart;
    }
  }
  if (exec_argument->post_s >= 0) {
    if (flg->restart >= 0) {
      csvperrorf("<command line>", 0, 0, CSV_EL_WARN, NULL,
                 "For -post mode, -restart and -restart_job will be ignored");
    }
    flg->restart = -1;
    flg->post_s = exec_argument->post_s;
    flg->post_e = exec_argument->post_e;
  } else {
    flg->post_s = -1;
    flg->post_e = -1;
  }

  /* Set variable or constant delta */
#define NON_UNIFORM_GRID_X_KEYNAME "variable_delta_x"
#define NON_UNIFORM_GRID_Y_KEYNAME "variable_delta_y"
#define NON_UNIFORM_GRID_Z_KEYNAME "variable_delta_z"
  if (param_csv) {
    csv_row *row_x, *row_y, *row_z;
    row_x = findCSVRow(param_csv, NON_UNIFORM_GRID_X_KEYNAME,
                       strlen(NON_UNIFORM_GRID_X_KEYNAME));
    row_y = findCSVRow(param_csv, NON_UNIFORM_GRID_Y_KEYNAME,
                       strlen(NON_UNIFORM_GRID_Y_KEYNAME));
    row_z = findCSVRow(param_csv, NON_UNIFORM_GRID_Z_KEYNAME,
                       strlen(NON_UNIFORM_GRID_Z_KEYNAME));
    if (row_x || row_y || row_z) {
      flg->has_non_uniform_grid = ON;
    } else {
      flg->has_non_uniform_grid = OFF;
    }
  } else {
    flg->has_non_uniform_grid = OFF;
  }

  flg->print =  ON; // printf flag [ON or OFF]
  flg->debug =  OFF; // debug  flag [ON or OFF]
  //  flg->fp = fopen("info.txt","w"); // "= stdout;" or "= fopen(,);"
  flg->fp = stdout; // "= stdout;" or "= fopen(,);"
  flg->geom_in = ON; // geometry input [ON or OFF]
  //--- MPI
  /*************** DEBUG OFF *********************************************************/
  /* YSE: Removes all debug == ON part while input process. */
  /* YSE: Set parameters using SET_P* macros */

  /* YSE: Default values should be discussed (last argument of macro) */
  SET_P(&flg->print, bool, "print", 1, ON);
  /* YSE: Currently disabled */
  // SET_P(flg->debug, bool, "debug", 1, OFF);
  SET_P(&flg->geom_in, bool, "geom_in", 1, -1);
  /* YSE: Set of flg->list_fp is moved to set_input_list_fp. */

  /*
   * YSE: Setting of MPI_processes is moved to below
   *      (because it depends to radiation flag)
   */

  //--- Physical model
  SET_P(&flg->fluid_dynamics, bool, "fluid_dynamics", 1, -1);
  SET_P(&flg->heat_eq, bool, "heat_eq", 1, -1);
  SET_P(&flg->phase_change, bool, "phase_change", 1, -1);
  SET_P(&flg->melting, bool, "melting", 1, -1);
  SET_P(&flg->solidification, bool, "solidification", 1, -1);
  SET_P(&flg->vaporization, bool, "vaporization", 1, -1);
  SET_P(&flg->condensation, bool, "condensation", 1, -1);
  SET_P(&flg->surface_tension, bool, "surface_tension", 1, -1);
  SET_P(&flg->radiation, bool, "radiation", 1, -1);  // Radiation model < 2016 Added by KKE
  if (flg->radiation == ON && flg->has_non_uniform_grid == ON) {
    SET_P_PERROR(ERROR, "Radiation model does not support non-uniform grid");
    if (stat)
      *stat = ON;
  }
  SET_P(&flg->laser, bool, "laser", 1, -1); // Laser irradiation [ON, OFF]
  if (flg->laser == ON && flg->has_non_uniform_grid == ON) {
    SET_P_PERROR(ERROR, "Laser irradiation does not support non-uniform grid");
    if (stat)
      *stat = ON;
  }
  SET_P(&flg->wettability, bool, "wettability", 1, -1); // contact angle [ON, OFF]
#ifndef JUPITER_NOMETI
  SET_P(&flg->oxidation, bool, "oxidation", 1, -1);
#else
  SET_P_PASS_NOTFOUND(&flg->oxidation, bool, "oxidation", 1, OFF);
  if (flg->oxidation == ON) {
    SET_P_PERROR(ERROR, "oxidation is disabled on this JUPITER");
    if (stat) *stat = ON;
  }
  flg->oxidation = OFF;
#endif
  SET_P(&flg->temperature_dependency, bool, "temperature_dependency", 1, OFF);
  //<= works only Iron
  SET_P(&flg->vof_adv_fls, bool, "vof_adv_fls", 1, -1); //type of vof advection
  if (flg->vof_adv_fls == ON && flg->fluid_dynamics == ON &&
      flg->has_non_uniform_grid == ON) {
    SET_P_PERROR(WARN, "THINC (and WLIC) does not support variable delta");
  }

  SET_P(&flg->multi_layer_no_coalescence, bool, "multi_layer_no_coalescence", 1, OFF); //multi_layer_no_coalescence
  SET_P(&flg->multi_layer_less_coalescence, bool, "multi_layer_less_coalescence", 1, OFF); //multi_layer_less_coalescence

  if(flg->multi_layer_no_coalescence==ON && flg->multi_layer_less_coalescence==ON){
    SET_P_PERROR(ERROR, "Models of multi_layer_no_coalescence and multi_layer_less_coalescence are imcompatible. Select one of them");
  }

  if(flg->multi_layer_no_coalescence==ON || flg->multi_layer_less_coalescence==ON) flg->multi_layer=ON; // multi_layer

  if (flg->multi_layer == ON && flg->phase_change == ON) {
    SET_P_PERROR(ERROR, "multi_layer model does not support phase_change");
  }

  SET_P(&flg->film_drainage, bool, "film_drainage", 1, OFF); // film_drainage

  if (flg->film_drainage == ON && flg->multi_layer == OFF) {
    SET_P_PERROR(ERROR, "film drainage model works only when multi-layer models used");
  }

#ifndef JUPITER_NOMETI
  SET_P(&flg->solute_diff, bool, "solute_diff", 1, -1); //solute diffusion
  if (flg->solute_diff == ON && (flg->vof_adv_fls == ON || flg->multi_layer == ON)) {
    SET_P_PERROR(ERROR, "Only single-component VOF advection calculation (i.e., vof_adv_fls == OFF & multi_layer == OFF) is available when solute diffusion is enabled");
    if (stat) *stat = ON;
  }
  if (flg->solute_diff == ON) {
    if (flg->porous == ON) {
      SET_P_PERROR(ERROR, "solute diffusion does not support porous media");
      if (stat)
        *stat = ON;
    }
    if (flg->fluid_dynamics == ON && flg->has_non_uniform_grid == ON) {
      SET_P_PERROR(WARN, "solute diffusion cannot be used with variable "
                         "delta, because THINC is always applied for solute "
                         "diffusion, which does not support variable delta");
    }
  }
  SET_P(&flg->eutectic, bool, "eutectic", 1, OFF); // Eutectic

#else
  SET_P_PASS_NOTFOUND(&flg->solute_diff, bool, "solute_diff", 1, OFF);
  if (flg->solute_diff == ON) {
    SET_P_PERROR(ERROR, "solute_diff is disabled on this JUPITER");
    if (stat) *stat = ON;
  }
  SET_P_PASS_NOTFOUND(&flg->eutectic, bool, "eutectic", 1, OFF);
  if (flg->eutectic == ON) {
    SET_P_PERROR(ERROR, "eutectic is disabled on this JUPITER");
    if (stat) *stat = ON;
  }

  flg->solute_diff = OFF;
  flg->eutectic = OFF;
#endif

  //--- Numerical model
  SET_P(&flg->WENO, bool, "WENO", 1, OFF);
  if (flg->WENO == ON && flg->has_non_uniform_grid == ON) {
    SET_P_PERROR(ERROR, "WENO does not support variable delta yet");
    if (stat)
      *stat = ON;
  }
  SET_P(&flg->heat_tvd3, bool, "heat_tvd3", 1, -1);
  SET_P(&flg->visc_tvd3, bool, "visc_tvd3", 1, -1);
  SET_P(&flg->interface_capturing_scheme, interface_capturing_scheme, "interface_capturing_scheme", 1, -1); // [PLIC, THINC, THINC_WLIC or THINC_AWLIC]
  if (flg->interface_capturing_scheme != THINC && flg->has_non_uniform_grid == ON) {
    SET_P_PERROR(WARN, "variable delta only supports THINC");
  }
  if (flg->interface_capturing_scheme != PLIC && flg->multi_layer == ON) {
    SET_P_PERROR(WARN, "multi_layer only supports PLIC");
  }
  
  SET_P(&flg->PHASEFIELD, bool, "phasefield", 1, -1);
  if (flg->PHASEFIELD == ON && flg->multi_layer == ON) {
    SET_P_PERROR(ERROR, "Only normal VOF advection calculation (multi_layer == OFF) is available when phase_field is enabled");
    if (stat) *stat = ON;
  }
  if (flg->PHASEFIELD == ON && flg->has_non_uniform_grid == ON) {
    SET_P_PERROR(ERROR, "phasefield does not support variable delta yet");
    if (stat)
      *stat = ON;
  }
  SET_P(&flg->IBM, bool, "IBM", 1, OFF); // using Immersed Boundary Method
  SET_P(&flg->porous, bool, "porous", 1, OFF); // using Darcy
  SET_P(&flg->two_energy, bool, "two_energy", 1, OFF); // using two energy model

  //--- Boundary condition
  SET_P(&flg->bc_xm, boundary, "bc_xm", 1, -1); // x- : [WALL, SLIP or OUT]
  SET_P(&flg->bc_xp, boundary, "bc_xp", 1, -1); // x+ : [WALL, SLIP or OUT]
  SET_P(&flg->bc_ym, boundary, "bc_ym", 1, -1); // y- : [WALL, SLIP or OUT]
  SET_P(&flg->bc_yp, boundary, "bc_yp", 1, -1); // y+ : [WALL, SLIP or OUT]
  SET_P(&flg->bc_zm, boundary, "bc_zm", 1, -1); // z- : [WALL, SLIP or OUT]
  SET_P(&flg->bc_zp, boundary, "bc_zp", 1, -1); // z+ : [WALL, SLIP or OUT]
  SET_P(&flg->bct_xm, tboundary, "bct_xm", 1, -1);  // x- [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)]
  SET_P(&flg->bct_xp, tboundary, "bct_xp", 1, -1);  // x+ [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)]
  SET_P(&flg->bct_ym, tboundary, "bct_ym", 1, -1);  // y- [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)]
  SET_P(&flg->bct_yp, tboundary, "bct_yp", 1, -1);  // y+ [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)]
  SET_P(&flg->bct_zm, tboundary, "bct_zm", 1, -1);  // z- [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)]
  SET_P(&flg->bct_zp, tboundary, "bct_zp", 1, -1);  // z+ [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)]
  {
    int radiation_boundary_default;
    if (flg->radiation == ON) {
      radiation_boundary_default = -1;
    } else {
      radiation_boundary_default = INSULATION;
    }
    SET_P(&flg->bcrad_xm, tboundary, "bcrad_xm", 1, radiation_boundary_default);  // x- [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)] < 2016 Added by KKE
    SET_P(&flg->bcrad_xp, tboundary, "bcrad_xp", 1, radiation_boundary_default);  // x+ [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)] < 2016 Added by KKE
    SET_P(&flg->bcrad_ym, tboundary, "bcrad_ym", 1, radiation_boundary_default);  // y- [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)] < 2016 Added by KKE
    SET_P(&flg->bcrad_yp, tboundary, "bcrad_yp", 1, radiation_boundary_default);  // y+ [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)] < 2016 Added by KKE
    SET_P(&flg->bcrad_zm, tboundary, "bcrad_zm", 1, radiation_boundary_default);  // z- [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)] < 2016 Added by KKE
    SET_P(&flg->bcrad_zp, tboundary, "bcrad_zp", 1, radiation_boundary_default);  // z+ [INSULATION(11), ISOTHERMAL(13) or DIFFUSION(17)] < 2016 Added by KKE
  }

#ifndef JUPITER_NOMETI
  //--- Oxidation
  SET_P(&flg->ox_kp_model, ox_kp_model, "oxide_kp_model", 1,
        OX_RRMODEL_URBANIC_HEIDRICK);
  //--- H2 Absorption
  SET_P(&flg->h2_absorp_eval, bool, "h2_absorp_eval", 1, OFF);
  if (flg->h2_absorp_eval == ON) {
    if (flg->oxidation != ON) {
      SET_P_PERROR(ERROR, "H2 Absorption model requires oxidation to be "
                   "enabled. It uses the component ID informations given for "
                   "the oxidation model.");
      if (stat) *stat = ON;
    }
  }

  SET_P(&flg->h2_absorp_eval_p_change, bool, "h2_absorp_eval_p_change", 1, OFF);
#else
  SET_P(&flg->h2_absorp_eval, bool, "h2_absorp_eval", 1, OFF);
  if (flg->h2_absorp_eval == ON) {
    SET_P_PERROR(ERROR, "H2 Absorption model is not enabled for this JUPITER");
    if (stat) *stat = ON;
  }
#endif

  //--- LPT
#ifdef HAVE_LPT
  SET_P(&flg->lpt_calc, bool, "LPT_calc", 1, -1);
#else
  SET_P_PASS_NOTFOUND(&flg->lpt_calc, bool, "LPT_calc", 1, OFF);
#endif
#ifndef HAVE_LPT
  if (flg->lpt_calc == ON) {
    SET_P_PERROR(ERROR, "This JUPITER has not been compiled with LPT feature");
    if (stat) *stat = ON;
  }
#endif

  {
    int defval = -1;
    int req = 0;
#ifdef HAVE_LPT
    req = (flg->lpt_calc == ON);
#ifdef LPT
    defval = LPT_TIME_SCHEME_ADAMS_BASHFORTH_2;
#endif
#ifdef LPTX
    defval = LPTX_TIME_SCHEME_ADAMS_BASHFORTH_2;
#endif
#endif
    SET_P_BASE(&flg->lpt_ipttim, LPTts, "LPT_time_integration_scheme", 1,
               defval, req, 1, CSV_EL_WARN, CSV_EL_ERROR, NULL);
  }

  {
    int defval = OFF;
    int req = 0;
#ifdef LPTX
    req = (flg->lpt_calc == ON);
    defval = LPTX_HEAT_OFF;
#endif
    SET_P_BASE(&flg->lpt_heat, LPTht, "LPT_heat_exchange", 1, defval, req,
               1, CSV_EL_WARN, CSV_EL_ERROR, NULL);
#ifdef LPT
    if (flg->lpt_calc == ON && flg->lpt_heat != OFF) {
      SET_P_PERROR(ERROR, "LPT module does not support heat exchange");
      if (stat)
        *stat = ON;
    }
#endif
  }

#ifdef HAVE_LPT
  if (flg->lpt_calc == ON) {
    SET_P(&flg->lpt_wbcal, bool, "LPT_wbcal", 1, ON);
  } else {
    SET_P_PASS_NOTFOUND(&flg->lpt_wbcal, bool, "LPT_wbcal", 1, ON);
  }
#else
  SET_P_PASS_NOTFOUND(&flg->lpt_wbcal, bool, "LPT_wbcal", 1, ON);
#endif

#ifdef LPTX
  if (flg->lpt_calc == ON) {
    SET_P(&flg->lpt_brownian, bool, "LPT_brownian_force", 1, OFF);
  } else {
    SET_P_PASS_NOTFOUND(&flg->lpt_brownian, bool, "LPT_brownian_force", 1, OFF);
  }
#else
  SET_P_PASS_NOTFOUND(&flg->lpt_brownian, bool, "LPT_brownian_force", 1, OFF);
  if (flg->lpt_calc == ON && flg->lpt_brownian == ON) {
    SET_P_PERROR(ERROR, "LPT module does not support Brownian force");
    if (stat)
      *stat = ON;
  }
#endif

#ifdef LPTX
  if (flg->lpt_calc == ON) {
    SET_P(&flg->lpt_thermophoretic, bool, "LPT_thermophoretic_force", 1, OFF);
  } else {
    SET_P_PASS_NOTFOUND(&flg->lpt_thermophoretic, bool,
                        "LPT_thermophoretic_force", 1, OFF);
  }
#else
  SET_P_PASS_NOTFOUND(&flg->lpt_thermophoretic, bool,
                      "LPT_thermophoretic_force", 1, OFF);
  if (flg->lpt_calc == ON && flg->lpt_thermophoretic == ON) {
    SET_P_PERROR(ERROR, "LPT module does not support thermophoretic force");
    if (stat)
      *stat = ON;
  }
#endif

#ifdef LPTX
  if (flg->lpt_calc == ON && flg->lpt_brownian == ON) {
    SET_P(&flg->lpt_use_constant_Cc, bool, "LPT_const_cunningham_correction",
          1, ON);
  } else {
    SET_P_PASS_NOTFOUND(&flg->lpt_use_constant_Cc, bool,
                        "LPT_const_cunningham_correction", 1, ON);
  }
#else
  SET_P_PASS_NOTFOUND(&flg->lpt_use_constant_Cc, bool,
                      "LPT_const_cunningham_correction", 1, ON);
#endif

  //--- Misc program control
  {
    int print_update_level_set_reason = OFF;

    SET_P(&print_update_level_set_reason, bool, "print_update_level_set_reason",
          1, OFF);
    if (print_update_level_set_reason == ON) {
      update_level_set_reason_printer_data.flg = flg;
      update_level_set_flags_set_describe_reason_func(
        update_level_set_reason_printer, &update_level_set_reason_printer_data);
    }

    set_update_level_set_flags(&flg->update_level_set_ls, flags_fname,
                               flags_csv, "force_update_level_set_ls", NULL,
                               NULL, stat, print_update_level_set_reason);
    set_update_level_set_flags(&flg->update_level_set_lls, flags_fname,
                               flags_csv, "force_update_level_set_lls", NULL,
                               NULL, stat, print_update_level_set_reason);
    set_update_level_set_flags(&flg->update_level_set_ll, flags_fname,
                               flags_csv, NULL, NULL, NULL, stat,
                               print_update_level_set_reason);
  }

  if (flg->phase_change == ON) {
    update_level_set_flags_mark_update(&flg->update_level_set_lls,
                                       UPDATE_LEVEL_SET_BY_PHASE_CHANGE);
    update_level_set_flags_mark_update(&flg->update_level_set_ls,
                                       UPDATE_LEVEL_SET_BY_PHASE_CHANGE);
    // update_level_set_flags_mark_update(&flg->update_level_set_ll,
    //                                    UPDATE_LEVEL_SET_BY_PHASE_CHANGE);
  }

  {
    int iflg;
    flg->rebuild_components = init_component_zero();

    SET_P(&iflg, bool, "Reinit_vof", 1, OFF);
    if (iflg == ON)
      init_component_set(&flg->rebuild_components, INIT_COMPONENT_VOF);

    SET_P(&iflg, bool, "Reinit_velocity_u", 1, OFF);
    if (iflg == ON)
      init_component_set(&flg->rebuild_components, INIT_COMPONENT_VELOCITY_U);

    SET_P(&iflg, bool, "Reinit_velocity_v", 1, OFF);
    if (iflg == ON)
      init_component_set(&flg->rebuild_components, INIT_COMPONENT_VELOCITY_V);

    SET_P(&iflg, bool, "Reinit_velocity_w", 1, OFF);
    if (iflg == ON)
      init_component_set(&flg->rebuild_components, INIT_COMPONENT_VELOCITY_W);

    SET_P(&iflg, bool, "Reinit_temperature", 1, OFF);
    if (iflg == ON)
      init_component_set(&flg->rebuild_components, INIT_COMPONENT_TEMPERATURE);

    SET_P(&iflg, bool, "Reinit_pressure", 1, OFF);
    if (iflg == ON)
      init_component_set(&flg->rebuild_components, INIT_COMPONENT_PRESSURE);

    SET_P(&iflg, bool, "Reinit_fixed_heat_source", 1, OFF);
    if (iflg == ON)
      init_component_set(&flg->rebuild_components,
                         INIT_COMPONENT_FIXED_HSOURCE);

    SET_P(&iflg, bool, "Reinit_boundary", 1, OFF);
    if (iflg == ON) {
      init_component_set(&flg->rebuild_components, INIT_COMPONENT_BOUNDARY);
      init_component_set(&flg->rebuild_components,
                         INIT_COMPONENT_THERMAL_BOUNDARY);
      init_component_set(&flg->rebuild_components,
                         INIT_COMPONENT_SURFACE_BOUNDARY);
    }

    SET_P(&iflg, bool, "Reinit_fluid_boundary", 1, OFF);
    if (iflg == ON)
      init_component_set(&flg->rebuild_components, INIT_COMPONENT_BOUNDARY);

    SET_P(&iflg, bool, "Reinit_thermal_boundary", 1, OFF);
    if (iflg == ON)
      init_component_set(&flg->rebuild_components,
                         INIT_COMPONENT_THERMAL_BOUNDARY);

    SET_P(&iflg, bool, "Reinit_surface_boundary", 1, ON);
    if (iflg == OFF &&
        (exec_argument->restart > 0 || exec_argument->restart_job)) {
      SET_P_PERROR(
        WARN, "Current version does not support restart for surface boundary. "
              "So surface boundary will always be re-initialized");
    }
    init_component_set(&flg->rebuild_components,
                       INIT_COMPONENT_SURFACE_BOUNDARY);

#ifdef HAVE_LPT
    SET_P(&iflg, bool, "Reinit_lpt_pewall_n", 1, OFF);
    if (iflg == ON)
      init_component_set(&flg->rebuild_components,
                         INIT_COMPONENT_LPT_PEWALL_N);
#endif
  }

  SET_P(&flg->validate_VOF_init, bool, "validate_VOF_init", 1, ON);
  SET_P_NEXT(&flg->validate_VOF_print_max, int, 100);

  //--- Other option
  SET_P(&flg->binary, bool, "binary", 1, ON);
  SET_P(&flg->gnuplot, bool, "gnuplot", 1, OFF);

  /* YSE: Add IO flags */
  flg->output_data.readdir = NULL;
  flg->output_data.writedir = NULL;
  flg->output_data.filename_template.time = NULL;
  flg->output_data.filename_template.comp_based = NULL;
  flg->output_data.filename_template.others = NULL;

  flg->restart_data.readdir = NULL;
  flg->restart_data.writedir = NULL;
  flg->restart_data.filename_template.time = NULL;
  flg->restart_data.filename_template.comp_based = NULL;
  flg->restart_data.filename_template.others = NULL;

  if (flg->binary == ON) {
    SET_P(&flg->use_double_binary, bool, "output_double", 1, OFF);
    SET_P(&flg->output_mode, binary_output_mode, "output_mode", 1,
          BINARY_OUTPUT_UNIFY_MPI);

    SET_P(&flg->restart_output_mode, binary_output_mode, "restart_output_mode",
          1, BINARY_OUTPUT_UNIFY_MPI);
    SET_P(&flg->restart_input_mode, binary_output_mode, "restart_input_mode", 1,
          flg->restart_output_mode);

    SET_P(&flg->restart_data.readdir, DIRn,
          "restart_data_directory", 1, NULL);
    if (!flg->restart_data.readdir) {
      flg->restart_data.readdir = jupiter_strdup("data/binary_data");
    }

    SET_P_NEXT_PASS_NOTFOUND(&flg->restart_data.writedir, DIRn, NULL);
    if (!flg->restart_data.writedir && flg->restart_data.readdir) {
      char *rd;
      rd = jupiter_strdup(flg->restart_data.readdir);
      flg->restart_data.writedir = rd;
    }

    SET_P(&flg->output_data.writedir, DIRn,
          "output_data_directory", 1, NULL);
    if (!flg->output_data.writedir) {
      flg->output_data.writedir = jupiter_strdup("data/binary_data");
    }

    SET_P_NEXT_PASS_NOTFOUND(&flg->output_data.readdir, DIRn, NULL);
    if (!flg->output_data.readdir && flg->output_data.writedir) {
      char *rd;
      rd = jupiter_strdup(flg->output_data.writedir);
      flg->output_data.readdir = rd;
    }

    {
      struct filename_template_set time, others, comp_based;
      enum embed_requirement io_rank_set;
      enum embed_requirement restart_io_rank_set;

      if (flg->output_mode == BINARY_OUTPUT_BYPROCESS) {
        io_rank_set = required;
      } else {
        io_rank_set = optional;
      }

      if (flg->restart_output_mode == BINARY_OUTPUT_BYPROCESS ||
          flg->restart_input_mode == BINARY_OUTPUT_BYPROCESS) {
        restart_io_rank_set = required;
      } else {
        restart_io_rank_set = optional;
      }

      time.output_meta.default_name = "time/%04n.dat";
      time.output_meta.compname = optional;
      time.output_meta.rank = optional;
      time.output_meta.chrono_idx = required;
      time.output_meta.compid = optional;
      time.restart_meta.default_name = "time.dat";
      time.restart_meta.compname = optional;
      time.restart_meta.rank = optional;
      time.restart_meta.chrono_idx = optional;
      time.restart_meta.compid = optional;

      comp_based.output_meta.default_name = "%c_%i/%04n.%04r.dat";
      comp_based.output_meta.compname = required;
      comp_based.output_meta.rank = io_rank_set;
      comp_based.output_meta.chrono_idx = required;
      comp_based.output_meta.compid = required;
      comp_based.restart_meta.default_name = "%c_%i/%04r.dat";
      comp_based.restart_meta.compname = required;
      comp_based.restart_meta.rank = restart_io_rank_set;
      comp_based.restart_meta.chrono_idx = optional;
      comp_based.restart_meta.compid = required;

      others.output_meta.default_name = "%c/%04n.%04r.dat";
      others.output_meta.compname = required;
      others.output_meta.rank = io_rank_set;
      others.output_meta.chrono_idx = required;
      others.output_meta.compid = optional;
      others.restart_meta.default_name = "%c/%04r.dat";
      others.restart_meta.compname = required;
      others.restart_meta.rank = restart_io_rank_set;
      others.restart_meta.chrono_idx = optional;
      others.restart_meta.compid = optional;

      set_fname_template(&flg->output_data.filename_template, flg->binary,
                         flags_csv, flags_fname, "output_filename_templates",
                         &time.output_meta, &comp_based.output_meta,
                         &others.output_meta, stat);

      set_fname_template(&flg->restart_data.filename_template, flg->binary,
                         flags_csv, flags_fname, "restart_filename_templates",
                         &time.restart_meta, &comp_based.restart_meta,
                         &others.restart_meta, stat);

      time.output = flg->output_data.filename_template.time;
      time.restart = flg->restart_data.filename_template.time;
      others.output = flg->output_data.filename_template.others;
      others.restart = flg->restart_data.filename_template.others;
      comp_based.output = flg->output_data.filename_template.comp_based;
      comp_based.restart = flg->restart_data.filename_template.comp_based;

      /* Output flags */
      SET_OUTPUT_SPEC(time, ON, ON, 0, 0, NULL, time);
      SET_OUTPUT_SPEC(comp_data, ON, ON, 0, 0, NULL, others);

      if (flg->solute_diff == ON) {
        SET_OUTPUT_SPEC(fs, ON, ON, 1, 0, "output_fs", others);
        SET_OUTPUT_SPEC(fl, ON, ON, 1, 0, "output_fl", others);
        SET_OUTPUT_SPEC(Y, ON, ON, 1, 0, "output_Y", comp_based);
        SET_OUTPUT_SPEC(Vf, OFF, ON, 1, 0, "output_Vf", comp_based);
      } else {
        SET_OUTPUT_SPEC(fs, ON, ON, 1, 0, "output_fs", comp_based);
        SET_OUTPUT_SPEC(fl, ON, ON, 1, 0, "output_fl", comp_based);
        SET_OUTPUT_SPEC(Y, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(Vf, OFF, OFF, 0, 0, NULL, others);
      }
      
      SET_OUTPUT_SPEC(t,  ON, ON, 1, 0, "output_t", others);
      SET_OUTPUT_SPEC(u, OFF, ON, 1, 0, "output_u", others);
      SET_OUTPUT_SPEC(v, OFF, ON, 1, 0, "output_v", others);
      SET_OUTPUT_SPEC(w, OFF, ON, 1, 0, "output_w", others);
      SET_OUTPUT_SPEC(p, OFF, ON, 1, 0, "output_p", others);

      if (flg->phase_change == ON) {
        if (flg->solute_diff == ON) {
          SET_OUTPUT_SPEC(entha, OFF, ON, 1, 0, "output_entha", others);
          SET_OUTPUT_SPEC(mushy, OFF, ON, 1, 0, "output_mushy", others);
          SET_OUTPUT_SPEC(df, OFF, OFF, 0, 0, NULL, comp_based);
          SET_OUTPUT_SPEC(dfs, OFF, OFF, 0, 0, NULL, comp_based);
          /* SET_OUTPUT_SPEC(df, OFF, ON, 1, 0, "output_df", others);   */
          /* SET_OUTPUT_SPEC(dfs, OFF, ON, 1, 0, "output_dfs", others); */
        } else {
          SET_OUTPUT_SPEC(entha, OFF, OFF, 0, 0, NULL, others);
          SET_OUTPUT_SPEC(mushy, OFF, OFF, 0, 0, NULL, others);
          if (flg->melting == ON) {
            SET_OUTPUT_SPEC(df, OFF, ON, 1, 0, "output_df", comp_based);
          } else {
            SET_OUTPUT_SPEC(df, OFF, OFF, 0, 0, NULL, comp_based);
          }
          if (flg->solidification == ON) {
            SET_OUTPUT_SPEC(dfs, OFF, ON, 1, 0, "output_dfs", comp_based);
          } else {
            SET_OUTPUT_SPEC(dfs, OFF, OFF, 0, 0, NULL, comp_based);
          }
        }
      } else {
        SET_OUTPUT_SPEC(entha, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(mushy, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(df, OFF, OFF, 0, 0, NULL, comp_based);
        SET_OUTPUT_SPEC(dfs, OFF, OFF, 0, 0, NULL, comp_based);
      }
      if (flg->oxidation == ON) {
        SET_OUTPUT_SPEC(ox_dt, ON, ON, 1, 0, "output_ox_dt", others);
        SET_OUTPUT_SPEC(ox_dt_local, OFF, ON, 1, 0, "output_ox_dt_local", others);
        SET_OUTPUT_SPEC(ox_flag, OFF, ON, 1, 0, "output_ox_flag", others);
        SET_OUTPUT_SPEC(ox_lset, OFF, ON, 1, 0, "output_ox_lset", others);
        SET_OUTPUT_SPEC(ox_vof, ON, ON, 1, 0, "output_ox_vof", others);
        SET_OUTPUT_SPEC(ox_q, ON, OFF, 1, 1, "output_ox_q", others);
        if (flg->solute_diff == ON) {
          SET_OUTPUT_SPEC(ox_h2, OFF, ON, 1, 0, "output_ox_h2", others);
        } else {
          SET_OUTPUT_SPEC(ox_h2, OFF, OFF, 0, 0, NULL, others);
        }
        SET_OUTPUT_SPEC(ox_f_h2o, OFF, OFF, 1, 1, "output_ox_f_h2o", others);
        SET_OUTPUT_SPEC(ox_lset_h2o, OFF, ON, 1, 0, "output_ox_lset_h2o", others);
        SET_OUTPUT_SPEC(ox_lset_h2o_s, OFF, ON, 1, 0, "output_ox_lset_h2o_s", others);
      } else {
        SET_OUTPUT_SPEC(ox_dt, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(ox_dt_local, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(ox_flag, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(ox_lset, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(ox_vof, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(ox_q, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(ox_h2, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(ox_f_h2o, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(ox_lset_h2o, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(ox_lset_h2o_s, OFF, OFF, 0, 0, NULL, others);
      }
      if (flg->h2_absorp_eval == ON) {
        SET_OUTPUT_SPEC(h2_absorp_eval, ON, OFF, 1, 1, "output_h2_absorp_eval", others);
        SET_OUTPUT_SPEC(h2_absorp_Ks, OFF, OFF, 1, 1, "output_h2_absorp_Ks", others);
        SET_OUTPUT_SPEC(h2_absorp_P, OFF, OFF, 1, 1, "output_h2_absorp_P", others);
        SET_OUTPUT_SPEC(h2_absorp_T, OFF, OFF, 1, 1, "output_h2_absorp_T", others);
        SET_OUTPUT_SPEC(h2_absorp_Zr, OFF, OFF, 1, 1, "output_h2_absorp_Zr", others);
      } else {
        SET_OUTPUT_SPEC(h2_absorp_eval, OFF, OFF, 0, 0, "output_h2_absorp_eval", others);
        SET_OUTPUT_SPEC(h2_absorp_Ks, OFF, OFF, 0, 0, "output_h2_absorp_Ks", others);
        SET_OUTPUT_SPEC(h2_absorp_P, OFF, OFF, 0, 0, "output_h2_absorp_P", others);
        SET_OUTPUT_SPEC(h2_absorp_T, OFF, OFF, 0, 0, "output_h2_absorp_T", others);
        SET_OUTPUT_SPEC(h2_absorp_Zr, OFF, OFF, 0, 0, "output_h2_absorp_Zr", others);
      }
      if (flg->radiation == ON) {
        SET_OUTPUT_SPEC(rad, ON, OFF, 1, 1, "output_rad", others);
      } else {
        SET_OUTPUT_SPEC(rad, OFF, OFF, 1, 1, "output_rad", others);
      }

      SET_OUTPUT_SPEC(bnd, OFF, OFF, 0, 0, "output_bnd", others);
      SET_OUTPUT_SPEC(bnd_norm_u, OFF, ON, 1, 0, "output_bnd_norm_u", others);
      SET_OUTPUT_SPEC(bnd_norm_v, OFF, ON, 1, 0, "output_bnd_norm_v", others);
      SET_OUTPUT_SPEC(bnd_norm_w, OFF, ON, 1, 0, "output_bnd_norm_w", others);

      SET_OUTPUT_SPEC(uvw, ON, OFF, 1, 1, "output_uvw", others);
      SET_OUTPUT_SPEC(q, OFF, OFF, 1, 1, "output_q", others);
      SET_OUTPUT_SPEC(qgeom, OFF, ON, 1, 0, "output_qgeom", others);

      SET_OUTPUT_SPEC(lls, OFF, ON, 1, 0, "output_lls", others);
      SET_OUTPUT_SPEC(ll, OFF, ON, 1, 0, "output_ll", others);

      SET_OUTPUT_SPEC(ls, OFF, ON, 1, 0, "output_ls", others);

      SET_OUTPUT_SPEC(uplsflg, OFF, ON, 1, 0, "output_uplsflg", others);

      if (flg->multi_layer == ON){
        SET_OUTPUT_SPEC(fl_layer, ON, ON, 1, 0, "output_fl_layer", comp_based);
        SET_OUTPUT_SPEC(fls_layer, OFF, OFF, 1, 0, "output_fls_layer", comp_based);
        SET_OUTPUT_SPEC(ll_layer, OFF, OFF, 1, 0, "output_ll_layer", comp_based);
        SET_OUTPUT_SPEC(lls_layer, OFF, OFF, 1, 0, "output_lls_layer", comp_based);
        SET_OUTPUT_SPEC(curv_layer, OFF, OFF, 1, 0, "output_curv_layer", comp_based);
        SET_OUTPUT_SPEC(label_layer, ON, ON, 1, 0, "output_label_layer", comp_based);

        if(flg->film_drainage == ON){
          SET_OUTPUT_SPEC(liquid_film, ON, OFF, 1, 0, "output_liquid_film", others);
        }else{
          SET_OUTPUT_SPEC(liquid_film, OFF, OFF, 0, 0, "output_liquid_film", others);
        }

      }else{
        SET_OUTPUT_SPEC(fl_layer, OFF, OFF, 0, 0, "output_fl_layer", comp_based);
        SET_OUTPUT_SPEC(fls_layer, OFF, OFF, 0, 0, "output_fls_layer", comp_based);
        SET_OUTPUT_SPEC(ll_layer, OFF, OFF, 0, 0, "output_ll_layer", comp_based);
        SET_OUTPUT_SPEC(lls_layer, OFF, OFF, 0, 0, "output_lls_layer", comp_based);
        SET_OUTPUT_SPEC(curv_layer, OFF, OFF, 0, 0, "output_curv_layer", comp_based);
        SET_OUTPUT_SPEC(label_layer, OFF, OFF, 0, 0, "output_label_layer", comp_based);
      }

      if (flg->IBM == ON) {
        SET_OUTPUT_SPEC(fs_ibm, OFF, OFF, 1, 1, "output_fs_ibm", others);
        SET_OUTPUT_SPEC(ls_ibm, OFF, OFF, 1, 1, "output_ls_ibm", others);
      } else {
        SET_OUTPUT_SPEC(fs_ibm, OFF, OFF, 0, 0, "output_fs_ibm", others);
        SET_OUTPUT_SPEC(ls_ibm, OFF, OFF, 0, 0, "output_ls_ibm", others);
      }

      if (flg->porous == ON) {
        SET_OUTPUT_SPEC(eps, OFF, ON, 1, 0, "output_eps", others);
        SET_OUTPUT_SPEC(epss, OFF, ON, 1, 0, "output_epss", others);
        SET_OUTPUT_SPEC(perm, OFF, ON, 1, 0, "output_perm", others);
        if (flg->two_energy == ON) {
          SET_OUTPUT_SPEC(tf, OFF, ON, 1, 0, "output_tf", others);
          SET_OUTPUT_SPEC(ts, OFF, ON, 1, 0, "output_ts", others);
        } else {
          SET_OUTPUT_SPEC(tf, OFF, OFF, 0, 0, NULL, others);
          SET_OUTPUT_SPEC(ts, OFF, OFF, 0, 0, NULL, others);
        }
        SET_OUTPUT_SPEC(sgm, OFF, OFF, 1, 1, "output_sgm", others);
      } else {
        SET_OUTPUT_SPEC(eps, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(epss, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(perm, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(tf, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(ts, OFF, OFF, 0, 0, NULL, others);
        SET_OUTPUT_SPEC(sgm, OFF, OFF, 0, 0, NULL, others);
      }

      if (flg->solute_diff == ON) {
        SET_OUTPUT_SPEC(Yt, ON, OFF, 1, 1, "output_Yt", others);
        SET_OUTPUT_SPEC(flux, ON, OFF, 1, 1, "output_flux", others);
      } else {
        SET_OUTPUT_SPEC(Yt, OFF, OFF, 0, 0, "output_Yt", others);
        SET_OUTPUT_SPEC(flux, OFF, OFF, 0, 0, "output_flux", others);
      }

      {
        int any_lpt = (flg->lpt_calc == ON);
        int lpt_only;
        int lptx_only;
#ifndef HAVE_LPT
        any_lpt = 0;
#endif
        lpt_only = any_lpt;
        lptx_only = any_lpt;
#ifndef LPT
        lpt_only = 0;
#endif
#ifndef LPTX
        lptx_only = 0;
#endif

        if (any_lpt) {
          CSVASSERT((!!lpt_only) + (!!lptx_only) == 1);
        } else {
          CSVASSERT((!!lpt_only) + (!!lptx_only) == 0);
        }

        SET_OUTPUT_SPEC_MR(lptx_only, lpt_ptid, OFF, "output_lpt_ptid", others);
        SET_OUTPUT_SPEC_MR(lptx_only, lpt_oid, OFF, "output_lpt_oid", others);

        SET_OUTPUT_SPEC_MR(any_lpt, lpt_xpt, OFF, "output_lpt_xpt", others);
        SET_OUTPUT_SPEC_MR(any_lpt, lpt_ypt, OFF, "output_lpt_ypt", others);
        SET_OUTPUT_SPEC_MR(any_lpt, lpt_zpt, OFF, "output_lpt_zpt", others);
        SET_OUTPUT_SPEC_M1(any_lpt, lpt_pospt, OFF, OFF, "output_lpt_pospt",
                           others);

        SET_OUTPUT_SPEC_MR(any_lpt, lpt_uxpt, OFF, "output_lpt_uxpt", others);
        SET_OUTPUT_SPEC_MR(any_lpt, lpt_uypt, OFF, "output_lpt_uypt", others);
        SET_OUTPUT_SPEC_MR(any_lpt, lpt_uzpt, OFF, "output_lpt_uzpt", others);
        SET_OUTPUT_SPEC_M1(any_lpt, lpt_upt, OFF, OFF, "output_lpt_upt",
                           others);

        SET_OUTPUT_SPEC_MR(any_lpt, lpt_timpt, OFF, "output_lpt_timpt", others);

        SET_OUTPUT_SPEC_MR(any_lpt, lpt_fuxpt, OFF, "output_lpt_fuxpt", others);
        SET_OUTPUT_SPEC_MR(any_lpt, lpt_fuypt, OFF, "output_lpt_fuypt", others);
        SET_OUTPUT_SPEC_MR(any_lpt, lpt_fuzpt, OFF, "output_lpt_fuzpt", others);
        SET_OUTPUT_SPEC_M1(any_lpt, lpt_fupt, OFF, OFF, "output_lpt_fupt",
                           others);

        SET_OUTPUT_SPEC_MR(any_lpt, lpt_fduxt, OFF, "output_lpt_fduxt", others);
        SET_OUTPUT_SPEC_MR(any_lpt, lpt_fduyt, OFF, "output_lpt_fduyt", others);
        SET_OUTPUT_SPEC_MR(any_lpt, lpt_fduzt, OFF, "output_lpt_fduzt", others);
        SET_OUTPUT_SPEC_M1(any_lpt, lpt_fdt, OFF, OFF, "output_lpt_fdt",
                           others);

        SET_OUTPUT_SPEC_MR(lptx_only, lpt_dTdt, OFF, "output_lpt_dTdt", others);

        SET_OUTPUT_SPEC_M1(any_lpt, lpt_uf, OFF, OFF, "output_lpt_uf", others);
        SET_OUTPUT_SPEC_M1(lptx_only, lpt_muf, OFF, OFF, "output_lpt_muf",
                           others);
        SET_OUTPUT_SPEC_M1(lptx_only, lpt_densf, OFF, OFF, "output_lpt_densf",
                           others);
        SET_OUTPUT_SPEC_M1(lptx_only, lpt_cpf, OFF, OFF, "output_lpt_cpf",
                           others);
        SET_OUTPUT_SPEC_M1(lptx_only, lpt_thcf, OFF, OFF, "output_lpt_thcf",
                           others);
        SET_OUTPUT_SPEC_M1(lptx_only, lpt_tempf, OFF, OFF, "output_lpt_tempf",
                           others);
        SET_OUTPUT_SPEC_M1(lptx_only, lpt_gradTf, OFF, OFF, "output_lpt_gradTf",
                           others);
        SET_OUTPUT_SPEC_M1(lptx_only, lpt_pathf, OFF, OFF, "output_lpt_pathf",
                           others);
        SET_OUTPUT_SPEC_M1(lptx_only, lpt_mwf, OFF, OFF, "output_lpt_mwf",
                           others);

        SET_OUTPUT_SPEC_MR(any_lpt, lpt_diapt, OFF, "output_lpt_diapt", others);
        SET_OUTPUT_SPEC_MR(any_lpt, lpt_denspt, OFF, "output_lpt_denspt",
                           others);
        SET_OUTPUT_SPEC_MR(lptx_only, lpt_cppt, OFF, "output_lpt_cppt",
                           others);
        SET_OUTPUT_SPEC_MR(lptx_only, lpt_thcpt, OFF, "output_lpt_thcpt",
                           others);
        SET_OUTPUT_SPEC_M1(lptx_only, lpt_htrpt, OFF, OFF, "output_lpt_htrpt",
                           others);
        SET_OUTPUT_SPEC_M1(lptx_only, lpt_tothtpt, OFF, OFF,
                           "output_lpt_tothtpt", others);
        SET_OUTPUT_SPEC_MR(lptx_only, lpt_temppt, OFF, "output_lpt_temppt",
                           others);
        SET_OUTPUT_SPEC_MR(lptx_only, lpt_inipospt, OFF, "output_lpt_inipospt",
                           others);
        SET_OUTPUT_SPEC_MR(lptx_only, lpt_iniupt, OFF, "output_lpt_iniupt",
                           others);
        SET_OUTPUT_SPEC_MR(lptx_only, lpt_initemppt, OFF,
                           "output_lpt_initemppt", others);
        SET_OUTPUT_SPEC_M1(lptx_only, lpt_exit, OFF, OFF, "output_lpt_exit",
                           others);
        SET_OUTPUT_SPEC_MR(lptx_only, lpt_flags, OFF, "output_lpt_flags",
                           others);
        SET_OUTPUT_SPEC_MR(lptx_only, lpt_parceln, OFF, "output_lpt_parceln",
                           others);
        SET_OUTPUT_SPEC_M1(lptx_only, lpt_fbpt, OFF, OFF, "output_lpt_fbpt",
                           others);
        SET_OUTPUT_SPEC_M1(lptx_only, lpt_fTpt, OFF, OFF, "output_lpt_fTpt",
                           others);
        SET_OUTPUT_SPEC_MR(lptx_only, lpt_seed, OFF, "output_lpt_seed", others);
        SET_OUTPUT_SPEC_MR(lptx_only, lpt_Y, OFF, "output_lpt_Y", comp_based);
        SET_OUTPUT_SPEC_MR(any_lpt, lpt_ewall, OFF, "output_lpt_ewall", others);
      }
      {
        int lpt_ctrl_out, lpt_ctrl_res;
        lpt_ctrl_out = get_whether_output_lpt_ctrl(&flg->output_data);
        lpt_ctrl_res = get_whether_output_lpt_ctrl(&flg->restart_data);

        SET_OUTPUT_SPEC(lpt_ctrl, lpt_ctrl_out, lpt_ctrl_res, 0, 0, NULL, others);
      }

      SET_OUTPUT_SPEC(div_u, OFF, OFF, 1, 1, "output_div_u", others);
      SET_OUTPUT_SPEC(mass_source_g, ON, OFF, 1, 1, "output_mass_source_g", comp_based);

      SET_P(&flg->output_data.mat, bool, "output_mat", 1, OFF);
      SET_P_NEXT(&flg->restart_data.mat, bool, OFF);

      {
        int omat;
        int rmat;
        omat = flg->output_data.mat;
        rmat = flg->restart_data.mat;

        SET_OUTPUT_SPEC(dens, omat, rmat, 1, 1, "output_dens", others);
        SET_OUTPUT_SPEC(denss, omat, rmat, 1, 1, "output_denss", others);
        SET_OUTPUT_SPEC(densf, omat, rmat, 1, 1, "output_densf", others);
        SET_OUTPUT_SPEC(thc, omat, rmat, 1, 1, "output_thc", others);
        SET_OUTPUT_SPEC(thcs, omat, rmat, 1, 1, "output_thcs", others);
        SET_OUTPUT_SPEC(thcf, omat, rmat, 1, 1, "output_thcf", others);
        SET_OUTPUT_SPEC(specht, omat, rmat, 1, 1, "output_specht", others);
        SET_OUTPUT_SPEC(spechts, OFF, OFF, 0, 0, "output_spechts", others);
        SET_OUTPUT_SPEC(spechtf, omat, rmat, 1, 1, "output_spechtf", others);
        SET_OUTPUT_SPEC(mu, omat, rmat, 1, 1, "output_mu", others);

        if (flg->solute_diff == ON) {
          SET_OUTPUT_SPEC(t_liq, omat, rmat, 1, 1, "output_t_liq", others);
          SET_OUTPUT_SPEC(t_soli, omat, rmat, 1, 1, "output_t_soli", others);
          SET_OUTPUT_SPEC(diff_g, omat, rmat, 1, 1, "output_diff_g", others);
          SET_OUTPUT_SPEC(latent, omat, rmat, 1, 1, "output_latent", others);
        } else {
          SET_OUTPUT_SPEC(t_liq, OFF, OFF, 0, 0, "output_t_liq", others);
          SET_OUTPUT_SPEC(t_soli, OFF, OFF, 0, 0, "output_t_soli", others);
          SET_OUTPUT_SPEC(diff_g, OFF, OFF, 0, 0, "output_diff_g", others);
          SET_OUTPUT_SPEC(latent, OFF, OFF, 0, 0, "output_latent", others);
        }

        if (flg->radiation == ON) {
          SET_OUTPUT_SPEC(emi, omat, rmat, 1, 1, "output_emi", others);
        } else {
          SET_OUTPUT_SPEC(emi, OFF, OFF, 0, 0, "output_emi", others);
        }

        if (flg->oxidation == ON) {
          SET_OUTPUT_SPEC(ox_kp, omat, rmat, 1, 1, "output_ox_kp", others);
          SET_OUTPUT_SPEC(ox_dens, omat, rmat, 1, 1, "output_ox_dens", others);
          SET_OUTPUT_SPEC(ox_recess_rate, omat, rmat, 1, 1,
                          "output_ox_recess_rate", others);
        } else {
          SET_OUTPUT_SPEC(ox_kp, OFF, OFF, 0, 0, "output_ox_kp", others);
          SET_OUTPUT_SPEC(ox_dens, OFF, OFF, 0, 0, "output_ox_dens", others);
          SET_OUTPUT_SPEC(ox_recess_rate, OFF, OFF, 0, 0,
                          "output_ox_recess_rate", others);
        }
      }
    }
  }

  /* YSE: parameters from params.txt */
  SET_P_FROM(param_csv, param_fname);

  SET_P_REQUIRED(&flg->pex, int, "MPI_processes", 1, 1, stat);
  if (flg->pex <= 0) {
    if (stat)
      *stat = ON;
    SET_P_PERROR(ERROR, "# of MPI processes for X direction must be positive");
  }

  SET_P_NEXT_REQUIRED(&flg->pey, int, 1, stat);
  if (flg->pey <= 0) {
    if (stat)
      *stat = ON;
    SET_P_PERROR(ERROR, "# of MPI processes for Y direction must be positive");
  }

  SET_P_NEXT_REQUIRED(&flg->pez, int, 1, stat);
  if (flg->pez <= 0) {
    if (stat)
      *stat = ON;
    SET_P_PERROR(ERROR, "# of MPI processes for Z direction must be positive");
  }

  if (flg->radiation == ON) {
    SET_P_NEXT(&flg->pea, int, -1);  // number of procs. in angular division < 2016 Added by KKE
    if (flg->pea <= 0) {
      SET_P_PERROR(ERROR, "# of MPI processes for angle must be positive");
    }
  } else {
    SET_P_NEXT_PASS_NOTFOUND(&flg->pea, int, 0);
    if (flg->pea != 0) {
      SET_P_PERROR(WARN,
                   "When radiation is not enabled, "
                   "# of MPI processes for angle should be zero.");
    }
  }
}

static
void set_vin_parameter(const char *fname, csv_data *csv,
                       component_data *comp_data_head,
                       struct vin_data *vin, const char *keyname, int cond,
                       jcntrl_executive_manager *manager,
                       controllable_type *control_head, int *stat)
{
  int j;
  int ncomp;
  int nacomp;
  SET_P_INIT_NOLOC(csv, fname);
  struct csv_to_controllable_type_data cnti = {
    .manager = manager,
    .head = control_head,
  };
  struct csv_to_component_info_data_data cidd = {
    .comp_data_head = comp_data_head,
  };

  cnti.dest = &vin->u;
  SET_P(&cnti, controllable_type, keyname, 1, 0.0);
  if (!vin->u.exec) {
    SET_P_PERROR_FINITE(vin->u.current_value, ERROR,
                        "Inlet (U) must be finite");
  }

  cnti.dest = &vin->v;
  SET_P_NEXT(&cnti, controllable_type, 0.0);
  if (!vin->v.exec) {
    SET_P_PERROR_FINITE(vin->v.current_value, ERROR,
                        "Inlet (V) must be finite");
  }

  cnti.dest = &vin->w;
  SET_P_NEXT(&cnti, controllable_type, 0.0);
  if (!vin->w.exec) {
    SET_P_PERROR_FINITE(vin->w.current_value, ERROR,
                        "Inlet (W) must be finite");
  }

  SET_P_NEXT(&ncomp, int, 0);
  if (cond == INLET && ncomp == 0) {
    if (stat) *stat = ON;
    SET_P_PERROR(ERROR, "No components specified for inlet");
  } else if (ncomp < 0) {
    if (stat) *stat = ON;
    SET_P_PERROR(ERROR, "Number of inlet components must be positive");
  }

  nacomp = 0;
  vin->comps = inlet_component_data_new(ncomp);
  if (vin->comps) {
    nacomp = inlet_component_data_ncomp(vin->comps);
  } else {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    if (stat)
      *stat = ON;
  }

  for (j = 0; j < ncomp; ++j) {
    controllable_type ratio;
    struct component_info_data dummy;
    struct inlet_component_element *e;

    e = NULL;
    if (vin->comps && j < nacomp)
      e = inlet_component_data_get(vin->comps, j);

    cidd.dest = e ? &e->comp : &dummy;
    SET_P_NEXT(&cidd, component_info_data, JUPITER_ID_INVALID);
    if (!cidd.dest->d) {
      if (stat)
        *stat = ON;
    }

    if (e) {
      cnti.dest = &e->ratio;
    } else {
      cnti.dest = &ratio;
    }
    SET_P_NEXT(&cnti, controllable_type, HUGE_VAL);
    if (!cnti.dest->exec) {
      SET_P_PERROR_GREATER(cnti.dest->current_value, 0.0, ON, OFF, ERROR,
                           "Inlet ratio must be positive value");
    }
    if (cnti.dest == &ratio) {
      controllable_type_remove_from_list(&ratio);
    }
  }
}

static int set_outp_neumann_parameters(const char *fname, csv_data *csv,
                                        struct pout_data *outp,
                                        csv_row *found_row,
                                        csv_column **start_col, int *stat)
{
  /* Reserved for future use. NOP */
  return 0;
}

static int set_outp_const_parameters(
  const char *fname, csv_data *csv, struct pout_data *outp, csv_row *found_row,
  csv_column **start_col, jcntrl_executive_manager *manager,
  controllable_type *control_head, int *stat)
{
  struct csv_to_controllable_type_data cnti = {
    .manager = manager,
    .head = control_head,
  };

  SET_P_INIT(csv, fname, &found_row, start_col);

  cnti.dest = &outp->const_p;
  SET_P_NEXT(&cnti, controllable_type, 0.0);
  return 0;
}

static int set_outp_parameters(const char *fname, csv_data *csv,
                               struct pout_data *outp, const char *keyname,
                               int cond, jcntrl_executive_manager *manager,
                               controllable_type *control_head, int *stat)
{
  csv_row *row;
  csv_column *col;
  SET_P_INIT(csv, fname, &row, &col);

  SET_P(&outp->cond, out_p_cond, keyname, 1, OUT_P_COND_INVALID);

  switch (outp->cond) {
  case OUT_P_COND_NEUMANN:
    return set_outp_neumann_parameters(fname, csv, outp, row, &col, stat);

  case OUT_P_COND_CONST:
    return set_outp_const_parameters(fname, csv, outp, row, &col, manager,
                                     control_head, stat);

  case OUT_P_COND_INVALID:
    break;
  }

  if (cond == OUT) {
    SET_P_PERROR(ERROR, "Invalid pressure condition for OUT");
    if (stat)
      *stat = ON;
  }
  return 0;
}

static
int set_lpt_particle_sets(const char *fname, csv_data *csv, flags *flg,
                          domain *cdo, struct particle_set_input *head,
                          const char *keyname, int lpt_enabled, int *stat)
{
  csv_row *row;
  csv_column *col;
  csv_row *next_row;
  int npt;
  ptrdiff_t npt_total;
  int mxpset;
  int iset;
  int temp_required;
  int thc_required;
  int cp_required;

  SET_P_INIT(csv, fname, &row, &col);

  CSVASSERT(head);
  CSVASSERT(keyname);

  mxpset = -1;
  iset = 0;
  npt_total = 0;

  thc_required = 0;
  temp_required = 0;
  cp_required = 0;

#ifdef LPTX
  {
    LPTX_param *lptx_param;
    lptx_param = jLPTX_param_for_flg(flg);
    if (lptx_param) {
      thc_required = !!LPTX_param_want_thermal_conductivity(lptx_param);
      temp_required = !!LPTX_param_want_temperature(lptx_param);
      cp_required = !!LPTX_param_want_specific_heat(lptx_param);
      LPTX_param_delete(lptx_param);
    } else {
      if (stat)
        *stat = ON;
    }
  }
#endif

#ifdef LPT
  mxpset = cLPTmxpset();
  if (mxpset < 0) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_WARN, NULL,
               "Could not obtain maximum number of allowable particle sets. "
               "You can ignore this warning if you know that it has been set "
               "to the value which cannot be representable for default "
               "integral type of C language, `int`. But in case of that, it "
               "is limited to the maximum representable value of `int`.");
  }
#endif

#ifdef HAVE_LPT
  SET_P(&npt, int, keyname, 1, -1);
#else
  SET_P(&npt, int, keyname, 1, 0);
#endif
  if (row) {
    while (1) {
#ifdef LPTX
      LPTX_particle_init_set *setp;
#endif
      struct particle_set_input *inp;
      double xs, xe, ys, ye, zs, ze, tms, tme, di, ri, ux, uy, uz, temp, thc;
      double cp;
      int itrdm;

      if (iset == INT_MAX) {
        SET_P_PERROR(WARN, "Too many particle sets specified, accepts "
                     "first %d particle sets.", iset);
      } else {
        iset += 1;
      }
      if (mxpset >= 0 && iset > mxpset) {
        const char *suff;
        switch (iset % 10) {
        case 1:  suff = "st"; break;
        case 2:  suff = "nd"; break;
        case 3:  suff = "rd"; break;
        default: suff = "th"; break;
        }
        SET_P_PERROR(ERROR, "This is %d%s particle set, though only %d set%s "
                     "can be accepted", iset, suff, mxpset,
                     (mxpset != 1) ? "s" : "");
        if (stat) *stat = ON;
      }

      if (npt < 0) {
        SET_P_PERROR(ERROR, "Number of particles must be 0 or greater");
        if (stat) *stat = ON;
      } else {
        npt_total += npt;
      }

      SET_P_NEXT(&xs, exact_double, HUGE_VAL);
      if (!SET_P_PERROR_FINITE(xs, ERROR, "X start position must be finite")) {
        if (stat) *stat = ON;
      } else {
        SET_P_PERROR_RANGE(xs, 0.0, cdo->gLx, ON, ON, WARN,
                           "X start position is outside of computational "
                           "domain, [0, %g]", cdo->gLx);
      }

      SET_P_NEXT(&xe, exact_double, HUGE_VAL);
      if (!SET_P_PERROR_FINITE(xe, ERROR, "X end position must be finite")) {
        if (stat) *stat = ON;
      } else {
        SET_P_PERROR_RANGE(xe, 0.0, cdo->gLx, ON, ON, WARN,
                           "X end position is outside of computational "
                           "domain, [0, %g]", cdo->gLx);
        if (xe < xs) {
          SET_P_PERROR(ERROR, "X end position is less than X start position");
          if (stat) *stat = ON;
        }
      }

      SET_P_NEXT(&ys, exact_double, HUGE_VAL);
      if (!SET_P_PERROR_FINITE(ys, ERROR, "Y start position must be finite")) {
        if (stat) *stat = ON;
      } else {
        SET_P_PERROR_RANGE(ys, 0.0, cdo->gLy, ON, ON, WARN,
                           "Y start position is outside of computational "
                           "domain, [0, %g]", cdo->gLy);
      }

      SET_P_NEXT(&ye, exact_double, HUGE_VAL);
      if (!SET_P_PERROR_FINITE(ye, ERROR, "Y end position must be finite")) {
        if (stat) *stat = ON;
      } else {
        SET_P_PERROR_RANGE(ye, 0.0, cdo->gLy, ON, ON, WARN,
                           "Y end position is outside of computational "
                           "domain, [0, %g]", cdo->gLy);
        if (ye < ys) {
          SET_P_PERROR(ERROR, "Y end position is less than Y start position");
          if (stat) *stat = ON;
        }
      }

      SET_P_NEXT(&zs, exact_double, HUGE_VAL);
      if (!SET_P_PERROR_FINITE(zs, ERROR, "Z start position must be finite")) {
        if (stat) *stat = ON;
      } else {
        SET_P_PERROR_RANGE(zs, 0.0, cdo->gLz, ON, ON, WARN,
                           "Z start position is outside of computational "
                           "domain, [0, %g]", cdo->gLz);
      }

      SET_P_NEXT(&ze, exact_double, HUGE_VAL);
      if (!SET_P_PERROR_FINITE(ze, ERROR, "Z end position must be finite")) {
        if (stat) *stat = ON;
      } else {
        SET_P_PERROR_RANGE(ze, 0.0, cdo->gLz, ON, ON, WARN,
                           "Z end position is outside of computational "
                           "domain, [0, %g]", cdo->gLz);
        if (ze < zs) {
          SET_P_PERROR(ERROR, "Z end position is less than Z start position");
          if (stat) *stat = ON;
        }
      }

      SET_P_NEXT(&tms, exact_double, HUGE_VAL);
      if (!SET_P_PERROR_FINITE(tms, ERROR, "Start time must be finite")) {
        if (stat) *stat = ON;
      }

      SET_P_NEXT(&tme, exact_double, HUGE_VAL);
      if (!SET_P_PERROR_FINITE(tme, ERROR, "End time must be finite")) {
        if (stat) *stat = ON;
      } else {
        if (tme < tms) {
          SET_P_PERROR(ERROR, "End time is less than start time");
          if (stat) *stat = ON;
        } else {
          if (tms > cdo->tend || tme < cdo->time) {
            SET_P_PERROR(WARN,
                         "Given time range for particle injection, [%g, %g], "
                         "does not overlap to the computational time range, "
                         "[%g, %g]", tms, tme, cdo->time, cdo->tend);
          }
        }
      }

      SET_P_NEXT(&itrdm, bool, -1);

      SET_P_NEXT(&di, exact_double, 0.0);
      if (!SET_P_PERROR_GREATER(di, 0.0, OFF, OFF, ERROR,
                                "Particle diameter must be positive")) {
        if (stat) *stat = ON;
      }

      SET_P_NEXT(&ri, exact_double, 0.0);
      if (!SET_P_PERROR_GREATER(ri, 0.0, OFF, OFF, ERROR,
                                "Particle density must be positive")) {
        if (stat) *stat = ON;
      }

      SET_P_NEXT(&ux, exact_double, HUGE_VAL);
      if (!SET_P_PERROR_FINITE(ux, ERROR,
                               "Particle velocity U must be finite")) {
        if (stat) *stat = ON;
      }

      SET_P_NEXT(&uy, exact_double, HUGE_VAL);
      if (!SET_P_PERROR_FINITE(uy, ERROR,
                               "Particle velocity V must be finite")) {
        if (stat) *stat = ON;
      }

      SET_P_NEXT(&uz, exact_double, HUGE_VAL);
      if (!SET_P_PERROR_FINITE(uz, ERROR,
                               "Particle velocity W must be finite")) {
        if (stat) *stat = ON;
      }

      if (temp_required) {
        SET_P_NEXT(&temp, exact_double, 0.0);
      } else {
        SET_P_NEXT_PASS_NOTFOUND(&temp, exact_double, 0.0);
      }
#ifdef LPTX
      if (!SET_P_PERROR_FINITE(temp, ERROR,
                               "Particle temperature must be finite")) {
        if (stat) *stat = ON;
      }
#else
      if (SET_P_SOURCE_COL()) {
        SET_P_PERROR(WARN, "Particles in LPT does not have temperature");
      }
#endif

      if (thc_required) {
        SET_P_NEXT(&thc, exact_double, 0.0);
      } else {
        SET_P_NEXT_PASS_NOTFOUND(&thc, exact_double, 0.0);
      }
#ifdef LPTX
      if (!SET_P_PERROR_FINITE(
            thc, ERROR, "Particle thermal conductivity must be finite")) {
        if (stat)
          *stat = ON;
      }
#else
      if (SET_P_SOURCE_COL()) {
        SET_P_PERROR(WARN,
                     "Particles in LPT does not have thermal conductivity");
      }
#endif

      if (cp_required) {
        SET_P_NEXT(&cp, exact_double, 0.0);
      } else {
        SET_P_NEXT_PASS_NOTFOUND(&cp, exact_double, 0.0);
      }
#ifdef LPTX
      if (!SET_P_PERROR_FINITE(
            cp, ERROR, "Particle specific heat must be finite")) {
        if (stat)
          *stat = ON;
      }
#else
      if (SET_P_SOURCE_COL()) {
        SET_P_PERROR(WARN, "Particles in LPT does not have specific heat");
      }
#endif

      inp = (struct particle_set_input *)
        calloc(sizeof(struct particle_set_input), 1);
      if (!inp) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
                  CSV_ERR_NOMEM, errno, 0, NULL);
        if (stat) *stat = ON;
        return 1;
      }

      geom_list_insert_prev(&head->list, &inp->list);

#ifdef LPT
      cLPTpset_init(&inp->set);
      inp->set.nistpt = npt;
      inp->set.itrdm = itrdm;
      inp->set.x.start = xs;
      inp->set.x.end = xe;
      inp->set.y.start = ys;
      inp->set.y.end = ye;
      inp->set.z.start = zs;
      inp->set.z.end = ze;
      inp->set.tm.start = tms;
      inp->set.tm.end = tme;
      inp->set.di = di;
      inp->set.ri = ri;
      inp->set.u.x = ux;
      inp->set.u.y = uy;
      inp->set.u.z = uz;
#endif

#ifdef LPTX
      inp->set = LPTX_particle_init_set_new();
      if (inp->set) {
        LPTX_particle_flags f;
        LPTX_vector rs, re, u;
        rs = LPTX_vector_c(xs, ys, zs);
        re = LPTX_vector_c(xe, ye, ze);
        u = LPTX_vector_c(ux, uy, uz);
        f = LPTX_particle_flags_none();

        LPTX_particle_init_set_set_number_of_particles(inp->set, npt);
        LPTX_particle_init_set_set_origin_id(inp->set,
                                             JUPITER_LPTX_ORIGIN_INJECT);
        LPTX_particle_init_set_set_time_random(inp->set, !!itrdm);
        LPTX_particle_init_set_set_range_start(inp->set, rs);
        LPTX_particle_init_set_set_range_end(inp->set, re);
        LPTX_particle_init_set_set_time_start(inp->set, tms);
        LPTX_particle_init_set_set_time_end(inp->set, tme);
        LPTX_particle_init_set_set_diameter(inp->set, di);
        LPTX_particle_init_set_set_density(inp->set, ri);
        LPTX_particle_init_set_set_initial_velocity(inp->set, u);
        LPTX_particle_init_set_set_temperature(inp->set, temp);
        LPTX_particle_init_set_set_thermal_conductivity(inp->set, thc);
        LPTX_particle_init_set_set_specific_heat(inp->set, cp);
        LPTX_particle_init_set_set_flags(inp->set, f);
      } else {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM,
                  0, 0, NULL);
        if (stat)
          *stat = ON;
      }
#endif

      next_row = findCSVRowNext(row);
      if (!next_row) break;

      row = next_row;
      col = getColumnOfCSV(row, 1);
#ifdef HAVE_LPT
      SET_P_CURRENT(&npt, int, -1);
#else
      SET_P_CURRENT(&npt, int, 0);
#endif
    }
  }
#ifdef HAVE_LPT
  if (lpt_enabled == ON && npt_total <= 0) {
    if (row) {
      csvperrorf_row(fname, row, 0, CSV_EL_WARN,
                     "No particles set for calculation");
    }
  }
#endif
  return 0;
}

static
void set_lpt_wallref(const char *fname, csv_data *csv,
                     type *target, const char *keyname,
                     const char *boundary_name, int *stat)
{
  type outv;
  SET_P_INIT_NOLOC(csv, fname);

  CSVASSERT(target);
  CSVASSERT(keyname);
  CSVASSERT(boundary_name);

#ifdef HAVE_LPT
  SET_P(&outv, double, keyname, 1, -999.999);
#else
  SET_P_PASS_NOTFOUND(&outv, double, keyname, 1, -999.999);
#endif
  if (!SET_P_PERROR_FINITE(outv, ERROR,
                           "Wall restitution coefficient for %s boundary "
                           "must be finite", boundary_name)) {
    if (stat) *stat = ON;
  }
  *target = outv;
}

static
int set_solver_block_size_check(int value_got, int n, const char *axis_name,
                                const char *fname, csv_column *found_col,
                                csv_row *found_row, int *stat)
{
  SET_P_INIT(NULL, fname, &found_row, &found_col);

  if (value_got == 0) {
    return n;
  }
  if (value_got < 0) {
    SET_P_PERROR(ERROR, "Solver block size must be positive");
    if (stat)
      *stat = ON;
    return value_got;
  }
  if (n % value_got != 0) {
    SET_P_PERROR(WARN, "Given size can not divide the overall %s size %d, "
                 "so %d may be used for the size",
                 axis_name, n, n);
  }
  return value_got;
}

static void set_solver_block_size(struct solver_block_size *p, int defx,
                                  int defy, int defz, int nx, int ny,
                                  int nz, const char *fname, csv_data *csv,
                                  const char *keyname, int base,
                                  csv_column **col, csv_row **row, int *stat)
{
  csv_row *xrow;
  csv_column *xcol;
  SET_P_INIT(csv, fname, (row ? row : &xrow), (col ? col : &xcol));

  CSVASSERT(nx > 0);
  CSVASSERT(ny > 0);
  CSVASSERT(nz > 0);
  CSVASSERT(defx > 0);
  CSVASSERT(defy > 0);
  CSVASSERT(defz > 0);

  if (keyname) {
    SET_P(&p->nxblock, int, keyname, base, defx);
  } else {
    SET_P_NEXT(&p->nxblock, int, defx);
  }
  p->nxblock =
    set_solver_block_size_check(p->nxblock, nx, "X", fname, SET_P_SOURCE_COL(),
                                SET_P_SOURCE_ROW(), stat);

  SET_P_NEXT(&p->nyblock, int, defy);
  p->nyblock =
    set_solver_block_size_check(p->nyblock, ny, "Y", fname, SET_P_SOURCE_COL(),
                                SET_P_SOURCE_ROW(), stat);

  SET_P_NEXT(&p->nzblock, int, defz);
  p->nzblock =
    set_solver_block_size_check(p->nzblock, nz, "Z", fname, SET_P_SOURCE_COL(),
                                SET_P_SOURCE_ROW(), stat);
}

static int solver_block_size_default_axis(int base, int base_n, int n, int def)
{
  if (base == base_n) {
    return n;
  }
  if (n % base == 0) {
    return base;
  }
  return def;
}

static void solver_block_size_modify_default(struct solver_block_size *base,
                                             int *defx, int *defy, int *defz,
                                             int base_nx, int base_ny,
                                             int base_nz, int nx, int ny,
                                             int nz)
{
  CSVASSERT(defx);
  CSVASSERT(defy);
  CSVASSERT(defz);

  *defx = solver_block_size_default_axis(base->nxblock, base_nx, nx, *defx);
  *defy = solver_block_size_default_axis(base->nyblock, base_ny, ny, *defy);
  *defz = solver_block_size_default_axis(base->nzblock, base_nz, nz, *defz);
}

static int set_non_uniform_grid_by_key(int have_non_uniform_grid,
                                       const char *keystr,
                                       struct non_uniform_grid_input_data *inp,
                                       csv_data *csv, const char *fname,
                                       csv_row **row, csv_column **col,
                                       int *stat)
{
  int ret;
  int nreg;
  type low;
  SET_P_INIT(csv, fname, row, col);

  non_uniform_grid_input_data_init(inp);

  SET_P_BASE(&nreg, int, keystr, 1, 0, ON, ON,
             (have_non_uniform_grid == ON) ? CSV_EL_ERROR : CSV_EL_WARN,
             CSV_EL_ERROR, NULL);
  if (have_non_uniform_grid == ON && nreg <= 0) {
    SET_P_PERROR(ERROR, "Number of regions must be positive");
    if (stat)
      *stat = ON;
  }
  if (have_non_uniform_grid != ON)
    return 0;

  SET_P_NEXT(&low, double, HUGE_VAL);
  if (!SET_P_PERROR_FINITE(low, ERROR, "The lowest domain must be finite")) {
    if (stat)
      *stat = ON;
  }

  inp->high = low;
  ret = non_uniform_grid_input_read_csv(inp, nreg, csv, fname, row, col, stat);
  return ret;
}

static void check_non_uniform_grid(csv_data *csv, const char *fname,
                                   csv_row *row, csv_column *column,
                                   struct non_uniform_grid_input_data *head,
                                   const char *axis_name, //
                                   int *gn, int *n, type *L, int npe, int *stat)
{
  int lgn;
  type lL;

  SET_P_INIT(csv, fname, &row, &column);
  CSVASSERT(gn);
  CSVASSERT(L);

  lgn = non_uniform_grid_total_ndivs(head);
  lL = non_uniform_grid_total_length(head);
  *gn = lgn;
  *L = lL;

  if (row)
    column = getColumnOfCSV(row, 0);

  if (lgn <= 0) {
    SET_P_PERROR(ERROR, "Total number of cells (%d) is 0 or negative",
                 axis_name, lgn);
    if (stat)
      *stat = ON;
  } else if (lgn % npe != 0) {
    SET_P_PERROR(
      ERROR,
      "Total number of cells (%d) is not divisible by number of ranks (%d)",
      lgn, npe);
    if (stat)
      *stat = ON;
  } else {
    *n = lgn / npe;
  }

  if (lL < 0.0) {
    SET_P_PERROR(ERROR, "Total length of domain (%g) is negative", lL);
    if (stat)
      *stat = ON;
  } else if (!SET_P_PERROR_GREATER(lL, 0.0, OFF, OFF, ERROR,
                                   "Total length (%g) of domain must be finite",
                                   lL)) {
    if (stat)
      *stat = ON;
  }
}

/* YSE: Set values for domain from CSV data */
void set_cdomain(flags *flg, domain *cdo, mpi_param *mpi,
                 const char *fname, csv_data *csv,
                 jcntrl_executive_manager *manager,
                 controllable_type *control_head,
                 component_data *comp_data_head, int *stat)
{
  int i;
  SET_P_INIT_NOLOC(csv, fname);

  struct csv_to_controllable_type_data cnti = {
    .manager = manager,
    .head = control_head,
  };

  /* these 3 variables are mandatory. */
  CSVASSERT(flg);
  CSVASSERT(cdo);
  CSVASSERT(mpi);

  SET_P_REQUIRED(&cdo->nx, int, "Cell", 1, 0, stat);
  if (cdo->nx <= 0) {
    if (stat)
      *stat = ON;
    SET_P_PERROR(ERROR, "Number of cells in X direction must be positive");
  }

  SET_P_NEXT_REQUIRED(&cdo->ny, int, 0, stat);
  if (cdo->ny <= 0) {
    if (stat)
      *stat = ON;
    SET_P_PERROR(ERROR, "Number of cells in Y direction must be positive");
  }

  SET_P_NEXT_REQUIRED(&cdo->nz, int, 0, stat);
  if (cdo->nz <= 0) {
    if (stat)
      *stat = ON;
    SET_P_PERROR(ERROR, "Number of cells in Z direction must be positive");
  }

  cdo->gnx = cdo->nx * flg->pex;
  cdo->gny = cdo->ny * flg->pey;
  cdo->gnz = cdo->nz * flg->pez;

  SET_P_REQUIRED(&cdo->gLx, double, "Length", 1, 0.0, stat);  // spatial domain(x)[m]
  SET_P_PERROR_GREATER(cdo->gLx, 0.0, OFF, OFF,
                       ERROR, "Length of X direction must be positive");

  SET_P_NEXT_REQUIRED(&cdo->gLy, double, 0.0, stat);          // spatial domain(y)[m]
  SET_P_PERROR_GREATER(cdo->gLy, 0.0, OFF, OFF,
                       ERROR, "Length of Y direction must be positive");

  SET_P_NEXT_REQUIRED(&cdo->gLz, double, 0.0, stat);          // spatial domain(z)[m]
  SET_P_PERROR_GREATER(cdo->gLz, 0.0, OFF, OFF,
                       ERROR, "Length of Z direction must be positive");

  {
    csv_row **row;
    csv_column **col;
    type L;
    int n;

    row = &SET_P_SOURCE_ROW();
    col = &SET_P_SOURCE_COL();

    set_non_uniform_grid_by_key(flg->has_non_uniform_grid,
                                NON_UNIFORM_GRID_X_KEYNAME,
                                &cdo->non_uniform_grid_x_head, csv, fname, row,
                                col, stat);
    if (flg->has_non_uniform_grid == ON)
      check_non_uniform_grid(csv, fname, *row, *col,
                             &cdo->non_uniform_grid_x_head, "X", &cdo->gnx,
                             &cdo->nx, &cdo->gLx, flg->pex, stat);

    set_non_uniform_grid_by_key(flg->has_non_uniform_grid,
                                NON_UNIFORM_GRID_Y_KEYNAME,
                                &cdo->non_uniform_grid_y_head, csv, fname, row,
                                col, stat);
    if (flg->has_non_uniform_grid == ON)
      check_non_uniform_grid(csv, fname, *row, *col,
                             &cdo->non_uniform_grid_y_head, "Y", &cdo->gny,
                             &cdo->ny, &cdo->gLy, flg->pey, stat);

    set_non_uniform_grid_by_key(flg->has_non_uniform_grid,
                                NON_UNIFORM_GRID_Z_KEYNAME,
                                &cdo->non_uniform_grid_z_head, csv, fname, row,
                                col, stat);
    if (flg->has_non_uniform_grid == ON)
      check_non_uniform_grid(csv, fname, *row, *col,
                             &cdo->non_uniform_grid_z_head, "Z", &cdo->gnz,
                             &cdo->nz, &cdo->gLz, flg->pez, stat);
  }

  SET_P(&cdo->stm, int, "stm", 1, 3); // - stencil
  if (cdo->stm != 3) {
    if (stat) *stat = ON;
    SET_P_PERROR(ERROR, "stm is restricted to 3 in this version");
  }

  SET_P(&cdo->stp, int, "stp", 1, 4); // + stencil
  if (cdo->stp != 4) {
    if (stat) *stat = ON;
    SET_P_PERROR(ERROR, "stp is restricted to 4 in this version");
  }

  SET_P(&cdo->NIBaseComponent, int, "NumberOfComponent", 1, 0);
  if (cdo->NIBaseComponent < 1) {
    SET_P_PERROR(ERROR, "One component must be defined at least");
  }

  if (flg->solute_diff == ON) {
    SET_P_NEXT(&cdo->NIGasComponent, int, 0);
    if (cdo->NIGasComponent < 0) {
      SET_P_PERROR(ERROR, "Number of Gas-only components must be 0 or positive");
    }
  } else {
    cdo->NIGasComponent = 0;
  }
  cdo->NBaseComponent = -1;
  cdo->NGasComponent = -1;
  cdo->NumberOfComponent = -1;
  cdo->NIComponent = cdo->NIBaseComponent + cdo->NIGasComponent;
  if (cdo->NIComponent < 0) {
    if (cdo->NIGasComponent >= 0 && cdo->NIBaseComponent >= 0) {
      SET_P_PERROR(ERROR, "Total number of components overflowed");
    }
  }
  for (int ic = -1; ic < cdo->NIComponent; ++ic) {
    component_data *d = component_data_new();
    if (!d) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      if (stat)
        *stat = ON;
      return;
    }

    d->generated = 0;
    d->phase_comps_index = ic;
    d->comp_index = ic;
    d->jupiter_id = ic;
    d->phases = component_phases_none();
    if (ic == -1 || ic >= cdo->NIBaseComponent) {
      component_phases_set(&d->phases, COMPONENT_PHASE_GAS, 1);
    } else {
      component_phases_set(&d->phases, COMPONENT_PHASE_SOLID, 1);
      component_phases_set(&d->phases, COMPONENT_PHASE_LIQUID, 1);
    }

    geom_list_insert_prev(&comp_data_head->list, &d->list);
  }

  SET_P(&cdo->NumberOfLayer, int, "NumberOfLayer", 1, 0);
  if (flg->multi_layer == ON && cdo->NumberOfLayer < 1) {
    SET_P_PERROR(ERROR, "One layer must be defined at least when multi_layer method is enabled");
  }

  //--- Wall temperature (works when flg->bct=ISOTHERMAL is selected.)
  cnti.dest = &cdo->tw_xm;
  SET_P(&cnti, controllable_type, "tw_xm", 1, 273.15);
  if (!cdo->tw_xm.exec) {
    SET_P_PERROR_FINITE(cdo->tw_xm.current_value, ERROR,
                        "tw_xm must be finite");
  }

  cnti.dest = &cdo->tw_xp;
  SET_P(&cnti, controllable_type, "tw_xp", 1, 273.15);
  if (!cdo->tw_xp.exec) {
    SET_P_PERROR_FINITE(cdo->tw_xp.current_value, ERROR,
                        "tw_xp must be finite");
  }

  cnti.dest = &cdo->tw_ym;
  SET_P(&cnti, controllable_type, "tw_ym", 1, 273.15);
  if (!cdo->tw_ym.exec) {
    SET_P_PERROR_FINITE(cdo->tw_ym.current_value, ERROR,
                        "tw_ym must be finite");
  }

  cnti.dest = &cdo->tw_yp;
  SET_P(&cnti, controllable_type, "tw_yp", 1, 273.15);
  if (!cdo->tw_yp.exec) {
    SET_P_PERROR_FINITE(cdo->tw_yp.current_value, ERROR,
                        "tw_yp must be finite");
  }

  cnti.dest = &cdo->tw_zm;
  SET_P(&cnti, controllable_type, "tw_zm", 1, 273.15);
  if (!cdo->tw_zm.exec) {
    SET_P_PERROR_FINITE(cdo->tw_zm.current_value, ERROR,
                        "tw_zm must be finite");
  }

  cnti.dest = &cdo->tw_zp;
  SET_P(&cnti, controllable_type, "tw_zp", 1, 273.15);
  if (!cdo->tw_zp.exec) {
    SET_P_PERROR_FINITE(cdo->tw_zp.current_value, ERROR,
                        "tw_zp must be finite");
  }

  //--- Phase Field parameters
  SET_P(&cdo->c_delta, double, "c_delta", 1, 2.0);
  if (!SET_P_PERROR_RANGE(cdo->c_delta, 2.0, 3.0, ON, ON, ERROR,
        "c_delta must be in the range of 2.0 <= c_delta <= 3.0")) {
    if (stat) *stat = ON;
  }
  SET_P(&cdo->lambda, double, "lambda", 1, 0.01);
  if (!SET_P_PERROR_RANGE(cdo->lambda, 0.01, 0.1, ON, ON, ERROR,
        "lambda must be in the range of 0.01 <= lambda <= 0.1")) {
    if (stat) *stat = ON;
  }
  SET_P(&cdo->sigma_pf, double, "sigma_pf", 1, 0.0728);
  if (!SET_P_PERROR_GREATER(cdo->sigma_pf, 0.0, OFF, OFF, ERROR, "sigma_pf must be positive")) {
    if (stat) *stat = ON;
  }
  SET_P(&cdo->mobility, double, "mobility", 1, 1.0);
  if (!SET_P_PERROR_RANGE(cdo->mobility, 1.0, 3.0, ON, ON, ERROR,
        "mobility must be in the range of 1.0 <= mobility <= 3.0")) {
    if (stat) *stat = ON;
  }

  //--- Porous model parameters (two energy model)
  SET_P(&cdo->d_p, double, "d_p", 1, 0.01);
  if (!SET_P_PERROR_GREATER(cdo->d_p, 0.0, OFF, OFF, ERROR, "d_p must be positive")) {
    if (stat) *stat = ON;
  }
  SET_P(&cdo->U_ref, double, "U_ref", 1, 1.0); // [m/s]
  if (!SET_P_PERROR_GREATER(cdo->U_ref, 0.0, OFF, OFF, ERROR, "U_ref must be positive")) {
    if (stat) *stat = ON;
  }
  SET_P(&cdo->L_ref, double, "L_ref", 1, 1.0); // [m]
  if (!SET_P_PERROR_GREATER(cdo->L_ref, 0.0, OFF, OFF, ERROR, "L_ref must be positive")) {
    if (stat) *stat = ON;
  }
  SET_P(&cdo->Forch_coef, double, "Forch_coef", 1, 0.099418); // [-]
  if (!SET_P_PERROR_GREATER(cdo->Forch_coef, 0.0, OFF, OFF, ERROR, "Forch_coef must be positive (Forch_coef > 0.0)")) {
    if (stat) *stat = ON;
  }

  //--- Pressure constant value at the outflow boundary
  SET_P(&cdo->pconst, double, "P_const", 1, 0.0); //constant P at outflow boundary
  if (SET_P_SOURCE_ROW()) {
    if (cdo->pconst == 0.0) {
      SET_P_PERROR(
        WARN,
        "P_const has been deprecated and not used anymore. Use p_* parameters");
    } else {
      SET_P_PERROR(
        ERROR,
        "P_const has been deprecated and not used anymore. Use p_* parameters");
      if (stat)
        *stat = ON;
    }
  }

  //--- Contact angle
  SET_P(&cdo->contact_angle, double, "contact_angle", 1, 90.0);
  if (!SET_P_PERROR_RANGE(cdo->contact_angle, 0.0, 180.0, ON, ON, ERROR,
      "contact_angle must be in the range of 0 <= theta <= 180.0")) {
    if (stat) *stat = ON;
  }
  SET_P(&cdo->CA_iteration , int, "CA_iteration", 1, 6); 
  if (cdo->CA_iteration< 1) {
    if (stat) *stat = ON;
    SET_P_PERROR(ERROR, "CA iteration must be greater than 1");
  }

  //--- Film dranaige
  SET_P(&cdo->film_cell, double, "film_cell", 1, 3.0);
  if (!SET_P_PERROR_GREATER(cdo->film_cell, 0.0, OFF, OFF, ERROR, "film_cell must be positive")) {
    if (stat) *stat = ON;
  }
  SET_P(&cdo->rapture_thickness, double, "rapture_thickness", 1, 1.0);
  if (!SET_P_PERROR_GREATER(cdo->rapture_thickness, 0.0, OFF, OFF, ERROR, "rapture_thickness must be positive")) {
    if (stat) *stat = ON;
  }
  SET_P(&cdo->height_threshold, double, "height_threshold", 1, -1e10);

  /* YSE: Add inlet velocity input for global set */
  //--- Inlet velocity (works when flg->bc=INLET is selected.)
  set_vin_parameter(fname, csv, comp_data_head, &cdo->vin_xm, "vin_xm",
                    flg->bc_xm, manager, control_head, stat);
  set_vin_parameter(fname, csv, comp_data_head, &cdo->vin_xp, "vin_xp",
                    flg->bc_xp, manager, control_head, stat);
  set_vin_parameter(fname, csv, comp_data_head, &cdo->vin_ym, "vin_ym",
                    flg->bc_ym, manager, control_head, stat);
  set_vin_parameter(fname, csv, comp_data_head, &cdo->vin_yp, "vin_yp",
                    flg->bc_yp, manager, control_head, stat);
  set_vin_parameter(fname, csv, comp_data_head, &cdo->vin_zm, "vin_zm",
                    flg->bc_zm, manager, control_head, stat);
  set_vin_parameter(fname, csv, comp_data_head, &cdo->vin_zp, "vin_zp",
                    flg->bc_zp, manager, control_head, stat);

  //--- Pressure condition (works when flg->bc=OUT is selected.)
  set_outp_parameters(fname, csv, &cdo->p_xm, "p_xm", flg->bc_xm, manager,
                      control_head, stat);
  set_outp_parameters(fname, csv, &cdo->p_xp, "p_xp", flg->bc_xp, manager,
                      control_head, stat);
  set_outp_parameters(fname, csv, &cdo->p_ym, "p_ym", flg->bc_ym, manager,
                      control_head, stat);
  set_outp_parameters(fname, csv, &cdo->p_yp, "p_yp", flg->bc_yp, manager,
                      control_head, stat);
  set_outp_parameters(fname, csv, &cdo->p_zm, "p_zm", flg->bc_zm, manager,
                      control_head, stat);
  set_outp_parameters(fname, csv, &cdo->p_zp, "p_zp", flg->bc_zp, manager,
                      control_head, stat);

  //--- Time step restriction
  SET_P(&cdo->dt, double, "dt", 1, 0.0); // dt[s] (reads, but not used)
  SET_P(&cdo->cfl_num, double, "CFL_num", 1, 0.0);   // cfl number[u dt dx-1]
  SET_P_PERROR_FINITE(cdo->cfl_num, ERROR, "CFL_num must be finite");

  SET_P(&cdo->diff_num, double, "DIFF_num", 1, 0.0); // diffusion number[k dt dx-2]
  SET_P_PERROR_FINITE(cdo->diff_num, ERROR, "DIFF_num must be finite");

  SET_P(&cdo->dt_rad, double, "dt_rad", 1,
        ((flg->radiation == ON) ? 0.0 : 1.0)); //(radiation == OFF のときは, どんな値でも問題ない) < 2016 Added by KKE radiation calc interval [sec]
  SET_P_PERROR_GREATER(cdo->dt_rad, 0.0, OFF, OFF,
                       ERROR, "dt_rad must be positive");

  SET_P(&cdo->coef_lsts, double, "coef_lsts", 1, 0.25);
  if (!SET_P_PERROR_RANGE(cdo->coef_lsts, 0.0, 0.5, OFF, ON, ERROR,
        "coefficient of time step of level-set iteration must be in the range of 0 < coef_lsts <= 0.5")) {
    if(stat) *stat = ON;
  }
  SET_P(&cdo->ls_iteration, int, "ls_iteration", 1, 20); 
  if (cdo->ls_iteration< 1) {
    if (stat) *stat = ON;
    SET_P_PERROR(ERROR, "level-set iteration must be greater than 1");
  }

  //--- Simulation time (physical)
  {
    int t;
    SET_P(&cdo->time, double, "Time", 1, 0.0); // init time[sec]
    t = SET_P_PERROR_FINITE(cdo->time, ERROR, "Start time must be finite");

    SET_P_NEXT(&cdo->tend, double, 0.0);       // end  time[sec]
    if (t) {
      SET_P_PERROR_GREATER(cdo->tend, cdo->time, OFF, OFF, ERROR,
                           "End time must be greater than Start time, %g [s]",
                           cdo->time);
    } else {
      SET_P_PERROR_FINITE(cdo->tend, ERROR, "End time must be greater than Start time (and finite)");
    }
  }

  //--- Output
  SET_P(&cdo->view, int, "View", 1, 0); // interval of printf
  if (cdo->view <= 0) {
    if (stat) *stat = ON;
    SET_P_PERROR(ERROR, "View must be positive");
  }

  SET_P(&cdo->outs, int, "Output", 1, 0); // number of output data
  if (cdo->outs < 0) {
    if (stat) *stat = ON;
    SET_P_PERROR(ERROR, "Output must be positive or zero");
  }

  SET_P(&cdo->nsub_step_t, int, "nsub_step_t", 1, 0); // number of substeps for heat conduction
  if (cdo->nsub_step_t <= 0) {
    if (stat) *stat = ON;
    SET_P_PERROR(ERROR, "nsub_step_t must be positive");
  }

  SET_P(&cdo->nsub_step_mu, int, "nsub_step_mu", 1, 0); // number of substeps for viscosity
  if (cdo->nsub_step_mu <= 0) {
    if (stat) *stat = ON;
    SET_P_PERROR(ERROR, "nsub_step_mu must be positive");
  }

  SET_P(&cdo->icnt_end, int, "radiation_step_limit", 1, 9999999);  // < 2016 Modified by KKE
  if (cdo->icnt_end <= 0) {
    if (stat) *stat = ON;
    SET_P_PERROR(ERROR, "radiation_step_limit must be positive");
  }

  //--- Gravity
  /* YSE: Moved from below */
  SET_P(&cdo->grav_x, double, "Gravity", 1,  0.0); // gravity(x)[m s-2]
  SET_P_PERROR_FINITE(cdo->grav_x, ERROR, "Gravity (X dir) must be finite");

  SET_P_NEXT(&cdo->grav_y, double,           0.0); // gravity(y)[m s-2]
  SET_P_PERROR_FINITE(cdo->grav_y, ERROR, "Gravity (Y dir) must be finite");

  SET_P_NEXT(&cdo->grav_z, double,          -9.8); // gravity(z)[m s-2]
  SET_P_PERROR_FINITE(cdo->grav_z, ERROR, "Gravity (Z dir) must be finite");

  //--- Phase Change (Vaporization/Condensation)
  {
    struct csv_to_component_info_data_data cidd = {
      .comp_data_head = comp_data_head,
      .dest = &cdo->vap_cond_liquid_id,
    };
    SET_P(&cidd, component_info_data, "vaporization_liquid_id", 1,
          JUPITER_ID_INVALID);
    if (flg->phase_change == ON &&
        (flg->vaporization == ON || flg->condensation == ON)) {
      if (cdo->vap_cond_liquid_id.d) {
        CSVASSERT(!cdo->vap_cond_liquid_id.d->generated);
        CSVASSERT(cdo->vap_cond_liquid_id.d->phase_comps_index >= 0);
        if (!component_phases_has_liquid(cdo->vap_cond_liquid_id.d->phases)) {
          SET_P_PERROR(ERROR, "Invalid liquid material ID for vaporization or condensation");
          if (stat)
            *stat = ON;
        }
      } else {
        if (stat)
          *stat = ON;
      }
    }
  }

  //--- Oxidation
  tempdep_property_init(&cdo->ox_kp);
  if (flg->oxidation == ON) {
    csv_row *row_h2, *row_h2o;

    row_h2 = NULL;
    row_h2o = NULL;
    ox_set_component_info(&cdo->ox_zry, comp_data_head, -1, 1, 0,
                          "oxide_zircaloy", csv, fname, NULL, NULL,
                          ox_set_component_info_id_check_zry, NULL, NULL, stat);
    ox_set_component_info(&cdo->ox_zro2, comp_data_head, -1, 1, 0, "oxide_zro2",
                          csv, fname, NULL, NULL,
                          ox_set_component_info_id_check_zro2, NULL, NULL,
                          stat);
    ox_set_component_info(&cdo->ox_h2, comp_data_head, 1, 0, 1, "oxide_h2", csv,
                          fname, &row_h2, NULL,
                          ox_set_component_info_id_check_h2, NULL, NULL, stat);
    ox_set_component_info(&cdo->ox_h2o, comp_data_head, 1, 0, 1, "oxide_h2o",
                          csv, fname, &row_h2o, NULL,
                          ox_set_component_info_id_check_h2o, NULL, NULL, stat);
    CSVASSERT(ox_component_info_ncompo(&cdo->ox_h2) <= 1);
    if (ox_is_enabled_component(&cdo->ox_h2)) {
      CSVASSERT(row_h2);
      if (flg->solute_diff != ON) {
        csvperrorf_row(fname, row_h2, 0, CSV_EL_ERROR,
                       "To use hydrogen generation, solute_diff must be ON");
        if (stat) *stat = ON;
      } else {
        CSVASSERT(flg->restart_data.ox_h2.outf == ON);
      }
    }
    if (flg->h2_absorp_eval == ON && !ox_is_enabled_component(&cdo->ox_h2)) {
      const char *msg;
      msg = "To evaluate H2 absorption, H2 ID must be specified to a valid "
        "value.";
      if (row_h2) {
        csvperrorf_row(fname, row_h2, 0, CSV_EL_ERROR, "%s", msg);
      } else {
        csvperrorf(fname, 0, 0, CSV_EL_ERROR, NULL,
                   "%s Additionally, \"ox_h2\" is not specified.", msg);
      }
      if (stat) *stat = ON;
    }

    CSVASSERT(ox_component_info_ncompo(&cdo->ox_h2o) <= 1);
    if (ox_is_enabled_component(&cdo->ox_h2o)) {
      CSVASSERT(row_h2o);
      if (flg->solute_diff != ON) {
        csvperrorf_row(fname, row_h2o, 0, CSV_EL_ERROR,
                       "To use steam consumption, solute_diff must be ON");
        if (stat) *stat = ON;
      } else {
        CSVASSERT(flg->restart_data.ox_lset_h2o.outf == ON);
        CSVASSERT(flg->restart_data.ox_lset_h2o_s.outf == ON);
      }
      if (!ox_is_enabled_component(&cdo->ox_h2)) {
        csvperrorf_row(fname, row_h2o, 0, CSV_EL_ERROR,
                       "To use steam consumption, hydrogen generation also "
                       "needs to be enabled");
        if (stat) *stat = ON;
      }
    }

    SET_P(&cdo->ox_q_fac, double, "oxide_q_factor", 1, 1.0);
    SET_P_PERROR_FINITE(cdo->ox_q_fac, ERROR, "oxide_q_factor must be finite");

    SET_P(&cdo->ox_diff_h2_sol, double, "oxide_diff_h2", 1, 0.5);
    SET_P_PERROR_FINITE(cdo->ox_diff_h2_sol, ERROR, "oxide_diff_h2 must be finite");

    SET_P_NEXT(&cdo->ox_diff_h2_srf, double, 0.5);
    SET_P_PERROR_FINITE(cdo->ox_diff_h2_srf, ERROR, "oxide_diff_h2 must be finite");

    if (flg->ox_kp_model == OX_RRMODEL_TEMPDEP) {
      enum tempdep_property_type ptype;

      SET_P(&ptype, tempdep_property_type, "oxide_kp_func", 1,
            TEMPDEP_PROPERTY_INVALID);
      if (ptype != TEMPDEP_PROPERTY_INVALID) {
        int r;
        r = tempdep_property_set(&cdo->ox_kp, ptype, csv, fname,
                                 SET_P_SOURCE_ROW(), &SET_P_SOURCE_COL());
        if (r) {
          if (stat) *stat = ON;
        }
      } else {
        if (stat) *stat = ON;
        SET_P_PERROR(ERROR, "Invalid temperature dependency function");
      }
    } else {
      tempdep_property_type ptype;
      switch(flg->ox_kp_model) {
      case OX_RRMODEL_INVALID:
        ptype = TEMPDEP_PROPERTY_INVALID;
        break;
      case OX_RRMODEL_BAKER_JUST:
        ptype = TEMPDEP_PROPERTY_OX_BAKER_JUST;
        break;
      case OX_RRMODEL_CATHCART_PAWEL:
        ptype = TEMPDEP_PROPERTY_OX_CATHCART_PAWEL;
        break;
      case OX_RRMODEL_LEISTIKOW_SCHANZ:
        ptype = TEMPDEP_PROPERTY_OX_LEISTIKOW_SCHANZ;
        break;
      case OX_RRMODEL_PRATER_COURTRIGHT:
        ptype = TEMPDEP_PROPERTY_OX_PRATER_COURTRIGHT;
        break;
      case OX_RRMODEL_URBANIC_HEIDRICK:
        ptype = TEMPDEP_PROPERTY_OX_URBANIC_HEIDRICK;
        break;
      case OX_RRMODEL_TEMPDEP:
        CSVUNREACHABLE();
        ptype = TEMPDEP_PROPERTY_INVALID;
        break;
      }
      if (ptype != TEMPDEP_PROPERTY_INVALID) {
        int r;
        /* Reaction rate model must not have user-input parameters. */
        r = tempdep_property_set(&cdo->ox_kp, ptype, csv, fname,
                                 NULL, &SET_P_SOURCE_COL());
        if (r) {
          if (stat) *stat = ON;
        }
      }
    }

    SET_P(&cdo->ox_recess_init, double, "oxide_recess_initial_delta", 1, 4.9e-5);
    if (!SET_P_PERROR_FINITE(cdo->ox_recess_init, ERROR, "oxide_recess_iniital_delta should be finite")) {
      if (stat) *stat = ON;
    } else if (cdo->ox_recess_init <= 0.0) {
      SET_P_PERROR(INFO, "Ziracloy oxidation recession will be disabled");
    }

    SET_P(&cdo->ox_recess_min, double, "oxide_recess_minimum", 1, 1.0e-9);
    if (!SET_P_PERROR_GREATER(cdo->ox_recess_min, 0.0, OFF, OFF, ERROR,
                              "Zircaloy oxidation minimum recession width must be positive")) {
      if (stat) *stat = ON;
    }

    tempdep_property_init(&cdo->ox_recess_rate);
    {
      tempdep_property_type ptyp;
      SET_P(&ptyp, tempdep_property_type, "oxide_recess_rate", 1, TEMPDEP_PROPERTY_INVALID);
      if (!SET_P_SOURCE_COL()) { /* not given */
        ptyp = TEMPDEP_PROPERTY_OX_RECESSION;
      }
      if (ptyp != TEMPDEP_PROPERTY_INVALID) {
        if (tempdep_property_set(&cdo->ox_recess_rate, ptyp, csv, fname,
                                 SET_P_SOURCE_ROW(), &SET_P_SOURCE_COL())) {
          if (stat) *stat = ON;
        }
      } else {
        if (stat) *stat = ON;
      }
    }

    SET_P(&cdo->ox_h2o_threshold, double, "oxide_h2o_threshold", 1, -1.0);
    if (!SET_P_PERROR_RANGE(cdo->ox_h2o_threshold, 0.0, 1.0, ON, ON, ERROR,
                            "oxide_h2o_threshold must be in range of 0 to 1")) {
      if(stat) *stat = ON;
    }

    SET_P(&cdo->ox_h2o_lset_min_to_recess, double, "oxide_h2o_lset_min_to_recess", 1, 0.0);
    if (!SET_P_PERROR_GREATER(cdo->ox_h2o_lset_min_to_recess, 0.0, ON, OFF,
                              ERROR, "oxide_h2o_lset_min_to_recess must be "
                              "greater than 0")) {
      if (stat) *stat = ON;
    }
  }
  cdo->ox_nox_calc = 0;
  cdo->ox_nre_calc = 0;

  //--- H2 Absorption
  if (flg->h2_absorp_eval == ON) {
    SET_P(&cdo->h2_absorp_base_p, double, "h2_absorp_base_p", 1, 101325.0);
    if (!SET_P_PERROR_GREATER(cdo->h2_absorp_base_p, 0.0, ON, OFF, ERROR,
                              "Reference pressure must be positive")) {
      if (stat) *stat = ON;
    }
  }

  //--- misc
  cnti.dest = &cdo->reference_pressure;
  SET_P(&cnti, controllable_type, "reference_pressure", 1, 0.0);
  if (!cdo->reference_pressure.exec &&
      !SET_P_PERROR_GREATER(cdo->reference_pressure.current_value,
                            0.0, ON, OFF, ERROR,
                            "Rerefence presure must be non-negative value")) {
    if (stat) *stat = ON;
  }

  //--- misc
  SET_P(&cdo->restart_dump_time, double, "restart_dump_time", 1, HUGE_VAL);
#ifdef isnan
  if (isnan(cdo->restart_dump_time)) {
    cdo->restart_dump_time = HUGE_VAL;
  }
#endif
  if (cdo->restart_dump_time < 0.0) {
    cdo->restart_dump_time = HUGE_VAL;
  }

  // 輻射反復計算用変数 (radiation == OFF のときは, どんな値でも問題ない) < 2016 Added by KKE
  SET_P(&cdo->picard_max, int, "picard_max", 1, 500);
  if (cdo->picard_max <= 0) {
    SET_P_PERROR(ERROR, "picard_max must be positive");
  }

  SET_P(&cdo->picard_out, int, "picard_out", 1, 10);
  if (cdo->picard_out <= 0) {
    SET_P_PERROR(ERROR, "picard_out must be positive");
  }

  SET_P(&cdo->newton_max, int, "newton_max", 1, 50);
  if (cdo->newton_max <= 0) {
    SET_P_PERROR(ERROR, "newton_max must be positive");
  }

  SET_P(&cdo->E_cell_err_max, double, "E_cell_err_max", 1, 1.0e-05);
  SET_P_PERROR_GREATER(cdo->E_cell_err_max, 0.0, OFF, OFF,
                       ERROR, "E_cell_err_max must be positive");

  SET_P(&cdo->tmp_cell_err_max, double, "tmp_cell_err_max", 1, 1.0e-05);
  SET_P_PERROR_GREATER(cdo->tmp_cell_err_max, 0.0, OFF, OFF,
                       ERROR, "dtmp_cell_err_max must be positive");

  SET_P(&cdo->dtmp_cell_err_max, double, "dtmp_cell_err_max", 1, 1.0e-07);
  SET_P_PERROR_GREATER(cdo->dtmp_cell_err_max, 0.0, OFF, OFF,
                       ERROR, "dtmp_cell_err_max must be positive");

  /* YSE: moved from below */
  SET_P(&cdo->nlat, int, "division_latitude", 1, ((flg->radiation == ON) ? 0 : 1));   // < 2016 Added by KKE (輻射計算用：0 ≦ θ ≦ 2π の分割数)
  if (cdo->nlat <= 0) {
    SET_P_PERROR(ERROR, "division_latitude must be positive");
  }
  SET_P(&cdo->nlon, int, "division_longitude", 1, ((flg->radiation == ON) ? 0 : 1));  // < 2016 Added by KKE (輻射計算用：0 ≦ φ ≦  π の分割数)
  if (cdo->nlon <= 0) {
    SET_P_PERROR(ERROR, "division_longitude must be positive");
  }

  /* LPT parameters */
#ifdef LPT
  SET_P(&cdo->lpt_outinterval, double, "LPT_outdt", 1, 0.0);
#else
  /*
   * Though it is no problem even if invalid value was given, but we
   * will check the value.
   */
  SET_P_PASS_NOTFOUND(&cdo->lpt_outinterval, double, "LPT_outdt", 1, 0.0);
#endif
  if (!SET_P_PERROR_GREATER(cdo->lpt_outinterval, 0.0, ON, OFF,
                            ERROR, "LPT Output interval must be 0 or greater")) {
    if (stat) *stat = ON;
  }
#ifdef LPTX
  if (SET_P_SOURCE_COL()) {
    csvperrorf_col(fname, SET_P_SOURCE_COL(), CSV_EL_WARN,
                   "In LPTX, the value to given for LPT_outdt is unused.");
  }
#endif

  {
    int ret;
    struct csv_to_FILEn_data fndata;
    fndata.filename = &cdo->lpt_outname;
    fndata.has_r = NULL;
    /*
     * Accepts both without conditionally. %r will be formatted to
     * 0, and thus other %-notation will be an error.
     */
#ifdef LPT
    ret = SET_P_NEXT(&fndata, FILEn, "LPT.dat");
#else
    ret = SET_P_NEXT_PASS_NOTFOUND(&fndata, FILEn, NULL);
#endif
    /* found and invalid, or fatal error (e.g. memory allocation failure) */
    if (((SET_P_SOURCE_COL() && ret) || ret < 0) && stat) {
      *stat = ON;
    }
  }

#ifdef LPTX
  SET_P_BASE(&cdo->lpt_cc, double, "LPT_cunningham_correction", 1, 1.0,
             flg->lpt_calc == ON && flg->lpt_use_constant_Cc == ON, 1,
             CSV_EL_WARN, CSV_EL_ERROR, NULL);
  if (!SET_P_PERROR_GREATER(cdo->lpt_cc, 0.0, OFF, OFF, ERROR,
                            "Cunningham correction must be positive")) {
    if (stat)
      *stat = ON;
  } else if (flg->lpt_use_constant_Cc != ON) {
    SET_P_PERROR(WARN, "Specified value will not be used");
  } else if (cdo->lpt_cc < 1.0) {
    SET_P_PERROR(WARN, "LPT_cunningham_correction accepts a value less than 1, "
                 "but usually should not be.");
  }
#else
  SET_P_PASS_NOTFOUND(&cdo->lpt_cc, double, "LPT_cunningham_correction", 1,
                      1.0);
#ifdef LPT
  SET_P_PERROR(WARN, "The value given LPT_cunningham_corr is used in LPT");
#endif
#endif

  {
    type def_A1, def_A2, def_A3;
    int f = (flg->lpt_calc == ON && flg->lpt_use_constant_Cc == OFF);
#ifdef LPTX
    def_A1 = LPTX_cunningham_correction_A1();
    def_A2 = LPTX_cunningham_correction_A2();
    def_A3 = LPTX_cunningham_correction_A3();
#else
    def_A1 = HUGE_VAL;
    def_A2 = HUGE_VAL;
    def_A3 = HUGE_VAL;
    f = 0;
#endif

    SET_P_BASE(&cdo->lpt_cc_A1, double, "LPT_cunningham_correction_coeffs", 1,
               def_A1, f, 1, CSV_EL_WARN, CSV_EL_ERROR, NULL);
#if defined(LPTX)
    if (!SET_P_PERROR_FINITE(cdo->lpt_cc_A1, ERROR,
                             "Cunningham correction coffs must be finite")) {
      if (stat)
        *stat = ON;
    }
#elif defined(LPT)
    SET_P_PERROR(
      WARN,
      "The value given LPT_cunnigham_currection_coeffs are not used in LPT");
#endif

    SET_P_BASE(&cdo->lpt_cc_A2, double, NULL, 1, def_A2, f, 1, CSV_EL_WARN,
               CSV_EL_ERROR, NULL);
#if defined(LPTX)
    if (!SET_P_PERROR_GREATER(cdo->lpt_cc_A2, 0.0, OFF, OFF, ERROR,
                              "Cunningham correction must be positive")) {
      if (stat)
        *stat = ON;
    }
#endif

    SET_P_BASE(&cdo->lpt_cc_A3, double, NULL, 1, def_A3, f, 1, CSV_EL_WARN,
               CSV_EL_ERROR, NULL);
#if defined(LPTX)
    if (!SET_P_PERROR_GREATER(cdo->lpt_cc_A3, 0.0, OFF, OFF, ERROR,
                              "Cunningham correction must be positive")) {
      if (stat)
        *stat = ON;
    }
#endif
  }

  {
    int f = (flg->lpt_calc == ON) && (flg->lpt_thermophoretic == ON);
    type def_Cs, def_Cm, def_Ct;
#ifdef LPTX
    def_Cs = LPTX_thermophoretic_constant_Cs();
    def_Cm = LPTX_thermophoretic_constant_Cm();
    def_Ct = LPTX_thermophoretic_constant_Ct();
#else
    def_Cs = HUGE_VAL;
    def_Cm = HUGE_VAL;
    def_Ct = HUGE_VAL;
    f = 0;
#endif

    SET_P_BASE(&cdo->lpt_tp_Cs, double, "LPT_thermophoretic_const_Cs", 1,
               def_Cs, f, 1, CSV_EL_WARN, CSV_EL_ERROR, NULL);
#if defined(LPTX)
    if (!SET_P_PERROR_FINITE(cdo->lpt_tp_Cs, ERROR,
                             "Thermophretic force coeffs must be finite")) {
      if (stat)
        *stat = ON;
    }
#elif defined(LPT)
    SET_P_PERROR(
      WARN, "The value given LPT_thermophoretic_const_Cs is not used in LPT");
#endif

    SET_P_BASE(&cdo->lpt_tp_Cm, double, "LPT_thermophoretic_const_Cm", 1,
               def_Cm, f, 1, CSV_EL_WARN, CSV_EL_ERROR, NULL);
#if defined(LPTX)
    if (!SET_P_PERROR_FINITE(cdo->lpt_tp_Cm, ERROR,
                             "Thermophretic force coeffs must be finite")) {
      if (stat)
        *stat = ON;
    }
#elif defined(LPT)
    SET_P_PERROR(
      WARN, "The value given LPT_thermophoretic_const_Cm is not used in LPT");
#endif

    SET_P_BASE(&cdo->lpt_tp_Ct, double, "LPT_thermophoretic_const_Ct", 1,
               def_Ct, f, 1, CSV_EL_WARN, CSV_EL_ERROR, NULL);
#if defined(LPTX)
    if (!SET_P_PERROR_FINITE(cdo->lpt_tp_Ct, ERROR,
                             "Thermophretic force coeffs must be finite")) {
      if (stat)
        *stat = ON;
    }
#elif defined(LPT)
    SET_P_PERROR(
      WARN, "The value given LPT_thermophoretic_const_Ct is not used in LPT");
#endif
  }

  geom_list_init(&cdo->lpt_particle_set_head.list);
  if (set_lpt_particle_sets(fname, csv, flg, cdo, &cdo->lpt_particle_set_head,
                            "LPT_particle_set", flg->lpt_calc, stat)) {
    if (stat) *stat = ON;
    return;
  }

  set_lpt_wallref(fname, csv, &cdo->lpt_wallref_xm, "LPT_wallref_xm",
                  "X-", stat);
  set_lpt_wallref(fname, csv, &cdo->lpt_wallref_xp, "LPT_wallref_xp",
                  "X+", stat);
  set_lpt_wallref(fname, csv, &cdo->lpt_wallref_ym, "LPT_wallref_ym",
                  "Y-", stat);
  set_lpt_wallref(fname, csv, &cdo->lpt_wallref_yp, "LPT_wallref_yp",
                  "Y+", stat);
  set_lpt_wallref(fname, csv, &cdo->lpt_wallref_zm, "LPT_wallref_zm",
                  "Z-", stat);
  set_lpt_wallref(fname, csv, &cdo->lpt_wallref_zp, "LPT_wallref_zp",
                  "Z+", stat);

  /* YSE: Heat source input */
  set_heat_source_param(cdo, csv, fname, comp_data_head, manager, control_head,
                        stat);

  /* YSE: Init as BOUNDARY_MPI. */
  cdo->fluid_boundary_head.cond = BOUNDARY_MPI;
  cdo->thermal_boundary_head.cond = BOUNDARY_MPI;

  /***********************************************************************************/
  {
    int npe_x, npe_y, npe_z;
    int nx, ny, nz;
    int nouts;

    npe_x = (mpi->npe_x > 0) ? mpi->npe_x : 1;
    npe_y = (mpi->npe_y > 0) ? mpi->npe_y : 1;
    npe_z = (mpi->npe_z > 0) ? mpi->npe_z : 1;

    //--- Piercing
    cdo->vof->px =  0.5*cdo->gLx;  // x-coordinates at hole center
    cdo->vof->py =  0.6*cdo->gLy; // y-coordinates at hole center
    cdo->vof->pr =  0.1*cdo->gLx;  // radius of piercing hole
    //--- Test piece
    cdo->vof->fs_zs = 0.005; // back  surface [m]
    cdo->vof->fs_ze = 0.055; // front surface [m]
    //--- Gravity
    /* YSE: Moved to above. */
    //--- Local geometry & resolution
    /* YSE: Set boundary domain parameter */
    cdo->stmb = 1;
    cdo->stpb = 1;
    cdo->nbx = cdo->stmb + cdo->nx + cdo->stpb;
    cdo->nby = cdo->stmb + cdo->ny + cdo->stpb;
    cdo->nbz = cdo->stmb + cdo->nz + cdo->stpb;
    if (cdo->nbx < 1) cdo->nbx = 1;
    if (cdo->nby < 1) cdo->nby = 1;
    if (cdo->nbz < 1) cdo->nbz = 1;
    /* YSE: Division of angle (nlat, nlon) is moved to above */
    cdo->Lx = cdo->gLx/((type)npe_x); // spatial domain(x)[m]/PEx
    cdo->Ly = cdo->gLy/((type)npe_y); // spatial domain(y)[m]/PEy
    cdo->Lz = cdo->gLz/((type)npe_z); // spatial domain(z)[m]/PEz
    // local
    cdo->nxy= cdo->nx*cdo->ny;
    cdo->n  = cdo->nx*cdo->ny*cdo->nz;
    cdo->mx = cdo->stm + cdo->nx + cdo->stp;
    cdo->my = cdo->stm + cdo->ny + cdo->stp;
    cdo->mz = cdo->stm + cdo->nz + cdo->stp;
    if (cdo->mx < 1) cdo->mx = 1;
    if (cdo->my < 1) cdo->my = 1;
    if (cdo->mz < 1) cdo->mz = 1;
    cdo->mxy= cdo->mx*cdo->my;
    cdo->m  = cdo->mx*cdo->my*cdo->mz;
    // global
    cdo->gn  = cdo->gnx*cdo->gny*cdo->gnz;
    cdo->gmx = cdo->stm + cdo->gnx + cdo->stp;
    cdo->gmy = cdo->stm + cdo->gny + cdo->stp;
    cdo->gmz = cdo->stm + cdo->gnz + cdo->stp;
    if (cdo->gmx < 1) cdo->gmx = 1;
    if (cdo->gmy < 1) cdo->gmy = 1;
    if (cdo->gmz < 1) cdo->gmz = 1;
    cdo->gm  = cdo->gmx*cdo->gmy*cdo->gmz;
    cdo->iout = 0;
    cdo->dtout= (cdo->outs > 0) ? (cdo->tend - cdo->time)/cdo->outs : 0.0;
    cdo->tout = (cdo->dtout <= 0.0) ? HUGE_VAL : cdo->time;

    /* Following setting has been moved to init_mesh() */
    // nx = (cdo->nx > 0) ? cdo->nx : 1;
    // ny = (cdo->ny > 0) ? cdo->ny : 1;
    // nz = (cdo->nz > 0) ? cdo->nz : 1;
    // cdo->dx = cdo->Lx/nx;  cdo->dxi = 1.0/cdo->dx;
    // cdo->dy = cdo->Ly/ny;  cdo->dyi = 1.0/cdo->dy;
    // cdo->dz = cdo->Lz/nz;  cdo->dzi = 1.0/cdo->dz;
    // cdo->width = 3.0*cdo->dx;// surface width (for surface tension)
  }
  //--- Oxidation
  /* YSE: Moved to above */

  //--- Solver setting
  if (cdo->nx > 0 && cdo->ny > 0 && cdo->nz > 0) {
    int defx, defy, defz;
    int nx, ny, nz;

    set_solver_block_size(&cdo->solver_p_block_size, 1, cdo->ny, cdo->nz,
                          cdo->nx, cdo->ny, cdo->nz, fname, csv,
                          "solver_block_size", 1,
                          &SET_P_SOURCE_COL(), &SET_P_SOURCE_ROW(), stat);

    nx = cdo->nx;
    ny = cdo->ny;
    nz = cdo->nz;
    if (mpi->rank_x == mpi->npe_x - 1) {
      nx += 1;
    }
    defx = 1;
    defy = ny;
    defz = nz;
    solver_block_size_modify_default(&cdo->solver_p_block_size, &defx, &defy,
                                     &defz, cdo->nx, cdo->ny, cdo->nz,
                                     nx, ny, nz);
    set_solver_block_size(&cdo->solver_u_block_size, defx, defy, defz, nx, ny,
                          nz, fname, csv, "solver_block_size_U", 1,
                          &SET_P_SOURCE_COL(), &SET_P_SOURCE_ROW(), stat);

    nx = cdo->nx;
    ny = cdo->ny;
    nz = cdo->nz;
    if (mpi->rank_y == mpi->npe_y - 1) {
      ny += 1;
    }
    defx = 1;
    defy = ny;
    defz = nz;
    solver_block_size_modify_default(&cdo->solver_p_block_size, &defx, &defy,
                                     &defz, cdo->nx, cdo->ny, cdo->nz,
                                     nx, ny, nz);
    set_solver_block_size(&cdo->solver_v_block_size, defx, defy, defz, nx, ny,
                          nz, fname, csv, "solver_block_size_V", 1,
                          &SET_P_SOURCE_COL(), &SET_P_SOURCE_ROW(), stat);

    nx = cdo->nx;
    ny = cdo->ny;
    nz = cdo->nz;
    if (mpi->rank_z == mpi->npe_z - 1) {
      nz += 1;
    }
    defx = 1;
    defy = ny;
    defz = nz;
    solver_block_size_modify_default(&cdo->solver_p_block_size, &defx, &defy,
                                     &defz, cdo->nx, cdo->ny, cdo->nz,
                                     nx, ny, nz);
    set_solver_block_size(&cdo->solver_w_block_size, defx, defy, defz, nx, ny,
                          nz, fname, csv, "solver_block_size_W", 1,
                          &SET_P_SOURCE_COL(), &SET_P_SOURCE_ROW(), stat);
  }
}

/* Heat source set function */
static void set_heat_source_param(domain *cdo, csv_data *csv, const char *fname,
                                  component_data *comp_data_head,
                                  jcntrl_executive_manager *manager,
                                  controllable_type *control_head, int *stat)
{
  csv_row *row;
  csv_column *col;
  struct component_info_data d;
  heat_source_param *hp;
  SET_P_INIT(csv, fname, &row, &col);

  CSVASSERT(cdo);

  if (!csv) return;

  row = findCSVRow(csv, "heat_source", strlen("heat_source"));
  heat_source_param_init(&cdo->heat_sources_head);

  for (; row; row = findCSVRowNext(row)) {
    component_phases ph;
    int dup;
    heat_source_param p;
    struct csv_to_component_info_data_data cidd = {
      .comp_data_head = comp_data_head,
    };

    col = getColumnOfCSV(row, 0);
    if (!col) break;

    heat_source_param_init(&p);
    hp = heat_source_param_new();
    if (!hp) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      hp = &p;
    }

    cidd.dest = &hp->comp;
    SET_P_NEXT(&cidd, component_info_data, JUPITER_ID_INVALID);
    if (hp->comp.d) {
      dup = 0;
      if (heat_source_param_find_by_data(&cdo->heat_sources_head, hp->comp.d)) {
        SET_P_PERROR(WARN, "Multiple heat source data given for ID %d",
                     hp->comp.d->jupiter_id);
        dup = 1;
      }
    } else {
      if (stat) *stat = ON;
    }

    ph = component_phases_none();
    if (hp->comp.d) {
      ph = hp->comp.d->phases;
    } else {
      if (hp->comp.id != -1) {
        component_phases_set(&ph, COMPONENT_PHASE_GAS, 1);
      } else {
        component_phases_set(&ph, COMPONENT_PHASE_SOLID, 1);
        component_phases_set(&ph, COMPONENT_PHASE_LIQUID, 1);
      }
    }

    SET_P_NEXT(&hp->control, trip_control, TRIP_CONTROL_INVALID);
    if (hp->control == TRIP_CONTROL_CONST) {
      controllable_type_init(&hp->q_s);
      controllable_type_init(&hp->q_l);
      // First entry is always solid or gas even if the corresponding phase
      // does not exist.
      SET_P_NEXT(&hp->q_s.current_value, exact_double, 0.0);
      if (!SET_P_PERROR_FINITE(hp->q_s.current_value, ERROR,
                               "Heat source must be finite value")) {
        if (stat)
          *stat = ON;
      }
      if (component_phases_has_liquid(ph)) {
        SET_P_NEXT(&hp->q_l.current_value, exact_double, 0.0);
        if (!SET_P_PERROR_FINITE(hp->q_l.current_value, ERROR,
                                 "Heat source must be finite value")) {
          if (stat)
            *stat = ON;
        }
      }
    } else if (hp->control == TRIP_CONTROL_CONTROL) {
      struct csv_to_controllable_type_data cset;
      cset.manager = manager;
      cset.head = control_head;

      cset.dest = &hp->q_s;
      SET_P_NEXT(&cset, controllable_type, 0.0);

      cset.dest = &hp->q_l;
      if (component_phases_has_liquid(ph)) {
        SET_P_NEXT(&cset, controllable_type, 0.0);
      }
    }

    if (hp != &p)
      geom_list_insert_prev(&cdo->heat_sources_head.list, &hp->list);
    heat_source_param_clean(&p);
  }
}

// Laser parameter set function
void set_laser(laser *lsr, const char *fname, csv_data *csv, int *stat){
  SET_P_INIT_NOLOC(csv, fname);

  CSVASSERT(lsr);

  SET_P(&lsr->pw0, double, "laser_power", 1, 0.0);  // power[W]

  SET_P(&lsr->r0, double, "irrad_radius", 1, 0.001);  // irradiation radius[m]
  if (!SET_P_PERROR_GREATER(lsr->r0, 0.0, OFF, OFF, ERROR, "Irradiation radius must be positive")) {
    if (stat) *stat = ON;
  }

  lsr->qm = (lsr->pw0)/(M_PI*(lsr->r0)*(lsr->r0)); //heat flux[W m-2]

  SET_P(&lsr->lambda, double, "wave_langth", 1, 1.0e-6);  // wave length[W]
  if (!SET_P_PERROR_GREATER(lsr->lambda, 0.0, OFF, OFF, ERROR, "Wave length must be positive")) {
    if (stat) *stat = ON;
  }

  SET_P(&lsr->R, double, "reflectivity", 1, 0.52);  // reflectivity[-]
  if (!SET_P_PERROR_RANGE(lsr->R, 0.0, 1.0, ON, OFF, ERROR, "reflectivity must be between 0 and 1 (but not include 1).")) {
    if(lsr->R >= 1.0){
      lsr->R = 1.0;
    }
    if(lsr->R < 0.0){
      lsr->R = 0.0;
    }
  }

  SET_P(&lsr->alpha, double, "absorptivity", 1, 7.2e+7);  // absorptivity[m-1]
  if (!SET_P_PERROR_GREATER(lsr->alpha, 0.0, OFF, OFF, ERROR, "Apsorptivity must be positive")) {
    if (stat) *stat = ON;
  }
  
  SET_P(&lsr->lsr_x, double, "nozzle_position", 1, 0.0);  // position x[m]
  SET_P_NEXT(&lsr->lsr_y, double, 0.0);  // position y[m]

  SET_P(&lsr->swp_vel, double, "sweep_velocity", 1, 0.0);  //sweep velocity [m s-1]

}

static enum solid_form get_default_solid_form(flags *flg)
{
  if (!flg)
    return SOLID_FORM_INVALID;

  if (flg->IBM == ON) {
    if (flg->porous == ON)
      return SOLID_FORM_INVALID; /* no default */
    return SOLID_FORM_IBM;
  }
  if (flg->porous == ON)
    return SOLID_FORM_POROUS;
  return SOLID_FORM_UNUSED; /* no solid form */
}

/* YSE: Adjust parameters to use CSV. */
void set_property(int i, phase_value_component *f, flags *flg,
                  csv_data *csv, const char *fname, int *stat)
{
  csv_row *row;
  csv_column *col;
  tempdep_property_type tdep_type;
  int ret;
  SET_P_INIT(csv, fname, &row, &col);

  CSVASSERT(f);
  CSVASSERT(flg);
  CSVASSERT(csv);
  CSVASSERT(fname);

  SET_P(&f->tm_soli, double, "tm_soli", 1, 0.0);
  if (!SET_P_PERROR_FINITE(f->tm_soli, ERROR, "tm_soli must be finite")) {
    if (stat) *stat = ON;
  }

  SET_P(&f->tm_liq, double, "tm_liq", 1, 0.0);
  if (!SET_P_PERROR_FINITE(f->tm_liq, ERROR, "tm_liq must be finite")) {
    if (stat) *stat = ON;
  }

  SET_P(&f->tb, double, "tb", 1, 0.0);
  if (!SET_P_PERROR_FINITE(f->tb, ERROR, "tb must be finite")) {
    if (stat) *stat = ON;
  }

  SET_P(&f->lh, double, "lh", 1, 0.0);
  if (!SET_P_PERROR_FINITE(f->lh, ERROR, "lh must be finite")) {
    if (stat) *stat = ON;
  }

  SET_P(&f->lhv, double, "lhv", 1, 0.0);
  if (!SET_P_PERROR_FINITE(f->lhv, ERROR, "lhv must be finite")) {
    if (stat) *stat = ON;
  }

  /*added by Chai*/
  SET_P(&f->molar_mass, double, "molar_mass", 1, 0.0);
  if (!SET_P_PERROR_FINITE(f->molar_mass, ERROR, "molar mass must be finite")) {
    if (stat) *stat = ON;
  }

  SET_P(&f->sform, solid_form, "solid_form", 1, get_default_solid_form(flg));
  if (f->sform == SOLID_FORM_INVALID) {
    SET_P_PERROR(ERROR, "Invalid model is specified");
    if (stat)
      *stat = ON;
  } else {
    if (flg->IBM == ON || flg->porous == ON) {
      if ((flg->IBM != ON && f->sform == SOLID_FORM_IBM) ||
          (flg->porous != ON && f->sform == SOLID_FORM_POROUS)) {
        SET_P_PERROR(ERROR, "Specified model is not enabled");
        if (stat)
          *stat = ON;
      }
    } else if (f->sform != SOLID_FORM_UNUSED) {
      SET_P_PERROR(WARN, "No solid model is enabled");
      f->sform = SOLID_FORM_UNUSED;
    }
  }

  SET_P(&f->poros, double, "porosity", 1, 0.0);
  if (f->sform == SOLID_FORM_POROUS) {
    if (!SET_P_PERROR_GREATER(f->poros, 0.0, OFF, OFF, ERROR,
                              "porosity must be greater than 0 (and finite)")) {
      if (stat)
        *stat = ON;
    }
  }

  SET_P(&f->permea, double, "permeability", 1, 0.0);
  if (f->sform == SOLID_FORM_POROUS) {
    if (!SET_P_PERROR_GREATER(
          f->permea, 0.0, OFF, OFF, ERROR,
          "permeability must be greater than 0 (and finite)")) {
      if (stat)
        *stat = ON;
    }
  }

  SET_P(&tdep_type, tempdep_property_type, "beta", 1, 0.0);
  ret = tempdep_property_set(&f->beta, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "sigma", 1, 0.0);
  ret = tempdep_property_set(&f->sigma, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "rho_s", 1, 0.0);
  ret = tempdep_property_set(&f->rho_s, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "rho_l", 1, 0.0);
  ret = tempdep_property_set(&f->rho_l, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "emi_s", 1, 0.0);
  ret = tempdep_property_set(&f->emi_s, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "emi_l", 1, 0.0);
  ret = tempdep_property_set(&f->emi_l, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "emi_g", 1, 0.0);
  ret = tempdep_property_set(&f->emi_g, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "mu_s", 1, 0.0);
  ret = tempdep_property_set(&f->mu_s, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "mu_l", 1, 0.0);
  ret = tempdep_property_set(&f->mu_l, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "specht_s", 1, 0.0);
  ret = tempdep_property_set(&f->specht_s, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "specht_l", 1, 0.0);
  ret = tempdep_property_set(&f->specht_l, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "thc_s", 1, 0.0);
  ret = tempdep_property_set(&f->thc_s, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "thc_l", 1, 0.0);
  ret = tempdep_property_set(&f->thc_l, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "radf", 1, 0.0);
  ret = tempdep_property_set(&f->radf, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;
}

/* YSE: Set physical properties for Gaseous-only phase */
void set_property_g(int i, phase_value_component *f, flags *flg,
                    csv_data *csv, const char *fname, int *stat)
{
  csv_row *row;
  csv_column *col;
  tempdep_property_type tdep_type;
  int ret;
  SET_P_INIT(csv, fname, &row, &col);

  CSVASSERT(f);
  CSVASSERT(flg);
  CSVASSERT(csv);
  CSVASSERT(fname);

  f->sform = SOLID_FORM_UNUSED;

  SET_P(&f->molar_mass, double, "molar_mass", 1, 0.0);
  if (!SET_P_PERROR_GREATER(f->molar_mass, 0.0, OFF, OFF, ERROR,
                            "molar_mass must be positive")) {
    if (stat) *stat = ON;
  }

  SET_P(&f->lhv, double, "lhv", 1, 0.0);
  if (!SET_P_PERROR_GREATER(f->lhv, 0.0, OFF, OFF, ERROR,
                            "lhv must be positive")) {
    if (stat) *stat = ON;
  }

  SET_P(&tdep_type, tempdep_property_type, "rho_g", 1,
        TEMPDEP_PROPERTY_INVALID);
  ret = tempdep_property_set(&f->rho_g, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "specht_g", 1,
        TEMPDEP_PROPERTY_INVALID);
  ret = tempdep_property_set(&f->specht_g, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "thc_g", 1,
        TEMPDEP_PROPERTY_INVALID);
  ret = tempdep_property_set(&f->thc_g, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "mu_g", 1,
        TEMPDEP_PROPERTY_INVALID);
  ret = tempdep_property_set(&f->mu_g, tdep_type,
                             csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "saturated_pressure", 1,
        TEMPDEP_PROPERTY_INVALID);
  ret = tempdep_property_set(&f->saturated_pressure, tdep_type, csv, fname,
                             row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "condensation_E", 1,
        TEMPDEP_PROPERTY_INVALID);
  ret = tempdep_property_set(&f->condensation_E, tdep_type, csv, fname,
                             row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "sigma", 1,
        TEMPDEP_PROPERTY_INVALID);
  ret = tempdep_property_set(&f->sigma, tdep_type, csv, fname, row, &col);
  if (ret && stat) *stat = ON;
}

void set_property_gm1(phase_value *phv, flags *flg, csv_data *csv,
                      const char *fname, int *stat)
{
  int ret;
  tempdep_property_type tdep_type;
  csv_row *row;
  csv_column *col;
  SET_P_INIT(csv, fname, &row, &col);

  SET_P(&tdep_type, tempdep_property_type, "rho_g", 1,
        TEMPDEP_PROPERTY_INVALID);
  ret = tempdep_property_set(&phv->rho_g, tdep_type, csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "mu_g", 1,
        TEMPDEP_PROPERTY_INVALID);
  tempdep_property_set(&phv->mu_g, tdep_type, csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "specht_g", 1,
        TEMPDEP_PROPERTY_INVALID);
  tempdep_property_set(&phv->specht_g, tdep_type, csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&tdep_type, tempdep_property_type, "thc_g", 1,
        TEMPDEP_PROPERTY_INVALID);
  tempdep_property_set(&phv->thc_g, tdep_type, csv, fname, row, &col);
  if (ret && stat) *stat = ON;

  SET_P(&phv->molar_mass_g, double, "molar_mass", 1, 0.0);
  if (!SET_P_PERROR_GREATER(phv->molar_mass_g, 0.0, 0, 0,
                            ERROR, "Molar mass must be positive")) {
    if (stat) *stat = ON;
  }
}

/* YSE: Set Liquidus/Solidus Tables */
/**
 * @brief tm_table_usage list (Use with array of Material ID)
 */
struct tm_table_usage
{
  struct geom_list list;      /*!< list */
  struct tm_table_param *use; /*!< Table parameter data which is used */
  csv_column *csv_pos;        /*!< CSV Column where is set by input file */
  const component_data *comp; /*!< Component */
};
#define tm_table_usage_entry(ptr) \
  geom_list_entry(ptr, struct tm_table_usage, list)

static void tm_table_usage_init(struct tm_table_usage *p)
{
  geom_list_init(&p->list);
  p->use = NULL;
  p->csv_pos = NULL;
  p->comp = NULL;
}

static struct tm_table_usage *tm_table_usage_new(void)
{
  struct tm_table_usage *p;
  p = (struct tm_table_usage *)malloc(sizeof(struct tm_table_usage));
  if (!p)
    return NULL;

  tm_table_usage_init(p);
  return p;
}

static void tm_table_usage_delete(struct tm_table_usage *p)
{
  geom_list_delete(&p->list);
  free(p);
}

static void tm_table_usage_delete_all(struct tm_table_usage *tlist)
{
  struct geom_list *lp, *ln, *lh;
  lh = &tlist->list;
  geom_list_foreach_safe (lp, ln, lh) {
    struct tm_table_usage *p = tm_table_usage_entry(lp);
    tm_table_usage_delete(p);
  }
}

static struct tm_table_usage *tm_table_usage_add(struct tm_table_usage *tlist,
                                                 struct tm_table_param *use,
                                                 csv_column *col,
                                                 const component_data *comp)
{
  struct tm_table_usage *p;
  p = tm_table_usage_new();
  if (!p)
    return NULL;

  p->comp = comp;
  p->csv_pos = col;
  p->comp = comp;
  geom_list_insert_prev(&tlist->list, &p->list);
  return p;
}

static struct tm_table_usage *tm_table_usage_find(struct tm_table_usage *tlist,
                                                  const component_data *d)
{
  struct geom_list *lp, *lh;
  lh = &tlist->list;
  geom_list_foreach(lp, lh) {
    struct tm_table_usage *p = tm_table_usage_entry(lp);
    if (p->comp == d)
      return p;
  }
  return NULL;
}

static void set_tm_tables_check_id(
  struct tm_table_param *p, component_phases phases, int allow_n1_null,
  struct component_info_data *id_to_test, const char *id_for,
  struct tm_table_usage *tlist, const char *fname, csv_data *csv,
  csv_row *found_row, csv_column *found_col, int *stat)
{
  component_phases ph;
  struct tm_table_usage *pp;
  long llo, cco, ll, cc;
  SET_P_INIT(csv, fname, &found_row, &found_col);

  CSVASSERT(p);
  CSVASSERT(id_to_test);
  CSVASSERT(id_for);
  CSVASSERT(tlist);
  CSVASSERT(found_row);
  CSVASSERT(csv);
  CSVASSERT(fname);

  if (allow_n1_null) {
    if (!id_to_test->d || id_to_test->d->jupiter_id == -1) {
      id_to_test->d = NULL;
      id_to_test->id = -1;
      return;
    }
  } else {
    if (!id_to_test->d) {
      if (stat)
        *stat = ON;
      return;
    }
  }

  CSVASSERT(id_to_test->d);
  ph = id_to_test->d->phases;
  if (!component_phases_eql(component_phases_band(ph, phases), phases)) {
    for (int ib = 0; ib < COMPONENT_PHASE_MAX; ++ib) {
      if (component_phases_get(phases, ib) && !component_phases_get(ph, ib)) {
        csvperrorf_col(fname, found_col, CSV_EL_ERROR,
                       "This material does not exist in %s phase, which is "
                       "required for here",
                       PP_component_phase_value_format_v(ib));
        break;
      }
    }
    if (stat)
      *stat = ON;
  }

  if ((pp = tm_table_usage_find(tlist, id_to_test->d))) {
    int llo, cco;
    llo = getCSVTextLineOrigin(pp->csv_pos);
    cco = getCSVTextColumnOrigin(pp->csv_pos);
    csvperrorf_col(fname, found_col, CSV_EL_ERROR,
                   "This material ID is already defined to use table %s,"
                   "defined at line %ld and column %ld",
                   pp->use->table_file, llo, cco);
    if (stat)
      *stat = ON;
  }

  if (!tm_table_usage_add(tlist, p, found_col, id_to_test->d)) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    if (stat)
      *stat = ON;
  }
}

static
void set_tm_tables(struct tm_table_param **param, component_phases phases,
                   struct component_data *comp_data_head, const char *fname,
                   csv_data *csv, csv_row *start_row, int *stat)
{
  csv_error r;
  csv_column *col;
  csv_row *row;
  table_error te;
  int *nlist;
  struct tm_table_param *p, *lst, *st;
  const char *keyn;
  char *tabf;
  struct tm_table_usage tlist;
  struct csv_to_component_info_data_data cidd = {
    .comp_data_head = comp_data_head,
  };

  SET_P_INIT(csv, fname, &row, &col);
  tm_table_usage_init(&tlist);

  CSVASSERT(param);

  st = NULL;
  nlist = NULL;
  p = NULL;
  lst = NULL;
  row = start_row;

  r = CSV_ERR_SUCC;
  for (; row; row = findCSVRowNext(row)) {
    p = (struct tm_table_param *)calloc(sizeof(struct tm_table_param), 1);
    if (!p) {
      r = CSV_ERR_NOMEM;
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, __func__, r, 0, 0, NULL);
      break;
    }
    p->next = NULL;

    p->table = table_alloc();
    if (!p->table) {
      r = CSV_ERR_NOMEM;
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, __func__, r, 0, 0, NULL);
      free(p);
      p = NULL;
      break;
    }

    table_set_interp_mode(p->table, TABLE_INTERP_BARYCENTRIC);
    table_set_algorithm(p->table, TABLE_SALG_BIN_TREE_MINMAX);

    col = getColumnOfCSV(row, 0);
    CSVASSERT(col);

    keyn = getCSVValue(col);

    SET_P_NEXT(&tabf, charp, NULL);
    if (!tabf) {
      SET_P_PERROR(ERROR, "Table file for %s must not be stdin", keyn);
    } else {
      te = table_read_binary(p->table, tabf);
      p->table_file = tabf;
      if (!p->table_file) {
        if (te == TABLE_SUCCESS) {
          te = TABLE_ERR_NOMEM;
        }
      }
      if (te != TABLE_SUCCESS) {
        int ll, cc;
        ll = getCSVTextLineOrigin(col);
        cc = getCSVTextColumnOrigin(col);
        if (te == TABLE_ERR_SYS) {
          csvperror(fname, ll, cc, CSV_EL_ERROR, p->table_file,
                    CSV_ERR_SYS, errno, 0, NULL);
        } else {
          csvperrorf(fname, ll, cc, CSV_EL_ERROR, p->table_file,
                     "%s", table_errorstr(te));
        }
        table_free(p->table);
        p->table = NULL;
      }
    }

    cidd.dest = &p->rid;
    SET_P_NEXT_REQUIRED(&cidd, component_info_data, JUPITER_ID_INVALID, stat);
    set_tm_tables_check_id(p, phases, 0, &p->rid, "Remainder", &tlist, fname,
                           csv, row, col, stat);

    cidd.dest = &p->xid;
    SET_P_NEXT_REQUIRED(&cidd, component_info_data, JUPITER_ID_INVALID, stat);
    set_tm_tables_check_id(p, phases, 0, &p->xid, "X", &tlist, fname,
                           csv, row, col, stat);

    cidd.dest = &p->yid;
    SET_P_NEXT_PASS_NOTFOUND(&cidd, component_info_data, -1);
    set_tm_tables_check_id(p, phases, 1, &p->yid, "Y", &tlist, fname,
                           csv, row, col, stat);

    if (p->yid.d && p->table &&
        table_get_geometry(p->table) != TABLE_GEOMETRY_SUM_CONSTANT) {
      csvperrorf_row(fname, row, 1, CSV_EL_ERROR,
                     "Table file must be sum-constant map data");
      if (stat) *stat = ON;
    }

    if (!st) st = p;
    if (lst) lst->next = p;
    lst = p;
  }

  tm_table_usage_delete_all(&tlist);
  if (st) *param = st;
}

static void set_tm_funcs(struct tm_func2_param **param, component_phases phases,
                         component_data *comp_data_head, const char *fname,
                         csv_data *csv, csv_row *start_row, int *stat)
{
  csv_row *row;
  csv_column *col;
  struct tm_func2_param *st;
  struct tm_func2_param *p, *n;
  const char *keyn;
  csv_error r;
  struct csv_to_component_info_data_data cidd = {
    .comp_data_head = comp_data_head,
  };
  SET_P_INIT(csv, fname, &row, &col);

  CSVASSERT(param);

  n = NULL;
  st = NULL;
  row = start_row;
  for (; row; row = findCSVRowNext(row)) {
    p = (struct tm_func2_param *)calloc(sizeof(struct tm_func2_param), 1);
    if (!p) {
      r = CSV_ERR_NOMEM;
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, __func__, r, 0, 0, NULL);
      if (stat)
        *stat = ON;
      break;
    }
    if (n) {
      n->next = p;
    }
    if (!st) {
      st = p;
    }
    p->next = NULL;
    col = getColumnOfCSV(row, 0);
    CSVASSERT(col);

    keyn = getCSVValue(col);

    SET_P_NEXT(&p->model, tm_func2_model, TM_FUNC_INVALID);

    cidd.dest = &p->yid;
    SET_P_NEXT(&cidd, component_info_data, JUPITER_ID_INVALID);
    if (p->yid.d) {
      if (p->yid.d->jupiter_id == -1) {
        p->yid.d = NULL;
        p->yid.id = -1;
      }
    } else if (p->yid.id != -1) {
      if (stat)
        *stat = ON;
    }

    cidd.dest = &p->xid;
    SET_P_NEXT(&cidd, component_info_data, JUPITER_ID_INVALID);
    if (!p->xid.d) {
      if (stat)
        *stat = ON;
    }
    n = p;
  }

  *param = st;
}

typedef int set_solute_diffc_getid(struct dc_calc_param_input *ip,
                                   const struct component_info_data *id,
                                   void *arg);

/**
 * returned array points the struct dc_calc_param data in given list.
 */
void set_solute_diffc_build_params_base(
  struct dc_calc_param_input *iparam_head, int ncompo, int commutative,
  set_solute_diffc_getid *getid, void *arg, struct dc_calc_param ***params,
  ptrdiff_t *nparam, int *stat)
{
  struct geom_list *lp, *lh;
  ptrdiff_t np;
  struct dc_calc_param **p;

  if (commutative) {
    np = dc_calc_binary_size_commutative(ncompo);
  } else {
    np = dc_calc_binary_size(ncompo);
  }

  if (np <= 0) {
    *nparam = np;
    *params = NULL;
    return;
  }

  p = (struct dc_calc_param **)calloc(sizeof(struct dc_calc_param *), np);
  if (!p) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    if (stat)
      *stat = ON;
    *nparam = 0;
    *params = NULL;
    return;
  }

  lh = &iparam_head->list;
  geom_list_foreach (lp, lh) {
    int idbase, iddiff;
    ptrdiff_t idx;
    struct dc_calc_param_input *ip;

    ip = dc_calc_param_input_entry(lp);
    if (ip->missing)
      continue; /* Ignores missing entries */

    iddiff = getid(ip, &ip->diffusing.id, arg);
    idbase = getid(ip, &ip->base.id, arg);
    if (commutative) {
      idx = dc_calc_binary_address_commutative(iddiff, idbase, ncompo);
    } else {
      idx = dc_calc_binary_address(iddiff, idbase, ncompo);
    }
    if (idx < 0)
      continue; /* Ignores invalid entries */

    if (p[idx]) {
      csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, NULL,
                 "[dev] Diffusivity definition for (diff, base) = (%d, %d) "
                 "is duplicated. Please program to check dupilcation when "
                 "adding entries (don't rely on this error).",
                 iddiff, idbase);
      if (stat)
        *stat = ON;
    } else {
      p[idx] = &ip->data;
    }
  }

  *nparam = np;
  *params = p;
}

typedef int read_solute_diffc_iderr(struct dc_calc_param_input *ip,
                                    struct dc_calc_input_idinfo *idinfo,
                                    const char *fname, void *arg);

static void read_solute_diffc_id(struct dc_calc_param_input *ip,
                                 struct dc_calc_input_idinfo *idinfo,
                                 struct component_data *comp_data_head,
                                 read_solute_diffc_iderr *iderr, void *arg,
                                 const char *fname, csv_data *csv,
                                 csv_row **row, csv_column **col, int *stat)
{
  struct csv_to_component_info_data_data cidd = {
    .comp_data_head = comp_data_head,
  };

  SET_P_INIT(csv, fname, row, col);

  cidd.dest = &idinfo->id;
  SET_P_NEXT(&cidd, component_info_data, JUPITER_ID_INVALID);
  idinfo->column = *col;
  idinfo->row = *row;

  if (iderr && idinfo->id.d) {
    if (!iderr(ip, idinfo, fname, arg)) {
      if (stat)
        *stat = ON;

      idinfo->id.id = idinfo->id.d->jupiter_id;
      idinfo->id.d = NULL;
    }
  }
}

static int solute_diffc_id_eql(struct dc_calc_input_idinfo *id1,
  struct dc_calc_input_idinfo *id2)
{
  if (id1->id.d && id2->id.d)
    return id1->id.d == id2->id.d;
  return 0;
}

static int solute_diffc_ideql(struct dc_calc_param_input *ip1,
                              struct dc_calc_param_input *ip2,
                              int commutative)
{
  if (solute_diffc_id_eql(&ip1->base, &ip2->base) &&
      solute_diffc_id_eql(&ip1->diffusing, &ip2->diffusing))
    return 1;
  if (commutative) {
    if (solute_diffc_id_eql(&ip1->base, &ip2->diffusing) &&
        solute_diffc_id_eql(&ip1->diffusing, &ip2->base))
      return 1;
  }
  return 0;
}

/**
 * @brief Read solute diffusivity ID data
 * @param iparam_head Head pointer of input data set.
 * @param comp_data_head List of available components
 * @param fname Filename of CSV
 * @param csv CSV data
 * @param start_row Pointer to a row to start reading from
 * @param stat Sets ON if any errors occured.
 */
static void
read_solute_diffc_funcs_base(struct dc_calc_param_input *iparam_head,
                             struct component_data *comp_data_head,
                             read_solute_diffc_iderr *iderr, void *arg,
                             int commutative, const char *fname, csv_data *csv,
                             csv_row *start_row, int *stat)
{
  csv_row *row;
  csv_column *col;
  struct dc_calc_param_input *ip;
  struct csv_to_component_info_data_data cidd = {
    .comp_data_head = comp_data_head,
  };

  SET_P_INIT(csv, fname, &row, &col);

  CSVASSERT(iparam_head);
  CSVASSERT(fname);
  CSVASSERT(csv);
  CSVASSERT(start_row);

  row = start_row;
  for (; row; row = findCSVRowNext(row)) {
    int ok;
    tempdep_property_type tdep_type;

    ip = (struct dc_calc_param_input *)
      calloc(sizeof(struct dc_calc_param_input), 1);
    if (!ip) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM,
                0, 0, NULL);
      if (stat) *stat = ON;
      break;
    }

    col = getColumnOfCSV(row, 0);

    read_solute_diffc_id(ip, &ip->diffusing, comp_data_head, iderr, arg, fname,
                         csv, &row, &col, stat);
    read_solute_diffc_id(ip, &ip->base, comp_data_head, iderr, arg, fname, csv,
                         &row, &col, stat);

    if (solute_diffc_id_eql(&ip->base, &ip->diffusing)) {
      SET_P_PERROR(ERROR, "Given diffusing and base IDs are equal");
      if (stat)
        *stat = ON;
    } else if (ip->diffusing.id.d && ip->base.id.d) {
      struct geom_list *lp, *lh;
      lh = &iparam_head->list;
      geom_list_foreach (lp, lh) {
        struct dc_calc_param_input *ipp;
        ipp = dc_calc_param_input_entry(lp);
        if (solute_diffc_ideql(ip, ipp, commutative)) {
          if (ipp->base.column) {
            SET_P_PERROR(
              ERROR, "Given ID combination is already specified at line %ld",
              getCSVTextLineOrigin(ipp->base.column));
          } else {
            SET_P_PERROR(ERROR, "Given ID combination is already specified");
          }
          break;
        }
      }
    }

    ok = 1;
    tdep_type = TEMPDEP_PROPERTY_INVALID;
    SET_P_NEXT(&ip->data.model, dc_func2_model, DC_FUNCS_INVALID);
    if (ip->data.model == DC_FUNCS_INVALID) {
      ok = 0;
    } else {
      if (ip->data.model == DC_FUNCS_TEMPDEP_PROPERTY) {
        SET_P_CURRENT(&tdep_type, tempdep_property_type,
                      TEMPDEP_PROPERTY_INVALID);
        if (tdep_type == TEMPDEP_PROPERTY_INVALID)
          ok = 0;
      }
    }

    tempdep_property_set(&ip->data.prop, tdep_type, csv, fname,
                         SET_P_SOURCE_ROW(), &SET_P_SOURCE_COL());
    if (!ok) {
      if (stat)
        *stat = ON;
    }

    geom_list_insert_prev(&iparam_head->list, &ip->list);
  }
}

static int add_solute_diffc_compn_sort1(const struct component_info_data *a,
                                        const struct component_info_data *b,
                                        void *arg)
{
  if (a->d && b->d) {
    ptrdiff_t p = a->d - b->d;
    if (p < 0)
      return -1;
    if (p > 0)
      return 1;
    return 0;
  } else if (a->d) {
    return -1;
  } else if (b->d) {
    return 1;
  }
  return 0;
}

static void component_info_solute_diffc_sort(struct component_info *comps)
{
  component_info_sort_base(comps, add_solute_diffc_compn_sort1, NULL);
}

static int add_solute_diffc_compn_sortn(const struct component_info_data *a,
                                        void *arg)
{
  CSVASSERT(arg);
  if (a->d) {
    ptrdiff_t p = a->d - (struct component_data *)arg;
    if (p < 0)
      return -1;
    if (p > 0)
      return 1;
    return 0;
  }
  return -1;
}

static int component_info_solute_diffc_find(struct component_info *comps,
                                            struct component_data *d)
{
  return component_info_find_sorted_base(comps, add_solute_diffc_compn_sortn,
                                         d);
}

typedef int add_solute_diffc_miss_filter(const struct component_data *p,
                                         void *arg);

static
void add_solute_diffc_missings_base(struct dc_calc_param_input *head,
                                    struct component_data *comp_data_head,
                                    add_solute_diffc_miss_filter *filter,
                                    void *arg, int commutative, int *stat)
{
  ptrdiff_t ncp;
  int npt, npa;
  struct geom_list *lp, *lh;
  struct component_info comps;
  struct geom_bitarray *iflg;
  struct dc_calc_param_input missing_head;
  int ier;

  geom_list_init(&missing_head.list);

  ier = 0;
  iflg = NULL;

  /* Count total number of available components */
  npa = 0;
  lh = &comp_data_head->list;
  geom_list_foreach(lp, lh) {
    ++npa;
  }

  if (npa <= 0)
    return;

  component_info_init(&comps);
  if (!component_info_resize(&comps, npa, 0)) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    if (stat)
      *stat = ON;
    component_info_clear(&comps);
    if (iflg)
      geom_bitarray_delete(iflg);
    return;
  }

  npt = 0;
  lh = &comp_data_head->list;
  geom_list_foreach(lp, lh) {
    component_data *d;
    d = component_data_entry(lp);
    if (!filter || filter(d, arg))
      component_info_setc(&comps, npt++, d);
  }
  for (npa = npt; npa < component_info_ncompo(&comps); ++npa)
    component_info_seti(&comps, npa, JUPITER_ID_INVALID);

  if (commutative) {
    ncp = dc_calc_binary_size_commutative(npt);
  } else {
    ncp = dc_calc_binary_size(npt);
  }
  if (ncp == 0) {
    component_info_clear(&comps); /* Nothing can be added */
    return;
  }

  if (ncp < 0 || ncp >= INT_MAX) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL,
               "Array size exceeds available limit: %d", INT_MAX);
    if (stat)
      *stat = ON;
    component_info_clear(&comps);
    return;
  }

  iflg = geom_bitarray_new(ncp);
  if (!iflg) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    if (stat)
      *stat = ON;
    component_info_clear(&comps);
    return;
  }

  component_info_solute_diffc_sort(&comps);

  geom_bitarray_setall(iflg, 0);

  lh = &head->list;
  geom_list_foreach(lp, lh) {
    int idiff, ibase;
    ptrdiff_t adr;
    struct dc_calc_param_input *ip;
    ip = dc_calc_param_input_entry(lp);

    if (!ip->diffusing.id.d || !ip->base.id.d)
      continue;

    idiff = component_info_solute_diffc_find(&comps, ip->diffusing.id.d);
    ibase = component_info_solute_diffc_find(&comps, ip->base.id.d);
    if (idiff < 0 || ibase < 0)
      continue;

    if (commutative) {
      adr = dc_calc_binary_address_commutative(idiff, ibase, npt);
    } else {
      adr = dc_calc_binary_address(idiff, ibase, npt);
    }
    CSVASSERT(adr >= 0 && adr < ncp);
    geom_bitarray_set(iflg, adr, 1);
  }

  for (ptrdiff_t iadr = 0; iadr < ncp; ++iadr) {
    struct dc_calc_param_input *ip;
    int ret, idiff, ibase;

    if (geom_bitarray_get(iflg, iadr))
      continue;

    if (commutative) {
      ret = dc_calc_binary_ids_commutative(iadr, npt, &idiff, &ibase);
    } else {
      ret = dc_calc_binary_ids(iadr, npt, &idiff, &ibase);
    }
    CSVASSERT(ret);

    ip = (struct dc_calc_param_input *)
      calloc(1, sizeof(struct dc_calc_param_input));
    if (!ip) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
      ier = 1;
      break;
    }

    geom_list_init(&ip->list);
    geom_list_insert_prev(&missing_head.list, &ip->list);
    ip->diffusing.id.d = component_info_getc(&comps, idiff);
    ip->base.id.d = component_info_getc(&comps, ibase);
    ip->missing = 1;
    ip->data.model = DC_FUNCS_INVALID;
    tempdep_property_init(&ip->data.prop);
  }

  component_info_clear(&comps);
  geom_bitarray_delete(iflg);

  if (ier) {
    struct geom_list *ln;

    lh = &missing_head.list;
    geom_list_foreach_safe(lp, ln, lh) {
      struct dc_calc_param_input *p;

      p = dc_calc_param_input_entry(lp);
      free(p);
    }
  } else {
    geom_list_insert_list_prev(&head->list, &missing_head.list);
    geom_list_delete(&missing_head.list);
  }
}

//--- for solid diffusivity

static int solute_diffc_getid(struct dc_calc_param_input *ip,
                              const struct component_info_data *id, void *arg)
{
  if (!id->d)
    return -1;

  if (!component_phases_has_solid_or_liquid(id->d->phases))
    return -1;

  return id->d->comp_index;
}

void set_solute_diffc_build_params(struct dc_calc_param_input *iparam_head,
                                   int NBCompo, struct dc_calc_param ***params,
                                   ptrdiff_t *nparam, int *stat)
{
  set_solute_diffc_build_params_base(iparam_head, NBCompo, DIFF_S_COMMUTATITVE,
                                     solute_diffc_getid, NULL, params, nparam,
                                     stat);
}

static int read_solute_diffc_iderr_s(struct dc_calc_param_input *ip,
                                     struct dc_calc_input_idinfo *idinfo,
                                     const char *fname, void *arg)
{
  SET_P_INIT(NULL, fname, &idinfo->row, &idinfo->column);

  CSVASSERT(idinfo->id.d);
  CSVASSERT(!idinfo->id.d->generated);

  if (!component_phases_has_solid_or_liquid(idinfo->id.d->phases)) {
    SET_P_PERROR(ERROR, "This material ID is not for solid or liquid phase");
    return 0;
  }
  return 1;
}

static void read_solute_diffc_funcs(struct dc_calc_param_input *iparam_head,
                                    struct component_data *comp_data_head,
                                    const char *fname, csv_data *csv,
                                    csv_row *start_row, int *stat)
{
  read_solute_diffc_funcs_base(iparam_head, comp_data_head,
                               read_solute_diffc_iderr_s, NULL,
                               DIFF_S_COMMUTATITVE, fname, csv, start_row,
                               stat);
}

static int add_solute_diffc_miss_filter_s(const struct component_data *p,
                                          void *arg)
{
  if (p->generated)
    return 0;
  return component_phases_has_solid_or_liquid(p->phases);
}

static void add_solute_diffc_missings(struct dc_calc_param_input *head,
                                      struct component_data *comp_data_head,
                                      int *stat)
{
  add_solute_diffc_missings_base(head, comp_data_head,
                                 add_solute_diffc_miss_filter_s, NULL,
                                 DIFF_S_COMMUTATITVE, stat);
}

//--- for gas diffusivity

struct set_solute_diffc_build_g_args
{
  int NBCompo, NGCompo;
};

static int solute_diffc_getid_g(struct dc_calc_param_input *ip,
                                const struct component_info_data *id, void *arg)
{
  int ic;
  struct set_solute_diffc_build_g_args *p;
  p = (struct set_solute_diffc_build_g_args *)arg;

  if (!id->d)
    return -1;

  if (!component_phases_is_gas_only(id->d->phases))
    return -1;

  if (id->d->jupiter_id == -1) {
    CSVASSERT(id->d->comp_index < 0);
    ic = -1;
  } else {
    ic = id->d->comp_index;
  }
  return convert_dc_calc_gas_id_to_index(ic, p->NBCompo, p->NGCompo);
}

void set_solute_diffc_build_params_g(struct dc_calc_param_input *iparam_head,
                                     int NBCompo, int NGCompo,
                                     struct dc_calc_param ***params,
                                     ptrdiff_t *nparam, int *stat)
{
  struct set_solute_diffc_build_g_args d = {
    .NBCompo = NBCompo,
    .NGCompo = NGCompo,
  };
  set_solute_diffc_build_params_base(iparam_head, NGCompo + 1,
                                     DIFF_G_COMMUTATITVE, solute_diffc_getid_g,
                                     &d, params, nparam, stat);
}

static int read_solute_diffc_iderr_g(struct dc_calc_param_input *ip,
                                     struct dc_calc_input_idinfo *idinfo,
                                     const char *fname, void *arg)
{
  SET_P_INIT(NULL, fname, &idinfo->row, &idinfo->column);

  if (!component_phases_has_gas(idinfo->id.d->phases)) {
    SET_P_PERROR(ERROR, "This material ID is not for solid or liquid phase");
    return 0;
  }
  return 1;
}

static void read_solute_diffc_funcs_g(struct dc_calc_param_input *iparam_head,
                                      struct component_data *comp_data_head,
                                      const char *fname, csv_data *csv,
                                      csv_row *start_row, int *stat)
{
  read_solute_diffc_funcs_base(iparam_head, comp_data_head,
                               read_solute_diffc_iderr_g, NULL,
                               DIFF_G_COMMUTATITVE, fname, csv, start_row,
                               stat);
}

static int add_solute_diffc_miss_filter_g(const struct component_data *p,
                                          void *arg)
{
  if (p->generated)
    return 0;
  return component_phases_has_gas(p->phases);
}

static void add_solute_diffc_missings_g(struct dc_calc_param_input *head,
                                        struct component_data *comp_data_head,
                                        int *stat)
{
  add_solute_diffc_missings_base(head, comp_data_head,
                                 add_solute_diffc_miss_filter_g, NULL,
                                 DIFF_G_COMMUTATITVE, stat);
}

//----

/* YSE: Adjust argument to take CSV data */
void set_phase(phase_value *phv, flags *flg, domain *cdo,
               const char *plist_file, csv_data *plist_data,
               const char *param_file, csv_data *param_data,
               component_data *comp_data_head, int *stat)
{
  struct geom_list *lp, *lh;
  int i;
  csv_row *row;
  csv_column *col;
  csv_column *tm_table_col;
  csv_column *tm_funcs_col;
  tm_func2_model funct;
  dc_func2_model dfunct;
  tempdep_property_type tdep_type;
  int ret;
  char *tab_file;
  component_phases liqsol_phase =
    component_phases_a(COMPONENT_PHASE_SOLID, COMPONENT_PHASE_LIQUID);

  SET_P_INIT(param_data, param_file, &row, &col);

  CSVASSERT(phv);
  CSVASSERT(flg);
  CSVASSERT(cdo);
  CSVASSERT(comp_data_head);

  lh = &comp_data_head->list;
  geom_list_foreach(lp, lh) {
    component_data *d;
    d = component_data_entry(lp);

    if (!d->csv || !d->fname)
      continue;

    if (d->generated)
      continue;

    d->phases = component_phases_none();

    if (d->jupiter_id == -1) {
      CSVASSERT(d->phase_comps_index == -1);

      set_property_gm1(phv, flg, d->csv, d->fname, stat);

      component_phases_set(&d->phases, COMPONENT_PHASE_GAS, 1);

    } else if (d->phase_comps_index < 0) {
      CSVUNREACHABLE();

    } else if (d->phase_comps_index < cdo->NIBaseComponent) {
      set_property(d->jupiter_id, &phv->comps[d->phase_comps_index], flg,
                   d->csv, d->fname, stat);

      component_phases_set(&d->phases, COMPONENT_PHASE_LIQUID, 1);
      if (phv->comps[d->phase_comps_index].sform != SOLID_FORM_UNUSED)
        component_phases_set(&d->phases, COMPONENT_PHASE_SOLID, 1);

    } else if (d->phase_comps_index < cdo->NIComponent) {
      set_property_g(d->jupiter_id, &phv->comps[d->phase_comps_index], flg,
                     d->csv, d->fname, stat);

      component_phases_set(&d->phases, COMPONENT_PHASE_GAS, 1);
    }
  }

  SET_P_FROM(param_data, param_file);

  SET_P(&phv->tr, double, "tr", 1, 300.0);
  /* SET_P(phv->sol_tmp, double, "Init_temperature", 1, 293.0); */
  /* SET_P_NEXT(phv->liq_tmp, double, 293.0); */
  /* SET_P_NEXT(phv->gas_tmp, double, 293.0); */

  SET_P_FROM(plist_data, plist_file);

  tm_table_col = NULL;
  SET_P_PASS_NOTFOUND(&tab_file, charp, "tm_liq_table", 1, NULL);
  free(tab_file);
  if (col) {
    tm_table_col = col;
    set_tm_tables(&phv->liq_tables, liqsol_phase, comp_data_head, plist_file,
                  plist_data, row, stat);
  }

  tm_funcs_col = NULL;
  SET_P_PASS_NOTFOUND(&funct, tm_func2_model, "tm_liq_func", 1,
                      TM_FUNC_INVALID);
  if (col) {
    tm_funcs_col = col;
    set_tm_funcs(&phv->liq_funcs, liqsol_phase, comp_data_head, plist_file,
                 plist_data, row, stat);
  }

  if (!(tm_table_col || tm_funcs_col)) {
    if (flg->solute_diff == ON) {
      csvperrorf(plist_file, 0, 0, CSV_EL_INFO, "tm_liq_table/tm_liq_func",
                 "No Liquidus Temperature tables/functions are given");
    }
  }

  tm_table_col = NULL;
  SET_P_PASS_NOTFOUND(&tab_file, charp, "tm_soli_table", 1, NULL);
  free(tab_file);
  if (col) {
    tm_table_col = col;
    set_tm_tables(&phv->sol_tables, liqsol_phase, comp_data_head, plist_file,
                  plist_data, row, stat);
  }

  tm_funcs_col = NULL;
  SET_P_PASS_NOTFOUND(&funct, tm_func2_model, "tm_soli_func", 1,
                      TM_FUNC_INVALID);
  if (col) {
    tm_funcs_col = col;
    set_tm_funcs(&phv->sol_funcs, liqsol_phase, comp_data_head, plist_file,
                 plist_data, row, stat);
  }

  if(!(tm_table_col || tm_funcs_col)) {
    if (flg->solute_diff == ON) {
      csvperrorf(plist_file, 0, 0, CSV_EL_INFO, "tm_soli_table/tm_soli_func",
                 "No Solidus Temperature tables/functions are given");
    }
  }

  SET_P_PASS_NOTFOUND(&dfunct, dc_func2_model, "solute_diffusivity", 1,
                      DC_FUNCS_INVALID);
  if (col) {
    read_solute_diffc_funcs(&phv->diff_input_head, comp_data_head,
                            plist_file, plist_data, row, stat);
  } else {
    if (flg->solute_diff == ON && cdo->NIBaseComponent > 1) {
      csvperrorf(plist_file, 0, 0, CSV_EL_WARN, "solute_diffusivity",
                 "No solute diffusivity defined (will become all 0)!");
    }
  }
  if (flg->solute_diff == ON) {
    add_solute_diffc_missings(&phv->diff_input_head, comp_data_head, stat);
  }

  SET_P_PASS_NOTFOUND(&dfunct, dc_func2_model, "gas_diffusivity", 1,
                      DC_FUNCS_INVALID);
  if (col) {
    read_solute_diffc_funcs_g(&phv->diff_g_input_head, comp_data_head,
                              plist_file, plist_data, row, stat);
  } else {
    if (flg->solute_diff == ON && cdo->NIGasComponent > 0) {
      csvperrorf(plist_file, 0, 0, CSV_EL_WARN, "gas_diffusivity",
                 "No gas diffusivity defined (will become all 0)!");
    }
  }
  if (flg->solute_diff == ON) {
    add_solute_diffc_missings_g(&phv->diff_g_input_head, comp_data_head, stat);
  }
}


/* YSE: Add separate flags set which requires MPI info. */
void set_input_list_fp(flags *flg, mpi_param *mpi, csv_data *flags_data,
                       const char *flags_fname, int *stat)
{
  csv_row *row;
  csv_column *col;
  int has_r;
  int ret;
  struct csv_to_FILEn_data fndata;
  SET_P_INIT(flags_data, flags_fname, &row, &col);

  CSVASSERT(flg);
  CSVASSERT(mpi);

  fndata.filename = &flg->list_fp_name;
  fndata.has_r = &has_r;
  ret = SET_P(&fndata, FILEn, "fp", 1, NULL);
  if (has_r < 0) {
    SET_P_PERROR(INFO, "Using stdout for this job");
    if (stat)
      *stat = ON;
  }
  if (ret > 0) /* non fatal errors are fine */
    ret = 0;
  if (ret && stat)
    *stat = ON;

  flg->list_fp = NULL;
  flg->list_fp_open = OFF;
#ifdef JUPITER_MPI
  flg->list_fp_mpi = MPI_FILE_NULL;
  flg->list_fp_comm = MPI_COMM_NULL;
#endif

  if (!flg->list_fp_name || has_r < 0 || ret) {
    /* stdout or invalid format, or other errors */
    flg->list_fp = stdout;
    if (!flg->list_fp_name) {
      flg->list_fp_name = jupiter_strdup("-");
      if (!flg->list_fp_name) {
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
        if (stat)
          *stat = ON;
      }
    }
  } else if (has_r > 0) { /* valid format and includes %r */
    errno = 0;
    flg->list_fp = fopen(flg->list_fp_name, "w");
    if (flg->list_fp) {
      flg->list_fp_open = ON;
    } else {
      csvperror_col(flags_fname, col, CSV_EL_ERROR, CSV_ERR_SYS, errno, 0,
                    NULL);
      flg->list_fp = stdout;
    }
  } else { /* valid format but does not include %r */
#ifdef JUPITER_MPI
    /* Shared write mode (may be very heavy) */
    int ret;
    MPI_Errhandler h;
    MPI_Comm_get_errhandler(mpi->CommJUPITER, &h);
    MPI_Comm_set_errhandler(mpi->CommJUPITER, MPI_ERRORS_RETURN);
    ret = MPI_Comm_dup(mpi->CommJUPITER, &flg->list_fp_comm);
    if (ret == MPI_SUCCESS) {
      ret = MPI_File_open(flg->list_fp_comm, flg->list_fp_name,
                          MPI_MODE_WRONLY | MPI_MODE_CREATE, MPI_INFO_NULL,
                          &flg->list_fp_mpi);
      if (ret == MPI_SUCCESS)
        ret = MPI_File_set_size(flg->list_fp_mpi, 0);
    }
    if (ret != MPI_SUCCESS) {
      csvperror_col(flags_fname, col, CSV_EL_ERROR, CSV_ERR_MPI, 0, ret, NULL);
      if (flg->list_fp_mpi != MPI_FILE_NULL) {
        MPI_File_close(&flg->list_fp_mpi);
      }
      flg->list_fp_mpi = MPI_FILE_NULL;
      flg->list_fp = stdout;
    }
    if (flg->list_fp_comm != MPI_COMM_NULL)
      MPI_Comm_set_errhandler(flg->list_fp_comm, MPI_ERRORS_ARE_FATAL);
    MPI_Comm_set_errhandler(mpi->CommJUPITER, h);
#else
    errno = 0;
    flg->list_fp = fopen(flg->list_fp_name, "w");
    if (flg->list_fp) {
      flg->list_fp_open = ON;
    } else {
      csvperror_col(flags_fname, col, CSV_EL_ERROR, CSV_ERR_SYS, errno, 0,
                    NULL);
      flg->list_fp = stdout;
    }
#endif
  }
}

/**
 * @file param.c
 * @brief Perameter setting routines
 */
