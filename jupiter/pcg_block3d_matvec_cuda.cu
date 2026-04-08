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

#include "pcg_block3d_cuda.h"
#include "sleev_comm_block3d_cuda.h"
#include "pcg_block3d_matvec_cuda.h"

#include "os/os.h"

//#define JUPITER_CUDA_AWARE_MPI 1

__global__ void setEast_axpy_block3D_cuda_kernel(
						type *sb_e ,type *y,type *x,type beta,
					       int *block_nxe, 
					       int *block_nys, int *block_nye,
					       int *block_nzs, int *block_nze,
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
    
    const int jjzbs = block_nzs[jzb];
    const int jjzbe = block_nze[jzb];
    const int jjybs = block_nys[jyb];
    const int jjybe = block_nye[jyb];

    for(int jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
      for(int jjyb=jjybs;jjyb<=jjybe;jjyb++){	  
	
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jyb + jzb*nydivblock + jjyb * (nydivblock*nzdivblock) + jjzb * (nydivblock*nzdivblock*nyblock);
	sb_e[j] =  beta * y[jj] + x[jj];
      }
    }    
  }
  
  return ;
}

__global__ void setWest_axpy_block3D_cuda_kernel(
					       type *sb_w ,type *y,type *x,type beta,
					       int *block_nxs, 
					       int *block_nys, int *block_nye,
					       int *block_nzs, int *block_nze,
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

    const int jjzbs = block_nzs[jzb];
    const int jjzbe = block_nze[jzb];
    const int jjybs = block_nys[jyb];
    const int jjybe = block_nye[jyb];

    for(int jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
      for(int jjyb=jjybs;jjyb<=jjybe;jjyb++){
	
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jyb + jzb*nydivblock + jjyb * (nydivblock*nzdivblock) + jjzb * (nydivblock*nzdivblock*nyblock);
	sb_w[j]  =  beta * y[jj] + x[jj];;
      }
    }
  }

  return ;
}

__global__ void setNorth_axpy_block3D_cuda_kernel(
					       type *sb_n ,type *y,type *x,type beta,
					       int *block_nye,
					       int *block_nxs, int *block_nxe,
					       int *block_nzs, int *block_nze,
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
    
    const int jjzbs = block_nzs[jzb];
    const int jjzbe = block_nze[jzb];
    const int jjxbs = block_nxs[jxb];
    const int jjxbe = block_nxe[jxb];

    for(int jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
      for(int jjxb=jjxbs;jjxb<=jjxbe;jjxb++){	  
	
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	sb_n[j] = beta * y[jj] + x[jj];
      }
    }
  }

  return ;
}

__global__ void setSouth_axpy_block3D_cuda_kernel(
					       type *sb_s, type *y,type *x,type beta,
					       int *block_nys, 
					       int *block_nxs, int *block_nxe,
					       int *block_nzs, int *block_nze,
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

    const int jjzbs = block_nzs[jzb];
    const int jjzbe = block_nze[jzb];
    const int jjxbs = block_nxs[jxb];
    const int jjxbe = block_nxe[jxb];

    for(int jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
      for(int jjxb=jjxbs;jjxb<=jjxbe;jjxb++){	  
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	sb_s[j] =  beta * y[jj] + x[jj];
      }
    }
  }

  return ;
}

__global__ void setTop_axpy_block3D_cuda_kernel(
					       type *sb_t ,type *y,type *x,type beta,
					       int *block_nze,
					       int *block_nxs, int *block_nxe,
					       int *block_nys, int *block_nye,
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

    const int jjybs = block_nys[jyb];
    const int jjybe = block_nye[jyb];
    const int jjxbs = block_nxs[jxb];
    const int jjxbe = block_nxe[jxb];

    for(int jjyb=jjybs;jjyb<=jjybe;jjyb++){    
      for(int jjxb=jjxbs;jjxb<=jjxbe;jjxb++){	  
	
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jxb + jyb*nxdivblock + jjxb * (nxdivblock*nydivblock) + jjyb * (nxdivblock*nydivblock*nxblock);
	sb_t[j] =  beta * y[jj] + x[jj];
      }
    }
  }

  return ;
}

