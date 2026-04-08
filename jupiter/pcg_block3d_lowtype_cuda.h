#ifndef HEADER_PCGBLOCK3DLOWTYPECUDA
#define HEADER_PCGBLOCK3DLOWTYPECUDA
#include "common.h"
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
						     const int nxdivblock,const int nydivblock,const int nzdivblock);
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
						     const int nxdivblock,const int nydivblock,const int nzdivblock);
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
		       );
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
		       );
int zero_initialize_lowtype(int m,low_type *x);
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
								      const int nxdivblock,const int nydivblock,const int nzdivblock);

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
								      const int nxdivblock,const int nydivblock,const int nzdivblock);

__global__ void solve_pre_subdividing_mat2_local_nxnynz_ILU_1_block3D_lowtype_cuda_kernel(
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
										  const int nxdivblock,const int nydivblock,const int nzdivblock);

__global__ void solve_pre_subdividing_mat2_local_nxnynz_ILU_2_block3D_lowtype_cuda_kernel(
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
										  const int nxdivblock,const int nydivblock,const int nzdivblock);

JUPITER_DECL
int solve_pre_subdividing_mat2_local_nxnynz_block3D_lowtype_cuda(
				   mpi_prm prm,low_type* A,low_type* Dinv,
				   type *r, type *s,type *q, 				   
				   type alpha,
				   type* cuda_buf,
				   low_type* cuda_lowtype_buf,
				   low_type* cpu_lowtype_buf,
				   int *block_nxs, int *block_nxe, 
				   int *block_nys, int *block_nye, 
				   int *block_nzs, int *block_nze,
				   int *stride_xp, int *stride_xm,
				   int *stride_yp, int *stride_ym,
				   int *stride_zp, int *stride_zm,
				   type *block_xfilterL,type *block_xfilterU,
				   type *block_yfilterL,type *block_yfilterU,
				   type *block_zfilterL,type *block_zfilterU
					     );
JUPITER_DECL
int solve_pre_subdividing_mat0_local_nxnynz_block3D_lowtype_cuda(
				   mpi_prm prm,low_type* A,low_type* Dinv,
				   type *r, type *s,
				   int *block_nxs, int *block_nxe, 
				   int *block_nys, int *block_nye, 
				   int *block_nzs, int *block_nze,
				   int *stride_xp, int *stride_xm,
				   int *stride_yp, int *stride_ym,
				   int *stride_zp, int *stride_zm,
				   type *block_xfilterL,type *block_xfilterU,
				   type *block_yfilterL,type *block_yfilterU,
				   type *block_zfilterL,type *block_zfilterU,
				   type *cuda_buf,
				   low_type *cuda_lowtype_buf,
				   low_type *cpu_lowtype_buf
							 );
  
#endif
