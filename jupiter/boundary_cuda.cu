/*
 * boundary_cuda.cu
 *
 */
#include "func.h"
#include "boundary_cuda.h"
#include "communication_cuda.h"


//extern double bcf_wall_cuda_time;



//bottom
__global__ void bcf_wall_bottom_cuda_kernel(type *d_f, type *d_ft, int mx, int my){
  int j, jx, jy;
  int mxy = mx*my;
  jx=blockDim.x * blockIdx.x + threadIdx.x;
  jy=blockDim.y * blockIdx.y + threadIdx.y;
  if( jx>=mx || jy>=my) return ;

  j=jx + mx*jy;
  d_ft[j + mxy*0] = d_f[j + mxy*5];
  d_ft[j + mxy*1] = d_f[j + mxy*4];
  d_ft[j + mxy*2] = d_f[j + mxy*3];
}
//top
__global__ void bcf_wall_top_cuda_kernel(type *d_f, type *d_ft, int mx, int my,int nz){
  int j, jx, jy;
  int mxy = mx*my;
  jx=blockDim.x * blockIdx.x + threadIdx.x;
  jy=blockDim.y * blockIdx.y + threadIdx.y;
  if(jx>=mx || jy>=my) return ;
  
  j=jx + mx*jy;
  d_ft[j + mxy*(nz+3)] = d_f[j + mxy*(nz+2)];
  d_ft[j + mxy*(nz+4)] = d_f[j + mxy*(nz+1)];
  d_ft[j + mxy*(nz+5)] = d_f[j + mxy*(nz  )];
  d_ft[j + mxy*(nz+6)] = d_f[j + mxy*(nz-1)];
}
//south
__global__ void bcf_wall_south_cuda_kernel(type *d_f, type *d_ft, int mx, int my, int mz){
  int j, jx, jz;
  int mxy = mx*my;
  jx=blockDim.x * blockIdx.x + threadIdx.x;
  jz=blockDim.z * blockIdx.z + threadIdx.z;
  if(jx>=mx || jz>=mz) return ;

  j=jx + mxy*jz;
  d_ft[j + mx*0] = d_f[j + mx*5];
  d_ft[j + mx*1] = d_f[j + mx*4];
  d_ft[j + mx*2] = d_f[j + mx*3];
}

//north
__global__ void bcf_wall_north_cuda_kernel(type *d_f, type *d_ft, int mx, int my, int mz, int ny){
  int j, jx, jz;
  int mxy = mx*my;
  jx=blockDim.x * blockIdx.x + threadIdx.x;
  jz=blockDim.z * blockIdx.z + threadIdx.z;
  if(jx>=mx || jz>=mz ) return;

  j=jx + mxy*jz;
  d_ft[j + mx*(ny+3)] = d_f[j + mx*(ny+2)];
  d_ft[j + mx*(ny+4)] = d_f[j + mx*(ny+1)];
  d_ft[j + mx*(ny+5)] = d_f[j + mx*(ny  )];
  d_ft[j + mx*(ny+6)] = d_f[j + mx*(ny-1)];
}

//west
__global__ void bcf_wall_west_cuda_kernel(type *d_f, type *d_ft, int mx, int my,int mz){
  int j, jy, jz;
  jy=blockDim.y * blockIdx.y + threadIdx.y;
  jz=blockDim.z * blockIdx.z + threadIdx.z;
  if(jy >=my || jz >=mz) return;

  j=jy + my*jz;
  d_ft[0 + mx*j] = d_f[5 + mx*j];
  d_ft[1 + mx*j] = d_f[4 + mx*j];
  d_ft[2 + mx*j] = d_f[3 + mx*j];
}

//east
__global__ void bcf_wall_east_cuda_kernel(type *d_f, type *d_ft, int mx, int my,int mz,int nx){
  int j, jy, jz;
  jy=blockDim.y * blockIdx.y + threadIdx.y;
  jz=blockDim.z * blockIdx.z + threadIdx.z;
  if(jy >=my || jz >=mz) return;

  j=jy + my*jz;
  d_ft[(nx+3) + mx*j ] = d_f[(nx+2) + mx*j ];
  d_ft[(nx+4) + mx*j ] = d_f[(nx+1) + mx*j ];
  d_ft[(nx+5) + mx*j ] = d_f[(nx  ) + mx*j ];
  d_ft[(nx+6) + mx*j ] = d_f[(nx-1) + mx*j ];
}


__global__ void bcf_wall_temp2f_cuda_kernel(type *d_f, type *d_ft, int mx, int my, int mz){
  int j, jx, jy, jz;
  jx=blockDim.x * blockIdx.x + threadIdx.x;
  jy=blockDim.y * blockIdx.y + threadIdx.y;
  jz=blockDim.z * blockIdx.z + threadIdx.z;
  if(jx>=mx || jy >=my || jz >=mz) return;

  j= jx + mx*jy + mx*my*jz;
  d_f[j]=d_ft[j];
}


