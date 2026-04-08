#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// #define pjacobi 1
// #define PJACOBIIR 1

#include "struct.h"
#include "cg.h"

#ifdef GMP
#include "gmp.h"
#endif

// #include "kernel.h"
//------
// 7 point difference 
//------
int zero_initialize(int m,type *x){
  int i;
#pragma omp parallel for private(i) 
  for(i=0;i<m;i++){ 
    x[i] = 0.0;
  }
  return 0;
}

int set_max_threads(int nz,int *max_threads_){
  int max_threads;

#ifdef _OPENMP

#pragma omp parallel 
  {
#pragma omp master
    {
      max_threads=omp_get_num_threads();
    }
  }
  if(nz<max_threads){
    omp_set_num_threads(nz);
  }
  max_threads_[0]=max_threads;
#else
  max_threads_[0]=1;
#endif

  return 0;
}

int set_omp_threads(int max_threads){
#ifdef _OPENMP
  omp_set_num_threads(max_threads);
#endif
  return 0;
}

int set_dim(mpi_prm prm,int* stm,int* nx,int* ny,int* nz,int* m,int* mx,int* my,int* mxy){
  stm[0]=prm.stm;
  nx[0] =prm.nx;
  ny[0] =prm.ny;
  nz[0] =prm.nz;
  m[0]  =prm.m;
  mx[0] =prm.mx;
  my[0] =prm.my;
  mxy[0]=prm.mxy;
  return 0;
}



int solve_pre_mat2(mpi_prm prm, type *A,type *Dinv,type *r, type *s,type *q,type alpha,type *tmp,type *tmpn){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  type tmp1_;
  type tmp2_;

  type s_reduce[2],r_reduce[2];

  type sum;
  int j;
  int jz,jx,jy;
  //  int jcw,jcs,jcb,jce,jcn,jct,jcc;
  int jcs,jcb,jcn,jct;

  int thid,num_th;
  int nzs_th,nze_th,nzs,nze;

  nzs=0;
  nze=nz-1;

  tmp1_=(double)0.0;
  tmp2_=(double)0.0;

  int max_threads;

#ifdef _OPENMP

#pragma omp parallel 
  {
#pragma omp master
    {
      max_threads = omp_get_num_threads();
      //      printf("max_threads  = %d\n",max_threads);
    }
  }
  if(nz<max_threads){
    omp_set_num_threads(nz);
  }
#else
   max_threads = 1;
#endif

#ifdef  poisson

#else
#pragma omp parallel default(none)				\
  shared(stm,nx,ny,nz,m,mx,mxy,A,Dinv,r,s,q,alpha,nzs,nze)	\
  private(nzs_th,nze_th,thid,num_th)				\
  private(j,jcs,jcb,jcn,jct,jx,jy,jz,sum)			\
  reduction(+:tmp1_,tmp2_)
#endif
  {
#ifdef _OPENMP
    thid   = omp_get_thread_num(); 
    num_th = omp_get_num_threads(); 
#else
    thid   = 0;
    num_th = 1;
#endif
    Sep_region(&thid,&num_th,&nzs,&nze,&nzs_th,&nze_th);    

    type sum_[nx+1];  

    jz =nzs_th;
    for(jy = 0; jy < ny; jy++) {
      jx=0;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcs = j-mx  ;
      jcb = j-mxy ;
      for(jx = 0; jx < nx; jx++) {
	r[j]=r[j]+q[j]*alpha;
	sum_[jx]=r[j]-(A[j+1*m]*s[jcs]);
	j=j+1;
	jcs=jcs+1;
	jcb=jcb+1;
      }
      jx=0;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      j = j     ;
      //      jcw = j-1   ;
      sum=0;
      for(jx = 0; jx < nx; jx++) {
	sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
	s[j]=sum;
	j=j+1;
      }
    }   

    for(jz =nzs_th+1;jz<=nze_th;jz++){
      for(jy = 0; jy < ny; jy++) {

	jx=0;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcs = j-mx  ;
	jcb = j-mxy ;
	for(jx = 0; jx < nx; jx++) {
	  r[j]=r[j]+q[j]*alpha;
	  sum_[jx]=r[j]-(A[j+1*m]*s[jcs] + A[j+0*m]*s[jcb]);
	  j=j+1;
	  jcs=jcs+1;
	  jcb=jcb+1;
	}

	jx=0;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sum=0;
	for(jx = 0; jx < nx; jx++) {
	  sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
	  s[j]=sum;
	  j=j+1;
	}
      }  
    }
 
    sum_[0]=(double)0.0;
    jz =nze_th;
    for(jy = ny-1; jy >=0 ; jy--) {
      jx = nx-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcn = j+mx  ;
      jct = j+mxy ;
      for(jx = nx-1; jx >=0 ; jx--) {
	sum_[jx+1] =  A[j+5*m]*s[jcn] ;
	j   = j-1;
	jcn = jcn-1;
	jct = jct-1;
      }
      jx=nx-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      sum=0;
      for(jx = nx-1; jx >=0 ; jx--) {
	sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
	s[j]=sum;
	j=j-1;
      }
      
      jx=nx-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      for(jx = nx-1; jx >=0 ; jx--) {
	tmp1_=tmp1_+s[j]*r[j];
	tmp2_=tmp2_+r[j]*r[j];
	j=j-1;
      }    
    }

    for(jz =nze_th-1;jz>=nzs_th;jz--){
      for(jy = ny-1; jy >=0 ; jy--) {

	jx = nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcn = j+mx  ;
	jct = j+mxy ;
	for(jx = nx-1; jx >=0 ; jx--) {
	  sum_[jx+1] =  A[j+5*m]*s[jcn] + A[j+6*m]*s[jct];
	  j   = j-1;
	  jcn = jcn-1;
	  jct = jct-1;
	}

	jx=nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sum=0;
	for(jx = nx-1; jx >=0 ; jx--) {
	  sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
	  s[j]=sum;
	  j=j-1;
	}

	jx=nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	for(jx = nx-1; jx >=0 ; jx--) {
	  tmp1_=tmp1_+s[j]*r[j];
	  tmp2_=tmp2_+r[j]*r[j];
	  j=j-1;
	}

      }  
    } 

  }

  s_reduce[0]=tmp1_;
  s_reduce[1]=tmp2_;
  r_reduce[0]=(double)0.0;
  r_reduce[1]=(double)0.0;

#ifdef JUPITER_MPI
  MPI_Allreduce(s_reduce, r_reduce, 2,MPI_TYPE, MPI_SUM,prm.comm); 
#else
  r_reduce[0]=s_reduce[0];
  r_reduce[1]=s_reduce[1];
#endif
  tmp[0]=r_reduce[0]; 
  tmpn[0]=sqrt(r_reduce[1]);

#ifdef _OPENMP
  omp_set_num_threads(max_threads);
#endif

  return 0;
}


int solve_pre_mat2_local(mpi_prm prm, type *A,type *Dinv,type *r, type *s,type *q,type alpha,type *tmp,type *tmpn){
  
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  type tmp1_;
  type tmp2_;

  type s_reduce[2],r_reduce[2];

  type sum;
  int j;
  int jz,jx,jy;
  //  int jcw,jcs,jcb,jce,jcn,jct,jcc;
  int jcs,jcb,jcn,jct;

  int thid,num_th;
  int nzs_th,nze_th,nzs,nze;

  nzs=0;
  nze=nz-1;

  tmp1_=(double)0.0;
  tmp2_=(double)0.0;

  int max_threads;
#ifdef _OPENMP

#pragma omp parallel 
  {
#pragma omp master
    {
      max_threads=omp_get_num_threads();
      //      printf("max_threads  = %d\n",max_threads);
    }
  }
  if(nz<max_threads){
    omp_set_num_threads(nz);
  }
#else
  max_threads=1;
#endif

#ifdef pjacobi

#pragma omp parallel for private(j,jz,jy,jx) reduction(+:tmp1_,tmp2_)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	//	s[j]=r[j];
	r[j]=r[j]+q[j]*alpha;
	s[j]=r[j]/A[j+3*m];
	tmp1_=tmp1_+s[j]*r[j];
	tmp2_=tmp2_+r[j]*r[j];
      }
    }
  }
  s_reduce[0]=tmp1_;
  s_reduce[1]=tmp2_;
  tmp[0] =tmp1_;// r_reduce[0]; 
  tmpn[0]=tmp2_; // sqrt(r_reduce[1]);

  return 0;
#endif

#ifdef PJACOBIIR

#pragma omp parallel for private(j,jz,jy,jx) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	r[j]=r[j]+q[j]*alpha;
      }
    }
  }
  type y[m];
  for(j = 0; j < m; j++) {
     y[j] = 0.0;
  }

#pragma omp parallel for private(j,jz,jy,jx)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	s[j]=r[j]/A[j+3*m];
      }
    }
  }
  calc_res(prm, A,s,r,y);
#pragma omp parallel for private(j,jz,jy,jx)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	s[j]=s[j] + y[j]/A[j+3*m];
      }
    }
  }

#pragma omp parallel for private(j,jz,jy,jx) reduction(+:tmp1_,tmp2_)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	tmp1_=tmp1_+s[j]*r[j];
	tmp2_=tmp2_+r[j]*r[j];
      }
    }
  }
  s_reduce[0]=tmp1_;
  s_reduce[1]=tmp2_;
  tmp[0] =tmp1_;// r_reduce[0]; 
  tmpn[0]=tmp2_; // sqrt(r_reduce[1]);

  return 0;
#endif

#ifdef  poisson

#else
#pragma omp parallel default(none)				\
  shared(stm,nx,ny,nz,m,mx,mxy,A,Dinv,r,s,q,alpha,nzs,nze)	\
  private(nzs_th,nze_th,thid,num_th)				\
  private(j,jcs,jcb,jcn,jct,jx,jy,jz,sum)			\
  reduction(+:tmp1_,tmp2_)
#endif
  {
#ifdef _OPENMP
    thid   = omp_get_thread_num(); 
    num_th = omp_get_num_threads(); 
#else
    thid   = 0;
    num_th = 1;
#endif
    Sep_region(&thid,&num_th,&nzs,&nze,&nzs_th,&nze_th);    

    type sum_[nx+1];  

    jz =nzs_th;
    for(jy = 0; jy < ny; jy++) {
      jx=0;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcs = j-mx  ;
      jcb = j-mxy ;
      for(jx = 0; jx < nx; jx++) {
	r[j]=r[j]+q[j]*alpha;
	sum_[jx]=r[j]-(A[j+1*m]*s[jcs]);
	j=j+1;
	jcs=jcs+1;
	jcb=jcb+1;
      }
      jx=0;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      j = j     ;
      //      jcw = j-1   ;
      sum=0;
      for(jx = 0; jx < nx; jx++) {
	sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
	s[j]=sum;
	j=j+1;
      }
    }  

    for(jz =nzs_th+1;jz<=nze_th;jz++){
      for(jy = 0; jy < ny; jy++) {

	jx=0;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcs = j-mx  ;
	jcb = j-mxy ;
	for(jx = 0; jx < nx; jx++) {
	  r[j]=r[j]+q[j]*alpha;
	  sum_[jx]=r[j]-(A[j+1*m]*s[jcs] + A[j+0*m]*s[jcb]);
	  j=j+1;
	  jcs=jcs+1;
	  jcb=jcb+1;
	}

	jx=0;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sum=0;
	for(jx = 0; jx < nx; jx++) {
	  sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
	  s[j]=sum;
	  j=j+1;
	}
      }  
    }
 
    sum_[0]=(double)0.0;
    jz =nze_th;
    for(jy = ny-1; jy >=0 ; jy--) {
      jx = nx-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcn = j+mx  ;
      jct = j+mxy ;
      for(jx = nx-1; jx >=0 ; jx--) {
	sum_[jx+1] =  A[j+5*m]*s[jcn] ;
	j   = j-1;
	jcn = jcn-1;
	jct = jct-1;
      }
      jx=nx-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      sum=0;
      for(jx = nx-1; jx >=0 ; jx--) {
	sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
	s[j]=sum;
	j=j-1;
      }
      

      jx=nx-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

#ifdef intelopt
      // ループ融合を抑制してSIMD化を促進させることが可能。
      // 処理時間はループ融合した方が速い
      // #pragma nofusion
#endif
      for(jx = nx-1; jx >=0 ; jx--) {
	tmp1_=tmp1_+s[j]*r[j];
	tmp2_=tmp2_+r[j]*r[j];
	j=j-1;
      }    
    }

    for(jz =nze_th-1;jz>=nzs_th;jz--){
      for(jy = ny-1; jy >=0 ; jy--) {

	jx = nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcn = j+mx  ;
	jct = j+mxy ;
	for(jx = nx-1; jx >=0 ; jx--) {
	  sum_[jx+1] =  A[j+5*m]*s[jcn] + A[j+6*m]*s[jct];
	  j   = j-1;
	  jcn = jcn-1;
	  jct = jct-1;
	}

	jx=nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sum=0;
	for(jx = nx-1; jx >=0 ; jx--) {
	  sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
	  s[j]=sum;
	  j=j-1;
	}


	jx=nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
#ifdef intelopt
	// ループ融合を抑制してSIMD化を促進させることが可能。
	// 処理時間はループ融合した方が速い
	// #pragma nofusion
#endif
	for(jx = nx-1; jx >=0 ; jx--) {
	  tmp1_=tmp1_+s[j]*r[j];
	  tmp2_=tmp2_+r[j]*r[j];
	  j=j-1;
	}

      }  
    } 

  }

  s_reduce[0]=tmp1_;
  s_reduce[1]=tmp2_;
  r_reduce[0]=(double)0.0;
  r_reduce[1]=(double)0.0;

  //  MPI_Allreduce(s_reduce, r_reduce, 2,MPI_TYPE, MPI_SUM,prm.comm); 
  tmp[0] =tmp1_;// r_reduce[0]; 
  tmpn[0]=tmp2_; // sqrt(r_reduce[1]);

#ifdef _OPENMP
  omp_set_num_threads(max_threads);
#endif

  return 0;
}

int solve_pre_subdividing_mat2_local(
				     mpi_prm prm, type *A,type *Dinv,
				     type *r, type *s,type *q,
				     type *z,
				     type alpha,type *tmp,type *tmpn,
				     type *block_yfilterL,type *block_zfilterL,
				     type *block_yfilterU,type *block_zfilterU,
				     int *block_ys,int *block_zs,
				     int *block_ye,int *block_ze
				     ){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  type tmp1_;
  type tmp2_;

  type s_reduce[2],r_reduce[2];

  int j;
  int jz,jx,jy;
  int jcw,jcs,jcb,jce,jcn,jct,jcc;

  tmp1_=(double)0.0;
  tmp2_=(double)0.0;

#ifdef pjacobi

#pragma omp parallel for private(j,jz,jy,jx) reduction(+:tmp1_,tmp2_)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	r[j]  = r[j]+q[j]*alpha;
	s[j]  = r[j]/A[j+3*m];
	tmp1_ = tmp1_+s[j]*r[j];
	tmp2_ = tmp2_+r[j]*r[j];
      }
    }
  }
  s_reduce[0]=tmp1_;
  s_reduce[1]=tmp2_;
  tmp[0] =tmp1_;
  tmpn[0]=tmp2_;

  return 0;

#endif


  tmp1_=(double)0.0;
  tmp2_=(double)0.0;

  int xdivblock = prm.xdivblock;
  int ydivblock = prm.ydivblock;
  int zdivblock = prm.zdivblock;
  int nxblock   = prm.nxblock;
  int nyblock   = prm.nyblock;
  int nzblock   = prm.nzblock;
  int jxblock,jyblock,jzblock;

#pragma omp parallel for private(jzblock,jyblock,jz,jy,jx,j,jcb,jcs,jcn,jct)
  for(jzblock = 0; jzblock < zdivblock; jzblock++) {
    int jzs = block_zs[jzblock];
    int jze = block_ze[jzblock];    
    for(jyblock = 0; jyblock < ydivblock; jyblock++) {
      int jys = block_ys[jyblock];
      int jye = block_ye[jyblock];

      for(jz = jzs; jz <= jze; jz++) {
	for(jy = jys; jy <= jye; jy++) {
	  for(jx = 0; jx < nx; jx++) {
	    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    jcs = j-mx  ;
	    jcb = j-mxy ;
	    r[j] = r[j]+q[j]*alpha;
	    s[j] = ( r[j] - ( 
			     A[j+1*m]*s[jcs] * block_yfilterL[ jy-jys + jyblock*nyblock ]  + 
			     A[j+0*m]*s[jcb] * block_zfilterL[ jz-jzs + jzblock*nzblock ] 
			      ) ) * Dinv[j];
	  }
	}
      }
      for(jz = jze; jz >=jzs ; jz--) {
	for(jy = jye; jy >=jys ; jy--) {
	  for(jx = nx-1; jx >=0 ; jx--) {
	    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    jcn = j+mx  ;
	    jct = j+mxy ;
	    s[j] = s[j] - (
			   A[j+5*m]*s[jcn] * block_yfilterU[ jy-jys + jyblock*nyblock ] + 
			   A[j+6*m]*s[jct] * block_zfilterU[ jz-jzs + jzblock*nzblock ]
			   )*Dinv[j];
	  }
	}
      }

    }
  }
  // IRしない場合はゼロに指定
#if 1
  // IR 
  // z = r - As
  //  type *z = (type*)malloc(sizeof(type)*(m));
  //  zero_initialize(m,z);

  //  type *y = (type*)malloc(sizeof(type)*(m));
  //  zero_initialize(m,y);

#ifdef JUPITER_MPI
  sleev_comm(s,&prm); 
#endif

#if sMatrix
  calc_sres(prm,A,s,r,z);
#else
  calc_res(prm,A,s,r,z);
#endif
  // solve Ay = z
  // s = s + y
#pragma omp parallel for private(jzblock,jyblock,jz,jy,jx,j,jcb,jcs,jcn,jct,jcw,jce)  reduction(+:tmp1_,tmp2_)
  for(jzblock = 0; jzblock < zdivblock; jzblock++) {
    int jzs = block_zs[jzblock];
    int jze = block_ze[jzblock];    
    for(jyblock = 0; jyblock < ydivblock; jyblock++) {
      int jys = block_ys[jyblock];
      int jye = block_ye[jyblock];

      for(jz = jzs; jz <= jze; jz++) {
	for(jy = jys; jy <= jye; jy++) {
	  for(jx = 0; jx < nx; jx++) {
	    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    jcs = j-mx  ;
	    jcb = j-mxy ;

#if 1
	    z[j] = ( z[j] - ( 
			     A[j+1*m]*z[jcs] * block_yfilterL[ jy-jys + jyblock*nyblock ]  + 
			     A[j+0*m]*z[jcb] * block_zfilterL[ jz-jzs + jzblock*nzblock ] 
			      ) ) * Dinv[j];
#else
	    y[j] = ( z[j] - ( 
			     A[j+1*m]*y[jcs] * block_yfilterL[ jy-jys + jyblock*nyblock ]  + 
			     A[j+0*m]*y[jcb] * block_zfilterL[ jz-jzs + jzblock*nzblock ] 
			      ) ) * Dinv[j];
#endif

	  }
	}
      }
      for(jz = jze; jz >=jzs ; jz--) {
	for(jy = jye; jy >=jys ; jy--) {
	  for(jx = nx-1; jx >=0 ; jx--) {
	    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    jcn = j+mx  ;
	    jct = j+mxy ;
#if 1
	    z[j] = z[j] - (
			   A[j+5*m]*z[jcn] * block_yfilterU[ jy-jys + jyblock*nyblock ] + 
			   A[j+6*m]*z[jct] * block_zfilterU[ jz-jzs + jzblock*nzblock ]
			   )*Dinv[j];
	    s[j] = s[j] + z[j] ;
#else
	    y[j] = y[j] - (
			   A[j+5*m]*y[jcn] * block_yfilterU[ jy-jys + jyblock*nyblock ] + 
			   A[j+6*m]*y[jct] * block_zfilterU[ jz-jzs + jzblock*nzblock ]
			   )*Dinv[j];
	    s[j] = s[j] + y[j] ;
#endif
	    tmp1_=tmp1_+s[j]*r[j];
	    tmp2_=tmp2_+r[j]*r[j];
	  }
	}
      }

    }
  }

  //  free(y);
  //  free(z);
#else
#pragma omp parallel for private(j,jz,jy,jx) reduction(+:tmp1_,tmp2_)
  for(jz = nz-1; jz >=0 ; jz--) {
    for(jy = ny-1; jy >=0 ; jy--) {
      for(jx = nx-1; jx >=0 ; jx--) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	tmp1_=tmp1_+s[j]*r[j];
	tmp2_=tmp2_+r[j]*r[j];
      }
    }
  }
#endif

  s_reduce[0]=tmp1_;
  s_reduce[1]=tmp2_;
  tmp[0] =tmp1_;// r_reduce[0]; 
  tmpn[0]=tmp2_; // sqrt(r_reduce[1]);

  return 0;
}


#if 0
int solve_pre_mat2_local_fp16(mpi_prm prm, type *A,type *Dinv,type *r, type *s,type *q,type alpha,type *tmp,type *tmpn){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

#if 1
  solve_pre_mat2_local_fp16_(
			&stm,&nx,&ny,&nz,
			A,Dinv,r,s,q,
			&alpha,tmp,tmpn);
  return 0;
#endif

  type tmp1_;
  //  type tmp2_;

  type s_reduce[2],r_reduce[2];

  //  type sum;
  int j;
  int jz,jx,jy;
  int jcw,jcs,jcb,jce,jcn,jct,jcc;
  //  int jcs,jcb,jcn,jct;
  
  int thid,num_th;
  int nzs_th,nze_th,nzs,nze;

  //  type r_[m];
  type scaling_vec[m];

  pre_datatype valfp16; // データ変換用
  pretype A_fp16[m*7]; // 演算用
  pretype Dinv_fp16[m]; // 演算用
  pretype s_fp16[m];  // 演算用
  pretype r_fp16[m]; // 演算用
  //  pretype sum_fp16; 

  double ts=cpu_time();

#pragma omp parallel for private(j,valfp16)
 for(j=0;j<m  ;j++){ 
   valfp16   = Dinv[j];
   Dinv_fp16[j] = valfp16;    
   valfp16   = 0.0; 
   s_fp16[j] = valfp16; 
  } 

#pragma omp parallel for private(j)
  for(j=0;j<m;j++){ 
    scaling_vec[j]=0.0;
  }
#pragma omp parallel for private(j,jx,jy,jz) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	scaling_vec[j]=fabs(1.0/Dinv[j] );
      }
    }
  }

#pragma omp parallel for private(j,valfp16)
  for(j=0;j<m*7;j++){
    valfp16   = 0.0;
    A_fp16[j] = valfp16;
  }
