#ifndef HEADER_SLEEVCOMMBLOCK3DCUDA
#define HEADER_SLEEVCOMMBLOCK3DCUDA
#include "common.h"

JUPITER_DECL
int sleev_comm_hostmem_block3D_cuda(
			    type *f,mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    type *cuda_buf,
			    type *sb_b,  type *rb_b,  type *sb_t,  type *rb_t,
			    type *sb_s,  type *rb_s,  type *sb_n,  type *rb_n,
			    type *sb_w,  type *rb_w,  type *sb_e,  type *rb_e,
			    cudaStream_t *cuStream
				    );
JUPITER_DECL
int sleev_comm_block3D_commX_cuda(
				  mpi_prm *prm,
				  type *sb_w,  type *rb_w,
				  type *sb_e,  type *rb_e
				  );
JUPITER_DECL
int sleev_comm_block3D_commY_cuda(
				  mpi_prm *prm,
				  type *sb_s,  type *rb_s,
				  type *sb_n,  type *rb_n
				  );
JUPITER_DECL
int sleev_comm_block3D_commZ_cuda(
				  mpi_prm *prm,				  
				  type *sb_b,  type *rb_b,
				  type *sb_t,  type *rb_t
				  );


__global__ void setWestAndEast_block3D_cuda_kernel(
					       type *sb_w, type *sb_e ,type *f,
					       int *block_nxs, int *block_nxe,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void setNorthAndSouth_block3D_cuda_kernel(
					       type *sb_s, type *sb_n ,type *f,
					       int *block_nys, int *block_nye,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void setTopAndBottom_block3D_cuda_kernel(
					       type *sb_b, type *sb_t ,type *f,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);

__global__ void readBackWestAndEast_block3D_cuda_kernel(
					       type *rb_w, type *rb_e ,type *f,
					       int *block_nxs, int *block_nxe,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void readBackNorthAndSouth_block3D_cuda_kernel(
					       type *rb_s, type *rb_n ,type *f,
					       int *block_nys, int *block_nye,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);

__global__ void readBackTopAndBottom_block3D_cuda_kernel(
					       type *rb_b, type *rb_t ,type *f,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);

JUPITER_DECL
int sleev_comm_block3D_set_cuda(
				 type *f,mpi_prm *prm,
				 int *block_nxs, int *block_nxe, 
				 int *block_nys, int *block_nye, 
				 int *block_nzs, int *block_nze,
				 type *cuda_buf,
				 type *cpu_buf,
				 cudaStream_t *cudaStream
				);
JUPITER_DECL
int sleev_comm_block3D_comm_cuda(
			    mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    type *cuda_buf,
			    type *cpu_buf
				 );
JUPITER_DECL
int sleev_comm_block3D_icomm_cuda(
			    mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    type *cuda_buf,
			    type *cpu_buf
				  );
JUPITER_DECL
int sleev_comm_block3D_wait_cuda(mpi_prm *prm);
JUPITER_DECL
int sleev_comm_block3D_readBack_cuda(
			    type *f,mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    type *cuda_buf,
			    type *cpu_buf,
			    cudaStream_t *cudaStream
				     );


JUPITER_DECL
int sleev_comm_block3D_cuda(
			    type *f,mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    type *cuda_buf,
			    type *cpu_buf
			    );
JUPITER_DECL
 int sleev_comm_block3D_cuda_time(
			    type *f,mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    type *cuda_buf,
			    type *cpu_buf,
			    type *time
				  );


JUPITER_DECL
int sleev_comm_hostmem_block3D_cuda_time(
			    type *f,mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    type *cuda_buf,
			    type *sb_b,  type *rb_b,  type *sb_t,  type *rb_t,
			    type *sb_s,  type *rb_s,  type *sb_n,  type *rb_n,
			    type *sb_w,  type *rb_w,  type *sb_e,  type *rb_e,
			    cudaStream_t *cuStream,
			    double *time
					 );

__global__ void setEast_block3D_cuda_kernel(
					       type *sb_e ,type *f,
					       int *block_nxs, int *block_nxe,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void setWest_block3D_cuda_kernel(
					       type *sb_w ,type *f,
					       int *block_nxs, int *block_nxe,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void setNorth_block3D_cuda_kernel(
					       type *sb_n ,type *f,
					       int *block_nys, int *block_nye,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void setSouth_block3D_cuda_kernel(
					       type *sb_s, type *f,
					       int *block_nys, int *block_nye,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void setTop_block3D_cuda_kernel(
					       type *sb_t ,type *f,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
 __global__ void setBottom_block3D_cuda_kernel(
					       type *sb_b, type *f,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
 __global__ void setBottom_block3D_cuda_kernel(
					       type *sb_b, type *f,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void readBackWest_block3D_cuda_kernel(
					       type *rb_w, type *f,
					       int *block_nxs, int *block_nxe,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);

__global__ void readBackEast_block3D_cuda_kernel(
					       type *rb_e ,type *f,
					       int *block_nxs, int *block_nxe,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void readBackNorth_block3D_cuda_kernel(
					       type *rb_n ,type *f,
					       int *block_nys, int *block_nye,
 					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
 __global__ void readBackSouth_block3D_cuda_kernel(
					       type *rb_s, type *f,
					       int *block_nys, int *block_nye,
 					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void readBackTop_block3D_cuda_kernel(
					       type *rb_t, type *f,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void readBackBottom_block3D_cuda_kernel(
					       type *rb_b, type *f,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);

JUPITER_DECL
int sleev_comm_block3D_waitBottom_cuda(mpi_prm *prm);
JUPITER_DECL
int sleev_comm_block3D_waitTop_cuda(mpi_prm *prm);
JUPITER_DECL
int sleev_comm_block3D_waitSouth_cuda(mpi_prm *prm);
JUPITER_DECL
int sleev_comm_block3D_waitNorth_cuda(mpi_prm *prm);
JUPITER_DECL
int sleev_comm_block3D_waitWest_cuda(mpi_prm *prm);
JUPITER_DECL
int sleev_comm_block3D_waitEast_cuda(mpi_prm *prm);

JUPITER_DECL
int sleev_comm_block3D_icommBottom_cuda(
				  mpi_prm *prm,				  
				  type *sb_b,  type *rb_b
				  );
  
JUPITER_DECL
int sleev_comm_block3D_icommTop_cuda(
				  mpi_prm *prm,				  
				  type *sb_t,  type *rb_t
				  );

JUPITER_DECL
int sleev_comm_block3D_icommSouth_cuda(
				  mpi_prm *prm,
				  type *sb_s,  type *rb_s
				  );

JUPITER_DECL
int sleev_comm_block3D_icommNorth_cuda(
				  mpi_prm *prm,
				  type *sb_n,  type *rb_n
				  );


JUPITER_DECL
int sleev_comm_block3D_icommWest_cuda(
				  mpi_prm *prm,
				  type *sb_w,  type *rb_w
				  );

JUPITER_DECL
int sleev_comm_block3D_icommEast_cuda(
				  mpi_prm *prm,
				  type *sb_e,  type *rb_e
				  );



#endif
