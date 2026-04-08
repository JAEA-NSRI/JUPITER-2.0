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

#if 0
__global__ void BF16toFP64(
			   low_type *BF16,double *FP64,int ndim){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = ndim;
  if(tid >= domain) return;
  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
    //    BF16[i] = __double2bfloat16(FP64[i]);
    FP64[i] = __bfloat162float(BF16[i]);
  }
}


__global__ void FP64toBF16(
			   double *FP64,low_type *BF16,int ndim){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = ndim;
  if(tid >= domain) return;
  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
    BF16[i] = __double2bfloat16(FP64[i]);
  }
}
#endif

__global__ void FP32toFP64(
			   float *FP32,double *FP64,int ndim){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = ndim;
  if(tid >= domain) return;
  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
    FP64[i] = FP32[i];
  }
}

__global__ void FP64toFP32(
			   double *FP64,float *FP32,int ndim){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = ndim;
  if(tid >= domain) return;
  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
    FP32[i] = FP64[i];
  }
}

int view_vec_gpu(const type* const vec,mpi_prm prm){
  int m = prm.m;
  const size_t VecSize = sizeof(type)*m  ;  

  type *vec_cpu = (type*)malloc(VecSize);  
  cudaDeviceSynchronize();
  cudaMemcpy(vec_cpu,vec, VecSize, cudaMemcpyDeviceToHost);
  cudaDeviceSynchronize();
  x_view(prm,vec_cpu);
  free(vec_cpu);
  cudaDeviceSynchronize();
  
  return 0;
}

__global__ void sum_vec_local_cuda_kernel(
				     const type* const __restrict__ vec1,   // input vector 1 
				     type* __restrict__ dot_result,         // dot product of the result vector
				     int m){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = m;

  type val = 0.0;
  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
    val = val + vec1[i] ;
  }
  local_sum_cuda(val,dot_result,threadIdx.x);

}

__global__ void initialize_type_vec(
			       type* __restrict__ y, // Output vector
			       int m){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = m;
  if(tid >= domain) return;
  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
    y[i] = 0.0;
  }  
}

//------


//   y[] = y[] + a*x[] 
__global__ void axpy_cuda_kernel(
				type a,type* x,type* y,
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
    y[j]=a*x[j]+y[j];
  }
}

int axpy_cuda(mpi_prm prm,type a,type* x,type* y){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  const int blocks  = 1024;  
  const int threads = 96;  
  axpy_cuda_kernel<<<blocks,threads>>>(
				       a,x,y,
				       stm,
				       nx,ny,nz,
				       m,
				       mx,my,mxy
				       );
  return 0;
}

//   y[] = x / D ;
__global__ void pjacobi_cuda_kernel(
				type* D,type* x,type* y,
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
    y[j]=x[j]/D[j];
  }
}

int pjacobi_cuda(mpi_prm prm,type* D,type* x,type* y){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  const int blocks  = 1024;  
  const int threads = 96;  
  pjacobi_cuda_kernel<<<blocks,threads>>>(
				       D,x,y,
				       stm,
				       nx,ny,nz,
				       m,
				       mx,my,mxy
				       );
  return 0;
}



__global__ void MatVec_dot_local_cuda_kernel(
					     type* A,type* x,type* y,type* tmp,
					     int stm,
					     int nx,int ny,int nz,
					     int m,
					     int mx,int my,int mxy
				  ){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nz*ny*nx;
  if(tid >= domain) return;

  type tmp1_;
  tmp1_=(double)0.0;

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
    
    y[j]=
      A[j+0*m]*x[jcb] 
      +A[j+1*m]*x[jcs] 
      +A[j+2*m]*x[jcw] 
      +A[j+3*m]*x[jcc] 
      +A[j+4*m]*x[jce] 
      +A[j+5*m]*x[jcn] 
      +A[j+6*m]*x[jct] 
      ;
    tmp1_=tmp1_+y[j]*x[j];
  }
  
  local_sum_cuda(tmp1_,tmp,threadIdx.x);
  
  return ;

}

