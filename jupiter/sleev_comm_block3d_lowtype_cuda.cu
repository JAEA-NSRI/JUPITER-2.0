#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <cuda_runtime.h>
//#include <cuda_bf16.h>
//#include <cuda_fp16.h>
#include "cublas_v2.h"

#include "struct.h"
#include "cg.h"
#include "pcg_block3d_lowtype_cuda.h"
#include "sleev_comm_block3d_lowtype_cuda.h"
#include "sleev_comm_block3d_cuda.h"
// #include "sleev_comm_block3d_cuda.h"
// #include "sleev_comm_block3d.h"

//#define JUPITER_CUDA_AWARE_MPI 1

__global__ void setEast_block3D_lowtype_cuda_kernel(
					       low_type *sb_e ,low_type *f,
					       int *block_nxs, int *block_nxe,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nzdivblock*nydivblock;
  if(tid >= domain) return;
  int jxb ;
  int jjxb;

  jxb  = nxdivblock-1;
  jjxb = block_nxe[nxdivblock-1];
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jzb = (jb%(nydivblock*nzdivblock))/nydivblock;
    int jyb = (jb%(nydivblock*nzdivblock))%nydivblock;
    for(int jjzb=0;jjzb<nzblock;jjzb++){
      for(int jjyb=0;jjyb<nyblock;jjyb++){
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jyb + jzb*nydivblock + jjyb * (nydivblock*nzdivblock) + jjzb * (nydivblock*nzdivblock*nyblock);
	sb_e[j] = f[jj];
      }
    }    
  }
  
  return ;
}

__global__ void setWest_block3D_lowtype_cuda_kernel(
					       low_type *sb_w ,low_type *f,
					       int *block_nxs, int *block_nxe,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nzdivblock*nydivblock;
  if(tid >= domain) return;
  int jxb ;
  int jjxb;

  jxb  = 0;
  jjxb = block_nxs[0];
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jzb = (jb%(nydivblock*nzdivblock))/nydivblock;
    int jyb = (jb%(nydivblock*nzdivblock))%nydivblock ;    
    for(int jjzb=0;jjzb<nzblock;jjzb++){
      for(int jjyb=0;jjyb<nyblock;jjyb++){
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jyb + jzb*nydivblock + jjyb * (nydivblock*nzdivblock) + jjzb * (nydivblock*nzdivblock*nyblock);
	sb_w[j] = f[jj];
      }
    }
  }

  return ;
}

__global__ void setNorth_block3D_lowtype_cuda_kernel(
					       low_type *sb_n ,low_type *f,
					       int *block_nys, int *block_nye,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nzdivblock*nxdivblock;
  if(tid >= domain) return;

  int jyb;
  int jjyb;

  jyb  = nydivblock-1;
  jjyb = block_nye[nydivblock-1];
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jzb = (jb%(nxdivblock*nzdivblock))/nxdivblock;
    int jxb = (jb%(nxdivblock*nzdivblock))%nxdivblock ;
    for(int jjzb=0;jjzb<nzblock;jjzb++){
      for(int jjxb=0;jjxb<nxblock;jjxb++){
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	sb_n[j] = f[jj];
      }
    }
  }

  return ;
}

__global__ void setSouth_block3D_lowtype_cuda_kernel(
					       low_type *sb_s, low_type *f,
					       int *block_nys, int *block_nye,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nzdivblock*nxdivblock;
  if(tid >= domain) return;

  int jyb;
  int jjyb;

  jyb  = 0;
  jjyb = block_nys[0];
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jzb = (jb%(nxdivblock*nzdivblock))/nxdivblock;
    int jxb = (jb%(nxdivblock*nzdivblock))%nxdivblock ;    
    for(int jjzb=0;jjzb<nzblock;jjzb++){
      for(int jjxb=0;jjxb<nxblock;jjxb++){
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	sb_s[j] = f[jj];
      }
    }
  }

  return ;
}

