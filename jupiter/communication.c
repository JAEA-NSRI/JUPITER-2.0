#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "control/mpi_controller.h"
#include "func.h"
#include "struct.h"

#include "csvutil.h"

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

void info_decomposition(mpi_param *mpi, FILE *fp)
{
  /* YSE: Support for MPI not enabled */
  int i, j;
  int namelen;
  const char *host_name_print;
  int neighbor_rank[6];
  int rank[3];
#ifdef JUPITER_MPI
  char host_name[MPI_MAX_PROCESSOR_NAME];
#else
  const char *host_name_no_mpi = "(MPI not enabled)";
#endif

  fflush(fp);
#ifdef JUPITER_MPI
  MPI_Barrier(mpi->CommJUPITER);
#endif
  if(mpi->rank == 0) {
    fprintf(fp," =========================================================================================== \n");
    fprintf(fp,"             x  y  z    ");
    fprintf(fp,"(-x) (+x) (-y) (+y) (-z) (+z)\n");
    for (j = 0; j < mpi->npe; ++j) {
      if (j == 0) {
        rank[0] = mpi->rank_x;
        rank[1] = mpi->rank_y;
        rank[2] = mpi->rank_z;
        for (i = 0; i < 6; i++) {
          neighbor_rank[i] = mpi->nrk[i];
        }
#ifdef JUPITER_MPI
        MPI_Get_processor_name(host_name, &namelen);
        host_name_print = host_name;
#else
        host_name_print = host_name_no_mpi;
        namelen = strlen(host_name_print);
#endif
      } else {
#ifdef JUPITER_MPI
        MPI_Recv(rank, 3, MPI_INT, j, 1,
                 mpi->CommJUPITER, MPI_STATUS_IGNORE);
        MPI_Recv(neighbor_rank, 6, MPI_INT, j, 2,
                 mpi->CommJUPITER, MPI_STATUS_IGNORE);
        MPI_Recv(&namelen, 1, MPI_INT, j, 3,
                 mpi->CommJUPITER, MPI_STATUS_IGNORE);
        MPI_Recv(host_name, namelen, MPI_CHAR, j, 4,
                 mpi->CommJUPITER, MPI_STATUS_IGNORE);
        host_name_print = host_name;
#else
        CSVUNREACHABLE();
#endif
      }
      fprintf(fp, " Rank = %3d(%2d,%2d,%2d) : %4d %4d %4d %4d %4d %4d : "
              "Host_name = %.*s\n",
              j, rank[0], rank[1], rank[2],
              neighbor_rank[4], neighbor_rank[5],
              neighbor_rank[2], neighbor_rank[3],
              neighbor_rank[0], neighbor_rank[1],
              namelen, host_name_print);
    }
    fprintf(fp," ------------------------------------------------------------------------------------------- \n");
    fflush(fp);
  } else {
#ifdef JUPITER_MPI
    rank[0] = mpi->rank_x;
    rank[1] = mpi->rank_y;
    rank[2] = mpi->rank_z;
    MPI_Get_processor_name(host_name, &namelen);
    MPI_Send(rank, 3, MPI_INT, 0, 1, mpi->CommJUPITER);
    MPI_Send(mpi->nrk, 6, MPI_INT, 0, 2, mpi->CommJUPITER);
    MPI_Send(&namelen, 1, MPI_INT, 0, 3, mpi->CommJUPITER);
    MPI_Send(host_name, namelen, MPI_CHAR, 0, 4, mpi->CommJUPITER);
#else
    CSVUNREACHABLE();
#endif
  }
#ifdef JUPITER_MPI
  MPI_Barrier(mpi->CommJUPITER);
#endif
}

void set_mpi_for_rank(mpi_param *mpi, int npex, int npey, int npez,
                      int npea, int numGPU, int rank, int *status)
{
  int i;
  int rkxm, rkxp, rkym, rkyp, rkzm, rkzp;

  // default param (= 1CPU)
  mpi->npe =1; mpi->npe_x =1; mpi->npe_y =1; mpi->npe_z =1;
  mpi->rank=0; mpi->rank_x=0; mpi->rank_y=0; mpi->rank_z=0;
  // init neighbor rank
  for(i=0; i < 6; i++) mpi->nrk[i] = -1;

  mpi->npe_glob = 1;
  mpi->rank_glob = 0;
  mpi->npe = 1;
  mpi->rank = 0;
  mpi->rank_x = 0;
  mpi->rank_y = 0;
  mpi->rank_z = 0;
  mpi->rank_xy = 0;
  mpi->rank_yz = 0;
  mpi->rank_xz = 0;

#ifdef JUPITER_MPI
  mpi->rank = rank;
  mpi->npe_x = npex;
  mpi->npe_y = npey;
  mpi->npe_xy = npex * npey;
  mpi->npe_z = npez;
  mpi->npe = mpi->npe_xy * npez;
  mpi->npe_a = npea;
  if (mpi->npe_a < 0) {
    mpi->npe_a = 0;
  }

  if (rank < 0 || rank >= mpi->npe) {
    if (status)
      *status = ON;
  }

  i = 0;
  MPI_Initialized(&i);
  if (i) {
    MPI_Comm_size(MPI_COMM_WORLD, &mpi->npe_glob);
    MPI_Comm_rank(MPI_COMM_WORLD, &mpi->rank_glob);
  }

  if (mpi->npe_x > 0 && mpi->npe_y > 0 && mpi->npe_z > 0) {
    mpi->rank_x = (mpi->rank % mpi->npe_xy) % mpi->npe_x;
    mpi->rank_y = (mpi->rank % mpi->npe_xy) / mpi->npe_x;
    mpi->rank_z = mpi->rank / mpi->npe_xy;
  } else {
    mpi->rank_x = 0;
    mpi->rank_y = 0;
    mpi->rank_z = 0;
  }

  mpi->rank_yz = mpi->rank_y + npey*mpi->rank_z;
  mpi->rank_xz = mpi->rank_x + npex*mpi->rank_z;
  mpi->rank_xy = mpi->rank_x + npex*mpi->rank_y;

  rkxm = mpi->rank_x - 1;
  rkxp = mpi->rank_x + 1;
  rkym = mpi->rank_y - 1;
  rkyp = mpi->rank_y + 1;
  rkzm = mpi->rank_z - 1;
  rkzp = mpi->rank_z + 1;

  // neighbor rank
  if(rkzm > -1        ) mpi->nrk[0] = mpi->rank_x + mpi->npe_x*mpi->rank_y + mpi->npe_xy*rkzm;
  if(rkzp < mpi->npe_z) mpi->nrk[1] = mpi->rank_x + mpi->npe_x*mpi->rank_y + mpi->npe_xy*rkzp;
  if(rkym > -1        ) mpi->nrk[2] = mpi->rank_x + mpi->npe_x*rkym + mpi->npe_xy*mpi->rank_z;
  if(rkyp < mpi->npe_y) mpi->nrk[3] = mpi->rank_x + mpi->npe_x*rkyp + mpi->npe_xy*mpi->rank_z;
  if(rkxm > -1        ) mpi->nrk[4] = rkxm + mpi->npe_x*mpi->rank_y + mpi->npe_xy*mpi->rank_z;
  if(rkxp < mpi->npe_x) mpi->nrk[5] = rkxp + mpi->npe_x*mpi->rank_y + mpi->npe_xy*mpi->rank_z;

#endif
}

