
//==================================
//   Physical properties
//==================================


#ifndef PHYSICAL_PROPERTIES_H

#define PHYSICAL_PROPERTIES_H

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

//======================
//  Fe (pure iron)
//----------------------
JUPITER_DECL
type fe_dens(type t);
JUPITER_DECL
type fe_visc(type t);
JUPITER_DECL
type fe_st(type t);
JUPITER_DECL
type fe_tc(type t);
JUPITER_DECL
type fe_shc(type t);

//================================================
//  Diffusion coefficients
//------------------------------------------------
JUPITER_DECL
type diff_coef(int compo, type t);
JUPITER_DECL
type liq(int compo, type Y);

#ifdef __cplusplus
}
#endif

#endif
