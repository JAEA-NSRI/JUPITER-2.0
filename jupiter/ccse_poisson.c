#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#ifdef JUPITER_MPI
#include "mpi.h"
#endif
//#include <fjcoll.h>

#include "struct.h"
#include "func.h"
#include "cg.h"

int pcg_block3d_call_cuda(mpi_prm prm,
			  int itrmax,
			  type rtolmax,type abstolmax,
			  type* x_xyz_cpu,type* b_xyz_cpu,type* A_xyz_cpu
			  );


/* YSE: Avoid 0 division */
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
int check_residual_norm(mpi_prm prm,type rtolmax,type abstolmax,type* x,type* b,type* A)
{
  int m = prm.m;
  type *r = (type*)malloc(sizeof(type)*m);
  type abstol = 0.0;
  type rtol = 0.0;
  type bmax = 0.0;
  int flag = -1;

  int i;
#pragma omp parallel for private(i) 
  for(i=0;i<m;i++){ 
    r[i] = 0.0;
  }

#ifdef JUPITER_MPI
  sleev_comm(x,&prm);
#endif
  calc_res(prm,A,x,b,r);
  calc_norm(prm,b,&bmax);
  calc_norm(prm,r,&abstol);

  rtol = abstol/avoid_zero(bmax,abstolmax);
#if 0
  int rank;


#ifdef JUPITER_MPI
  MPI_Comm_rank(prm.comm, &rank);
#else
  rank=0;
#endif
  if(rank ==0){
    printf("@res@  %4d, %16.14e %16.14e \n",-1,abstol,rtol);
  }
#endif

  if( rtol < rtolmax ){
    flag = 0;
  }
  if( abstol < abstolmax ){
    flag = 0;
  }  

  free(r);

  return flag;
}


int make_matrix_array(type *A, type *dens, variable *val, parameter *prm,mpi_prm *cg_prm);
int make_matrix_array_T(type *A, type *b, type *dens,type *thc,type *specht,variable *val, parameter *prm,mpi_prm *cg_prm);
int make_matrix_array_Tf(type *A, type *b, type *dens,type *thc,type *specht,variable *val, parameter *prm,mpi_prm *cg_prm);
int make_matrix_array_Ts(type *A, type *b, type *dens,type *thc,type *specht,variable *val, parameter *prm,mpi_prm *cg_prm);
int make_matrix_array_U(int flag, type *A, type *b, type *dens,type *mu, variable *val, parameter *prm,mpi_prm *cg_prm);

#ifdef PROFILE
int get_profile_poisson(type time0,type *time,type *time_all,
                        int its,int *iter,int *iter_all);
#endif

int ccse_poisson_f(int topo_flag, const char *name, type *f, type *div,
                   parameter *prm, int itrmax, type rtolmax, type abstolmax,
                   make_matrix_array_func *make_matrix_arr_f, void *arg)
{
  domain    *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;
  int 
    nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
    mx=cdo->mx, my=cdo->my, mz=cdo->mz, m=mx*my*mz,
    mxy=mx*my;
  int stm=cdo->stm;

  mpi_prm cg_prm;
  convert(topo_flag,prm,&cg_prm);

#ifdef PROFILE
  type   time0 = cpu_time();
  timer_main *time = prm->time;
  iterate *iter=prm->iter;
#endif

  int stm_ = cg_prm.stm;
  int stp_ = cg_prm.stp;
  int nx_  = cg_prm.nx;
  int ny_  = cg_prm.ny;
  int nz_  = cg_prm.nz;
  int mx_  = cg_prm.mx;
  int my_  = cg_prm.my;
  int mz_  = cg_prm.mz;
  int mxy_ = cg_prm.mxy;
  int m_   = cg_prm.m;

  type *A = (type*)malloc(sizeof(type)*m_*7);
  type *b = (type*)malloc(sizeof(type)*m_);
  type *x = (type*)malloc(sizeof(type)*m_);

  int jx,jy,jz,i;
#pragma omp parallel for private(i) 
  for(i=0;i<m_*7;i++){ 
    A[i] = 0.0;
  }
#pragma omp parallel for private(i) 
  for(i=0;i<m_;i++){ 
    b[i] = 0.0;
    x[i] = 0.0;
  }

#if 0
#pragma omp parallel for private(jz,jy,jx)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {

	int j   = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
        int ii  = jx + nx*jy + nx*ny*jz;
        int j_  = (jx+stm_) + mx_*(jy+stm_) + mxy_*(jz+stm_);
#else
#pragma omp parallel for private(jz,jy,jx)
  for(jz = 0; jz < nz_; jz++) {
    for(jy = 0; jy < ny_; jy++) {
      for(jx = 0; jx < nx_; jx++) {

	int j   = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
        int ii  = jx + nx_ * jy + nx_ * ny_ * jz;
        int j_  = (jx+stm_) + mx_*(jy+stm_) + mxy_*(jz+stm_);
#endif
        b[j_] = div[ii];
        x[j_] = f[j];
      }
    }
  }

  make_matrix_arr_f(topo_flag, A, b,
                    stm_, stm_, stm_,
                    stp_, stp_, stp_,
                    mx_, my_, mz_,
                    prm, arg);

  int flag_krylov = check_residual_norm( cg_prm,rtolmax, abstolmax,x,b,A);
  int its;
  if(flag_krylov!=0){
#ifdef JUPITER_SOLVER_CUDA_CG
    its = pcg_block3d_call_cuda(cg_prm,
				itrmax,
				rtolmax, abstolmax,
				x, b, A);
#elif JUPITER_SOLVER_BiCG
    its = pbicg_call(cg_prm,
		     itrmax,
		     rtolmax, abstolmax,
		     x, b, A);
#else
    its = pcg_call(cg_prm,
		     itrmax,
		     rtolmax, abstolmax,
		     x, b, A);
#endif
  }else{
    its = 0;
  }

  if ( cdo->icnt % cdo->view == 0 && mpi->rank == 0 ) printf("    iter in %-4s = %d\n", name, its);

  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        int j  = (jx+stm)  + mx*(jy+stm)   + mxy*(jz+stm);
        int j_ = (jx+stm_) + mx_*(jy+stm_) + mxy_*(jz+stm_);
        f[j]=x[j_];
      }
    }
  }

  free(A);
  free(b);
  free(x);
  return its;
}

static
type boundary_matrix_coeff_P(fluid_boundary_data **cond_array,
                             int jbx, int jby, int nbx, int nby,
                             type cc, type cn)
{
  fluid_boundary_data *fp;

  fp = cond_array[jbx + jby * nbx];

  if (fp->cond == OUT) {
    return cc - cn;
  } else {
    return cc + cn;
  }
}

static void surface_boundary_matrix_coeff_P(surface_boundary_data *sb,
                                            type bnd_norm, type *cc, type *cn)
{
  switch(sb->cond) {
  case INLET:
    switch (sb->inlet_dir) {
    case SURFACE_INLET_DIR_NORMAL:
      /* Neumann condition */
      *cc += *cn;
      *cn = 0.0;
      break;
    case SURFACE_INLET_DIR_INVALID:
      break;
    }
    break;
  }
}

