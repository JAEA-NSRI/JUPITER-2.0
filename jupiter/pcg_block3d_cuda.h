#ifndef HEADER_PCGBLOCK3DCUDA
#define HEADER_PCGBLOCK3DCUDA

#include "common.h"

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
					      const int nxdivblock,const int nydivblock,const int nzdivblock);

JUPITER_DECL
int calc_sres_block3D_cuda(
			   mpi_prm prm,type* A,type* x,type* b,type* r,type* cuda_buf,
			   int *block_nxs, int *block_nxe, 
			   int *block_nys, int *block_nye, 
			   int *block_nzs, int *block_nze,
			   int *stride_xp,int *stride_xm,
			   int *stride_yp,int *stride_ym,
			   int *stride_zp,int *stride_zm 
			   );
__global__ void  axpy_block3D_cuda_kernel(
					  type *x,type *y,type alpha,type beta,
					  int *block_nxs, int *block_nxe, 
					  int *block_nys, int *block_nye, 
					  int *block_nzs, int *block_nze,
					  int nxblock,int nyblock,int nzblock,
					  int mxdivblock,int mydivblock,int mzdivblock,
					  int nxdivblock,int nydivblock,int nzdivblock);

JUPITER_DECL
int axpy_block3D_cuda(
		      mpi_prm prm,type *x,type *y,type alpha,type beta,
		      int *block_nxs, int *block_nxe, 
		      int *block_nys, int *block_nye, 
		      int *block_nzs, int *block_nze
		      );

__global__ void axpy2_block3D_cuda_kernel(
					  type alpha,type beta,type *x,type *y,type *z,
					  int *block_nxs, int *block_nxe, 
					  int *block_nys, int *block_nye, 
					  int *block_nzs, int *block_nze,
					  int nxblock,int nyblock,int nzblock,
					  int mxdivblock,int mydivblock,int mzdivblock,
					  int nxdivblock,int nydivblock,int nzdivblock);

JUPITER_DECL
int axpy2_block3D_cuda(
		       mpi_prm prm,type alpha,type beta,type *x,type *y,type *z,
		       int *block_nxs, int *block_nxe, 
		       int *block_nys, int *block_nye, 
		       int *block_nzs, int *block_nze
		       );

  
__global__ void calc_dot_local_block3D_cuda_kernel(
						   type *x,type *y,type *ret_type,
						   int *block_nxs, int *block_nxe, 
						   int *block_nys, int *block_nye, 
						   int *block_nzs, int *block_nze,
						   const int nxblock,const int nyblock,const int nzblock,
						   const int mxdivblock,const int mydivblock,const int mzdivblock,
						   const int nxdivblock,const int nydivblock,const int nzdivblock);

JUPITER_DECL
int calc_dot_local_block3D_cuda(
				mpi_prm prm,type *x,type *y,type *cuda_buf,
				int *block_nxs, int *block_nxe, 
				int *block_nys, int *block_nye, 
				int *block_nzs, int *block_nze
				);

JUPITER_DECL
int calc_dot_block3D_cuda(
			  mpi_prm prm,type *x,type *y,type *ret_type_cpu,type *cuda_buf,
			  int *block_nxs, int *block_nxe, 
			  int *block_nys, int *block_nye, 
			  int *block_nzs, int *block_nze
			  );

JUPITER_DECL
int calc_norm_block3D_cuda(
			   mpi_prm prm,type *x,type *y,type *ret_type_cpu,type *cuda_buf,
			   int *block_nxs, int *block_nxe, 
			   int *block_nys, int *block_nye, 
			   int *block_nzs, int *block_nze
			   );

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
						      const int nxdivblock,const int nydivblock,const int nzdivblock);
JUPITER_DECL
int sMatVec_dot_local_block3D_cuda(
				   mpi_prm prm,type* A,type* x,type* y,type *cuda_buf,
				   int *block_nxs, int *block_nxe, 
				   int *block_nys, int *block_nye, 
				   int *block_nzs, int *block_nze,				   
				   int *stride_xp,int *stride_xm,
				   int *stride_yp,int *stride_ym,
				   int *stride_zp,int *stride_zm
				   );

#include "reduciton_cuda.h"
#include "pcg_cuda.h"
#endif
