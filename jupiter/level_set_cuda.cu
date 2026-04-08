
#include <math.h>
#include <cuda_runtime.h>
#include "level_set_cuda_wrapper.h"
#include "level_set_cuda.h"
#include "boundary_cuda.h"



__global__ void vof2ls_cuda_kernel(type *fs, type *ls, int mx, int my, int m, type coeff){
  int j, jx, jy, jz;
  jx=blockDim.x * blockIdx.x + threadIdx.x;
  jy=blockDim.y * blockIdx.y + threadIdx.y;
  jz=blockDim.z * blockIdx.z + threadIdx.z;
  j = jx + mx*jy + mx*my*jz;
  if(j>=m) return ;

  ls[j]= coeff * (fs[j]-0.5);
}

void vof2ls_cuda(int init_flg, type *fs, type *ls, domain *cdo)
{
  int  mx=cdo->mx, my=cdo->my, mz=cdo->mz, m=cdo->m;
  int block_x,block_y,block_z;
  type coeff;

  if(init_flg == 1) {
    coeff=3.0*cdo->dx;
  }else{
    coeff=cdo->gLz;
  }

  block_x = mx/blockDim_x;
  if(mx%blockDim_x > 0){
    block_x +=1;
  }
  block_y = my/blockDim_y;
  if(my%blockDim_y > 0){
    block_y +=1;
  }
  block_z = mz/blockDim_z;
  if(mz/blockDim_z>0){
    block_z +=1;
  }
  dim3 Dg(block_x,block_y,block_z);
  dim3 Db(blockDim_x,blockDim_y,blockDim_z);
  vof2ls_cuda_kernel<<<Dg,Db>>>(fs,ls,mx,my,m,coeff);
}

__global__ void level_set_eq_cuda_kernel(type *fn, type a0, type *f0, type af, type aft, type *f,
  int nx, int ny, int nz, int mx,int my,int mz,int stm,
  type dx, type dy, type dz, type dzi,type dyi,type dxi){

  int j, jx, jy, jz, mxy;
  type  fx2, fy2, fz2, ft, sign;
  jx=blockDim.x * blockIdx.x + threadIdx.x;
  jy=blockDim.y * blockIdx.y + threadIdx.y;
  jz=blockDim.z * blockIdx.z + threadIdx.z;
  if(jx >= nx || jy >= ny || jz >=nz) return;

  mxy = mx*my;
  j=jx+stm + mx*(jy+stm) + mxy*(jz + stm);

  sign = f[j]/sqrt(f[j]*f[j] + dx*dy);
  LS_ADV_mp(fz2, f[j-2*mxy], f[j-mxy], f[j], f[j+mxy], f[j+2*mxy], sign, dzi);
  LS_ADV_mp(fy2, f[j-2*mx ],f[j-mx ],f[j],f[j+mx ],f[j+2*mx ], sign, dyi);
  LS_ADV_mp(fx2, f[j-2    ],f[j-1  ],f[j],f[j+1  ],f[j+2    ], sign, dxi);
  ft = sign*(1.0 - sqrt(fx2 + fy2 + fz2));
  fn[j] = a0*f0[j] + af*f[j] + aft*ft;

}

void init_GPUs(parameter *prm){
  int num_gpu;
  //set GPU
  cudaError_t cu_err;
  cudaGetDeviceCount(&num_gpu);
  const int selected_gpu = prm->mpi->rank % num_gpu;
  cu_err = cudaSetDevice(selected_gpu);
  prm->mpi->myGPU = selected_gpu;
  prm->mpi->totGPU = num_gpu;
  if (cu_err != cudaSuccess) {
    printf(" cudaSetDevice error \n");
    fprintf(stderr, "[Error] %s (error code: %d) at %s line %d\n",
            cudaGetErrorString(cu_err), cu_err, __FILE__, __LINE__);
  //    return -1;
}
cudaDeviceSynchronize();

}

