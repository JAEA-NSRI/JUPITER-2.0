#ifndef HEADER_SLEEVCOMMCUDA
#define HEADER_SLEEVCOMMCUDA

#include "common.h"

__global__ void initializ_sleev_kernel_Z_fixed(int ny, int nx, int nz,
                                    int stm,
                                    int mx, int mxy,
				    type* __restrict__ vec);
__global__ void initializ_sleev_kernel_Y_fixed(int ny, int nx, int nz,
                                    int stm,
                                    int mx, int mxy,
                                    type* __restrict__ vec);
__global__ void initializ_sleev_kernel_X_fixed(int ny, int nx, int nz,
                                    int stm,
                                    int mx, int mxy,
                                    type* __restrict__ vec);

JUPITER_DECL
void initializ_sleev_cuda(type *vec,
			  int ny, int nx, int nz,
			  int stm,
			  int mx, int mxy);

__global__ void setTopAndBottom_kernel(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            type* __restrict__ sb_b, 
                            type* __restrict__ sb_t,
			    const type* const __restrict__ f);

__global__ void setNorthAndSouth_kernel(int nl, int ny, int nx, int nz,
			     int stm,
			     int mx, int mxy,
			     type* __restrict__ sb_s, 
			     type* __restrict__ sb_n,
			     const type* const __restrict__ f);

__global__ void setWestAndEast_kernel(int nl, int ny, int nx, int nz,
			   int stm,
			   int mx, int mxy,
			   type* __restrict__ sb_w, 
			   type* __restrict__ sb_e,
			   const type* const __restrict__ f);

// ReadBack Kernels
__global__ void readBackTopAndBottom_kernel(int nl, int ny, int nx, int nz,
				 int stm,
				 int mx, int mxy,
				 const type* const __restrict__ rb_b, 
				 const type* const __restrict__ rb_t,
				 type*  __restrict__ f);
__global__ void readBackNorthAndSouth_kernel(int nl, int ny, int nx, int nz,
				  int stm,
				  int mx, int mxy,
				  const type* const  __restrict__ rb_s, 
				  const type* const  __restrict__ rb_n,
				  type* __restrict__ f);
__global__ void readBackWestAndEast_kernel(int nl, int ny, int nx, int nz,
				int stm,
				int mx, int mxy,
				const type* const  __restrict__ rb_w, 
				const type* const  __restrict__ rb_e,
				type* __restrict__ f);
JUPITER_DECL
int sleev_comm_cuda(type *f, mpi_prm *prm, type *cuda_buf);


__global__ void setTopAndBottom_kernel_BF16(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            low_type* __restrict__ sb_b, 
                            low_type* __restrict__ sb_t,
					    const low_type* const __restrict__ f);

__global__ void setNorthAndSouth_kernel_BF16(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            low_type* __restrict__ sb_s, 
                            low_type* __restrict__ sb_n,
                            const low_type* const __restrict__ f);

__global__ void setWestAndEast_kernel_BF16(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            low_type* __restrict__ sb_w, 
                            low_type* __restrict__ sb_e,
                            const low_type* const __restrict__ f);

__global__ void readBackTopAndBottom_kernel_BF16(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            const low_type* const __restrict__ rb_b, 
                            const low_type* const __restrict__ rb_t,
                            low_type*  __restrict__ f);

__global__ void readBackNorthAndSouth_kernel_BF16(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            const low_type* const  __restrict__ rb_s, 
                            const low_type* const  __restrict__ rb_n,
						  low_type* __restrict__ f);

__global__ void readBackWestAndEast_kernel_BF16(int nl, int ny, int nx, int nz,
                            int stm,
                            int mx, int mxy,
                            const low_type* const  __restrict__ rb_w, 
                            const low_type* const  __restrict__ rb_e,
                             low_type* __restrict__ f);
JUPITER_DECL
int sleev_comm_cuda_BF16(low_type *f, mpi_prm *prm, low_type *cuda_buf);

#endif
