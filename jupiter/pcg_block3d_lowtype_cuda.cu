#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <omp.h> 

#include <cuda_runtime.h>
//#include <cuda_bf16.h>
//#include <cuda_fp16.h>
#include "cublas_v2.h"

#include "struct.h"
#include "cg.h"

#include "pcg_block3d_cuda.h"
#include "sleev_comm_block3d_lowtype_cuda.h"
// #include "pcg_block3d_lowtype_cuda.h"

__global__ void calc_sres_block3D_lowtype_cuda_kernel(
						     low_type* A,
						     low_type* x,
						     type* b,
						     low_type* r,
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

__global__ void calc_res_block3D_lowtype_cuda_kernel(
						     low_type* A,
						     low_type* x,
						     type* b,
						     low_type* r,
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
	
	type val = 
	   A[j + 0 * block_m]*x[jcb] 
	  +A[j + 1 * block_m]*x[jcs] 
	  +A[j + 2 * block_m]*x[jcw] 
	  +A[j + 3 * block_m]*x[jcc] 
	  +A[j + 4 * block_m]*x[jce] 
	  +A[j + 5 * block_m]*x[jcn] 
	  +A[j + 6 * block_m]*x[jct] 
	  ;
	r[j] = b[j] - val;
	} 
      }
    }

  }

  return ;
}

