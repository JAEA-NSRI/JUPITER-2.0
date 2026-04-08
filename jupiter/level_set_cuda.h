/*
 * level_set_cuda.h
 *
 */

#ifndef LEVEL_SET_CUDA_H_
#define LEVEL_SET_CUDA_H_

#include "common.h"
#include "struct.h"
#include "func.h"
#include "LevelSetCuda.h"


#define FD3mp(fx2, fw2,fw1,fc,fe1,fe2, sig,dxs)         \
{   type  fxp, fxm;                                     \
    fxp = -(fe2 - 6.*fe1 + 3.*fc + 2.*fw1)*dxs/6.;      \
    fxm =  (fw2 - 6.*fw1 + 3.*fc + 2.*fe1)*dxs/6.;      \
	                                                      \
    if( sig > 0.0 ) {                                   \
      fx2  = MAX2( MAX2(fxm, 0.0)*MAX2(fxm, 0.0),     \
	                   MIN2(fxp, 0.0)*MIN2(fxp, 0.0) );   \
    } else {                                            \
      fx2  = MAX2( MIN2(fxm, 0.0)*MIN2(fxm, 0.0),     \
	                   MAX2(fxp, 0.0)*MAX2(fxp, 0.0) );   \
    }                                                   \
}



#define LS_ADV_mp FD3mp

JUPITER_DECL
void vof2ls_cuda(int init_flg, type *fs, type *ls, domain *cdo);
JUPITER_DECL
void level_set_eq_cuda(type *fn, type a0, type *f0, type af, type aft, type *f,
                       domain *cdo);

#endif /* LEVEL_SET_CUDA_H_ */
