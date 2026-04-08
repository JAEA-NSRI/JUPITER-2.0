#include <stdio.h>
#include <cuda_runtime.h>
//#include <cuda_bf16.h>
//#include <cuda_fp16.h>
#include "cublas_v2.h"

#include "struct.h"
#include "cg.h"
#include "sleev_comm_cuda.h"
#include "pcg_cuda.h"

__global__ void initializ_sleev_kernel_Z_fixed(int ny, int nx, int nz,
                                    int stm,
                                    int mx, int mxy,
                                    type* __restrict__ vec){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = ny*nx;
  //if(tid >= domain) return;

  for(int k=tid;k<domain;k+=(blockDim.x*gridDim.x))
  {
      int jz = -1;
      const int j1 =       ((k % (ny*nx))%nx)+stm; 
      const int j2 =   mx*(((k % (ny*nx))/nx)+stm);  
      int       j  = j1 + j2 +  mxy*(jz + stm);
      vec[j]=0.0;

      jz=nz;
      j            = j1 + j2 +  mxy*(jz + stm);
      vec[j]=0.0;

  }
}

__global__ void initializ_sleev_kernel_Y_fixed(int ny, int nx, int nz,
                                    int stm,
                                    int mx, int mxy,
                                    type* __restrict__ vec){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nz*nx;
  //if(tid >= domain) return;

  for(int k=tid;k<domain;k+=(blockDim.x*gridDim.x))
  {
       int jy = -1;
       const int j1 =      ((k % (nz*nx))% nx)+stm;
       const int j2 =  mxy*((k % (nz*nx) / nx)+stm);
       int j  = j1 + mx*(jy+stm) + j2;

       vec[j]=0.0;
       jy = ny;
       j  = j1 + mx*(jy+stm) + j2;

       vec[j]=0.0;
  }
}

__global__ void initializ_sleev_kernel_X_fixed(int ny, int nx, int nz,
                                    int stm,
                                    int mx, int mxy,
                                    type* __restrict__ vec){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nz*ny;
  //if(tid >= domain) return;

  for(int k=tid;k<domain;k+=(blockDim.x*gridDim.x))
  {
       int jx = -1;
       int j1 =  mx*(((k % (ny*nz)) % ny)+stm);
       int j2 =  mxy*((k % (ny*nz)  / ny)+stm);
       int j = (jx+stm) + j1 + j2;
       vec[j]=0.0;

       jx = nx;
       j = (jx+stm) + j1 + j2;
       vec[j]=0.0;
  }
}

void initializ_sleev_cuda(type *vec,
                         int ny, int nx, int nz,
                         int stm,
                         int mx, int mxy){

    const int threads_n = 1024;
    int blocks_n  = ceil((double)(nx*ny)/threads_n);

    initializ_sleev_kernel_Z_fixed<<<blocks_n, threads_n>>> (ny, nx, nz,
                                                             stm,
                                                             mx, mxy,
                                                             vec);
    blocks_n  = ceil((double)(nx*nz)/threads_n);
    initializ_sleev_kernel_Y_fixed<<<blocks_n, threads_n>>> (ny, nx, nz,
                                                             stm,
                                                             mx, mxy,
                                                             vec);
    blocks_n  = ceil((double)(ny*nz)/threads_n);
    initializ_sleev_kernel_X_fixed<<<blocks_n, threads_n>>> (ny, nx, nz,
                                                             stm,
                                                             mx, mxy,
                                                             vec);

}
                         
__global__ void setTopAndBottom_kernel(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            type* __restrict__ sb_b, 
                            type* __restrict__ sb_t,
                            const type* const __restrict__ f)
{
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nl*ny*nx;
  //if(tid >= domain) return;

  const int l = 0;
  for(int k=tid;k<domain;k+=(blockDim.x*gridDim.x))
  {
	int jz = 0 + l;
        const int j1 =     ((k % (ny*nx))%nx)+stm;
        const int j2 = mx*(((k % (ny*nx))/nx)+stm);
	int j  = j1 + j2 +  mxy*(jz + stm);

	sb_b[k] = f[j];                     //j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);

	jz = nz - 1 + l;
	j      = j1 + j2 +  mxy*(jz + stm);
	sb_t[k] = f[j];                     //j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);
   }
}

__global__ void setNorthAndSouth_kernel(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            type* __restrict__ sb_s, 
                            type* __restrict__ sb_n,
                            const type* const __restrict__ f){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nl*nz*nx;
  //if(tid >= domain) return;

  const int l = 0;
  for(int k=tid;k<domain;k+=(blockDim.x*gridDim.x))
  {
	int jy = 0 + l;
        const int j1 =     ((k % (nz*nx))%nx)+stm ;
        const int j2 = mxy*((k % (nz*nx) /nx)+stm);
        int j  = j1 +  mx*(jy+stm) + j2;

	sb_s[k] = f[j];               //j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);

	jy = ny - 1 - l;
        j  = j1 +  mx*(jy+stm) + j2;
	sb_n[k] = f[j];               //j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);
   }
}