void MatVec_dot_local_cuda(mpi_prm prm,type* A,type* x,type* y,type* cuda_buf){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  const int warp_size = 32;
  initialize_type_vec<<<1,warp_size>>>(cuda_buf,1);
  
  const int blocks  = 1024;  
  const int threads = 96;  

  MatVec_dot_local_cuda_kernel<<<blocks,threads>>>(
						   A, x, y, cuda_buf,
						   stm,
						   nx,ny,nz,
						   m,
						   mx,my,mxy
						   );  
  return ;
}

__global__ void sMatVec_dot_local_cuda_kernel(
					      type* A,type* x,type* y,type* tmp,
					      int stm,
					      int nx,int ny,int nz,
					      int m,
					      int mx,int my,int mxy
				  ){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nz*ny*nx;
  if(tid >= domain) return;

  type tmp1_ = (double)0.0;;
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

#if 0
    y[j]=
      A[j+0*m]*x[jcb] 
      +A[j+1*m]*x[jcs] 
      +A[j+2*m]*x[jcw] 
      +A[j+3*m]*x[jcc] 
      +A[j+4*m]*x[jce] 
      +A[j+5*m]*x[jcn] 
      +A[j+6*m]*x[jct] 
      ;
#else
    y[j]=
       A[j   + 0*m]*x[jcb] 
      +A[j   + 1*m]*x[jcs] 
      +A[j   + 2*m]*x[jcw] 
      +A[j   + 3*m]*x[jcc] 
      +A[jce + 2*m]*x[jce] 
      +A[jcn + 1*m]*x[jcn] 
      +A[jct + 0*m]*x[jct] 
      ;
#endif

    tmp1_=tmp1_+y[j]*x[j];
  }
  
  local_sum_cuda(tmp1_,tmp,threadIdx.x);
  
  return ;

}

void sMatVec_dot_local_cuda(mpi_prm prm,type* A,type* x,type* y,type* cuda_buf){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
 const int warp_size = 32;
 initialize_type_vec<<<1,warp_size>>>(cuda_buf,1);

 const int blocks  = 1024;  
 const int threads = 96;  

 sMatVec_dot_local_cuda_kernel<<<blocks,threads>>>(
 //  MatVec_dot_local_cuda_kernel<<<blocks,threads>>>(
    A, x, y, cuda_buf,      
      stm,
      nx,ny,nz,
      m,
      mx,my,mxy
      );  
  return ;
}

//z[] = z[] + a*y[]
//y[] = x[] + b*y[]
__global__ void axpy2_cuda_kernel(
		       type a,type b,type* x,type* y,type* z,
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
      z[j]=a*y[j]+z[j];
      y[j]=b*y[j]+x[j];
  }
  return ;  
}

void axpy2_cuda(mpi_prm prm,type a,type b,type* x,type* y,type* z){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  const int blocks  = 1024;  
  const int threads = 96;  
  axpy2_cuda_kernel<<<blocks,threads>>>(
		    a, b, x, y, z,
		    stm,
		    nx, ny, nz,
		    m,
		    mx, my, mxy
		    );
  return ;
}

__global__ void calc_res_cuda_kernel(
			  const type* const __restrict__ A, // Matrix A
			  const type* const __restrict__ x, // Input  vector
			  const type* const __restrict__ b, // Input  vector
			  type* __restrict__ r,             // Output vector
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
      r[j] = b[j] - (
		       A[j + 0 * m] * x[jcb]
		     + A[j + 1 * m] * x[jcs]
		     + A[j + 2 * m] * x[jcw]
		     + A[j + 3 * m] * x[jcc]
		     + A[j + 4 * m] * x[jce]
		     + A[j + 5 * m] * x[jcn]
		     + A[j + 6 * m] * x[jct] )
	;
  }
}

int calc_res_cuda(mpi_prm prm, type *A, type *x,type *b,type *r){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  const int blocks  = 1024;  
  const int threads = 96;  

  calc_res_cuda_kernel<<<blocks,threads>>>(
					   A,x,b,r,
					   stm,
					   nx, ny, nz,
					   m,
					   mx, my, mxy);    

  return 0;
}

