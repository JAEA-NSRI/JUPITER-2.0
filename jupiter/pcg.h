#ifndef HEADER_PCG
#define HEADER_PCG

#include "common.h"

JUPITER_DECL
int zero_initialize(int m,type *x);
JUPITER_DECL
int set_max_threads(int nz,int *max_threads_);
JUPITER_DECL
int set_omp_threads(int max_threads);
JUPITER_DECL
int set_dim(mpi_prm prm,int* stm,int* nx,int* ny,int* nz,int* m,int* mx,int* my,int* mxy);
JUPITER_DECL
int solve_pre_mat2(mpi_prm prm, type *A,type *Dinv,type *r, type *s,type *q,type alpha,type *tmp,type *tmpn);
JUPITER_DECL
int solve_pre_mat2_local(mpi_prm prm, type *A,type *Dinv,type *r, type *s,type *q,type alpha,type *tmp,type *tmpn);
JUPITER_DECL
int solve_pre_mat2_local_fp16(mpi_prm prm, type *A,type *Dinv,type *r, type *s,type *q,type alpha,type *tmp,type *tmpn);

JUPITER_DECL
int solve_pre_subdividing_mat0(
			       mpi_prm prm, type *A,type *Dinv,type *r, type *s, type *z,
			       type *block_yfilterL,type *block_zfilterL,
			       type *block_yfilterU,type *block_zfilterU,
			       int  *block_ys,int *block_zs,
			       int *block_ye,int *block_ze
			       );
JUPITER_DECL
int solve_pre_subdividing_mat2_local(
				     mpi_prm prm, type *A,type *Dinv,type *r, type *s,type *q, type *z,
				     type alpha,type *tmp,type *tmpn,
				     type *block_yfilterL,type *block_zfilterL,
				     type *block_yfilterU,type *block_zfilterU,
				     int  *block_ys,int *block_zs,
				     int *block_ye,int *block_ze
				     );


JUPITER_DECL
int solve_pre_mat0(mpi_prm prm, type *A,type *Dinv,type *r, type *s);
JUPITER_DECL
int solve_pre_mat0_dot(mpi_prm prm, type *A,type *Dinv,type *r, type *s, type *tmp);
JUPITER_DECL
int solve_pre_mat0_nosleev( mpi_prm prm, type *A,type *Dinv,type *r, type *s);
JUPITER_DECL
int solve_pre_mat0_nosleev_dot(mpi_prm prm, type *A,type *Dinv,type *r, type *s, type *tmp);
JUPITER_DECL
int calc_norm2(mpi_prm prm,type *x, type *y,type *tmp,type *tmpn);
JUPITER_DECL
int calc_norm2_local(mpi_prm prm,type *x, type *y,type *tmp,type *tmpn);
JUPITER_DECL
int calc_res(mpi_prm prm, type *A, type *x,type *b,type *r);
JUPITER_DECL
int calc_sres(mpi_prm prm, type *A, type *x,type *b,type *r);
JUPITER_DECL
int initializ_sleev(mpi_prm prm,type* x);
JUPITER_DECL
int axpy_(mpi_prm prm,type a,type* x,type* y);
JUPITER_DECL
int axpy2(mpi_prm prm,type a,type b,type* x,type* y,type* z);
JUPITER_DECL
int syrk(mpi_prm prm,type* x,type* y,type* z,int s);
JUPITER_DECL
int MatVec_dot(mpi_prm prm,type* A,type* x,type* y,type* tmp);
JUPITER_DECL
int sMatVec_dot(mpi_prm prm,type* A,type* x,type* y,type* tmp);
JUPITER_DECL
int make_sMat(mpi_prm prm,type* A);
JUPITER_DECL
int MatVec_dot_local(mpi_prm prm,type* A,type* x,type* y,type* tmp);
JUPITER_DECL
int sMatVec_dot_local(mpi_prm prm,type* A,type* x,type* y,type* tmp);
JUPITER_DECL
int MatVec_dot_local_dcomm2(mpi_prm* prm,type* A,type* x,type* y,type* tmp);
JUPITER_DECL
int axpy(mpi_prm prm,type a,type* x,type* y);
JUPITER_DECL
int Halo2noHalo(mpi_prm prm,type* x,type* y);
JUPITER_DECL
int axpy_inp(mpi_prm prm,type a,type b,type* x,type* y,type* z);
JUPITER_DECL
int axpy2_inp(mpi_prm prm,
	      type a,type b,
	      type* x1,type* z1,type* z2
	      );