#pragma omp parallel for private(j,jx,jy,jz,valfp16)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

	valfp16 = A[j+0*m] /  scaling_vec[j];
	A_fp16[j+0*m] = valfp16;

	valfp16 = A[j+1*m] / scaling_vec[j];
	A_fp16[j+1*m] = valfp16;

	valfp16 = A[j+2*m] / scaling_vec[j];
	A_fp16[j+2*m] = valfp16;

	valfp16 = A[j+3*m] / scaling_vec[j];
	A_fp16[j+3*m] = valfp16;

	valfp16 = A[j+4*m] / scaling_vec[j];
	A_fp16[j+4*m] = valfp16;

	valfp16 = A[j+5*m] / scaling_vec[j];
	A_fp16[j+5*m] = valfp16;

	valfp16 = A[j+6*m] / scaling_vec[j];
	A_fp16[j+6*m] = valfp16;
	
	double tmpval = ( 1.0/Dinv[j] ) / scaling_vec[j];
	valfp16 = 1.0/tmpval;
	Dinv_fp16[j] = valfp16; 

      }
    }
  }
 
  double te=cpu_time();
  //  printf("    @ init @ %f \n",te-ts);

  double rdot=0.0;
  double rmax=0.0;
#pragma omp parallel for private(j,jx,jy,jz)  reduction(max:rmax) reduction(+:rdot)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	r[j]  = r[j] + q[j]*alpha;
	rdot = rdot + r[j]  * r[j] ;
	double rtmp = r[j] / scaling_vec[j];
	if( rmax<fabs(rtmp) ){
	  rmax = fabs(rtmp);
	}
      }
    }
  }
  // rmax = 1.0;

#pragma omp parallel for private(j,valfp16)
 for(j=0;j<m  ;j++){ 
   valfp16   = r[j] / rmax  / scaling_vec[j]; 
   r_fp16[j] = valfp16;  
  } 

   nzs=0; 
   nze=nz-1; 
   tmp1_=(double)0.0; 
   
   int max_threads; 
#if 1

#ifdef _OPENMP

#pragma omp parallel  
   { 
#pragma omp master 
     { 
       max_threads=omp_get_num_threads(); 
     } 
   } 
   if(nz<max_threads){ 
     omp_set_num_threads(nz); 
   }
#else
   max_threads = 1;
#endif
   
   tmp1_ = 0.0;
#pragma omp parallel default(none)				\
  shared(stm,nx,ny,nz,m,mx,mxy,s,r,nzs,nze)	\
  shared(A_fp16,Dinv_fp16,r_fp16,s_fp16,rmax)			\
  private(nzs_th,nze_th,thid,num_th)		       	\
  private(j,jcs,jcb,jcn,jct,jx,jy,jz,valfp16)  \
  reduction(+:tmp1_) 
   { 
#ifdef _OPENMP
     thid   = omp_get_thread_num();  
     num_th = omp_get_num_threads();  
#else
     thid   = 0;
     num_th = 1;
#endif
     Sep_region(&thid,&num_th,&nzs,&nze,&nzs_th,&nze_th);     
       
     //     type sum_[nx+1];   
#if 0
     
#if 0
     _Float16 sum_fp32;
     _Float16 sum_fp32_[nx+1]; 
     _Float16 sumX_fp32_[(nx+1)]; 
     _Float16 sumXY_fp32_[(nx+1)*(ny+1)]; 
#else
     __fp16 sum_fp32;  
     __fp16 sum_fp32_[nx+1]; 
     __fp16 sumX_fp32_[(nx+1)]; 
     __fp16 sumXY_fp32_[(nx+1)*(ny+1)]; 
#endif

#else
     float sum_fp32; 
     float sum_fp32_[nx+1];
     float sumX_fp32_[(nx+1)];
     float sumXY_fp32_[(nx+1)*(ny+1)];
#endif
 
     for(j = 0; j < (nx+1)*(ny+1); j++) { 
       sumXY_fp32_[j]=0.0;
     }
     for(j = 0; j < (nx+1) ; j++) { 
       sum_fp32_[j]  = 0.0;
       sumX_fp32_[j] = 0.0;
     }

     jz =nzs_th; 
     for(jy = 0; jy < ny; jy++) { 
       jx=0; 
       j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm); 
       jcs = j-mx  ; 
       jcb = j-mxy ;
       for(jx = 0; jx < nx; jx++) {
	 valfp16 = r_fp16[j] ; 
	 sum_fp32_[jx] = valfp16;
	 sum_fp32_[jx] = sum_fp32_[jx] - (A_fp16[j+1*m] * sumX_fp32_[jx]);
       
	 j=j+1;
	 jcs=jcs+1;
	 jcb=jcb+1;
       }
       jx=0;
       j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
       j = j     ;
       valfp16=0.0;sum_fp32 = valfp16;
       for(jx = 0; jx < nx; jx++) {
	 sum_fp32 = (sum_fp32_[jx]-A_fp16[j+2*m]*sum_fp32)*Dinv_fp16[j];
	 s_fp16[j]      = sum_fp32;
	 sumX_fp32_[jx] = sum_fp32;
	 sumXY_fp32_[jx + jy*(nx+1)] = sum_fp32;
	 j=j+1;
       }
     }

     for(jz =nzs_th+1;jz<=nze_th;jz++){
       
       for(j = 0; j < (nx+1) ; j++) { 
	 sumX_fp32_[j] = 0.0;
       }
       for(jy = 0; jy < ny; jy++) {
	 jx=0;
	 j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	 jcs = j-mx  ;
	 jcb = j-mxy ;
	 for(jx = 0; jx < nx; jx++) {
	   valfp16 = r_fp16[j] ;
	   sum_fp32_[jx] = valfp16;
	   sum_fp32_[jx] = sum_fp32_[jx] - ( A_fp16[j+1*m] * sumX_fp32_[jx] + A_fp16[j+0*m] * sumXY_fp32_[jx + jy*(nx+1)] );
	   j=j+1;
	   jcs=jcs+1;
	   jcb=jcb+1;
	 }

	 jx=0;
	 j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	 valfp16=0.0;sum_fp32 = valfp16;
	 for(jx = 0; jx < nx; jx++) {
	   sum_fp32=(sum_fp32_[jx]-A_fp16[j+2*m]*sum_fp32)*Dinv_fp16[j];
	   s_fp16[j]=sum_fp32;
	   sumX_fp32_[jx] = sum_fp32;
	   sumXY_fp32_[jx + jy*(nx+1)] = sum_fp32;
	   j=j+1;
	 }
       }  
     }

     //// --------
     //// --------
     //// --------
     //// --------

     for(j = 0; j < (nx+1)*(ny+1); j++) { 
       sumXY_fp32_[j]=0.0;
     }
     for(j = 0; j < (nx+1) ; j++) { 
       sumX_fp32_[j] = 0.0;
     }
     valfp16=0.0;sum_fp32_[0]=valfp16;
     jz =nze_th;
     for(jy = ny-1; jy >=0 ; jy--) {
       jx = nx-1;
       j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
       jcn = j+mx  ;
       jct = j+mxy ;
       for(jx = nx-1; jx >=0 ; jx--) {
	 sum_fp32_[jx+1] =  A_fp16[j+5*m] * sumX_fp32_[jx] ;
	 j   = j-1;
	 jcn = jcn-1;
	 jct = jct-1;
       }
       jx=nx-1;
       j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
       valfp16=0.0; sum_fp32=valfp16;
       for(jx = nx-1; jx >=0 ; jx--) {
	 sum_fp32 = s_fp16[j] - ( (A_fp16[j+4*m]*sum_fp32 + sum_fp32_[jx+1] )*Dinv_fp16[j]);
	 s[j] = sum_fp32  * rmax;
	 sumX_fp32_[jx] = sum_fp32;
	 sumXY_fp32_[jx + jy*(nx+1)] = sum_fp32;
	 tmp1_ = tmp1_ +  s[j] * r[j] ;

	 j=j-1;
       }
     }

     for(jz =nze_th-1;jz>=nzs_th;jz--){

       for(j = 0; j < (nx+1) ; j++) { 
	 sumX_fp32_[j] = 0.0;
       }
       for(jy = ny-1; jy >=0 ; jy--) {
	 jx = nx-1;
	 j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	 jcn = j+mx  ;
	 jct = j+mxy ;
	 for(jx = nx-1; jx >=0 ; jx--) {
	   sum_fp32_[jx+1] =  A_fp16[j+5*m] * sumX_fp32_[jx] + A_fp16[j+6*m]*sumXY_fp32_[jx+jy*(nx+1)];
	   j   = j-1;
	   jcn = jcn-1;
	   jct = jct-1;
	 }

	 jx=nx-1;
	 j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	 valfp16=0.0; sum_fp32=valfp16;
	 for(jx = nx-1; jx >=0 ; jx--) {
	   sum_fp32 = s_fp16[j] - ( (A_fp16[j+4*m]*sum_fp32 + sum_fp32_[jx+1])*Dinv_fp16[j] );
	   sumX_fp32_[jx] = sum_fp32;
	   sumXY_fp32_[jx + jy*(nx+1)] = sum_fp32;
	   s[j] = sum_fp32  * rmax;

	   tmp1_ = tmp1_ +  s[j] * r[j] ;
	   j=j-1;
	 }
       }
     }

   }
#ifdef _OPENMP
   omp_set_num_threads(max_threads);
#endif

#endif

#if 0
   for(j=0;j<m  ;j++){ 
     valfp16 = s_fp16[j];
     s[j] = valfp16 ;
     s[j] =  s[j] * rmax;
   }
#endif

   //x   calc_dot_local(prm,s,r,&tmp1_);
   //   calc_dot_local(prm,r,r,&tmp2_);
 
   s_reduce[0]=tmp1_;
   s_reduce[1]=rdot;
   r_reduce[0]=(double)0.0;
   r_reduce[1]=(double)0.0;

   //  MPI_Allreduce(s_reduce, r_reduce, 2,MPI_TYPE, MPI_SUM,prm.comm); 
   tmp[0] =tmp1_;// r_reduce[0]; 
   tmpn[0]=rdot; // sqrt(r_reduce[1]);

   return 0;
}
#endif

int solve_pre_mat0(mpi_prm prm, type *A,type *Dinv,type *r, type *s){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  type sum;
  int j;
  int jz,jx,jy;
  //  int jcw,jcs,jcb,jce,jcn,jct,jcc;
  int jcs,jcb,jcn,jct;

  int thid,num_th;
  int nzs_th,nze_th,nzs,nze;

  int max_threads;

#ifdef _OPENMP

#pragma omp parallel 
  {
#pragma omp master
    {
      max_threads=omp_get_num_threads();
      //      printf("max_threads  = %d\n",max_threads);
    }
  }
  if(nz<max_threads){
    omp_set_num_threads(nz);
  }
#else
  max_threads = 1;
#endif

#ifdef pjacobi
#pragma omp parallel for private(j,jz,jy,jx)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	//	s[j]=r[j];
	s[j]=r[j]/A[j+3*m];
      }
    }
  }


  return 0;
#endif

#ifdef PJACOBIIR

  type y[m];
  for(j = 0; j < m; j++) {
     y[j] = 0.0;
  }

#pragma omp parallel for private(j,jz,jy,jx)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	s[j]=r[j]/A[j+3*m];
      }
    }
  }
  calc_res(prm, A,s,r,y);
#pragma omp parallel for private(j,jz,jy,jx)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	s[j]=s[j] + y[j]/A[j+3*m];
      }
    }
  }

  return 0;
#endif

  nzs=0;
  nze=nz-1;
#ifdef  poisson

#else
#pragma omp parallel default(none)			\
  shared(stm,nx,ny,nz,m,mx,mxy,A,Dinv,r,s,nzs,nze)	\
  private(nzs_th,nze_th,thid,num_th)			\
  private(j,jcs,jcb,jcn,jct,jx,jy,jz,sum)    
#endif
  {
#ifdef _OPENMP
    thid   = omp_get_thread_num();  
    num_th = omp_get_num_threads();  
#else
    thid   = 0;
    num_th = 1;
#endif
    Sep_region(&thid,&num_th,&nzs,&nze,&nzs_th,&nze_th);    

    type sum_[nx+1];  

    jz =nzs_th;
    for(jy = 0; jy < ny; jy++) {
      jx=0;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcs = j-mx  ;
      jcb = j-mxy ;
      for(jx = 0; jx < nx; jx++) {
	sum_[jx]=r[j]-(A[j+1*m]*s[jcs]);
	j=j+1;
	jcs=jcs+1;
	jcb=jcb+1;
      }
      jx=0;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      j = j     ;
      //      jcw = j-1   ;
      sum=0;
      for(jx = 0; jx < nx; jx++) {
	sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
	s[j]=sum;
	j=j+1;
      }
    }  

    for(jz =nzs_th+1;jz<=nze_th;jz++){
      for(jy = 0; jy < ny; jy++) {

	jx=0;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcs = j-mx  ;
	jcb = j-mxy ;
	for(jx = 0; jx < nx; jx++) {
	  sum_[jx]=r[j]-(A[j+1*m]*s[jcs] + A[j+0*m]*s[jcb]);
	  j=j+1;
	  jcs=jcs+1;
	  jcb=jcb+1;
	}

	jx=0;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sum=0;
	for(jx = 0; jx < nx; jx++) {
	  sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
	  s[j]=sum;
	  j=j+1;
	}

      }  
    }
 
    sum_[0]=(double)0.0;
    jz =nze_th;
    for(jy = ny-1; jy >=0 ; jy--) {
      jx = nx-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcn = j+mx  ;
      jct = j+mxy ;
      for(jx = nx-1; jx >=0 ; jx--) {
	sum_[jx+1] =  A[j+5*m]*s[jcn] ;
	j   = j-1;
	jcn = jcn-1;
	jct = jct-1;
      }
      jx=nx-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      sum=0;
      for(jx = nx-1; jx >=0 ; jx--) {
	sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
	s[j]=sum;
	j=j-1;
      }
    }

    for(jz =nze_th-1;jz>=nzs_th;jz--){
      for(jy = ny-1; jy >=0 ; jy--) {

	jx = nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcn = j+mx  ;
	jct = j+mxy ;
	for(jx = nx-1; jx >=0 ; jx--) {
	  sum_[jx+1] =  A[j+5*m]*s[jcn] + A[j+6*m]*s[jct];
	  j   = j-1;
	  jcn = jcn-1;
	  jct = jct-1;
	}

	jx=nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sum=0;
	for(jx = nx-1; jx >=0 ; jx--) {
	  sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
	  s[j]=sum;
	  j=j-1;
	}
      }  
    } 
  }

#ifdef _OPENMP
  omp_set_num_threads(max_threads);
#endif

  return 0;
}

int solve_pre_mat0_dot(mpi_prm prm, type *A,type *Dinv,type *r, type *s, type *tmp){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  type sum;
  int j;
  int jz,jx,jy;
  //  int jcw,jcs,jcb,jce,jcn,jct,jcc;
  int jcs,jcb,jcn,jct;

  int thid,num_th;
  int nzs_th,nze_th,nzs,nze;
  type tmp_;
  tmp_=(double)0.0;

  int max_threads;

#ifdef _OPENMP
#pragma omp parallel 
  {
#pragma omp master
    {
      max_threads=omp_get_num_threads();
      //      printf("max_threads  = %d\n",max_threads);
    }
  }
  if(nz<max_threads){
    omp_set_num_threads(nz);
  }
#else
  max_threads = 1;
#endif

#ifdef pjacobi
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	//	s[j]=r[j];
	s[j]=r[j]/A[j+3*m];
      }
    }
  }
  return 0;
#endif

  nzs=0;
  nze=nz-1;
#ifdef  poisson

#else
#pragma omp parallel default(none)			\
  shared(stm,nx,ny,nz,m,mx,mxy,A,Dinv,r,s,nzs,nze)	\
  private(nzs_th,nze_th,thid,num_th)			\
  private(j,jcs,jcb,jcn,jct,jx,jy,jz,sum)    \
  reduction(+:tmp_)
#endif
  {

#ifdef _OPENMP
    thid   = omp_get_thread_num();  
    num_th = omp_get_num_threads();  
#else
    thid   = 0;
    num_th = 1;
#endif

    Sep_region(&thid,&num_th,&nzs,&nze,&nzs_th,&nze_th);    

    type sum_[nx+1];  

    jz =nzs_th;
    for(jy = 0; jy < ny; jy++) {
      jx=0;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcs = j-mx  ;
      jcb = j-mxy ;
      for(jx = 0; jx < nx; jx++) {
	sum_[jx]=r[j]-(A[j+1*m]*s[jcs]);
	j=j+1;
	jcs=jcs+1;
	jcb=jcb+1;
      }
      jx=0;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      j = j     ;
      //      jcw = j-1   ;
      sum=0;
      for(jx = 0; jx < nx; jx++) {
	sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
	s[j]=sum;
	j=j+1;
      }
    }  

    for(jz =nzs_th+1;jz<=nze_th;jz++){
      for(jy = 0; jy < ny; jy++) {

	jx=0;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcs = j-mx  ;
	jcb = j-mxy ;
	for(jx = 0; jx < nx; jx++) {
	  sum_[jx]=r[j]-(A[j+1*m]*s[jcs] + A[j+0*m]*s[jcb]);
	  j=j+1;
	  jcs=jcs+1;
	  jcb=jcb+1;
	}

	jx=0;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sum=0;
	for(jx = 0; jx < nx; jx++) {
	  sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
	  s[j]=sum;
	  j=j+1;
	}

      }  
    }
 
    sum_[0]=(double)0.0;
    jz =nze_th;
    for(jy = ny-1; jy >=0 ; jy--) {
      jx = nx-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcn = j+mx  ;
      jct = j+mxy ;
      for(jx = nx-1; jx >=0 ; jx--) {
	sum_[jx+1] =  A[j+5*m]*s[jcn] ;
	j   = j-1;
	jcn = jcn-1;
	jct = jct-1;
      }
      jx=nx-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      sum=0;
      for(jx = nx-1; jx >=0 ; jx--) {
	sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
	s[j]=sum;
	j=j-1;
      }
    }

    for(jz =nze_th-1;jz>=nzs_th;jz--){
      for(jy = ny-1; jy >=0 ; jy--) {

	jx = nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcn = j+mx  ;
	jct = j+mxy ;
	for(jx = nx-1; jx >=0 ; jx--) {
	  sum_[jx+1] =  A[j+5*m]*s[jcn] + A[j+6*m]*s[jct];
	  j   = j-1;
	  jcn = jcn-1;
	  jct = jct-1;
	}

	jx=nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sum=0;
	for(jx = nx-1; jx >=0 ; jx--) {
	  sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
	  s[j]=sum;
	  tmp_=tmp_+s[j]*s[j];

	  j=j-1;
	}
      }  
    } 
  }

#ifdef JUPITER_MPI
  MPI_Allreduce(&tmp_,tmp, 1,MPI_TYPE, MPI_SUM,prm.comm); 
#else
  tmp[0]=tmp_;
#endif

#ifdef _OPENMP
  omp_set_num_threads(max_threads);
#endif
  return 0;
}


int solve_pre_subdividing_mat0(
			       mpi_prm prm, type *A,type *Dinv,
			       type *r, type *s,
			       type *z,
			       type *block_yfilterL,type *block_zfilterL,
			       type *block_yfilterU,type *block_zfilterU,
			       int *block_ys,int *block_zs,
			       int *block_ye,int *block_ze
			       ){

  type alpha,tmp,tmpn;
  tmp = 0.0;
  tmpn = 0.0;
  alpha = 0.0;
  int m = prm.m;
  type *q=(type*)malloc(sizeof(type)*m);
  int j;
  for(j=0;j<m;j++){
    q[j] = 0.0;
  }
  solve_pre_subdividing_mat2_local(prm, A,Dinv,r,s,q,z,
				   alpha,&tmp,&tmpn,
				   block_yfilterL, block_zfilterL,
				   block_yfilterU, block_zfilterU,
				   block_ys, block_zs,
				   block_ye, block_ze
				   );
  free(q);
  return 0;
}

int solve_pre_mat0_nosleev(
			   mpi_prm prm, type *A,type *Dinv,type *r, type *s){

#if 1
  type alpha,tmp,tmpn;
  tmp = 0.0;
  tmpn = 0.0;
  alpha = 0.0;
  int m = prm.m;
  type *q=(type*)malloc(sizeof(type)*m);
  int j;
  for(j=0;j<m;j++){
    q[j] = 0.0;
  }
  solve_pre_mat2_local(prm, A,Dinv,r,s,q,alpha,&tmp,&tmpn);
  free(q);
  return 0;
#else
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  type sum;
  int j;
  int jz,jx,jy;
  //  int jcw,jcs,jcb,jce,jcn,jct,jcc;
  int jcs,jcb,jcn,jct;

  int thid,num_th;
  int nzs_th,nze_th,nzs,nze;

  int max_threads;

#ifdef _OPENMP
#pragma omp parallel 
  {
#pragma omp master
    {
      max_threads=omp_get_num_threads();
      //      printf("max_threads  = %d\n",max_threads);
    }
  }
  if(nz<max_threads){
    omp_set_num_threads(nz);
  }
#else
  max_threads = 1;
#endif

#ifdef pjacobi
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	//	s[j]=r[j];
	s[j]=r[j]/A[j+3*m];
      }
    }
  }
  return 0;
#endif

  nzs=0;
  nze=nz-1;
#ifdef  poisson

#else
#pragma omp parallel default(none)			\
  shared(stm,nx,ny,nz,m,mx,mxy,A,Dinv,r,s,nzs,nze)	\
  private(nzs_th,nze_th,thid,num_th)			\
  private(j,jcs,jcb,jcn,jct,jx,jy,jz,sum)    