__global__ void setBottom_axpy_block3D_cuda_kernel(
					       type *sb_b, type *y,type *x,type beta,
					       int *block_nzs,
					       int *block_nxs, int *block_nxe,
					       int *block_nys, int *block_nye,
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

    const int jjybs = block_nys[jyb];
    const int jjybe = block_nye[jyb];
    const int jjxbs = block_nxs[jxb];
    const int jjxbe = block_nxe[jxb];

    for(int jjyb=jjybs;jjyb<=jjybe;jjyb++){    
      for(int jjxb=jjxbs;jjxb<=jjxbe;jjxb++){	  
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int j   = jxb + jyb*nxdivblock + jjxb * (nxdivblock*nydivblock) + jjyb * (nxdivblock*nydivblock*nxblock);
	sb_b[j] =  beta * y[jj] + x[jj];
      }
    }
  }

  return ;
}

__global__ void sMatVec_dot_local_block3D_unable_bound_cuda_kernel(
						     type* A,type* x,type* y,type* ret_type,
						     const  int block_m,
						     int *block_nxs, int *block_nxe, 
						     int *block_nys, int *block_nye, 
						     int *block_nzs, int *block_nze, 
						     int *stride_xp,int *stride_xm,
						     int *stride_yp,int *stride_ym,
						     int *stride_zp,int *stride_zm,
						     type *block_West_filter,type *block_East_filter,
						     type *block_South_filter,type *block_North_filter,
						     type *block_Bottom_filter,type *block_Top_filter,
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
	// 現在の実装はx方向のオーバーラップをしないためx方向のフィルターを無効にしている
	y[j]=
	     A[j   + 0 * block_m]*x[jcb] * block_Bottom_filter[ jjzb + jzb*nzblock ] 
	    +A[j   + 1 * block_m]*x[jcs] * block_South_filter[ jjyb + jyb*nyblock ]
	    +A[j   + 2 * block_m]*x[jcw] // * block_West_filter[ jjxb + jxb*nxblock ] 
	    +A[j   + 3 * block_m]*x[jcc] 
	    +A[jce + 2 * block_m]*x[jce] // * block_East_filter[ jjxb + jxb*nxblock ] 
	    +A[jcn + 1 * block_m]*x[jcn] * block_North_filter[ jjyb + jyb*nyblock ]
	    +A[jct + 0 * block_m]*x[jct] * block_Top_filter[ jjzb + jzb*nzblock ]
	    ;
	    val = val + y[j]*x[j] * (
				     block_Bottom_filter[ jjzb + jzb*nzblock ]
				   * block_South_filter[ jjyb + jyb*nyblock ]
				     //				   * block_West_filter[ jjxb + jxb*nxblock ] 
				     //				   * block_East_filter[ jjxb + jxb*nxblock ] 
				   * block_North_filter[ jjyb + jyb*nyblock ]
				   * block_Top_filter[ jjzb + jzb*nzblock ]
				   );
#else	
	// 現在の実装はx方向のオーバーラップをしないためx方向のフィルターを無効にしている
	y[j]=
	     A[j   + 0 * block_m]*x[jcb] * block_Bottom_filter[ jjzb + jzb*nzblock ] 
	    +A[j   + 1 * block_m]*x[jcs] * block_South_filter[ jjyb + jyb*nyblock ]
	    +A[j   + 2 * block_m]*x[jcw] // * block_West_filter[ jjxb + jxb*nxblock ] 
	    +A[j   + 3 * block_m]*x[jcc] 
	    +A[jce + 2 * block_m]*x[jce] // * block_East_filter[ jjxb + jxb*nxblock ] 
	    +A[jcn + 1 * block_m]*x[jcn] * block_North_filter[ jjyb + jyb*nyblock ]
	    +A[jct + 0 * block_m]*x[jct] * block_Top_filter[ jjzb + jzb*nzblock ]
	    ;
	    val = val + y[j]*x[j] * (
				     block_Bottom_filter[ jjzb + jzb*nzblock ]
				   * block_South_filter[ jjyb + jyb*nyblock ]
				     //				   * block_West_filter[ jjxb + jxb*nxblock ] 
				     //				   * block_East_filter[ jjxb + jxb*nxblock ] 
				   * block_North_filter[ jjyb + jyb*nyblock ]
				   * block_Top_filter[ jjzb + jzb*nzblock ]
				   );
#endif
	    
	} 
      }
    }

  }
  local_sum_cuda(val,ret_type,threadIdx.x);
  return ;
}

