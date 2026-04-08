#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include "struct.h"
#include "cg.h"
#ifdef GMP
#include "gmp.h"
#endif

int calc_GramMat_local(mpi_prm prm,
                       type* V,type* Z_,type* W,type* Gkk_1,type* Gkk,
                       int smax)
{
  int stm,stmx;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &stmx, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

  int s_j,s_i;
  type tmp1,tmp2,tmp3;
  int thid;
  int num_th;

#pragma omp parallel
  {
#pragma omp master
    {
      num_th = omp_get_num_threads();
    }
  }

  //  type tmp1_[(num_th)*(smax+1)*(smax+1)],tmp2_[(num_th)*(smax+1)*(smax+1)],tmp3_[(num_th)*(smax+1)*(smax+1)];
  type tmp1_sum[(smax+1)*(smax+1)],tmp2_sum[(smax+1)*(smax+1)],tmp3_sum[(smax+1)*(smax+1)];
  type tmp1_[(smax+1)*(smax+1)],tmp2_[(smax+1)*(smax+1)],tmp3_[(smax+1)*(smax+1)];
  //  for(thid=0; thid<num_th; thid++){
  for(s_i=0; s_i<smax+1; s_i++) {
    for(s_j=0; s_j<smax+1; s_j++) {
      tmp1_sum[s_i*(smax+1)+s_j]=0.0;
      tmp2_sum[s_i*(smax+1)+s_j]=0.0;
      tmp3_sum[s_i*(smax+1)+s_j]=0.0;
    }
  }
  //  }

#pragma omp parallel  private(j,jz,jy,jx,thid,tmp1,tmp2,s_j,s_i,tmp1_,tmp2_,tmp3_)
  {
    thid   = omp_get_thread_num();
    for(s_i=0; s_i<smax+1; s_i++) {
      for(s_j=0; s_j<smax+1; s_j++) {
        tmp1_[s_i*(smax+1)+s_j]=0.0;
        tmp2_[s_i*(smax+1)+s_j]=0.0;
        tmp3_[s_i*(smax+1)+s_j]=0.0;
      }
    }
    //    int itr=0;
#pragma omp for nowait
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        for(s_i=0; s_i<smax-1; s_i++) {
          for(s_j=0; s_j<smax+1; s_j++) {
#ifdef intelopt
#pragma ivdep
#endif
            for(jx = 0; jx < nx; jx++) {
              j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
              tmp1_[s_i*(smax+1)+s_j]=tmp1_[s_i*(smax+1)+s_j]+V[(s_j)*m+j]*Z_[(s_i)*m+j];
              tmp2_[s_i*(smax+1)+s_j]=tmp2_[s_i*(smax+1)+s_j]+V[(s_j)*m+j]*W[(s_i)*m+j];
            }
          }
        }
        s_i=smax-1;
        for(s_j=0; s_j<smax+1; s_j++) {
#ifdef intelopt
#pragma ivdep
#endif
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
            tmp1_[s_i*(smax+1)+s_j]=tmp1_[s_i*(smax+1)+s_j]+V[(s_j)*m+j]*Z_[(s_i)*m+j];
            tmp2_[s_i*(smax+1)+s_j]=tmp2_[s_i*(smax+1)+s_j]+V[(s_j)*m+j]*W[(s_i)*m+j];
            tmp3_[s_i*(smax+1)+s_j]=tmp3_[s_i*(smax+1)+s_j]+V[(s_j)*m+j]*W[(smax)*m+j];
          }
        }

      }
    }

    //  printf("Gram itr=%14d \n",itr);

#pragma omp critical
    {
      for(s_i=0; s_i<smax+1; s_i++) {
        for(s_j=0; s_j<smax+1; s_j++) {
          tmp1_sum[s_i*(smax+1)+s_j]=
            tmp1_sum[s_i*(smax+1)+s_j]+tmp1_[s_i*(smax+1)+s_j];
          tmp2_sum[s_i*(smax+1)+s_j]=
            tmp2_sum[s_i*(smax+1)+s_j]+tmp2_[s_i*(smax+1)+s_j];
          tmp3_sum[s_i*(smax+1)+s_j]=
            tmp3_sum[s_i*(smax+1)+s_j]+tmp3_[s_i*(smax+1)+s_j];
        }
      }
    }
  } // OMP region end


  for(s_j=0; s_j<smax+1; s_j++) {
    for(s_i=0; s_i<smax; s_i++) {
      Gkk_1[(s_i)*(smax+1)+s_j]=tmp1_sum[s_i*(smax+1)+s_j];
      Gkk[(s_j)*(smax+1)+s_i]=tmp2_sum[s_i*(smax+1)+s_j];
    }
    s_i=smax-1;
    Gkk[(s_j)*(smax+1)+smax]=tmp3_sum[s_i*(smax+1)+s_j];
  }

  return 0;
}