int make_matrix_array(type *A, type *dens, variable *val, parameter *prm,mpi_prm *cg_prm)
{
  flags     *flg=prm->flg;
  domain    *cdo=prm->cdo;
  mpi_param *mpi=prm->mpi;

  int i,j,jx,jy,jz,
    nx=cdo->nx, ny=cdo->ny, nz=cdo->nz, n=nx*ny*nz,
    mx=cdo->mx, my=cdo->my, mz=cdo->mz, m=mx*my*mz,
    mxy=mx*my;
  int stm=cdo->stm;
  int icc,icw,ice,ics,icn,icb,ict;
  type cc, cw, ce, cs, cn, cb, ct,
    dxi2=cdo->dxi*cdo->dxi,
    dyi2=cdo->dyi*cdo->dyi,
    dzi2=cdo->dzi*cdo->dzi;
  type *ls = val->ls;

/* Sato */
  type *fs;
  type *fs_ibm = NULL;
  if (prm->flg->solute_diff == OFF) {
    fs = val->fs_sum;
  } else {
    fs = val->fs;
  }
  fs_ibm = val->fs_ibm ? val->fs_ibm : fs;

//***************** 
/* Yamashita 2020/6/10 */
  type *eps = val->eps;

  int bc_xm ,bc_xp ,bc_ym ,bc_yp ,bc_zm ,bc_zp ;
  bc_xm=flg->bc_xm;
  bc_xp=flg->bc_xp;
  bc_ym=flg->bc_ym;
  bc_yp=flg->bc_yp;
  bc_zm=flg->bc_zm;
  bc_zp=flg->bc_zp;
  int rank;
  rank=mpi->rank;

  int nrk[6];
  for(i=0;i<6;i++){
    nrk[i]=mpi->nrk[i];
  }

  //-------
  int j_;
  int stm_;
  int m_;
  int mx_,my_,mxy_;

  stm_ = cg_prm->stm;
  m_   = cg_prm->m;
  mx_  = cg_prm->mx;
  my_  = cg_prm->my;
  mxy_ = cg_prm->mxy;
  //--------
  type ccw, cce, ccs, ccn, ccb, cct;

#pragma omp parallel for                             \
  private(i,j,j_,jx,jy,jz,                           \
  cw,ce,cs,cn,cb,ct,cc,icc,icw,ice,ics,icn,icb,ict)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        type aw, ae, as, an, ab, at, qs;
        type ep_w, ep_e, ep_s, ep_n, ep_b, ep_t;

        j  = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
        j_ = (jx+stm_) + mx_*(jy+stm_) + mxy_*(jz+stm_);
        i  = (rank*n) + jx + nx*jy + nx*ny*jz;

        if (prm->flg->porous == ON && eps) {
          /*
           * YSE: There were two following schemes as commented out,
           *      but has been removed for cleaning up.
           *      * Averaging: `ep_w = 0.5 * (eps[j] + eps[j - 1]); ...`
           *      * Maximum:   `ep_w = MAX2(eps[j], eps[j - 1]); ...`
           */
          ep_w = MIN2(eps[j], eps[j-1]);
          ep_e = MIN2(eps[j], eps[j+1]);
          ep_s = MIN2(eps[j], eps[j-mx]);
          ep_n = MIN2(eps[j], eps[j+mx]);
          ep_b = MIN2(eps[j], eps[j-mxy]);
          ep_t = MIN2(eps[j], eps[j+mxy]);
        } else {
          ep_w = 1.0;
          ep_e = 1.0;
          ep_s = 1.0;
          ep_n = 1.0;
          ep_b = 1.0;
          ep_t = 1.0;
        }

        if (prm->flg->IBM == ON && fs_ibm) {
          type fsi, fsc, fsw, fse, fss, fsn, fsb, fst;

          // clip vof
          fsc = clip(1.0 - fs_ibm[j]);
          fsw = clip(1.0 - fs_ibm[j-1]);
          fse = clip(1.0 - fs_ibm[j+1]);
          fss = clip(1.0 - fs_ibm[j-mx]);
          fsn = clip(1.0 - fs_ibm[j+mx]);
          fsb = clip(1.0 - fs_ibm[j-mxy]);
          fst = clip(1.0 - fs_ibm[j+mxy]);
          //--- calc solid ratio at cell face ---
          aw = MIN2(fsc, fsw);
          ae = MIN2(fsc, fse);
          as = MIN2(fsc, fss);
          an = MIN2(fsc, fsn);
          ab = MIN2(fsc, fsb);
          at = MIN2(fsc, fst);
        } else {
          aw = 1.0;
          ae = 1.0;
          as = 1.0;
          an = 1.0;
          ab = 1.0;
          at = 1.0;
        }

        //cw = aw * ep_w * 2.0 / (dens[j] + dens[j - 1  ]) * dxi2;
        //ce = ae * ep_e * 2.0 / (dens[j] + dens[j + 1  ]) * dxi2;
        //cs = as * ep_s * 2.0 / (dens[j] + dens[j - mx ]) * dyi2;
        //cn = an * ep_n * 2.0 / (dens[j] + dens[j + mx ]) * dyi2;
        //cb = ab * ep_b * 2.0 / (dens[j] + dens[j - mxy]) * dzi2;
        //ct = at * ep_t * 2.0 / (dens[j] + dens[j + mxy]) * dzi2;
        cw = ep_w * 2.0 / (dens[j] + dens[j - 1  ]) * dxi2;
        ce = ep_e * 2.0 / (dens[j] + dens[j + 1  ]) * dxi2;
        cs = ep_s * 2.0 / (dens[j] + dens[j - mx ]) * dyi2;
        cn = ep_n * 2.0 / (dens[j] + dens[j + mx ]) * dyi2;
        cb = ep_b * 2.0 / (dens[j] + dens[j - mxy]) * dzi2;
        ct = ep_t * 2.0 / (dens[j] + dens[j + mxy]) * dzi2;
        cc = -(cw + ce + cs + cn + cb + ct + 1.0e-10);
        //cc = - (cw + ce + cs + cn + cb + ct + 1.0e-8);

        icc = i;
        icw = i - 1;
        ice = i + 1;
        ics = i - nx;
        icn = i + nx;
        icb = i - nx*ny;
        ict = i + nx*ny;

        // Embed the wall boundary to satisfy Neumann condition
#ifdef PW
        const type dd = cc;
        int flag_is_solid[6] = { 0 , 0 , 0, 0, 0, 0};
        if (ls[j] + ls[j-1  ] >= 0.0) { cc += cw;  cw = 0.0; flag_is_solid[0] = 1; }
        if (ls[j] + ls[j+1  ] >= 0.0) { cc += ce;  ce = 0.0; flag_is_solid[1] = 1; }
        if (ls[j] + ls[j-mx ] >= 0.0) { cc += cs;  cs = 0.0; flag_is_solid[2] = 1; }
        if (ls[j] + ls[j+mx ] >= 0.0) { cc += cn;  cn = 0.0; flag_is_solid[3] = 1; }
        if (ls[j] + ls[j-mxy] >= 0.0) { cc += cb;  cb = 0.0; flag_is_solid[4] = 1; }
        if (ls[j] + ls[j+mxy] >= 0.0) { cc += ct;  ct = 0.0; flag_is_solid[5] = 1; }
        if (flag_is_solid[0] && flag_is_solid[1] && flag_is_solid[2] && flag_is_solid[3] && flag_is_solid[4] && flag_is_solid[5]) {
          cc = dd;
          cw = ce = cs = cn = cb = ct = -dd/6.0;
        }
#endif

        if (val->surface_bnd) {
          struct surface_boundary_data *sbw, *sbe, *sbs, *sbn, *sbb, *sbt;
          type nvbw, nvbe, nvbs, nvbn, nvbb, nvbt;
          int jxm, jxp, jym, jyp, jzm, jzp;

          jxm = 3 * j;
          jym = 3 * j + 1;
          jzm = 3 * j + 2;
          jxp = 3 * (j + 1);
          jyp = 3 * (j + mx) + 1;
          jzp = 3 * (j + mxy) + 2;

          sbw = val->surface_bnd[jxm];
          sbe = val->surface_bnd[jxp];
          sbs = val->surface_bnd[jym];
          sbn = val->surface_bnd[jyp];
          sbb = val->surface_bnd[jzm];
          sbt = val->surface_bnd[jzp];
          nvbw = val->bnd_norm_u[jxm];
          nvbe = val->bnd_norm_u[jxp];
          nvbs = val->bnd_norm_v[jym];
          nvbn = val->bnd_norm_v[jyp];
          nvbb = val->bnd_norm_w[jzm];
          nvbt = val->bnd_norm_w[jzp];

          if (sbw)
            surface_boundary_matrix_coeff_P(sbw, nvbw, &cc, &cw);
          if (sbe)
            surface_boundary_matrix_coeff_P(sbe, nvbe, &cc, &ce);
          if (sbs)
            surface_boundary_matrix_coeff_P(sbs, nvbs, &cc, &cs);
          if (sbn)
            surface_boundary_matrix_coeff_P(sbn, nvbn, &cc, &cn);
          if (sbb)
            surface_boundary_matrix_coeff_P(sbb, nvbb, &cc, &cb);
          if (sbt)
            surface_boundary_matrix_coeff_P(sbt, nvbt, &cc, &ct);
        }

        if(nrk[4] == -1 && jx == 0   ) {
          icw = -1;
          cc = boundary_matrix_coeff_P(val->bnd_W.fl,
                                       jy + cdo->stmb, jz + cdo->stmb,
                                       cdo->nby, cdo->nbz, cc, cw);
        }
        if(nrk[5] == -1 && jx == nx-1) {
          ice = -1;
          cc = boundary_matrix_coeff_P(val->bnd_E.fl,
                                       jy + cdo->stmb, jz + cdo->stmb,
                                       cdo->nby, cdo->nbz, cc, ce);
        }
        if(nrk[2] == -1 && jy == 0   ) {
          ics = -1;
          cc = boundary_matrix_coeff_P(val->bnd_S.fl,
                                       jx + cdo->stmb, jz + cdo->stmb,
                                       cdo->nbx, cdo->nbz, cc, cs);
        }
        if(nrk[3] == -1 && jy == ny-1) {
          icn = -1;
          cc = boundary_matrix_coeff_P(val->bnd_N.fl,
                                       jx + cdo->stmb, jz + cdo->stmb,
                                       cdo->nbx, cdo->nbz, cc, cn);
        }
        if(nrk[0] == -1 && jz == 0   ) {
          icb = -1;
          cc = boundary_matrix_coeff_P(val->bnd_B.fl,
                                       jx + cdo->stmb, jy + cdo->stmb,
                                       cdo->nbx, cdo->nby, cc, cb);
        }
        if(nrk[1] == -1 && jz == nz-1) {
          ict = -1;
          cc = boundary_matrix_coeff_P(val->bnd_T.fl,
                                       jx + cdo->stmb, jy + cdo->stmb,
                                       cdo->nbx, cdo->nby, cc, ct);
        }

        if(icb != -1){A[j_+0*m_]=cb;}else{A[j_+0*m_]=0.0;}
        if(ics != -1){A[j_+1*m_]=cs;}else{A[j_+1*m_]=0.0;}
        if(icw != -1){A[j_+2*m_]=cw;}else{A[j_+2*m_]=0.0;}
        if(icc != -1){A[j_+3*m_]=cc;}else{A[j_+3*m_]=0.0;}
        if(ice != -1){A[j_+4*m_]=ce;}else{A[j_+4*m_]=0.0;}
        if(icn != -1){A[j_+5*m_]=cn;}else{A[j_+5*m_]=0.0;}
        if(ict != -1){A[j_+6*m_]=ct;}else{A[j_+6*m_]=0.0;}
      }
    }
  }
  return 0;
}


