#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include <cuda_runtime.h>
//#include <cuda_bf16.h>
//#include <cuda_fp16.h>
#include "cublas_v2.h"

#include "struct.h"
#include "cg.h"

#include "pcg_block3d_cuda.h"
#include "sleev_comm_block3d_cuda.h"

__global__ void calc_sres_block3D_cuda_kernel(
					      type* A,
					      type* x,
					      type* b,
					      type* r,
					      const  int block_m,
					      int *block_nxs, int *block_nxe, 
					      int *block_nys, int *block_nye, 
					      int *block_nzs, int *block_nze,
					      int *stride_xp,int *stride_xm,
					      int *stride_yp,int *stride_ym,
					      int *stride_zp,int *stride_zm,
					      const int nxblock,const int nyblock,const int nzblock,
					      const int mxdivblock,const int mydivblock,const int mzdivblock,
					      const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int ndivblock  = nxdivblock * nydivblock * nzdivblock;
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = ndivblock;
  //  if(tid >= domain) return;

  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    const int jzb =  jb / (nxdivblock*nydivblock);
    const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
    const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;

    const int jjzbs = block_nzs[jzb];
    const int jjzbe = block_nze[jzb];
    const int jjybs = block_nys[jyb];
    const int jjybe = block_nye[jyb];
    const int jjxbs = block_nxs[jxb];
    const int jjxbe = block_nxe[jxb];

    for(int jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
      for(int jjyb=jjybs;jjyb<=jjybe;jjyb++){    
	for(int jjxb=jjxbs;jjxb<=jjxbe;jjxb++){
	  
	  const int stxm = stride_xm[jjxb];
	  const int stxp = stride_xp[jjxb];
	  const int stym = stride_ym[jjyb];
	  const int styp = stride_yp[jjyb];
	  const int stzm = stride_zm[jjzb];
	  const int stzp = stride_zp[jjzb];

	  // 内側インデックス
	  const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	

	  const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	
	  const int jcb = j + stzm;
	  const int jcs = j + stym;
	  const int jcw = j + stxm;
	  const int jcc = j       ;
	  const int jce = j + stxp;
	  const int jcn = j + styp;
	  const int jct = j + stzp;

#if 1	
	  type val = 
	    A[j   + 0 * block_m]*x[jcb] 
	    +A[j   + 1 * block_m]*x[jcs] 
	    +A[j   + 2 * block_m]*x[jcw] 
	    +A[j   + 3 * block_m]*x[jcc] 
	    +A[jce + 2 * block_m]*x[jce] 
	    +A[jcn + 1 * block_m]*x[jcn] 
	    +A[jct + 0 * block_m]*x[jct] 
	    ;
#else
	  type val = 
	    A[j + 0 * block_m]*x[jcb] 
	    +A[j + 1 * block_m]*x[jcs] 
	    +A[j + 2 * block_m]*x[jcw] 
	    +A[j + 3 * block_m]*x[jcc] 
	    +A[j + 4 * block_m]*x[jce] 
	    +A[j + 5 * block_m]*x[jcn] 
	    +A[j + 6 * block_m]*x[jct] 
	    ;
#endif
	  r[j] = b[j] - val;
	} 
      }
    }

  }

  return ;
}

int calc_sres_block3D_cuda(
			   mpi_prm prm,type* A,type* x,type* b,type* r,type* cuda_buf,
			   int *block_nxs, int *block_nxe, 
			   int *block_nys, int *block_nye, 
			   int *block_nzs, int *block_nze,
			   int *stride_xp,int *stride_xm,
			   int *stride_yp,int *stride_ym,
			   int *stride_zp,int *stride_zm 
			   ){

  int block_m = prm.block_m;
  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;

  const int blocks  = 1024;  
  const int threads = 96;  

  calc_sres_block3D_cuda_kernel<<<blocks,threads>>>(
						    A, x, b, r,
						    block_m,
						    block_nxs, block_nxe, 
						    block_nys, block_nye, 
						    block_nzs, block_nze,
						    stride_xp, stride_xm,
						    stride_yp, stride_ym,
						    stride_zp, stride_zm,
						    nxblock,  nyblock,  nzblock,
						    mxdivblock,  mydivblock,  mzdivblock,
						    nxdivblock,  nydivblock,  nzdivblock);
  return 0;

}

