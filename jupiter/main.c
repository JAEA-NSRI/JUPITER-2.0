/*===========================================================
          GLS_NCompo 2014 / 4 / 12                          |
                                   Susumu Yamashita         |
-----------------------------------------------------------*/
#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#ifdef JUPITER_FPE
#include <fenv.h>
#pragma STD FENV_ACCESS ON
#endif

#include "struct.h"
#include "func.h"
/* YSE: Use CSV utility function */
#include "csvutil.h"
#include "jupiter/grid_data_feeder.h"

#include "os/os.h"

//Level_Set_cuda
#ifdef GPU
#include "LevelSetCuda.h"
#include "level_set_cuda_wrapper.h"
#endif

#ifdef LPT
#include "lpt/LPTbnd.h"
#endif

int main(int argc, char *argv[])
{
  /* YSE: Initialize MPI first (because we need rank number info
          during the setup work) */
  //--- Parameters
  parameter *prm;
  //--- Array allocation
  variable  *val;
  material  *mtl;
  //--- YSE Added: Temporary job start time
  //--- level_set_cuda
	//--- add timer 2022/04
  type job_start, inte_start, inte_end, job_end;
	int i;
  int exitval = EXIT_SUCCESS;


#if defined(JUPITER_MPI)
  MPI_Init(&argc, &argv);
#endif
#ifdef JUPITER_FPE
  feenableexcept(FE_INVALID | FE_DIVBYZERO | FE_OVERFLOW);
#endif

  //
  /* YSE: Set Job start time */
  job_start = cpu_time();

  /* YSE: Handle errors properly */
  errno = 0;
  val = NULL;
  mtl = NULL;
  prm = set_parameters(&argc, &argv, SET_PARAMETERS_ALL, 1);
  if (prm) {
    if (prm->cdo && prm->flg) {
      val = malloc_variable(prm->cdo, prm->flg);
      mtl = malloc_material(prm->cdo, prm->flg); // added prm->flg, 2017/9/20
    }
  } else {
    csvperror("parameters", 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0, NULL);
    free_parameter(prm);
#ifdef JUPITER_MPI
    MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    exit(EXIT_FAILURE);
  }
  /* YSE: Stop if any errors occured */
  if (prm->status == ON) {
    csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL,
               __func__, "Stopping by a previous error. Please check.");
    if (val)
      free_variable(val);
    if (mtl)
      free_material(mtl);
    free_parameter(prm);
#ifdef JUPITER_MPI
    MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    exit(EXIT_FAILURE);
  }
  if (!val || !mtl) {
    csvperror("val, mtl", 0, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0, 0, NULL);
#ifdef JUPITER_MPI
    MPI_Abort(MPI_COMM_WORLD, 1);
#endif
    free_parameter(prm);
    exit(EXIT_FAILURE);
  }

  if (prm->flg->list_fp) {
    fflush(prm->flg->list_fp);
  }
#ifdef JUPITER_MPI
  if (prm->flg->list_fp_mpi != MPI_FILE_NULL) {
    MPI_File_sync(prm->flg->list_fp_mpi);
  }
#endif

  /* Assign allocated val and mtl */
  if (prm->grid_feeder) {
    jupiter_grid_data_feeder_set_val(prm->grid_feeder, val);
    jupiter_grid_data_feeder_set_mtl(prm->grid_feeder, mtl);
  }

  //-- Init variables
  prm->cdo->dt = 0.0;
  prm->cdo->icnt = -1;
  prm->time->job_start = job_start;
  init_variables(val, mtl, prm);
  /* YSE: Stop if any errors occured */
  if (for_any_rank(prm->mpi, prm->status == ON)) goto abort_jupiter;
  //-- Init materials
  materials(mtl, val, prm);
  /* YSE: Stop if any errors occured */
  if (for_any_rank(prm->mpi, prm->status == ON)) goto abort_jupiter;
  //analysis_mass_flowrate(val, mtl, prm);
  //-- Read restart file
  restart(prm->flg->restart, val, mtl, prm);
  /* YSE: Stop if any errors occured */
  if (for_any_rank(prm->mpi, prm->status == ON)) goto abort_jupiter;
  // input level-set function (converted by STL)
  //geometry_in(prm->cdo->icnt, val, prm);
  //-- Post processing
  //post(prm->flg->post_s, prm->flg->post_e, val, mtl, prm);
  //-- Source term (ex. heat source, nozzle, assist gas, ...)
  heat_source(val, mtl, prm);
  /* YSE: Stop if any errors occured */
  if (for_any_rank(prm->mpi, prm->status == ON)) goto abort_jupiter;
  /* YSE: If no restart calculation, set step count to 0. */
  if (prm->cdo->icnt < 0) {
    prm->cdo->icnt = 0;
  }
  //-- Update initial control values
  update_control_values(val, mtl, prm);

  //-- Multi-layer model for numerical bubble coalescence prevention
  multi_layer(val, prm, 0);

  //-- Output data
  output_data(0,val, mtl, prm);
  //-- Post intialization check
  post_initial_check(prm, val, mtl);

  /* YSE: Stop if any errors occured */
  if (for_any_rank(prm->mpi, prm->status == ON)) goto abort_jupiter;
  //-- Show the whole of the configuration before time iteration < 2016 Added by KKE
  /* YSE: flush only if fp is set. */
  if (prm->flg->fp) {
    fflush(prm->flg->fp);
  }
  if (prm->flg->list_fp) {
    fflush(prm->flg->list_fp);
  }