__global__ void setTop_block3D_lowtype_cuda_kernel(
					       low_type *sb_t ,low_type *f,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nxdivblock * nydivblock;

  if(tid >= domain) return;

  int jzb,jjzb;
  jzb  = nzdivblock-1;
  jjzb = block_nze[nzdivblock-1];
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jyb  = (jb%(nxdivblock*nydivblock))/nxdivblock;
    int jxb  = (jb%(nxdivblock*nydivblock))%nxdivblock ;
    for(int jjyb=0;jjyb<nyblock;jjyb++){
      for(int jjxb=0;jjxb<nxblock;jjxb++){
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jxb + jyb*nxdivblock + jjxb * (nxdivblock*nydivblock) + jjyb * (nxdivblock*nydivblock*nxblock);
	sb_t[j] = f[jj];
      }
    }
  }

  return ;
}

 __global__ void setBottom_block3D_lowtype_cuda_kernel(
					       low_type *sb_b, low_type *f,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nxdivblock * nydivblock;

  if(tid >= domain) return;

  int jzb,jjzb;
  jzb  = 0;
  jjzb = block_nzs[0];
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jyb  = (jb%(nxdivblock*nydivblock))/nxdivblock;
    int jxb  = (jb%(nxdivblock*nydivblock))%nxdivblock ;
    for(int jjyb=0;jjyb<nyblock;jjyb++){
      for(int jjxb=0;jjxb<nxblock;jjxb++){
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jxb + jyb*nxdivblock + jjxb * (nxdivblock*nydivblock) + jjyb * (nxdivblock*nydivblock*nxblock);
	sb_b[j] = f[jj];
      }
    }
  }

  return ;
}

__global__ void readBackWest_block3D_lowtype_cuda_kernel(
					       low_type *rb_w, low_type *f,
					       int *block_nxs, int *block_nxe,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nzdivblock*nydivblock;
  if(tid >= domain) return;

  int jxb ;
  int jjxb;

  jxb  = -1;
  jjxb = nxblock-1;
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jzb = (jb%(nydivblock*nzdivblock))/nydivblock;
    int jyb = (jb%(nydivblock*nzdivblock))%nydivblock ;    
    for(int jjzb=0;jjzb<nzblock;jjzb++){
      for(int jjyb=0;jjyb<nyblock;jjyb++){
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jyb + jzb*nydivblock + jjyb * (nydivblock*nzdivblock) + jjzb * (nydivblock*nzdivblock*nyblock);
	f[jj] = rb_w[j];
      }
    }
  }
  
  return ;
}

 
__global__ void readBackEast_block3D_lowtype_cuda_kernel(
					       low_type *rb_e ,low_type *f,
					       int *block_nxs, int *block_nxe,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nzdivblock*nydivblock;
  if(tid >= domain) return;

  int jxb ;
  int jjxb;

  if(block_nxe[nxdivblock-1]==(nxblock-1)){   
    jxb  = nxdivblock;
    jjxb = 0;
  }else{
    jxb  = nxdivblock-1;
    jjxb = block_nxe[nxdivblock-1]+1;
  }
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jzb = (jb%(nydivblock*nzdivblock))/nydivblock;
    int jyb = (jb%(nydivblock*nzdivblock))%nydivblock;
    for(int jjzb=0;jjzb<nzblock;jjzb++){
      for(int jjyb=0;jjyb<nyblock;jjyb++){
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jyb + jzb*nydivblock + jjyb * (nydivblock*nzdivblock) + jjzb * (nydivblock*nzdivblock*nyblock);
	f[jj] = rb_e[j];
      }
    }    
  }
  
  return ;
}

