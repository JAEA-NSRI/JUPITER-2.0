#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "struct.h"
#include "cg.h"

#define BLOCK_1DLOOOP 1

#ifdef JUPITER_MPI

int sleev_comm_block3D(type *f,mpi_prm *prm,
		       int *block_nxs, int *block_nxe, 
		       int *block_nys, int *block_nye, 
		       int *block_nzs, int *block_nze	    
		       ){

  //  int ptr[6];
  /* int nx=prm->nx,ny=prm->ny,nz=prm->nz; */
  /* int mx=prm->mx;   */
  /* int mxy=prm->mxy;   */
  /* int stm=prm->stm;   */
  int rank;
  MPI_Comm_rank(prm->comm, &rank);

  int block_m = prm->block_m;
  int nzblock    = prm->nzblock;
  int nyblock    = prm->nyblock;
  int nxblock    = prm->nxblock;

  int nzdivblock = prm->nzdivblock;
  int nydivblock = prm->nydivblock;
  int nxdivblock = prm->nxdivblock;
  int ndivblock  = nxdivblock * nydivblock * nzdivblock; 

  int mzdivblock = prm->mzdivblock;
  int mydivblock = prm->mydivblock;
  int mxdivblock = prm->mxdivblock;

  int jjxb,jjyb,jjzb;
  int jxb,jyb,jzb;
  int jb;

  int block_nz = nzblock * nzdivblock;
  int block_ny = nyblock * nydivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxy = block_nx * block_ny;
  int block_nxz = block_nx * block_nz;
  int block_nyz = block_ny * block_nz;

  type sb_b[block_nxy],rb_b[block_nxy];
  type sb_t[block_nxy],rb_t[block_nxy];
  type sb_s[block_nxz],rb_s[block_nxz];
  type sb_n[block_nxz],rb_n[block_nxz];
  type sb_w[block_nyz],rb_w[block_nyz];
  type sb_e[block_nyz],rb_e[block_nyz];

  MPI_Status   stat_send[6],stat_recv[6];
  int i;

  for(jb=0;jb<block_nxy;jb++){
    sb_b[jb] = 0.0;
    rb_b[jb] = 0.0;
    sb_t[jb] = 0.0;
    rb_t[jb] = 0.0;
  }
  for(jb=0;jb<block_nxz;jb++){
    sb_s[jb] = 0.0;
    rb_s[jb] = 0.0;
    sb_n[jb] = 0.0;
    rb_n[jb] = 0.0;
  }
  for(jb=0;jb<block_nyz;jb++){
    sb_w[jb] = 0.0;
    rb_w[jb] = 0.0;
    sb_e[jb] = 0.0;
    rb_e[jb] = 0.0;
  }

  //  printf(" -z bound -- %04d --> %04d\n",block_nzs[0]           ,(0)*nzblock + block_nzs[0]);
  //  printf(" +z bound -- %04d --> %04d\n",block_nze[nzdivblock-1], (nzdivblock-1)*nzblock + block_nze[nzdivblock-1]);

  // bottom (z-)
  jzb  = 0;
  jjzb = block_nzs[jzb];
  for(jjyb=0;jjyb<nyblock;jjyb++){
    for(jjxb=0;jjxb<nxblock;jjxb++){
      for(jyb=0;jyb<nydivblock;jyb++){ 
	for(jxb=0;jxb<nxdivblock;jxb++){ 
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  int j   = jxb + jyb*nxdivblock + jjxb * (nxdivblock*nydivblock) + jjyb * (nxdivblock*nydivblock*nxblock);
	  sb_b[j] = f[jj];
	}
      }
    }
  }

  // top    (z+)
  jzb  = nzdivblock-1;
  jjzb = block_nze[jzb];
  for(jjyb=0;jjyb<nyblock;jjyb++){
    for(jjxb=0;jjxb<nxblock;jjxb++){
      for(jyb=0;jyb<nydivblock;jyb++){ 
	for(jxb=0;jxb<nxdivblock;jxb++){ 
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  int j   = jxb + jyb*nxdivblock + jjxb * (nxdivblock*nydivblock) + jjyb * (nxdivblock*nydivblock*nxblock);
	  sb_t[j] = f[jj];
	}
      }
    }
  }

  // south (y-)
  jyb  = 0;
  jjyb = block_nys[jyb];
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjxb=0;jjxb<nxblock;jjxb++){
      for(jzb=0;jzb<nzdivblock;jzb++){ 
	for(jxb=0;jxb<nxdivblock;jxb++){ 
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  int j   = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	  sb_s[j] = f[jj];
	}
      }
    }
  }

  // north (y+)
  jyb  = nydivblock-1;
  jjyb = block_nye[jyb];
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjxb=0;jjxb<nxblock;jjxb++){
      for(jzb=0;jzb<nzdivblock;jzb++){ 
	for(jxb=0;jxb<nxdivblock;jxb++){ 
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  int j   = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	  sb_n[j] = f[jj];
	}
      }
    }
  }

  // west (x-)
  jxb  = 0;
  jjxb = block_nxs[jxb];
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jzb=0;jzb<nzdivblock;jzb++){ 
	for(jyb=0;jyb<nydivblock;jyb++){ 
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  int j   = jyb + jzb*nydivblock + jjyb * (nydivblock*nzdivblock) + jjzb * (nydivblock*nzdivblock*nyblock);
	  sb_w[j] = f[jj];
	}
      }
    }
  }

  // east (x+)
  jxb  = nxdivblock-1;
  jjxb = block_nxe[jxb];
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jzb=0;jzb<nzdivblock;jzb++){ 
	for(jyb=0;jyb<nydivblock;jyb++){ 
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  int j   = jyb + jzb*nydivblock + jjyb * (nydivblock*nzdivblock) + jjzb * (nydivblock*nzdivblock*nyblock);
	  sb_e[j] = f[jj];
	}
      }
    }
  }



  // ----------------

  // bottom (z-)
  if(prm->nrk[0] > -1) { 
    MPI_Isend(sb_b, block_nxy, MPI_TYPE, prm->nrk[0], 1, prm->comm, &prm->req_send[0]);
    MPI_Irecv(rb_b, block_nxy, MPI_TYPE, prm->nrk[0], 2, prm->comm, &prm->req_recv[0]);
  }
 // top    (z+)
  if(prm->nrk[1] > -1) {
    MPI_Isend(sb_t, block_nxy, MPI_TYPE, prm->nrk[1], 2, prm->comm, &prm->req_send[1]);
    MPI_Irecv(rb_t, block_nxy, MPI_TYPE, prm->nrk[1], 1, prm->comm, &prm->req_recv[1]);
  }

 // south (y-)
  if(prm->nrk[2] > -1) {
    MPI_Isend(sb_s, block_nxz, MPI_TYPE, prm->nrk[2], 3, prm->comm, &prm->req_send[2]);
    MPI_Irecv(rb_s, block_nxz, MPI_TYPE, prm->nrk[2], 4, prm->comm, &prm->req_recv[2]);
  }
  // north (y+)
  if(prm->nrk[3] > -1) {
    MPI_Isend(sb_n, block_nxz, MPI_TYPE, prm->nrk[3], 4, prm->comm, &prm->req_send[3]);
    MPI_Irecv(rb_n, block_nxz, MPI_TYPE, prm->nrk[3], 3, prm->comm, &prm->req_recv[3]);
  }

  // west (x-)
  if(prm->nrk[4] > -1) {
    MPI_Isend(sb_w, block_nyz, MPI_TYPE, prm->nrk[4], 5, prm->comm, &prm->req_send[4]);
    MPI_Irecv(rb_w, block_nyz, MPI_TYPE, prm->nrk[4], 6, prm->comm, &prm->req_recv[4]);
  }
  // east (x+)
  if(prm->nrk[5] > -1) {
    MPI_Isend(sb_e, block_nyz, MPI_TYPE, prm->nrk[5], 6, prm->comm, &prm->req_send[5]);
    MPI_Irecv(rb_e, block_nyz, MPI_TYPE, prm->nrk[5], 5, prm->comm, &prm->req_recv[5]);
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

  // bottom (z-)
  jzb  = -1;
  jjzb = nzblock-1;
  for(jjyb=0;jjyb<nyblock;jjyb++){
    for(jjxb=0;jjxb<nxblock;jjxb++){
      for(jyb=0;jyb<nydivblock;jyb++){ 
	for(jxb=0;jxb<nxdivblock;jxb++){ 
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  int j   = jxb + jyb*nxdivblock + jjxb * (nxdivblock*nydivblock) + jjyb * (nxdivblock*nydivblock*nxblock);
	  f[jj] = rb_b[j];
	}
      }
    }
  }

  // top    (z+)
  if(block_nze[nzdivblock-1]==(nzblock-1)){   
    jzb  = nzdivblock;
    jjzb = 0;
  }else{
    jzb  = nzdivblock-1;
    jjzb = block_nze[nzdivblock-1]+1;
  }
  for(jjyb=0;jjyb<nyblock;jjyb++){
    for(jjxb=0;jjxb<nxblock;jjxb++){
      for(jyb=0;jyb<nydivblock;jyb++){ 
	for(jxb=0;jxb<nxdivblock;jxb++){ 
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  int j   = jxb + jyb*nxdivblock + jjxb * (nxdivblock*nydivblock) + jjyb * (nxdivblock*nydivblock*nxblock);
	  f[jj] = rb_t[j];
	}
      }
    }
  }


  // south (y-)
  jyb  = -1;
  jjyb = nyblock-1;
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjxb=0;jjxb<nxblock;jjxb++){
      for(jzb=0;jzb<nzdivblock;jzb++){ 
	for(jxb=0;jxb<nxdivblock;jxb++){ 
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  int j   = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	  f[jj] = rb_s[j];
	}
      }
    }
  }

  // north (y+)
  if(block_nye[nydivblock-1]==(nyblock-1)){   
    jyb  = nydivblock;
    jjyb = 0;
  }else{
    jyb  = nydivblock-1;
    jjyb = block_nye[nydivblock-1]+1;
  }
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjxb=0;jjxb<nxblock;jjxb++){
      for(jzb=0;jzb<nzdivblock;jzb++){ 
	for(jxb=0;jxb<nxdivblock;jxb++){ 
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  int j   = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	  f[jj] = rb_n[j];
	}
      }
    }
  }

  // west (x-)
  jxb  = -1;
  jjxb = nxblock-1;
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jzb=0;jzb<nzdivblock;jzb++){ 
	for(jyb=0;jyb<nydivblock;jyb++){ 
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  int j   = jyb + jzb*nydivblock + jjyb * (nydivblock*nzdivblock) + jjzb * (nydivblock*nzdivblock*nyblock);
	  f[jj] = rb_w[j];
	}
      }
    }
  }

  // east (x+)
  if(block_nxe[nxdivblock-1]==(nxblock-1)){   
    jxb  = nxdivblock;
    jjxb = 0;
  }else{
    jxb  = nxdivblock-1;
    jjxb = block_nxe[nxdivblock-1]+1;
  }
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jzb=0;jzb<nzdivblock;jzb++){ 
	for(jyb=0;jyb<nydivblock;jyb++){ 
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  int j   = jyb + jzb*nydivblock + jjyb * (nydivblock*nzdivblock) + jjzb * (nydivblock*nzdivblock*nyblock);
	  f[jj] = rb_e[j];
	}
      }
    }
  }

