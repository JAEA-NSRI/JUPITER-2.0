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

#include "sleev_comm_cuda.h"
#include "pcg_cuda.h"
#include "pcg_cuda_BF16.h"


__global__ void initialize_low_type_vec(
			       low_type* __restrict__ y, // Output vector
			       int m){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = m;
  if(tid >= domain) return;
  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
    y[i] = 0.0;
  }  
}

__global__ void calc_res_cuda_kernel_BF16(
			  const low_type* const __restrict__ A, // Matrix A
			  const low_type* const __restrict__ x, // Input  vector
			  const type* const __restrict__ b, // Input  vector
			  low_type* __restrict__ r,             // Output vector
			  int stm,
			  int nx,int ny,int nz,
			  int m,
			  int mx,int my,int mxy
			  ){
  
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nz*ny*nx;
  if(tid >= domain) return;

  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
      int j = (((i % (ny*nx))%nx)+stm) + 
                 mx*(((i % (ny*nx))/nx)+stm) + 
                 mxy*((i / (ny*nx))+stm);

      int jcb = j - mxy;
      int jcs = j - mx;
      int jcw = j - 1;
      int jcc = j;
      int jce = j + 1;
      int jcn = j + mx;
      int jct = j + mxy;

#if precon_BF16
      float val =   
	     b[j] - (
		       __bfloat162float( A[j + 0 * m] ) * __bfloat162float( x[jcb] )
		     + __bfloat162float( A[j + 1 * m] ) * __bfloat162float( x[jcs] )
		     + __bfloat162float( A[j + 2 * m] ) * __bfloat162float( x[jcw] )
		     + __bfloat162float( A[j + 3 * m] ) * __bfloat162float( x[jcc] )
		     + __bfloat162float( A[j + 4 * m] ) * __bfloat162float( x[jce] )
		     + __bfloat162float( A[j + 5 * m] ) * __bfloat162float( x[jcn] )
		     + __bfloat162float( A[j + 6 * m] ) * __bfloat162float( x[jct] ) ) 
	;
      r[j] = __float2bfloat16(val);
#else
      r[j] = b[j] - (
		       A[j + 0 * m] * x[jcb]
		     + A[j + 1 * m] * x[jcs]
		     + A[j + 2 * m] * x[jcw]
		     + A[j + 3 * m] * x[jcc]
		     + A[j + 4 * m] * x[jce]
		     + A[j + 5 * m] * x[jcn]
		     + A[j + 6 * m] * x[jct] )
	;
#endif
  }
}

int calc_res_cuda_BF16(mpi_prm prm, low_type *A, low_type *x,type *b,low_type *r){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  const int blocks  = 1024;  
  const int threads = 96;  

  calc_res_cuda_kernel_BF16<<<blocks,threads>>>(
					   A,x,b,r,
					   stm,
					   nx, ny, nz,
					   m,
					   mx, my, mxy);    

  return 0;
}

__global__ void calc_sres_cuda_kernel_BF16(
			  const low_type* const __restrict__ A, // Matrix A
			  const low_type* const __restrict__ x, // Input  vector
			  const type* const __restrict__ b, // Input  vector
			  low_type* __restrict__ r,             // Output vector
			  int stm,
			  int nx,int ny,int nz,
			  int m,
			  int mx,int my,int mxy
			  ){
  
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nz*ny*nx;
  if(tid >= domain) return;

  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
      int j = (((i % (ny*nx))%nx)+stm) + 
                 mx*(((i % (ny*nx))/nx)+stm) + 
                 mxy*((i / (ny*nx))+stm);

      int jcb = j - mxy;
      int jcs = j - mx;
      int jcw = j - 1;
      int jcc = j;
      int jce = j + 1;
      int jcn = j + mx;
      int jct = j + mxy;
#if precon_BF16
      float val =   
     	         b[j]  - (
		       __bfloat162float( A[j   + 0*m] ) * __bfloat162float( x[jcb]  )
		     + __bfloat162float( A[j   + 1*m] ) * __bfloat162float( x[jcs]  )
		     + __bfloat162float( A[j   + 2*m] ) * __bfloat162float( x[jcw]  )
		     + __bfloat162float( A[j   + 3*m] ) * __bfloat162float( x[jcc]  )
		     + __bfloat162float( A[jce + 2*m] ) * __bfloat162float( x[jce]  )
		     + __bfloat162float( A[jcn + 1*m] ) * __bfloat162float( x[jcn]  )
		     + __bfloat162float( A[jct + 0*m] ) * __bfloat162float( x[jct]  ) )		     
	;
      r[j] = __float2bfloat16(val);

#else
      r[j] = b[j] - (
		      A[j   + 0*m]*x[jcb] 
		     +A[j   + 1*m]*x[jcs] 
		     +A[j   + 2*m]*x[jcw] 
		     +A[j   + 3*m]*x[jcc] 
		     +A[jce + 2*m]*x[jce] 
		     +A[jcn + 1*m]*x[jcn] 
		     +A[jct + 0*m]*x[jct] 
		     )
	;
#endif

  }
}