__global__ void calc_sres_cuda_kernel(
			  const type* const __restrict__ A, // Matrix A
			  const type* const __restrict__ x, // Input  vector
			  const type* const __restrict__ b, // Input  vector
			  type* __restrict__ r,             // Output vector
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
#if 0
      r[j] = b[j] - (
		       A[j + 0 * m] * x[jcb]
		     + A[j + 1 * m] * x[jcs]
		     + A[j + 2 * m] * x[jcw]
		     + A[j + 3 * m] * x[jcc]
		     + A[j + 4 * m] * x[jce]
		     + A[j + 5 * m] * x[jcn]
		     + A[j + 6 * m] * x[jct] )
	;
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

int calc_sres_cuda(mpi_prm prm, type *A, type *x,type *b,type *r){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  const int blocks  = 1024;  
  const int threads = 96;  

  calc_sres_cuda_kernel<<<blocks,threads>>>(
					   A,x,b,r,
					   stm,
					   nx, ny, nz,
					   m,
					   mx, my, mxy);    

  return 0;
}


__global__ void solve_pjacobi_mat2_local_cuda_kernel(
						     type *A,
						     type *r, type *s,type *q,
						     type alpha,type *tmp,type *tmpn,
						     int stm,
						     int nx,int ny,int nz,
						     int m,
						     int mx,int my,int mxy
						     ){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nz*ny*nx;
  if(tid >= domain) return;

  type tmp1_ = 0.0;
  type tmp2_ = 0.0;

  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
    int j = (((i % (ny*nx))%nx)+stm) + 
      mx*(((i % (ny*nx))/nx)+stm) + 
      mxy*((i / (ny*nx))+stm);    
    r[j]  = r[j] + q[j] * alpha;
    s[j]  = r[j] / A[j+3*m];
    tmp1_ = tmp1_+s[j]*r[j];
    tmp2_ = tmp2_+r[j]*r[j];
  }
  local_sum_cuda(tmp1_,tmp,threadIdx.x);
  local_sum_cuda(tmp2_,tmpn,threadIdx.x);

  return ;
}

__global__ void solve_pre_subdividing_mat2_local_ILU_1_cuda_kernel(
						 type *A,type *Dinv,
						 type *r, type *s,type *q,
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

#if 1
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = zdivblock*ydivblock *nx;
  if(tid >= domain) return;

  int j;
  int jz,jx,jy;
  int jcs,jcb,jcn,jct;

  //  int jcw,jcs,jcb,jce,jcn,jct,jcc;
  int jxblock,jyblock,jzblock;

  //  for(jx = 0; jx < nx; jx++) {
    for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
      jx      = ( i%(nx*ydivblock) ) % nx;
      jyblock =   i%(nx*ydivblock)   / nx;
      jzblock = i / (nx*ydivblock);
      int jzs = block_zs[jzblock];
      int jze = block_ze[jzblock];    
      //    for(jyblock = 0; jyblock < ydivblock; jyblock++) {
      int jys = block_ys[jyblock];
      int jye = block_ye[jyblock];

      for(jz = jzs; jz <= jze; jz++) {
	for(jy = jys; jy <= jye; jy++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  jcs = j-mx  ;
	  jcb = j-mxy ;
	  r[j] = r[j]+q[j]*alpha;
	  s[j] = ( r[j] - ( 
			   A[j+1*m]*s[jcs] * block_yfilterL[ jy-jys + jyblock*nyblock ]  + 
			   A[j+0*m]*s[jcb] * block_zfilterL[ jz-jzs + jzblock*nzblock ] 
			    ) ) * Dinv[j];
	}      
      }
      for(jz = jze; jz >=jzs ; jz--) {
	for(jy = jye; jy >=jys ; jy--) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  jcn = j+mx  ;
	  jct = j+mxy ;
	  s[j] = s[j] - (
			 A[j+5*m]*s[jcn] * block_yfilterU[ jy-jys + jyblock*nyblock ] + 
			 A[j+6*m]*s[jct] * block_zfilterU[ jz-jzs + jzblock*nzblock ]
			 )*Dinv[j];
	}
      }      
    }
    //  }