__global__ void setWestAndEast_kernel(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            type* __restrict__ sb_w, 
                            type* __restrict__ sb_e,
                            const type* const __restrict__ f){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nl*ny*nz;
  //if(tid >= domain) return;

  const int l = 0;
  for(int k=tid;k<domain;k+=(blockDim.x*gridDim.x))
  {
        const int j1 = mx*(((k % (ny*nz))%ny)+stm);
        const int j2 = mxy*((k % (ny*nz) /ny)+stm);
        const int j  = stm + j1 + j2;

	int jx = 0 + l;
	sb_w[k] = f[jx + j]; //j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);

	jx = nx - 1 - l;
	sb_e[k] = f[jx + j]; //j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);
   }
}



// ReadBack Kernels
__global__ void readBackTopAndBottom_kernel(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            const type* const __restrict__ rb_b, 
                            const type* const __restrict__ rb_t,
                            type*  __restrict__ f){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nl*ny*nx;
  //if(tid >= domain) return;

  const int l = 0;
  for(int k=tid;k<domain;k+=(blockDim.x*gridDim.x))
  {
	int jz = -1 - l;
        int j1 =      (((k % (ny*nx)))% nx)+stm;
        int j2 =   mx*(((k % (ny*nx)) / nx)+stm);
        int j  = j1 + j2;
        //printf("%i %i %i \n",j+mxy*(jz+stm), j1, j2);
        //printf("%i %i %i \n",k, ny*nx, nx);

	f[j+mxy*(jz+stm)] = rb_b[k];
	jz = nz + l;
	f[j+mxy*(jz+stm)] = rb_t[k];
   }
}

__global__ void readBackNorthAndSouth_kernel(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            const type* const  __restrict__ rb_s, 
                            const type* const  __restrict__ rb_n,
                            type* __restrict__ f)
{
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nl*nz*nx;
  //if(tid >= domain) return;

  //for (l = 0, i=0; l < nl; l++) {
  //	for (jz = 0; jz < nz; jz++) {
  //		for (jx = 0; jx < nx; jx++) {
  //			jy = -1 - l;
  //			j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);
  //			tmp_vec[j] = rb_s[i];
  //
  //			jy = ny + l;
  //			j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);
  //			tmp_vec[j] = rb_n[i];
  //			i = i + 1;
  //		}
  //	}
  //}
  const int l = 0;
  for(int k=tid;k<domain;k+=(blockDim.x*gridDim.x))
  {
	int jy = -1 - l;
        const int j1 =     ((k % (nz*nx))%nx)+stm;
        const int j2 = mxy*((k % (nz*nx) /nx)+stm);
        int j  = j1 + mx*(jy+stm) + j2;

	f[j] = rb_s[k];                 //j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);

	jy   = ny + l;
        j    = j1 + mx*(jy+stm) + j2;
	f[j] = rb_n[k];                 //j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);
   }
}

__global__ void readBackWestAndEast_kernel(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            const type* const  __restrict__ rb_w, 
                            const type* const  __restrict__ rb_e,
                            type* __restrict__ f)
{
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nl*ny*nz;
  //if(tid >= domain) return;

  const int l = 0;
  for(int k=tid;k<domain;k+=(blockDim.x*gridDim.x))
  {
	int jx = -1 - l + stm;
        const int j1 =  mx*(((k % (ny*nz))%ny)+stm);
        const int j2 =  mxy*((k % (ny*nz) /ny)+stm);
        const int j  = j1 +j2;
        //printf("%i %i %i \n",j+jx, j1, j2);

	f[j+jx] = rb_w[k];

	jx = nx + l + stm;
	f[j+jx] = rb_e[k];
   }
}

__global__ void setTopAndBottom_kernel_BF16(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            low_type* __restrict__ sb_b, 
                            low_type* __restrict__ sb_t,
                            const low_type* const __restrict__ f){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nl*ny*nx;
  //if(tid >= domain) return;

  const int l = 0;
  for(int k=tid;k<domain;k+=(blockDim.x*gridDim.x))
  {
	int jz = 0 + l;
        const int j1 =     ((k % (ny*nx))%nx)+stm;
        const int j2 = mx*(((k % (ny*nx))/nx)+stm);
	int j  = j1 + j2 +  mxy*(jz + stm);

	sb_b[k] = f[j];                     //j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);

	jz = nz - 1 + l;
	j      = j1 + j2 +  mxy*(jz + stm);
	sb_t[k] = f[j];                     //j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);
   }
}

__global__ void setNorthAndSouth_kernel_BF16(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            low_type* __restrict__ sb_s, 
                            low_type* __restrict__ sb_n,
                            const low_type* const __restrict__ f){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nl*nz*nx;
  //if(tid >= domain) return;

  const int l = 0;
  for(int k=tid;k<domain;k+=(blockDim.x*gridDim.x))
  {
	int jy = 0 + l;
        const int j1 =     ((k % (nz*nx))%nx)+stm ;
        const int j2 = mxy*((k % (nz*nx) /nx)+stm);
        int j  = j1 +  mx*(jy+stm) + j2;

	sb_s[k] = f[j];               //j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);

	jy = ny - 1 - l;
        j  = j1 +  mx*(jy+stm) + j2;
	sb_n[k] = f[j];               //j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);
   }
}


