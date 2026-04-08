#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "struct.h"
#include "cg.h"
#include "sleev_comm_block3d.h"

#define BLOCK_1DLOOOP 1
// #define BLOCK_1DLOOOP 0

int XYZ_to_blockXYZ(
		    mpi_prm prm,type *f_xyz,type *f_bxyz,
		    int *block_nxs, int *block_nxe, 
		    int *block_nys, int *block_nye, 
		    int *block_nzs, int *block_nze	    
		    ){
  
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;

  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int jzb,jjzb;
  int jyb,jjyb;
  int jxb,jjxb;

#pragma omp parallel for private(jzb,jjzb,jyb,jjyb,jxb,jjxb) // collapse(3)
  for(jzb=0;jzb<nzdivblock;jzb++){	     
    for(jjzb=block_nzs[jzb];jjzb<=block_nze[jzb];jjzb++){    
      for(jyb=0;jyb<nydivblock;jyb++){	     
	for(jjyb=block_nys[jyb];jjyb<=block_nye[jyb];jjyb++){    	
	  for(jxb=0;jxb<nxdivblock;jxb++){ 
	    for(jjxb=block_nxs[jxb];jjxb<=block_nxe[jxb];jjxb++){    
	      int jz = jzb * nzblock + jjzb;
	      int jy = jyb * nyblock + jjyb;
	      int jx = jxb * nxblock + jjxb;
	      int j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

	      int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	      int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	      int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;

	      // printf(" %04d,%04d : %04d \n",jzb,jjzb,jz);
	      // printf(" %04d,%04d : %04d \n",jyb,jjyb,jy);
	      // printf(" %04d,%04d : %04d \n",jxb,jjxb,jx);
	      // printf(" %04d,%04d,%04d \n",jx,jy,jz);
	      //	      printf(" %04d,%04d,%04d : %d \n",jx,jy,jz,j);
	      ///	      f_xyz[j] = 1.0;
	      f_bxyz[jj] = f_xyz[j];
	    }
	  }
	}
      }
    }
  }

  return 0;
}

int blockXYZ_to_XYZ(
		    mpi_prm prm,type *f_xyz,type *f_bxyz,
		    int *block_nxs, int *block_nxe, 
		    int *block_nys, int *block_nye, 
		    int *block_nzs, int *block_nze	    
		    ){
  
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;

  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int jzb,jjzb;
  int jyb,jjyb;
  int jxb,jjxb;

#pragma omp parallel for private(jzb,jjzb,jyb,jjyb,jxb,jjxb) // collapse(3)
  for(jzb=0;jzb<nzdivblock;jzb++){	     
    for(jjzb=block_nzs[jzb];jjzb<=block_nze[jzb];jjzb++){    
      for(jyb=0;jyb<nydivblock;jyb++){	     
	for(jjyb=block_nys[jyb];jjyb<=block_nye[jyb];jjyb++){    	
	  for(jxb=0;jxb<nxdivblock;jxb++){ 
	    for(jjxb=block_nxs[jxb];jjxb<=block_nxe[jxb];jjxb++){    
	      int jz = jzb * nzblock + jjzb;
	      int jy = jyb * nyblock + jjyb;
	      int jx = jxb * nxblock + jjxb;
	      int j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

	      int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	      int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	      int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;

	      // printf(" %04d,%04d : %04d \n",jzb,jjzb,jz);
	      // printf(" %04d,%04d : %04d \n",jyb,jjyb,jy);
	      // printf(" %04d,%04d : %04d \n",jxb,jjxb,jx);
	      // printf(" %04d,%04d,%04d \n",jx,jy,jz);
	      //	      printf(" @ xyz @ %04d,%04d,%04d : %d \n",jx,jy,jz,j);
	      ///	      f_xyz[j] = 1.0;
	      f_xyz[j] = f_bxyz[jj];
	    }
	  }
	}
      }
    }
  }

  return 0;
}

int axpy_block3D(mpi_prm prm,type *x,type *y,type alpha,type beta){

  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;
  int ndivblock  = nxdivblock * nydivblock * nzdivblock; 

  int jjxb,jjyb,jjzb;
  int jxb,jyb,jzb;
  int jb;

  // 内側インデックス  
#pragma omp parallel for private(jjxb,jjyb,jjzb,jxb,jyb,jzb,jb) // collapse(3)
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jjxb=0;jjxb<nxblock;jjxb++){
	
 	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
#if BLOCK_1DLOOOP
	for(jb=0;jb<ndivblock;jb++){{{
	      int jzb =  jb / (nxdivblock*nydivblock);
	      int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
	      int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
	for(jzb=0;jzb<nzdivblock;jzb++){
	  for(jyb=0;jyb<nydivblock;jyb++){
	    for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	      int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	      // int jx = jxb * nxblock + jjxb;
	      // int jy = jyb * nyblock + jjyb;
	      // int jz = jzb * nzblock + jjzb;
	      // printf(" %04d,%04d,%04d : %d \n",jx,jy,jz,j);
	      y[j] = alpha * x[j] + beta * y[j];
	    }
	  }
	}
      }
    }
   }

   return 0;
}

int pjacobi_block3D(mpi_prm prm,type *x,type *y,type *D){

  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;
  int ndivblock  = nxdivblock * nydivblock * nzdivblock; 

  int jjxb,jjyb,jjzb;
  int jxb,jyb,jzb;
  int jb;

  // 内側インデックス  
#pragma omp parallel for private(jjxb,jjyb,jjzb,jxb,jyb,jzb,jb) // collapse(3)
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jjxb=0;jjxb<nxblock;jjxb++){
	
 	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
#if BLOCK_1DLOOOP
	for(jb=0;jb<ndivblock;jb++){{{
	      int jzb =  jb / (nxdivblock*nydivblock);
	      int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
	      int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
	for(jzb=0;jzb<nzdivblock;jzb++){
	  for(jyb=0;jyb<nydivblock;jyb++){
	    for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	      int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	      // int jx = jxb * nxblock + jjxb;
	      // int jy = jyb * nyblock + jjyb;
	      // int jz = jzb * nzblock + jjzb;
	      // printf(" %04d,%04d,%04d : %d \n",jx,jy,jz,j);
#if 1
	      if(D[j]==0.0){
		y[j] = 0.0;
	      }else{
		y[j] = x[j] / D[j];
	      }
#else
		y[j] = x[j];
#endif
	    }
	  }
	}
      }
    }
   }

   return 0;
}