mpi_param *mpi_init_param(int argc, char *argv[], int npex, int npey, int npez, int npea, int numGPU, int *status)
{
  int i,ier, rank, npe;
  mpi_param *mpi;

  /* YSE: Error flag. */
  ier = 0;

  mpi = (mpi_param *) malloc( sizeof(mpi_param) );
  /* YSE: Return if not allocated. */
  if (!mpi) return NULL;

  npe = 1;
  rank = 0;
  mpi->radiation_running = OFF;
  mpi->rank = 0;

#ifdef JUPITER_MPI
  /* YSE: MPI_Init is moved to main(). */
  MPI_Comm_size(MPI_COMM_WORLD, &npe);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  // 輻射対応 (npe は, jupite を構成するランク数 (!= npe_glob)
  // mpi_parameter を決める段階では, まだ flg->radiation が使えない
  /* YSE: (FYI) ^ This comment is NOT true (just declared as
   * mpi_init_param does not use flg, so if really required, we can
   * use flg) */
  /* YSE: It's not needed to check mpi->npe_a value */
  MPI_Comm_split(MPI_COMM_WORLD, 0, rank, &mpi->CommJUPITER);
  MPI_Comm_size(mpi->CommJUPITER, &npe);
  MPI_Comm_rank(mpi->CommJUPITER, &rank);
#endif

  set_mpi_for_rank(mpi, npex, npey, npez, npea, numGPU, rank, status);

#ifdef JUPITER_MPI
  mpi->npe = npe; /* Replace with actual value */
  if (mpi->npe < mpi->npe_glob) {
    mpi->radiation_running = ON;
  }

  /* YSE: All of MPI processes should check. */
  if (mpi->npe_x*mpi->npe_y*mpi->npe_z != mpi->npe ||
      (mpi->npe_a*mpi->npe_x*mpi->npe_y*mpi->npe_z) !=
      (mpi->npe_glob - mpi->npe)) {
    if (mpi->rank == 0) {
      /* YSE: Add comment for #PE for radiation */
      printf(" ======================================================\n");
      printf(" CAUTION !!  ncpu is inconsistent !!\n");
      printf(" npe (of JUPITER):   %5d npex: %2d npey: %2d npez: %2d\n", mpi->npe, mpi->npe_x, mpi->npe_y, mpi->npe_z);
      printf(" npe (of RADIATION): %5d npea: %2d (requires: %5d)\n", mpi->npe_glob - mpi->npe, mpi->npe_a, mpi->npe_a*mpi->npe_x*mpi->npe_y*mpi->npe_z);
      printf(" ------------------------------------------------------\n");
      if (status) *status = ON;
    }
    /* YSE: To check remaining of inputs, just raise flag instead of terminate. */
    ier = 1;
  }

  //    MPI_Comm_split(MPI_COMM_WORLD, mpi->rank_x, mpi->rank, &mpi->CommSx);
  //    MPI_Comm_split(MPI_COMM_WORLD, mpi->rank_y, mpi->rank, &mpi->CommSy);
  //    MPI_Comm_split(MPI_COMM_WORLD, mpi->rank_z, mpi->rank, &mpi->CommSz);
  //    MPI_Comm_split(MPI_COMM_WORLD, mpi->rank_yz, mpi->rank, &mpi->CommLx);
  //    MPI_Comm_split(MPI_COMM_WORLD, mpi->rank_xz, mpi->rank, &mpi->CommLy);
  //    MPI_Comm_split(MPI_COMM_WORLD, mpi->rank_xy, mpi->rank, &mpi->CommLz);

// for laser, added at 20200929
  MPI_Comm_split(mpi->CommJUPITER, mpi->rank_yz, mpi->rank, &mpi->CommLx);
  MPI_Comm_split(mpi->CommJUPITER, mpi->rank_xz, mpi->rank, &mpi->CommLy);
  MPI_Comm_split(mpi->CommJUPITER, mpi->rank_xy, mpi->rank, &mpi->CommLz);
  mpi->CommSx = MPI_COMM_NULL;
  mpi->CommSy = MPI_COMM_NULL;
  mpi->CommSz = MPI_COMM_NULL;

  MPI_Barrier(mpi->CommJUPITER);

  if (!ier) {
    info_decomposition(mpi, stdout);
  }

  /* Share the status among processes */
  MPI_Allreduce(MPI_IN_PLACE, &ier, 1, MPI_INT, MPI_LOR, mpi->CommJUPITER);
#endif

  if (status) {
    if (ier) *status = ON;
  }

#ifdef JUPITER_MPI
  mpi->control_controller = jcntrl_mpi_controller_new();
  if (mpi->control_controller) {
    if (!jcntrl_mpi_controller_set_comm(mpi->control_controller,
                                        mpi->CommJUPITER)) {
      if (status)
        *status = ON;
    }
  } else {
    if (status)
      *status = ON;
  }
#else
  mpi->control_controller = NULL;
#endif

  return mpi;
}