JUPITER_DECL
int gemv(mpi_prm prm,type* x,type* y,type* z,int s);
JUPITER_DECL
int gemv_inplace(mpi_prm prm,type* x,type* y,type* z,int s);
JUPITER_DECL
int gemv_norm(mpi_prm prm,type* x,type* y,type* z,type* tmp,int s);

JUPITER_DECL
int gemv_dot_local(mpi_prm prm,type* x,type* y,type* z,type* tmp,int s);
JUPITER_DECL
int gemm(mpi_prm prm,type* x,type* y,type* z,int s);
JUPITER_DECL
int gemm_syrk(mpi_prm prm,type* x,type* y,type* z,type* w,type* v,int s);
JUPITER_DECL
int gemm_dot(mpi_prm prm,type* x,type* y,type* z,int s);
JUPITER_DECL
int gemm_dot_local(mpi_prm prm,type* x,type* y,type* z,int s);
JUPITER_DECL
int gemm2_dot_local(mpi_prm prm,type* x,type* y,type* z,
		    type* w,type* x_w,
		    type* v,type* u,
		    int s);
JUPITER_DECL
void swap_ptr(type **x,type **y);
JUPITER_DECL
int gemm2_dot_local_3_(mpi_prm prm,
		      type* x,type* y,type* z,
		      type* w,type* x_w,
		      type* v,type* u,
		      type* t,type* o,type* x_t,
		       int s);
JUPITER_DECL
int gemm2_dot_local_3(mpi_prm prm,
		      type* x,type* y,type* z,
		      type* w,type* x_w,
		      type* v,type* u,
		      type* t,type* o,type* x_t,
		      int s);
JUPITER_DECL
int gemv_cp(mpi_prm prm,type* x,type* y,type* z,type* w,int s);
JUPITER_DECL
int Vec_inplace(mpi_prm prm,type a,type* x,type* y);
JUPITER_DECL
int Vec_inplace_dot2(mpi_prm prm,type a,type* x,type* y,type* tmp,type* tmp2);
JUPITER_DECL
int calc_norm(mpi_prm prm,type* x,type* tmp);
JUPITER_DECL
int calc_norm_local(mpi_prm prm,type* x,type* tmp);
JUPITER_DECL
int calc_Maxnorm(mpi_prm prm,type* x,type* tmp);
JUPITER_DECL
int calc_dot(mpi_prm prm,type* x,type* y,type* tmp);
JUPITER_DECL
int make_pre_idiagMat1(mpi_prm prm,type* A,type* D);
JUPITER_DECL
int make_pre_subdividing_idiagMat1(
				   mpi_prm prm,
				   type* A,type* D,
				   type *block_yfilterL,type *block_zfilterL,
				   type *block_yfilterU,type *block_zfilterU,
				   int *block_ys,int *block_zs,
				   int *block_ye,int *block_ze
				   );
JUPITER_DECL
int make_pre_step_idiagMat1(mpi_prm prm,type* A,type* D);
JUPITER_DECL
int MatVec_dot_dcomm(mpi_prm prm,type* A,type* x,type* y,type* tmp,
		     type *q_b,type *q_t,type *q_s,type *q_n,type *q_w,type *q_e
		     );
JUPITER_DECL
int MatVec_dcomm(mpi_prm prm,type* A,type* x,type* y,
		     type *q_b,type *q_t,type *q_s,type *q_n,type *q_w,type *q_e
		 );