int MatVec_BJ_pre(mpi_prm prm, type *A,type *Dinv,type *x,type *r, type *s)
{
  int stm,stmx;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &stmx, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  type sum;
  int j;
  int jz,jx,jy;
  int jcs,jcb,jcn,jct,jcw,jcc,jce;

  int thid,num_th;
  int nzs_th,nze_th,nzs,nze;


  nzs=0;
  nze=nz-1;
#ifdef  poisson

#else
#pragma omp parallel default(none)                         \
  shared(stm,stmx,nx,ny,nz,m,mx,mxy,A,Dinv,x,r,s,nzs,nze)  \
  private(nzs_th,nze_th,thid,num_th)                       \
  private(j,jcs,jcb,jcn,jct,jcw,jcc,jce,jx,jy,jz,sum)
#endif
  {
    thid   = omp_get_thread_num();
    num_th = omp_get_num_threads();
    Sep_region(&thid,&num_th,&nzs,&nze,&nzs_th,&nze_th);

    type sum_[nx+1];

    //  start_collection("matvec_pre_1");
    jz =nzs_th;
    for(jy = 0; jy < ny; jy++) {
      jx=0;
      j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
      jcs = j-mx  ;
      jcb = j-mxy ;
      jcw = j-1;
      jcc = j;
      jce = j+1;
      jcn = j+mx;
      jct = j+mxy;

      for(jx = 0; jx < nx; jx++) {
        r[j]=
          A[j+0*m]*x[jcb]
          +A[j+1*m]*x[jcs]
          +A[j+2*m]*x[jcw]
          +A[j+3*m]*x[jcc]
          +A[j+4*m]*x[jce]
          +A[j+5*m]*x[jcn]
          +A[j+6*m]*x[jct];

        sum_[jx]=r[j]-(A[j+1*m]*s[jcs]);
        j=j+1;
        jcs=jcs+1;
        jcb=jcb+1;

        jcw=jcw+1;
        jcc=jcc+1;
        jce=jce+1;
        jcn=jcn+1;
        jct=jct+1;
      }
      jx=0;
      j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
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
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        jcs = j-mx  ;
        jcb = j-mxy ;
        jcw = j-1;
        jcc = j;
        jce = j+1;
        jcn = j+mx;
        jct = j+mxy;

        for(jx = 0; jx < nx; jx++) {
          r[j]=
            A[j+0*m]*x[jcb]
            +A[j+1*m]*x[jcs]
            +A[j+2*m]*x[jcw]
            +A[j+3*m]*x[jcc]
            +A[j+4*m]*x[jce]
            +A[j+5*m]*x[jcn]
            +A[j+6*m]*x[jct];

          sum_[jx]=r[j]-(A[j+1*m]*s[jcs] + A[j+0*m]*s[jcb]);
          j=j+1;
          jcb=jcb+1;
          jcs=jcs+1;

          jcw=jcw+1;
          jcc=jcc+1;
          jce=jce+1;
          jcn=jcn+1;
          jct=jct+1;
        }

        jx=0;
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        sum=0;
        for(jx = 0; jx < nx; jx++) {
          sum=(sum_[jx]-A[j+2*m]*sum)*Dinv[j];
          s[j]=sum;
          j=j+1;
        }

      }
    }
    // stop_collection("matvec_pre_1");

    //  start_collection("matvec_pre_2");
    sum_[0]=0.0;
    jz =nze_th;
    for(jy = ny-1; jy >=0 ; jy--) {
      jx = nx-1;
      j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
      jcn = j+mx  ;
      jct = j+mxy ;
      for(jx = nx-1; jx >=0 ; jx--) {
        sum_[jx+1] =  A[j+5*m]*s[jcn] ;
        j   = j-1;
        jcn = jcn-1;
        jct = jct-1;
      }
      jx=nx-1;
      j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
      sum=0;
      for(jx = nx-1; jx >=0 ; jx--) {
        sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
        s[j]=sum;
        j=j-1;
      }
    }

    for(jz =nze_th-1;jz>=nzs_th;jz--) {
      for(jy = ny-1; jy >=0 ; jy--) {

        jx = nx-1;
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        jcn = j+mx  ;
        jct = j+mxy ;
        for(jx = nx-1; jx >=0 ; jx--) {
          sum_[jx+1] =  A[j+5*m]*s[jcn] + A[j+6*m]*s[jct];
          j   = j-1;
          jcn = jcn-1;
          jct = jct-1;
        }

        jx=nx-1;
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        sum=0;
        for(jx = nx-1; jx >=0 ; jx--) {
          sum=s[j]-((A[j+4*m]*sum + sum_[jx+1])*Dinv[j]);
          s[j]=sum;
          j=j-1;
        }
      }
    }
    // stop_collection("matvec_pre_2");

  }
  return 0;
}
/*
int calc_GramMat_local_sym(mpi_prm prm,
                           type* V,type* Z_,type* W,type* Gkk_1,type* Gkk,
                           int smax){
  int stm,stmx;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &stmx, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

  int s_j,s_i;
  type tmp1,tmp2;
  int thid;
  int num_th;

#pragma omp parallel
  {
#pragma omp master
    {
      num_th = omp_get_num_threads();
    }
  }

  type tmp1_sum[(smax+1)*(smax+1)],tmp2_sum[(smax+1)*(smax+1)];
  type tmp1_[(smax+1)*(smax+1)],tmp2_[(smax+1)*(smax+1)];
  for(s_i=0; s_i<smax+1; s_i++){
    for(s_j=0; s_j<smax+1; s_j++){
      tmp1_sum[s_i*(smax+1)+s_j]=0.0;
      tmp2_sum[s_i*(smax+1)+s_j]=0.0;
    }
  }
#pragma omp parallel  private(j,jz,jy,jx,thid,tmp1,tmp2,s_j,s_i,tmp1_,tmp2_)
  {
    thid   = omp_get_thread_num();
    for(s_i=0; s_i<smax+1; s_i++){
      for(s_j=0; s_j<smax+1; s_j++){
        tmp1_[s_i*(smax+1)+s_j]=0.0;
        tmp2_[s_i*(smax+1)+s_j]=0.0;
      }
    }

#pragma omp for nowait
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {

        for(s_j=0; s_j<smax+1; s_j++){
          for(s_i=0; s_i<smax; s_i++){
#ifdef intelopt
#pragma ivdep
#endif
            for(jx = 0; jx < nx; jx++) {
              j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
              tmp1_[s_i*(smax+1)+s_j]=tmp1_[s_i*(smax+1)+s_j]+V[(s_j)*m+j]*Z_[(s_i)*m+j];
            }
          }

          for(s_i=s_j; s_i<smax+1; s_i++){
#ifdef intelopt
#pragma ivdep
#endif
            for(jx = 0; jx < nx; jx++) {
              j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
              tmp2_[s_i*(smax+1)+s_j]=tmp2_[s_i*(smax+1)+s_j]+V[(s_j)*m+j]*W[(s_i)*m+j];
            }
          }
        }


      }
    }

#pragma omp critical
    {
      for(s_i=0; s_i<smax+1; s_i++){
        for(s_j=0; s_j<smax+1; s_j++){
          tmp1_sum[s_i*(smax+1)+s_j]=
            tmp1_sum[s_i*(smax+1)+s_j]+tmp1_[s_i*(smax+1)+s_j];
          tmp2_sum[s_i*(smax+1)+s_j]=
            tmp2_sum[s_i*(smax+1)+s_j]+tmp2_[s_i*(smax+1)+s_j];
        }
      }
    }

  } // OMP region end


  for(s_j=0; s_j<smax+1; s_j++){ 
    for(s_i=0; s_i<smax; s_i++){
      Gkk_1[(s_i)*(smax+1)+s_j]=tmp1_sum[s_i*(smax+1)+s_j];
    }
  }
  for(s_j=0; s_j<smax+1; s_j++){ 
    for(s_i=0; s_i<smax+1; s_i++){
      Gkk[(s_j)*(smax+1)+s_i]=tmp2_sum[s_i*(smax+1)+s_j];
    }
  }

  for(s_j=0; s_j<smax+1; s_j++){
    for(s_i=s_j; s_i<smax+1; s_i++){
      //      Gkk[(s_j)*(smax+1)+s_i]=Gkk[(s_i)*(smax+1)+s_j];
      Gkk[(s_i)*(smax+1)+s_j]=Gkk[(s_j)*(smax+1)+s_i];
    }
  }

  return 0;
}
*/
#ifdef  dd_calc_
int calc_GramMat_local_dd(mpi_prm prm,
                          type* V,type* Z_,type* W,type* Gkk_1,type* Gkk,
                          int smax){
  int stm,stmx;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &stmx, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

  int s_j,s_i;
  type tmp1,tmp2,tmp3;
  int thid;
  int num_th;
#pragma omp parallel
  {
#pragma omp master
    {
      num_th = omp_get_num_threads(); 
    }
  }

  type tmp1_[num_th][smax+1][smax+1],tmp2_[num_th][smax+1][smax+1],tmp3_[num_th][smax+1][smax+1];
  type tmp1_h[num_th][smax+1][smax+1],tmp2_h[num_th][smax+1][smax+1],tmp3_h[num_th][smax+1][smax+1];
  type tmp1_l[num_th][smax+1][smax+1],tmp2_l[num_th][smax+1][smax+1],tmp3_l[num_th][smax+1][smax+1];
  type tmp1_hi,tmp1_lo,tmp2_hi,tmp2_lo,tmp3_hi,tmp3_lo;

  for(thid=0; thid<num_th; thid++) {
    for(s_i=0; s_i<smax+1; s_i++){
      for(s_j=0; s_j<smax+1; s_j++) {
        tmp1_[thid][s_i][s_j]=0.0;
        tmp2_[thid][s_i][s_j]=0.0;
        tmp3_[thid][s_i][s_j]=0.0;

        tmp1_h[thid][s_i][s_j]=0.0;
        tmp2_h[thid][s_i][s_j]=0.0;
        tmp3_h[thid][s_i][s_j]=0.0;
        tmp1_l[thid][s_i][s_j]=0.0;
        tmp2_l[thid][s_i][s_j]=0.0;
        tmp3_l[thid][s_i][s_j]=0.0;

      }
    }
  }


#pragma omp parallel  private(j,jz,jy,jx,thid,tmp1,tmp2,s_j,s_i,tmp1_hi,tmp1_lo,tmp2_hi,tmp2_lo,tmp3_hi,tmp3_lo)
  {
    tmp1=0.0;
    tmp2=0.0;
    thid   = omp_get_thread_num();
#pragma omp for
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        for(s_i=0; s_i<smax-1; s_i++){
          for(s_j=0; s_j<smax+1; s_j++){
#ifdef intelopt
#pragma ivdep
#endif
            for(jx = 0; jx < nx; jx++) {
              j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
              tmp1_[thid][s_i][s_j]=tmp1_[thid][s_i][s_j]+V[(s_j)*m+j]*Z_[(s_i)*m+j];
              tmp2_[thid][s_i][s_j]=tmp2_[thid][s_i][s_j]+V[(s_j)*m+j]*W[(s_i)*m+j];

              dd_mul(V[(s_j)*m+j],0.0,
                     Z_[(s_i)*m+j],0.0,
                     &tmp1_hi,&tmp1_lo);
              dd_add(tmp1_h[thid][s_i][s_j],tmp1_l[thid][s_i][s_j],
                     tmp1_hi,tmp1_lo,
                     &tmp1_h[thid][s_i][s_j],&tmp1_l[thid][s_i][s_j]);

              dd_mul(V[(s_j)*m+j],0.0,
                     W[(s_i)*m+j],0.0,
                     &tmp2_hi,&tmp2_lo);
              dd_add(tmp2_h[thid][s_i][s_j],tmp2_l[thid][s_i][s_j],
                     tmp2_hi,tmp2_lo,
                     &tmp2_h[thid][s_i][s_j],&tmp2_l[thid][s_i][s_j]);
            }
          }
        }
        s_i=smax-1;
        for(s_j=0; s_j<smax+1; s_j++){
#ifdef intelopt
#pragma ivdep
#endif
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
            tmp1_[thid][s_i][s_j]=tmp1_[thid][s_i][s_j]+V[(s_j)*m+j]*Z_[(s_i)*m+j];
            tmp2_[thid][s_i][s_j]=tmp2_[thid][s_i][s_j]+V[(s_j)*m+j]*W[(s_i)*m+j];
            tmp3_[thid][s_i][s_j]=tmp3_[thid][s_i][s_j]+V[(s_j)*m+j]*W[(smax)*m+j];

            dd_mul(V[(s_j)*m+j],0.0,
                   Z_[(s_i)*m+j],0.0,
                   &tmp1_hi,&tmp1_lo);
            dd_add(tmp1_h[thid][s_i][s_j],tmp1_l[thid][s_i][s_j],
                   tmp1_hi,tmp1_lo,
                   &tmp1_h[thid][s_i][s_j],&tmp1_l[thid][s_i][s_j]);

            dd_mul(V[(s_j)*m+j],0.0,
                   W[(s_i)*m+j],0.0,
                   &tmp2_hi,&tmp2_lo);
            dd_add(tmp2_h[thid][s_i][s_j],tmp2_l[thid][s_i][s_j],
                   tmp2_hi,tmp2_lo,
                   &tmp2_h[thid][s_i][s_j],&tmp2_l[thid][s_i][s_j]);

            dd_mul(V[(s_j)*m+j],0.0,
                   W[(smax)*m+j],0.0,
                   &tmp3_hi,&tmp3_lo);
            dd_add(tmp3_h[thid][s_i][s_j],tmp3_l[thid][s_i][s_j],
                   tmp3_hi,tmp3_lo,
                   &tmp3_h[thid][s_i][s_j],&tmp3_l[thid][s_i][s_j]);

          }
        }

      }
    }
  }

  for(thid=1; thid<num_th; thid++) {
    for(s_j=0; s_j<smax+1; s_j++) {
      //      for(s_i=0; s_i<smax; s_i++){
      for(s_i=0; s_i<smax+1; s_i++){
        dd_add(tmp1_h[0][s_i][s_j],tmp1_l[0][s_i][s_j],
               tmp1_h[thid][s_i][s_j],tmp1_l[thid][s_i][s_j],
               &tmp1_h[0][s_i][s_j],&tmp1_l[0][s_i][s_j]);

        dd_add(tmp2_h[0][s_i][s_j],tmp2_l[0][s_i][s_j],
               tmp2_h[thid][s_i][s_j],tmp2_l[thid][s_i][s_j],
               &tmp2_h[0][s_i][s_j],&tmp2_l[0][s_i][s_j]);

        dd_add(tmp3_h[0][s_i][s_j],tmp3_l[0][s_i][s_j],
               tmp3_h[thid][s_i][s_j],tmp3_l[thid][s_i][s_j],
               &tmp3_h[0][s_i][s_j],&tmp3_l[0][s_i][s_j]);
      }
    }
  }
  for(s_j=0; s_j<smax+1; s_j++){
    //    for(s_i=0; s_i<smax; s_i++){
    for(s_i=0; s_i<smax+1; s_i++){
      //            printf("dd Gram = %30.20e, %30.20e, %30.20e \n",
      //                   tmp1_h[0][s_i][s_j]+tmp1_l[0][s_i][s_j],tmp1_[0][s_i][s_j],
      //                   (tmp1_h[0][s_i][s_j]+tmp1_l[0][s_i][s_j])-tmp1_[0][s_i][s_j]);
      tmp1_[0][s_i][s_j]=tmp1_h[0][s_i][s_j]+tmp1_l[0][s_i][s_j];
      tmp2_[0][s_i][s_j]=tmp2_h[0][s_i][s_j]+tmp2_l[0][s_i][s_j];
      tmp3_[0][s_i][s_j]=tmp3_h[0][s_i][s_j]+tmp3_l[0][s_i][s_j];
    }
  }

  for(s_j=0; s_j<smax+1; s_j++){
    for(s_i=0; s_i<smax; s_i++){
      Gkk_1[(s_i)*(smax+1)+s_j]=0.0;
      Gkk[(s_j)*(smax+1)+s_i]=0.0;
    }
    s_i=smax-1;
    Gkk[(s_j)*(smax+1)+smax]=0.0;
  }
  //  for(thid=0; thid<num_th; thid++){
  thid=0;
  {
    for(s_j=0; s_j<smax+1; s_j++){
      for(s_i=0; s_i<smax; s_i++){
        // Gkk_1[(s_i)*(smax+1)+s_j]=Gkk_1[(s_i)*(smax+1)+s_j]+tmp1_[thid][s_i][s_j];
        // Gkk[(s_j)*(smax+1)+s_i]=Gkk[(s_j)*(smax+1)+s_i]+tmp2_[thid][s_i][s_j];

        Gkk_1[(s_i)*(smax+1)+s_j]=tmp1_[thid][s_i][s_j];
        Gkk[(s_j)*(smax+1)+s_i]=tmp2_[thid][s_i][s_j];
      }
      s_i=smax-1;
      // Gkk[(s_j)*(smax+1)+smax]=Gkk[(s_j)*(smax+1)+smax]+tmp3_[thid][s_i][s_j];
      Gkk[(s_j)*(smax+1)+smax]=tmp3_[thid][s_i][s_j];
    }
  }


  return 0;
}
#endif
/*

// -------
// ブロックの端数処理は未実装
int calc_GramMat_local_block(mpi_prm prm,
                             type* V,type* Z_,type* W,type* Gkk_1,type* Gkk,
                             int smax){
  int stm,stmx;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &stmx, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

  int s_j,s_i;
  type tmp1,tmp2,tmp3;
  int thid;
  int num_th;

#pragma omp parallel
  {
#pragma omp master
    {
      num_th = omp_get_num_threads(); 
    }
  }

  //  type tmp1_[(num_th)*(smax+1)*(smax+1)],tmp2_[(num_th)*(smax+1)*(smax+1)],tmp3_[(num_th)*(smax+1)*(smax+1)];
  type tmp1_sum[(smax+1)*(smax+1)],tmp2_sum[(smax+1)*(smax+1)],tmp3_sum[(smax+1)*(smax+1)];
  type tmp1_[(smax+1)*(smax+1)],tmp2_[(smax+1)*(smax+1)],tmp3_[(smax+1)*(smax+1)];
  //  for(thid=0; thid<num_th; thid++){ 
  for(s_i=0; s_i<smax+1; s_i++){
    for(s_j=0; s_j<smax+1; s_j++){ 
      tmp1_sum[s_i*(smax+1)+s_j]=0.0;
      tmp2_sum[s_i*(smax+1)+s_j]=0.0;
      tmp3_sum[s_i*(smax+1)+s_j]=0.0;
    }
  }
  //  }

#pragma omp parallel  private(j,jz,jy,jx,thid,tmp1,tmp2,s_j,s_i,tmp1_,tmp2_,tmp3_)
  {
    thid   = omp_get_thread_num(); 
    for(s_i=0; s_i<smax+1; s_i++){
      for(s_j=0; s_j<smax+1; s_j++){ 
        tmp1_[s_i*(smax+1)+s_j]=0.0;
        tmp2_[s_i*(smax+1)+s_j]=0.0;
        tmp3_[s_i*(smax+1)+s_j]=0.0;
      }
    }
    //    int itr=0;
#pragma omp for nowait
    for(jz = 0; jz < nz; jz++) {

      int jyblock;
      int Nyblock=ny/4;
      for(jyblock=0;jyblock<ny;jyblock=jyblock+Nyblock){   
        for(s_i=0; s_i<smax-1; s_i++){
          for(jy = jyblock; jy < jyblock+Nyblock;jy++) {
            for(s_j=0; s_j<smax+1; s_j++){
              for(jx = 0; jx < nx; jx++) {
                j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
                tmp1_[s_i*(smax+1)+s_j]=tmp1_[s_i*(smax+1)+s_j]+V[(s_j)*m+j]*Z_[(s_i)*m+j];
                tmp2_[s_i*(smax+1)+s_j]=tmp2_[s_i*(smax+1)+s_j]+V[(s_j)*m+j]*W[(s_i)*m+j];
              }
            }
          }
        }
        s_i=smax-1;
        for(jy = jyblock; jy < jyblock+Nyblock;jy++) {
          for(s_j=0; s_j<smax+1; s_j++){
            for(jx = 0; jx < nx; jx++) {
              j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
              tmp1_[s_i*(smax+1)+s_j]=tmp1_[s_i*(smax+1)+s_j]+V[(s_j)*m+j]*Z_[(s_i)*m+j];
              tmp2_[s_i*(smax+1)+s_j]=tmp2_[s_i*(smax+1)+s_j]+V[(s_j)*m+j]*W[(s_i)*m+j];
              tmp3_[s_i*(smax+1)+s_j]=tmp3_[s_i*(smax+1)+s_j]+V[(s_j)*m+j]*W[(smax)*m+j];
            }
          }
        }
      }
    }
  
    //  printf("Gram itr=%14d \n",itr);

#pragma omp critical
    {
      for(s_i=0; s_i<smax+1; s_i++){
        for(s_j=0; s_j<smax+1; s_j++){
          tmp1_sum[s_i*(smax+1)+s_j]=
            tmp1_sum[s_i*(smax+1)+s_j]+tmp1_[s_i*(smax+1)+s_j];
          tmp2_sum[s_i*(smax+1)+s_j]=
            tmp2_sum[s_i*(smax+1)+s_j]+tmp2_[s_i*(smax+1)+s_j];
          tmp3_sum[s_i*(smax+1)+s_j]=
            tmp3_sum[s_i*(smax+1)+s_j]+tmp3_[s_i*(smax+1)+s_j];
        }
      }
    }
  } // OMP region end


  for(s_j=0; s_j<smax+1; s_j++){ 
    for(s_i=0; s_i<smax; s_i++){
      Gkk_1[(s_i)*(smax+1)+s_j]=tmp1_sum[s_i*(smax+1)+s_j];
      Gkk[(s_j)*(smax+1)+s_i]=tmp2_sum[s_i*(smax+1)+s_j];      
    }
    s_i=smax-1;
    Gkk[(s_j)*(smax+1)+smax]=tmp3_sum[s_i*(smax+1)+s_j];
  }

  return 0;

}
*/
int pre_inner(int k,int smax,
              type *rhogamma_inv, type *rhogamma_inv_,
              type *Gk,
              type *TBsk,
              type *B, type *T, type *T_, type *e1e1,
              type *rho_, type *myu_,
              type *TBdsk,
              type *dsk,type *dsk_1, type *dsk_2,
              type *gsk,type *gsk_1, type *gsk_2,
              type *Tdsk, type *Bdsk,
              type *Gkgsk, type *Gkdsk,
              type rho,type rho0,type gamma0,
              type myu_1,type myu_2,type myu,
              type nyu_1,type nyu,
              type gamma_1,type gamma_2,type gamma,
              type *dsk_new0,type *rho_new0,type *gamma_new0,
              type *rho_new,type *gamma_new,
              type *rho0_new,type *gamma0_new,
              type *myu_1_new,type *myu_2_new,type *myu_new,
              type *nyu_1_new,type *nyu_new,
              type *gamma_1_new,type *gamma_2_new
              )
{
  int j;
  int s;
  int s_i,s_j;

  for(j=1;j<smax+1;j=j+1){
    for(s=0; s<2*smax+1; s++){
      dsk_2[s]=dsk_1[s];
      dsk_1[s]=dsk[s];
      gsk_2[s]=gsk_1[s];
      gsk_1[s]=gsk[s];
    }

    // -------
    //　Mononial basisであるため代入処理
    for(s_i=0; s_i<smax; s_i++){
      T_[smax*(smax)+s_i]=0.0;
    }
    T_[smax*(smax)+j-1]=-rhogamma_inv[(j-1)*(smax)+j-1];

    if(j==1){
      for(s=0; s<2*smax+1; s++){
        gsk[s]=0.0;
      }
      gsk[smax]=1.0;
    }else{
      for(s=0; s<2*smax+1; s++){
        gsk[s]=
          -rho*gamma*dsk_1[s]
          +rho*gsk_1[s]
          +(1.0-rho)*gsk_2[s] ;
      }
    }
    //---------------------
    if(j==1){
      for(s=0; s<2*smax+1; s++){
        dsk[s]=0.0;
      }
      dsk[smax]  =B[0*smax+0]; // Bk(1,1)
      dsk[smax+1]=B[1*smax+0]; // Bk(2,1)

      for(s=0; s<2*smax+1; s++){
        TBdsk[s]=0.0;
      }
    }else{
      for(s=0; s<2*smax+1; s++){
        dsk[s]=0.0;
      }
      ////////////////////////////////////////////////////////////
      // dn = rhoprev*dp + (1-rhoprev)*dpp;
      for(s_i=0;s_i<2*smax+1;s_i++){
        dsk[s_i]=rho*dsk_1[s_i]+(1.0-rho)*dsk_2[s_i];
      }

      // dn = dn - rhoprev*gammaprev*(vertcat(Tprev_u(1:s,:)*dp(1:s,:), (-dp(s)/(rhosk*gammask))*e1 ));
      for(s_i=0; s_i<2*smax+1; s_i++){
        for(s_j=0; s_j<2*smax+1; s_j++){
          TBsk[s_i*(2*smax+1)+s_j]=0.0;
        }
      }

      for(s_i=0; s_i<smax; s_i++){
        for(s_j=0; s_j<smax; s_j++){
          TBsk[s_i*(2*smax+1)+s_j]=T[s_i*smax+s_j];
        }
      }

      for(s_i=smax; s_i<2*smax+1; s_i++){
        for(s_j=smax; s_j<2*smax+1; s_j++){
          TBsk[s_i*(2*smax+1)+s_j-1]=-e1e1[(s_i-smax)*(smax+1)+s_j-smax]/(rho0*gamma0);
        }
      }

      for(s_i=0; s_i<2*smax+1; s_i++){
        TBdsk[s_i]=0.0;
        for(s_j=0; s_j<2*smax+1; s_j++){
          TBdsk[s_i]=TBdsk[s_i]+TBsk[s_i*(2*smax+1)+s_j]*dsk_1[s_j];
        }
      }

      for(s_i=0;s_i<2*smax+1;s_i++){
        dsk[s_i]=dsk[s_i] - rho*gamma*TBdsk[s_i];
      }

      // dn = dn - rhoprev*gammaprev*(vertcat(zeros(s,1), B1_u*dp(s+1:2*s,:)));
      for(s_i=0; s_i<2*smax+1; s_i++){
        for(s_j=0; s_j<2*smax+1; s_j++){
          TBsk[s_i*(2*smax+1)+s_j]=0.0;
        }
      }
      for(s_i=smax; s_i<2*smax+1; s_i++){
        for(s_j=smax; s_j<2*smax; s_j++){
          TBsk[s_i*(2*smax+1)+s_j]=B[(s_i-smax)*smax+s_j-smax];
        }
      }

      for(s_i=0; s_i<2*smax+1; s_i++){
        TBdsk[s_i]=0.0;
        for(s_j=0; s_j<2*smax+1; s_j++){
          TBdsk[s_i]=TBdsk[s_i]+TBsk[s_i*(2*smax+1)+s_j]*dsk_1[s_j];
        }
      }

      for(s_i=0;s_i<2*smax+1;s_i++){
        dsk[s_i]=dsk[s_i] - rho*gamma*TBdsk[s_i];
      }
    }

    myu_2=myu_1;
    myu_1=myu;
    for(s_i=0; s_i<2*smax+1; s_i++){
      Gkgsk[s_i]=0.0;
      for(s_j=0; s_j<2*smax+1; s_j++){
        Gkgsk[s_i]= Gkgsk[s_i]+Gk[s_i*(2*smax+1)+s_j]*gsk[s_j];
      }
    }
    myu=0.0;
    for(s=0; s<2*smax+1; s++){
      myu=myu+Gkgsk[s]*gsk[s];
    }

    //次のループは前のループとループ融合が可能
    for(s_i=0; s_i<2*smax+1; s_i++){
      Gkdsk[s_i]=0.0;
      for(s_j=0; s_j<2*smax+1; s_j++){
        Gkdsk[s_i]= Gkdsk[s_i]+Gk[s_i*(2*smax+1)+s_j]*dsk[s_j];
      }
    }

    nyu_1=nyu;
    nyu=0.0;
    for(s=0; s<2*smax+1; s++){
      nyu=nyu+Gkdsk[s]*gsk[s];
    }

    gamma_2=gamma_1;
    gamma_1=gamma;
    gamma=myu/nyu;
    if( (smax*k+j) == 1){
      rho=1.0;
    }else{
      rho=(1.0-gamma/gamma_1*myu/myu_1/rho);
      rho=1.0/rho;
    }

    myu_[j-1]=myu;
    rho_[j-1]=rho;
    rhogamma_inv_[(j-1)*(smax)+j-1]=rho*gamma;

    for(s_i=0; s_i<2*smax+1; s_i++){
      dsk_new0[(j-1)*(2*smax+1)+s_i]=dsk[s_i];
    }
    rho_new0[j-1]=rho;
    gamma_new0[j-1]=gamma;
  }

  rho_new[0]     = rho;
  rho0_new[0]    = rho;
  gamma_new[0]   = gamma;
  gamma0_new[0]  = gamma;
  myu_1_new[0]   = myu_1;
  myu_2_new[0]   = myu_2;
  myu_new[0]     = myu;
  nyu_1_new[0]   = nyu_1;
  nyu_new[0]     = nyu;
  gamma_1_new[0] = gamma_1;
  gamma_2_new[0] = gamma_2;

  for(s_i=0; s_i<smax; s_i++){
    rhogamma_inv[s_i*smax+s_i]=1.0/rhogamma_inv_[s_i*smax+s_i];
  }

  return 0;
}


