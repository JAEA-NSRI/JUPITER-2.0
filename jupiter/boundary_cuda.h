/*
 * boundary_cuda.h
 *
 */

#ifndef BOUNDARY_CUDA_H_
#define BOUNDARY_CUDA_H_

#include "struct.h"
#include "LevelSetCuda.h"


/*
 * d_ft temp
 */
#ifdef __cplusplus
extern "C" {
#endif

type bcf_cuda_buf(type *f, parameter *prm, comm_buf_cuda *comm_buf_cu);
void bcf_wall_cuda(type *d_f, domain *cdo, int *nrk);
#ifdef __cplusplus
  }
#endif

#endif /* BOUNDARY_CUDA_H_ */