#else
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = zdivblock*ydivblock;
  if(tid >= domain) return;

  int j;
  int jz,jx,jy;
  int jcw,jcs,jcb,jce,jcn,jct,jcc;

  int jxblock,jyblock,jzblock;

  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
    jzblock = i / ydivblock;
    jyblock = i % ydivblock;
    int jzs = block_zs[jzblock];
    int jze = block_ze[jzblock];    
    //    for(jyblock = 0; jyblock < ydivblock; jyblock++) {
    int jys = block_ys[jyblock];
    int jye = block_ye[jyblock];

    for(jz = jzs; jz <= jze; jz++) {
      for(jy = jys; jy <= jye; jy++) {
	for(jx = 0; jx < nx; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  jcs = j-mx  ;
	  jcb = j-mxy ;
	  r[j] = r[j]+q[j]*alpha;
	  s[j] = ( r[j] - ( 
			   A[j+1*m]*s[jcs] * block_yfilterL[ jy-jys + jyblock*nyblock ]  + 
			   A[j+0*m]*s[jcb] * block_zfilterL[ jz-jzs + jzblock*nzblock ] 
			    ) ) * Dinv[j];
	}
      }
    }
    for(jz = jze; jz >=jzs ; jz--) {
      for(jy = jye; jy >=jys ; jy--) {
	for(jx = nx-1; jx >=0 ; jx--) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  jcn = j+mx  ;
	  jct = j+mxy ;
	  s[j] = s[j] - (
			 A[j+5*m]*s[jcn] * block_yfilterU[ jy-jys + jyblock*nyblock ] + 
			 A[j+6*m]*s[jct] * block_zfilterU[ jz-jzs + jzblock*nzblock ]
			 )*Dinv[j];
	}
      }
    }
  }
#endif

  return ;
}

__global__ void solve_pre_subdividing_mat2_local_ILU_2_cuda_kernel(
								  type *A,type *Dinv,
								  type *r, type *s,
								  type *z,
								  type *tmp,type *tmpn,
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
#if 1
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = zdivblock*ydivblock *nx;
  if(tid >= domain) return;
 
  type tmp1_ = 0.0;
  type tmp2_ = 0.0;

  //  int jxblock,jyblock,jzblock;

  // solve Ay = z
  // s = s + y

  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
    int jx      = ( i%(nx*ydivblock) ) % nx;
    int jyblock =   i%(nx*ydivblock)   / nx;
    int jzblock = i / (nx*ydivblock);
    int jzs = block_zs[jzblock];
    int jze = block_ze[jzblock];    
    int jys = block_ys[jyblock];
    int jye = block_ye[jyblock];

      for(int jz = jzs; jz <= jze; jz++) {
	for(int jy = jys; jy <= jye; jy++) {
	    int j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    int jcs = j-mx  ;
	    int jcb = j-mxy ;

	    z[j] = ( z[j] - ( 
			     A[j+1*m]*z[jcs] * block_yfilterL[ jy-jys + jyblock*nyblock ]  + 
			     A[j+0*m]*z[jcb] * block_zfilterL[ jz-jzs + jzblock*nzblock ] 
			      ) ) * Dinv[j];

	}
      }
      for(int jz = jze; jz >=jzs ; jz--) {
	for(int jy = jye; jy >=jys ; jy--) {
	    int j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    int jcn = j+mx  ;
	    int jct = j+mxy ;

	    z[j] = z[j] - (
			   A[j+5*m]*z[jcn] * block_yfilterU[ jy-jys + jyblock*nyblock ] + 
			   A[j+6*m]*z[jct] * block_zfilterU[ jz-jzs + jzblock*nzblock ]
			   )*Dinv[j];
	    s[j] = s[j] + z[j] ;

	    tmp1_ = tmp1_+s[j]*r[j];
	    tmp2_ = tmp2_+r[j]*r[j];
	}
      }

    }

