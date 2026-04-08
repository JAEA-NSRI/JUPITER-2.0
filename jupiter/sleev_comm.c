#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "struct.h"
#include "cg.h"

#ifdef JUPITER_MPI

int sleev_comm(type *f,mpi_prm *prm){
  //  int ptr[6];
  int nx=prm->nx,ny=prm->ny,nz=prm->nz;
  int mx=prm->mx;  
  int mxy=prm->mxy;  
  int stm=prm->stm;  
  int nxy,nxz,nyz;
  int jx,jy,jz,i,j,l;
  int nl;

  MPI_Status   stat_send[6],stat_recv[6];

  /*
  for(i=0;i<6;i++){
    ptr[i]=prm->ptr[i];
  }
  */
  nl=1; //袖の長さ
  nxy=nx*ny*nl;
  nxz=nx*nz*nl;
  nyz=ny*nz*nl;

  type sb_b[nxy],rb_b[nxy];
  type sb_t[nxy],rb_t[nxy];
  type sb_s[nxz],rb_s[nxz];
  type sb_n[nxz],rb_n[nxz];
  type sb_w[nyz],rb_w[nyz];
  type sb_e[nyz],rb_e[nyz];

#pragma omp parallel for private(i)
  for(i = 0; i < nxy ; i++) {
    sb_b[i]=0.0;
    rb_b[i]=0.0;
    sb_t[i]=0.0;
    rb_t[i]=0.0;
  }
#pragma omp parallel for private(i)
  for(i = 0; i < nxz ; i++) {
    sb_s[i]=0.0;
    rb_s[i]=0.0;
    sb_n[i]=0.0;
    rb_n[i]=0.0;
  }
#pragma omp parallel for private(i)
  for(i = 0; i < nyz ; i++) {
    sb_w[i]=0.0;
    rb_w[i]=0.0;
    sb_e[i]=0.0;
    rb_e[i]=0.0;
  }

  i=0;
  for(l = 0; l < nl; l++) {
#pragma omp parallel for private(jx,jy,jz,j,i)
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	i=jx+nx*jy+nx*ny*l;
	jz=0+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_b[i]=f[j];

	jz=nz-1+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_t[i]=f[j];
	//	i=i+1;
      }
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
#pragma omp parallel for private(jx,jy,jz,j,i)
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
	i=jx+nx*jz+nx*nz*l;
	jy=0+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_s[i]=f[j];

	jy=ny-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_n[i]=f[j];
	//	i=i+1;
      }
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
#pragma omp parallel for private(jx,jy,jz,j,i)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
	i=jy+ny*jz+ny*nz*l;
	jx=0+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_w[i]=f[j];

	jx=nx-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_e[i]=f[j];
	//	i=i+1;
      }
    }
  }
#if 0
// bottom (z-)
  if(prm->nrk[0] > -1) { 
    MPI_Isend(sb_b,nxy, MPI_TYPE, prm->nrk[0], prm->nrk[0], prm->comm, &prm->req_send[0]);
    MPI_Irecv(rb_b,nxy, MPI_TYPE, prm->nrk[0], prm->rank,   prm->comm, &prm->req_recv[0]);
  }
 // top    (z+)
  if(prm->nrk[1] > -1) {
    MPI_Isend(sb_t,nxy, MPI_TYPE, prm->nrk[1], prm->nrk[1], prm->comm, &prm->req_send[1]);
    MPI_Irecv(rb_t,nxy, MPI_TYPE, prm->nrk[1], prm->rank,   prm->comm, &prm->req_recv[1]);
  }
 // south (y-)
  if(prm->nrk[2] > -1) {
    MPI_Isend(sb_s, nxz, MPI_TYPE, prm->nrk[2], prm->nrk[2], prm->comm, &prm->req_send[2]);
    MPI_Irecv(rb_s, nxz, MPI_TYPE, prm->nrk[2], prm->rank,   prm->comm, &prm->req_recv[2]);
  }
  // north (y+)
  if(prm->nrk[3] > -1) {
    MPI_Isend(sb_n, nxz, MPI_TYPE, prm->nrk[3], prm->nrk[3], prm->comm, &prm->req_send[3]);
    MPI_Irecv(rb_n, nxz, MPI_TYPE, prm->nrk[3], prm->rank,   prm->comm, &prm->req_recv[3]);
  }
  // west (x-)
  if(prm->nrk[4] > -1) {
    MPI_Isend(sb_w, nyz, MPI_TYPE, prm->nrk[4], prm->nrk[4], prm->comm, &prm->req_send[4]);
    MPI_Irecv(rb_w, nyz, MPI_TYPE, prm->nrk[4], prm->rank,   prm->comm, &prm->req_recv[4]);
  }
  // east (x+)
  if(prm->nrk[5] > -1) {
    MPI_Isend(sb_e, nyz, MPI_TYPE, prm->nrk[5], prm->nrk[5], prm->comm, &prm->req_send[5]);
    MPI_Irecv(rb_e, nyz, MPI_TYPE, prm->nrk[5], prm->rank,   prm->comm, &prm->req_recv[5]);
  }