#endif
  {
#ifdef _OPENMP
    thid   = omp_get_thread_num();  
    num_th = omp_get_num_threads(); 
#else
    thid   = 0;
    num_th = 1;
#endif 
    Sep_region(&thid,&num_th,&nzs,&nze,&nzs_th,&nze_th);    

    type sum_[nx+1];  

    jz =nzs_th;

    jy=0;
    jx=0;
    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
    for(jx = 0; jx < nx; jx++) {
      sum_[jx]=r[j];
      j=j+1;
    }
    jx=0;
    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
    sum=0;
    for(jx = 0; jx < nx; jx++) {
      sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
      s[j]=sum;
      j=j+1;
    }

    for(jy = 1; jy < ny; jy++) {
      jx=0;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcs = j-mx  ;
      for(jx = 0; jx < nx; jx++) {
	sum_[jx]=r[j]-(A[j+1*m]*s[jcs]);
	j=j+1;
	jcs=jcs+1;
      }
      jx=0;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      sum=0;
      for(jx = 0; jx < nx; jx++) {
	sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
	s[j]=sum;
	j=j+1;
      }
    }

    for(jz =nzs_th+1;jz<=nze_th;jz++){
      jy = 0;
      jx=0;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcb = j-mxy ;
      for(jx = 0; jx < nx; jx++) {
	//	sum_[jx]=r[j]-(A[j+1*m]*s[jcs] + A[j+0*m]*s[jcb]);
	sum_[jx]=r[j]-( A[j+0*m]*s[jcb]);
	j=j+1;
	jcb=jcb+1;
      }

      jx=0;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      sum=0;
      for(jx = 0; jx < nx; jx++) {
	sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
	s[j]=sum;
	j=j+1;
      }
      //
      for(jy = 1; jy < ny; jy++) {
	jx=0;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcs = j-mx  ;
	jcb = j-mxy ;
	for(jx = 0; jx < nx; jx++) {
	  sum_[jx]=r[j]-(A[j+1*m]*s[jcs] + A[j+0*m]*s[jcb]);
	  j=j+1;
	  jcs=jcs+1;
	  jcb=jcb+1;
	}

	jx=0;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sum=0;
	for(jx = 0; jx < nx; jx++) {
	  sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
	  s[j]=sum;
	  j=j+1;
	}

      }  
    }
 
    sum_[0]=(double)0.0;
    jz =nze_th;

    jy = ny-1;
    jx = nx-1;
    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
    for(jx = nx-1; jx >=0 ; jx--) {
      //      sum_[jx+1] =  A[j+5*m]*s[jcn] ;
      sum_[jx+1] = (double)0.0;
    }
    jx=nx-1;
    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
    sum=0;
    for(jx = nx-1; jx >=0 ; jx--) {
      sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
      s[j]=sum;
      j=j-1;
    }

    //
    for(jy = ny-2; jy >=0 ; jy--) {
      jx = nx-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcn = j+mx  ;
      for(jx = nx-1; jx >=0 ; jx--) {
	sum_[jx+1] =  A[j+5*m]*s[jcn] ;
	j   = j-1;
	jcn = jcn-1;
      }
      jx=nx-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      sum=0;
      for(jx = nx-1; jx >=0 ; jx--) {
	sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
	s[j]=sum;
	j=j-1;
      }
    }

    for(jz =nze_th-1;jz>=nzs_th;jz--){

	jy = ny-1;
	jx = nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jct = j+mxy ;
	for(jx = nx-1; jx >=0 ; jx--) {
	  //	  sum_[jx+1] =  A[j+5*m]*s[jcn] + A[j+6*m]*s[jct];
	  sum_[jx+1] = A[j+6*m]*s[jct];
	  j   = j-1;
	  jct = jct-1;
	}

	jx=nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sum=0;
	for(jx = nx-1; jx >=0 ; jx--) {
	  sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
	  s[j]=sum;
	  j=j-1;
	}

      //
      for(jy = ny-2; jy >=0 ; jy--) {
	jx = nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcn = j+mx  ;
	jct = j+mxy ;
	for(jx = nx-1; jx >=0 ; jx--) {
	  sum_[jx+1] =  A[j+5*m]*s[jcn] + A[j+6*m]*s[jct];
	  j   = j-1;
	  jcn = jcn-1;
	  jct = jct-1;
	}

	jx=nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sum=0;
	for(jx = nx-1; jx >=0 ; jx--) {
	  sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
	  s[j]=sum;
	  j=j-1;
	}
      }  

    } 
  }

#ifdef _OPENMP
  omp_set_num_threads(max_threads);
#endif

#endif

  return 0;
}

int solve_pre_mat0_nosleev_dot(mpi_prm prm, type *A,type *Dinv,type *r, type *s, type *tmp){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  type sum;
  int j;
  int jz,jx,jy;
  //  int jcw,jcs,jcb,jce,jcn,jct,jcc;
  int jcs,jcb,jcn,jct;

  int thid,num_th;
  int nzs_th,nze_th,nzs,nze;

  type tmp_;
  tmp_=(double)0.0;


  int max_threads;

#ifdef _OPENMP

#pragma omp parallel 
  {
#pragma omp master
    {
      max_threads=omp_get_num_threads();
      //      printf("max_threads  = %d\n",max_threads);
    }
  }
  if(nz<max_threads){
    omp_set_num_threads(nz);
  }
#else
  max_threads = 1;
#endif

#ifdef pjacobi
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	//	s[j]=r[j];
	s[j]=r[j]/A[j+3*m];
      }
    }
  }
  return 0;
#endif

  nzs=0;
  nze=nz-1;
#ifdef  poisson

#else
#pragma omp parallel default(none)			\
  shared(stm,nx,ny,nz,m,mx,mxy,A,Dinv,r,s,nzs,nze)	\
  private(nzs_th,nze_th,thid,num_th)			\
  private(j,jcs,jcb,jcn,jct,jx,jy,jz,sum)    \
  reduction(+:tmp_)
#endif
  {
#ifdef _OPENMP
    thid   = omp_get_thread_num();  
    num_th = omp_get_num_threads();  
#else
    thid   = 0;
    num_th = 1;
#endif
    Sep_region(&thid,&num_th,&nzs,&nze,&nzs_th,&nze_th);    

    type sum_[nx+1];  

    jz =nzs_th;

    jy=0;
    jx=0;
    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
    for(jx = 0; jx < nx; jx++) {
      sum_[jx]=r[j];
      j=j+1;
    }
    jx=0;
    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
    sum=0;
    for(jx = 0; jx < nx; jx++) {
      sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
      s[j]=sum;
      j=j+1;
    }

    for(jy = 1; jy < ny; jy++) {
      jx=0;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcs = j-mx  ;
      for(jx = 0; jx < nx; jx++) {
	sum_[jx]=r[j]-(A[j+1*m]*s[jcs]);
	j=j+1;
	jcs=jcs+1;
      }
      jx=0;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      sum=0;
      for(jx = 0; jx < nx; jx++) {
	sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
	s[j]=sum;
	j=j+1;
      }
    }

    for(jz =nzs_th+1;jz<=nze_th;jz++){
      jy = 0;
      jx=0;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcb = j-mxy ;
      for(jx = 0; jx < nx; jx++) {
	//	sum_[jx]=r[j]-(A[j+1*m]*s[jcs] + A[j+0*m]*s[jcb]);
	sum_[jx]=r[j]-( A[j+0*m]*s[jcb]);
	j=j+1;
	jcb=jcb+1;
      }

      jx=0;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      sum=0;
      for(jx = 0; jx < nx; jx++) {
	sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
	s[j]=sum;
	j=j+1;
      }
      //
      for(jy = 1; jy < ny; jy++) {
	jx=0;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcs = j-mx  ;
	jcb = j-mxy ;
	for(jx = 0; jx < nx; jx++) {
	  sum_[jx]=r[j]-(A[j+1*m]*s[jcs] + A[j+0*m]*s[jcb]);
	  j=j+1;
	  jcs=jcs+1;
	  jcb=jcb+1;
	}

	jx=0;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sum=0;
	for(jx = 0; jx < nx; jx++) {
	  sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
	  s[j]=sum;
	  j=j+1;
	}

      }  
    }
 
    sum_[0]=(double)0.0;
    jz =nze_th;

    jy = ny-1;
    jx = nx-1;
    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
    for(jx = nx-1; jx >=0 ; jx--) {
      //      sum_[jx+1] =  A[j+5*m]*s[jcn] ;
      sum_[jx+1] = (double)0.0;
    }
    jx=nx-1;
    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
    sum=0;
    for(jx = nx-1; jx >=0 ; jx--) {
      sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
      s[j]=sum;
      j=j-1;
    }

    //
    for(jy = ny-2; jy >=0 ; jy--) {
      jx = nx-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcn = j+mx  ;
      for(jx = nx-1; jx >=0 ; jx--) {
	sum_[jx+1] =  A[j+5*m]*s[jcn] ;
	j   = j-1;
	jcn = jcn-1;
      }
      jx=nx-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      sum=0;
      for(jx = nx-1; jx >=0 ; jx--) {
	sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
	s[j]=sum;
	j=j-1;
      }
    }

    for(jz =nze_th-1;jz>=nzs_th;jz--){

	jy = ny-1;
	jx = nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jct = j+mxy ;
	for(jx = nx-1; jx >=0 ; jx--) {
	  //	  sum_[jx+1] =  A[j+5*m]*s[jcn] + A[j+6*m]*s[jct];
	  sum_[jx+1] = A[j+6*m]*s[jct];
	  j   = j-1;
	  jct = jct-1;
	}

	jx=nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sum=0;
	for(jx = nx-1; jx >=0 ; jx--) {
	  sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
	  s[j]=sum;
	  j=j-1;
	}

      //
      for(jy = ny-2; jy >=0 ; jy--) {
	jx = nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcn = j+mx  ;
	jct = j+mxy ;
	for(jx = nx-1; jx >=0 ; jx--) {
	  sum_[jx+1] =  A[j+5*m]*s[jcn] + A[j+6*m]*s[jct];
	  j   = j-1;
	  jcn = jcn-1;
	  jct = jct-1;
	}

	jx=nx-1;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sum=0;
	for(jx = nx-1; jx >=0 ; jx--) {
	  sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
	  s[j]=sum;
	  tmp_=tmp_+s[j]*s[j];

	  j=j-1;
	}
      }  

    } 
  }

#ifdef JUPITER_MPI
  MPI_Allreduce(&tmp_,tmp, 1,MPI_TYPE, MPI_SUM,prm.comm); 
#else
  tmp[0]=tmp_;
#endif

#ifdef _OPENMP
  omp_set_num_threads(max_threads);
#endif
  return 0;
}


int calc_norm2(mpi_prm prm,type *x, type *y,type *tmp,type *tmpn){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  int j;
  int jz,jx,jy;

  type tmp1_;
  type tmp2_;
  type s_reduce[2],r_reduce[2];
  tmp1_=(double)0.0;
  tmp2_=(double)0.0;
#pragma omp parallel for private(jx,jy,jz,j) reduction(+:tmp1_,tmp2_)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	tmp1_=tmp1_+y[j]*x[j];
	tmp2_=tmp2_+x[j]*x[j];
      }
    }
  }
  s_reduce[0]=tmp1_;
  s_reduce[1]=tmp2_;
  r_reduce[0]=(double)0.0;
  r_reduce[1]=(double)0.0;

#ifdef JUPITER_MPI
  MPI_Allreduce(s_reduce, r_reduce, 2,MPI_TYPE, MPI_SUM,prm.comm); 
#else
  r_reduce[0]=s_reduce[0];
  r_reduce[1]=s_reduce[1];
#endif
  tmp[0]=r_reduce[0]; 
  tmpn[0]=sqrt(r_reduce[1]);
  return 0;
}

int calc_norm2_local(mpi_prm prm,type *x, type *y,type *tmp,type *tmpn){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  int j;
  int jz,jx,jy;

  type tmp1_;
  type tmp2_;
  type s_reduce[2],r_reduce[2];
  tmp1_=(double)0.0;
  tmp2_=(double)0.0;
#pragma omp parallel for private(jx,jy,jz,j) reduction(+:tmp1_,tmp2_)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	tmp1_=tmp1_+y[j]*x[j];
	tmp2_=tmp2_+x[j]*x[j];
      }
    }
  }
  s_reduce[0]=tmp1_;
  s_reduce[1]=tmp2_;
  r_reduce[0]=(double)0.0;
  r_reduce[1]=(double)0.0;

  //  MPI_Allreduce(s_reduce, r_reduce, 2,MPI_TYPE, MPI_SUM,prm.comm); 
  r_reduce[0]=s_reduce[0];
  r_reduce[1]=s_reduce[1];
  tmp[0]=r_reduce[0]; 
  tmpn[0]=sqrt(r_reduce[1]);
  return 0;
}

int calc_res(mpi_prm prm, type *A, type *x,type *b,type *r){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;


#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	jce = j+1;
	jcn = j+mx;
	jct = j+mxy;

	r[j]=b[j]-
	  (
	   A[j+0*m]*x[jcb] 
	   +A[j+1*m]*x[jcs] 
	   +A[j+2*m]*x[jcw] 
	   +A[j+3*m]*x[jcc] 
	   +A[j+4*m]*x[jce] 
	   +A[j+5*m]*x[jcn] 
	   +A[j+6*m]*x[jct] 
	   	   );


      }
    }
  }
  return 0;
}

int calc_sres(mpi_prm prm, type *A, type *x,type *b,type *r){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;


#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	jce = j+1;
	jcn = j+mx;
	jct = j+mxy;

#if 0
	r[j]=b[j]-
	  (
	    A[j+0*m]*x[jcb] 
	   +A[j+1*m]*x[jcs] 
	   +A[j+2*m]*x[jcw] 
	   +A[j+3*m]*x[jcc] 
	   +A[j+4*m]*x[jce] 
	   +A[j+5*m]*x[jcn] 
	   +A[j+6*m]*x[jct] 
	   	   );
#else
	r[j]=b[j]-
	  (
	   A[j   + 0*m]*x[jcb] 
	  +A[j   + 1*m]*x[jcs] 
	  +A[j   + 2*m]*x[jcw] 
	  +A[j   + 3*m]*x[jcc] 
	  +A[jce + 2*m]*x[jce] 
	  +A[jcn + 1*m]*x[jcn] 
	  +A[jct + 0*m]*x[jct] 
	   	   );
#endif


      }
    }
  }
  return 0;
}

int initializ_sleev(mpi_prm prm,type* x){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int l;
  int nl;
  nl=1;

#pragma omp parallel 
  {

#pragma omp for nowait  private(j,jz,jy,jx)  schedule(dynamic,1) 
  for(jy = 0; jy < ny; jy++) {
    for(jx = 0; jx < nx; jx++) {
      jz=-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      x[j]=0.0;

      jz=nz;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      x[j]=0.0;
    }
  }
  
#pragma omp for nowait  private(j,jz,jy,jx)  schedule(dynamic,1) 
  for(jz = 0; jz < nz; jz++) {
    for(jx = 0; jx < nx; jx++) {
      jy=-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      x[j]=0.0;

      jy=ny;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      x[j]=0.0;
    }
  }
  
#pragma omp for nowait  private(j,jz,jy,jx)  schedule(dynamic,1) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      jx=-1;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      x[j]=0.0;
      jx=nx;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      x[j]=0.0;
    }
  }
  }
  return 0;
}


//    y[] = a*y[] + x[]
int axpy_(mpi_prm prm,type a,type* x,type* y){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

#pragma omp parallel for private(j,jz,jy,jx)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
        y[j]=x[j]+a*y[j];
      }
    }
  }
  return 0;
}

//z[] = z[] + a*y[]
//y[] = x[] + b*y[]
int axpy2(mpi_prm prm,type a,type b,type* x,type* y,type* z){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

#pragma omp parallel for private(j,jz,jy,jx)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	z[j]=a*y[j]+z[j];
	y[j]=b*y[j]+x[j];
      }
    }
  }
  return 0;
}

// -----
// 結果が対称行列になる行列積の計算
// 計算は上三角領域のみ行う
int syrk(mpi_prm prm,type* x,type* y,type* z,int s){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy,js,jk;
  type tmp[s*s];

  for(js = 0; js < s; js++) {
    for(jk = 0; jk < s; jk++) {
      z[jk+js*s]=(double)0.0;
    }
  }
  //      printf("js=%4d ,jk=%4d \n",js,jk);
#pragma omp parallel  private(j,jz,jy,jx,js,jk,tmp)
  {
    for(js = 0; js < s; js++) {
      for(jk = 0; jk < s; jk++) {
	tmp[jk+js*s]=(double)0.0;
      }
    }

#pragma omp for nowait // reduction(+:tmp)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
	for(js = 0; js < s; js++) {
	  for(jk = js; jk < s; jk++) {
	    for(jx = 0; jx < nx; jx++) {
	      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	      //	      z[jk+js*s]=z[jk+js*s]+x[j+m*jk]*y[j+m*js];
	      tmp[jk+js*s]=tmp[jk+js*s]+x[j+m*jk]*y[j+m*js];
	    }
	  }
	}
      }
    }
    //#pragma omp critical
    {
      for(js = 0; js < s; js++) {
	for(jk = js; jk < s; jk++) {
#pragma omp atomic
	  z[jk+js*s]=z[jk+js*s]+tmp[jk+js*s];
	}
      }
    }

  }

  for(js = 0; js < s; js++) {
    for(jk = js; jk < s; jk++) {
      z[js+jk*s]=z[jk+js*s];
    }
  }

  return 0;
}


// y[] = A[][]x[]
// dot = (A[][]x[],x)
int MatVec_dot(mpi_prm prm,type* A,type* x,type* y,type* tmp){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;
  type tmp1_,tmp1;
  tmp1_=(double)0.0;

#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) reduction(+:tmp1_) schedule(static,1)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	jce = j+1;
	jcn = j+mx;
	jct = j+mxy;
	y[j]=
	   A[j+0*m]*x[jcb] 
	  +A[j+1*m]*x[jcs] 
	  +A[j+2*m]*x[jcw] 
	  +A[j+3*m]*x[jcc] 
	  +A[j+4*m]*x[jce] 
	  +A[j+5*m]*x[jcn] 
	  +A[j+6*m]*x[jct] 
	  ;


	tmp1_=tmp1_+y[j]*x[j];
      }
    }
  }

  tmp1=(double)0.0;
#ifdef JUPITER_MPI
  MPI_Allreduce(&tmp1_, &tmp1, 1,MPI_TYPE, MPI_SUM,prm.comm);
#else
  tmp1=tmp1_;
#endif
  tmp[0]=tmp1;

  return 0;

}

int sMatVec_dot(mpi_prm prm,type* A,type* x,type* y,type* tmp){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;
  type tmp1_,tmp1;
  tmp1_=(double)0.0;

#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) reduction(+:tmp1_) schedule(static,1)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	jce = j+1;
	jcn = j+mx;
	jct = j+mxy;

#if 0
	y[j]=
	   A[j+0*m]*x[jcb] 
	  +A[j+1*m]*x[jcs] 
	  +A[j+2*m]*x[jcw] 
	  +A[j+3*m]*x[jcc] 
	  +A[j+4*m]*x[jce] 
	  +A[j+5*m]*x[jcn] 
	  +A[j+6*m]*x[jct] 
	  ;

#else
	y[j]=
	   A[j   + 0*m]*x[jcb] 
	  +A[j   + 1*m]*x[jcs] 
	  +A[j   + 2*m]*x[jcw] 
	  +A[j   + 3*m]*x[jcc] 
	  +A[jce + 2*m]*x[jce] 
	  +A[jcn + 1*m]*x[jcn] 
	  +A[jct + 0*m]*x[jct] 
	  ;

#endif

	tmp1_=tmp1_+y[j]*x[j];
      }
    }
  }

  tmp1=(double)0.0;
#ifdef JUPITER_MPI
  MPI_Allreduce(&tmp1_, &tmp1, 1,MPI_TYPE, MPI_SUM,prm.comm);
#else
  tmp1=tmp1_;
#endif
  tmp[0]=tmp1;

  return 0;

}

int make_sMat(mpi_prm prm,type* A){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;
  type tmp1_,tmp1;

#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) schedule(static,1)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	jce = j+1;
	jcn = j+mx;
	jct = j+mxy;

#if 0
	type Ae = abs( A[jce + 2*m]-A[j+4*m] );
	type An = abs( A[jcn + 1*m]-A[j+5*m] );
	type At = abs( A[jct + 0*m]-A[j+6*m] );
	if(Ae>1.0e-10)printf(" %4d,%4d,%4d : jce -- %f \n",jx,jy,jz,A[jce + 2*m]-A[j+4*m]);	
	if(An>1.0e-10)printf(" %4d,%4d,%4d : jcn -- %f \n",jx,jy,jz,A[jcn + 1*m]-A[j+5*m]);
	if(At>1.0e-10)printf(" %4d,%4d,%4d : jct -- %f \n",jx,jy,jz,A[jct + 0*m]-A[j+6*m]);
#endif

	A[jce + 2*m]=A[j+4*m];
	A[jcn + 1*m]=A[j+5*m];
	A[jct + 0*m]=A[j+6*m];
      }
    }
  }

  return 0;
}


int MatVec_dot_local(mpi_prm prm,type* A,type* x,type* y,type* tmp){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;
  type tmp1_,tmp1;
  tmp1_=(double)0.0;

#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) reduction(+:tmp1_) schedule(static,1)
  //#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) reduction(+:tmp1_) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
 	jcb = j-mxy; 
 	jcs = j-mx; 
 	jcw = j-1; 
 	jcc = j; 
 	jce = j+1; 
 	jcn = j+mx; 
 	jct = j+mxy; 

	y[j]=
	  A[j+0*m]*x[jcb] 
	  +A[j+1*m]*x[jcs] 
	  +A[j+2*m]*x[jcw] 
	  +A[j+3*m]*x[jcc] 
	  +A[j+4*m]*x[jce] 
	  +A[j+5*m]*x[jcn] 
	  +A[j+6*m]*x[jct] 
	  ;
	tmp1_=tmp1_+y[j]*x[j];
      }
    }
  }
  tmp[0]=tmp1_;
  //  exit(0);
  return 0;

}



int sMatVec_dot_local(mpi_prm prm,type* A,type* x,type* y,type* tmp){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;
  type tmp1_,tmp1;
  tmp1_=(double)0.0;

#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) reduction(+:tmp1_) schedule(static,1)
  //#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) reduction(+:tmp1_) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
 	jcb = j-mxy; 
 	jcs = j-mx; 
 	jcw = j-1; 
 	jcc = j; 
 	jce = j+1; 
 	jcn = j+mx; 
 	jct = j+mxy; 
#if 0
	y[j]=
	  A[j+0*m]*x[jcb] 
	  +A[j+1*m]*x[jcs] 
	  +A[j+2*m]*x[jcw] 
	  +A[j+3*m]*x[jcc] 
	  +A[j+4*m]*x[jce] 
	  +A[j+5*m]*x[jcn] 
	  +A[j+6*m]*x[jct] 
	  ;
#else
	y[j]=
	   A[j   + 0*m]*x[jcb] 
	  +A[j   + 1*m]*x[jcs] 
	  +A[j   + 2*m]*x[jcw] 
	  +A[j   + 3*m]*x[jcc] 
	  +A[jce + 2*m]*x[jce] 
	  +A[jcn + 1*m]*x[jcn] 
	  +A[jct + 0*m]*x[jct] 
	  ;

#endif
	tmp1_=tmp1_+y[j]*x[j];
      }
    }
  }
  tmp[0]=tmp1_;
  //  exit(0);
  return 0;

}