#ifdef dd_calc_
int pre_inner_dd(int k,int smax,
                 type *rhogamma_inv, type *rhogamma_inv_,
                 type *Gk,
                 type *TBsk,
                 type *B, type *T, type *T_, type *e1e1,
                 type *rho_, type *myu_,
                 type *TBdsk,
                 type *dsk,type *dsk_1, type *dsk_2,
                 type *gsk,type *gsk_1, type *gsk_2,
                 type *Tdsk, type *Bdsk,
                 type *Gkgsk, type *Gkdsk,
                 type rho,type rho0,type gamma0,
                 type myu_1,type myu_2,type myu,
                 type nyu_1,type nyu,
                 type gamma_1,type gamma_2,type gamma,
                 type *dsk_new0,type *rho_new0,type *gamma_new0,
                 type *rho_new,type *gamma_new,
                 type *rho0_new,type *gamma0_new,
                 type *myu_1_new,type *myu_2_new,type *myu_new,
                 type *nyu_1_new,type *nyu_new,
                 type *gamma_1_new,type *gamma_2_new
                 )
{
  int j;
  int s;
  int s_i,s_j;

  for(j=1;j<smax+1;j=j+1){
    for(s=0; s<2*smax+1; s++){ 
      dsk_2[s]=dsk_1[s]; 
      dsk_1[s]=dsk[s]; 
      gsk_2[s]=gsk_1[s]; 
      gsk_1[s]=gsk[s]; 
    } 

    // -------
    //　Mononial basisであるため代入処理
    for(s_i=0; s_i<smax; s_i++){
      T_[smax*(smax)+s_i]=0.0;
    }
    T_[smax*(smax)+j-1]=-rhogamma_inv[(j-1)*(smax)+j-1];

    if(j==1){
      for(s=0; s<2*smax+1; s++){
        gsk[s]=0.0;
      }
      gsk[smax]=1.0;
    }else{
      for(s=0; s<2*smax+1; s++){
        gsk[s]=
          -rho*gamma*dsk_1[s]
          +rho*gsk_1[s]
          +(1.0-rho)*gsk_2[s] ;
      }
    }
    //---------------------
    if(j==1){
      for(s=0; s<2*smax+1; s++){
        dsk[s]=0.0;
      }
      dsk[smax]  =B[0*smax+0]; // Bk(1,1)
      dsk[smax+1]=B[1*smax+0]; // Bk(2,1)

      for(s=0; s<2*smax+1; s++){
        TBdsk[s]=0.0;
      }
    }else{
      for(s=0; s<2*smax+1; s++){
        dsk[s]=0.0;
      }
      ////////////////////////////////////////////////////////////
      // dn = rhoprev*dp + (1-rhoprev)*dpp;
      for(s_i=0;s_i<2*smax+1;s_i++){
        dsk[s_i]=rho*dsk_1[s_i]+(1.0-rho)*dsk_2[s_i];
      }

      // dn = dn - rhoprev*gammaprev*(vertcat(Tprev_u(1:s,:)*dp(1:s,:), (-dp(s)/(rhosk*gammask))*e1 ));
      for(s_i=0; s_i<2*smax+1; s_i++){
        for(s_j=0; s_j<2*smax+1; s_j++){
          TBsk[s_i*(2*smax+1)+s_j]=0.0;
        }
      }

      for(s_i=0; s_i<smax; s_i++){
        for(s_j=0; s_j<smax; s_j++){
          TBsk[s_i*(2*smax+1)+s_j]=T[s_i*smax+s_j];
        }
      }

      for(s_i=smax; s_i<2*smax+1; s_i++){
        for(s_j=smax; s_j<2*smax+1; s_j++){
          TBsk[s_i*(2*smax+1)+s_j-1]=-e1e1[(s_i-smax)*(smax+1)+s_j-smax]/(rho0*gamma0);
        }
      }

      for(s_i=0; s_i<2*smax+1; s_i++){
        TBdsk[s_i]=0.0;
        for(s_j=0; s_j<2*smax+1; s_j++){
          TBdsk[s_i]=TBdsk[s_i]+TBsk[s_i*(2*smax+1)+s_j]*dsk_1[s_j];
        }
      }

      for(s_i=0;s_i<2*smax+1;s_i++){
        dsk[s_i]=dsk[s_i] - rho*gamma*TBdsk[s_i];
      }

      // dn = dn - rhoprev*gammaprev*(vertcat(zeros(s,1), B1_u*dp(s+1:2*s,:)));
      for(s_i=0; s_i<2*smax+1; s_i++){
        for(s_j=0; s_j<2*smax+1; s_j++){
          TBsk[s_i*(2*smax+1)+s_j]=0.0;
        }
      }
      for(s_i=smax; s_i<2*smax+1; s_i++){
        for(s_j=smax; s_j<2*smax; s_j++){
          TBsk[s_i*(2*smax+1)+s_j]=B[(s_i-smax)*smax+s_j-smax];
        }
      }

      for(s_i=0; s_i<2*smax+1; s_i++){
        TBdsk[s_i]=0.0;
        for(s_j=0; s_j<2*smax+1; s_j++){
          TBdsk[s_i]=TBdsk[s_i]+TBsk[s_i*(2*smax+1)+s_j]*dsk_1[s_j];
        }
      }

      for(s_i=0;s_i<2*smax+1;s_i++){
        dsk[s_i]=dsk[s_i] - rho*gamma*TBdsk[s_i];
      }
    }

    myu_2=myu_1;
    myu_1=myu;

    type tmp1_hi,tmp1_lo;

    type Gkgsk_hi[2*smax+1],Gkgsk_lo[2*smax+1];
    type myu_hi,myu_lo;
    for(s_i=0; s_i<2*smax+1; s_i++){
      Gkgsk[s_i]=0.0;
      Gkgsk_hi[s_i]=0.0;
      Gkgsk_lo[s_i]=0.0;
      for(s_j=0; s_j<2*smax+1; s_j++){
        Gkgsk[s_i]= Gkgsk[s_i]+Gk[s_i*(2*smax+1)+s_j]*gsk[s_j];
        dd_mul(Gk[s_i*(2*smax+1)+s_j],0.0,
               gsk[s_j],0.0,
               &tmp1_hi,&tmp1_lo);
        dd_add(tmp1_hi,tmp1_lo,
               Gkgsk_hi[s_i], Gkgsk_lo[s_i],
               &Gkgsk_hi[s_i],&Gkgsk_lo[s_i]);
      }
    }
    myu=0.0;
    myu_hi=0.0;
    myu_lo=0.0;
    for(s=0; s<2*smax+1; s++){
      myu=myu+Gkgsk[s]*gsk[s];
      dd_mul(Gkgsk_hi[s],Gkgsk_lo[s],
             gsk[s],0.0,
             &tmp1_hi,&tmp1_lo);
      dd_add(tmp1_hi,tmp1_lo,
             myu_hi,myu_lo,
             &myu_hi,&myu_lo);
    }


    //次のループは前のループとループ融合が可能
    type nyu_hi,nyu_lo;
    type Gkdsk_hi[2*smax+1],Gkdsk_lo[2*smax+1];
    for(s_i=0; s_i<2*smax+1; s_i++){
      Gkdsk[s_i]=0.0;
      Gkdsk_hi[s_i]=0.0;
      Gkdsk_lo[s_i]=0.0;
      for(s_j=0; s_j<2*smax+1; s_j++){
        Gkdsk[s_i]= Gkdsk[s_i]+Gk[s_i*(2*smax+1)+s_j]*dsk[s_j];

        dd_mul(Gk[s_i*(2*smax+1)+s_j],0.0,
               dsk[s_j],0.0,
               &tmp1_hi,&tmp1_lo);
        dd_add(tmp1_hi,tmp1_lo,
               Gkdsk_hi[s_i],Gkdsk_lo[s_i],
               &Gkdsk_hi[s_i],&Gkdsk_lo[s_i]);
      }
    }

    nyu_1=nyu;
    nyu=0.0;
    nyu_hi=0.0;
    nyu_lo=0.0;
    for(s=0; s<2*smax+1; s++){
      nyu=nyu+Gkdsk[s]*gsk[s];
      dd_mul(Gkdsk_hi[s],Gkdsk_lo[s],
             gsk[s],0.0,
             &tmp1_hi,&tmp1_lo);
      dd_add(tmp1_hi,tmp1_lo,
             nyu_hi,nyu_lo,
             &nyu_hi,&nyu_lo);
    }
    /*
      printf("myu = %20.16e,%20.16e,%20.16e\n"
      ,myu,myu_hi+myu_lo,myu-(myu_hi+myu_lo));
      printf("nyu = %20.16e,%20.16e,%20.16e\n"
      ,nyu,nyu_hi+nyu_lo,nyu-(nyu_hi+nyu_lo));
    */
    nyu=nyu_hi+nyu_lo;
    myu=myu_hi+myu_lo;
    gamma_2=gamma_1;
    gamma_1=gamma;
    gamma=myu/nyu;
    if( (smax*k+j) == 1){
      rho=1.0;
    }else{
      rho=(1.0-gamma/gamma_1*myu/myu_1/rho);
      rho=1.0/rho;
    }

    myu_[j-1]=myu;
    rho_[j-1]=rho;
    rhogamma_inv_[(j-1)*(smax)+j-1]=rho*gamma;

    for(s_i=0; s_i<2*smax+1; s_i++){
      dsk_new0[(j-1)*(2*smax+1)+s_i]=dsk[s_i];
    }
    rho_new0[j-1]=rho;
    gamma_new0[j-1]=gamma;
  }

  rho_new[0]     = rho;
  rho0_new[0]    = rho;
  gamma_new[0]   = gamma;
  gamma0_new[0]  = gamma;
  myu_1_new[0]   = myu_1;
  myu_2_new[0]   = myu_2;
  myu_new[0]     = myu;
  nyu_1_new[0]   = nyu_1;
  nyu_new[0]     = nyu;
  gamma_1_new[0] = gamma_1;
  gamma_2_new[0] = gamma_2;

  for(s_i=0; s_i<smax; s_i++){
    rhogamma_inv[s_i*smax+s_i]=1.0/rhogamma_inv_[s_i*smax+s_i];
  }

  return 0;
}
#endif