#else
// bottom (z-)
  if(prm->nrk[0] > -1) { 
    MPI_Isend(sb_b,nxy, MPI_TYPE, prm->nrk[0], 1, prm->comm, &prm->req_send[0]);
    MPI_Irecv(rb_b,nxy, MPI_TYPE, prm->nrk[0], 2,   prm->comm, &prm->req_recv[0]);
  }
 // top    (z+)
  if(prm->nrk[1] > -1) {
    MPI_Isend(sb_t,nxy, MPI_TYPE, prm->nrk[1], 2, prm->comm, &prm->req_send[1]);
    MPI_Irecv(rb_t,nxy, MPI_TYPE, prm->nrk[1], 1,   prm->comm, &prm->req_recv[1]);
  }

 // south (y-)
  if(prm->nrk[2] > -1) {
    MPI_Isend(sb_s, nxz, MPI_TYPE, prm->nrk[2], 3, prm->comm, &prm->req_send[2]);
    MPI_Irecv(rb_s, nxz, MPI_TYPE, prm->nrk[2], 4,   prm->comm, &prm->req_recv[2]);
  }
  // north (y+)
  if(prm->nrk[3] > -1) {
    MPI_Isend(sb_n, nxz, MPI_TYPE, prm->nrk[3], 4, prm->comm, &prm->req_send[3]);
    MPI_Irecv(rb_n, nxz, MPI_TYPE, prm->nrk[3], 3,   prm->comm, &prm->req_recv[3]);
  }

  // west (x-)
  if(prm->nrk[4] > -1) {
    MPI_Isend(sb_w, nyz, MPI_TYPE, prm->nrk[4], 5, prm->comm, &prm->req_send[4]);
    MPI_Irecv(rb_w, nyz, MPI_TYPE, prm->nrk[4], 6,   prm->comm, &prm->req_recv[4]);
  }
  // east (x+)
  if(prm->nrk[5] > -1) {
    MPI_Isend(sb_e, nyz, MPI_TYPE, prm->nrk[5], 6, prm->comm, &prm->req_send[5]);
    MPI_Irecv(rb_e, nyz, MPI_TYPE, prm->nrk[5], 5,   prm->comm, &prm->req_recv[5]);
  }
#endif
  for(i = 0; i < 2; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }
  for(i = 2; i < 4; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }
  for(i = 4; i < 6; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
#pragma omp parallel for private(jx,jy,jz,j,i)
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	i=jx+nx*jy+nx*ny*l;
	jz=-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_b[i];

	jz=nz+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_t[i];
	//	i=i+1;
      }
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
#pragma omp parallel for private(jx,jy,jz,j,i)
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
	i=jx+nx*jz+nx*nz*l;
	jy=-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_s[i];

	jy=ny+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_n[i];
	//	i=i+1;
      }
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
#pragma omp parallel for private(jx,jy,jz,j,i)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
	i=jy+ny*jz+ny*nz*l;
	jx=-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_w[i];

	jx=nx+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_e[i];
	//	i=i+1;
      }
    }
  }

  return 0;

}


