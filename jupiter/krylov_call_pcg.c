#include <stdio.h>
#include <stdlib.h>
#include <math.h>

// #include <mkl.h>
// calc_resについてもcalc_sresに置き換えた方がよい !!!!!

#include "struct.h"
#include "func.h"
#include "cg.h"
#include "os/os.h"

#define AVOIDZERO 0

static inline type avoid_zero(type f, type abstolmax)
{
  if (fabs(f) < abstolmax) {
    if (f < 0.0) {
      f = -abstolmax;
    } else {
      f = abstolmax;
    }
  }
  return f;
}

int pcg_call(mpi_prm prm,
	     int itrmax,
	     type rtolmax,type abstolmax,
	     type* x,type* b,type* A
	     ){
  int rank;
#ifdef JUPITER_MPI
  MPI_Comm_rank(prm.comm, &rank);
#endif
  int m = prm.m;
  int itr;
  type alpha,beta,gamma;
  type rhon[2];
  type rtol,abstol,bmax;
  type *r = (type*)malloc(sizeof(type)*m);
  type *s = (type*)malloc(sizeof(type)*m);
  type *p = (type*)malloc(sizeof(type)*m);
  type *q = (type*)malloc(sizeof(type)*m);
  type *Dinv = (type*)malloc(sizeof(type)*m);

  int i;
#pragma omp parallel for private(i) 
  for(i=0;i<m;i++){ 
  r[i] =  0.0;
  s[i] =  0.0;
  p[i] =  0.0;
  q[i] =  0.0;
  Dinv[i] = 0.0;
}
  
#if sMatrix
  make_sMat(prm,A);
#endif
  
  calc_norm(prm,b,&bmax);
  make_pre_idiagMat1(prm,A,Dinv);
#ifdef JUPITER_MPI
  sleev_comm(x,&prm);
#endif

#if sMatrix
  calc_sres(prm,A,x,b,r);
#else
  calc_res(prm,A,x,b,r);
#endif

  alpha=0.0;
  beta=0.0;
  solve_pre_mat0(prm,A,Dinv,r,s);
  calc_norm2(prm,r,s,&rhon[0],&abstol);

#if AVOIDZERO
  rtol=abstol/avoid_zero(bmax,abstolmax);
#else
  rtol=abstol/bmax;
#endif

#ifdef RES_history
  // show_mpiprm(prm);
  if(rank ==0 ){
    // iter=0;
    printf("Iteration ,Absolute res ,Relative res \n");
    printf("@res@  %4d, %16.14e %16.14e \n",0,abstol,rtol);
  }
#endif

  for(itr=0;itr<itrmax;itr++){
    axpy2(prm,alpha,beta,s,p,x);

#ifdef JUPITER_MPI
    sleev_comm(p,&prm);
#endif

#if sMatrix
   sMatVec_dot(prm,A,p,q,&gamma);
#else    
    MatVec_dot(prm,A,p,q,&gamma);
#endif

#if AVOIDZERO
    alpha=-rhon[0]/avoid_zero(gamma,abstolmax);
#else
    alpha=-rhon[0]/gamma;
#endif
    solve_pre_mat2(prm,A,Dinv,r,s,q,alpha,&rhon[1],&abstol);
    alpha=-alpha;
#if AVOIDZERO
    beta=rhon[1]/avoid_zero(rhon[0],abstolmax);
#else
    beta=rhon[1]/rhon[0];
#endif
    rhon[0]=rhon[1];

#if AVOIDZERO
    rtol=abstol/avoid_zero(bmax,abstolmax);
#else
    rtol=abstol/bmax;
#endif

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
  axpy(prm,alpha,p,x);
  
  free(r);
  free(s);
  free(p);
  free(q);
  free(Dinv);

  return itr;
}

int pbicg_call(mpi_prm prm,
	       int itrmax,
	       type rtolmax,type abstolmax,
	       type* x,type* b,type* A){	       
  int i,itr;

  double time[20];
  double stime,etime; 
  double ts,te; 
  for(i=0;i<20;i++){time[i]=(type)0.0;}

  int rank;
#ifdef JUPITER_MPI
  MPI_Comm_rank(prm.comm, &rank);
#else
  rank=0;
#endif

  type alpha,beta;
  type a1,b1;
  type c1,c2,c3;

  type evdot,vvdot;
  type abstol,rtol;
  type bmax;

  int m = prm.m;
  /*
  type e[m];
  type v[m];
  type q[m];
  type p[m];
  type p_[m];
  type e_[m];
  type r0[m];
  */

  type *e=(type*)malloc(sizeof(type)*m);
  type *v=(type*)malloc(sizeof(type)*m);
  type *q=(type*)malloc(sizeof(type)*m);
  type *p=(type*)malloc(sizeof(type)*m);
  type *p_=(type*)malloc(sizeof(type)*m);
  type *e_=(type*)malloc(sizeof(type)*m);
  type *r0=(type*)malloc(sizeof(type)*m);

  type *r=(type*)malloc(sizeof(type)*m);
  type *Dinv=(type*)malloc(sizeof(type)*m);
  
#pragma omp parallel for private(i) 
  for(i=0;i<m;i++){
    e[i]=(type)0.0;
    v[i]=(type)0.0;
    q[i]=(type)0.0;
    p[i]=(type)0.0;
    p_[i]=(type)0.0;
    e_[i]=(type)0.0;
    r0[i]=(type)0.0;
    r[i]=(type)0.0;
    Dinv[i]=(type)0.0;
  }

  int mx,my,mz;
  mx =prm.mx;
  my =prm.my;
  mz =prm.mz;

  type q_b[mx*my];
  type q_t[mx*my];
  type q_s[mx*mz];
  type q_n[mx*mz];
  type q_w[my*mz];
  type q_e[my*mz];

  make_pre_idiagMat1(prm,A,Dinv);

  //p=r=b-Ax
  //c1=(r,r)
#ifdef JUPITER_MPI
  sleev_comm(x,&prm); 
#endif
  calc_res_dot_cp2(prm, A, x,b,r,r0,p_,&c1);
  initializ_sleev(prm,p);
  solve_pre_mat0(prm,A,Dinv,p_,p);

  calc_norm(prm,b,&bmax);
  calc_norm(prm,r,&abstol);
  rtol=abstol/bmax;
#ifdef RES_history
  //  show_mpiprm(prm);
  if(rank==0 ){
    printf("Iteration ,Absolute res ,Relative res \n");
    printf("@res@  %4d, %16.14e %16.14e \n",0,abstol,rtol);
  }
#endif

  stime=cpu_time();
  for(itr=0;itr<itrmax;itr++){
    //------
    // q = Ap
    // c2 = (r0,q)
    // alpha = c1/c2
#ifdef dcomm
    ts=cpu_time();
    MatVec_dot_dcomm_(prm,A,p,q,r0,&c2,q_b,q_t,q_s,q_n,q_w,q_e);
    te=cpu_time();
    alpha = c1/c2;
    time[1]=time[1]+te-ts;
#else

#ifdef use_sleev1
    ts=cpu_time();
#ifdef JUPITER_MPI
    sleev_comm(p,&prm); 
#endif
    te=cpu_time();
    time[8]=time[8]+te-ts;

    ts=cpu_time();
    MatVec_dot_(prm,A,p,q,r0,&c2);
    alpha = c1/c2;
    te=cpu_time();
    time[1]=time[1]+te-ts;
#else    
    ts=cpu_time();
#ifdef JUPITER_MPI
    sleev_comm_nopack(p,q_b,q_t,q_s,q_n,q_w,q_e,&prm);
#endif
    te=cpu_time();
    time[8]=time[8]+te-ts;

    ts=cpu_time();
    MatVec_dot_(prm,A,p,q,r0,&c2);
    MatVec_dot_sface_(prm,A,r0,q,&c2,q_b,q_t,q_s,q_n,q_w,q_e);

    alpha = c1/c2;
    te=cpu_time();
    time[1]=time[1]+te-ts;
#endif

#endif

    //------
    // e_ = r-alpha*q
    // solve preMatrix Ke=e_ :: K : 前処理行列
    ts=cpu_time();

#ifdef use_sleev2
    initializ_sleev(prm,e);
#endif
    te=cpu_time();
    time[9]=time[9]+te-ts;

    alpha=-alpha;
    solve_pre_mat_BiCG1(prm,A,Dinv,e_,e,alpha,q,r);
    alpha=-alpha;

    te=cpu_time();
    time[3]=time[3]+te-ts;

    //------
    // v = Ae
    // c3 = (e_,v)/(v,v)

#ifdef dcomm
    ts=cpu_time();
    /*
      sleev_comm_nopack(e,q_b,q_t,q_s,q_n,q_w,q_e,&prm);
      MatVec_dot2(prm,A,e,v,e_,&evdot,&vvdot);
      MatVec_dot2_sface(prm,A,v,e_,&vvdot,&evdot,q_b,q_t,q_s,q_n,q_w,q_e);
    */
    MatVec_dot2_dcomm(prm,A,e,v,e_,&evdot,&vvdot,q_b,q_t,q_s,q_n,q_w,q_e);

    c3=evdot/vvdot;
    te=cpu_time();
    time[4]=time[4]+te-ts;
#else

#ifdef use_sleev2
    ts=cpu_time();

#ifdef JUPITER_MPI
    sleev_comm(e,&prm);
#endif
    te=cpu_time();
    time[8]=time[8]+te-ts;

    ts=cpu_time();

    MatVec_dot2(prm,A,e,v,e_,&evdot,&vvdot);
    c3=evdot/vvdot;

    te=cpu_time();
    time[4]=time[4]+te-ts;
#else
    ts=cpu_time();
#ifdef JUPITER_MPI
    sleev_comm_nopack(e,q_b,q_t,q_s,q_n,q_w,q_e,&prm);
#endif
    te=cpu_time();
    time[8]=time[8]+te-ts;

    ts=cpu_time();

    MatVec_dot2(prm,A,e,v,e_,&evdot,&vvdot);
    MatVec_dot2_sface(prm,A,v,e_,&vvdot,&evdot,q_b,q_t,q_s,q_n,q_w,q_e);
    c3=evdot/vvdot;

    te=cpu_time();
    time[4]=time[4]+te-ts;
#endif

#endif
    //------
    // x=x+alpha*p+c3*e
    ts=cpu_time();

    axpy_2(prm,alpha,c3,p,e,x);

    te=cpu_time();
    time[5]=time[5]+te-ts;

    // r=e_-c3*v
    // c1=(r0,r)
    ts=cpu_time();

    c3=-c3;
    axpy_dot2_out(prm,c3,v,e_,r,r0,&c1,&abstol);
    c3=-c3;

    te=cpu_time();
    time[6]=time[6]+te-ts;
    //------

    // || r ||/ || b || <= tol --> break
    abstol=sqrt(abstol);
    rtol=abstol/bmax;

#ifdef RES_history
    if(rank ==0 ){
      printf("@res@  %4d, %16.14e %16.14e \n",itr+1,abstol,rtol);
      //      printf("@res@  %4d, %16.14e %16.14e]  \n",itr+1,c1,c2,c3);
    }
#endif
    if( rtol < rtolmax ){
      break;
    }
    if( abstol < abstolmax ){
      break;
    }

    // solve preMatrix Kp=p :: K : 前処理行列
    ts=cpu_time();

    beta=c1/(c3*c2);

    a1=-beta*c3;
    b1=beta;

    ts=cpu_time();
#ifdef use_sleev1
    initializ_sleev(prm,p);
#endif
    te=cpu_time();
    time[9]=time[9]+te-ts;

    solve_pre_mat_BiCG2(prm,A,Dinv,p_,p,a1,b1,q,r);

    te=cpu_time();
    time[0]=time[0]+te-ts;

  }

  //  MPI_Barrier(prm.comm);
  free(e);
  free(v);
  free(q);
  free(p);
  free(p_);
  free(e_);
  free(r0);
  free(r);
  free(Dinv);
  
  etime=cpu_time();
#if 0  
  if(rank ==0 ){
    printf("solve = %f [s] \n", etime-stime);
    for(i=0;i<8;i++){
      printf("    region_%d   = %f [s] \n",i+1,time[i]);
    }
    printf("    comm        = %f [s] \n",time[8]);
    printf("    init_sleev  = %f [s] \n",time[9]);
  }
#endif
  
  return itr ;
}

