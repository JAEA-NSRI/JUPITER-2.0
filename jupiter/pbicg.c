#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "struct.h"
#include "cg.h"

int solve_pre_mat_BiCG1(mpi_prm prm, type *A,type *Dinv,type *r, type *s,type a, type *x, type *y){
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
  nzs=0;
  nze=nz-1;

  int max_threads;
  set_max_threads(nz,&max_threads);

#pragma omp parallel default(none)				\
  shared(stm,nx,ny,nz,m,mx,mxy,A,Dinv,r,s,nzs,nze,a,x,y)	\
  private(nzs_th,nze_th,thid,num_th)				\
  private(j,jcs,jcb,jcn,jct,jx,jy,jz,sum)    
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
	r[j]=a*x[j]+y[j];
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
	  r[j]=a*x[j]+y[j];
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
 
    sum_[0]=0.0;
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
  set_omp_threads(max_threads);
  return 0;
}


int solve_pre_mat_BiCG2(mpi_prm prm, type *A,type *Dinv,type *r, type *s,type a,type b,type *x,type *y){
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
  nzs=0;
  nze=nz-1;

  int max_threads;
  set_max_threads(nz,&max_threads);
#pragma omp parallel default(none)				\
  shared(stm,nx,ny,nz,m,mx,mxy,A,Dinv,r,s,nzs,nze,a,b,x,y)	\
  private(nzs_th,nze_th,thid,num_th)				\
  private(j,jcs,jcb,jcn,jct,jx,jy,jz,sum)    
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
	r[j]=y[j]+b*r[j]+a*x[j];
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
	  r[j]=y[j]+b*r[j]+a*x[j];
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
 
    sum_[0]=0.0;
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
  set_omp_threads(max_threads);
  return 0;
}

//r=b-Ax
//p=r
//c1=(r,r)
int calc_res_dot_cp2(mpi_prm prm, type *A, type *x,type *b,type *r,type *r0,type *p,type *tmp){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;

  type tmp1,tmp1_;
  tmp1_=0.0;
#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) reduction(+:tmp1_)
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
	p[j] =r[j];
	r0[j]=r[j];
	tmp1_=tmp1_+r[j]*r[j];
      }
    }
  }
  tmp1=0.0;
#ifdef JUPITER_MPI
  MPI_Allreduce(&tmp1_, &tmp1, 1,MPI_TYPE, MPI_SUM,prm.comm);
#else
  tmp1=tmp1_;
#endif
  tmp[0]=tmp1;

  return 0;

}

// y[] = A[][]x[]
// dot = (A[][]x[],z)
int MatVec_dot_(mpi_prm prm,type* A,type* x,type* y,type* z,type* tmp){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;
  type tmp1_,tmp1;
  tmp1_=0.0;

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
	tmp1_=tmp1_+y[j]*z[j];
      }

    }
  }

#ifdef use_sleev1
  tmp1=0.0;
#ifdef JUPITER_MPI
  MPI_Allreduce(&tmp1_, &tmp1, 1,MPI_TYPE, MPI_SUM,prm.comm);
#else
  tmp1=tmp1_;
#endif
  tmp[0]=tmp1;
#else
  tmp[0]=tmp1_;
#endif
  return 0;

}


int MatVec_dot_dcomm_(mpi_prm prm,type* A,type* x,type* y,type* z,type* tmp,type *rb_b,type *rb_t,type *rb_s,type *rb_n,type *rb_w,type *rb_e){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;
  type tmp1_,tmp1;
  tmp1_=0.0;

#pragma omp parallel 
  {
#pragma omp master
    {
#ifdef JUPITER_MPI
    sleev_comm_nopack(x,rb_b,rb_t,rb_s,rb_n,rb_w,rb_e,&prm);
#endif
    }
#pragma omp  for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) reduction(+:tmp1_) schedule(dynamic,1)
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
	tmp1_=tmp1_+y[j]*z[j];
      }

    }
  }
  }
  MatVec_dot_sface_(prm,A,z,y,&tmp1_,rb_b,rb_t,rb_s,rb_n,rb_w,rb_e);
  tmp[0]=tmp1_;

  return 0;

}

//   z[] = y[] + a*x[] 
int axpy_out(mpi_prm prm,type a,type* x,type* y,type* z){
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
	z[j]=a*x[j]+y[j];
      }
    }
  }
  return 0;
}