#ifdef GMP
int pre_inner_GMP(int k,int smax,
                  type *rhogamma_inv, type *rhogamma_inv_,
                  type *Gk,
                  type *TBsk,
                  type *B, type *T, type *T_, type *e1e1,
                  type *rho_, type *myu_,
                  type *TBdsk,
                  type *dsk,type *dsk_1, type *dsk_2,
                  type *gsk,type *gsk_1, type *gsk_2,
                  type *Tdsk, type *Bdsk,
                  type *Gkgsk, type *Gkdsk,
                  type rho,type rho0,type gamma0,
                  type myu_1,type myu_2,type myu,
                  type nyu_1,type nyu,
                  type gamma_1,type gamma_2,type gamma,
                  type *dsk_new0,type *rho_new0,type *gamma_new0,
                  type *rho_new,type *gamma_new,
                  type *rho0_new,type *gamma0_new,
                  type *myu_1_new,type *myu_2_new,type *myu_new,
                  type *nyu_1_new,type *nyu_new,
                  type *gamma_1_new,type *gamma_2_new
                  )
{
  int j;
  int s;
  int s_i,s_j;

  for(j=1;j<smax+1;j=j+1){
    for(s=0; s<2*smax+1; s++){ 
      dsk_2[s]=dsk_1[s]; 
      dsk_1[s]=dsk[s]; 
      gsk_2[s]=gsk_1[s]; 
      gsk_1[s]=gsk[s]; 
    } 

    // -------
    //　Mononial basisであるため代入処理
    for(s_i=0; s_i<smax; s_i++){
      T_[smax*(smax)+s_i]=0.0;
    }
    T_[smax*(smax)+j-1]=-rhogamma_inv[(j-1)*(smax)+j-1];

    if(j==1){
      for(s=0; s<2*smax+1; s++){
        gsk[s]=0.0;
      }
      gsk[smax]=1.0;
    }else{
      for(s=0; s<2*smax+1; s++){
        gsk[s]=
          -rho*gamma*dsk_1[s]
          +rho*gsk_1[s]
          +(1.0-rho)*gsk_2[s] ;
      }
    }
    //---------------------
    if(j==1){
      for(s=0; s<2*smax+1; s++){
        dsk[s]=0.0;
      }
      dsk[smax]  =B[0*smax+0]; // Bk(1,1)
      dsk[smax+1]=B[1*smax+0]; // Bk(2,1)

      for(s=0; s<2*smax+1; s++){
        TBdsk[s]=0.0;
      }
    }else{
      for(s=0; s<2*smax+1; s++){
        dsk[s]=0.0;
      }
      ////////////////////////////////////////////////////////////
      // dn = rhoprev*dp + (1-rhoprev)*dpp;
      for(s_i=0;s_i<2*smax+1;s_i++){
        dsk[s_i]=rho*dsk_1[s_i]+(1.0-rho)*dsk_2[s_i];
      }

      // dn = dn - rhoprev*gammaprev*(vertcat(Tprev_u(1:s,:)*dp(1:s,:), (-dp(s)/(rhosk*gammask))*e1 ));
      for(s_i=0; s_i<2*smax+1; s_i++){
        for(s_j=0; s_j<2*smax+1; s_j++){
          TBsk[s_i*(2*smax+1)+s_j]=0.0;
        }
      }

      for(s_i=0; s_i<smax; s_i++){
        for(s_j=0; s_j<smax; s_j++){
          TBsk[s_i*(2*smax+1)+s_j]=T[s_i*smax+s_j];
        }
      }

      for(s_i=smax; s_i<2*smax+1; s_i++){
        for(s_j=smax; s_j<2*smax+1; s_j++){
          TBsk[s_i*(2*smax+1)+s_j-1]=-e1e1[(s_i-smax)*(smax+1)+s_j-smax]/(rho0*gamma0);
        }
      }

      for(s_i=0; s_i<2*smax+1; s_i++){
        TBdsk[s_i]=0.0;
        for(s_j=0; s_j<2*smax+1; s_j++){
          TBdsk[s_i]=TBdsk[s_i]+TBsk[s_i*(2*smax+1)+s_j]*dsk_1[s_j];
        }
      }

      for(s_i=0;s_i<2*smax+1;s_i++){
        dsk[s_i]=dsk[s_i] - rho*gamma*TBdsk[s_i];
      }

      // dn = dn - rhoprev*gammaprev*(vertcat(zeros(s,1), B1_u*dp(s+1:2*s,:)));
      for(s_i=0; s_i<2*smax+1; s_i++){
        for(s_j=0; s_j<2*smax+1; s_j++){
          TBsk[s_i*(2*smax+1)+s_j]=0.0;
        }
      }
      for(s_i=smax; s_i<2*smax+1; s_i++){
        for(s_j=smax; s_j<2*smax; s_j++){
          TBsk[s_i*(2*smax+1)+s_j]=B[(s_i-smax)*smax+s_j-smax];
        }
      }

      for(s_i=0; s_i<2*smax+1; s_i++){
        TBdsk[s_i]=0.0;
        for(s_j=0; s_j<2*smax+1; s_j++){
          TBdsk[s_i]=TBdsk[s_i]+TBsk[s_i*(2*smax+1)+s_j]*dsk_1[s_j];
        }
      }

      for(s_i=0;s_i<2*smax+1;s_i++){
        dsk[s_i]=dsk[s_i] - rho*gamma*TBdsk[s_i];
      }
    }

    myu_2=myu_1;
    myu_1=myu;

    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@
    mpf_set_default_prec(2048);
    mpf_t x_mp;
    mpf_init(x_mp);
    mpf_t y_mp;
    mpf_init(y_mp);
    mpf_t z_mp;
    mpf_init(z_mp);

    mpf_t Gkgsk_mp[2*smax+1];
    for(s_i=0;s_i<2*smax+1;s_i++){
      mpf_init(Gkgsk_mp[s_i]);
    }
    for(s_i=0; s_i<2*smax+1; s_i++){
      mpf_set_d(Gkgsk_mp[s_i],0.0);
      for(s_j=0; s_j<2*smax+1; s_j++){
        mpf_set_d(x_mp,Gk[s_i*(2*smax+1)+s_j]);
        mpf_set_d(y_mp,gsk[s_j]);
        mpf_mul(z_mp,x_mp,y_mp);
        mpf_add(Gkgsk_mp[s_i],Gkgsk_mp[s_i],z_mp);
      }
    }

    mpf_t myu_mp;
    mpf_init(myu_mp);
    mpf_set_d(myu_mp,0.0);
    for(s=0; s<2*smax+1; s++){
      mpf_set_d(y_mp,gsk[s]);
      mpf_mul(z_mp,Gkgsk_mp[s],y_mp);
      mpf_add(myu_mp,myu_mp,z_mp);
    }
    myu=mpf_get_d(myu_mp);

    mpf_t Gkdsk_mp[2*smax+1];
    for(s_i=0;s_i<2*smax+1;s_i++){
      mpf_init(Gkdsk_mp[s_i]);
    }
    for(s_i=0; s_i<2*smax+1; s_i++){
      mpf_set_d(Gkdsk_mp[s_i],0.0);
      for(s_j=0; s_j<2*smax+1; s_j++){
        mpf_set_d(x_mp,Gk[s_i*(2*smax+1)+s_j]);
        mpf_set_d(y_mp,dsk[s_j]);
        mpf_mul(z_mp,x_mp,y_mp);
        mpf_add(Gkdsk_mp[s_i],Gkdsk_mp[s_i],z_mp);
        // Gkdsk[s_i]= Gkdsk[s_i]+Gk[s_i*(2*smax+1)+s_j]*dsk[s_j];
      }
    }
    nyu_1=nyu;

    mpf_t nyu_mp;
    mpf_init(nyu_mp);
    mpf_set_d(nyu_mp,0.0);
    //    nyu=0.0;
    for(s=0; s<2*smax+1; s++){
      mpf_set_d(y_mp,gsk[s]);
      mpf_mul(z_mp,Gkdsk_mp[s],y_mp);
      mpf_add(nyu_mp,nyu_mp,z_mp);
      //      nyu=nyu+Gkdsk[s]*gsk[s];
    }
    nyu=mpf_get_d(nyu_mp);

    mpf_clear(x_mp);
    mpf_clear(y_mp);
    for(s_i=0;s_i<2*smax+1;s_i++){
      mpf_clear(Gkgsk_mp[s_i]);
    }
    for(s_i=0;s_i<2*smax+1;s_i++){
      mpf_clear(Gkdsk_mp[s_i]);
    }
    mpf_clear(myu_mp);
    mpf_clear(nyu_mp);
    //@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@

    gamma_2=gamma_1;
    gamma_1=gamma;
    gamma=myu/nyu;
    if( (smax*k+j) == 1){
      rho=1.0;
    }else{
      rho=(1.0-gamma/gamma_1*myu/myu_1/rho);
      rho=1.0/rho;
    }

    myu_[j-1]=myu;
    rho_[j-1]=rho;
    rhogamma_inv_[(j-1)*(smax)+j-1]=rho*gamma;

    for(s_i=0; s_i<2*smax+1; s_i++){
      dsk_new0[(j-1)*(2*smax+1)+s_i]=dsk[s_i];
    }
    rho_new0[j-1]=rho;
    gamma_new0[j-1]=gamma;
  }

  rho_new[0]     = rho;
  rho0_new[0]    = rho;
  gamma_new[0]   = gamma;
  gamma0_new[0]  = gamma;
  myu_1_new[0]   = myu_1;
  myu_2_new[0]   = myu_2;
  myu_new[0]     = myu;
  nyu_1_new[0]   = nyu_1;
  nyu_new[0]     = nyu;
  gamma_1_new[0] = gamma_1;
  gamma_2_new[0] = gamma_2;

  for(s_i=0; s_i<smax; s_i++){
    rhogamma_inv[s_i*smax+s_i]=1.0/rhogamma_inv_[s_i*smax+s_i];
  }

  return 0;
}
#endif

