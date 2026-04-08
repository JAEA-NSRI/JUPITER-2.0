/*
 * LevelSetCuda.h
 *
 */

#ifndef LEVELSETCUDA_H_
#define LEVELSETCUDA_H_

#ifdef GPU
#include <cuda_runtime.h>
#endif
#include "struct.h"

#ifdef __cplusplus
extern "C" {
#endif

#define BLOCK_SIZE 8
#define blockDim_x BLOCK_SIZE
#define blockDim_y BLOCK_SIZE
#define blockDim_z BLOCK_SIZE

typedef struct {
  type *d_sbuff;
  type *d_rbuff;
  type *h_sbuff;
  type *h_rbuff;
  size_t buf_size;

}comm_buf_cuda;

#ifdef __cplusplus
}
#endif
#endif /* LEVELSETCUDA_H_ */