int sleev_comm_x(type *f,mpi_prm *prm){
  //  int ptr[6];
  int nx=prm->nx,ny=prm->ny,nz=prm->nz;
  int mx=prm->mx;  
  int mxy=prm->mxy;  
  int stm=prm->stm;  
  int nxy,nxz,nyz;
  int jx,jy,jz,i,j,l;
  int nl;

  MPI_Status   stat_send[6],stat_recv[6];

  nl=1; //袖の長さ
  nxy=nx*ny*nl;
  nxz=nx*nz*nl;
  nyz=ny*nz*nl;

  type sb_w[nyz],rb_w[nyz];
  type sb_e[nyz],rb_e[nyz];

#pragma omp parallel for private(i)
  for(i = 0; i < nyz ; i++) {
    sb_w[i]=0.0;
    rb_w[i]=0.0;
    sb_e[i]=0.0;
    rb_e[i]=0.0;
  }

  i=0;
  for(l = 0; l < nl; l++) {
#pragma omp parallel for private(jx,jy,jz,j,i)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
	i=jy+ny*jz+ny*nz*l;
	jx=0+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_w[i]=f[j];

	jx=nx-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_e[i]=f[j];
      }
    }
  }

  // west (x-)
  if(prm->nrk[4] > -1) {
    MPI_Isend(&sb_w, nyz, MPI_TYPE, prm->nrk[4], prm->nrk[4], prm->comm, &prm->req_send[4]);
    MPI_Irecv(&rb_w, nyz, MPI_TYPE, prm->nrk[4], prm->rank,   prm->comm, &prm->req_recv[4]);
  }
  // east (x+)
  if(prm->nrk[5] > -1) {
    MPI_Isend(&sb_e, nyz, MPI_TYPE, prm->nrk[5], prm->nrk[5], prm->comm, &prm->req_send[5]);
    MPI_Irecv(&rb_e, nyz, MPI_TYPE, prm->nrk[5], prm->rank,   prm->comm, &prm->req_recv[5]);
  }
  for(i = 4; i < 6; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
#pragma omp parallel for private(jx,jy,jz,j,i)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
	i=jy+ny*jz+ny*nz*l;
	jx=-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_w[i];

	jx=nx+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_e[i];
      }
    }
  }
  return 0;
}

int sleev_comm_yz(type *f,mpi_prm *prm){
  //  int ptr[6];
  int nx=prm->nx,ny=prm->ny,nz=prm->nz;
  int mx=prm->mx;  
  int mxy=prm->mxy;  
  int stm=prm->stm;  
  int nxy,nxz,nyz;
  int jx,jy,jz,i,j,l;
  int nl;

  MPI_Status   stat_send[6],stat_recv[6];

  nl=1; //袖の長さ
  nxy=nx*ny*nl;
  nxz=nx*nz*nl;
  nyz=ny*nz*nl;

  type sb_b[nxy],rb_b[nxy];
  type sb_t[nxy],rb_t[nxy];
  type sb_s[nxz],rb_s[nxz];
  type sb_n[nxz],rb_n[nxz];

  // #pragma omp parallel for private(i)
  for(i = 0; i < nxy ; i++) {
    sb_b[i]=0.0;
    rb_b[i]=0.0;
    sb_t[i]=0.0;
    rb_t[i]=0.0;
  }
  // #pragma omp parallel for private(i)
  for(i = 0; i < nxz ; i++) {
    sb_s[i]=0.0;
    rb_s[i]=0.0;
    sb_n[i]=0.0;
    rb_n[i]=0.0;
  }

  i=0;
  for(l = 0; l < nl; l++) {
    // #pragma omp parallel for private(jx,jy,jz,j,i)
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	i=jx+nx*jy+nx*ny*l;
	jz=0+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_b[i]=f[j];

	jz=nz-1+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_t[i]=f[j];
	//	i=i+1;
      }
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
    // #pragma omp parallel for private(jx,jy,jz,j,i)
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
	i=jx+nx*jz+nx*nz*l;
	jy=0+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_s[i]=f[j];

	jy=ny-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_n[i]=f[j];
	//	i=i+1;
      }
    }
  }