#ifdef JUPITER_MPI
  if (prm->flg->list_fp_mpi != MPI_FILE_NULL) {
    MPI_File_sync(prm->flg->list_fp_mpi);
  }
#endif

  //GPUs
#ifdef GPU
  init_GPUs(prm);
#endif
  //
  //-- Time integration (start) ===========================
  FILE       *fp   = prm->flg->fp;
  domain     *cdo  = prm->cdo;
  timer_main *time = prm->time;
  time->total_job = 0.0;
	inte_start=cpu_time();
  while ( cdo->time <= cdo->tend && cdo->icnt < 999999999 )
    //while ( cdo->icnt < 500)
    {
      if(prm->mpi->rank == 0) {fprintf(fp, "o"); fflush(fp);}
      //-- Time interval
      time->dtctl = dt_control(val, mtl, prm);

      //-- Explicit time integration (Runge-Kutta)
      time->rk3 = tvd_runge_kutta_3(val, mtl, prm);

      //-- Heat conduction
      time->heat = heat_conduction(val->t, mtl, val, prm);

      //-- Radiation
      time->radiation = radiation(val, mtl, prm);

      //-- Phase change -- modified by Chai
      time->phase = phase_change(val, mtl, prm);


      //-- Eutectic reaction
      //time->eutectic = eutectic(val, mtl, prm);

      //-- Divergence free correction (fractional step method)
      time->div = divergence_free(val, mtl, prm);

      time->vof = vof_advection(val, prm);

      //-- Multi-layer model for numerical bubble coalescence prevention
      time->multi_layer = multi_layer(val, prm, 1);

      // solute transportation ! 2017/09/22
      time->solute = solute_transport(val, mtl, prm);

      //-- Oxidation
      time->oxide = oxidation(val, mtl, prm);

      //-- Set material
      time->matel = materials(mtl, val, prm);

      //-- Source term (ex. heat source)
      time->hsource = heat_source(val, mtl, prm);

      //-- LPT
      time->lpt = calc_lpt(val, mtl, prm);

      /*
       * YSE: Update time before output data (Are calculated variables
       * already at new time, aren't they?)
       */
      cdo->time += cdo->dt;
      cdo->icnt++;

      //--- Update control data
      time->control_update = update_control_values(val, mtl, prm);

      //-- Output data and restart data file
      time->data = output_data(0,val, mtl, prm);

      /* YSE: Abort if status is flagged */
      if (for_any_rank(prm->mpi, prm->status == ON)) goto abort_jupiter;

      //-- Print elapsed time
      print_timer(prm);

      /* YSE: Exit time integration loop if the calculation time
         reached to the restart dump time. */
      {
        // total_job is shared in the print_timer function,
        // so time->total_job will be same value in each ranks.
        int dump_restart = OFF;
        if (prm->mpi->rank == 0) {
          if (time->total_job > prm->cdo->restart_dump_time) {
            dump_restart = ON;
          }
        }
#ifdef JUPITER_MPI
        MPI_Bcast(&dump_restart, 1, MPI_INT, 0, prm->mpi->CommJUPITER);
#endif
        if (dump_restart == ON) break;
      }
    }
  //-- Time integration (end) ============================
	inte_end = cpu_time();
  /* YSE: Output restart data and cleanup */
  output_data(1, val, mtl, prm);
  if(for_any_rank(prm->mpi, prm->status != OFF))
    goto abort_jupiter;
  if(prm->mpi->rank == 0)
	{
    printf("Output restart files on icnt = %d\n",cdo->icnt);
	}

	job_end=cpu_time();


cleanup:
  if (prm->mpi->radiation_running == ON) {
    communication_rad(val, mtl, prm, 1);
  }
  free_variable(val);
  free_material(mtl);
  free_parameter(prm);
#ifdef LPT
  cLPTdealloc(1);
  cLPTdealloc(2);
#endif

#ifdef PETSc
  PetscFinalize();
#endif
  /* YSE: MPI is available only if MPI is defined */

#ifdef JUPITER_MPI
  MPI_Finalize();
#endif
  return exitval;

  /* YSE: Process for aborting JUPITER */
abort_jupiter:
  csvperrorf(__FILE__, __LINE__, 0, CSV_EL_FATAL,
             __func__, "Stopping by error(s). Please check.");
  exitval = EXIT_FAILURE;
  goto cleanup;
}

/* YSE: cpu_time() is moved to os.c.re */
