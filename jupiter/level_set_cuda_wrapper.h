/*
 * Level_Set_cuda_wrapper.h
 */

#ifndef LEVEL_SET_CUDA_WRAPPER_H_
#define LEVEL_SET_CUDA_WRAPPER_H_

#include "common.h"
#include "struct.h"

//
#ifdef __cplusplus
extern "C" {
#endif
JUPITER_DECL
void init_GPUs(parameter *prm);
//void vof2ls_cuda(int init_flg, type *fs, type *ls, domain *cdo);
JUPITER_DECL
type Level_Set_cuda(int init_flg, int itr_max, type *ls, type *fs, parameter *prm);

#ifdef __cplusplus
  }
#endif

#endif /* LEVEL_SET_CUDA_WRAPPER_H_ */
