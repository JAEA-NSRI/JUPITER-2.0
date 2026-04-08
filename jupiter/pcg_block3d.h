#ifndef HEADER_PCGBLOCK3D
#define HEADER_PCGBLOCK3D

#include "common.h"

#ifdef __cplusplus
extern "C" {
#endif

JUPITER_DECL
int XYZ_to_blockXYZ(
		    mpi_prm prm,type *f_xyz,type *f_bxyz,
		    int *block_nxs, int *block_nxe, 
		    int *block_nys, int *block_nye, 
		    int *block_nzs, int *block_nze	    
		    );
JUPITER_DECL
int blockXYZ_to_XYZ(
		    mpi_prm prm,type *f_xyz,type *f_bxyz,
		    int *block_nxs, int *block_nxe, 
		    int *block_nys, int *block_nye, 
		    int *block_nzs, int *block_nze 
		    );
  
JUPITER_DECL
int axpy_block3D(mpi_prm prm,type *x,type *y,type alpha,type beta);
JUPITER_DECL
int axpy2_block3D(mpi_prm prm,type alpha,type beta,type *x,type *y,type *z);

JUPITER_DECL
int calc_dot_local_nxnynz_block3D(
		    mpi_prm prm,type *x,type *y,type *ret_type,
		    int *block_nxs, int *block_nxe, 
		    int *block_nys, int *block_nye, 
		    int *block_nzs, int *block_nze	    
				  );
JUPITER_DECL
 int calc_dot_nxnynz_block3D(
			     mpi_prm prm,type *x,type *y,type *ret_type,
			     int *block_nxs, int *block_nxe, 
			     int *block_nys, int *block_nye, 
			     int *block_nzs, int *block_nze	    
			     );
JUPITER_DECL
 int calc_norm_nxnynz_block3D(mpi_prm prm,type *x,type *y,type *ret_type,
			     int *block_nxs, int *block_nxe, 
			     int *block_nys, int *block_nye, 
			     int *block_nzs, int *block_nze	    
			      );

JUPITER_DECL
int calc_dot_block3D_local(mpi_prm prm,type *x,type *y,type *ret_type);

JUPITER_DECL
int calc_dot_block3D(mpi_prm prm,type *x,type *y,type *ret_type);
JUPITER_DECL
int calc_norm_block3D(mpi_prm prm,type *x,type *y,type *ret_type);
JUPITER_DECL
int MatVec_dot_local_nxnynz_block3D(
		       mpi_prm prm,type* A,type* x,type* y,type* ret_type,
		       int *block_nxs, int *block_nxe, 
		       int *block_nys, int *block_nye, 
		       int *block_nzs, int *block_nze,
		       int *stride_xp,int *stride_xm,
		       int *stride_yp,int *stride_ym,
		       int *stride_zp,int *stride_zm 
				     );

JUPITER_DECL
int MatVec_dot_local_block3D(
		       mpi_prm prm,type* A,type* x,type* y,type* ret_type,
		       int *stride_xp,int *stride_xm,
		       int *stride_yp,int *stride_ym,
		       int *stride_zp,int *stride_zm 
		       );
JUPITER_DECL
int sMatVec_dot_local_block3D(
		       mpi_prm prm,type* A,type* x,type* y,type* ret_type,
		       int *stride_xp,int *stride_xm,
		       int *stride_yp,int *stride_ym,
		       int *stride_zp,int *stride_zm 
		       );
JUPITER_DECL
int calc_res_block3D(
		       mpi_prm prm,type* A,type* x,type* b,type* r,
		       int *stride_xp,int *stride_xm,
		       int *stride_yp,int *stride_ym,
		       int *stride_zp,int *stride_zm 
		       );
JUPITER_DECL
int calc_res_nxnynz_block3D(
		       mpi_prm prm,type* A,type* x,type* b,type* r,
		       int *block_nxs, int *block_nxe, 
		       int *block_nys, int *block_nye, 
		       int *block_nzs, int *block_nze,
		       int *stride_xp,int *stride_xm,
		       int *stride_yp,int *stride_ym,
		       int *stride_zp,int *stride_zm 
			    );

JUPITER_DECL
int calc_sres_block3D(
		       mpi_prm prm,type* A,type* x,type* b,type* r,
		       int *stride_xp,int *stride_xm,
		       int *stride_yp,int *stride_ym,
		       int *stride_zp,int *stride_zm 
		       );
JUPITER_DECL
int make_Matvec_ovl_flter(
			  mpi_prm prm,
			  int *block_nxs, int *block_nxe, 
			  int *block_nys, int *block_nye, 
			  int *block_nzs, int *block_nze,			  
			  type *block_West_filter   , type *block_East_filter,
			  type *block_South_filter  , type *block_North_filter,
			  type *block_Bottom_filter , type *block_Top_filter
			  );

JUPITER_DECL
int make_pre_subdividing_idiagMat1_block3D(
				   mpi_prm prm,
				   type* A,type* D,
				   int *block_nxs, int *block_nxe, 
				   int *block_nys, int *block_nye, 
				   int *block_nzs, int *block_nze,
				   int *stride_xp, int *stride_xm,
				   int *stride_yp, int *stride_ym,
				   int *stride_zp, int *stride_zm,
				   type *block_xfilterL,type *block_xfilterU,
				   type *block_yfilterL,type *block_yfilterU,
				   type *block_zfilterL,type *block_zfilterU
				   );
JUPITER_DECL
int solve_pre_subdividing_mat2_local_block3D(
				   mpi_prm prm,type* A,type* Dinv,
				   type *r, type *s,type *q,
				   type *z,				   
				   type alpha,type *sr_dot_local,type *rr_dot_local,
				   int *block_nxs, int *block_nxe, 
				   int *block_nys, int *block_nye, 
				   int *block_nzs, int *block_nze,
				   int *stride_xp, int *stride_xm,
				   int *stride_yp, int *stride_ym,
				   int *stride_zp, int *stride_zm,
				   type *block_xfilterL,type *block_xfilterU,
				   type *block_yfilterL,type *block_yfilterU,
				   type *block_zfilterL,type *block_zfilterU
					     );
JUPITER_DECL
int solve_pre_subdividing_mat2_local_nxnynz_block3D(
				   mpi_prm prm,type* A,type* Dinv,
				   type *r, type *s,type *q,
				   type *z,				   
				   type alpha,type *sr_dot_local,type *rr_dot_local,
				   int *block_nxs, int *block_nxe, 
				   int *block_nys, int *block_nye, 
				   int *block_nzs, int *block_nze,
				   int *stride_xp, int *stride_xm,
				   int *stride_yp, int *stride_ym,
				   int *stride_zp, int *stride_zm,
				   type *block_xfilterL,type *block_xfilterU,
				   type *block_yfilterL,type *block_yfilterU,
				   type *block_zfilterL,type *block_zfilterU
						    );
JUPITER_DECL
int solve_pre_subdividing_mat0_local_nxnynz_block3D(
				   mpi_prm prm,type* A,type* Dinv,
				   type *r, type *s,
				   int *block_nxs, int *block_nxe, 
				   int *block_nys, int *block_nye, 
				   int *block_nzs, int *block_nze,
				   int *stride_xp, int *stride_xm,
				   int *stride_yp, int *stride_ym,
				   int *stride_zp, int *stride_zm,
				   type *block_xfilterL,type *block_xfilterU,
				   type *block_yfilterL,type *block_yfilterU,
				   type *block_zfilterL,type *block_zfilterU
						    );


JUPITER_DECL
int solve_pre_subdividing_mat0_local_block3D(
				   mpi_prm prm,type* A,type* Dinv,
				   type *r, type *s,
				   int *block_nxs, int *block_nxe, 
				   int *block_nys, int *block_nye, 
				   int *block_nzs, int *block_nze,
				   int *stride_xp, int *stride_xm,
				   int *stride_yp, int *stride_ym,
				   int *stride_zp, int *stride_zm,
				   type *block_xfilterL,type *block_xfilterU,
				   type *block_yfilterL,type *block_yfilterU,
				   type *block_zfilterL,type *block_zfilterU
					     );

JUPITER_DECL
 int make_sMat_block3D(
	       mpi_prm prm,type* A,
	       int *stride_xp,int *stride_xm,
	       int *stride_yp,int *stride_ym,
	       int *stride_zp,int *stride_zm 
		       );

#ifdef __cplusplus
}
#endif

#endif
