
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <limits.h>
#include <time.h>

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#include "component_data.h"
#include "component_data_defs.h"
#include "component_info_defs.h"
#include "heat_source.h"
#include "lpt.h"
#include "os/asprintf.h"

#include "print_param_core.h"
#include "print_param_basic.h"
#include "print_param_comp.h"
#include "print_param_comps.h"
#include "print_param_geom.h"
#include "print_param_props.h"
#include "print_param_keywords.h"
#include "print_param_vecmat.h"
#include "control/defs.h"
#include "csv.h"
#include "field_control.h"
#include "geometry/defs.h"
#include "geometry/list.h"
#include "geometry/surface_shape.h"
#include "geometry/svector.h"
#include "geometry_source.h"
#include "init_component.h"
#include "non_uniform_grid.h"

#ifdef LPT
#include "lpt/LPTdefs.h"
#endif

#ifdef LPTX
#include "lptx/defs.h"
#include "lptx/init_set.h"
#include "lptx/vector.h"
#include "lptx/param.h"
#endif

#include "oxidation.h"
#include "struct.h"
#include "func.h"
#include "boundary_util.h"
#include "common_util.h"
#include "component_info.h"

#include "csvutil.h"

#include "table/table.h"

#include "tmcalc.h"
#include "dccalc.h"

#include "tempdep_properties.h"

#include "geometry/init.h"
#include "geometry/file.h"
#include "geometry/data.h"
#include "geometry/infomap.h"
#include "geometry/variant.h"
#include "geometry/shape.h"
#include "geometry/udata.h"

#include "control/manager.h"

#ifdef LPT
#include "lpt/LPTbnd.h"
#endif

static int print_param_flags(parameter *prm);
static int print_param_domain(parameter *prm);
static int print_param_physical_properties(parameter *prm);
static int print_param_geometry(parameter *prm);
static int print_param_control_geometry(parameter *prm);
static int print_param_end(parameter *prm);

void print_param(parameter *prm)
{
  flags *flg;
  domain *cdo;
  phase_value *phv;
  mpi_param *mpi;
  int nogood;
  int r;

  /*
   * YSE: We needs flg data almost of anywhere in this function, so
   *      abort if these structures are not allocated and made code
   *      simplified.
   */
  CSVASSERT(prm);
  CSVASSERT(prm->flg);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->phv);
  CSVASSERT(prm->mpi);

  flg = prm->flg;
  cdo = prm->cdo;
  phv = prm->phv;
  mpi = prm->mpi;

  nogood = OFF;

  r = print_param_flags(prm);
  if (r)
    nogood = ON;

  r = print_param_domain(prm);
  if (r)
    nogood = ON;

  r = print_param_physical_properties(prm);
  if (r)
    nogood = ON;

  r = print_param_geometry(prm);
  if (r)
    nogood = ON;

  r = print_param_control_geometry(prm);
  if (r)
    nogood = ON;

  r = print_param_end(prm);
  if (r)
    nogood = ON;

#ifdef JUPITER_MPI
  MPI_Barrier(mpi->CommJUPITER);
#endif

  if (flg->list_fp) {
    fflush(flg->list_fp);
  }
#ifdef JUPITER_MPI
  if (flg->list_fp_mpi != MPI_FILE_NULL) {
    MPI_File_sync(flg->list_fp_mpi);
  }
#endif
  if (nogood == ON) {
    csvperrorf("print_param", 0, 0, CSV_EL_FATAL, NULL,
               "Some parameters with " PP_INVALID_MARK
               " are invalid value. Please check.");
    /*
     * communication_rad() will not work to terminate radiation process here.
     */
    free_parameter(prm);
#ifdef JUPITER_MPI
    MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    exit(EXIT_FAILURE);
  }
}

static int PPibool_to_j(int value)
{
  if (value)
    return ON;
  return OFF;
}

/* Use of this macro is deprecated. */
#define PPdata(type, indent, var, name, unit, ok)                              \
  PP_##type(flg, indent, name, var, unit, ok, &nogood)