int update_cacg_opt(
                    mpi_prm prm,
                    int smax,
                    type* x,type* x_1,type* x_2,
                    type* q,type* q_1,type* q_2,
                    type* z,type* z_1,type* z_2,
                    type* Q_,type* V,
                    type* Z_,type* W,
                    type* dsk,type* rho,type* gamma,
                    type* rmax)
{
  int stm,stmx;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &stmx, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  int s;
  int j,jj;
  int jz,jx,jy;
  type x1,x2;
  type z1[nx],z2;
  type x_[nx];

  type q1,q2;
  type u[mx*(smax+1)],y[mx*(smax+1)];

  type rho_[smax+1];
  type tmp;
  for(jj=1;jj<smax+1;jj=jj+1){
    rho_[jj]=1.0-rho[jj-1];
  }

  tmp=0.0;

#pragma omp parallel for private(j,jj,jz,jy,jx,s,x1,q1,u,x_,x2)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {

#ifdef funroll
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
#ifdef opts1
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#elif opts2
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=2;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#elif opts3
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=2;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=3;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#elif opts4
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=2;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=3;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=4;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#elif opts5
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=2;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=3;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=4;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=5;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#endif
      }
      for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);

#ifdef opts1
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#elif opts2
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=2;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#elif opts3
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=2;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=3;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#elif opts4
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=2;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=3;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=4;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#elif opts5
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=2;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=3;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=4;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=5;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#endif
        }
      }
#else

      for(jj=1;jj<smax+1;jj=jj+1){
        s=smax;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
        }
        for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
            u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
              +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
              +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          }
        }
      }
#endif

#ifdef opts3

#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        jj=1;
        Q_[0*m+j]  = Q_[smax*m+j];
        x1  = rho[jj-1]*(x[j]+gamma[jj-1]*Q_[0*m+j])+(rho_[jj])*x_1[j];
        Q_[jj*m+j] = rho[jj-1]*(Q_[smax*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(smax-1)*m+j];

        jj=2;
        x_1[j] = rho[jj-1]*(x1+gamma[jj-1]*Q_[(jj-1)*m+j])+(rho_[jj])*x[j];
        Q_[jj*m+j] = rho[jj-1]*(Q_[(jj-1)*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(jj-2)*m+j];

        jj=3;
        x[j] = rho[jj-1]*(x_1[j]+gamma[jj-1]*Q_[(jj-1)*m+j])+(rho_[jj])*x1;
        Q_[jj*m+j] = rho[jj-1]*(Q_[(jj-1)*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(jj-2)*m+j];
        V[0*m+j]   =Q_[smax*m+j];
      }
#else
      jj=1;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        Q_[0*m+j]  = Q_[smax*m+j];
        Q_[jj*m+j] = rho[jj-1]*(Q_[smax*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(smax-1)*m+j];
      }
      for(jj=2;jj<smax+1;jj=jj+1){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          Q_[jj*m+j] = rho[jj-1]*(Q_[(jj-1)*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(jj-2)*m+j];
        }
      }
#ifdef intelopt
#pragma ivdep
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        V[0*m+j]   =Q_[smax*m+j];
      }
      jj=1;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        x1=x[j];
        x[j] = rho[jj-1]*(x[j]+gamma[jj-1]*Q_[0*m+j])+(rho_[jj])*x_1[j];
        x_1[j]=x1;
      }
      for(jj=2;jj<smax+1;jj=jj+1){
#ifdef intelopt
#pragma ivdep
#endif
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          x1=x[j];
          x[j] = rho[jj-1]*(x[j]+gamma[jj-1]*Q_[(jj-1)*m+j])+(rho_[jj])*x_1[j];
          x_1[j]=x1;
        }
      }
#endif

    }
  }


  //  start_collection("update_2");
#pragma omp parallel for private(jj,j,jz,jy,jx,s,z1,y) reduction(+:tmp) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {

#ifdef funroll

#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);

#ifdef opts1
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#elif opts2
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=2;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#elif opts3
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=2;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=3;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#elif opts4
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=2;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=3;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=4;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#elif opts5
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=2;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=3;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=4;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=5;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#endif
      }
      for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);

#ifdef opts1
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];

#elif opts2
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=2;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
#elif opts3
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=2;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=3;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
#elif opts4
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=2;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=3;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=4;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
#elif opts5
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=2;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=3;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=4;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=5;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
#endif
        }
      }

#else
      for(jj=1;jj<smax+1;jj=jj+1){
        s=smax;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        }
        for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
            ;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
               +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
               +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          }
        }
      }
#endif

      jj=1;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        Z_[0*m+j]  = Z_[smax*m+j];
        Z_[jj*m+j] = rho[jj-1]*(Z_[smax*m+j]-gamma[jj-1]*y[(jj-1)*mx+jx])+rho_[jj]*Z_[(smax-1)*m+j];
      }
      for(jj=2;jj<smax+1;jj=jj+1){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          Z_[jj*m+j] = rho[jj-1]*(Z_[(jj-1)*m+j]-gamma[jj-1]*y[(jj-1)*mx+jx])+rho_[jj]*Z_[(jj-2)*m+j];
        }
      }

      jj=smax;
#ifdef intelopt
#pragma ivdep
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        W[0*m+j]=Z_[jj*m+j];
        tmp=tmp+Z_[jj*m+j]*Z_[jj*m+j];
      }

    }
  }
  rmax[0]=tmp;
  // stop_collection("update_2");
  return 0;
}