int calc_sres_cuda_BF16(mpi_prm prm, low_type *A, low_type *x,type *b,low_type *r){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  const int blocks  = 1024;  
  const int threads = 96;  

  calc_sres_cuda_kernel_BF16<<<blocks,threads>>>(
					   A,x,b,r,
					   stm,
					   nx, ny, nz,
					   m,
					   mx, my, mxy);    

  return 0;
}



__global__ void solve_pre_subdividing_mat2_local_ILU_1_cuda_kernel_BF16(
						 low_type *A,low_type *Dinv,
						 type *r,low_type *s,type *q,
						 type alpha,
						 type *block_yfilterL,type *block_zfilterL,
 						 type *block_yfilterU,type *block_zfilterU,
						 int *block_ys,int *block_zs,
						 int *block_ye,int *block_ze,
						 int xdivblock , int ydivblock , int zdivblock ,
						 int nxblock   , int nyblock   , int nzblock   ,
						 int stm,
						 int nx,int ny,int nz,
						 int m,
						 int mx,int my,int mxy
				     ){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = zdivblock*ydivblock *nx;
  if(tid >= domain) return;
 
  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
    int jx      = ( i%(nx*ydivblock) ) % nx;
    int jyblock =   i%(nx*ydivblock)   / nx;
    int jzblock = i / (nx*ydivblock);

    int jzs = block_zs[jzblock];
    int jze = block_ze[jzblock];    
    //    for(jyblock = 0; jyblock < ydivblock; jyblock++) {
    int jys = block_ys[jyblock];
    int jye = block_ye[jyblock];

#if precon_BF16
    float s_FP32[ (SUBDIVIDING_YBLOCKSIZE+1+2)*(SUBDIVIDING_ZBLOCKSIZE+1+2) ];    
    for(int j = 0; j < ( (SUBDIVIDING_YBLOCKSIZE+1+2)*(SUBDIVIDING_ZBLOCKSIZE+1+2) ) ; j++) {
      s_FP32[j] = 0.0;
    }
#endif

    for(int jz = jzs; jz <= jze; jz++) {
      for(int jy = jys; jy <= jye; jy++) {
	int j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	r[j] = r[j] + q[j]*alpha;
	//	r[j] = 1.0;

#if precon_BF16

#if 0
	int jj  = (jy-jys+stm) + (jz-jzs+stm)*(nyblock+2) ;
        int jcs = jj - 1;
        int jcb = jj - (nyblock+2);
	s_FP32[jj] = ( r[j] - ( 					      
			       __bfloat162float(A[j+1*m]) * s_FP32[jcs] * block_yfilterL[ jy-jys + jyblock*nyblock ] +
			       __bfloat162float(A[j+0*m]) * s_FP32[jcb] * block_zfilterL[ jz-jzs + jzblock*nzblock ]  
									      ) ) * __bfloat162float(Dinv[j]);
#else
	int jcs = j-mx  ;
	int jcb = j-mxy ;
	s[j] = ( r[j] - ( 
			 __bfloat162float(A[j+1*m]*s[jcs]) * block_yfilterL[ jy-jys + jyblock*nyblock ]  + 
			 __bfloat162float(A[j+0*m]*s[jcb]) * block_zfilterL[ jz-jzs + jzblock*nzblock ] 
			  ) ) * __bfloat162float(Dinv[j]);

#endif

#else
	int jcs = j-mx  ;
	int jcb = j-mxy ;
	s[j] = ( r[j] - ( 
			 A[j+1*m]*s[jcs] * block_yfilterL[ jy-jys + jyblock*nyblock ]  + 
			 A[j+0*m]*s[jcb] * block_zfilterL[ jz-jzs + jzblock*nzblock ] 
			  ) ) * Dinv[j];
#endif
      }
    }

    for(int jz = jze; jz >=jzs ; jz--) {
      for(int jy = jye; jy >=jys ; jy--) {
#if precon_BF16

#if 0
	int j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        int jj  = (jy-jys+stm) + (jz-jzs+stm)*(nyblock+2) ;
        int jcn = jj + 1;
        int jct = jj + (nyblock+2);

	s_FP32[jj] = s_FP32[jj] - ( 
				 __bfloat162float(A[j+5*m]) * s_FP32[jcn] * block_yfilterU[ jy-jys + jyblock*nyblock ] +
				 __bfloat162float(A[j+6*m]) * s_FP32[jct] * block_zfilterU[ jz-jzs + jzblock*nzblock ]
				  ) * __bfloat162float( Dinv[j] );
	s[j] = __float2bfloat16(s_FP32[jj]);
#else
	int j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	int jcn = j+mx  ;
	int jct = j+mxy ;
	float val = s[j] = __bfloat162float(s[j]) - (
		       __bfloat162float(A[j+5*m])*__bfloat162float(s[jcn]) * block_yfilterU[ jy-jys + jyblock*nyblock ] + 
		       __bfloat162float(A[j+6*m])*__bfloat162float(s[jct]) * block_zfilterU[ jz-jzs + jzblock*nzblock ]
		       )*__bfloat162float(Dinv[j]);

	s[j]  = __float2bfloat16(val);

#endif

#else
	int j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	int jcn = j+mx  ;
	int jct = j+mxy ;
	s[j] = s[j] - (
		       A[j+5*m]*s[jcn] * block_yfilterU[ jy-jys + jyblock*nyblock ] + 
		       A[j+6*m]*s[jct] * block_zfilterU[ jz-jzs + jzblock*nzblock ]
		       )*Dinv[j];
#endif
      }
    }
  }

  return ;

}