static int print_param_flags(parameter *prm)
{
  int nogood;
  flags *flg;

  CSVASSERT(prm);

  flg = prm->flg;
  CSVASSERT(flg);

  nogood = 0;

  /* TODO: Please check unit. */
  PPHead1("Flags data");
  PPdata(fname, 0, prm->flags_file, "File", "", prm->flags_file != NULL);
  PPdata(bool, 0, flg->print, "Print Messages", "", 1);
  /* PPdata(bool, 0, flg->debug, "Debug Mode", "", 1); */
  PPdata(bool, 0, flg->has_non_uniform_grid, "Use non-uniform grid (info only)",
         "", 1);
  PPdata(bool, 0, flg->geom_in, "Load Geometry", "", 1);
  PPdata(bool, 0, flg->fluid_dynamics, "Fluid Dynamics", "", 1);
  PPdata(bool, 0, flg->heat_eq, "Heat Equation", "", 1);
  PPdata(bool, 0, flg->phase_change, "Phase Change", "", 1);
  PPdata(bool, 0, flg->melting, "Melting", "", 1);
  PPdata(bool, 0, flg->solidification, "Solidification", "", 1);
  PPdata(bool, 0, flg->vaporization, "Vaporization", "", 1);
  PPdata(bool, 0, flg->condensation, "Condensation", "", 1);
  PPdata(bool, 0, flg->surface_tension, "Surface Tension", "", 1);
#ifndef JUPITER_NOMETI
  PPdata(bool, 0, flg->oxidation, "Oxidation", "", 1);
#endif
  PPdata(bool, 0, flg->radiation, "Radiation", "",
         (flg->has_non_uniform_grid == ON) ? (flg->radiation == OFF) : 1);
#ifdef HAVE_LPT
  PPdata(bool, 0, flg->lpt_calc, "Lagrangean Particle Tracking", "", 1);
#endif
  PPdata(bool, 0, flg->laser, "Laser Irradiation", "",
         (flg->has_non_uniform_grid == ON) ? (flg->laser == OFF) : 1);

  PPdata(bool, 0, flg->WENO, "WENO Method", "",
         (flg->has_non_uniform_grid == ON) ? (flg->WENO == OFF) : 1);
  PPdata(bool, 0, flg->heat_tvd3, "3rd Order TVD RK to Heat eq.", "", 1);
  PPdata(bool, 0, flg->visc_tvd3, "3rd Order TVD RK to Visc eq.", "", 1);
  PPdata(bool, 0, flg->IBM, "Immersed Boundary Method", "", 1);
  PPdata(bool, 0, flg->vof_adv_fls, "VOF Advection calculation", "",
         (flg->vof_adv_fls == ON) ? (flg->solute_diff == OFF) : 1);
  PPdata(bool, 0, flg->multi_layer_no_coalescence, "Multi layer method where no bubble coalesce", "",
         (flg->multi_layer_no_coalescence == ON) ? (flg->solute_diff == OFF) : 1);
  PPdata(bool, 0, flg->multi_layer_less_coalescence, "Multi layer method where less bubble coalesce", "",
         (flg->multi_layer_less_coalescence == ON) ? (flg->solute_diff == OFF) : 1);
  PPdata(bool, 0, flg->film_drainage, "Film drainage model", "", 1);
#ifndef JUPITER_NOMETI
  PPdata(bool, 0, flg->eutectic, "Eutectic calculation", "", 1);
  PPdata(bool, 0, flg->solute_diff, "Solute diffusion calculation", "",
         (flg->solute_diff == ON)
           ? (flg->porous == OFF && flg->vof_adv_fls == OFF)
           : 1);
#endif

  PPdata(bool, 0, flg->PHASEFIELD, "Phasefield Method", "",
         (flg->has_non_uniform_grid == ON) ? (flg->PHASEFIELD == OFF) : 1);
  PPdata(ics, 0, flg->interface_capturing_scheme, "Interface Capturing Scheme",
         "", 1);

  PPdata(bool, 0, flg->porous, "Use porous model", "",
         (flg->porous == ON) ? (flg->solute_diff == OFF) : 1);
  PPdata(bool, 0, flg->two_energy, "Use two energy model for porous model", "", 1);

  PPdata(bool, 0, flg->wettability, "Use contact angle model", "", 1);

  PPdata(bnd, 0, flg->bc_xm, "X- Boundary", "", 1);
  PPdata(bnd, 0, flg->bc_xp, "X+ Boundary", "", 1);
  PPdata(bnd, 0, flg->bc_ym, "Y- Boundary", "", 1);
  PPdata(bnd, 0, flg->bc_yp, "Y+ Boundary", "", 1);
  PPdata(bnd, 0, flg->bc_zm, "Z- Boundary", "", 1);
  PPdata(bnd, 0, flg->bc_zp, "Z+ Boundary", "", 1);

  PPdata(tbnd, 0, flg->bct_xm, "X- Thermal Boundary", "", 1);
  PPdata(tbnd, 0, flg->bct_xp, "X+ Thermal Boundary", "", 1);
  PPdata(tbnd, 0, flg->bct_ym, "Y- Thermal Boundary", "", 1);
  PPdata(tbnd, 0, flg->bct_yp, "Y+ Thermal Boundary", "", 1);
  PPdata(tbnd, 0, flg->bct_zm, "Z- Thermal Boundary", "", 1);
  PPdata(tbnd, 0, flg->bct_zp, "Z+ Thermal Boundary", "", 1);

  PPdata(tbnd, 0, flg->bcrad_xm, "X- Radiation Boundary", "", 1);
  PPdata(tbnd, 0, flg->bcrad_xp, "X+ Radiation Boundary", "", 1);
  PPdata(tbnd, 0, flg->bcrad_ym, "Y- Radiation Boundary", "", 1);
  PPdata(tbnd, 0, flg->bcrad_yp, "Y+ Radiation Boundary", "", 1);
  PPdata(tbnd, 0, flg->bcrad_zm, "Z- Radiation Boundary", "", 1);
  PPdata(tbnd, 0, flg->bcrad_zp, "Z+ Radiation Boundary", "", 1);

  PPdata(bool, 0, flg->update_level_set_ls.force_update,
         "Enforce to update level-set ls", "", 1);
  PPdata(bool, 0, flg->update_level_set_lls.force_update,
         "Enforce to update level-set lls", "", 1);
  // PPdata(bool, 0, flg->update_level_set_ll.force_update,
  //        "Enforce to update level-set ll", "", 1);

  {
    int iflg;
    iflg = init_component_is_set(&flg->rebuild_components, INIT_COMPONENT_VOF);
    PPdata(bool, 0, PPibool_to_j(iflg), "Re-initialize Geom_vof", "", 1);

    iflg = init_component_is_set(&flg->rebuild_components,
                                 INIT_COMPONENT_VELOCITY_U);
    PPdata(bool, 0, PPibool_to_j(iflg), "Re-initialize Geom_velocity_u", "", 1);

    iflg = init_component_is_set(&flg->rebuild_components,
                                 INIT_COMPONENT_VELOCITY_V);
    PPdata(bool, 0, PPibool_to_j(iflg), "Re-initialize Geom_velocity_v", "", 1);

    iflg = init_component_is_set(&flg->rebuild_components,
                                 INIT_COMPONENT_VELOCITY_W);
    PPdata(bool, 0, PPibool_to_j(iflg), "Re-initialize Geom_velocity_w", "", 1);

    iflg = init_component_is_set(&flg->rebuild_components,
                                 INIT_COMPONENT_TEMPERATURE);
    PPdata(bool, 0, PPibool_to_j(iflg), "Re-initialize Geom_temperature", "",
           1);

    iflg = init_component_is_set(&flg->rebuild_components,
                                 INIT_COMPONENT_PRESSURE);
    PPdata(bool, 0, PPibool_to_j(iflg), "Re-initialize Geom_pressure", "", 1);

    iflg = init_component_is_set(&flg->rebuild_components,
                                 INIT_COMPONENT_FIXED_HSOURCE);
    PPdata(bool, 0, PPibool_to_j(iflg),
           "Re-initialize Geom_fixed_heat_source", "", 1);

    iflg = init_component_is_set(&flg->rebuild_components,
                                 INIT_COMPONENT_BOUNDARY);
    PPdata(bool, 0, PPibool_to_j(iflg),
           "Re-initialize Geom_boundary", "", 1);

    iflg = init_component_is_set(&flg->rebuild_components,
                                 INIT_COMPONENT_THERMAL_BOUNDARY);
    PPdata(bool, 0, PPibool_to_j(iflg),
           "Re-initialize Geom_tboundary", "", 1);

    iflg = init_component_is_set(&flg->rebuild_components,
                                 INIT_COMPONENT_SURFACE_BOUNDARY);
    PPdata(bool, 0, PPibool_to_j(iflg),
           "Re-initialize Geom_surface_boundary", "", 1);

#ifdef HAVE_LPT
    iflg = init_component_is_set(&flg->rebuild_components,
                                 INIT_COMPONENT_LPT_PEWALL_N);
    PPdata(bool, 0, PPibool_to_j(iflg),
           "Re-initialize Geom_LPT_wall_reflection_n and "
           "Geom_LPT_boundary_wall_reflection",
           "", 1);
#endif
  }

  PP_bool(flg, 0, "Validate initialized VOF values", flg->validate_VOF_init,
          "", 1, &nogood);
  if (flg->validate_VOF_init == ON) {
    PP_int(flg, 0, "Maximum number of outputs for VOF validation",
           flg->validate_VOF_print_max, "", 1, &nogood);
  }

  if (flg->oxidation == ON) {
    PPHead2("Oxidation Flags");
    PPdata(ox_kp_model, 0, flg->ox_kp_model, "Reaction Rate Model", "", 1);
    PPdata(bool, 0, flg->h2_absorp_eval, "H2 Absorption evaluation", "", 1);
    if (flg->h2_absorp_eval == ON) {
      PPdata(bool, 0, flg->h2_absorp_eval_p_change,
             "Include P change for H2 absorption", "", 1);
    }
  }

  if (flg->lpt_calc == ON) {
    PPHead2("LPT flags");
    PPdata(LPTts, 0, flg->lpt_ipttim, "LPT time integration scheme", "", 1);
#ifdef LPTX
    PPdata(LPTht, 0, flg->lpt_heat, "LPT heat exchange scheme", "", 1);
#endif
    PPdata(bool, 0, flg->lpt_wbcal, "LPT wall boundary calculation", "", 1);
#ifdef LPTX
    PPdata(bool, 0, flg->lpt_brownian, "LPT Brownian force", "", 1);
    PPdata(bool, 0, flg->lpt_thermophoretic, "LPT Thermophoretic force", "", 1);
    PPdata(bool, 0, flg->lpt_use_constant_Cc,
           "LPT use constant cunningham correction", "", 1);
#endif
  }

  PPHead2("Binary output settings");
  PPdata(bool, 0, flg->binary, "Output binary data", "", 1);
  /* PPdata(bool, 0, flg->gnuplot, "gnulot (not implemented)", "", 1); */

  if (flg->binary == ON) {
    PPdata(bool, 0, flg->use_double_binary, "Output double precision data", "",
           1);
    PPdata(binary_output_mode, 0, flg->output_mode,
           "Binary data file output mode", "", 1);

    PPdata(charp, 0, flg->output_data.writedir, "Binary output directory", "",
           1);
    PPdata(charp, 0, flg->output_data.readdir, "Binary input directory", "", 1);
    PPdata(charp, 0, flg->output_data.filename_template.time,
           "Binary time file template", "", 1);
    PPdata(charp, 0, flg->output_data.filename_template.comp_based,
           "Binary component based file template", "", 1);
    PPdata(charp, 0, flg->output_data.filename_template.others,
           "Binary other file template", "", 1);

    PPdata(charp, 0, flg->restart_data.writedir,
           "Restart data output directory", "", 1);
    PPdata(charp, 0, flg->restart_data.readdir, "Restart data input directory",
           "", 1);
    PPdata(charp, 0, flg->restart_data.filename_template.time,
           "Restart time file template", "", 1);
    PPdata(charp, 0, flg->restart_data.filename_template.comp_based,
           "Restart component based file template", "", 1);
    PPdata(charp, 0, flg->restart_data.filename_template.others,
           "Restart other file template", "", 1);

    PPdata(bool, 0, flg->output_data.fs.outf, "Output solid VOF", "", 1);
    PPdata(bool, 0, flg->output_data.fl.outf, "Output liquid VOF", "", 1);

    PPdata(bool, 0, flg->output_data.u.outf, "Output staggered velocity u", "",
           1);
    PPdata(bool, 0, flg->output_data.v.outf, "Output staggered velocity v", "",
           1);
    PPdata(bool, 0, flg->output_data.w.outf, "Output staggered velocity w", "",
           1);
    PPdata(bool, 0, flg->output_data.p.outf, "Output relative pressure p", "",
           1);
    PPdata(bool, 0, flg->output_data.t.outf, "Output temperature t", "", 1);

    PPdata(bool, 0, flg->output_data.uvw.outf, "Output velocity uvw vector", "",
           1);
    PPdata(bool, 0, flg->output_data.q.outf, "Output heat source q", "", 1);

    PPdata(bool, 0, flg->output_data.lls.outf, "Output G/S+L levelset lls", "",
           1);
    PPdata(bool, 0, flg->output_data.ll.outf, "Output L/S+G levelset ll", "",
           1);
    PPdata(bool, 0, flg->output_data.ls.outf, "Output S/L+G levelset ls", "",
           1);

    //  Susumu, 2020/6/22
    if (flg->porous == ON) {
      PPdata(bool, 0, flg->output_data.eps.outf, "Output prosity eps", "", 1);
      PPdata(bool, 0, flg->output_data.epss.outf, "Output effective prosity epss", "", 1);
      PPdata(bool, 0, flg->output_data.perm.outf, "Output permeability perm",
             "", 1);
      if (flg->two_energy == ON) {
        PPdata(bool, 0, flg->output_data.tf.outf, "Output fluid phase temperature tf", "", 1);
        PPdata(bool, 0, flg->output_data.ts.outf, "Output solid phase temperature ts",
               "", 1);
      }
      PPdata(bool, 0, flg->output_data.sgm.outf, "Output sgm", "", 1);
    }

    if (flg->solute_diff == ON) {
      PPdata(bool, 0, flg->output_data.Y.outf, "Output solute mole fraction Y",
             "", 1);
      PPdata(bool, 0, flg->output_data.Yt.outf,
             "Output sum of solute concentrations Yt", "", 1);
      PPdata(bool, 0, flg->output_data.Vf.outf, "Output solute VOF Vf", "", 1);
      PPdata(bool, 0, flg->output_data.flux.outf,
             "Output solute concentration flux", "", 1);
    }

    PPdata(bool, 0, flg->output_data.rad.outf, "Output radiation heat rad", "",
           1);

    if (flg->phase_change == ON) {
      if (flg->solute_diff == ON) {
        PPdata(bool, 0, flg->output_data.entha.outf,
               "Output Phase Change Enthalpy entha", "", 1);
        PPdata(bool, 0, flg->output_data.mushy.outf, "Output Mushy Zone mushy",
               "", 1);
      } else {
        PPdata(bool, 0, flg->output_data.df.outf,
               "Output VOF delta for melting df", "", 1);
        PPdata(bool, 0, flg->output_data.dfs.outf,
               "Output VOF delta for solidification dfs", "", 1);
      }
    }

    if (flg->oxidation == ON) {
      PPdata(bool, 0, flg->output_data.ox_dt.outf,
             "Output oxidation thickness (cumlative) ox_dt", "", 1);
      PPdata(bool, 0, flg->output_data.ox_dt_local.outf,
             "Output oxidation thickness (local) ox_dt_local", "", 1);
      PPdata(bool, 0, flg->output_data.ox_flag.outf,
             "Output oxidation state flag ox_flag", "", 1);
      PPdata(bool, 0, flg->output_data.ox_lset.outf,
             "Output oxidation Zr level set ox_lset", "", 1);
      PPdata(bool, 0, flg->output_data.ox_vof.outf,
             "Output oxidation Zr VOF ox_vof", "", 1);
      PPdata(bool, 0, flg->output_data.ox_h2.outf,
             "Output generated H2 by oxidation ox_h2 (internal)", "", 1);
      PPdata(bool, 0, flg->output_data.ox_q.outf,
             "Output heat generation by oxidation ox_q", "", 1);
      PPdata(bool, 0, flg->output_data.ox_f_h2o.outf,
             "Output enough H2O region mark ox_f_h2o", "", 1);
      PPdata(bool, 0, flg->output_data.ox_lset_h2o.outf,
             "Output distance to H2O regino surface ox_lset_h2o", "", 1);
      PPdata(bool, 0, flg->output_data.ox_lset_h2o_s.outf,
             "Output saved value of ox_lset_h2o, ox_lset_h2o_s", "", 1);

      if (flg->h2_absorp_eval == ON) {
        PPdata(bool, 0, flg->output_data.h2_absorp_eval.outf,
               "Output H2 Absorption rate", "", 1);
        PPdata(bool, 0, flg->output_data.h2_absorp_P.outf,
               "Output Averaged P for H2 Absorption", "", 1);
        PPdata(bool, 0, flg->output_data.h2_absorp_T.outf,
               "Output Averaged T for H2 Absorption", "", 1);
        PPdata(bool, 0, flg->output_data.h2_absorp_Zr.outf,
               "Output Mark whether Oxidized or not", "", 1);
        PPdata(bool, 0, flg->output_data.h2_absorp_Ks.outf,
               "Output Ks value of H2 Absorption", "", 1);
      }
    }

    if (flg->lpt_calc == ON) {
      PPdata(bool, 0, flg->output_data.lpt_xpt.outf,
             "Output LPT particle X position lpt_xpt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_ypt.outf,
             "Output LPT particle Y position lpt_ypt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_zpt.outf,
             "Output LPT particle Z position lpt_zpt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_pospt.outf,
             "Output LPT particle position vector lpt_pospt", "", 1);

      PPdata(bool, 0, flg->output_data.lpt_uxpt.outf,
             "Output LPT paritcle X velocity lpt_uxpt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_uypt.outf,
             "Output LPT particle Y velocity lpt_uypt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_uzpt.outf,
             "Output LPT particle Z velocity lpt_uzpt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_upt.outf,
             "Output LPT particle velocity vector lpt_upt", "", 1);

      PPdata(bool, 0, flg->output_data.lpt_uf.outf,
             "Output LPT fluid velocity around particle lpt_uf", "", 1);
#ifdef LPTX
      PPdata(bool, 0, flg->output_data.lpt_muf.outf,
             "Output LPT fluid viscosity around particle lpt_muf", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_densf.outf,
             "Output LPT fluid density around particle lpt_densf", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_cpf.outf,
             "Output LPT fluid specific heat around particle lpt_cpf", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_thcf.outf,
             "Output LPT fluid thermal conductivity around particle lpt_thcf",
             "", 1);
#endif

      PPdata(bool, 0, flg->output_data.lpt_timpt.outf,
             "Output LPT particle injection time lpt_timpt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_fuxpt.outf,
             "Output LPT particle X vel. on time scheme lpt_fuxpt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_fuypt.outf,
             "Output LPT particle Y vel. on time scheme lpt_fuypt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_fuzpt.outf,
             "Output LPT particle Z vel. on time scheme lpt_fuzpt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_fupt.outf,
             "Output LPT particle vel. vec. on time scheme lpt_fupt", "", 1);
#ifdef LPTX
      PPdata(bool, 0, flg->output_data.lpt_dTdt.outf,
             "Output LPT particle delta T on time scheme lpt_dTdt", "", 1);
#endif

      PPdata(bool, 0, flg->output_data.lpt_fduxt.outf,
             "Output LPT particle dUx/dt lpt_fduxt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_fduyt.outf,
             "Output LPT particle dUy/dt lpt_fduyt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_fduzt.outf,
             "Output LPT particle dUz/dt lpt_fduzt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_fdt.outf,
             "Output LPT paritcle dU/dt (vector) lpt_fdt", "", 1);

      PPdata(bool, 0, flg->output_data.lpt_diapt.outf,
             "Output LPT particle diameter lpt_diapt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_denspt.outf,
             "Output LPT particle density lpt_denspt", "", 1);
#ifdef LPTX
      PPdata(bool, 0, flg->output_data.lpt_tempf.outf,
             "Output LPT fluid temperature around particle lpt_tempf", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_gradTf.outf,
             "Output LPT fluid temperature gradient around particle lpt_gradTf",
             "", 1);
      PPdata(bool, 0, flg->output_data.lpt_pathf.outf,
             "Output LPT mean free path around particle lpt_pathf", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_mwf.outf,
             "Output LPT mean molecular weight around particle lpt_mwf", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_cppt.outf,
             "Output LPT particle specific heat lpt_cppt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_thcpt.outf,
             "Output LPT particle thermal conductivity lpt_thcpt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_temppt.outf,
             "Output LPT particle temperature lpt_temppt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_htrpt.outf,
             "Output LPT particle heat transfer rate lpt_htrpt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_tothtpt.outf,
             "Output LPT particle total heat transfer lpt_tothtpt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_inipospt.outf,
             "Output LPT particle initial position lpt_inipospt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_iniupt.outf,
             "Output LPT particle initial velocity lpt_iniupt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_initemppt.outf,
             "Output LPT particle initial temperature lpt_initemppt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_fbpt.outf,
             "Output LPT particle brownian force lpt_fbpt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_fTpt.outf,
             "Output LPT particle thermophoretic force lpt_fTpt", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_exit.outf,
             "Output LPT particle exited flag lpt_exit", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_flags.outf,
             "Output LPT particle flags lpt_flags", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_parceln.outf,
             "Output LPT particle number of particles lpt_parceln", "", 1);
      PPdata(bool, 0, flg->output_data.lpt_seed.outf,
             "Output LPT particle random seed lpt_seed", "", 1);
#endif
      PPdata(bool, 0, flg->output_data.lpt_ewall.outf,
             "Output LPT restitution coefficient lpt_ewall", "", 1);
#ifdef LPTX
      PPdata(bool, 0, flg->output_data.qpt.outf,
             "Output Heat source particles qpt", "", 1);
#endif
    }

    PPdata(bool, 0, flg->output_data.dens.outf, "Output density dens", "", 1);
    PPdata(bool, 0, flg->output_data.denss.outf, "Output solid density denss",
           "", 1);
    PPdata(bool, 0, flg->output_data.densf.outf, "Output fluid density densf",
           "", 1);
    PPdata(bool, 0, flg->output_data.specht.outf, "Output specific heat specht",
           "", 1);
    // PPdata(bool, 0, flg->output_data.spechts.outf,
    //        "Output solid specific heat spechts", "", 1);
    PPdata(bool, 0, flg->output_data.spechtf.outf,
           "Output fluid specific heat spechtf", "", 1);
    PPdata(bool, 0, flg->output_data.thc.outf,
           "Output thermal conductivity thc", "", 1);
    PPdata(bool, 0, flg->output_data.thcs.outf,
           "Output solid thermal conductivity thcs", "", 1);
    PPdata(bool, 0, flg->output_data.thcf.outf,
           "Output fluid thermal conductivety thcf", "", 1);
    PPdata(bool, 0, flg->output_data.mu.outf, "Output viscousity mu", "", 1);

    if (flg->solute_diff == ON) {
      PPdata(bool, 0, flg->output_data.t_liq.outf,
             "Output liquidus temperature t_liq", "", 1);
      PPdata(bool, 0, flg->output_data.t_soli.outf,
             "Output solidus temperature t_soli", "", 1);
      PPdata(bool, 0, flg->output_data.latent.outf, "Output latent heat latent",
             "", 1);
      PPdata(bool, 0, flg->output_data.diff_g.outf,
             "Output gas diffusivity diff_g", "", 1);
    }

    PPdata(bool, 0, flg->output_data.emi.outf, "Output emissivity emi", "", 1);

    if (flg->oxidation == ON) {
      PPdata(bool, 0, flg->output_data.ox_kp.outf,
             "Output oxidation reaction rate ox_kp", "", 1);
      PPdata(bool, 0, flg->output_data.ox_dens.outf,
             "Output Zr density for oxidation ox_dens", "", 1);
      PPdata(bool, 0, flg->output_data.ox_recess_rate.outf,
             "Output oxidation recession rate ox_recess_rate", "", 1);
    }

    PPdata(bool, 0, flg->output_data.bnd.outf, "Output boundary information",
           "", 1);
    PPdata(bool, 0, flg->output_data.uplsflg.outf,
           "Output level set update flags", "", 1);

    PPdata(bool, 0, flg->restart_data.uvw.outf, "Output uvw on restart", "", 1);
    PPdata(bool, 0, flg->restart_data.q.outf, "Output q on restart", "", 1);
    PPdata(bool, 0, flg->restart_data.rad.outf, "Output rad on restart", "", 1);
    if (flg->solute_diff == ON) {
      PPdata(bool, 0, flg->restart_data.Yt.outf, "Output Yt on restart", "", 1);
    }
    if (flg->oxidation == ON) {
      PPdata(bool, 0, flg->restart_data.ox_q.outf, "Output ox_q on restart", "",
             1);
      PPdata(bool, 0, flg->restart_data.ox_f_h2o.outf,
             "Output ox_f_h2o on restart", "", 1);
    }
    if (flg->porous == ON) {
      PPdata(bool, 0, flg->restart_data.sgm.outf, "Output sgm on restart", "",
             1);
    }
    PPdata(bool, 0, flg->restart_data.flux.outf, "Output flux on restart", "",
           1);
    PPdata(bool, 0, flg->restart_data.dens.outf, "Output dens on restart", "",
           1);
    PPdata(bool, 0, flg->restart_data.denss.outf, "Output denss on restart", "",
           1);
    PPdata(bool, 0, flg->restart_data.densf.outf, "Output densf on restart", "",
           1);
    PPdata(bool, 0, flg->restart_data.specht.outf, "Output specht on restart",
           "", 1);
    // PPdata(bool, 0, flg->restart_data.spechts.outf,
    //        "Output spechts on restart", "", 1);
    PPdata(bool, 0, flg->restart_data.spechtf.outf, "Output spechtf on restart",
           "", 1);
    PPdata(bool, 0, flg->restart_data.thc.outf, "Output thc on restart", "", 1);
    PPdata(bool, 0, flg->restart_data.thcs.outf, "Output thcs on restart", "",
           1);
    PPdata(bool, 0, flg->restart_data.thcf.outf, "Output thcf on restart", "",
           1);
    PPdata(bool, 0, flg->restart_data.mu.outf, "Output mu on restart", "", 1);
    if (flg->solute_diff == ON) {
      PPdata(bool, 0, flg->restart_data.t_liq.outf, "Output t_liq on restart",
             "", 1);
      PPdata(bool, 0, flg->restart_data.t_soli.outf, "Output t_soli on restart",
             "", 1);
      PPdata(bool, 0, flg->restart_data.latent.outf, "Output latent on restart",
             "", 1);
      PPdata(bool, 0, flg->restart_data.diff_g.outf, "Output diff_g on restart",
             "", 1);
    }
    if (flg->oxidation == ON) {
      PPdata(bool, 0, flg->restart_data.ox_kp.outf, "Output ox_kp on restart",
             "", 1);
      PPdata(bool, 0, flg->restart_data.ox_dens.outf,
             "Output ox_dens on restart", "", 1);
      PPdata(bool, 0, flg->restart_data.ox_recess_rate.outf,
             "Output ox_recess_rate on restart", "", 1);

      if (flg->h2_absorp_eval == ON) {
        PPdata(bool, 0, flg->restart_data.h2_absorp_eval.outf,
               "Output h2_absorp_eval on restart", "", 1);
        PPdata(bool, 0, flg->restart_data.h2_absorp_P.outf,
               "Output h2_absorp_P on restart", "", 1);
        PPdata(bool, 0, flg->restart_data.h2_absorp_T.outf,
               "Output h2_absorp_T on restart", "", 1);
        PPdata(bool, 0, flg->restart_data.h2_absorp_Zr.outf,
               "Output h2_absorp_Zr on restart", "", 1);
        PPdata(bool, 0, flg->restart_data.h2_absorp_Ks.outf,
               "Output h2_absorp_Ks on restart", "", 1);
      }
    }

    if (flg->lpt_calc == ON) {
      PPdata(bool, 0, flg->restart_data.lpt_pospt.outf,
             "Output lpt_pospt on restart", "", 1);
      PPdata(bool, 0, flg->restart_data.lpt_upt.outf,
             "Output lpt_upt on restart", "", 1);
      PPdata(bool, 0, flg->restart_data.lpt_uf.outf,
             "Output lpt_uf on restart", "", 1);
#ifdef LPTX
      PPdata(bool, 0, flg->restart_data.lpt_muf.outf,
             "Output lpt_muf on restart", "", 1);
      PPdata(bool, 0, flg->restart_data.lpt_densf.outf,
             "Output lpt_densf on restart", "", 1);
      PPdata(bool, 0, flg->restart_data.lpt_cpf.outf,
             "Output lpt_cpf on restart", "", 1);
      PPdata(bool, 0, flg->restart_data.lpt_thcf.outf,
             "Output lpt_thcf on restart", "", 1);
#endif
      PPdata(bool, 0, flg->restart_data.lpt_fupt.outf,
             "Output lpt_fupt on restart", "", 1);
      PPdata(bool, 0, flg->restart_data.lpt_fdt.outf,
             "Output lpt_fdt on restart", "", 1);
#ifdef LPTX
      PPdata(bool, 0, flg->restart_data.lpt_tempf.outf,
             "Output lpt_tempf on restart", "", 1);
      PPdata(bool, 0, flg->restart_data.lpt_gradTf.outf,
             "Output lpt_gradTf on restart", "", 1);
      PPdata(bool, 0, flg->restart_data.lpt_pathf.outf,
             "Output lpt_pathf on restart", "", 1);
      PPdata(bool, 0, flg->restart_data.lpt_mwf.outf,
             "Output lpt_mwf on restart", "", 1);
      PPdata(bool, 0, flg->restart_data.lpt_htrpt.outf,
             "Output lpt_hrtpt on restart", "", 1);
      PPdata(bool, 0, flg->restart_data.lpt_tothtpt.outf,
             "Output lpt_tothtrpt on restart", "", 1);
      PPdata(bool, 0, flg->restart_data.lpt_exit.outf,
             "Output lpt_exit on restart", "", 1);
      PPdata(bool, 0, flg->restart_data.lpt_fbpt.outf,
             "Output lpt_fbpt on restart", "", 1);
      PPdata(bool, 0, flg->restart_data.lpt_fTpt.outf,
             "Output lpt_fTpt on restart", "", 1);
#endif

#ifdef LPTX
      PPdata(bool, 0, flg->restart_data.qpt.outf, "Output qpt on restart", "",
             1);
#endif
    }

    PPdata(bool, 0, flg->restart_data.emi.outf, "Output emi on restart", "", 1);
  }

  return nogood;
}

