#ifndef HEADER_PCGBLOCK3DMATVECCUDA
#define HEADER_PCGBLOCK3DMATVECCUDA
__global__ void setEast_axpy_block3D_cuda_kernel(
						type *sb_e ,type *y,type *x,type beta,
					       int *block_nxe, 
					       int *block_nys, int *block_nye,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void setWest_axpy_block3D_cuda_kernel(
					       type *sb_w ,type *y,type *x,type beta,
					       int *block_nxs, 
					       int *block_nys, int *block_nye,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void setNorth_axpy_block3D_cuda_kernel(
					       type *sb_n ,type *y,type *x,type beta,
					       int *block_nye,
					       int *block_nxs, int *block_nxe,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void setSouth_axpy_block3D_cuda_kernel(
					       type *sb_s, type *y,type *x,type beta,
					       int *block_nys, 
					       int *block_nxs, int *block_nxe,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void setTop_axpy_block3D_cuda_kernel(
					       type *sb_t ,type *y,type *x,type beta,
					       int *block_nze,
					       int *block_nxs, int *block_nxe,
					       int *block_nys, int *block_nye,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void setBottom_axpy_block3D_cuda_kernel(
					       type *sb_b, type *y,type *x,type beta,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
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
						     const int nxdivblock,const int nydivblock,const int nzdivblock);
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
						     const int nxdivblock,const int nydivblock,const int nzdivblock);
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
						     const int nxdivblock,const int nydivblock,const int nzdivblock);
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
						     const int nxdivblock,const int nydivblock,const int nzdivblock);
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
						     const int nxdivblock,const int nydivblock,const int nzdivblock);


JUPITER_DECL
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
				      );

#endif