static
type boundary_matrix_coeff_T(thermal_boundary_data **cond_array,
                             int jbx, int jby, int nbx, int nby,
                             type cc, type cn, type *b)
{
  thermal_boundary_data *fp;

  fp = cond_array[jbx + jby * nbx];
  if (fp->cond == ISOTHERMAL) {
    /* Add to source term */
    *b -= 2.0 * cn * fp->temperature.current_value;
    return cc - cn;
  } else {
    return cc + cn;
  }
}

int make_matrix_array_T(type *A, type *b, type *dens,type *thc,type *specht,variable *val, parameter *prm,mpi_prm *cg_prm)
{

  flags     *flg=prm->flg;
  domain    *cdo=prm->cdo;
  mpi_param *mpi=prm->mpi;

  int i,j,jx,jy,jz,
    nx=cdo->nx, ny=cdo->ny, nz=cdo->nz, n=nx*ny*nz,
    mx=cdo->mx, my=cdo->my, mz=cdo->mz, m=mx*my*mz,
    mxy=mx*my;

  int stm=cdo->stm;
  int icc,icw,ice,ics,icn,icb,ict;
  type cc, cw, ce, cs, cn, cb, ct,
    dxi2=cdo->dxi*cdo->dxi,
    dyi2=cdo->dyi*cdo->dyi,
    dzi2=cdo->dzi*cdo->dzi;

  int rank;
  rank=mpi->rank;
  int nrk[6];
  for(i=0;i<6;i++){
    nrk[i]=mpi->nrk[i];
  }
  type dti = 1.0/cdo->dt;

  int j_;
  int stm_;
  int m_;
  int mx_,my_,mxy_;

  stm_ = cg_prm->stm;
  m_   = cg_prm->m;
  mx_  = cg_prm->mx;
  my_  = cg_prm->my;
  mxy_ = cg_prm->mxy;

  type *sgm=val->sgm;

#pragma omp parallel for                             \
  private(i,j,j_,jx,jy,jz,                           \
  cw,ce,cs,cn,cb,ct,cc,icc,icw,ice,ics,icn,icb,ict  )
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
        j_ = (jx+stm_) + mx_*(jy+stm_) + mxy_*(jz+stm_);
        i = (rank*n) + jx + nx*jy + nx*ny*jz;

        // coefficient of matrix

        // harmonic mean
        cw = 2.0/(1.0/thc[j] + 1.0/thc[j-1  ])*dxi2;
        ce = 2.0/(1.0/thc[j] + 1.0/thc[j+1  ])*dxi2;
        cs = 2.0/(1.0/thc[j] + 1.0/thc[j-mx ])*dyi2;
        cn = 2.0/(1.0/thc[j] + 1.0/thc[j+mx ])*dyi2;
        cb = 2.0/(1.0/thc[j] + 1.0/thc[j-mxy])*dzi2;
        ct = 2.0/(1.0/thc[j] + 1.0/thc[j+mxy])*dzi2;

        // Arithmetric mean
        //cw = 0.5*(thc[j] + thc[j-1  ])*dxi2;
        //ce = 0.5*(thc[j] + thc[j+1  ])*dxi2;
        //cs = 0.5*(thc[j] + thc[j-mx ])*dyi2;
        //cn = 0.5*(thc[j] + thc[j+mx ])*dyi2;
        //cb = 0.5*(thc[j] + thc[j-mxy])*dzi2;
        //ct = 0.5*(thc[j] + thc[j+mxy])*dzi2;

        if (flg->porous == ON)
          cc = - (cw + ce + cs + cn + cb + ct + sgm[j]*dens[j]*specht[j]*dti + 1.0e-8);
        else
          cc = - (cw + ce + cs + cn + cb + ct + dens[j]*specht[j]*dti + 1.0e-8);

        // index of matrix
        icc = i;
        icw = i - 1;
        ice = i + 1;
        ics = i - nx;
        icn = i + nx;
        icb = i - nx*ny;
        ict = i + nx*ny;
        // Neumann-boundary
        if(nrk[4] == -1 && jx == 0   ) {
          icw = -1;
          cc = boundary_matrix_coeff_T(val->bnd_W.th,
                                       jy + cdo->stmb, jz + cdo->stmb,
                                       cdo->nby, cdo->nbz, cc, cw, &b[j_]);
        }
        if(nrk[5] == -1 && jx == nx-1) {
          ice = -1;
          cc = boundary_matrix_coeff_T(val->bnd_E.th,
                                       jy + cdo->stmb, jz + cdo->stmb,
                                       cdo->nby, cdo->nbz, cc, ce, &b[j_]);
        }
        if(nrk[2] == -1 && jy == 0   ) {
          ics = -1;
          cc = boundary_matrix_coeff_T(val->bnd_S.th,
                                       jx + cdo->stmb, jz + cdo->stmb,
                                       cdo->nbx, cdo->nbz, cc, cs, &b[j_]);
        }
        if(nrk[3] == -1 && jy == ny-1) {
          icn = -1;
          cc = boundary_matrix_coeff_T(val->bnd_N.th,
                                       jx + cdo->stmb, jz + cdo->stmb,
                                       cdo->nbx, cdo->nbz, cc, cn, &b[j_]);
        }
        if(nrk[0] == -1 && jz == 0   ) {
          icb = -1;
          cc = boundary_matrix_coeff_T(val->bnd_B.th,
                                       jx + cdo->stmb, jy + cdo->stmb,
                                       cdo->nbx, cdo->nby, cc, cb, &b[j_]);
        }
        if(nrk[1] == -1 && jz == nz-1) {
          ict = -1;
          cc = boundary_matrix_coeff_T(val->bnd_T.th,
                                       jx + cdo->stmb, jy + cdo->stmb,
                                       cdo->nbx, cdo->nby, cc, ct, &b[j_]);
        }

        if(icb != -1){A[j_+0*m_]=cb;}else{A[j_+0*m_]=0.0;}
        if(ics != -1){A[j_+1*m_]=cs;}else{A[j_+1*m_]=0.0;}
        if(icw != -1){A[j_+2*m_]=cw;}else{A[j_+2*m_]=0.0;}
        if(icc != -1){A[j_+3*m_]=cc;}else{A[j_+3*m_]=0.0;}
        if(ice != -1){A[j_+4*m_]=ce;}else{A[j_+4*m_]=0.0;}
        if(icn != -1){A[j_+5*m_]=cn;}else{A[j_+5*m_]=0.0;}
        if(ict != -1){A[j_+6*m_]=ct;}else{A[j_+6*m_]=0.0;}
      }
    }
  }

  return 0;
}