void mpi_bcast_radiation_configuration(parameter *prm, FILE *fp)
{
#ifdef JUPITER_MPI
  int I_bcd_flg= 0;
  int npea= prm->mpi->npe_a;
  int npex= prm->mpi->npe_x, npey= prm->mpi->npe_y, npez= prm->mpi->npe_z;
  int nx= prm->cdo->nx, ny= prm->cdo->ny, nz= prm->cdo->nz;
  int picard_max= prm->cdo->picard_max, picard_out= prm->cdo->picard_out, newton_max= prm->cdo->newton_max;
  int nlat= prm->cdo->nlat, nlon= prm->cdo->nlon;
  int NumberOfComponent= prm->cdo->NumberOfComponent;
  int output_result_data_interval= (int)(prm->cdo->tend/(prm->cdo->outs*prm->cdo->dt_rad));

  double dt_rad= (double)(prm->cdo->dt_rad);
  double Lx_min= 0.0, Ly_min= 0.0, Lz_min= 0.0;
  double Lx_max= (double)(prm->cdo->Lx), Ly_max= (double)(prm->cdo->Ly), Lz_max= (double)(prm->cdo->Lz);
  double E_cell_err_max= (double)(prm->cdo->E_cell_err_max);
  double tmp_cell_err_max= (double)(prm->cdo->tmp_cell_err_max);
  double dtmp_cell_err_max= (double)(prm->cdo->dtmp_cell_err_max);

  if(prm->flg->bc_xm== WALL){
    if(prm->flg->bcrad_xm== ISOTHERMAL){
      I_bcd_flg= 100000;
    }
    else if(prm->flg->bcrad_xm== INSULATION || prm->flg->bcrad_xm== DIFFUSION){
      I_bcd_flg= 200000;
    }
    else{
      I_bcd_flg= 300000;
    }
  }

  if(prm->flg->bc_xp== WALL){
    if(prm->flg->bcrad_xp== ISOTHERMAL){
      I_bcd_flg+= 10000;
    }
    else if(prm->flg->bcrad_xp== INSULATION || prm->flg->bcrad_xp== DIFFUSION){
      I_bcd_flg+= 20000;
    }
    else{
      I_bcd_flg+= 30000;
    }
  }

  if(prm->flg->bc_ym== WALL){
    if(prm->flg->bcrad_ym== ISOTHERMAL){
      I_bcd_flg+= 1000;
    }
    else if(prm->flg->bcrad_ym== INSULATION || prm->flg->bcrad_ym== DIFFUSION){
      I_bcd_flg+= 2000;
    }
    else{
      I_bcd_flg+= 3000;
    }
  }

  if(prm->flg->bc_yp== WALL){
    if(prm->flg->bcrad_yp== ISOTHERMAL){
      I_bcd_flg+= 100;
    }
    else if(prm->flg->bcrad_yp== INSULATION || prm->flg->bcrad_yp== DIFFUSION){
      I_bcd_flg+= 200;
    }
    else{
      I_bcd_flg+= 300;
    }
  }

  if(prm->flg->bc_zm== WALL){
    if(prm->flg->bcrad_zm== ISOTHERMAL){
      I_bcd_flg+= 10;
    }
    else if(prm->flg->bcrad_zm== INSULATION || prm->flg->bcrad_zm== DIFFUSION){
      I_bcd_flg+= 20;
    }
    else{
      I_bcd_flg+= 30;
    }
  }

  if(prm->flg->bc_zp== WALL){
    if(prm->flg->bcrad_zp== ISOTHERMAL){
      I_bcd_flg+= 1;
    }
    else if(prm->flg->bcrad_zp== INSULATION || prm->flg->bcrad_zp== DIFFUSION){
      I_bcd_flg+= 2;
    }
    else{
      I_bcd_flg+= 3;
    }
  }

  MPI_Bcast(&npex, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
  MPI_Bcast(&npey, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
  MPI_Bcast(&npez, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
  MPI_Bcast(&npea, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);

  MPI_Bcast(&dt_rad, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD);
  //      fprintf(fp, "MPI_Bcast dt_rad %d\n", prm->mpi->rank);
  if(output_result_data_interval< 1){
    output_result_data_interval= 1;
  }
  MPI_Bcast(&output_result_data_interval, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
  //      fprintf(fp, "MPI_Bcast dt_rad %d\n", prm->mpi->rank);

  MPI_Bcast(&Lx_max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD);
  MPI_Bcast(&Ly_max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD);
  MPI_Bcast(&Lz_max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD);
  MPI_Bcast(&Lx_min, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD);
  MPI_Bcast(&Ly_min, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD);
  MPI_Bcast(&Lz_min, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD);
  //      fprintf(fp, "MPI_Bcast xyz_range %d\n", prm->mpi->rank);

  MPI_Bcast(&nx, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
  //      fprintf(fp, "MPI_Bcast nx %d\n", prm->mpi->rank);
  MPI_Bcast(&ny, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
  //      fprintf(fp, "MPI_Bcast ny %d\n", prm->mpi->rank);
  MPI_Bcast(&nz, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
  //      fprintf(fp, "MPI_Bcast nz %d\n", prm->mpi->rank);

  MPI_Bcast(&nlat, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
  //      fprintf(fp, "MPI_Bcast nlat %d\n", prm->mpi->rank);
  MPI_Bcast(&nlon, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
  //      fprintf(fp, "MPI_Bcast nlon %d\n", prm->mpi->rank);

  MPI_Bcast(&picard_max, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
  //      fprintf(fp, "MPI_Bcast picard_max %d\n", prm->mpi->rank);
  MPI_Bcast(&picard_out, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
  //      fprintf(fp, "MPI_Bcast picard_max %d\n", prm->mpi->rank);
  MPI_Bcast(&newton_max, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
  //      fprintf(fp, "MPI_Bcast newton_max %d\n", prm->mpi->rank);

  MPI_Bcast(&E_cell_err_max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD);
  //      fprintf(fp, "MPI_Bcast picard_max %d\n", prm->mpi->rank);
  MPI_Bcast(&tmp_cell_err_max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD);
  //      fprintf(fp, "MPI_Bcast picard_max %d\n", prm->mpi->rank);
  MPI_Bcast(&dtmp_cell_err_max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD);
  //      fprintf(fp, "MPI_Bcast newton_max %d\n", prm->mpi->rank);

  MPI_Bcast(&I_bcd_flg, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
  //      fprintf(fp, "MPI_Bcast I_bcd_flg %d\n", prm->mpi->rank);

  MPI_Bcast(&NumberOfComponent, 1, MPI_INTEGER, 0, MPI_COMM_WORLD);
  //      fprintf(fp, "MPI_Bcast NumberOfComponent %d\n", prm->mpi->rank);
#endif
}

#ifdef JUPITER_MPI
// ==================================================
//    Unpacking recieved data
// --------------------------------------------------
void unpack_z(type *rbuff, int ptr, type *f, int jzs, int mx, int my, int mz, int stcl)
{
  int j,jx,jy,jz;

#pragma omp parallel for private(jz,jy,jx,j)

  for(jz = 0; jz < stcl; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*jy + mx*my*(jzs + jz);
        f[j] = rbuff[ptr + jx + mx*jy + mx*my*jz];
      }
    }
  }
}
void unpack_y(type *rbuff, int ptr, type *f, int jys, int mx, int my, int mz, int stcl)
{
  int j,jx,jy,jz;

#pragma omp parallel for private(jz,jy,jx,j)

  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < stcl; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*(jys + jy) + mx*my*jz;
        f[j] = rbuff[ptr + jx + mx*jy + mx*stcl*jz];
      }
    }
  }
}
void unpack_x(type *rbuff, int ptr, type *f, int jxs, int mx, int my, int mz, int stcl)
{
  int j,jx,jy,jz;

#pragma omp parallel for private(jz,jy,jx,j)

  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < stcl; jx++) {
        j = (jxs + jx) + mx*jy + mx*my*jz;
        f[j] = rbuff[ptr + jx + stcl*jy + stcl*my*jz];
      }
    }
  }
}

void unpack_z_int(int *rbuff, int ptr, int *f, int jzs, int mx, int my, int mz, int stcl)
{
  int j,jx,jy,jz;

#pragma omp parallel for private(jz,jy,jx,j)

  for(jz = 0; jz < stcl; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*jy + mx*my*(jzs + jz);
        f[j] = rbuff[ptr + jx + mx*jy + mx*my*jz];
      }
    }
  }
}

void unpack_y_int(int *rbuff, int ptr, int *f, int jys, int mx, int my, int mz, int stcl)
{
  int j,jx,jy,jz;

#pragma omp parallel for private(jz,jy,jx,j)

  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < stcl; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*(jys + jy) + mx*my*jz;
        f[j] = rbuff[ptr + jx + mx*jy + mx*stcl*jz];
      }
    }
  }
}

void unpack_x_int(int *rbuff, int ptr, int *f, int jxs, int mx, int my, int mz, int stcl)
{
  int j,jx,jy,jz;

#pragma omp parallel for private(jz,jy,jx,j)

  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < stcl; jx++) {
        j = (jxs + jx) + mx*jy + mx*my*jz;
        f[j] = rbuff[ptr + jx + stcl*jy + stcl*my*jz];
      }
    }
  }
}