// z[] = z[] + a*y[]
// y[] = x[] + b*y[]
int axpy2_block3D(mpi_prm prm,type alpha,type beta,type *x,type *y,type *z){

  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;
  int ndivblock  = nxdivblock * nydivblock * nzdivblock; 

  int jjxb,jjyb,jjzb;
  int jxb,jyb,jzb;
  int jb;

  // 内側インデックス  
#pragma omp parallel for private(jjxb,jjyb,jjzb,jxb,jyb,jzb,jb) // collapse(3)
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jjxb=0;jjxb<nxblock;jjxb++){
	
 	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
#if BLOCK_1DLOOOP
	for(jb=0;jb<ndivblock;jb++){{{
	      int jzb =  jb / (nxdivblock*nydivblock);
	      int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
	      int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
	for(jzb=0;jzb<nzdivblock;jzb++){
	  for(jyb=0;jyb<nydivblock;jyb++){
	    for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	      int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	      // int jx = jxb * nxblock + jjxb;
	      // int jy = jyb * nyblock + jjyb;
	      // int jz = jzb * nzblock + jjzb;
	      // printf(" %04d,%04d,%04d : %d \n",jx,jy,jz,j);
	      //	      y[j] = alpha * x[j] + beta * y[j];
	      z[j] = alpha * y[j] + z[j];
	      y[j] = beta  * y[j] + x[j];
	    }
	  }
	}
      }
    }
   }

   return 0;
}

int calc_dot_local_nxnynz_block3D(
		    mpi_prm prm,type *x,type *y,type *ret_type,
		    int *block_nxs, int *block_nxe, 
		    int *block_nys, int *block_nye, 
		    int *block_nzs, int *block_nze	    
		    ){
  
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;

  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int jzb,jjzb;
  int jyb,jjyb;
  int jxb,jjxb;
  type tmp = 0.0; 

#pragma omp parallel for private(jzb,jjzb,jyb,jjyb,jxb,jjxb) reduction(+:tmp) // collapse(3)
  for(jzb=0;jzb<nzdivblock;jzb++){	     
    for(jjzb=block_nzs[jzb];jjzb<=block_nze[jzb];jjzb++){    
      for(jyb=0;jyb<nydivblock;jyb++){	     
	for(jjyb=block_nys[jyb];jjyb<=block_nye[jyb];jjyb++){    	
	  for(jxb=0;jxb<nxdivblock;jxb++){ 
	    for(jjxb=block_nxs[jxb];jjxb<=block_nxe[jxb];jjxb++){    
	      int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	      int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	      int jj  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;

	      tmp = tmp + x[jj] * y[jj];
	    }
	  }
	}
      }
    }
  }

  ret_type[0] = tmp;

  return 0;
}

 int calc_dot_nxnynz_block3D(
			     mpi_prm prm,type *x,type *y,type *ret_type,
			     int *block_nxs, int *block_nxe, 
			     int *block_nys, int *block_nye, 
			     int *block_nzs, int *block_nze	    
			     ){
  type tmp  = 0.0;
  calc_dot_local_nxnynz_block3D(prm,x,y,&tmp,
				block_nxs, block_nxe, 
				block_nys, block_nye, 
				block_nzs, block_nze	    
				);
#ifdef JUPITER_MPI
  MPI_Allreduce(&tmp,ret_type, 1,MPI_TYPE, MPI_SUM,prm.comm);
#else
  *ret_type = tmp;
#endif
  return 0;
}

 int calc_norm_nxnynz_block3D(mpi_prm prm,type *x,type *y,type *ret_type,
			     int *block_nxs, int *block_nxe, 
			     int *block_nys, int *block_nye, 
			     int *block_nzs, int *block_nze	    
			      ){
  type tmp = 0.0;
  calc_dot_nxnynz_block3D(prm,x,y,&tmp,
			  block_nxs, block_nxe, 
			  block_nys, block_nye, 
			  block_nzs, block_nze	    
			  );
  ret_type[0] = sqrt(tmp);
  return 0;
}


int calc_dot_local_block3D(mpi_prm prm,type *x,type *y,type *ret_type){

  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;
  int ndivblock  = nxdivblock * nydivblock * nzdivblock; 

  int jjxb,jjyb,jjzb;
  int jxb,jyb,jzb;
  int jb;

  type tmp = 0.0;
  // 内側インデックス  
#pragma omp parallel for private(jjxb,jjyb,jjzb,jxb,jyb,jzb,jb) reduction(+:tmp) // collapse(3)
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jjxb=0;jjxb<nxblock;jjxb++){
	
 	int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
#if BLOCK_1DLOOOP
	for(jb=0;jb<ndivblock;jb++){{{
	      int jzb =  jb / (nxdivblock*nydivblock);
	      int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
	      int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
	for(jzb=0;jzb<nzdivblock;jzb++){
	  for(jyb=0;jyb<nydivblock;jyb++){
	    for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	      int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	      // int jx = jxb * nxblock + jjxb;
	      // int jy = jyb * nyblock + jjyb;
	      // int jz = jzb * nzblock + jjzb;
	      // printf(" %04d,%04d,%04d : %d \n",jx,jy,jz,j);
	      tmp = tmp + x[j]*y[j];
	    } 
	  }
	}
      }
    }
   }

   ret_type[0] = tmp;

   return 0;
}

int calc_dot_block3D(mpi_prm prm,type *x,type *y,type *ret_type){
  type tmp  = 0.0;
  calc_dot_local_block3D(prm,x,y,&tmp);
#ifdef JUPITER_MPI
  MPI_Allreduce(&tmp,ret_type, 1,MPI_TYPE, MPI_SUM,prm.comm);
#else
  *ret_type = tmp;
#endif
  return 0;
}

int calc_norm_block3D(mpi_prm prm,type *x,type *y,type *ret_type){
  type tmp = 0.0;
  calc_dot_block3D(prm,x,y,&tmp);
  ret_type[0] = sqrt(tmp);
  return 0;
}


int MatVec_dot_local_block3D(
		       mpi_prm prm,type* A,type* x,type* y,type* ret_type,
		       int *stride_xp,int *stride_xm,
		       int *stride_yp,int *stride_ym,
		       int *stride_zp,int *stride_zm 
		       ){

  int block_m = prm.block_m;
  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;
  int ndivblock  = nxdivblock * nydivblock * nzdivblock; 

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int jjxb,jjyb,jjzb;
  int jxb,jyb,jzb;
  int jb;
  type tmp = 0.0;

#pragma omp parallel for private(jjxb,jjyb,jjzb,jxb,jyb,jzb,jb) reduction(+:tmp) // collapse(3)
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jjxb=0;jjxb<nxblock;jjxb++){

	const int stxm = stride_xm[jjxb];
	const int stxp = stride_xp[jjxb];
	const int stym = stride_ym[jjyb];
	const int styp = stride_yp[jjyb];
	const int stzm = stride_zm[jjzb];
	const int stzp = stride_zp[jjzb];

	// 内側インデックス
 	const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	

#if BLOCK_1DLOOOP
	for(jb=0;jb<ndivblock;jb++){{{
	      const int jzb =  jb / (nxdivblock*nydivblock);
	      const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
	      const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
	for(jzb=0;jzb<nzdivblock;jzb++){
	  for(jyb=0;jyb<nydivblock;jyb++){
	    for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	      const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;

	      const int jcb = j + stzm;
	      const int jcs = j + stym;
	      const int jcw = j + stxm;
	      const int jcc = j       ;
	      const int jce = j + stxp;
	      const int jcn = j + styp;
	      const int jct = j + stzp;

	      y[j]=
		 A[j + 0 * block_m]*x[jcb] 
		+A[j + 1 * block_m]*x[jcs] 
		+A[j + 2 * block_m]*x[jcw] 
		+A[j + 3 * block_m]*x[jcc] 
		+A[j + 4 * block_m]*x[jce] 
		+A[j + 5 * block_m]*x[jcn] 
		+A[j + 6 * block_m]*x[jct] 
		;
	      tmp=tmp+y[j]*x[j];
	    } 
	  }
	}
      }
     }
   }
	//printf(" ret_dot = %f \n",tmp);
   ret_type[0] = tmp;
   return 0;
}

