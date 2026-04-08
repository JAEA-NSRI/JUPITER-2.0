
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <cuda_runtime.h>
//#include <cuda_bf16.h>
//#include <cuda_fp16.h>
#include "cublas_v2.h"

#include "struct.h"
#include "func.h"
#include "cg.h"
#include "sleev_comm_block3d_cuda.h"
#include "sleev_comm_block3d.h"

#include "os/os.h"

// #define JUPITER_CUDA_AWARE_MPI 1

#if 1
int sleev_comm_hostmem_block3D_cuda(
			    type *f,mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    type *cuda_buf,
			    type *sb_b,  type *rb_b,  type *sb_t,  type *rb_t,
			    type *sb_s,  type *rb_s,  type *sb_n,  type *rb_n,
			    type *sb_w,  type *rb_w,  type *sb_e,  type *rb_e,
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

  type *cu_sb_b = &cuda_buf[0];
  type *cu_rb_b = &cuda_buf[block_nxy*1];
  type *cu_sb_t = &cuda_buf[block_nxy*2];
  type *cu_rb_t = &cuda_buf[block_nxy*3];

  type *cu_sb_s = &cuda_buf[block_nxy*4];
  type *cu_rb_s = &cuda_buf[block_nxy*4 + block_nxz*1];
  type *cu_sb_n = &cuda_buf[block_nxy*4 + block_nxz*2];
  type *cu_rb_n = &cuda_buf[block_nxy*4 + block_nxz*3];

  type *cu_sb_w = &cuda_buf[block_nxy*4 + block_nxz*4];
  type *cu_rb_w = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  type *cu_sb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  type *cu_rb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];
  
  cudaDeviceSynchronize();

  const int threads = 32;  
  const int blocks  = 1024;  

  // bottom (z-)
  // top    (z+)
  setBottom_block3D_cuda_kernel<<<blocks,threads,0,cuStream[0]>>>(
						     cu_sb_b ,f,
						     block_nzs, block_nze,
						     nxblock, nyblock, nzblock,
						     mxdivblock, mydivblock, mzdivblock,
						     nxdivblock, nydivblock, nzdivblock);
  setTop_block3D_cuda_kernel<<<blocks,threads,0,cuStream[1]>>>(
						     cu_sb_t ,f,
						     block_nzs, block_nze,
						     nxblock, nyblock, nzblock,
						     mxdivblock, mydivblock, mzdivblock,
						     nxdivblock, nydivblock, nzdivblock);
#if JUPITER_CUDA_AWARE_MPI  

#else
  cudaMemcpyAsync(sb_b, cu_sb_b, sizeof(type)*block_nxy, cudaMemcpyDeviceToHost,cuStream[0]);
  cudaMemcpyAsync(sb_t, cu_sb_t, sizeof(type)*block_nxy, cudaMemcpyDeviceToHost,cuStream[1]);
#endif
  
  // south (y-)
  // north (y+)
  setSouth_block3D_cuda_kernel<<<blocks,threads,0,cuStream[2]>>>(
						      cu_sb_s, f,
						      block_nys, block_nye,
						      nxblock, nyblock, nzblock,
						      mxdivblock, mydivblock, mzdivblock,
						      nxdivblock, nydivblock, nzdivblock);
  setNorth_block3D_cuda_kernel<<<blocks,threads,0,cuStream[3]>>>(
						      cu_sb_n ,f,
						      block_nys, block_nye,
						      nxblock, nyblock, nzblock,
						      mxdivblock, mydivblock, mzdivblock,
						      nxdivblock, nydivblock, nzdivblock);
#if JUPITER_CUDA_AWARE_MPI  

#else
  cudaMemcpyAsync(sb_s, cu_sb_s, sizeof(type)*block_nxz, cudaMemcpyDeviceToHost,cuStream[2]);
  cudaMemcpyAsync(sb_n, cu_sb_n, sizeof(type)*block_nxz, cudaMemcpyDeviceToHost,cuStream[3]);
#endif
  
  // west (x-)
  // east (x+)
  setWest_block3D_cuda_kernel<<<blocks,threads,0,cuStream[4]>>>(
						    cu_sb_w ,f,
						    block_nxs, block_nxe,
						    nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);
  setEast_block3D_cuda_kernel<<<blocks,threads,0,cuStream[5]>>>(
						    cu_sb_e ,f,
						    block_nxs, block_nxe,
						    nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);
#if JUPITER_CUDA_AWARE_MPI  

#else
  cudaMemcpyAsync(sb_w, cu_sb_w, sizeof(type)*block_nyz, cudaMemcpyDeviceToHost,cuStream[4]);
  cudaMemcpyAsync(sb_e, cu_sb_e, sizeof(type)*block_nyz, cudaMemcpyDeviceToHost,cuStream[5]);
#endif
  // ----------------------------------------------------------

  // top    (z+)
  // bottom (z-)
#if JUPITER_CUDA_AWARE_MPI  
  cudaStreamSynchronize(cuStream[0]);
  sleev_comm_block3D_icommBottom_cuda(prm,cu_sb_b,cu_rb_b); 
  cudaStreamSynchronize(cuStream[1]);
  sleev_comm_block3D_icommTop_cuda(prm,cu_sb_t,cu_rb_t);  
#else
  cudaStreamSynchronize(cuStream[0]);
  sleev_comm_block3D_icommBottom_cuda(prm,sb_b,rb_b); 
  cudaStreamSynchronize(cuStream[1]);
  sleev_comm_block3D_icommTop_cuda(prm,sb_t,rb_t);  
#endif

  sleev_comm_block3D_waitBottom_cuda(prm);
#if JUPITER_CUDA_AWARE_MPI  

#else
  cudaMemcpyAsync(cu_rb_b, rb_b, sizeof(type)*block_nxy, cudaMemcpyHostToDevice,cuStream[0]);
#endif
  readBackBottom_block3D_cuda_kernel<<<blocks,threads,0,cuStream[0]>>>(
							  cu_rb_b, f,
							  block_nzs, block_nze,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);
  
  sleev_comm_block3D_waitTop_cuda(prm);  
#if JUPITER_CUDA_AWARE_MPI  

#else
  cudaMemcpyAsync(cu_rb_t, rb_t, sizeof(type)*block_nxy, cudaMemcpyHostToDevice,cuStream[1]);
#endif
  readBackTop_block3D_cuda_kernel<<<blocks,threads,0,cuStream[1]>>>(
							  cu_rb_t, f,
							  block_nzs, block_nze,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);
  // south (y-)
  // north (y+)
#if JUPITER_CUDA_AWARE_MPI  
  cudaStreamSynchronize(cuStream[2]);
  sleev_comm_block3D_icommSouth_cuda(prm,cu_sb_s,cu_rb_s);  
  cudaStreamSynchronize(cuStream[3]);  
  sleev_comm_block3D_icommNorth_cuda(prm,cu_sb_n,cu_rb_n);
#else
  cudaStreamSynchronize(cuStream[2]);
  sleev_comm_block3D_icommSouth_cuda(prm,sb_s,rb_s);  
  cudaStreamSynchronize(cuStream[3]);  
  sleev_comm_block3D_icommNorth_cuda(prm,sb_n,rb_n);
#endif
  
  sleev_comm_block3D_waitSouth_cuda(prm);
#if JUPITER_CUDA_AWARE_MPI  

#else
  cudaMemcpyAsync(cu_rb_s, rb_s, sizeof(type)*block_nxz, cudaMemcpyHostToDevice,cuStream[2]);
#endif
  readBackSouth_block3D_cuda_kernel<<<blocks,threads,0,cuStream[2]>>>(
				       cu_rb_s, f,
				       block_nys, block_nye,
				       nxblock, nyblock, nzblock,
				       mxdivblock, mydivblock, mzdivblock,
				       nxdivblock, nydivblock, nzdivblock);
  
  sleev_comm_block3D_waitNorth_cuda(prm);
#if JUPITER_CUDA_AWARE_MPI  

#else
  cudaMemcpyAsync(cu_rb_n, rb_n, sizeof(type)*block_nxz, cudaMemcpyHostToDevice,cuStream[3]);  
