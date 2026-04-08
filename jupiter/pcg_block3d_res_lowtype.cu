#include <cuda_runtime.h>
//#include <cuda_bf16.h>
//#include <cuda_fp16.h>
#include "cublas_v2.h"

#include "struct.h"
#include "cg.h"

__global__ void calc_sres_block3D_unable_bound_core_cuda_lowtype_kernel(
						     low_type* A,
						     low_type* x,low_type* r,type* b,
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

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
#if 0
  const int nzdivblock_core = nzdivblock;
  const int nydivblock_core = nydivblock;
  const int nxdivblock_core = nxdivblock;

  const int ncoreloop = nxdivblock_core * nydivblock_core * nzdivblock_core;
  const int domain = ncoreloop;
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    const int jzb =  jb / (nxdivblock_core*nydivblock_core);
    const int jyb = (jb%(nxdivblock_core*nydivblock_core))/nxdivblock_core;
    const int jxb = (jb%(nxdivblock_core*nydivblock_core))%nxdivblock_core;    
#else  
  const int nzdivblock_core = nzdivblock-2;
  const int nydivblock_core = nydivblock-2;
  const int nxdivblock_core = nxdivblock;

  const int ncoreloop = nxdivblock_core * nydivblock_core * nzdivblock_core;
  const int domain = ncoreloop;
  for(int jb=tid;jb<domain;jb+=(blockDim.x*gridDim.x)){
    const int jzb =  jb / (nxdivblock_core*nydivblock_core) + 1;
    const int jyb = (jb%(nxdivblock_core*nydivblock_core))/nxdivblock_core + 1;
    const int jxb = (jb%(nxdivblock_core*nydivblock_core))%nxdivblock_core;
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


__global__ void calc_sres_block3D_unable_bound_South_cuda_lowtype_kernel(
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

__global__ void calc_sres_block3D_unable_bound_North_lowtype_cuda_kernel(
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

__global__ void calc_sres_block3D_unable_bound_Bottom_lowtype_cuda_kernel(
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

__global__ void calc_sres_block3D_unable_bound_Top_lowtype_cuda_kernel(
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


__global__ void calc_sres_block3D_able_South_cuda_kernel(
						     low_type* A,low_type* x,low_type* r,type* b,
						     low_type* rb_s,
						     const  int block_m,  
						     int *block_nys,
						     int *block_nxs, int *block_nxe, 
						     int *block_nzs, int *block_nze,  
						     int *stride_ym,
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
	int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int jj = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);

	const int stym = stride_ym[jjyb];
	const int jcs = j + stym;
	r[j] = r[j] - A[j   + 1 * block_m] * rb_s[jj] ;
	//	r[j] = r[j] - A[j   + 1 * block_m]*x[jcs] ;

      }
    }
  }  

  return ;

}

  
__global__ void calc_sres_block3D_able_North_cuda_kernel(
						     low_type* A,low_type* x,low_type* r,type* b,
						     low_type* rb_n,						     
						     const  int block_m,
						     int *block_nye,
						     int *block_nxs, int *block_nxe, 
						     int *block_nzs, int *block_nze,  
						     int *stride_yp,						     
						     const int nxblock,const int nyblock,const int nzblock,
						     const int mxdivblock,const int mydivblock,const int mzdivblock,
						     const int nxdivblock,const int nydivblock,const int nzdivblock){

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
	r[j] = r[j] - A[jcn + 1 * block_m] * rb_n[jj] ;
	//	r[j] = r[j] - A[jcn + 1 * block_m] * x[jcn] ;
      }
    }
  }
  
  
  return ;

}

__global__ void calc_sres_block3D_able_Bottom_cuda_kernel(
						     low_type* A,low_type* x,low_type* r,type* b,
						     low_type* rb_b,
						     const  int block_m,
						     int *block_nzs,
						     int *block_nxs, int *block_nxe, 
						     int *block_nys, int *block_nye,  
						     int *stride_zm,
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
	//	int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	//	int jj = jxb + jzb*nxdivblock + jjxb * (nxdivblock*nzdivblock) + jjzb * (nxdivblock*nzdivblock*nxblock);
	int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int jj = jxb + jyb*nxdivblock + jjxb * (nxdivblock*nydivblock) + jjyb * (nxdivblock*nydivblock*nxblock);
	
	const int stzm = stride_zm[jjzb];
	const int jcb = j + stzm;
	r[j] = r[j] - A[j   + 0 * block_m] * rb_b[jj] ;
	//	r[j] = r[j] - A[j   + 0 * block_m] * x[jcb] ;
      }
    }
  }
  
  return ;

}
 
__global__ void calc_sres_block3D_able_Top_cuda_kernel(
						     low_type* A,low_type* x,low_type* r,type* b,
						     low_type* rb_t,						     
						     const  int block_m,
						     int *block_nze,
						     int *block_nxs, int *block_nxe, 
						     int *block_nys, int *block_nye,  
						     int *stride_zp,
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
	int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	int jj   = jxb + jyb*nxdivblock + jjxb * (nxdivblock*nydivblock) + jjyb * (nxdivblock*nydivblock*nxblock);
	// sb_t[j] = f[jj];
	const int stzp = stride_zp[jjzb];	
	const int jct = j + stzp;	
	//	r[j] = r[j] - A[jct + 0 * block_m] * x[jct];
	r[j] = r[j] - A[jct + 0 * block_m] * rb_t[jj];	
      }
    }
  }
  return ;

}

 