#else
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = zdivblock*ydivblock;
  if(tid >= domain) return;
 
  type tmp1_ = 0.0;
  type tmp2_ = 0.0;

  int jxblock,jyblock,jzblock;

  // solve Ay = z
  // s = s + y

  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
    jzblock = i / ydivblock;
    jyblock = i % ydivblock;
    int jzs = block_zs[jzblock];
    int jze = block_ze[jzblock];    
    int jys = block_ys[jyblock];
    int jye = block_ye[jyblock];

      for(int jz = jzs; jz <= jze; jz++) {
	for(int jy = jys; jy <= jye; jy++) {
	  for(int jx = 0; jx < nx; jx++) {
	    int j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    int jcs = j-mx  ;
	    int jcb = j-mxy ;

	    z[j] = ( z[j] - ( 
			     A[j+1*m]*z[jcs] * block_yfilterL[ jy-jys + jyblock*nyblock ]  + 
			     A[j+0*m]*z[jcb] * block_zfilterL[ jz-jzs + jzblock*nzblock ] 
			      ) ) * Dinv[j];

	  }
	}
      }
      for(int jz = jze; jz >=jzs ; jz--) {
	for(int jy = jye; jy >=jys ; jy--) {
	  for(int jx = nx-1; jx >=0 ; jx--) {
	    int j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    int jcn = j+mx  ;
	    int jct = j+mxy ;

	    z[j] = z[j] - (
			   A[j+5*m]*z[jcn] * block_yfilterU[ jy-jys + jyblock*nyblock ] + 
			   A[j+6*m]*z[jct] * block_zfilterU[ jz-jzs + jzblock*nzblock ]
			   )*Dinv[j];
	    s[j] = s[j] + z[j] ;

	    tmp1_ = tmp1_+s[j]*r[j];
	    tmp2_ = tmp2_+r[j]*r[j];
	  }
	}
      }

    }
#endif  

  local_sum_cuda(tmp1_,tmp ,threadIdx.x);
  local_sum_cuda(tmp2_,tmpn,threadIdx.x);

  return ;
}

int solve_pre_subdividing_mat2_local_cuda(
				     mpi_prm prm, type *A,type *Dinv,
				     type *r, type *s,type *q,
				     type alpha,type *tmp,type *tmpn,
				     type *block_yfilterL,type *block_zfilterL,
				     type *block_yfilterU,type *block_zfilterU,
				     int *block_ys,int *block_zs,
				     int *block_ye,int *block_ze,
				     type *cuda_type_buf
				     ){
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

  type *z = &cuda_type_buf[(nx*ny+nx*nz+ny*nz)*2*2];

  int xdivblock = prm.xdivblock;
  int ydivblock = prm.ydivblock;
  int zdivblock = prm.zdivblock;
  int nxblock   = prm.nxblock;
  int nyblock   = prm.nyblock;
  int nzblock   = prm.nzblock;

  solve_pre_subdividing_mat2_local_ILU_1_cuda_kernel<<<blocks,threads>>>(
									A,Dinv,
									r,s,q,
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

  //  view_vec_gpu(r,prm);
  //  view_vec_gpu(s,prm);

#ifdef JUPITER_MPI
  sleev_comm_cuda(s,&prm,cuda_type_buf);
#endif

  // IR 
  // z = r - As
#if sMatrix
  calc_sres_cuda(prm,A,s,r,z);
#else
  calc_res_cuda(prm,A,s,r,z);
#endif
  //  view_vec_gpu(z,prm);
  // solve Ay = z
  // s = s + y
  solve_pre_subdividing_mat2_local_ILU_2_cuda_kernel<<<blocks,threads>>>(
						     A,Dinv,
						     r,s,
						     z,
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

  //  view_vec_gpu(s,prm);									  
  return 0;
}

int solve_pre_subdividing_mat0_cuda(
			       mpi_prm prm, type *A,type *Dinv,
			       type *r, type *s,
			       type *block_yfilterL,type *block_zfilterL,
			       type *block_yfilterU,type *block_zfilterU,
			       int *block_ys,int *block_zs,
			       int *block_ye,int *block_ze,
			       type *cuda_type_buf
			       ){

  int m = prm.m;
  type *q = cuda_type_buf;
  //  initialize_type_vec<<<cuda_blocks_m,cuda_threads>>>(q,m);

  const int warp_size = 32;
  initialize_type_vec<<<1,warp_size>>>(&cuda_type_buf[m],2);
  type alpha = 0.0;
  type *tmp  = &cuda_type_buf[m];
  type *tmpn = &cuda_type_buf[m+1];

  //  solve_pre_subdividing_mat2_local(
  solve_pre_subdividing_mat2_local_cuda(
				   prm, A,Dinv,
				   r,s,q,
				   alpha,tmp,tmpn,
				   block_yfilterL, block_zfilterL,
				   block_yfilterU, block_zfilterU,
				   block_ys, block_zs,
				   block_ye, block_ze,
				   &cuda_type_buf[m+2]
				   );
  return 0;
}


__global__ void MatVec_cuda_kernel(
			  const type* const __restrict__ A, // Matrix A
			  const type* const __restrict__ x, // Input  vector
			  type* __restrict__ r,             // Output vector
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
      r[j] = 
	  A[j + 0 * m] * x[jcb]
	+ A[j + 1 * m] * x[jcs]
	+ A[j + 2 * m] * x[jcw]
	+ A[j + 3 * m] * x[jcc]
	+ A[j + 4 * m] * x[jce]
	+ A[j + 5 * m] * x[jcn]
	+ A[j + 6 * m] * x[jct] ;
  }
  return ;
}

int MatVec_cuda(mpi_prm prm, type *A, type *x,type *y){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  const int blocks  = 1024;  
  const int threads = 96;  

  MatVec_cuda_kernel<<<blocks,threads>>>(
					   A,x,y,
					   stm,
					   nx, ny, nz,
					   m,
					   mx, my, mxy);    

  return 0;
}

__global__ void initialize_int_vec(
			       int* __restrict__ y, // Output vector
			       int m){
  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = m;
  if(tid >= domain) return;
  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
    y[i] = 0;
  }  
}