__global__ void setWestAndEast_kernel_BF16(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            low_type* __restrict__ sb_w, 
                            low_type* __restrict__ sb_e,
                            const low_type* const __restrict__ f){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nl*ny*nz;
  //if(tid >= domain) return;

  const int l = 0;
  for(int k=tid;k<domain;k+=(blockDim.x*gridDim.x))
  {
        const int j1 = mx*(((k % (ny*nz))%ny)+stm);
        const int j2 = mxy*((k % (ny*nz) /ny)+stm);
        const int j  = stm + j1 + j2;

	int jx = 0 + l;
	sb_w[k] = f[jx + j]; //j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);

	jx = nx - 1 - l;
	sb_e[k] = f[jx + j]; //j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);
   }
}



// ReadBack Kernels
__global__ void readBackTopAndBottom_kernel_BF16(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            const low_type* const __restrict__ rb_b, 
                            const low_type* const __restrict__ rb_t,
                            low_type*  __restrict__ f){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nl*ny*nx;
  //if(tid >= domain) return;

  const int l = 0;
  for(int k=tid;k<domain;k+=(blockDim.x*gridDim.x))
  {
	int jz = -1 - l;
        int j1 =      (((k % (ny*nx)))% nx)+stm;
        int j2 =   mx*(((k % (ny*nx)) / nx)+stm);
        int j  = j1 + j2;
        //printf("%i %i %i \n",j+mxy*(jz+stm), j1, j2);
        //printf("%i %i %i \n",k, ny*nx, nx);

	f[j+mxy*(jz+stm)] = rb_b[k];
	jz = nz + l;
	f[j+mxy*(jz+stm)] = rb_t[k];
   }
}

__global__ void readBackNorthAndSouth_kernel_BF16(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            const low_type* const  __restrict__ rb_s, 
                            const low_type* const  __restrict__ rb_n,
                            low_type* __restrict__ f){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nl*nz*nx;
  //if(tid >= domain) return;

  //for (l = 0, i=0; l < nl; l++) {
  //	for (jz = 0; jz < nz; jz++) {
  //		for (jx = 0; jx < nx; jx++) {
  //			jy = -1 - l;
  //			j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);
  //			tmp_vec[j] = rb_s[i];
  //
  //			jy = ny + l;
  //			j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);
  //			tmp_vec[j] = rb_n[i];
  //			i = i + 1;
  //		}
  //	}
  //}
  const int l = 0;
  for(int k=tid;k<domain;k+=(blockDim.x*gridDim.x))
  {
	int jy = -1 - l;
        const int j1 =     ((k % (nz*nx))%nx)+stm;
        const int j2 = mxy*((k % (nz*nx) /nx)+stm);
        int j  = j1 + mx*(jy+stm) + j2;

	f[j] = rb_s[k];                 //j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);

	jy   = ny + l;
        j    = j1 + mx*(jy+stm) + j2;
	f[j] = rb_n[k];                 //j = (jx + stmx) + mx*(jy + stm) + mxy*(jz + stm);
   }
}

__global__ void readBackWestAndEast_kernel_BF16(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            const low_type* const  __restrict__ rb_w, 
                            const low_type* const  __restrict__ rb_e,
                             low_type* __restrict__ f){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nl*ny*nz;
  //if(tid >= domain) return;

  const int l = 0;
  for(int k=tid;k<domain;k+=(blockDim.x*gridDim.x))
  {
	int jx = -1 - l + stm;
        const int j1 =  mx*(((k % (ny*nz))%ny)+stm);
        const int j2 =  mxy*((k % (ny*nz) /ny)+stm);
        const int j  = j1 +j2;
        //printf("%i %i %i \n",j+jx, j1, j2);

	f[j+jx] = rb_w[k];

	jx = nx + l + stm;
	f[j+jx] = rb_e[k];
   }
}