int MatVec_dot_local_dcomm2(mpi_prm* prm,type* A,type* x,type* y,type* tmp){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(*prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;
  type tmp1_,tmp1;
  tmp1_=(double)0.0;

#pragma omp parallel 
  {
#pragma omp master
    {
#ifdef JUPITER_MPI
      sleev_comm_yz(x,prm);
#endif
    }

    // #pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) reduction(+:tmp1_) 
#pragma omp  for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) reduction(+:tmp1_)  schedule(dynamic,1) 
  for(jz = 0+1; jz < nz-1; jz++) {
    for(jy = 0+1; jy < ny-1; jy++) {
      for(jx = 0; jx < nx; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
 	jcb = j-mxy; 
 	jcs = j-mx; 
 	jcw = j-1; 
 	jcc = j; 
 	jce = j+1; 
 	jcn = j+mx; 
 	jct = j+mxy; 

	y[j]=
	  A[j+0*m]*x[jcb] 
	  +A[j+1*m]*x[jcs] 
	  +A[j+2*m]*x[jcw] 
	  +A[j+3*m]*x[jcc] 
	  +A[j+4*m]*x[jce] 
	  +A[j+5*m]*x[jcn] 
	  +A[j+6*m]*x[jct] 
	  ;
	tmp1_=tmp1_+y[j]*x[j];
      }
    }
  }

#pragma  omp barrier
#pragma  omp flush(x)

  //    for(jz = 0+1; jz < nz-1; jz++) {
  jz=0;
#pragma omp for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jy,jx) reduction(+:tmp1_) schedule(dynamic,1) 
  for(jy = 0; jy < ny; jy++) {
    for(jx = 0; jx < nx; jx++) {
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcb = j-mxy; 
      jcs = j-mx; 
      jcw = j-1; 
      jcc = j; 
      jce = j+1; 
      jcn = j+mx; 
      jct = j+mxy; 

      y[j]=
	A[j+0*m]*x[jcb] 
	+A[j+1*m]*x[jcs] 
	+A[j+2*m]*x[jcw] 
	+A[j+3*m]*x[jcc] 
	+A[j+4*m]*x[jce] 
	+A[j+5*m]*x[jcn] 
	+A[j+6*m]*x[jct] 
	;
      tmp1_=tmp1_+y[j]*x[j];

    }
  }
  
#pragma omp for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) reduction(+:tmp1_) schedule(dynamic,1) 
  for(jz = 0+1; jz < nz-1; jz++) {
    jy = 0;
    for(jx = 0; jx < nx; jx++) {
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcb = j-mxy; 
      jcs = j-mx; 
      jcw = j-1; 
      jcc = j; 
      jce = j+1; 
      jcn = j+mx; 
      jct = j+mxy; 

      y[j]=
	A[j+0*m]*x[jcb] 
	+A[j+1*m]*x[jcs] 
	+A[j+2*m]*x[jcw] 
	+A[j+3*m]*x[jcc] 
	+A[j+4*m]*x[jce] 
	+A[j+5*m]*x[jcn] 
	+A[j+6*m]*x[jct] 
	;
      tmp1_=tmp1_+y[j]*x[j];

    }
    jy= ny-1 ;
    for(jx = 0; jx < nx; jx++) {
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcb = j-mxy; 
      jcs = j-mx; 
      jcw = j-1; 
      jcc = j; 
      jce = j+1; 
      jcn = j+mx; 
      jct = j+mxy; 

      y[j]=
	A[j+0*m]*x[jcb] 
	+A[j+1*m]*x[jcs] 
	+A[j+2*m]*x[jcw] 
	+A[j+3*m]*x[jcc] 
	+A[j+4*m]*x[jce] 
	+A[j+5*m]*x[jcn] 
	+A[j+6*m]*x[jct] 
	;
      tmp1_=tmp1_+y[j]*x[j];

    }
  }
  jz=nz-1;
#pragma omp for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jy,jx) reduction(+:tmp1_) schedule(dynamic,1) 
  for(jy = 0; jy < ny; jy++) {
    for(jx = 0; jx < nx; jx++) {
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcb = j-mxy; 
      jcs = j-mx; 
      jcw = j-1; 
      jcc = j; 
      jce = j+1; 
      jcn = j+mx; 
      jct = j+mxy; 

      y[j]=
	A[j+0*m]*x[jcb] 
	+A[j+1*m]*x[jcs] 
	+A[j+2*m]*x[jcw] 
	+A[j+3*m]*x[jcc] 
	+A[j+4*m]*x[jce] 
	+A[j+5*m]*x[jcn] 
	+A[j+6*m]*x[jct] 
	;
      tmp1_=tmp1_+y[j]*x[j];

    }
  }

  }
      
  tmp[0]=tmp1_;

  return 0;

}

//   y[] = y[] + a*x[] 
int axpy(mpi_prm prm,type a,type* x,type* y){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

#pragma omp parallel for private(j,jz,jy,jx) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	y[j]=a*x[j]+y[j];
      }
    }
  }
  return 0;
}

int Halo2noHalo(mpi_prm prm,type* x,type* y){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j,j_;
  int jz,jx,jy;

#pragma omp parallel for private(j,j_,jz,jy,jx) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
        j_ = jx + nx*jy + nx*ny*jz;
	y[j_]=x[j];
      }
    }
  }
  return 0;
}

//   z[] = b*y[] + a*x[] 
int axpy_inp(mpi_prm prm,type a,type b,type* x,type* y,type* z){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

#pragma omp parallel for private(j,jz,jy,jx) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	z[j]=b*y[j]+a*x[j];
      }
    }
  }
  return 0;
}


//   z1[] = b*z1[] +   x1[] 
//   z2[] =   z2[] + a*z1[] 
int axpy2_inp(mpi_prm prm,
	      type a,type b,
	      type* x1,type* z1,type* z2
	      ){

  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int max_threads;
  //  set_max_threads(nz,&max_threads);
#pragma omp parallel for private(j,jz,jy,jx) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      /*
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	z1[j]=b*z1[j]+x1[j];
	z2[j]=  z2[j]+a*z1[j];
      }
      */
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	z1[j]=b*z1[j]+x1[j];
      }
    }
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	z2[j]=  z2[j]+a*z1[j];
      }
    }
  }
  //  set_omp_threads(max_threads);

  return 0;
}

//   z[] = z[] + y[0:(s-1)][]*x[] 
int gemv(mpi_prm prm,type* x,type* y,type* z,int s){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy,js;
  type a;

#pragma omp parallel for private(j,jz,jy,jx,js,a) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(js = 0; js < s; js++) {
	a=x[js];
	for(jx = 0; jx < nx; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  z[j]=a*y[j+m*js]+z[j];
	}
      }
    }
  }
  return 0;
}


//   z[] = y[0:(s-1)][]*x[] 
int gemv_inplace(mpi_prm prm,type* x,type* y,type* z,int s){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy,js;
  type a;
  for(js = 0; js < s; js++) {
    //    printf("debug %i %f\n",js,x[js]);
  }
#pragma omp parallel for private(j,jz,jy,jx,js,a) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {

      for(jx = 0; jx < nx; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	z[j]=0.0;
      }
      for(js = 0; js < s; js++) {
	a=x[js];
	for(jx = 0; jx < nx; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  z[j]=z[j]+a*y[j+m*js];
	}
      }

    }
  }
  return 0;
}

// ----
//   z[] = z[] + y[0:(s-1)][]*x[] 
//   tmp=<z|z>
int gemv_norm(mpi_prm prm,type* x,type* y,type* z,type* tmp,int s){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy,js;
  type a;
  type tmp_;
  tmp_=(double)0.0;

#pragma omp parallel for private(j,jz,jy,js,jx,a) reduction(+:tmp_)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {

      for(js = 0; js < s; js++) {
	a=x[js];
	for(jx = 0; jx < nx; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  z[j]=a*y[j+m*js]+z[j];
	}
      }
      for(jx = 0; jx < nx; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	tmp_=tmp_+z[j]*z[j];
      }

    }
  }
#ifdef JUPITER_MPI
  MPI_Allreduce(&tmp_,tmp,1,MPI_TYPE, MPI_SUM,prm.comm);
#else
  tmp[0]=tmp_;
#endif
  tmp[0]=sqrt(tmp[0]);

  return 0;
}

// ----
//   z[] = z[] + y[0:(s-1)][]*x[] 
//   tmp=<z|z>
int gemv_dot_local(mpi_prm prm,type* x,type* y,type* z,type* tmp,int s){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy,js,jxy;
  type a;
  type tmp_;
  tmp_=(double)0.0;

#ifdef funroll

#ifdef opts1
#define cbcg_unroll 1
#endif
#ifdef opts2
#define cbcg_unroll 2
#endif
#ifdef opts3
#define cbcg_unroll 3
#endif
#ifdef opts4
#define cbcg_unroll 4
#endif
#ifdef opts5
#define cbcg_unroll 5
#endif

#ifdef opts6
#define cbcg_unroll 6
#endif

#ifdef opts7
#define cbcg_unroll 7
#endif

#ifdef opts8
#define cbcg_unroll 8
#endif

#ifdef opts9
#define cbcg_unroll 9
#endif

#ifdef opts10
#define cbcg_unroll 10
#endif

#ifdef opts11
#define cbcg_unroll 11
#endif

#ifdef opts12
#define cbcg_unroll 12
#endif

#ifndef cbcg_unroll
#define cbcg_unroll 0
#endif

#endif

#pragma omp parallel for private(j,jz,jy,jx,a,jxy,js) reduction(+:tmp_) // collapse(2)
  for(jz = 0; jz < nz; jz++) {

#ifdef funroll
    for(jxy = 0; jxy < mx*ny; jxy++) {
        j = jxy + mxy*(jz+stm) + mx*stm;
    // -----
    //        for(jy = 0; jy < ny; jy++) {    
    //        for(jx = 0; jx < nx; jx++) {    
    //    	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
    z[j]=z[j]+	
#if cbcg_unroll > 11
      x[11]*y[j+m*11]+
#endif
#if cbcg_unroll > 10
      x[10]*y[j+m*10]+
#endif
#if cbcg_unroll > 9
      x[9]*y[j+m*9]+
#endif
#if cbcg_unroll > 8
      x[8]*y[j+m*8]+
#endif
#if cbcg_unroll > 7
      x[7]*y[j+m*7]+
#endif
#if cbcg_unroll > 6
      x[6]*y[j+m*6]+
#endif
#if cbcg_unroll > 5
      x[5]*y[j+m*5]+
#endif
#if cbcg_unroll > 4
      x[4]*y[j+m*4]+
#endif
#if cbcg_unroll > 3
      x[3]*y[j+m*3]+
#endif
#if cbcg_unroll > 2
      x[2]*y[j+m*2]+
#endif
#if cbcg_unroll > 1
      x[1]*y[j+m*1]+
#endif
#if cbcg_unroll > 0
      x[0]*y[j+m*0];
#else
      0.0;
      for(js = 0; js < s; js=js+1) {
	j = jxy + mxy*(jz+stm) + mx*stm;
	z[j]=x[js]*y[j+m*js]+z[j];
      }
#endif


      tmp_=tmp_+z[j]*z[j];
  }
	//  }

#else
    for(jxy = 0; jxy < mx*ny; jxy++) {
      j = jxy + mxy*(jz+stm) + mx*stm;
      for(js = 0; js < s; js=js+1) {
	z[j]=x[js]*y[j+m*js]+z[j];
	//	z[j]=x[0]*y[j+m*0]+z[j];
	//	z[j]=x[1]*y[j+m*1]+z[j];
      }
      /*    }
    for(jxy = 0; jxy < mx*ny; jxy++) {
    j = jxy + mxy*(jz+stm) + mx*stm;*/
      tmp_=tmp_+z[j]*z[j];
    }
#endif
  }

  tmp[0]=tmp_;

#undef cbcg_unroll
  return 0;
}



// -----
// y[s][]=x[s][]*z[s][s]
int gemm(mpi_prm prm,type* x,type* y,type* z,int s){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy,js,jk,jl;
  type tmp[s*nx];

#pragma omp parallel for private(j,jz,jy,jx,js,jk,tmp)  collapse(2)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      
      for(jk = 0; jk < s; jk++) {
	js=0;
	for(jx = 0; jx < nx; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  //	  tmp[jx+jk*nx]=x[j+m*js]*z[jk+js*s];
	  tmp[jx+jk*nx]=x[j+m*js]*z[js+jk*s];
	}
	for(js = 1; js < s; js++) {
	  for(jx = 0; jx < nx; jx++) {
	    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    //	    tmp[jx+jk*nx]=tmp[jx+jk*nx]+x[j+m*js]*z[jk+js*s];
	    tmp[jx+jk*nx]=tmp[jx+jk*nx]+x[j+m*js]*z[js+jk*s];
	  }
	}
      }

      for(jk = 0; jk < s; jk++) {
	for(jx = 0; jx < nx; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  //	  x[j+m*jk]=y[j+m*jk]+tmp[jx+jk*nx];
	  y[j+m*jk]=tmp[jx+jk*nx];
	}
      }
	
   }
  }

  return 0;
}


// -----
// x[s][]=y[s][]+x[s][]*z[s][s]
// v[s][s]=x[s][]*w[s][s] // 上三角のみ計算
int gemm_syrk(mpi_prm prm,type* x,type* y,type* z,type* w,type* v,int s){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy,js,jk,jl;
  type tmp[s*nx];
  type tmp_syrk[s*s];

  for(js = 0; js < s; js++) {
    for(jk = 0; jk < s; jk++) {
      v[jk+js*s]=(double)0.0;
    }
  }

#pragma omp parallel private(j,jz,jy,jx,js,jk,tmp,tmp_syrk) 
  {
    for(js = 0; js < s; js++) {
      for(jk = 0; jk < s; jk++) {
	tmp_syrk[jk+js*s]=(double)0.0;
      }
    }

#pragma omp for nowait
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {

      for(jk = 0; jk < s; jk++) {
	js=0;
	for(jx = 0; jx < nx; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  tmp[jx+jk*nx]=x[j+m*js]*z[jk+js*s];
	}
	for(js = 1; js < s; js++) {
	  for(jx = 0; jx < nx; jx++) {
	    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    tmp[jx+jk*nx]=tmp[jx+jk*nx]+x[j+m*js]*z[jk+js*s];
	  }
	}
      }

      for(jk = 0; jk < s; jk++) {
	for(jx = 0; jx < nx; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  x[j+m*jk]=y[j+m*jk]+tmp[jx+jk*nx];
	}

	for(js = jk; js < s; js++) {
	  for(jx = 0; jx < nx; jx++) {
	    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    tmp_syrk[js+jk*s]=tmp_syrk[js+jk*s]+x[j+m*jk]*w[j+m*js];	
	  }
	}

      }

    }
  }

  //#pragma omp critical
    {
      for(jk = 0; jk < s; jk++) {
	for(js = jk; js < s; js++) {
#pragma omp atomic
	  	  v[js+jk*s]=v[js+jk*s]+tmp_syrk[js+jk*s];
	}
      }
    }
  }

  for(jk = 0; jk < s; jk++) {
    for(js = jk; js < s; js++) {
      v[jk+js*s]=v[js+jk*s];
    }
  }

  return 0;
}

// -----
// z[s][s]=x[s][]*y[s][]
int gemm_dot(mpi_prm prm,type* x,type* y,type* z,int s){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy,js,jk,jl;
  type tmp[s*s];
  type z_[s*s];

  for(jk = 0; jk < s; jk++) {
    for(js = 0; js < s; js++) {
      z_[js+jk*s]=(double)0.0;
    }
  }

#pragma omp parallel private(j,jz,jy,jx,js,jk,tmp) 
  {

    for(jk = 0; jk < s; jk++) {
      for(js = 0; js < s; js++) {
	tmp[js+jk*s]=(double)0.0;
      }
    }
    if(nx%2==0){
#pragma omp  for nowait
      for(jz = 0; jz < nz; jz++) {
	for(jy = 0; jy < ny; jy++) {
	  for(js = 0; js < s; js++) {
	    for(jk = 0; jk < s; jk++) {
	      for(jx = 0; jx < nx; jx=jx+2) {
		j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
		tmp[js+jk*s]=tmp[js+jk*s]+x[j+m*jk]*y[j+m*js]; 
		tmp[js+jk*s]=tmp[js+jk*s]+x[j+1+m*jk]*y[j+1+m*js];
	      }
	    }
	  }
	}
      }
    }else{
#pragma omp  for nowait
      for(jz = 0; jz < nz; jz++) {
	for(jy = 0; jy < ny; jy++) {
	  for(js = 0; js < s; js++) {
	    for(jk = 0; jk < s; jk++) {
	      for(jx = 0; jx < nx; jx++) {
		j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
		tmp[js+jk*s]=tmp[js+jk*s]+x[j+m*jk]*y[j+m*js]; 
		//		tmp[js+jk*s]=tmp[js+jk*s]+x[j+1+m*jk]*y[j+1+m*js];
	      }
	    }
	  }
	}
      }
    }
    //#pragma omp critical
    {
      for(jk = 0; jk < s; jk++) {
	for(js = 0; js < s; js++) {
#pragma omp atomic
	  z_[js+jk*s]=z_[js+jk*s]+tmp[js+jk*s];
	}
      }
    }
  }

#ifdef JUPITER_MPI
  MPI_Allreduce(z_,z,s*s,MPI_TYPE, MPI_SUM,prm.comm);
#else
  for(js = 0; js < s*s; js++) {
    z[js]=z_[js];
  }
#endif
  return 0;
}

// -----
// z[s][s]=x[s][]*y[s][]
int gemm_dot_local(mpi_prm prm,type* x,type* y,type* z,int s){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy,js,jk,jl,jkk,jss,jxy;
  int jk_e,js_e,jxy_s,jxy_e,jy_block;
  int jk_block,js_block;

  type tmp[s*s];
  type z_[s*s];

  for(jk = 0; jk < s; jk++) {
    for(js = 0; js < s; js++) {
      z_[js+jk*s]=(double)0.0;
    }
  }
  jy_block=mx*2;
  jk_block=2;
  js_block=jk_block;
#pragma omp parallel private(j,jz,jy,jx,js,jk,tmp,jxy,jxy_e,jkk,jss,js_e,jk_e) 
  {

    for(jk = 0; jk < s; jk++) {
      for(js = 0; js < s; js++) {
	tmp[js+jk*s]=(double)0.0;
      }
    }

    if((s%2)==1){
#pragma omp  for nowait  collapse(2)
      for(jz = 0; jz < nz; jz++) {
	//	for(jy = 0; jy < ny; jy++) {
	for(jy = 0; jy < mx*ny; jy=jy+jy_block) {
	  if((jy+jy_block)<mx*ny){
	    jxy_e=jy+jy_block;
	  }else{
	    jxy_e=mx*ny;
	  }

	  jk = 0; 
	  for(js = 0; js < s; js++) {
	    //	    for(jx = 0; jx < nx; jx++) {
	    //	      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    for(jxy = jy; jxy < jxy_e; jxy++) {
	      j = jxy + mxy*(jz+stm) + mx*stm;
	      tmp[js+jk*s]=tmp[js+jk*s]+x[j+m*jk]*y[j+m*js]; 
	    }
	  }
	  for(jk = 1; jk < s; jk=jk+2) {
	    js=0;
	    //	    for(jx = 0; jx < nx; jx++) {
	    //	      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    for(jxy = jy; jxy < jxy_e; jxy++) {
	      j = jxy + mxy*(jz+stm) + mx*stm;
	      tmp[js+jk*s]=tmp[js+jk*s]+x[j+m*jk]*y[j+m*js]; 
	      tmp[js+(jk+1)*s]=tmp[js+(jk+1)*s]+x[j+m*(jk+1)]*y[j+m*js]; 
	    }
	    for(js = 1; js < s; js=js+2) {
	      //	      for(jx = 0; jx < nx; jx++) {
	      //		j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	      for(jxy = jy; jxy < jxy_e; jxy++) {
		j = jxy + mxy*(jz+stm) + mx*stm;
		tmp[js+jk*s]=tmp[js+jk*s]+x[j+m*jk]*y[j+m*js]; 
		tmp[(js+1)+jk*s]=tmp[(js+1)+jk*s]+x[j+m*jk]*y[j+m*(js+1)]; 
		tmp[js+(jk+1)*s]=tmp[js+(jk+1)*s]+x[j+m*(jk+1)]*y[j+m*js]; 
		tmp[(js+1)+(jk+1)*s]=tmp[(js+1)+(jk+1)*s]+x[j+m*(jk+1)]*y[j+m*(js+1)]; 
	      }
	    }
	  }
	}
      }
  }else{
#pragma omp  for nowait collapse(2)
      for(jz = 0; jz < nz; jz++) {
	/*
	for(jy = 0; jy < ny; jy++) {
	  for(jk = 0; jk < s; jk=jk+2) {
	    for(js = 0; js < s; js=js+2) {
	      for(jx = 0; jx < nx; jx++) {
		j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
		tmp[js+jk*s]=tmp[js+jk*s]+x[j+m*jk]*y[j+m*js]; 
		tmp[(js+1)+jk*s]=tmp[(js+1)+jk*s]+x[j+m*jk]*y[j+m*(js+1)]; 
		tmp[js+(jk+1)*s]=tmp[js+(jk+1)*s]+x[j+m*(jk+1)]*y[j+m*js]; 
		tmp[(js+1)+(jk+1)*s]=tmp[(js+1)+(jk+1)*s]+x[j+m*(jk+1)]*y[j+m*(js+1)]; 
	      }
	    }
	  }
	}
	*/
	
	for(jy = 0; jy < mx*ny; jy=jy+jy_block) {
	  if((jy+jy_block)<mx*ny){
	    jxy_e=jy+jy_block;
	  }else{
	    jxy_e=mx*ny;
	  }
	  for(jk = 0; jk < s; jk=jk+2) {
	    for(js = 0; js < s; js=js+2) {	      
	      for(jxy = jy; jxy < jxy_e; jxy++) {
		j = jxy + mxy*(jz+stm) + mx*stm;
		tmp[js+jk*s]=tmp[js+jk*s]+x[j+m*jk]*y[j+m*js]; 
		tmp[(js+1)+jk*s]=tmp[(js+1)+jk*s]+x[j+m*jk]*y[j+m*(js+1)]; 
		tmp[js+(jk+1)*s]=tmp[js+(jk+1)*s]+x[j+m*(jk+1)]*y[j+m*js]; 
		tmp[(js+1)+(jk+1)*s]=tmp[(js+1)+(jk+1)*s]+x[j+m*(jk+1)]*y[j+m*(js+1)]; 
	      }
	    }
	  }
	  // -----

    /*
  for(jkk = 0; jkk < s; jkk=jkk+jk_block) {
    if((jkk+jk_block)<s){
      jk_e=jkk+jk_block;
    }else{
      jk_e=s;
    }   
    for(jss = 0; jss < s; jss=jss+js_block) {
      if((jss+js_block)<s){
	js_e=jss+js_block;
      }else{
	js_e=s;
      }

      for(jk = jkk; jk < jk_e; jk=jk+1) {
	for(js = jss; js < js_e; js=js+1) {
	  for(jxy = jy; jxy < jxy_e; jxy++) {
	    j = jxy + mxy*(jz+stm) + mx*stm;
	    tmp[js+jk*s]=tmp[js+jk*s]+x[j+m*jk]*y[j+m*js]; 
	  }
	}
      }
    }
  
    if((jkk+jk_block)<=s){
      jk_e=jkk+jk_block;
      for(jss = 0; jss < s; jss=jss+js_block) {
	if((jss+js_block)<=s){
	  js_e=jss+js_block;
	  //	  	  for(jk = jkk; jk < jk_e; jk=jk+1) {
	  //	    	    for(js = jss; js < js_e; js=js+1) {
	  for(jxy = jy; jxy < jxy_e; jxy++) {
	    j = jxy + mxy*(jz+stm) + mx*stm;
	    //		tmp[js+jk*s]=tmp[js+jk*s]+x[j+m*jk]*y[j+m*js]; 
	    tmp[jss+jkk*s]=tmp[jss+jkk*s]+x[j+m*jkk]*y[j+m*jss]; 
	    tmp[(jss+1)+jkk*s]=tmp[(jss+1)+jkk*s]+x[j+m*jkk]*y[j+m*(jss+1)]; 

	    tmp[jss+(jkk+1)*s]=tmp[jss+(jkk+1)*s]+x[j+m*(jkk+1)]*y[j+m*jss]; 
	    tmp[(jss+1)+(jkk+1)*s]=tmp[(jss+1)+(jkk+1)*s]+x[j+m*(jkk+1)]*y[j+m*(jss+1)]; 
	  }
	}else{
	  js_e=s;
	  //	  for(jk = jkk; jk < jk_e; jk=jk+2) {
	    for(js = jss; js < js_e; js=js+1) {
	      for(jxy = jy; jxy < jxy_e; jxy++) {
		j = jxy + mxy*(jz+stm) + mx*stm;
		//		tmp[js+jk*s]=tmp[js+jk*s]+x[j+m*jk]*y[j+m*js]; 
		//		tmp[js+jkk*s]=tmp[js+jkk*s]+x[j+m*jkk]*y[j+m*js]; 
		tmp[js+(jkk+0)*s]=tmp[js+(jkk+0)*s]+x[j+m*(jkk+0)]*y[j+m*js]; 
		tmp[js+(jkk+1)*s]=tmp[js+(jkk+1)*s]+x[j+m*(jkk+1)]*y[j+m*js]; 
	      }
	    }
	    //	  }
	}
      }
    }else{
      jk_e=s;
      for(jss = 0; jss < s; jss=jss+js_block) {
	if((jss+js_block)<=s){
	  js_e=jss+js_block;
	  for(jk = jkk; jk < jk_e; jk=jk+1) {
	    //	    for(js = jss; js < js_e; js=js+2) {
	      for(jxy = jy; jxy < jxy_e; jxy++) {
		j = jxy + mxy*(jz+stm) + mx*stm;
		//		tmp[js+jk*s]=tmp[js+jk*s]+x[j+m*jk]*y[j+m*js]; 
		tmp[(jss+0)+jk*s]=tmp[(jss+0)+jk*s]+x[j+m*jk]*y[j+m*(jss+0)]; 
		tmp[(jss+1)+jk*s]=tmp[(jss+1)+jk*s]+x[j+m*jk]*y[j+m*(jss+1)]; 
	      }
	      //	    }
	  }
	  
	}else{
	  js_e=s;
	  for(jk = jkk; jk < jk_e; jk=jk+1) {
	    for(js = jss; js < js_e; js=js+1) {
	      for(jxy = jy; jxy < jxy_e; jxy++) {
		j = jxy + mxy*(jz+stm) + mx*stm;
		tmp[js+jk*s]=tmp[js+jk*s]+x[j+m*jk]*y[j+m*js]; 
	      }
	    }
	  }	  
	}
      }   
    }
    
 
  }
    */

	}
	
      }
    }
    /*
    if(nx%2==0){
#pragma omp  for nowait
      for(jz = 0; jz < nz; jz++) {
	for(jy = 0; jy < ny; jy++) {
	  for(js = 0; js < s; js++) {
	    for(jk = 0; jk < s; jk++) {
	      for(jx = 0; jx < nx; jx=jx+2) {
		j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
		tmp[js+jk*s]=tmp[js+jk*s]+x[j+m*jk]*y[j+m*js]; 
		tmp[js+jk*s]=tmp[js+jk*s]+x[j+1+m*jk]*y[j+1+m*js];
	      }
	    }
	  }
	}
      }
    }else{
#pragma omp  for nowait
      for(jz = 0; jz < nz; jz++) {
	for(jy = 0; jy < ny; jy++) {
	  for(js = 0; js < s; js++) {
	    for(jk = 0; jk < s; jk++) {
	      for(jx = 0; jx < nx; jx++) {
		j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
		tmp[js+jk*s]=tmp[js+jk*s]+x[j+m*jk]*y[j+m*js]; 
		//		tmp[js+jk*s]=tmp[js+jk*s]+x[j+1+m*jk]*y[j+1+m*js];
	      }
	    }
	  }
	}
      }
    }
*/
//#pragma omp critical
    {
      for(jk = 0; jk < s; jk++) {
	for(js = 0; js < s; js++) {
#pragma omp atomic
	  z_[js+jk*s]=z_[js+jk*s]+tmp[js+jk*s];
	}
      }
    }
  }
  for(jk = 0; jk < s; jk++) {
    for(js = 0; js < s; js++) {
      z[js+jk*s]=z_[js+jk*s];
    }
  }
  //  MPI_Allreduce(z_,z,s*s,MPI_TYPE, MPI_SUM,prm.comm);

  return 0;
}


