#ifndef JUPITER_CONSTANTS_H
#define JUPITER_CONSTANTS_H

/* Universal physical constants */

#include "struct.h"

#ifdef __cplusplus
extern "C"
{
#endif

/**
 * Gas constant in [J K-1 mol-1], [m3 Pa K-1 mol-1] or [kg m2 s-2 K-1 mol-1]
 */
static const type GAS_CONSTANT_R = 8.31446261815324;

/**
 * Gas constant in [J K-1 kmol-1], [m3 Pa K-1 kmol-1] or [kg m2 s-2 K-1 kmol-1]
 */
static const type GAS_CONSTANT_Rk = 8.31446261815324e+3;

#ifdef __cplusplus
}
#endif

#endif