__global__ void  axpy_block3D_cuda_kernel(
					  type *x,type *y,type alpha,type beta,
					  int *block_nxs, int *block_nxe, 
					  int *block_nys, int *block_nye, 
					  int *block_nzs, int *block_nze,
					  int nxblock,int nyblock,int nzblock,
					  int mxdivblock,int mydivblock,int mzdivblock,
					  int nxdivblock,int nydivblock,int nzdivblock){

  const int ndivblock  = nxdivblock * nydivblock * nzdivblock;
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = ndivblock;
  if(tid >= domain) return;

  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){

    int jzb =  jb / (nxdivblock*nydivblock);
    int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
    int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;

    const int jjzbs = block_nzs[jzb];
    const int jjzbe = block_nze[jzb];
    const int jjybs = block_nys[jyb];
    const int jjybe = block_nye[jyb];
    const int jjxbs = block_nxs[jxb];
    const int jjxbe = block_nxe[jxb];

    for(int jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
      for(int jjyb=jjybs;jjyb<=jjybe;jjyb++){    
	for(int jjxb=jjxbs;jjxb<=jjxbe;jjxb++){
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  y[j] = alpha * x[j] + beta * y[j];
	}
      }
    }

  }
  return ;
}


int axpy_block3D_cuda(
		      mpi_prm prm,type *x,type *y,type alpha,type beta,
		      int *block_nxs, int *block_nxe, 
		      int *block_nys, int *block_nye, 
		      int *block_nzs, int *block_nze
		      ){

  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;

  const int blocks  = 1024;  
  const int threads = 96;  
  axpy_block3D_cuda_kernel<<<blocks,threads>>>(
					       x, y, alpha, beta,
					       block_nxs, block_nxe, 
					       block_nys, block_nye, 
					       block_nzs, block_nze,
					       nxblock,nyblock,nzblock,
					       mxdivblock,mydivblock,mzdivblock,
					       nxdivblock,nydivblock,nzdivblock
					       );
  return 0;
}

__global__ void axpy2_block3D_cuda_kernel(
					  type alpha,type beta,type *x,type *y,type *z,
					  int *block_nxs, int *block_nxe, 
					  int *block_nys, int *block_nye, 
					  int *block_nzs, int *block_nze,
					  int nxblock,int nyblock,int nzblock,
					  int mxdivblock,int mydivblock,int mzdivblock,
					  int nxdivblock,int nydivblock,int nzdivblock){

  const int ndivblock  = nxdivblock * nydivblock * nzdivblock;
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = ndivblock;
  if(tid >= domain) return;

  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jzb =  jb / (nxdivblock*nydivblock);
    int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
    int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;

    const int jjzbs = block_nzs[jzb];
    const int jjzbe = block_nze[jzb];
    const int jjybs = block_nys[jyb];
    const int jjybe = block_nye[jyb];
    const int jjxbs = block_nxs[jxb];
    const int jjxbe = block_nxe[jxb];

    for(int jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
      for(int jjyb=jjybs;jjyb<=jjybe;jjyb++){    
	for(int jjxb=jjxbs;jjxb<=jjxbe;jjxb++){
    
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  z[j] = alpha * y[j] + z[j];
	  y[j] = beta  * y[j] + x[j];
	}
      }
    }
  }
  return ;
}


int axpy2_block3D_cuda(
		       mpi_prm prm,type alpha,type beta,type *x,type *y,type *z,
		       int *block_nxs, int *block_nxe, 
		       int *block_nys, int *block_nye, 
		       int *block_nzs, int *block_nze
		       ){

  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;

  const int blocks  = 1024;  
  const int threads = 96;  

  axpy2_block3D_cuda_kernel<<<blocks,threads>>>(
						alpha, beta, x, y, z,
						block_nxs, block_nxe, 
						block_nys, block_nye, 
						block_nzs, block_nze,
						nxblock,nyblock,nzblock,
						mxdivblock,mydivblock,mzdivblock,
						nxdivblock,nydivblock,nzdivblock);

  return 0;

}