static void PP_inlet_vel(struct vin_data *vin, const char *dir, int ncompo,
                         int *nogoodp, flags *flg)
{
  int nogood;
  int i;
  int ncomp;
  int raw_ncomp;

  nogood = OFF;

  ncomp = 0;
  raw_ncomp = 0;
  if (vin->comps) {
    ncomp = inlet_component_data_ncomp(vin->comps);
    raw_ncomp = vin->comps->ncomp;
  }

  PPHead3("%s Inlet Velocity", dir);
  PPdata(controllable_type, 0, &vin->u, "Inlet velocity U", "m/s", 1);
  PPdata(controllable_type, 0, &vin->v, "Inlet velocity V", "m/s", 1);
  PPdata(controllable_type, 0, &vin->w, "Inlet velocity W", "m/s", 1);
  PPdata(int, 0, raw_ncomp, "Number of parts of inlet mixture", "", ncomp > 0);
  for (i = 0; i < ncomp; ++i) {
    struct inlet_component_element *e;
    e = inlet_component_data_get(vin->comps, i);
    PPHead3I(2, "Part %d of inlet mixture", i + 1);
    PPdata(component_info_data, 2, &e->comp, "Material ID", "", 1);
    PPdata(controllable_type, 2, &e->ratio, "Ratio of inlet", "", 1);
  }

  if (nogood == ON) {
    if (nogoodp) {
      *nogoodp = ON;
    }
  }
}

static void PP_pout_data(struct pout_data *pout, const char *dir, flags *flg,
                         int *nogood)
{
  PPHead3("%s Pressure Condition", dir);
  PP_out_p_cond(flg, 0, "Condition", pout->cond, "", 1, nogood);
  switch (pout->cond) {
  case OUT_P_COND_NEUMANN:
    break;

  case OUT_P_COND_CONST:
    PP_controllable_type(flg, 0, "Pressure value", &pout->const_p, "Pa", 1,
                         nogood);
    break;

  case OUT_P_COND_INVALID:
    break;
  }
}

static void PP_particle_set_input(flags *flg, domain *cdo,
                                  struct particle_set_input *p,
                                  int *nogoodp)
{
  int nogood;
  nogood = OFF;
#ifdef LPT
  PPdata(int, 0, p->set.nistpt, "Number of particles to inject", "particle(s)",
         1);
  PPdata(double, 0, p->set.x.start,
         "Start position for X-axis of injection area", "m", 1);
  PPdata(double, 0, p->set.x.end, "End position for X-axis of injection area",
         "m", p->set.x.end >= p->set.x.start);
  PPdata(double, 0, p->set.y.start,
         "Start position for Y-axis of injection area", "m", 1);
  PPdata(double, 0, p->set.y.end, "End position for Y-axis of injection area",
         "m", p->set.y.end >= p->set.y.start);
  PPdata(double, 0, p->set.z.start,
         "Start position for Z-axis of injection area", "m", 1);
  PPdata(double, 0, p->set.z.end, "End position for Z-axis of injection area",
         "m", p->set.z.end >= p->set.z.start);
  PPdata(double, 0, p->set.tm.start, "Start time of injection", "s", 1);
  PPdata(double, 0, p->set.tm.end, "End time of injection", "s",
         p->set.tm.end >= p->set.tm.start);
  PPdata(bool, 0, p->set.itrdm, "Inject particles randomly in time range", "",
         1);
  PPdata(double, 0, p->set.di, "Initial diameter of particles", "m",
         p->set.di > 0.0);
  PPdata(double, 0, p->set.ri, "Initial density of particles", "kg/m3",
         p->set.ri > 0.0);
  PPdata(double, 0, p->set.u.x, "Initial X velocity of particles", "m/s", 1);
  PPdata(double, 0, p->set.u.y, "Initial Y velocity of particles", "m/s", 1);
  PPdata(double, 0, p->set.u.z, "Initial Z velocity of particles", "m/s", 1);
#endif

#ifdef LPTX
  type di, ri;
  LPTX_vector u, vs, ve;
  LPTX_param *lp;

  if (!p->set)
    return;

  nogood = OFF;
  lp = jLPTX_param_for_flg(flg);

  PPdata(int, 0, LPTX_particle_init_set_number_of_particles(p->set),
         "Number of particles to inject", "particle(s)", 1);

  vs = LPTX_particle_init_set_range_start(p->set);
  ve = LPTX_particle_init_set_range_end(p->set);
  PPdata(double, 0, LPTX_vector_x(vs),
         "Start position for X-axis of injection area", "m", 1);
  PPdata(double, 0, LPTX_vector_x(ve),
         "End position for X-axis of injection area", "m",
         LPTX_vector_x(ve) >= LPTX_vector_x(vs));
  PPdata(double, 0, LPTX_vector_y(vs),
         "Start position for Y-axis of injection area", "m", 1);
  PPdata(double, 0, LPTX_vector_y(ve),
         "End position for Y-axis of injection area", "m",
         LPTX_vector_y(ve) >= LPTX_vector_y(vs));
  PPdata(double, 0, LPTX_vector_z(vs),
         "Start position for Z-axis of injection area", "m", 1);
  PPdata(double, 0, LPTX_vector_z(ve),
         "End position for Z-axis of injection area", "m",
         LPTX_vector_z(ve) >= LPTX_vector_z(vs));

  PPdata(double, 0, LPTX_particle_init_set_time_start(p->set),
         "Start time of injection", "s", 1);
  PPdata(double, 0, LPTX_particle_init_set_time_end(p->set),
         "End time of injection", "s",
         LPTX_particle_init_set_time_end(p->set) >=
         LPTX_particle_init_set_time_start(p->set));
  PPdata(bool, 0, LPTX_particle_init_set_time_random(p->set) ? ON : OFF,
         "Inject particles randomly in time range", "", 1);

  di = LPTX_particle_init_set_diameter(p->set);
  PPdata(double, 0, di, "Initial diameter of particles", "m", di > 0.0);

  ri = LPTX_particle_init_set_density(p->set);
  PPdata(double, 0, ri, "Initial density of particles", "kg/m3", ri > 0.0);

  u = LPTX_particle_init_set_initial_velocity(p->set);
  PPdata(double, 0, LPTX_vector_x(u), "Initial X velocity of particles", "m/s",
         1);
  PPdata(double, 0, LPTX_vector_y(u), "Initial Y velocity of particles", "m/s",
         1);
  PPdata(double, 0, LPTX_vector_z(u), "Initial Z velocity of particles", "m/s",
         1);

  if (!lp || LPTX_param_want_temperature(lp)) {
    PPdata(double, 0, LPTX_particle_init_set_temperature(p->set),
           "Initial particle temperature", "K", 1);
  }
  if (!lp || LPTX_param_want_thermal_conductivity(lp)) {
    PPdata(double, 0, LPTX_particle_init_set_thermal_conductivity(p->set),
           "Particle thermal conductivity", "W/m.K", 1);
  }
  if (!lp || LPTX_param_want_specific_heat(lp)) {
    PPdata(double, 0, LPTX_particle_init_set_specific_heat(p->set),
           "Particle specific heat", "J/kg.K", 1);
  }

  if (lp)
    LPTX_param_delete(lp);
#endif
  if (nogoodp && nogood == ON)
    *nogoodp = nogood;
}

static void
PP_non_uniform_grid_input_data(struct non_uniform_grid_input_data *p,
                               int *nogoodp, flags *flg)
{
  int nogood = OFF;
  int n;
  struct non_uniform_grid_input_data *h = p;

  p = non_uniform_grid_input_data_next(p);
  for (n = 1; p != h; ++n, p = non_uniform_grid_input_data_next(p)) {
    type rs = non_uniform_grid_input_data_start(p);
    type re = non_uniform_grid_input_data_end(p);
    int ndiv = non_uniform_grid_input_data_ndiv(p);
    non_uniform_grid_function func = non_uniform_grid_input_data_function(p);

    PPHead3("Region %d", n);
    PPdata(double, 0, rs, "Start coord of region", "m", 1);
    PPdata(double, 0, re, "End coord of region", "m", re > rs);
    PPdata(int, 0, ndiv, "Number of cells", "", ndiv > 0);
    PPdata(non_uniform_grid_func, 0, func, "Function to use", "", 1);
  }
  if (n == 1) {
    PPHead3("No regions defined !!");
    nogood = ON;
  }
  if (nogoodp && nogood == ON)
    *nogoodp = nogood;
}

static int print_param_domain(parameter *prm)
{
  flags *flg;
  domain *cdo;
  mpi_param *mpi;
  phase_value *phv;
  laser *lsr;
  int nogood;
  int i, ic;
  int mpic;
  int mpir;

  CSVASSERT(prm);
  CSVASSERT(prm->flg);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->phv);
  CSVASSERT(prm->mpi);
  CSVASSERT(prm->lsr);

  flg = prm->flg;
  cdo = prm->cdo;
  phv = prm->phv;
  mpi = prm->mpi;
  lsr = prm->lsr;
  nogood = OFF;

  PPHead1("Parameter data");
  PPdata(fname, 0, prm->param_file, "File", "", prm->param_file != NULL);

#ifdef JUPITER_MPI
  mpic = (mpi->npe == (flg->pex * flg->pey * flg->pez));
  if (flg->radiation == ON) {
    mpir = (flg->pea > 0 && (mpi->npe_glob - mpi->npe) == flg->pea * mpi->npe);
  } else {
    mpir = (flg->pea <= 0 && (mpi->npe_glob - mpi->npe) == 0);
  }
#else
  mpic = (flg->pex == 1 && flg->pey == 1 && flg->pez == 1);
  mpir = (flg->pea <= 0);
