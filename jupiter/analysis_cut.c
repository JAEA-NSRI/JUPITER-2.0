#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "os/asprintf.h"

#include "struct.h"
#include "func.h"

//====================================
//  Point data (over time)
//------------------------------------
/*
type get_point_data(type *f, char *loc, type xd, type yd, type zd, parameter *prm)
{
  domain    *cdo = prm->cdo;
  int  j, jx, jy, jz,
       mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;
  type *x=cdo->x, *y=cdo->y, *z=cdo->z,
       xw,xe,ys,yn,zb,zt, ddx,ddy,ddz, fb,ft;
  int  s=0;
  type val=0.0;

  // interpolate output value

  for(jz = 0; jz < mz-1; jz++) {
    for(jy = 0; jy < my-1; jy++) {
      for(jx = 0; jx < mx-1; jx++) {
        j = jx + mx*jy + mxy*jz;

        if(strncmp(loc, "center", 6) == 0) {
          xw = 0.5*(x[jx  ] + x[jx+1]);
          xe = 0.5*(x[jx+1] + x[jx+2]);
          ys = 0.5*(y[jy  ] + y[jy+1]);
          yn = 0.5*(y[jy+1] + y[jy+2]);
          zb = 0.5*(z[jz  ] + z[jz+1]);
          zt = 0.5*(z[jz+1] + z[jz+2]);
        } else if(strncmp(loc, "face_x", 6) == 0) {
          xw = x[jx];
          xe = x[jx+1];
          ys = 0.5*(y[jy  ] + y[jy+1]);
          yn = 0.5*(y[jy+1] + y[jy+2]);
          zb = 0.5*(z[jz  ] + z[jz+1]);
          zt = 0.5*(z[jz+1] + z[jz+2]);
        } else if(strncmp(loc, "face_y", 6) == 0) {
          xw = 0.5*(x[jx  ] + x[jx+1]);
          xe = 0.5*(x[jx+1] + x[jx+2]);
          ys = y[jy];
          yn = y[jy+1];
          zb = 0.5*(z[jz  ] + z[jz+1]);
          zt = 0.5*(z[jz+1] + z[jz+2]);
        } else if(strncmp(loc, "face_z", 6) == 0) {
          xw = 0.5*(x[jx  ] + x[jx+1]);
          xe = 0.5*(x[jx+1] + x[jx+2]);
          ys = 0.5*(y[jy  ] + y[jy+1]);
          yn = 0.5*(y[jy+1] + y[jy+2]);
          zb = z[jz];
          zt = z[jz+1];
        } else {
          printf("Error: analysis_cut.c  function: get_point_data(),\n prease set variable location (center, face-x, face-y or face-z).\n");
          exit(-1);
        }

        if( xw <= xd && xd < xe &&
            ys <= yd && yd < yn &&
            zb <= zd && zd < zt ) {
          ddx = (xd - xw)/(xe - xw);
          ddy = (yd - ys)/(yn - ys);
          ddz = (zd - zb)/(zt - zb);

          fb =        ddy *( ddx*f[j+1   ] + (1.0 - ddx)*f[j   ] )
             + (1.0 - ddy)*( ddx*f[j+1+mx] + (1.0 - ddx)*f[j+mx] );
          ft =        ddy *( ddx*f[j+1   +mxy] + (1.0 - ddx)*f[j   +mxy] )
             + (1.0 - ddy)*( ddx*f[j+1+mx+mxy] + (1.0 - ddx)*f[j+mx+mxy] );

          val = ddz*ft + (1.0 - ddz)*fb;
          s = 1; // flag
        }
      }
    }
  }

  // MPIiSendRecv
  type val_max, val_min;
  MPI_Allreduce(&val, &val_max, 1, MPI_TYPE, MPI_MAX, MPI_COMM_WORLD);
  MPI_Allreduce(&val, &val_min, 1, MPI_TYPE, MPI_MIN, MPI_COMM_WORLD);
  if(val_max == 0.0) val = val_min;
  if(val_min == 0.0) val = val_max;

  return val;
}

type kerf_depth(type *z_sol, type *z_liq, type *d_sol, type *d_liq, type *fs, type *fl, parameter *prm)
{
  mpi_param *mpi = prm->mpi;
  domain    *cdo = prm->cdo;
  laser     *lsr = prm->lsr;
  int  j, jx, jy, jz, stm=cdo->stm,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       mx=cdo->mx, my=cdo->my, mxy=mx*my;
  type *x=cdo->x, *y=cdo->y, *z=cdo->z,
       sol_z, liq_z, surf=0.5;

  sol_z=0.0;
  liq_z=0.0;
  for(jy = stm; jy < ny+stm; jy++) {
    for(jx = stm; jx < nx+stm; jx++) {
      if(x[jx] <= lsr->lsr_x && lsr->lsr_x <= x[jx+1] &&
         y[jy] <= lsr->lsr_y && lsr->lsr_y <= y[jy+1] ) {
        //printf("(x[jx], x[jx+1]) = (%9.3e, %9.3e), y[jy] = %9.3e\n", x[jx], x[jx+1], y[jy]);//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        for(jz = stm; jz < nz+stm; jz++) {
          j = jx + mx*jy + mxy*jz;
          // search liquid or solid
          if((fs[j]-surf)*(fs[j+mxy]-surf) < 0.0) sol_z = MAX2(sol_z, z[jz+1]);
          if((fl[j]-surf)*(fl[j+mxy]-surf) < 0.0) liq_z = MAX2(liq_z, z[jz+1]);
        }
#ifdef JUPITER_MPI
        MPI_Allreduce(&sol_z, &sol_z, 1, MPI_TYPE, MPI_MAX, mpi->CommLz);
        MPI_Allreduce(&liq_z, &liq_z, 1, MPI_TYPE, MPI_MAX, mpi->CommLz);
        MPI_Barrier(mpi->CommLz);
#endif
        if(liq_z > sol_z) liq_z = sol_z;
        *z_sol = sol_z;
        *z_liq = liq_z;
        *d_sol = cdo->vof->fs_ze - sol_z + 0.5*cdo->dz;
        *d_liq = cdo->vof->fs_ze - liq_z + 0.5*cdo->dz;
      }

    }
  }
  //printf("z_sol = %9.3e, z_liq = %9.3e, lsr(x,y) = (%9.3e, %9.3e)\n", sol_z, liq_z, lsr->lsr_x, lsr->lsr_y);//!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
  return 0.0;
}

type search_kerf_front(type *x_front, type *x_middle, type *x_back, type *fs, parameter *prm)
{
  domain              *cdo = prm->cdo;
  initial_vof_profile *vof = cdo->vof;
  int  j, jx, jy, jz,
       mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;
  type *x=cdo->x, *y=cdo->y, *z=cdo->z, zc, surf=0.5,
       zfront=vof->fs_ze, zback=vof->fs_zs;

  *x_front = 0.0;
  *x_middle= 0.0;
  *x_back  = 0.0;

  for(jz = 0; jz < mz; jz++) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j = jx + mx*jy + mxy*jz;

        if(y[jy] <= vof->py && vof->py < y[jy+1] && vof->px < x[jx]) {
          zc = 0.5*(z[jz] + z[jz+1]);
          // Front point
          if(zfront - cdo->dz <= zc && zc < zfront) {
            if((fs[j]-surf)*(fs[j+1]-surf) < 0.0) {
              *x_front = 0.5*(x[jx+1] + x[jx+2]);
            }
          }
          // Middle point
          if(0.5*(zfront+zback) - cdo->dz <= zc && zc < 0.5*(zfront+zback) + cdo->dz) {
            if((fs[j]-surf)*(fs[j+1]-surf) < 0.0) {
              *x_middle = 0.5*(x[jx+1] + x[jx+2]);
            }
          }
          // Back point
          if(zback < zc && zc <= zback + cdo->dz) {
            if((fs[j]-surf)*(fs[j+1]-surf) < 0.0) {
              *x_back = 0.5*(x[jx+1] + x[jx+2]);
            }
          }
        }
      }
    }
  }
#ifdef JUPITER_MPI
  MPI_Allreduce(&x_front, &x_front,  1, MPI_TYPE, MPI_MAX, MPI_COMM_WORLD);
  MPI_Allreduce(&x_middle,&x_middle, 1, MPI_TYPE, MPI_MAX, MPI_COMM_WORLD);
  MPI_Allreduce(&x_back,  &x_back,   1, MPI_TYPE, MPI_MAX, MPI_COMM_WORLD);
#endif
  return 0.0;
}

type analysis_point_data(variable *val, material *mtl, parameter *prm)
{
  domain              *cdo = prm->cdo;
  mpi_param           *mpi = prm->mpi;
  initial_vof_profile *vof = cdo->vof;
  char filename[100];
  FILE *fp;

  // Kerf temperature
  type x_front, x_middle, x_back,
       T_front, T_middle, T_back;
  sprintf(filename,"./data/1D_data/pt_Temp.dat");
  search_kerf_front(&x_front, &x_middle, &x_back, val->fs, prm);
  T_front = get_point_data(val->t, "center", x_front, vof->py, vof->fs_ze-cdo->dz, prm);
  T_middle= get_point_data(val->t, "center", x_middle,vof->py, 0.5*(vof->fs_ze+vof->fs_zs), prm);
  T_back  = get_point_data(val->t, "center", x_back,  vof->py, vof->fs_zs+cdo->dz, prm);
  if(mpi->rank == 0) {
    if(cdo->iout == 0) fp = fopen(filename,"w");
    else               fp = fopen(filename,"a");
    if(cdo->iout == 0) fprintf(fp, "# Time[sec], T_front[K], T_middle[K], T_back[K], cut_delay[m], x_front[m], x_middle[m], x_back[m]\n");
    fprintf(fp, "%9.5e, %9.5e, %9.5e, %9.5e, %9.5e, %9.5e, %9.5e, %9.5e\n",
        cdo->time, T_front, T_middle, T_back, x_front-x_back, x_front, x_middle, x_back);
    fclose(fp);
  }

  // Kerf depth
  type z_sol, z_liq, d_sol, d_liq;
  sprintf(filename,"./data/1D_data/pt_Kerf_depth.dat");
  kerf_depth(&z_sol, &z_liq, &d_sol, &d_liq, val->fs, val->fl, prm);
  if(mpi->rank == 0) {
    if(cdo->iout == 0) fp = fopen(filename,"w");
    else               fp = fopen(filename,"a");
    if(cdo->iout == 0) fprintf(fp, "# Time[sec], z_sol[m], z_liq[m], d_sol[m], d_liq[m]\n");
    fprintf(fp, "%9.5e, %9.5e, %9.5e, %9.5e, %9.5e\n", cdo->time, z_sol, z_liq, d_sol, d_liq);
    fclose(fp);
  }


  // Thermo couple location
  int  num=27,  i;
  type xoff=cdo->vof->px + cdo->vof->pr, tt[27];
  //======= TC 1 ============
  type x1A=xoff+0.001, x1B=xoff+0.010, y1=0.003,
       z11=0.050,
       z12=0.045,
       z13=0.035,
       z14=0.025,
       z15=0.010;
  // T/C position 1A
  tt[0] = get_point_data(val->t, "center", x1A, y1, z11, prm);
  tt[1] = get_point_data(val->t, "center", x1A, y1, z12, prm);
  tt[2] = get_point_data(val->t, "center", x1A, y1, z13, prm);
  tt[3] = get_point_data(val->t, "center", x1A, y1, z14, prm);
  tt[4] = get_point_data(val->t, "center", x1A, y1, z15, prm);
  // T/C position 1B
  tt[5] = get_point_data(val->t, "center", x1B, y1, z11, prm);
  tt[6] = get_point_data(val->t, "center", x1B, y1, z12, prm);
  tt[7] = get_point_data(val->t, "center", x1B, y1, z13, prm);
  tt[8] = get_point_data(val->t, "center", x1B, y1, z14, prm);
  tt[9] = get_point_data(val->t, "center", x1B, y1, z15, prm);

  //======= TC 2 ============
  type x2A=xoff+0.0, x2B=xoff+0.005, x2C=xoff+0.010,
       y2 = 0.005,
       z21=0.050,
       z22=0.030,
       z23=0.010;
  // T/C position 2A
  tt[10] = get_point_data(val->t, "center", x2A, y2, z21, prm);
  tt[11] = get_point_data(val->t, "center", x2A, y2, z22, prm);
  tt[12] = get_point_data(val->t, "center", x2A, y2, z23, prm);
  // T/C position 2B
  tt[13] = get_point_data(val->t, "center", x2B, y2, z21, prm);
  tt[14] = get_point_data(val->t, "center", x2B, y2, z22, prm);
  tt[15] = get_point_data(val->t, "center", x2B, y2, z23, prm);
  // T/C position 2C
  tt[16] = get_point_data(val->t, "center", x2C, y2, z21, prm);
  tt[17] = get_point_data(val->t, "center", x2C, y2, z22, prm);
  tt[18] = get_point_data(val->t, "center", x2C, y2, z23, prm);

  //======= TC 3 ============
  type x3A=xoff+0.001, x3B=xoff+0.010, z3 = 0.050,
       y31=0.0015,
       y32=0.0020,
       y33=0.0028,
       y34=0.0040;
  // T/C position 3A
  tt[19] = get_point_data(val->t, "center", x3A, y31, z3, prm);
  tt[20] = get_point_data(val->t, "center", x3A, y32, z3, prm);
  tt[21] = get_point_data(val->t, "center", x3A, y33, z3, prm);
  tt[22] = get_point_data(val->t, "center", x3A, y34, z3, prm);
  // T/C position 3B
  tt[23] = get_point_data(val->t, "center", x3B, y31, z3, prm);
  tt[24] = get_point_data(val->t, "center", x3B, y32, z3, prm);
  tt[25] = get_point_data(val->t, "center", x3B, y33, z3, prm);
  tt[26] = get_point_data(val->t, "center", x3B, y34, z3, prm);

  sprintf(filename,"./data/1D_data/pt_TC_Temp.dat");
  if(mpi->rank == 0) {
    if(cdo->iout == 0) fp = fopen(filename,"w");
    else               fp = fopen(filename,"a");
    if(cdo->iout == 0) fprintf(fp, "# Time[sec], TC_1A1, TC_1A2, TC_1A3, TC_1A4, TC_1A5, TC_1B1, TC_1B2, TC_1B3, TC_1B4, TC_1B5, TC_2A1, TC_2A2, TC_2A3, TC_2B1, TC_2B2, TC_2B3, TC_2C1, TC_2C2, TC_2C3, TC_3A1, TC_3A2, TC_3A3, TC_3A4, TC_3B1, TC_3B2, TC_3B3, TC_3B4 \n");
    fprintf(fp, "%9.5e, ", cdo->time);
    for(i = 0; i < num; i++) fprintf(fp, "%9.5e, ", tt[i]);
    fprintf(fp, "\n");
    fclose(fp);
  }

  // Laser heat input
  sprintf(filename,"./data/1D_data/pt_heat_input.dat");
  if(mpi->rank == 0) {
    if(cdo->iout == 0) fp = fopen(filename,"w");
    else               fp = fopen(filename,"a");
    if(cdo->iout == 0) fprintf(fp, "# Time[sec], inPW[W], percent, radiation[W] \n");
    fprintf(fp, "%9.5e, %9.5e, %9.5e, %9.5e\n",
        cdo->time, prm->lsr->q_sum, 100.0*prm->lsr->q_sum/prm->lsr->pw0, prm->phv->rad_W);
    fclose(fp);
  }

  return 0.0;
}


//====================================
//  Line data (each time, max value => over time)
//------------------------------------

// output: fout,  line data(cell-center)
type get_line_data(type *fout, type *fin, char *loc, char *line, type xd, type yd, type zd, parameter *prm)
{
  domain    *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;
  MPI_Request req_send,  req_recv;
  MPI_Status  stat_send, stat_recv;
  int  j, jx,jy,jz, num=0,
       line_rank_x=-1, line_rank_y=-1, line_rank_z=-1,
       mx =cdo->mx,  my =cdo->my,  mz =cdo->mz,  mxy=mx*my,
       px,py,pz,s_rank;
  if(strncmp(line, "x", 1) == 0) num = mx;
  if(strncmp(line, "y", 1) == 0) num = my;
  if(strncmp(line, "z", 1) == 0) num = mz;
  type  Lx=cdo->Lx, Ly=cdo->Ly, Lz=cdo->Lz, *fl, *val;
  type *x=cdo->x, *y=cdo->y, *z=cdo->z, xr=0.0, yr=0.0, zr=0.0,
       xw,xe,ys,yn,zb,zt, ddx,ddy,ddz, fb,ft;
  size_t lsize = sizeof(type)*(num);

#ifdef _MEM_ALIGN_
  fl = (type *) _mm_malloc(lsize,32);
  val= (type *) _mm_malloc(lsize,32);
#else
  fl = (type *) malloc(lsize);
  val= (type *) malloc(lsize);
#endif

  // search rank_x, rank_y, rank_z
  if(strncmp(line,"y",1)==0 || strncmp(line,"z",1)==0) {
    for(j = 0; j < mpi->npe_x; j++) {
      if(Lx*j <= xd && xd < Lx*(j + 1)) {
        line_rank_x = j;
      }
    }
  }
  if(strncmp(line,"x",1)==0 || strncmp(line,"z",1)==0) {
    for(j = 0; j < mpi->npe_y; j++) {
      if(Ly*j <= yd && yd < Ly*(j + 1)) {
        line_rank_y = j;
      }
    }
  }
  if(strncmp(line,"x",1)==0 || strncmp(line,"y",1)==0) {
    for(j = 0; j < mpi->npe_z; j++) {
      if(Lz*j <= zd && zd < Lz*(j + 1)) {
        line_rank_z = j;
      }
    }
  }

  // interpolate output value & packing data
  int i = 0;

  for(jz = 0; jz < mz-1; jz++) {
    for(jy = 0; jy < my-1; jy++) {
      for(jx = 0; jx < mx-1; jx++) {
        j = jx + mx*jy + mxy*jz;
        if(strncmp(loc, "center", 6) == 0) {
          xw=0.5*(x[jx]+x[jx+1]);  xe=0.5*(x[jx+1]+x[jx+2]);
          ys=0.5*(y[jy]+y[jy+1]);  yn=0.5*(y[jy+1]+y[jy+2]);
          zb=0.5*(z[jz]+z[jz+1]);  zt=0.5*(z[jz+1]+z[jz+2]);
        } else if(strncmp(loc, "face_x", 6) == 0) {
          xw=x[jx];                xe=x[jx+1];
          ys=0.5*(y[jy]+y[jy+1]);  yn=0.5*(y[jy+1]+y[jy+2]);
          zb=0.5*(z[jz]+z[jz+1]);  zt=0.5*(z[jz+1]+z[jz+2]);
        } else if(strncmp(loc, "face_y", 6) == 0) {
          xw=0.5*(x[jx]+x[jx+1]);  xe=0.5*(x[jx+1]+x[jx+2]);
          ys=y[jy];                yn=y[jy+1];
          zb=0.5*(z[jz]+z[jz+1]);  zt=0.5*(z[jz+1]+z[jz+2]);
        } else if(strncmp(loc, "face_z", 6) == 0) {
          xw=0.5*(x[jx]+x[jx+1]);  xe=0.5*(x[jx+1]+x[jx+2]);
          ys=0.5*(y[jy]+y[jy+1]);  yn=0.5*(y[jy+1]+y[jy+2]);
          zb=z[jz];                zt=z[jz+1];
        } else {
          printf("Error: analysis_cut.c  function: get_line_data(),\n prease set variable location (center, face-x, face-y or face-z).\n");
          exit(-1);
        }
        if(strncmp(line, "x", 1) == 0) xr = 0.5*(x[jx] + x[jx+1]);
        if(strncmp(line, "y", 1) == 0) yr = 0.5*(y[jy] + y[jy+1]);
        if(strncmp(line, "z", 1) == 0) zr = 0.5*(z[jz] + z[jz+1]);
        if( xw <= xr && xr < xe &&
            ys <= yr && yr < yn &&
            zb <= zr && zr < zt ) {
          ddx = (xr - xw)/(xe - xw);
          ddy = (yr - ys)/(yn - ys);
          ddz = (zr - zb)/(zt - zb);
          fb =        ddy *( ddx*fin[j+1   ] + (1.0 - ddx)*fin[j   ] )
             + (1.0 - ddy)*( ddx*fin[j+1+mx] + (1.0 - ddx)*fin[j+mx] );
          ft =        ddy *( ddx*fin[j+1   +mxy] + (1.0 - ddx)*fin[j   +mxy] )
             + (1.0 - ddy)*( ddx*fin[j+1+mx+mxy] + (1.0 - ddx)*fin[j+mx+mxy] );
          val[i] = ddz*ft + (1.0 - ddz)*fb;
          i++;
        }
      }
    }
  }
  // send data to rank0
  // x-line
  if(strncmp(line,"x",1) == 0) {
    for(px = 0; px < mpi->npe_x; px++) {
      s_rank = px + mpi->npe_x*line_rank_y + mpi->npe_xy*line_rank_z;
      if(s_rank != 0) {
        if(mpi->rank == s_rank) MPI_Isend(val, mx, MPI_TYPE, 0,      0, MPI_COMM_WORLD, &req_send);
        if(mpi->rank == 0     ) MPI_Irecv(fl,  mx, MPI_TYPE, s_rank, 0, MPI_COMM_WORLD, &req_recv);
        if(mpi->rank == s_rank) MPI_Wait(&req_send, &stat_send);
        if(mpi->rank == 0     ) MPI_Wait(&req_recv, &stat_recv);
      }
      if(mpi->rank == 0) {
        for(j = 0; j < mx; j++) {
          if(s_rank == 0) fout[j+mx*px] = val[j];
          else            fout[j+mx*px] =  fl[j];
        }
      }
    }
  }
  // y-line
  if(strncmp(line,"y",1) == 0) {
    for(py = 0; py < mpi->npe_y; py++) {
      s_rank = line_rank_x + mpi->npe_x*py + mpi->npe_xy*line_rank_z;
      if(s_rank != 0) {
        if(mpi->rank == s_rank) MPI_Isend(val, my, MPI_TYPE, 0,      0, MPI_COMM_WORLD, &req_send);
        if(mpi->rank == 0     ) MPI_Irecv(fl,  my, MPI_TYPE, s_rank, 0, MPI_COMM_WORLD, &req_recv);
        if(mpi->rank == s_rank) MPI_Wait(&req_send, &stat_send);
        if(mpi->rank == 0     ) MPI_Wait(&req_recv, &stat_recv);
      }
      if(mpi->rank == 0) {
        for(j = 0; j < my; j++) {
          if(s_rank == 0) fout[j+my*py] = val[j];
          else            fout[j+my*py] =  fl[j];
        }
      }
    }
  }
  // z-line
  if(strncmp(line,"z",1) == 0) {
    for(pz = 0; pz < mpi->npe_z; pz++) {
      s_rank = line_rank_x + mpi->npe_x*line_rank_y + mpi->npe_xy*pz;
      if(s_rank != 0) {
        if(mpi->rank == s_rank) MPI_Isend(val, mz, MPI_TYPE, 0,      0, MPI_COMM_WORLD, &req_send);
        if(mpi->rank == 0     ) MPI_Irecv(fl,  mz, MPI_TYPE, s_rank, 0, MPI_COMM_WORLD, &req_recv);
        if(mpi->rank == s_rank) MPI_Wait(&req_send, &stat_send);
        if(mpi->rank == 0     ) MPI_Wait(&req_recv, &stat_recv);
      }
      if(mpi->rank == 0) {
        for(j = 0; j < mz; j++) {
          if(s_rank == 0) fout[j+mz*pz] = val[j];
          else            fout[j+mz*pz] =  fl[j];
        }
      }
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);
  return 0.0;
}

type analysis_line_data(variable *val, material *mtl, parameter *prm)
{
  domain *cdo = prm->cdo;
  char filename[100];
  FILE *fp;
  int    jz, gmz=cdo->gmz;
  size_t zsize = sizeof(type)*(gmz);
  type   *f1, *f2, *gz=cdo->gz;

#ifdef _MEM_ALIGN_
  f1 = (type *) _mm_malloc(zsize,32);
  f2 = (type *) _mm_malloc(zsize,32);
#else
  f1 = (type *) malloc(zsize);
  f2 = (type *) malloc(zsize);
#endif

  type xoff=cdo->vof->px + cdo->vof->pr;
  // T/C position
  type x1 = xoff+0.001,  x2 = xoff+0.010,
       y1 = 0.003,  y2 = 0.003,  z1=0.0, z2=0.0;
  get_line_data(f1, val->t, "center", "z", x1, y1, z1, prm);
  get_line_data(f2, val->t, "center", "z", x2, y2, z2, prm);
  if(prm->mpi->rank == 0) {
    sprintf(filename,"./data/1D_data/line_TC_Temp_%04d.dat", cdo->iout);
    fp = fopen(filename,"w");
    for(jz = 0; jz < gmz; jz++) {
      fprintf(fp,"%f %f %f\n", 0.5*(gz[jz] + gz[jz+1]), f1[jz], f2[jz]);
    }
    fclose(fp);
  }

  // Front surface

  // Middle

  // Back surface

  // Kerf depth

  // Laser axis

  // Maximum value

#ifdef _MEM_ALIGN_
  _mm_free(f1);
  _mm_free(f2);
#else
  free(f1);
  free(f2);
#endif
  return 0.0;
}



//====================================
//  Surface data (over time)
//------------------------------------

type get_surf_data(type *fout, type *fin, type zd, parameter *prm)
{
  domain    *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;
  MPI_Request req_send,  req_recv;
  MPI_Status  stat_send, stat_recv;
  int  j, jx,jy,jz,j3,surf_rank_z=-1, jz_surf=0, stm=cdo->stm, jf,
       nx =cdo->nx,  ny =cdo->ny,
       mx =cdo->mx,  my =cdo->my,  mz =cdo->mz,  mxy=mx*my,
       gmx=cdo->gmx,
       px,py,s_rank;
  type Lz=cdo->Lz, *z=cdo->z, zc, ddz, *fl, *val;
  size_t lsize = sizeof(type)*( mx* my);

#ifdef _MEM_ALIGN_
  fl = (type *) _mm_malloc(lsize,32);
  val= (type *) _mm_malloc(lsize,32);
#else
  fl = (type *) malloc(lsize);
  val= (type *) malloc(lsize);
#endif

  // search rank_z
  for(j = 0; j < mpi->npe_z; j++) {
    if(Lz*j <= zd && zd < Lz*(j + 1)) {
      surf_rank_z = j;
    }
  }
  // rank = surf_rank_z
  if(mpi->rank_z == surf_rank_z) {
    // get index jz_surf
    for(jz = 0; jz < mz; jz++) {
      if(z[jz] <= zd && zd < z[jz+1]) {
        jz_surf = jz;
      }
    }
    // packing of the surf data (cell-center only!)
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        j3 = jx + mx*jy + mxy*jz_surf;
        j  = jx + mx*jy;
        zc = 0.5*(z[jz_surf] + z[jz_surf+1]);
        if(zc <= zd) {
          ddz = (zd - zc)/(z[jz_surf+1] - z[jz_surf]);
          val[j] = ddz*fin[j3+mxy] + (1.0 - ddz)*fin[j3];
        } else {
          ddz = (zc - zd)/(z[jz_surf] - z[jz_surf-1]);
          val[j] = ddz*fin[j3-mxy] + (1.0 - ddz)*fin[j3];
        }
      }
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);
  // send data to rank0
  for(py = 0; py < mpi->npe_y; py++) {
    for(px = 0; px < mpi->npe_x; px++) {
      s_rank = px + mpi->npe_x*py + mpi->npe_xy*surf_rank_z;
      if(s_rank != 0) {
        if(mpi->rank == s_rank) MPI_Isend(val, mx*my, MPI_TYPE, 0,      0, MPI_COMM_WORLD, &req_send);
        if(mpi->rank == 0     ) MPI_Irecv(fl,  mx*my, MPI_TYPE, s_rank, 0, MPI_COMM_WORLD, &req_recv);
        if(mpi->rank == s_rank) MPI_Wait(&req_send, &stat_send);
        if(mpi->rank == 0     ) MPI_Wait(&req_recv, &stat_recv);
      }
      if(mpi->rank == 0) {
        int jxs=stm, jxe=nx+stm, jys=stm, jye=ny+stm;
        if(mpi->rank_x==0           ) jxs = 0;
        if(mpi->rank_x==mpi->npe_x-1) jxe = mx;
        if(mpi->rank_y==0           ) jys = 0;
        if(mpi->rank_y==mpi->npe_y-1) jye = my;
        for(jy = jys; jy < jye; jy++) {
          for(jx = jxs; jx < jxe; jx++) {
            j = jx + mx*jy;// local index
            jf = (jx + px*nx) + gmx*(jy + py*ny);// global index
            if(s_rank == 0 && mpi->rank_z == surf_rank_z) fout[jf] = val[j];
            else                                          fout[jf] = fl[j];
          }
        }
      }
    }
  }
  MPI_Barrier(MPI_COMM_WORLD);
#ifdef _MEM_ALIGN_
  _mm_free(val);
  _mm_free(fl);
#else
  free(val);
  free(fl);
#endif
  return 0.0;
}



// global
type laser_input_profile(type *l_in, type *l_front, type *l_back, parameter *prm)
{
  domain *cdo = prm->cdo;
  int  j, jx, jy;
  for(jy = 0; jy < cdo->gmy; jy++) {
    for(jx = 0; jx < cdo->gmx; jx++) {
      j = jx + cdo->gmx*jy;
      l_in[j] = l_front[j] - l_back[j];
    }
  }
  return 0.0;
}

type analysis_surface_data(variable *val, material *mtl, parameter *prm)
{
  domain *cdo=prm->cdo;
  type *l_in, *l_front, *l_back;
#ifdef _MEM_ALIGN_
  l_in    = (type *) _mm_malloc( sizeof(type)*cdo->gmx*cdo->gmy,32 );
  l_front = (type *) _mm_malloc( sizeof(type)*cdo->gmx*cdo->gmy,32 );
  l_back  = (type *) _mm?malloc( sizeof(type)*cdo->gmx*cdo->gmy,32 );
#else
  l_in    = (type *) malloc( sizeof(type)*cdo->gmx*cdo->gmy );
  l_front = (type *) malloc( sizeof(type)*cdo->gmx*cdo->gmy );
  l_back  = (type *) malloc( sizeof(type)*cdo->gmx*cdo->gmy );
#endif

  get_surf_data(l_front, mtl->laser, 0.95*cdo->gLz, prm);
  get_surf_data(l_back,  mtl->laser, 0.05*cdo->gLz, prm);
  laser_input_profile(l_in, l_front, l_back, prm);
  // laser
  int  j, jx, jy;
  type *gx=cdo->gx, *gy=cdo->gy;
  char filename[100];
  FILE *fp;
  if(prm->mpi->rank == 0) {
    sprintf(filename,"./data/2D_data/laser%04d.dat", cdo->iout);
    fp = fopen(filename,"w");
    for(jy = 0; jy < cdo->gmy; jy++) {
      for(jx = 0; jx < cdo->gmx; jx++) {
        j = jx + cdo->gmx*jy;
        fprintf(fp,"%f %f %f\n", 0.5*(gx[jx] + gx[jx+1]), 0.5*(gy[jy] + gy[jy+1]), l_in[j]);
      }
      fprintf(fp,"\n");
    }
    fclose(fp);
  }

  // Front surface

  // Middle

  // Back surface

  // Laser axis

  // Maximum value

#ifdef _MEM_ALIGN_
  _mm_free(l_in);
  _mm_free(l_front);
  _mm_free(l_back);
#else
  free(l_in);
  free(l_front);
  free(l_back);
#endif
  return 0.0;
}
*/