void unpack_mpi(type *f, type *rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi)
{
  int jxs_w =       0,   jys_s =       0,   jzs_b = 0,
    jxs_e = nx+stcl,   jys_n = ny+stcl,   jzs_t = nz+stcl;   // <= for scalar value
  if(mpi->nrk[0] > -1) unpack_z(rbuff, ptr[0], f, jzs_b, mx,my,mz, stcl); // bottom (z-)
  if(mpi->nrk[1] > -1) unpack_z(rbuff, ptr[1], f, jzs_t, mx,my,mz, stcl); // top    (z+)
  if(mpi->nrk[2] > -1) unpack_y(rbuff, ptr[2], f, jys_s, mx,my,mz, stcl); // south (y-)
  if(mpi->nrk[3] > -1) unpack_y(rbuff, ptr[3], f, jys_n, mx,my,mz, stcl); // north (y+)
  if(mpi->nrk[4] > -1) unpack_x(rbuff, ptr[4], f, jxs_w, mx,my,mz, stcl); // west (x-)
  if(mpi->nrk[5] > -1) unpack_x(rbuff, ptr[5], f, jxs_e, mx,my,mz, stcl); // east (x+)
}

void unpack_mpi_int(int *f, int *rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi)
{
  int jxs_w =       0,   jys_s =       0,   jzs_b = 0,
    jxs_e = nx+stcl,   jys_n = ny+stcl,   jzs_t = nz+stcl;
  if(mpi->nrk[0] > -1) unpack_z_int(rbuff, ptr[0], f, jzs_b, mx,my,mz, stcl); // bottom (z-)
  if(mpi->nrk[1] > -1) unpack_z_int(rbuff, ptr[1], f, jzs_t, mx,my,mz, stcl); // top    (z+)
  if(mpi->nrk[2] > -1) unpack_y_int(rbuff, ptr[2], f, jys_s, mx,my,mz, stcl); // south (y-)
  if(mpi->nrk[3] > -1) unpack_y_int(rbuff, ptr[3], f, jys_n, mx,my,mz, stcl); // north (y+)
  if(mpi->nrk[4] > -1) unpack_x_int(rbuff, ptr[4], f, jxs_w, mx,my,mz, stcl); // west (x-)
  if(mpi->nrk[5] > -1) unpack_x_int(rbuff, ptr[5], f, jxs_e, mx,my,mz, stcl); // east (x+)
}

void unpack_mpi_x(type *f, type *rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi)
{
  int jxs_w = 0, jxs_e = nx+stcl;
  if(mpi->nrk[4] > -1) unpack_x(rbuff, ptr[4], f, jxs_w, mx,my,mz, stcl); // west (x-)
  if(mpi->nrk[5] > -1) unpack_x(rbuff, ptr[5], f, jxs_e, mx,my,mz, stcl); // east (x+)
}

void unpack_mpi_y(type *f, type *rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi)
{
  int jys_s = 0, jys_n = ny+stcl;
  if(mpi->nrk[2] > -1) unpack_y(rbuff, ptr[2], f, jys_s, mx,my,mz, stcl); // south (y-)
  if(mpi->nrk[3] > -1) unpack_y(rbuff, ptr[3], f, jys_n, mx,my,mz, stcl); // north (y+)
}

void unpack_mpi_z(type *f, type *rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi)
{
  int jzs_b = 0, jzs_t = nz+stcl;
  if(mpi->nrk[0] > -1) unpack_z(rbuff, ptr[0], f, jzs_b, mx,my,mz, stcl); // bottom (z-)
  if(mpi->nrk[1] > -1) unpack_z(rbuff, ptr[1], f, jzs_t, mx,my,mz, stcl); // top    (z+)
}

void unpack_mpi_x_int(int *f, int *rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi)
{
  int jxs_w = 0, jxs_e = nx+stcl;
  if(mpi->nrk[4] > -1) unpack_x_int(rbuff, ptr[4], f, jxs_w, mx,my,mz, stcl); // west (x-)
  if(mpi->nrk[5] > -1) unpack_x_int(rbuff, ptr[5], f, jxs_e, mx,my,mz, stcl); // east (x+)
}

void unpack_mpi_y_int(int *f, int *rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi)
{
  int jys_s = 0, jys_n = ny+stcl;
  if(mpi->nrk[2] > -1) unpack_y_int(rbuff, ptr[2], f, jys_s, mx,my,mz, stcl); // south (y-)
  if(mpi->nrk[3] > -1) unpack_y_int(rbuff, ptr[3], f, jys_n, mx,my,mz, stcl); // north (y+)
}

void unpack_mpi_z_int(int *f, int *rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi)
{
  int jzs_b = 0, jzs_t = nz+stcl;
  if(mpi->nrk[0] > -1) unpack_z_int(rbuff, ptr[0], f, jzs_b, mx,my,mz, stcl); // bottom (z-)
  if(mpi->nrk[1] > -1) unpack_z_int(rbuff, ptr[1], f, jzs_t, mx,my,mz, stcl); // top    (z+)
}


// ==================================================
//    Packing data to send
// --------------------------------------------------
void pack_z(type *sbuff, int ptr, type *f, int jzs, int mx, int my, int mz, int stcl)
{
  int j,jx,jy,jz;

#pragma omp parallel for private(jz,jy,jx,j)

  for(jz = 0; jz < stcl; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*jy + mx*my*(jzs + jz);
        sbuff[ptr + jx + mx*jy + mx*my*jz] = f[j];
      }
    }
  }
}
void pack_y(type *sbuff, int ptr, type *f, int jys, int mx, int my, int mz, int stcl)
{
  int j,jx,jy,jz;

#pragma omp parallel for private(jz,jy,jx,j)

  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < stcl; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*(jys + jy) + mx*my*jz;
        sbuff[ptr + jx + mx*jy + mx*stcl*jz] = f[j];
      }
    }
  }
}
void pack_x(type *sbuff, int ptr, type *f, int jxs, int mx, int my, int mz, int stcl)
{
  int j,jx,jy,jz;

#pragma omp parallel for private(jz,jy,jx,j)

  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < stcl; jx++) {
        j = (jxs + jx) + mx*jy + mx*my*jz;
        sbuff[ptr + jx + stcl*jy + stcl*my*jz] = f[j];
      }
    }
  }
}

void pack_z_int(int *sbuff, int ptr, int *f, int jzs, int mx, int my, int mz, int stcl)
{
  int j,jx,jy,jz;

#pragma omp parallel for private(jz,jy,jx,j)

  for(jz = 0; jz < stcl; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*jy + mx*my*(jzs + jz);
        sbuff[ptr + jx + mx*jy + mx*my*jz] = f[j];
      }
    }
  }
}
void pack_y_int(int *sbuff, int ptr, int *f, int jys, int mx, int my, int mz, int stcl)
{
  int j,jx,jy,jz;

#pragma omp parallel for private(jz,jy,jx,j)

  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < stcl; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*(jys + jy) + mx*my*jz;
        sbuff[ptr + jx + mx*jy + mx*stcl*jz] = f[j];
      }
    }
  }
}
void pack_x_int(int *sbuff, int ptr, int *f, int jxs, int mx, int my, int mz, int stcl)
{
  int j,jx,jy,jz;

#pragma omp parallel for private(jz,jy,jx,j)

  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < stcl; jx++) {
        j = (jxs + jx) + mx*jy + mx*my*jz;
        sbuff[ptr + jx + stcl*jy + stcl*my*jz] = f[j];
      }
    }
  }
}