#endif

  PPHead2("Common parameters");
  PPdata(int, 0, flg->pex, "MPI processes(x)", "", flg->pex > 0 && mpic);
  PPdata(int, 0, flg->pey, "MPI processes(y)", "", flg->pey > 0 && mpic);
  PPdata(int, 0, flg->pez, "MPI processes(z)", "", flg->pez > 0 && mpic);
  PPdata(int, 0, flg->pea, "MPI processes(angular)", "", mpir);

  {
    int fnxd = cdo->nx > 0;
    int fnyd = cdo->ny > 0;
    int fnzd = cdo->nz > 0;

    if (flg->has_non_uniform_grid == ON) {
      int gnx = non_uniform_grid_total_ndivs(&cdo->non_uniform_grid_x_head);
      int gny = non_uniform_grid_total_ndivs(&cdo->non_uniform_grid_y_head);
      int gnz = non_uniform_grid_total_ndivs(&cdo->non_uniform_grid_z_head);

      fnxd = fnxd && (gnx % flg->pex == 0);
      fnyd = fnyd && (gny % flg->pey == 0);
      fnzd = fnzd && (gnz % flg->pez == 0);
    }

    PPdata(int, 0, cdo->nx, "Cell(x)", "", fnxd);
    PPdata(int, 0, cdo->ny, "Cell(y)", "", fnyd);
    PPdata(int, 0, cdo->nz, "Cell(z)", "", fnzd);
  }

  PPdata(double, 0, cdo->gLx, "Length(x)", "m", cdo->gLx > 0.0);
  PPdata(double, 0, cdo->gLy, "Length(y)", "m", cdo->gLy > 0.0);
  PPdata(double, 0, cdo->gLz, "Length(z)", "m", cdo->gLz > 0.0);

  PPdata(double, 0, cdo->grav_x, "Gravity, X", "m/s2", 1);
  PPdata(double, 0, cdo->grav_y, "Gravity, Y", "m/s2", 1);
  PPdata(double, 0, cdo->grav_z, "Gravity, Z", "m/s2", 1);

  PPdata(int, 0, cdo->stm, "Stencil, - dir", "cell(s)", cdo->stm == 3);
  PPdata(int, 0, cdo->stp, "Stencil, + dir", "cell(s)", cdo->stp == 4);

  PPdata(int, 0, cdo->NumberOfComponent, "Number of components", "",
         cdo->NumberOfComponent > 0);
  if(flg->multi_layer == ON){
  PPdata(int, 0, cdo->NumberOfLayer, "Number of layers", "",
         cdo->NumberOfLayer > 0);
  }
  PPdata(int, 0, cdo->NBaseComponent, "Number of Solid-Liquid components", "",
         cdo->NBaseComponent > 0);

  if (flg->solute_diff == ON) {
    PPdata(int, 0, cdo->NGasComponent, "Number of Gas components", "",
           cdo->NGasComponent >= 0);
  }

  if (flg->phase_change == ON &&
      (flg->condensation == ON || flg->vaporization == ON)) {
    PPdata(component_info_data, 0, &cdo->vap_cond_liquid_id,
           "Liquid material ID for Vaporization and Condensation", "",
           cdo->vap_cond_liquid_id.d &&
           component_phases_has_liquid(cdo->vap_cond_liquid_id.d->phases) &&
           cdo->vap_cond_liquid_id.d->phase_comps_index >= 0 &&
           cdo->vap_cond_liquid_id.d->comp_index >= 0);
  }

  PPHead2("Boundary parameters");
  if (flg->bct_xm == ISOTHERMAL) {
    PPdata(controllable_type, 0, &cdo->tw_xm, "X- Boundary Temperature", "K",
           1);
  }
  if (flg->bct_xp == ISOTHERMAL) {
    PPdata(controllable_type, 0, &cdo->tw_xp, "X+ Boundary Temperature", "K",
           1);
  }
  if (flg->bct_ym == ISOTHERMAL) {
    PPdata(controllable_type, 0, &cdo->tw_ym, "Y- Boundary Temperature", "K",
           1);
  }
  if (flg->bct_ym == ISOTHERMAL) {
    PPdata(controllable_type, 0, &cdo->tw_yp, "Y+ Boundary Temperature", "K",
           1);
  }
  if (flg->bct_zm == ISOTHERMAL) {
    PPdata(controllable_type, 0, &cdo->tw_zm, "Z- Boundary Temperature", "K",
           1);
  }
  if (flg->bct_zp == ISOTHERMAL) {
    PPdata(controllable_type, 0, &cdo->tw_zp, "Z+ Boundary Temperature", "K",
           1);
  }

  if (flg->bc_xm == INLET) {
    PP_inlet_vel(&cdo->vin_xm, "X-", cdo->NumberOfComponent, &nogood, flg);
  }
  if (flg->bc_xp == INLET) {
    PP_inlet_vel(&cdo->vin_xp, "X+", cdo->NumberOfComponent, &nogood, flg);
  }
  if (flg->bc_ym == INLET) {
    PP_inlet_vel(&cdo->vin_ym, "Y-", cdo->NumberOfComponent, &nogood, flg);
  }
  if (flg->bc_yp == INLET) {
    PP_inlet_vel(&cdo->vin_yp, "Y+", cdo->NumberOfComponent, &nogood, flg);
  }
  if (flg->bc_zm == INLET) {
    PP_inlet_vel(&cdo->vin_zm, "Z-", cdo->NumberOfComponent, &nogood, flg);
  }
  if (flg->bc_zp == INLET) {
    PP_inlet_vel(&cdo->vin_zp, "Z+", cdo->NumberOfComponent, &nogood, flg);
  }

  if (flg->bc_xm == OUT) {
    PP_pout_data(&cdo->p_xm, "X-", flg, &nogood);
  }
  if (flg->bc_xp == OUT) {
    PP_pout_data(&cdo->p_xp, "X+", flg, &nogood);
  }
  if (flg->bc_ym == OUT) {
    PP_pout_data(&cdo->p_ym, "Y-", flg, &nogood);
  }
  if (flg->bc_yp == OUT) {
    PP_pout_data(&cdo->p_yp, "Y+", flg, &nogood);
  }
  if (flg->bc_zm == OUT) {
    PP_pout_data(&cdo->p_zm, "Z-", flg, &nogood);
  }
  if (flg->bc_zp == OUT) {
    PP_pout_data(&cdo->p_zp, "Z+", flg, &nogood);
  }

  if (flg->has_non_uniform_grid == ON) {
    struct non_uniform_grid_input_data *p;

    PPHead2("Variable delta parameters for X axis");
    PP_non_uniform_grid_input_data(&cdo->non_uniform_grid_x_head, &nogood, flg);

    PPHead2("Variable delta parameters for Y axis");
    PP_non_uniform_grid_input_data(&cdo->non_uniform_grid_y_head, &nogood, flg);

    PPHead2("Variable delta parameters for Z axis");
    PP_non_uniform_grid_input_data(&cdo->non_uniform_grid_z_head, &nogood, flg);
  }

  if (cdo->stm >= 0 && cdo->gnx > 0 && cdo->gny > 0 && cdo->gnz > 0 &&
      cdo->nx > 0 && cdo->ny > 0 && cdo->nz && cdo->gx && cdo->gy && cdo->gz &&
      cdo->xv && cdo->yv && cdo->zv) {
    PPHead2("Mesh information (information only)");
    if (flg->has_non_uniform_grid == ON) {
      PPdata(double, 0, cdo->dx, "Minimum mesh width along X-axis", "m", 1);
      PPdata(double, 0, cdo->dy, "Minimum mesh width along Y-axis", "m", 1);
      PPdata(double, 0, cdo->dz, "Minimum mesh width along Z-axis", "m", 1);
    } else {
      PPdata(double, 0, cdo->dx, "Mesh width along X-axis", "m", 1);
      PPdata(double, 0, cdo->dy, "Mesh width along Y-axis", "m", 1);
      PPdata(double, 0, cdo->dz, "Mesh width along Z-axis", "m", 1);
    }
    PPdata(double, 0, cdo->gx[cdo->stm], "Coord of world X start", "m", 1);
    PPdata(double, 0, cdo->gy[cdo->stm], "Coord of world Y start", "m", 1);
    PPdata(double, 0, cdo->gz[cdo->stm], "Coord of world Z start", "m", 1);
    PPdata(double, 0, cdo->gx[cdo->stm + cdo->gnx], "Coord of world X end", "m",
           1);
    PPdata(double, 0, cdo->gy[cdo->stm + cdo->gny], "Coord of world Y end", "m",
           1);
    PPdata(double, 0, cdo->gz[cdo->stm + cdo->gnz], "Coord of world Z end", "m",
           1);
    PPdata(double, 0, cdo->xv[cdo->stm], "Coord of local X start", "m", 1);
    PPdata(double, 0, cdo->yv[cdo->stm], "Coord of local Y start", "m", 1);
    PPdata(double, 0, cdo->zv[cdo->stm], "Coord of local Z start", "m", 1);
    PPdata(double, 0, cdo->xv[cdo->stm + cdo->nx], "Coord of local X end", "m",
           1);
    PPdata(double, 0, cdo->yv[cdo->stm + cdo->ny], "Coord of local Y end", "m",
           1);
    PPdata(double, 0, cdo->zv[cdo->stm + cdo->nz], "Coord of local Z end", "m",
           1);
  }

  PPHead2("Chronological parameters");
  PPdata(double, 0, cdo->dt, "Time step width (ignored)", "s", 1);
  PPdata(double, 0, cdo->cfl_num, "CFL Coeff. for fluid dynamics", "", 1);
  PPdata(double, 0, cdo->diff_num, "CFL Coeff. for thermal eq.", "", 1);

  PPdata(double, 0, cdo->time, "Start time", "s", 1);
  PPdata(double, 0, cdo->tend, "End time", "s", 1);
  PPdata(double_inf, 0, cdo->restart_dump_time, "Restart dump elapsed time",
         "s", 1);

  PPdata(int, 0, cdo->view, "Message print interval", "step(s)", cdo->view > 0);
  PPdata(int, 0, cdo->outs, "Number of Outputs", "", cdo->outs >= 0);
  PPdata(int, 0, cdo->nsub_step_t, "# of substeps for heat_tvd3", "step(s)",
         flg->heat_tvd3 != ON || cdo->nsub_step_t > 0);
  PPdata(int, 0, cdo->nsub_step_mu, "# of substeps for visc_tvd3", "step(s)",
         flg->visc_tvd3 != ON || cdo->nsub_step_mu > 0);
  PPdata(double, 0, cdo->pconst, "Constant pressure value at outflow boundary", "", 1);

  /* PPHead2("Surface tension model parameters"); */
  /* PPdata(double, 0, cdo, width, "surface width", 1); */

  PPHead2("Phase Field parameters");
  PPdata(double, 0, cdo->c_delta, "Coefficient of the surface width in PF method", "",
         cdo->c_delta >= 2.0 && cdo->c_delta <= 3.0);
  PPdata(double, 0, cdo->lambda, "Interface smoothness", "",
         cdo->lambda >= 0.01 && cdo->lambda <= 0.1);
  PPdata(double, 0, cdo->sigma_pf, "Surface tension (depends on physical prop.)", "N/m", 1);
  PPdata(double, 0, cdo->mobility, "Intensity of phase field (anti)diffusion", "",
         cdo->mobility >= 1.0 && cdo->mobility <= 3.0);

  PPHead2("Level-set parameters");
  PPdata(double, 0, cdo->coef_lsts,
         "Coefficient of the time step of level-set iteration", "",
         cdo->coef_lsts > 0.0 && cdo->coef_lsts <= 0.5);

  PPHead2("Contact angle parameters");
  PPdata(double, 0, cdo->contact_angle, "Contact angle", "",
         cdo->contact_angle >= 0.0 && cdo->contact_angle <= 180.0);
  PPdata(int, 0, cdo->CA_iteration, "Iteration number of contact angle extrapolation", "", 1);

  PPHead2("Film drainage model parameters");
  PPdata(double, 0, cdo->film_cell, "film_cell", "", 1);
  PPdata(double, 0, cdo->rapture_thickness, "rapture_thickness", "", 1);
  PPdata(double, 0, cdo->height_threshold, "height_threshold angle", "", 1);

  PPHead2("Porous model parameters");
  PPdata(double, 0, cdo->d_p, "Particle diameter", "m", 1);
  PPdata(double, 0, cdo->U_ref, "Representative velocity (depends on flow field.)", "m/s", 1);
  PPdata(double, 0, cdo->L_ref, "Representative length (depends on domain size.)", "m", 1);

  if (flg->oxidation == ON) {
    PPHead2("Oxidation parameters");

    PPox_component_info_solid(flg, 0, "Zircaloy", '.', &cdo->ox_zry,
                              0, 1, &nogood);
    PPox_component_info_solid(flg, 0, "Zirconium Dioxide", '.', &cdo->ox_zro2,
                              0, 1, &nogood);

    if (!ox_is_enabled_component(&cdo->ox_h2)) {
      PPdata(bool, 0, OFF, "Hydrogen generation", "", 1);
    } else {
      PPdata(bool, 0, ON, "Hydrogen generation", "", 1);
      PPox_component_info_gas(flg, 0, "Hydrogen", '.', &cdo->ox_h2, 1, 0,
                              &nogood);
    }
    if (!ox_is_enabled_component(&cdo->ox_h2o)) {
      PPdata(bool, 0, OFF, "Steam consumption", "", 1);
    } else {
      PPdata(bool, 0, ON, "Steam consumption", "",
             ox_is_enabled_component(&cdo->ox_h2));
      PPox_component_info_gas(flg, 0, "Steam (H2O)", '.', &cdo->ox_h2o,
                              1, 0, &nogood);
    }

    if (flg->ox_kp_model == OX_RRMODEL_TEMPDEP) {
      PPtempdep_property(flg, 0, "Reaction Rate Function", '.', "kg2/m4.s", "K",
                         &cdo->ox_kp, &nogood);
    }

    if (cdo->ox_recess_rate.type != TEMPDEP_PROPERTY_OX_RECESSION) {
      PPtempdep_property(flg, 0, "Recession Rate Function", '.', "m/s2", "K",
                         &cdo->ox_recess_rate, &nogood);
    }

    PPHead3("Miscellaneous oxidation parameters");
    PPdata(double, 0, cdo->ox_q_fac, "Factor to oxidation heat source", "",
           isfinite(cdo->ox_q_fac));

    PPdata(double, 0, cdo->ox_recess_init, "Recession initial thickness", "m",
           isfinite(cdo->ox_recess_init));

    PPdata(double, 0, cdo->ox_recess_min, "Minimum delta to recess", "m",
           isfinite(cdo->ox_recess_min) && cdo->ox_recess_min >= 0.0);

    PPdata(double, 0, cdo->ox_h2o_threshold,
           "Threshold for enough H2O to oxidize", "",
           isfinite(cdo->ox_h2o_threshold) && cdo->ox_h2o_threshold >= 0.0 &&
             cdo->ox_h2o_threshold <= 1.0);

    PPdata(double, 0, cdo->ox_h2o_lset_min_to_recess,
           "Minimum steam lset change to recess", "m",
           isfinite(cdo->ox_h2o_lset_min_to_recess) &&
             cdo->ox_h2o_lset_min_to_recess >= 0.0);

    if (ox_is_enabled_component(&cdo->ox_h2)) {
      PPdata(double, 0, cdo->ox_diff_h2_sol,
             "Diffusion coefficient in solid for generating H2", "m2/s",
             isfinite(cdo->ox_diff_h2_sol));
      PPdata(double, 0, cdo->ox_diff_h2_sol,
             "Diffusion coefficient on surface for generating H2", "m2/s",
             isfinite(cdo->ox_diff_h2_srf));
    }

    if (flg->h2_absorp_eval == ON) {
      PPdata(double, 0, cdo->h2_absorp_base_p,
             "Reference pressure for H2 Absorption", "Pa",
             cdo->h2_absorp_base_p >= 0.0);
    }
  }

  if (component_info_ncompo(&cdo->lpt_mass_fractions) > 0) {
    PPHead2("Components with mass fraction in partilces");
    PPcomponent_info(flg, 0, "having mass fraction in particles",
                     '.', &cdo->lpt_mass_fractions, NULL, NULL, NULL, NULL,
                     NULL, NULL, &nogood);
  }

  if (component_info_ncompo(&cdo->mass_source_g_comps) > 0) {
    struct PPcomponent_info_default_ok_data args;
    component_phases p;
    PPHead2("Components with mass source");

    p = component_phases_none();
    component_phases_set(&p, COMPONENT_PHASE_GAS, 1);
    PPcomponent_info_default_ok_init(&args, 0, p);

    PPcomponent_info(flg, 0, "Components with mass source", '.',
                     &cdo->mass_source_g_comps, NULL, NULL,
                     PPcomponent_info_default_ok_func, &args,
                     NULL, NULL, &nogood);
  }

  if (flg->lpt_calc == ON) {
    int mxpset = -1;
    int nset;
    struct geom_list *lp, *lh;

    PPHead2("LPT parameters");

#ifdef LPT
    mxpset = cLPTmxpset();
    if (mxpset >= 0) {
      PPdata(int, 0, mxpset, "Max # of particle sets (informational)", "set(s)",
             1);
    } else {
      PPdata(charp, 0, "(error)", "Max # of particle sets (informational)", "",
             1);
    }

    PPdata(double, 0, cdo->lpt_outinterval, "Message output interval", "s",
           cdo->lpt_outinterval >= 0.0);
    PPdata(fname, 0, cdo->lpt_outname, "Message output filename", "", 1);
#endif

#ifdef LPTX
    if (flg->lpt_use_constant_Cc == ON) {
      PPdata(double, 0, cdo->lpt_cc, "Cunningham correction", "",
             cdo->lpt_cc > 0.0);
    } else {
      PPdata(double, 0, cdo->lpt_cc_A1, "Cunningham correction coeff A1", "",
             1);
      PPdata(double, 0, cdo->lpt_cc_A2, "Cunningham correction coeff A2", "",
             1);
      PPdata(double, 0, cdo->lpt_cc_A3, "Cunningham correction coeff A3", "",
             1);
    }
    if (flg->lpt_thermophoretic == ON) {
      PPdata(double, 0, cdo->lpt_tp_Cs, "Thermophoretic constant Cs", "", 1);
      PPdata(double, 0, cdo->lpt_tp_Cm, "Thermophoretic constant Cm", "", 1);
      PPdata(double, 0, cdo->lpt_tp_Ct, "Thermophoretic constant Ct", "", 1);
    }

/*
    if (flg->lpt_aerosol_collision == ON ||
        flg->lpt_non_aerosol_collision == ON) {
      PPdata(controllable_type, 0, &cdo->lpt_aerosol_collision_dp_max,
             "Diameter limit for collision", "m", 1);
      PPdata(controllable_type, 0, &cdo->lpt_aerosol_collision_t_ref,
             "Reference temperature for collision", "K", 1);
    }
*/
#endif

    PPHead3("Default restitution coefficient for domain boundary");
    PPdata(double, 0, cdo->lpt_wallref_xm, "on X- boundary", "", 1);
    PPdata(double, 0, cdo->lpt_wallref_xp, "on X+ boundary", "", 1);
    PPdata(double, 0, cdo->lpt_wallref_ym, "on Y- boundary", "", 1);
    PPdata(double, 0, cdo->lpt_wallref_yp, "on Y+ boundary", "", 1);
    PPdata(double, 0, cdo->lpt_wallref_zm, "on Z- boundary", "", 1);
    PPdata(double, 0, cdo->lpt_wallref_zp, "on Z+ boundary", "", 1);

    nset = 0;
    lh = &cdo->lpt_particle_set_head.list;
    geom_list_foreach (lp, lh) {
      struct particle_set_input *p;

      p = particle_set_input_entry(lp);
      nset = nset + 1;
#ifdef HAVE_LPT
      PPHead3("Parameters of Particle Set %d", nset);
      PP_particle_set_input(flg, cdo, p, &nogood);
#endif
    }
#ifdef HAVE_LPT
    if (nset == 0) {
      PPHead3("No particle sets are defined for computation");
    }
#else
    if (nset > 0) {
      PPHead3("%d particle sets are defined, "
              "but omit printing because LPT is not compiled in.",
              nset);
    }
#endif
  }

  /* Radiation parameters */
  if (flg->radiation == ON) {
    PPHead2("Radiation parameters");
    PPdata(int, 0, cdo->nlat, "Division Latitude", "", cdo->nlat > 0);
    PPdata(int, 0, cdo->nlon, "Division Longitude", "", cdo->nlon > 0);
    PPdata(int, 0, cdo->icnt_end, "Step Limit for radiation", "",
           cdo->icnt_end > 0);
    PPdata(int, 0, cdo->picard_max, "Max iter. for I-T composition", "",
           cdo->picard_max > 0);
    PPdata(int, 0, cdo->picard_out, "I-T composition Output offset", "",
           cdo->picard_out > 0);
    PPdata(int, 0, cdo->newton_max, "Max iter. for Newton of T", "",
           cdo->newton_max > 0);
    PPdata(double, 0, cdo->dt_rad, "Time step for radiation", "s",
           cdo->dt_rad > 0.0);
    PPdata(double, 0, cdo->E_cell_err_max, "Max error for radiation field", "",
           cdo->E_cell_err_max > 0.0);
    PPdata(double, 0, cdo->tmp_cell_err_max, "Max error for temperature field",
           "", cdo->tmp_cell_err_max > 0.0);
    PPdata(double, 0, cdo->dtmp_cell_err_max, "Max error for Newton of T", "",
           cdo->dtmp_cell_err_max > 0.0);
  }
  /* Laser parameters */
  if (flg->laser == ON) {
    PPHead2("Laser Irradiation Parameters");
    PPdata(double, 0, lsr->pw0, "Laser Power", "W", 1);
    PPdata(double, 0, lsr->r0, "Irradiation radius", "m", lsr->r0 > 0.0);
    PPdata(double, 0, lsr->lambda, "Wave Length", "m", lsr->lambda > 0.0);
    PPdata(double, 0, lsr->R, "Reflectivity", "", lsr->R >= 0.0 && lsr->R < 1.0);
    PPdata(double, 0, lsr->alpha, "Absorptivity", "/m", lsr->alpha > 0.0);
    PPdata(double, 0, lsr->lsr_x, "Nozzle Position (x)", "m", 1);
    PPdata(double, 0, lsr->lsr_y, "Nozzle Position (y)", "m", 1);
    PPdata(double, 0, lsr->swp_vel, "Sweep velocity", "m/s", 1);
  }

  PPHead2("Miscellaneous parameters");
  PPdata(double, 0, phv->tr, "Room temperature", "K", 1);

  /* Heat sources */
  PPHead1("Heat sources");
  ic = 0;
  if (heat_source_param_has_any(&cdo->heat_sources_head)) {
    struct geom_list *lp, *lh;
    lh = &cdo->heat_sources_head.list;
    geom_list_foreach(lp, lh) {
      heat_source_param *hp;
      hp = heat_source_param_entry(lp);

      ++ic;
      PPHead2("Entry %d", ic);
      PPdata(component_info_data, 0, &hp->comp, "Material ID", "" , 1);
      PPdata(control, 0, hp->control, "Trip control type", "", 1);
      if (!hp->comp.d)
        continue;
      if (component_phases_is_gas_only(hp->comp.d->phases)) {
        PPdata(controllable_type, 0, &hp->q_s, "Heat source for Gas", "W/m3",
               1);
      } else {
        PPdata(controllable_type, 0, &hp->q_s, "Heat source for Solid", "W/m3",
               1);
        PPdata(controllable_type, 0, &hp->q_l, "Heat source for Liquid", "W/m3",
               1);
      }
    }
  }
  if (ic == 0) {
    print_param_support(flg, " * (No heat source)");
  }

  return nogood;
}