// z[] = y[] + a*x[] 
// dot1  = (z,z0) 
// dot2  = (z,z) 
int axpy_dot2_out(mpi_prm prm,type a,type* x,type* y,type* z,type* z0,type* tmp,type* tmpn){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  type ztmp;
  type tmp1,tmp1_;
  type tmp2,tmp2_;

  type s_reduce[2],r_reduce[2];

  tmp1_=0.0;
  tmp2_=0.0;

  //  start_collection("dot2_out_calc");
#pragma omp parallel for private(j,jz,jy,jx,ztmp)  reduction(+:tmp1_,tmp2_) schedule(static,1)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	z[j]=a*x[j]+y[j];
      }
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	tmp1_=tmp1_+z[j]*z0[j];
	tmp2_=tmp2_+z[j]*z[j];
      }
    }
  }
  //  stop_collection("dot2_out_calc");

  //  start_collection("dot2_out_comm");
  /*
  s_reduce[0]=tmp1_;
  s_reduce[1]=tmp2_;
  r_reduce[0]=0.0;
  r_reduce[1]=0.0;

//  MPI_Allreduce(&s_reduce[0], &r_reduce[0], 1,MPI_TYPE, MPI_SUM,prm.comm); 
//  MPI_Allreduce(&s_reduce[1], &r_reduce[1], 1,MPI_TYPE, MPI_SUM,prm.comm); 

  MPI_Allreduce(s_reduce, r_reduce, 1,MPI_TYPE, MPI_SUM,prm.comm); 
  tmp[0] =r_reduce[0]; 
  tmpn[0]=r_reduce[1];
  */

#ifdef JUPITER_MPI
  tmp1=0.0;
  MPI_Allreduce(&tmp1_, &tmp1, 1,MPI_TYPE, MPI_SUM,prm.comm);
  tmp[0]=tmp1;
  tmp2=0.0;
  MPI_Allreduce(&tmp2_, &tmp2, 1,MPI_TYPE, MPI_SUM,prm.comm);
  tmpn[0]=tmp2;
#else
  tmp1=0.0;
  tmp1=tmp1_;
  tmp[0]=tmp1;
  tmp2=0.0;
  tmp2=tmp2_;
  tmpn[0]=tmp2;
#endif

  //  stop_collection("dot2_out_comm");

  return 0;
}


//   z[] = z[]+ b*y[] + a*x[] 
int axpy_2(mpi_prm prm,type a,type b,type* x,type* y,type* z){
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
	z[j]=z[j]+b*y[j]+a*x[j];
      }
    }
  }
  return 0;
}



/////   w[] = z[]+ b*y[] + a*x[] 
//   y[] = z[]+ b*y[] + a*x[] 
//int axpy_2_out(mpi_prm prm,type a,type b,type* x,type* y,type* z,type* w){
int axpy_2_inout(mpi_prm prm,type a,type b,type* x,type* y,type* z){
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
	//	z[j]=z[j]+b*y[j]+a*x[j];
	//	w[j]=z[j]+b*y[j]+a*x[j];
	y[j]=z[j]+b*y[j]+a*x[j];
      }
    }
  }
  return 0;
}


// y[]  = A[][]x[]
// tmp  = (A[][]x[],z)
// tmpn = (A[][]x[],A[][]x[])
int MatVec_dot2(mpi_prm prm,type* A,type* x,type* y,type* z,type* tmp,type* tmpn){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;
  type tmp1_;
  type tmp2_;
  type s_reduce[2],r_reduce[2];

  tmp1_=0.0;
  tmp2_=0.0;

  //  start_collection("MatVec_dot2_calc");
#pragma omp parallel for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) reduction(+:tmp1_,tmp2_) schedule(static,1)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      /*
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
	tmp1_=tmp1_+y[j]*z[j];
	tmp2_=tmp2_+y[j]*y[j];
      }
      */
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
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	tmp1_=tmp1_+y[j]*z[j];
	tmp2_=tmp2_+y[j]*y[j];
      }

    }
  }
  //  stop_collection("MatVec_dot2_calc");

  //  start_collection("MatVec_dot2_comm");


#ifdef use_sleev2
  s_reduce[0]=tmp1_;
  s_reduce[1]=tmp2_;
  r_reduce[0]=0.0;
  r_reduce[1]=0.0;
#ifdef JUPITER_MPI
  MPI_Allreduce(s_reduce, r_reduce, 2,MPI_TYPE, MPI_SUM,prm.comm); 
#else
  r_reduce[0]=s_reduce[0];
  r_reduce[1]=s_reduce[1];
#endif
  tmp[0] =r_reduce[0]; 
  tmpn[0]=r_reduce[1];
#else
  tmp[0] =tmp1_; 
  tmpn[0]=tmp2_;
#endif
  //  stop_collection("MatVec_dot2_comm");

  return 0;
}

