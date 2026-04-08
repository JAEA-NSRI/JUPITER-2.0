/*
 * communication_cuda.cu
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "func.h"
#include "struct.h"

#include "csvutil.h"

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#include "communication_cuda.h"


__global__ void unpack_z_cuda_kernel(type *rbuff, int ptr, type *f, int jzs, int mx, int my, int stcl)
{
  int j, jx, jy, jz;
  jx = blockDim.x * blockIdx.x + threadIdx.x;
  jy = blockDim.y * blockIdx.y + threadIdx.y;
  jz = threadIdx.z;
  if (jx >= mx || jy >= my || jz >= stcl) return;

  j = jx + mx * jy + mx * my * (jzs + jz);
  f[j] = rbuff[ptr + jx + mx * jy + mx * my * jz];
}

__global__ void unpack_y_cuda_kernel(type *rbuff, int ptr, type *f, int jys, int mx, int my, int mz, int stcl)
{
  int j, jx, jy, jz;  //my=stcl
  jx = blockDim.x * blockIdx.x + threadIdx.x;
  jy = stcl * blockIdx.y + threadIdx.y;
  jz = blockDim.z * blockIdx.z + threadIdx.z;
  if( jx>= mx || jy>= stcl || jz>=mz) return ;

  j = jx + mx*(jys + jy) + mx*my*jz;
  f[j] = rbuff[ptr + jx + mx*jy + mx*stcl*jz];
}

__global__ void unpack_x_cuda_kernel(type *rbuff, int ptr, type *f, int jxs, int mx, int my, int mz, int stcl)
{
  int j, jx, jy, jz;  //mx=stcl
  jx = threadIdx.x;
  jy = blockDim.y * blockIdx.y + threadIdx.y;
  jz = blockDim.z * blockIdx.z + threadIdx.z;
  if (jx >= stcl || jy >= my || jz >= mz) return;

  j = (jxs + jx) + mx * jy + mx * my * jz;
  f[j] = rbuff[ptr + jx + stcl * jy + stcl * my * jz];
}

__global__ void pack_x_cuda_kernel(type *sbuff, int ptr, type *f, int jxs, int mx, int my, int mz, int stcl)
{
  int j, jx, jy, jz;  //mx=stcl
  jx=threadIdx.x;
  jy=blockDim.y * blockIdx.y + threadIdx.y;
  jz=blockDim.z * blockIdx.z + threadIdx.z;
  if(jx >= stcl || jy >=my || jz >=mz) return ;

  j = (jxs + jx) + mx*jy + mx*my*jz;
sbuff[ptr + jx + stcl*jy + stcl*my*jz] = f[j];
}

__global__ void pack_y_cuda_kernel(type *sbuff, int ptr, type *f, int jys, int mx, int my,int mz, int stcl)
{
  int j, jx, jy, jz;  //my=stcl
  jx=blockDim.x * blockIdx.x + threadIdx.x;
  jy=threadIdx.y;
  jz=blockDim.z * blockIdx.z + threadIdx.z;
  if(jx >= mx || jy >= stcl || jz >=mz) return ;

  j = jx + mx*(jys + jy) + mx*my*jz;
  sbuff[ptr + jx + mx*jy + mx*stcl*jz] = f[j];
}

__global__ void pack_z_cuda_kernel(type *sbuff, int ptr, type *f, int jzs, int mx, int my,int stcl)
{
  int j, jx, jy, jz;
  jx = blockDim.x * blockIdx.x + threadIdx.x;
  jy = blockDim.y * blockIdx.y + threadIdx.y;
  jz = threadIdx.z;
  if (jx >= mx || jy >= my || jz >= stcl) return;

  j = jx + mx * jy + mx * my * (jzs + jz);
  sbuff[ptr + jx + mx * jy + mx * my * jz] = f[j];
}

#ifdef JUPITER_MPI
void pack_mpi_x_cuda(type *d_f, type *d_sbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, int *nrk)
{
  int jxs_w = stcl, jxs_e = nx;
  int block_y,block_z;

  block_y=my/blockDim_y;
  if(my%blockDim_y>0){
    block_y +=1;
  }

  block_z=mz/blockDim_z;
  if(mz%blockDim_z >0){
    block_z +=1;
  }
  dim3 Dg(1,block_y,block_z);
  dim3 Db(stcl, blockDim_y,blockDim_z);
  if (nrk[4] > -1) {
    pack_x_cuda_kernel<<<Dg,Db>>>(d_sbuff, ptr[4], d_f, jxs_w, mx, my, mz, stcl); // west (x-)
  }
  if (nrk[5] > -1) {
    pack_x_cuda_kernel<<<Dg,Db>>>(d_sbuff, ptr[5], d_f, jxs_e, mx, my, mz, stcl); // east (x+)
  }
}


void unpack_mpi_x_cuda(type *d_f, type *d_rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, int *nrk)
{
  int jxs_w = 0, jxs_e = nx+stcl;
  int block_y,block_z;

  block_y=my/blockDim_y;
  if(my%blockDim_y>0){
    block_y +=1;
  }
  block_z=mz/blockDim_z;
  if(mz%blockDim_z >0){
    block_z +=1;
  }
  dim3 Dg(1,block_y,block_z);
  dim3 Db(stcl, blockDim_y,blockDim_z);
  if(nrk[4] > -1){
	  unpack_x_cuda_kernel<<<Dg,Db>>>(d_rbuff, ptr[4], d_f, jxs_w, mx,my,mz, stcl); // west (x-)
  }
  if(nrk[5] > -1){
    unpack_x_cuda_kernel<<<Dg,Db>>>(d_rbuff, ptr[5], d_f, jxs_e, mx,my,mz, stcl); // east (x+)
  }
}

void pack_mpi_y_cuda(type *d_f, type *d_sbuff, int *ptr, int nx, int ny, int nz,int mx, int my, int mz, int stcl, int *nrk) {
  //my = stcl
  int jys_s = stcl, jys_n = ny;
  int block_x, block_z;

  block_x = mx / blockDim_x;
  if (mx % blockDim_x > 0) {
    block_x += 1;
  }
  block_z = mz / blockDim_z;
  if (mz % blockDim_z > 0) {
    block_z += 1;
  }
  dim3 Dg(block_x, 1, block_z);
  dim3 Db(blockDim_x, stcl, blockDim_z);

  if (nrk[2] > -1) {
    pack_y_cuda_kernel<<<Dg,Db>>>(d_sbuff, ptr[2], d_f, jys_s, mx, my, mz, stcl); // south (y-)
  }
  if (nrk[3] > -1) {
	  pack_y_cuda_kernel<<<Dg,Db>>>(d_sbuff, ptr[3], d_f, jys_n, mx, my, mz, stcl); // north (y+)
  }
}

void unpack_mpi_y_cuda(type *d_f, type *d_rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, int *nrk)
{
  int jys_s = 0, jys_n = ny + stcl;
  int block_x,block_z;

  block_x = mx/blockDim_x;
  if(mx%blockDim_x >0 ){
    block_x +=1;
  }
  block_z = mz/blockDim_z;
  if(mz%blockDim_z > 0){
    block_z +=1;
  }
  dim3 Dg(block_x,1,block_z);
  dim3 Db(blockDim_x,stcl,blockDim_z);
  if (nrk[2] > -1){
    unpack_y_cuda_kernel<<<Dg,Db>>>(d_rbuff, ptr[2], d_f, jys_s, mx, my, mz, stcl); // south (y-)
  }
  if (nrk[3] > -1){
    unpack_y_cuda_kernel<<<Dg,Db>>>(d_rbuff, ptr[3], d_f, jys_n, mx, my, mz, stcl); // north (y+)
  }
}

void pack_mpi_z_cuda(type *d_f, type *d_sbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, int *nrk)
{
  int jzs_b = stcl, jzs_t = nz;
  int block_x,block_y;

  block_x = mx / blockDim_x;
  if(mx%blockDim_x >0 ){
    block_x +=1;
  }
  block_y = my/blockDim_y;
  if(my%blockDim_y > 0){
	  block_y +=1;
  }

  dim3 Dg(block_x,block_y,1);
  dim3 Db(blockDim_x,blockDim_y,stcl);
  if(nrk[0] > -1) {
	  pack_z_cuda_kernel<<<Dg,Db>>>(d_sbuff, ptr[0], d_f, jzs_b, mx,my, stcl); // bottom (z-)
  }
  if(nrk[1] > -1) {
	  pack_z_cuda_kernel<<<Dg,Db>>>(d_sbuff, ptr[1], d_f, jzs_t, mx,my, stcl); // top    (z+)
  }
}


void unpack_mpi_z_cuda(type *f, type *rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, int *nrk)
{
  int jzs_b = 0, jzs_t = nz+stcl;
  int block_x,block_y;

  block_x = mx / blockDim_x;
  if (mx % blockDim_x > 0) {
    block_x += 1;
  }
  block_y = my / blockDim_y;
  if (my % blockDim_y > 0) {
    block_y += 1;
  }

  dim3 Dg(block_x,block_y,1);
  dim3 Db(blockDim_x,blockDim_y,stcl);
  if(nrk[0] > -1) {
    unpack_z_cuda_kernel<<<Dg,Db>>>(rbuff, ptr[0], f, jzs_b, mx,my, stcl); // bottom (z-)
  }
  if(nrk[1] > -1) {
    unpack_z_cuda_kernel<<<Dg,Db>>>(rbuff, ptr[1], f, jzs_t, mx,my, stcl); // top    (z+)
  }
}


// ==================================================
//    MPI iSend & iRecv
// --------------------------------------------------
void mpi_isend_irecv_cuda(type *sbuff, type *rbuff, int *ptr, int mx, int my, int mz, int stcl,
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

void mpi_isend_irecv_x_cuda(type *sbuff, type *rbuff, int *ptr, int mx, int my, int mz, int stcl,
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

void mpi_isend_irecv_y_cuda(type *sbuff, type *rbuff, int *ptr, int mx, int my, int mz, int stcl,
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

void mpi_isend_irecv_z_cuda(type *sbuff, type *rbuff, int *ptr, int mx, int my, int mz, int stcl,
               MPI_Request *req_send, MPI_Request *req_recv, mpi_param *mpi)
{
  // z-direction
  if(mpi->nrk[0] > -1) {
  	//printf("%d:z:nrk[0]=%d ptr[0]=%d\n",mpi->rank,mpi->nrk[0],ptr[0]);
    MPI_Isend(&sbuff[ptr[0]], mx*my*stcl, MPI_TYPE, mpi->nrk[0], mpi->nrk[0], mpi->CommJUPITER, &req_send[0]);
    MPI_Irecv(&rbuff[ptr[0]], mx*my*stcl, MPI_TYPE, mpi->nrk[0], mpi->rank,   mpi->CommJUPITER, &req_recv[0]);
  }
  if(mpi->nrk[1] > -1) {
  	//printf("%d:z:nrk[1]=%d ptr[1]=%d \n",mpi->rank,mpi->nrk[1],ptr[1]);
    MPI_Isend(&sbuff[ptr[1]], mx*my*stcl, MPI_TYPE, mpi->nrk[1], mpi->nrk[1], mpi->CommJUPITER, &req_send[1]);
    MPI_Irecv(&rbuff[ptr[1]], mx*my*stcl, MPI_TYPE, mpi->nrk[1], mpi->rank,   mpi->CommJUPITER, &req_recv[1]);
  }
}

// ==================================================
//     Calc Pointer
// --------------------------------------------------
int calc_ptr_gpu(int *ptr, int mx, int my, int mz, int stcl, mpi_param *mpi)
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



void communication_cuda_buff(type *f, parameter *prm, comm_buf_cuda *comm_buf_cu){


#ifdef JUPITER_MPI
  domain    *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;
  MPI_Request  req_send[6], req_recv[6];
  MPI_Status   stat_send[6],stat_recv[6];

  //MPI_Barrier(mpi->CommJUPITER);

  int  ptr[7], i, nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
    stcl=cdo->stm, mx=cdo->mx, my=cdo->my, mz=cdo->mz;

  size_t size =  comm_buf_cu->buf_size;
  type *d_sbuff;
  type *d_rbuff;
  type *h_sbuff;
  type *h_rbuff;

  d_sbuff = comm_buf_cu->d_sbuff;
  d_rbuff = comm_buf_cu->d_rbuff;
  h_sbuff = comm_buf_cu->h_sbuff;
  h_rbuff = comm_buf_cu->h_rbuff;


  // calculate initial pointer
  calc_ptr_gpu(ptr, mx,my,mz, stcl, mpi);

  //f is on GPU

  // z-direction
  if(mpi->nrk[0]>-1 || mpi->nrk[1]>-1) {
		pack_mpi_z_cuda(f, d_sbuff, ptr, nx,ny,nz, mx,my,mz, stcl, mpi->nrk);
		cudaDeviceSynchronize();

	#if JUPITER_CUDA_AWARE_MPI
		mpi_isend_irecv_z_cuda(d_sbuff,d_rbuff,ptr, mx,my,mz,stcl, req_send,req_recv,mpi);
	#else
		//not GPU DIRECT d_sbuff-> sbuff
		//z -direction
		cudaMemcpy(h_sbuff,d_sbuff,size,cudaMemcpyDeviceToHost);
		// comm sbuff -> rbuff
		mpi_isend_irecv_z_cuda(h_sbuff,h_rbuff,ptr, mx,my,mz,stcl, req_send,req_recv,mpi);

  #endif

    for(i = 0; i < 2; i++) {
      if(mpi->nrk[i] > -1) {
        MPI_Wait(&req_send[i], &stat_send[i]);
        MPI_Wait(&req_recv[i], &stat_recv[i]);
      }
    }


	#if JUPITER_CUDA_AWARE_MPI

  #else
    // rbuff->d_rbuff
      cudaMemcpy(d_rbuff,h_rbuff,size,cudaMemcpyHostToDevice);
    //cudaDeviceSynchronize();
  #endif
    //comm_z_cuda_time +=  cpu_time() - comm_z_cuda_time0;
    //unpack d_rbuff on device

    unpack_mpi_z_cuda(f,d_rbuff,ptr,nx,ny,nz,mx,my,mz,stcl,mpi->nrk);
    cudaDeviceSynchronize();

  }//end z-direction


  // y-direction
  if(mpi->nrk[2]>-1 || mpi->nrk[3]>-1) {

    pack_mpi_y_cuda(f, d_sbuff, ptr, nx,ny,nz, mx,my,mz, stcl, mpi->nrk);
    cudaDeviceSynchronize();

  #if JUPITER_CUDA_AWARE_MPI
    mpi_isend_irecv_y_cuda(d_sbuff,d_rbuff,ptr, mx,my,mz,stcl, req_send,req_recv,mpi);
  #else
    // not GPUDirect  f is on GPU
    // d_sbuff -> sbuff
    cudaMemcpy(h_sbuff,d_sbuff,size,cudaMemcpyDeviceToHost);
    // comm sbuff -> rbuff
    mpi_isend_irecv_y_cuda(h_sbuff,h_rbuff,ptr, mx,my,mz,stcl, req_send,req_recv,mpi);

  #endif
    for(i = 2; i < 4; i++) {
      if(mpi->nrk[i] > -1) {
        MPI_Wait(&req_send[i], &stat_send[i]);
        MPI_Wait(&req_recv[i], &stat_recv[i]);
      }
    }

  #if JUPITER_CUDA_AWARE_MPI

  #else
    // rbuff->d_rbuff
    cudaMemcpy(d_rbuff,h_rbuff,size,cudaMemcpyHostToDevice);

  #endif
  //unpack y
    unpack_mpi_y_cuda(f, d_rbuff, ptr, nx,ny,nz, mx,my,mz, stcl, mpi->nrk);
    cudaDeviceSynchronize();
  }// end y-direction

  // x-direction
  if (mpi->nrk[4]>-1 || mpi->nrk[5]>-1) {

    pack_mpi_x_cuda(f, d_sbuff, ptr, nx,ny,nz, mx,my,mz, stcl, mpi->nrk);
    cudaDeviceSynchronize();

  #if JUPITER_CUDA_AWARE_MPI
    mpi_isend_irecv_x_cuda(d_sbuff,d_rbuff,ptr, mx,my,mz,stcl, req_send,req_recv,mpi);

  #else
    // not GPUDirect  f is on GPU
    // d_sbuff -> sbuff
    cudaMemcpy(h_sbuff,d_sbuff,size,cudaMemcpyDeviceToHost);
    // comm sbuff -> rbuff
    mpi_isend_irecv_x_cuda(h_sbuff,h_rbuff,ptr, mx,my,mz,stcl, req_send,req_recv,mpi);

  #endif
    for(i = 4; i < 6; i++) {
      if(mpi->nrk[i] > -1) {
        MPI_Wait(&req_send[i], &stat_send[i]);
        MPI_Wait(&req_recv[i], &stat_recv[i]);
      }
    }

  #if JUPITER_CUDA_AWARE_MPI

  #else
    cudaMemcpy(d_rbuff,h_rbuff,size,cudaMemcpyHostToDevice);
  #endif

    //unpack x
    unpack_mpi_x_cuda(f, d_rbuff, ptr, nx,ny,nz, mx,my,mz, stcl, mpi->nrk);
    cudaDeviceSynchronize();
  } // end x - direction

#endif /* JUPITER_MPI */
}