// bottom (z-)
  if(prm->nrk[0] > -1) { 
    MPI_Isend(&sb_b,nxy, MPI_TYPE, prm->nrk[0], prm->nrk[0], prm->comm, &prm->req_send[0]);
    MPI_Irecv(&rb_b,nxy, MPI_TYPE, prm->nrk[0], prm->rank,   prm->comm, &prm->req_recv[0]);
  }
 // top    (z+)
  if(prm->nrk[1] > -1) {
    MPI_Isend(&sb_t,nxy, MPI_TYPE, prm->nrk[1], prm->nrk[1], prm->comm, &prm->req_send[1]);
    MPI_Irecv(&rb_t,nxy, MPI_TYPE, prm->nrk[1], prm->rank,   prm->comm, &prm->req_recv[1]);
  }
 // south (y-)
  if(prm->nrk[2] > -1) {
    MPI_Isend(&sb_s, nxz, MPI_TYPE, prm->nrk[2], prm->nrk[2], prm->comm, &prm->req_send[2]);
    MPI_Irecv(&rb_s, nxz, MPI_TYPE, prm->nrk[2], prm->rank,   prm->comm, &prm->req_recv[2]);
  }
  // north (y+)
  if(prm->nrk[3] > -1) {
    MPI_Isend(&sb_n, nxz, MPI_TYPE, prm->nrk[3], prm->nrk[3], prm->comm, &prm->req_send[3]);
    MPI_Irecv(&rb_n, nxz, MPI_TYPE, prm->nrk[3], prm->rank,   prm->comm, &prm->req_recv[3]);
  }


  for(i = 0; i < 2; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }
  for(i = 2; i < 4; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
    // #pragma omp parallel for private(jx,jy,jz,j,i)
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	i=jx+nx*jy+nx*ny*l;
	jz=-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_b[i];

	jz=nz+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_t[i];
	//	i=i+1;
      }
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
    // #pragma omp parallel for private(jx,jy,jz,j,i)
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
	i=jx+nx*jz+nx*nz*l;
	jy=-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_s[i];

	jy=ny+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_n[i];
      }
    }
  }
  return 0;
}


int sleev_dcomm(type *f,type *A,
		type *q_b,type *q_t,type *q_s,type *q_n,type *q_w,type *q_e,
		mpi_prm *prm){


  //  int ptr[6];
  int nx=prm->nx,ny=prm->ny,nz=prm->nz;
  int m=prm->m;  
  int mx=prm->mx;  
  int mxy=prm->mxy;  
  int stm=prm->stm;  
  int nxy,nxz,nyz;
  int nl;

  int jx,jy,jz,i,j,l;
  int jj,jj_;

  MPI_Status   stat_send[6],stat_recv[6];

  /*
  for(i=0;i<6;i++){
    ptr[i]=prm->ptr[i];
  }
*/
  nl=1; //袖の長さ
  nxy=nx*ny*nl;
  nxz=nx*nz*nl;
  nyz=ny*nz*nl;

  type sb_b[nxy],rb_b[nxy];
  type sb_t[nxy],rb_t[nxy];
  type sb_s[nxz],rb_s[nxz];
  type sb_n[nxz],rb_n[nxz];
  type sb_w[nyz],rb_w[nyz];
  type sb_e[nyz],rb_e[nyz];

  for(i = 0; i < nxy ; i++) {
    sb_b[i]=0.0;
    rb_b[i]=0.0;
    sb_t[i]=0.0;
    rb_t[i]=0.0;
  }
  for(i = 0; i < nxz ; i++) {
    sb_s[i]=0.0;
    rb_s[i]=0.0;
    sb_n[i]=0.0;
    rb_n[i]=0.0;
  }
  for(i = 0; i < nyz ; i++) {
    sb_w[i]=0.0;
    rb_w[i]=0.0;
    sb_e[i]=0.0;
    rb_e[i]=0.0;
  }

  i=0;
  for(l = 0; l < nl; l++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	jz=0+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_b[i]=f[j];

	jz=nz-1+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_t[i]=f[j];
	i=i+1;
      }
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
	jy=0+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_s[i]=f[j];

	jy=ny-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_n[i]=f[j];
	i=i+1;
      }
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
	jx=0+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_w[i]=f[j];

	jx=nx-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_e[i]=f[j];
	i=i+1;
      }
    }
  }