int calc_res_block3D_lowtype_cuda(
		       mpi_prm prm,
		       low_type* A,
		       low_type* x,
		       type* b,
		       low_type* r,
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

  calc_res_block3D_lowtype_cuda_kernel<<<blocks,threads>>>(
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

int calc_sres_block3D_lowtype_cuda(
		       mpi_prm prm,
		       low_type* A,
		       low_type* x,
		       type* b,
		       low_type* r,
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

  calc_sres_block3D_lowtype_cuda_kernel<<<blocks,threads>>>(
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


int zero_initialize_lowtype(int m,low_type *x){
  int i;
#pragma omp parallel for private(i) 
  for(i=0;i<m;i++){ 
    x[i] = 0.0;
  }
  return 0;
}


__global__ void solve_pre_subdividing_mat2_local_ILU_1_block3D_lowtype_cuda_kernel(
								      low_type* A,low_type* Dinv,
								      type *r, low_type *s,type *q,
								      type alpha,
								      type *sr_dot_local,type *rr_dot_local,
								      int *block_nxs, int *block_nxe, 
								      int *block_nys, int *block_nye, 
								      int *block_nzs, int *block_nze,
								      int *stride_xp, int *stride_xm,
								      int *stride_yp, int *stride_ym,
								      int *stride_zp, int *stride_zm,
								      type *block_xfilterL,type *block_xfilterU,
								      type *block_yfilterL,type *block_yfilterU,
								      type *block_zfilterL,type *block_zfilterU,
								      const  int block_m,
								      const int nxblock,const int nyblock,const int nzblock,
								      const int mxdivblock,const int mydivblock,const int mzdivblock,
								      const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int ndivblock  = nxdivblock * nydivblock * nzdivblock;
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = ndivblock;
  //  if(tid >= domain) return;

  type sr_val = 0.0;
  type rr_val = 0.0;
#if precon_BF16
  const int mblock = (SUBDIVIDING_XBLOCKSIZE+2) * (SUBDIVIDING_YBLOCKSIZE+2) * (SUBDIVIDING_ZBLOCKSIZE*2);
#endif
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    const int jzb =  jb / (nxdivblock*nydivblock);
    const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
    const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;

#if precon_BF16
    //@@@@
    /*
    float s_FP32[(SUBDIVIDING_XFP32SIZE+2) * (SUBDIVIDING_YFP32SIZE+2) * (SUBDIVIDING_ZFP32SIZE*2)];
    for(int jj=0;jj<mblock;jj++){
      s_FP32[jj] = 0.0;
    }
    */
#endif
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
	  const int stym = stride_ym[jjyb];
	  const int stzm = stride_zm[jjzb];

	  const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	

	  const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  const int jcb = j + stzm;
	  const int jcs = j + stym;
	  const int jcw = j + stxm;

	  r[j] = r[j] + q[j]*alpha;
#if precon_BF16
	  //@@@
#else
	  s[j] = ( r[j] - ( 
			   A[j + 2*block_m] * s[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
			   A[j + 1*block_m] * s[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
			   A[j + 0*block_m] * s[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
			    ) ) * Dinv[j];
#endif
	} 
      }
    }

    for(int jjzb=jjzbe;jjzb>=jjzbs;jjzb--){
      for(int jjyb=jjybe;jjyb>=jjybs;jjyb--){
	for(int jjxb=jjxbe;jjxb>=jjxbs;jjxb--){
	  const int stxp = stride_xp[jjxb];
	  const int styp = stride_yp[jjyb];
	  const int stzp = stride_zp[jjzb];

	  const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	

	  const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  const int jce = j + stxp;
	  const int jcn = j + styp;
	  const int jct = j + stzp;

#if precon_BF16
	  //@@@
#else
	  s[j] = s[j] - (
			 A[jce + 2*block_m]*s[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			 A[jcn + 1*block_m]*s[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			 A[jct + 0*block_m]*s[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
			 )*Dinv[j];
#endif
	  sr_val = sr_val + s[j]*r[j];
	  rr_val = rr_val + r[j]*r[j];
	} 
      }
    }
  }

  local_sum_cuda(sr_val,sr_dot_local,threadIdx.x);
  local_sum_cuda(rr_val,rr_dot_local,threadIdx.x);
  
  return ;
}

__global__ void solve_pre_subdividing_mat2_local_ILU_2_block3D_lowtype_cuda_kernel(
								      low_type* A,low_type* Dinv,
								      type *r, low_type *s_in, type *s_out,
								      low_type *z,
								      type *sr_dot_local,type *rr_dot_local,
								      int *block_nxs, int *block_nxe, 
								      int *block_nys, int *block_nye, 
								      int *block_nzs, int *block_nze,
								      int *stride_xp, int *stride_xm,
								      int *stride_yp, int *stride_ym,
								      int *stride_zp, int *stride_zm,
								      type *block_xfilterL,type *block_xfilterU,
								      type *block_yfilterL,type *block_yfilterU,
								      type *block_zfilterL,type *block_zfilterU,
								      const  int block_m,
								      const int nxblock,const int nyblock,const int nzblock,
								      const int mxdivblock,const int mydivblock,const int mzdivblock,
								      const int nxdivblock,const int nydivblock,const int nzdivblock){


  const int ndivblock  = nxdivblock * nydivblock * nzdivblock;
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = ndivblock;
  //  if(tid >= domain) return;

  type sr_val = 0.0;
  type rr_val = 0.0;
#if precon_BF16
  const int mblock = (SUBDIVIDING_XBLOCKSIZE+2) * (SUBDIVIDING_YBLOCKSIZE+2) * (SUBDIVIDING_ZBLOCKSIZE*2);
#endif
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
   const int jzb =  jb / (nxdivblock*nydivblock);
   const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
   const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#if precon_BF16
#endif
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
	 const int stym = stride_ym[jjyb];
	 const int stzm = stride_zm[jjzb];
	 //	 const int stxp = stride_xp[jjxb];
	 //	 const int styp = stride_yp[jjyb];
	 //	 const int stzp = stride_zp[jjzb];

	 const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	 const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	

	 const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	 const int jcb = j + stzm;
	 const int jcs = j + stym;
	 const int jcw = j + stxm;

#if precon_BF16
	 //@@@
#else
	 z[j] = ( z[j] - ( 
			  A[j + 2*block_m]*z[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
			  A[j + 1*block_m]*z[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
			  A[j + 0*block_m]*z[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
			   ) ) * Dinv[j];
#endif
       } 
     }
   }

#if precon_BF16
#endif
    
    for(int jjzb=jjzbe;jjzb>=jjzbs;jjzb--){
      for(int jjyb=jjybe;jjyb>=jjybs;jjyb--){
	for(int jjxb=jjxbe;jjxb>=jjxbs;jjxb--){
    
	 const int stxp = stride_xp[jjxb];
	 const int styp = stride_yp[jjyb];
	 const int stzp = stride_zp[jjzb];

	 const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	 const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock); 

	 const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	 const int jce = j + stxp;
	 const int jcn = j + styp;
	 const int jct = j + stzp;

#if precon_BF16
	 //@@@
#else
	 z[j] = z[j] - (
			A[j + 4*block_m]*z[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			A[j + 5*block_m]*z[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			A[j + 6*block_m]*z[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
			)*Dinv[j];
	 s_out[j] = s_in[j] + z[j] ;
#endif 
	 sr_val = sr_val + s_out[j]*r[j];
	 rr_val = rr_val + r[j]*r[j];
       } 
     }
   }
  }

  local_sum_cuda(sr_val,sr_dot_local,threadIdx.x);
  local_sum_cuda(rr_val,rr_dot_local,threadIdx.x);
  return ;

}