static int print_param_physical_properties(parameter *prm)
{
  flags *flg;
  domain *cdo;
  phase_value *phv;
  int i;
  int isol;
  int iliq;
  int nogood;
  struct tm_table_param *tmp;
  struct tm_func2_param *func;
  component_phases liqsol_phases;
  component_data *d;
  table_geometry tabg;

  CSVASSERT(prm);
  CSVASSERT(prm->flg);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->phv);

  liqsol_phases = component_phases_none();
  component_phases_set(&liqsol_phases, COMPONENT_PHASE_SOLID, 1);
  component_phases_set(&liqsol_phases, COMPONENT_PHASE_LIQUID, 1);

  flg = prm->flg;
  cdo = prm->cdo;
  phv = prm->phv;
  nogood = OFF;

  PPHead1("Physical Property data");
  PPHead2("Gas (ID = -1)");
  d = component_data_find_by_jupiter_id(&prm->comps_data_head, -1);
  PPdata(fname, 0, d ? d->fname : NULL, "File", "", d && d->fname);
  PPtempdep_property(flg, 0, "Density", '.', "kg/m3", "K", &phv->rho_g,
                     &nogood);
  PPtempdep_property(flg, 0, "Viscousity", '.', "Pa.s", "K", &phv->mu_g,
                     &nogood);
  PPtempdep_property(flg, 0, "Specific Heat", '.', "J/kg.K", "K",
                     &phv->specht_g, &nogood);
  PPtempdep_property(flg, 0, "Thermal Conductivity", '.', "W/m.K", "K",
                     &phv->thc_g, &nogood);
  PPdata(double, 0, phv->molar_mass_g, "Molar Mass", "g/mol",
         phv->molar_mass_g > 0.0);

  isol = 0;
  iliq = 0;
  for (i = 0; i < cdo->NIBaseComponent; ++i) {
    int tm_sol_from_tab;
    int tm_liq_from_tab;

    PPHead2("Material ID = %d", i);
    d = component_data_find_by_jupiter_id(&prm->comps_data_head, i);
    PPdata(fname, 0, d ? d->fname : NULL, "File", "", d && d->fname);

    tm_sol_from_tab = 0;
    tm_liq_from_tab = 0;
    if (d && flg->solute_diff == ON) {
      struct tm_table_param *const tp[] = {phv->liq_tables, phv->sol_tables};
      int *const itab[] = {&tm_sol_from_tab, &tm_liq_from_tab};
      const int ntp = sizeof(tp) / sizeof(tp[0]);
      for (int itp = 0; itp < ntp; ++itp) {
        struct tm_table_param *p = tp[itp];
        for (; p; p = p->next) {
          if (p->rid.d == d || p->xid.d == d || p->yid.d == d) {
            *(itab[itp]) = 1;
            break;
          }
        }
      }
    }

    /* check for future improvements */
    CSVASSERT(i == d->phase_comps_index);
    PPphase_value_component(flg, 0, tm_sol_from_tab, tm_liq_from_tab,
                            &phv->comps[d->phase_comps_index], &nogood);
  }
  for (; i < cdo->NIComponent; ++i) {
    PPHead2("Material ID = %d (gas only)", i);
    d = component_data_find_by_jupiter_id(&prm->comps_data_head, i);
    PPdata(fname, 0, d ? d->fname : NULL, "File", "", d && d->fname);

    /* check for future improvements */
    CSVASSERT(i == d->phase_comps_index);
    PPphase_value_component_g(flg, 0, &phv->comps[d->phase_comps_index],
                              &nogood);
  }
  if (i == 0) {
    PPHead2("(No material data)");
  }

  PPtm_table_param(flg, 0, "Liquidus Temperature", '-', PP_HEADER_LEN,
                   phv->liq_tables, liqsol_phases, &nogood);
  PPtm_funcs_param(flg, 0, "Liquidus Temperature", '-', PP_HEADER_LEN,
                   phv->liq_funcs, liqsol_phases, &nogood);

  PPtm_table_param(flg, 0, "Solidus Temperature", '-', PP_HEADER_LEN,
                   phv->sol_tables, liqsol_phases, &nogood);
  PPtm_funcs_param(flg, 0, "Solidus Temperature", '-', PP_HEADER_LEN,
                   phv->sol_funcs, liqsol_phases, &nogood);

  PPbinary_diffusivity(flg, 0, "Solute Diffusivity", '-', PP_HEADER_LEN,
                       phv->diff_params, &phv->diff_input_head,
                       cdo->NBaseComponent, 0, &nogood);

  PPbinary_diffusivity(flg, 0, "Gas Diffusivity", '-', PP_HEADER_LEN,
                       phv->diff_g_params, &phv->diff_g_input_head,
                       cdo->NGasComponent + 1, 1, &nogood);
  return nogood;
}