__global__ void solve_pre_subdividing_mat2_local_ILU_2_cuda_kernel_BF16(
									low_type *A,low_type *Dinv,
									type *r, low_type *s_in, type *s_out,
									low_type *z,
									type *tmp,type *tmpn,
									type *block_yfilterL,type *block_zfilterL,
									type *block_yfilterU,type *block_zfilterU,
									int *block_ys,int *block_zs,
									int *block_ye,int *block_ze,
									int xdivblock , int ydivblock , int zdivblock ,
									int nxblock ,const int nyblock   , const int nzblock ,
									int stm,
									const int nx,int ny,int nz,
									int m,
									int mx,int my,int mxy
									){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = zdivblock*ydivblock *nx;
  if(tid >= domain) return;
 
  type tmp1_ = 0.0;
  type tmp2_ = 0.0;

  // solve Ay = z
  // s = s + y
  //  float z_FP32[(nzblock+2)*(nyblock+2)];
  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
    int jx      = ( i%(nx*ydivblock) ) % nx;
    int jyblock =   i%(nx*ydivblock)   / nx;
    int jzblock = i / (nx*ydivblock);

    int jzs = block_zs[jzblock];
    int jze = block_ze[jzblock];    
    int jys = block_ys[jyblock];
    int jye = block_ye[jyblock];

#if precon_BF16
    float z_FP32[ (SUBDIVIDING_YBLOCKSIZE+1+2)*(SUBDIVIDING_ZBLOCKSIZE+1+2) ];    
    for(int j = 0; j < ( (SUBDIVIDING_YBLOCKSIZE+1+2)*(SUBDIVIDING_ZBLOCKSIZE+1+2) ) ; j++) {
      z_FP32[j] = 0.0;
    }
#endif

    for(int jz = jzs; jz <= jze; jz++) {
      for(int jy = jys; jy <= jye; jy++) {

#if precon_BF16

#if 0
	int j   = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	int jj  = (jy-jys+stm) + (jz-jzs+stm)*(nyblock+2) ;
	int jcs = jj - 1;
	int jcb = jj - (nyblock+2);
	z_FP32[jj] = ( __bfloat162float(z[j]) - ( 
			       __bfloat162float(A[j+1*m]) * z_FP32[jcs] * block_yfilterL[ jy-jys + jyblock*nyblock ]  + 
			       __bfloat162float(A[j+0*m]) * z_FP32[jcb] * block_zfilterL[ jz-jzs + jzblock*nzblock ] 
				) ) * __bfloat162float(Dinv[j]);
#else
	int j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	int jcs = j-mx  ;
	int jcb = j-mxy ;

	float val = ( __bfloat162float(z[j]) - ( 
  __bfloat162float(A[j+1*m])*__bfloat162float(z[jcs]) * block_yfilterL[ jy-jys + jyblock*nyblock ]  + 
  __bfloat162float(A[j+0*m])*__bfloat162float(z[jcb]) * block_zfilterL[ jz-jzs + jzblock*nzblock ] 
			  ) ) * __bfloat162float(Dinv[j]);
	z[j] = __float2bfloat16(val);

#endif

#else
	int j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	int jcs = j-mx  ;
	int jcb = j-mxy ;

	z[j] = ( z[j] - ( 
			 A[j+1*m]*z[jcs] * block_yfilterL[ jy-jys + jyblock*nyblock ]  + 
			 A[j+0*m]*z[jcb] * block_zfilterL[ jz-jzs + jzblock*nzblock ] 
			  ) ) * Dinv[j];
#endif

      }
    }
    for(int jz = jze; jz >=jzs ; jz--) {
      for(int jy = jye; jy >=jys ; jy--) {

#if precon_BF16
#if 0
	int j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	int jj  = (jy-jys+stm) + (jz-jzs+stm)*(nyblock+2) ;
	int jcn = jj + 1;
	int jct = jj + (nyblock+2);

	z_FP32[jj] = z_FP32[jj] - (
				   __bfloat162float(A[j+5*m]) * z_FP32[jcn] * block_yfilterU[ jy-jys + jyblock*nyblock ] + 
				   __bfloat162float(A[j+6*m]) * z_FP32[jct] * block_zfilterU[ jz-jzs + jzblock*nzblock ]
				   ) * __bfloat162float(Dinv[j]);
	s_out[j] = __bfloat162float(s_in[j]) + z_FP32[jj];
#else
	int j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	int jcn = j+mx  ;
	int jct = j+mxy ;
	float val = __bfloat162float(z[j]) - (
		       __bfloat162float(A[j+5*m]) * __bfloat162float(z[jcn]) * block_yfilterU[ jy-jys + jyblock*nyblock ] + 
		       __bfloat162float(A[j+6*m]) * __bfloat162float(z[jct]) * block_zfilterU[ jz-jzs + jzblock*nzblock ]
		       ) * __bfloat162float(Dinv[j]);
	z[j] = __float2bfloat16(val);
	s_out[j] = __bfloat162float(s_in[j]) + val ;
#endif

#else
	int j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	int jcn = j+mx  ;
	int jct = j+mxy ;
	z[j] = z[j] - (
		       A[j+5*m]*z[jcn] * block_yfilterU[ jy-jys + jyblock*nyblock ] + 
		       A[j+6*m]*z[jct] * block_zfilterU[ jz-jzs + jzblock*nzblock ]
		       )*Dinv[j];
	s_out[j] = s_in[j] + z[j] ;
#endif
	tmp1_ = tmp1_+s_out[j]*r[j];
	tmp2_ = tmp2_+r[j]*r[j];
      }
    }

  }
 
  local_sum_cuda(tmp1_,tmp ,threadIdx.x);
  local_sum_cuda(tmp2_,tmpn,threadIdx.x);

  return ;
}