__global__ void readBackNorth_block3D_lowtype_cuda_kernel(
					       low_type *rb_n ,low_type *f,
					       int *block_nys, int *block_nye,
 					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nzdivblock*nxdivblock;
  if(tid >= domain) return;
  int jyb ;
  int jjyb;

  if(block_nye[nydivblock-1]==(nyblock-1)){   
    jyb  = nydivblock;
    jjyb = 0;
  }else{
    jyb  = nydivblock-1;
    jjyb = block_nye[nydivblock-1]+1;
  }
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jzb = (jb%(nxdivblock*nzdivblock))/nxdivblock;
    int jxb = (jb%(nxdivblock*nzdivblock))%nxdivblock ;    
    for(int jjzb=0;jjzb<nzblock;jjzb++){
      for(int jjxb=0;jjxb<nxblock;jjxb++){
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	f[jj] = rb_n[j];
      }
    }
  }

  return ;
}

 __global__ void readBackSouth_block3D_lowtype_cuda_kernel(
					       low_type *rb_s, low_type *f,
					       int *block_nys, int *block_nye,
 					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nzdivblock*nxdivblock;
  if(tid >= domain) return;
  int jyb ;
  int jjyb;

  jyb  = -1;
  jjyb = nyblock-1;
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jzb = (jb%(nxdivblock*nzdivblock))/nxdivblock;
    int jxb = (jb%(nxdivblock*nzdivblock))%nxdivblock ;    
    for(int jjzb=0;jjzb<nzblock;jjzb++){
      for(int jjxb=0;jjxb<nxblock;jjxb++){
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	f[jj] = rb_s[j];
      }
    }
  }

  return ;
}


 
__global__ void readBackTop_block3D_lowtype_cuda_kernel(
					       low_type *rb_t, low_type *f,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nxdivblock * nydivblock;
  if(tid >= domain) return;
  int jzb ;
  int jjzb;

  if(block_nze[nzdivblock-1]==(nzblock-1)){   
    jzb  = nzdivblock;
    jjzb = 0;
  }else{
    jzb  = nzdivblock-1;
    jjzb = block_nze[nzdivblock-1]+1;
  }
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jyb  = (jb%(nxdivblock*nydivblock))/nxdivblock;
    int jxb  = (jb%(nxdivblock*nydivblock))%nxdivblock ;
    for(int jjyb=0;jjyb<nyblock;jjyb++){
      for(int jjxb=0;jjxb<nxblock;jjxb++){
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jxb + jyb*nxdivblock + jjxb * (nxdivblock*nydivblock) + jjyb * (nxdivblock*nydivblock*nxblock);
	f[jj] = rb_t[j];
      }
    }
  }

  return ;
}

__global__ void readBackBottom_block3D_lowtype_cuda_kernel(
					       low_type *rb_b, low_type *f,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nxdivblock * nydivblock;
  if(tid >= domain) return;
  int jzb ;
  int jjzb;

  jzb  = -1;
  jjzb = nzblock-1;
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jyb  = (jb%(nxdivblock*nydivblock))/nxdivblock;
    int jxb  = (jb%(nxdivblock*nydivblock))%nxdivblock ;
    for(int jjyb=0;jjyb<nyblock;jjyb++){
      for(int jjxb=0;jjxb<nxblock;jjxb++){
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jxb + jyb*nxdivblock + jjxb * (nxdivblock*nydivblock) + jjyb * (nxdivblock*nydivblock*nxblock);
	f[jj] = rb_b[j];
      }
    }
  }
  return ;
}
 
int sleev_comm_block3D_icommEast_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_e,  low_type *rb_e
				  ){
 
  int nzblock    = prm->nzblock;
  int nyblock    = prm->nyblock;

  int nzdivblock = prm->nzdivblock;
  int nydivblock = prm->nydivblock;

  int block_nz  = nzblock * nzdivblock;
  int block_ny  = nyblock * nydivblock;
  int block_nyz = block_ny * block_nz;

  // east (x+)
  if(prm->nrk[5] > -1) {
    MPI_Isend(sb_e, block_nyz, MPI_LOW_TYPE, prm->nrk[5], 6, prm->comm, &prm->req_send[5]);
    MPI_Irecv(rb_e, block_nyz, MPI_LOW_TYPE, prm->nrk[5], 5, prm->comm, &prm->req_recv[5]);
  }

  return 0;
}
  
int sleev_comm_block3D_icommWest_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_w,  low_type *rb_w
				  ){

  int nzblock    = prm->nzblock;
  int nyblock    = prm->nyblock;

  int nzdivblock = prm->nzdivblock;
  int nydivblock = prm->nydivblock;

  int block_nz  = nzblock * nzdivblock;
  int block_ny  = nyblock * nydivblock;
  int block_nyz = block_ny * block_nz;

  // west (x-)
  if(prm->nrk[4] > -1) {
    MPI_Isend(sb_w, block_nyz, MPI_LOW_TYPE, prm->nrk[4], 5, prm->comm, &prm->req_send[4]);
    MPI_Irecv(rb_w, block_nyz, MPI_LOW_TYPE, prm->nrk[4], 6, prm->comm, &prm->req_recv[4]);
  }

  return 0;
}