JUPITER_DECL
int MatVec_dot_sface(mpi_prm prm,type* A,type* x,type* y,type* tmp,
		     type *q_b,type *q_t,type *q_s,type *q_n,type *q_w,type *q_e
		     );
JUPITER_DECL
int MatVec_sface(mpi_prm prm,type* A,type* x,type* y,
		     type *q_b,type *q_t,type *q_s,type *q_n,type *q_w,type *q_e
		 );
JUPITER_DECL
int xcpy(mpi_prm prm,type* x,type* y);
JUPITER_DECL
int x_view(mpi_prm prm,type* x);
JUPITER_DECL
int xy_view(mpi_prm prm,type* x,type* y);
JUPITER_DECL
int x_write(mpi_prm prm,type* x);
JUPITER_DECL
int MatVec(mpi_prm prm,type* A,type* x,type* y);
JUPITER_DECL
int sMatVec(mpi_prm prm,type* A,type* x,type* y);
JUPITER_DECL
int MatVec_cb(mpi_prm prm,type a,type b,
	      type* A,type* x,type* y,type* z,type* w,type* v);
JUPITER_DECL
int sMatVec_cb(mpi_prm prm,type a,type b,
	       type* A,type* x,type* y,type* z,type* w,type* v);
JUPITER_DECL
int MatVec_unl(mpi_prm prm,type* A,type* x,type* y,
	       int mpkxs, int mpkxe,
	       int mpkys, int mpkye,
	       int mpkzs, int mpkze);
JUPITER_DECL
int MatVec_unl_dcomm2(mpi_prm* prm,type* A,type* x,type* y,
		     int mpkxs, int mpkxe,
		     int mpkys, int mpkye,
		      int mpkzs, int mpkze);
JUPITER_DECL
int calc_dot_local(mpi_prm prm,type* x,type* y,type* tmp);

JUPITER_DECL
int calc_dot2_local(mpi_prm prm,type* x,type* y,type* z,type* tmp1,type* tmp2);
JUPITER_DECL
int update_cacg(
		mpi_prm prm,
 		type* x,type* x_1,type* x_2,
		type* q,type* q_1,type* q_2,
		type* z,type* z_1,type* z_2,
		type* u,type* y,type rho,type gamma);
JUPITER_DECL
int neumann_pre(mpi_prm prm,type* A,type Amax,
		type* r,type* s,int order);
JUPITER_DECL
int neumann_pre_MPK(mpi_prm prm,type* A,type Amax,
		    type* r,type* s,int order,
		    int mpkxs, int mpkxe,
		    int mpkys, int mpkye,
		    int mpkzs, int mpkze);
JUPITER_DECL
int solve_pre_mat_unl(mpi_prm prm, type *A,type *Dinv,type *r, type *s,
		      int mpkxs, int mpkxe,
		      int mpkys, int mpkye,
		      int mpkzs, int mpkze);
JUPITER_DECL
int make_pre_idiagMat1_unl(mpi_prm prm,type* A,type* D,
			   int mpkxs, int mpkxe,
			   int mpkys, int mpkye,
			   int mpkzs, int mpkze);
JUPITER_DECL
int pjacobi_sleev_unl(mpi_prm prm,type* A,type* r,type* s,
		      int mpkxs, int mpkxe,
		      int mpkys, int mpkye,
		      int mpkzs, int mpkze);
JUPITER_DECL
int initializ_sleev_MPK_unl(mpi_prm prm,type* x,
			    int mpkxs, int mpkxe,
			    int mpkys, int mpkye,
			    int mpkzs, int mpkze);
JUPITER_DECL
int initializ_sleev_MPK_tes(mpi_prm prm,type* x,int smax_);
JUPITER_DECL
int pjacobi_sleev(mpi_prm prm,type* A,type* r,type* s,int smax_);
JUPITER_DECL
int gemm_dot_1_local(mpi_prm prm,type* Q,type* V,type* R,int mm,int nn);
JUPITER_DECL
int gemm_dot_2(mpi_prm prm,type* Q,type* V,type* R,int mm,int nn);

#endif