#if 0
  if(rank==0){
    for(i=0;i<block_nxz;i++){
      printf("cpu i = %04d : %f,%f \n",i,rb_n[i],rb_e[i]);
    }
  }
#endif

  return 0;

#if 0
 // south (y-)
  jjyb = 0;
  jyb = block_nys[jjyb];
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjxb=0;jjxb<nxblock;jjxb++){
      for(jzb=0;jzb<nzdivblock;jzb++){ 
	for(jxb=0;jxb<nxdivblock;jxb++){ 
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  int j   = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	  sb_s[j] = f[jj];
	}
      }
    }
  }

  // north (y+)
  jjyb = nydivblock-1;
  jyb  = block_nye[jjyb];
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjxb=0;jjxb<nxblock;jjxb++){
      for(jzb=0;jzb<nzdivblock;jzb++){ 
	for(jxb=0;jxb<nxdivblock;jxb++){ 
	  int jj0 = jjxb + jjzb*nxblock + jjzb*(nxblock*nzblock);
	  int j0  = 1 + mxdivblock + mzdivblock * mxdivblock + jj0 * (mxdivblock * mzdivblock * mzdivblock);	
	  int jj  = j0 + jxb + jzb * mxdivblock + jzb * mzdivblock * mxdivblock;
	  int j   = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	  sb_n[j] = f[jj];
	}
      } 
    }
  }


  // west (x-)
  jjxb = 0;
  jxb = block_nxs[jjxb];
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jzb=0;jzb<nzdivblock;jzb++){ 
	for(jyb=0;jyb<nydivblock;jyb++){ 
	  //      int jy = jjyb + jyb * nyblock;
	  //      printf(" jy = %04d \n",jy);
	  int jj0 = jjyb + jjzb*nyblock + jjzb*(nyblock*nzblock);
	  int j0  = 1 + mydivblock + mzdivblock * mydivblock + jj0 * (mydivblock * mzdivblock * mzdivblock);	
	  int jj  = j0 + jyb + jzb * mydivblock + jzb * mzdivblock * mydivblock;
	  int j   = jyb + jzb*nydivblock + jjyb * (nydivblock*nzdivblock) + jjzb * (nydivblock*nzdivblock*nyblock);
	  sb_w[j] = f[jj];
	}
      }
    }
  }

  // east (x+)
  jjxb = nxdivblock-1;
  jxb  = block_nxe[jjxb];
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jzb=0;jzb<nzdivblock;jzb++){ 
	for(jyb=0;jyb<nydivblock;jyb++){ 
	  //      int jy = jjyb + jyb * nyblock;
	  //      printf(" jy = %04d \n",jy);
	  int jj0 = jjyb + jjzb*nyblock + jjzb*(nyblock*nzblock);
	  int j0  = 1 + mydivblock + mzdivblock * mydivblock + jj0 * (mydivblock * mzdivblock * mzdivblock);	
	  int jj  = j0 + jyb + jzb * mydivblock + jzb * mzdivblock * mydivblock;
	  int j   = jyb + jzb*nydivblock + jjyb * (nydivblock*nzdivblock) + jjzb * (nydivblock*nzdivblock*nyblock);
	  sb_e[j] = f[jj];
	}
      }
    }
  }

  // MPI communication
  return 0;

  // set 

 // south (y-)
  jjyb = 0;
  jyb = block_nys[jjyb];
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjxb=0;jjxb<nxblock;jjxb++){
      for(jzb=0;jzb<nzdivblock;jzb++){ 
	for(jxb=0;jxb<nxdivblock;jxb++){ 
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  int j   = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	  sb_s[j] = f[jj];
	}
      }
    }
  }

  // north (y+)
  jjyb = nydivblock-1;
  jyb  = block_nye[jjyb];
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjxb=0;jjxb<nxblock;jjxb++){
      for(jzb=0;jzb<nzdivblock;jzb++){ 
	for(jxb=0;jxb<nxdivblock;jxb++){ 
	  int jj0 = jjxb + jjzb*nxblock + jjzb*(nxblock*nzblock);
	  int j0  = 1 + mxdivblock + mzdivblock * mxdivblock + jj0 * (mxdivblock * mzdivblock * mzdivblock);	
	  int jj  = j0 + jxb + jzb * mxdivblock + jzb * mzdivblock * mxdivblock;
	  int j   = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	  sb_n[j] = f[jj];
	}
      } 
    }
  }


  // west (x-)
  jjxb = 0;
  jxb = block_nxs[jjxb];
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jzb=0;jzb<nzdivblock;jzb++){ 
	for(jyb=0;jyb<nydivblock;jyb++){ 
	  //      int jy = jjyb + jyb * nyblock;
	  //      printf(" jy = %04d \n",jy);
	  int jj0 = jjyb + jjzb*nyblock + jjzb*(nyblock*nzblock);
	  int j0  = 1 + mydivblock + mzdivblock * mydivblock + jj0 * (mydivblock * mzdivblock * mzdivblock);	
	  int jj  = j0 + jyb + jzb * mydivblock + jzb * mzdivblock * mydivblock;
	  int j   = jyb + jzb*nydivblock + jjyb * (nydivblock*nzdivblock) + jjzb * (nydivblock*nzdivblock*nyblock);
	  sb_w[j] = f[jj];
	}
      }
    }
  }

  // east (x+)
  jjxb = nxdivblock-1;
  jxb  = block_nxe[jjxb];
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jzb=0;jzb<nzdivblock;jzb++){ 
	for(jyb=0;jyb<nydivblock;jyb++){ 
	  //      int jy = jjyb + jyb * nyblock;
	  //      printf(" jy = %04d \n",jy);
	  int jj0 = jjyb + jjzb*nyblock + jjzb*(nyblock*nzblock);
	  int j0  = 1 + mydivblock + mzdivblock * mydivblock + jj0 * (mydivblock * mzdivblock * mzdivblock);	
	  int jj  = j0 + jyb + jzb * mydivblock + jzb * mzdivblock * mydivblock;
	  int j   = jyb + jzb*nydivblock + jjyb * (nydivblock*nzdivblock) + jjzb * (nydivblock*nzdivblock*nyblock);
	  sb_e[j] = f[jj];
	}
      }
    }
  }

#endif

  return 0;

}

#endif