/*
int update_cacg_opt_noz(
                    mpi_prm prm,
                    int smax,
                    type* x,type* x_1,type* x_2,
                    type* q,type* q_1,type* q_2,
                    type* z,type* z_1,type* z_2,
                    type* Q_,type* V,
                    type* Z_,type* W,
                    type* dsk,type* rho,type* gamma,
                    type* rmax){
  int stm,stmx;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &stmx, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  int s;
  int j,jj;
  int jz,jx,jy;
  type x1,x2;
  type z1[nx],z2;
  type x_[nx];

  type q1,q2;
  type u[mx*(smax+1)],y[mx*(smax+1)];

  type rho_[smax+1];
  type tmp;
  for(jj=1;jj<smax+1;jj=jj+1){
    rho_[jj]=1.0-rho[jj-1];
  }

  tmp=0.0;

#pragma omp parallel for private(jj,j,jz,jy,jx,s,x1,q1,u,x_,x2)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {

#ifdef funroll
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
    j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
#ifdef opts1
    jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#elif opts2
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=2;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#elif opts3
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=2;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=3;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#elif opts4
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=2;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=3;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=4;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#elif opts5
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=2;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=3;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=4;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=5;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#endif
      }
      for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);

#ifdef opts1
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#elif opts2
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=2;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#elif opts3
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=2;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=3;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#elif opts4
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=2;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=3;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=4;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#elif opts5
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=2;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=3;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=4;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=5;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#endif
        }
      }
#else

      for(jj=1;jj<smax+1;jj=jj+1){
        s=smax;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
        }
        for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
            u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
              +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
              +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          }
        }
      }
#endif

#ifdef opts3

#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        jj=1;
        Q_[0*m+j]  = Q_[smax*m+j];
        x1  = rho[jj-1]*(x[j]+gamma[jj-1]*Q_[0*m+j])+(rho_[jj])*x_1[j];
        Q_[jj*m+j] = rho[jj-1]*(Q_[smax*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(smax-1)*m+j];

        jj=2;
        x_1[j] = rho[jj-1]*(x1+gamma[jj-1]*Q_[(jj-1)*m+j])+(rho_[jj])*x[j];
        Q_[jj*m+j] = rho[jj-1]*(Q_[(jj-1)*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(jj-2)*m+j];

        jj=3;
        x[j] = rho[jj-1]*(x_1[j]+gamma[jj-1]*Q_[(jj-1)*m+j])+(rho_[jj])*x1;
        Q_[jj*m+j] = rho[jj-1]*(Q_[(jj-1)*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(jj-2)*m+j];
        V[0*m+j]   =Q_[smax*m+j];
      }
#else
      jj=1;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        Q_[0*m+j]  = Q_[smax*m+j];
        Q_[jj*m+j] = rho[jj-1]*(Q_[smax*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(smax-1)*m+j];
      }
      for(jj=2;jj<smax+1;jj=jj+1){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          Q_[jj*m+j] = rho[jj-1]*(Q_[(jj-1)*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(jj-2)*m+j];
        }
      }
#ifdef intelopt
#pragma ivdep
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        V[0*m+j]   =Q_[smax*m+j];
      }
      jj=1;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        x1=x[j];
        x[j] = rho[jj-1]*(x[j]+gamma[jj-1]*Q_[0*m+j])+(rho_[jj])*x_1[j];
        x_1[j]=x1;
      }
      for(jj=2;jj<smax+1;jj=jj+1){
#ifdef intelopt
#pragma ivdep
#endif
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          x1=x[j];
          x[j] = rho[jj-1]*(x[j]+gamma[jj-1]*Q_[(jj-1)*m+j])+(rho_[jj])*x_1[j];
          x_1[j]=x1;
        }
      }
#endif

    }
  }


  /*

  //  start_collection("update_2");
#pragma omp parallel for private(j,jz,jy,jx,s,z1,y) reduction(+:tmp) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
 
#ifdef funroll

#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);

#ifdef opts1
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#elif opts2
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=2;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#elif opts3
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=2;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=3;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#elif opts4
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=2;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=3;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=4;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#elif opts5
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=2;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=3;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=4;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=5;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#endif
      }
      for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);

#ifdef opts1
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];

#elif opts2
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=2;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
#elif opts3
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=2;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=3;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
#elif opts4
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=2;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=3;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=4;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
#elif opts5
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=2;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=3;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=4;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=5;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
#endif
        }
      }
      
#else      
      for(jj=1;jj<smax+1;jj=jj+1){
        s=smax;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        }
        for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
            ;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
               +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
               +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          }
        }
      }
#endif

      jj=1;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
                Z_[0*m+j]  = Z_[smax*m+j];
        Z_[jj*m+j] = rho[jj-1]*(Z_[smax*m+j]-gamma[jj-1]*y[(jj-1)*mx+jx])+rho_[jj]*Z_[(smax-1)*m+j];
      }
      for(jj=2;jj<smax+1;jj=jj+1){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          Z_[jj*m+j] = rho[jj-1]*(Z_[(jj-1)*m+j]-gamma[jj-1]*y[(jj-1)*mx+jx])+rho_[jj]*Z_[(jj-2)*m+j];
        }
      }

      jj=smax;
#ifdef intelopt
#pragma ivdep
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        W[0*m+j]=Z_[jj*m+j];
        tmp=tmp+Z_[jj*m+j]*Z_[jj*m+j];
      }

    }
  }
  rmax[0]=tmp;
  // stop_collection("update_2");
  */
/*
  return 0;
}
*/

int update_cacg_opt_nox(
                        mpi_prm prm,
                        int smax,
                        type* x,type* x_1,type* x_2,
                        type* q,type* q_1,type* q_2,
                        type* z,type* z_1,type* z_2,
                        type* Q_,type* V,
                        type* Z_,type* W,
                        type* dsk,type* rho,type* gamma,
                        type* rho_x,type* gamma_x,
                        type* rmax)
{
  int stm,stmx;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &stmx, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  int s;
  int j,jj;
  int jz,jx,jy;
  type x1,x2;
  type z1[nx],z2;
  type x_[nx];

  type q1,q2;
  type u[mx*(smax+1)],y[mx*(smax+1)];

  type rho_[smax+1];
  type tmp;
  for(jj=1;jj<smax+1;jj=jj+1){
    rho_[jj]=1.0-rho[jj-1];
  }

  tmp=0.0;

#pragma omp parallel for private(jj,j,jz,jy,jx,s,x1,q1,u,x_,x2)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {

#ifdef funroll
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);

#ifdef opts1
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#elif opts2
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=2;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#elif opts3
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=2;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=3;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#elif opts4
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=2;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=3;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=4;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#elif opts5
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=2;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=3;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=4;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=5;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#endif
      }
      for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);

#ifdef opts1
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#elif opts2
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=2;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#elif opts3
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=2;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=3;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#elif opts4
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=2;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=3;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=4;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#elif opts5
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=2;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=3;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=4;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=5;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#endif
        }
      }
#else

      for(jj=1;jj<smax+1;jj=jj+1){
        s=smax;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
        }
        for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
            u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
              +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
              +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          }
        }
      }
#endif

      jj=1;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        Q_[0*m+j]  = Q_[smax*m+j];
        Q_[jj*m+j] = rho[jj-1]*(Q_[smax*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(smax-1)*m+j];
      }

      for(jj=2;jj<smax+1;jj=jj+1){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          Q_[jj*m+j] = rho[jj-1]*(Q_[(jj-1)*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(jj-2)*m+j];
        }
      }
#ifdef intelopt
#pragma ivdep
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        V[0*m+j]   =Q_[smax*m+j];
      }

    }
  }


  //  start_collection("update_2");
#pragma omp parallel for private(jj,j,jz,jy,jx,s,z1,y) reduction(+:tmp)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {

#ifdef  funroll

#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);

#ifdef opts1
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#elif opts2
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=2;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#elif opts3
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=2;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=3;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#elif opts4
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=2;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=3;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=4;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#elif opts5
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=2;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=3;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=4;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=5;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#endif
      }
      for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);

#ifdef opts1
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];

#elif opts2
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=2;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
#elif opts3
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=2;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=3;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
#elif opts4
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=2;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=3;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=4;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
#elif opts5
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=2;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=3;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=4;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=5;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
#endif
        }
      }

#else
      for(jj=1;jj<smax+1;jj=jj+1){
        s=smax;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        }
        for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
            y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
              +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
              +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          }
        }
      }
#endif

      jj=1;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        Z_[0*m+j]  = Z_[smax*m+j];
        Z_[jj*m+j] = rho[jj-1]*(Z_[smax*m+j]-gamma[jj-1]*y[(jj-1)*mx+jx])+rho_[jj]*Z_[(smax-1)*m+j];
      }
      for(jj=2;jj<smax+1;jj=jj+1){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          Z_[jj*m+j] = rho[jj-1]*(Z_[(jj-1)*m+j]-gamma[jj-1]*y[(jj-1)*mx+jx])+rho_[jj]*Z_[(jj-2)*m+j];
        }
      }

      jj=smax;
#ifdef intelopt
#pragma ivdep
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        W[0*m+j]=Z_[jj*m+j];
        tmp=tmp+Z_[jj*m+j]*Z_[jj*m+j];
      }

    }
  }
  rmax[0]=tmp;
  // stop_collection("update_2");
  return 0;
}

int update_cacg_opt_x(
                      mpi_prm prm,
                      int smax,
                      type* x,type* x_1,type* x_2,
                      type* q,type* q_1,type* q_2,
                      type* z,type* z_1,type* z_2,
                      type* Q_,type* V,
                      type* Z_,type* W,
                      type* dsk,type* rho,type* gamma,
                      type* rho_x,type* gamma_x,
                      type* rmax)
{
  int stm,stmx;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &stmx, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  int s;
  int j,jj;
  int jz,jx,jy;
  type x1,x2;
  type z1[nx],z2;
  type x_[nx];

  type q1,q2;
  type u[mx*(smax+1)],y[mx*(smax+1)];

  type rho_[smax+1];
  type rho_x_[smax+1];
  type tmp;

  for(jj=1;jj<smax+1;jj=jj+1){
    rho_[jj]=1.0-rho[jj-1];
    rho_x_[jj]=1.0-rho_x[jj-1];
  }

  tmp=0.0;
#pragma omp parallel for private(jj,j,jz,jy,jx,s,x1,q1,u,x_,x2)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {

#ifdef funroll
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);

#ifdef opts1
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#elif opts2
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=2;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#elif opts3
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=2;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=3;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#elif opts4
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=2;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=3;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=4;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#elif opts5
        jj=1;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=2;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=3;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=4;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
        jj=5;u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*V[smax*m+j];
#endif
      }
      for(s=0; s<smax; s++){

#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);

#ifdef opts1
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#elif opts2
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=2;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#elif opts3
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=2;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=3;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#elif opts4
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=2;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=3;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=4;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#elif opts5
          jj=1;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=2;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=3;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=4;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          jj=5;u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
#endif
        }
      }
#else

      for(jj=1;jj<smax+1;jj=jj+1){
        s=smax;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
        }
        for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
            u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
              +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
              +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          }
        }
      }
#endif


      // 前回のQでxを更新
      //
      for(jj=1;jj<smax+1;jj=jj+1){

#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          x1=x[j];
          x[j] = rho_x[jj-1]*(x[j]+gamma_x[jj-1]*Q_[(jj-1)*m+j])+(rho_x_[jj])*x_1[j];
          x_1[j]=x1;
        }
      }

