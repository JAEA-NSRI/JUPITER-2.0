#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "struct.h"
#include "func.h"
#include "cg.h"
#include "pcg_block3d.h"
#include "sleev_comm_block3d.h"
#include "os/os.h"

int pcg_block3d_call(mpi_prm prm,
		     int itrmax,
		     type rtolmax,type abstolmax,
		     type* x_xyz,type* b_xyz,type* A_xyz
		     ){
#if 0
  pcg_call( prm,
	    10,
	    1.0e-50,1.0e-50,
	    x_xyz, b_xyz, A_xyz
	    );
#endif
 
  int rank;
#ifdef JUPITER_MPI
  MPI_Comm_rank(prm.comm, &rank);
#else
  rank=0;
#endif
  int i,itr;
  double ts,ts0,te;
  double time[20];
  int m = prm.m;
  int mx =prm.mx;
  int my =prm.my;
  int mz =prm.mz;
  int stm  = prm.stm;
  int nx = prm.nx;
  int ny = prm.ny;
  int nz = prm.nz;

  // --------------

  set_subdividing_3Dblockjacobi(&prm);
  int nxdivblock = prm.nxdivblock;
  int nydivblock = prm.nydivblock;
  int nzdivblock = prm.nzdivblock;
  int block_nxs[nxdivblock];
  int block_nxe[nxdivblock];
  int block_nys[nydivblock];
  int block_nye[nydivblock];
  int block_nzs[nzdivblock];
  int block_nze[nzdivblock];

  int nzblock = prm.nzblock;
  int nyblock = prm.nyblock;
  int nxblock = prm.nxblock;

  int stride_zp[nzblock];
  int stride_zm[nzblock];
  int stride_yp[nyblock];
  int stride_ym[nyblock];
  int stride_xp[nxblock];
  int stride_xm[nxblock];

  set_subdividing_3Dblock_list(
			       prm,
			       block_nxs,block_nxe,
			       block_nys,block_nye,
			       block_nzs,block_nze,
			       stride_xp,stride_xm,
			       stride_yp,stride_ym,
			       stride_zp,stride_zm			       
			       );
  int block_m = prm.block_m;

  if(rank ==0 ){
    //    printf("m,block_m = %d,%d \n",m,block_m);;
    //    printf(" block_mx,block_my,block_mz = %08d, %08d, %08d \n",prm.block_mx,prm.block_my,prm.block_mz);;
  }
  type *A    = (type*)malloc(sizeof(type)*(block_m*7));
  type *x    = (type*)malloc(sizeof(type)*(block_m));
  type *b    = (type*)malloc(sizeof(type)*(block_m));

  type *r    = (type*)malloc(sizeof(type)*(block_m));
  type *Dinv = (type*)malloc(sizeof(type)*(block_m));
  type *r_   = (type*)malloc(sizeof(type)*(block_m));
  type *s    = (type*)malloc(sizeof(type)*(block_m));
  type *p    = (type*)malloc(sizeof(type)*(block_m));
  type *q    = (type*)malloc(sizeof(type)*(block_m));
  type *z    = (type*)malloc(sizeof(type)*(block_m));

  zero_initialize(block_m*7,A);
  zero_initialize(block_m,x);
  zero_initialize(block_m,b);

  zero_initialize(block_m,Dinv);
  zero_initialize(block_m,r);
  zero_initialize(block_m,r_);
  zero_initialize(block_m,s);
  zero_initialize(block_m,p);
  zero_initialize(block_m,q);
  zero_initialize(block_m,z);

#if sMatrix
  make_sMat(prm,A);
#endif
  int j;
  XYZ_to_blockXYZ(
		  prm,
		  x_xyz, x,
		  block_nxs,  block_nxe, 
		  block_nys,  block_nye, 
		  block_nzs,  block_nze);
  XYZ_to_blockXYZ(
		  prm,
		  b_xyz, b,
		  block_nxs,  block_nxe, 
		  block_nys,  block_nye, 
		  block_nzs,  block_nze);

  for(j=0;j<7;j++){
    XYZ_to_blockXYZ(
		    prm,
		    &A_xyz[m*j], &A[block_m*j],
		    block_nxs,  block_nxe, 
		    block_nys,  block_nye, 
		    block_nzs,  block_nze);

#if 0
    type Anorm = 0.0;
    calc_norm_block3D(prm,&A[block_m*j],&A[block_m*j],&Anorm);
    printf(" A matrix %d : %f \n",j,Anorm);
#endif

  }

#if sMatrix
  make_sMat_block3D(prm,A,
		    stride_xp,stride_xm,
		    stride_yp,stride_ym,
		    stride_zp,stride_zm 
		    );
#endif

  type *block_zfilterL = (type*)malloc(sizeof(type)*(nzblock*nzdivblock));
  type *block_zfilterU = (type*)malloc(sizeof(type)*(nzblock*nzdivblock));
  zero_initialize(nzblock*nzdivblock,block_zfilterL);
  zero_initialize(nzblock*nzdivblock,block_zfilterU);
  type *block_yfilterL = (type*)malloc(sizeof(type)*(nyblock*nydivblock));
  type *block_yfilterU = (type*)malloc(sizeof(type)*(nyblock*nydivblock));
  zero_initialize(nyblock*nydivblock,block_yfilterL);
  zero_initialize(nyblock*nydivblock,block_yfilterU);
  type *block_xfilterL = (type*)malloc(sizeof(type)*(nxblock*nxdivblock));
  type *block_xfilterU = (type*)malloc(sizeof(type)*(nxblock*nxdivblock));
  zero_initialize(nxblock*nxdivblock,block_xfilterL);
  zero_initialize(nxblock*nxdivblock,block_xfilterU);

  make_pre_subdividing_idiagMat1_block3D(
					 prm,
					 A,Dinv,
					 block_nxs, block_nxe, 
					 block_nys, block_nye, 
					 block_nzs, block_nze,
					 stride_xp, stride_xm,
					 stride_yp, stride_ym,
					 stride_zp, stride_zm,
					 block_xfilterL,block_xfilterU,
					 block_yfilterL,block_yfilterU,
					 block_zfilterL,block_zfilterU
					 );

  // -------------------

  type rhon[2];
  rhon[0]=(double)0.0;
  rhon[1]=(double)0.0;
  type tmp;
  type abstol,rtol,abstol0;
  type alpha,beta,gamma,rmax,bmax;

  calc_norm_block3D(prm,b,b,&bmax);

#ifdef JUPITER_MPI
  //  zero_initialize(block_m,x);
  sleev_comm_block3D(x,&prm,
		     block_nxs,  block_nxe, 
		     block_nys,  block_nye, 
		     block_nzs,  block_nze);
#endif

#if sMatrix
  calc_sres_block3D(
#else
  calc_res_block3D(
#endif
		   prm, A, x, b, r,
		   stride_xp,stride_xm,
		   stride_yp,stride_ym,
		   stride_zp,stride_zm 
		   );

  alpha=(double)0.0;
  beta=(double)0.0;
  calc_norm_block3D(prm,r,r,&abstol);

  solve_pre_subdividing_mat0_local_block3D(
					   prm, A, Dinv,
					   r, s,
					   block_nxs, block_nxe, 
					   block_nys, block_nye, 
					   block_nzs, block_nze,
					   stride_xp, stride_xm,
					   stride_yp, stride_ym,
					   stride_zp, stride_zm,
					   block_xfilterL,block_xfilterU,
					   block_yfilterL,block_yfilterU,
					   block_zfilterL,block_zfilterU
					   );
  
  calc_dot_block3D(prm,r,s,&rhon[0]);
  rtol=abstol/bmax;

#ifdef RES_history
  if(rank ==0 ){
    printf("Iteration ,Absolute res ,Relative res \n");
    printf("@res@  %4d, %16.14e %16.14e \n",0,abstol,rtol);
  }
#endif

  // Overlapのために計算領域をcore+z,x,yに分ける
  // block_m <--> x
  // block_m <--> y
  // データ変換の関数が必要
  // 袖交換のコードが必要.
  // -------
  
  for(itr=0;itr<itrmax;itr++){

#if 0
    printf("      alpha,beta = %f, %f \n",alpha,beta);
    printf("      rho0, rho1 = %f, %f \n",rhon[0],rhon[1]);
    printf("      gamma      = %f \n",gamma);
#endif

    //    return 0;
    ts=cpu_time();
    //    axpy2(prm,alpha,beta,s,p,x);
    axpy2_block3D(prm,alpha,beta,s,p,x);

    te=cpu_time();
    time[0]=time[0]+te-ts;

    // -----------------

    ts=cpu_time();
#ifdef JUPITER_MPI
    sleev_comm_block3D(p,&prm,
		       block_nxs,  block_nxe, 
		       block_nys,  block_nye, 
		       block_nzs,  block_nze);
#endif
    te=cpu_time();
    time[1]=time[1]+te-ts;

    ts0=cpu_time();
#if sMatrix
    sMatVec_dot_local_block3D(
#else
    MatVec_dot_local_block3D(
#endif
			     // #endif
			     prm, A, p, q,&gamma,
			     stride_xp,stride_xm,
			     stride_yp,stride_ym,
			     stride_zp,stride_zm 
			     );
    ts=cpu_time();
    tmp=gamma;
#ifdef JUPITER_MPI
    MPI_Allreduce(&tmp, &gamma, 1,MPI_TYPE, MPI_SUM,prm.comm);
#else
    gamma=tmp;
#endif

    te=cpu_time();
    time[9]=time[9]+te-ts;

    te=cpu_time();
    time[2]=time[2]+te-ts0;

    // -----------------
    // -----------------

    ts0=cpu_time();
    alpha=-rhon[0]/gamma;

    solve_pre_subdividing_mat2_local_block3D(
					     prm, A, Dinv,
					     r, s,q,
					     z,				   
					     alpha,&rhon[1],&abstol,
					     block_nxs, block_nxe, 
					     block_nys, block_nye, 
					     block_nzs, block_nze,
					     stride_xp, stride_xm,
					     stride_yp, stride_ym,
					     stride_zp, stride_zm,
					     block_xfilterL,block_xfilterU,
					     block_yfilterL,block_yfilterU,
					     block_zfilterL,block_zfilterU
					     );

    ts=cpu_time(); 
    type s_reduce[2];
    type r_reduce[2];
    s_reduce[0]=rhon[1];
    s_reduce[1]=abstol;
    r_reduce[0]=(double)0.0;
    r_reduce[1]=(double)0.0;
#ifdef JUPITER_MPI
    MPI_Allreduce(s_reduce, r_reduce, 2,MPI_TYPE, MPI_SUM,prm.comm); 
#else
    r_reduce[0]=s_reduce[0];
    r_reduce[1]=s_reduce[1];
#endif
    rhon[1]=r_reduce[0];        
    abstol=sqrt(fabs(r_reduce[1])); 

    te=cpu_time();
    time[8]=time[8]+te-ts;

    //--------
    te=cpu_time();
    time[4]=time[4]+te-ts0;

    alpha=-alpha;
    beta=rhon[1]/rhon[0];
    rhon[0]=rhon[1];
    rtol=abstol/bmax;

#ifdef RES_history
    if(rank ==0 ){
      printf("@res@  %4d, %16.14e %16.14e \n",itr+1,abstol,rtol);
    }
#endif

    if( rtol < rtolmax){
      break;
    }
    if( abstol < abstolmax ){
      break;
    }  
  }
  //  axpy(prm,alpha,p,x);
  // x = x + alpha*p
  axpy_block3D(prm,p,x,alpha,1.0);

#if 1
  blockXYZ_to_XYZ(
		  prm,
		  x_xyz, x,
		  block_nxs,  block_nxe, 
		  block_nys,  block_nye, 
		  block_nzs,  block_nze);
#endif

  // --------

  free(A);
  free(x);
  free(b);
  
  free(Dinv);
  free(r);
  free(r_);
  free(s);
  free(p);
  free(q);
  free(z);

  free(block_zfilterL);
  free(block_zfilterU);
  free(block_yfilterL);
  free(block_yfilterU);
  free(block_xfilterL);
  free(block_xfilterU);

#if 1
  if(rank ==0 ){
    printf(" \n");
  }
  pcg_call( prm,
	    10,
	    1.0e-50,1.0e-50,
	    //	    itrmax,
	    //	     rtolmax, abstolmax,
	    x_xyz, b_xyz, A_xyz
	    );
#endif

  return itr ;

}

#if 1
int pcg_block3d_common_call(mpi_prm prm,
		     int itrmax,
		     type rtolmax,type abstolmax,
		     type* x_xyz,type* b_xyz,type* A_xyz
		     ){
#if 0
  pcg_call( prm,
	    10,
	    1.0e-50,1.0e-50,
	    x_xyz, b_xyz, A_xyz
	    );
#endif
 
  int rank;
#ifdef JUPITER_MPI
  MPI_Comm_rank(prm.comm, &rank);
#else
  rank=0;
#endif
  int i,itr;
  double ts,ts0,te;
  double time[20];
  int m = prm.m;
  int mx =prm.mx;
  int my =prm.my;
  int mz =prm.mz;
  int stm  = prm.stm;
  int nx = prm.nx;
  int ny = prm.ny;
  int nz = prm.nz;

  // --------------

  set_subdividing_3Dblockjacobi(&prm);
  int nxdivblock = prm.nxdivblock;
  int nydivblock = prm.nydivblock;
  int nzdivblock = prm.nzdivblock;
  int block_nxs[nxdivblock];
  int block_nxe[nxdivblock];
  int block_nys[nydivblock];
  int block_nye[nydivblock];
  int block_nzs[nzdivblock];
  int block_nze[nzdivblock];

  int nzblock = prm.nzblock;
  int nyblock = prm.nyblock;
  int nxblock = prm.nxblock;

  int stride_zp[nzblock];
  int stride_zm[nzblock];
  int stride_yp[nyblock];
  int stride_ym[nyblock];
  int stride_xp[nxblock];
  int stride_xm[nxblock];

  set_subdividing_3Dblock_list(
			       prm,
			       block_nxs,block_nxe,
			       block_nys,block_nye,
			       block_nzs,block_nze,
			       stride_xp,stride_xm,
			       stride_yp,stride_ym,
			       stride_zp,stride_zm			       
			       );
  int block_m = prm.block_m;

  if(rank ==0 ){
    //    printf("m,block_m = %d,%d \n",m,block_m);;
    //    printf(" block_mx,block_my,block_mz = %08d, %08d, %08d \n",prm.block_mx,prm.block_my,prm.block_mz);;
  }
  type *A    = (type*)malloc(sizeof(type)*(block_m*7));
  type *x    = (type*)malloc(sizeof(type)*(block_m));
  type *b    = (type*)malloc(sizeof(type)*(block_m));

  type *r    = (type*)malloc(sizeof(type)*(block_m));
  type *Dinv = (type*)malloc(sizeof(type)*(block_m));
  type *r_   = (type*)malloc(sizeof(type)*(block_m));
  type *s    = (type*)malloc(sizeof(type)*(block_m));
  type *p    = (type*)malloc(sizeof(type)*(block_m));
  type *q    = (type*)malloc(sizeof(type)*(block_m));
  type *z    = (type*)malloc(sizeof(type)*(block_m));

  zero_initialize(block_m*7,A);
  zero_initialize(block_m,x);
  zero_initialize(block_m,b);

  zero_initialize(block_m,Dinv);
  zero_initialize(block_m,r);
  zero_initialize(block_m,r_);
  zero_initialize(block_m,s);
  zero_initialize(block_m,p);
  zero_initialize(block_m,q);
  zero_initialize(block_m,z);

#if sMatrix
  make_sMat(prm,A);
#endif
  int j;
  XYZ_to_blockXYZ(
		  prm,
		  x_xyz, x,
		  block_nxs,  block_nxe, 
		  block_nys,  block_nye, 
		  block_nzs,  block_nze);
  XYZ_to_blockXYZ(
		  prm,
		  b_xyz, b,
		  block_nxs,  block_nxe, 
		  block_nys,  block_nye, 
		  block_nzs,  block_nze);

  for(j=0;j<7;j++){
    XYZ_to_blockXYZ(
		    prm,
		    &A_xyz[m*j], &A[block_m*j],
		    block_nxs,  block_nxe, 
		    block_nys,  block_nye, 
		    block_nzs,  block_nze);

  }

#if sMatrix
  make_sMat_block3D(prm,A,
		    stride_xp,stride_xm,
		    stride_yp,stride_ym,
		    stride_zp,stride_zm 
		    );
#endif

  type *block_zfilterL = (type*)malloc(sizeof(type)*(nzblock*nzdivblock));
  type *block_zfilterU = (type*)malloc(sizeof(type)*(nzblock*nzdivblock));
  zero_initialize(nzblock*nzdivblock,block_zfilterL);
  zero_initialize(nzblock*nzdivblock,block_zfilterU);
  type *block_yfilterL = (type*)malloc(sizeof(type)*(nyblock*nydivblock));
  type *block_yfilterU = (type*)malloc(sizeof(type)*(nyblock*nydivblock));
  zero_initialize(nyblock*nydivblock,block_yfilterL);
  zero_initialize(nyblock*nydivblock,block_yfilterU);
  type *block_xfilterL = (type*)malloc(sizeof(type)*(nxblock*nxdivblock));
  type *block_xfilterU = (type*)malloc(sizeof(type)*(nxblock*nxdivblock));
  zero_initialize(nxblock*nxdivblock,block_xfilterL);
  zero_initialize(nxblock*nxdivblock,block_xfilterU);

  make_pre_subdividing_idiagMat1_block3D(
					 prm,
					 A,Dinv,
					 block_nxs, block_nxe, 
					 block_nys, block_nye, 
					 block_nzs, block_nze,
					 stride_xp, stride_xm,
					 stride_yp, stride_ym,
					 stride_zp, stride_zm,
					 block_xfilterL,block_xfilterU,
					 block_yfilterL,block_yfilterU,
					 block_zfilterL,block_zfilterU
					 );

  // -------------------

  type rhon[2];
  rhon[0]=(double)0.0;
  rhon[1]=(double)0.0;
  type tmp;
  type abstol,rtol,abstol0;
  type alpha,beta,gamma,rmax,bmax;

#if 1
  calc_norm_nxnynz_block3D(prm,b,b,&bmax,
			  block_nxs, block_nxe, 
			  block_nys, block_nye, 
			  block_nzs, block_nze	    
			  );
#else
  calc_norm_block3D(prm,b,b,&bmax);
#endif

#ifdef JUPITER_MPI
  //  zero_initialize(block_m,x);
  sleev_comm_block3D(x,&prm,
		     block_nxs,  block_nxe, 
		     block_nys,  block_nye, 
		     block_nzs,  block_nze);
#endif

#if 1 
  calc_res_nxnynz_block3D(
		   prm, A, x, b, r,
		   block_nxs, block_nxe, 
		   block_nys, block_nye, 
		   block_nzs, block_nze,
		   stride_xp,stride_xm,
		   stride_yp,stride_ym,
		   stride_zp,stride_zm 
		   );
#else
  calc_res_block3D(
		   prm, A, x, b, r,
		   stride_xp,stride_xm,
		   stride_yp,stride_ym,
		   stride_zp,stride_zm 
		   );
#endif
  alpha=(double)0.0;
  beta=(double)0.0;

#if 1
  calc_norm_nxnynz_block3D(prm,r,r,&abstol,
			  block_nxs, block_nxe, 
			  block_nys, block_nye, 
			  block_nzs, block_nze	    
			  );
  solve_pre_subdividing_mat0_local_nxnynz_block3D(
					   prm, A, Dinv,
					   r, s,
					   block_nxs, block_nxe, 
					   block_nys, block_nye, 
					   block_nzs, block_nze,
					   stride_xp, stride_xm,
					   stride_yp, stride_ym,
					   stride_zp, stride_zm,
					   block_xfilterL,block_xfilterU,
					   block_yfilterL,block_yfilterU,
					   block_zfilterL,block_zfilterU
					   );
  
  calc_dot_nxnynz_block3D(prm,r,s,&rhon[0],
			  block_nxs, block_nxe, 
			  block_nys, block_nye, 
			  block_nzs, block_nze	    
			  );

#else
  calc_norm_block3D(prm,r,r,&abstol);
  solve_pre_subdividing_mat0_local_block3D(
					   prm, A, Dinv,
					   r, s,
					   block_nxs, block_nxe, 
					   block_nys, block_nye, 
					   block_nzs, block_nze,
					   stride_xp, stride_xm,
					   stride_yp, stride_ym,
					   stride_zp, stride_zm,
					   block_xfilterL,block_xfilterU,
					   block_yfilterL,block_yfilterU,
					   block_zfilterL,block_zfilterU
					   );
  
  calc_dot_block3D(prm,r,s,&rhon[0]);
#endif

  rtol=abstol/bmax;

#ifdef RES_history
  if(rank ==0 ){
    printf("Iteration ,Absolute res ,Relative res \n");
    printf("@res@  %4d, %16.14e %16.14e \n",0,abstol,rtol);
  }
#endif

#if 0
  printf("      bmax = %f \n",bmax);
#endif

  for(itr=0;itr<itrmax;itr++){

#if 0
    printf("      alpha,beta = %f, %f \n",alpha,beta);
    printf("      rho0, rho1 = %f, %f \n",rhon[0],rhon[1]);
    printf("      gamma      = %f \n",gamma);
#endif

    //    return 0;
    ts=cpu_time();
    //    axpy2(prm,alpha,beta,s,p,x);
    axpy2_block3D(prm,alpha,beta,s,p,x);

    te=cpu_time();
    time[0]=time[0]+te-ts;

    // -----------------

    ts=cpu_time();
#ifdef JUPITER_MPI
    sleev_comm_block3D(p,&prm,
		       block_nxs,  block_nxe, 
		       block_nys,  block_nye, 
		       block_nzs,  block_nze);
#endif
    te=cpu_time();
    time[1]=time[1]+te-ts;

    ts0=cpu_time();
#if 1
    MatVec_dot_local_nxnynz_block3D(
			     prm, A, p, q,&gamma,
			     block_nxs, block_nxe, 
			     block_nys, block_nye, 
			     block_nzs, block_nze,
			     stride_xp,stride_xm,
			     stride_yp,stride_ym,
			     stride_zp,stride_zm 
			     );    
    calc_dot_nxnynz_block3D(prm,p,q,&gamma,
			    block_nxs, block_nxe, 
			    block_nys, block_nye, 
			    block_nzs, block_nze	    
			    );

#else

    MatVec_dot_local_block3D(
			     // #endif
			     prm, A, p, q,&gamma,
			     stride_xp,stride_xm,
			     stride_yp,stride_ym,
			     stride_zp,stride_zm 
			     );
    ts=cpu_time();
    tmp=gamma;
#ifdef JUPITER_MPI
    MPI_Allreduce(&tmp, &gamma, 1,MPI_TYPE, MPI_SUM,prm.comm);
#else
    gamma=tmp;
#endif

#endif

    te=cpu_time();
    time[9]=time[9]+te-ts;

    te=cpu_time();
    time[2]=time[2]+te-ts0;

    // -----------------
    // -----------------

    ts0=cpu_time();
    alpha=-rhon[0]/gamma;

#if 1
    solve_pre_subdividing_mat2_local_nxnynz_block3D(
					     prm, A, Dinv,
					     r, s,q,
					     z,				   
					     alpha,&rhon[1],&abstol,
					     block_nxs, block_nxe, 
					     block_nys, block_nye, 
					     block_nzs, block_nze,
					     stride_xp, stride_xm,
					     stride_yp, stride_ym,
					     stride_zp, stride_zm,
					     block_xfilterL,block_xfilterU,
					     block_yfilterL,block_yfilterU,
					     block_zfilterL,block_zfilterU
					     );
    calc_dot_nxnynz_block3D(prm,r,s,&rhon[1],
			    block_nxs, block_nxe, 
			    block_nys, block_nye, 
			    block_nzs, block_nze	    
			    );
    calc_norm_nxnynz_block3D(prm,r,r,&abstol,
			    block_nxs, block_nxe, 
			    block_nys, block_nye, 
			    block_nzs, block_nze	    
			     );


#else
    solve_pre_subdividing_mat2_local_block3D(
					     prm, A, Dinv,
					     r, s,q,
					     z,				   
					     alpha,&rhon[1],&abstol,
					     block_nxs, block_nxe, 
					     block_nys, block_nye, 
					     block_nzs, block_nze,
					     stride_xp, stride_xm,
					     stride_yp, stride_ym,
					     stride_zp, stride_zm,
					     block_xfilterL,block_xfilterU,
					     block_yfilterL,block_yfilterU,
					     block_zfilterL,block_zfilterU
					     );

    ts=cpu_time(); 
    type s_reduce[2];
    type r_reduce[2];
    s_reduce[0]=rhon[1];
    s_reduce[1]=abstol;
    r_reduce[0]=(double)0.0;
    r_reduce[1]=(double)0.0;
#ifdef JUPITER_MPI
    MPI_Allreduce(s_reduce, r_reduce, 2,MPI_TYPE, MPI_SUM,prm.comm); 
#else
    r_reduce[0]=s_reduce[0];
    r_reduce[1]=s_reduce[1];
#endif
    rhon[1]=r_reduce[0];        
    abstol=sqrt(fabs(r_reduce[1])); 
#endif

    te=cpu_time();
    time[8]=time[8]+te-ts;

    //--------
    te=cpu_time();
    time[4]=time[4]+te-ts0;

    alpha=-alpha;
    beta=rhon[1]/rhon[0];
    rhon[0]=rhon[1];
    rtol=abstol/bmax;

#ifdef RES_history
    if(rank ==0 ){
      printf("@res@  %4d, %16.14e %16.14e \n",itr+1,abstol,rtol);
    }
#endif

    if( rtol < rtolmax ){
      break;
    }
    if( abstol < abstolmax ){
      break;
    }  
  }
  //  axpy(prm,alpha,p,x);
  // x = x + alpha*p
  axpy_block3D(prm,p,x,alpha,1.0);

#if 1
  blockXYZ_to_XYZ(
		  prm,
		  x_xyz, x,
		  block_nxs,  block_nxe, 
		  block_nys,  block_nye, 
		  block_nzs,  block_nze);
#else
  printf(" @@@@@@@@@@@@@@@@@ \n");
  printf(" @@@@@@@@@@@@@@@@@ \n");
  printf(" @@@@@@@@@@@@@@@@@ \n");
#if 0
  pcg_block3d_call( prm,
		    itrmax,
		    rtolmax, abstolmax,
		    x_xyz, b_xyz, A_xyz
		    );
#endif

#endif

  // --------

  free(A);
  free(x);
  free(b);
  
  free(Dinv);
  free(r);
  free(r_);
  free(s);
  free(p);
  free(q);
  free(z);

  free(block_zfilterL);
  free(block_zfilterU);
  free(block_yfilterL);
  free(block_yfilterU);
  free(block_xfilterL);
  free(block_xfilterU);

#if 1
  if(rank ==0 ){
    printf(" \n");
  }
  pcg_call( prm,
	    10,
	    1.0e-50,1.0e-50,
	    //	    itrmax,
	    //	     rtolmax, abstolmax,
	    x_xyz, b_xyz, A_xyz
	    );
#endif

  return itr ;

}

#else
int pcg_block3d_common_call(mpi_prm prm,
		     int itrmax,
		     type rtolmax,type abstolmax,
		     type* x_xyz,type* b_xyz,type* A_xyz
		     ){
  int rank;
#ifdef JUPITER_MPI
  MPI_Comm_rank(prm.comm, &rank);
#else
  rank=0;
#endif
  int i,itr;
  double ts,ts0,te;
  double time[20];
  int m = prm.m;
  int mx =prm.mx;
  int my =prm.my;
  int mz =prm.mz;
  int stm  = prm.stm;
  int nx = prm.nx;
  int ny = prm.ny;
  int nz = prm.nz;

  // --------------

  set_subdividing_3Dblockjacobi(&prm);
  int nxdivblock = prm.nxdivblock;
  int nydivblock = prm.nydivblock;
  int nzdivblock = prm.nzdivblock;
  int block_nxs[nxdivblock];
  int block_nxe[nxdivblock];
  int block_nys[nydivblock];
  int block_nye[nydivblock];
  int block_nzs[nzdivblock];
  int block_nze[nzdivblock];

  int nzblock = prm.nzblock;
  int nyblock = prm.nyblock;
  int nxblock = prm.nxblock;

  int stride_zp[nzblock];
  int stride_zm[nzblock];
  int stride_yp[nyblock];
  int stride_ym[nyblock];
  int stride_xp[nxblock];
  int stride_xm[nxblock];

  set_subdividing_3Dblock_list(
			       prm,
			       block_nxs,block_nxe,
			       block_nys,block_nye,
			       block_nzs,block_nze,
			       stride_xp,stride_xm,
			       stride_yp,stride_ym,
			       stride_zp,stride_zm			       
			       );
  int block_m = prm.block_m;

  if(rank ==0 ){
    //        printf("m,block_m = %d,%d \n",m,block_m);
    //        printf(" block_mx,block_my,block_mz = %08d, %08d, %08d \n",prm.block_mx,prm.block_my,prm.block_mz);
  }
  type *A    = (type*)malloc(sizeof(type)*(block_m*7));
  type *x    = (type*)malloc(sizeof(type)*(block_m));
  type *b    = (type*)malloc(sizeof(type)*(block_m));

  type *r    = (type*)malloc(sizeof(type)*(block_m));
  type *Dinv = (type*)malloc(sizeof(type)*(block_m));
  type *r_   = (type*)malloc(sizeof(type)*(block_m));
  type *s    = (type*)malloc(sizeof(type)*(block_m));
  type *p    = (type*)malloc(sizeof(type)*(block_m));
  type *q    = (type*)malloc(sizeof(type)*(block_m));
  type *z    = (type*)malloc(sizeof(type)*(block_m));

  zero_initialize(block_m*7,A);
  zero_initialize(block_m,x);
  zero_initialize(block_m,b);

  zero_initialize(block_m,Dinv);
  zero_initialize(block_m,r);
  zero_initialize(block_m,r_);
  zero_initialize(block_m,s);
  zero_initialize(block_m,p);
  zero_initialize(block_m,q);
  zero_initialize(block_m,z);

  int j;
  XYZ_to_blockXYZ(
		  prm,
		  x_xyz, x,
		  block_nxs,  block_nxe, 
		  block_nys,  block_nye, 
		  block_nzs,  block_nze);
  XYZ_to_blockXYZ(
		  prm,
		  b_xyz, b,
		  block_nxs,  block_nxe, 
		  block_nys,  block_nye, 
		  block_nzs,  block_nze);

  for(j=0;j<7;j++){
    XYZ_to_blockXYZ(
		    prm,
		    &A_xyz[m*j], &A[block_m*j],
		    block_nxs,  block_nxe, 
		    block_nys,  block_nye, 
		    block_nzs,  block_nze);

#if 0
    type Anorm = 0.0;
    calc_norm_block3D(prm,&A[block_m*j],&A[block_m*j],&Anorm);
    printf(" A matrix %d : %f \n",j,Anorm);
#endif

  }

  type *block_zfilterL = (type*)malloc(sizeof(type)*(nzblock*nzdivblock));
  type *block_zfilterU = (type*)malloc(sizeof(type)*(nzblock*nzdivblock));
  zero_initialize(nzblock*nzdivblock,block_zfilterL);
  zero_initialize(nzblock*nzdivblock,block_zfilterU);
  type *block_yfilterL = (type*)malloc(sizeof(type)*(nyblock*nydivblock));
  type *block_yfilterU = (type*)malloc(sizeof(type)*(nyblock*nydivblock));
  zero_initialize(nyblock*nydivblock,block_yfilterL);
  zero_initialize(nyblock*nydivblock,block_yfilterU);
  type *block_xfilterL = (type*)malloc(sizeof(type)*(nxblock*nxdivblock));
  type *block_xfilterU = (type*)malloc(sizeof(type)*(nxblock*nxdivblock));
  zero_initialize(nxblock*nxdivblock,block_xfilterL);
  zero_initialize(nxblock*nxdivblock,block_xfilterU);

  make_pre_subdividing_idiagMat1_block3D(
					 prm,
					 A,Dinv,
					 block_nxs, block_nxe, 
					 block_nys, block_nye, 
					 block_nzs, block_nze,
					 stride_xp, stride_xm,
					 stride_yp, stride_ym,
					 stride_zp, stride_zm,
					 block_xfilterL,block_xfilterU,
					 block_yfilterL,block_yfilterU,
					 block_zfilterL,block_zfilterU
					 );

  type rhon[2];
  rhon[0]=(double)0.0;
  rhon[1]=(double)0.0;
  type tmp;
  type abstol,rtol,abstol0;
  type alpha,beta,gamma,rmax,bmax;

  calc_norm_block3D(prm,b,b,&bmax);

#ifdef JUPITER_MPI
  //  zero_initialize(block_m,x);
  sleev_comm_block3D(x,&prm,
		     block_nxs,  block_nxe, 
		     block_nys,  block_nye, 
		     block_nzs,  block_nze);
#endif

  //  calc_res(prm,A,x,b,r);
#if 0
  calc_res_nxnynz_block3D(
		   prm, A, x, b, r,
		   block_nxs, block_nxe, 
		   block_nys, block_nye, 
		   block_nzs, block_nze,
		   stride_xp,stride_xm,
		   stride_yp,stride_ym,
		   stride_zp,stride_zm 
		   );
#else
  calc_res_block3D(
		   prm, A, x, b, r,
		   stride_xp,stride_xm,
		   stride_yp,stride_ym,
		   stride_zp,stride_zm 
		   );
#endif
  alpha=(double)0.0;
  beta=(double)0.0;

#if 0
  calc_norm_nxnynz_block3D(prm,r,r,&abstol,
			  block_nxs, block_nxe, 
			  block_nys, block_nye, 
			  block_nzs, block_nze	    
			  );
#else
  calc_norm_block3D(prm,r,r,&abstol);
#endif
  //  solve_pre_mat0_nosleev(prm,A,Dinv,r,s);
  //  axpy_block3D(prm,r,s,1.0,0.0);
#if 0
  solve_pre_subdividing_mat0_local_nxnynz_block3D(
					   prm, A, Dinv,
					   r, s,
					   block_nxs, block_nxe, 
					   block_nys, block_nye, 
					   block_nzs, block_nze,
					   stride_xp, stride_xm,
					   stride_yp, stride_ym,
					   stride_zp, stride_zm,
					   block_xfilterL,block_xfilterU,
					   block_yfilterL,block_yfilterU,
					   block_zfilterL,block_zfilterU
					   );
  
  calc_dot_nxnynz_block3D(prm,r,s,&rhon[0],
			  block_nxs, block_nxe, 
			  block_nys, block_nye, 
			  block_nzs, block_nze	    
			  );
#else
  solve_pre_subdividing_mat0_local_block3D(
					   prm, A, Dinv,
					   r, s,
					   block_nxs, block_nxe, 
					   block_nys, block_nye, 
					   block_nzs, block_nze,
					   stride_xp, stride_xm,
					   stride_yp, stride_ym,
					   stride_zp, stride_zm,
					   block_xfilterL,block_xfilterU,
					   block_yfilterL,block_yfilterU,
					   block_zfilterL,block_zfilterU
					   );

  calc_dot_block3D(prm,r,s,&rhon[0]);
#endif

  rtol=abstol/bmax;

#ifdef RES_history
  if(rank ==0 ){
    printf("Iteration ,Absolute res ,Relative res \n");
    printf("@res@  %4d, %16.14e %16.14e \n",0,abstol,rtol);
  }
#endif

  for(itr=0;itr<itrmax;itr++){

#if 1
    printf("      alpha,beta = %f, %f \n",alpha,beta);
    printf("      rho0, rho1 = %f, %f \n",rhon[0],rhon[1]);
    printf("      gamma      = %f \n",gamma);
#endif

    ts=cpu_time();
    //    axpy2(prm,alpha,beta,s,p,x);
    axpy2_block3D(prm,alpha,beta,s,p,x);

    te=cpu_time();
    time[0]=time[0]+te-ts;

    // -----------------

    ts=cpu_time();
#ifdef JUPITER_MPI
    sleev_comm_block3D(p,&prm,
		       block_nxs,  block_nxe, 
		       block_nys,  block_nye, 
		       block_nzs,  block_nze);
#endif
    te=cpu_time();
    time[1]=time[1]+te-ts;

    ts0=cpu_time();
    //    MatVec_dot_local(prm,A,p,q,&gamma);
#if 0
    MatVec_dot_local_nxnynz_block3D(
			     prm, A, p, q,&gamma,
			     block_nxs, block_nxe, 
			     block_nys, block_nye, 
			     block_nzs, block_nze,
			     stride_xp,stride_xm,
			     stride_yp,stride_ym,
			     stride_zp,stride_zm 
			     );    
    calc_dot_nxnynz_block3D(prm,p,q,&gamma,
			    block_nxs, block_nxe, 
			    block_nys, block_nye, 
			    block_nzs, block_nze	    
			    );

#else
    MatVec_dot_local_block3D(
			     prm, A, p, q,&gamma,
			     stride_xp,stride_xm,
			     stride_yp,stride_ym,
			     stride_zp,stride_zm 
			     );
    tmp=gamma;
#ifdef JUPITER_MPI
    MPI_Allreduce(&tmp, &gamma, 1,MPI_TYPE, MPI_SUM,prm.comm);
#else
    gamma=tmp;
#endif

#endif


    te=cpu_time();
    time[9]=time[9]+te-ts;

    te=cpu_time();
    time[2]=time[2]+te-ts0;

    // -----------------
    // -----------------

    ts0=cpu_time();
    alpha=-rhon[0]/gamma;

#if 0
    //    solve_pre_mat2_local(prm,A,Dinv,r,s,q,alpha,&rhon[1],&abstol);
    solve_pre_subdividing_mat2_local_nxnynz_block3D(
					     prm, A, Dinv,
					     r, s,q,
					     z,				   
					     alpha,&rhon[1],&abstol,
					     block_nxs, block_nxe, 
					     block_nys, block_nye, 
					     block_nzs, block_nze,
					     stride_xp, stride_xm,
					     stride_yp, stride_ym,
					     stride_zp, stride_zm,
					     block_xfilterL,block_xfilterU,
					     block_yfilterL,block_yfilterU,
					     block_zfilterL,block_zfilterU
					     );
    calc_dot_nxnynz_block3D(prm,r,s,&rhon[1],
			    block_nxs, block_nxe, 
			    block_nys, block_nye, 
			    block_nzs, block_nze	    
			    );
    calc_norm_nxnynz_block3D(prm,r,r,&abstol,
			    block_nxs, block_nxe, 
			    block_nys, block_nye, 
			    block_nzs, block_nze	    
			     );

#else
    solve_pre_subdividing_mat2_local_block3D(
					     prm, A, Dinv,
					     r, s,q,
					     z,				   
					     alpha,&rhon[1],&abstol,
					     block_nxs, block_nxe, 
					     block_nys, block_nye, 
					     block_nzs, block_nze,
					     stride_xp, stride_xm,
					     stride_yp, stride_ym,
					     stride_zp, stride_zm,
					     block_xfilterL,block_xfilterU,
					     block_yfilterL,block_yfilterU,
					     block_zfilterL,block_zfilterU
					     );


    ts=cpu_time(); 
    type s_reduce[2];
    type r_reduce[2];
    s_reduce[0]=rhon[1];
    s_reduce[1]=abstol;
    r_reduce[0]=(double)0.0;
    r_reduce[1]=(double)0.0;
#ifdef JUPITER_MPI
    MPI_Allreduce(s_reduce, r_reduce, 2,MPI_TYPE, MPI_SUM,prm.comm); 
#else
    r_reduce[0]=s_reduce[0];
    r_reduce[1]=s_reduce[1];
#endif
    rhon[1]=r_reduce[0];        
    abstol=sqrt(fabs(r_reduce[1])); 

#endif


    te=cpu_time();
    time[8]=time[8]+te-ts;

    //--------
    te=cpu_time();
    time[4]=time[4]+te-ts0;

    alpha=-alpha;
    beta=rhon[1]/rhon[0];
    rhon[0]=rhon[1];
    rtol=abstol/bmax;

#ifdef RES_history
    if(rank ==0 ){
      printf("@res@  %4d, %16.14e %16.14e \n",itr+1,abstol,rtol);
    }
#endif

    if( rtol < rtolmax ){
      break;
    }
    if( abstol < abstolmax ){
      break;
    }  
  }
  //  axpy(prm,alpha,p,x);
  // x = x + alpha*p
  axpy_block3D(prm,p,x,alpha,1.0);

#if 0
  blockXYZ_to_XYZ(
		  prm,
		  x_xyz, x,
		  block_nxs,  block_nxe, 
		  block_nys,  block_nye, 
		  block_nzs,  block_nze);
#else
  printf(" @@@@@@@@@@@@@@@@@ \n");
  printf(" @@@@@@@@@@@@@@@@@ \n");
  printf(" @@@@@@@@@@@@@@@@@ \n");
  pcg_block3d_call( prm,
		    itrmax,
		    rtolmax, abstolmax,
		    x_xyz, b_xyz, A_xyz
		    );

#endif

  // --------

  free(A);
  free(x);
  free(b);
  
  free(Dinv);
  free(r);
  free(r_);
  free(s);
  free(p);
  free(q);
  free(z);

  free(block_zfilterL);
  free(block_zfilterU);
  free(block_yfilterL);
  free(block_yfilterU);
  free(block_xfilterL);
  free(block_xfilterU);

#if 0
  if(rank ==0 ){
    printf(" \n");
  }
  pcg_call( prm,
	    10,
	    1.0e-50,1.0e-50,
	    //	    itrmax,
	    //	     rtolmax, abstolmax,
	    x_xyz, b_xyz, A_xyz
	    );
#endif
  return itr ;

}
#endif