void pack_mpi(type *f, type *sbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi)
{
  int jxs_w = stcl, jys_s = stcl, jzs_b = stcl,
    jxs_e = nx,   jys_n = ny,   jzs_t = nz;   // <= for scalar value
  if(mpi->nrk[0] > -1) pack_z(sbuff, ptr[0], f, jzs_b, mx,my,mz, stcl); // bottom (z-)
  if(mpi->nrk[1] > -1) pack_z(sbuff, ptr[1], f, jzs_t, mx,my,mz, stcl); // top    (z+)
  if(mpi->nrk[2] > -1) pack_y(sbuff, ptr[2], f, jys_s, mx,my,mz, stcl); // south (y-)
  if(mpi->nrk[3] > -1) pack_y(sbuff, ptr[3], f, jys_n, mx,my,mz, stcl); // north (y+)
  if(mpi->nrk[4] > -1) pack_x(sbuff, ptr[4], f, jxs_w, mx,my,mz, stcl); // west (x-)
  if(mpi->nrk[5] > -1) pack_x(sbuff, ptr[5], f, jxs_e, mx,my,mz, stcl); // east (x+)
}
void pack_mpi_x(type *f, type *sbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi)
{
  int jxs_w = stcl, jxs_e = nx;
  if(mpi->nrk[4] > -1) pack_x(sbuff, ptr[4], f, jxs_w, mx,my,mz, stcl); // west (x-)
  if(mpi->nrk[5] > -1) pack_x(sbuff, ptr[5], f, jxs_e, mx,my,mz, stcl); // east (x+)
}
void pack_mpi_y(type *f, type *sbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi)
{
  int jys_s = stcl, jys_n = ny;
  if(mpi->nrk[2] > -1) pack_y(sbuff, ptr[2], f, jys_s, mx,my,mz, stcl); // south (y-)
  if(mpi->nrk[3] > -1) pack_y(sbuff, ptr[3], f, jys_n, mx,my,mz, stcl); // north (y+)
}
void pack_mpi_z(type *f, type *sbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi)
{
  int jzs_b = stcl, jzs_t = nz;
  if(mpi->nrk[0] > -1) pack_z(sbuff, ptr[0], f, jzs_b, mx,my,mz, stcl); // bottom (z-)
  if(mpi->nrk[1] > -1) pack_z(sbuff, ptr[1], f, jzs_t, mx,my,mz, stcl); // top    (z+)
}

void pack_mpi_x_int(int *f, int *sbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi)
{
  int jxs_w = stcl, jxs_e = nx;
  if(mpi->nrk[4] > -1) pack_x_int(sbuff, ptr[4], f, jxs_w, mx,my,mz, stcl); // west (x-)
  if(mpi->nrk[5] > -1) pack_x_int(sbuff, ptr[5], f, jxs_e, mx,my,mz, stcl); // east (x+)
}
void pack_mpi_y_int(int *f, int *sbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi)
{
  int jys_s = stcl, jys_n = ny;
  if(mpi->nrk[2] > -1) pack_y_int(sbuff, ptr[2], f, jys_s, mx,my,mz, stcl); // south (y-)
  if(mpi->nrk[3] > -1) pack_y_int(sbuff, ptr[3], f, jys_n, mx,my,mz, stcl); // north (y+)
}
void pack_mpi_z_int(int *f, int *sbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, mpi_param *mpi)
{
  int jzs_b = stcl, jzs_t = nz;
  if(mpi->nrk[0] > -1) pack_z_int(sbuff, ptr[0], f, jzs_b, mx,my,mz, stcl); // bottom (z-)
  if(mpi->nrk[1] > -1) pack_z_int(sbuff, ptr[1], f, jzs_t, mx,my,mz, stcl); // top    (z+)
}


// ==================================================
//    MPI iSend & iRecv
// --------------------------------------------------
void mpi_isend_irecv(type *sbuff, type *rbuff, int *ptr, int mx, int my, int mz, int stcl,
             MPI_Request *req_send, MPI_Request *req_recv, mpi_param *mpi)
{
  // z-direction
  if(mpi->nrk[0] > -1) {
    MPI_Isend(&sbuff[ptr[0]], mx*my*stcl, MPI_TYPE, mpi->nrk[0], mpi->nrk[0], mpi->CommJUPITER, &req_send[0]);
    MPI_Irecv(&rbuff[ptr[0]], mx*my*stcl, MPI_TYPE, mpi->nrk[0], mpi->rank,   mpi->CommJUPITER, &req_recv[0]);
  }
  if(mpi->nrk[1] > -1) {
    MPI_Isend(&sbuff[ptr[1]], mx*my*stcl, MPI_TYPE, mpi->nrk[1], mpi->nrk[1], mpi->CommJUPITER, &req_send[1]);
    MPI_Irecv(&rbuff[ptr[1]], mx*my*stcl, MPI_TYPE, mpi->nrk[1], mpi->rank,   mpi->CommJUPITER, &req_recv[1]);
  }
  // y-direction
  if(mpi->nrk[2] > -1) {
    MPI_Isend(&sbuff[ptr[2]], mx*mz*stcl, MPI_TYPE, mpi->nrk[2], mpi->nrk[2], mpi->CommJUPITER, &req_send[2]);
    MPI_Irecv(&rbuff[ptr[2]], mx*mz*stcl, MPI_TYPE, mpi->nrk[2], mpi->rank,   mpi->CommJUPITER, &req_recv[2]);
  }
  if(mpi->nrk[3] > -1) {
    MPI_Isend(&sbuff[ptr[3]], mx*mz*stcl, MPI_TYPE, mpi->nrk[3], mpi->nrk[3], mpi->CommJUPITER, &req_send[3]);
    MPI_Irecv(&rbuff[ptr[3]], mx*mz*stcl, MPI_TYPE, mpi->nrk[3], mpi->rank,   mpi->CommJUPITER, &req_recv[3]);
  }
  // x-direction
  if(mpi->nrk[4] > -1) {
    MPI_Irecv(&rbuff[ptr[4]], my*mz*stcl, MPI_TYPE, mpi->nrk[4], mpi->rank,   mpi->CommJUPITER, &req_recv[4]);
    MPI_Isend(&sbuff[ptr[4]], my*mz*stcl, MPI_TYPE, mpi->nrk[4], mpi->nrk[4], mpi->CommJUPITER, &req_send[4]);
  }
  if(mpi->nrk[5] > -1) {
    MPI_Irecv(&rbuff[ptr[5]], my*mz*stcl, MPI_TYPE, mpi->nrk[5], mpi->rank,   mpi->CommJUPITER, &req_recv[5]);
    MPI_Isend(&sbuff[ptr[5]], my*mz*stcl, MPI_TYPE, mpi->nrk[5], mpi->nrk[5], mpi->CommJUPITER, &req_send[5]);
  }
}