int solve_pre_subdividing_mat2_local_cuda_BF16(
				     mpi_prm prm, low_type *A,low_type *Dinv,
				     type *r, type *s,type *q,
				     type alpha,type *tmp,type *tmpn,
				     type *block_yfilterL,type *block_zfilterL,
				     type *block_yfilterU,type *block_zfilterU,
				     int *block_ys,int *block_zs,
				     int *block_ye,int *block_ze,
				     low_type *cuda_type_buf
				     ){

#if 0
  solve_pre_subdividing_mat2_local_cuda(
				      prm,  A, Dinv,
				      r,  s, q,
				      alpha, tmp, tmpn,
				      block_yfilterL, block_zfilterL,
				      block_yfilterU, block_zfilterU,
				      block_ys, block_zs,
				      block_ye, block_ze,
				      cuda_type_buf
					);
  return ;
#endif

  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;

  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  const int warp_size = 32;
  initialize_type_vec<<<1,warp_size>>>(tmp,1);
  initialize_type_vec<<<1,warp_size>>>(tmpn,1);
  const int blocks  = 1024;  
  const int threads = 96;  

  low_type *s_BF16 = &cuda_type_buf[(nx*ny+nx*nz+ny*nz)*2*2];
  low_type *z_BF16 = &cuda_type_buf[(nx*ny+nx*nz+ny*nz)*2*2+m];

  int xdivblock = prm.xdivblock;
  int ydivblock = prm.ydivblock;
  int zdivblock = prm.zdivblock;
  int nxblock   = prm.nxblock;
  int nyblock   = prm.nyblock;
  int nzblock   = prm.nzblock;

//    printf(" -------------------- \n");
//    printf(" -- @ 1 @ solve r \n");
//    view_vec_gpu(r,prm);
//    printf(" -- @ 1 @ solve s \n");
//    view_vec_gpu(s,prm);

  solve_pre_subdividing_mat2_local_ILU_1_cuda_kernel_BF16<<<blocks,threads>>>(
//    solve_pre_subdividing_mat2_local_ILU_1_cuda_kernel<<<blocks,threads>>>(
									A,Dinv,
									r,s_BF16,q,
									// r,s,q,
									alpha,
									block_yfilterL,block_zfilterL,
									block_yfilterU,block_zfilterU,
									block_ys,block_zs,
									block_ye,block_ze,
									xdivblock,ydivblock,zdivblock,
									nxblock,nyblock,nzblock,
									stm,
									nx,ny,nz,
									m,
									mx,my,mxy);

//    printf(" -------------------- \n");
//    printf(" -- @ 2 @ solve r \n");
//    view_vec_gpu(r,prm);
//    printf(" -- @ 2 @ solve s \n");
//    view_vec_gpu(s,prm);

#ifdef JUPITER_MPI
  // ここが間違い   !!!!
  sleev_comm_cuda_BF16(s_BF16,&prm,cuda_type_buf);
  //sleev_comm_cuda(s_BF16,&prm,cuda_type_buf);
#endif

  // IR 
  // z = r - As
#if sMatrix
  calc_sres_cuda_BF16(prm,A,s_BF16,r,z_BF16);
  //  calc_sres_cuda(prm,A,s_BF16,r,z_BF16);
#else
  calc_res_cuda_BF16(prm,A,s_BF16,r,z_BF16);
  //calc_res_cuda(prm,A,s_BF16,r,z_BF16);
#endif

//    printf(" -------------------- \n");
//    printf(" -- @ 3 @ solve r \n");
//    view_vec_gpu(r,prm);
//    printf(" -- @ 3 @ solve s \n");
//    view_vec_gpu(s,prm);
  // solve Ay = z
  // s = s + y
  solve_pre_subdividing_mat2_local_ILU_2_cuda_kernel_BF16<<<blocks,threads>>>(
  //  solve_pre_subdividing_mat2_local_ILU_2_cuda_kernel<<<blocks,threads>>>(
						     A,Dinv,
						     r,s_BF16,s,
						     z_BF16,
						     tmp,tmpn,
						     block_yfilterL,block_zfilterL,
						     block_yfilterU,block_zfilterU,
						     block_ys,block_zs,
						     block_ye,block_ze,
						     xdivblock,ydivblock,zdivblock,
						     nxblock,nyblock,nzblock,
						     stm,
						     nx,ny,nz,
						     m,
						     mx,my,mxy
						     );

//  printf(" -------------------- \n");
//  printf(" -- @ 4 @ solve r \n");
//  view_vec_gpu(r,prm);
//  printf(" -- @ 4 @ solve s \n");
//  view_vec_gpu(s,prm);

  return 0;
}

