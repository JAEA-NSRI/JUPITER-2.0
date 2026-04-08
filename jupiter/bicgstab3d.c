#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "struct.h"

int make_matrix(type **A, type *dens, parameter *prm);
//int set_source(Vec *b, type *div, int n, mpi_param *mpi);
//int Vec2Value(type *f, Vec *x, domain *cdo, mpi_param *mpi);


int Bicgstab3d(type *f, type **A, type *div, type *dens, parameter *prm)
{
  domain    *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;
  int  n = cdo->n, gn = n*mpi->npe;

  int    is, ie, its;
  type   res=1.0;
  //static Mat  A;
  //static Vec  x, b;
  //static KSP  ksp;
  //static PC   pc;
  static int   n1st = 0;
  static char  help[] = "MPI test";

  make_matrix(A, dens, prm);

  MPI_Barrier(mpi->CommJUPITER);

  if ( cdo->icnt % cdo->view == 0 && mpi->rank == 0 ) {
    printf("    iter     = %d\n", its);
  }

//printf("rank[%d], test_3\n", prm->mpi->rank);
//   Vec2Value(f, &x, cdo, mpi);

  return its;
}


int make_matrix(type **A, type *dens, parameter *prm)
{
  flags     *flg=prm->flg;
  domain    *cdo=prm->cdo;
  mpi_param *mpi=prm->mpi;
  int j,jx,jy,jz, i,
      nx=cdo->nx, ny=cdo->ny, nz=cdo->nz, n=nx*ny*nz,
      mx=cdo->mx, my=cdo->my, mxy=mx*my;
  int icc,icw,ice,ics,icn,icb,ict;
  type cc, cw, ce, cs, cn, cb, ct,
       dxi2=cdo->dxi*cdo->dxi,
       dyi2=cdo->dyi*cdo->dyi,
       dzi2=cdo->dzi*cdo->dzi;

  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+cdo->stm) + mx*(jy+cdo->stm) + mxy*(jz+cdo->stm);
        i = (mpi->rank*n) + jx + nx*jy + nx*ny*jz;

        // coefficient of matrix
        cb = 2.0/(dens[j] + dens[j-mxy])*dzi2;
        cs = 2.0/(dens[j] + dens[j-mx ])*dyi2;
        cw = 2.0/(dens[j] + dens[j-1  ])*dxi2;
        ce = 2.0/(dens[j] + dens[j+1  ])*dxi2;
        cn = 2.0/(dens[j] + dens[j+mx ])*dyi2;
        ct = 2.0/(dens[j] + dens[j+mxy])*dzi2;
        cc = - (cw + ce + cs + cn + cb + ct);
        A[0][j] = cb;
        A[1][j] = cs;
        A[2][j] = cw;
        A[3][j] = cc;
        A[4][j] = ce;
        A[5][j] = cn;
        A[6][j] = ct;

        // index of matrix
        /*
        icc = i;
        icw = i - 1;
        ice = i + 1;
        ics = i - nx;
        icn = i + nx;
        icb = i - nx*ny;
        ict = i + nx*ny;

        if(mpi->nrk[4] > -1 && jx == 0   ) icw = i - n + (nx-1);
        if(mpi->nrk[5] > -1 && jx == nx-1) ice = i + n - (nx-1);
        if(mpi->nrk[2] > -1 && jy == 0   ) ics = i - n*mpi->npe_x + nx*(ny-1);
        if(mpi->nrk[3] > -1 && jy == ny-1) icn = i + n*mpi->npe_x - nx*(ny-1);
        if(mpi->nrk[0] > -1 && jz == 0   ) icb = i - n*mpi->npe_xy + nx*ny*(nz-1);
        if(mpi->nrk[1] > -1 && jz == nz-1) ict = i + n*mpi->npe_xy - nx*ny*(nz-1);
        */