int sMatVec_dot_local_block3D(
		       mpi_prm prm,type* A,type* x,type* y,type* ret_type,
		       int *stride_xp,int *stride_xm,
		       int *stride_yp,int *stride_ym,
		       int *stride_zp,int *stride_zm 
		       ){

  int block_m = prm.block_m;
  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;
  int ndivblock  = nxdivblock * nydivblock * nzdivblock; 

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int jjxb,jjyb,jjzb;
  int jxb,jyb,jzb;
  int jb;
  type tmp = 0.0;

#pragma omp parallel for private(jjxb,jjyb,jjzb,jxb,jyb,jzb,jb) reduction(+:tmp) // collapse(3)
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jjxb=0;jjxb<nxblock;jjxb++){

	const int stxm = stride_xm[jjxb];
	const int stxp = stride_xp[jjxb];
	const int stym = stride_ym[jjyb];
	const int styp = stride_yp[jjyb];
	const int stzm = stride_zm[jjzb];
	const int stzp = stride_zp[jjzb];

	// 内側インデックス
 	const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	

#if BLOCK_1DLOOOP
	for(jb=0;jb<ndivblock;jb++){{{
	      const int jzb =  jb / (nxdivblock*nydivblock);
	      const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
	      const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
	for(jzb=0;jzb<nzdivblock;jzb++){
	  for(jyb=0;jyb<nydivblock;jyb++){
	    for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	      const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;

	      const int jcb = j + stzm;
	      const int jcs = j + stym;
	      const int jcw = j + stxm;
	      const int jcc = j       ;
	      const int jce = j + stxp;
	      const int jcn = j + styp;
	      const int jct = j + stzp;
#if 1
	y[j]=
	   A[j   + 0 * block_m]*x[jcb] 
	  +A[j   + 1 * block_m]*x[jcs] 
	  +A[j   + 2 * block_m]*x[jcw] 
	  +A[j   + 3 * block_m]*x[jcc] 
	  +A[jce + 2 * block_m]*x[jce] 
	  +A[jcn + 1 * block_m]*x[jcn] 
	  +A[jct + 0 * block_m]*x[jct] 
	  ;

#else
	      y[j]=
		 A[j + 0 * block_m]*x[jcb] 
		+A[j + 1 * block_m]*x[jcs] 
		+A[j + 2 * block_m]*x[jcw] 
		+A[j + 3 * block_m]*x[jcc] 
		+A[j + 4 * block_m]*x[jce] 
		+A[j + 5 * block_m]*x[jcn] 
		+A[j + 6 * block_m]*x[jct] 
		;
#endif
	      tmp=tmp+y[j]*x[j];
	    } 
	  }
	}
      }
     }
   }
	//printf(" ret_dot = %f \n",tmp);
   ret_type[0] = tmp;
   return 0;
}