int sleev_comm_cuda(type *f, mpi_prm *prm, type *cuda_buf){
  // まずはcpu側に渡して通信する

  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm[0], &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

#if 0
  const size_t VecSize = sizeof(type)*m;
  type *vec_cpu = (type*)malloc(VecSize);
  
  cudaMemcpy(vec_cpu,f,VecSize,cudaMemcpyDeviceToHost);
  cudaDeviceSynchronize();
  sleev_comm(vec_cpu,prm); 
  cudaMemcpy(f,vec_cpu,VecSize,cudaMemcpyHostToDevice);

  return 0;
#endif

  const int nl = 1; //袖の長さ
  const int nxy= nx*ny*nl;
  const int nxz= nx*nz*nl;
  const int nyz= ny*nz*nl;

  // ここのポインタ付け替えが怪しい
  type *cu_sb_b = &cuda_buf[0];
  type *cu_sb_t = &cuda_buf[nxy];
  type *cu_sb_s = &cuda_buf[nxy*2];
  type *cu_sb_n = &cuda_buf[nxy*2+nxz];
  type *cu_sb_w = &cuda_buf[nxy*2+nxz*2];
  type *cu_sb_e = &cuda_buf[nxy*2+nxz*2+nyz];

  int sbufSize = nxy*2+nxz*2+nyz*2;
  type *cu_rb_b = &cuda_buf[sbufSize + 0];
  type *cu_rb_t = &cuda_buf[sbufSize + nxy];
  type *cu_rb_s = &cuda_buf[sbufSize + nxy*2];
  type *cu_rb_n = &cuda_buf[sbufSize + nxy*2+nxz];
  type *cu_rb_w = &cuda_buf[sbufSize + nxy*2+nxz*2];
  type *cu_rb_e = &cuda_buf[sbufSize + nxy*2+nxz*2+nyz];
  
  const int threads = 32;  
  const int blocks  = 1024;  
  
  setTopAndBottom_kernel<<<blocks,threads>>>(nl, 
						ny, nx, nz,
						stm,
						mx, mxy,
						cu_sb_b, 
						cu_sb_t,
						f);
  setNorthAndSouth_kernel<<<blocks,threads>>>(nl, ny, nx, nz,
						 stm,
						 mx, mxy,
						 cu_sb_s, 
						 cu_sb_n,
					       f);
  setWestAndEast_kernel<<<blocks,threads>>>(nl, ny, nx, nz,
					       stm,
					       mx, mxy,
					       cu_sb_w, 
					       cu_sb_e,
					       f);
  cudaDeviceSynchronize();
  MPI_Request reqs[12];
  int counter = 0;

#if 0
  // CUDA-Aware MPI用コード
  // bottom (z-)
  if (prm->nrk[0] > -1){
    //printf("Wait bottom.\n");
    int err1 = MPI_Irecv(cu_rb_b, nxy, MPI_TYPE, prm->nrk[0], prm->rank, prm->comm,  &reqs[0]);
    int err2 = MPI_Isend(cu_sb_b, nxy, MPI_TYPE, prm->nrk[0], prm->nrk[0], prm->comm,&reqs[1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }
  // top    (z+)
  if (prm->nrk[1] > -1){
    int err1 = MPI_Irecv(cu_rb_t, nxy, MPI_TYPE, prm->nrk[1], prm->rank,   prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(cu_sb_t, nxy, MPI_TYPE, prm->nrk[1], prm->nrk[1], prm->comm, &reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }

  // south (y-)
  if (prm->nrk[2] > -1){
    int err1 = MPI_Irecv(cu_rb_s, nxz, MPI_TYPE, prm->nrk[2], prm->rank,   prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(cu_sb_s, nxz, MPI_TYPE, prm->nrk[2], prm->nrk[2], prm->comm,&reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }
  // north (y+)
  if (prm->nrk[3] > -1){
    int err1 = MPI_Irecv(cu_rb_n, nxz, MPI_TYPE, prm->nrk[3], prm->rank,  prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(cu_sb_n, nxz, MPI_TYPE, prm->nrk[3], prm->nrk[3],prm->comm,&reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }

  // west (x-)
  if (prm->nrk[4] > -1){
    int err1 = MPI_Irecv(cu_rb_w, nyz, MPI_TYPE, prm->nrk[4], prm->rank, prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(cu_sb_w, nyz, MPI_TYPE, prm->nrk[4], prm->nrk[4], prm->comm,&reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }

  // east (x+)
  if (prm->nrk[5] > -1){
    int err1 = MPI_Irecv(cu_rb_e, nyz, MPI_TYPE, prm->nrk[5], prm->rank, prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(cu_sb_e, nyz, MPI_TYPE, prm->nrk[5], prm->nrk[5], prm->comm,&reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }
  MPI_Waitall(counter, reqs, MPI_STATUSES_IGNORE);

#else
  
  type *sb_b = (type*)malloc(sizeof(type)*nxy);
  type *sb_t = (type*)malloc(sizeof(type)*nxy);
  type *sb_s = (type*)malloc(sizeof(type)*nxz);
  type *sb_n = (type*)malloc(sizeof(type)*nxz);
  type *sb_w = (type*)malloc(sizeof(type)*nyz);
  type *sb_e = (type*)malloc(sizeof(type)*nyz);
  
  type *rb_b = (type*)malloc(sizeof(type)*nxy);
  type *rb_t = (type*)malloc(sizeof(type)*nxy);
  type *rb_s = (type*)malloc(sizeof(type)*nxz);
  type *rb_n = (type*)malloc(sizeof(type)*nxz);
  type *rb_w = (type*)malloc(sizeof(type)*nyz);
  type *rb_e = (type*)malloc(sizeof(type)*nyz);

// -----
  int j;
  for(j=0;j<nxy;j++){
    sb_b[j] = 0.0;
    sb_t[j] = 0.0;
    rb_b[j] = 0.0;
    rb_t[j] = 0.0;
  }
  for(j=0;j<nxz;j++){
    sb_s[j] = 0.0;
    sb_n[j] = 0.0;
    rb_s[j] = 0.0;
    rb_n[j] = 0.0;
  }
  for(j=0;j<nyz;j++){
    sb_w[j] = 0.0;
    sb_e[j] = 0.0;
    rb_w[j] = 0.0;
    rb_e[j] = 0.0;
  }
  // -----



  cudaMemcpy(sb_b, cu_sb_b, sizeof(type)*nxy, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_t, cu_sb_t, sizeof(type)*nxy, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_s, cu_sb_s, sizeof(type)*nxz, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_n, cu_sb_n, sizeof(type)*nxz, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_w, cu_sb_w, sizeof(type)*nyz, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_e, cu_sb_e, sizeof(type)*nyz, cudaMemcpyDeviceToHost);
  
  // bottom (z-)
  if (prm->nrk[0] > -1){
    //printf("Wait bottom.\n");
    int err1 = MPI_Irecv(rb_b, nxy, MPI_TYPE, prm->nrk[0], prm->rank, prm->comm,  &reqs[0]);
    int err2 = MPI_Isend(sb_b, nxy, MPI_TYPE, prm->nrk[0], prm->nrk[0], prm->comm,&reqs[1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }
  // top    (z+)
  if (prm->nrk[1] > -1){
    int err1 = MPI_Irecv(rb_t, nxy, MPI_TYPE, prm->nrk[1], prm->rank,   prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(sb_t, nxy, MPI_TYPE, prm->nrk[1], prm->nrk[1], prm->comm, &reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }

  // south (y-)
  if (prm->nrk[2] > -1){
    int err1 = MPI_Irecv(rb_s, nxz, MPI_TYPE, prm->nrk[2], prm->rank,   prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(sb_s, nxz, MPI_TYPE, prm->nrk[2], prm->nrk[2], prm->comm,&reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }
  // north (y+)
  if (prm->nrk[3] > -1){
    int err1 = MPI_Irecv(rb_n, nxz, MPI_TYPE, prm->nrk[3], prm->rank,  prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(sb_n, nxz, MPI_TYPE, prm->nrk[3], prm->nrk[3],prm->comm,&reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }

  // west (x-)
  if (prm->nrk[4] > -1){
    int err1 = MPI_Irecv(rb_w, nyz, MPI_TYPE, prm->nrk[4], prm->rank, prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(sb_w, nyz, MPI_TYPE, prm->nrk[4], prm->nrk[4], prm->comm,&reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }

  // east (x+)
  if (prm->nrk[5] > -1){
    int err1 = MPI_Irecv(rb_e, nyz, MPI_TYPE, prm->nrk[5], prm->rank, prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(sb_e, nyz, MPI_TYPE, prm->nrk[5], prm->nrk[5], prm->comm,&reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }

  MPI_Waitall(counter, reqs, MPI_STATUSES_IGNORE);

  cudaMemcpy(cu_rb_b, rb_b, sizeof(type)*nxy, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_t, rb_t, sizeof(type)*nxy, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_s, rb_s, sizeof(type)*nxz, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_n, rb_n, sizeof(type)*nxz, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_w, rb_w, sizeof(type)*nyz, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_e, rb_e, sizeof(type)*nyz, cudaMemcpyHostToDevice);

  free(sb_t);
  free(sb_s);
  free(sb_n);
  free(sb_w);
  free(sb_e);
  free(rb_b);
  free(rb_t);
  free(rb_s);
  free(rb_n);
  free(rb_w);
  free(rb_e);

#endif


#if 0	
  //CHECK(cudaMemcpy(sb_b, cu_sb_b, sizeof(type)*nxy, cudaMemcpyDeviceToHost));
  //CHECK(cudaMemcpy(rb_b, cu_rb_b, sizeof(type)*nxy, cudaMemcpyDeviceToHost));
  //CHECK(cudaMemcpy(sb_t, cu_sb_t, sizeof(type)*nxy, cudaMemcpyDeviceToHost));
  //CHECK(cudaMemcpy(rb_t, cu_rb_t, sizeof(type)*nxy, cudaMemcpyDeviceToHost));
  //CHECK(cudaMemcpy(sb_s, cu_sb_s, sizeof(type)*nxz, cudaMemcpyDeviceToHost));
  //CHECK(cudaMemcpy(rb_s, cu_rb_s, sizeof(type)*nxz, cudaMemcpyDeviceToHost));
  //CHECK(cudaMemcpy(sb_n, cu_sb_n, sizeof(type)*nxz, cudaMemcpyDeviceToHost));
  //CHECK(cudaMemcpy(rb_n, cu_rb_n, sizeof(type)*nxz, cudaMemcpyDeviceToHost));
  //CHECK(cudaMemcpy(sb_w, cu_sb_w, sizeof(type)*nyz, cudaMemcpyDeviceToHost));
  //CHECK(cudaMemcpy(rb_w, cu_rb_w, sizeof(type)*nyz, cudaMemcpyDeviceToHost));
  //CHECK(cudaMemcpy(sb_e, cu_sb_e, sizeof(type)*nyz, cudaMemcpyDeviceToHost));
  //CHECK(cudaMemcpy(rb_e, cu_rb_e, sizeof(type)*nyz, cudaMemcpyDeviceToHost));
  //printf("after cudaMemcpy.\n");

  //sleev_comm(tmp_vec, prm);
  //CHECK(cudaMemcpy(cu_rb_b, rb_b, sizeof(type)*nxy, cudaMemcpyHostToDevice));
  //CHECK(cudaMemcpy(cu_rb_t, rb_t, sizeof(type)*nxy, cudaMemcpyHostToDevice));
  //CHECK(cudaMemcpy(cu_rb_s, rb_s, sizeof(type)*nxz, cudaMemcpyHostToDevice));
  //CHECK(cudaMemcpy(cu_rb_n, rb_n, sizeof(type)*nxz, cudaMemcpyHostToDevice));
  //CHECK(cudaMemcpy(cu_rb_w, rb_w, sizeof(type)*nyz, cudaMemcpyHostToDevice));
  //CHECK(cudaMemcpy(cu_rb_e, rb_e, sizeof(type)*nyz, cudaMemcpyHostToDevice));
#endif

  readBackTopAndBottom_kernel<<<blocks, threads>>> (nl, 
                                                    ny, nx, nz,
                                                    stm,
                                                    mx, mxy,
                                                    cu_rb_b, 
                                                    cu_rb_t,
                                                    f);
       
  readBackNorthAndSouth_kernel<<<blocks, threads>>> (nl, ny, nx, nz,
                                                     stm,
                                                     mx, mxy,
                                                     cu_rb_s, 
                                                     cu_rb_n,
                                                     f);

  readBackWestAndEast_kernel<<<blocks,threads>>> (nl, ny, nx, nz,
                                                  stm,
                                                  mx, mxy,
                                                  cu_rb_w, 
                                                  cu_rb_e,
                                                  f);
  return 0;
}

int sleev_comm_cuda_BF16(low_type *f, mpi_prm *prm, low_type *cuda_buf){

  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm[0], &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

#if 0
  const size_t VecSize = sizeof(type)*m;
  type *f_FP64;
  cudaMalloc(&f_FP64   ,VecSize);

  const int cuda_threads = 512;
  const int cuda_blocks_m   = ceil((double)(m)/cuda_threads);

#if precon_BF16
  BF16toFP64<<<cuda_blocks_m,cuda_threads>>>(f,f_FP64,m);
#elif precon_FP32
  FP32toFP64<<<cuda_blocks_m,cuda_threads>>>(f,f_FP64,m);
#else

#endif  

  type *vec_cpu = (type*)malloc(VecSize);

#if precon_BF16
  cudaMemcpy(vec_cpu,f_FP64,VecSize,cudaMemcpyDeviceToHost);
  cudaDeviceSynchronize();
  sleev_comm(vec_cpu,prm); 
  cudaMemcpy(f_FP64,vec_cpu,VecSize,cudaMemcpyHostToDevice);
#elif precon_FP32
  cudaMemcpy(vec_cpu,f_FP64,VecSize,cudaMemcpyDeviceToHost);
  cudaDeviceSynchronize();
  sleev_comm(vec_cpu,prm); 
  cudaMemcpy(f_FP64,vec_cpu,VecSize,cudaMemcpyHostToDevice);
#else
  cudaMemcpy(vec_cpu,f,VecSize,cudaMemcpyDeviceToHost);
  cudaDeviceSynchronize();
  sleev_comm(vec_cpu,prm); 
  cudaMemcpy(f,vec_cpu,VecSize,cudaMemcpyHostToDevice);
#endif  

#if precon_BF16
  FP64toBF16<<<cuda_blocks_m,cuda_threads>>>(f_FP64,f,m);
#elif precon_FP32
  FP64toFP32<<<cuda_blocks_m,cuda_threads>>>(f_FP64,f,m);
#else

#endif
  free(vec_cpu);
  cudaFree(f_FP64);
  return 0;
#endif

  const int nl = 1; //袖の長さ
  const int nxy= nx*ny*nl;
  const int nxz= nx*nz*nl;
  const int nyz= ny*nz*nl;

  low_type *cu_sb_b = &cuda_buf[0];
  low_type *cu_sb_t = &cuda_buf[nxy];
  low_type *cu_sb_s = &cuda_buf[nxy*2];
  low_type *cu_sb_n = &cuda_buf[nxy*2+nxz];
  low_type *cu_sb_w = &cuda_buf[nxy*2+nxz*2];
  low_type *cu_sb_e = &cuda_buf[nxy*2+nxz*2+nyz];

  int sbufSize = nxy*2+nxz*2+nyz*2;
  low_type *cu_rb_b = &cuda_buf[sbufSize + 0];
  low_type *cu_rb_t = &cuda_buf[sbufSize + nxy];
  low_type *cu_rb_s = &cuda_buf[sbufSize + nxy*2];
  low_type *cu_rb_n = &cuda_buf[sbufSize + nxy*2+nxz];
  low_type *cu_rb_w = &cuda_buf[sbufSize + nxy*2+nxz*2];
  low_type *cu_rb_e = &cuda_buf[sbufSize + nxy*2+nxz*2+nyz];

  
  const int threads = 32;  
  const int blocks  = 1024;  
  
  setTopAndBottom_kernel_BF16<<<blocks,threads>>>(nl, 
						ny, nx, nz,
						stm,
						mx, mxy,
						cu_sb_b, 
						cu_sb_t,
						f);
  setNorthAndSouth_kernel_BF16<<<blocks,threads>>>(nl, ny, nx, nz,
						 stm,
						 mx, mxy,
						 cu_sb_s, 
						 cu_sb_n,
						 f);
  setWestAndEast_kernel_BF16<<<blocks,threads>>>(nl, ny, nx, nz,
					       stm,
					       mx, mxy,
					       cu_sb_w, 
					       cu_sb_e,
					       f);
  cudaDeviceSynchronize();
  MPI_Request reqs[12];
  int counter = 0;

#if 0
  // CUDA-Aware MPI用コード
  // bottom (z-)
  if (prm->nrk[0] > -1){
    //printf("Wait bottom.\n");
    int err1 = MPI_Irecv(cu_rb_b, nxy, MPI_LOW_TYPE, prm->nrk[0], prm->rank, prm->comm,  &reqs[0]);
    int err2 = MPI_Isend(cu_sb_b, nxy, MPI_LOW_TYPE, prm->nrk[0], prm->nrk[0], prm->comm,&reqs[1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }
  // top    (z+)
  if (prm->nrk[1] > -1){
    int err1 = MPI_Irecv(cu_rb_t, nxy, MPI_LOW_TYPE, prm->nrk[1], prm->rank,   prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(cu_sb_t, nxy, MPI_LOW_TYPE, prm->nrk[1], prm->nrk[1], prm->comm, &reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }

  // south (y-)
  if (prm->nrk[2] > -1){
    int err1 = MPI_Irecv(cu_rb_s, nxz, MPI_LOW_TYPE, prm->nrk[2], prm->rank,   prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(cu_sb_s, nxz, MPI_LOW_TYPE, prm->nrk[2], prm->nrk[2], prm->comm,&reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }
  // north (y+)
  if (prm->nrk[3] > -1){
    int err1 = MPI_Irecv(cu_rb_n, nxz, MPI_LOW_TYPE, prm->nrk[3], prm->rank,  prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(cu_sb_n, nxz, MPI_LOW_TYPE, prm->nrk[3], prm->nrk[3],prm->comm,&reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }

  // west (x-)
  if (prm->nrk[4] > -1){
    int err1 = MPI_Irecv(cu_rb_w, nyz, MPI_LOW_TYPE, prm->nrk[4], prm->rank, prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(cu_sb_w, nyz, MPI_LOW_TYPE, prm->nrk[4], prm->nrk[4], prm->comm,&reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }

  // east (x+)
  if (prm->nrk[5] > -1){
    int err1 = MPI_Irecv(cu_rb_e, nyz, MPI_LOW_TYPE, prm->nrk[5], prm->rank, prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(cu_sb_e, nyz, MPI_LOW_TYPE, prm->nrk[5], prm->nrk[5], prm->comm,&reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }
  MPI_Waitall(counter, reqs, MPI_STATUSES_IGNORE);

#else


  low_type *sb_b = (low_type*)malloc(sizeof(low_type)*nxy);
  low_type *sb_t = (low_type*)malloc(sizeof(low_type)*nxy);
  low_type *sb_s = (low_type*)malloc(sizeof(low_type)*nxz);
  low_type *sb_n = (low_type*)malloc(sizeof(low_type)*nxz);
  low_type *sb_w = (low_type*)malloc(sizeof(low_type)*nyz);
  low_type *sb_e = (low_type*)malloc(sizeof(low_type)*nyz);
  
  low_type *rb_b = (low_type*)malloc(sizeof(low_type)*nxy);
  low_type *rb_t = (low_type*)malloc(sizeof(low_type)*nxy);
  low_type *rb_s = (low_type*)malloc(sizeof(low_type)*nxz);
  low_type *rb_n = (low_type*)malloc(sizeof(low_type)*nxz);
  low_type *rb_w = (low_type*)malloc(sizeof(low_type)*nyz);
  low_type *rb_e = (low_type*)malloc(sizeof(low_type)*nyz);

// -----
  int j;
  for(j=0;j<nxy;j++){
    sb_b[j] = 0.0;
    sb_t[j] = 0.0;
    rb_b[j] = 0.0;
    rb_t[j] = 0.0;
  }
  for(j=0;j<nxz;j++){
    sb_s[j] = 0.0;
    sb_n[j] = 0.0;
    rb_s[j] = 0.0;
    rb_n[j] = 0.0;
  }
  for(j=0;j<nyz;j++){
    sb_w[j] = 0.0;
    sb_e[j] = 0.0;
    rb_w[j] = 0.0;
    rb_e[j] = 0.0;
  }
  // -----

  cudaMemcpy(sb_b, cu_sb_b, sizeof(low_type)*nxy, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_t, cu_sb_t, sizeof(low_type)*nxy, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_s, cu_sb_s, sizeof(low_type)*nxz, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_n, cu_sb_n, sizeof(low_type)*nxz, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_w, cu_sb_w, sizeof(low_type)*nyz, cudaMemcpyDeviceToHost);
  cudaMemcpy(sb_e, cu_sb_e, sizeof(low_type)*nyz, cudaMemcpyDeviceToHost);
  
  // bottom (z-)
  if (prm->nrk[0] > -1){
    //printf("Wait bottom.\n");
    int err1 = MPI_Irecv(rb_b, nxy, MPI_LOW_TYPE, prm->nrk[0], prm->rank, prm->comm,  &reqs[0]);
    int err2 = MPI_Isend(sb_b, nxy, MPI_LOW_TYPE, prm->nrk[0], prm->nrk[0], prm->comm,&reqs[1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }
  // top    (z+)
  if (prm->nrk[1] > -1){
    int err1 = MPI_Irecv(rb_t, nxy, MPI_LOW_TYPE, prm->nrk[1], prm->rank,   prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(sb_t, nxy, MPI_LOW_TYPE, prm->nrk[1], prm->nrk[1], prm->comm, &reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }

  // south (y-)
  if (prm->nrk[2] > -1){
    int err1 = MPI_Irecv(rb_s, nxz, MPI_LOW_TYPE, prm->nrk[2], prm->rank,   prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(sb_s, nxz, MPI_LOW_TYPE, prm->nrk[2], prm->nrk[2], prm->comm,&reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }
  // north (y+)
  if (prm->nrk[3] > -1){
    int err1 = MPI_Irecv(rb_n, nxz, MPI_LOW_TYPE, prm->nrk[3], prm->rank,  prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(sb_n, nxz, MPI_LOW_TYPE, prm->nrk[3], prm->nrk[3],prm->comm,&reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }

  // west (x-)
  if (prm->nrk[4] > -1){
    int err1 = MPI_Irecv(rb_w, nyz, MPI_LOW_TYPE, prm->nrk[4], prm->rank, prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(sb_w, nyz, MPI_LOW_TYPE, prm->nrk[4], prm->nrk[4], prm->comm,&reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }

  // east (x+)
  if (prm->nrk[5] > -1){
    int err1 = MPI_Irecv(rb_e, nyz, MPI_LOW_TYPE, prm->nrk[5], prm->rank, prm->comm, &reqs[counter]);
    int err2 = MPI_Isend(sb_e, nyz, MPI_LOW_TYPE, prm->nrk[5], prm->nrk[5], prm->comm,&reqs[counter+1]);
    counter+=2;
    if(err1 != 0 || err2 != 0)printf("Error in MPI communication");
  }

  MPI_Waitall(counter, reqs, MPI_STATUSES_IGNORE);
  cudaDeviceSynchronize();

  cudaMemcpy(cu_rb_b, rb_b, sizeof(low_type)*nxy, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_t, rb_t, sizeof(low_type)*nxy, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_s, rb_s, sizeof(low_type)*nxz, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_n, rb_n, sizeof(low_type)*nxz, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_w, rb_w, sizeof(low_type)*nyz, cudaMemcpyHostToDevice);
  cudaMemcpy(cu_rb_e, rb_e, sizeof(low_type)*nyz, cudaMemcpyHostToDevice);

  free(sb_t);
  free(sb_s);
  free(sb_n);
  free(sb_w);
  free(sb_e);
  free(rb_b);
  free(rb_t);
  free(rb_s);
  free(rb_n);
  free(rb_w);
  free(rb_e);

#endif

  readBackTopAndBottom_kernel_BF16<<<blocks, threads>>> (nl, 
                                                         ny, nx, nz,
                                                         stm,
                                                         mx, mxy,
                                                         cu_rb_b, 
                                                         cu_rb_t,
                                                         f);
  readBackNorthAndSouth_kernel_BF16<<<blocks, threads>>> (nl, ny, nx, nz,
                                                          stm,
                                                          mx, mxy,
                                                          cu_rb_s, 
                                                          cu_rb_n,
                                                          f);

  readBackWestAndEast_kernel_BF16<<<blocks,threads>>> (nl, ny, nx, nz,
                                                       stm,
                                                       mx, mxy,
                                                       cu_rb_w, 
                                                       cu_rb_e,
                                                       f);
  return 0;
}