__global__ void sMatVec_dot_local_block3D_able_North_cuda_kernel(
						     type* A,type* x,type* y,type* ret_type,
						     type* rb_n,						     
						     const  int block_m,
						     int *block_nye,
						     int *block_nxs, int *block_nxe, 
						     int *block_nzs, int *block_nze,  
						     int *stride_yp,						     
						     type *block_Bottom_filter,type *block_Top_filter,
						     const int nxblock,const int nyblock,const int nzblock,
						     const int mxdivblock,const int mydivblock,const int mzdivblock,
						     const int nxdivblock,const int nydivblock,const int nzdivblock){
  
  type val = 0.0;
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nzdivblock*nxdivblock;
  if(tid >= domain) return;

  int jyb ;
  int jjyb;

  jyb  = nydivblock-1;
  jjyb = block_nye[nydivblock-1];
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jzb = (jb%(nxdivblock*nzdivblock))/nxdivblock;
    int jxb = (jb%(nxdivblock*nzdivblock))%nxdivblock ;
    
    const int jjzbs = block_nzs[jzb];
    const int jjzbe = block_nze[jzb];
    const int jjxbs = block_nxs[jxb];
    const int jjxbe = block_nxe[jxb];
    for(int jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
      for(int jjxb=jjxbs;jjxb<=jjxbe;jjxb++){
	
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int jj = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);

	const int styp = stride_yp[jjyb];	  
	const int jcn = j + styp;
	y[j] = y[j] + A[jcn + 1 * block_m] * rb_n[jj] ;
	val = val+y[j]*x[j]
	    * block_Top_filter[jjzb + jzb*nzblock]
	    * block_Bottom_filter[jjzb + jzb*nzblock] ;
      }
    }
  }
  
  local_sum_cuda(val,ret_type,threadIdx.x);
  return ;

}

__global__ void sMatVec_dot_local_block3D_able_South_cuda_kernel(
						     type* A,type* x,type* y,type* ret_type,
						     type* rb_s,
						     const  int block_m,  
						     int *block_nys,
						     int *block_nxs, int *block_nxe, 
						     int *block_nzs, int *block_nze,    
						     int *stride_ym,
						     type *block_Bottom_filter,type *block_Top_filter,
						     const int nxblock,const int nyblock,const int nzblock,
						     const int mxdivblock,const int mydivblock,const int mzdivblock,
						     const int nxdivblock,const int nydivblock,const int nzdivblock){
  type val = 0.0;  
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

    const int jjzbs = block_nzs[jzb];
    const int jjzbe = block_nze[jzb];
    const int jjxbs = block_nxs[jxb];
    const int jjxbe = block_nxe[jxb];
    for(int jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
      for(int jjxb=jjxbs;jjxb<=jjxbe;jjxb++){
	
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int jj = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);

	const int stym = stride_ym[jjyb];
	y[j] = y[j] + A[j   + 1 * block_m] * rb_s[jj] ;
	val = val+y[j]*x[j]
	  * block_Top_filter[jjzb + jzb*nzblock]
	  * block_Bottom_filter[jjzb + jzb*nzblock] ;
      }
    }
  }  
  
  local_sum_cuda(val,ret_type,threadIdx.x);
  
  return ;

}

__global__ void sMatVec_dot_local_block3D_able_Bottom_cuda_kernel(
						     type* A,type* x,type* y,type* ret_type,
						     type* rb_b,
						     const  int block_m,
						     int *block_nzs,
						     int *block_nxs, int *block_nxe, 
						     int *block_nys, int *block_nye,
  						     int *stride_zm,
						     const int nxblock,const int nyblock,const int nzblock,
						     const int mxdivblock,const int mydivblock,const int mzdivblock,
						     const int nxdivblock,const int nydivblock,const int nzdivblock){
  type val = 0.0;
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nxdivblock * nydivblock;
  if(tid >= domain) return;
  int jzb,jjzb;
  jzb  = 0;
  jjzb = block_nzs[0];
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jyb  = (jb%(nxdivblock*nydivblock))/nxdivblock;
    int jxb  = (jb%(nxdivblock*nydivblock))%nxdivblock ;

    const int jjybs = block_nys[jyb];
    const int jjybe = block_nye[jyb];
    const int jjxbs = block_nxs[jxb];
    const int jjxbe = block_nxe[jxb];
    for(int jjyb=jjybs;jjyb<=jjybe;jjyb++){    
      for(int jjxb=jjxbs;jjxb<=jjxbe;jjxb++){
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int jj = jxb + jyb*nxdivblock + jjxb * (nxdivblock*nydivblock) + jjyb * (nxdivblock*nydivblock*nxblock);
	
	const int stzm = stride_zm[jjzb];
	const int jcb = j + stzm;
	y[j] = y[j] + A[j   + 0 * block_m] * rb_b[jj] ;
	val = val+y[j]*x[j];
      }
    }
  }
   
  local_sum_cuda(val,ret_type,threadIdx.x);
  
  return ;

}