int make_matrix_array_Tf(type *A, type *b, type *dens,type *thc,type *specht,variable *val, parameter *prm,mpi_prm *cg_prm)
{

  flags     *flg=prm->flg;
  domain    *cdo=prm->cdo;
  mpi_param *mpi=prm->mpi;
  phase_value *phv=prm->phv;

  int i,j,jx,jy,jz,
    nx=cdo->nx, ny=cdo->ny, nz=cdo->nz, n=nx*ny*nz,
    mx=cdo->mx, my=cdo->my, mz=cdo->mz, m=mx*my*mz,
    mxy=mx*my;

  int stm=cdo->stm;
  int icc,icw,ice,ics,icn,icb,ict;
  type cc, cw, ce, cs, cn, cb, ct,
    dxi2=cdo->dxi*cdo->dxi,
    dyi2=cdo->dyi*cdo->dyi,
    dzi2=cdo->dzi*cdo->dzi;

  int rank;
  rank=mpi->rank;
  int nrk[6];
  for(i=0;i<6;i++){
    nrk[i]=mpi->nrk[i];
  }
  type dti = 1.0/cdo->dt;

  int j_;
  int stm_;
  int m_;
  int mx_,my_,mxy_;

  stm_ = cg_prm->stm;
  m_   = cg_prm->m;
  mx_  = cg_prm->mx;
  my_  = cg_prm->my;
  mxy_ = cg_prm->mxy;


  type *sgm=val->sgm;
  type *epss=val->epss;
  type *eps=val->eps;
  type *ls = val->ls;
  type eps_w, eps_e, eps_s, eps_n, eps_b, eps_t;
  type g_tmp;

#pragma omp parallel for                             \
  private(i,j,j_,jx,jy,jz,                           \
  cw,ce,cs,cn,cb,ct,cc,icc,icw,ice,ics,icn,icb,ict,  \
  eps_w,eps_e,eps_s,eps_n,eps_b,eps_t              )
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
        j_ = (jx+stm_) + mx_*(jy+stm_) + mxy_*(jz+stm_);
        i = (rank*n) + jx + nx*jy + nx*ny*jz;

        // coefficient of matrix
        //g_tmp = tempdep_calc(&phv->thc_g, val->tf[j]);
        eps_w = MIN2(epss[j], epss[j-1]);
        eps_e = MIN2(epss[j], epss[j+1]);
        eps_s = MIN2(epss[j], epss[j-mx]);
        eps_n = MIN2(epss[j], epss[j+mx]);
        eps_b = MIN2(epss[j], epss[j-mxy]);
        eps_t = MIN2(epss[j], epss[j+mxy]);

        // harmonic mean
        cw = eps_w*2.0/(1.0/thc[j] + 1.0/thc[j-1  ])*dxi2;
        ce = eps_e*2.0/(1.0/thc[j] + 1.0/thc[j+1  ])*dxi2;
        cs = eps_s*2.0/(1.0/thc[j] + 1.0/thc[j-mx ])*dyi2;
        cn = eps_n*2.0/(1.0/thc[j] + 1.0/thc[j+mx ])*dyi2;
        cb = eps_b*2.0/(1.0/thc[j] + 1.0/thc[j-mxy])*dzi2;
        ct = eps_t*2.0/(1.0/thc[j] + 1.0/thc[j+mxy])*dzi2;

        // Arithmetric mean
        //cw = eps_w*0.5*(thc[j] + thc[j-1  ])*dxi2;
        //ce = eps_e*0.5*(thc[j] + thc[j+1  ])*dxi2;
        //cs = eps_s*0.5*(thc[j] + thc[j-mx ])*dyi2;
        //cn = eps_n*0.5*(thc[j] + thc[j+mx ])*dyi2;
        //cb = eps_b*0.5*(thc[j] + thc[j-mxy])*dzi2;
        //ct = eps_t*0.5*(thc[j] + thc[j+mxy])*dzi2;

        cc = - (cw + ce + cs + cn + cb + ct + eps[j]*dens[j]*specht[j]*dti); 

        // index of matrix
        icc = i;
        icw = i - 1;
        ice = i + 1;
        ics = i - nx;
        icn = i + nx;
        icb = i - nx*ny;
        ict = i + nx*ny;

#ifdef PW
        const type dd = cc;
        int flag_is_solid[6] = { 0 , 0 , 0, 0, 0, 0};
        if (ls[j] + ls[j-1  ] >= 0.0) { cc += cw; cw = 0.0; flag_is_solid[0] = 1; }
        if (ls[j] + ls[j+1  ] >= 0.0) { cc += ce; ce = 0.0; flag_is_solid[1] = 1; }
        if (ls[j] + ls[j-mx ] >= 0.0) { cc += cs; cs = 0.0; flag_is_solid[2] = 1; }
        if (ls[j] + ls[j+mx ] >= 0.0) { cc += cn; cn = 0.0; flag_is_solid[3] = 1; }
        if (ls[j] + ls[j-mxy] >= 0.0) { cc += cb; cb = 0.0; flag_is_solid[4] = 1; }
        if (ls[j] + ls[j+mxy] >= 0.0) { cc += ct; ct = 0.0; flag_is_solid[5] = 1; }
        /*
        if (ls[j] >= 0.0 || ls[j-1  ] >= 0.0) { cc += cw; cw = 0.0; flag_is_solid[0] = 1; }
        if (ls[j] >= 0.0 || ls[j+1  ] >= 0.0) { cc += ce; ce = 0.0; flag_is_solid[1] = 1; }
        if (ls[j] >= 0.0 || ls[j-mx ] >= 0.0) { cc += cs; cs = 0.0; flag_is_solid[2] = 1; }
        if (ls[j] >= 0.0 || ls[j+mx ] >= 0.0) { cc += cn; cn = 0.0; flag_is_solid[3] = 1; }
        if (ls[j] >= 0.0 || ls[j-mxy] >= 0.0) { cc += cb; cb = 0.0; flag_is_solid[4] = 1; }
        if (ls[j] >= 0.0 || ls[j+mxy] >= 0.0) { cc += ct; ct = 0.0; flag_is_solid[5] = 1; }
        */
        if (flag_is_solid[0] && flag_is_solid[1] && flag_is_solid[2] && flag_is_solid[3] && flag_is_solid[4] && flag_is_solid[5]) {
          cc = dd;
          cw = ce = cs = cn = cb = ct = -dd/6.0*0.0;
        }
#endif
        // Neumann-boundary
        if(nrk[4] == -1 && jx == 0   ) {
          icw = -1;
          cc = boundary_matrix_coeff_T(val->bnd_W.th,
                                       jy + cdo->stmb, jz + cdo->stmb,
                                       cdo->nby, cdo->nbz, cc, cw, &b[j_]);
        }
        if(nrk[5] == -1 && jx == nx-1) {
          ice = -1;
          cc = boundary_matrix_coeff_T(val->bnd_E.th,
                                       jy + cdo->stmb, jz + cdo->stmb,
                                       cdo->nby, cdo->nbz, cc, ce, &b[j_]);
        }
        if(nrk[2] == -1 && jy == 0   ) {
          ics = -1;
          cc = boundary_matrix_coeff_T(val->bnd_S.th,
                                       jx + cdo->stmb, jz + cdo->stmb,
                                       cdo->nbx, cdo->nbz, cc, cs, &b[j_]);
        }
        if(nrk[3] == -1 && jy == ny-1) {
          icn = -1;
          cc = boundary_matrix_coeff_T(val->bnd_N.th,
                                       jx + cdo->stmb, jz + cdo->stmb,
                                       cdo->nbx, cdo->nbz, cc, cn, &b[j_]);
        }
        if(nrk[0] == -1 && jz == 0   ) {
          icb = -1;
          cc = boundary_matrix_coeff_T(val->bnd_B.th,
                                       jx + cdo->stmb, jy + cdo->stmb,
                                       cdo->nbx, cdo->nby, cc, cb, &b[j_]);
        }
        if(nrk[1] == -1 && jz == nz-1) {
          ict = -1;
          cc = boundary_matrix_coeff_T(val->bnd_T.th,
                                       jx + cdo->stmb, jy + cdo->stmb,
                                       cdo->nbx, cdo->nby, cc, ct, &b[j_]);
        }

        if(icb != -1){A[j_+0*m_]=cb;}else{A[j_+0*m_]=0.0;}
        if(ics != -1){A[j_+1*m_]=cs;}else{A[j_+1*m_]=0.0;}
        if(icw != -1){A[j_+2*m_]=cw;}else{A[j_+2*m_]=0.0;}
        if(icc != -1){A[j_+3*m_]=cc;}else{A[j_+3*m_]=0.0;}
        if(ice != -1){A[j_+4*m_]=ce;}else{A[j_+4*m_]=0.0;}
        if(icn != -1){A[j_+5*m_]=cn;}else{A[j_+5*m_]=0.0;}
        if(ict != -1){A[j_+6*m_]=ct;}else{A[j_+6*m_]=0.0;}
      }
    }
  }

  return 0;
}