//x --> e
//y --> v
//z --> e_
// v = Ae
// c3 = (e_,v)/(v,v)
//  v = vc +vs --> (v,v)  = ( vc +vs , vc +vs) = (vc,vc) + (vc,vs) + (vs,vc) + (vs,vs) 
//             --> (e_,v) = ( e_, vc +vs) = (e_,vc) + (e_,vs)
int MatVec_dot2_sface(mpi_prm prm,type* A,type* y,type* z,type* tmp,type* tmpn,
		     type *rb_b,type *rb_t,type *rb_s,type *rb_n,type *rb_w,type *rb_e
		     ){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j,j_;
  int jz,jx,jy;
  type tmp1_,tmp1;
  type tmp2_,tmp2;

  tmp1_=tmp[0];
  tmp2_=tmpn[0];
  type q_b,q_t,q_s,q_n,q_w,q_e;

#pragma omp parallel private(j,j_,jz,jy,jx,q_n,q_e,q_w,q_s,q_t,q_b) reduction(+:tmp1_,tmp2_)
  {

  jz = 0;
#pragma omp for schedule(static,1) 
  for(jy = 0; jy < ny; jy++) {
    for(jx = 0; jx < nx; jx++) {
      j_ = jx + nx*jy ;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

      q_b=rb_b[j_]*A[j+0*m];
      tmp1_=tmp1_+2.0*y[j]*q_b+q_b*q_b;
      y[j]=y[j]+q_b;
      tmp2_=tmp2_+q_b*z[j];
    }
  }


  jz  = nz-1;
#pragma omp for schedule(static,1) 
  for(jy = 0; jy < ny; jy++) {
    for(jx = 0; jx < nx; jx++) {
      j_ = jx + nx*jy ;
      j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      q_t=rb_t[j_]*A[j+6*m];
      tmp1_=tmp1_+2.0*y[j]*q_t+q_t*q_t;
      y[j]=y[j]+q_t;
      tmp2_=tmp2_+q_t*z[j];
    }
  }

  jy  = 0;
#pragma omp for schedule(static,1) 
  for(jz = 0; jz < nz; jz++) {
    for(jx = 0; jx < nx; jx++) {
      j_ = jx + nx*jz ;
      j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      q_s=rb_s[j_]*A[j+1*m];
      tmp1_=tmp1_+2.0*y[j]*q_s+q_s*q_s;
      y[j]=y[j]+q_s;
      tmp2_=tmp2_+q_s*z[j];
    }
  }


  jy  = ny-1;
#pragma omp for schedule(static,1) 
  for(jz = 0; jz < nz; jz++) {
    for(jx = 0; jx < nx; jx++) {
      j_ = jx + nx*jz ;
      j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

      q_n=rb_n[j_]*A[j+5*m];
      tmp1_=tmp1_+2.0*y[j]*q_n+q_n*q_n;
      y[j]=y[j]+q_n;
      tmp2_=tmp2_+q_n*z[j];
    }
  }


  jx  = 0;
#pragma omp for schedule(static,1) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      j_ = jy + ny*jz ;
      j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

      q_w=rb_w[j_]*A[j+2*m];
      tmp1_=tmp1_+2.0*y[j]*q_w+q_w*q_w;
      y[j]=y[j]+q_w;
      tmp2_=tmp2_+q_w*z[j];
    }
  }


  jx  = nx-1;
#pragma omp for schedule(static,1) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      j_ = jy + ny*jz ;
      j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

      q_e=rb_e[j_]*A[j+4*m];
      tmp1_=tmp1_+2.0*y[j]*q_e+q_e*q_e;
      y[j]=y[j]+q_e;
      tmp2_=tmp2_+q_e*z[j];
    }
  }

}

#ifdef JUPITER_MPI
  tmp1=0.0;
  MPI_Allreduce(&tmp1_, &tmp1, 1,MPI_TYPE, MPI_SUM,prm.comm);
  tmp[0]=tmp1;

  tmp2=0.0;
  MPI_Allreduce(&tmp2_, &tmp2, 1,MPI_TYPE, MPI_SUM,prm.comm);
  tmpn[0]=tmp2;
#else
  tmp1=0.0;
  tmp1=tmp1_;
  tmp[0]=tmp1;

  tmp2=0.0;
  tmp2=tmp2_;
  tmpn[0]=tmp2;
#endif

  return 0;

}

