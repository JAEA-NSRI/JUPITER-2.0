/*
 * communication_cuda.h
 *
 */

#ifndef COMMUNICATION_CUDA_H_
#define COMMUNICATION_CUDA_H_

#include "common.h"
#include "struct.h"
#include "LevelSetCuda.h"


#ifdef __cplusplus
extern "C" {
#endif
//void communication_cuda(type *f, parameter *prm);
JUPITER_DECL
void communication_cuda_buff(type *f, parameter *prm,
                             comm_buf_cuda *comm_buf_cu);

JUPITER_DECL
void pack_mpi_x_cuda(type *d_f, type *d_sbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, int *nrk);
JUPITER_DECL
void unpack_mpi_x_cuda(type *d_f, type *d_rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, int *nrk);
JUPITER_DECL
void pack_mpi_y_cuda(type *d_f, type *d_sbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, int *nrk);
JUPITER_DECL
void unpack_mpi_y_cuda(type *d_f, type *d_rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, int *nrk);
JUPITER_DECL
void pack_mpi_z_cuda(type *d_f, type *d_sbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, int *nrk);
JUPITER_DECL
void unpack_mpi_z_cuda(type *f, type *rbuff, int *ptr, int nx, int ny, int nz, int mx, int my, int mz, int stcl, int *nrk);
#ifdef __cplusplus
  }
#endif
#endif /* COMMUNICATION_CUDA_H_ */