// bottom (z-)
  if(prm->nrk[0] > -1) { 
    MPI_Isend(&sb_b,nxy, MPI_TYPE, prm->nrk[0], prm->nrk[0], prm->comm, &prm->req_send[0]);
    MPI_Irecv(&rb_b,nxy, MPI_TYPE, prm->nrk[0], prm->rank,   prm->comm, &prm->req_recv[0]);
  }
 // top    (z+)
  if(prm->nrk[1] > -1) {
    MPI_Isend(&sb_t,nxy, MPI_TYPE, prm->nrk[1], prm->nrk[1], prm->comm, &prm->req_send[1]);
    MPI_Irecv(&rb_t,nxy, MPI_TYPE, prm->nrk[1], prm->rank,   prm->comm, &prm->req_recv[1]);
  }
 // south (y-)
  if(prm->nrk[2] > -1) {
    MPI_Isend(&sb_s, nxz, MPI_TYPE, prm->nrk[2], prm->nrk[2], prm->comm, &prm->req_send[2]);
    MPI_Irecv(&rb_s, nxz, MPI_TYPE, prm->nrk[2], prm->rank,   prm->comm, &prm->req_recv[2]);
  }
  // north (y+)
  if(prm->nrk[3] > -1) {
    MPI_Isend(&sb_n, nxz, MPI_TYPE, prm->nrk[3], prm->nrk[3], prm->comm, &prm->req_send[3]);
    MPI_Irecv(&rb_n, nxz, MPI_TYPE, prm->nrk[3], prm->rank,   prm->comm, &prm->req_recv[3]);
  }
  // west (x-)
  if(prm->nrk[4] > -1) {
    MPI_Isend(&sb_w, nyz, MPI_TYPE, prm->nrk[4], prm->nrk[4], prm->comm, &prm->req_send[4]);
    MPI_Irecv(&rb_w, nyz, MPI_TYPE, prm->nrk[4], prm->rank,   prm->comm, &prm->req_recv[4]);
  }
  // east (x+)
  if(prm->nrk[5] > -1) {
    MPI_Isend(&sb_e, nyz, MPI_TYPE, prm->nrk[5], prm->nrk[5], prm->comm, &prm->req_send[5]);
    MPI_Irecv(&rb_e, nyz, MPI_TYPE, prm->nrk[5], prm->rank,   prm->comm, &prm->req_recv[5]);
  }


  for(i = 0; i < 2; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }
  for(i = 2; i < 4; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }
  for(i = 4; i < 6; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        jj_ = jx + nx*jy ;
	jz = 0;
        jj = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	q_b[jj_]=rb_b[i]*A[jj+0*m];
	i=i+1;
      }
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        jj_ = jx + nx*jy ;
	jz  = nz-1;
	jj  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	q_t[jj_]=rb_t[i]*A[jj+6*m];
	i=i+1;
      }
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
        jj_ = jx + nx*jz ;
	jy  = 0;
	jj  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	q_s[jj_]=rb_s[i]*A[jj+1*m];

	i=i+1;
      }
    }
  }
  i=0;
  for(l = 0; l < nl; l++) {
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
        jj_ = jx + nx*jz ;
	jy  = ny-1;
	jj  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	q_n[jj_]=rb_n[i]*A[jj+5*m];

	i=i+1;
      }
    }
  }


  i=0;
  for(l = 0; l < nl; l++) {
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        jj_ = jy + ny*jz ;
	jx  = 0;
	jj  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	q_w[jj_]=rb_w[i]*A[jj+2*m];

	i=i+1;
      }
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        jj_ = jy + ny*jz ;
	jx  = nx-1;
	jj  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	q_e[jj_]=rb_e[i]*A[jj+4*m];

	i=i+1;
      }
    }
  }

  return 0;

}