__global__ void sMatVec_dot_local_block3D_able_Top_cuda_kernel(
						     type* A,type* x,type* y,type* ret_type,
						     type* rb_t,						     
						     const  int block_m,
						     int *block_nze,
						     int *block_nxs, int *block_nxe, 
						     int *block_nys, int *block_nye,
						     int *stride_zp,
						     const int nxblock,const int nyblock,const int nzblock,
						     const int mxdivblock,const int mydivblock,const int mzdivblock,
						     const int nxdivblock,const int nydivblock,const int nzdivblock){

  type val = 0.0;
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nxdivblock * nydivblock;

  if(tid >= domain) return;

  int jzb,jjzb;
  jzb  = nzdivblock-1;
  jjzb = block_nze[nzdivblock-1];
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    int jyb  = (jb%(nxdivblock*nydivblock))/nxdivblock;
    int jxb  = (jb%(nxdivblock*nydivblock))%nxdivblock ;

    const int jjybs = block_nys[jyb];
    const int jjybe = block_nye[jyb];
    const int jjxbs = block_nxs[jxb];
    const int jjxbe = block_nxe[jxb];
    for(int jjyb=jjybs;jjyb<=jjybe;jjyb++){    
      for(int jjxb=jjxbs;jjxb<=jjxbe;jjxb++){
	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int jj   = jxb + jyb*nxdivblock + jjxb * (nxdivblock*nydivblock) + jjyb * (nxdivblock*nydivblock*nxblock);
	// sb_t[j] = f[jj];
	const int stzp = stride_zp[jjzb];	
	const int jct = j + stzp;	
	//	y[j] = y[j] + A[jct + 0 * block_m]*x[jct] ;
		y[j] = y[j] + A[jct + 0 * block_m]* rb_t[jj] ;
	val = val+y[j]*x[j];
	
      }
    }
  }

  local_sum_cuda(val,ret_type,threadIdx.x);

  return ;

}

