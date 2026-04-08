#include <cuda_runtime.h>

//#include <cuda_bf16.h>
//#include <cuda_fp16.h>
#include "cublas_v2.h"

#include "struct.h"
#include "func.h"

#include "jupiter/os/os.h"

#include "cg.h"

#include "pcg_block3d_cuda.h"
#include "sleev_comm_block3d_lowtype_cuda.h"
#include "pcg_block3d_precon_lowtype_cuda.h"
#include "pcg_block3d_lowtype_cuda.h"
#include "sleev_comm_block3d_cuda.h"
#include "pcg_block3d_res_lowtype.h"

#define ILU_LOCAL_BUF 0

//#define JUPITER_CUDA_AWARE_MPI 1

__global__ void solve_pre_subdividing_mat2_local_ILU_1_block3D_core_lowtype_cuda_kernel(
								      low_type* A,low_type* Dinv,
								      type *r, low_type *s,type *q,
								      type alpha,
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

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int nzdivblock_core = nzdivblock-2;
  const int nydivblock_core = nydivblock;
  const int nxdivblock_core = nxdivblock;
  const int ncoreloop = nxdivblock_core * nydivblock_core * nzdivblock_core;
  const int domain = ncoreloop;
  
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    const int jzb =  jb / (nxdivblock_core*nydivblock_core) + 1;
    const int jyb = (jb%(nxdivblock_core*nydivblock_core))/nxdivblock_core ;
    const int jxb = (jb%(nxdivblock_core*nydivblock_core))%nxdivblock_core ;
    
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
	  s[j] = ( r[j] - ( 
			   A[j + 2*block_m] * s[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
			   A[j + 1*block_m] * s[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
			   A[j + 0*block_m] * s[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
			    ) ) * Dinv[j];
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

	  s[j] = s[j] - (
			 A[jce + 2*block_m]*s[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			 A[jcn + 1*block_m]*s[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			 A[jct + 0*block_m]*s[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
			 )*Dinv[j];
	} 
      }
    }
  }
  
  return ;
}

__global__ void solve_pre_subdividing_mat2_local_ILU_1_block3D_South_lowtype_cuda_kernel(
								      low_type* A,low_type* Dinv,
								      type *r, low_type *s,type *q,
								      type alpha,
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

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int nzdivblock_core = nzdivblock-2;
  const int nNorthSouth = nxdivblock * nzdivblock_core;
  const int domain = nNorthSouth;
  
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    const int jzb = (jb%(nxdivblock*nzdivblock_core))/nxdivblock + 1;    
    const int jyb = 0;
    const int jxb = (jb%(nxdivblock*nzdivblock_core))%nxdivblock;

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
	  s[j] = ( r[j] - ( 
			   A[j + 2*block_m] * s[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
			   A[j + 1*block_m] * s[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
			   A[j + 0*block_m] * s[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
			    ) ) * Dinv[j];
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

	  s[j] = s[j] - (
			 A[jce + 2*block_m]*s[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			 A[jcn + 1*block_m]*s[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			 A[jct + 0*block_m]*s[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
			 )*Dinv[j];
	} 
      }
    }
  }
  
  return ;
}

__global__ void solve_pre_subdividing_mat2_local_ILU_1_block3D_North_lowtype_cuda_kernel(
								      low_type* A,low_type* Dinv,
								      type *r, low_type *s,type *q,
								      type alpha,
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

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int nzdivblock_core = nzdivblock-2;
  const int nNorthSouth = nxdivblock * nzdivblock_core;
  const int domain = nNorthSouth;
  
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    const int jzb = (jb%(nxdivblock*nzdivblock_core))/nxdivblock + 1;    
    const int jyb = nydivblock-1;
    const int jxb = (jb%(nxdivblock*nzdivblock_core))%nxdivblock;

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
	  s[j] = ( r[j] - ( 
			   A[j + 2*block_m] * s[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
			   A[j + 1*block_m] * s[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
			   A[j + 0*block_m] * s[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
			    ) ) * Dinv[j];
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

	  s[j] = s[j] - (
			 A[jce + 2*block_m]*s[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			 A[jcn + 1*block_m]*s[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			 A[jct + 0*block_m]*s[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
			 )*Dinv[j];
	} 
      }
    }
  }
  
  return ;
}

__global__ void solve_pre_subdividing_mat2_local_ILU_1_block3D_Bottom_lowtype_cuda_kernel(
								      low_type* A,low_type* Dinv,
								      type *r, low_type *s,type *q,
								      type alpha,
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

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int nTopBottom  = nxdivblock * nydivblock;
  const int domain = nTopBottom;
  
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    const int jzb = 0;
    const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock ;
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
	  const int stym = stride_ym[jjyb];
	  const int stzm = stride_zm[jjzb];

	  const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	

	  const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  const int jcb = j + stzm;
	  const int jcs = j + stym;
	  const int jcw = j + stxm;

	  r[j] = r[j] + q[j]*alpha;
	  s[j] = ( r[j] - ( 
			   A[j + 2*block_m] * s[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
			   A[j + 1*block_m] * s[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
			   A[j + 0*block_m] * s[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
			    ) ) * Dinv[j];
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

	  s[j] = s[j] - (
			 A[jce + 2*block_m]*s[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			 A[jcn + 1*block_m]*s[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			 A[jct + 0*block_m]*s[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
			 )*Dinv[j];
	} 
      }
    }
  }
  
  return ;
}

__global__ void solve_pre_subdividing_mat2_local_ILU_1_block3D_Top_lowtype_cuda_kernel(
								      low_type* A,low_type* Dinv,
								      type *r, low_type *s,type *q,
								      type alpha,
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

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int nTopBottom  = nxdivblock * nydivblock;
  const int domain = nTopBottom;
  
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    const int jzb = nzdivblock-1;
    const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock ;
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
	  const int stym = stride_ym[jjyb];
	  const int stzm = stride_zm[jjzb];

	  const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	

	  const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  const int jcb = j + stzm;
	  const int jcs = j + stym;
	  const int jcw = j + stxm;

	  r[j] = r[j] + q[j]*alpha;
	  s[j] = ( r[j] - ( 
			   A[j + 2*block_m] * s[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
			   A[j + 1*block_m] * s[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
			   A[j + 0*block_m] * s[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
			    ) ) * Dinv[j];
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

	  s[j] = s[j] - (
			 A[jce + 2*block_m]*s[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			 A[jcn + 1*block_m]*s[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			 A[jct + 0*block_m]*s[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
			 )*Dinv[j];
	} 
      }
    }
  }
  
  return ;
}


__global__ void calc_res_solve_pre_subdividing_mat2_local_ILU_2_block3D_lowtype_cuda_kernel(
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

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int nzdivblock_core = nzdivblock;
  const int nydivblock_core = nydivblock;
  const int nxdivblock_core = nxdivblock;
  const int ncoreloop = nxdivblock_core * nydivblock_core * nzdivblock_core;
  const int domain = ncoreloop;
  //  const int ndivblock  = nxdivblock * nydivblock * nzdivblock;
  type sr_val = 0.0;
  type rr_val = 0.0;    

#if ILU_LOCAL_BUF
  const int mxblock = nxblock + 2;
  const int myblock = nyblock + 2;
  const int mzblock = nzblock + 2;
  const int mxyblock = mxblock*myblock;
  const int mblock = mxblock * myblock * mzblock;
#endif

  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
#if ILU_LOCAL_BUF
    type z_local[ (SUBDIVIDING_XBLOCKSIZE + 2) * (SUBDIVIDING_YBLOCKSIZE + 2) * (SUBDIVIDING_ZBLOCKSIZE + 2) ];
    for(int jj=0;jj<mblock;jj++){
      z_local[jj] = 0.0;
    }
#endif        
    const int jzb =  jb / (nxdivblock_core*nydivblock_core);
    const int jyb = (jb%(nxdivblock_core*nydivblock_core))/nxdivblock_core ;
    const int jxb = (jb%(nxdivblock_core*nydivblock_core))%nxdivblock_core ;

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
	 const int stxp = stride_xp[jjxb];
	 const int styp = stride_yp[jjyb];
	 const int stzp = stride_zp[jjzb];

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
	     A[j   + 0 * block_m]*s_in[jcb] 
	    +A[j   + 1 * block_m]*s_in[jcs] 
	    +A[j   + 2 * block_m]*s_in[jcw] 
	    +A[j   + 3 * block_m]*s_in[jcc] 
	    +A[jce + 2 * block_m]*s_in[jce] 
	    +A[jcn + 1 * block_m]*s_in[jcn] 
	    +A[jct + 0 * block_m]*s_in[jct] 
	    ;
	  type res = r[j] - val;
#if ILU_LOCAL_BUF
	  const int jj = (jjxb+1) + (jjyb+1)*mxblock + (jjzb+1)*mxyblock;
	  const int jjcb = jj - mxyblock;
	  const int jjcs = jj - mxblock;
	  const int jjcw = jj - 1;
	  //	  const int jjce = jj + 1;
	  //	  const int jjcn = jj + mxblock;
	  //	  const int jjct = jj + mxyblock;
	  z_local[jj] = ( res - ( 
				  A[j + 2*block_m]*z_local[jjcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
				  A[j + 1*block_m]*z_local[jjcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
				  A[j + 0*block_m]*z_local[jjcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
				   ) ) * Dinv[j];  
#else	  
	  z[j] = ( res - ( 
				  A[j + 2*block_m]*z[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
				  A[j + 1*block_m]*z[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
				  A[j + 0*block_m]*z[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
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

#if ILU_LOCAL_BUF
	  const int jj = (jjxb+1) + (jjyb+1)*mxblock + (jjzb+1)*mxyblock;
	  //	  const int jjcb = jjb - mxyblock;
	  //	  const int jjcs = jjb - mxblock;
	  //	  const int jjcw = jjb - 1;
	  	  const int jjce = jj + 1;
	  	  const int jjcn = jj + mxblock;
	  	  const int jjct = jj + mxyblock;
	 z_local[jj] = z_local[jj] - (
			A[jce + 2*block_m]*z_local[jjce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			A[jcn + 1*block_m]*z_local[jjcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			A[jct + 0*block_m]*z_local[jjct] * block_zfilterU[ jjzb + jzb*nzblock ]
			)*Dinv[j];
	 s_out[j] = s_in[j] + z_local[jj] ;
#else	  
	 z[j] = z[j] - (
			A[jce + 2*block_m]*z[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			A[jcn + 1*block_m]*z[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			A[jct + 0*block_m]*z[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
			)*Dinv[j];
	 s_out[j] = s_in[j] + z[j] ;
#endif
	 
#if 0
	 s_out[j] = r[j];
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


__global__ void calc_res_solve_pre_subdividing_mat2_local_ILU_2_block3D_core_lowtype_cuda_kernel(
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

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int nzdivblock_core = nzdivblock-2;
  const int nydivblock_core = nydivblock-2;
  const int nxdivblock_core = nxdivblock;
  const int ncoreloop = nxdivblock_core * nydivblock_core * nzdivblock_core;
  const int domain = ncoreloop;
  //  const int ndivblock  = nxdivblock * nydivblock * nzdivblock;
  type sr_val = 0.0;
  type rr_val = 0.0;
#if ILU_LOCAL_BUF
  const int mxblock = nxblock + 2;
  const int myblock = nyblock + 2;
  const int mzblock = nzblock + 2;
  const int mxyblock = mxblock*myblock;
  const int mblock = mxblock * myblock * mzblock;
#endif
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
#if ILU_LOCAL_BUF
    type z_local[ (SUBDIVIDING_XBLOCKSIZE + 2) * (SUBDIVIDING_YBLOCKSIZE + 2) * (SUBDIVIDING_ZBLOCKSIZE + 2) ];
    for(int jj=0;jj<mblock;jj++){
      z_local[jj] = 0.0;
    }
#endif        
    const int jzb =  jb / (nxdivblock_core*nydivblock_core) + 1;
    const int jyb = (jb%(nxdivblock_core*nydivblock_core))/nxdivblock_core + 1;
    const int jxb = (jb%(nxdivblock_core*nydivblock_core))%nxdivblock_core ;   

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
	 const int stxp = stride_xp[jjxb];
	 const int styp = stride_yp[jjyb];
	 const int stzp = stride_zp[jjzb];

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
	     A[j   + 0 * block_m]*s_in[jcb] 
	    +A[j   + 1 * block_m]*s_in[jcs] 
	    +A[j   + 2 * block_m]*s_in[jcw] 
	    +A[j   + 3 * block_m]*s_in[jcc] 
	    +A[jce + 2 * block_m]*s_in[jce] 
	    +A[jcn + 1 * block_m]*s_in[jcn] 
	    +A[jct + 0 * block_m]*s_in[jct] 
	    ;
	  type res = r[j] - val;
#if ILU_LOCAL_BUF
	  const int jj = (jjxb+1) + (jjyb+1)*mxblock + (jjzb+1)*mxyblock;
	  const int jjcb = jj - mxyblock;
	  const int jjcs = jj - mxblock;
	  const int jjcw = jj - 1;
	  //	  const int jjce = jj + 1;
	  //	  const int jjcn = jj + mxblock;
	  //	  const int jjct = jj + mxyblock;
	  z_local[jj] = ( res - ( 
				  A[j + 2*block_m]*z_local[jjcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
				  A[j + 1*block_m]*z_local[jjcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
				  A[j + 0*block_m]*z_local[jjcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
				   ) ) * Dinv[j];  
#else	  
	  z[j] = ( res - ( 
				  A[j + 2*block_m]*z[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
				  A[j + 1*block_m]*z[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
				  A[j + 0*block_m]*z[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
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

#if ILU_LOCAL_BUF
	  const int jj = (jjxb+1) + (jjyb+1)*mxblock + (jjzb+1)*mxyblock;
	  //	  const int jjcb = jjb - mxyblock;
	  //	  const int jjcs = jjb - mxblock;
	  //	  const int jjcw = jjb - 1;
	  	  const int jjce = jj + 1;
	  	  const int jjcn = jj + mxblock;
	  	  const int jjct = jj + mxyblock;
	 z_local[jj] = z_local[jj] - (
			A[jce + 2*block_m]*z_local[jjce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			A[jcn + 1*block_m]*z_local[jjcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			A[jct + 0*block_m]*z_local[jjct] * block_zfilterU[ jjzb + jzb*nzblock ]
			)*Dinv[j];
	 s_out[j] = s_in[j] + z_local[jj] ;
#else	  
	 z[j] = z[j] - (
			A[jce + 2*block_m]*z[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			A[jcn + 1*block_m]*z[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			A[jct + 0*block_m]*z[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
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

__global__ void calc_res_solve_pre_subdividing_mat2_local_ILU_2_block3D_unable_core_lowtype_cuda_kernel(
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
						     type *block_West_filter,type *block_East_filter,
						     type *block_South_filter,type *block_North_filter,
						     type *block_Bottom_filter,type *block_Top_filter,
								      const  int block_m,
								      const int nxblock,const int nyblock,const int nzblock,
								      const int mxdivblock,const int mydivblock,const int mzdivblock,
								      const int nxdivblock,const int nydivblock,const int nzdivblock){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int nzdivblock_core = nzdivblock-2;
  const int nydivblock_core = nydivblock-2;
  const int nxdivblock_core = nxdivblock;
  const int ncoreloop = nxdivblock_core * nydivblock_core * nzdivblock_core;
  const int domain = ncoreloop;
  //  const int ndivblock  = nxdivblock * nydivblock * nzdivblock;
  type sr_val = 0.0;
  type rr_val = 0.0;
#if ILU_LOCAL_BUF
  const int mxblock = nxblock + 2;
  const int myblock = nyblock + 2;
  const int mzblock = nzblock + 2;
  const int mxyblock = mxblock*myblock;
  const int mblock = mxblock * myblock * mzblock;
#endif
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
#if ILU_LOCAL_BUF
    type z_local[ (SUBDIVIDING_XBLOCKSIZE + 2) * (SUBDIVIDING_YBLOCKSIZE + 2) * (SUBDIVIDING_ZBLOCKSIZE + 2) ];
    for(int jj=0;jj<mblock;jj++){
      z_local[jj] = 0.0;
    }
#endif        
    const int jzb =  jb / (nxdivblock_core*nydivblock_core) + 1;
    const int jyb = (jb%(nxdivblock_core*nydivblock_core))/nxdivblock_core + 1;
    const int jxb = (jb%(nxdivblock_core*nydivblock_core))%nxdivblock_core ;
    
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
	 const int stxp = stride_xp[jjxb];
	 const int styp = stride_yp[jjyb];
	 const int stzp = stride_zp[jjzb];

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
	     A[j   + 0 * block_m]*s_in[jcb] * block_Bottom_filter[ jjzb + jzb*nzblock ]
	    +A[j   + 1 * block_m]*s_in[jcs] * block_South_filter[ jjyb + jyb*nyblock ]
	    +A[j   + 2 * block_m]*s_in[jcw] 
	    +A[j   + 3 * block_m]*s_in[jcc] 
	    +A[jce + 2 * block_m]*s_in[jce] 
	    +A[jcn + 1 * block_m]*s_in[jcn] * block_North_filter[ jjyb + jyb*nyblock ]
	    +A[jct + 0 * block_m]*s_in[jct] * block_Top_filter[ jjzb + jzb*nzblock ]
	    ;
	  type res = r[j] - val;
#if ILU_LOCAL_BUF
	  const int jj = (jjxb+1) + (jjyb+1)*mxblock + (jjzb+1)*mxyblock;
	  const int jjcb = jj - mxyblock;
	  const int jjcs = jj - mxblock;
	  const int jjcw = jj - 1;
	  //	  const int jjce = jj + 1;
	  //	  const int jjcn = jj + mxblock;
	  //	  const int jjct = jj + mxyblock;
	  z_local[jj] = ( res - ( 
				  A[j + 2*block_m]*z_local[jjcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
				  A[j + 1*block_m]*z_local[jjcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
				  A[j + 0*block_m]*z_local[jjcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
				   ) ) * Dinv[j];  
#else	  
	  z[j] = ( res - ( 
				  A[j + 2*block_m]*z[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
				  A[j + 1*block_m]*z[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
				  A[j + 0*block_m]*z[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
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

#if ILU_LOCAL_BUF
	  const int jj = (jjxb+1) + (jjyb+1)*mxblock + (jjzb+1)*mxyblock;
	  //	  const int jjcb = jjb - mxyblock;
	  //	  const int jjcs = jjb - mxblock;
	  //	  const int jjcw = jjb - 1;
	  	  const int jjce = jj + 1;
	  	  const int jjcn = jj + mxblock;
	  	  const int jjct = jj + mxyblock;
	 z_local[jj] = z_local[jj] - (
			A[jce + 2*block_m]*z_local[jjce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			A[jcn + 1*block_m]*z_local[jjcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			A[jct + 0*block_m]*z_local[jjct] * block_zfilterU[ jjzb + jzb*nzblock ]
			)*Dinv[j];
	 s_out[j] = s_in[j] + z_local[jj] ;
#else	  
	 z[j] = z[j] - (
			A[jce + 2*block_m]*z[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			A[jcn + 1*block_m]*z[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			A[jct + 0*block_m]*z[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
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

__global__ void calc_res_solve_pre_subdividing_mat2_local_ILU_2_Bottom_block3D_lowtype_cuda_kernel(
								      low_type* A,low_type* Dinv,
								      type *r, low_type *s_lowtype, type *s,
								      low_type *z_lowtype,
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

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int nTopBottom  = nxdivblock * nydivblock;
  const int domain = nTopBottom;

  type sr_val = 0.0;
  type rr_val = 0.0; 
#if ILU_LOCAL_BUF
  const int mxblock = nxblock + 2;
  const int myblock = nyblock + 2;
  const int mzblock = nzblock + 2;
  const int mxyblock = mxblock*myblock;
  const int mblock = mxblock * myblock * mzblock;
#endif
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
#if ILU_LOCAL_BUF
    type z_local[ (SUBDIVIDING_XBLOCKSIZE + 2) * (SUBDIVIDING_YBLOCKSIZE + 2) * (SUBDIVIDING_ZBLOCKSIZE + 2) ];
    for(int jj=0;jj<mblock;jj++){
      z_local[jj] = 0.0;
    }
#endif    
    const int jzb = 0;
    const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock ;
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
	  
	  // 内側インデックス
	  const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);  
	  const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  
	  const int stxm = stride_xm[jjxb];
	  const int stym = stride_ym[jjyb];
	  const int stzm = stride_zm[jjzb];
	  const int stxp = stride_xp[jjxb];
	  const int styp = stride_yp[jjyb];
	  const int stzp = stride_zp[jjzb];

	  const int jcb = j + stzm;
	  const int jcs = j + stym;
	  const int jcw = j + stxm;
	  const int jcc = j       ;
	  const int jce = j + stxp;
	  const int jcn = j + styp;
	  const int jct = j + stzp;

#if 0
	  type val = 
	     A[j   + 0 * block_m]*s_lowtype[jcb] 
	    +A[j   + 1 * block_m]*s_lowtype[jcs] 
	    +A[j   + 2 * block_m]*s_lowtype[jcw] 
	    +A[j   + 3 * block_m]*s_lowtype[jcc] 
	    +A[jce + 2 * block_m]*s_lowtype[jce] 
	    +A[jcn + 1 * block_m]*s_lowtype[jcn] 
	    +A[jct + 0 * block_m]*s_lowtype[jct] 
	    ;
	  type res = r[j] - val;
#endif
	  
#if ILU_LOCAL_BUF
	  const int jj = (jjxb+1) + (jjyb+1)*mxblock + (jjzb+1)*mxyblock;
	  const int jjcb = jj - mxyblock;
	  const int jjcs = jj - mxblock;
	  const int jjcw = jj - 1;
	  //	  const int jjce = jj + 1;
	  //	  const int jjcn = jj + mxblock;
	  //	  const int jjct = jj + mxyblock;
	  z_local[jj] = ( res - ( 
				  A[j + 2*block_m]*z_local[jjcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
				  A[j + 1*block_m]*z_local[jjcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
				  A[j + 0*block_m]*z_local[jjcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
				   ) ) * Dinv[j];  
#else	  
	  //	  z_lowtype[j] = ( res - (
	  z_lowtype[j] = ( z_lowtype[j] - ( 	  
				  A[j + 2*block_m]*z_lowtype[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
				  A[j + 1*block_m]*z_lowtype[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
				  A[j + 0*block_m]*z_lowtype[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
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

#if ILU_LOCAL_BUF
	  const int jj = (jjxb+1) + (jjyb+1)*mxblock + (jjzb+1)*mxyblock;
	  //	  const int jjcb = jjb - mxyblock;
	  //	  const int jjcs = jjb - mxblock;
	  //	  const int jjcw = jjb - 1;
	  	  const int jjce = jj + 1;
	  	  const int jjcn = jj + mxblock;
	  	  const int jjct = jj + mxyblock;
	 z_local[jj] = z_local[jj] - (
			A[jce + 2*block_m]*z_local[jjce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			A[jcn + 1*block_m]*z_local[jjcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			A[jct + 0*block_m]*z_local[jjct] * block_zfilterU[ jjzb + jzb*nzblock ]
			)*Dinv[j];
	 s[j] = s_lowtype[j] + z_local[jj] ;
#else	         
	  z_lowtype[j] = z_lowtype[j] - (
	    A[jce + 2 * block_m]*z_lowtype[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
	    A[jcn + 1 * block_m]*z_lowtype[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
	    A[jct + 0 * block_m]*z_lowtype[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
	    )*Dinv[j];
	  s[j] = s_lowtype[j] + z_lowtype[j] ;
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

__global__ void calc_res_solve_pre_subdividing_mat2_local_ILU_2_Top_block3D_lowtype_cuda_kernel(
								      low_type* A,low_type* Dinv,
								      type *r, low_type *s_lowtype, type *s,
								      low_type *z_lowtype,
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

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int nTopBottom  = nxdivblock * nydivblock;
  const int domain = nTopBottom;

  type sr_val = 0.0;
  type rr_val = 0.0; 

#if ILU_LOCAL_BUF
  const int mxblock = nxblock + 2;
  const int myblock = nyblock + 2;
  const int mzblock = nzblock + 2;
  const int mxyblock = mxblock*myblock;
  const int mblock = mxblock * myblock * mzblock;
#endif
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
#if ILU_LOCAL_BUF
    type z_local[ (SUBDIVIDING_XBLOCKSIZE + 2) * (SUBDIVIDING_YBLOCKSIZE + 2) * (SUBDIVIDING_ZBLOCKSIZE + 2) ];
    for(int jj=0;jj<mblock;jj++){
      z_local[jj] = 0.0;
    }
#endif    
    const int jzb = nzdivblock-1;
    const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock ;
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
	  
	  // 内側インデックス
	  const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);  
	  const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;

	  const int stxm = stride_xm[jjxb];
	  const int stym = stride_ym[jjyb];
	  const int stzm = stride_zm[jjzb];
	  const int stxp = stride_xp[jjxb];
	  const int styp = stride_yp[jjyb];
	  const int stzp = stride_zp[jjzb];

	  const int jcb = j + stzm;
	  const int jcs = j + stym;
	  const int jcw = j + stxm;
	  const int jcc = j       ;
	  const int jce = j + stxp;
	  const int jcn = j + styp;
	  const int jct = j + stzp;

#if 0
	  type val = 
	     A[j   + 0 * block_m]*s_lowtype[jcb] 
	    +A[j   + 1 * block_m]*s_lowtype[jcs] 
	    +A[j   + 2 * block_m]*s_lowtype[jcw] 
	    +A[j   + 3 * block_m]*s_lowtype[jcc] 
	    +A[jce + 2 * block_m]*s_lowtype[jce] 
	    +A[jcn + 1 * block_m]*s_lowtype[jcn] 
	    +A[jct + 0 * block_m]*s_lowtype[jct] 
	    ;
	  type res = r[j] - val;
#endif
	  
#if ILU_LOCAL_BUF
	  const int jj = (jjxb+1) + (jjyb+1)*mxblock + (jjzb+1)*mxyblock;
	  const int jjcb = jj - mxyblock;
	  const int jjcs = jj - mxblock;
	  const int jjcw = jj - 1;
	  //	  const int jjce = jj + 1;
	  //	  const int jjcn = jj + mxblock;
	  //	  const int jjct = jj + mxyblock;
	  z_local[jj] = ( res - ( 
				  A[j + 2*block_m]*z_local[jjcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
				  A[j + 1*block_m]*z_local[jjcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
				  A[j + 0*block_m]*z_local[jjcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
				   ) ) * Dinv[j];  
#else	  
	  //	  z_lowtype[j] = ( res - (
	  z_lowtype[j] = ( z_lowtype[j] - (				  
				  A[j + 2*block_m]*z_lowtype[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
				  A[j + 1*block_m]*z_lowtype[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
				  A[j + 0*block_m]*z_lowtype[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
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

#if ILU_LOCAL_BUF
	  const int jj = (jjxb+1) + (jjyb+1)*mxblock + (jjzb+1)*mxyblock;
	  //	  const int jjcb = jjb - mxyblock;
	  //	  const int jjcs = jjb - mxblock;
	  //	  const int jjcw = jjb - 1;
	  	  const int jjce = jj + 1;
	  	  const int jjcn = jj + mxblock;
	  	  const int jjct = jj + mxyblock;
	 z_local[jj] = z_local[jj] - (
			A[jce + 2*block_m]*z_local[jjce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			A[jcn + 1*block_m]*z_local[jjcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			A[jct + 0*block_m]*z_local[jjct] * block_zfilterU[ jjzb + jzb*nzblock ]
			)*Dinv[j];
	 s[j] = s_lowtype[j] + z_local[jj] ;
#else	         
	  z_lowtype[j] = z_lowtype[j] - (
	    A[jce + 2 * block_m]*z_lowtype[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
	    A[jcn + 1 * block_m]*z_lowtype[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
	    A[jct + 0 * block_m]*z_lowtype[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
	    )*Dinv[j];
	  s[j] = s_lowtype[j] + z_lowtype[j] ;
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

__global__ void calc_solve_pre_subdividing_mat2_local_ILU_2_block3D_South_lowtype_cuda_kernel(
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

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int nzdivblock_core = nzdivblock-2;
  const int nNorthSouth = nxdivblock * nzdivblock_core;
  const int domain = nNorthSouth;

  type sr_val = 0.0;
  type rr_val = 0.0;
#if ILU_LOCAL_BUF
  const int mxblock = nxblock + 2;
  const int myblock = nyblock + 2;
  const int mzblock = nzblock + 2;
  const int mxyblock = mxblock*myblock;
  const int mblock = mxblock * myblock * mzblock;
#endif
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
#if ILU_LOCAL_BUF
    type z_local[ (SUBDIVIDING_XBLOCKSIZE + 2) * (SUBDIVIDING_YBLOCKSIZE + 2) * (SUBDIVIDING_ZBLOCKSIZE + 2) ];
    for(int jj=0;jj<mblock;jj++){
      z_local[jj] = 0.0;
    }
#endif    
    const int jzb = (jb%(nxdivblock*nzdivblock_core))/nxdivblock + 1;    
    const int jyb = 0;
    const int jxb = (jb%(nxdivblock*nzdivblock_core))%nxdivblock;
    
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
	 const int stxp = stride_xp[jjxb];
	 const int styp = stride_yp[jjyb];
	 const int stzp = stride_zp[jjzb];

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
#if 0
	  type val = 
	     A[j   + 0 * block_m]*s_in[jcb] 
	    +A[j   + 1 * block_m]*s_in[jcs] 
	    +A[j   + 2 * block_m]*s_in[jcw] 
	    +A[j   + 3 * block_m]*s_in[jcc] 
	    +A[jce + 2 * block_m]*s_in[jce] 
	    +A[jcn + 1 * block_m]*s_in[jcn] 
	    +A[jct + 0 * block_m]*s_in[jct] 
	    ;
	  type res = r[j] - val;
#endif
	  
#if ILU_LOCAL_BUF
	  const int jj = (jjxb+1) + (jjyb+1)*mxblock + (jjzb+1)*mxyblock;
	  const int jjcb = jj - mxyblock;
	  const int jjcs = jj - mxblock;
	  const int jjcw = jj - 1;
	  //	  const int jjce = jj + 1;
	  //	  const int jjcn = jj + mxblock;
	  //	  const int jjct = jj + mxyblock;
	  z_local[jj] = ( res - ( 
				  A[j + 2*block_m]*z_local[jjcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
				  A[j + 1*block_m]*z_local[jjcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
				  A[j + 0*block_m]*z_local[jjcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
				   ) ) * Dinv[j];  
#else	  
	  //	  z[j] = ( res - (
	  	  z[j] = ( z[j] - ( 
				  A[j + 2*block_m]*z[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
				  A[j + 1*block_m]*z[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
				  A[j + 0*block_m]*z[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
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

#if ILU_LOCAL_BUF
	  const int jj = (jjxb+1) + (jjyb+1)*mxblock + (jjzb+1)*mxyblock;
	  //	  const int jjcb = jjb - mxyblock;
	  //	  const int jjcs = jjb - mxblock;
	  //	  const int jjcw = jjb - 1;
	  	  const int jjce = jj + 1;
	  	  const int jjcn = jj + mxblock;
	  	  const int jjct = jj + mxyblock;
	 z_local[jj] = z_local[jj] - (
			A[jce + 2*block_m]*z_local[jjce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			A[jcn + 1*block_m]*z_local[jjcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			A[jct + 0*block_m]*z_local[jjct] * block_zfilterU[ jjzb + jzb*nzblock ]
			)*Dinv[j];
	 s_out[j] = s_in[j] + z_local[jj] ;
#else	         
	 z[j] = z[j] - (
			A[jce + 2*block_m]*z[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			A[jcn + 1*block_m]*z[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			A[jct + 0*block_m]*z[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
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

__global__ void calc_res_solve_pre_subdividing_mat2_local_ILU_2_block3D_North_lowtype_cuda_kernel(
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

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int nzdivblock_core = nzdivblock-2;
  const int nNorthSouth = nxdivblock * nzdivblock_core;
  const int domain = nNorthSouth;

  type sr_val = 0.0;
  type rr_val = 0.0;
#if ILU_LOCAL_BUF
  const int mxblock = nxblock + 2;
  const int myblock = nyblock + 2;
  const int mzblock = nzblock + 2;
  const int mxyblock = mxblock*myblock;
  const int mblock = mxblock * myblock * mzblock;
#endif
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
#if ILU_LOCAL_BUF
    type z_local[ (SUBDIVIDING_XBLOCKSIZE + 2) * (SUBDIVIDING_YBLOCKSIZE + 2) * (SUBDIVIDING_ZBLOCKSIZE + 2) ];
    for(int jj=0;jj<mblock;jj++){
      z_local[jj] = 0.0;
    }
#endif    
    const int jzb = (jb%(nxdivblock*nzdivblock_core))/nxdivblock + 1;    
    const int jyb = nydivblock-1;
    const int jxb = (jb%(nxdivblock*nzdivblock_core))%nxdivblock;

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
	 const int stxp = stride_xp[jjxb];
	 const int styp = stride_yp[jjyb];
	 const int stzp = stride_zp[jjzb];

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
#if 0
	  type val = 
	     A[j   + 0 * block_m]*s_in[jcb] 
	    +A[j   + 1 * block_m]*s_in[jcs] 
	    +A[j   + 2 * block_m]*s_in[jcw] 
	    +A[j   + 3 * block_m]*s_in[jcc] 
	    +A[jce + 2 * block_m]*s_in[jce] 
	    +A[jcn + 1 * block_m]*s_in[jcn] 
	    +A[jct + 0 * block_m]*s_in[jct] 
	    ;
	  type res = r[j] - val;
#endif
	  
#if ILU_LOCAL_BUF
	  const int jj = (jjxb+1) + (jjyb+1)*mxblock + (jjzb+1)*mxyblock;
	  const int jjcb = jj - mxyblock;
	  const int jjcs = jj - mxblock;
	  const int jjcw = jj - 1;
	  //	  const int jjce = jj + 1;
	  //	  const int jjcn = jj + mxblock;
	  //	  const int jjct = jj + mxyblock;
	  z_local[jj] = ( res - ( 
				  A[j + 2*block_m]*z_local[jjcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
				  A[j + 1*block_m]*z_local[jjcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
				  A[j + 0*block_m]*z_local[jjcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
				   ) ) * Dinv[j];  
#else	  
	  //	  z[j] = ( res - ( 
	  	  z[j] = ( z[j] - ( 
				  A[j + 2*block_m]*z[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
				  A[j + 1*block_m]*z[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
				  A[j + 0*block_m]*z[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
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

#if ILU_LOCAL_BUF
	  const int jj = (jjxb+1) + (jjyb+1)*mxblock + (jjzb+1)*mxyblock;
	  //	  const int jjcb = jjb - mxyblock;
	  //	  const int jjcs = jjb - mxblock;
	  //	  const int jjcw = jjb - 1;
	  	  const int jjce = jj + 1;
	  	  const int jjcn = jj + mxblock;
	  	  const int jjct = jj + mxyblock;
	 z_local[jj] = z_local[jj] - (
			A[jce + 2*block_m]*z_local[jjce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			A[jcn + 1*block_m]*z_local[jjcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			A[jct + 0*block_m]*z_local[jjct] * block_zfilterU[ jjzb + jzb*nzblock ]
			)*Dinv[j];
	 s_out[j] = s_in[j] + z_local[jj] ;
#else	         
	 z[j] = z[j] - (
			A[jce + 2*block_m]*z[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			A[jcn + 1*block_m]*z[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			A[jct + 0*block_m]*z[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
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




__global__ void calc_sres_block3D_unable_bound_cuda_kernel(
						     low_type* A,low_type* x,low_type* r,type* b,
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

	// 現在の実装はx方向のオーバーラップをしないためx方向のフィルターを無効にしている

	type val = 
	     A[j   + 0 * block_m]*x[jcb] * block_Bottom_filter[ jjzb + jzb*nzblock ] 
	    +A[j   + 1 * block_m]*x[jcs] * block_South_filter[ jjyb + jyb*nyblock ]
	    +A[j   + 2 * block_m]*x[jcw] // * block_West_filter[ jjxb + jxb*nxblock ] 
	    +A[j   + 3 * block_m]*x[jcc] 
	    +A[jce + 2 * block_m]*x[jce] // * block_East_filter[ jjxb + jxb*nxblock ] 
	    +A[jcn + 1 * block_m]*x[jcn] * block_North_filter[ jjyb + jyb*nyblock ]
	    +A[jct + 0 * block_m]*x[jct] * block_Top_filter[ jjzb + jzb*nzblock ]
	    ;
	r[j] = b[j] - val;
	} 
      }
    }

  }
  
  return ;
}

int calc_sres_block3D_OVL_lowtype_cuda(
		       mpi_prm prm,
		       low_type* A,
		       low_type* x,low_type* r,type* b,
		       int *block_nxs, int *block_nxe, 
		       int *block_nys, int *block_nye, 
		       int *block_nzs, int *block_nze,
		       int *stride_xp, int *stride_xm,
		       int *stride_yp, int *stride_ym,
		       int *stride_zp, int *stride_zm,
		       type *block_West_filter, type *block_East_filter,
		       type *block_South_filter, type *block_North_filter,
		       type *block_Bottom_filter, type *block_Top_filter,
		       low_type *cuda_buf,
		       low_type *sb_b ,low_type *rb_b ,low_type *sb_t ,low_type *rb_t ,
		       low_type *sb_s ,low_type *rb_s ,low_type *sb_n ,low_type *rb_n ,
		       low_type *sb_w ,low_type *rb_w ,low_type *sb_e ,low_type *rb_e ,		       
		       cudaStream_t *cudaStream_Hi_Priority
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
  
  const int blocks  = 1024;  
  const int threads = 96;  

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

#if 1

  cudaDeviceSynchronize();

  // west (x-)
  // east (x+)
  setWest_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[4]>>>(
						    cu_sb_w ,x,
						    block_nxs, block_nxe,
						    nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);
  setEast_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[5]>>>(
						    cu_sb_e ,x,
						    block_nxs, block_nxe,
						    nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);
  cudaMemcpyAsync(sb_w, cu_sb_w, sizeof(low_type)*block_nyz, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[4]);
  cudaMemcpyAsync(sb_e, cu_sb_e, sizeof(low_type)*block_nyz, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[5]);

  // south (y-)
  // north (y+)
  setSouth_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[2]>>>(
						      cu_sb_s, x,
						      block_nys, block_nye,
						      nxblock, nyblock, nzblock,
						      mxdivblock, mydivblock, mzdivblock,
						      nxdivblock, nydivblock, nzdivblock);
  setNorth_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[3]>>>(
						      cu_sb_n ,x,
						      block_nys, block_nye,
						      nxblock, nyblock, nzblock,
						      mxdivblock, mydivblock, mzdivblock,
						      nxdivblock, nydivblock, nzdivblock);
  cudaMemcpyAsync(sb_s, cu_sb_s, sizeof(low_type)*block_nxz, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[2]);
  cudaMemcpyAsync(sb_n, cu_sb_n, sizeof(low_type)*block_nxz, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[3]);
  
  // bottom (z-)
  // top    (z+)
  setBottom_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[0]>>>(
						     cu_sb_b ,x,
						     block_nzs, block_nze,
						     nxblock, nyblock, nzblock,
						     mxdivblock, mydivblock, mzdivblock,
						     nxdivblock, nydivblock, nzdivblock);
  setTop_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[1]>>>(
						     cu_sb_t ,x,
						     block_nzs, block_nze,
						     nxblock, nyblock, nzblock,
						     mxdivblock, mydivblock, mzdivblock,
						     nxdivblock, nydivblock, nzdivblock);
  
  cudaMemcpyAsync(sb_b, cu_sb_b, sizeof(low_type)*block_nxy, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[0]);
  cudaMemcpyAsync(sb_t, cu_sb_t, sizeof(low_type)*block_nxy, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[1]);

  // ----------------------------------------------------------
  // west (x-)
  // east (x+)
  cudaStreamSynchronize(cudaStream_Hi_Priority[4]);
  sleev_comm_block3D_icommWest_lowtype_cuda(&prm,sb_w,rb_w);
  cudaStreamSynchronize(cudaStream_Hi_Priority[5]);
  sleev_comm_block3D_icommEast_lowtype_cuda(&prm,sb_e,rb_e);

  sleev_comm_block3D_waitWest_cuda(&prm);
  cudaMemcpyAsync(cu_rb_w, rb_w, sizeof(low_type)*block_nyz, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[6]);
  readBackWest_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[6]>>>(
							 cu_rb_w,x,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);
  
  sleev_comm_block3D_waitEast_cuda(&prm);
  cudaMemcpyAsync(cu_rb_e, rb_e, sizeof(low_type)*block_nyz, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[6]);
  readBackEast_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[6]>>>(
							 cu_rb_e ,x,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);
  // ----------------------------------------------------------
#if 1
  calc_sres_block3D_unable_bound_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[6]>>>(
											     A, x, r, b,
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
#endif  
  // ----------------------------------------------------------  

  // south (y-)
  // north (y+)
  cudaStreamSynchronize(cudaStream_Hi_Priority[2]);
  sleev_comm_block3D_icommSouth_lowtype_cuda(&prm,sb_s,rb_s);
  cudaStreamSynchronize(cudaStream_Hi_Priority[3]);
  sleev_comm_block3D_icommNorth_lowtype_cuda(&prm,sb_n,rb_n);
 
  sleev_comm_block3D_waitSouth_cuda(&prm); 
  cudaMemcpyAsync(cu_rb_s, rb_s, sizeof(low_type)*block_nxz, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[2]);
  readBackSouth_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[2]>>>(
				       cu_rb_s, x,
				       block_nys, block_nye,
				       nxblock, nyblock, nzblock,
				       mxdivblock, mydivblock, mzdivblock,
				       nxdivblock, nydivblock, nzdivblock);
  
  sleev_comm_block3D_waitNorth_cuda(&prm);
  cudaMemcpyAsync(cu_rb_n, rb_n, sizeof(low_type)*block_nxz, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[3]);
  readBackNorth_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[3]>>>(
				       cu_rb_n, x,
				       block_nys, block_nye,
				       nxblock, nyblock, nzblock,
				       mxdivblock, mydivblock, mzdivblock,
				       nxdivblock, nydivblock, nzdivblock);
  
  // top    (z+)
  // bottom (z-)
  cudaStreamSynchronize(cudaStream_Hi_Priority[0]);
  sleev_comm_block3D_icommBottom_lowtype_cuda(&prm,sb_b,rb_b);
  
  cudaStreamSynchronize(cudaStream_Hi_Priority[1]);
  sleev_comm_block3D_icommTop_lowtype_cuda(&prm,sb_t,rb_t);
  
  sleev_comm_block3D_waitBottom_cuda(&prm);
  cudaMemcpyAsync(cu_rb_b, rb_b, sizeof(low_type)*block_nxy, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[0]);
  readBackBottom_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[0]>>>(
							  cu_rb_b, x,
							  block_nzs, block_nze,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);
  
  sleev_comm_block3D_waitTop_cuda(&prm);  
  cudaMemcpyAsync(cu_rb_t, rb_t, sizeof(low_type)*block_nxy, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[1]);
  readBackTop_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[1]>>>(
							  cu_rb_t, x,
							  block_nzs, block_nze,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);

  
  cudaDeviceSynchronize();

#else  
  sleev_comm_hostmem_block3D_lowtype_cuda(
				  x,&prm,
				  block_nxs,  block_nxe, 
				  block_nys,  block_nye, 
				  block_nzs,  block_nze,
				  cuda_buf,
				  sb_b,  rb_b,  sb_t,  rb_t,
				  sb_s,  rb_s,  sb_n,  rb_n,
				  sb_w,  rb_w,  sb_e,  rb_e,
				  cudaStream_Hi_Priority
				  );
#endif

#if 0
  calc_sres_block3D_unable_bound_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[6]>>>(
											     A, x, r, b,
							    block_m,
							    stride_xp, stride_xm,
							    stride_yp, stride_ym,
							    stride_zp, stride_zm,
							    block_West_filter,block_East_filter,
							    block_South_filter,block_North_filter,
							    block_Bottom_filter,block_Top_filter,
							    nxblock,  nyblock,  nzblock,
							    mxdivblock,  mydivblock,  mzdivblock,
							    nxdivblock,  nydivblock,  nzdivblock);
#endif
  

  cudaStreamSynchronize(cudaStream_Hi_Priority[2]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[3]);
  calc_sres_block3D_able_South_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[6]>>>(
							    A, x, r, b,
							    cu_rb_s,
							    block_m,
							    block_nys,							   
						      block_nxs, block_nxe, 
						      block_nzs, block_nze,
							    stride_ym,
							    nxblock,  nyblock,  nzblock,
							    mxdivblock,  mydivblock,  mzdivblock,
							    nxdivblock,  nydivblock,  nzdivblock);
  
  calc_sres_block3D_able_North_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[6]>>>(
							    A, x, r, b,
							    cu_rb_n, 
							    block_m,
							    block_nye,
						      block_nxs, block_nxe, 
						      block_nzs, block_nze,
							    stride_yp,						     
							    nxblock,  nyblock,  nzblock,
							    mxdivblock,  mydivblock,  mzdivblock,
							    nxdivblock,  nydivblock,  nzdivblock);

  cudaStreamSynchronize(cudaStream_Hi_Priority[0]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[1]);  
  calc_sres_block3D_able_Bottom_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[6]>>>(
							    A, x, r, b,
							    cu_rb_b,
							    block_m,
							    block_nzs,
						      block_nxs, block_nxe, 
						      block_nys, block_nye,
							    stride_zm,
							    nxblock,  nyblock,  nzblock,
							    mxdivblock,  mydivblock,  mzdivblock,
							    nxdivblock,  nydivblock,  nzdivblock);
  

  calc_sres_block3D_able_Top_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[6]>>>(
	       									 A, x, r, b,
		       								 cu_rb_t,     
			       	       						 block_m,
		       	       							 block_nze,
						      block_nxs, block_nxe, 
						      block_nys, block_nye,
					       					 stride_zp,
								       		 nxblock,  nyblock,  nzblock,
       						       				 mxdivblock,  mydivblock,  mzdivblock,
							       			 nxdivblock,  nydivblock,  nzdivblock);
  
  
  
  cudaDeviceSynchronize();
  
  return 0;
}

int calc_solve_pre_subdividing_mat2_local_ILU_2_block3D_OVL_lowtype_cuda(
		       mpi_prm prm,
		       low_type* A,low_type* Dinv,
		       type *r, low_type *s_lowtype, type *s,
		       low_type *z_lowtype,
		       type *cuda_buf,
		       low_type *cuda_lowtype_buf,
		       int *block_nxs, int *block_nxe, 
		       int *block_nys, int *block_nye, 
		       int *block_nzs, int *block_nze,
		       int *stride_xp, int *stride_xm,
		       int *stride_yp, int *stride_ym,
		       int *stride_zp, int *stride_zm,
				   type *block_xfilterL,type *block_xfilterU,
				   type *block_yfilterL,type *block_yfilterU,
				   type *block_zfilterL,type *block_zfilterU,		       
		       type *block_West_filter, type *block_East_filter,
		       type *block_South_filter, type *block_North_filter,
		       type *block_Bottom_filter, type *block_Top_filter,
		       low_type *sb_b ,low_type *rb_b ,low_type *sb_t ,low_type *rb_t ,
		       low_type *sb_s ,low_type *rb_s ,low_type *sb_n ,low_type *rb_n ,
		       low_type *sb_w ,low_type *rb_w ,low_type *sb_e ,low_type *rb_e ,		       
		       cudaStream_t *cudaStream_Hi_Priority,
		       cudaStream_t *cudaStream_Low_Priority,
		       cudaEvent_t *cudaEvent_OVL,		       
		       double *time
		       ){

  double ts,te;

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
  
  const int blocks  = 1024;  
  const int threads = 96;  

  int block_nz = nzblock * nzdivblock;
  int block_ny = nyblock * nydivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxy = block_nx * block_ny;
  int block_nxz = block_nx * block_nz;
  int block_nyz = block_ny * block_nz;

  low_type *cu_sb_b = &cuda_lowtype_buf[0];
  low_type *cu_rb_b = &cuda_lowtype_buf[block_nxy*1];
  low_type *cu_sb_t = &cuda_lowtype_buf[block_nxy*2];
  low_type *cu_rb_t = &cuda_lowtype_buf[block_nxy*3];

  low_type *cu_sb_s = &cuda_lowtype_buf[block_nxy*4];
  low_type *cu_rb_s = &cuda_lowtype_buf[block_nxy*4 + block_nxz*1];
  low_type *cu_sb_n = &cuda_lowtype_buf[block_nxy*4 + block_nxz*2];
  low_type *cu_rb_n = &cuda_lowtype_buf[block_nxy*4 + block_nxz*3];

  low_type *cu_sb_w = &cuda_lowtype_buf[block_nxy*4 + block_nxz*4];
  low_type *cu_rb_w = &cuda_lowtype_buf[block_nxy*4 + block_nxz*4 + block_nyz*1];
  low_type *cu_sb_e = &cuda_lowtype_buf[block_nxy*4 + block_nxz*4 + block_nyz*2];
  low_type *cu_rb_e = &cuda_lowtype_buf[block_nxy*4 + block_nxz*4 + block_nyz*3];

  type *sr_dot_local = &cuda_buf[0];
  type *rr_dot_local = &cuda_buf[1];

  type *sr_dot_core   = &cuda_buf[2];
  type *sr_dot_Bottom = &cuda_buf[3];
  type *sr_dot_Top    = &cuda_buf[4];
  type *sr_dot_South  = &cuda_buf[5];
  type *sr_dot_North  = &cuda_buf[6];

  type *rr_dot_core   = &cuda_buf[7];
  type *rr_dot_Bottom = &cuda_buf[8];
  type *rr_dot_Top    = &cuda_buf[9];
  type *rr_dot_South  = &cuda_buf[10];
  type *rr_dot_North  = &cuda_buf[11];
  
  const int warp_size = 32;
  initialize_type_vec<<<1,warp_size>>>(cuda_buf,12);

  cudaDeviceSynchronize();

  ts=cpu_time(); 
  
#if  JUPITER_CUDA_AWARE_MPI
  sleev_comm_block3D_irecvWest_lowtype_cuda(&prm,cu_sb_w,cu_rb_w);
  sleev_comm_block3D_irecvEast_lowtype_cuda(&prm,cu_sb_e,cu_rb_e);
  sleev_comm_block3D_irecvSouth_lowtype_cuda(&prm,cu_sb_s,cu_rb_s);
  sleev_comm_block3D_irecvNorth_lowtype_cuda(&prm,cu_sb_n,cu_rb_n); 
  sleev_comm_block3D_irecvBottom_lowtype_cuda(&prm,cu_sb_b,cu_rb_b);
  sleev_comm_block3D_irecvTop_lowtype_cuda(&prm,cu_sb_t,cu_rb_t);
#else
  sleev_comm_block3D_irecvWest_lowtype_cuda(&prm,sb_w,rb_w);
  sleev_comm_block3D_irecvEast_lowtype_cuda(&prm,sb_e,rb_e);
  sleev_comm_block3D_irecvSouth_lowtype_cuda(&prm,sb_s,rb_s);
  sleev_comm_block3D_irecvNorth_lowtype_cuda(&prm,sb_n,rb_n); 
  sleev_comm_block3D_irecvBottom_lowtype_cuda(&prm,sb_b,rb_b);
  sleev_comm_block3D_irecvTop_lowtype_cuda(&prm,sb_t,rb_t);
#endif  
  // west (x-)
  // east (x+)
  setWest_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[4]>>>(
						    cu_sb_w ,s_lowtype,
						    block_nxs, block_nxe,
						    nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);
  setEast_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[5]>>>(
						    cu_sb_e ,s_lowtype,
						    block_nxs, block_nxe,
						    nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);
#if  JUPITER_CUDA_AWARE_MPI
#else
  cudaMemcpyAsync(sb_w, cu_sb_w, sizeof(low_type)*block_nyz, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[4]);
  cudaMemcpyAsync(sb_e, cu_sb_e, sizeof(low_type)*block_nyz, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[5]);
#endif

  // ----------------------------------------------------------    
  // south (y-)
  // north (y+)
  setSouth_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[2]>>>(
						      cu_sb_s, s_lowtype,
						      block_nys, block_nye,
						      nxblock, nyblock, nzblock,
						      mxdivblock, mydivblock, mzdivblock,
						      nxdivblock, nydivblock, nzdivblock);
  setNorth_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[3]>>>(
						      cu_sb_n , s_lowtype,
						      block_nys, block_nye,
						      nxblock, nyblock, nzblock,
						      mxdivblock, mydivblock, mzdivblock,
						      nxdivblock, nydivblock, nzdivblock);
#if  JUPITER_CUDA_AWARE_MPI
#else
  cudaMemcpyAsync(sb_s, cu_sb_s, sizeof(low_type)*block_nxz, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[2]);
  cudaMemcpyAsync(sb_n, cu_sb_n, sizeof(low_type)*block_nxz, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[3]);
#endif  
  // bottom (z-)
  // top    (z+)
  setBottom_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[0]>>>(
						     cu_sb_b , s_lowtype,
						     block_nzs, block_nze,
						     nxblock, nyblock, nzblock,
						     mxdivblock, mydivblock, mzdivblock,
						     nxdivblock, nydivblock, nzdivblock);
  setTop_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[1]>>>(
						     cu_sb_t , s_lowtype,
						     block_nzs, block_nze,
						     nxblock, nyblock, nzblock,
						     mxdivblock, mydivblock, mzdivblock,
						     nxdivblock, nydivblock, nzdivblock);
  
#if  JUPITER_CUDA_AWARE_MPI
#else
  cudaMemcpyAsync(sb_b, cu_sb_b, sizeof(low_type)*block_nxy, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[0]);
  cudaMemcpyAsync(sb_t, cu_sb_t, sizeof(low_type)*block_nxy, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[1]);
#endif  
  // west (x-)
  // east (x+)
  cudaStreamSynchronize(cudaStream_Hi_Priority[4]);
  te=cpu_time();
  time[2]=time[2]+te-ts;
  ts=cpu_time();
  
  //  sleev_comm_block3D_icommWest_lowtype_cuda(&prm,sb_w,rb_w);
#if  JUPITER_CUDA_AWARE_MPI
  sleev_comm_block3D_isendWest_lowtype_cuda(&prm,cu_sb_w,cu_rb_w);
#else
  sleev_comm_block3D_isendWest_lowtype_cuda(&prm,sb_w,rb_w);
#endif
  cudaStreamSynchronize(cudaStream_Hi_Priority[5]);
  te=cpu_time();
  time[3]=time[3]+te-ts;
  ts=cpu_time();
  
  //  sleev_comm_block3D_icommEast_lowtype_cuda(&prm,sb_e,rb_e);
#if  JUPITER_CUDA_AWARE_MPI
  sleev_comm_block3D_isendEast_lowtype_cuda(&prm,cu_sb_e,cu_rb_e);
#else
  sleev_comm_block3D_isendEast_lowtype_cuda(&prm,sb_e,rb_e);
#endif

  sleev_comm_block3D_waitWest_cuda(&prm);
  te=cpu_time();
  time[11]=time[11]+te-ts;
  ts=cpu_time();
  
#if  JUPITER_CUDA_AWARE_MPI
#else
  cudaMemcpyAsync(cu_rb_w, rb_w, sizeof(low_type)*block_nyz, cudaMemcpyHostToDevice,cudaStream_Low_Priority[6]);
#endif
  readBackWest_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[6]>>>(
							 cu_rb_w, s_lowtype,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);
  
  sleev_comm_block3D_waitEast_cuda(&prm);
  te=cpu_time();
  time[12]=time[12]+te-ts;
  ts=cpu_time();
  
#if  JUPITER_CUDA_AWARE_MPI
#else
  cudaMemcpyAsync(cu_rb_e, rb_e, sizeof(low_type)*block_nyz, cudaMemcpyHostToDevice,cudaStream_Low_Priority[6]);
#endif
  readBackEast_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[6]>>>(
							 cu_rb_e , s_lowtype,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);
    
  cudaEventRecord(cudaEvent_OVL[0] ,cudaStream_Low_Priority[6]);
  cudaStreamWaitEvent(cudaStream_Low_Priority[7],cudaEvent_OVL[0],0);
  cudaStreamWaitEvent(cudaStream_Low_Priority[8],cudaEvent_OVL[0],0);
  cudaStreamWaitEvent(cudaStream_Low_Priority[9],cudaEvent_OVL[0],0);
  cudaStreamWaitEvent(cudaStream_Low_Priority[10],cudaEvent_OVL[0],0);  
  // ----------------------------------------------------------
 
#if 1
  calc_res_solve_pre_subdividing_mat2_local_ILU_2_block3D_core_lowtype_cuda_kernel
   <<<blocks,threads,0,cudaStream_Low_Priority[6]>>>(
									   A, Dinv,
									   r,  s_lowtype, s, z_lowtype,
									   sr_dot_core, rr_dot_core,
									   block_nxs,  block_nxe, 
									   block_nys,  block_nye, 
									   block_nzs,  block_nze,
									   stride_xp,  stride_xm,
									   stride_yp,  stride_ym,
									   stride_zp,  stride_zm,
									   block_xfilterL, block_xfilterU,
									   block_yfilterL, block_yfilterU,
									   block_zfilterL, block_zfilterU,
									   block_m,
									   nxblock, nyblock, nzblock,
									   mxdivblock, mydivblock, mzdivblock,
									   nxdivblock, nydivblock, nzdivblock);

  
  // ----------------------------------------------------------
  calc_sres_block3D_unable_bound_Bottom_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[7]>>>( 
									   A,
									   s_lowtype, z_lowtype,r,
									   block_m,
						      block_nxs, block_nxe, 
						      block_nys, block_nye, 
						      block_nzs, block_nze,
									   stride_xp,  stride_xm,
									   stride_yp,  stride_ym,
									   stride_zp,  stride_zm,
				   block_West_filter,block_East_filter,
				   block_South_filter,block_North_filter,
				   block_Bottom_filter,block_Top_filter,				   
									   nxblock, nyblock, nzblock,
									   mxdivblock, mydivblock, mzdivblock,
									   nxdivblock, nydivblock, nzdivblock);   

  calc_sres_block3D_unable_bound_Top_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[8]>>>( 
									   A,
									   s_lowtype,z_lowtype,r,
									   block_m,
						      block_nxs, block_nxe, 
						      block_nys, block_nye, 
						      block_nzs, block_nze,
									   stride_xp,  stride_xm,
									   stride_yp,  stride_ym,
									   stride_zp,  stride_zm,
				   block_West_filter,block_East_filter,
				   block_South_filter,block_North_filter,
				   block_Bottom_filter,block_Top_filter,				   
									   nxblock, nyblock, nzblock,
									   mxdivblock, mydivblock, mzdivblock,
									   nxdivblock, nydivblock, nzdivblock);

  
 calc_sres_block3D_unable_bound_South_cuda_lowtype_kernel<<<blocks,threads,0,cudaStream_Low_Priority[9]>>>( 
									   A,
									   s_lowtype, z_lowtype,r,
									   block_m,
						      block_nxs, block_nxe, 
						      block_nys, block_nye, 
						      block_nzs, block_nze,
									   stride_xp,  stride_xm,
									   stride_yp,  stride_ym,
									   stride_zp,  stride_zm,
				   block_West_filter,block_East_filter,
				   block_South_filter,block_North_filter,
				   block_Bottom_filter,block_Top_filter,				   
									   nxblock, nyblock, nzblock,
									   mxdivblock, mydivblock, mzdivblock,
									   nxdivblock, nydivblock, nzdivblock);

  calc_sres_block3D_unable_bound_North_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[10]>>>( 
									   A,
									   s_lowtype, z_lowtype,r,
									   block_m,
						      block_nxs, block_nxe, 
						      block_nys, block_nye, 
						      block_nzs, block_nze,
									   stride_xp,  stride_xm,
									   stride_yp,  stride_ym,
									   stride_zp,  stride_zm,
				   block_West_filter,block_East_filter,
				   block_South_filter,block_North_filter,
				   block_Bottom_filter,block_Top_filter,				   
									   nxblock, nyblock, nzblock,
									   mxdivblock, mydivblock, mzdivblock,
									   nxdivblock, nydivblock, nzdivblock);   
#endif
    
  // ----------------------------------------------------------
  // south (y-)
  // north (y+)
  cudaStreamSynchronize(cudaStream_Hi_Priority[2]);
  te=cpu_time();
  time[4]=time[4]+te-ts;
  ts=cpu_time();
  
  //  sleev_comm_block3D_icommSouth_lowtype_cuda(&prm,sb_s,rb_s);
#if  JUPITER_CUDA_AWARE_MPI
  sleev_comm_block3D_isendSouth_lowtype_cuda(&prm,cu_sb_s,cu_rb_s);
#else
  sleev_comm_block3D_isendSouth_lowtype_cuda(&prm,sb_s,rb_s);
#endif
  cudaStreamSynchronize(cudaStream_Hi_Priority[3]);
  te=cpu_time();
  time[5]=time[5]+te-ts;
  ts=cpu_time();
  
  //  sleev_comm_block3D_icommNorth_lowtype_cuda(&prm,sb_n,rb_n);
#if  JUPITER_CUDA_AWARE_MPI
  sleev_comm_block3D_isendNorth_lowtype_cuda(&prm,cu_sb_n,cu_rb_n);
#else
  sleev_comm_block3D_isendNorth_lowtype_cuda(&prm,sb_n,rb_n);
#endif
  sleev_comm_block3D_waitSouth_cuda(&prm);
  te=cpu_time();
  time[13]=time[13]+te-ts;
  ts=cpu_time();
  
#if  JUPITER_CUDA_AWARE_MPI
#else
  cudaMemcpyAsync(cu_rb_s, rb_s, sizeof(low_type)*block_nxz, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[2]);
#endif

#if 0
  readBackSouth_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[2]>>>(
				       cu_rb_s, s_lowtype,
				       block_nys, block_nye,
				       nxblock, nyblock, nzblock,
				       mxdivblock, mydivblock, mzdivblock,
				       nxdivblock, nydivblock, nzdivblock);
#endif
  
  sleev_comm_block3D_waitNorth_cuda(&prm);
  te=cpu_time();
  time[14]=time[14]+te-ts;
  ts=cpu_time();
  
#if  JUPITER_CUDA_AWARE_MPI
#else
  cudaMemcpyAsync(cu_rb_n, rb_n, sizeof(low_type)*block_nxz, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[3]);
#endif

#if 0
  readBackNorth_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[3]>>>(
				       cu_rb_n, s_lowtype,
				       block_nys, block_nye,
				       nxblock, nyblock, nzblock,
				       mxdivblock, mydivblock, mzdivblock,
				       nxdivblock, nydivblock, nzdivblock);
#endif  
  // top    (z+)
  // bottom (z-)
  cudaStreamSynchronize(cudaStream_Hi_Priority[0]);
  te=cpu_time();
  time[6]=time[6]+te-ts;
  ts=cpu_time();
  
  //  sleev_comm_block3D_icommBottom_lowtype_cuda(&prm,sb_b,rb_b);
#if  JUPITER_CUDA_AWARE_MPI
  sleev_comm_block3D_isendBottom_lowtype_cuda(&prm,cu_sb_b,cu_rb_b);
#else
  sleev_comm_block3D_isendBottom_lowtype_cuda(&prm,sb_b,rb_b);
#endif  
  cudaStreamSynchronize(cudaStream_Hi_Priority[1]);
  te=cpu_time();
  time[7]=time[7]+te-ts;
  ts=cpu_time();
  
  //  sleev_comm_block3D_icommTop_lowtype_cuda(&prm,sb_t,rb_t);
#if  JUPITER_CUDA_AWARE_MPI
  sleev_comm_block3D_isendTop_lowtype_cuda(&prm,cu_sb_t,cu_rb_t);
#else    
  sleev_comm_block3D_isendTop_lowtype_cuda(&prm,sb_t,rb_t);
#endif
  sleev_comm_block3D_waitBottom_cuda(&prm);
  te=cpu_time();
  time[15]=time[15]+te-ts;
  ts=cpu_time();
  
#if  JUPITER_CUDA_AWARE_MPI
#else
  cudaMemcpyAsync(cu_rb_b, rb_b, sizeof(low_type)*block_nxy, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[0]);
#endif

#if 0
  readBackBottom_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[0]>>>(
							  cu_rb_b, s_lowtype,
							  block_nzs, block_nze,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);
#endif  
  sleev_comm_block3D_waitTop_cuda(&prm);
  te=cpu_time();
  time[16]=time[16]+te-ts;
  ts=cpu_time();
  
#if  JUPITER_CUDA_AWARE_MPI
#else
  cudaMemcpyAsync(cu_rb_t, rb_t, sizeof(low_type)*block_nxy, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[1]);
#endif

#if 0
  readBackTop_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[1]>>>(
							  cu_rb_t, s_lowtype,
							  block_nzs, block_nze,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);
#endif
  
  cudaStreamSynchronize(cudaStream_Hi_Priority[2]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[3]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[0]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[1]);

  
  te=cpu_time();
  time[8]=time[8]+te-ts;
  ts=cpu_time();
  
  // -----------------------------------------------------------------

  calc_sres_block3D_able_Bottom_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[7]>>>(
							    A,s_lowtype,z_lowtype,r,
							    cu_rb_b,
							    block_m,
							    block_nzs,
						      block_nxs, block_nxe, 
						      block_nys, block_nye, 
							    stride_zm,
							    nxblock,  nyblock,  nzblock,
							    mxdivblock,  mydivblock,  mzdivblock,
							    nxdivblock,  nydivblock,  nzdivblock);
  
  calc_sres_block3D_able_Top_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[8]>>>(
	       									 A,s_lowtype,z_lowtype,r,
		       								 cu_rb_t,     
			       	       						 block_m,
		       	       							 block_nze,
						      block_nxs, block_nxe, 
						      block_nys, block_nye, 
					       					 stride_zp,
								       		 nxblock,  nyblock,  nzblock,
       						       				 mxdivblock,  mydivblock,  mzdivblock,
							       			 nxdivblock,  nydivblock,  nzdivblock);
  
  cudaStreamSynchronize(cudaStream_Low_Priority[7]);    
  cudaStreamSynchronize(cudaStream_Low_Priority[8]);
  calc_sres_block3D_able_South_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[9]>>>(
							    A,s_lowtype,z_lowtype,r,
							    cu_rb_s,
							    block_m,
							    block_nys,
						      block_nxs, block_nxe, 
						      block_nzs, block_nze, 
							    stride_ym,
							    nxblock,  nyblock,  nzblock,
							    mxdivblock,  mydivblock,  mzdivblock,
							    nxdivblock,  nydivblock,  nzdivblock);
    
  calc_sres_block3D_able_North_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[10]>>>(
							    A,s_lowtype,z_lowtype,r,
							    cu_rb_n, 
							    block_m,
							    block_nye,
						      block_nxs, block_nxe, 
						      block_nzs, block_nze, 
							    stride_yp,						     
							    nxblock,  nyblock,  nzblock,
							    mxdivblock,  mydivblock,  mzdivblock,
							    nxdivblock,  nydivblock,  nzdivblock);

  // ------------------------------------------------------------------------    
  //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
  cudaStreamSynchronize(cudaStream_Low_Priority[9]);    
  cudaStreamSynchronize(cudaStream_Low_Priority[10]);
  calc_res_solve_pre_subdividing_mat2_local_ILU_2_Bottom_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[7]>>>(
									   A, Dinv,
									   r,  s_lowtype, s, z_lowtype,
									   sr_dot_Bottom, rr_dot_Bottom,
									   block_nxs,  block_nxe, 
									   block_nys,  block_nye, 
									   block_nzs,  block_nze,
									   stride_xp,  stride_xm,
									   stride_yp,  stride_ym,
									   stride_zp,  stride_zm,
									   block_xfilterL, block_xfilterU,
									   block_yfilterL, block_yfilterU,
									   block_zfilterL, block_zfilterU,
									   block_m,
									   nxblock, nyblock, nzblock,
									   mxdivblock, mydivblock, mzdivblock,
									   nxdivblock, nydivblock, nzdivblock);   


  calc_res_solve_pre_subdividing_mat2_local_ILU_2_Top_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[8]>>>(
									   A, Dinv,
									   r,  s_lowtype, s, z_lowtype,
									   sr_dot_Top, rr_dot_Top,
									   block_nxs,  block_nxe, 
									   block_nys,  block_nye, 
									   block_nzs,  block_nze,
									   stride_xp,  stride_xm,
									   stride_yp,  stride_ym,
									   stride_zp,  stride_zm,
									   block_xfilterL, block_xfilterU,
									   block_yfilterL, block_yfilterU,
									   block_zfilterL, block_zfilterU,
									   block_m,
									   nxblock, nyblock, nzblock,
									   mxdivblock, mydivblock, mzdivblock,
									   nxdivblock, nydivblock, nzdivblock);   

  calc_solve_pre_subdividing_mat2_local_ILU_2_block3D_South_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[9]>>>( 
									   A, Dinv,
									   r,  s_lowtype, s, z_lowtype,
									   sr_dot_South, rr_dot_South,
									   block_nxs,  block_nxe, 
									   block_nys,  block_nye, 
									   block_nzs,  block_nze,
									   stride_xp,  stride_xm,
									   stride_yp,  stride_ym,
									   stride_zp,  stride_zm,
									   block_xfilterL, block_xfilterU,
									   block_yfilterL, block_yfilterU,
									   block_zfilterL, block_zfilterU,
									   block_m,
									   nxblock, nyblock, nzblock,
									   mxdivblock, mydivblock, mzdivblock,
									   nxdivblock, nydivblock, nzdivblock);   

   calc_res_solve_pre_subdividing_mat2_local_ILU_2_block3D_North_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Low_Priority[10]>>>(  
									   A, Dinv,
									   r,  s_lowtype, s, z_lowtype,
									   sr_dot_North, rr_dot_North,
									   block_nxs,  block_nxe, 
									   block_nys,  block_nye, 
									   block_nzs,  block_nze,
									   stride_xp,  stride_xm,
									   stride_yp,  stride_ym,
									   stride_zp,  stride_zm,
									   block_xfilterL, block_xfilterU,
									   block_yfilterL, block_yfilterU,
									   block_zfilterL, block_zfilterU,
									   block_m,
									   nxblock, nyblock, nzblock,
									   mxdivblock, mydivblock, mzdivblock,
									   nxdivblock, nydivblock, nzdivblock);     
  

  
  cudaDeviceSynchronize();
  te=cpu_time();
  time[9]=time[9]+te-ts;
  ts=cpu_time();
  
  sum_vec_local_cuda_kernel<<<1,warp_size>>>(sr_dot_core,sr_dot_local,5);
  sum_vec_local_cuda_kernel<<<1,warp_size>>>(rr_dot_core,rr_dot_local,5);
  
  cudaDeviceSynchronize();
  
  return 0;
  
}

int calc_solve_pre_subdividing_mat2_local_ILU_IR_block3D_OVL_lowtype_cuda(
		       mpi_prm prm,
		       low_type* A,low_type* Dinv,
		       type *r, type *s, type *q,
		       // low_type *s_lowtype,low_type *z_lowtype,
		       type alpha,
		       type *cuda_buf,
		       low_type *cuda_lowtype_buf,
		       int *block_nxs, int *block_nxe, 
		       int *block_nys, int *block_nye, 
		       int *block_nzs, int *block_nze,
		       int *stride_xp, int *stride_xm,
		       int *stride_yp, int *stride_ym,
		       int *stride_zp, int *stride_zm,
				   type *block_xfilterL,type *block_xfilterU,
				   type *block_yfilterL,type *block_yfilterU,
				   type *block_zfilterL,type *block_zfilterU,		       
		       type *block_West_filter, type *block_East_filter,
		       type *block_South_filter, type *block_North_filter,
		       type *block_Bottom_filter, type *block_Top_filter,
		       low_type *sb_b ,low_type *rb_b ,low_type *sb_t ,low_type *rb_t ,
		       low_type *sb_s ,low_type *rb_s ,low_type *sb_n ,low_type *rb_n ,
		       low_type *sb_w ,low_type *rb_w ,low_type *sb_e ,low_type *rb_e ,		       
		       cudaStream_t *cudaStream_Hi_Priority
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
  
  const int blocks  = 1024;  
  const int threads = 96;  
  low_type *s_lowtype = &cuda_lowtype_buf[0];
  low_type *z_lowtype = &cuda_lowtype_buf[block_m];

  int block_nz = nzblock * nzdivblock;
  int block_ny = nyblock * nydivblock;
  int block_nx = nxblock * nxdivblock;
  int block_nxy = block_nx * block_ny;
  int block_nxz = block_nx * block_nz;
  int block_nyz = block_ny * block_nz;

  low_type *cu_sb_b = &cuda_lowtype_buf[block_m*2 + 0];
  low_type *cu_rb_b = &cuda_lowtype_buf[block_m*2 + block_nxy*1];
  low_type *cu_sb_t = &cuda_lowtype_buf[block_m*2 + block_nxy*2];
  low_type *cu_rb_t = &cuda_lowtype_buf[block_m*2 + block_nxy*3];

  low_type *cu_sb_s = &cuda_lowtype_buf[block_m*2 + block_nxy*4];
  low_type *cu_rb_s = &cuda_lowtype_buf[block_m*2 + block_nxy*4 + block_nxz*1];
  low_type *cu_sb_n = &cuda_lowtype_buf[block_m*2 + block_nxy*4 + block_nxz*2];
  low_type *cu_rb_n = &cuda_lowtype_buf[block_m*2 + block_nxy*4 + block_nxz*3];

  low_type *cu_sb_w = &cuda_lowtype_buf[block_m*2 + block_nxy*4 + block_nxz*4];
  low_type *cu_rb_w = &cuda_lowtype_buf[block_m*2 + block_nxy*4 + block_nxz*4 + block_nyz*1];
  low_type *cu_sb_e = &cuda_lowtype_buf[block_m*2 + block_nxy*4 + block_nxz*4 + block_nyz*2];
  low_type *cu_rb_e = &cuda_lowtype_buf[block_m*2 + block_nxy*4 + block_nxz*4 + block_nyz*3];

  type *sr_dot_local = &cuda_buf[0];
  type *rr_dot_local = &cuda_buf[1];

  type *sr_dot_core   = &cuda_buf[2];
  type *sr_dot_Bottom = &cuda_buf[3];
  type *sr_dot_Top    = &cuda_buf[4];
  type *sr_dot_South  = &cuda_buf[5];
  type *sr_dot_North  = &cuda_buf[6];

  type *rr_dot_core   = &cuda_buf[7];
  type *rr_dot_Bottom = &cuda_buf[8];
  type *rr_dot_Top    = &cuda_buf[9];
  type *rr_dot_South  = &cuda_buf[10];
  type *rr_dot_North  = &cuda_buf[11];
  
  const int warp_size = 32;
  initialize_type_vec<<<1,warp_size>>>(cuda_buf,12);

#if 1
  solve_pre_subdividing_mat2_local_ILU_1_block3D_core_lowtype_cuda_kernel<<<blocks,threads>>>(
							     A, Dinv,
							     r,  s_lowtype, q,
							     alpha,
							     block_nxs,  block_nxe, 
							     block_nys,  block_nye, 
							     block_nzs,  block_nze,
							     stride_xp,  stride_xm,
							     stride_yp,  stride_ym,
							     stride_zp,  stride_zm,
							     block_xfilterL, block_xfilterU,
							     block_yfilterL, block_yfilterU,
							     block_zfilterL, block_zfilterU,
							     block_m,
							     nxblock,  nyblock,  nzblock,
							     mxdivblock,  mydivblock,  mzdivblock,
							     nxdivblock,  nydivblock,  nzdivblock);
#if 0
  solve_pre_subdividing_mat2_local_ILU_1_block3D_South_lowtype_cuda_kernel<<<blocks,threads>>>(
							     A, Dinv,
							     r,  s_lowtype, q,
							     alpha,
							     block_nxs,  block_nxe, 
							     block_nys,  block_nye, 
							     block_nzs,  block_nze,
							     stride_xp,  stride_xm,
							     stride_yp,  stride_ym,
							     stride_zp,  stride_zm,
							     block_xfilterL, block_xfilterU,
							     block_yfilterL, block_yfilterU,
							     block_zfilterL, block_zfilterU,
							     block_m,
							     nxblock,  nyblock,  nzblock,
							     mxdivblock,  mydivblock,  mzdivblock,
							     nxdivblock,  nydivblock,  nzdivblock);
  solve_pre_subdividing_mat2_local_ILU_1_block3D_North_lowtype_cuda_kernel<<<blocks,threads>>>(
							     A, Dinv,
							     r,  s_lowtype, q,
							     alpha,
							     block_nxs,  block_nxe, 
							     block_nys,  block_nye, 
							     block_nzs,  block_nze,
							     stride_xp,  stride_xm,
							     stride_yp,  stride_ym,
							     stride_zp,  stride_zm,
							     block_xfilterL, block_xfilterU,
							     block_yfilterL, block_yfilterU,
							     block_zfilterL, block_zfilterU,
							     block_m,
							     nxblock,  nyblock,  nzblock,
							     mxdivblock,  mydivblock,  mzdivblock,
							     nxdivblock,  nydivblock,  nzdivblock);
#endif  
  solve_pre_subdividing_mat2_local_ILU_1_block3D_Bottom_lowtype_cuda_kernel<<<blocks,threads>>>(
							     A, Dinv,
							     r,  s_lowtype, q,
							     alpha,
							     block_nxs,  block_nxe, 
							     block_nys,  block_nye, 
							     block_nzs,  block_nze,
							     stride_xp,  stride_xm,
							     stride_yp,  stride_ym,
							     stride_zp,  stride_zm,
							     block_xfilterL, block_xfilterU,
							     block_yfilterL, block_yfilterU,
							     block_zfilterL, block_zfilterU,
							     block_m,
							     nxblock,  nyblock,  nzblock,
							     mxdivblock,  mydivblock,  mzdivblock,
							     nxdivblock,  nydivblock,  nzdivblock);
  solve_pre_subdividing_mat2_local_ILU_1_block3D_Top_lowtype_cuda_kernel<<<blocks,threads>>>(
							     A, Dinv,
							     r,  s_lowtype, q,
							     alpha,
							     block_nxs,  block_nxe, 
							     block_nys,  block_nye, 
							     block_nzs,  block_nze,
							     stride_xp,  stride_xm,
							     stride_yp,  stride_ym,
							     stride_zp,  stride_zm,
							     block_xfilterL, block_xfilterU,
							     block_yfilterL, block_yfilterU,
							     block_zfilterL, block_zfilterU,
							     block_m,
							     nxblock,  nyblock,  nzblock,
							     mxdivblock,  mydivblock,  mzdivblock,
							     nxdivblock,  nydivblock,  nzdivblock);
  
#else  
  solve_pre_subdividing_mat2_local_ILU_1_block3D_lowtype_cuda_kernel<<<blocks,threads>>>(
							     A, Dinv,
							     r,  s_lowtype, q,
							     alpha,
							     &cuda_buf[15],  &cuda_buf[16],
							     block_nxs,  block_nxe, 
							     block_nys,  block_nye, 
							     block_nzs,  block_nze,
							     stride_xp,  stride_xm,
							     stride_yp,  stride_ym,
							     stride_zp,  stride_zm,
							     block_xfilterL, block_xfilterU,
							     block_yfilterL, block_yfilterU,
							     block_zfilterL, block_zfilterU,
							     block_m,
							     nxblock,  nyblock,  nzblock,
							     mxdivblock,  mydivblock,  mzdivblock,
							     nxdivblock,  nydivblock,  nzdivblock);
#endif
  cudaDeviceSynchronize();
  // west (x-)
  // east (x+)
  setWest_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[4]>>>(
						    cu_sb_w ,s_lowtype,
						    block_nxs, block_nxe,
						    nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);
  setEast_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[5]>>>(
						    cu_sb_e ,s_lowtype,
						    block_nxs, block_nxe,
						    nxblock, nyblock, nzblock,
						    mxdivblock, mydivblock, mzdivblock,
						    nxdivblock, nydivblock, nzdivblock);
  cudaMemcpyAsync(sb_w, cu_sb_w, sizeof(low_type)*block_nyz, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[4]);
  cudaMemcpyAsync(sb_e, cu_sb_e, sizeof(low_type)*block_nyz, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[5]);

  // south (y-)
  // north (y+)
  setSouth_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[2]>>>(
						      cu_sb_s, s_lowtype,
						      block_nys, block_nye,
						      nxblock, nyblock, nzblock,
						      mxdivblock, mydivblock, mzdivblock,
						      nxdivblock, nydivblock, nzdivblock);
  setNorth_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[3]>>>(
						      cu_sb_n , s_lowtype,
						      block_nys, block_nye,
						      nxblock, nyblock, nzblock,
						      mxdivblock, mydivblock, mzdivblock,
						      nxdivblock, nydivblock, nzdivblock);
  cudaMemcpyAsync(sb_s, cu_sb_s, sizeof(low_type)*block_nxz, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[2]);
  cudaMemcpyAsync(sb_n, cu_sb_n, sizeof(low_type)*block_nxz, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[3]);
  
  // bottom (z-)
  // top    (z+)
  setBottom_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[0]>>>(
						     cu_sb_b , s_lowtype,
						     block_nzs, block_nze,
						     nxblock, nyblock, nzblock,
						     mxdivblock, mydivblock, mzdivblock,
						     nxdivblock, nydivblock, nzdivblock);
  setTop_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[1]>>>(
						     cu_sb_t , s_lowtype,
						     block_nzs, block_nze,
						     nxblock, nyblock, nzblock,
						     mxdivblock, mydivblock, mzdivblock,
						     nxdivblock, nydivblock, nzdivblock);
  
  cudaMemcpyAsync(sb_b, cu_sb_b, sizeof(low_type)*block_nxy, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[0]);
  cudaMemcpyAsync(sb_t, cu_sb_t, sizeof(low_type)*block_nxy, cudaMemcpyDeviceToHost,cudaStream_Hi_Priority[1]);
  
  // ----------------------------------------------------------
  // west (x-)
  // east (x+)
  cudaStreamSynchronize(cudaStream_Hi_Priority[4]);
  sleev_comm_block3D_icommWest_lowtype_cuda(&prm,sb_w,rb_w);
  cudaStreamSynchronize(cudaStream_Hi_Priority[5]);
  sleev_comm_block3D_icommEast_lowtype_cuda(&prm,sb_e,rb_e);

  sleev_comm_block3D_waitWest_cuda(&prm);
  cudaMemcpyAsync(cu_rb_w, rb_w, sizeof(low_type)*block_nyz, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[6]);
  readBackWest_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[6]>>>(
							 cu_rb_w, s_lowtype,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);
  
  sleev_comm_block3D_waitEast_cuda(&prm);
  cudaMemcpyAsync(cu_rb_e, rb_e, sizeof(low_type)*block_nyz, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[6]);
  readBackEast_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[6]>>>(
							 cu_rb_e , s_lowtype,
							 block_nxs, block_nxe,
							 nxblock, nyblock, nzblock,
							 mxdivblock, mydivblock, mzdivblock,
							 nxdivblock, nydivblock, nzdivblock);
  // ----------------------------------------------------------
#if 1
 calc_res_solve_pre_subdividing_mat2_local_ILU_2_block3D_core_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[6]>>>(
									   A, Dinv,
									   r,  s_lowtype, s, z_lowtype,
									   sr_dot_core, rr_dot_core,
									   block_nxs,  block_nxe, 
									   block_nys,  block_nye, 
									   block_nzs,  block_nze,
									   stride_xp,  stride_xm,
									   stride_yp,  stride_ym,
									   stride_zp,  stride_zm,
									   block_xfilterL, block_xfilterU,
									   block_yfilterL, block_yfilterU,
									   block_zfilterL, block_zfilterU,
									   block_m,
									   nxblock, nyblock, nzblock,
									   mxdivblock, mydivblock, mzdivblock,
									   nxdivblock, nydivblock, nzdivblock);
#endif  
  // ----------------------------------------------------------  

  // south (y-)
  // north (y+)
  cudaStreamSynchronize(cudaStream_Hi_Priority[2]);
  sleev_comm_block3D_icommSouth_lowtype_cuda(&prm,sb_s,rb_s);
  cudaStreamSynchronize(cudaStream_Hi_Priority[3]);
  sleev_comm_block3D_icommNorth_lowtype_cuda(&prm,sb_n,rb_n);
 
  sleev_comm_block3D_waitSouth_cuda(&prm); 
  cudaMemcpyAsync(cu_rb_s, rb_s, sizeof(low_type)*block_nxz, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[2]);
  readBackSouth_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[2]>>>(
				       cu_rb_s, s_lowtype,
				       block_nys, block_nye,
				       nxblock, nyblock, nzblock,
				       mxdivblock, mydivblock, mzdivblock,
				       nxdivblock, nydivblock, nzdivblock);
  
  sleev_comm_block3D_waitNorth_cuda(&prm);
  cudaMemcpyAsync(cu_rb_n, rb_n, sizeof(low_type)*block_nxz, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[3]);
  readBackNorth_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[3]>>>(
				       cu_rb_n, s_lowtype,
				       block_nys, block_nye,
				       nxblock, nyblock, nzblock,
				       mxdivblock, mydivblock, mzdivblock,
				       nxdivblock, nydivblock, nzdivblock);
  
  // top    (z+)
  // bottom (z-)
  cudaStreamSynchronize(cudaStream_Hi_Priority[0]);
  sleev_comm_block3D_icommBottom_lowtype_cuda(&prm,sb_b,rb_b);
  
  cudaStreamSynchronize(cudaStream_Hi_Priority[1]);
  sleev_comm_block3D_icommTop_lowtype_cuda(&prm,sb_t,rb_t);
  
  sleev_comm_block3D_waitBottom_cuda(&prm);
  cudaMemcpyAsync(cu_rb_b, rb_b, sizeof(low_type)*block_nxy, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[0]);
  readBackBottom_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[0]>>>(
							  cu_rb_b, s_lowtype,
							  block_nzs, block_nze,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);
  
  sleev_comm_block3D_waitTop_cuda(&prm);  
  cudaMemcpyAsync(cu_rb_t, rb_t, sizeof(low_type)*block_nxy, cudaMemcpyHostToDevice,cudaStream_Hi_Priority[1]);
  readBackTop_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[1]>>>(
							  cu_rb_t, s_lowtype,
							  block_nzs, block_nze,
							  nxblock, nyblock, nzblock,
							  mxdivblock, mydivblock, mzdivblock,
							  nxdivblock, nydivblock, nzdivblock);

  cudaStreamSynchronize(cudaStream_Hi_Priority[2]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[3]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[0]);
  cudaStreamSynchronize(cudaStream_Hi_Priority[1]);  
  //  calc_solve_pre_subdividing_mat2_local_ILU_2_block3D_South_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[6]>>>(
    calc_solve_pre_subdividing_mat2_local_ILU_2_block3D_South_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[2]>>>( 
									   A, Dinv,
									   r,  s_lowtype, s, z_lowtype,
									   sr_dot_South, rr_dot_South,
									   block_nxs,  block_nxe, 
									   block_nys,  block_nye, 
									   block_nzs,  block_nze,
									   stride_xp,  stride_xm,
									   stride_yp,  stride_ym,
									   stride_zp,  stride_zm,
									   block_xfilterL, block_xfilterU,
									   block_yfilterL, block_yfilterU,
									   block_zfilterL, block_zfilterU,
									   block_m,
									   nxblock, nyblock, nzblock,
									   mxdivblock, mydivblock, mzdivblock,
									   nxdivblock, nydivblock, nzdivblock);   
    //  calc_res_solve_pre_subdividing_mat2_local_ILU_2_block3D_North_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[6]>>>(
      calc_res_solve_pre_subdividing_mat2_local_ILU_2_block3D_North_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[3]>>>(  
									   A, Dinv,
									   r,  s_lowtype, s, z_lowtype,
									   sr_dot_North, rr_dot_North,
									   block_nxs,  block_nxe, 
									   block_nys,  block_nye, 
									   block_nzs,  block_nze,
									   stride_xp,  stride_xm,
									   stride_yp,  stride_ym,
									   stride_zp,  stride_zm,
									   block_xfilterL, block_xfilterU,
									   block_yfilterL, block_yfilterU,
									   block_zfilterL, block_zfilterU,
									   block_m,
									   nxblock, nyblock, nzblock,
									   mxdivblock, mydivblock, mzdivblock,
									   nxdivblock, nydivblock, nzdivblock);     
  
      //  calc_res_solve_pre_subdividing_mat2_local_ILU_2_Bottom_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[6]>>>(
        calc_res_solve_pre_subdividing_mat2_local_ILU_2_Bottom_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[0]>>>(
									   A, Dinv,
									   r,  s_lowtype, s, z_lowtype,
									   sr_dot_Bottom, rr_dot_Bottom,
									   block_nxs,  block_nxe, 
									   block_nys,  block_nye, 
									   block_nzs,  block_nze,
									   stride_xp,  stride_xm,
									   stride_yp,  stride_ym,
									   stride_zp,  stride_zm,
									   block_xfilterL, block_xfilterU,
									   block_yfilterL, block_yfilterU,
									   block_zfilterL, block_zfilterU,
									   block_m,
									   nxblock, nyblock, nzblock,
									   mxdivblock, mydivblock, mzdivblock,
									   nxdivblock, nydivblock, nzdivblock);   

	// calc_res_solve_pre_subdividing_mat2_local_ILU_2_Top_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[6]>>>(
	 calc_res_solve_pre_subdividing_mat2_local_ILU_2_Top_block3D_lowtype_cuda_kernel<<<blocks,threads,0,cudaStream_Hi_Priority[1]>>>(
									   A, Dinv,
									   r,  s_lowtype, s, z_lowtype,
									   sr_dot_Top, rr_dot_Top,
									   block_nxs,  block_nxe, 
									   block_nys,  block_nye, 
									   block_nzs,  block_nze,
									   stride_xp,  stride_xm,
									   stride_yp,  stride_ym,
									   stride_zp,  stride_zm,
									   block_xfilterL, block_xfilterU,
									   block_yfilterL, block_yfilterU,
									   block_zfilterL, block_zfilterU,
									   block_m,
									   nxblock, nyblock, nzblock,
									   mxdivblock, mydivblock, mzdivblock,
									   nxdivblock, nydivblock, nzdivblock);   

  
  cudaDeviceSynchronize();
  
  sum_vec_local_cuda_kernel<<<1,warp_size>>>(sr_dot_core,sr_dot_local,5);
  sum_vec_local_cuda_kernel<<<1,warp_size>>>(rr_dot_core,rr_dot_local,5);
  
  cudaDeviceSynchronize();
  
  return 0;
}

int solve_pre_subdividing_mat2_local_OVL_block3D_lowtype_cuda(
				   mpi_prm prm,low_type* A,low_type* Dinv,
				   type *r, type *s,type *q,
				   type alpha,
				   type* cuda_buf,
				   low_type* cuda_lowtype_buf,
				   low_type *sb_b ,low_type *rb_b ,low_type *sb_t ,low_type *rb_t ,
				   low_type *sb_s ,low_type *rb_s ,low_type *sb_n ,low_type *rb_n ,
				   low_type *sb_w ,low_type *rb_w ,low_type *sb_e ,low_type *rb_e ,
				   int *block_nxs, int *block_nxe, 
				   int *block_nys, int *block_nye, 
				   int *block_nzs, int *block_nze,
				   int *stride_xp, int *stride_xm,
				   int *stride_yp, int *stride_ym,
				   int *stride_zp, int *stride_zm,
				   type *block_xfilterL,type *block_xfilterU,
				   type *block_yfilterL,type *block_yfilterU,
				   type *block_zfilterL,type *block_zfilterU,
				   type *block_West_filter,type *block_East_filter,
				   type *block_South_filter,type *block_North_filter,
				   type *block_Bottom_filter,type *block_Top_filter,				   
				   cudaStream_t *cudaStream_Hi_Priority 
						  ){
  
  int block_m = prm.block_m;
  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  const int blocks  = 1024;  
  const int threads = 96;  

  type *sr_dot_local = &cuda_buf[0];
  type *rr_dot_local = &cuda_buf[1];

  low_type *s_lowtype = &cuda_lowtype_buf[0];
  low_type *z_lowtype = &cuda_lowtype_buf[block_m];

  const int warp_size = 32;
  initialize_type_vec<<<1,warp_size>>>(sr_dot_local,1);
  initialize_type_vec<<<1,warp_size>>>(rr_dot_local,1);
  solve_pre_subdividing_mat2_local_ILU_1_block3D_lowtype_cuda_kernel<<<blocks,threads>>>(
							     A, Dinv,
							     r,  s_lowtype, q,
							     alpha,
							     sr_dot_local, rr_dot_local,
							     block_nxs,  block_nxe, 
							     block_nys,  block_nye, 
							     block_nzs,  block_nze,
							     stride_xp,  stride_xm,
							     stride_yp,  stride_ym,
							     stride_zp,  stride_zm,
							     block_xfilterL, block_xfilterU,
							     block_yfilterL, block_yfilterU,
							     block_zfilterL, block_zfilterU,
							     block_m,
							     nxblock,  nyblock,  nzblock,
							     mxdivblock,  mydivblock,  mzdivblock,
							     nxdivblock,  nydivblock,  nzdivblock);
      calc_sres_block3D_OVL_lowtype_cuda(
				  prm,
				  A,
				  s_lowtype,z_lowtype,r,
				  block_nxs,  block_nxe, 
				  block_nys,  block_nye, 
				  block_nzs,  block_nze,
				  stride_xp,stride_xm,
				  stride_yp,stride_ym,
				  stride_zp,stride_zm,
				  block_West_filter, block_East_filter,
				  block_South_filter, block_North_filter,
				  block_Bottom_filter, block_Top_filter,
				  &cuda_lowtype_buf[block_m*2],
				  sb_b,   rb_b,   sb_t,   rb_t,
				  sb_s,   rb_s,   sb_n,   rb_n,
				  sb_w,   rb_w,   sb_e,   rb_e,
				  cudaStream_Hi_Priority
					 );
      
 initialize_type_vec<<<1,warp_size>>>(sr_dot_local,1);
 initialize_type_vec<<<1,warp_size>>>(rr_dot_local,1);
 solve_pre_subdividing_mat2_local_ILU_2_block3D_lowtype_cuda_kernel<<<blocks,threads>>>(
									   A, Dinv,
									   r,  s_lowtype, s, z_lowtype,
									   sr_dot_local, rr_dot_local,
									   block_nxs,  block_nxe, 
									   block_nys,  block_nye, 
									   block_nzs,  block_nze,
									   stride_xp,  stride_xm,
									   stride_yp,  stride_ym,
									   stride_zp,  stride_zm,
									   block_xfilterL, block_xfilterU,
									   block_yfilterL, block_yfilterU,
									   block_zfilterL, block_zfilterU,
									   block_m,
									   nxblock, nyblock, nzblock,
									   mxdivblock, mydivblock, mzdivblock,
									   nxdivblock, nydivblock, nzdivblock);

 return 0;

}


int solve_pre_subdividing_mat2_local_OVL2_block3D_lowtype_cuda(
				   mpi_prm prm,low_type* A,low_type* Dinv,
				   type *r, type *s,type *q,
				   type alpha,
				   type* cuda_buf,
				   low_type* cuda_lowtype_buf,
				   low_type *sb_b ,low_type *rb_b ,low_type *sb_t ,low_type *rb_t ,
				   low_type *sb_s ,low_type *rb_s ,low_type *sb_n ,low_type *rb_n ,
				   low_type *sb_w ,low_type *rb_w ,low_type *sb_e ,low_type *rb_e ,
				   int *block_nxs, int *block_nxe, 
				   int *block_nys, int *block_nye, 
				   int *block_nzs, int *block_nze,
				   int *stride_xp, int *stride_xm,
				   int *stride_yp, int *stride_ym,
				   int *stride_zp, int *stride_zm,
				   type *block_xfilterL,type *block_xfilterU,
				   type *block_yfilterL,type *block_yfilterU,
				   type *block_zfilterL,type *block_zfilterU,
				   type *block_West_filter,type *block_East_filter,
				   type *block_South_filter,type *block_North_filter,
				   type *block_Bottom_filter,type *block_Top_filter,				   
				   cudaStream_t *cudaStream_Hi_Priority ,
				   cudaStream_t *cudaStream_Low_Priority ,
				   cudaEvent_t *cudaEvent_OVL,		       
				   double *time
						  ){
  
  int block_m = prm.block_m;
  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  const int blocks  = 1024;  
  const int threads = 96;  
  
  const int warp_size = 32;
  
#if 0
  calc_solve_pre_subdividing_mat2_local_ILU_IR_block3D_OVL_lowtype_cuda(
							      prm,
							      A, Dinv,
							      r,  s, q,
							      // s_lowtype,  z_lowtype,
							      alpha,
							      cuda_buf,
							      cuda_lowtype_buf,
							      block_nxs, block_nxe, 
							      block_nys, block_nye, 
							      block_nzs, block_nze,
							      stride_xp, stride_xm,
							      stride_yp, stride_ym,
							      stride_zp, stride_zm,
				    block_xfilterL, block_xfilterU,
				    block_yfilterL, block_yfilterU,
				    block_zfilterL, block_zfilterU,
							      block_West_filter,  block_East_filter,
							      block_South_filter,  block_North_filter,
							      block_Bottom_filter,  block_Top_filter,
							      sb_b , rb_b , sb_t , rb_t ,
							      sb_s , rb_s , sb_n , rb_n ,
							      sb_w , rb_w , sb_e , rb_e ,		       
							      cudaStream_Hi_Priority
							    );
  
#else  
  cudaDeviceSynchronize();
  double ts,te;

  ts=cpu_time(); 
  type *sr_dot_local = &cuda_buf[0];
  type *rr_dot_local = &cuda_buf[1];
  initialize_type_vec<<<1,warp_size>>>(sr_dot_local,1);
  initialize_type_vec<<<1,warp_size>>>(rr_dot_local,1);  
  low_type *s_lowtype = &cuda_lowtype_buf[0];
  low_type *z_lowtype = &cuda_lowtype_buf[block_m];

  solve_pre_subdividing_mat2_local_ILU_1_block3D_lowtype_cuda_kernel<<<blocks,threads>>>(
							     A, Dinv,
							     r,  s_lowtype, q,
							     alpha,
							     sr_dot_local, rr_dot_local,
							     block_nxs,  block_nxe, 
							     block_nys,  block_nye, 
							     block_nzs,  block_nze,
							     stride_xp,  stride_xm,
							     stride_yp,  stride_ym,
							     stride_zp,  stride_zm,
							     block_xfilterL, block_xfilterU,
							     block_yfilterL, block_yfilterU,
							     block_zfilterL, block_zfilterU,
							     block_m,
							     nxblock,  nyblock,  nzblock,
							     mxdivblock,  mydivblock,  mzdivblock,
							     nxdivblock,  nydivblock,  nzdivblock);
  cudaDeviceSynchronize();
  te=cpu_time();
  time[0]=time[0]+te-ts;
  ts=cpu_time();
  
  calc_solve_pre_subdividing_mat2_local_ILU_2_block3D_OVL_lowtype_cuda(
							      prm,
							      A, Dinv,
							      r,  s_lowtype,  s,
							      z_lowtype,
							      cuda_buf,
							      &cuda_lowtype_buf[block_m*2],
							      block_nxs, block_nxe, 
							      block_nys, block_nye, 
							      block_nzs, block_nze,
							      stride_xp, stride_xm,
							      stride_yp, stride_ym,
							      stride_zp, stride_zm,
				    block_xfilterL, block_xfilterU,
				    block_yfilterL, block_yfilterU,
				    block_zfilterL, block_zfilterU,
							      block_West_filter,  block_East_filter,
							      block_South_filter,  block_North_filter,
							      block_Bottom_filter,  block_Top_filter,
							      sb_b , rb_b , sb_t , rb_t ,
							      sb_s , rb_s , sb_n , rb_n ,
							      sb_w , rb_w , sb_e , rb_e ,		       
							      cudaStream_Hi_Priority,
							      cudaStream_Low_Priority,
		       cudaEvent_OVL,		       							      
							      time
							    );
  cudaDeviceSynchronize();
  te=cpu_time();
  time[1]=time[1]+te-ts;

  
#endif
  
 return 0;

}

int solve_pre_subdividing_mat2_local_block3D_lowtype_cuda(
				   mpi_prm prm,low_type* A,low_type* Dinv,
				   type *r, type *s,type *q,
				   type alpha,
				   type* cuda_buf,
				   low_type* cuda_lowtype_buf,
				   low_type *sb_b ,low_type *rb_b ,low_type *sb_t ,low_type *rb_t ,
				   low_type *sb_s ,low_type *rb_s ,low_type *sb_n ,low_type *rb_n ,
				   low_type *sb_w ,low_type *rb_w ,low_type *sb_e ,low_type *rb_e ,
				   int *block_nxs, int *block_nxe, 
				   int *block_nys, int *block_nye, 
				   int *block_nzs, int *block_nze,
				   int *stride_xp, int *stride_xm,
				   int *stride_yp, int *stride_ym,
				   int *stride_zp, int *stride_zm,
				   type *block_xfilterL,type *block_xfilterU,
				   type *block_yfilterL,type *block_yfilterU,
				   type *block_zfilterL,type *block_zfilterU,
				   type *block_West_filter,type *block_East_filter,
				   type *block_South_filter,type *block_North_filter,
				   type *block_Bottom_filter,type *block_Top_filter,				   
				   cudaStream_t *cudaStream_Hi_Priority 
						  ){
  
  int block_m = prm.block_m;
  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  const int blocks  = 1024;  
  const int threads = 96;  

  type *sr_dot_local = &cuda_buf[0];
  type *rr_dot_local = &cuda_buf[1];

  low_type *s_lowtype = &cuda_lowtype_buf[0];
  low_type *z_lowtype = &cuda_lowtype_buf[block_m];

  const int warp_size = 32;
  
  initialize_type_vec<<<1,warp_size>>>(sr_dot_local,1);
  initialize_type_vec<<<1,warp_size>>>(rr_dot_local,1);
  solve_pre_subdividing_mat2_local_ILU_1_block3D_lowtype_cuda_kernel<<<blocks,threads>>>(
							     A, Dinv,
							     r,  s_lowtype, q,
							     alpha,
							     sr_dot_local, rr_dot_local,
							     block_nxs,  block_nxe, 
							     block_nys,  block_nye, 
							     block_nzs,  block_nze,
							     stride_xp,  stride_xm,
							     stride_yp,  stride_ym,
							     stride_zp,  stride_zm,
							     block_xfilterL, block_xfilterU,
							     block_yfilterL, block_yfilterU,
							     block_zfilterL, block_zfilterU,
							     block_m,
							     nxblock,  nyblock,  nzblock,
							     mxdivblock,  mydivblock,  mzdivblock,
							     nxdivblock,  nydivblock,  nzdivblock);

#ifdef JUPITER_MPI
   sleev_comm_hostmem_block3D_lowtype_cuda(
				   s_lowtype, &prm,
				   block_nxs,  block_nxe, 
				   block_nys,  block_nye, 
				   block_nzs,  block_nze,
				   &cuda_lowtype_buf[block_m*2],
				   sb_b,   rb_b,   sb_t,   rb_t,
				   sb_s,   rb_s,   sb_n,   rb_n,
				   sb_w,   rb_w,   sb_e,   rb_e,
				   cudaStream_Hi_Priority
				   );
#endif
   
 initialize_type_vec<<<1,warp_size>>>(sr_dot_local,1);
 initialize_type_vec<<<1,warp_size>>>(rr_dot_local,1);

 cudaDeviceSynchronize();
 calc_res_solve_pre_subdividing_mat2_local_ILU_2_block3D_lowtype_cuda_kernel<<<blocks,threads>>>(
									   A, Dinv,
									   r,  s_lowtype, s, z_lowtype,
									   sr_dot_local, rr_dot_local,
									   block_nxs,  block_nxe, 
									   block_nys,  block_nye, 
									   block_nzs,  block_nze,
									   stride_xp,  stride_xm,
									   stride_yp,  stride_ym,
									   stride_zp,  stride_zm,
									   block_xfilterL, block_xfilterU,
									   block_yfilterL, block_yfilterU,
									   block_zfilterL, block_zfilterU,
									   block_m,
									   nxblock, nyblock, nzblock,
									   mxdivblock, mydivblock, mzdivblock,
									   nxdivblock, nydivblock, nzdivblock);

 return 0;

}

int solve_pre_subdividing_mat0_local_block3D_lowtype_cuda(
				   mpi_prm prm,low_type* A,low_type* Dinv,
				   type *r, type *s,
				   type* cuda_buf,
				   low_type* cuda_lowtype_buf,
				   low_type *sb_b ,low_type *rb_b ,low_type *sb_t ,low_type *rb_t ,
				   low_type *sb_s ,low_type *rb_s ,low_type *sb_n ,low_type *rb_n ,
				   low_type *sb_w ,low_type *rb_w ,low_type *sb_e ,low_type *rb_e ,
				   int *block_nxs, int *block_nxe, 
				   int *block_nys, int *block_nye, 
				   int *block_nzs, int *block_nze,
				   int *stride_xp, int *stride_xm,
				   int *stride_yp, int *stride_ym,
				   int *stride_zp, int *stride_zm,
				   type *block_xfilterL,type *block_xfilterU,
				   type *block_yfilterL,type *block_yfilterU,
				   type *block_zfilterL,type *block_zfilterU,
				   type *block_West_filter,type *block_East_filter,
				   type *block_South_filter,type *block_North_filter,
				   type *block_Bottom_filter,type *block_Top_filter,				   
				   cudaStream_t *cudaStream_Hi_Priority 
						  ){

  int block_m = prm.block_m;
  type *q = &cuda_buf[0];
  solve_pre_subdividing_mat2_local_block3D_lowtype_cuda(
				   prm, A, Dinv,
				   r, s, q,
				   0.0,
				   &cuda_buf[block_m],
				   cuda_lowtype_buf,
				   sb_b ,rb_b ,sb_t ,rb_t ,
				   sb_s ,rb_s ,sb_n ,rb_n ,
				   sb_w ,rb_w ,sb_e ,rb_e , 
				   block_nxs, block_nxe, 
				   block_nys, block_nye, 
				   block_nzs, block_nze,
				   stride_xp, stride_xm,
				   stride_yp, stride_ym,
				   stride_zp, stride_zm,
				   block_xfilterL,block_xfilterU,
				   block_yfilterL,block_yfilterU,
				   block_zfilterL,block_zfilterU,
				   block_West_filter,block_East_filter,
				   block_South_filter,block_North_filter,
				   block_Bottom_filter,block_Top_filter,
				   cudaStream_Hi_Priority 
					   );

  return 0;
}