int sleev_comm_nopack(type *f,
		type *rb_b,type *rb_t,type *rb_s,type *rb_n,type *rb_w,type *rb_e,
		mpi_prm *prm){

  //  int ptr[6];
  int nx=prm->nx,ny=prm->ny,nz=prm->nz;
  //  int m=prm->m;  
  int mx=prm->mx;  
  int mxy=prm->mxy;  
  int stm=prm->stm;  
  int nxy,nxz,nyz;
  int nl;

  int jx,jy,jz,i,j,l;
  //  int jj,jj_;

  MPI_Status   stat_send[6],stat_recv[6];

  /*
  for(i=0;i<6;i++){
    ptr[i]=prm->ptr[i];
  }
  */
  nl=1; //袖の長さ
  nxy=nx*ny*nl;
  nxz=nx*nz*nl;
  nyz=ny*nz*nl;
  /*
  type sb_b[nxy],rb_b[nxy];
  type sb_t[nxy],rb_t[nxy];
  type sb_s[nxz],rb_s[nxz];
  type sb_n[nxz],rb_n[nxz];
  type sb_w[nyz],rb_w[nyz];
  type sb_e[nyz],rb_e[nyz];
  */
  type sb_b[nxy];
  type sb_t[nxy];
  type sb_s[nxz];
  type sb_n[nxz];
  type sb_w[nyz];
  type sb_e[nyz];


  for(i = 0; i < nxy ; i++) {
    sb_b[i]=0.0;
    rb_b[i]=0.0;
    sb_t[i]=0.0;
    rb_t[i]=0.0;
  }
  for(i = 0; i < nxz ; i++) {
    sb_s[i]=0.0;
    rb_s[i]=0.0;
    sb_n[i]=0.0;
    rb_n[i]=0.0;
  }
  for(i = 0; i < nyz ; i++) {
    sb_w[i]=0.0;
    rb_w[i]=0.0;
    sb_e[i]=0.0;
    rb_e[i]=0.0;
  }

  i=0;
  for(l = 0; l < nl; l++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	jz=0+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_b[i]=f[j];

	jz=nz-1+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_t[i]=f[j];
	i=i+1;
      }
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
	jy=0+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_s[i]=f[j];

	jy=ny-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_n[i]=f[j];
	i=i+1;
      }
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
	jx=0+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_w[i]=f[j];

	jx=nx-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_e[i]=f[j];
	i=i+1;
      }
    }
  }

// bottom (z-)
  if(prm->nrk[0] > -1) { 
    MPI_Isend(&sb_b,nxy, MPI_TYPE, prm->nrk[0], prm->nrk[0], prm->comm, &prm->req_send[0]);
    MPI_Irecv(rb_b,nxy, MPI_TYPE, prm->nrk[0], prm->rank,   prm->comm, &prm->req_recv[0]);
  }
 // top    (z+)
  if(prm->nrk[1] > -1) {
    MPI_Isend(&sb_t,nxy, MPI_TYPE, prm->nrk[1], prm->nrk[1], prm->comm, &prm->req_send[1]);
    MPI_Irecv(rb_t,nxy, MPI_TYPE, prm->nrk[1], prm->rank,   prm->comm, &prm->req_recv[1]);
  }
 // south (y-)
  if(prm->nrk[2] > -1) {
    MPI_Isend(&sb_s, nxz, MPI_TYPE, prm->nrk[2], prm->nrk[2], prm->comm, &prm->req_send[2]);
    MPI_Irecv(rb_s, nxz, MPI_TYPE, prm->nrk[2], prm->rank,   prm->comm, &prm->req_recv[2]);
  }
  // north (y+)
  if(prm->nrk[3] > -1) {
    MPI_Isend(&sb_n, nxz, MPI_TYPE, prm->nrk[3], prm->nrk[3], prm->comm, &prm->req_send[3]);
    MPI_Irecv(rb_n, nxz, MPI_TYPE, prm->nrk[3], prm->rank,   prm->comm, &prm->req_recv[3]);
  }
  // west (x-)
  if(prm->nrk[4] > -1) {
    MPI_Isend(&sb_w, nyz, MPI_TYPE, prm->nrk[4], prm->nrk[4], prm->comm, &prm->req_send[4]);
    MPI_Irecv(rb_w, nyz, MPI_TYPE, prm->nrk[4], prm->rank,   prm->comm, &prm->req_recv[4]);
  }
  // east (x+)
  if(prm->nrk[5] > -1) {
    MPI_Isend(&sb_e, nyz, MPI_TYPE, prm->nrk[5], prm->nrk[5], prm->comm, &prm->req_send[5]);
    MPI_Irecv(rb_e, nyz, MPI_TYPE, prm->nrk[5], prm->rank,   prm->comm, &prm->req_recv[5]);
  }


  for(i = 0; i < 2; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }
  for(i = 2; i < 4; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }
  for(i = 4; i < 6; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }

  return 0;
}