int MatVec_dot_local_nxnynz_block3D(
		       mpi_prm prm,type* A,type* x,type* y,type* ret_type,
		       int *block_nxs, int *block_nxe, 
		       int *block_nys, int *block_nye, 
		       int *block_nzs, int *block_nze,
		       int *stride_xp,int *stride_xm,
		       int *stride_yp,int *stride_ym,
		       int *stride_zp,int *stride_zm 
		       ){

  int block_m = prm.block_m;
  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;
  int ndivblock  = nxdivblock * nydivblock * nzdivblock; 

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int jjxb,jjyb,jjzb;
  int jxb,jyb,jzb;
  int jb;
  type tmp = 0.0;


#if BLOCK_1DLOOOP
  // #pragma omp parallel for private(jb)
  for(jb=0;jb<ndivblock;jb++){{{
      const int jzb =  jb / (nxdivblock*nydivblock);
      const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
      const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
      // #pragma omp parallel for private(jxb,jyb,jzb)
      for(jzb=0;jzb<nzdivblock;jzb++){
	for(jyb=0;jyb<nydivblock;jyb++){
	  for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	    const int jjzbs = block_nzs[jzb];
	    const int jjzbe = block_nze[jzb];
	    const int jjybs = block_nys[jyb];
	    const int jjybe = block_nye[jyb];
	    const int jjxbs = block_nxs[jxb];
	    const int jjxbe = block_nxe[jxb];

	    for(jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
	      for(jjyb=jjybs;jjyb<=jjybe;jjyb++){    
		for(jjxb=jjxbs;jjxb<=jjxbe;jjxb++){

		  const int stxm = stride_xm[jjxb];
		  const int stxp = stride_xp[jjxb];
		  const int stym = stride_ym[jjyb];
		  const int styp = stride_yp[jjyb];
		  const int stzm = stride_zm[jjzb];
		  const int stzp = stride_zp[jjzb];

		  // 内側インデックス
		  const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
		  const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	

		  const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;

		  const int jcb = j + stzm;
		  const int jcs = j + stym;
		  const int jcw = j + stxm;
		  const int jcc = j       ;
		  const int jce = j + stxp;
		  const int jcn = j + styp;
		  const int jct = j + stzp;
		  y[j]=
		    A[j + 0 * block_m]*x[jcb] 
		    +A[j + 1 * block_m]*x[jcs] 
		    +A[j + 2 * block_m]*x[jcw] 
		    +A[j + 3 * block_m]*x[jcc] 
		    +A[j + 4 * block_m]*x[jce] 
		    +A[j + 5 * block_m]*x[jcn] 
		    +A[j + 6 * block_m]*x[jct] 
		;
	      tmp=tmp+y[j]*x[j];

		}
	      }
	    }
	  }
	}
      }

   ret_type[0] = tmp;
   return 0;
}

int sMatVec_dot_local_nxnynz_block3D(
		       mpi_prm prm,type* A,type* x,type* y,type* ret_type,
		       int *block_nxs, int *block_nxe, 
		       int *block_nys, int *block_nye, 
		       int *block_nzs, int *block_nze,
		       int *stride_xp,int *stride_xm,
		       int *stride_yp,int *stride_ym,
		       int *stride_zp,int *stride_zm 
		       ){

  int block_m = prm.block_m;
  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;
  int ndivblock  = nxdivblock * nydivblock * nzdivblock; 

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int jjxb,jjyb,jjzb;
  int jxb,jyb,jzb;
  int jb;
  type tmp = 0.0;


#if BLOCK_1DLOOOP
  // #pragma omp parallel for private(jb)
  for(jb=0;jb<ndivblock;jb++){{{
      const int jzb =  jb / (nxdivblock*nydivblock);
      const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
      const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
      // #pragma omp parallel for private(jxb,jyb,jzb)
      for(jzb=0;jzb<nzdivblock;jzb++){
	for(jyb=0;jyb<nydivblock;jyb++){
	  for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	    const int jjzbs = block_nzs[jzb];
	    const int jjzbe = block_nze[jzb];
	    const int jjybs = block_nys[jyb];
	    const int jjybe = block_nye[jyb];
	    const int jjxbs = block_nxs[jxb];
	    const int jjxbe = block_nxe[jxb];

	    for(jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
	      for(jjyb=jjybs;jjyb<=jjybe;jjyb++){    
		for(jjxb=jjxbs;jjxb<=jjxbe;jjxb++){
	const int stxm = stride_xm[jjxb];
	const int stxp = stride_xp[jjxb];
	const int stym = stride_ym[jjyb];
	const int styp = stride_yp[jjyb];
	const int stzm = stride_zm[jjzb];
	const int stzp = stride_zp[jjzb];

	// 内側インデックス
 	const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	

	      const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;

	      const int jcb = j + stzm;
	      const int jcs = j + stym;
	      const int jcw = j + stxm;
	      const int jcc = j       ;
	      const int jce = j + stxp;
	      const int jcn = j + styp;
	      const int jct = j + stzp;

#if 1
	      y[j]=
		 A[j   + 0 * block_m]*x[jcb] 
		+A[j   + 1 * block_m]*x[jcs] 
		+A[j   + 2 * block_m]*x[jcw] 
		+A[j   + 3 * block_m]*x[jcc] 
		+A[jce + 2 * block_m]*x[jce] 
		+A[jcn + 1 * block_m]*x[jcn] 
		+A[jct + 0 * block_m]*x[jct] 
		;

#else
	      y[j]=
		 A[j + 0 * block_m]*x[jcb] 
		+A[j + 1 * block_m]*x[jcs] 
		+A[j + 2 * block_m]*x[jcw] 
		+A[j + 3 * block_m]*x[jcc] 
		+A[j + 4 * block_m]*x[jce] 
		+A[j + 5 * block_m]*x[jcn] 
		+A[j + 6 * block_m]*x[jct] 
		;
#endif
	      tmp=tmp+y[j]*x[j];

		}
	      }
	    }
	  }
	}
      }

   ret_type[0] = tmp;
   return 0;
}

int calc_res_nxnynz_block3D(
		       mpi_prm prm,type* A,type* x,type* b,type* r,
		       int *block_nxs, int *block_nxe, 
		       int *block_nys, int *block_nye, 
		       int *block_nzs, int *block_nze,
		       int *stride_xp,int *stride_xm,
		       int *stride_yp,int *stride_ym,
		       int *stride_zp,int *stride_zm 
		       ){
  type ret_type = 0.0;
  MatVec_dot_local_nxnynz_block3D(
		     prm, A, x, r, &ret_type,
		     block_nxs, block_nxe, 
		     block_nys, block_nye, 
		     block_nzs, block_nze,
		     stride_xp,stride_xm,
		     stride_yp,stride_ym,
		     stride_zp,stride_zm 
		     );
  axpy_block3D(prm,b,r,1.0,-1.0);
  return 0;

}

int calc_res_block3D(
		       mpi_prm prm,type* A,type* x,type* b,type* r,
		       int *stride_xp,int *stride_xm,
		       int *stride_yp,int *stride_ym,
		       int *stride_zp,int *stride_zm 
		       ){
  type ret_type = 0.0;
  MatVec_dot_local_block3D(
		     prm, A, x, r, &ret_type,
		     stride_xp,stride_xm,
		     stride_yp,stride_ym,
		     stride_zp,stride_zm 
		     );
  axpy_block3D(prm,b,r,1.0,-1.0);
  return 0;

}

int calc_sres_block3D(
		       mpi_prm prm,type* A,type* x,type* b,type* r,
		       int *stride_xp,int *stride_xm,
		       int *stride_yp,int *stride_ym,
		       int *stride_zp,int *stride_zm 
		       ){
  type ret_type = 0.0;
  sMatVec_dot_local_block3D(
		     prm, A, x, r, &ret_type,
		     stride_xp,stride_xm,
		     stride_yp,stride_ym,
		     stride_zp,stride_zm 
		     );
  axpy_block3D(prm,b,r,1.0,-1.0);
  return 0;

}

int make_Matvec_ovl_flter(
			  mpi_prm prm,
			  int *block_nxs, int *block_nxe, 
			  int *block_nys, int *block_nye, 
			  int *block_nzs, int *block_nze,			  
			  type *block_West_filter   , type *block_East_filter,
			  type *block_South_filter  , type *block_North_filter,
			  type *block_Bottom_filter , type *block_Top_filter
			  ){
  /*  
  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  */
  
  int block_m = prm.block_m;
  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int mzblock    = nzblock+2;
  int myblock    = nyblock+2;
  int mxblock    = nxblock+2;
  int mblock = mzblock*myblock*mxblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;
  int ndivblock  = nxdivblock * nydivblock * nzdivblock; 

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int j;
  int jxb,jyb,jzb;
  int jjxb,jjyb,jjzb;
  
  int jjzbs,jjzbe;
  int jjybs,jjybe;
  int jjxbs,jjxbe;
  
  for(j=0;j<nzblock*nzdivblock;j++){
    block_Bottom_filter[j] = 0.0;
    block_Top_filter[j]    = 0.0;
  }
  for(jzb=0;jzb<nzdivblock;jzb++){
     jjzbs = block_nzs[jzb];
     jjzbe = block_nze[jzb];
    for(jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
      block_Bottom_filter[jjzb - jjzbs + jzb*nzblock] = 1.0;
      block_Top_filter[jjzb - jjzbs + jzb*nzblock]    = 1.0;
    }
  }  
  block_Bottom_filter[0] = 0.0;
  
  jzb=nzdivblock-1;
  jjzbs = block_nzs[jzb];
  jjzbe = block_nze[jzb];
  block_Top_filter[jjzbe - jjzbs + jzb*nzblock] = 0.0;

  // ------------
  
  for(j=0;j<nyblock*nydivblock;j++){
    block_South_filter[j] = 0.0;
    block_North_filter[j]    = 0.0;
  }
  for(jyb=0;jyb<nydivblock;jyb++){
     jjybs = block_nys[jyb];
     jjybe = block_nye[jyb];
    for(jjyb=jjybs;jjyb<=jjybe;jjyb++){    
      block_South_filter[jjyb - jjybs + jyb*nyblock] = 1.0;
      block_North_filter[jjyb - jjybs + jyb*nyblock]    = 1.0;
    }
  }
  
  block_South_filter[0] = 0.0;
  
  jyb=nydivblock-1;
  jjybs = block_nys[jyb];
  jjybe = block_nye[jyb];
  block_North_filter[jjybe - jjybs + jyb*nyblock] = 0.0;

  for(j=0;j<nyblock*nydivblock;j++){
    //        printf(" %d : %f \n",j,block_South_filter[j]);
    //        printf(" %d : %f \n",j,block_North_filter[j]);
  }
  for(jyb=0;jyb<nydivblock;jyb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      //      printf(" @ South @ %d , %f \n",jjyb + jyb*nyblock,block_South_filter[ jjyb + jyb*nyblock ]);
    }
  }
  // ------------
  for(j=0;j<nxblock*nxdivblock;j++){
    block_West_filter[j] = 0.0;
    block_East_filter[j]    = 0.0;
  }
  for(jxb=0;jxb<nxdivblock;jxb++){
     jjxbs = block_nxs[jxb];
     jjxbe = block_nxe[jxb];
    for(jjxb=jjxbs;jjxb<=jjxbe;jjxb++){    
      block_West_filter[jjxb - jjxbs + jxb*nxblock] = 1.0;
      block_East_filter[jjxb - jjxbs + jxb*nxblock] = 1.0;
    }
  }
  
  block_West_filter[0] = 0.0;
  
  jxb=nxdivblock-1;
  jjxbs = block_nxs[jxb];
  jjxbe = block_nxe[jxb];
  block_East_filter[jjxbe - jjxbs + jxb*nxblock] = 0.0;
  
  for(j=0;j<nxblock*nxdivblock;j++){
    //    printf(" %d : %f \n",j,block_West_filter[j]);
    //    printf(" %d : %f \n",j,block_East_filter[j]);
  }
  return 0;

}
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
				   ){

  int stm;
  int nx,ny,nz;
  int m;
  int mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int block_m = prm.block_m;
  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int mzblock    = nzblock+2;
  int myblock    = nyblock+2;
  int mxblock    = nxblock+2;
  int mblock = mzblock*myblock*mxblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;
  int ndivblock  = nxdivblock * nydivblock * nzdivblock; 

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int j;
  int jxb,jyb,jzb;
  int jjxb,jjyb,jjzb;

#pragma omp parallel for private(j)
  for(j=0;j<block_m;j++){
    D[j]=1.0;
  }

  for(j=0;j<nzblock*nzdivblock;j++){
    block_zfilterL[j] = 0.0;
    block_zfilterU[j] = 0.0;
  }
  for(jzb=0;jzb<nzdivblock;jzb++){
    const int jjzbs = block_nzs[jzb];
    const int jjzbe = block_nze[jzb];
    for(jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
      block_zfilterL[jjzb - jjzbs + jzb*nzblock] = 1.0;
      block_zfilterU[jjzb - jjzbs + jzb*nzblock] = 1.0;
    }
    block_zfilterL[jjzbs - jjzbs + jzb*nzblock] = 0.0;
    block_zfilterU[jjzbe - jjzbs + jzb*nzblock] = 0.0;
  }

  for(jyb=0;jyb<nydivblock;jyb++){
    const int jjybs = block_nys[jyb];
    const int jjybe = block_nye[jyb];
    for(jjyb=jjybs;jjyb<=jjybe;jjyb++){    
      block_yfilterL[jjyb - jjybs + jyb*nyblock] = 1.0;
      block_yfilterU[jjyb - jjybs + jyb*nyblock] = 1.0;
    }
    block_yfilterL[jjybs - jjybs + jyb*nyblock] = 0.0;
    block_yfilterU[jjybe - jjybs + jyb*nyblock] = 0.0;
  }

  for(jxb=0;jxb<nxdivblock;jxb++){
    const int jjxbs = block_nxs[jxb];
    const int jjxbe = block_nxe[jxb];
    for(jjxb=jjxbs;jjxb<=jjxbe;jjxb++){    
      block_xfilterL[jjxb - jjxbs + jxb*nxblock] = 1.0;
      block_xfilterU[jjxb - jjxbs + jxb*nxblock] = 1.0;
    }
    block_xfilterL[jjxbs - jjxbs + jxb*nxblock] = 0.0;
    block_xfilterU[jjxbe - jjxbs + jxb*nxblock] = 0.0;
  }


#if BLOCK_1DLOOOP
  int jb;
	for(jb=0;jb<ndivblock;jb++){{{
	      jzb =  jb / (nxdivblock*nydivblock);
	      jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
	      jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
	for(jzb=0;jzb<nzdivblock;jzb++){
	  for(jyb=0;jyb<nydivblock;jyb++){
	    for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     


#if 0
	      const int jjzbs = block_nzs[jzb];
	      const int jjzbe = block_nze[jzb];
	      const int jjybs = block_nys[jyb];
	      const int jjybe = block_nye[jyb];
	      const int jjxbs = block_nxs[jxb];
	      const int jjxbe = block_nxe[jxb];
#else
	      const int jjzbs = 0;
	      const int jjzbe = nzblock-1; 
	      const int jjybs = 0; 
	      const int jjybe = nyblock-1; 
	      const int jjxbs = 0; 
	      const int jjxbe = nxblock-1; 
#endif
  for(jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
    for(jjyb=jjybs;jjyb<=jjybe;jjyb++){    
      for(jjxb=jjxbs;jjxb<=jjxbe;jjxb++){

    
	      const int stxm = stride_xm[jjxb];
	      const int stxp = stride_xp[jjxb];
	      const int stym = stride_ym[jjyb];
	      const int styp = stride_yp[jjyb];
	      const int stzm = stride_zm[jjzb];
	      const int stzp = stride_zp[jjzb];

	      const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	      const int j  = 
		(jxb+1) + (jyb+1) * mxdivblock + (jzb+1) * mydivblock * mxdivblock + 
		jj0 * (mxdivblock * mydivblock * mzdivblock);
	      const int jcb = j + stzm;
	      const int jcs = j + stym;
	      const int jcw = j + stxm;
	      const int jcc = j       ;
	      const int jce = j + stxp;
	      const int jcn = j + styp;
	      const int jct = j + stzp;

	      const int jz = jjzb - jjzbs;
	      const int jy = jjyb - jjybs;
	      const int jx = jjxb - jjxbs;
	      const int jj = (jx+stm) + (jy+stm)*mxblock + (jz+stm) * mxblock*myblock ;		
	      const int jjcb = jj - mxblock*myblock;
	      const int jjcs = jj - mxblock;
	      const int jjcw = jj - 1;

	      type xfilter = block_xfilterL[jjxb - jjxbs + jxb*nxblock] * block_xfilterU[jjxb - jjxbs + jxb*nxblock];
	      type yfilter = block_yfilterL[jjyb - jjybs + jyb*nyblock] * block_yfilterU[jjyb - jjybs + jyb*nyblock];
	      type zfilter = block_zfilterL[jjzb - jjzbs + jzb*nzblock] * block_zfilterU[jjzb - jjzbs + jzb*nzblock];
	      
	      D[j] = A[jcc + 3*block_m] - ( 
	   A[jcc + 0 * block_m] * A[jcb + 6 * block_m] / D[jcb] * zfilter
	 + A[jcc + 1 * block_m] * A[jcs + 5 * block_m] / D[jcs] * yfilter
	 + A[jcc + 2 * block_m] * A[jcw + 4 * block_m] / D[jcw] * xfilter
			    );
	      
	    }
	  }
	}


      }
    }
  }

#pragma omp parallel for private(j)
  for(j=0;j<block_m;j++){
    //  printf(" j = %08d : D[j] = %e,%e \n",j, D[j],A[j + 3* block_m]);
    D[j]=1.0/D[j];
  }

  return 0;

}

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
					     ){

  int block_m = prm.block_m;
  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int mzblock    = nzblock+2;
  int myblock    = nyblock+2;
  int mxblock    = nxblock+2;
  int mblock = mzblock*myblock*mxblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;
  int ndivblock  = nxdivblock * nydivblock * nzdivblock; 

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int jjxb,jjyb,jjzb;

#if BLOCK_1DLOOOP
#else
  int jxb,jyb,jzb;
#endif
  int jb;

  type sr_dot = 0.0;
  type rr_dot = 0.0;

#if 0
  // 前処理無し
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jjxb=0;jjxb<nxblock;jjxb++){

	const int stxm = stride_xm[jjxb];
	const int stym = stride_ym[jjyb];
	const int stzm = stride_zm[jjzb];

	// 内側インデックス
 	const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	

#if BLOCK_1DLOOOP
#pragma omp parallel for private(jb) reduction(+:sr_dot,rr_dot)
	for(jb=0;jb<ndivblock;jb++){{{
	      const int jzb =  jb / (nxdivblock*nydivblock);
	      const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
	      const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
#pragma omp parallel for private(jxb,jyb,jzb)
	      for(jzb=0;jzb<nzdivblock;jzb++){
		for(jyb=0;jyb<nydivblock;jyb++){
		  for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	      const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	      const int jcb = j + stzm;
	      const int jcs = j + stym;
	      const int jcw = j + stxm;

	      r[j] = r[j] + q[j]*alpha;
	      s[j] = r[j] / A[j + 3*block_m];
	      sr_dot = sr_dot + s[j]*r[j];
	      rr_dot = rr_dot + r[j]*r[j];

	    } 
	  }
	}
       }
     }
   }

   sr_dot_local[0] = sr_dot;
   rr_dot_local[0] = rr_dot;

  return 0;

#endif

  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jjxb=0;jjxb<nxblock;jjxb++){

	const int stxm = stride_xm[jjxb];
	const int stym = stride_ym[jjyb];
	const int stzm = stride_zm[jjzb];

	// 内側インデックス
 	const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	

#if BLOCK_1DLOOOP
#pragma omp parallel for private(jb)
	for(jb=0;jb<ndivblock;jb++){{{
	      const int jzb =  jb / (nxdivblock*nydivblock);
	      const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
	      const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
#pragma omp parallel for private(jxb,jyb,jzb)
	      for(jzb=0;jzb<nzdivblock;jzb++){
		for(jyb=0;jyb<nydivblock;jyb++){
		  for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	      const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	      const int jcb = j + stzm;
	      const int jcs = j + stym;
	      const int jcw = j + stxm;

	      r[j] = r[j] + q[j]*alpha;
	      s[j] = ( r[j] - ( 
	      		     A[j + 2*block_m]*s[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
			     A[j + 1*block_m]*s[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
			     A[j + 0*block_m]*s[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
			      ) ) * Dinv[j];
	    } 
	  }
	}
       }
     }
   }
  

	for(jjzb=nzblock-1;jjzb>-1;jjzb--){
	  for(jjyb=nyblock-1;jjyb>-1;jjyb--){
	    for(jjxb=nxblock-1;jjxb>-1;jjxb--){
    
	const int stxp = stride_xp[jjxb];
	const int styp = stride_yp[jjyb];
	const int stzp = stride_zp[jjzb];

	// 内側インデックス
 	const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	

#if BLOCK_1DLOOOP
#pragma omp parallel for private(jb) reduction(+:sr_dot,rr_dot)
	for(jb=0;jb<ndivblock;jb++){{{
	      const int jzb =  jb / (nxdivblock*nydivblock);
	      const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
	      const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
#pragma omp parallel for private(jxb,jyb,jzb) reduction(+:sr_dot,rr_dot)
	      for(jzb=0;jzb<nzdivblock;jzb++){
		for(jyb=0;jyb<nydivblock;jyb++){
		  for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     

	      const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	      const int jce = j + stxp;
	      const int jcn = j + styp;
	      const int jct = j + stzp;

	      s[j] = s[j] - (
	                   A[j + 4*block_m]*s[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			   A[j + 5*block_m]*s[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			   A[j + 6*block_m]*s[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
			   )*Dinv[j];

	      sr_dot = sr_dot + s[j]*r[j];
	      rr_dot = rr_dot + r[j]*r[j];

	    } 
	  }
	}

      }
     }
   }

#if 0
   sr_dot_local[0] = sr_dot;
   rr_dot_local[0] = rr_dot;
   return 0;
#else
   sr_dot = 0.0;
   rr_dot = 0.0;
#endif

#ifdef JUPITER_MPI
	// @@@  sleev_comm(s,&prm); 
 sleev_comm_block3D(s,&prm,
		       block_nxs,  block_nxe, 
		       block_nys,  block_nye, 
		       block_nzs,  block_nze);

#endif

#if sMatrix
	calc_sres_block3D(
			 prm,A,s,r,z,
			 stride_xp,stride_xm,
			 stride_yp,stride_ym,
			 stride_zp,stride_zm 
			 );

#else
	calc_res_block3D(
			 prm,A,s,r,z,
			 stride_xp,stride_xm,
			 stride_yp,stride_ym,
			 stride_zp,stride_zm 
			 );
#endif

  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jjxb=0;jjxb<nxblock;jjxb++){

	const int stxm = stride_xm[jjxb];
	const int stxp = stride_xp[jjxb];
	const int stym = stride_ym[jjyb];
	const int styp = stride_yp[jjyb];
	const int stzm = stride_zm[jjzb];
	const int stzp = stride_zp[jjzb];

	// 内側インデックス
 	const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	

#if BLOCK_1DLOOOP
#pragma omp parallel for private(jb)
	for(jb=0;jb<ndivblock;jb++){{{
	      const int jzb =  jb / (nxdivblock*nydivblock);
	      const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
	      const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
#pragma omp parallel for private(jxb,jyb,jzb)
	for(jzb=0;jzb<nzdivblock;jzb++){
	  for(jyb=0;jyb<nydivblock;jyb++){
	    for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	      const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	      const int jcb = j + stzm;
	      const int jcs = j + stym;
	      const int jcw = j + stxm;

	      // const int jcc = j       ;
	      // const int jce = j + stxp;
	      // const int jcn = j + styp;
	      // const int jct = j + stzp;

	      z[j] = ( z[j] - ( 
			     A[j + 2*block_m]*z[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
			     A[j + 1*block_m]*z[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
			     A[j + 0*block_m]*z[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
			      ) ) * Dinv[j];

	    } 
	  }
	}

	    }
	  }
	}

	for(jjzb=nzblock-1;jjzb>-1;jjzb--){
	  for(jjyb=nyblock-1;jjyb>-1;jjyb--){
	    for(jjxb=nxblock-1;jjxb>-1;jjxb--){
    
	const int stxp = stride_xp[jjxb];
	const int styp = stride_yp[jjyb];
	const int stzp = stride_zp[jjzb];

	// 内側インデックス
 	const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock); 

#if BLOCK_1DLOOOP
#pragma omp parallel for private(jb) reduction(+:sr_dot,rr_dot)
	for(jb=0;jb<ndivblock;jb++){{{
	      const int jzb =  jb / (nxdivblock*nydivblock);
	      const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
	      const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
#pragma omp parallel for private(jxb,jyb,jzb) reduction(+:sr_dot,rr_dot)
	for(jzb=0;jzb<nzdivblock;jzb++){
	  for(jyb=0;jyb<nydivblock;jyb++){
	    for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	      const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	      const int jce = j + stxp;
	      const int jcn = j + styp;
	      const int jct = j + stzp;

	      z[j] = z[j] - (
			   A[j + 4*block_m]*z[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			   A[j + 5*block_m]*z[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			   A[j + 6*block_m]*z[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
			   )*Dinv[j];
	      s[j] = s[j] + z[j] ;
	      sr_dot = sr_dot + s[j]*r[j];
	      rr_dot = rr_dot + r[j]*r[j];
	    } 
	  }
	}

      }
     }
   }

   sr_dot_local[0] = sr_dot;
   rr_dot_local[0] = rr_dot;
   return 0;
}

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
					     ){

  int block_m = prm.block_m;
  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int mzblock    = nzblock+2;
  int myblock    = nyblock+2;
  int mxblock    = nxblock+2;
  int mblock = mzblock*myblock*mxblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;
  int ndivblock  = nxdivblock * nydivblock * nzdivblock; 

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int jjxb,jjyb,jjzb;

#if BLOCK_1DLOOOP
#else
  int jxb,jyb,jzb;
#endif
  int jb;

  type sr_dot = 0.0;
  type rr_dot = 0.0;
  
#if 1
  // 前処理無し

  // #pragma omp parallel for private(jb,jjxb,jjyb,jjzb) reduction(+:sr_dot,rr_dot)
  for(jb=0;jb<ndivblock;jb++){
    const int jzb =  jb / (nxdivblock*nydivblock);
    const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
    const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
    const int jjzbs = block_nzs[jzb];
    const int jjzbe = block_nze[jzb];
    const int jjybs = block_nys[jyb];
    const int jjybe = block_nye[jyb];
    const int jjxbs = block_nxs[jxb];
    const int jjxbe = block_nxe[jxb];

    for(jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
      for(jjyb=jjybs;jjyb<=jjybe;jjyb++){    
	for(jjxb=jjxbs;jjxb<=jjxbe;jjxb++){
	  
	  // 内側インデックス
	  const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	  const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
	  const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
	  
	  r[j] = r[j] + q[j]*alpha;
	  s[j] = r[j] / A[j + 3*block_m];
	  sr_dot = sr_dot + s[j]*r[j];
	  rr_dot = rr_dot + r[j]*r[j];
	  
	} 
      }
    }
  }
  
   sr_dot_local[0] = sr_dot;
   rr_dot_local[0] = rr_dot;

  return 0;

#endif

#if 1

#if BLOCK_1DLOOOP
  // #pragma omp parallel for private(jb)
  for(jb=0;jb<ndivblock;jb++){{{
      const int jzb =  jb / (nxdivblock*nydivblock);
      const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
      const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
      // #pragma omp parallel for private(jxb,jyb,jzb)
      for(jzb=0;jzb<nzdivblock;jzb++){
	for(jyb=0;jyb<nydivblock;jyb++){
	  for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	    const int jjzbs = block_nzs[jzb];
	    const int jjzbe = block_nze[jzb];
	    const int jjybs = block_nys[jyb];
	    const int jjybe = block_nye[jyb];
	    const int jjxbs = block_nxs[jxb];
	    const int jjxbe = block_nxe[jxb];

	    for(jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
	      for(jjyb=jjybs;jjyb<=jjybe;jjyb++){    
		for(jjxb=jjxbs;jjxb<=jjxbe;jjxb++){
		  const int stxm = stride_xm[jjxb];
		  const int stym = stride_ym[jjyb];
		  const int stzm = stride_zm[jjzb];

		  // 内側インデックス
		  const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
		  const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
		  const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
		  const int jcb = j + stzm;
		  const int jcs = j + stym;
		  const int jcw = j + stxm;

		  r[j] = r[j] + q[j]*alpha;
		  s[j] = ( r[j] - ( 
				   A[j + 2*block_m]*s[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
				   A[j + 1*block_m]*s[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
				   A[j + 0*block_m]*s[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
				    ) ) * Dinv[j];    
		}
	      }
	    }
	  }
	}
      }

#if BLOCK_1DLOOOP
  // #pragma omp parallel for private(jb)
  for(jb=0;jb<ndivblock;jb++){{{
      const int jzb =  jb / (nxdivblock*nydivblock);
      const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
      const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
      // #pragma omp parallel for private(jxb,jyb,jzb)
      for(jzb=0;jzb<nzdivblock;jzb++){
	for(jyb=0;jyb<nydivblock;jyb++){
	  for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	    const int jjzbs = block_nzs[jzb];
	    const int jjzbe = block_nze[jzb];
	    const int jjybs = block_nys[jyb];
	    const int jjybe = block_nye[jyb];
	    const int jjxbs = block_nxs[jxb];
	    const int jjxbe = block_nxe[jxb];

	    for(jjzb=jjzbe;jjzb>=jjzbs;jjzb--){    
	      for(jjyb=jjybe;jjyb>=jjybs;jjyb--){    
		for(jjxb=jjxbe;jjxb>=jjxbs;jjxb--){    
		  const int stxp = stride_xp[jjxb];
		  const int styp = stride_yp[jjyb];
		  const int stzp = stride_zp[jjzb];

		  // 内側インデックス
		  const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
		  const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);


		  const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
		  const int jce = j + stxp;
		  const int jcn = j + styp;
		  const int jct = j + stzp;

		  s[j] = s[j] - (
				 A[j + 4*block_m]*s[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
				 A[j + 5*block_m]*s[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
				 A[j + 6*block_m]*s[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
				 )*Dinv[j];
		  
		  sr_dot = sr_dot + s[j]*r[j];
		  rr_dot = rr_dot + r[j]*r[j];

		}
	      }
	    }
	  }
	}
      }

#if 0
   sr_dot_local[0] = sr_dot;
   rr_dot_local[0] = rr_dot;
   return 0;
#endif

#ifdef JUPITER_MPI
 sleev_comm_block3D(s,&prm,
		       block_nxs,  block_nxe, 
		       block_nys,  block_nye, 
		       block_nzs,  block_nze);

#endif

 calc_res_block3D(
		  prm,A,s,r,z,
		  stride_xp,stride_xm,
		  stride_yp,stride_ym,
		  stride_zp,stride_zm 
		  );

#if BLOCK_1DLOOOP
  // #pragma omp parallel for private(jb)
  for(jb=0;jb<ndivblock;jb++){{{
      const int jzb =  jb / (nxdivblock*nydivblock);
      const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
      const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
      // #pragma omp parallel for private(jxb,jyb,jzb)
      for(jzb=0;jzb<nzdivblock;jzb++){
	for(jyb=0;jyb<nydivblock;jyb++){
	  for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	    const int jjzbs = block_nzs[jzb];
	    const int jjzbe = block_nze[jzb];
	    const int jjybs = block_nys[jyb];
	    const int jjybe = block_nye[jyb];
	    const int jjxbs = block_nxs[jxb];
	    const int jjxbe = block_nxe[jxb];

	    for(jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
	      for(jjyb=jjybs;jjyb<=jjybe;jjyb++){    
		for(jjxb=jjxbs;jjxb<=jjxbe;jjxb++){
		  const int stxm = stride_xm[jjxb];
		  const int stym = stride_ym[jjyb];
		  const int stzm = stride_zm[jjzb];

		  // 内側インデックス
		  const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
		  const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	
		  const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
		  const int jcb = j + stzm;
		  const int jcs = j + stym;
		  const int jcw = j + stxm;

	      z[j] = ( z[j] - ( 
			     A[j + 2*block_m]*z[jcw] * block_xfilterL[ jjxb + jxb*nxblock ] +
			     A[j + 1*block_m]*z[jcs] * block_yfilterL[ jjyb + jyb*nyblock ] +
			     A[j + 0*block_m]*z[jcb] * block_zfilterL[ jjzb + jzb*nzblock ]  
			      ) ) * Dinv[j];
		}
	      }
	    }
	  }
	}
      }

#if BLOCK_1DLOOOP
  // #pragma omp parallel for private(jb)
  for(jb=0;jb<ndivblock;jb++){{{
      const int jzb =  jb / (nxdivblock*nydivblock);
      const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
      const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
      // #pragma omp parallel for private(jxb,jyb,jzb)
      for(jzb=0;jzb<nzdivblock;jzb++){
	for(jyb=0;jyb<nydivblock;jyb++){
	  for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	    const int jjzbs = block_nzs[jzb];
	    const int jjzbe = block_nze[jzb];
	    const int jjybs = block_nys[jyb];
	    const int jjybe = block_nye[jyb];
	    const int jjxbs = block_nxs[jxb];
	    const int jjxbe = block_nxe[jxb];

	    for(jjzb=jjzbe;jjzb>=jjzbs;jjzb--){    
	      for(jjyb=jjybe;jjyb>=jjybs;jjyb--){    
		for(jjxb=jjxbe;jjxb>=jjxbs;jjxb--){    
		  const int stxp = stride_xp[jjxb];
		  const int styp = stride_yp[jjyb];
		  const int stzp = stride_zp[jjzb];

		  // 内側インデックス
		  const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
		  const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);


		  const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;
		  const int jce = j + stxp;
		  const int jcn = j + styp;
		  const int jct = j + stzp;

	      z[j] = z[j] - (
			   A[j + 4*block_m]*z[jce] * block_xfilterU[ jjxb + jxb*nxblock ] +
			   A[j + 5*block_m]*z[jcn] * block_yfilterU[ jjyb + jyb*nyblock ] +
			   A[j + 6*block_m]*z[jct] * block_zfilterU[ jjzb + jzb*nzblock ]
			   )*Dinv[j];
	      s[j] = s[j] + z[j] ;
	      sr_dot = sr_dot + s[j]*r[j];
	      rr_dot = rr_dot + r[j]*r[j];

		}
	      }
	    }
	  }
	}
      }

   sr_dot_local[0] = sr_dot;
   rr_dot_local[0] = rr_dot;
   return 0;


#endif


}



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
					     ){

  int block_m = prm.block_m;
  type *z = (type*)malloc(sizeof(type)*( block_m ) );
  type *q = (type*)malloc(sizeof(type)*( block_m ) );
  zero_initialize(block_m,z);
  zero_initialize(block_m,q);
  type sr_dot_local = 0.0;
  type rr_dot_local = 0.0;

  solve_pre_subdividing_mat2_local_block3D(
				   prm, A, Dinv,
				   r, s, q,
				   z,				   
				   0.0,&sr_dot_local,&rr_dot_local,
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
  
  free(z);
  free(q);

  return 0;
}

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
					     ){

  int block_m = prm.block_m;
  type *z = (type*)malloc(sizeof(type)*( block_m ) );
  type *q = (type*)malloc(sizeof(type)*( block_m ) );
  zero_initialize(block_m,z);
  zero_initialize(block_m,q);
  type sr_dot_local = 0.0;
  type rr_dot_local = 0.0;

  solve_pre_subdividing_mat2_local_nxnynz_block3D(
				   prm, A, Dinv,
				   r, s, q,
				   z,				   
				   0.0,&sr_dot_local,&rr_dot_local,
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
  
  free(z);
  free(q);

  return 0;
}



 int make_sMat_block3D(
	       mpi_prm prm,type* A,
	       int *stride_xp,int *stride_xm,
	       int *stride_yp,int *stride_ym,
	       int *stride_zp,int *stride_zm 
	       ){

  int block_m = prm.block_m;
  int nzblock    = prm.nzblock;
  int nyblock    = prm.nyblock;
  int nxblock    = prm.nxblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;
  int ndivblock  = nxdivblock * nydivblock * nzdivblock; 

  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int jjxb,jjyb,jjzb;
  int jxb,jyb,jzb;
  int jb;


#pragma omp parallel for private(jjxb,jjyb,jjzb,jxb,jyb,jzb,jb) // collapse(3)
  for(jjzb=0;jjzb<nzblock;jjzb++){
    for(jjyb=0;jjyb<nyblock;jjyb++){
      for(jjxb=0;jjxb<nxblock;jjxb++){

	const int stxm = stride_xm[jjxb];
	const int stxp = stride_xp[jjxb];
	const int stym = stride_ym[jjyb];
	const int styp = stride_yp[jjyb];
	const int stzm = stride_zm[jjzb];
	const int stzp = stride_zp[jjzb];

	// 内側インデックス
 	const int jj0 = jjxb + jjyb*nxblock + jjzb*(nxblock*nyblock);
	const int j0  = 1 + mxdivblock + mydivblock * mxdivblock + jj0 * (mxdivblock * mydivblock * mzdivblock);	

#if BLOCK_1DLOOOP
	for(jb=0;jb<ndivblock;jb++){{{
	      const int jzb =  jb / (nxdivblock*nydivblock);
	      const int jyb = (jb%(nxdivblock*nydivblock))/nxdivblock;
	      const int jxb = (jb%(nxdivblock*nydivblock))%nxdivblock ;
#else
	for(jzb=0;jzb<nzdivblock;jzb++){
	  for(jyb=0;jyb<nydivblock;jyb++){
	    for(jxb=0;jxb<nxdivblock;jxb++){
#endif	     
	      const int j  = j0 + jxb + jyb * mxdivblock + jzb * mydivblock * mxdivblock;

	      const int jcb = j + stzm;
	      const int jcs = j + stym;
	      const int jcw = j + stxm;
	      const int jcc = j       ;
	      const int jce = j + stxp;
	      const int jcn = j + styp;
	      const int jct = j + stzp;

	      A[jce + 2 * block_m] = A[j+4 * block_m];
	      A[jcn + 1 * block_m] = A[j+5 * block_m];
	      A[jct + 0 * block_m] = A[j+6 * block_m];
	    } 
	  }
	}
      }
     }
   }

  return 0;
}