// -----
// v[]=v[]+x[s][]*u[s]
// x[s][]=y[s][]+x[s][]*z[s][s]
// x_w[s]=(x[s][],w)
int gemm2_dot_local(mpi_prm prm,type* x,type* y,type* z,
		    type* w,type* x_w,
		    type* v,type* u,
		    int s){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy,js,jk,jl;
  type tmp[s*nx];
  type x_w_tmp[s];

  for(jk = 0; jk < s; jk++) {
    x_w[jk]=(double)0.0;
  }
#pragma omp parallel private(j,jz,jy,jx,js,jk,tmp,x_w_tmp) 
  {
    for(jk = 0; jk < s; jk++){
      x_w_tmp[jk]=(double)0.0;
    }
#pragma omp  for nowait
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {

	for(jk = 0; jk < s; jk++) {
	  js=0;
	  for(jx = 0; jx < nx; jx++) {	  
	    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    tmp[jx+jk*nx]=x[j+m*js]*z[jk+js*s];
	  }
	  for(js = 1; js < s; js++) {
	    for(jx = 0; jx < nx; jx++) {	  
	      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	      tmp[jx+jk*nx]=tmp[jx+jk*nx]+x[j+m*js]*z[jk+js*s];
	    }
	  }

	}
	for(jk = 0; jk < s; jk++) {
	  for(jx = 0; jx < nx; jx++) {	  
	    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    v[j]=v[j]+x[j+m*jk]*u[jk];
	    x[j+m*jk]=y[j+m*jk]+tmp[jx+jk*nx];
	    x_w_tmp[jk]=x_w_tmp[jk]+x[j+m*jk]*w[j];
	  }
	}

      }
    }

    //#pragma omp critical
    {
      for(jk = 0; jk < s; jk++) {
#pragma omp atomic
	x_w[jk]=x_w[jk]+x_w_tmp[jk];
      }
    }

  }
  return 0;
}

void swap_ptr(type **x,type **y){
  type *tmp;
  tmp=*y;
  *y=*x;
  *x=tmp;
}
// -----
// v[]=v[]+x[s][]*u[s]
// x[s][]=y[s][]+x[s][]*z[s][s]
// x_w[s]=(x[s][],w)
int gemm2_dot_local_3_(mpi_prm prm,
		      type* x,type* y,type* z,
		      type* w,type* x_w,
		      type* v,type* u,
		      type* t,type* o,type* x_t,
		      int s){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy,js,jk,jl,iss;
  type tmp[s*nx],tmp2[s*nx],tmp3[s*nx];
  type x_w_tmp[s];
  type tmp_syrk[s*s];
  int jss,jkk;
  int kblock,sblock;

  kblock=4;
  sblock=4;
  for(jk = 0; jk < s; jk++) {
    x_w[jk]=(double)0.0;
  }
  for(js = 0; js < s; js++) {
    for(jk = 0; jk < s; jk++) {
      x_t[jk+js*s]=(double)0.0;
    }
  }
  iss=0;
  if(s%2!=0){
    iss=1;
  }

#pragma omp parallel private(j,jz,jy,jx,js,jk,tmp_syrk,x_w_tmp,jss,jkk)
  {

    for(jk = 0; jk < s; jk++){
      x_w_tmp[jk]=(double)0.0;
    }
    for(js = 0; js < s; js++) {
      for(jk = 0; jk < s; jk++) {
	tmp_syrk[jk+js*s]=(double)0.0;
      }
    }

#pragma omp  for nowait
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {

	// -----	
	//
	/*
	for(jkk = 0;jkk<s;jkk=jkk+kblock){
	  for(jss = 0;jss<s;jss=jss+sblock){
	    for(jk = jkk; jk < jkk+kblock; jk++) {
	      for(js = jss; js < jss+sblock; js++) {
		for(jx = 0; jx < nx; jx++) {	  
		  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
		  y[j+m*jk]=y[j+m*jk]+x[j+m*js]*z[jk+js*s];
		  o[j+m*jk]=o[j+m*jk]+t[j+m*js]*z[jk+js*s];
		}
	      }
	    }
	  }
	}
	*/
	
	for(jk = 0;jk<s;jk=jk+1){
	  for(js = 0;js<s;js=js+1){
	    for(jx = 0; jx < nx; jx++) {	  
	      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	      y[j+m*jk]=y[j+m*jk]+x[j+m*js]*z[jk+js*s];
	      o[j+m*jk]=o[j+m*jk]+t[j+m*js]*z[jk+js*s];
	    }
	  }
	}
	for(jk = 0;jk<s;jk=jk+1){
	  for(jx = 0; jx < nx; jx++) {	  
	    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    v[j]=v[j]+x[j+m*jk]*u[jk]; 
	    x_w_tmp[jk]=x_w_tmp[jk]+y[j+m*jk]*w[j];
	  }
	}      

	//	for(jk = s-1; jk > -1; jk--) {	  
	jkk=0;
	if(s%2==1){
	  jk = jkk;
	  for(js = jk; js < s; js=js+1) {  
	    for(jx = 0; jx < nx; jx++) {
	      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	      tmp_syrk[js+jk*s]=tmp_syrk[js+jk*s]+o[j+m*jk]*y[j+m*js];	
	    }
	  }
	  jkk=jkk+1;
	}

	for(jk = jkk; jk < s; jk=jk+2) {  
	  js = jk; 
	  for(jx = 0; jx < nx; jx++) {
	    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    tmp_syrk[js+jk*s]=tmp_syrk[js+jk*s]+o[j+m*jk]*y[j+m*js];	
	  }
	  for(js = (jk+1); js < s; js=js+1) {  
	    for(jx = 0; jx < nx; jx++) {
	      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	      tmp_syrk[js+(jk  )*s]=tmp_syrk[js+(jk  )*s]+o[j+m*(jk  )]*y[j+m*js];	
	      tmp_syrk[js+(jk+1)*s]=tmp_syrk[js+(jk+1)*s]+o[j+m*(jk+1)]*y[j+m*js];	
	    }
	  }
	}

      }
    }
    //#pragma omp critical
    {
      for(jk = 0; jk < s; jk++) {
#pragma omp atomic
	x_w[jk]=x_w[jk]+x_w_tmp[jk];
      }
    }
    //#pragma omp critical
    {
      for(jk = 0; jk < s; jk++) {
	for(js = jk; js < s; js++) {
#pragma omp atomic
	  x_t[js+jk*s]=x_t[js+jk*s]+tmp_syrk[js+jk*s];
	}
      }
    }
  }

  for(jk = 0; jk < s; jk++) {
    for(js = jk; js < s; js++) {
      x_t[jk+js*s]=x_t[js+jk*s];
    }
  }

  return 0;
}

int gemm2_dot_local_3(mpi_prm prm,
		      type* x,type* y,type* z,
		      type* w,type* x_w,
		      type* v,type* u,
		      type* t,type* o,type* x_t,
		      int s){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy,js,jk,jl,iss;
  type tmp[s*nx],tmp2[s*nx],tmp3[s*nx];
  type x_w_tmp[s];
  type tmp_syrk[s*s];
  int jss,jkk;
  int kblock,sblock;
  int jxy,jxy_e;

  for(jk = 0; jk < s; jk++) {
    x_w[jk]=(double)0.0;
  }
  for(js = 0; js < s; js++) {
    for(jk = 0; jk < s; jk++) {
      x_t[jk+js*s]=(double)0.0;
    }
  }
  iss=0;
  if(s%2!=0){
    iss=1;
  }

#ifdef funroll

#ifdef opts1
#define cbcg_unroll 1
#endif
#ifdef opts2
#define cbcg_unroll 2
#endif
#ifdef opts3
#define cbcg_unroll 3
#endif
#ifdef opts4
#define cbcg_unroll 4
#endif
#ifdef opts5
#define cbcg_unroll 5
#endif

#ifdef opts6
#define cbcg_unroll 6
#endif

#ifdef opts7
#define cbcg_unroll 7
#endif

#ifdef opts8
#define cbcg_unroll 8
#endif

#ifdef opts9
#define cbcg_unroll 9
#endif

#ifdef opts10
#define cbcg_unroll 10
#endif

#ifdef opts11
#define cbcg_unroll 11
#endif

#ifdef opts12
#define cbcg_unroll 12
#endif

#ifndef cbcg_unroll
#define cbcg_unroll 0
#endif

#endif

  jkk=mx*2;
#pragma omp parallel private(j,jz,jy,jx,jxy,js,jk,tmp_syrk,x_w_tmp,jss,jxy_e)
  {

    for(jk = 0; jk < s; jk++){
      x_w_tmp[jk]=(double)0.0;
    }
    for(js = 0; js < s; js++) {
      for(jk = 0; jk < s; jk++) {
	tmp_syrk[jk+js*s]=(double)0.0;
      }
    }

#pragma omp  for nowait  // collapse(2)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < mx*ny; jy=jy+jkk) {
	if((jy+jkk)<mx*ny){
	  jxy_e=jy+jkk;
	}else{
	  jxy_e=mx*ny;
	}

	for(jk = 0;jk<s;jk=jk+1){
#ifdef funroll
      //      for(jy = 0; jy < ny; jy++) {
	  //  for(js = 0;js<s;js=js+1){
	  //	  for(jx = 0; jx < nx; jx++) {  
	  //	    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);    
	    for(jxy = jy; jxy < jxy_e; jxy++) {
	      j = jxy + mxy*(jz+stm) + mx*stm;		
	          y[j+m*jk]=y[j+m*jk]+
#if cbcg_unroll > 11
		    x[j+m*11]*z[jk+11*s]+
#endif
#if cbcg_unroll > 10
		    x[j+m*10]*z[jk+10*s]+
#endif
#if cbcg_unroll > 9
		    x[j+m*9]*z[jk+9*s]+
#endif
#if cbcg_unroll > 8
		    x[j+m*8]*z[jk+8*s]+
#endif
#if cbcg_unroll > 7
		    x[j+m*7]*z[jk+7*s]+
#endif
#if cbcg_unroll > 6
		    x[j+m*6]*z[jk+6*s]+
#endif
#if cbcg_unroll > 5
		    x[j+m*5]*z[jk+5*s]+
#endif
#if cbcg_unroll > 4
		    x[j+m*4]*z[jk+4*s]+
#endif
#if cbcg_unroll > 3
		    x[j+m*3]*z[jk+3*s]+
#endif
#if cbcg_unroll > 2
		    x[j+m*2]*z[jk+2*s]+
#endif
#if cbcg_unroll > 1
		    x[j+m*1]*z[jk+1*s]+
#endif
#if cbcg_unroll > 0
		    x[j+m*0]*z[jk+0*s];
#else
      0.0;

      for(js = 0; js < s; js=js+1) {
	y[j+m*jk]=y[j+m*jk]+
	  x[j+m*js]*z[jk+js*s];
      }
#endif
		  
		        o[j+m*jk]=o[j+m*jk]+
#if cbcg_unroll > 11
			  t[j+m*11]*z[jk+11*s]+
#endif
#if cbcg_unroll > 10
			  t[j+m*10]*z[jk+10*s]+
#endif
#if cbcg_unroll > 9 
			  t[j+m*9]*z[jk+9*s]+
#endif
#if cbcg_unroll > 8
			  t[j+m*8]*z[jk+8*s]+
#endif
#if cbcg_unroll > 7
			  t[j+m*7]*z[jk+7*s]+
#endif
#if cbcg_unroll > 6
			  t[j+m*6]*z[jk+6*s]+			 
#endif
#if cbcg_unroll > 5
			  t[j+m*5]*z[jk+5*s]+
#endif
#if cbcg_unroll > 4
			  t[j+m*4]*z[jk+4*s]+
#endif
#if cbcg_unroll > 3
			  t[j+m*3]*z[jk+3*s]+
#endif
#if cbcg_unroll > 2
			  t[j+m*2]*z[jk+2*s]+
#endif
#if cbcg_unroll > 1
			  t[j+m*1]*z[jk+1*s]+
#endif
#if cbcg_unroll > 0
			  t[j+m*0]*z[jk+0*s];
#else
      0.0;

      for(js = 0; js < s; js=js+1) {
	o[j+m*jk]=o[j+m*jk]+
	  t[j+m*js]*z[jk+js*s];
      }
#endif
      v[j]=v[j]+x[j+m*jk]*u[jk]; 
      x_w_tmp[jk]=x_w_tmp[jk]+y[j+m*jk]*w[j];
	  }
	  //  }
	//      }
#else

      //      for(jy = 0; jy < ny; jy++) {
/*       for(jy = 0; jy < mx*ny; jy=jy+jkk) { */
/* 	if((jy+jkk)<mx*ny){ */
/* 	  jxy_e=jy+jkk; */
/* 	}else{ */
/* 	  jxy_e=mx*ny; */
/* 	} */
	  for(js = 0;js<s;js=js+1){
	    //	    for(jx = 0; jx < nx; jx++) {  
	    //	      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);    
	    for(jxy = jy; jxy < jxy_e; jxy++) {
	      j = jxy + mxy*(jz+stm) + mx*stm;		
	      y[j+m*jk]=y[j+m*jk]+
		x[j+m*js]*z[jk+js*s];
	      o[j+m*jk]=o[j+m*jk]+
		t[j+m*js]*z[jk+js*s];
	    }
	  }	      
	  //	  for(jx = 0; jx < nx; jx++) {  
	  //	    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);    
	  for(jxy = jy; jxy < jxy_e; jxy++) {
	    j = jxy + mxy*(jz+stm) + mx*stm;		
	    v[j]=v[j]+x[j+m*jk]*u[jk]; 
	    x_w_tmp[jk]=x_w_tmp[jk]+y[j+m*jk]*w[j];
	  }
#endif
	}

      if((s%2)==1){
/* 	for(jy = 0; jy < mx*ny; jy=jy+jkk) { */
/* 	  if((jy+jkk)<mx*ny){ */
/* 	    jxy_e=jy+jkk; */
/* 	  }else{ */
/* 	    jxy_e=mx*ny; */
/* 	  } */
	  jk = 0; 
	  for(js = 0; js < s; js++) {
	    for(jxy = jy; jxy < jxy_e; jxy++) {
	      j = jxy + mxy*(jz+stm) + mx*stm;	      
	      tmp_syrk[js+jk*s]=tmp_syrk[js+jk*s]+o[j+m*jk]*y[j+m*js];
	    }
	  }
	  for(jk = 1; jk < s; jk=jk+2) {
	    js=0;
	    for(jxy = jy; jxy < jxy_e; jxy++) {
	      j = jxy + mxy*(jz+stm) + mx*stm;
	      tmp_syrk[js+jk*s]=tmp_syrk[js+jk*s]+o[j+m*jk]*y[j+m*js];
	      tmp_syrk[js+(jk+1)*s]=tmp_syrk[js+(jk+1)*s]+o[j+m*(jk+1)]*y[j+m*js];
	    }
	    for(js = 1; js < s; js=js+2) {
	      for(jxy = jy; jxy < jxy_e; jxy++) {
		j = jxy + mxy*(jz+stm) + mx*stm;
		tmp_syrk[js+jk*s]=tmp_syrk[js+jk*s]+o[j+m*jk]*y[j+m*js];
		tmp_syrk[js+(jk+1)*s]=tmp_syrk[js+(jk+1)*s]+o[j+m*(jk+1)]*y[j+m*js];
				
		tmp_syrk[(js+1)+jk*s]=tmp_syrk[(js+1)+jk*s]+o[j+m*jk]*y[j+m*(js+1)];
		tmp_syrk[(js+1)+(jk+1)*s]=tmp_syrk[(js+1)+(jk+1)*s]+o[j+m*(jk+1)]*y[j+m*(js+1)];		
	      }
	    }
	  }
	  
	  //	}
      }else{
/* 	for(jy = 0; jy < mx*ny; jy=jy+jkk) { */
/* 	  if((jy+jkk)<mx*ny){ */
/* 	    jxy_e=jy+jkk; */
/* 	  }else{ */
/* 	    jxy_e=mx*ny; */
/* 	  } */
	  for(jk = 0; jk < s; jk=jk+2) {
	    for(js = 0; js < s; js=js+2) {	      
	      for(jxy = jy; jxy < jxy_e; jxy++) {
		j = jxy + mxy*(jz+stm) + mx*stm;		
		tmp_syrk[js+jk*s]=tmp_syrk[js+jk*s]+o[j+m*jk]*y[j+m*js];
		tmp_syrk[js+(jk+1)*s]=tmp_syrk[js+(jk+1)*s]+o[j+m*(jk+1)]*y[j+m*js];
		tmp_syrk[(js+1)+jk*s]=tmp_syrk[(js+1)+jk*s]+o[j+m*jk]*y[j+m*(js+1)];
		tmp_syrk[(js+1)+(jk+1)*s]=tmp_syrk[(js+1)+(jk+1)*s]+o[j+m*(jk+1)]*y[j+m*(js+1)];
	      }
	    }
	  }
	}

      }
    }
    
    //#pragma omp critical
    {
      for(jk = 0; jk < s; jk++) {
#pragma omp atomic
	x_w[jk]=x_w[jk]+x_w_tmp[jk];
      }
    }
    //#pragma omp critical
    {
      for(jk = 0; jk < s; jk++) {
	for(js = jk; js < s; js++) {
#pragma omp atomic
	  x_t[js+jk*s]=x_t[js+jk*s]+tmp_syrk[js+jk*s];
	}
      }
    }
  }

  for(jk = 0; jk < s; jk++) {
    for(js = jk; js < s; js++) {
      x_t[jk+js*s]=x_t[js+jk*s];
    }
  }

#undef cbcg_unroll
  return 0;
}



//   z[] = z[] + y[0:(s-1)][]*x[] 
//   w[] = z[] 
int gemv_cp(mpi_prm prm,type* x,type* y,type* z,type* w,int s){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy,js;
  type a;

  for(js = 0; js < s; js++) {
    a=x[js];
#pragma omp parallel for private(j,jz,jy,jx) 
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
	for(jx = 0; jx < nx; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  z[j]=a*y[j+m*js]+z[j];
	  w[j]=z[j];
	}
      }
    }
  }
  return 0;
}


