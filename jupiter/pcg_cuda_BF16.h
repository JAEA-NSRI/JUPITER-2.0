#ifndef HEADER_PCGCUDABF16
#define HEADER_PCGCUDABF16

#include "common.h"
__global__ void initialize_low_type_vec(
			       low_type* __restrict__ y, // Output vector
			       int m);
__global__ void calc_res_cuda_kernel_BF16(
			  const low_type* const __restrict__ A, // Matrix A
			  const low_type* const __restrict__ x, // Input  vector
			  const type* const __restrict__ b, // Input  vector
			  low_type* __restrict__ r,             // Output vector
			  int stm,
			  int nx,int ny,int nz,
			  int m,
			  int mx,int my,int mxy
			  );
JUPITER_DECL
int calc_res_cuda_BF16(mpi_prm prm, low_type *A, low_type *x,type *b,low_type *r);
__global__ void calc_sres_cuda_kernel_BF16(
			  const low_type* const __restrict__ A, // Matrix A
			  const low_type* const __restrict__ x, // Input  vector
			  const low_type* const __restrict__ b, // Input  vector
			  low_type* __restrict__ r,             // Output vector
			  int stm,
			  int nx,int ny,int nz,
			  int m,
			  int mx,int my,int mxy
			  );
JUPITER_DECL
int calc_sres_cuda_BF16(mpi_prm prm, low_type *A, low_type *x,type *b,low_type *r);
__global__ void solve_pre_subdividing_mat2_local_ILU_1_cuda_kernel_BF16(
						 low_type *A,low_type *Dinv,
						 type *r, low_type *s,type *q,
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
								  int nxblock   , int nyblock   , int nzblock   ,
								  int stm,
								  int nx,int ny,int nz,
								  int m,
								  int mx,int my,int mxy
								  );
JUPITER_DECL
int solve_pre_subdividing_mat2_local_cuda_BF16(
				     mpi_prm prm, low_type *A,low_type *Dinv,
				     type *r, type *s,type *q,
				     type alpha,type *tmp,type *tmpn,
				     type *block_yfilterL,type *block_zfilterL,
				     type *block_yfilterU,type *block_zfilterU,
				     int *block_ys,int *block_zs,
				     int *block_ye,int *block_ze,
				     low_type *cuda_type_buf
				     );

JUPITER_DECL
int solve_pre_subdividing_mat0_cuda_BF16(
			       mpi_prm prm, low_type *A,low_type *Dinv,
			       type *r, type *s,
			       type *block_yfilterL,type *block_zfilterL,
			       type *block_yfilterU,type *block_zfilterU,
			       int *block_ys,int *block_zs,
			       int *block_ye,int *block_ze,
			       low_type *cuda_type_buf
			       );

#endif