#ifdef opts3
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        jj=1;
        Q_[0*m+j]  = Q_[smax*m+j];
        x1  = rho[jj-1]*(x[j]+gamma[jj-1]*Q_[0*m+j])+(rho_[jj])*x_1[j];
        Q_[jj*m+j] = rho[jj-1]*(Q_[smax*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(smax-1)*m+j];

        jj=2;
        x_1[j] = rho[jj-1]*(x1+gamma[jj-1]*Q_[(jj-1)*m+j])+(rho_[jj])*x[j];
        Q_[jj*m+j] = rho[jj-1]*(Q_[(jj-1)*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(jj-2)*m+j];

        jj=3;
        x[j] = rho[jj-1]*(x_1[j]+gamma[jj-1]*Q_[(jj-1)*m+j])+(rho_[jj])*x1;
        Q_[jj*m+j] = rho[jj-1]*(Q_[(jj-1)*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(jj-2)*m+j];
        V[0*m+j]   =Q_[smax*m+j];
      }
#else

      jj=1;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        Q_[0*m+j]  = Q_[smax*m+j];
        Q_[jj*m+j] = rho[jj-1]*(Q_[smax*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(smax-1)*m+j];

        x1=x[j];
        x[j] = rho[jj-1]*(x[j]+gamma[jj-1]*Q_[(jj-1)*m+j])+(rho_[jj])*x_1[j];
        x_1[j]=x1;
      }

      for(jj=2;jj<smax;jj=jj+1){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          Q_[jj*m+j] = rho[jj-1]*(Q_[(jj-1)*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(jj-2)*m+j];

          x1=x[j];
          x[j] = rho[jj-1]*(x[j]+gamma[jj-1]*Q_[(jj-1)*m+j])+(rho_[jj])*x_1[j];
          x_1[j]=x1;
        }
      }
      jj=smax;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        Q_[jj*m+j] = rho[jj-1]*(Q_[(jj-1)*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(jj-2)*m+j];
        V[0*m+j]   =Q_[smax*m+j];

        x1=x[j];
        x[j] = rho[jj-1]*(x[j]+gamma[jj-1]*Q_[(jj-1)*m+j])+(rho_[jj])*x_1[j];
        x_1[j]=x1;
      }


#endif


    }
  }


  //  start_collection("update_2");
#pragma omp parallel for private(jj,j,jz,jy,jx,s,z1,y) reduction(+:tmp)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {

#ifdef funroll

#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);

#ifdef opts1
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#elif opts2
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=2;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#elif opts3
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=2;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=3;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#elif opts4
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=2;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=3;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=4;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#elif opts5
        jj=1;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=2;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=3;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=4;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        jj=5;y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
#endif
      }
      for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
#ifdef opts1
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];

#elif opts2
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=2;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
#elif opts3
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=2;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=3;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
#elif opts4
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=2;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=3;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=4;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
#elif opts5
          jj=1;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=2;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=3;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=4;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          jj=5;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
                 +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
                 +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
#endif
        }
      }

#else
      for(jj=1;jj<smax+1;jj=jj+1){
        s=smax;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        }
        for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
            ;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
               +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
               +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          }
        }
      }
#endif

      jj=1;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        Z_[0*m+j]  = Z_[smax*m+j];
        Z_[jj*m+j] = rho[jj-1]*(Z_[smax*m+j]-gamma[jj-1]*y[(jj-1)*mx+jx])+rho_[jj]*Z_[(smax-1)*m+j];
      }
      for(jj=2;jj<smax+1;jj=jj+1){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          Z_[jj*m+j] = rho[jj-1]*(Z_[(jj-1)*m+j]-gamma[jj-1]*y[(jj-1)*mx+jx])+rho_[jj]*Z_[(jj-2)*m+j];
        }
      }

      jj=smax;
#ifdef intelopt
#pragma ivdep
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        W[0*m+j]=Z_[jj*m+j];
        tmp=tmp+Z_[jj*m+j]*Z_[jj*m+j];
      }

    }
  }
  rmax[0]=tmp;
  // stop_collection("update_2");
  return 0;
}


int update_cacg_opt_s1(
                       mpi_prm prm,
                       int smax,
                       type* x,type* x_1,type* x_2,
                       type* q,type* q_1,type* q_2,
                       type* z,type* z_1,type* z_2,
                       type* Q_,type* V,
                       type* Z_,type* W,
                       type* dsk,type* rho,type* gamma,
                       type* rmax)
{
  int stm,stmx;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &stmx, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  int s;
  int j,jj;
  int jz,jx,jy;
  type x1,x2;
  type z1,z2;
  type x_[nx];

  type q1,q2;
  type u[mx*(smax+1)],y[mx*(smax+1)];

  type rho_[smax+1];
  type tmp;
  for(jj=1;jj<smax+1;jj=jj+1){
    rho_[jj]=1.0-rho[jj-1];
  }

  tmp=0.0;




#pragma omp parallel for private(j,jz,jy,jx,s,x1,q1,u,x_,x2)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {


      for(jj=1;jj<smax+1;jj=jj+1){
        s=smax;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
        }
        for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
            u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
              +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
              +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          }
        }
      }

      jj=1;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        // Q_[0*m+j]  = Q_[smax*m+j];
        // Q_[jj*m+j] = rho[jj-1]*(Q_[smax*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(smax-1)*m+j];
        q1  = Q_[smax*m+j];
        Q_[jj*m+j] = rho[jj-1]*(Q_[smax*m+j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*Q_[(smax-1)*m+j];
        Q_[0*m+j]=q1;
      }

#ifdef intelopt
#pragma ivdep
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        V[0*m+j]   =Q_[smax*m+j];
      }
      jj=1;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        x1=x[j];
        x[j] = rho[jj-1]*(x[j]+gamma[jj-1]*Q_[0*m+j])+(rho_[jj])*x_1[j];
        x_1[j]=x1;
      }

    }
  }


  //  start_collection("update_2");
#pragma omp parallel for private(j,jz,jy,jx,s,z1,y) reduction(+:tmp)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {



      for(jj=1;jj<smax+1;jj=jj+1){
        s=smax;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+smax+smax]*W[smax*m+j];
        }
        for(s=0; s<smax; s++){
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
            ;y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
               +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
               +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          }
        }
      }

      jj=1;
#ifdef fjopt
#pragma loop norecurrence
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        // Z_[0*m+j]  = Z_[smax*m+j];
        // Z_[jj*m+j] = rho[jj-1]*(Z_[smax*m+j]-gamma[jj-1]*y[(jj-1)*mx+jx])+rho_[jj]*Z_[(smax-1)*m+j];

        z1  = Z_[smax*m+j];
        Z_[jj*m+j] = rho[jj-1]*(Z_[smax*m+j]-gamma[jj-1]*y[(jj-1)*mx+jx])+rho_[jj]*Z_[(smax-1)*m+j];
        Z_[0*m+j]=z1;
      }

      jj=smax;
#ifdef intelopt
#pragma ivdep
#endif
#ifdef s_aligned
#pragma loop simd aligned
#endif
#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        W[0*m+j]=Z_[jj*m+j];
        tmp=tmp+Z_[jj*m+j]*Z_[jj*m+j];
      }

    }
  }
  rmax[0]=tmp;
  // stop_collection("update_2");
  return 0;
}

/*
int update_cacg_opt(
                    mpi_prm prm,
                    int smax,
                    type* x,type* x_1,type* x_2,
                    type* q,type* q_1,type* q_2,
                    type* z,type* z_1,type* z_2,
                //    type* Q,type* Z,
                    //              type* u,
                    // type* u_,
                    type* Q_,type* V,
                    //              type* y,
                    // type* y_,
                    type* Z_,type* W,
                    type* dsk,type* rho,type* gamma,
                    type* rmax){
  int stm,stmx;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &stmx, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  int s;
  int j,jj;
  int jz,jx,jy;
  type x1,x2;
  type z1,z2;
  type q1,q2;
  type u[mx*(smax+1)],y[mx*(smax+1)];

  type rho_[smax+1];
  type tmp;
   for(jj=1;jj<smax+1;jj=jj+1){
    rho_[jj]=1.0-rho[jj-1];
  }

  //
  tmp=0.0;
  // 

   start_collection("update_1");
#pragma omp parallel for private(j,jz,jy,jx,s,x1,q1,u)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
 
      for(jj=1;jj<smax+1;jj=jj+1){
        s=smax;
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          u[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
        }
        for(s=0; s<smax; s++){
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
            u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
              +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
              +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          }
        }
      }
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        Q_[0*m+j]=q[j];
      }

      for(jj=1;jj<smax+1;jj=jj+1){
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          x1=x_1[j];
          x_1[j]=x[j];
          x[j] = rho[jj-1]*(x[j]+gamma[jj-1]*q[j])+(rho_[jj])*x1;

          q1=q_1[j];
          q_1[j]=q[j];
          q[j] = rho[jj-1]*(q[j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*q1;
          Q_[jj*m+j]=q[j];

        }
      }

#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        V[0*m+j]=q[j];
      }

    }
  }
   // stop_collection("update_1");

   start_collection("update_2");
#pragma omp parallel for private(j,jz,jy,jx,s,z1,y) reduction(+:tmp) 
 for(jz = 0; jz < nz; jz++) {
   for(jy = 0; jy < ny; jy++) {

  for(jj=1;jj<smax+1;jj=jj+1){
    s=smax;
    for(jx = 0; jx < nx; jx++) {
      j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
      y[(jj-1)*mx+jx]=dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
    }
    for(s=0; s<smax; s++){
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
          +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
          +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
      }
    }
  }
     for(jx = 0; jx < nx; jx++) {
       j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
       Z_[0*m+j]=z[j];
     }
     for(jj=1;jj<smax+1;jj=jj){
#ifdef intelopt
#pragma ivdep
#endif
       for(jx = 0; jx < nx; jx++) {
         j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
         z1=z_1[j];
         z_1[j]=z[j];
         z[j] = rho[jj-1]*(z[j]-gamma[jj-1]*y[(jj-1)*mx+jx])+(rho_[jj])*z1;
         Z_[jj*m+j]=z[j];
       }
     }

#ifdef intelopt
#pragma ivdep
#endif
     for(jx = 0; jx < nx; jx++) {
       j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
       W[0*m+j]=z[j];
       tmp=tmp+z[j]*z[j];
     }

   }
 }
 rmax[0]=tmp;
 // stop_collection("update_2");

  return 0;
}
*/