int Vec_inplace(mpi_prm prm,type a,type* x,type* y){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

#pragma omp parallel for private(j,jz,jy,jx) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	y[j]=a*x[j];
      }
    }
  }
  return 0;
}

int Vec_inplace_dot2(mpi_prm prm,type a,type* x,type* y,type* tmp,type* tmp2){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  type tmp_,tmp2_;
  type sbuf[2],rbuf[2];
  tmp_=(double)0.0;
  tmp2_=(double)0.0;
#pragma omp parallel for private(j,jz,jy,jx)  reduction(+:tmp_,tmp2_)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	tmp_=tmp_+y[j]*y[j];
	tmp2_=tmp2_+x[j]*y[j];
	y[j]=a*x[j];
      }
    }
  }
  sbuf[0]=tmp_;
  sbuf[1]=tmp2_;

#ifdef JUPITER_MPI
  MPI_Allreduce(sbuf,rbuf,2,MPI_TYPE, MPI_SUM,prm.comm);
#else
  rbuf[0]=sbuf[0];
  rbuf[1]=sbuf[1];
#endif

  tmp[0]=rbuf[0];
  tmp2[0]=rbuf[1];
  return 0;
}

int calc_norm(mpi_prm prm,type* x,type* tmp){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

  type tmp1_,tmp1;
  tmp1_=(double)0.0;

#pragma omp parallel for private(j,jz,jy,jx) reduction(+:tmp1_)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	tmp1_=tmp1_+x[j]*x[j];
      }
    }
  }
  tmp1=(double)0.0;

#ifdef JUPITER_MPI
  MPI_Allreduce(&tmp1_, &tmp1, 1,MPI_TYPE, MPI_SUM,prm.comm);
#else
  tmp1=tmp1_;
#endif

  //  tmp[0]=sqrt(tmp1);
  tmp[0]=sqrt(tmp1);
  return 0;
}

int calc_norm_local(mpi_prm prm,type* x,type* tmp){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

  type tmp1_,tmp1;
  tmp1_=(double)0.0;

#pragma omp parallel for private(j,jz,jy,jx) reduction(+:tmp1_)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	tmp1_=tmp1_+x[j]*x[j];
      }
    }
  }
  tmp1=(double)0.0;
  //  MPI_Allreduce(&tmp1_, &tmp1, 1,MPI_TYPE, MPI_SUM,prm.comm);
  //  tmp[0]=sqrt(tmp1);
  tmp[0]=sqrt(tmp1_);
  return 0;
}

int calc_Maxnorm(mpi_prm prm,type* x,type* tmp){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

  type tmp1_,tmp1;
  tmp1_=(double)0.0;

  //#pragma omp parallel for private(j,jz,jy,jx) reduction(max:tmp1_)
#pragma omp parallel for private(j,jz,jy,jx) reduction(min:tmp1_)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	//	tmp1_=tmp1_+x[j]*x[j];
	//	if(tmp1_<x[j]){
	if(tmp1_>x[j]){
	  tmp1_=x[j];
	}
      }
    }
  }
  tmp1=(double)0.0;
  //  MPI_Allreduce(&tmp1_, &tmp1, 1,MPI_TYPE, MPI_MAX,prm.comm);
#ifdef JUPITER_MPI
  MPI_Allreduce(&tmp1_, &tmp1, 1,MPI_TYPE, MPI_MIN,prm.comm);
#else
  tmp1=tmp1_;
#endif
  //  tmp[0]=sqrt(tmp1);
  tmp[0]=tmp1;
  return 0;
}


int calc_dot(mpi_prm prm,type* x,type* y,type* tmp){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

  type tmp1_,tmp1;
  tmp1_=(double)0.0;

#pragma omp parallel for private(j,jz,jy,jx) reduction(+:tmp1_)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	tmp1_=tmp1_+x[j]*y[j];
      }
    }
  }
  tmp1=(double)0.0;
#ifdef JUPITER_MPI
  MPI_Allreduce(&tmp1_, &tmp1, 1,MPI_TYPE, MPI_SUM,prm.comm);
#else
  tmp1=tmp1_;
#endif
  tmp[0]=tmp1;
  return 0;
}


int make_pre_idiagMat1(mpi_prm prm,type* A,type* D){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb;

  int thid,num_th;
  int nzs_th,nze_th,nzs,nze;
  nzs=0;
  nze=nz-1;

#pragma omp parallel for private(j)
  for(j=0;j<m;j++){
    D[j]=1.0;
  }
  
  int max_threads;

#ifdef _OPENMP

#pragma omp parallel 
  {
#pragma omp master
    {
      max_threads=omp_get_num_threads();
      //      printf("max_threads  = %d\n",max_threads);
    }
  }
  if(nz<max_threads){
    omp_set_num_threads(nz);
  }

#else
  max_threads = 1;
#endif

  /*
#pragma omp parallel 
  {
#pragma omp master
    {
      printf("num_th = %d \n",omp_get_num_threads());
    }
  }
  */
  //    FILE  *fp=NULL;
    //    fp=fopen("debug1.txt","w");
#pragma omp parallel default(none)			\
  shared(prm,stm,nx,ny,nz,m,mx,mxy,A,D,nzs,nze)	\
  private(nzs_th,nze_th,thid,num_th)			\
  private(j,jcw,jcc,jcs,jcb,jx,jy,jz) 
  {

#ifdef _OPENMP
    thid   = omp_get_thread_num(); 
    num_th = omp_get_num_threads(); 
#else
    thid   = 0;
    num_th = 1;
#endif
    Sep_region(&thid,&num_th,&nzs,&nze,&nzs_th,&nze_th);    
    //    printf("id = %d --- nz = %d, %d \n",thid,nzs_th,nze_th);
    jz =nzs_th;
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	D[j]=A[jcc+3*m]-( 
			 A[jcc+1*m]*A[jcs+5*m]/D[j-mx] 
			 +A[jcc+2*m]*A[jcw+4*m]/D[j-1] 
			  ); 
      }
    }
    for(jz =nzs_th+1;jz<=nze_th;jz++){
      for(jy = 0; jy < ny; jy++) {
	for(jx = 0; jx < nx; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  jcb = j-mxy;
	  jcs = j-mx;
	  jcw = j-1;
	  jcc = j;

	  D[j]=A[jcc+3*m]-( 
			   A[jcc+0*m]*A[jcb+6*m]/D[j-mxy] 
			   +A[jcc+1*m]*A[jcs+5*m]/D[j-mx] 
			   +A[jcc+2*m]*A[jcw+4*m]/D[j-1] 
			    );
	}
      }
    }

    for(jz =nzs_th;jz<=nze_th;jz++){
      for(jy = 0; jy < ny; jy++) {
	for(jx = 0; jx < nx; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	   D[j]=1.0/D[j];
	}
      }
    }
  }

  //  fclose(fp);
#ifdef _OPENMP
  omp_set_num_threads(max_threads);
#endif
  return 0;
}

int make_pre_subdividing_idiagMat1(
				   mpi_prm prm,
				   type* A,type* D,
				   type *block_yfilterL,type *block_zfilterL,
				   type *block_yfilterU,type *block_zfilterU,
				   int *block_ys,int *block_zs,
				   int *block_ye,int *block_ze
				   ){

  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  int j,jx,jy,jz;
#pragma omp parallel for private(j)
  for(j=0;j<m;j++){
    D[j]=1.0;
  }
  int xdivblock = prm.xdivblock;
  int ydivblock = prm.ydivblock;
  int zdivblock = prm.zdivblock;
  int nxblock   = prm.nxblock;
  int nyblock   = prm.nyblock;
  int nzblock   = prm.nzblock;
  int jxblock,jyblock,jzblock;
  zero_initialize(nyblock * ydivblock,block_yfilterL);
  zero_initialize(nzblock * zdivblock,block_zfilterL);
  zero_initialize(nyblock * ydivblock,block_yfilterU);
  zero_initialize(nzblock * zdivblock,block_zfilterU);

  int jzs0 = 0;
  int jze0 = nz-1;
  int jys0 = 0;
  int jye0 = ny-1;

  //  printf(" xdivblock,ydivblock,zdivblock = %d, %d, %d \n",xdivblock,ydivblock,zdivblock);
  //  printf(" nxblock,nyblock,nzblock       = %d, %d, %d \n",nxblock,nyblock,nzblock);

  for(jzblock = 0; jzblock < zdivblock; jzblock++) {
    int jzs  = -1;
    int jze  = -1;
    Sep_region(&jzblock,&zdivblock,&jzs0,&jze0,&jzs,&jze);
    //    printf(" jzs,jze = %d,%d \n",jzs,jze);
    block_zs[jzblock] = jzs;
    block_ze[jzblock] = jze;
    for(jz = jzs; jz <= jze; jz++) {
      block_zfilterL[ jz-jzs + jzblock*nzblock ] = 1.0;
      block_zfilterU[ jz-jzs + jzblock*nzblock ] = 1.0;
    }
    block_zfilterL[           jzblock*nzblock ] = 0.0;
    block_zfilterU[ jze-jzs + jzblock*nzblock ] = 0.0;
  }

  for(jyblock = 0; jyblock < ydivblock; jyblock++) {
    int jys  = -1;
    int jye  = -1;
    Sep_region(&jyblock,&ydivblock,&jys0,&jye0,&jys,&jye);
    //    printf(" jys,jye = %d,%d \n",jys,jye);
    block_ys[jyblock] = jys;
    block_ye[jyblock] = jye;
    for(jy = jys; jy <= jye; jy++) {
      block_yfilterL[ jy-jys + jyblock*nyblock ] = 1.0;
      block_yfilterU[ jy-jys + jyblock*nyblock ] = 1.0;
    }
    block_yfilterL[           jyblock*nyblock ] = 0.0;
    block_yfilterU[ jye-jys + jyblock*nyblock ] = 0.0;
  }  

  for(jzblock = 0; jzblock < zdivblock; jzblock++) {
    int jzs  = -1;
    int jze  = -1;
    Sep_region(&jzblock,&zdivblock,&jzs0,&jze0,&jzs,&jze);
    for(jyblock = 0; jyblock < ydivblock; jyblock++) {
      int jys  = -1;
      int jye  = -1;
      Sep_region(&jyblock,&ydivblock,&jys0,&jye0,&jys,&jye);

      for(jz = jzs; jz <= jze; jz++) {
	for(jy = jys; jy <= jye; jy++) {
	  for(jx = 0; jx < nx; jx++) {
	    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    int jcb = j-mxy;
	    int jcs = j-mx;
	    int jcw = j-1;
	    int jcc = j;	
#if 1
	    int zfilter = block_zfilterL[ jz-jzs + jzblock*nzblock ] * block_zfilterU[ jz-jzs + jzblock*nzblock ];
	    int yfilter = block_yfilterL[ jy-jys + jyblock*nyblock ] * block_yfilterU[ jy-jys + jyblock*nyblock ];
	    D[j] =  A[jcc+3*m] - ( 
				   A[jcc+0*m]*A[jcb+6*m]/D[j-mxy] * zfilter
				  +A[jcc+1*m]*A[jcs+5*m]/D[j-mx]  * yfilter
				   ) ;
#else
	    if(
	       ( block_zfilterL[ jz-jzs + jzblock*nzblock ] == 0.0 ) || 
	       ( block_yfilterL[ jy-jys + jyblock*nyblock ] == 0.0 ) 
	       ){
	      D[j] = 0.0;
	    }else{
	      D[j] =  A[jcc+3*m] - ( 
				     A[jcc+0*m]*A[jcb+6*m]/D[j-mxy] 
				    +A[jcc+1*m]*A[jcs+5*m]/D[j-mx]  
				     ) ;
	    }
#endif
	  }
	}
      }      
    }
  }

  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	D[j]=1.0/D[j];
      }
    }
  }

#if 0
  printf(" \n ------------- \n");
  printf(" ydivblock,zdivblock = %d, %d \n",ydivblock,zdivblock);
  for(jzblock = 0; jzblock < zdivblock; jzblock++) {
    printf(" jzblock = %d : %d -- %d \n",
	   jzblock,
	   block_zs[jzblock] ,
	   block_ze[jzblock] 
	   );
  }  
  for(jyblock = 0; jyblock < ydivblock; jyblock++) {
    printf(" jyblock = %d : %d -- %d \n",
	   jyblock,
	   block_ys[jyblock] ,
	   block_ye[jyblock] );
  }

#endif

  return 0;

}



int make_pre_step_idiagMat1(mpi_prm prm,type* A,type* D){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb;

  int thid,num_th;
  int nzs_th,nze_th,nzs,nze;
  nzs=0;
  nze=nz-1;

  int nxdiv   = prm.nxdiv;
  int nxblock = prm.nxblock;
  printf(" nxdiv = %4d, nxblock = %4d \n",nxdiv,nxblock); 

#pragma omp parallel for private(j)
  for(j=0;j< (nxdiv)*(nxblock+stm*2)*ny*nz ;j++){
    D[j]=1.0;
  }
  
  int max_threads;
#ifdef _OPENMP
#pragma omp parallel 
  {
#pragma omp master
    {
      max_threads=omp_get_num_threads();
    }
  }
  if(nz<max_threads){
    omp_set_num_threads(nz);
  }
#else
  max_threads = 1;
#endif

#pragma omp parallel default(none)			\
  shared(prm,stm,nx,ny,nz,m,mx,mxy,A,D,nzs,nze)	\
  shared(nxdiv,nxblock,my)				\
  private(nzs_th,nze_th,thid,num_th)			\
  private(j,jcw,jcc,jcs,jcb,jx,jy,jz) 
  {

#ifdef _OPENMP
    thid   = omp_get_thread_num(); 
    num_th = omp_get_num_threads(); 
#else
    thid   = 0;
    num_th = 1;
#endif
    Sep_region(&thid,&num_th,&nzs,&nze,&nzs_th,&nze_th);    

    for(jz =nzs_th;jz<=nze_th;jz++){
      for(jy = 0; jy < ny; jy++) {
	int i,ii;
	if(jz==nzs_th){
	  //	  for(jx = 0; jx < nx; jx++) {
	  for(ii = 0; ii < nxblock ; ii++) {
	    for(i = 0; i < nxdiv ; i++) {
	      jx = ii + i*nxblock;
	      int jj = ii + nxdiv*(i+stm) + ( nxdiv*(nxblock+stm*2) )*(jy+stm) + ( nxdiv*(nxblock+stm*2) )*my*(jz+stm);
	      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	      jcb = j-mxy;
	      jcs = j-mx;
	      jcw = j-1;
	      jcc = j;

	      D[jj]=A[jcc+3*m]-( 
				 A[jcc+1*m] * A[jcs+5*m] / D[ j - ( nxdiv*(nxblock+stm*2) ) ]
				+A[jcc+2*m] * A[jcw+4*m] / D[ j - nxdiv ] 
				 ); 
	    }
	  }
	}else{
	  for(ii = 0; ii < nxblock ; ii++) {
	    for(i = 0; i < nxdiv ; i++) {
	      jx = ii + i*nxblock;
 	      int jj = ii + nxdiv*(i+stm) + ( nxdiv*(nxblock+stm*2) )*(jy+stm) + ( nxdiv*(nxblock+stm*2) )*my*(jz+stm);

	      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	      jcb = j-mxy;
	      jcs = j-mx;
	      jcw = j-1;
	      jcc = j;
	      D[jj]=A[jcc+3*m]-( 
				 A[jcc+0*m] * A[jcb+6*m] / D[ j - ( nxdiv*(nxblock+stm*2) )*my ] 
				+A[jcc+1*m] * A[jcs+5*m] / D[ j - ( nxdiv*(nxblock+stm*2) )    ] 
				+A[jcc+2*m] * A[jcw+4*m] / D[ j - nxdiv]
				 );
	    }
	  }
	}
      }
    }

    for(jz =nzs_th;jz<=nze_th;jz++){
      for(jy = 0; jy < ny; jy++) {
	int i,ii;
	for(ii = 0; ii < nxblock ; ii++) {
	  for(i = 0; i < nxdiv ; i++) {
	    jx = ii + i*nxblock;
	    int jj = ii + nxdiv*(i+stm) + ( nxdiv*(nxblock+stm*2) )*(jy+stm) + ( nxdiv*(nxblock+stm*2) )*my*(jz+stm);
	    D[jj]=1.0/D[jj];
	    printf(" %d,%d,%d -- %f \n",jx,jy,jz,D[jj]);
	  }
	}
      }
    }
  }

#ifdef _OPENMP
  omp_set_num_threads(max_threads);
#endif
 
  return 0;
}


int MatVec_dot_dcomm(mpi_prm prm,type* A,type* x,type* y,type* tmp,
		     type *q_b,type *q_t,type *q_s,type *q_n,type *q_w,type *q_e
		     ){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;
  type tmp1_,tmp1;
  tmp1_=(double)0.0;

#pragma omp parallel 
  {
#pragma omp master
    {
#ifdef JUPITER_MPI
      sleev_dcomm(x,A,q_b,q_t,q_s,q_n,q_w,q_e,&prm); 
#endif
    }
    // #pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) reduction(+:tmp1_) schedule(static,1)
#pragma omp for  private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx,tmp1) reduction(+:tmp1_) schedule(dynamic,1) 
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
	for(jx = 0; jx < nx; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

	  jcb = j-mxy;
	  jcs = j-mx;
	  jcw = j-1;
	  jcc = j;
	  jce = j+1;
	  jcn = j+mx;
	  jct = j+mxy;
	  //	  y[j]=
	  tmp1=
	    A[j+0*m]*x[jcb] 
	    +A[j+1*m]*x[jcs] 
	    +A[j+2*m]*x[jcw] 
	    +A[j+3*m]*x[jcc] 
	    +A[j+4*m]*x[jce] 
	    +A[j+5*m]*x[jcn] 
	    +A[j+6*m]*x[jct] 
	    ;
	  //	  tmp1_=tmp1_+y[j]*x[j];
	  tmp1_=tmp1_+tmp1*x[j];
	  y[j]=tmp1;
	}
      }
    }
  }
  tmp[0]=tmp1_;
  MatVec_dot_sface(prm,A,x,y,tmp,q_b,q_t,q_s,q_n,q_w,q_e);

  return 0;

}


int MatVec_dcomm(mpi_prm prm,type* A,type* x,type* y,
		     type *q_b,type *q_t,type *q_s,type *q_n,type *q_w,type *q_e
		     ){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;
  type tmp1_,tmp1;
  tmp1_=(double)0.0;

   #pragma omp parallel 
  {
     #pragma omp master
    {
#ifdef JUPITER_MPI
      sleev_dcomm(x,A,q_b,q_t,q_s,q_n,q_w,q_e,&prm); 
#endif
    }

  #pragma omp for  private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) schedule(dynamic,1) 
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
	for(jx = 0; jx < nx; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

	  jcb = j-mxy;
	  jcs = j-mx;
	  jcw = j-1;
	  jcc = j;
	  jce = j+1;
	  jcn = j+mx;
	  jct = j+mxy;

	  y[j]=
	    A[j+0*m]*x[jcb] 
	    +A[j+1*m]*x[jcs] 
	    +A[j+2*m]*x[jcw] 
	    +A[j+3*m]*x[jcc] 
	    +A[j+4*m]*x[jce] 
	    +A[j+5*m]*x[jcn] 
	    +A[j+6*m]*x[jct] 
	    ;
	}
      }
    }
  }
  MatVec_sface(prm,A,x,y,q_b,q_t,q_s,q_n,q_w,q_e);

  return 0;

}

// y[] = A[][]x[]
// dot = (A[][]x[],x)
int MatVec_dot_sface(mpi_prm prm,type* A,type* x,type* y,type* tmp,
		     type *q_b,type *q_t,type *q_s,type *q_n,type *q_w,type *q_e
		     ){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j,j_;
  int jz,jx,jy;
  type tmp1_,tmp1;
  tmp1_=tmp[0];

#pragma omp parallel private(j,j_,jy,jx,jz)
  {

    jz = 0;
#pragma omp for  reduction(+:tmp1_) schedule(dynamic,1) 
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	j_ = jx + nx*jy ;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	y[j]=y[j]+q_b[j_];
	tmp1_=tmp1_+q_b[j_]*x[j];
      }
    }

    jz  = nz-1;
#pragma omp for  reduction(+:tmp1_) schedule(dynamic,1) 
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	j_ = jx + nx*jy ;
	j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	y[j]=y[j]+q_t[j_];
	tmp1_=tmp1_+q_t[j_]*x[j];
      }
    }

  
    jy  = 0;
#pragma omp for  reduction(+:tmp1_) schedule(dynamic,1) 
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
	j_ = jx + nx*jz ;
	j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	y[j]=y[j]+q_s[j_];
	tmp1_=tmp1_+q_s[j_]*x[j];
      }
    }


    jy  = ny-1;
#pragma omp for  reduction(+:tmp1_) schedule(dynamic,1) 
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
	j_ = jx + nx*jz ;
	j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	y[j]=y[j]+q_n[j_];
	tmp1_=tmp1_+q_n[j_]*x[j];
      }
    }


    jx  = 0;
#pragma omp for  reduction(+:tmp1_) schedule(dynamic,1) 
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
	j_ = jy + ny*jz ;
	j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	y[j]=y[j]+q_w[j_];
	tmp1_=tmp1_+q_w[j_]*x[j];
      }
    }


    jx  = nx-1;
#pragma omp for  reduction(+:tmp1_) schedule(dynamic,1) 
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
	j_ = jy + ny*jz ;
	j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	y[j]=y[j]+q_e[j_];
	tmp1_=tmp1_+q_e[j_]*x[j];
      }
    }

  }
  tmp1=(double)0.0;
#ifdef JUPITER_MPI
  MPI_Allreduce(&tmp1_, &tmp1, 1,MPI_TYPE, MPI_SUM,prm.comm);
#else
  tmp1=tmp1_;
#endif
  tmp[0]=tmp1;

  return 0;

}

int MatVec_sface(mpi_prm prm,type* A,type* x,type* y,
		     type *q_b,type *q_t,type *q_s,type *q_n,type *q_w,type *q_e
		     ){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j,j_;
  int jz,jx,jy;
  type tmp1_,tmp1;

#pragma omp parallel private(j,j_,jy,jx,jz)
  {
    jz = 0;
#pragma omp for   schedule(dynamic,1) 
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	j_ = jx + nx*jy ;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	y[j]=y[j]+q_b[j_];
      }
    }


    jz  = nz-1;
#pragma omp for   schedule(dynamic,1) 
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	j_ = jx + nx*jy ;
	j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	y[j]=y[j]+q_t[j_];
      }
    }

  
    jy  = 0;
#pragma omp for   schedule(dynamic,1) 
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
	j_ = jx + nx*jz ;
	j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	y[j]=y[j]+q_s[j_];
      }
    }


    jy  = ny-1;
#pragma omp for   schedule(dynamic,1) 
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
	j_ = jx + nx*jz ;
	j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	y[j]=y[j]+q_n[j_];
      }
    }


    jx  = 0;