int sleev_comm_block3D_icommNorth_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_n,  low_type *rb_n
				  ){

  int nzblock    = prm->nzblock;
  int nxblock    = prm->nxblock;

  int nzdivblock = prm->nzdivblock;
  int nxdivblock = prm->nxdivblock;

  int block_nz = nzblock * nzdivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxz = block_nx * block_nz;

  // north (y+)
  if(prm->nrk[3] > -1) {
    MPI_Isend(sb_n, block_nxz, MPI_LOW_TYPE, prm->nrk[3], 4, prm->comm, &prm->req_send[3]);
    MPI_Irecv(rb_n, block_nxz, MPI_LOW_TYPE, prm->nrk[3], 3, prm->comm, &prm->req_recv[3]);
  }

  return 0;
}

int sleev_comm_block3D_icommSouth_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_s,  low_type *rb_s
				  ){

  int block_m = prm->block_m;

  int nzblock    = prm->nzblock;
  int nxblock    = prm->nxblock;

  int nzdivblock = prm->nzdivblock;
  int nxdivblock = prm->nxdivblock;

  int block_nz = nzblock * nzdivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxz = block_nx * block_nz;

  // south (y-)
  if(prm->nrk[2] > -1) {
    MPI_Isend(sb_s, block_nxz, MPI_LOW_TYPE, prm->nrk[2], 3, prm->comm, &prm->req_send[2]);
    MPI_Irecv(rb_s, block_nxz, MPI_LOW_TYPE, prm->nrk[2], 4, prm->comm, &prm->req_recv[2]);
  }

  return 0;
}

int sleev_comm_block3D_icommTop_lowtype_cuda(
				  mpi_prm *prm,				  
				  low_type *sb_t,  low_type *rb_t
				  ){

  int block_m = prm->block_m;

  int nyblock    = prm->nyblock;
  int nxblock    = prm->nxblock;

  int nydivblock = prm->nydivblock;
  int nxdivblock = prm->nxdivblock;

  int block_ny = nyblock * nydivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxy = block_nx * block_ny;

  // top    (z+)
  if(prm->nrk[1] > -1) {
    MPI_Isend(sb_t, block_nxy, MPI_LOW_TYPE, prm->nrk[1], 2, prm->comm, &prm->req_send[1]);
    MPI_Irecv(rb_t, block_nxy, MPI_LOW_TYPE, prm->nrk[1], 1, prm->comm, &prm->req_recv[1]);
  }

  return 0;
}
 
int sleev_comm_block3D_icommBottom_lowtype_cuda(
				  mpi_prm *prm,				  
				  low_type *sb_b,  low_type *rb_b
				  ){

  int i;
  MPI_Status stat_send[6],stat_recv[6];

  int block_m = prm->block_m;

  int nyblock    = prm->nyblock;
  int nxblock    = prm->nxblock;

  int nydivblock = prm->nydivblock;
  int nxdivblock = prm->nxdivblock;

  int block_ny = nyblock * nydivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxy = block_nx * block_ny;

  // bottom (z-)
  if(prm->nrk[0] > -1) { 
    MPI_Isend(sb_b, block_nxy, MPI_LOW_TYPE, prm->nrk[0], 1, prm->comm, &prm->req_send[0]);
    MPI_Irecv(rb_b, block_nxy, MPI_LOW_TYPE, prm->nrk[0], 2, prm->comm, &prm->req_recv[0]);
  }

  return 0;
} 