void mpi_isend_irecv_x(type *sbuff, type *rbuff, int *ptr, int mx, int my, int mz, int stcl,
               MPI_Request *req_send, MPI_Request *req_recv, mpi_param *mpi)
{
  // x-direction
  if(mpi->nrk[4] > -1) {
    MPI_Irecv(&rbuff[ptr[4]], my*mz*stcl, MPI_TYPE, mpi->nrk[4], mpi->rank,   mpi->CommJUPITER, &req_recv[4]);
    MPI_Isend(&sbuff[ptr[4]], my*mz*stcl, MPI_TYPE, mpi->nrk[4], mpi->nrk[4], mpi->CommJUPITER, &req_send[4]);
  }
  if(mpi->nrk[5] > -1) {
    MPI_Irecv(&rbuff[ptr[5]], my*mz*stcl, MPI_TYPE, mpi->nrk[5], mpi->rank,   mpi->CommJUPITER, &req_recv[5]);
    MPI_Isend(&sbuff[ptr[5]], my*mz*stcl, MPI_TYPE, mpi->nrk[5], mpi->nrk[5], mpi->CommJUPITER, &req_send[5]);
  }
}

void mpi_isend_irecv_y(type *sbuff, type *rbuff, int *ptr, int mx, int my, int mz, int stcl,
               MPI_Request *req_send, MPI_Request *req_recv, mpi_param *mpi)
{
  // y-direction
  if(mpi->nrk[2] > -1) {
    MPI_Isend(&sbuff[ptr[2]], mx*mz*stcl, MPI_TYPE, mpi->nrk[2], mpi->nrk[2], mpi->CommJUPITER, &req_send[2]);
    MPI_Irecv(&rbuff[ptr[2]], mx*mz*stcl, MPI_TYPE, mpi->nrk[2], mpi->rank,   mpi->CommJUPITER, &req_recv[2]);
  }
  if(mpi->nrk[3] > -1) {
    MPI_Isend(&sbuff[ptr[3]], mx*mz*stcl, MPI_TYPE, mpi->nrk[3], mpi->nrk[3], mpi->CommJUPITER, &req_send[3]);
    MPI_Irecv(&rbuff[ptr[3]], mx*mz*stcl, MPI_TYPE, mpi->nrk[3], mpi->rank,   mpi->CommJUPITER, &req_recv[3]);
  }
}

void mpi_isend_irecv_z(type *sbuff, type *rbuff, int *ptr, int mx, int my, int mz, int stcl,
               MPI_Request *req_send, MPI_Request *req_recv, mpi_param *mpi)
{
  // z-direction
  if(mpi->nrk[0] > -1) {
    MPI_Isend(&sbuff[ptr[0]], mx*my*stcl, MPI_TYPE, mpi->nrk[0], mpi->nrk[0], mpi->CommJUPITER, &req_send[0]);
    MPI_Irecv(&rbuff[ptr[0]], mx*my*stcl, MPI_TYPE, mpi->nrk[0], mpi->rank,   mpi->CommJUPITER, &req_recv[0]);
  }
  if(mpi->nrk[1] > -1) {
    MPI_Isend(&sbuff[ptr[1]], mx*my*stcl, MPI_TYPE, mpi->nrk[1], mpi->nrk[1], mpi->CommJUPITER, &req_send[1]);
    MPI_Irecv(&rbuff[ptr[1]], mx*my*stcl, MPI_TYPE, mpi->nrk[1], mpi->rank,   mpi->CommJUPITER, &req_recv[1]);
  }
}

void mpi_isend_irecv_x_int(int *sbuff, int *rbuff, int *ptr, int mx, int my, int mz, int stcl,
               MPI_Request *req_send, MPI_Request *req_recv, mpi_param *mpi)
{
  // x-direction
  if(mpi->nrk[4] > -1) {
    MPI_Irecv(&rbuff[ptr[4]], my*mz*stcl, MPI_INT, mpi->nrk[4], mpi->rank,   mpi->CommJUPITER, &req_recv[4]);
    MPI_Isend(&sbuff[ptr[4]], my*mz*stcl, MPI_INT, mpi->nrk[4], mpi->nrk[4], mpi->CommJUPITER, &req_send[4]);
  }
  if(mpi->nrk[5] > -1) {
    MPI_Irecv(&rbuff[ptr[5]], my*mz*stcl, MPI_INT, mpi->nrk[5], mpi->rank,   mpi->CommJUPITER, &req_recv[5]);
    MPI_Isend(&sbuff[ptr[5]], my*mz*stcl, MPI_INT, mpi->nrk[5], mpi->nrk[5], mpi->CommJUPITER, &req_send[5]);
  }
}

void mpi_isend_irecv_y_int(int *sbuff, int *rbuff, int *ptr, int mx, int my, int mz, int stcl,
               MPI_Request *req_send, MPI_Request *req_recv, mpi_param *mpi)
{
  // y-direction
  if(mpi->nrk[2] > -1) {
    MPI_Isend(&sbuff[ptr[2]], mx*mz*stcl, MPI_INT, mpi->nrk[2], mpi->nrk[2], mpi->CommJUPITER, &req_send[2]);
    MPI_Irecv(&rbuff[ptr[2]], mx*mz*stcl, MPI_INT, mpi->nrk[2], mpi->rank,   mpi->CommJUPITER, &req_recv[2]);
  }
  if(mpi->nrk[3] > -1) {
    MPI_Isend(&sbuff[ptr[3]], mx*mz*stcl, MPI_INT, mpi->nrk[3], mpi->nrk[3], mpi->CommJUPITER, &req_send[3]);
    MPI_Irecv(&rbuff[ptr[3]], mx*mz*stcl, MPI_INT, mpi->nrk[3], mpi->rank,   mpi->CommJUPITER, &req_recv[3]);
  }
}

void mpi_isend_irecv_z_int(int *sbuff, int *rbuff, int *ptr, int mx, int my, int mz, int stcl,
               MPI_Request *req_send, MPI_Request *req_recv, mpi_param *mpi)
{
  // z-direction
  if(mpi->nrk[0] > -1) {
    MPI_Isend(&sbuff[ptr[0]], mx*my*stcl, MPI_INT, mpi->nrk[0], mpi->nrk[0], mpi->CommJUPITER, &req_send[0]);
    MPI_Irecv(&rbuff[ptr[0]], mx*my*stcl, MPI_INT, mpi->nrk[0], mpi->rank,   mpi->CommJUPITER, &req_recv[0]);
  }
  if(mpi->nrk[1] > -1) {
    MPI_Isend(&sbuff[ptr[1]], mx*my*stcl, MPI_INT, mpi->nrk[1], mpi->nrk[1], mpi->CommJUPITER, &req_send[1]);
    MPI_Irecv(&rbuff[ptr[1]], mx*my*stcl, MPI_INT, mpi->nrk[1], mpi->rank,   mpi->CommJUPITER, &req_recv[1]);
  }
}