int solve_pre_subdividing_mat0_cuda_BF16(
			       mpi_prm prm, low_type *A,low_type *Dinv,
			       type *r, type *s,
			       type *block_yfilterL,type *block_zfilterL,
			       type *block_yfilterU,type *block_zfilterU,
			       int *block_ys,int *block_zs,
			       int *block_ye,int *block_ze,
 			       low_type *cuda_type_buf
			       ){

//    printf(" -------------------- \n");
//    printf(" -- @ 0 @ solve r \n");
//    view_vec_gpu(r,prm);
//    printf(" -- @ 0 @ solve s \n");
//    view_vec_gpu(s,prm);

  int m = prm.m;
  //  initialize_type_vec<<<cuda_blocks_m,cuda_threads>>>(q,m);

  type *cuda_mem;
  cudaMalloc(&cuda_mem,sizeof(type)*(2+m));

  type *q = cuda_mem;

  // const int warp_size = 32;
  // initialize_type_vec<<<1,warp_size>>>(&cuda_type_buf[m],2);
  // type *tmp  = &cuda_type_buf[m];
  // type *tmpn = &cuda_type_buf[m+1];

  const int cuda_threads = 512;
  const int cuda_blocks  = ceil((double)(m+2)/cuda_threads);
  initialize_type_vec<<<cuda_blocks,cuda_threads>>>(cuda_mem,m+2);

  type alpha = 0.0;
  type *tmp  = &cuda_mem[m];
  type *tmpn = &cuda_mem[m+1];

  //  solve_pre_subdividing_mat2_local(
  solve_pre_subdividing_mat2_local_cuda_BF16(
				   prm, A,Dinv,
				   r,s,q,
				   alpha,tmp,tmpn,
				   block_yfilterL, block_zfilterL,
				   block_yfilterU, block_zfilterU,
				   block_ys, block_zs,
				   block_ye, block_ze,
				   cuda_type_buf
				   );
  cudaFree(cuda_mem);
  return 0;
}

