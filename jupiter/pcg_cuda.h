#ifndef HEADER_PCGCUDA
#define HEADER_PCGCUDA


__global__ void BF16toFP64(
			   low_type *BF16,double *FP64,int ndim);
__global__ void FP64toBF16(
			   type *FP64,low_type *BF16,int ndim);
__global__ void FP64toFP32(
			   double *FP64,float *FP32,int ndim);
__global__ void FP32toFP64(
			   float *FP32,double *FP64,int ndim);

JUPITER_DECL
int view_vec_gpu(const type* const vec,mpi_prm prm);
__global__ void initialize_type_vec(
			       type* __restrict__ y, // Output vector
			       int m);
__global__ void axpy_cuda_kernel(
				type a,type* x,type* y,
				int stm,
				int nx,int ny,int nz,
				int m,
				int mx,int my,int mxy
				);
__global__ void sum_vec_local_cuda_kernel(
				     const type* const __restrict__ vec1,   // input vector 1 
				     type* __restrict__ dot_result,         // dot product of the result vector
				     int m);

JUPITER_DECL
int axpy_cuda(mpi_prm prm,type a,type* x,type* y);
__global__ void pjacobi_cuda_kernel(
				type* D,type* x,type* y,
				int stm,
				int nx,int ny,int nz,
				int m,
				int mx,int my,int mxy
				);
JUPITER_DECL
int pjacobi_cuda(mpi_prm prm,type* D,type* x,type* y);
__global__ void MatVec_dot_local_cuda_kernel(
					     type* A,type* x,type* y,type* tmp,
					     int stm,
					     int nx,int ny,int nz,
					     int m,
					     int mx,int my,int mxy
				  );
JUPITER_DECL
void MatVec_dot_local_cuda(mpi_prm prm,type* A,type* x,type* y,type* cuda_buf);
__global__ void sMatVec_dot_local_cuda_kernel(
					      type* A,type* x,type* y,type* tmp,
					      int stm,
					      int nx,int ny,int nz,
					      int m,
					      int mx,int my,int mxy
				  );
JUPITER_DECL
void sMatVec_dot_local_cuda(mpi_prm prm,type* A,type* x,type* y,type* cuda_buf);
__global__ void axpy2_cuda_kernel(
		       type a,type b,type* x,type* y,type* z,
		       int stm,
		       int nx,int ny,int nz,
		       int m,
		       int mx,int my,int mxy
		       );
JUPITER_DECL
void axpy2_cuda(mpi_prm prm,type a,type b,type* x,type* y,type* z);
__global__ void calc_res_cuda_kernel(
			  const type* const __restrict__ A, // Matrix A
			  const type* const __restrict__ x, // Input  vector
			  const type* const __restrict__ b, // Input  vector
			  type* __restrict__ r,             // Output vector
			  int stm,
			  int nx,int ny,int nz,
			  int m,
			  int mx,int my,int mxy
			  );
JUPITER_DECL
int calc_res_cuda(mpi_prm prm, type *A, type *x,type *b,type *r);
__global__ void calc_sres_cuda_kernel(
			  const type* const __restrict__ A, // Matrix A
			  const type* const __restrict__ x, // Input  vector
			  const type* const __restrict__ b, // Input  vector
			  type* __restrict__ r,             // Output vector
			  int stm,
			  int nx,int ny,int nz,
			  int m,
			  int mx,int my,int mxy
			  );
JUPITER_DECL
int calc_sres_cuda(mpi_prm prm, type *A, type *x,type *b,type *r);
__global__ void solve_pjacobi_mat2_local_cuda_kernel(
						     type *A,
						     type *r, type *s,type *q,
						     type alpha,type *tmp,type *tmpn,
						     int stm,
						     int nx,int ny,int nz,
						     int m,
						     int mx,int my,int mxy
						     );
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
				     );
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
								  );
JUPITER_DECL
int solve_pre_subdividing_mat2_local_cuda(
				     mpi_prm prm, type *A,type *Dinv,
				     type *r, type *s,type *q,
				     type alpha,type *tmp,type *tmpn,
				     type *block_yfilterL,type *block_zfilterL,
				     type *block_yfilterU,type *block_zfilterU,
				     int *block_ys,int *block_zs,
				     int *block_ye,int *block_ze,
				     type *cuda_type_buf
				     );
JUPITER_DECL
int solve_pre_subdividing_mat0_cuda(
			       mpi_prm prm, type *A,type *Dinv,
			       type *r, type *s,
			       type *block_yfilterL,type *block_zfilterL,
			       type *block_yfilterU,type *block_zfilterU,
			       int *block_ys,int *block_zs,
			       int *block_ye,int *block_ze,
			       type *cuda_type_buf
			       );
__global__ void MatVec_cuda_kernel(
			  const type* const __restrict__ A, // Matrix A
			  const type* const __restrict__ x, // Input  vector
			  type* __restrict__ r,             // Output vector
			  int stm,
			  int nx,int ny,int nz,
			  int m,
			  int mx,int my,int mxy
			  );
JUPITER_DECL
int MatVec_cuda(mpi_prm prm, type *A, type *x,type *y);
__global__ void initialize_int_vec(
			       int* __restrict__ y, // Output vector
			       int m);

__global__ void calc_dot_cuda_kernel(
				     const type* const __restrict__ vec1,   // input vector 1 
				     const type* const __restrict__ vec2,   // input vector 2
				     type* __restrict__ dot_result,         // dot product of the result vector
				     int m, int stm, 
				     int nx, int ny, int nz, int mx, int mxy);
JUPITER_DECL
int cuda_MPI_Allreduce_sum(int n,type *cpumem,type *cudamem,MPI_Comm comm);
JUPITER_DECL
int calc_dot_cuda(
		  mpi_prm prm,
		  const type* const x, // input vector1
                  const type* const y, // input vector2
                  type* result_dot_cpu,    // cpu  output value
		  type* cuda_buf   );
JUPITER_DECL
int calc_norm_cuda(
		  mpi_prm prm,
		  const type* const x, // input vector1
                  const type* const y, // input vector2
                  type* result_dot_cpu,    // cpu  output value
		  type* cuda_buf   );

#include "reduciton_cuda.h"

#endif