//----
// CACG
//----
int sleev_comm_MPK(type *f,mpi_prm *prm){

  //  int ptr[6];
  int nx=prm->nx,ny=prm->ny,nz=prm->nz;
  int mx=prm->mx,my=prm->my,mz=prm->mz;
  int mxy=prm->mxy;  
  int stm=prm->stm;  
  int nxy,nxz,nyz;
  int mxz; // ,myz;
  int jx,jy,jz,i,j,l;
  int nl;
  int smax=prm->smax;  
  //  int order=prm->order;  
  MPI_Status   stat_send[6],stat_recv[6];

  /*
  for(i=0;i<6;i++){
    ptr[i]=prm->ptr[i];
  }
  */
  //
#ifdef Neuman
  //  nl=smax+order*2+1 +8-1; //袖の長さ
  nl=1+order+smax*order ; //袖の長さ
#else
  nl=smax; //袖の長さ
#endif
  //  nl=1; //袖の長さ
  /*   mxy=mx*my*nl; */
  /*   mxz=mx*mz*nl; */
  /*   myz=my*mz*nl; */

  //  nxy=nx*ny*nl;
  //  nxz=nx*nz*nl;
  //  nyz=ny*nz*nl;

  nxy=mx*my*nl;
  nxz=mx*mz*nl;
  nyz=my*mz*nl;

  type sb_b[nxy],rb_b[nxy];
  type sb_t[nxy],rb_t[nxy];
  type sb_s[nxz],rb_s[nxz];
  type sb_n[nxz],rb_n[nxz];
  type sb_w[nyz],rb_w[nyz];
  type sb_e[nyz],rb_e[nyz];

#pragma omp parallel for private(i)
  for(i = 0; i < nxy ; i++) {
    sb_b[i]=0.0;
    rb_b[i]=0.0;
    sb_t[i]=0.0;
    rb_t[i]=0.0;
  }
#pragma omp parallel for private(i)
  for(i = 0; i < nxz ; i++) {
    sb_s[i]=0.0;
    rb_s[i]=0.0;
    sb_n[i]=0.0;
    rb_n[i]=0.0;
  }
#pragma omp parallel for private(i)
  for(i = 0; i < nyz ; i++) {
    sb_w[i]=0.0;
    rb_w[i]=0.0;
    sb_e[i]=0.0;
    rb_e[i]=0.0;
  }

  for(l = 0; l < nl; l++) {
#pragma omp parallel for private(jx,jy,jz,j,i)
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	i=jx+nx*jy+nx*ny*l;
	
	jz=0+l;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_b[i]=f[j];
	
	jz=nz-1-l;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_t[i]=f[j];
      }
    }
  }

  //@@@@@

  // bottom (z-)
  if(prm->nrk[0] > -1) { 
    MPI_Isend(&sb_b,nxy, MPI_TYPE, prm->nrk[0], prm->nrk[0], prm->comm, &prm->req_send[0]);
    MPI_Irecv(&rb_b,nxy, MPI_TYPE, prm->nrk[0], prm->rank,   prm->comm, &prm->req_recv[0]);
  }
  // top    (z+)
  if(prm->nrk[1] > -1) {
    MPI_Isend(&sb_t,nxy, MPI_TYPE, prm->nrk[1], prm->nrk[1], prm->comm, &prm->req_send[1]);
    MPI_Irecv(&rb_t,nxy, MPI_TYPE, prm->nrk[1], prm->rank,   prm->comm, &prm->req_recv[1]);
  }
  for(i = 0; i < 2; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }
  i=0;
  for(l = 0; l < nl; l++) {
#pragma omp parallel for private(jx,jy,jz,j,i)
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	i=jx+nx*jy+nx*ny*l;

	jz=-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_b[i];

	jz=nz+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_t[i];
      }
    }
  }

  //@@@@@

  i=0;
  for(l = 0; l < nl; l++) {
#pragma omp parallel for private(jx,jy,jz,j,i)
    for(jz = 0 - nl; jz < nz + nl; jz++) {
      for(jx = 0; jx < nx; jx++) {
	i=jx+nx*(jz+nl)+nx*(nz+nl*2)*l;

	jy=0+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_s[i]=f[j];

	jy=ny-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_n[i]=f[j];
      }
    }
  }
  //@@@@@
 // south (y-)
  if(prm->nrk[2] > -1) {
    MPI_Isend(&sb_s, nxz, MPI_TYPE, prm->nrk[2], prm->nrk[2], prm->comm, &prm->req_send[2]);
    MPI_Irecv(&rb_s, nxz, MPI_TYPE, prm->nrk[2], prm->rank,   prm->comm, &prm->req_recv[2]);
  }
  // north (y+)
  if(prm->nrk[3] > -1) {
    MPI_Isend(&sb_n, nxz, MPI_TYPE, prm->nrk[3], prm->nrk[3], prm->comm, &prm->req_send[3]);
    MPI_Irecv(&rb_n, nxz, MPI_TYPE, prm->nrk[3], prm->rank,   prm->comm, &prm->req_recv[3]);
  }

  for(i = 2; i < 4; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }
  i=0;
  for(l = 0; l < nl; l++) {
#pragma omp parallel for private(jx,jy,jz,j,i)
    for(jz = 0 - nl; jz < nz + nl; jz++) {
      for(jx = 0; jx < nx; jx++) {
	i=jx+nx*(jz+nl)+nx*(nz+nl*2)*l;

	jy=-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_s[i];

	jy=ny+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_n[i];
	i=i+1;
      }
    }
  }

  //@@@@@

  i=0;
  for(l = 0; l < nl; l++) {
#pragma omp parallel for private(jx,jy,jz,j,i)
    for(jz = 0 - nl; jz < nz + nl; jz++) {
      for(jy = 0 - nl; jy < ny + nl; jy++) {
	i=(jy+nl)+(ny+nl*2)*(jz+nl)+(ny+nl*2)*(nz+nl*2)*l;

	jx=0+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_w[i]=f[j];
	jx=nx-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sb_e[i]=f[j];
      }
    }
  }
  //@@@@@
  // west (x-)
  if(prm->nrk[4] > -1) {
    MPI_Isend(&sb_w, nyz, MPI_TYPE, prm->nrk[4], prm->nrk[4], prm->comm, &prm->req_send[4]);
    MPI_Irecv(&rb_w, nyz, MPI_TYPE, prm->nrk[4], prm->rank,   prm->comm, &prm->req_recv[4]);
  }
  // east (x+)
  if(prm->nrk[5] > -1) {
    MPI_Isend(&sb_e, nyz, MPI_TYPE, prm->nrk[5], prm->nrk[5], prm->comm, &prm->req_send[5]);
    MPI_Irecv(&rb_e, nyz, MPI_TYPE, prm->nrk[5], prm->rank,   prm->comm, &prm->req_recv[5]);
  }
  for(i = 4; i < 6; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }

  i=0;
  for(l = 0; l < nl; l++) {
#pragma omp parallel for private(jx,jy,jz,j,i)
    for(jz = 0 - nl; jz < nz + nl; jz++) {
      for(jy = 0 - nl; jy < ny + nl; jy++) {
	i=(jy+nl)+(ny+nl*2)*(jz+nl)+(ny+nl*2)*(nz+nl*2)*l;
	jx=-1-l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_w[i];

	jx=nx+l;
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	f[j]=rb_e[i];
	i=i+1;
      }
    }
  }

  return 0;

}

#endif