int sleev_comm_hostmem_block3D_lowtype_cuda(
			    low_type *f,mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    low_type *cuda_buf,
			    low_type *sb_b,  low_type *rb_b,  low_type *sb_t,  low_type *rb_t,
			    low_type *sb_s,  low_type *rb_s,  low_type *sb_n,  low_type *rb_n,
			    low_type *sb_w,  low_type *rb_w,  low_type *sb_e,  low_type *rb_e,
			    cudaStream_t *cuStream
		       ){

  int i;
  MPI_Status stat_send[6],stat_recv[6];

  int block_m = prm->block_m;

  int nzblock    = prm->nzblock;
  int nyblock    = prm->nyblock;
  int nxblock    = prm->nxblock;

  int nzdivblock = prm->nzdivblock;
  int nydivblock = prm->nydivblock;
  int nxdivblock = prm->nxdivblock;

  int mzdivblock = prm->mzdivblock;
  int mydivblock = prm->mydivblock;
  int mxdivblock = prm->mxdivblock;

  int block_nz = nzblock * nzdivblock;
  int block_ny = nyblock * nydivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxy = block_nx * block_ny;
  int block_nxz = block_nx * block_nz;
  int block_nyz = block_ny * block_nz;

  low_type *cu_sb_b = &cuda_buf[0];
  low_type *cu_rb_b = &cuda_buf[block_nxy*1];
  low_type *cu_sb_t = &cuda_buf[block_nxy*2];
  low_type *cu_rb_t = &cuda_buf[block_nxy*3];

  low_type *cu_sb_s = &cuda_buf[block_nxy*4];
  low_type *cu_rb_s = &cuda_buf[block_nxy*4 + block_nxz*1];
  low_type *cu_sb_n = &cuda_buf[block_nxy*4 + block_nxz*2];
  low_type *cu_rb_n = &cuda_buf[block_nxy*4 + block_nxz*3];

  low_type *cu_sb_w = &cuda_buf[block_nxy*4 + block_nxz*4];
  low_type *cu_rb_w = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  low_type *cu_sb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  low_type *cu_rb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];
  
  cudaDeviceSynchronize();

  const int threads = 32;  
  const int blocks  = 1024;  

  // bottom (z-)
  // top    (z+)
  setBottom_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cuStream[0]>>>(
						     cu_sb_b ,f,
						     block_nzs, block_nze,
						     nxblock, nyblock, nzblock,
						     mxdivblock, mydivblock, mzdivblock,
						     nxdivblock, nydivblock, nzdivblock);
  setTop_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cuStream[1]>>>(
						     cu_sb_t ,f,
						     block_nzs, block_nze,
						     nxblock, nyblock, nzblock,
						     mxdivblock, mydivblock, mzdivblock,
						     nxdivblock, nydivblock, nzdivblock);
#if JUPITER_CUDA_AWARE_MPI
#else  
  cudaMemcpyAsync(sb_b, cu_sb_b, sizeof(low_type)*block_nxy, cudaMemcpyDeviceToHost,cuStream[0]);
  cudaMemcpyAsync(sb_t, cu_sb_t, sizeof(low_type)*block_nxy, cudaMemcpyDeviceToHost,cuStream[1]);
#endif  

  // south (y-)
  // north (y+)
  setSouth_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cuStream[2]>>>(
						      cu_sb_s, f,
						      block_nys, block_nye,
						      nxblock, nyblock, nzblock,
						      mxdivblock, mydivblock, mzdivblock,
						      nxdivblock, nydivblock, nzdivblock);
  setNorth_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cuStream[3]>>>(
						      cu_sb_n ,f,
						      block_nys, block_nye,
						      nxblock, nyblock, nzblock,
						      mxdivblock, mydivblock, mzdivblock,
						      nxdivblock, nydivblock, nzdivblock);
#if JUPITER_CUDA_AWARE_MPI
#else  
  cudaMemcpyAsync(sb_s, cu_sb_s, sizeof(low_type)*block_nxz, cudaMemcpyDeviceToHost,cuStream[2]);
  cudaMemcpyAsync(sb_n, cu_sb_n, sizeof(low_type)*block_nxz, cudaMemcpyDeviceToHost,cuStream[3]);
#endif  
  // west (x-)
  // east (x+)
  setWest_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cuStream[4]>>>(
						    cu_sb_w ,f,
						    block_nxs, block_nxe,
						    nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);
  setEast_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cuStream[5]>>>(
						    cu_sb_e ,f,
						    block_nxs, block_nxe,
						    nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);
#if JUPITER_CUDA_AWARE_MPI
#else  
  cudaMemcpyAsync(sb_w, cu_sb_w, sizeof(low_type)*block_nyz, cudaMemcpyDeviceToHost,cuStream[4]);
  cudaMemcpyAsync(sb_e, cu_sb_e, sizeof(low_type)*block_nyz, cudaMemcpyDeviceToHost,cuStream[5]);
#endif
  // ----------------------------------------------------------

  // top    (z+)
  // bottom (z-)
#if JUPITER_CUDA_AWARE_MPI
  cudaStreamSynchronize(cuStream[0]);
  sleev_comm_block3D_icommBottom_lowtype_cuda(prm,cu_sb_b,cu_rb_b);
  cudaStreamSynchronize(cuStream[1]);
  sleev_comm_block3D_icommTop_lowtype_cuda(prm,cu_sb_t,cu_rb_t);