__global__ void init_temp_cuda_kernel(type *f, type *ft, int mx, int my, int m){
  int j, jx, jy, jz;
  jx=blockDim.x * blockIdx.x + threadIdx.x;
  jy=blockDim.y * blockIdx.y + threadIdx.y;
  jz=blockDim.z * blockIdx.z + threadIdx.z;
  j = jx + mx*jy + mx*my*jz;
  if(j >=m) return ;

  ft[j]= f[j];
}

void bcf_wall_cuda(type *d_f, domain *cdo, int *nrk){
  int block_x,block_y,block_z,
  nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
  mx=cdo->mx, my=cdo->my, mz=cdo->mz, m=cdo->m;
  type *d_ft;

  block_x=mx/blockDim_x;
  if(mx%blockDim_x>0){
    block_x +=1;
  }
  block_y=my/blockDim_y;
  if(my%blockDim_y>0){
	  block_y +=1;
  }
  block_z=mz/blockDim_z;
  if(mz%blockDim_z>0){
    block_z +=1;
  }

  //temp

  dim3 Dga(block_x,block_y,block_z);
  dim3 Dba(blockDim_x,blockDim_y,blockDim_z);

  size_t size = sizeof(type)*m;
  cudaMalloc((void**)&d_ft,size);
  init_temp_cuda_kernel<<<Dga,Dba>>>(d_f,d_ft,mx,my,m);

  //bottom
  if(nrk[0] == -1){
    dim3 Dg(block_x, block_y, 1);
    dim3 Db(blockDim_x, blockDim_y, 1);
    bcf_wall_bottom_cuda_kernel<<<Dg,Db>>>(d_f,d_ft,mx,my);
    bcf_wall_temp2f_cuda_kernel<<<Dga,Dba>>>(d_f,d_ft,mx,my,mz);
  }
  //top
  if(nrk[1] == -1){
    dim3 Dg(block_x,block_y,1);
    dim3 Db(blockDim_x,blockDim_y,1);
    bcf_wall_top_cuda_kernel<<<Dg,Db>>>(d_f,d_ft,mx,my,nz);
    bcf_wall_temp2f_cuda_kernel<<<Dga,Dba>>>(d_f,d_ft,mx,my,mz);
  }
  //south
  if(nrk[2] == -1){
    dim3 Dg(block_x,1,block_z);
    dim3 Db(blockDim_x,1,blockDim_z);
    bcf_wall_south_cuda_kernel<<<Dg,Db>>>(d_f, d_ft, mx, my, mz);
    bcf_wall_temp2f_cuda_kernel<<<Dga,Dba>>>(d_f,d_ft,mx,my,mz);
  }
  //north
  if(nrk[3] == -1){
    dim3 Dg(block_x,1,block_z);
    dim3 Db(blockDim_x,1,blockDim_z);
    bcf_wall_north_cuda_kernel<<<Dg,Db>>>(d_f, d_ft, mx, my,mz,ny);
    bcf_wall_temp2f_cuda_kernel<<<Dga,Dba>>>(d_f,d_ft,mx,my,mz);
  }
  //west
  if(nrk[4] == -1){
    dim3 Dg(1,block_y,block_z);
    dim3 Db(1,blockDim_y,blockDim_z);
    bcf_wall_west_cuda_kernel<<<Dg,Db>>>(d_f, d_ft, mx, my, mz);
    bcf_wall_temp2f_cuda_kernel<<<Dga,Dba>>>(d_f,d_ft,mx,my,mz);
  }
  //east
  if(nrk[5] == -1){
    dim3 Dg(1,block_y,block_z);
    dim3 Db(1,blockDim_y,blockDim_z);
    bcf_wall_east_cuda_kernel<<<Dg,Db>>>(d_f, d_ft, mx, my, mz,nx);
    bcf_wall_temp2f_cuda_kernel<<<Dga,Dba>>>(d_f,d_ft,mx,my,mz);
  }
  //cudaDeviceSynchronize();
  cudaFree(d_ft);
}

type bcf_cuda_buf(type *f, parameter *prm, comm_buf_cuda *comm_buf_cu)
{
  double time0 = 0.0;

  // MPI communication
  if (prm->mpi->npe > 1) {
    communication_cuda_buff(f, prm, comm_buf_cu);
  }
  // Boundary

  bcf_wall_cuda(f, prm->cdo, prm->mpi->nrk);

  //cudaDeviceSynchronize();
  return time0;
}