#pragma omp for   schedule(dynamic,1) 
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
	j_ = jy + ny*jz ;
	j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	y[j]=y[j]+q_w[j_];
      }
    }


    jx  = nx-1;
#pragma omp for   schedule(dynamic,1) 
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
	j_ = jy + ny*jz ;
	j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	y[j]=y[j]+q_e[j_];
      }
    }

  }
  return 0;

}

// CACG 
int xcpy(mpi_prm prm,type* x,type* y){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

#pragma omp parallel for private(j,jz,jy,jx)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	y[j]=x[j];
      }
    }
  }
  return 0;
}


int x_view(mpi_prm prm,type* x){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  int j;
  int jz,jx,jy;

  /* for(jz = 0; jz < nz; jz++) { */
  /*   for(jy = 0; jy < ny; jy++) { */
  /*     for(jx = 0; jx < nx; jx++) { */
  type val = 0.0;
#if 1
  for(jz = 0; jz < nz; jz=jz+101) {
    for(jy = 0; jy < ny; jy=jy+4) {
      for(jx = 0; jx < nx; jx=jx+3) {  
#else
  for(jz = 0; jz < nz; jz=jz+1) {
    for(jy = 0; jy < ny; jy=jy+1) {
      for(jx = 0; jx < nx; jx=jx+1) {
#endif
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

	//	printf("%4d, %4d ,%4d ,%4d , %16.14e \n",
	//	       j,jx,jy,jz,x[j]);
	//	if(x[j]!=y[j])
	/*
	if(jz==0 &&  jy==0)
	printf("%4d, %4d ,%4d ,%4d , %16.14e \n",
	       j,jx,jy,jz,x[j]);
	*/
	
	printf(" %4d ,%4d ,%4d , %16.14e \n",
	       jx,jy,jz,x[j]);
	
	val = val + x[j];
	/*
	if(fabs( x[j+3*m] )<10.0){	
	printf("%4d, %4d ,%4d ,%4d ,%4d , %16.14e \n",
	       j,jx,jy,jz,x[j+3*m]);
	}
*/
      }
    }
  }
  printf(" val = %f \n",val);
  return 0;
}

int xy_view(mpi_prm prm,type* x,type* y){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  int j;
  int jz,jx,jy;

  /* for(jz = 0; jz < nz; jz++) { */
  /*   for(jy = 0; jy < ny; jy++) { */
  /*     for(jx = 0; jx < nx; jx++) { */

  for(jz = 0; jz < nz; jz=jz+3) {
    for(jy = 0; jy < ny; jy=jy+3) {
      for(jx = 0; jx < nx; jx=jx+3) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

	//	printf("%4d, %4d ,%4d ,%4d , %16.14e \n",
	//	       j,jx,jy,jz,x[j]);
	//	if(x[j]!=y[j])
	/*
	if(jz==0 &&  jy==0)
	printf("%4d, %4d ,%4d ,%4d , %16.14e \n",
	       j,jx,jy,jz,x[j]);
	*/
	
	printf(" %4d ,%4d ,%4d , %16.14e , %16.14e \n",
	       jx,jy,jz,x[j],y[j]);
	
	/*
	printf(" %4d ,%4d ,%4d , %16.14e \n",
	       jx,jy,jz,x[j]/y[j]);
	*/

	/*
	if(fabs( x[j+3*m] )<10.0){	
	printf("%4d, %4d ,%4d ,%4d ,%4d , %16.14e \n",
	       j,jx,jy,jz,x[j+3*m]);
	}
*/
      }
    }
  }
  return 0;
}

int x_write(mpi_prm prm,type* x){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

  FILE *fp;
  char fname[100];
  for(jz = 0; jz < nz; jz=jz+1) {
    sprintf(fname,"./x_dat/x_%04d.dat",jz);
    fp = fopen(fname,"w");
    for(jy = 0; jy < ny; jy=jy+1) {
      for(jx = 0; jx < nx; jx=jx+1) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	fprintf(fp,"%d %d %f \n",
		jx,jy,x[j]);
      }
      fprintf(fp," \n");
    }
    fclose(fp);
  }
  return 0;
}

int MatVec(mpi_prm prm,type* A,type* x,type* y){
  int stm,swtmx;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;

  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;

  // 
#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) schedule(static,1)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	jce = j+1;
	jcn = j+mx;
	jct = j+mxy;

	y[j]=
	  A[j+0*m]*x[jcb] 
	  +A[j+1*m]*x[jcs] 
	  +A[j+2*m]*x[jcw] 
	  +A[j+3*m]*x[jcc] 
	  +A[j+4*m]*x[jce] 
	  +A[j+5*m]*x[jcn] 
	  +A[j+6*m]*x[jct] 	  ;
      }
    }
  }

  return 0;
}

int sMatVec(mpi_prm prm,type* A,type* x,type* y){
  int stm,swtmx;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;

  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;

  // 
#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) schedule(static,1)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	jce = j+1;
	jcn = j+mx;
	jct = j+mxy;

#if 0
	y[j]=
	  A[j+0*m]*x[jcb] 
	  +A[j+1*m]*x[jcs] 
	  +A[j+2*m]*x[jcw] 
	  +A[j+3*m]*x[jcc] 
	  +A[j+4*m]*x[jce] 
	  +A[j+5*m]*x[jcn] 
	  +A[j+6*m]*x[jct] 	  ;

#else
	y[j]=
	   A[j   + 0*m]*x[jcb] 
	  +A[j   + 1*m]*x[jcs] 
	  +A[j   + 2*m]*x[jcw] 
	  +A[j   + 3*m]*x[jcc] 
	  +A[jce + 2*m]*x[jce] 
	  +A[jcn + 1*m]*x[jcn] 
	  +A[jct + 0*m]*x[jct] 
	  ;

#endif

      }
    }
  }

  return 0;
}


int MatVec_cb(mpi_prm prm,type a,type b,
	      type* A,type* x,type* y,type* z,type* w,type* v){
  int stm,swtmx;
  int nx,ny,nz;

  int m;
  int mx,my,mxy;

  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;

  // 
#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) schedule(static,1)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	jce = j+1;
	jcn = j+mx;
	jct = j+mxy;

	y[j]=
	  A[j+0*m]*x[jcb] 
	  +A[j+1*m]*x[jcs] 
	  +A[j+2*m]*x[jcw] 
	  +A[j+3*m]*x[jcc] 
	  +A[j+4*m]*x[jce] 
	  +A[j+5*m]*x[jcn] 
	  +A[j+6*m]*x[jct] 	  ;
	v[j]=a*y[j]+b*z[j]-w[j];
      }
    }
  }

  return 0;
}

int sMatVec_cb(mpi_prm prm,type a,type b,
	      type* A,type* x,type* y,type* z,type* w,type* v){
  int stm,swtmx;
  int nx,ny,nz;

  int m;
  int mx,my,mxy;

  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;

  // 
#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) schedule(static,1)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	jce = j+1;
	jcn = j+mx;
	jct = j+mxy;

#if 0
	y[j]=
	  A[j+0*m]*x[jcb] 
	  +A[j+1*m]*x[jcs] 
	  +A[j+2*m]*x[jcw] 
	  +A[j+3*m]*x[jcc] 
	  +A[j+4*m]*x[jce] 
	  +A[j+5*m]*x[jcn] 
	  +A[j+6*m]*x[jct] 	  ;
#else
	y[j]=
	   A[j   + 0*m]*x[jcb] 
	  +A[j   + 1*m]*x[jcs] 
	  +A[j   + 2*m]*x[jcw] 
	  +A[j   + 3*m]*x[jcc] 
	  +A[jce + 2*m]*x[jce] 
	  +A[jcn + 1*m]*x[jcn] 
	  +A[jct + 0*m]*x[jct] 
	  ;

#endif

	v[j]=a*y[j]+b*z[j]-w[j];
      }
    }
  }

  return 0;
}


int MatVec_unl(mpi_prm prm,type* A,type* x,type* y,
	       int mpkxs, int mpkxe,
	       int mpkys, int mpkye,
	       int mpkzs, int mpkze){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;
  //  int smax=smax_-1;
  //  smax=smax-1;

#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) schedule(static,1)
  for(jz = 0-mpkzs; jz < nz+mpkze; jz++) {
    for(jy = 0-mpkys; jy < ny+mpkye; jy++) {
      for(jx = 0-mpkxs; jx < nx+mpkxe; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	jce = j+1;
	jcn = j+mx;
	jct = j+mxy;
	y[j]=
	  A[j+0*m]*x[jcb] 
	  +A[j+1*m]*x[jcs] 
	  +A[j+2*m]*x[jcw] 
	  +A[j+3*m]*x[jcc] 
	  +A[j+4*m]*x[jce] 
	  +A[j+5*m]*x[jcn] 
	  +A[j+6*m]*x[jct];
      }
    }
  }
  return 0;
}


int MatVec_unl_dcomm2(mpi_prm* prm,type* A,type* x,type* y,
		     int mpkxs, int mpkxe,
		     int mpkys, int mpkye,
		     int mpkzs, int mpkze){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(*prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;

#pragma omp parallel 
  {
#pragma omp master
    {
#ifdef JUPITER_MPI
      sleev_comm_yz(x,prm);
#endif
    }

#pragma omp for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) schedule(dynamic,1) 
    for(jz = 0-mpkzs+1; jz < nz+mpkze-1; jz++) {
      for(jy = 0-mpkys+1; jy < ny+mpkye-1; jy++) {
	for(jx = 0-mpkxs; jx < nx+mpkxe; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  jcb = j-mxy;
	  jcs = j-mx;
	  jcw = j-1;
	  jcc = j;
	  jce = j+1;
	  jcn = j+mx;
	  jct = j+mxy;
	  y[j]=
	    A[j+0*m]*x[jcb] 
	    +A[j+1*m]*x[jcs] 
	    +A[j+2*m]*x[jcw] 
	    +A[j+3*m]*x[jcc] 
	    +A[j+4*m]*x[jce] 
	    +A[j+5*m]*x[jcn] 
	    +A[j+6*m]*x[jct];
	}
      }
    }

#pragma  omp barrier
#pragma  omp flush(x)

    jz=0-mpkzs;
#pragma omp for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jy,jx) schedule(dynamic,1) nowait
    for(jy = 0-mpkys; jy < ny+mpkye; jy++) {
      for(jx = 0-mpkxs; jx < nx+mpkxe; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	jce = j+1;
	jcn = j+mx;
	jct = j+mxy;
	y[j]=
	  A[j+0*m]*x[jcb] 
	  +A[j+1*m]*x[jcs] 
	  +A[j+2*m]*x[jcw] 
	  +A[j+3*m]*x[jcc] 
	  +A[j+4*m]*x[jce] 
	  +A[j+5*m]*x[jcn] 
	  +A[j+6*m]*x[jct];
      }
    }

#pragma omp for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) schedule(dynamic,1) nowait
    for(jz = 0-mpkzs+1; jz < nz+mpkze-1; jz++) {
      jy = 0-mpkys;
      for(jx = 0-mpkxs; jx < nx+mpkxe; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	jce = j+1;
	jcn = j+mx;
	jct = j+mxy;
	y[j]=
	  A[j+0*m]*x[jcb] 
	  +A[j+1*m]*x[jcs] 
	  +A[j+2*m]*x[jcw] 
	  +A[j+3*m]*x[jcc] 
	  +A[j+4*m]*x[jce] 
	  +A[j+5*m]*x[jcn] 
	  +A[j+6*m]*x[jct];
      }
      jy = ny+mpkye-1;
      for(jx = 0-mpkxs; jx < nx+mpkxe; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	jce = j+1;
	jcn = j+mx;
	jct = j+mxy;
	y[j]=
	  A[j+0*m]*x[jcb] 
	  +A[j+1*m]*x[jcs] 
	  +A[j+2*m]*x[jcw] 
	  +A[j+3*m]*x[jcc] 
	  +A[j+4*m]*x[jce] 
	  +A[j+5*m]*x[jcn] 
	  +A[j+6*m]*x[jct];
      }
    }

    jz=nz+mpkze-1;
#pragma omp for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jy,jx) schedule(dynamic,1) nowait
    for(jy = 0-mpkys; jy < ny+mpkye; jy++) {
      for(jx = 0-mpkxs; jx < nx+mpkxe; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	jce = j+1;
	jcn = j+mx;
	jct = j+mxy;
	y[j]=
	  A[j+0*m]*x[jcb] 
	  +A[j+1*m]*x[jcs] 
	  +A[j+2*m]*x[jcw] 
	  +A[j+3*m]*x[jcc] 
	  +A[j+4*m]*x[jce] 
	  +A[j+5*m]*x[jcn] 
	  +A[j+6*m]*x[jct];
      }
    }   
  }
  return 0;
}


int calc_dot_local(mpi_prm prm,type* x,type* y,type* tmp){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

  type tmp1_,tmp1;
  tmp1_=(double)0.0;
#pragma omp parallel for private(j,jz,jy,jx) reduction(+:tmp1_)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	tmp1_=tmp1_+x[j]*y[j];
      }
    }
  }
  tmp[0]=tmp1_;
  return 0;
}


int calc_dot2_local(mpi_prm prm,type* x,type* y,type* z,type* tmp1,type* tmp2){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  type tmp1_,tmp2_;
  tmp1_=(double)0.0;
  tmp2_=(double)0.0;

#pragma omp parallel for private(j,jz,jy,jx) reduction(+:tmp1_,tmp2_)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	tmp1_=tmp1_+x[j]*y[j];
	tmp2_=tmp2_+x[j]*z[j];
      }
    }
  }
  tmp1[0]=tmp1_;
  tmp2[0]=tmp2_;
  return 0;
}


int update_cacg(
		mpi_prm prm,
 		type* x,type* x_1,type* x_2,
		type* q,type* q_1,type* q_2,
		type* z,type* z_1,type* z_2,
		type* u,type* y,type rho,type gamma){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
#pragma omp parallel for private(j,jz,jy,jx) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	x[j] = rho*(x_1[j]+gamma*q_1[j])+(1.0-rho)*x_2[j];
	q[j] = rho*(q_1[j]-gamma*u[j]  )+(1.0-rho)*q_2[j];
	z[j] = rho*(z_1[j]-gamma*y[j]  )+(1.0-rho)*z_2[j];
      }
    }
  }
  return 0;
}

int neumann_pre(mpi_prm prm,type* A,type Amax,
		type* r,type* s,int order){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int ex;
  int i,j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;

  // スケーリング
  type r_[m];
  type rpre[m];
  type sum;

#pragma omp parallel for private(jx,jy,jz,i) schedule(static,1)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	rpre[j]=r[j]/A[j+3*m]; // スケーリング
	s[j]   =rpre[j];
      }
    }
  }

  for(ex=0;ex<order;ex++){
#ifdef JUPITER_MPI
    sleev_comm(rpre,&prm); 
#endif
#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx,sum) schedule(static,1)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
	for(jx = 0; jx < nx; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  jcb = j-mxy;
	  jcs = j-mx;
	  jcw = j-1;
	  jcc = j;
	  jce = j+1;
	  jcn = j+mx;
	  jct = j+mxy;

	  sum=
	    -A[j+0*m]/A[j+3*m] * rpre[jcb] 
	    -A[j+1*m]/A[j+3*m] * rpre[jcs] 
	    -A[j+2*m]/A[j+3*m] * rpre[jcw] 
	    -A[j+4*m]/A[j+3*m] * rpre[jce] 
	    -A[j+5*m]/A[j+3*m] * rpre[jcn] 
	    -A[j+6*m]/A[j+3*m] * rpre[jct] 
	    ;

	  s[j]=s[j]+sum;
	  r_[jcc]=sum;	
	}
      }
    }

    // ここはmemcopyに置き換えられている
    // 最適化メッセージ : memcopy(with guard) generated
#pragma omp parallel for private(i)
    for(i=0; i<m; i++){
      rpre[i]=r_[i];
    }
  }
  
  return 0;
}

int neumann_pre_MPK(mpi_prm prm,type* A,type Amax,
		    type* r,type* s,int order,
		    int mpkxs, int mpkxe,
		    int mpkys, int mpkye,
		    int mpkzs, int mpkze){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int ex;
  int i,j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;

  // スケーリング
  type r_[m];
  type rpre[m];
  type sum;

  // #pragma omp parallel for private(jx,jy,jz,i)
  //  for(jz = 0; jz < nz; jz++) {
  //    for(jy = 0; jy < ny; jy++) {
  //      for(jx = 0; jx < nx; jx++) {
  for(jz = 0-mpkzs; jz < nz+mpkze; jz++) {
    for(jy = 0-mpkys; jy < ny+mpkye; jy++) {
      for(jx = 0-mpkxs; jx < nx+mpkxe; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	rpre[j]=r[j]/A[j+3*m]; // スケーリング
	s[j]   =rpre[j];
      }
    }
  }

  for(ex=0;ex<order;ex++){
    //    sleev_comm(rpre,&prm); 
    //#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx,sum) schedule(static,1)
    //    for(jz = 0; jz < nz; jz++) {
    //      for(jy = 0; jy < ny; jy++) {
    //	for(jx = 0; jx < nx; jx++) {
    for(jz = 0-mpkzs; jz < nz+mpkze; jz++) {
      for(jy = 0-mpkys; jy < ny+mpkye; jy++) {
	for(jx = 0-mpkxs; jx < nx+mpkxe; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  jcb = j-mxy;
	  jcs = j-mx;
	  jcw = j-1;
	  jcc = j;
	  jce = j+1;
	  jcn = j+mx;
	  jct = j+mxy;

	  sum=
	    -A[j+0*m]/A[j+3*m] * rpre[jcb] 
	    -A[j+1*m]/A[j+3*m] * rpre[jcs] 
	    -A[j+2*m]/A[j+3*m] * rpre[jcw] 
	    -A[j+4*m]/A[j+3*m] * rpre[jce] 
	    -A[j+5*m]/A[j+3*m] * rpre[jcn] 
	    -A[j+6*m]/A[j+3*m] * rpre[jct] 
	    ;

	  s[j]=s[j]+sum;
	  r_[jcc]=sum;	
	}
      }
    }

    // #pragma omp parallel for private(i)
    for(i=0; i<m; i++){
      rpre[i]=r_[i];
    }
  }
  
  return 0;
}

int solve_pre_mat_unl(mpi_prm prm, type *A,type *Dinv,type *r, type *s,
		      int mpkxs, int mpkxe,
		      int mpkys, int mpkye,
		      int mpkzs, int mpkze){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  type sum;
  int j;
  int jz,jx,jy;
  //  int jcw,jcs,jcb,jce,jcn,jct,jcc;
  int jcs,jcb,jcn,jct;

  int thid,num_th;
  int nzs_th,nze_th,nzs,nze;

  int max_threads;

#ifdef pjacobi  

#pragma omp parallel for private(j,jz,jy,jx) schedule(static,1)
  for(jz = 0+mpkzs; jz < nz-mpkze; jz++) {
    for(jy = 0+mpkys; jy < ny-mpkye; jy++) {
      for(jx = 0+mpkxs; jx < nx-mpkxe; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	//	s[j]=r[j];
	s[j]=r[j]/A[j+3*m];
      }
    }
  }
  return 0;
#endif
  nzs=0    + mpkzs;
  nze=nz-1 - mpkze;

#ifdef _OPENMP  
  set_max_threads( (nze-nzs+1),&max_threads);
#endif

#ifdef  poisson

#else
#pragma omp parallel default(none)			\
  shared(stm,nx,ny,nz,m,mx,mxy,A,Dinv,r,s,nzs,nze)	\
  shared(mpkxs,mpkxe,mpkys,mpkye,mpkzs,mpkze)		\
  private(nzs_th,nze_th,thid,num_th)			\
  private(j,jcs,jcb,jcn,jct,jx,jy,jz,sum)    
#endif
  {
#ifdef _OPENMP
    thid   = omp_get_thread_num();  
    num_th = omp_get_num_threads();  
#else
    thid   = 0;
    num_th = 1;
#endif
    Sep_region(&thid,&num_th,&nzs,&nze,&nzs_th,&nze_th);    

    type sum_[nx+1];  

    jz =nzs_th;
    //    for(jy = 0; jy < ny; jy++) {
    for(jy = 0+mpkys; jy < ny-mpkye; jy++) {
      //      jx=0;
      jx=0+mpkxs;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcs = j-mx  ;
      jcb = j-mxy ;
      //      for(jx = 0; jx < nx; jx++) {
      for(jx = 0+mpkxs; jx < nx-mpkxe; jx++) {
	sum_[jx]=r[j]-(A[j+1*m]*s[jcs]);
	j=j+1;
	jcs=jcs+1;
	jcb=jcb+1;
      }
      //      jx=0;
      jx=0+mpkxs;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      j = j     ;
      //      jcw = j-1   ;
      sum=0;
      //      for(jx = 0; jx < nx; jx++) {
      for(jx = 0+mpkxs; jx < nx-mpkxe; jx++) {
	sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
	//	printf(" sum  [%4d] %4d,%4d,%4d, = %30.16e \n",j,jx,jy,jz,sum);
	s[j]=sum;
	j=j+1;
      }
    }  

    for(jz =nzs_th+1;jz<=nze_th;jz++){
      //      for(jy = 0; jy < ny; jy++) {
      for(jy = 0+mpkys; jy < ny-mpkye; jy++) {
      
	//	jx=0;
	jx=0+mpkxs;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcs = j-mx  ;
	jcb = j-mxy ;
	//	for(jx = 0; jx < nx; jx++) {
	for(jx = 0+mpkxs; jx < nx-mpkxe; jx++) {
	  sum_[jx]=r[j]-(A[j+1*m]*s[jcs] + A[j+0*m]*s[jcb]);
	  j=j+1;
	  jcs=jcs+1;
	  jcb=jcb+1;
	}

	//	jx=0;
	jx=0+mpkxs;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sum=0;
	//	for(jx = 0; jx < nx; jx++) {
	for(jx = 0+mpkxs; jx < nx-mpkxe; jx++) {
	  sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
	  s[j]=sum;
	  j=j+1;
	}

      }  
    }
 
    sum_[0]=(double)0.0;
    jz =nze_th;
    //    for(jy = ny-1; jy >=0 ; jy--) {
    for(jy = ny-1-mpkye; jy >=0+mpkys ; jy--) {
      //      jx = nx-1;
      jx = nx-1-mpkxe;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      jcn = j+mx  ;
      jct = j+mxy ;
      //      for(jx = nx-1; jx >=0 ; jx--) {
      for(jx = nx-1-mpkxe; jx >=0+mpkxs ; jx--) {
	sum_[jx+1] =  A[j+5*m]*s[jcn] ;
	j   = j-1;
	jcn = jcn-1;
	jct = jct-1;
      }
      //      jx=nx-1;
      jx = nx-1-mpkxe;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      sum=0;
      //      for(jx = nx-1; jx >=0 ; jx--) {
      for(jx = nx-1-mpkxe; jx >=0+mpkxs ; jx--) {
	sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
	s[j]=sum;	 
	j=j-1;
      }
    }

    for(jz =nze_th-1;jz>=nzs_th;jz--){
      //      for(jy = ny-1; jy >=0 ; jy--) {
      for(jy = ny-1-mpkye; jy >=0+mpkys ; jy--) {
	//	jx = nx-1;
	jx = nx-1-mpkxe;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcn = j+mx  ;
	jct = j+mxy ;
	//	for(jx = nx-1; jx >=0 ; jx--) {
	for(jx = nx-1-mpkxe; jx >=0+mpkxs ; jx--) {
	  sum_[jx+1] =  A[j+5*m]*s[jcn] + A[j+6*m]*s[jct];
	  j   = j-1;
	  jcn = jcn-1;
	  jct = jct-1;
	}

	//	jx=nx-1;
	jx = nx-1-mpkxe;
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	sum=0;
	//	for(jx = nx-1; jx >=0 ; jx--) {
	for(jx = nx-1-mpkxe; jx >=0+mpkxs ; jx--) {
	  sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
	  //	  printf("%4d,%4d,%4d --> %20.14e,%20.14e,%20.14e \n",jx,jy,jz,Dinv[j],r[j],s[j]);
	  s[j]=sum;
	  j=j-1;
	}
      }  
    } 
  }
#ifdef _OPENMP
  set_omp_threads(max_threads);
#endif
  return 0;
}

