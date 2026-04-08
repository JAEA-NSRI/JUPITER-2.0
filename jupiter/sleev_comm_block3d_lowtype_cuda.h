#ifndef HEADER_SLEEVCOMMBLOCK3DLOWTYPECUDA
#define HEADER_SLEEVCOMMBLOCK3DLOWTYPECUDA
#include "common.h"

__global__ void setEast_block3D_lowtype_cuda_kernel(
					       low_type *sb_e ,low_type *f,
					       int *block_nxs, int *block_nxe,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void setWest_block3D_lowtype_cuda_kernel(
					       low_type *sb_w ,low_type *f,
					       int *block_nxs, int *block_nxe,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void setNorth_block3D_lowtype_cuda_kernel(
					       low_type *sb_n ,low_type *f,
					       int *block_nys, int *block_nye,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void setSouth_block3D_lowtype_cuda_kernel(
					       low_type *sb_s, low_type *f,
					       int *block_nys, int *block_nye,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void setTop_block3D_lowtype_cuda_kernel(
					       low_type *sb_t ,low_type *f,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);

 __global__ void setBottom_block3D_lowtype_cuda_kernel(
					       low_type *sb_b, low_type *f,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void readBackWest_block3D_lowtype_cuda_kernel(
					       low_type *rb_w, low_type *f,
					       int *block_nxs, int *block_nxe,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void readBackEast_block3D_lowtype_cuda_kernel(
					       low_type *rb_e ,low_type *f,
					       int *block_nxs, int *block_nxe,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void readBackNorth_block3D_lowtype_cuda_kernel(
					       low_type *rb_n ,low_type *f,
					       int *block_nys, int *block_nye,
 					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);

 __global__ void readBackSouth_block3D_lowtype_cuda_kernel(
					       low_type *rb_s, low_type *f,
					       int *block_nys, int *block_nye,
 					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
__global__ void readBackTop_block3D_lowtype_cuda_kernel(
					       low_type *rb_t, low_type *f,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);

__global__ void readBackBottom_block3D_lowtype_cuda_kernel(
					       low_type *rb_b, low_type *f,
					       int *block_nzs, int *block_nze,
					       const int nxblock,const int nyblock,const int nzblock,
					       const int mxdivblock,const int mydivblock,const int mzdivblock,
					       const int nxdivblock,const int nydivblock,const int nzdivblock);
JUPITER_DECL
int sleev_comm_block3D_icommEast_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_e,  low_type *rb_e
				  );
JUPITER_DECL
int sleev_comm_block3D_icommWest_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_w,  low_type *rb_w
				  );
JUPITER_DECL
int sleev_comm_block3D_icommNorth_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_n,  low_type *rb_n
				  );
JUPITER_DECL
int sleev_comm_block3D_icommSouth_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_s,  low_type *rb_s
				  );
JUPITER_DECL
int sleev_comm_block3D_icommTop_lowtype_cuda(
				  mpi_prm *prm,				  
				  low_type *sb_t,  low_type *rb_t
				  );
JUPITER_DECL
int sleev_comm_block3D_icommBottom_lowtype_cuda(
				  mpi_prm *prm,				  
				  low_type *sb_b,  low_type *rb_b
				  );
JUPITER_DECL
int sleev_comm_hostmem_block3D_lowtype_cuda(
			    low_type *f,mpi_prm *prm,
			    int *block_nxs, int *block_nxe, 
			    int *block_nys, int *block_nye, 
			    int *block_nzs, int *block_nze,
			    low_type *cuda_buf,
			    low_type *sb_b,  low_type *rb_b,  low_type *sb_t,  low_type *rb_t,
			    low_type *sb_s,  low_type *rb_s,  low_type *sb_n,  low_type *rb_n,
			    low_type *sb_w,  low_type *rb_w,  low_type *sb_e,  low_type *rb_e,
			    cudaStream_t *cuStream
		       );


JUPITER_DECL
int sleev_comm_block3D_isendEast_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_e,  low_type *rb_e
				  );
JUPITER_DECL
int sleev_comm_block3D_isendWest_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_w,  low_type *rb_w
				  );
JUPITER_DECL
int sleev_comm_block3D_isendNorth_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_n,  low_type *rb_n
				  );
JUPITER_DECL
int sleev_comm_block3D_isendSouth_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_s,  low_type *rb_s
				  );
JUPITER_DECL
int sleev_comm_block3D_isendTop_lowtype_cuda(
				  mpi_prm *prm,				  
				  low_type *sb_t,  low_type *rb_t
				  );
JUPITER_DECL
int sleev_comm_block3D_isendBottom_lowtype_cuda(
				  mpi_prm *prm,				  
				  low_type *sb_b,  low_type *rb_b
				  );


JUPITER_DECL
int sleev_comm_block3D_irecvEast_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_e,  low_type *rb_e
				  );
JUPITER_DECL
int sleev_comm_block3D_irecvWest_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_w,  low_type *rb_w
				  );
JUPITER_DECL
int sleev_comm_block3D_irecvNorth_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_n,  low_type *rb_n
				  );
JUPITER_DECL
int sleev_comm_block3D_irecvSouth_lowtype_cuda(
				  mpi_prm *prm,
				  low_type *sb_s,  low_type *rb_s
				  );
JUPITER_DECL
int sleev_comm_block3D_irecvTop_lowtype_cuda(
				  mpi_prm *prm,				  
				  low_type *sb_t,  low_type *rb_t
				  );
JUPITER_DECL
int sleev_comm_block3D_irecvBottom_lowtype_cuda(
				  mpi_prm *prm,				  
				  low_type *sb_b,  low_type *rb_b
				  );

#endif