type analysis_mass_flowrate(variable *val, material *mtl, parameter *prm)
{
  domain              *cdo = prm->cdo;
  mpi_param           *mpi = prm->mpi;
  initial_vof_profile *vof = cdo->vof;
  char filename[100];
  FILE *fp;
  int  icompo, j, jx,jy,jz,m=cdo->m, mx =cdo->mx, my =cdo->my, mz =cdo->mz, mxy=mx*my,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz;
  type dx=cdo->dx, dy=cdo->dy, dz=cdo->dz, dxdydz=dx*dy*dz;
  type xc,yc,zc;
  type *dens=mtl->dens, *fl=val->fl, *fs=val->fs;
  type *Y=val->Y, *Y0=val->Y0;
  type *mass;
  type *Ymass;
  size_t size;
  int noc;
  /* YSE: Best align size, investigation of recent x86_64 CPUs. */
  enum { align_size = 48 }; /* Bytes */
  /* YSE: note: If C11 is available, you do not need to use magic number here.
     #include <stddef.h>
     enum { align_size = sizeof(max_align_t) }
  */
  //type mass_g, mass, mass_fr, mass_fr_g;
  type *alloc;

  /* YSE: Avoid leaking memory (Always allocate and frees) */
  size = sizeof(type)*cdo->NumberOfComponent;
  size = (size / align_size + 1) * align_size;
  alloc = (type *) malloc (size * 2);
  if (!alloc) return 0.0;
  j = 0;
  if (prm->flg->fluid_dynamics == ON) {
    mass = (type *)((char *)alloc + j * size);
    j++;
  } else {
    mass = NULL;
  }
  if (prm->flg->solute_diff == ON) {
    Ymass = (type *)((char *)alloc + j * size);
    j++;
  } else {
    Ymass = NULL;
  }

  /* YSE: Original mass-flow-rate analysis */
  if (prm->flg->solute_diff == ON) {
    noc = 1;
  } else {
    noc = cdo->NumberOfComponent;
  }
  if (prm->flg->fluid_dynamics == ON) {
    for (icompo = 0; icompo < noc; ++icompo) {
      mass[icompo] = 0.0;
    }
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        for(jx = 0; jx < nx; jx++) {
          j = (jx+cdo->stm) + mx*(jy+cdo->stm) + mxy*(jz+cdo->stm);
          xc = 0.5*(cdo->x[jx] + cdo->x[jx+1]);
          yc = 0.5*(cdo->y[jy] + cdo->y[jy+1]);
          zc = 0.5*(cdo->z[jz] + cdo->z[jz+1]);
          for(icompo=0; icompo < noc; ++icompo) {
            mass[icompo] += dens[j] * dxdydz *
              (clip(fl[j+cdo->m*icompo]) + clip(fs[j+cdo->m*icompo])); //[kg]
          }
        }
      }
    }