int axpy_sMatVec_dot_OVL_block3D_cuda(
		       mpi_prm *prm,
		       type alpha,type beta,
		       type *A,
		       type *x,type *y,type *z,type *q,
		       int *block_nxs, int *block_nxe, 
		       int *block_nys, int *block_nye, 
		       int *block_nzs, int *block_nze,
		       int *stride_xp,int *stride_xm,
		       int *stride_yp,int *stride_ym,
		       int *stride_zp,int *stride_zm,
		       type *block_West_filter,type *block_East_filter,
		       type *block_South_filter,type *block_North_filter,
		       type *block_Bottom_filter,type *block_Top_filter,
		       type *cuda_buf,
		       type *cpu_buf,
		       type *sb_b ,type *rb_b ,type *sb_t ,type *rb_t ,
		       type *sb_s ,type *rb_s ,type *sb_n ,type *rb_n ,
		       type *sb_w ,type *rb_w ,type *sb_e ,type *rb_e ,
		       cudaStream_t *cudaStream_Hi_Priority ,
		       cudaStream_t *cudaStream_Low_Priority ,
		       cudaEvent_t *cudaEvent_OVL
				      ){
  double ts,te;
  type *time = cpu_buf;
  int block_m = prm[0].block_m;

  int nzblock    = prm[0].nzblock;
  int nyblock    = prm[0].nyblock;
  int nxblock    = prm[0].nxblock;

  int mzdivblock = prm[0].mzdivblock;
  int mydivblock = prm[0].mydivblock;
  int mxdivblock = prm[0].mxdivblock;

  int nzdivblock = prm[0].nzdivblock;
  int nydivblock = prm[0].nydivblock;
  int nxdivblock = prm[0].nxdivblock;

#if 0
  const int blocks  = 32;  
  const int threads = 64;
#else
  const int blocks  = 1024;  
  const int threads = 96;
#endif
  
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

  type *ret_dot_core   = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*4 + 0];
  type *ret_dot_Bottom = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*4 + 1];
  type *ret_dot_Top    = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*4 + 2];
  type *ret_dot_South  = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*4 + 3];
  type *ret_dot_North  = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*4 + 4];
  type *ret_dot_West   = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*4 + 5];
  type *ret_dot_East   = &cuda_buf[block_nxy*4 + block_nxz*4 + block_nyz*4 + 6];

  const int warp_size = 32;
  initialize_type_vec<<<1,warp_size>>>(ret_dot_core,7);
  cudaDeviceSynchronize();
  ts=cpu_time();

  // ----------------------------------------------------
  // west (x-)
  setWest_axpy_block3D_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[4]>>>(
									      cu_sb_w, y,x,beta,
					       block_nxs, 
					       block_nys, block_nye,
					       block_nzs, block_nze,
									      nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);

  // east (x+)
  setEast_axpy_block3D_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[5]>>>(
					            cu_sb_e ,y,x,beta,
					       block_nxe, 
					       block_nys, block_nye,
					       block_nzs, block_nze,
						    nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);

  // south (y-)
  setSouth_axpy_block3D_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[2]>>>(
							   cu_sb_s, y,x,beta,
					       block_nys, 
					       block_nxs, block_nxe,
					       block_nzs, block_nze,
							   nxblock, nyblock, nzblock,
							   mxdivblock, mydivblock, mzdivblock,
							   nxdivblock, nydivblock, nzdivblock);
  
  // north (y+)
  setNorth_axpy_block3D_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[3]>>>(
							   cu_sb_n ,y,x,beta,
					       block_nye,
					       block_nxs, block_nxe,
					       block_nzs, block_nze,
							   nxblock, nyblock, nzblock,
							   mxdivblock, mydivblock, mzdivblock,
							   nxdivblock, nydivblock, nzdivblock);

  // bottom (z-)
  setBottom_axpy_block3D_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[0]>>>(
							  cu_sb_b ,y,x,beta,
					       block_nzs,
					       block_nxs, block_nxe,
					       block_nys, block_nye,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);

  // top    (z+)
  setTop_axpy_block3D_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[1]>>>(
							  cu_sb_t ,y,x,beta,
					       block_nze,
					       block_nxs, block_nxe,
					       block_nys, block_nye,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);
   
  // ----------------------------------------------------
#if  JUPITER_CUDA_AWARE_MPI
  cudaStreamSynchronize(cudaStream_Hi_Priority[4]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[5]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[2]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[3]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[0]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[1]);
#else
  cudaStreamSynchronize(cudaStream_Hi_Priority[4]);
  cudaMemcpyAsync(sb_w, cu_sb_w, sizeof(type)*block_nyz, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[4]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[5]);
  cudaMemcpyAsync(sb_e, cu_sb_e, sizeof(type)*block_nyz, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[5]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[2]);
  cudaMemcpyAsync(sb_s,cu_sb_s, sizeof(type)*block_nxz, cudaMemcpyDeviceToHost,cudaStream_Low_Priority[2]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[3]);
  cudaMemcpyAsync(sb_n,cu_sb_n, sizeof(type)*block_nxz, cudaMemcpyDeviceToHost,cudaStream_Low_Priority[3]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[0]);
  cudaMemcpyAsync(sb_b, cu_sb_b, sizeof(type)*block_nxy, cudaMemcpyDeviceToHost,cudaStream_Low_Priority[0]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[1]);
  cudaMemcpyAsync(sb_t, cu_sb_t, sizeof(type)*block_nxy, cudaMemcpyDeviceToHost,cudaStream_Low_Priority[1]);