#else
  cudaStreamSynchronize(cuStream[0]);
  sleev_comm_block3D_icommBottom_lowtype_cuda(prm,sb_b,rb_b);
  cudaStreamSynchronize(cuStream[1]);
  sleev_comm_block3D_icommTop_lowtype_cuda(prm,sb_t,rb_t);
#endif

  sleev_comm_block3D_waitBottom_cuda(prm);
#if JUPITER_CUDA_AWARE_MPI
#else  
  cudaMemcpyAsync(cu_rb_b, rb_b, sizeof(low_type)*block_nxy, cudaMemcpyHostToDevice,cuStream[0]);
#endif
  readBackBottom_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cuStream[0]>>>(
							  cu_rb_b, f,
							  block_nzs, block_nze,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);
  
  sleev_comm_block3D_waitTop_cuda(prm);  
#if JUPITER_CUDA_AWARE_MPI
#else  
  cudaMemcpyAsync(cu_rb_t, rb_t, sizeof(low_type)*block_nxy, cudaMemcpyHostToDevice,cuStream[1]);
#endif
  readBackTop_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cuStream[1]>>>(
							  cu_rb_t, f,
							  block_nzs, block_nze,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);
  // south (y-)
  // north (y+)
#if JUPITER_CUDA_AWARE_MPI
  cudaStreamSynchronize(cuStream[2]);
  sleev_comm_block3D_icommSouth_lowtype_cuda(prm,cu_sb_s,cu_rb_s);
  cudaStreamSynchronize(cuStream[3]);
  sleev_comm_block3D_icommNorth_lowtype_cuda(prm,cu_sb_n,cu_rb_n);
#else
  cudaStreamSynchronize(cuStream[2]);
  sleev_comm_block3D_icommSouth_lowtype_cuda(prm,sb_s,rb_s);
  cudaStreamSynchronize(cuStream[3]);
  sleev_comm_block3D_icommNorth_lowtype_cuda(prm,sb_n,rb_n);
#endif
  
  sleev_comm_block3D_waitSouth_cuda(prm);
#if JUPITER_CUDA_AWARE_MPI
#else  
  cudaMemcpyAsync(cu_rb_s, rb_s, sizeof(low_type)*block_nxz, cudaMemcpyHostToDevice,cuStream[2]);
#endif
  readBackSouth_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cuStream[2]>>>(
				       cu_rb_s, f,
				       block_nys, block_nye,
				       nxblock, nyblock, nzblock,
				       mxdivblock, mydivblock, mzdivblock,
				       nxdivblock, nydivblock, nzdivblock);
  
  sleev_comm_block3D_waitNorth_cuda(prm);
#if JUPITER_CUDA_AWARE_MPI
#else  
  cudaMemcpyAsync(cu_rb_n, rb_n, sizeof(low_type)*block_nxz, cudaMemcpyHostToDevice,cuStream[3]);
#endif
  readBackNorth_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cuStream[3]>>>(
				       cu_rb_n, f,
				       block_nys, block_nye,
				       nxblock, nyblock, nzblock,
				       mxdivblock, mydivblock, mzdivblock,
				       nxdivblock, nydivblock, nzdivblock);

  // ----------------------------------------------------------
  // west (x-)
  // east (x+)
#if JUPITER_CUDA_AWARE_MPI
  cudaStreamSynchronize(cuStream[4]);
  sleev_comm_block3D_icommWest_lowtype_cuda(prm,cu_sb_w,cu_rb_w);
  cudaStreamSynchronize(cuStream[5]);
  sleev_comm_block3D_icommEast_lowtype_cuda(prm,cu_sb_e,cu_rb_e);
#else  
  cudaStreamSynchronize(cuStream[4]);
  sleev_comm_block3D_icommWest_lowtype_cuda(prm,sb_w,rb_w);
  cudaStreamSynchronize(cuStream[5]);
  sleev_comm_block3D_icommEast_lowtype_cuda(prm,sb_e,rb_e);
#endif
  
  sleev_comm_block3D_waitWest_cuda(prm);
#if JUPITER_CUDA_AWARE_MPI
#else  
  cudaMemcpyAsync(cu_rb_w, rb_w, sizeof(low_type)*block_nyz, cudaMemcpyHostToDevice,cuStream[4]);
#endif
  readBackWest_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cuStream[4]>>>(
							 cu_rb_w,f,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);
  
  sleev_comm_block3D_waitEast_cuda(prm);
