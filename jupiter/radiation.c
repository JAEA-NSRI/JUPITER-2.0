#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "struct.h"
#include "func.h"
#include "os/os.h"

type radiation(variable *val, material *mtl, parameter *prm)
{
  domain *cdo= prm->cdo;
  mpi_param *mpi= prm->mpi;
  type time0= cpu_time();
  int iflg= 0;
  int j, jx, jy, jz;
  int nx=cdo->nx, ny=cdo->ny, nz=cdo->nz, mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  static int n1st= 0;
  static type elapsed_time= 0.0;
  type dt_max= (cdo->dt> cdo->dt_rad)? cdo->dt: cdo->dt_rad;

  if(n1st++ == 0){
    elapsed_time= cdo->dt_rad;
  }

  if(prm->flg->radiation== ON){
    if(elapsed_time+ cdo->dt >= cdo->dt_rad){
      elapsed_time= 0.0;
      // 輻射プロセス終了判定
      if(cdo->time+ dt_max>= cdo->tend || cdo->icnt== cdo->icnt_end- 1){
        iflg= 1;
      }
      /* YSE: Communicate to Radiation if running */
      if (prm->mpi->radiation_running == ON) {
        communication_rad(val, mtl, prm, iflg);
      }
      /* YSE: Radiation is terminated */
      if (iflg) {
        prm->mpi->radiation_running = OFF;
      }
    }

    elapsed_time+= cdo->dt;
  }

  return cpu_time()- time0;
}