//        if(flg->outflow == ON) {
//          // Dirichlet-boundary
//          if(mpi->nrk[4] == -1 && jx == 0   ) { cc += cw;  icw = -1; }
//          if(mpi->nrk[5] == -1 && jx == nx-1) { cc += ce;  ice = -1; }
//          if(mpi->nrk[2] == -1 && jy == 0   ) { cc += cs;  ics = -1; }
//          if(mpi->nrk[3] == -1 && jy == ny-1) { cc += cn;  icn = -1; }
//          if(mpi->nrk[0] == -1 && jz == 0   ) {            icb = -1; }
//          if(mpi->nrk[1] == -1 && jz == nz-1) {            ict = -1; }
//        } else {
//          // Neumann-boundary
//          if(mpi->nrk[4] == -1 && jx == 0   ) { cc += cw;  icw = -1; }
//          if(mpi->nrk[5] == -1 && jx == nx-1) { cc += ce;  ice = -1; }
//          if(mpi->nrk[2] == -1 && jy == 0   ) { cc += cs;  ics = -1; }
//          if(mpi->nrk[3] == -1 && jy == ny-1) { cc += cn;  icn = -1; }
//          if(mpi->nrk[0] == -1 && jz == 0   ) { cc += cb;  icb = -1; }
//          if(mpi->nrk[1] == -1 && jz == nz-1) { cc += ct;  ict = -1; }
//        }

        if(mpi->nrk[4] == -1 && jx == 0   ) {
          j = (jx+cdo->stm) + mx*(jy+cdo->stm) + mxy*(jz+cdo->stm);
          if(flg->bc_xm == OUT) {            icw = -1; }
          //else                  { cc += cw;  icw = -1; }
          else                  { A[3][j] += A[2][j];  icw = -1; }
        }
        if(mpi->nrk[5] == -1 && jx == nx-1) {
          j = (jx+cdo->stm) + mx*(jy+cdo->stm) + mxy*(jz+cdo->stm);
          if(flg->bc_xp == OUT) {            ice = -1; }
          //else                  { cc += ce;  ice = -1; }
          else                  { A[3][j] += A[4][j];  ice = -1; }
        }
        if(mpi->nrk[2] == -1 && jy == 0   ) {
          j = (jx+cdo->stm) + mx*(jy+cdo->stm) + mxy*(jz+cdo->stm);
          if(flg->bc_ym == OUT) {            ics = -1; }
          //else                  { cc += cs;  ics = -1; }
          else                  { A[3][j] += A[1][j];  ics = -1; }
        }
        if(mpi->nrk[3] == -1 && jy == ny-1) {
          j = (jx+cdo->stm) + mx*(jy+cdo->stm) + mxy*(jz+cdo->stm);
          if(flg->bc_yp == OUT) {            icn = -1; }
          //else                  { cc += cn;  icn = -1; }
          else                  { A[3][j] += A[5][j];  icn = -1; }
        }
        if(mpi->nrk[0] == -1 && jz == 0   ) {
          j = (jx+cdo->stm) + mx*(jy+cdo->stm) + mxy*(jz+cdo->stm);
          if(flg->bc_zm == OUT) {            icb = -1; }
          //else                  { cc += cb;  icb = -1; }
          else                  { A[3][j] += A[0][j];  icb = -1; }
        }
        if(mpi->nrk[1] == -1 && jz == nz-1) {
          j = (jx+cdo->stm) + mx*(jy+cdo->stm) + mxy*(jz+cdo->stm);
          if(flg->bc_zp == OUT) {            ict = -1; }
          //else                  { cc += ct;  ict = -1; }
          else                  { A[3][j] += A[6][j];  ict = -1; }
        }

        // Make value & index of the matrix
        /*
        if(icb != -1) MatSetValues(*A,1,&i,1,&icb,&cb,INSERT_VALUES);
        if(ics != -1) MatSetValues(*A,1,&i,1,&ics,&cs,INSERT_VALUES);
        if(icw != -1) MatSetValues(*A,1,&i,1,&icw,&cw,INSERT_VALUES);
        if(icc != -1) MatSetValues(*A,1,&i,1,&icc,&cc,INSERT_VALUES);
        if(ice != -1) MatSetValues(*A,1,&i,1,&ice,&ce,INSERT_VALUES);
        if(icn != -1) MatSetValues(*A,1,&i,1,&icn,&cn,INSERT_VALUES);
        if(ict != -1) MatSetValues(*A,1,&i,1,&ict,&ct,INSERT_VALUES);
        */
      }
    }
  }
//  MatAssemblyBegin(*A,MAT_FINAL_ASSEMBLY);
//  MatAssemblyEnd(*A,MAT_FINAL_ASSEMBLY);

  return 0;
}

/*
int set_source(Vec *b, type *div, int n, mpi_param *mpi)
{
  int i, gi;
  for(i = 0; i < n; i++) {
    gi = n*mpi->rank + i;
    VecSetValues(*b, 1, &gi, &div[i], INSERT_VALUES);
  }
  VecAssemblyBegin(*b);
  VecAssemblyEnd(*b);
  return 0;
}
*/

/*
int Vec2Value(type *f, Vec *x, domain *cdo, mpi_param *mpi)
{
  int i, j, jx, jy, jz;
  type val;
  for(jz = 0; jz < cdo->nz; jz++) {
    for(jy = 0; jy < cdo->ny; jy++) {
      for(jx = 0; jx < cdo->nx; jx++) {
        j = (jx+cdo->stm) + cdo->mx*(jy+cdo->stm) + cdo->mxy*(jz+cdo->stm);
        i = cdo->n*mpi->rank + jx + cdo->nx*jy + cdo->nxy*jz;
        VecGetValues(*x,1,&i,&val);
        f[j] = val;
      }
    }
  }
  return 0;
}
*/