static int print_param_geom_data(parameter *prm, geom_data *data)
{
  flags *flg;
  domain *cdo;
  mpi_param *mpi;
  phase_value *phv;
  int nogood;
  int lc;
  const geom_user_defined_data *ud;
  geom_data_element *data_el;
  struct jupiter_geom_ext_data *ext_data;
  int print_mat;

  CSVASSERT(prm);
  CSVASSERT(prm->flg);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->mpi);

  flg = prm->flg;
  cdo = prm->cdo;
  mpi = prm->mpi;
  phv = prm->phv;

  ud = NULL;
  ext_data = NULL;
  data_el = NULL;
  print_mat = OFF;
  nogood = OFF;

  if (data) {
    ud = geom_data_get_extra_data(data);
    ext_data = geom_user_defined_data_get(ud);
    data_el = geom_data_get_element(data);
    print_mat = ext_data->print_matrix;
  }

  for (lc = 1; data_el; data_el = geom_data_element_next(data_el), ++lc) {
    geom_file_data *fdata;
    geom_init_data *idata;
    geom_shape_data *sdata;
    geom_surface_shape_data *ssdata;
    jupiter_geom_ext_eldata *ext_eldata;
    const char *name;
    int ic;
    int nsurfs;

    PPHead2("Geometry Data Number %d", lc);
    fdata = geom_data_element_get_file(data_el);
    sdata = geom_data_element_get_shape(data_el);
    ssdata = geom_data_element_get_surface_shape(data_el);
    ud = geom_data_element_get_extra_data(data_el);
    name = geom_data_element_get_name(data_el);
    ext_eldata = geom_user_defined_data_get(ud);

    if (name) {
      jcntrl_executive *exe;
      jupiter_geometry_source *src;
      exe = jcntrl_executive_manager_get(prm->controls, name);
      src = NULL;
      if (exe) {
        src = jupiter_geometry_source_downcast(exe);
      }
      PPdata(charp, 0, name, "Geometry name", "",
             src && jupiter_geometry_source_get_geometry(src) == data_el);
    }

    if (ext_eldata && ext_eldata->dump_file) {
      PPdata(fname, 0, ext_eldata->dump_file, "Dump Geometry Data to", "", 1);
      PPdata(binary_output_mode, 0, ext_eldata->dump_united, "Dump output mode",
             "", 1);
    }

    nsurfs = 0;
    if (fdata) {
      const char *afname;
      jupiter_geom_ext_file_data *ext_fldata;
      binary_output_mode rmode;

      afname = geom_file_data_get_alt_file_path(fdata);
      ud = geom_file_data_get_extra_data(fdata);
      ext_fldata = geom_user_defined_data_get(ud);
      if (!ext_fldata) {
        rmode = BINARY_OUTPUT_INVALID;
      } else {
        rmode = ext_fldata->read_mode;
      }

      PPdata(fname, 0, afname, "Geometry Data File", "", 1);
      PPdata(binary_output_mode, 0, rmode, "Read mode", "", 1);
      if (rmode != BINARY_OUTPUT_BYPROCESS) {
        geom_svec3 size, origin, offset, repeat;
        size = geom_file_data_get_size(fdata);
        origin = geom_file_data_get_origin(fdata);
        offset = geom_file_data_get_offset(fdata);
        repeat = geom_file_data_get_repeat(fdata);
        PPdata(geom_svec3, 0, size, "Data File Dimension", "cells", 1);
        PPdata(geom_svec3, 0, origin, "Data File Origin", "cells", 1);
        PPdata(geom_svec3, 0, offset, "Data File Repeat Offset", "cells", 1);
        PPdata(geom_svec3, 0, repeat, "Data File Repeat Count", "", 1);
      }
    } else if (sdata) {
      geom_shape_element *shape_el;
      geom_vec3 origin, offset;
      geom_svec3 repeat;
      int nsub;
      shape_el = geom_shape_data_get_element(sdata);
      origin = geom_shape_data_get_origin(sdata);
      offset = geom_shape_data_get_offset(sdata);
      repeat = geom_shape_data_get_repeat(sdata);
      nsub = geom_shape_data_get_nsub_cell(sdata);
      PPdata(geom_vec3, 0, origin, "Shape Origin", "m", 1);
      PPdata(geom_vec3, 0, offset, "Shape Repeat Offset", "m", 1);
      PPdata(geom_svec3, 0, repeat, "Shape Repeat Count", "", 1);
      PPdata(int, 0, nsub, "Number of Subcells", "", nsub > 0);
      ic = 1;
      for (; shape_el; shape_el = geom_shape_element_next(shape_el), ++ic) {
        geom_shape shape;
        geom_shape_operator eop;
        geom_shape_operator oop;
        geom_shape_type type;
        const geom_user_defined_data *shape_ud;
        jupiter_geom_ext_shp_eldata *shape_data;
        int is_copy;
        shape = geom_shape_element_get_shape(shape_el);
        eop = geom_shape_element_effective_operator(shape_el);
        oop = geom_shape_element_original_operator(shape_el);
        type = geom_shape_element_get_shape_type(shape_el);
        is_copy = geom_shape_element_is_copied(shape_el);
        shape_ud = geom_shape_element_get_extra_data(shape_el);
        shape_data = (jupiter_geom_ext_shp_eldata *)
          geom_user_defined_data_get(shape_ud);

        nsurfs += geom_shape_element_n_enabled_surface(shape_el);

        PPHead3("Shape Definition %d", ic);
        if (!print_mat && is_copy) {
          print_param_support(
            flg, " * (Generated by transformation copy. Skip printing)");
          continue;
        }
        if (print_mat) {
          PPdata(bool_yn, 0, is_copy, "Generated by transformation", "", 1);
        }

        if (print_mat && eop != oop) {
          PPdata(geom_sop, 0, eop, "Effective Shape Operation", "", 1);
          PPdata(geom_sop, 0, oop, "Given Shape Operation", "", 1);
        } else {
          PPdata(geom_sop, 0, oop, "Shape Operation", "", 1);
        }
        switch (type) {
        case GEOM_SHPT_TRANS:
          PPdata(gshape, 0, shape, "Transformation", "", 1);
          break;
        case GEOM_SHPT_BODY:
          PPdata(gshape, 0, shape, "Body Shape", "", 1);
          break;
        case GEOM_SHPT_SPECIAL:
          PPdata(gshape, 0, shape, "Special Command", "", 1);
          break;
        case GEOM_SHPT_INVALID:
          PPdata(gshape, 0, shape, "(unknown shape type)", "", 0);
          break;
        }
        PPgeom_shape_info(flg, 0, "", "m", shape_el, &nogood);

        if (type == GEOM_SHPT_TRANS) {
          int ncopy = geom_shape_element_get_transformation_copy_n(shape_el);
          PPdata(int, 0, ncopy, "Number of copies to generate", "",
                 (eop == GEOM_SOP_SET) ? ncopy == 0 : ncopy > 0);
        }

        if (print_mat && (type == GEOM_SHPT_BODY || type == GEOM_SHPT_TRANS)) {
          geom_mat43 mat;
          mat = *geom_shape_element_get_transform(shape_el);
          PPdata(geom_mat43, 0, mat, "Transformation matrix", "", 1);
        }
      }
      if (ic == 1) {
        print_param_support(flg, " * (No shapes defined or error occured) !!");
        nogood = 1;
      } else {
        PPHead3("End of shape definitions for Data %d", lc);
      }
    } else {
      print_param_support(flg, " * (whole region)");
    }

    if (ssdata && nsurfs > 0) {
      geom_surface_shape_element *shape_el;
      const geom_user_defined_data *shape_ud;
      jupiter_geom_ext_sshp_eldata *shape_data;
      shape_el = geom_surface_shape_data_get_element(ssdata);
      shape_ud = geom_surface_shape_element_get_extra_data(shape_el);
      shape_data = (jupiter_geom_ext_sshp_eldata *)
        geom_user_defined_data_get(shape_ud);

      ic = 1;
      for (; shape_el;
           shape_el = geom_surface_shape_element_next(shape_el), ++ic) {
        geom_shape_operator sop;
        geom_shape_type type;
        geom_surface_shape shape;
        sop = geom_surface_shape_element_shape_operator(shape_el);
        type = geom_surface_shape_element_get_shape_type(shape_el);
        shape = geom_surface_shape_element_get_shape(shape_el);

        PPHead3("Surface Shape Definition %d", ic);
        PPdata(geom_sop, 0, sop, "Shape Operation", "", 1);
        switch(type) {
        case GEOM_SHPT_TRANS:
          PPdata(gsshape, 0, shape, "Transformation", "", 1);
          break;
        case GEOM_SHPT_BODY:
          PPdata(gsshape, 0, shape, "Shape", "", 1);
          break;
        case GEOM_SHPT_SPECIAL:
          PPdata(gsshape, 0, shape, "Special Command", "", 1);
          break;
        case GEOM_SHPT_INVALID:
          PPdata(gsshape, 0, shape, "(unknown shape type)", "", 1);
          break;
        }
        PPgeom_surface_shape_info(flg, 0, "", "m", shape_el, &nogood);
      }
      if (ic == 1) {
        if (nsurfs > 0) {
          print_param_support(flg, " * (no surface shape defined)");
        }
      } else {
        PPHead3("End of surface shape definition for Data %d", lc);
      }
    } else if (nsurfs > 0) {
      print_param_support(flg, " * (no surface shape defined)");
    }

    idata = geom_data_element_get_init(data_el);
    ic = 1;
    if (idata) {
      init_component init_comps;
      geom_init_element *init_el;

      init_comps = init_component_all();
      init_component_for_initial(&init_comps, prm);

      init_el = geom_init_data_get_element(idata);
      for (; init_el; init_el = geom_init_element_next(init_el), ++ic) {
        const char *comp_name;
        const char *base_unit;
        const geom_user_defined_data *comp_ud;
        const geom_user_defined_data *init_ud;
        int comp_id;
        void *comp_data;
        jupiter_geom_ext_init_eldata *init_data;
        int enabled;
        comp_name = geom_init_element_get_comp_name(init_el);
        comp_id = geom_init_element_get_comp_id(init_el);
        comp_ud = geom_init_element_get_comp_data(init_el);
        comp_data = geom_user_defined_data_get(comp_ud);
        init_ud = geom_init_element_get_extra_data(init_el);
        init_data = (jupiter_geom_ext_init_eldata *)
          geom_user_defined_data_get(init_ud);

        enabled = init_component_is_set(&init_comps, comp_id);
        PPHead3I(0, "Initialization %d%s", ic, enabled ? "" : " (unused)");

        PPdata(charp, 0, comp_name, "Target", "", 1);
        switch (comp_id) {
        case INIT_COMPONENT_BOUNDARY:
        {
          struct boundary_init_data *bnd_data;
          bnd_data = (struct boundary_init_data *)comp_data;
          PPgeom_boundary_data(flg, 0, bnd_data, '.', &nogood);
        }
          continue;
        case INIT_COMPONENT_THERMAL_BOUNDARY:
        {
          struct tboundary_init_data *tbnd_data;
          tbnd_data = (struct tboundary_init_data *)comp_data;
          PPgeom_tboundary_data(flg, 0, tbnd_data, '.', &nogood);
        }
          continue;
        case INIT_COMPONENT_SURFACE_BOUNDARY:
        {
          struct surface_boundary_init_data *surf_bnd_data;
          surf_bnd_data = (struct surface_boundary_init_data *)comp_data;
          PPgeom_surface_boundary_data(flg, 0, surf_bnd_data, '.', &nogood);
        }
          continue;
        case INIT_COMPONENT_VOF:
        {
          int good_phase = 0;
          struct init_vof_data *vof_data;
          vof_data = (struct init_vof_data *)comp_data;
          PPgeom_init_vof_data(flg, 0, vof_data, &nogood);
        }
          base_unit = "";
          break;
        case INIT_COMPONENT_LPT_PEWALL_N:
        {
          struct init_lpt_pewall_data *pewall_data;
          pewall_data = (struct init_lpt_pewall_data *)comp_data;
          PPgeom_init_lpt_pewall_data(flg, 0, pewall_data, &nogood);
        }
          base_unit = "";
          break;
        case INIT_COMPONENT_PRESSURE:
          base_unit = "Pa";
          break;
        case INIT_COMPONENT_TEMPERATURE:
          base_unit = "K";
          break;
        case INIT_COMPONENT_VELOCITY_U:
        case INIT_COMPONENT_VELOCITY_V:
        case INIT_COMPONENT_VELOCITY_W:
          base_unit = "m/s";
          break;
        case INIT_COMPONENT_FIXED_HSOURCE:
          base_unit = "W/m3";
          break;
        default:
          CSVUNREACHABLE();
          break;
        }
        {
          double threshold;
          geom_data_operator op;
          geom_init_func fun;
          op = geom_init_element_get_operator(init_el);
          threshold = geom_init_element_get_threshold(init_el);
          fun = geom_init_element_get_func(init_el);

          PPdata(geom_op, 0, op, "Initialization mode", "", 1);
          PPdata(init_func, 0, fun, "Initialization function", "", 1);
          PPdata(double_ns, 0, threshold, "Geometry Threshold", "", 1);
          PPgeom_init_func_info(flg, 0, base_unit, "m", init_el, &nogood);
        }
      }
    }
    if (!idata || ic <= 1) {
      print_param_support(flg, " * (No Initialization)");
    } else {
      PPHead3I(0, "End of Initializations for Data %d", lc);
    }
  }
  if (lc <= 1) {
    print_param_support(flg, " * (No Geometry data) ");
  }
  return nogood;
}

static int print_param_geometry(parameter *prm)
{
  flags *flg;
  domain *cdo;
  mpi_param *mpi;
  int nogood;

  CSVASSERT(prm);
  CSVASSERT(prm->flg);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->mpi);

  flg = prm->flg;
  cdo = prm->cdo;
  mpi = prm->mpi;

  if (flg->geom_in != ON)
    return OFF;

  nogood = OFF;

  PPHead1("Initial Geometry information");
  PPdata(fname, 0, prm->geom_file, "File", "", 1);

  if (print_param_geom_data(prm, prm->geom_sets) == ON) {
    nogood = ON;
  }
  return nogood;
}

static int print_param_control_geometry(parameter *prm)
{
  flags *flg;
  domain *cdo;
  mpi_param *mpi;
  int nogood;

  CSVASSERT(prm);
  CSVASSERT(prm->flg);
  CSVASSERT(prm->cdo);
  CSVASSERT(prm->mpi);

  flg = prm->flg;
  cdo = prm->cdo;
  mpi = prm->mpi;

  nogood = OFF;

  PPHead1("Controlling Geometry information");
  PPdata(fname, 0, prm->control_file, "File", "", 1);

  if (print_param_geom_data(prm, prm->control_sets) == ON) {
    nogood = ON;
  }
  return nogood;
}

static int print_param_end(parameter *prm)
{
  flags *flg;

  CSVASSERT(prm);
  CSVASSERT(prm->flg);

  flg = prm->flg;

  PPHead1("End of Parameter List");
  return OFF;
}

/* #define USE_UNICODE */

void dumb_visualizer(parameter *prm, type *val, int mx, int my, int mz, int stm,
                     int stp, type *xpos, type *ypos, type *zpos, type min,
                     type max, const char *title, const char *axes)
{
  flags *flg;
  char *tmp;
  char *text;
  char *t;
  int izz, j;
  const char *emb;
  int mxy;
  type th[9];
  char axis_name[3];
  int cmin[3];
  int cmax[3];
  int cidx[3];
  int idxm[3];
  type *posp[3];
  const int X = 0;
  const int Y = 1;
  const int Z = 2;
  int r;

#if defined(USE_UNICODE)
  const char *FULL = "\u2588";
  const char *F87P = "\u2593";
  const char *F75P = "\u2593";
  const char *F67P = "\u2592";
  const char *F50P = "\u2592";
  const char *F37P = "\u2592";
  const char *F25P = "\u2591";
  const char *F12P = "\u2591";
  const char *F00P = "\u2591";
  const char *FZER = " ";
  const char *FIVH = "H";
  const char *FIVL = "L";
  const char *FNAN = "N";
  const char *BDHR = "\u2500";
  const char *BDVT = "\u2502";
  const char *BDLT = "\u250C";
  const char *BDLB = "\u2514";
  const char *BDRT = "\u2510";
  const char *BDRB = "\u2518";
  const char *LTEE = "\u251C";
  const char *RTEE = "\u2524";
  const char *BTEE = "\u2534";
  const char *TTEE = "\u252C";
#else
  const char *FULL = "@";
  const char *F87P = "#";
  const char *F75P = "%";
  const char *F67P = "*";
  const char *F50P = "x";
  const char *F37P = "=";
  const char *F25P = "~";
  const char *F12P = "-";
  const char *F00P = ".";
  const char *FZER = " ";
  const char *FIVH = "H";
  const char *FIVL = "L";
  const char *FNAN = "N";
  const char *BDHR = "-";
  const char *BDVT = "|";
  const char *BDLT = "+";
  const char *BDLB = "+";
  const char *BDRT = "+";
  const char *BDRB = "+";
  const char *LTEE = "+";
  const char *RTEE = "+";
  const char *BTEE = "+";
  const char *TTEE = "+";
#endif

  CSVASSERT(prm);
  CSVASSERT(prm->flg);
  CSVASSERT(val);
  CSVASSERT(axes);
  CSVASSERT(!isnan(min) && !isinf(min));
  CSVASSERT(!isnan(max) && !isinf(max));

  flg = prm->flg;

  mxy = mx * my;

  idxm[0] = -1;
  idxm[1] = -1;
  idxm[2] = -1;

  for (j = 0; j < 3; j++) {
    switch (axes[j]) {
    case 'x':
    case 'X':
      idxm[X] = j;
      continue;
    case 'y':
    case 'Y':
      idxm[Y] = j;
      continue;
    case 'z':
    case 'Z':
      idxm[Z] = j;
      continue;
    default:
      break;
    }
    break;
  }
  if (idxm[X] < 0 || idxm[Y] < 0 || idxm[Z] < 0) {
    errno = EINVAL;
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_ERROR, __func__,
               "axes must be three charactors which consist of "
               "x, y or z (case-insensitive).");
    return;
  }

  cmin[idxm[X]] = 0;
  cmin[idxm[Y]] = 0;
  cmin[idxm[Z]] = 0;

  cmax[idxm[X]] = mx;
  cmax[idxm[Y]] = my;
  cmax[idxm[Z]] = mz;

  axis_name[idxm[X]] = 'x';
  axis_name[idxm[Y]] = 'y';
  axis_name[idxm[Z]] = 'z';

  posp[0] = NULL;
  posp[1] = NULL;
  posp[2] = NULL;

  posp[idxm[X]] = xpos;
  posp[idxm[Y]] = ypos;
  posp[idxm[Z]] = zpos;

  errno = 0;
  tmp = calloc(sizeof(char), (cmax[0] - cmin[0]) * 3 + 1);
  if (!tmp) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_ERROR, __func__, CSV_ERR_NOMEM, 0,
              0, NULL);
    return;
  }

  if (min > max) {
    th[0] = max;
    max = min;
    min = th[0];
  }

  th[0] = min;
  th[1] = (max - min) * 0.125 + min;
  th[2] = (max - min) * 0.250 + min;
  th[3] = (max - min) * 0.375 + min;
  th[4] = (max - min) * 0.500 + min;
  th[5] = (max - min) * 0.675 + min;
  th[6] = (max - min) * 0.750 + min;
  th[7] = (max - min) * 0.875 + min;
  th[8] = max;

  if (title) {
    r = jupiter_asprintf(&text, "\n---- %s\n", title);
  } else {
    r = jupiter_asprintf(&text, "\n");
  }
  if (r < 0) {
    text = NULL;
    goto clean;
  }
  r =
    jupiter_asprintf(&t,
                     "%s\n"
                     "'%s':                  x >  %13.6e\n"
                     "'%s':                  x == %13.6e\n"
                     "'%s': %13.6e <  x <  %13.6e\n"
                     "'%s': %13.6e <  x <= %13.6e\n"
                     "'%s': %13.6e <  x <= %13.6e\n"
                     "'%s': %13.6e <  x <= %13.6e\n"
                     "'%s': %13.6e <  x <= %13.6e\n"
                     "'%s': %13.6e <  x <= %13.6e\n"
                     "'%s': %13.6e <  x <= %13.6e\n"
                     "'%s': %13.6e <  x <= %13.6e\n"
                     "'%s':                  x == %13.6e\n"
                     "'%s':                  x <  %13.6e\n"
                     "'%s':                  x is NaN\n"
                     "\n",
                     text, FIVH, th[8], FULL, th[8], F87P, th[7], th[8], F75P,
                     th[6], th[7], F67P, th[5], th[6], F50P, th[4], th[5], F37P,
                     th[3], th[4], F25P, th[2], th[3], F12P, th[1], th[2], F00P,
                     th[0], th[1], FZER, th[0], FIVL, th[0], FNAN);
  if (r < 0)
    goto clean;
  free(text);
  text = t;

  for (cidx[2] = cmax[2] - 1; cidx[2] >= cmin[2]; cidx[2]--) {
    izz = 0;
    for (cidx[0] = cmin[0]; cidx[0] < cmax[0]; cidx[0]++) {
      if (cidx[0] == stm || cidx[0] == cmax[0] - stp - 1) {
        emb = TTEE;
      } else {
        emb = BDHR;
      }
      strcpy(&tmp[izz], emb);
      izz += strlen(emb);
    }
    tmp[izz] = '\0';

    if (posp[2]) {
      r = jupiter_asprintf(&t, "%s    %c = %d (%13.6e)\n", text, axis_name[2],
                           cidx[2], posp[2][cidx[2]]);
    } else {
      r = jupiter_asprintf(&t, "%s    %c = %d\n", text, axis_name[2], cidx[2]);
    }
    if (r < 0)
      goto clean;
    free(text);
    text = t;

    r = jupiter_asprintf(&t, "%s%5c %s%s%s\n", text, axis_name[1], BDLT, tmp,
                         BDRT);
    if (r < 0)
      goto clean;
    free(text);
    text = t;

    for (cidx[1] = cmax[1] - 1; cidx[1] >= cmin[1]; cidx[1]--) {
      izz = 0;
      for (cidx[0] = cmin[0]; cidx[0] < cmax[0]; cidx[0]++) {
        j = cidx[idxm[X]] + mx * cidx[idxm[Y]] + mxy * cidx[idxm[Z]];
        if (isnan(val[j])) {
          emb = FNAN;
        } else if (val[j] < th[0]) {
          emb = FIVL;
        } else if (val[j] == th[0]) {
          emb = FZER;
        } else if (val[j] <= th[1]) {
          emb = F00P;
        } else if (val[j] <= th[2]) {
          emb = F12P;
        } else if (val[j] <= th[3]) {
          emb = F25P;
        } else if (val[j] <= th[4]) {
          emb = F37P;
        } else if (val[j] <= th[5]) {
          emb = F50P;
        } else if (val[j] <= th[6]) {
          emb = F67P;
        } else if (val[j] <= th[7]) {
          emb = F75P;
        } else if (val[j] < th[8]) {
          emb = F87P;
        } else if (val[j] == th[8]) {
          emb = FULL;
        } else {
          emb = FIVH;
        }
        strcpy(&tmp[izz], emb);
        izz += strlen(emb);
      }
      tmp[izz] = '\0';
      if (cidx[1] == 0 || (cidx[1] - stm) % 5 == 0 ||
          cidx[1] == cmax[1] - stp - 1) {
        if (cidx[1] == stm || cidx[1] == cmax[1] - stp - 1) {
          if (posp[1]) {
            r = jupiter_asprintf(&t, "%s% 5d %s%s%s %13.6e\n", text, cidx[1],
                                 LTEE, tmp, RTEE, posp[1][cidx[1]]);
          } else {
            r = jupiter_asprintf(&t, "%s% 5d %s%s%s\n", text, cidx[1], LTEE,
                                 tmp, RTEE);
          }
        } else {
          if (posp[1]) {
            r = jupiter_asprintf(&t, "%s% 5d %s%s%s %13.6e\n", text, cidx[1],
                                 BDVT, tmp, BDVT, posp[1][cidx[1]]);
          } else {
            r = jupiter_asprintf(&t, "%s% 5d %s%s%s\n", text, cidx[1], BDVT,
                                 tmp, BDVT);
          }
        }
      } else {
        if (cidx[1] == stm || cidx[1] == cidx[1] - stp - 1) {
          r = jupiter_asprintf(&t, "%s%5s %s%s%s\n", text, "", LTEE, tmp, RTEE);
        } else {
          r = jupiter_asprintf(&t, "%s%5s %s%s%s\n", text, "", BDVT, tmp, BDVT);
        }
      }
      if (r < 0)
        goto clean;
      free(text);
      text = t;
    }
    izz = 0;
    for (cidx[0] = 0; cidx[0] < cmax[0]; cidx[0]++) {
      if (cidx[0] == stm || cidx[0] == cmax[0] - stp - 1) {
        emb = BTEE;
      } else {
        emb = BDHR;
      }
      strcpy(&tmp[izz], emb);
      izz += strlen(emb);
    }
    tmp[izz] = '\0';
    r = jupiter_asprintf(&t,
                         "%s"
                         "%5s %s%s%s\n"
                         "%5s  0%*d %c\n",
                         text, "", BDLB, tmp, BDRB, "", cmax[0] - 1,
                         cmax[0] - 1, axis_name[0]);
    if (r < 0)
      goto clean;
    free(text);
    text = t;

    if (posp[0]) {
      r = jupiter_asprintf(&t, "%s%5s   %*.6e\n\n", text, "", cmax[0] - stp - 1,
                           posp[0][cmax[0] - stp]);
    }
    if (r < 0)
      goto clean;
    free(text);
    text = t;
  }

  print_param_supportn(flg, text);

