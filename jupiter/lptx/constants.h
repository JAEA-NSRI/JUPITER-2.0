#ifndef JUPITER_LPTX_CONSTANTS_H
#define JUPITER_LPTX_CONSTANTS_H

#include "defs.h"

static const LPTX_type LPTX_boltzmann_constant = LPTX_C(1.380649e-23);

/**
 * Gas constant in [J/kmol/K]
 */
static const LPTX_type LPTX_gas_constant = LPTX_C(8.31446261815324e+3);

/**
 * Gas constant in [J/mol/K]
 */
static const LPTX_type LPTX_gas_constant_mol = LPTX_C(8.31446261815324);

#endif