int MatVec_dot2_dcomm(mpi_prm prm,type* A,type* x,type* y,type* z,type* tmp,type* tmpn,type *rb_b,type *rb_t,type *rb_s,type *rb_n,type *rb_w,type *rb_e){
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;
  int jcc,jcw,jcs,jcb,jce,jcn,jct;
  type tmp1_;
  type tmp2_;
  type s_reduce[2],r_reduce[2];

  tmp1_=0.0;
  tmp2_=0.0;

#pragma omp parallel 
  {
#pragma omp master
    {
#ifdef JUPITER_MPI
      sleev_comm_nopack(x,rb_b,rb_t,rb_s,rb_n,rb_w,rb_e,&prm);
#endif
    }
#pragma omp for private(jcb,jcs,jcw,jcc,jce,jcn,jct,j,jz,jy,jx) reduction(+:tmp1_,tmp2_) schedule(dynamic,1)
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
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
	tmp1_=tmp1_+y[j]*z[j];
	tmp2_=tmp2_+y[j]*y[j];
      }

    }
  }

  }
  MatVec_dot2_sface(prm,A,y,z,&tmp2_,&tmp1_,rb_b,rb_t,rb_s,rb_n,rb_w,rb_e);
  tmp[0] =tmp1_; 
  tmpn[0]=tmp2_;

  return 0;

}

int MatVec_dot_sface_(mpi_prm prm,type* A,type* x,type* y,type* tmp,
		     type *rb_b,type *rb_t,type *rb_s,type *rb_n,type *rb_w,type *rb_e
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
  type q_b,q_t,q_s,q_n,q_w,q_e;

#pragma omp parallel  private(j,j_,jy,jx,jz,q_b)
  {
  jz = 0;
#pragma omp for nowait reduction(+:tmp1_) schedule(dynamic,1) 
  for(jy = 0; jy < ny; jy++) {
    for(jx = 0; jx < nx; jx++) {
      j_ = jx + nx*jy ;
      j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      q_b=rb_b[j_]*A[j+0*m];
      y[j]=y[j]+q_b;
      tmp1_=tmp1_+q_b*x[j];

      //      y[j]=y[j]+q_b[j_];
      //      tmp1_=tmp1_+q_b[j_]*x[j];
    }
  }


  jz  = nz-1;
#pragma omp for nowait reduction(+:tmp1_) schedule(dynamic,1) 
  for(jy = 0; jy < ny; jy++) {
    for(jx = 0; jx < nx; jx++) {
      j_ = jx + nx*jy ;
      j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      q_t=rb_t[j_]*A[j+6*m];
      y[j]=y[j]+q_t;
      tmp1_=tmp1_+q_t*x[j];

      //      y[j]=y[j]+q_t[j_];
      //      tmp1_=tmp1_+q_t[j_]*x[j];
    }
  }

  
  jy  = 0;
#pragma omp for nowait reduction(+:tmp1_) schedule(dynamic,1) 
  for(jz = 0; jz < nz; jz++) {
    for(jx = 0; jx < nx; jx++) {
      j_ = jx + nx*jz ;
      j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      q_s=rb_s[j_]*A[j+1*m];
      y[j]=y[j]+q_s;
      tmp1_=tmp1_+q_s*x[j];

      //      y[j]=y[j]+q_s[j_];
      //      tmp1_=tmp1_+q_s[j_]*x[j];
    }
  }


  jy  = ny-1;
#pragma omp for nowait reduction(+:tmp1_) schedule(dynamic,1) 
  for(jz = 0; jz < nz; jz++) {
    for(jx = 0; jx < nx; jx++) {
      j_ = jx + nx*jz ;
      j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      q_n=rb_n[j_]*A[j+5*m];
      y[j]=y[j]+q_n;
      tmp1_=tmp1_+q_n*x[j];

      //      y[j]=y[j]+q_n[j_];
      //      tmp1_=tmp1_+q_n[j_]*x[j];
    }
  }


  jx  = 0;
#pragma omp for nowait  reduction(+:tmp1_) schedule(dynamic,1) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      j_ = jy + ny*jz ;
      j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
      q_w=rb_w[j_]*A[j+2*m];      
      y[j]=y[j]+q_w;
      tmp1_=tmp1_+q_w*x[j];

      //      y[j]=y[j]+q_w[j_];
      //      tmp1_=tmp1_+q_w[j_]*x[j];
    }
  }


  jx  = nx-1;
#pragma omp for nowait  reduction(+:tmp1_) schedule(dynamic,1) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      j_ = jy + ny*jz ;
      j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

      q_e=rb_e[j_]*A[j+4*m];
      y[j]=y[j]  +q_e;
      tmp1_=tmp1_+q_e*x[j];

      //      y[j]=y[j]+q_e[j_];
      //      tmp1_=tmp1_+q_e[j_]*x[j];

    }
  }

}
  tmp1=0.0;
#ifdef JUPITER_MPI
  MPI_Allreduce(&tmp1_, &tmp1, 1,MPI_TYPE, MPI_SUM,prm.comm);
#else
  tmp1=tmp1_;
#endif
  tmp[0]=tmp1;

  return 0;

}