int make_matrix_array_Ts(type *A, type *b, type *dens,type *thc,type *specht,variable *val, parameter *prm,mpi_prm *cg_prm)
{

  flags     *flg=prm->flg;
  domain    *cdo=prm->cdo;
  mpi_param *mpi=prm->mpi;

  int i,j,jx,jy,jz,
    nx=cdo->nx, ny=cdo->ny, nz=cdo->nz, n=nx*ny*nz,
    mx=cdo->mx, my=cdo->my, mz=cdo->mz, m=mx*my*mz,
    mxy=mx*my;

  int stm=cdo->stm;
  int icc,icw,ice,ics,icn,icb,ict;
  type cc, cw, ce, cs, cn, cb, ct,
    dxi2=cdo->dxi*cdo->dxi,
    dyi2=cdo->dyi*cdo->dyi,
    dzi2=cdo->dzi*cdo->dzi;

  int rank;
  rank=mpi->rank;
  int nrk[6];
  for(i=0;i<6;i++){
    nrk[i]=mpi->nrk[i];
  }
  type dti = 1.0/cdo->dt;

  int j_;
  int stm_;
  int m_;
  int mx_,my_,mxy_;

  stm_ = cg_prm->stm;
  m_   = cg_prm->m;
  mx_  = cg_prm->mx;
  my_  = cg_prm->my;
  mxy_ = cg_prm->mxy;

  type *sgm=val->sgm;
  type *eps=val->eps;
  type *epss=val->epss;
  type *ls=val->ls;
  type eps_w, eps_e, eps_s, eps_n, eps_b, eps_t;

#pragma omp parallel for                             \
  private(i,j,j_,jx,jy,jz,                           \
  cw,ce,cs,cn,cb,ct,cc,icc,icw,ice,ics,icn,icb,ict,  \
  eps_w,eps_e,eps_s,eps_n,eps_b,eps_t               )
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
        j_ = (jx+stm_) + mx_*(jy+stm_) + mxy_*(jz+stm_);
        i = (rank*n) + jx + nx*jy + nx*ny*jz;

        // coefficient of matrix
        eps_w = 1.0 - MIN2(epss[j], epss[j-1]);
        eps_e = 1.0 - MIN2(epss[j], epss[j+1]);
        eps_s = 1.0 - MIN2(epss[j], epss[j-mx]);
        eps_n = 1.0 - MIN2(epss[j], epss[j+mx]);
        eps_b = 1.0 - MIN2(epss[j], epss[j-mxy]);
        eps_t = 1.0 - MIN2(epss[j], epss[j+mxy]);

        // harmonic mean
        cw = eps_w*2.0/(1.0/thc[j] + 1.0/thc[j-1  ])*dxi2;
        ce = eps_e*2.0/(1.0/thc[j] + 1.0/thc[j+1  ])*dxi2;
        cs = eps_s*2.0/(1.0/thc[j] + 1.0/thc[j-mx ])*dyi2;
        cn = eps_n*2.0/(1.0/thc[j] + 1.0/thc[j+mx ])*dyi2;
        cb = eps_b*2.0/(1.0/thc[j] + 1.0/thc[j-mxy])*dzi2;
        ct = eps_t*2.0/(1.0/thc[j] + 1.0/thc[j+mxy])*dzi2;

        // Arithmetric mean
        //cw = eps_w*0.5*(thc[j] + thc[j-1  ])*dxi2;
        //ce = eps_e*0.5*(thc[j] + thc[j+1  ])*dxi2;
        //cs = eps_s*0.5*(thc[j] + thc[j-mx ])*dyi2;
        //cn = eps_n*0.5*(thc[j] + thc[j+mx ])*dyi2;
        //cb = eps_b*0.5*(thc[j] + thc[j-mxy])*dzi2;
        //ct = eps_t*0.5*(thc[j] + thc[j+mxy])*dzi2;

        cc = - (cw + ce + cs + cn + cb + ct + (1.0-eps[j])*dens[j]*specht[j]*dti + 1.0e-10);
        // index of matrix
        icc = i;
        icw = i - 1;
        ice = i + 1;
        ics = i - nx;
        icn = i + nx;
        icb = i - nx*ny;
        ict = i + nx*ny;

//#ifdef PW
/*
        const type dd = cc;
        int flag_is_solid[6] = {0, 0, 0, 0, 0, 0};
        if (ls[j] + ls[j-1  ] <= 0.0) { cc += cw; cw = 0.0; flag_is_solid[0] = 1; }
        if (ls[j] + ls[j+1  ] <= 0.0) { cc += ce; ce = 0.0; flag_is_solid[1] = 1; }
        if (ls[j] + ls[j-mx ] <= 0.0) { cc += cs; cs = 0.0; flag_is_solid[2] = 1; }
        if (ls[j] + ls[j+mx ] <= 0.0) { cc += cn; cn = 0.0; flag_is_solid[3] = 1; }
        if (ls[j] + ls[j-mxy] <= 0.0) { cc += cb; cb = 0.0; flag_is_solid[4] = 1; }
        if (ls[j] + ls[j+mxy] <= 0.0) { cc += ct; ct = 0.0; flag_is_solid[5] = 1; }
        if (flag_is_solid[0] && flag_is_solid[1] && flag_is_solid[2] && flag_is_solid[3] && flag_is_solid[4] && flag_is_solid[5]) {
          //cc = dd;
          //cw = ce = cs = cn = cb = ct = -dd/6.0*0.0;
        }
        */
//#endif

        // Neumann-boundary
        if(nrk[4] == -1 && jx == 0   ) {
          icw = -1;
          cc = boundary_matrix_coeff_T(val->bnd_W.th,
                                       jy + cdo->stmb, jz + cdo->stmb,
                                       cdo->nby, cdo->nbz, cc, cw, &b[j_]);
        }
        if(nrk[5] == -1 && jx == nx-1) {
          ice = -1;
          cc = boundary_matrix_coeff_T(val->bnd_E.th,
                                       jy + cdo->stmb, jz + cdo->stmb,
                                       cdo->nby, cdo->nbz, cc, ce, &b[j_]);
        }
        if(nrk[2] == -1 && jy == 0   ) {
          ics = -1;
          cc = boundary_matrix_coeff_T(val->bnd_S.th,
                                       jx + cdo->stmb, jz + cdo->stmb,
                                       cdo->nbx, cdo->nbz, cc, cs, &b[j_]);
        }
        if(nrk[3] == -1 && jy == ny-1) {
          icn = -1;
          cc = boundary_matrix_coeff_T(val->bnd_N.th,
                                       jx + cdo->stmb, jz + cdo->stmb,
                                       cdo->nbx, cdo->nbz, cc, cn, &b[j_]);
        }
        if(nrk[0] == -1 && jz == 0   ) {
          icb = -1;
          cc = boundary_matrix_coeff_T(val->bnd_B.th,
                                       jx + cdo->stmb, jy + cdo->stmb,
                                       cdo->nbx, cdo->nby, cc, cb, &b[j_]);
        }
        if(nrk[1] == -1 && jz == nz-1) {
          ict = -1;
          cc = boundary_matrix_coeff_T(val->bnd_T.th,
                                       jx + cdo->stmb, jy + cdo->stmb,
                                       cdo->nbx, cdo->nby, cc, ct, &b[j_]);
        }

        if(icb != -1){A[j_+0*m_]=cb;}else{A[j_+0*m_]=0.0;}
        if(ics != -1){A[j_+1*m_]=cs;}else{A[j_+1*m_]=0.0;}
        if(icw != -1){A[j_+2*m_]=cw;}else{A[j_+2*m_]=0.0;}
        if(icc != -1){A[j_+3*m_]=cc;}else{A[j_+3*m_]=0.0;}
        if(ice != -1){A[j_+4*m_]=ce;}else{A[j_+4*m_]=0.0;}
        if(icn != -1){A[j_+5*m_]=cn;}else{A[j_+5*m_]=0.0;}
        if(ict != -1){A[j_+6*m_]=ct;}else{A[j_+6*m_]=0.0;}
      }
    }
  }

  return 0;
}


