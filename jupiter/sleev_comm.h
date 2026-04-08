#ifndef HEADER_SLEEVCOMM
#define HEADER_SLEEVCOMM

#include "common.h"

JUPITER_DECL
int sleev_comm(type *f,mpi_prm *prm);
JUPITER_DECL
int sleev_comm_x(type *f,mpi_prm *prm);
JUPITER_DECL
int sleev_comm_yz(type *f,mpi_prm *prm);
JUPITER_DECL
int sleev_dcomm(type *f,type *A,
		type *q_b,type *q_t,type *q_s,type *q_n,type *q_w,type *q_e,
		mpi_prm *prm);
JUPITER_DECL
int sleev_comm_nopack(type *f,
		type *rb_b,type *rb_t,type *rb_s,type *rb_n,type *rb_w,type *rb_e,
		mpi_prm *prm);
JUPITER_DECL
int sleev_comm_MPK(type *f,mpi_prm *prm);

#endif