clean:
  free(text);
  free(tmp);
}

static void dumb_visualize_boundary_intern(
  parameter *prm, domain *cdo, struct boundary_array *val, int nx, int ny,
  int ncol_per_cel, const char *dirX, const char *dirY, const char *title,
  const char *subtitle, int (*getid)(struct boundary_array *val, ptrdiff_t loc),
  int (*print_cond)(char **buf, int ncol_per_cel, domain *cdo));

/* warning: this function does not write NUL charactor, '\0'. */
static void boundary_id_to_txt(int ncol_per_cel, char buf[ncol_per_cel], int id)
{
  const char *map = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";
  int i;
  int rid;
  for (i = ncol_per_cel - 1; i >= 0; --i) {
    rid = id % 36;
    id /= 36;
    buf[i] = map[rid];
  }
  /* Replace preceding '0' with '.' */
  for (i = 0; i < ncol_per_cel - 1; ++i) {
    if (buf[i] == map[0]) {
      buf[i] = '.';
    } else {
      break;
    }
  }
}

static int fluid_boundary_getid(struct boundary_array *val, ptrdiff_t loc)
{
  fluid_boundary_data *fp;
  fp = NULL;
  if (val->fl) {
    fp = val->fl[loc];
  }
  if (fp) {
    return fp->meta.id;
  } else {
    return -1;
  }
}

static int fluid_boundary_print_cond(char **buf, int ncol_per_cel, domain *cdo)
{
  int r;
  int rsum;
  char *sbuf, *t;
  const char *ct;
  fluid_boundary_data *fp;
  char *idbuf;
  const char *condt;
  int first;

  rsum = 0;
  sbuf = NULL;
  t = NULL;

  idbuf = (char *)malloc(sizeof(char) * ncol_per_cel);
  if (!idbuf) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return -1;
  }

  first = 1;
  fp = &cdo->fluid_boundary_head;
  for (; first || fp != &cdo->fluid_boundary_head;
       fp = fluid_boundary_data_next(fp)) {
    first = 0;
    if (!fp->meta.used)
      continue;

    condt = PP_bnd_value_format_v(fp->cond);
    if (!condt) {
      switch(fp->cond) {
      case BOUNDARY_MPI:
        condt = "(MPI connected boundary)";
        break;
      default:
        condt = "(Invalid boundary)";
        break;
      }
    }

    t = sbuf;
    if (!t) {
      ct = "";
    } else {
      ct = t;
    }
    boundary_id_to_txt(ncol_per_cel, idbuf, fp->meta.id);
    r = jupiter_asprintf(&sbuf, "%s%.*s: %s\n", ct, ncol_per_cel, idbuf, condt);
    if (r < 0)
      goto error;
    free(t);
    rsum += r;

    if (fp->cond == INLET) {
      char *tmp;
      geom_vec3 v;
      int j;
      int ncomp;
      r = controllable_type_format_vec3(&tmp, 'g', -1, -1, &fp->inlet_vel_u,
                                        &fp->inlet_vel_v, &fp->inlet_vel_w);
      if (r < 0)
        goto error;

      r = jupiter_asprintf(&t,
                           "%s%*s     (U, V, W) = %s [m/s]\n"
                           "%*s     (matID, ratio):",
                           sbuf, ncol_per_cel, "", tmp, ncol_per_cel, "");
      free(tmp);
      if (r < 0)
        goto error;
      free(sbuf);
      sbuf = t;
      rsum += r;

      ncomp = 0;
      if (fp->comps)
        ncomp = inlet_component_data_ncomp(fp->comps);
      if (ncomp > 0) {
        for (j = 0; j < ncomp; ++j) {
          struct inlet_component_element *e;
          const char *fmt;
          int offt;

          e = inlet_component_data_get(fp->comps, j);
          r = controllable_type_format(&tmp, 'g', -1, -1, &e->ratio);
          if (r < 0)
            goto error;

          offt = 1;
          fmt = "%s%*s(%d, %s)";
          if (j > 0 && j % 4 == 0) {
            offt = ncol_per_cel;
            fmt = "%s\n%*s                     (%d, %s)";
          }
          r = jupiter_asprintf(&t, fmt, sbuf, offt, "", e->comp.d->jupiter_id,
                               tmp);
          free(tmp);
          if (r < 0)
            goto error;
          free(sbuf);
          sbuf = t;
          rsum += r;
        }
        r = jupiter_asprintf(&t, "%s\n", sbuf);
      } else {
        r = jupiter_asprintf(&t, "%s(nothing for inlet)\n", sbuf);
      }
      if (r < 0)
        goto error;
      free(sbuf);
      sbuf = t;
      rsum += r;
    } else if (fp->cond == OUT) {
      r =
        jupiter_asprintf(&t, "%s%*s     P Condition: %s\n", sbuf, ncol_per_cel,
                         "", PP_out_p_cond_value_format_v(fp->out_p_cond));
      if (r < 0)
        goto error;

      free(sbuf);
      sbuf = t;
      rsum += r;

      if (fp->out_p_cond == OUT_P_COND_CONST) {
        char *tmp;

        r = controllable_type_format(&tmp, 'g', -1, -1, &fp->const_p);
        if (r < 0)
          goto error;

        r = jupiter_asprintf(&t, "%s%*s     Pconst = %s [Pa]\n",
                             sbuf, ncol_per_cel, "", tmp);
        free(tmp);
        if (r < 0)
          goto error;

        free(sbuf);
        sbuf = t;
        rsum += r;
      }
    }
  }
  if (!sbuf && rsum == 0) {
    r = jupiter_asprintf(&sbuf, "");
    if (r < 0) {
      goto error;
    }
    rsum += r;
  }

  *buf = sbuf;
  free(idbuf);
  return rsum;

error:
  free(idbuf);
  free(sbuf);
  return -1;
}

static int thermal_boundary_getid(struct boundary_array *val, ptrdiff_t loc)
{
  thermal_boundary_data *th;
  th = NULL;
  if (val->th) {
    th = val->th[loc];
  }
  if (th) {
    return th->meta.id;
  } else {
    return -1;
  }
}

static int thermal_boundary_print_cond(char **buf, int ncol_per_cel,
                                       domain *cdo)
{
  int r;
  int rsum;
  char *sbuf, *t;
  const char *ct;
  thermal_boundary_data *th;
  char *idbuf;
  const char *condt;
  int first;

  rsum = 0;
  sbuf = NULL;
  t = NULL;

  idbuf = (char *)malloc(sizeof(char) * ncol_per_cel);
  if (!idbuf) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return -1;
  }

  first = 1;
  th = &cdo->thermal_boundary_head;
  for (; first || th != &cdo->thermal_boundary_head;
       th = thermal_boundary_data_next(th)) {
    first = 0;
    if (!th->meta.used)
      continue;

    condt = PP_tbnd_value_format_v(th->cond);
    if (!condt) {
      switch (th->cond) {
      case BOUNDARY_MPI:
        condt = "(MPI connected boundary)";
        break;
      default:
        condt = "(Invalid boundary)";
        break;
      }
    }

    t = sbuf;
    if (!t) {
      ct = "";
    } else {
      ct = t;
    }
    boundary_id_to_txt(ncol_per_cel, idbuf, th->meta.id);
    r = jupiter_asprintf(&sbuf, "%s%.*s: %s\n", ct, ncol_per_cel, idbuf, condt);
    if (r < 0)
      goto error;
    free(t);
    rsum += r;

    if (th->cond == ISOTHERMAL) {
      char *tmp;
      r = controllable_type_format(&tmp, 'g', -1, -1, &th->temperature);
      if (r < 0)
        goto error;

      r = jupiter_asprintf(&t, "%s%*s     Tconst = %s [K]\n", sbuf,
                           ncol_per_cel, "", tmp);
      free(tmp);
      if (r < 0)
        goto error;
      free(sbuf);
      sbuf = t;
      rsum += r;
    }
  }
  if (!sbuf && rsum == 0) {
    r = jupiter_asprintf(&sbuf, "");
    if (r < 0) {
      goto error;
    }
    rsum += r;
  }

  *buf = sbuf;
  free(idbuf);
  return rsum;

error:
  free(idbuf);
  free(sbuf);
  return -1;
}

void dumb_visualize_boundary(parameter *prm, domain *cdo,
                             struct boundary_array *val, int nx, int ny,
                             const char *dirX, const char *dirY,
                             const char *title)
{
  fluid_boundary_data *fl_p;
  thermal_boundary_data *th_p;
  int nfl_d;
  int nth_d;
  int ncol_per_cel;
  int j, jx, jy;
  int idm;

  nfl_d = 0;
  fl_p = fluid_boundary_data_next(&cdo->fluid_boundary_head);
  while (fl_p != &cdo->fluid_boundary_head) {
    fl_p->meta.id = nfl_d + 1;
    fl_p->meta.used = 0;
    nfl_d++;
    fl_p = fluid_boundary_data_next(fl_p);
  }
  nfl_d++;

  nth_d = 0;
  th_p = thermal_boundary_data_next(&cdo->thermal_boundary_head);
  while (th_p != &cdo->thermal_boundary_head) {
    th_p->meta.id = nth_d + 1;
    th_p->meta.used = 0;
    nth_d++;
    th_p = thermal_boundary_data_next(th_p);
  }
  nth_d++;

  /*
   * We use number in base 36, using charactors 0-9, A-Z.
   *
   * `ncol_per_cel` charactors of `M` is reserved for BOUNDARY_MPI
   * condition (aka. 'head').
   */
  ncol_per_cel = nfl_d < nth_d ? nth_d : nfl_d;
  for (j = 0; ncol_per_cel > 0; ++j) {
    ncol_per_cel /= 36;
  }
  ncol_per_cel = j;

  idm = 22; /* M */
  for (j = 0; j < ncol_per_cel; ++j) {
    idm *= 36;
    idm += 22;
  }

  cdo->fluid_boundary_head.meta.id = idm;
  cdo->fluid_boundary_head.meta.used = 0;
  cdo->thermal_boundary_head.meta.id = idm;
  cdo->thermal_boundary_head.meta.used = 0;

  fl_p = fluid_boundary_data_prev(&cdo->fluid_boundary_head);
  while (fl_p != &cdo->fluid_boundary_head) {
    if (fl_p->meta.id < idm)
      break;
    fl_p->meta.id++;
    fl_p = fluid_boundary_data_prev(fl_p);
  }

  th_p = thermal_boundary_data_prev(&cdo->thermal_boundary_head);
  while (th_p != &cdo->thermal_boundary_head) {
    if (th_p->meta.id < idm)
      break;
    th_p->meta.id++;
    th_p = thermal_boundary_data_prev(th_p);
  }

#pragma omp parallel for collapse(2)
  for (jy = 0; jy < ny; ++jy) {
    for (jx = 0; jx < nx; ++jx) {
      ptrdiff_t jj;
      fluid_boundary_data *fp;
      thermal_boundary_data *tp;
      int ufp;
      int utp;

      jj = calc_address(jx, jy, 0, nx, ny, 1);
      fp = NULL;
      tp = NULL;
      if (val->fl)
        fp = val->fl[jj];
      if (val->th)
        tp = val->th[jj];

      if (fp) {
#pragma omp atomic read
        ufp = fp->meta.used;
        if (!ufp) {
#pragma omp atomic write
          fp->meta.used = 1;
        }
      }

      if (tp) {
#pragma omp atomic read
        utp = tp->meta.used;
        if (!utp) {
#pragma omp atomic write
          tp->meta.used = 1;
        }
      }
    }
  }

  if (val->fl) {
    dumb_visualize_boundary_intern(prm, cdo, val, nx, ny, ncol_per_cel, dirX,
                                   dirY, title, "fluid", fluid_boundary_getid,
                                   fluid_boundary_print_cond);
  }
  if (val->th) {
    dumb_visualize_boundary_intern(prm, cdo, val, nx, ny, ncol_per_cel, dirX,
                                   dirY, title, "thermal",
                                   thermal_boundary_getid,
                                   thermal_boundary_print_cond);
  }
}