static
type boundary_matrix_coeff_U(fluid_boundary_data **cond_array,
                             int jbx, int jby, int nbx, int nby,
                             type cc, type cn, type *b, int flag,
                             boundary_direction dir)
{
  fluid_boundary_data *fp, *fn;
  type vin;
  int perp_bnd;
  int xbnd;
  int ybnd;
  int zbnd;
  ptrdiff_t jj;

  xbnd = (dir & (BOUNDARY_DIR_WEST | BOUNDARY_DIR_EAST));
  ybnd = (dir & (BOUNDARY_DIR_SOUTH | BOUNDARY_DIR_NORTH));
  zbnd = (dir & (BOUNDARY_DIR_BOTTOM | BOUNDARY_DIR_TOP));

  perp_bnd = 1;
  jj = jbx + jby * nbx;
  fp = cond_array[jj];

  if ((flag == 0 && !xbnd) || (flag == 1 && !ybnd) || (flag == 2 && !zbnd)) {
    /* parallel to boundary */
    perp_bnd = 0;
  }

  fn = NULL;
  if (!perp_bnd) {
    if (flag == 0) {        /* cond_array = X x Y or X x Z */
      fn = cond_array[jj - 1];
    } else if (flag == 1) {
      if (xbnd) {           /* cond_array = Y x Z */
        fn = cond_array[jj - 1];
      } else if (zbnd) {    /* cond_array = X x Y */
        fn = cond_array[jj - nbx];
      }
    } else if (flag == 2) { /* cond_array = X x Z or Y x Z */
      fn = cond_array[jj - nbx];
    }
    if (fn && fp != fn) {
      switch(fp->cond) {
      case WALL:
      case SLIP:
        if (fn->cond == WALL) {
          fp = fn;
        }
        break;
      default:
        switch(fn->cond) {
        case WALL:
        case SLIP:
          fp = fn;
          break;
        case OUT:
        case INLET:
          if (fp->cond != INLET) {
            fp = fn;
          }
          break;
        }
      }
    }
  }

  switch(fp->cond) {
  case WALL:
    return cc;
  case SLIP:
    if (perp_bnd) {
      return cc;
    } else {
      return cc + cn;
    }
  case OUT:
    return cc + cn;
  case INLET:
    vin = 0.0;
    if (flag == 0) {        /* U */
      vin = fp->inlet_vel_u.current_value;
    } else if (flag == 1) { /* V */
      vin = fp->inlet_vel_v.current_value;
    } else if (flag == 2) { /* W */
      vin = fp->inlet_vel_w.current_value;
    }
    if (perp_bnd) {
      *b -= cn * vin;
      return cc;
    }
    *b -= 2.0 * cn * vin; /* add to source term */
    return cc - cn;
  default:
    return cc;
  }
}