#endif
  readBackNorth_block3D_cuda_kernel<<<blocks,threads,0,cuStream[3]>>>(
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
  sleev_comm_block3D_icommWest_cuda(prm,cu_sb_w,cu_rb_w); 
  cudaStreamSynchronize(cuStream[5]);
  sleev_comm_block3D_icommEast_cuda(prm,cu_sb_e,cu_rb_e);
#else
  cudaStreamSynchronize(cuStream[4]);
  sleev_comm_block3D_icommWest_cuda(prm,sb_w,rb_w);  
  cudaStreamSynchronize(cuStream[5]);
  sleev_comm_block3D_icommEast_cuda(prm,sb_e,rb_e);
#endif
  
  sleev_comm_block3D_waitWest_cuda(prm);
#if JUPITER_CUDA_AWARE_MPI  

#else
  cudaMemcpyAsync(cu_rb_w, rb_w, sizeof(type)*block_nyz, cudaMemcpyHostToDevice,cuStream[4]);
#endif
  readBackWest_block3D_cuda_kernel<<<blocks,threads,0,cuStream[4]>>>(
							 cu_rb_w,f,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);

  sleev_comm_block3D_waitEast_cuda(prm);
#if JUPITER_CUDA_AWARE_MPI  

#else
  cudaMemcpyAsync(cu_rb_e, rb_e, sizeof(type)*block_nyz, cudaMemcpyHostToDevice,cuStream[5]);
#endif
  readBackEast_block3D_cuda_kernel<<<blocks,threads,0,cuStream[5]>>>(
							 cu_rb_e ,f,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);
  
  cudaDeviceSynchronize();

  return 0;


}

#else
int sleev_comm_hostmem_block3D_cuda(
			    type *f,mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    type *cuda_buf,
			    type *sb_b,  type *rb_b,  type *sb_t,  type *rb_t,
			    type *sb_s,  type *rb_s,  type *sb_n,  type *rb_n,
			    type *sb_w,  type *rb_w,  type *sb_e,  type *rb_e,
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

  type *cu_sb_b = &cuda_buf[0];
  type *cu_rb_b = &cuda_buf[block_nxy*1];
  type *cu_sb_t = &cuda_buf[block_nxy*2];
  type *cu_rb_t = &cuda_buf[block_nxy*3];

  type *cu_sb_s = &cuda_buf[block_nxy*4];
  type *cu_rb_s = &cuda_buf[block_nxy*4 + block_nxz*1];
  type *cu_sb_n = &cuda_buf[block_nxy*4 + block_nxz*2];
  type *cu_rb_n = &cuda_buf[block_nxy*4 + block_nxz*3];

  type *cu_sb_w = &cuda_buf[block_nxy*4 + block_nxz*4];
  type *cu_rb_w = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  type *cu_sb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  type *cu_rb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];

  cudaDeviceSynchronize();

  const int threads = 32;  
  const int blocks  = 1024;
  
  // bottom (z-)
  // top    (z+)
  setTopAndBottom_block3D_cuda_kernel<<<blocks,threads,0,cuStream[0]>>>(
						     cu_sb_b, cu_sb_t ,f,
						     block_nzs, block_nze,
						     nxblock, nyblock, nzblock,
						     mxdivblock, mydivblock, mzdivblock,
						     nxdivblock, nydivblock, nzdivblock);
  cudaMemcpyAsync(sb_b, cu_sb_b, sizeof(type)*block_nxy, cudaMemcpyDeviceToHost,cuStream[0]);
  cudaMemcpyAsync(sb_t, cu_sb_t, sizeof(type)*block_nxy, cudaMemcpyDeviceToHost,cuStream[0]);

  // south (y-)
  // north (y+)
  setNorthAndSouth_block3D_cuda_kernel<<<blocks,threads,0,cuStream[1]>>>(
						      cu_sb_s, cu_sb_n ,f,
						      block_nys, block_nye,
						      nxblock, nyblock, nzblock,
						      mxdivblock, mydivblock, mzdivblock,
						      nxdivblock, nydivblock, nzdivblock);
  cudaMemcpyAsync(sb_s, cu_sb_s, sizeof(type)*block_nxz, cudaMemcpyDeviceToHost,cuStream[1]);
  cudaMemcpyAsync(sb_n, cu_sb_n, sizeof(type)*block_nxz, cudaMemcpyDeviceToHost,cuStream[1]);

  // west (x-)
  // east (x+)
  setWestAndEast_block3D_cuda_kernel<<<blocks,threads,0,cuStream[2]>>>(
						    cu_sb_w, cu_sb_e ,f,
						    block_nxs, block_nxe,
						    nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);
  cudaMemcpyAsync(sb_w, cu_sb_w, sizeof(type)*block_nyz, cudaMemcpyDeviceToHost,cuStream[2]);
  cudaMemcpyAsync(sb_e, cu_sb_e, sizeof(type)*block_nyz, cudaMemcpyDeviceToHost,cuStream[2]);


  // ----------------------------------------------------------
  cudaStreamSynchronize(cuStream[0]);
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
  for(i = 0; i < 2; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }
  cudaMemcpyAsync(cu_rb_b, rb_b, sizeof(type)*block_nxy, cudaMemcpyHostToDevice,cuStream[0]);
  cudaMemcpyAsync(cu_rb_t, rb_t, sizeof(type)*block_nxy, cudaMemcpyHostToDevice,cuStream[0]);
  // bottom (z-)
  // top    (z+)
  readBackTopAndBottom_block3D_cuda_kernel<<<blocks,threads,0,cuStream[0]>>>(
							  cu_rb_b, cu_rb_t ,f,
							  block_nzs, block_nze,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);
  // ----------------------------------------------------------
  cudaStreamSynchronize(cuStream[1]);
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
  for(i = 2; i < 4; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }
  cudaMemcpyAsync(cu_rb_s, rb_s, sizeof(type)*block_nxz, cudaMemcpyHostToDevice,cuStream[1]);
  cudaMemcpyAsync(cu_rb_n, rb_n, sizeof(type)*block_nxz, cudaMemcpyHostToDevice,cuStream[1]);
  // south (y-)
  // north (y+)
  //  int jyb,jjyb; 
  readBackNorthAndSouth_block3D_cuda_kernel<<<blocks,threads,0,cuStream[1]>>>(
				       cu_rb_s, cu_rb_n ,f,
				       block_nys, block_nye,
				       nxblock, nyblock, nzblock,
				       mxdivblock, mydivblock, mzdivblock,
				       nxdivblock, nydivblock, nzdivblock);

  // ----------------------------------------------------------
  cudaStreamSynchronize(cuStream[2]);

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

  for(i = 4; i < 6; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }

  cudaMemcpyAsync(cu_rb_w, rb_w, sizeof(type)*block_nyz, cudaMemcpyHostToDevice,cuStream[2]);
  cudaMemcpyAsync(cu_rb_e, rb_e, sizeof(type)*block_nyz, cudaMemcpyHostToDevice,cuStream[2]);
  // west (x-)
  // east (x+)
  //  int jxb,jjxb; 
  readBackWestAndEast_block3D_cuda_kernel<<<blocks,threads,0,cuStream[2]>>>(
							 cu_rb_w, cu_rb_e ,f,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);
  cudaStreamSynchronize(cuStream[0]);
  cudaStreamSynchronize(cuStream[1]);
  cudaStreamSynchronize(cuStream[2]);
  cudaDeviceSynchronize();

  return 0;
}
#endif

