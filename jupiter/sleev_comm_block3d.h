#ifndef HEADER_SLEEVCOMMBLOCK3D
#define HEADER_SLEEVCOMMBLOCK3D

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif
JUPITER_DECL
int sleev_comm_block3D(type *f,mpi_prm *prm,
		       int *block_nxs, int *block_nxe, 
		       int *block_nys, int *block_nye, 
		       int *block_nzs, int *block_nze	    
		       );
#ifdef __cplusplus
}
#endif

#endif