int make_matrix_array_U(int flag, type *A, type *b, type *dens,type *mu,variable *val, parameter *prm,mpi_prm *cg_prm)
{
  flags     *flg=prm->flg;
  domain    *cdo=prm->cdo;
  mpi_param *mpi=prm->mpi;

  int i,j,jx,jy,jz,
    nx=cdo->nx, ny=cdo->ny, nz=cdo->nz, n=nx*ny*nz,
    mx=cdo->mx, my=cdo->my, mz=cdo->mz, m=mx*my*mz,
    mxy=mx*my;

  int stm=cdo->stm;
  int icc,icw,ice,ics,icn,icb,ict;
  type cc, cw, ce, cs, cn, cb, ct,
    dxi2=cdo->dxi*cdo->dxi,
    dyi2=cdo->dyi*cdo->dyi,
    dzi2=cdo->dzi*cdo->dzi;
  type dti = 1.0/cdo->dt;

  //type *liquid_film = val->liquid_film;
  //type vis_augment_w=1.0, vis_augment_e=1.0, vis_augment_s=1.0, vis_augment_n=1.0, vis_augment_b=1.0, vis_augment_t=1.0;
  //type rapture_thickness = cdo->rapture_thickness;

  int rank;
  rank=mpi->rank;

  int nrk[6];
  for(i=0;i<6;i++){
    nrk[i]=mpi->nrk[i];
  }

  int j_;
  int stm_;
  int m_;
  int mx_,my_,mxy_;

  stm_ = cg_prm->stm;
  m_   = cg_prm->m;
  mx_  = cg_prm->mx;
  my_  = cg_prm->my;
  mxy_ = cg_prm->mxy;

  switch(flag) {
  case 0:
    if (mpi->rank_x == mpi->npe_x - 1) {
      nx += 1;
    }
    break;
  case 1:
    if (mpi->rank_y == mpi->npe_y - 1) {
      ny += 1;
    }
    break;
  case 2:
    if (mpi->rank_z == mpi->npe_z - 1) {
      nz += 1;
    }
    break;
  }

  /* Yamashita 2020/05/19  */
  type *fs = val->fs_sum;
  type *eps = val->eps;
  type *perm = val->perm;

#pragma omp parallel for                                    \
  private(i,j,j_,jx,jy,jz,cw,ce,cs,cn,cb,ct,cc,icc,icw,ice,ics,icn,icb,ict)
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        type dens_c, mu_c, fs_c, ep_c, ep_n, per_c, per_n, ep_term;

        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
        j_ = (jx+stm_) + mx_*(jy+stm_) + mxy_*(jz+stm_);
        i = (rank*n) + jx + nx*jy + nx*ny*jz;

        //if(prm->flg->film_drainage){
        //  if(cdo->z[jz] > cdo->height_threshold){
        //    int offset;
        //    if(flag==0)      offset = 1;
        //    else if(flag==1) offset = mx;
        //    else if(flag==2) offset = mxy;
        //
        //    vis_augment_w = viscosity_augmentation(liquid_film, j, j-1  , offset, rapture_thickness);
        //    vis_augment_e = viscosity_augmentation(liquid_film, j, j+1  , offset, rapture_thickness);
        //    vis_augment_s = viscosity_augmentation(liquid_film, j, j-mx , offset, rapture_thickness);
        //    vis_augment_n = viscosity_augmentation(liquid_film, j, j+mx , offset, rapture_thickness);
        //    vis_augment_b = viscosity_augmentation(liquid_film, j, j-mxy, offset, rapture_thickness);
        //    vis_augment_t = viscosity_augmentation(liquid_film, j, j+mxy, offset, rapture_thickness);            
        //  }
        //}

        //-- coefficient of matrix
        if(flag == 0){
          cw = mu[j-1]*dxi2;
          ce = mu[j  ]*dxi2;
          cs = 0.25*(mu[j] + mu[j-1] + mu[j-mx] + mu[j-1-mx])*dyi2;
          cn = 0.25*(mu[j] + mu[j-1] + mu[j+mx] + mu[j-1+mx])*dyi2;
          cb = 0.25*(mu[j] + mu[j-1] + mu[j-mxy] + mu[j-1-mxy])*dzi2;
          ct = 0.25*(mu[j] + mu[j-1] + mu[j+mxy] + mu[j-1+mxy])*dzi2;
          dens_c = 0.5*(dens[j-1] + dens[j]);
          /* yamashita 2020/06/09 */
          ep_term = 0.0;
          if (flg->porous == ON) {
            mu_c = 0.5 * (mu[j] + mu[j - 1]);
            ep_c = eps[j];
            ep_n = eps[j - 1];
            per_c = perm[j];
            per_n = perm[j - 1];
            // ep_c = MAX2(eps[j], eps[j-1]);
            // per_c = MAX2(perm[j], perm[j-1]);
            ep_c = MIN2(ep_c, ep_n);
            per_c = MIN2(per_c, per_n);
            if (ep_c > 0.0) {
              ep_term = ep_c * mu_c * per_c;
            }
          }
          cc = -(cw + ce + cs + cn + cb + ct + dens_c * dti + ep_term + 1.0e-8);
        }else if(flag == 1){
          cw = 0.25*(mu[j] + mu[j-1] + mu[j-mx] + mu[j-1-mx])*dxi2;
          ce = 0.25*(mu[j] + mu[j+1] + mu[j-mx] + mu[j+1-mx])*dxi2;
          cs = mu[j-mx]*dyi2;
          cn = mu[j   ]*dyi2;
          cb = 0.25*(mu[j] + mu[j-mx] + mu[j-mxy] + mu[j-mx-mxy])*dzi2;
          ct = 0.25*(mu[j] + mu[j-mx] + mu[j+mxy] + mu[j-mx+mxy])*dzi2;
          dens_c = 0.5*(dens[j-mx] + dens[j]);
          /* yamashita 2020/06/09  */
          ep_term = 0.0;
          if (flg->porous == ON) {
            mu_c = 0.5 * (mu[j] + mu[j - mx]);
            ep_c = eps[j];
            ep_n = eps[j - mx];
            per_c = perm[j];
            per_n = perm[j - mx];
            //ep_c = MAX2(eps[j], eps[j-mx]);
            //per_c = MAX2(perm[j], perm[j-mx]);
            ep_c = MIN2(ep_c, ep_n);
            per_c = MIN2(per_c, per_n);
            if (ep_c > 0.0) {
              ep_term = ep_c * mu_c * per_c;
            }
          }
          cc = -(cw + ce + cs + cn + cb + ct + dens_c * dti + ep_term + 1.0e-8);
        }else if(flag == 2){
          cw = 0.25*(mu[j] + mu[j-1] + mu[j-mxy] + mu[j-1-mxy])*dxi2;
          ce = 0.25*(mu[j] + mu[j+1] + mu[j-mxy] + mu[j+1-mxy])*dxi2;
          cs = 0.25*(mu[j] + mu[j-mxy] + mu[j-mx] + mu[j-mxy-mx])*dyi2;
          cn = 0.25*(mu[j] + mu[j-mxy] + mu[j+mx] + mu[j-mxy+mx])*dyi2;
          cb = mu[j-mxy]*dzi2;
          ct = mu[j    ]*dzi2;
          dens_c = 0.5*(dens[j-mxy] + dens[j]);
          /* yamashita 2020/06/09 */
          ep_term = 0.0;
          if (flg->porous == ON) {
            mu_c = 0.5 * (mu[j] + mu[j - mxy]);
            ep_c = eps[j];
            ep_n = eps[j - mxy];
            per_c = perm[j];
            per_n = perm[j - mxy];
            // ep_c = MAX2(eps[j], eps[j-mxy]);
            // per_c = MAX2(perm[j], perm[j-mxy]);
            ep_c = MIN2(ep_c, ep_n);
            per_c = MIN2(per_c, per_n);
            if (ep_c > 0.0) {
              ep_term = ep_c * mu_c * per_c;
            }
          }
          cc = -(cw + ce + cs + cn + cb + ct + dens_c * dti + ep_term + 1.0e-8);
        }
        // index of matrix
        icc = i;
        icw = i - 1;
        ice = i + 1;
        ics = i - nx;
        icn = i + nx;
        icb = i - nx*ny;
        ict = i + nx*ny;

        if(nrk[4] == -1 && jx == 0   ) {
          icw = -1;
          cc = boundary_matrix_coeff_U(val->bnd_W.fl,
                                       jy + cdo->stmb, jz + cdo->stmb,
                                       cdo->nby, cdo->nbz, cc, cw, &b[j_],
                                       flag, BOUNDARY_DIR_WEST);
        }
        if(nrk[5] == -1 && jx == nx-1) {
          ice = -1;
          cc = boundary_matrix_coeff_U(val->bnd_E.fl,
                                       jy + cdo->stmb, jz + cdo->stmb,
                                       cdo->nby, cdo->nbz, cc, ce, &b[j_],
                                       flag, BOUNDARY_DIR_EAST);
        }
        if(nrk[2] == -1 && jy == 0   ) {
          ics = -1;
          cc = boundary_matrix_coeff_U(val->bnd_S.fl,
                                       jx + cdo->stmb, jz + cdo->stmb,
                                       cdo->nbx, cdo->nbz, cc, cs, &b[j_],
                                       flag, BOUNDARY_DIR_SOUTH);
        }
        if(nrk[3] == -1 && jy == ny-1) {
          icn = -1;
          cc = boundary_matrix_coeff_U(val->bnd_N.fl,
                                       jx + cdo->stmb, jz + cdo->stmb,
                                       cdo->nbx, cdo->nbz, cc, cn, &b[j_],
                                       flag, BOUNDARY_DIR_NORTH);
        }
        if(nrk[0] == -1 && jz == 0   ) {
          icb = -1;
          cc = boundary_matrix_coeff_U(val->bnd_B.fl,
                                       jx + cdo->stmb, jy + cdo->stmb,
                                       cdo->nbx, cdo->nby, cc, cb, &b[j_],
                                       flag, BOUNDARY_DIR_BOTTOM);
        }
        if(nrk[1] == -1 && jz == nz-1) {
          ict = -1;
          cc = boundary_matrix_coeff_U(val->bnd_T.fl,
                                       jx + cdo->stmb, jy + cdo->stmb,
                                       cdo->nbx, cdo->nby, cc, ct, &b[j_],
                                       flag, BOUNDARY_DIR_TOP);
        }
        if(icb != -1){A[j_+0*m_]=cb;}else{A[j_+0*m_]=0.0;}
        if(ics != -1){A[j_+1*m_]=cs;}else{A[j_+1*m_]=0.0;}
        if(icw != -1){A[j_+2*m_]=cw;}else{A[j_+2*m_]=0.0;}
        if(icc != -1){A[j_+3*m_]=cc;}else{A[j_+3*m_]=0.0;}
        if(ice != -1){A[j_+4*m_]=ce;}else{A[j_+4*m_]=0.0;}
        if(icn != -1){A[j_+5*m_]=cn;}else{A[j_+5*m_]=0.0;}
        if(ict != -1){A[j_+6*m_]=ct;}else{A[j_+6*m_]=0.0;}

      }
    }
  }
  return 0;
}