#endif  
  te=cpu_time();
  time[0]=time[0]+te-ts;
  ts=cpu_time();
  
  axpy2_block3D_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[6]>>>(
						alpha, beta, x, y, z,
						     block_nxs, block_nxe, 
						     block_nys, block_nye, 
						     block_nzs, block_nze, 
						nxblock,nyblock,nzblock,
						mxdivblock,mydivblock,mzdivblock,
						nxdivblock,nydivblock,nzdivblock);
    
  // -----------------------------------------------------  
  cudaStreamSynchronize(cudaStream_Hi_Priority[4]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[5]);
  
  te=cpu_time();
  time[1]=time[1]+te-ts;
  ts=cpu_time();
    
#if  JUPITER_CUDA_AWARE_MPI
  sleev_comm_block3D_icommWest_cuda(prm,cu_sb_w,cu_rb_w);				
  sleev_comm_block3D_icommEast_cuda(prm,cu_sb_e,cu_rb_e);
#else
  sleev_comm_block3D_icommWest_cuda(prm,sb_w,rb_w);				
  sleev_comm_block3D_icommEast_cuda(prm,sb_e,rb_e);
#endif  
  // west (x-)
  sleev_comm_block3D_waitWest_cuda(prm);
#if  JUPITER_CUDA_AWARE_MPI
#else
  cudaMemcpyAsync(cu_rb_w, rb_w, sizeof(type)*block_nyz, cudaMemcpyHostToDevice,cudaStream_Low_Priority[6]);  
#endif
  readBackWest_block3D_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[6]>>>(
							 cu_rb_w, y,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);
  
  // east (x+)  
  sleev_comm_block3D_waitEast_cuda(prm);
#if  JUPITER_CUDA_AWARE_MPI
#else
  cudaMemcpyAsync(cu_rb_e, rb_e, sizeof(type)*block_nyz, cudaMemcpyHostToDevice,cudaStream_Low_Priority[6]);
#endif
  readBackEast_block3D_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[6]>>>(
							 cu_rb_e ,y,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);
  
  te=cpu_time();
  time[2]=time[2]+te-ts;
  ts=cpu_time();  
  
  // -----------------------------------------------------
  sMatVec_dot_local_block3D_unable_bound_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[6]>>>(
										       A, y, q, ret_dot_core,
										       block_m,
						     block_nxs, block_nxe, 
						     block_nys, block_nye, 
						     block_nzs, block_nze, 
									    	       stride_xp, stride_xm,
								     		       stride_yp, stride_ym,
							      			       stride_zp, stride_zm,
							    block_West_filter,block_East_filter,
							    block_South_filter,block_North_filter,
							    block_Bottom_filter,block_Top_filter,
					       					       nxblock,  nyblock,  nzblock,
						       				       mxdivblock,  mydivblock,  mzdivblock,
				       						       nxdivblock,  nydivblock,  nzdivblock);
  // -----------------------------------------------------  
  // south (y-)
  // north (y+)
  cudaStreamSynchronize(cudaStream_Low_Priority[2]);  
  cudaStreamSynchronize(cudaStream_Low_Priority[3]);  
  te=cpu_time();
  time[3]=time[4]+te-ts;
  ts=cpu_time();
#if  JUPITER_CUDA_AWARE_MPI
  sleev_comm_block3D_icommSouth_cuda(prm,cu_sb_s,cu_rb_s);
  sleev_comm_block3D_icommNorth_cuda(prm,cu_sb_n,cu_rb_n);
#else
  sleev_comm_block3D_icommSouth_cuda(prm,sb_s,rb_s);
  sleev_comm_block3D_icommNorth_cuda(prm,sb_n,rb_n);
#endif

#if  JUPITER_CUDA_AWARE_MPI
  sleev_comm_block3D_waitSouth_cuda(prm);
  sleev_comm_block3D_waitNorth_cuda(prm);
#else
  sleev_comm_block3D_waitSouth_cuda(prm);
  cudaMemcpyAsync(cu_rb_s,rb_s,sizeof(type)*block_nxz, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[2]);  
  sleev_comm_block3D_waitNorth_cuda(prm);
  cudaMemcpyAsync(cu_rb_n,rb_n,sizeof(type)*block_nxz, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[3]);
#endif  
  te=cpu_time();
  time[4]=time[4]+te-ts;
  ts=cpu_time();  

  // -----------------------------------------------------  
  // bottom (z-)
  // top    (z+)  
  cudaStreamSynchronize(cudaStream_Low_Priority[0]);  
  cudaStreamSynchronize(cudaStream_Low_Priority[1]);  
  te=cpu_time();
  time[5]=time[5]+te-ts;
  ts=cpu_time();