/*
int update_cacg_opt_kahan(
                    mpi_prm prm,
                    int smax,
                    type* x,type* x_1,type* x_2,
                    type* q,type* q_1,type* q_2,
                    type* z,type* z_1,type* z_2,
                //    type* Q,type* Z,
                    //              type* u,
                    // type* u_,
                    type* Q_,type* V,
                    //              type* y,
                    // type* y_,
                    type* Z_,type* W,
                    type* dsk,type* rho,type* gamma){
  int stm,stmx;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &stmx, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  int s;
  int j,jj;
  int jz,jx,jy;
  type x1,x2;
  type z1,z2;
  type q1,q2;
  type u[mx*(smax+1)],y[mx*(smax+1)];

  type rho_[smax+1];

  for(jj=1;jj<smax+1;jj=jj+1){
    rho_[jj]=1.0-rho[jj-1];
  }
  //
#pragma omp parallel for private(j,jz,jy,jx,s,x1,z1,q1,u,y) 
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {

      type u_q[nx],u_s[nx],a,t;
      type y_q[nx],y_s[nx];

      for(jj=1;jj<smax+1;jj=jj+1){
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          u[(jj-1)*mx+jx]=0.0;
          y[(jj-1)*mx+jx]=0.0;
        }
      }

      for(jj=1;jj<smax+1;jj=jj+1){
        for(jx = 0; jx < nx; jx++) {
          u_s[jx]=0.0;
          u_q[jx]=0.0;

          y_s[jx]=0.0;
          y_q[jx]=0.0;
        }
        s=smax;
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
            +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
            +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];

          a = dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
          t = u_s[jx];
          u_s[jx] = t + (a + u_q[jx]);
          u_q[jx] = (a + u_q[jx]) - (u_s[jx] - t);

          a = dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
          t = y_s[jx];
          y_s[jx] = t + (a + y_q[jx]);
          y_q[jx] = (a + y_q[jx]) - (y_s[jx] - t);

        }
        for(s=0; s<smax; s++){
#ifdef intelopt
#pragma ivdep
#endif
          for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
            u[(jj-1)*mx+jx]=u[(jj-1)*mx+jx]
              +dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j]
              +dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
            y[(jj-1)*mx+jx]=y[(jj-1)*mx+jx]
              +dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j]
              +dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];

            a = dsk[(jj-1)*(2*smax+1)+s]*Q_[s*m+j];
            t = u_s[jx];
            u_s[jx] = t + (a + u_q[jx]);
            u_q[jx] = (a + u_q[jx]) - (u_s[jx] - t);
            a = dsk[(jj-1)*(2*smax+1)+s+smax]*V[s*m+j];
            t = u_s[jx];
            u_s[jx] = t + (a + u_q[jx]);
            u_q[jx] = (a + u_q[jx]) - (u_s[jx] - t);

            a = dsk[(jj-1)*(2*smax+1)+s]*Z_[s*m+j];
            t = y_s[jx];
            y_s[jx] = t + (a + y_q[jx]);
            y_q[jx] = (a + y_q[jx]) - (y_s[jx] - t);
            a =dsk[(jj-1)*(2*smax+1)+s+smax]*W[s*m+j];
            t =y_s[jx];
            y_s[jx] = t + (a + y_q[jx]);
            y_q[jx] = (a + y_q[jx]) - (y_s[jx] - t);

          }
        }

        for(jx = 0; jx < nx; jx++) {
          u[(jj-1)*mx+jx]=u_s[jx];
          y[(jj-1)*mx+jx]=y_s[jx];
        }

      }
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        Z_[0*m+j]=z[j];
        Q_[0*m+j]=q[j];
      }
      for(jj=1;jj<smax+1;jj=jj+1){
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          x1=x_1[j];
          z1=z_1[j];
          x_1[j]=x[j];
          z_1[j]=z[j];

          x[j] = rho[jj-1]*(x[j]+gamma[jj-1]*q[j])+(rho_[jj])*x1;
          z[j] = rho[jj-1]*(z[j]-gamma[jj-1]*y[(jj-1)*mx+jx])+(rho_[jj])*z1;
          //      Z[jj*m+j]=z[j];
          Z_[jj*m+j]=z[j];
        }
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          q1=q_1[j];
          q_1[j]=q[j];
          q[j] = rho[jj-1]*(q[j]-gamma[jj-1]*u[(jj-1)*mx+jx])+(rho_[jj])*q1;
          //      Q[jj*m+j]=q[j];
          Q_[jj*m+j]=q[j];
        }
      }

#ifdef intelopt
#pragma ivdep
#endif
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
        W[0*m+j]=z[j];
        V[0*m+j]=q[j];
        //      Z_[0*m+j]=Z[0*m+j];
        //      Q_[0*m+j]=Q[0*m+j];
        //      Z[0*m+j]=z[j];
        //      Q[0*m+j]=q[j];
      }
      for(jj=1;jj<smax;jj=jj+1){
#ifdef intelopt
#pragma ivdep
#endif
        for(jx = 0; jx < nx; jx++) {
          j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
          //      Z_[jj*m+j]=Z[jj*m+j];
          //      Q_[jj*m+j]=Q[jj*m+j];
        }
      }


    }
  }

  return 0;
}
*/

/*
int calc_GramMat_local_kahan(mpi_prm prm,
                       type* V,type* Z_,type* W,type* Gkk_1,type* Gkk,
                       int smax){
  int stm,stmx;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &stmx, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int j;
  int jz,jx,jy;

  int s_j,s_i;
  int thid;
  int num_th;
#pragma omp parallel
  {
#pragma omp master
    {
      num_th = omp_get_num_threads(); 
    }
  }

  type tmp1_s[num_th][smax+1][smax+1],tmp2_s[num_th][smax+1][smax+1],tmp3_s[num_th][smax+1][smax+1];
  type tmp1_e[num_th][smax+1][smax+1],tmp2_e[num_th][smax+1][smax+1],tmp3_e[num_th][smax+1][smax+1];
  for(thid=0; thid<num_th; thid++){ 
    for(s_i=0; s_i<smax+1; s_i++){
      for(s_j=0; s_j<smax+1; s_j++){ 
        tmp1_s[thid][s_i][s_j]=0.0;
        tmp2_s[thid][s_i][s_j]=0.0;
        tmp3_s[thid][s_i][s_j]=0.0;

        tmp1_e[thid][s_i][s_j]=0.0;
        tmp2_e[thid][s_i][s_j]=0.0;
        tmp3_e[thid][s_i][s_j]=0.0;
      }
    }
  }
  type tmp1,s1,y1,x1,e1;
  type tmp2,s2,y2,x2,e2;
  type tmp3,s3,y3,x3,e3;
  
#pragma omp parallel  private(j,jz,jy,jx,thid,s_j,s_i,tmp1,s1,y1,x1,e1,tmp2,s2,y2,x2,e2,tmp3,s3,y3,x3,e3)
  {
    thid   = omp_get_thread_num(); 
#pragma omp for
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        for(s_i=0; s_i<smax-1; s_i++){
          for(s_j=0; s_j<smax+1; s_j++){
#ifdef intelopt
#pragma ivdep
#endif
            for(jx = 0; jx < nx; jx++) {
              j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);

              //              tmp1_s[thid][s_i][s_j]=tmp1_s[thid][s_i][s_j]+V[(s_j)*m+j]*Z_[(s_i)*m+j];
              //              tmp2_s[thid][s_i][s_j]=tmp2_s[thid][s_i][s_j]+V[(s_j)*m+j]*W[(s_i)*m+j];

              s1=tmp1_s[thid][s_i][s_j];
              x1=V[(s_j)*m+j]*Z_[(s_i)*m+j];
              e1=tmp1_e[thid][s_i][s_j];
              tmp1=s1;
              y1=x1 + e1;
              s1=tmp1+y1;
              tmp1=tmp1-s1;
              e1=tmp1+y1;
              tmp1_s[thid][s_i][s_j]=s1;
              tmp1_e[thid][s_i][s_j]=e1;

              s2=tmp2_s[thid][s_i][s_j];
              x2=V[(s_j)*m+j]*W[(s_i)*m+j];
              e2=tmp2_e[thid][s_i][s_j];
              tmp2=s2;
              y2=x2 + e2;
              s2=tmp2+y2;
              tmp2=tmp2-s2;
              e2=tmp2+y2;
              tmp2_s[thid][s_i][s_j]=s2;
              tmp2_e[thid][s_i][s_j]=e2;

            }
          }
        }
        s_i=smax-1;
        for(s_j=0; s_j<smax+1; s_j++){
#ifdef intelopt
#pragma ivdep
#endif
          for(jx = 0; jx < nx; jx++) {
            j = (jx+stmx) + mx*(jy+stm) + mxy*(jz+stm);
            //@@@@@@@
              s1=tmp1_s[thid][s_i][s_j];
              x1=V[(s_j)*m+j]*Z_[(s_i)*m+j];
              e1=tmp1_e[thid][s_i][s_j];
              tmp1=s1;
              y1=x1 + e1;
              s1=tmp1+y1;
              tmp1=tmp1-s1;
              e1=tmp1+y1;
              tmp1_s[thid][s_i][s_j]=s1;
              tmp1_e[thid][s_i][s_j]=e1;

              s2=tmp2_s[thid][s_i][s_j];
              x2=V[(s_j)*m+j]*W[(s_i)*m+j];
              e2=tmp2_e[thid][s_i][s_j];
              tmp2=s2;
              y2=x2 + e2;
              s2=tmp2+y2;
              tmp2=tmp2-s2;
              e2=tmp2+y2;
              tmp2_s[thid][s_i][s_j]=s2;
              tmp2_e[thid][s_i][s_j]=e2;

              s3=tmp3_s[thid][s_i][s_j];
              x3=V[(s_j)*m+j]*W[(smax)*m+j];
              e3=tmp3_e[thid][s_i][s_j];
              tmp3=s3;
              y3=x3 + e3;
              s3=tmp3+y3;
              tmp3=tmp3-s3;
              e3=tmp3+y3;
              tmp3_s[thid][s_i][s_j]=s3;
              tmp3_e[thid][s_i][s_j]=e3;
              //@@@@@
              //            tmp1_s[thid][s_i][s_j]=tmp1_s[thid][s_i][s_j]+V[(s_j)*m+j]*Z_[(s_i)*m+j];
              //            tmp2_s[thid][s_i][s_j]=tmp2_s[thid][s_i][s_j]+V[(s_j)*m+j]*W[(s_i)*m+j];
              //            tmp3_s[thid][s_i][s_j]=tmp3_s[thid][s_i][s_j]+V[(s_j)*m+j]*W[(smax)*m+j];
          }
        }
      }
    }
  }

  for(thid=1; thid<num_th; thid++){ 
    for(s_i=0; s_i<smax+1; s_i++){
      for(s_j=0; s_j<smax+1; s_j++){ 
        //      tmp1_s[0][s_i][s_j]=tmp1_s[0][s_i][s_j]+tmp1_s[thid][s_i][s_j];
        //      tmp2_s[0][s_i][s_j]=tmp2_s[0][s_i][s_j]+tmp2_s[thid][s_i][s_j];
        //      tmp3_s[0][s_i][s_j]=tmp3_s[0][s_i][s_j]+tmp3_s[thid][s_i][s_j];

        s1=tmp1_s[0][s_i][s_j];
        x1=tmp1_s[thid][s_i][s_j];
        e1=tmp1_e[0][s_i][s_j];
        tmp1=s1;
        y1=x1 + e1;
        s1=tmp1+y1;
        tmp1=tmp1-s1;
        e1=tmp1+y1;
        tmp1_s[0][s_i][s_j]=s1;
        tmp1_e[0][s_i][s_j]=e1;

        s2=tmp2_s[0][s_i][s_j];
        x2=tmp2_s[thid][s_i][s_j];
        e2=tmp2_e[0][s_i][s_j];
        tmp2=s2;
        y2=x2 + e2;
        s2=tmp2+y2;
        tmp2=tmp2-s2;
        e2=tmp2+y2;
        tmp2_s[0][s_i][s_j]=s2;
        tmp2_e[0][s_i][s_j]=e2;

        s3=tmp3_s[0][s_i][s_j];
        x3=tmp3_s[thid][s_i][s_j];
        e3=tmp3_e[0][s_i][s_j];
        tmp3=s3;
        y3=x3 + e3;
        s3=tmp3+y3;
        tmp3=tmp3-s3;
        e3=tmp3+y3;
        tmp3_s[0][s_i][s_j]=s3;
        tmp3_e[0][s_i][s_j]=e3;

      }
    }
  }

  for(s_j=0; s_j<smax+1; s_j++){ 
    for(s_i=0; s_i<smax; s_i++){
      Gkk_1[(s_i)*(smax+1)+s_j]=tmp1_s[0][s_i][s_j];
      Gkk[(s_j)*(smax+1)+s_i]=tmp2_s[0][s_i][s_j];      
    }
    s_i=smax-1;
    Gkk[(s_j)*(smax+1)+smax]=tmp3_s[0][s_i][s_j];
  }

  /*  
  for(s_j=0; s_j<smax+1; s_j++){ 
    for(s_i=0; s_i<smax; s_i++){
      Gkk_1[(s_i)*(smax+1)+s_j]=0.0;
      Gkk[(s_j)*(smax+1)+s_i]=0.0;
    }
    s_i=smax-1;
    Gkk[(s_j)*(smax+1)+smax]=0.0;
  }
  for(thid=0; thid<num_th; thid++){ 
    for(s_j=0; s_j<smax+1; s_j++){ 
      for(s_i=0; s_i<smax; s_i++){
        Gkk_1[(s_i)*(smax+1)+s_j]=Gkk_1[(s_i)*(smax+1)+s_j]+tmp1_s[thid][s_i][s_j];
        Gkk[(s_j)*(smax+1)+s_i]=Gkk[(s_j)*(smax+1)+s_i]+tmp2_s[thid][s_i][s_j];
      }
      s_i=smax-1;
      Gkk[(s_j)*(smax+1)+smax]=Gkk[(s_j)*(smax+1)+smax]+tmp3_s[thid][s_i][s_j];
    }
  }

  return 0;
}
*/