static void dumb_visualize_boundary_intern(
  parameter *prm, domain *cdo, struct boundary_array *val, int nx, int ny,
  int ncol_per_cel, const char *dirX, const char *dirY, const char *title,
  const char *subtitle, int (*getid)(struct boundary_array *val, ptrdiff_t loc),
  int (*print_cond)(char **buf, int ncol_per_cel, domain *cdo))
{
  char *buf, *t;
  char *line;
  int j, jx, jy;
  int izz;
  int r;
  size_t allocsz;
  const char *emb;

#if defined(USE_UNICODE)
  const char *BDHR = "\u2500";
  const char *BDVT = "\u2502";
  const char *BDLT = "\u250C";
  const char *BDLB = "\u2514";
  const char *BDRT = "\u2510";
  const char *BDRB = "\u2518";
#else
  const char *BDHR = "-";
  const char *BDVT = "|";
  const char *BDLT = "+";
  const char *BDLB = "+";
  const char *BDRT = "+";
  const char *BDRB = "+";
#endif

  buf = NULL;
  line = NULL;

  allocsz = ncol_per_cel + 1;
  allocsz *= nx;
  allocsz *= 3;
  allocsz += 1;
  line = calloc(sizeof(char), allocsz);
  if (!line) {
    csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0,
              NULL);
    return;
  }

  r = print_cond(&t, ncol_per_cel, cdo);
  if (r < 0) {
    goto clean;
  }

  if (title) {
    r = jupiter_asprintf(&buf, "\n---- %s (%s)\n\n%s", title, subtitle, t);
  } else {
    r = jupiter_asprintf(&buf, "\n---- (%s boundary)\n\n%s", subtitle, t);
  }
  free(t);
  if (r < 0) {
    buf = NULL;
    goto clean;
  }

  izz = 0;
  for (jx = 0; jx < nx; ++jx) {
    for (j = 0; j < ncol_per_cel; ++j) {
      emb = BDHR;
      strcpy(&line[izz], BDHR);
      izz += strlen(emb);
    }
    if (jx != nx - 1 && ncol_per_cel > 1) {
      emb = BDHR;
      strcpy(&line[izz], BDHR);
      izz += strlen(emb);
    }
  }
  line[izz] = '\0';
  r = jupiter_asprintf(&t, "%s\n%5s %s%s%s", buf, dirY, BDLT, line, BDRT);
  if (r < 0)
    goto clean;
  free(buf);
  buf = t;

  for (jy = ny - 1; jy >= 0; --jy) {
    izz = 0;
    for (jx = 0; jx < nx; ++jx) {
      int id;
      ptrdiff_t jj;

      jj = calc_address(jx, jy, 0, nx, ny, 1);
      id = getid(val, jj);
      boundary_id_to_txt(ncol_per_cel, &line[izz], id);
      izz += ncol_per_cel;

      if (jx != nx - 1 && ncol_per_cel > 1) {
        line[izz] = ' ';
        izz++;
      }
    }
    line[izz] = '\0';
    if (jy % 5 == 0 || jy == ny - 1) {
      r = jupiter_asprintf(&t,
                           "%s\n"
                           "%5d %s%s%s",
                           buf, jy, BDVT, line, BDVT);
    } else {
      r = jupiter_asprintf(&t,
                           "%s\n"
                           "%5s %s%s%s",
                           buf, "", BDVT, line, BDVT);
    }
    if (r < 0)
      goto clean;
    free(buf);
    buf = t;
  }

  izz = 0;
  for (jx = 0; jx < nx; ++jx) {
    for (j = 0; j < ncol_per_cel; ++j) {
      emb = BDHR;
      strcpy(&line[izz], BDHR);
      izz += strlen(emb);
    }
    if (jx != nx - 1 && ncol_per_cel > 1) {
      emb = BDHR;
      strcpy(&line[izz], BDHR);
      izz += strlen(emb);
    }
  }
  line[izz] = '\0';
  jx = nx - 1;
  if (ncol_per_cel > 1) {
    jx *= ncol_per_cel + 1;
  }
  r = jupiter_asprintf(&t,
                       "%s\n"
                       "%5s %s%s%s\n"
                       "%5s  %*d%*d %s\n\n",
                       buf, "", BDLB, line, BDRB, "", ncol_per_cel, 0, jx,
                       nx - 1, dirX);
  if (r < 0)
    goto clean;
  free(buf);
  buf = t;

  print_param_supportn(prm->flg, buf);

clean:
  free(line);
  free(buf);
}

struct surface_boundary_report_data
{
  int id;
  surface_boundary_data *data;
  char *aname;
  const char *name;
  int namelen;
  int nface[3];
  type area;
  type normal_area;
};

static void
surface_boundary_report_data_init(struct surface_boundary_report_data *p)
{
  p->id = -1;
  p->data = NULL;
  p->aname = NULL;
  p->name = NULL;
  p->namelen = 0;
  p->nface[0] = 0;
  p->nface[1] = 0;
  p->nface[2] = 0;
  p->area = 0.0;
  p->normal_area = 0.0;
}

static struct surface_boundary_report_data *
surface_boundary_report_data_new(int n)
{
  struct surface_boundary_report_data *p;

  CSVASSERT(n > 0);

  p = (struct surface_boundary_report_data *)
    malloc(sizeof(struct surface_boundary_report_data) * n);
  if (!p)
    return NULL;

  for (int i = 0; i < n; ++i) {
    surface_boundary_report_data_init(&p[i]);
  }
  return p;
}

static void
surface_boundary_report_data_clear_entry(struct surface_boundary_report_data *p)
{
  CSVASSERT(p);
  if (p->aname)
    free(p->aname);
  surface_boundary_report_data_init(p);
}

static void
surface_boundary_report_data_delete(struct surface_boundary_report_data *p,
                                    int n)
{
  for (int i = 0; i < n; ++i) {
    surface_boundary_report_data_clear_entry(&p[i]);
  }
  free(p);
}

void report_surface_boundary_area(flags *flg, mpi_param *mpi,
                                  struct surface_boundary_data *head,
                                  struct surface_boundary_data **array,
                                  type *bnd_array_u, type *bnd_array_v,
                                  type *bnd_array_w,            //
                                  type *x, type *y, type *z,    //
                                  int mx, int my, int mz,       //
                                  int stmx, int stmy, int stmz, //
                                  int stpx, int stpy, int stpz)
{
  int status;
  int srank;
  int adata;
  int ndata;
  int ldata;
  int maxnamelen;
  int nx, ny, nz;
  ptrdiff_t n;
  int use_id;
  struct surface_boundary_data *p;
  struct surface_boundary_report_data *data;

  CSVASSERT(mpi);
  CSVASSERT(head);
  CSVASSERT(array);
  CSVASSERT(bnd_array_u);
  CSVASSERT(bnd_array_v);
  CSVASSERT(bnd_array_w);
  CSVASSERT(stmx >= 0);
  CSVASSERT(stmy >= 0);
  CSVASSERT(stmz >= 0);
  CSVASSERT(stpx >= 1);
  CSVASSERT(stpy >= 1);
  CSVASSERT(stpz >= 1);
  CSVASSERT(mx > stmx + stpx);
  CSVASSERT(my > stmy + stpy);
  CSVASSERT(mz > stmz + stpz);

  nx = mx - stmx - stpx;
  ny = my - stmy - stpy;
  nz = mz - stmz - stpz;
  n = nx;
  n *= ny;
  n *= nz;

  status = 0;

  p = surface_boundary_data_next(head);
  for (ndata = 0; p != head; p = surface_boundary_data_next(p)) {
    if (p->geom_name) {
      ndata++;
      if (ndata < 0) {
        status = 1;
        break;
      }
    }
  }
  if (for_any_rank(mpi, status))
    return;

  /* Nothing needs to be printed */
  if (for_all_rank(mpi, ndata == 0))
    return;

  adata = ndata;
#ifdef JUPITER_MPI
  if (mpi->npe > 1) {
    MPI_Allreduce(&ndata, &adata, 1, MPI_INT, MPI_SUM, mpi->CommJUPITER);
    if (adata <= 0)
      return;
  }
#endif

  data = surface_boundary_report_data_new(adata);
  if (for_any_rank(mpi, !data)) {
    if (!data) {
      csvperror(__FILE__, __LINE__, 0, CSV_EL_WARN, NULL, CSV_ERR_NOMEM, 0, 0,
                NULL);
    }
    return;
  }

  p = surface_boundary_data_next(head);
  for (ldata = 0; p != head; p = surface_boundary_data_next(p)) {
    if (p->geom_name) {
      int m;
      m = strlen(p->geom_name);
      if (m + 1 < m) {
        status = 1;
        break;
      }
      m += 1;
      data[ldata].data = p;
      data[ldata].namelen = m;
      data[ldata].name = p->geom_name;
      ldata++;
    }
  }
  if (for_any_rank(mpi, status)) {
    surface_boundary_report_data_delete(data, adata);
    return;
  }

  maxnamelen = 0;
  for (int i = 0; i < ldata; ++i) {
    if (data[i].namelen > maxnamelen)
      maxnamelen = data[i].namelen;
  }

  use_id = 0;
#ifdef JUPITER_MPI
  if (mpi->npe > 1) {
    int cdata;
    int last_p;
    int id;
    char *buf;

    use_id = 1;
    MPI_Allreduce(MPI_IN_PLACE, &maxnamelen, 1, MPI_INT, MPI_MAX,
                  mpi->CommJUPITER);

    id = 0;
    buf = NULL;
    last_p = ldata;
    for (srank = 0; srank < mpi->npe; ++srank) {
      if (mpi->rank == srank) {
        cdata = ldata;
      }
      MPI_Bcast(&cdata, 1, MPI_INT, srank, mpi->CommJUPITER);

      for (int i = 0; i < cdata; ++i) {
        int nlen;

        if (mpi->rank == srank) {
          if (data[i].id < 0) {
            nlen = data[i].namelen;
          } else {
            nlen = -1;
          }
        }
        MPI_Bcast(&nlen, 1, MPI_INT, srank, mpi->CommJUPITER);
        if (nlen < 0)
          continue;

        if (!buf) {
          buf = (char *)malloc(maxnamelen);
          if (for_any_rank(mpi, !buf)) {
            if (!buf) {
              csvperror(__FILE__, __LINE__, 0, CSV_EL_WARN, NULL, CSV_ERR_NOMEM,
                        0, 0, NULL);
            }
            status = 1;
            break;
          }
        }

        if (mpi->rank == srank) {
          memcpy(buf, data[i].name, nlen);
        }
        MPI_Bcast(buf, nlen, MPI_CHAR, srank, mpi->CommJUPITER);
        MPI_Bcast(&id, 1, MPI_INT, srank, mpi->CommJUPITER);

        if (mpi->rank == srank) {
          data[i].id = id;
          id++;
        }

        if (mpi->rank != srank) {
          int j;
          for (j = 0; j < last_p; ++j) {
            if (memcmp(buf, data[j].name, nlen) == 0) {
              data[j].id = id;
              break;
            }
          }
          if (j == last_p) {
            data[j].aname = buf;
            data[j].name = buf;
            data[j].namelen = nlen;
            data[j].id = id;
            ++last_p;
            CSVASSERT(last_p <= ndata);

            buf = NULL;
          }
        }
        if (for_any_rank(mpi, status)) {
          surface_boundary_report_data_delete(data, adata);
          return;
        }
      }
    }
    if (buf)
      free(buf);
    ndata = last_p;
  }
#endif

  for (int id = 0; id < ndata; ++id) {
    struct surface_boundary_report_data *repp;
    if (use_id) {
      repp = NULL;
      for (int i = 0; i < ndata; ++i) {
        if (data[i].id == id) {
          repp = &data[id];
          break;
        }
      }
    } else {
      repp = &data[id];
    }
    if (for_any_rank(mpi, !repp))
      break;

    repp->nface[0] = 0;
    repp->nface[1] = 0;
    repp->nface[2] = 0;
    repp->area = 0.0;
    repp->normal_area = 0.0;
    if (repp->data) {
#pragma omp parallel
      {
        type la, lna;
        int nf[3];

        la = 0.0;
        lna = 0.0;
        nf[0] = 0;
        nf[1] = 0;
        nf[2] = 0;

#pragma omp for
        for (ptrdiff_t i = 0; i < 3 * n; ++i) {
          ptrdiff_t ixyz, jj;
          int iax, ix, iy, iz, jx, jy, jz;
          type a;
          type *norm;
          iax = i % 3;
          ixyz = i / 3;
          calc_struct_index(ixyz, nx, ny, nz, &ix, &iy, &iz);
          jx = ix + stmx;
          jy = iy + stmy;
          jz = iz + stmz;
          jj = calc_address(jx, jy, jz, mx, my, mz);
          jj *= 3;
          jj += iax;

          if (array[jj] != repp->data)
            continue;

          a = 0.0;
          switch (iax) {
          case 0: // X-face: y * z
            a = (y[jy + 1] - y[jy]) * (z[jz + 1] - z[jz]);
            norm = bnd_array_u;
            break;
          case 1: // Y-face: x * z
            a = (x[jx + 1] - x[jx]) * (z[jz + 1] - z[jz]);
            norm = bnd_array_v;
            break;
          case 2: // z-face: x * y
            a = (x[jx + 1] - x[jx]) * (y[jy + 1] - y[jy]);
            norm = bnd_array_w;
            break;
          }

          nf[iax] += 1;
          la += a;
          lna += a * fabs(norm[jj]);
        }

#pragma omp critical
        {
          repp->area += la;
          repp->normal_area += lna;
          repp->nface[0] += nf[0];
          repp->nface[1] += nf[1];
          repp->nface[2] += nf[2];
        }
      }
    }

#ifdef JUPITER_MPI
    MPI_Allreduce(MPI_IN_PLACE, repp->nface, 3, MPI_INT, MPI_SUM,
                  mpi->CommJUPITER);
    MPI_Allreduce(MPI_IN_PLACE, &repp->area, 1, MPI_TYPE, MPI_SUM,
                  mpi->CommJUPITER);
    MPI_Allreduce(MPI_IN_PLACE, &repp->normal_area, 1, MPI_TYPE, MPI_SUM,
                  mpi->CommJUPITER);
#endif
    if (mpi->rank == 0) {
      int totf;
      totf = repp->nface[0] + repp->nface[1] + repp->nface[2];
      if (totf > 0) {
        print_param_support(flg,
                            "---- Surface boundary report for geometry: %s\n"
                            "\n"
                            "  * Number of faces applied: %8d\n"
                            "                  ... for X: %8d\n"
                            "                  ... for Y: %8d\n"
                            "                  ... for Z: %8d\n"
                            "  * Total area of boundary    : %17.8e m2\n"
                            "    ... in direction of normal: %17.8e m2\n"
                            "\n",
                            repp->name, totf, repp->nface[0], repp->nface[1],
                            repp->nface[2], repp->area, repp->normal_area);
      } else {
        print_param_support(flg,
                            "---- Surface boundary report for geometry: %s\n"
                            "\n"
                            "  * Number of faces applied: %8d\n"
                            "\n",
                            repp->name, totf);
      }
    } else {
      print_param_support(flg, "");
    }
  }

  surface_boundary_report_data_delete(data, adata);
}