__global__ void calc_dot_local_block3D_cuda_kernel(
						   type *x,type *y,type *ret_type,
						   int *block_nxs, int *block_nxe, 
						   int *block_nys, int *block_nye, 
						   int *block_nzs, int *block_nze,
						   const int nxblock,const int nyblock,const int nzblock,
						   const int mxdivblock,const int mydivblock,const int mzdivblock,
						   const int nxdivblock,const int nydivblock,const int nzdivblock){
  const int ndivblock  = nxdivblock * nydivblock * nzdivblock;
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = ndivblock;

  type val = 0.0;
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jzb =  jb / (nxdivblock*nydivblock);
    int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
    int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;

    const int jjzbs = block_nzs[jzb];
    const int jjzbe = block_nze[jzb];
    const int jjybs = block_nys[jyb];
    const int jjybe = block_nye[jyb];
    const int jjxbs = block_nxs[jxb];
    const int jjxbe = block_nxe[jxb];

    for(int jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
      for(int jjyb=jjybs;jjyb<=jjybe;jjyb++){    
	for(int jjxb=jjxbs;jjxb<=jjxbe;jjxb++){
	  
	  int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  val = val + x[j]*y[j];
	} 
      }
    }
  }
  local_sum_cuda(val,ret_type,threadIdx.x);
  return ;
}

int calc_dot_local_block3D_cuda(
				mpi_prm prm,type *x,type *y,type *cuda_buf,
				int *block_nxs, int *block_nxe, 
				int *block_nys, int *block_nye, 
				int *block_nzs, int *block_nze
				){

  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;

  const int warp_size = 32;
  initialize_type_vec<<<1,warp_size>>>(cuda_buf,1);

  //  printf("nxblock,nyblock,nzblock = %d,%d,%d \n",nxblock,nyblock,nzblock);
  //  printf("nxdivblock,nydivblock,nzdivblock = %d,%d,%d \n",nxdivblock,nydivblock,nzdivblock);

  const int blocks  = 1024;  
  const int threads = 96;  
  calc_dot_local_block3D_cuda_kernel<<<blocks,threads>>>(
							 x,y,cuda_buf,
							 block_nxs, block_nxe, 
							 block_nys, block_nye, 
							 block_nzs, block_nze,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);

  return 0;
}

int calc_dot_block3D_cuda(
			  mpi_prm prm,type *x,type *y,type *ret_type_cpu,type *cuda_buf,
			  int *block_nxs, int *block_nxe, 
			  int *block_nys, int *block_nye, 
			  int *block_nzs, int *block_nze
			  ){
  calc_dot_local_block3D_cuda(prm,x,y,cuda_buf,
			      block_nxs, block_nxe, 
			      block_nys, block_nye, 
			      block_nzs, block_nze
			      );
  cuda_MPI_Allreduce_sum(1,ret_type_cpu,cuda_buf,prm.comm);
  return 0;
}

int calc_norm_block3D_cuda(
			   mpi_prm prm,type *x,type *y,type *ret_type_cpu,type *cuda_buf,
			   int *block_nxs, int *block_nxe, 
			   int *block_nys, int *block_nye, 
			   int *block_nzs, int *block_nze
			   ){
  calc_dot_block3D_cuda(prm,x,y,ret_type_cpu,cuda_buf,
			      block_nxs, block_nxe, 
			      block_nys, block_nye, 
			      block_nzs, block_nze
			);
  ret_type_cpu[0] = sqrt(ret_type_cpu[0]);
  return 0;
}