#ifdef JUPITER_MPI
    MPI_Allreduce(MPI_IN_PLACE, mass, noc, MPI_TYPE, MPI_SUM, prm->mpi->CommJUPITER);
#endif
    cdo->mass_before = cdo->mass_after;
    cdo->mass_after = 0.0;
    for (icompo = 0; icompo < noc; ++icompo) {
      cdo->mass_after += mass[icompo];
    }
    cdo->flow_rate = (cdo->mass_after - cdo->mass_before)/cdo->dtout;
  } else {
    cdo->mass_before = 0.0;
    cdo->mass_after = 0.0;
    cdo->flow_rate = 0.0;
  }

  if (prm->flg->solute_diff == ON) {
    for(j=0;j<cdo->NumberOfComponent;j++){
      Ymass[j] = 0.0;
    }
    for(icompo=0;icompo<cdo->NumberOfComponent;icompo++){
      for(jz = 0; jz < nz; jz++) {
        for(jy = 0; jy < ny; jy++) {
          for(jx = 0; jx < nx; jx++) {
            j = (jx+cdo->stm) + mx*(jy+cdo->stm) + mxy*(jz+cdo->stm);
            Ymass[icompo] += Y[j+icompo*m];
          }
        }
      }
    }
#ifdef JUPITER_MPI
    MPI_Allreduce(MPI_IN_PLACE, Ymass, cdo->NumberOfComponent, MPI_TYPE, MPI_SUM, prm->mpi->CommJUPITER);
#endif

    for (j = 0; j < cdo->NumberOfComponent; ++j) {
      if (Y0[j] != 0.0) {
        Ymass[j] = Ymass[j] / Y0[j];
      } else {
        Ymass[j] = 0.0;
      }
    }
  }

  if (mpi->rank == 0) {
    sprintf(filename,"t-m-fr.dat");
    if(cdo->iout == 0) fp = fopen(filename,"w");
    else               fp = fopen(filename,"a");
    if(cdo->iout == 0) {
      fprintf(fp, "# %12s, %14s, %14s", "Time", "Total Mass", "Mass Flow");
      if (mass) {
        for (icompo = 0; icompo < noc; ++icompo) {
          char *b; int r;
          r = jupiter_asprintf(&b, "Mass %d", icompo);
          if (r >= 0) {
            fprintf(fp, ", %14s", b);
            free(b);
          } else {
            fprintf(fp, ", Mass %8d", icompo);
          }
        }
      }
      if (Ymass) {
        for (icompo = 0; icompo < cdo->NumberOfComponent; ++icompo) {
          char *b; int r;
          r = jupiter_asprintf(&b, "Yconserv %d", icompo);
          if (r >= 0) {
            fprintf(fp, ", %14s", b);
            free(b);
          } else {
            fprintf(fp, ", Ycnsv %7d", icompo);
          }
        }
      }
      fprintf(fp, "\n");
      fprintf(fp, "# %12s, %14s, %14s", "[s]", "[kg]", "[kg/s]");
      if (mass) {
        for (icompo = 0; icompo < noc; ++icompo) {
          fprintf(fp, ", %14s", "[kg]");
        }
      }
      if (Ymass) {
        for (icompo = 0; icompo < cdo->NumberOfComponent; ++icompo) {
          fprintf(fp, ", %14s", "[-]");
        }
      }
      fprintf(fp, "\n");
    }
    fprintf(fp, "%14.6e, %14.6e, %14.6e",
            cdo->time, cdo->mass_after, cdo->flow_rate);
    if (mass) {
      for (icompo = 0; icompo < noc; ++icompo) {
        fprintf(fp, ", %14.6e", mass[icompo]);
      }
    }
    if (Ymass) {
      for (icompo = 0; icompo < cdo->NumberOfComponent; ++icompo) {
        fprintf(fp, ", %14.6e", Ymass[icompo]);
      }
    }
    fprintf(fp, "\n");

    fclose(fp);
  }


  free(alloc);
  return 0.0;
}