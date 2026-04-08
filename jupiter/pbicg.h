#ifndef HEADER_PBICG
#define HEADER_PBICG
#include "common.h"
JUPITER_DECL
int solve_pre_mat_BiCG1(mpi_prm prm, type *A,type *Dinv,type *r, type *s,type a, type *x, type *y);
JUPITER_DECL
int solve_pre_mat_BiCG2(mpi_prm prm, type *A,type *Dinv,type *r, type *s,type a,type b,type *x,type *y);
JUPITER_DECL
int calc_res_dot_cp2(mpi_prm prm, type *A, type *x,type *b,type *r,type *r0,type *p,type *tmp);
JUPITER_DECL
int MatVec_dot_(mpi_prm prm,type* A,type* x,type* y,type* z,type* tmp);
JUPITER_DECL
int MatVec_dot_dcomm_(mpi_prm prm,type* A,type* x,type* y,type* z,type* tmp,type *rb_b,type *rb_t,type *rb_s,type *rb_n,type *rb_w,type *rb_e);
JUPITER_DECL
int axpy_out(mpi_prm prm,type a,type* x,type* y,type* z);
JUPITER_DECL
int axpy_dot2_out(mpi_prm prm,type a,type* x,type* y,type* z,type* z0,type* tmp,type* tmpn);
JUPITER_DECL
int axpy_2(mpi_prm prm,type a,type b,type* x,type* y,type* z);
JUPITER_DECL

JUPITER_DECL
int axpy_2_inout(mpi_prm prm,type a,type b,type* x,type* y,type* z);
JUPITER_DECL
int MatVec_dot2(mpi_prm prm,type* A,type* x,type* y,type* z,type* tmp,type* tmpn);
JUPITER_DECL
int MatVec_dot2_sface(mpi_prm prm,type* A,type* y,type* z,type* tmp,type* tmpn,
		     type *rb_b,type *rb_t,type *rb_s,type *rb_n,type *rb_w,type *rb_e);
JUPITER_DECL
int MatVec_dot2_dcomm(mpi_prm prm,type* A,type* x,type* y,type* z,type* tmp,type* tmpn,type *rb_b,type *rb_t,type *rb_s,type *rb_n,type *rb_w,type *rb_e);
JUPITER_DECL
int MatVec_dot_sface_(mpi_prm prm,type* A,type* x,type* y,type* tmp,
		     type *rb_b,type *rb_t,type *rb_s,type *rb_n,type *rb_w,type *rb_e);
#endif