// ==================================================
//     Calc Pointer
// --------------------------------------------------
int calc_ptr(int *ptr, int mx, int my, int mz, int stcl, mpi_param *mpi)
{
  int  i, num_buffer;
  for (i = 0; i < 7; i++) ptr[i] = 0;

  // [z-surface] calc pointer
  if(mpi->nrk[0] > -1) {
    ptr[0] = 0;
    ptr[6] = mx*my*stcl;
  }
  if(mpi->nrk[1] > -1) {
    ptr[1] = ptr[6];
    ptr[6] += mx*my*stcl;
  }
  // [y-surface] calc pointer
  if(mpi->nrk[2] > -1) {
    ptr[2] = ptr[6];
    ptr[6] += mx*mz*stcl;
  }
  if(mpi->nrk[3] > -1) {
    ptr[3] = ptr[6];
    ptr[6] += mx*mz*stcl;
  }
  // [x-surface] calc pointer
  if(mpi->nrk[4] > -1) {
    ptr[4] = ptr[6];
    ptr[6] += my*mz*stcl;
  }
  if(mpi->nrk[5] > -1) {
    ptr[5] = ptr[6];
    ptr[6] += my*mz*stcl;
  }
  num_buffer = ptr[6];

  return  num_buffer;
}
#endif /* JUPITER_MPI */

#ifdef _TIME_
extern type time_communication;
#endif
void communication(type *f, parameter *prm)
{
#ifdef JUPITER_MPI
  domain    *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;
  MPI_Request  req_send[6],
    req_recv[6];
  MPI_Status   stat_send[6],
    stat_recv[6];
  int  ptr[7],  i,    nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
    stcl=cdo->stm, mx=cdo->mx, my=cdo->my, mz=cdo->mz;
  // send & recv buffer memory allocation
  static int  n1st = 0;
  static type *sbuff,
    *rbuff;
  size_t size = sizeof(type)*((mx*my + my*mz + mz*mx)*2*stcl);
  if(n1st++ == 0) {
#ifdef _MEM_ALIGN_
    sbuff = (type *) _mm_malloc( size,32 );
    rbuff = (type *) _mm_malloc( size,32 );
#else
    sbuff = (type *) malloc( size );
    rbuff = (type *) malloc( size );
#endif
  }
#ifdef _TIME_
  type time0=cpu_time();
#endif

  // calculate initial pointer
  calc_ptr(ptr, mx,my,mz, stcl, mpi);

  // z-direction
  pack_mpi_z(f, sbuff, ptr, nx,ny,nz, mx,my,mz, stcl, mpi);
  mpi_isend_irecv_z(sbuff,rbuff,ptr, mx,my,mz,stcl, req_send,req_recv,mpi);
  for(i = 0; i < 2; i++) {
    if(mpi->nrk[i] > -1) {
      MPI_Wait(&req_send[i], &stat_send[i]);
      MPI_Wait(&req_recv[i], &stat_recv[i]);
    }
  }
  unpack_mpi_z(f, rbuff, ptr, nx,ny,nz, mx,my,mz, stcl, mpi);

  // y-direction
  pack_mpi_y(f, sbuff, ptr, nx,ny,nz, mx,my,mz, stcl, mpi);
  mpi_isend_irecv_y(sbuff,rbuff,ptr, mx,my,mz,stcl, req_send,req_recv,mpi);
  for(i = 2; i < 4; i++) {
    if(mpi->nrk[i] > -1) {
      MPI_Wait(&req_send[i], &stat_send[i]);
      MPI_Wait(&req_recv[i], &stat_recv[i]);
    }
  }
  unpack_mpi_y(f, rbuff, ptr, nx,ny,nz, mx,my,mz, stcl, mpi);

  // x-direction
  pack_mpi_x(f, sbuff, ptr, nx,ny,nz, mx,my,mz, stcl, mpi);
  mpi_isend_irecv_x(sbuff,rbuff,ptr, mx,my,mz,stcl, req_send,req_recv,mpi);
  for(i = 4; i < 6; i++) {
    if(mpi->nrk[i] > -1) {
      MPI_Wait(&req_send[i], &stat_send[i]);
      MPI_Wait(&req_recv[i], &stat_recv[i]);
    }
  }
  unpack_mpi_x(f, rbuff, ptr, nx,ny,nz, mx,my,mz, stcl, mpi);

  MPI_Barrier(mpi->CommJUPITER);
#ifdef _TIME_
  time_communication += cpu_time()-time0;
#endif
#endif /* JUPITER_MPI */
}

#ifdef JUPITER_MPI
void pack_mpi_rad(type *f, double *sbuff_rad, int mx, int my, int mz, int stm, int stp){

  int i, j, k, l, m;
  int rank= 0;

  //      MPI_Comm_rank(MPI_COMM_WORLD, &rank);

  l= m= 0;
  if(sizeof(type)!= sizeof(double)){
    for(i= mx*my*(stm- 1); i< mx*my*(mz- stp+ 1); i+= mx*my){
      for(j= mx*(stm- 1); j< mx*(my- stp+ 1); j+= mx){
        for(k= stm- 1; k< mx- stp+ 1; k++){
          m= i+ j+ k;
          sbuff_rad[l++]= (double)f[m];
        }
      }
    }
  }
  else{
    for(i= mx*my*(stm- 1); i< mx*my*(mz- stp+ 1); i+= mx*my){
      for(j= mx*(stm- 1); j< mx*(my- stp+ 1); j+= mx){
        for(k= stm- 1; k< mx- stp+ 1; k++){
          m= i+ j+ k;
          sbuff_rad[l++]= f[m];
          //                      if(rank== 0) printf("l: %d, m: %d\n", l, m);
        }
      }
    }
  }
}

void unpack_mpi_rad(type *f, double *rbuff_rad, int mx, int my, int mz, int stm, int stp){

  int i, j, k, l, m;

  l= m= 0;
  if(sizeof(type)!= sizeof(double)){
    for(i= mx*my*(stm- 1); i< mx*my*(mz- stp+ 1); i+= mx*my){
      for(j= mx*(stm- 1); j< mx*(my- stp+ 1); j+= mx){
        for(k= stm- 1; k< mx- stp+ 1; k++){
          m= i+ j+ k;
          f[m]= (type)rbuff_rad[l++];
        }
      }
    }
  }
  else{
    for(i= mx*my*(stm- 1); i< mx*my*(mz- stp+ 1); i+= mx*my){
      for(j= mx*(stm- 1); j< mx*(my- stp+ 1); j+= mx){
        for(k= stm- 1; k< mx- stp+ 1; k++){
          m= i+ j+ k;
          f[m]= rbuff_rad[l++];
        }
      }
    }
  }
}
#endif