#if JUPITER_CUDA_AWARE_MPI
#else  
  cudaMemcpyAsync(cu_rb_e, rb_e, sizeof(low_type)*block_nyz, cudaMemcpyHostToDevice,cuStream[5]);
#endif
  readBackEast_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cuStream[5]>>>(
							 cu_rb_e ,f,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);
  
  cudaDeviceSynchronize();

  return 0;


}
// ----------------
int sleev_comm_block3D_isendEast_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_e,  low_type *rb_e
				  ){
 
  int nzblock    = prm->nzblock;
  int nyblock    = prm->nyblock;

  int nzdivblock = prm->nzdivblock;
  int nydivblock = prm->nydivblock;

  int block_nz  = nzblock * nzdivblock;
  int block_ny  = nyblock * nydivblock;
  int block_nyz = block_ny * block_nz;

  // east (x+)
  if(prm->nrk[5] > -1) {
    MPI_Isend(sb_e, block_nyz, MPI_LOW_TYPE, prm->nrk[5], 6, prm->comm, &prm->req_send[5]);
  }

  return 0;
}
  
int sleev_comm_block3D_isendWest_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_w,  low_type *rb_w
				  ){

  int nzblock    = prm->nzblock;
  int nyblock    = prm->nyblock;

  int nzdivblock = prm->nzdivblock;
  int nydivblock = prm->nydivblock;

  int block_nz  = nzblock * nzdivblock;
  int block_ny  = nyblock * nydivblock;
  int block_nyz = block_ny * block_nz;

  // west (x-)
  if(prm->nrk[4] > -1) {
    MPI_Isend(sb_w, block_nyz, MPI_LOW_TYPE, prm->nrk[4], 5, prm->comm, &prm->req_send[4]);
  }

  return 0;
}

int sleev_comm_block3D_isendNorth_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_n,  low_type *rb_n
				  ){

  int nzblock    = prm->nzblock;
  int nxblock    = prm->nxblock;

  int nzdivblock = prm->nzdivblock;
  int nxdivblock = prm->nxdivblock;

  int block_nz = nzblock * nzdivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxz = block_nx * block_nz;

  // north (y+)
  if(prm->nrk[3] > -1) {
    MPI_Isend(sb_n, block_nxz, MPI_LOW_TYPE, prm->nrk[3], 4, prm->comm, &prm->req_send[3]);
  }

  return 0;
}

int sleev_comm_block3D_isendSouth_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_s,  low_type *rb_s
				  ){

  int block_m = prm->block_m;

  int nzblock    = prm->nzblock;
  int nxblock    = prm->nxblock;

  int nzdivblock = prm->nzdivblock;
  int nxdivblock = prm->nxdivblock;

  int block_nz = nzblock * nzdivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxz = block_nx * block_nz;

  // south (y-)
  if(prm->nrk[2] > -1) {
    MPI_Isend(sb_s, block_nxz, MPI_LOW_TYPE, prm->nrk[2], 3, prm->comm, &prm->req_send[2]);
  }

  return 0;
}

int sleev_comm_block3D_isendTop_lowtype_cuda(
				  mpi_prm *prm,				  
				  low_type *sb_t,  low_type *rb_t
				  ){

  int block_m = prm->block_m;

  int nyblock    = prm->nyblock;
  int nxblock    = prm->nxblock;

  int nydivblock = prm->nydivblock;
  int nxdivblock = prm->nxdivblock;

  int block_ny = nyblock * nydivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxy = block_nx * block_ny;

  // top    (z+)
  if(prm->nrk[1] > -1) {
    MPI_Isend(sb_t, block_nxy, MPI_LOW_TYPE, prm->nrk[1], 2, prm->comm, &prm->req_send[1]);
  }

  return 0;
}
 
int sleev_comm_block3D_isendBottom_lowtype_cuda(
				  mpi_prm *prm,				  
				  low_type *sb_b,  low_type *rb_b
				  ){

  int i;
  MPI_Status stat_send[6],stat_recv[6];

  int block_m = prm->block_m;

  int nyblock    = prm->nyblock;
  int nxblock    = prm->nxblock;

  int nydivblock = prm->nydivblock;
  int nxdivblock = prm->nxdivblock;

  int block_ny = nyblock * nydivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxy = block_nx * block_ny;

  // bottom (z-)
  if(prm->nrk[0] > -1) { 
    MPI_Isend(sb_b, block_nxy, MPI_LOW_TYPE, prm->nrk[0], 1, prm->comm, &prm->req_send[0]);
  }

  return 0;
} 