int make_pre_idiagMat1_unl(mpi_prm prm,type* A,type* D,
			   int mpkxs, int mpkxe,
			   int mpkys, int mpkye,
			   int mpkzs, int mpkze){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb;

  int thid,num_th;
  int nzs_th,nze_th,nzs,nze;
  //  int smax=smax_-1;
  //  int smax=0;
  //  nzs=0;
  //  nze=nz-1;


  nzs=0    + mpkzs;
  nze=nz-1 - mpkze;
  int max_threads;
#ifdef _OPENMP  
  set_max_threads( (nze-nzs+1),&max_threads);
#endif


#pragma omp parallel for private(j)
  for(j=0;j<m;j++){
    D[j]=1.0;
  }

#ifdef  poisson

#else
#pragma omp parallel default(none)			\
  shared(stm,nx,ny,nz,m,mx,mxy,A,D,nzs,nze)	\
  shared(mpkxs,mpkxe,mpkys,mpkye,mpkzs,mpkze)		\
  private(nzs_th,nze_th,thid,num_th)			\
  private(j,jcw,jcc,jcs,jcb,jx,jy,jz)			
#endif
  {

#ifdef _OPENMP
    thid   = omp_get_thread_num(); 
    num_th = omp_get_num_threads(); 
#else
    thid   = 0;
    num_th = 1;
#endif

    Sep_region(&thid,&num_th,&nzs,&nze,&nzs_th,&nze_th);    

    jz =nzs_th;
    //    for(jy = 0; jy < ny; jy++) {
    //      for(jx = 0; jx < nx; jx++) {
    for(jy = 0+mpkys; jy < ny-mpkye; jy++) {
      for(jx = 0+mpkxs; jx < nx-mpkxe; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	D[j]=A[jcc+3*m]-( 
			 A[jcc+1*m]*A[jcs+5*m]/D[j-mx] 
			 +A[jcc+2*m]*A[jcw+4*m]/D[j-1] 
			  ); 
      }
    }
    for(jz =nzs_th+1;jz<=nze_th;jz++){
      //      for(jy = 0; jy < ny; jy++) {
      //	for(jx = 0; jx < nx; jx++) {
      for(jy = 0+mpkys; jy < ny-mpkye; jy++) {
	for(jx = 0+mpkxs; jx < nx-mpkxe; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  jcb = j-mxy;
	  jcs = j-mx;
	  jcw = j-1;
	  jcc = j;

	  D[j]=A[jcc+3*m]-( 
			   A[jcc+0*m]*A[jcb+6*m]/D[j-mxy] 
			   +A[jcc+1*m]*A[jcs+5*m]/D[j-mx] 
			   +A[jcc+2*m]*A[jcw+4*m]/D[j-1] 
			    );
	}
      }
    }

    for(jz =nzs_th;jz<=nze_th;jz++){
      for(jy = 0+mpkys; jy < ny-mpkye; jy++) {
	for(jx = 0+mpkxs; jx < nx-mpkxe; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  //	  printf("D[%4d] = %30.16e \n",j,D[j]);
	  D[j]=1.0/D[j];
	  //	  printf("CA-CG %4d  A[%4d][%4d] = %16.12e\n",j,jx,jy,D[j]); 
	}
      }
    }
  }

#ifdef _OPENMP
  set_omp_threads(max_threads);
#endif
  return 0;
}


int pjacobi_sleev_unl(mpi_prm prm,type* A,type* r,type* s,
		      int mpkxs, int mpkxe,
		      int mpkys, int mpkye,
		      int mpkzs, int mpkze){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;


#pragma omp parallel for private(jx,jy,jz,j) collapse(2)
  for(jz = 0-mpkzs; jz < 0+mpkzs; jz++) {
    for(jy = 0-mpkys; jy < ny+mpkye; jy++) {
      for(jx = 0-mpkxs; jx < nx+mpkxe; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	s[j]=r[j]/A[j+3*m];
      }
    }
  }

#pragma omp parallel for private(jx,jy,jz,j)
  for(jz =0+mpkzs;jz<nz-mpkze; jz++) {

    for(jy = 0-mpkys; jy < 0+mpkys; jy++) {
      for(jx = 0-mpkxs; jx < nx+mpkxe; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);	
	s[j]=r[j]/A[j+3*m];
      }
    }

    for(jy =0+mpkys;jy<ny-mpkye ;jy++) {
      for(jx = 0-mpkxs; jx < 0+mpkxs; jx++) {
 	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	s[j]=r[j]/A[j+3*m];
      }
      for(jx = nx -mpkxe; jx < nx+mpkxe; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	s[j]=r[j]/A[j+3*m];
      }
    }

    for(jy =ny-mpkye ; jy < ny+mpkye; jy++) {
      for(jx = 0 -mpkxs; jx < nx+mpkxe; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	s[j]=r[j]/A[j+3*m];
      }
    }
  }


#pragma omp parallel for private(jx,jy,jz,j) collapse(2)
  for(jz = nz-mpkze; jz < nz+mpkze; jz++) {
    for(jy = 0 -mpkys; jy < ny+mpkye; jy++) {
      for(jx = 0 -mpkxs; jx < nx+mpkxe; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	s[j]=r[j]/A[j+3*m];
      }
    }
  }

  return 0;
}

int initializ_sleev_MPK_unl(mpi_prm prm,type* x,
			    int mpkxs, int mpkxe,
			    int mpkys, int mpkye,
			    int mpkzs, int mpkze){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

  // ここはmemsetに置き換えられている
  // 最適化メッセージ : memset generated

#pragma omp parallel for private(jx,jy,jz,j) collapse(2)
  for(jz = 0-mpkzs; jz < 0+mpkzs; jz++) {
    for(jy = 0 -mpkys; jy < ny+mpkye; jy++) {
      for(jx = 0 -mpkxs; jx < nx+mpkxe; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	x[j]=(double)0.0;
      }
    }
  }

#pragma omp parallel for private(jx,jy,jz,j) 
  for(jz =0+mpkzs;jz<nz-mpkze; jz++) {
    for(jy = 0-mpkys; jy < 0+mpkys; jy++) {
      for(jx = 0 -mpkxs; jx < nx+mpkxe; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);	
	x[j]=(double)0.0;
      }
    }

    for(jy =0+mpkys;jy<ny-mpkye ;jy++) {
      for(jx = 0 -mpkxs; jx < 0+mpkxs; jx++) {
 	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	x[j]=(double)0.0;
      }
      for(jx = nx -mpkxe; jx < nx+mpkxe; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	x[j]=(double)0.0;
      }
    }

    for(jy =ny-mpkye ; jy < ny+mpkye; jy++) {
      for(jx = 0 -mpkxs; jx < nx+mpkxe; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	x[j]=(double)0.0;
      }
    }
  }

#pragma omp parallel for private(jx,jy,jz,j) collapse(2)
  for(jz = nz-mpkze; jz < nz+mpkze; jz++) {
    for(jy = 0 -mpkys; jy < ny+mpkye; jy++) {
      for(jx = 0 -mpkxs; jx < nx+mpkxe; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	x[j]=(double)0.0;
      }
    }
  }

  return 0;
}


int initializ_sleev_MPK_tes(mpi_prm prm,type* x,int smax_){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int smax=smax_-1;
  
  // ここはmemsetに置き換えられている
  // 最適化メッセージ : memset generated

#pragma omp parallel private(j,jz,jy,jx) 
  {

#pragma omp for nowait collapse(2)
    for(jz = 0-smax; jz < 0+smax; jz++) {
      for(jy = 0-smax; jy < ny+smax; jy++) {
	for(jx = 0-smax; jx < nx+smax; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  x[j]=(double)0.0;
	}
      }
    }

#pragma omp for nowait 
    for(jz =0+smax;jz<nz-smax; jz++) {
      for(jy = 0-smax; jy < 0+smax; jy++) {
	for(jx = 0 -smax; jx < nx+smax; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  x[j]=(double)0.0;
	}
      }
    
      for(jy =0+smax;jy<ny-smax ;jy++) {
	for(jx = 0 -smax; jx < 0+smax; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  x[j]=(double)0.0;
	}
	for(jx = nx -smax; jx < nx+smax; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  x[j]=(double)0.0;
	}
      }

      for(jy =ny-smax ; jy < ny+smax; jy++) {
	for(jx = 0 -smax; jx < nx+smax; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  x[j]=(double)0.0;
	}
      }
    }

#pragma omp for nowait collapse(2)
    for(jz = nz-smax; jz < nz+smax; jz++) {
      for(jy = 0-smax; jy < ny+smax; jy++) {
	for(jx = 0-smax; jx < nx+smax; jx++) {
	  j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	  x[j]=(double)0.0;
	}
      }
    }
  }
  return 0;
}


//---------

int pjacobi_sleev(mpi_prm prm,type* A,type* r,type* s,int smax_){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int smax=smax_-1;

  // 条件分岐でゼロを指定しているが範囲指定を調整することで
  // 条件分岐をなくす。
  for(jz = 0-smax; jz < 0+smax; jz++) {
    for(jy = 0 -smax; jy < ny+smax; jy++) {
      for(jx = 0 -smax; jx < nx+smax; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	if(A[j+3*m]==(type)0.0){
	  s[j]=(double)0.0;
	}else{
	  s[j]=r[j]/A[j+3*m];
	}
      }
    }
  }

  for(jz =0+smax;jz<nz-smax; jz++) {
    for(jy = 0-smax; jy < 0+smax; jy++) {
      for(jx = 0 -smax; jx < nx+smax; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

	if(A[j+3*m]==(type)0.0){
	  s[j]=(double)0.0;
	}else{
	  s[j]=r[j]/A[j+3*m];
	}
      }
    }

    for(jy =0+smax;jy<ny-smax ;jy++) {
      for(jx = 0 -smax; jx < 0+smax; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	if(A[j+3*m]==(type)0.0){
	  s[j]=(double)0.0;
	}else{
	  s[j]=r[j]/A[j+3*m];
	}
      }
      for(jx = nx -smax; jx < nx+smax; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	if(A[j+3*m]==(type)0.0){
	  s[j]=(double)0.0;
	}else{
	  s[j]=r[j]/A[j+3*m];
	}
      }
    }

    for(jy =ny-smax ; jy < ny+smax; jy++) {
      for(jx = 0 -smax; jx < nx+smax; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	if(A[j+3*m]==(type)0.0){
	  s[j]=(double)0.0;
	}else{
	  s[j]=r[j]/A[j+3*m];
	}
      }
    }
  }

  for(jz = nz-smax; jz < nz+smax; jz++) {
    for(jy = 0 -smax; jy < ny+smax; jy++) {
      for(jx = 0 -smax; jx < nx+smax; jx++) {
	j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	if(A[j+3*m]==(type)0.0){
	  s[j]=(double)0.0;
	}else{
	  s[j]=r[j]/A[j+3*m];
	}
      }
    }
  }

  return 0;
}

// --------------------
#ifdef GMP
int calc_dot_local_GMP(mpi_prm prm,type* x,type* y,type* tmp){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

  mpf_set_default_prec(2048);
  mpf_t x_mp;
  mpf_init(x_mp);
  mpf_t y_mp;
  mpf_init(y_mp);
  mpf_t z_mp;
  mpf_init(z_mp);
  mpf_t sum_mp;
  mpf_init(sum_mp);
  mpf_set_d(sum_mp,0.0);

  // #pragma omp parallel for private(j,jz,jy,jx) reduction(+:tmp1_)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	mpf_set_d(x_mp,x[j]);
	mpf_set_d(y_mp,y[j]);
	mpf_mul(z_mp,x_mp,y_mp);
	mpf_add(sum_mp,sum_mp,z_mp);
      }
    }
  }
  tmp[0]=mpf_get_d(sum_mp);

  mpf_clear(x_mp);
  mpf_clear(y_mp);
  mpf_clear(z_mp);
  mpf_clear(sum_mp);
  return 0;
}

int calc_dot2_local_GMP(mpi_prm prm,type* x,type* y,type* z,type* tmp1,type* tmp2){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

  mpf_set_default_prec(2048);
  mpf_t x_mp;
  mpf_init(x_mp);
  mpf_t y_mp;
  mpf_init(y_mp);
  mpf_t z_mp;
  mpf_init(z_mp);

  mpf_t sum1_mp;
  mpf_init(sum1_mp);
  mpf_set_d(sum1_mp,0.0);

  mpf_t sum2_mp;
  mpf_init(sum2_mp);
  mpf_set_d(sum2_mp,0.0);

  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

	mpf_set_d(x_mp,x[j]);
	mpf_set_d(y_mp,y[j]);
	mpf_mul(z_mp,x_mp,y_mp);
	mpf_add(sum1_mp,sum1_mp,z_mp);

	mpf_set_d(x_mp,x[j]);
	mpf_set_d(y_mp,z[j]);
	mpf_mul(z_mp,x_mp,y_mp);
	mpf_add(sum2_mp,sum2_mp,z_mp);

      }
    }
  }
  tmp1[0]=mpf_get_d(sum1_mp);
  tmp2[0]=mpf_get_d(sum2_mp);

  mpf_clear(x_mp);
  mpf_clear(y_mp);
  mpf_clear(z_mp);
  mpf_clear(sum1_mp);
  mpf_clear(sum2_mp);

  return 0;
}
#endif



#ifdef dd_calc_
int dd_MatVec(mpi_prm prm,type* A,type* x,type* y){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;

  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	jcb = j-mxy;
	jcs = j-mx;
	jcw = j-1;
	jcc = j;
	jce = j+1;
	jcn = j+mx;
	jct = j+mxy;

	// この部分を4倍精度演算
	/*
	  y[j]=
	  A[j+0*m]*x[jcb] 
	  +A[j+1*m]*x[jcs] 
	  +A[j+2*m]*x[jcw] 
	  +A[j+3*m]*x[jcc] 
	  +A[j+4*m]*x[jce] 
	  +A[j+5*m]*x[jcn] 
	  +A[j+6*m]*x[jct] 	  ;
	*/
	double tmp0_hi=0.0,tmp0_lo=(double)0.0;
	double tmp1_hi=0.0,tmp1_lo=(double)0.0;

	dd_mul(A[j+0*m],0.0,x[jcb],0.0,&tmp0_hi,&tmp0_lo);
	dd_add(tmp1_hi,tmp1_lo,tmp0_hi,tmp0_lo,&tmp1_hi,&tmp1_lo);
	dd_mul(A[j+1*m],0.0,x[jcs],0.0,&tmp0_hi,&tmp0_lo);
	dd_add(tmp1_hi,tmp1_lo,tmp0_hi,tmp0_lo,&tmp1_hi,&tmp1_lo);
	dd_mul(A[j+2*m],0.0,x[jcw],0.0,&tmp0_hi,&tmp0_lo);
	dd_add(tmp1_hi,tmp1_lo,tmp0_hi,tmp0_lo,&tmp1_hi,&tmp1_lo);
	dd_mul(A[j+3*m],0.0,x[jcc],0.0,&tmp0_hi,&tmp0_lo);
	dd_add(tmp1_hi,tmp1_lo,tmp0_hi,tmp0_lo,&tmp1_hi,&tmp1_lo);
	dd_mul(A[j+4*m],0.0,x[jce],0.0,&tmp0_hi,&tmp0_lo);
 	dd_add(tmp1_hi,tmp1_lo,tmp0_hi,tmp0_lo,&tmp1_hi,&tmp1_lo);
	dd_mul(A[j+5*m],0.0,x[jcn],0.0,&tmp0_hi,&tmp0_lo);
	dd_add(tmp1_hi,tmp1_lo,tmp0_hi,tmp0_lo,&tmp1_hi,&tmp1_lo);
	dd_mul(A[j+6*m],0.0,x[jct],0.0,&tmp0_hi,&tmp0_lo);
	dd_add(tmp1_hi,tmp1_lo,tmp0_hi,tmp0_lo,&tmp1_hi,&tmp1_lo);
	y[j]=tmp1_hi+tmp1_lo;

      }
    }
  }

  return 0;
}
#endif
#ifdef dd_calc_
int calc_dot_local_dd(mpi_prm prm,type* x,type* y,type* tmp){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

  type tmp1_,tmp1;
  tmp1_=(double)0.0;

  type tmp_hi,tmp_lo;
  type tmp1_hi,tmp1_lo;
  tmp_hi=(double)0.0;
  tmp_lo=(double)0.0;
  tmp1_hi=(double)0.0;
  tmp1_lo=(double)0.0;

  // #pragma omp parallel for private(j,jz,jy,jx) reduction(+:tmp1_)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
#pragma ivdep
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	tmp1_=tmp1_+x[j]*y[j];
	
 	dd_mul(x[j],0.0,
	       y[j],0.0,
	       &tmp1_hi,&tmp1_lo);	      
	dd_add(tmp_hi,tmp_lo,
	       tmp1_hi,tmp1_lo,
	       &tmp_hi,&tmp_lo);

      }
    }
  }
  //  tmp[0]=tmp1_;
  tmp[0]=tmp_hi+tmp_lo;
  return 0;
}

int calc_dot2_local_dd(mpi_prm prm,type* x,type* y,type* z,type* tmp1,type* tmp2){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

  type tmp1_,tmp2_;
  tmp1_=(double)0.0;
  tmp2_=(double)0.0;

  type tmp1_h,tmp1_l;
  type tmp2_h,tmp2_l;
  type tmp1_hi,tmp1_lo;
  type tmp2_hi,tmp2_lo;
  tmp1_h=(double)0.0;
  tmp1_l=(double)0.0;
  tmp2_h=(double)0.0;
  tmp2_l=(double)0.0;

  tmp1_hi=(double)0.0;
  tmp1_lo=(double)0.0;
  tmp2_hi=(double)0.0;
  tmp2_lo=(double)0.0;

  // #pragma omp parallel for private(j,jz,jy,jx) reduction(+:tmp1_,tmp2_)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
#pragma ivdep
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	tmp1_=tmp1_+x[j]*y[j];
	tmp2_=tmp2_+x[j]*z[j];

 	dd_mul(x[j],0.0,
	       y[j],0.0,
	       &tmp1_hi,&tmp1_lo);	      
	dd_add(tmp1_h,tmp1_l,
	       tmp1_hi,tmp1_lo,
	       &tmp1_h,&tmp1_l);

 	dd_mul(x[j],0.0,
	       z[j],0.0,
	       &tmp2_hi,&tmp2_lo);	      
	dd_add(tmp2_h,tmp2_l,
	       tmp2_hi,tmp2_lo,
	       &tmp2_h,&tmp2_l);
      }
    }
  }
  //  tmp1[0]=tmp1_;
  //  tmp2[0]=tmp2_;
  
  tmp1[0]=tmp1_h+tmp1_l;
  tmp2[0]=tmp2_h+tmp2_l;
 
  return 0;
}
#endif


// -----
// R[mm][nn]=Q[mm][]*V[nn][]
int gemm_dot_1_local(mpi_prm prm,type* Q,type* V,type* R,int mm,int nn){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy,js,jk,jl;
  type tmp[mm*nn];

  type R_tmp[mm*nn];
  for(jk = 0; jk < mm; jk++) {
    for(js = 0; js < nn; js++) {
      R_tmp[js+jk*nn]=0.0;
    }
  }

#pragma omp parallel private(j,jz,jy,jx,js,jk,tmp) 
  {
  for(jk = 0; jk < mm; jk++) {
    for(js = 0; js < nn; js++) {
      tmp[js+jk*nn]=0.0;
    }
  }
#pragma omp for nowait collapse(2)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jk = 0; jk < mm; jk++) {
	for(js = 0; js < nn; js++) {
	  for(jx = 0; jx < nx; jx++) {
	    j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	    //	    tmp[js+jk*nn]= tmp[js+jk*nn] + Q[j+m*jk]*V[j+m*js];
	    tmp[jk+js*mm]= tmp[jk+js*mm] + Q[j+m*jk]*V[j+m*js];
	  } 
	}
      }	
    }
  }

  for(jk = 0; jk < mm; jk++) {
    for(js = 0; js < nn; js++) {
#pragma omp atomic
      R_tmp[js+jk*nn]=R_tmp[js+jk*nn]+tmp[js+jk*nn];
    }
  }

  }
  //  MPI_Allreduce(R_tmp,R,mm*nn,MPI_TYPE, MPI_SUM,prm.comm);
  for(js = 0; js < mm*nn; js++) {
    R[js]=R_tmp[js];
  }

  /*
  for(jk = 0; jk < mm; jk++) {
    for(js = 0; js < nn; js++) {
      printf(" %d %f \n",js+jk*nn,R[js+jk*nn]);
    }
  }
  */
  return 0;
}

// -----
// V[nn][]=V[nn][] - Q[mm][]*R[mm][nn]
int gemm_dot_2(mpi_prm prm,type* Q,type* V,type* R,int mm,int nn){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy,js,jk,jl;

#pragma omp parallel private(j,jz,jy,jx,js,jk) 
  {
#pragma omp for nowait collapse(3)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
	  for(js = 0; js < nn; js++) {
	    for(jk = 0; jk < mm; jk++) {

	      for(jx = 0; jx < nx; jx++) {
		j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm); 
	      //	      V[j+m*js]= V[j+m*js] - Q[j+m*jk]*R[jk+js*mm];
	      V[j+m*js]= V[j+m*js] - Q[j+m*jk]*R[jk+js*mm];
	    } 
	  }

	}
      }
    }
  }

  for(js = 0; js < nn; js++) {
    for(jk = 0; jk < mm; jk++) {
      //	      V[j+m*js]= V[j+m*js] - Q[j+m*jk]*R[jk+js*mm];
      //      printf(" %d %f \n",jk+js*mm,R[jk+js*mm]);
    } 
  }

  return 0;
}