void communication_rad(variable *val, material *mtl, parameter *prm, int iflg)
{
#ifdef JUPITER_MPI
  domain    *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;
  int  i,    nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
    stm=cdo->stm, stp=cdo->stp, mx=cdo->mx, my=cdo->my, mz=cdo->mz,
    rank= mpi->rank, npe= mpi->npe;
  // send & recv buffer memory allocation
  static int  n1st = 0;
  static double *mpi_buff;
  size_t size = sizeof(double)*((cdo->nx+ 2)*(cdo->ny+ 2)*(cdo->nz+ 2));
  const int tag= 10;

  type *rad= mtl->rad;

  type *dens= mtl->dens;
  type *spcht= mtl->specht;
  type *emi= mtl->emi;
  type *t= val->t;
  type *fls= val->fls;

  MPI_Request req;
  MPI_Status stat;

  if(n1st++ == 0) {
    mpi_buff = (double *) malloc( size );
  }

  //    fprintf(prm->flg->fp, "Start...\n");
  //    fflush(prm->flg->fp);
  printf("Start to send/receive with RADIATION: %d (Global: %d)\n", rank, rank+ npe); // DEBUG
  //printf("Jupiter(heat_source): recv From %d to %d\n", rank+ npe, rank); // DEBUG
  MPI_Recv(mpi_buff, (nx+ 2)*(ny+ 2)*(nz+ 2), MPI_DOUBLE_PRECISION, rank+ npe, tag, MPI_COMM_WORLD, &stat);
  //    fprintf(prm->flg->fp, "\n%d OK: temperature\n", cdo->icnt);
  //    fflush(prm->flg->fp);
  //printf("Jupiter(send): send From %d to %d\n", rank+ npe, rank); // DEBUG
  MPI_Send(&iflg, 1, MPI_INTEGER, rank+ npe, tag+ 10, MPI_COMM_WORLD);
  //    fprintf(prm->flg->fp, "%d OK: iflg\n", cdo->icnt);
  //    fflush(prm->flg->fp);
  unpack_mpi_rad(rad, mpi_buff, mx, my, mz, stm, stp);

  if(iflg== 0){
    //printf("Jupiter(emi): send From %d to %d\n", rank+ npe, rank); // DEBUG
    pack_mpi_rad(emi, mpi_buff, mx, my, mz, stm, stp);
    MPI_Isend(mpi_buff, (nx+ 2)*(ny+ 2)*(nz+ 2), MPI_DOUBLE_PRECISION, rank+ npe, tag, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, &stat);
    //        fprintf(prm->flg->fp, "%d OK: emi\n", cdo->icnt);
    //        fflush(prm->flg->fp);

    //printf("Jupiter(dens): send From %d to %d\n", rank+ npe, rank); // DEBUG
    pack_mpi_rad(dens, mpi_buff, mx, my, mz, stm, stp);
    MPI_Isend(mpi_buff, (nx+ 2)*(ny+ 2)*(nz+ 2), MPI_DOUBLE_PRECISION, rank+ npe, tag+ 10, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, &stat);
    //        fprintf(prm->flg->fp, "%d OK: dens\n", cdo->icnt);
    //        fflush(prm->flg->fp);

    //printf("Jupiter(spcht): send From %d to %d\n", rank+ npe, rank); // DEBUG
    pack_mpi_rad(spcht, mpi_buff, mx, my, mz, stm, stp);
    MPI_Isend(mpi_buff, (nx+ 2)*(ny+ 2)*(nz+ 2), MPI_DOUBLE_PRECISION, rank+ npe, tag+ 20, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, &stat);
    //        fprintf(prm->flg->fp, "%d OK: spcht\n", cdo->icnt);
    //        fflush(prm->flg->fp);

    //printf("Jupiter(t): send From %d to %d\n", rank+ npe, rank); // DEBUG
    pack_mpi_rad(t, mpi_buff, mx, my, mz, stm, stp);
    MPI_Isend(mpi_buff, (nx+ 2)*(ny+ 2)*(nz+ 2), MPI_DOUBLE_PRECISION, rank+ npe, tag+ 30, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, &stat);
    //        fprintf(prm->flg->fp, "%d OK: t\n", cdo->icnt);
    //        fflush(prm->flg->fp);

    //printf("Jupiter(fls): send From %d to %d\n", rank+ npe, rank); // DEBUG
    pack_mpi_rad(fls, mpi_buff, mx, my, mz, stm, stp);
    MPI_Isend(mpi_buff, (nx+ 2)*(ny+ 2)*(nz+ 2), MPI_DOUBLE_PRECISION, rank+ npe, tag+ 40, MPI_COMM_WORLD, &req);
    MPI_Wait(&req, &stat);
    //        fprintf(prm->flg->fp, "%d OK: fls\n\n", cdo->icnt);
    //        fflush(prm->flg->fp);
  }
  printf("Finished to send/receive with RADIATION: %d (Global: %d)\n", rank, rank+ npe); // DEBUG
  if(iflg== 1) printf(" iflg is called \n"); // DEBUG
#endif /* JUPITER_MPI */
}

/* YSE: Condition checking function for all/any processes */
/**
 * @brief Check whether cond is satified for all MPI ranks (in JUPITER).
 *
 * @param mpi  MPI parameter
 * @param cond Condition value
 *
 * @retval non-0 if all processes' \p cond evaluates to true (non 0)
 * @retval 0     if any processes' \p cond evaluates to false (0),
 *               or MPI communication error occured
 *
 * This function is collective call for mpi->CommJUPITER.
 *
 * Use macro for_any_rank() to avoid function call when MPI is not
 * enabled (this function itself is still available even if MPI is not
 * enabled, and when so, just returns \p cond).
 */
int mpi_for_all_rank(mpi_param *mpi, int cond)
{
#ifdef JUPITER_MPI
  MPI_Errhandler h;
  int r;

  MPI_Comm_get_errhandler(mpi->CommJUPITER, &h);
  MPI_Comm_set_errhandler(mpi->CommJUPITER, MPI_ERRORS_RETURN);
  r = MPI_Allreduce(MPI_IN_PLACE, &cond, 1, MPI_INT, MPI_LAND, mpi->CommJUPITER);
  if (r) cond = 0;
  MPI_Comm_set_errhandler(mpi->CommJUPITER, h);

  return cond;
#else
  return cond;
#endif
}

/**
 * @brief Check whether cond is satisfied for any MPI ranks (in JUPITER).
 *
 * @param mpi  MPI parameter
 * @param cond Condition value
 *
 * @retval non-0 if any processes' \p cond evaluates to true (non 0)
 *               or MPI communication error occured
 * @retval 0     if all processes' \p cond evaluates to false (0),
 *
 * This function is collective call for mpi->CommJUPITER.
 *
 * Use macro for_any_rank() to avoid function call when MPI is not
 * enabled (this function itself is still available even if MPI is not
 * enabled, and when so, just returns \p cond).
 */
int mpi_for_any_rank(mpi_param *mpi, int cond)
{
#ifdef JUPITER_MPI
  MPI_Errhandler h;
  int r;

  MPI_Comm_get_errhandler(mpi->CommJUPITER, &h);
  MPI_Comm_set_errhandler(mpi->CommJUPITER, MPI_ERRORS_RETURN);
  r = MPI_Allreduce(MPI_IN_PLACE, &cond, 1, MPI_INT, MPI_LOR, mpi->CommJUPITER);
  if (r) cond = 1;
  MPI_Comm_set_errhandler(mpi->CommJUPITER, h);

  return cond;
#else
  return cond;
#endif
}