__global__ void calc_dot_cuda_kernel(
				     const type* const __restrict__ vec1,   // input vector 1 
				     const type* const __restrict__ vec2,   // input vector 2
				     type* __restrict__ dot_result,         // dot product of the result vector
				     int m, int stm, 
				     int nx, int ny, int nz, int mx, int mxy){

  const int tid = blockIdx.x * blockDim.x + threadIdx.x;
  const int domain = nz*ny*nx;

  type val = 0.0;
  for(int i=tid;i<domain;i+=(blockDim.x*gridDim.x)){
    int j = (((i % (ny*nx))%nx)+stm) + 
      mx*(((i % (ny*nx))/nx)+stm) + 
      mxy*((i / (ny*nx))+stm);

    val += vec1[j] * vec2[j];
  }
#if 1
   local_sum_cuda(val,dot_result,threadIdx.x);
#else
  val = warpReduceSum(val);
  int lane = threadIdx.x % warpSize;
  if(lane==0)atomicAdd(&dot_result[0], val);
#endif

}

int cuda_MPI_Allreduce_sum(int n,type *cpumem,type *cudamem,MPI_Comm comm){
  type sbuf[n];
  const size_t commSize = sizeof(type)*n;
  cudaMemcpy(sbuf,cudamem,commSize,cudaMemcpyDeviceToHost);
  cudaDeviceSynchronize();
  //  printf(" sbuf   = %e \n",sbuf[0]);
  MPI_Allreduce(sbuf,cpumem,n,MPI_TYPE,MPI_SUM,comm);
  //   printf(" cpumem = %e \n",cpumem[0]);
  //  MPI_Allreduce(sbuf,cpumem,n,MPI_TYPE,MPI_SUM,MPI_COMM_WORLD);
  cudaMemcpy(cudamem,cpumem,commSize,cudaMemcpyHostToDevice);
  return 0;
}

// 内積結果はcpu側
int calc_dot_cuda(
		  mpi_prm prm,
		  const type* const x, // input vector1
                  const type* const y, // input vector2
                  type* result_dot_cpu,    // cpu  output value
		  type* cuda_buf   ){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  const int warp_size = 32;
  initialize_type_vec<<<1,warp_size>>>(cuda_buf,1);
  
  const int blocks  = 1024;  
  const int threads = 96;  
  calc_dot_cuda_kernel<<<blocks,threads>>>(x,
					   y,
					   cuda_buf,
					   m, stm, 
					   nx,ny,nz,  mx, mxy);
  cuda_MPI_Allreduce_sum(1,result_dot_cpu,cuda_buf,prm.comm);

  return 0;
}

int calc_norm_cuda(
		  mpi_prm prm,
		  const type* const x, // input vector1
                  const type* const y, // input vector2
                  type* result_dot_cpu,    // cpu  output value
		  type* cuda_buf   ){  
  calc_dot_cuda(prm,x,y,result_dot_cpu,cuda_buf);

  int n = 1;
  int i;
  for(i=0;i<n;i++){
    result_dot_cpu[i] = sqrt(result_dot_cpu[i]);
  }
  const size_t nSize = sizeof(type)*n;
  cudaMemcpy(result_dot_cpu,result_dot_cpu,nSize,cudaMemcpyDeviceToHost);

  return 0;
} 