static int
make_matrix_array_flg(int topo_flag, type *A, type *b,
                      int stmx, int stmy, int stmz,
                      int stpx, int stpy, int stpz,
                      int mx, int my, int mz,
                      parameter *prm, void *arg);

struct make_matrix_array_flg_data
{
  int flag;
  type *dens;
  type *specht;
  material *mtl;
  variable *val;
};

int ccse_poisson(int flag, type *f, type *div, type *dens, material *mtl, variable *val, parameter *prm)
{
  const char *name;
  int itrmax;
  type rtolmax;
  type abstolmax;
  int topo_flag;
  struct make_matrix_array_flg_data fdata;

  fdata.flag = flag;
  fdata.dens = dens;
  fdata.mtl = mtl;
  fdata.val = val;

  if (prm->flg->porous == ON) {
    if (prm->flg->two_energy == OFF) fdata.specht = mtl->c_f;
    else                             fdata.specht = mtl->specht;
  } else
    fdata.specht = mtl->specht;
  // flag == 0 : U
  // flag == 1 : V
  // flag == 2 : W
  // flag == 3 : P
  // flag == 4 : T
  // flag == 5 : Tf
  // flag == 6 : Ts
  if(flag == 0){
    itrmax   =30000;    //反復回数の最大値
    rtolmax  =1.e-6;   // 相対残差の収束条件
    abstolmax=1.0e-50; // 絶対残差の収束条件
    name = "U";
    topo_flag = 1;
  }else if(flag == 1){
    itrmax   =30000;    //反復回数の最大値
    rtolmax  =1.e-6;   // 相対残差の収束条件
    abstolmax=1.0e-50; // 絶対残差の収束条件
    name = "V";
    topo_flag = 2;
  }else if(flag == 2){
    itrmax   =30000;    //反復回数の最大値
    rtolmax  =1.e-6;   // 相対残差の収束条件
    abstolmax=1.0e-50; // 絶対残差の収束条件
    name = "W";
    topo_flag = 3;
  }else if(flag == 3){
    itrmax   =100000;    //反復回数の最大値
    rtolmax  =1.e-5;   // 相対残差の収束条件
    abstolmax=1.0e-50; // 絶対残差の収束条件
    name = "P";
    topo_flag = 0;
  }else if(flag == 4){
    itrmax   =30000;    //反復回数の最大値
    rtolmax  =1.e-8;   // 相対残差の収束条件
    abstolmax=1.0e-50; // 絶対残差の収束条件
    name = "T";
    topo_flag = 0;
  }else if(flag == 5){
    itrmax   =30000;    //反復回数の最大値
    rtolmax  =1.e-8;   // 相対残差の収束条件
    abstolmax=1.0e-50; // 絶対残差の収束条件
    name = "Tf";
    topo_flag = 0;
  }else if(flag == 6){
    itrmax   =30000;    //反復回数の最大値
    rtolmax  =1.e-8;   // 相対残差の収束条件
    abstolmax=1.0e-50; // 絶対残差の収束条件
    name = "Ts";
    topo_flag = 0;
  } else {
    return -1;
  }


  return ccse_poisson_f(topo_flag, name, f, div, prm, itrmax, rtolmax,
                        abstolmax, make_matrix_array_flg, &fdata);
}

static int
make_matrix_array_flg(int topo_flag, type *A, type *b,
                      int stmx, int stmy, int stmz,
                      int stpx, int stpy, int stpz,
                      int mx, int my, int mz,
                      parameter *prm, void *arg)
{
  mpi_prm cg_prm;
  struct make_matrix_array_flg_data *fdata;
  int flag;
  material *mtl;
  variable *val;
  type *dens, *specht;

  fdata = (struct make_matrix_array_flg_data *)arg;
  flag = fdata->flag;
  mtl = fdata->mtl;
  val = fdata->val;
  dens = fdata->dens;
  specht = fdata->specht;

  convert(topo_flag, prm, &cg_prm);

  if(flag == 0){
    return make_matrix_array_U(flag,A,b,dens,mtl->mu,val,prm,&cg_prm);
  }else if(flag == 1){
    return make_matrix_array_U(flag,A,b,dens,mtl->mu,val,prm,&cg_prm);
  }else if(flag == 2){
    return make_matrix_array_U(flag,A,b,dens,mtl->mu,val,prm,&cg_prm);
  }else if(flag == 3){
    return make_matrix_array(A,dens,val,prm,&cg_prm);
  }else if(flag == 4){
    //return make_matrix_array_T(A,b,dens,mtl->thc,mtl->specht,val,prm,&cg_prm);
    return make_matrix_array_T(A,b,dens,mtl->thc,specht,val,prm,&cg_prm);
  }else if(flag == 5){
    return make_matrix_array_Tf(A,b,dens,mtl->thcf,mtl->c_f,val,prm,&cg_prm);
  }else if(flag == 6){
    return make_matrix_array_Ts(A,b,mtl->denss,mtl->thcs,specht,val,prm,&cg_prm);
  }else{
    return -1;
  }
}