#if  JUPITER_CUDA_AWARE_MPI
  sleev_comm_block3D_icommBottom_cuda(prm,cu_sb_b,cu_rb_b);
  sleev_comm_block3D_icommTop_cuda(prm,cu_sb_t,cu_rb_t);
#else
  sleev_comm_block3D_icommBottom_cuda(prm,sb_b,  rb_b);
  sleev_comm_block3D_icommTop_cuda(prm,sb_t,  rb_t);
#endif
  
#if  JUPITER_CUDA_AWARE_MPI
  sleev_comm_block3D_waitBottom_cuda(prm);
  sleev_comm_block3D_waitTop_cuda(prm);
#else
  sleev_comm_block3D_waitBottom_cuda(prm);
  cudaMemcpyAsync(cu_rb_b, rb_b, sizeof(type)*block_nxy, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[0]);  
  sleev_comm_block3D_waitTop_cuda(prm);
  cudaMemcpyAsync(cu_rb_t, rb_t, sizeof(type)*block_nxy, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[1]);
#endif
  
  te=cpu_time();
  time[6]=time[6]+te-ts;
  ts=cpu_time();
    
  cudaStreamSynchronize(cudaStream_Hi_Priority[2]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[3]);
  te=cpu_time();
  time[7]=time[7]+te-ts;
  ts=cpu_time();
  
  // -------------------------------------------------------
  // south (y-)
  sMatVec_dot_local_block3D_able_South_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[6]>>>(
							    A, y, q, ret_dot_South,
							    cu_rb_s,
							    block_m,
							    block_nys,
						block_nxs,block_nxe,
						block_nzs,block_nze,
							    stride_ym,
							    block_Bottom_filter,block_Top_filter,
							    nxblock,  nyblock,  nzblock,
							    mxdivblock,  mydivblock,  mzdivblock,
							    nxdivblock,  nydivblock,  nzdivblock);
  // north (y+)
  sMatVec_dot_local_block3D_able_North_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[6]>>>(
							    A, y, q, ret_dot_North,
							    cu_rb_n, 
							    block_m,
							    block_nye,
						block_nxs,block_nxe,
						block_nzs,block_nze,
							    stride_yp,						     
							    block_Bottom_filter, block_Top_filter,
							    nxblock,  nyblock,  nzblock,
							    mxdivblock,  mydivblock,  mzdivblock,
							    nxdivblock,  nydivblock,  nzdivblock);
  cudaStreamSynchronize(cudaStream_Hi_Priority[0]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[1]);
  te=cpu_time();
  time[8]=time[8]+te-ts;
  ts=cpu_time();
    
  // bottom (z-)
  sMatVec_dot_local_block3D_able_Bottom_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[6]>>>(
							    A, y, q, ret_dot_Bottom,
							    cu_rb_b,
							    block_m,
							    block_nzs,
						block_nxs,block_nxe,
						block_nys,block_nye,
							    stride_zm,
							    nxblock,  nyblock,  nzblock,
							    mxdivblock,  mydivblock,  mzdivblock,
							    nxdivblock,  nydivblock,  nzdivblock);
  // top    (z+)
  sMatVec_dot_local_block3D_able_Top_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[6]>>>(
							    A, y, q, ret_dot_Top,
		       								 cu_rb_t,     
			       	       						 block_m,
		       	       							 block_nze,
						block_nxs,block_nxe,
						block_nys,block_nye,
					       					 stride_zp,
							    nxblock,  nyblock,  nzblock,
							    mxdivblock,  mydivblock,  mzdivblock,
							    nxdivblock,  nydivblock,  nzdivblock);
    
  cudaDeviceSynchronize();
  te=cpu_time();
  time[9]=time[9]+te-ts;
  ts=cpu_time();
  
  type *ret_dot = &cuda_buf[0];
  initialize_type_vec<<<1,warp_size>>>(ret_dot,1);
  sum_vec_local_cuda_kernel<<<1,warp_size>>>(ret_dot_core,ret_dot,7);
  
  cudaDeviceSynchronize();
  te=cpu_time();
  time[10]=time[10]+te-ts;
  ts=cpu_time();
  
  return 0;
}