int sleev_comm_hostmem_block3D_cuda_time(
			    type *f,mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    type *cuda_buf,
			    type *sb_b,  type *rb_b,  type *sb_t,  type *rb_t,
			    type *sb_s,  type *rb_s,  type *sb_n,  type *rb_n,
			    type *sb_w,  type *rb_w,  type *sb_e,  type *rb_e,
			    cudaStream_t *cuStream,
			    double *time
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

  type *cu_sb_b = &cuda_buf[0];
  type *cu_rb_b = &cuda_buf[block_nxy*1];
  type *cu_sb_t = &cuda_buf[block_nxy*2];
  type *cu_rb_t = &cuda_buf[block_nxy*3];

  type *cu_sb_s = &cuda_buf[block_nxy*4];
  type *cu_rb_s = &cuda_buf[block_nxy*4 + block_nxz*1];
  type *cu_sb_n = &cuda_buf[block_nxy*4 + block_nxz*2];
  type *cu_rb_n = &cuda_buf[block_nxy*4 + block_nxz*3];

  type *cu_sb_w = &cuda_buf[block_nxy*4 + block_nxz*4];
  type *cu_rb_w = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  type *cu_sb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  type *cu_rb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];

  cudaDeviceSynchronize();

  const int threads = 32;  
  const int blocks  = 1024;

  double ts,te;
  // bottom (z-)
  // top    (z+)
  ts=cpu_time();
  setTopAndBottom_block3D_cuda_kernel<<<blocks,threads>>>(
						     cu_sb_b, cu_sb_t ,f,
						     block_nzs, block_nze,
						     nxblock, nyblock, nzblock,
						     mxdivblock, mydivblock, mzdivblock,
						     nxdivblock, nydivblock, nzdivblock);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[0]=time[0]+te-ts;

  ts=cpu_time();
  cudaMemcpy(sb_b, cu_sb_b, sizeof(type)*block_nxy,cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_t, cu_sb_t, sizeof(type)*block_nxy,cudaMemcpyDeviceToHost);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[3]=time[3]+te-ts;

  // south (y-)
  // north (y+)
  ts=cpu_time();
  setNorthAndSouth_block3D_cuda_kernel<<<blocks,threads>>>(
						      cu_sb_s, cu_sb_n ,f,
						      block_nys, block_nye,
						      nxblock, nyblock, nzblock,
						      mxdivblock, mydivblock, mzdivblock,
						      nxdivblock, nydivblock, nzdivblock);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[1]=time[1]+te-ts;

  ts=cpu_time();
  cudaMemcpy(sb_s, cu_sb_s, sizeof(type)*block_nxz,cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_n, cu_sb_n, sizeof(type)*block_nxz,cudaMemcpyDeviceToHost);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[3]=time[3]+te-ts;

  // west (x-)
  // east (x+)
  ts=cpu_time();
  setWestAndEast_block3D_cuda_kernel<<<blocks,threads>>>(
						    cu_sb_w, cu_sb_e ,f,
						    block_nxs, block_nxe,
						    nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[2]=time[2]+te-ts;
  
  ts=cpu_time();
  cudaMemcpy(sb_w, cu_sb_w, sizeof(type)*block_nyz,cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_e, cu_sb_e, sizeof(type)*block_nyz,cudaMemcpyDeviceToHost);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[3]=time[3]+te-ts;


  // ----------------------------------------------------------
  ts=cpu_time();
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
  for(i = 0; i < 2; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }
  cudaDeviceSynchronize();
  te=cpu_time();
  time[4]=time[4]+te-ts;
  
  ts=cpu_time();
  cudaMemcpy(cu_rb_b, rb_b, sizeof(type)*block_nxy,cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_t, rb_t, sizeof(type)*block_nxy,cudaMemcpyHostToDevice);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[5]=time[5]+te-ts;

  // bottom (z-)
  // top    (z+)
  ts=cpu_time();
  readBackTopAndBottom_block3D_cuda_kernel<<<blocks,threads>>>(
							  cu_rb_b, cu_rb_t ,f,
							  block_nzs, block_nze,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[6]=time[6]+te-ts;
  // ----------------------------------------------------------

  ts=cpu_time();
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
  for(i = 2; i < 4; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }
  cudaDeviceSynchronize();
  te=cpu_time();
  time[4]=time[4]+te-ts;

  ts=cpu_time();
  cudaMemcpy(cu_rb_s, rb_s, sizeof(type)*block_nxz,cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_n, rb_n, sizeof(type)*block_nxz,cudaMemcpyHostToDevice);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[5]=time[5]+te-ts;

  // south (y-)
  // north (y+)
  ts=cpu_time();
  readBackNorthAndSouth_block3D_cuda_kernel<<<blocks,threads>>>(
				       cu_rb_s, cu_rb_n ,f,
				       block_nys, block_nye,
				       nxblock, nyblock, nzblock,
				       mxdivblock, mydivblock, mzdivblock,
				       nxdivblock, nydivblock, nzdivblock);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[7]=time[7]+te-ts;

  // ----------------------------------------------------------

  // west (x-)
  ts=cpu_time();
  if(prm->nrk[4] > -1) {
    MPI_Isend(sb_w, block_nyz, MPI_TYPE, prm->nrk[4], 5, prm->comm, &prm->req_send[4]);
    MPI_Irecv(rb_w, block_nyz, MPI_TYPE, prm->nrk[4], 6, prm->comm, &prm->req_recv[4]);
  }
  // east (x+)
  if(prm->nrk[5] > -1) {
    MPI_Isend(sb_e, block_nyz, MPI_TYPE, prm->nrk[5], 6, prm->comm, &prm->req_send[5]);
    MPI_Irecv(rb_e, block_nyz, MPI_TYPE, prm->nrk[5], 5, prm->comm, &prm->req_recv[5]);
  }

  for(i = 4; i < 6; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }
  cudaDeviceSynchronize();
  te=cpu_time();
  time[4]=time[4]+te-ts;

  ts=cpu_time();
  cudaMemcpy(cu_rb_w, rb_w, sizeof(type)*block_nyz,cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_e, rb_e, sizeof(type)*block_nyz,cudaMemcpyHostToDevice);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[5]=time[5]+te-ts;
  
  // west (x-)
  // east (x+)
  ts=cpu_time();
  readBackWestAndEast_block3D_cuda_kernel<<<blocks,threads>>>(
							 cu_rb_w, cu_rb_e ,f,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[8]=time[8]+te-ts;
  cudaDeviceSynchronize();

  return 0;
}


int sleev_comm_block3D_commX_cuda(
				  mpi_prm *prm,
				  type *sb_w,  type *rb_w,
				  type *sb_e,  type *rb_e
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

  int block_nz  = nzblock * nzdivblock;
  int block_ny  = nyblock * nydivblock;
  int block_nx  = nxblock * nxdivblock;
  int block_nxy = block_nx * block_ny;
  int block_nxz = block_nx * block_nz;
  int block_nyz = block_ny * block_nz;

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

  for(i = 4; i < 6; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }

  return 0;
}

int sleev_comm_block3D_commY_cuda(
				  mpi_prm *prm,
				  type *sb_s,  type *rb_s,
				  type *sb_n,  type *rb_n
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

  int block_nz = nzblock * nzdivblock;
  int block_ny = nyblock * nydivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxy = block_nx * block_ny;
  int block_nxz = block_nx * block_nz;
  int block_nyz = block_ny * block_nz;

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

  for(i = 2; i < 4; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }

  return 0;
}
int sleev_comm_block3D_commZ_cuda(
				  mpi_prm *prm,				  
				  type *sb_b,  type *rb_b,
				  type *sb_t,  type *rb_t
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

  int block_nz = nzblock * nzdivblock;
  int block_ny = nyblock * nydivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxy = block_nx * block_ny;
  int block_nxz = block_nx * block_nz;
  int block_nyz = block_ny * block_nz;

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

  for(i = 0; i < 2; i++) {
    if(prm->nrk[i] > -1) {
      MPI_Wait(&prm->req_send[i], &stat_send[i]);
      MPI_Wait(&prm->req_recv[i], &stat_recv[i]);
    }
  }

  return 0;
}


#if 0

void setWestAndEast_block3D_cpu(
					       type *sb_w, type *sb_e ,type *f,
					       int *block_nxs, int *block_nxe,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = 0;
  const int domain = nzdivblock*nydivblock;
  if(tid >= domain) return;
  int jxb ;
  int jjxb;

  jxb  = 0;
  jjxb = block_nxs[0];
  for(int jb=tid;jb<domain;jb++){
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

  jxb  = nxdivblock-1;
  jjxb = block_nxe[nxdivblock-1];
  for(int jb=tid;jb<domain;jb++){
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

void setNorthAndSouth_block3D_cpu(
					       type *sb_s, type *sb_n ,type *f,
					       int *block_nys, int *block_nye,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = 0;
  const int domain = nzdivblock*nxdivblock;
  if(tid >= domain) return;

  int jyb;
  int jjyb;

  jyb  = 0;
  jjyb = block_nys[0];
#if 1
  for(int jb=tid;jb<domain;jb++){{
    int jzb = (jb%(nxdivblock*nzdivblock))/nxdivblock;
    int jxb = (jb%(nxdivblock*nzdivblock))%nxdivblock ;    
#else
  for(int jzb=0;jzb<nzdivblock;jzb++){ 
    for(int jxb=0;jxb<nxdivblock;jxb++){ 
#endif
    for(int jjzb=0;jjzb<nzblock;jjzb++){
      for(int jjxb=0;jjxb<nxblock;jjxb++){
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	sb_s[j] = f[jj];
      }
    }
    }}

  jyb  = nydivblock-1;
  jjyb = block_nye[nydivblock-1];
  for(int jb=tid;jb<domain;jb++){
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

void setTopAndBottom_block3D_cpu(
					       type *sb_b, type *sb_t ,type *f,
					       // const int jzb_b,const int jjzb_b,
					       // const int jzb_t,const int jjzb_t,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = 0;
  const int domain = nxdivblock * nydivblock;

  if(tid >= domain) return;

  int jzb,jjzb;
  jzb  = 0;
  jjzb = block_nzs[0];
  for(int jb=tid;jb<domain;jb++){
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

  jzb  = nzdivblock-1;
  jjzb = block_nze[nzdivblock-1];
  for(int jb=tid;jb<domain;jb++){
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

void readBackWestAndEast_block3D_cpu(
					       type *rb_w, type *rb_e ,type *f,
					       int *block_nxs, int *block_nxe,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = 0;
  const int domain = nzdivblock*nydivblock;
  if(tid >= domain) return;

  int jxb ;
  int jjxb;

  jxb  = -1;
  jjxb = nxblock-1;
  for(int jb=tid;jb<domain;jb++){
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

  if(block_nxe[nxdivblock-1]==(nxblock-1)){   
    jxb  = nxdivblock;
    jjxb = 0;
  }else{
    jxb  = nxdivblock-1;
    jjxb = block_nxe[nxdivblock-1]+1;
  }
  for(int jb=tid;jb<domain;jb++){
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

void readBackNorthAndSouth_block3D_cpu(
					       type *rb_s, type *rb_n ,type *f,
					       int *block_nys, int *block_nye,
 					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = 0;
  //  const int domain = nydivblock*nxdivblock;
  const int domain = nzdivblock*nxdivblock;
  if(tid >= domain) return;
  int jyb ;
  int jjyb;

  jyb  = -1;
  jjyb = nyblock-1;
  for(int jb=tid;jb<domain;jb++){
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

  if(block_nye[nydivblock-1]==(nyblock-1)){   
    jyb  = nydivblock;
    jjyb = 0;
  }else{
    jyb  = nydivblock-1;
    jjyb = block_nye[nydivblock-1]+1;
  }
  for(int jb=tid;jb<domain;jb++){
    int jzb = (jb%(nxdivblock*nzdivblock))/nxdivblock;
    int jxb = (jb%(nxdivblock*nzdivblock))%nxdivblock ;    
    for(int jjzb=0;jjzb<nzblock;jjzb++){
      for(int jjxb=0;jjxb<nxblock;jjxb++){
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	rb_n[j] = f[jj];
      }
    }
  }

  return ;
}

void readBackTopAndBottom_block3D_cpu(
					       type *rb_b, type *rb_t ,type *f,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = 0;
  const int domain = nxdivblock * nydivblock;
  if(tid >= domain) return;
  int jzb ;
  int jjzb;

  jzb  = -1;
  jjzb = nzblock-1;
  for(int jb=tid;jb<domain;jb++){
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

  if(block_nze[nzdivblock-1]==(nzblock-1)){   
    jzb  = nzdivblock;
    jjzb = 0;
  }else{
    jzb  = nzdivblock-1;
    jjzb = block_nze[nzdivblock-1]+1;
  }
  for(int jb=tid;jb<domain;jb++){
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

#endif

__global__ void setWestAndEast_block3D_cuda_kernel(
					       type *sb_w, type *sb_e ,type *f,
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

__global__ void setNorthAndSouth_block3D_cuda_kernel(
					       type *sb_s, type *sb_n ,type *f,
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

__global__ void setTopAndBottom_block3D_cuda_kernel(
					       type *sb_b, type *sb_t ,type *f,
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

__global__ void readBackWestAndEast_block3D_cuda_kernel(
					       type *rb_w, type *rb_e ,type *f,
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

__global__ void readBackNorthAndSouth_block3D_cuda_kernel(
					       type *rb_s, type *rb_n ,type *f,
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

__global__ void readBackTopAndBottom_block3D_cuda_kernel(
					       type *rb_b, type *rb_t ,type *f,
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

#if 1
int sleev_comm_block3D_set_cuda(
				 type *f,mpi_prm *prm,
				 int *block_nxs, int *block_nxe, 
				 int *block_nys, int *block_nye, 
				 int *block_nzs, int *block_nze,
				 type *cuda_buf,
				 type *cpu_buf,
				 cudaStream_t *cudaStream
				){

  if(prm[0].npe==1){
    return 0;
  }

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

  type *cu_sb_b = &cuda_buf[0];
  type *cu_rb_b = &cuda_buf[block_nxy*1];
  type *cu_sb_t = &cuda_buf[block_nxy*2];
  type *cu_rb_t = &cuda_buf[block_nxy*3];

  type *cu_sb_s = &cuda_buf[block_nxy*4];
  type *cu_rb_s = &cuda_buf[block_nxy*4 + block_nxz*1];
  type *cu_sb_n = &cuda_buf[block_nxy*4 + block_nxz*2];
  type *cu_rb_n = &cuda_buf[block_nxy*4 + block_nxz*3];

  type *cu_sb_w = &cuda_buf[block_nxy*4 + block_nxz*4];
  type *cu_rb_w = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  type *cu_sb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  type *cu_rb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];

  // ------------
  type *sb_b = &cpu_buf[0];
  type *rb_b = &cpu_buf[block_nxy*1];
  type *sb_t = &cpu_buf[block_nxy*2];
  type *rb_t = &cpu_buf[block_nxy*3];

  type *sb_s = &cpu_buf[block_nxy*4];
  type *rb_s = &cpu_buf[block_nxy*4 + block_nxz*1];
  type *sb_n = &cpu_buf[block_nxy*4 + block_nxz*2];
  type *rb_n = &cpu_buf[block_nxy*4 + block_nxz*3];

  type *sb_w = &cpu_buf[block_nxy*4 + block_nxz*4];
  type *rb_w = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  type *sb_e = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  type *rb_e = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];

  const int threads = 32;  
  const int blocks  = 1024;  

  // bottom (z-)
  // top    (z+)
  setTopAndBottom_block3D_cuda_kernel<<<blocks,threads>>>(
						     cu_sb_b, cu_sb_t ,f,
						     block_nzs, block_nze,
						     nxblock, nyblock, nzblock,
						     mxdivblock, mydivblock, mzdivblock,
						     nxdivblock, nydivblock, nzdivblock);
  // south (y-)
  // north (y+)
  setNorthAndSouth_block3D_cuda_kernel<<<blocks,threads>>>(
						      cu_sb_s, cu_sb_n ,f,
						      block_nys, block_nye,
						      nxblock, nyblock, nzblock,
						      mxdivblock, mydivblock, mzdivblock,
						      nxdivblock, nydivblock, nzdivblock);
  // west (x-)
  // east (x+)
  setWestAndEast_block3D_cuda_kernel<<<blocks,threads>>>(
						    cu_sb_w, cu_sb_e ,f,
						    block_nxs, block_nxe,
						    nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);

#if USE_KRYLOV_COMM_OVL
  cudaMemcpyAsync(sb_b, cu_sb_b, sizeof(type)*block_nxy, cudaMemcpyDeviceToHost,cudaStream[0]);
  cudaMemcpyAsync(sb_t, cu_sb_t, sizeof(type)*block_nxy, cudaMemcpyDeviceToHost,cudaStream[0]);
  cudaMemcpyAsync(sb_s, cu_sb_s, sizeof(type)*block_nxz, cudaMemcpyDeviceToHost,cudaStream[0]);
  cudaMemcpyAsync(sb_n, cu_sb_n, sizeof(type)*block_nxz, cudaMemcpyDeviceToHost,cudaStream[0]);
  cudaMemcpyAsync(sb_w, cu_sb_w, sizeof(type)*block_nyz, cudaMemcpyDeviceToHost,cudaStream[0]);
  cudaMemcpyAsync(sb_e, cu_sb_e, sizeof(type)*block_nyz, cudaMemcpyDeviceToHost,cudaStream[0]);
  cudaStreamSynchronize(cudaStream[0]);
#else
  cudaMemcpy(sb_b, cu_sb_b, sizeof(type)*block_nxy, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_t, cu_sb_t, sizeof(type)*block_nxy, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_s, cu_sb_s, sizeof(type)*block_nxz, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_n, cu_sb_n, sizeof(type)*block_nxz, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_w, cu_sb_w, sizeof(type)*block_nyz, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_e, cu_sb_e, sizeof(type)*block_nyz, cudaMemcpyDeviceToHost);
  cudaDeviceSynchronize();
#endif
  return 0;
}

int sleev_comm_block3D_comm_cuda(
			    mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    type *cuda_buf,
			    type *cpu_buf
		       ){
#if 0
  if(prm[0].npe==1){
    return 0;
  }
#endif

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

#if JUPITER_CUDA_AWARE_MPI
  type *cu_sb_b = &cuda_buf[0];
  type *cu_rb_b = &cuda_buf[block_nxy*1];
  type *cu_sb_t = &cuda_buf[block_nxy*2];
  type *cu_rb_t = &cuda_buf[block_nxy*3];

  type *cu_sb_s = &cuda_buf[block_nxy*4];
  type *cu_rb_s = &cuda_buf[block_nxy*4 + block_nxz*1];
  type *cu_sb_n = &cuda_buf[block_nxy*4 + block_nxz*2];
  type *cu_rb_n = &cuda_buf[block_nxy*4 + block_nxz*3];

  type *cu_sb_w = &cuda_buf[block_nxy*4 + block_nxz*4];
  type *cu_rb_w = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  type *cu_sb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  type *cu_rb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];

  // bottom (z-)
  if(prm->nrk[0] > -1) { 
    MPI_Isend(cu_sb_b, block_nxy, MPI_TYPE, prm->nrk[0], 1, prm->comm, &prm->req_send[0]);
    MPI_Irecv(cu_rb_b, block_nxy, MPI_TYPE, prm->nrk[0], 2, prm->comm, &prm->req_recv[0]);
  }
 // top    (z+)
  if(prm->nrk[1] > -1) {
    MPI_Isend(cu_sb_t, block_nxy, MPI_TYPE, prm->nrk[1], 2, prm->comm, &prm->req_send[1]);
    MPI_Irecv(cu_rb_t, block_nxy, MPI_TYPE, prm->nrk[1], 1, prm->comm, &prm->req_recv[1]);
  }

 // south (y-)
  if(prm->nrk[2] > -1) {
    MPI_Isend(cu_sb_s, block_nxz, MPI_TYPE, prm->nrk[2], 3, prm->comm, &prm->req_send[2]);
    MPI_Irecv(cu_rb_s, block_nxz, MPI_TYPE, prm->nrk[2], 4, prm->comm, &prm->req_recv[2]);
  }
  // north (y+)
  if(prm->nrk[3] > -1) {
    MPI_Isend(cu_sb_n, block_nxz, MPI_TYPE, prm->nrk[3], 4, prm->comm, &prm->req_send[3]);
    MPI_Irecv(cu_rb_n, block_nxz, MPI_TYPE, prm->nrk[3], 3, prm->comm, &prm->req_recv[3]);
  }

  // west (x-)
  if(prm->nrk[4] > -1) {
    MPI_Isend(cu_sb_w, block_nyz, MPI_TYPE, prm->nrk[4], 5, prm->comm, &prm->req_send[4]);
    MPI_Irecv(cu_rb_w, block_nyz, MPI_TYPE, prm->nrk[4], 6, prm->comm, &prm->req_recv[4]);
  }
  // east (x+)
  if(prm->nrk[5] > -1) {
    MPI_Isend(cu_sb_e, block_nyz, MPI_TYPE, prm->nrk[5], 6, prm->comm, &prm->req_send[5]);
    MPI_Irecv(cu_rb_e, block_nyz, MPI_TYPE, prm->nrk[5], 5, prm->comm, &prm->req_recv[5]);
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
#else
  type *sb_b = &cpu_buf[0];
  type *rb_b = &cpu_buf[block_nxy*1];
  type *sb_t = &cpu_buf[block_nxy*2];
  type *rb_t = &cpu_buf[block_nxy*3];

  type *sb_s = &cpu_buf[block_nxy*4];
  type *rb_s = &cpu_buf[block_nxy*4 + block_nxz*1];
  type *sb_n = &cpu_buf[block_nxy*4 + block_nxz*2];
  type *rb_n = &cpu_buf[block_nxy*4 + block_nxz*3];

  type *sb_w = &cpu_buf[block_nxy*4 + block_nxz*4];
  type *rb_w = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  type *sb_e = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  type *rb_e = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];

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

#endif

  return 0;
}


int sleev_comm_block3D_icomm_cuda(
			    mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    type *cuda_buf,
			    type *cpu_buf
		       ){

  if(prm[0].npe==1){
    return 0;
  }

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

#if JUPITER_CUDA_AWARE_MPI
  type *cu_sb_b = &cuda_buf[0];
  type *cu_rb_b = &cuda_buf[block_nxy*1];
  type *cu_sb_t = &cuda_buf[block_nxy*2];
  type *cu_rb_t = &cuda_buf[block_nxy*3];

  type *cu_sb_s = &cuda_buf[block_nxy*4];
  type *cu_rb_s = &cuda_buf[block_nxy*4 + block_nxz*1];
  type *cu_sb_n = &cuda_buf[block_nxy*4 + block_nxz*2];
  type *cu_rb_n = &cuda_buf[block_nxy*4 + block_nxz*3];

  type *cu_sb_w = &cuda_buf[block_nxy*4 + block_nxz*4];
  type *cu_rb_w = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  type *cu_sb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  type *cu_rb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];

  // bottom (z-)
  if(prm->nrk[0] > -1) { 
    MPI_Isend(cu_sb_b, block_nxy, MPI_TYPE, prm->nrk[0], 1, prm->comm, &prm->req_send[0]);
    MPI_Irecv(cu_rb_b, block_nxy, MPI_TYPE, prm->nrk[0], 2, prm->comm, &prm->req_recv[0]);
  }
 // top    (z+)
  if(prm->nrk[1] > -1) {
    MPI_Isend(cu_sb_t, block_nxy, MPI_TYPE, prm->nrk[1], 2, prm->comm, &prm->req_send[1]);
    MPI_Irecv(cu_rb_t, block_nxy, MPI_TYPE, prm->nrk[1], 1, prm->comm, &prm->req_recv[1]);
  }

 // south (y-)
  if(prm->nrk[2] > -1) {
    MPI_Isend(cu_sb_s, block_nxz, MPI_TYPE, prm->nrk[2], 3, prm->comm, &prm->req_send[2]);
    MPI_Irecv(cu_rb_s, block_nxz, MPI_TYPE, prm->nrk[2], 4, prm->comm, &prm->req_recv[2]);
  }
  // north (y+)
  if(prm->nrk[3] > -1) {
    MPI_Isend(cu_sb_n, block_nxz, MPI_TYPE, prm->nrk[3], 4, prm->comm, &prm->req_send[3]);
    MPI_Irecv(cu_rb_n, block_nxz, MPI_TYPE, prm->nrk[3], 3, prm->comm, &prm->req_recv[3]);
  }

  // west (x-)
  if(prm->nrk[4] > -1) {
    MPI_Isend(cu_sb_w, block_nyz, MPI_TYPE, prm->nrk[4], 5, prm->comm, &prm->req_send[4]);
    MPI_Irecv(cu_rb_w, block_nyz, MPI_TYPE, prm->nrk[4], 6, prm->comm, &prm->req_recv[4]);
  }
  // east (x+)
  if(prm->nrk[5] > -1) {
    MPI_Isend(cu_sb_e, block_nyz, MPI_TYPE, prm->nrk[5], 6, prm->comm, &prm->req_send[5]);
    MPI_Irecv(cu_rb_e, block_nyz, MPI_TYPE, prm->nrk[5], 5, prm->comm, &prm->req_recv[5]);
  }

#if 0
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
#endif

#else
  type *sb_b = &cpu_buf[0];
  type *rb_b = &cpu_buf[block_nxy*1];
  type *sb_t = &cpu_buf[block_nxy*2];
  type *rb_t = &cpu_buf[block_nxy*3];

  type *sb_s = &cpu_buf[block_nxy*4];
  type *rb_s = &cpu_buf[block_nxy*4 + block_nxz*1];
  type *sb_n = &cpu_buf[block_nxy*4 + block_nxz*2];
  type *rb_n = &cpu_buf[block_nxy*4 + block_nxz*3];

  type *sb_w = &cpu_buf[block_nxy*4 + block_nxz*4];
  type *rb_w = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  type *sb_e = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  type *rb_e = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];

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

#if 0
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
#endif

#endif

  return 0;
}

int sleev_comm_block3D_wait_cuda(mpi_prm *prm){
  MPI_Status stat_send[6],stat_recv[6];
  int i;
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

int sleev_comm_block3D_readBack_cuda(
			    type *f,mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    type *cuda_buf,
			    type *cpu_buf,
			    cudaStream_t *cudaStream
		       ){

  if(prm[0].npe==1){
    return 0;
  }

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

  type *cu_sb_b = &cuda_buf[0];
  type *cu_rb_b = &cuda_buf[block_nxy*1];
  type *cu_sb_t = &cuda_buf[block_nxy*2];
  type *cu_rb_t = &cuda_buf[block_nxy*3];

  type *cu_sb_s = &cuda_buf[block_nxy*4];
  type *cu_rb_s = &cuda_buf[block_nxy*4 + block_nxz*1];
  type *cu_sb_n = &cuda_buf[block_nxy*4 + block_nxz*2];
  type *cu_rb_n = &cuda_buf[block_nxy*4 + block_nxz*3];

  type *cu_sb_w = &cuda_buf[block_nxy*4 + block_nxz*4];
  type *cu_rb_w = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  type *cu_sb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  type *cu_rb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];

#if JUPITER_CUDA_AWARE_MPI


#else
  type *sb_b = &cpu_buf[0];
  type *rb_b = &cpu_buf[block_nxy*1];
  type *sb_t = &cpu_buf[block_nxy*2];
  type *rb_t = &cpu_buf[block_nxy*3];

  type *sb_s = &cpu_buf[block_nxy*4];
  type *rb_s = &cpu_buf[block_nxy*4 + block_nxz*1];
  type *sb_n = &cpu_buf[block_nxy*4 + block_nxz*2];
  type *rb_n = &cpu_buf[block_nxy*4 + block_nxz*3];

  type *sb_w = &cpu_buf[block_nxy*4 + block_nxz*4];
  type *rb_w = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  type *sb_e = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  type *rb_e = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];

  cudaMemcpy(cu_rb_b, rb_b, sizeof(type)*block_nxy, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_t, rb_t, sizeof(type)*block_nxy, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_s, rb_s, sizeof(type)*block_nxz, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_n, rb_n, sizeof(type)*block_nxz, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_w, rb_w, sizeof(type)*block_nyz, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_e, rb_e, sizeof(type)*block_nyz, cudaMemcpyHostToDevice);
#endif

  const int threads = 32;  
  const int blocks  = 1024;  
  // bottom (z-)
  // top    (z+)
  //  int jzb,jjzb; 
  readBackTopAndBottom_block3D_cuda_kernel<<<blocks,threads>>>(
							  cu_rb_b, cu_rb_t ,f,
							  block_nzs, block_nze,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);

  // south (y-)
  // north (y+)
  //  int jyb,jjyb; 
  readBackNorthAndSouth_block3D_cuda_kernel<<<blocks,threads>>>(
				       cu_rb_s, cu_rb_n ,f,
				       block_nys, block_nye,
				       nxblock, nyblock, nzblock,
				       mxdivblock, mydivblock, mzdivblock,
				       nxdivblock, nydivblock, nzdivblock);

  // west (x-)
  // east (x+)
  //  int jxb,jjxb; 
  readBackWestAndEast_block3D_cuda_kernel<<<blocks,threads>>>(
							 cu_rb_w, cu_rb_e ,f,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);

  return 0;
}
#endif

int sleev_comm_block3D_cuda(
			    type *f,mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    type *cuda_buf,
			    type *cpu_buf
		       ){

  if(prm[0].npe==1){
    return 0;
  }

#if 0

  sleev_comm_block3D_set_cuda(
			      f, prm,
			      block_nxs, block_nxe, 
			      block_nys, block_nye, 
			      block_nzs, block_nze,
			      cuda_buf,
			      cpu_buf
			      );

  sleev_comm_block3D_comm_cuda(
			       prm,
			       block_nxs, block_nxe, 
			       block_nys, block_nye, 
			       block_nzs, block_nze,
			       cuda_buf,
			       cpu_buf
			       );
  sleev_comm_block3D_readBack_cuda(
				   f, prm,
				   block_nxs, block_nxe, 
				   block_nys, block_nye, 
				   block_nzs, block_nze,
				   cuda_buf,
				   cpu_buf
				   );

#else


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

  type *cu_sb_b = &cuda_buf[0];
  type *cu_rb_b = &cuda_buf[block_nxy*1];
  type *cu_sb_t = &cuda_buf[block_nxy*2];
  type *cu_rb_t = &cuda_buf[block_nxy*3];

  type *cu_sb_s = &cuda_buf[block_nxy*4];
  type *cu_rb_s = &cuda_buf[block_nxy*4 + block_nxz*1];
  type *cu_sb_n = &cuda_buf[block_nxy*4 + block_nxz*2];
  type *cu_rb_n = &cuda_buf[block_nxy*4 + block_nxz*3];

  type *cu_sb_w = &cuda_buf[block_nxy*4 + block_nxz*4];
  type *cu_rb_w = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  type *cu_sb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  type *cu_rb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];

  MPI_Status stat_send[6],stat_recv[6];
  int i;

#if JUPITER_CUDA_AWARE_MPI

#else

#if 1
  type *sb_b = &cpu_buf[0];
  type *rb_b = &cpu_buf[block_nxy*1];
  type *sb_t = &cpu_buf[block_nxy*2];
  type *rb_t = &cpu_buf[block_nxy*3];

  type *sb_s = &cpu_buf[block_nxy*4];
  type *rb_s = &cpu_buf[block_nxy*4 + block_nxz*1];
  type *sb_n = &cpu_buf[block_nxy*4 + block_nxz*2];
  type *rb_n = &cpu_buf[block_nxy*4 + block_nxz*3];

  type *sb_w = &cpu_buf[block_nxy*4 + block_nxz*4];
  type *rb_w = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  type *sb_e = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  type *rb_e = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];
#else
  type sb_b[block_nxy],rb_b[block_nxy];
  type sb_t[block_nxy],rb_t[block_nxy];
  type sb_s[block_nxz],rb_s[block_nxz];
  type sb_n[block_nxz],rb_n[block_nxz];
  type sb_w[block_nyz],rb_w[block_nyz];
  type sb_e[block_nyz],rb_e[block_nyz];

  zero_initialize(block_nxy,sb_b);
  zero_initialize(block_nxy,rb_b);
  zero_initialize(block_nxy,sb_t);
  zero_initialize(block_nxy,rb_t);

  zero_initialize(block_nxz,sb_s);
  zero_initialize(block_nxz,rb_s);
  zero_initialize(block_nxz,sb_n);
  zero_initialize(block_nxz,rb_n);

  zero_initialize(block_nyz,sb_w);
  zero_initialize(block_nyz,rb_w);
  zero_initialize(block_nyz,sb_e);
  zero_initialize(block_nyz,rb_e);
#endif

#endif

  const int threads = 32;  
  const int blocks  = 1024;  

  // bottom (z-)
  // top    (z+)
  setTopAndBottom_block3D_cuda_kernel<<<blocks,threads>>>(
						     cu_sb_b, cu_sb_t ,f,
						     block_nzs, block_nze,
						     nxblock, nyblock, nzblock,
						     mxdivblock, mydivblock, mzdivblock,
						     nxdivblock, nydivblock, nzdivblock);
  // south (y-)
  // north (y+)
  setNorthAndSouth_block3D_cuda_kernel<<<blocks,threads>>>(
						      cu_sb_s, cu_sb_n ,f,
						      block_nys, block_nye,
						      nxblock, nyblock, nzblock,
						      mxdivblock, mydivblock, mzdivblock,
						      nxdivblock, nydivblock, nzdivblock);
  // west (x-)
  // east (x+)
  setWestAndEast_block3D_cuda_kernel<<<blocks,threads>>>(
						    cu_sb_w, cu_sb_e ,f,
						    block_nxs, block_nxe,
						    nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);
#if JUPITER_CUDA_AWARE_MPI

#else
  cudaMemcpy(sb_b, cu_sb_b, sizeof(type)*block_nxy, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_t, cu_sb_t, sizeof(type)*block_nxy, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_s, cu_sb_s, sizeof(type)*block_nxz, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_n, cu_sb_n, sizeof(type)*block_nxz, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_w, cu_sb_w, sizeof(type)*block_nyz, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_e, cu_sb_e, sizeof(type)*block_nyz, cudaMemcpyDeviceToHost);
#endif

  //  return 0;
  cudaDeviceSynchronize();

#if JUPITER_CUDA_AWARE_MPI
  // bottom (z-)
  if(prm->nrk[0] > -1) { 
    MPI_Isend(cu_sb_b, block_nxy, MPI_TYPE, prm->nrk[0], 1, prm->comm, &prm->req_send[0]);
    MPI_Irecv(cu_rb_b, block_nxy, MPI_TYPE, prm->nrk[0], 2, prm->comm, &prm->req_recv[0]);
  }
 // top    (z+)
  if(prm->nrk[1] > -1) {
    MPI_Isend(cu_sb_t, block_nxy, MPI_TYPE, prm->nrk[1], 2, prm->comm, &prm->req_send[1]);
    MPI_Irecv(cu_rb_t, block_nxy, MPI_TYPE, prm->nrk[1], 1, prm->comm, &prm->req_recv[1]);
  }

 // south (y-)
  if(prm->nrk[2] > -1) {
    MPI_Isend(cu_sb_s, block_nxz, MPI_TYPE, prm->nrk[2], 3, prm->comm, &prm->req_send[2]);
    MPI_Irecv(cu_rb_s, block_nxz, MPI_TYPE, prm->nrk[2], 4, prm->comm, &prm->req_recv[2]);
  }
  // north (y+)
  if(prm->nrk[3] > -1) {
    MPI_Isend(cu_sb_n, block_nxz, MPI_TYPE, prm->nrk[3], 4, prm->comm, &prm->req_send[3]);
    MPI_Irecv(cu_rb_n, block_nxz, MPI_TYPE, prm->nrk[3], 3, prm->comm, &prm->req_recv[3]);
  }

  // west (x-)
  if(prm->nrk[4] > -1) {
    MPI_Isend(cu_sb_w, block_nyz, MPI_TYPE, prm->nrk[4], 5, prm->comm, &prm->req_send[4]);
    MPI_Irecv(cu_rb_w, block_nyz, MPI_TYPE, prm->nrk[4], 6, prm->comm, &prm->req_recv[4]);
  }
  // east (x+)
  if(prm->nrk[5] > -1) {
    MPI_Isend(cu_sb_e, block_nyz, MPI_TYPE, prm->nrk[5], 6, prm->comm, &prm->req_send[5]);
    MPI_Irecv(cu_rb_e, block_nyz, MPI_TYPE, prm->nrk[5], 5, prm->comm, &prm->req_recv[5]);
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
#else
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
  //  printf(" prm->nrk[0] = %d \n",prm->nrk[0] );
  //  printf(" prm->nrk[1] = %d \n",prm->nrk[1] );
 //   printf(" prm->nrk[2] = %d \n",prm->nrk[2] );
 //   printf(" prm->nrk[3] = %d \n",prm->nrk[3] );
 //   printf(" prm->nrk[4] = %d \n",prm->nrk[4] );
 //   printf(" prm->nrk[5] = %d \n",prm->nrk[5] );
  cudaDeviceSynchronize();
  cudaMemcpy(cu_rb_b, rb_b, sizeof(type)*block_nxy, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_t, rb_t, sizeof(type)*block_nxy, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_s, rb_s, sizeof(type)*block_nxz, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_n, rb_n, sizeof(type)*block_nxz, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_w, rb_w, sizeof(type)*block_nyz, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_e, rb_e, sizeof(type)*block_nyz, cudaMemcpyHostToDevice);
#endif

  cudaDeviceSynchronize();

  // bottom (z-)
  // top    (z+)
  //  int jzb,jjzb; 
  readBackTopAndBottom_block3D_cuda_kernel<<<blocks,threads>>>(
							  cu_rb_b, cu_rb_t ,f,
							  block_nzs, block_nze,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);

  // south (y-)
  // north (y+)
  //  int jyb,jjyb; 
  readBackNorthAndSouth_block3D_cuda_kernel<<<blocks,threads>>>(
				       cu_rb_s, cu_rb_n ,f,
				       block_nys, block_nye,
				       nxblock, nyblock, nzblock,
				       mxdivblock, mydivblock, mzdivblock,
				       nxdivblock, nydivblock, nzdivblock);

  // west (x-)
  // east (x+)
  //  int jxb,jjxb; 
  readBackWestAndEast_block3D_cuda_kernel<<<blocks,threads>>>(
							 cu_rb_w, cu_rb_e ,f,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);

#endif

  return 0;

}

 int sleev_comm_block3D_cuda_time(
			    type *f,mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    type *cuda_buf,
			    type *cpu_buf,
			    type *time
		       ){

  if(prm[0].npe==1){
    return 0;
  }
  type ts,te;
#if 0

  sleev_comm_block3D_set_cuda(
			      f, prm,
			      block_nxs, block_nxe, 
			      block_nys, block_nye, 
			      block_nzs, block_nze,
			      cuda_buf,
			      cpu_buf
			      );

  sleev_comm_block3D_comm_cuda(
			       prm,
			       block_nxs, block_nxe, 
			       block_nys, block_nye, 
			       block_nzs, block_nze,
			       cuda_buf,
			       cpu_buf
			       );
  sleev_comm_block3D_readBack_cuda(
				   f, prm,
				   block_nxs, block_nxe, 
				   block_nys, block_nye, 
				   block_nzs, block_nze,
				   cuda_buf,
				   cpu_buf
				   );

#else


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

  type *cu_sb_b = &cuda_buf[0];
  type *cu_rb_b = &cuda_buf[block_nxy*1];
  type *cu_sb_t = &cuda_buf[block_nxy*2];
  type *cu_rb_t = &cuda_buf[block_nxy*3];

  type *cu_sb_s = &cuda_buf[block_nxy*4];
  type *cu_rb_s = &cuda_buf[block_nxy*4 + block_nxz*1];
  type *cu_sb_n = &cuda_buf[block_nxy*4 + block_nxz*2];
  type *cu_rb_n = &cuda_buf[block_nxy*4 + block_nxz*3];

  type *cu_sb_w = &cuda_buf[block_nxy*4 + block_nxz*4];
  type *cu_rb_w = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  type *cu_sb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  type *cu_rb_e = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];

  MPI_Status stat_send[6],stat_recv[6];
  int i;

#if JUPITER_CUDA_AWARE_MPI

#else

#if 1
  type *sb_b = &cpu_buf[0];
  type *rb_b = &cpu_buf[block_nxy*1];
  type *sb_t = &cpu_buf[block_nxy*2];
  type *rb_t = &cpu_buf[block_nxy*3];

  type *sb_s = &cpu_buf[block_nxy*4];
  type *rb_s = &cpu_buf[block_nxy*4 + block_nxz*1];
  type *sb_n = &cpu_buf[block_nxy*4 + block_nxz*2];
  type *rb_n = &cpu_buf[block_nxy*4 + block_nxz*3];

  type *sb_w = &cpu_buf[block_nxy*4 + block_nxz*4];
  type *rb_w = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  type *sb_e = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  type *rb_e = &cpu_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];
#else
  type sb_b[block_nxy],rb_b[block_nxy];
  type sb_t[block_nxy],rb_t[block_nxy];
  type sb_s[block_nxz],rb_s[block_nxz];
  type sb_n[block_nxz],rb_n[block_nxz];
  type sb_w[block_nyz],rb_w[block_nyz];
  type sb_e[block_nyz],rb_e[block_nyz];

  zero_initialize(block_nxy,sb_b);
  zero_initialize(block_nxy,rb_b);
  zero_initialize(block_nxy,sb_t);
  zero_initialize(block_nxy,rb_t);

  zero_initialize(block_nxz,sb_s);
  zero_initialize(block_nxz,rb_s);
  zero_initialize(block_nxz,sb_n);
  zero_initialize(block_nxz,rb_n);

  zero_initialize(block_nyz,sb_w);
  zero_initialize(block_nyz,rb_w);
  zero_initialize(block_nyz,sb_e);
  zero_initialize(block_nyz,rb_e);
#endif

#endif

  const int threads = 32;  
  const int blocks  = 1024;  

  // bottom (z-)
  // top    (z+)
  cudaDeviceSynchronize();
  ts=cpu_time();
  setTopAndBottom_block3D_cuda_kernel<<<blocks,threads>>>(
						     cu_sb_b, cu_sb_t ,f,
						     block_nzs, block_nze,
						     nxblock, nyblock, nzblock,
						     mxdivblock, mydivblock, mzdivblock,
						     nxdivblock, nydivblock, nzdivblock);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[0]=time[0]+te-ts;
  
  // south (y-)
  // north (y+)
  ts=cpu_time();
  setNorthAndSouth_block3D_cuda_kernel<<<blocks,threads>>>(
						      cu_sb_s, cu_sb_n ,f,
						      block_nys, block_nye,
						      nxblock, nyblock, nzblock,
						      mxdivblock, mydivblock, mzdivblock,
						      nxdivblock, nydivblock, nzdivblock);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[1]=time[1]+te-ts;
  // west (x-)
  // east (x+)
  ts=cpu_time();
  setWestAndEast_block3D_cuda_kernel<<<blocks,threads>>>(
						    cu_sb_w, cu_sb_e ,f,
						    block_nxs, block_nxe,
						    nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[2]=time[2]+te-ts;
  
  ts=cpu_time();
#if JUPITER_CUDA_AWARE_MPI

#else
  cudaMemcpy(sb_b, cu_sb_b, sizeof(type)*block_nxy, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_t, cu_sb_t, sizeof(type)*block_nxy, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_s, cu_sb_s, sizeof(type)*block_nxz, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_n, cu_sb_n, sizeof(type)*block_nxz, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_w, cu_sb_w, sizeof(type)*block_nyz, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_e, cu_sb_e, sizeof(type)*block_nyz, cudaMemcpyDeviceToHost);
#endif
  cudaDeviceSynchronize();
  te=cpu_time();
  time[3]=time[3]+te-ts;

  ts=cpu_time();
#if JUPITER_CUDA_AWARE_MPI
  // bottom (z-)
  if(prm->nrk[0] > -1) { 
    MPI_Isend(cu_sb_b, block_nxy, MPI_TYPE, prm->nrk[0], 1, prm->comm, &prm->req_send[0]);
    MPI_Irecv(cu_rb_b, block_nxy, MPI_TYPE, prm->nrk[0], 2, prm->comm, &prm->req_recv[0]);
  }
 // top    (z+)
  if(prm->nrk[1] > -1) {
    MPI_Isend(cu_sb_t, block_nxy, MPI_TYPE, prm->nrk[1], 2, prm->comm, &prm->req_send[1]);
    MPI_Irecv(cu_rb_t, block_nxy, MPI_TYPE, prm->nrk[1], 1, prm->comm, &prm->req_recv[1]);
  }

 // south (y-)
  if(prm->nrk[2] > -1) {
    MPI_Isend(cu_sb_s, block_nxz, MPI_TYPE, prm->nrk[2], 3, prm->comm, &prm->req_send[2]);
    MPI_Irecv(cu_rb_s, block_nxz, MPI_TYPE, prm->nrk[2], 4, prm->comm, &prm->req_recv[2]);
  }
  // north (y+)
  if(prm->nrk[3] > -1) {
    MPI_Isend(cu_sb_n, block_nxz, MPI_TYPE, prm->nrk[3], 4, prm->comm, &prm->req_send[3]);
    MPI_Irecv(cu_rb_n, block_nxz, MPI_TYPE, prm->nrk[3], 3, prm->comm, &prm->req_recv[3]);
  }

  // west (x-)
  if(prm->nrk[4] > -1) {
    MPI_Isend(cu_sb_w, block_nyz, MPI_TYPE, prm->nrk[4], 5, prm->comm, &prm->req_send[4]);
    MPI_Irecv(cu_rb_w, block_nyz, MPI_TYPE, prm->nrk[4], 6, prm->comm, &prm->req_recv[4]);
  }
  // east (x+)
  if(prm->nrk[5] > -1) {
    MPI_Isend(cu_sb_e, block_nyz, MPI_TYPE, prm->nrk[5], 6, prm->comm, &prm->req_send[5]);
    MPI_Irecv(cu_rb_e, block_nyz, MPI_TYPE, prm->nrk[5], 5, prm->comm, &prm->req_recv[5]);
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
#else
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
  //  printf(" prm->nrk[0] = %d \n",prm->nrk[0] );
  //  printf(" prm->nrk[1] = %d \n",prm->nrk[1] );
 //   printf(" prm->nrk[2] = %d \n",prm->nrk[2] );
 //   printf(" prm->nrk[3] = %d \n",prm->nrk[3] );
 //   printf(" prm->nrk[4] = %d \n",prm->nrk[4] );
 //   printf(" prm->nrk[5] = %d \n",prm->nrk[5] );
  cudaDeviceSynchronize();
  te=cpu_time();
  time[4]=time[4]+te-ts;
  
  ts=cpu_time();
  cudaMemcpy(cu_rb_b, rb_b, sizeof(type)*block_nxy, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_t, rb_t, sizeof(type)*block_nxy, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_s, rb_s, sizeof(type)*block_nxz, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_n, rb_n, sizeof(type)*block_nxz, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_w, rb_w, sizeof(type)*block_nyz, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_e, rb_e, sizeof(type)*block_nyz, cudaMemcpyHostToDevice);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[5]=time[5]+te-ts;
#endif

  cudaDeviceSynchronize();

  // bottom (z-)
  // top    (z+)
  //  int jzb,jjzb; 
  ts=cpu_time();
  readBackTopAndBottom_block3D_cuda_kernel<<<blocks,threads>>>(
							  cu_rb_b, cu_rb_t ,f,
							  block_nzs, block_nze,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);

  cudaDeviceSynchronize();
  te=cpu_time();
  time[6]=time[6]+te-ts;
  // south (y-)
  // north (y+)
  //  int jyb,jjyb; 
  ts=cpu_time();
  readBackNorthAndSouth_block3D_cuda_kernel<<<blocks,threads>>>(
				       cu_rb_s, cu_rb_n ,f,
				       block_nys, block_nye,
				       nxblock, nyblock, nzblock,
				       mxdivblock, mydivblock, mzdivblock,
				       nxdivblock, nydivblock, nzdivblock);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[7]=time[7]+te-ts;

  // west (x-)
  // east (x+)
  //  int jxb,jjxb; 
  ts=cpu_time();
  readBackWestAndEast_block3D_cuda_kernel<<<blocks,threads>>>(
							 cu_rb_w, cu_rb_e ,f,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);

  te=cpu_time();
  time[8]=time[8]+te-ts;
#endif

  return 0;

}


__global__ void setEast_block3D_cuda_kernel(
					       type *sb_e ,type *f,
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
__global__ void setWest_block3D_cuda_kernel(
					       type *sb_w ,type *f,
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

__global__ void setNorth_block3D_cuda_kernel(
					       type *sb_n ,type *f,
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

__global__ void setSouth_block3D_cuda_kernel(
					       type *sb_s, type *f,
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

__global__ void setTop_block3D_cuda_kernel(
					       type *sb_t ,type *f,
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

 __global__ void setBottom_block3D_cuda_kernel(
					       type *sb_b, type *f,
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

__global__ void readBackWest_block3D_cuda_kernel(
					       type *rb_w, type *f,
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

 
__global__ void readBackEast_block3D_cuda_kernel(
					       type *rb_e ,type *f,
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

__global__ void readBackNorth_block3D_cuda_kernel(
					       type *rb_n ,type *f,
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

 __global__ void readBackSouth_block3D_cuda_kernel(
					       type *rb_s, type *f,
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


 
__global__ void readBackTop_block3D_cuda_kernel(
					       type *rb_t, type *f,
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

__global__ void readBackBottom_block3D_cuda_kernel(
					       type *rb_b, type *f,
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
 
int sleev_comm_block3D_icommEast_cuda(
				  mpi_prm *prm,
				  type *sb_e,  type *rb_e
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
    MPI_Isend(sb_e, block_nyz, MPI_TYPE, prm->nrk[5], 6, prm->comm, &prm->req_send[5]);
    MPI_Irecv(rb_e, block_nyz, MPI_TYPE, prm->nrk[5], 5, prm->comm, &prm->req_recv[5]);
  }

  return 0;
}
  
int sleev_comm_block3D_icommWest_cuda(
				  mpi_prm *prm,
				  type *sb_w,  type *rb_w
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
    MPI_Isend(sb_w, block_nyz, MPI_TYPE, prm->nrk[4], 5, prm->comm, &prm->req_send[4]);
    MPI_Irecv(rb_w, block_nyz, MPI_TYPE, prm->nrk[4], 6, prm->comm, &prm->req_recv[4]);
  }

  return 0;
}

int sleev_comm_block3D_icommNorth_cuda(
				  mpi_prm *prm,
				  type *sb_n,  type *rb_n
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
    MPI_Isend(sb_n, block_nxz, MPI_TYPE, prm->nrk[3], 4, prm->comm, &prm->req_send[3]);
    MPI_Irecv(rb_n, block_nxz, MPI_TYPE, prm->nrk[3], 3, prm->comm, &prm->req_recv[3]);
  }

  return 0;
}

int sleev_comm_block3D_icommSouth_cuda(
				  mpi_prm *prm,
				  type *sb_s,  type *rb_s
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
    MPI_Isend(sb_s, block_nxz, MPI_TYPE, prm->nrk[2], 3, prm->comm, &prm->req_send[2]);
    MPI_Irecv(rb_s, block_nxz, MPI_TYPE, prm->nrk[2], 4, prm->comm, &prm->req_recv[2]);
  }

  return 0;
}

int sleev_comm_block3D_icommTop_cuda(
				  mpi_prm *prm,				  
				  type *sb_t,  type *rb_t
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
    MPI_Isend(sb_t, block_nxy, MPI_TYPE, prm->nrk[1], 2, prm->comm, &prm->req_send[1]);
    MPI_Irecv(rb_t, block_nxy, MPI_TYPE, prm->nrk[1], 1, prm->comm, &prm->req_recv[1]);
  }

  return 0;
}
 
int sleev_comm_block3D_icommBottom_cuda(
				  mpi_prm *prm,				  
				  type *sb_b,  type *rb_b
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
    MPI_Isend(sb_b, block_nxy, MPI_TYPE, prm->nrk[0], 1, prm->comm, &prm->req_send[0]);
    MPI_Irecv(rb_b, block_nxy, MPI_TYPE, prm->nrk[0], 2, prm->comm, &prm->req_recv[0]);
  }

  return 0;
} 

int sleev_comm_block3D_waitBottom_cuda(mpi_prm *prm){
  MPI_Status stat_send,stat_recv;
  if(prm->nrk[0] > -1) {
    MPI_Wait(&prm->req_send[0], &stat_send);
    MPI_Wait(&prm->req_recv[0], &stat_recv);
  }
  return 0;
}
int sleev_comm_block3D_waitTop_cuda(mpi_prm *prm){
  MPI_Status stat_send,stat_recv;
  if(prm->nrk[1] > -1) {
    MPI_Wait(&prm->req_send[1], &stat_send);
    MPI_Wait(&prm->req_recv[1], &stat_recv);
  }
  return 0;
}
int sleev_comm_block3D_waitSouth_cuda(mpi_prm *prm){
  MPI_Status stat_send,stat_recv;
  if(prm->nrk[2] > -1) {
    MPI_Wait(&prm->req_send[2], &stat_send);
    MPI_Wait(&prm->req_recv[2], &stat_recv);
  }
  return 0;
}
int sleev_comm_block3D_waitNorth_cuda(mpi_prm *prm){
  MPI_Status stat_send,stat_recv;
  if(prm->nrk[3] > -1) {
    MPI_Wait(&prm->req_send[3], &stat_send);
    MPI_Wait(&prm->req_recv[3], &stat_recv);
  }
  return 0;
}


int sleev_comm_block3D_waitWest_cuda(mpi_prm *prm){
  MPI_Status stat_send,stat_recv;
  if(prm->nrk[4] > -1) {
    MPI_Wait(&prm->req_send[4], &stat_send);
    MPI_Wait(&prm->req_recv[4], &stat_recv);
  }
  return 0;
}
int sleev_comm_block3D_waitEast_cuda(mpi_prm *prm){
  MPI_Status stat_send,stat_recv;
  if(prm->nrk[5] > -1) {
    MPI_Wait(&prm->req_send[5], &stat_send);
    MPI_Wait(&prm->req_recv[5], &stat_recv);
  }
  return 0;
}