int sleev_comm_block3D_irecvEast_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_e,  low_type *rb_e
				  ){
 
  int nzblock    = prm->nzblock;
  int nyblock    = prm->nyblock;

  int nzdivblock = prm->nzdivblock;
  int nydivblock = prm->nydivblock;

  int block_nz  = nzblock * nzdivblock;
  int block_ny  = nyblock * nydivblock;
  int block_nyz = block_ny * block_nz;

  // east (x+)
  if(prm->nrk[5] > -1) {
    MPI_Irecv(rb_e, block_nyz, MPI_LOW_TYPE, prm->nrk[5], 5, prm->comm, &prm->req_recv[5]);
  }

  return 0;
}
  
int sleev_comm_block3D_irecvWest_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_w,  low_type *rb_w
				  ){

  int nzblock    = prm->nzblock;
  int nyblock    = prm->nyblock;

  int nzdivblock = prm->nzdivblock;
  int nydivblock = prm->nydivblock;

  int block_nz  = nzblock * nzdivblock;
  int block_ny  = nyblock * nydivblock;
  int block_nyz = block_ny * block_nz;

  // west (x-)
  if(prm->nrk[4] > -1) {
    MPI_Irecv(rb_w, block_nyz, MPI_LOW_TYPE, prm->nrk[4], 6, prm->comm, &prm->req_recv[4]);
  }

  return 0;
}

int sleev_comm_block3D_irecvNorth_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_n,  low_type *rb_n
				  ){

  int nzblock    = prm->nzblock;
  int nxblock    = prm->nxblock;

  int nzdivblock = prm->nzdivblock;
  int nxdivblock = prm->nxdivblock;

  int block_nz = nzblock * nzdivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxz = block_nx * block_nz;

  // north (y+)
  if(prm->nrk[3] > -1) {
    MPI_Irecv(rb_n, block_nxz, MPI_LOW_TYPE, prm->nrk[3], 3, prm->comm, &prm->req_recv[3]);
  }

  return 0;
}

int sleev_comm_block3D_irecvSouth_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_s,  low_type *rb_s
				  ){

  int block_m = prm->block_m;

  int nzblock    = prm->nzblock;
  int nxblock    = prm->nxblock;

  int nzdivblock = prm->nzdivblock;
  int nxdivblock = prm->nxdivblock;

  int block_nz = nzblock * nzdivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxz = block_nx * block_nz;

  // south (y-)
  if(prm->nrk[2] > -1) {
    MPI_Irecv(rb_s, block_nxz, MPI_LOW_TYPE, prm->nrk[2], 4, prm->comm, &prm->req_recv[2]);
  }

  return 0;
}

int sleev_comm_block3D_irecvTop_lowtype_cuda(
				  mpi_prm *prm,				  
				  low_type *sb_t,  low_type *rb_t
				  ){

  int block_m = prm->block_m;

  int nyblock    = prm->nyblock;
  int nxblock    = prm->nxblock;

  int nydivblock = prm->nydivblock;
  int nxdivblock = prm->nxdivblock;

  int block_ny = nyblock * nydivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxy = block_nx * block_ny;

  // top    (z+)
  if(prm->nrk[1] > -1) {
    MPI_Irecv(rb_t, block_nxy, MPI_LOW_TYPE, prm->nrk[1], 1, prm->comm, &prm->req_recv[1]);
  }

  return 0;
}
 
int sleev_comm_block3D_irecvBottom_lowtype_cuda(
				  mpi_prm *prm,				  
				  low_type *sb_b,  low_type *rb_b
				  ){

  int i;
  MPI_Status stat_send[6],stat_recv[6];

  int block_m = prm->block_m;

  int nyblock    = prm->nyblock;
  int nxblock    = prm->nxblock;

  int nydivblock = prm->nydivblock;
  int nxdivblock = prm->nxdivblock;

  int block_ny = nyblock * nydivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxy = block_nx * block_ny;

  // bottom (z-)
  if(prm->nrk[0] > -1) { 
    MPI_Irecv(rb_b, block_nxy, MPI_LOW_TYPE, prm->nrk[0], 2, prm->comm, &prm->req_recv[0]);
  }

  return 0;
} 

