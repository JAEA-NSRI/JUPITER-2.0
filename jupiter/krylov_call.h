#ifndef HEADER_KRYLOV
#define HEADER_KRYLOV
#include "common.h"

JUPITER_DECL
int pcg_call(mpi_prm prm,
	     int itrmax,
	     type rtolmax,type abstolmax,
	     type* x,type* b,type* A);

JUPITER_DECL
int pbicg_call(mpi_prm prm,
	       int itrmax,
	       type rtolmax,type abstolmax,
	       type* x,type* b,type* A);

#endif