void level_set_eq_cuda(type *fn, type a0, type *f0, type af, type aft, type *f, domain *cdo)
{
  int stm=cdo->stm,
    nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
    mx=cdo->mx, my=cdo->my, mz=cdo->mz;
  type dx=cdo->dx, dy=cdo->dy, dz=cdo->dz,
    dxi=1.0/dx, dyi=1.0/dy, dzi=1.0/dz;
  int block_x, block_y, block_z;

  block_x=nx/blockDim_x;
  if(nx%blockDim_x > 0){
    block_x +=1;
  }
  block_y=ny/blockDim_y;
  if(ny%blockDim_y > 0){
    block_y +=1;
  }

  block_z=nz/blockDim_z;
  if(nz%blockDim_z >0 ){
    block_z +=1;
  }

  dim3 Dg(block_x,block_y,block_z);
  dim3 Db(blockDim_x,blockDim_y,blockDim_z);

  level_set_eq_cuda_kernel<<<Dg,Db>>>(fn, a0, f0, af, aft, f,nx,ny,nz,mx,my,mz,stm,dx,dy,dz,dzi,dyi,dxi);

}


type Level_Set_cuda(int init_flg, int itr_max, type *ls, type *fs, parameter *prm)
{
  type *d_ls, *d_fs, *d_tmp1, *d_tmp2;
  domain *cdo = prm->cdo;

  //printf("call Level_Set_cuda\n");

  size_t size = sizeof(type)*(cdo->m);
  int stcl=cdo->stm, mx=cdo->mx, my=cdo->my, mz=cdo->mz;

  size_t size_buf = sizeof(type)*((mx*my + my*mz + mz*mx)*2*stcl);

  comm_buf_cuda comm_buf_cu;
  comm_buf_cu.buf_size =size_buf;
  cudaMalloc((void**)&comm_buf_cu.d_sbuff,size_buf);
  cudaMalloc((void**)&comm_buf_cu.d_rbuff,size_buf);
  comm_buf_cu.h_sbuff = (type*)malloc(size_buf);
  comm_buf_cu.h_rbuff = (type*)malloc(size_buf);

  // VOF => init Level Set

  cudaMalloc((void**)&d_ls,size);
  cudaMalloc((void**)&d_fs,size);
  cudaMemcpy(d_fs,fs,size,cudaMemcpyHostToDevice);

  vof2ls_cuda(init_flg, d_fs, d_ls, cdo);

  bcf_cuda_buf(d_ls, prm, &comm_buf_cu);


  // 3rd-order TVD Runge-Kutta
  cudaMalloc((void**)&d_tmp1,size);
  cudaMalloc((void**)&d_tmp2,size);
  int  itr = 0;
  type dt = cdo->coef_lsts*cdo->dx;

  while (itr++ < itr_max) {

    level_set_eq_cuda(d_tmp1,    1., d_ls,    0.,      dt, d_ls,   cdo);

    bcf_cuda_buf(d_tmp1, prm,&comm_buf_cu);

    level_set_eq_cuda(d_tmp2, 3./4., d_ls, 1./4.,1./4.*dt, d_tmp1, cdo);

    bcf_cuda_buf(d_tmp2, prm,&comm_buf_cu);

    level_set_eq_cuda(d_ls,   1./3., d_ls, 2./3.,2./3.*dt, d_tmp2, cdo);

    bcf_cuda_buf(d_ls, prm,&comm_buf_cu);
  }

  cudaFree(d_tmp1);
  cudaFree(d_tmp2);

  cudaMemcpy(ls, d_ls, size, cudaMemcpyDeviceToHost);
  cudaFree(d_ls);
  cudaFree(d_fs);

  cudaFree(comm_buf_cu.d_rbuff);
  cudaFree(comm_buf_cu.d_sbuff);
  free(comm_buf_cu.h_sbuff);
  free(comm_buf_cu.h_rbuff);

  //cudaDeviceSynchronize();
  return 0.0;
}