__global__ void sMatVec_dot_local_block3D_cuda_kernel(
						      type* A,type* x,type* y,type* ret_type,
						      const  int block_m,
						      int *block_nxs, int *block_nxe, 
						      int *block_nys, int *block_nye, 
						      int *block_nzs, int *block_nze,
						      int *stride_xp,int *stride_xm,
						      int *stride_yp,int *stride_ym,
						      int *stride_zp,int *stride_zm,
						      const int nxblock,const int nyblock,const int nzblock,
						      const int mxdivblock,const int mydivblock,const int mzdivblock,
						      const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int ndivblock  = nxdivblock * nydivblock * nzdivblock;
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = ndivblock;
  //  if(tid >= domain) return;
  type val = 0.0;
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    const int jzb =  jb / (nxdivblock*nydivblock);
    const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
    const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;

    const int jjzbs = block_nzs[jzb];
    const int jjzbe = block_nze[jzb];
    const int jjybs = block_nys[jyb];
    const int jjybe = block_nye[jyb];
    const int jjxbs = block_nxs[jxb];
    const int jjxbe = block_nxe[jxb];

    for(int jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
      for(int jjyb=jjybs;jjyb<=jjybe;jjyb++){    
	for(int jjxb=jjxbs;jjxb<=jjxbe;jjxb++){
	  	  
	  const int stxm = stride_xm[jjxb];
	  const int stxp = stride_xp[jjxb];
	  const int stym = stride_ym[jjyb];
	  const int styp = stride_yp[jjyb];
	  const int stzm = stride_zm[jjzb];
	  const int stzp = stride_zp[jjzb];

	  // 内側インデックス
	  const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	

	  const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	
	  const int jcb = j + stzm;
	  const int jcs = j + stym;
	  const int jcw = j + stxm;
	  const int jcc = j       ;
	  const int jce = j + stxp;
	  const int jcn = j + styp;
	  const int jct = j + stzp;
	
#if 1
	  y[j]=
	    A[j   + 0 * block_m]*x[jcb] 
	    +A[j   + 1 * block_m]*x[jcs] 
	    +A[j   + 2 * block_m]*x[jcw] 
	    +A[j   + 3 * block_m]*x[jcc] 
	    +A[jce + 2 * block_m]*x[jce] 
	    +A[jcn + 1 * block_m]*x[jcn] 
	    +A[jct + 0 * block_m]*x[jct] 
	    ;

#else
	  y[j]=
	    A[j + 0 * block_m]*x[jcb] 
	    +A[j + 1 * block_m]*x[jcs] 
	    +A[j + 2 * block_m]*x[jcw] 
	    +A[j + 3 * block_m]*x[jcc] 
	    +A[j + 4 * block_m]*x[jce] 
	    +A[j + 5 * block_m]*x[jcn] 
	    +A[j + 6 * block_m]*x[jct] 
	    ;
#endif
	  val=val+y[j]*x[j];
	} 
      }
    }

  }
  local_sum_cuda(val,ret_type,threadIdx.x);
  return ;
}

int sMatVec_dot_local_block3D_cuda(
				   mpi_prm prm,type* A,type* x,type* y,type *cuda_buf,
				   int *block_nxs, int *block_nxe, 
				   int *block_nys, int *block_nye, 
				   int *block_nzs, int *block_nze,				   
				   int *stride_xp,int *stride_xm,
				   int *stride_yp,int *stride_ym,
				   int *stride_zp,int *stride_zm
				   ){

  int block_m = prm.block_m;
  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;
  //  int ndivblock  = nxdivblock * nydivblock * nzdivblock; 

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;
  //    const int warp_size = 32;
  //    initialize_type_vec<<<1,warp_size>>>(ret_dot,1);
  const int blocks  = 1024;  
  const int threads = 96;  

  const int warp_size = 32;
  initialize_type_vec<<<1,warp_size>>>(cuda_buf,1);
  sMatVec_dot_local_block3D_cuda_kernel<<<blocks,threads>>>(
							    A, x, y, cuda_buf,
							    block_m,
							    block_nxs, block_nxe, 
							    block_nys, block_nye, 
							    block_nzs, block_nze,
							    stride_xp, stride_xm,
							    stride_yp, stride_ym,
							    stride_zp, stride_zm,
							    nxblock,  nyblock,  nzblock,
							    mxdivblock,  mydivblock,  mzdivblock,
							    nxdivblock,  nydivblock,  nzdivblock);

  return 0;
}

