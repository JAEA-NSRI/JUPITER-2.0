#include "csvutil.h"

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef GPU
  #include <cuda.h>
  #include <cutil.h>
#endif

#include "struct.h"
#include "func.h"

#ifdef _TIME_
extern type time_advection_eq;
#endif

//#define _3rdFD(fx,u,fw2,fw1,fc,fe1,fe2,dxs)       \
//{                                                 \
//  if (u < 0.0) {                                  \
//    fx = -(fe2 - 6.*fe1 + 3.*fc + 2.*fw1)*dxs/6.; \
//  } else {                                        \
//    fx =  (fw2 - 6.*fw1 + 3.*fc + 2.*fe1)*dxs/6.; \
//  }                                               \
//}

#define _3rdFD(fx,u,fw2,fw1,fc,fe1,fe2,dxs)       \
{                                                 \
  if (u < 0.0) {                                  \
    fx = (fe1 - fc)*dxs; \
  } else {                                        \
    fx = (fc - fw1)*dxs; \
  }                                               \
}

#define C1312 (1.083333333333333)
#define C16   (0.16666666666667)

type FD3_flux(type u, type fw2, type fw, type fc, type fe)
{
	type flux;

	if (0.0 <= u){
		flux = u*fw;
	}else{
		flux = u*fc;
	}

	return(flux);
}//*/

type weno_flux(type u, type fw3, type fw2, type fw, type fc, type fe, type fe2)
{
	type  v1,v2,v3,v4,v5, tmp1,tmp2,eps=1.0e-08,
        al1,al2,al3, is1,is2,is3, sal, om1,om2,om3, ps1,ps2,ps3;

  if( 0.0 < u ) {
    v1 = fw3;
    v2 = fw2;
    v3 = fw;
    v4 = fc;
    v5 = fe;
  } else {
    v1 = fe2;
    v2 = fe;
    v3 = fc;
    v4 = fw;
    v5 = fw2;
  }

  tmp1 = v1 - 2.0*v2 + v3; tmp2 = v1 - 4.0*v2 + 3.0*v3;
  is1 = C1312*tmp1*tmp1 + 0.25*tmp2*tmp2 + eps;
  tmp1 = v2 - 2.0*v3 + v4; tmp2 = v2 - v4;
  is2 = C1312*tmp1*tmp1 + 0.25*tmp2*tmp2 + eps;
  tmp1 = v3 - 2.0*v4 + v5; tmp2 = 3.0*v3 - 4.0*v4 + v5;
  is3 = C1312*tmp1*tmp1 + 0.25*tmp2*tmp2 + eps;
  al1 = 0.1/(is1*is1);
  al2 = 0.6/(is2*is2);
  al3 = 0.3/(is3*is3);
  sal = 1.0/(al1 + al2 + al3);
  om1 = al1*sal;
  om2 = al2*sal;
  om3 = al3*sal;
  ps1 = (2.0*v1 - 7.0*v2 + 11.0*v3)*C16;
  ps2 = (-v2 + 5.0*v3 + 2.0*v4)*C16;
  ps3 = (2.0*v3 + 5.0*v4 - v5)*C16;

  return (u*(om1*ps1 + om2*ps2 + om3*ps3));

/*  type  tmp1,tmp2,eps=1.0e-08,
        al1,al2,al3, is1,is2,is3, sal, om1,om2,om3, ps1,ps2,ps3;

	ps1 =   fw2*3.0/8.0 - fw*5.0/4.0 + fc*15.0/8.0;
	ps2 = - fw*1.0/8.0  + fc*3.0/4.0 + fe*3.0/8.0;
	ps3 =   fc*3.0/8.0  + fe*3.0/4.0 - fe2*1.0/8.0;

  tmp1 = fw2 - 2.0*fw + fc; tmp2 = fw2 - 4.0*fw + 3.0*fc;
  is1 = C1312*tmp1*tmp1 + 0.25*tmp2*tmp2 + eps;
  tmp1 = fw - 2.0*fc + fe; tmp2 = fw - fe;
  is2 = C1312*tmp1*tmp1 + 0.25*tmp2*tmp2 + eps;
  tmp1 = fc - 2.0*fe + fe2; tmp2 = 3.0*fc - 4.0*fe + fe2;
  is3 = C1312*tmp1*tmp1 + 0.25*tmp2*tmp2 + eps;
  al1 = 1.0/16.0/(is1*is1);
  al2 = 5.0/8.0/(is2*is2);
  al3 = 5.0/16.0/(is3*is3);
  sal = 1.0/(al1 + al2 + al3);
  om1 = al1*sal;
  om2 = al2*sal;
  om3 = al3*sal;

  return (om1*ps1 + om2*ps2 + om3*ps3);*/

}//*/


type weno(type u, type fw3, type fw2, type fw, type fc, type fe, type fe2, type fe3, type dxi)
{
  type v1,v2,v3,v4,v5, tmp1,tmp2,eps=1.0e-08,
       al1,al2,al3, is1,is2,is3, sal, om1,om2,om3, ps1,ps2,ps3;
  if(u > 0.0) {
    v1 = (fw2 - fw3)*dxi;
    v2 = (fw  - fw2)*dxi;
    v3 = (fc  - fw )*dxi;
    v4 = (fe  - fc )*dxi;
    v5 = (fe2 - fe )*dxi;
  } else {
    v1 = (fe3 - fe2)*dxi;
    v2 = (fe2 - fe )*dxi;
    v3 = (fe  - fc )*dxi;
    v4 = (fc  - fw )*dxi;
    v5 = (fw  - fw2)*dxi;
  }
  tmp1 = v1 - 2.0*v2 + v3; tmp2 = v1 - 4.0*v2 + 3.0*v3;
  is1 = C1312*tmp1*tmp1 + 0.25*tmp2*tmp2 + eps;
  tmp1 = v2 - 2.0*v3 + v4; tmp2 = v2 - v4;
  is2 = C1312*tmp1*tmp1 + 0.25*tmp2*tmp2 + eps;
  tmp1 = v3 - 2.0*v4 + v5; tmp2 = 3.0*v3 - 4.0*v4 + v5;
  is3 = C1312*tmp1*tmp1 + 0.25*tmp2*tmp2 + eps;
  al1 = 0.1/(is1*is1);
  al2 = 0.6/(is2*is2);
  al3 = 0.3/(is3*is3);
  sal = 1.0/(al1 + al2 + al3);
  om1 = al1*sal;
  om2 = al2*sal;
  om3 = al3*sal;
  ps1 = (2.0*v1 - 7.0*v2 + 11.0*v3)*C16;
  ps2 = (-v2 + 5.0*v3 + 2.0*v4)*C16;
  ps3 = (2.0*v3 + 5.0*v4 - v5)*C16;

  return (om1*ps1 + om2*ps2 + om3*ps3);
}
#undef C1312
#undef C16

type advection_flux(int flag, type *ft, type *f, type *u, type *v, type *w, type *fs, parameter *prm)
{
#ifdef _TIME_
  type time0=cpu_time();
#endif
  flags  *flg=prm->flg;
  domain *cdo=prm->cdo;
  int   j,jx,jy,jz,
        nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
        mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  int   jxe, jye, jze, jxc, jyc, jzc;
  type  dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi,
        fx, fy, fz;
	type fsi, fsc,fsw,fse,fss,fsn,fsb,fst, aw,ae,as,an,ab,at, qs;

  // memory allocation for work array.
  static int  n1st = 0;
  static type *flx, *fly, *flz, *uc, *vc, *wc;
  size_t size = sizeof(type)*(cdo->m);

  if (n1st++ == 0) {
		uc = (type *) malloc( size );
    vc = (type *) malloc( size );
    wc = (type *) malloc( size );
    flx = (type *) malloc( size );
    fly = (type *) malloc( size );
    flz = (type *) malloc( size );
  }

	jxc = nx; jyc = ny; jzc = nz;
	jxe = nx+1; jye = ny+1; jze = nz+1;
	if(flag == 2)  jxc += 1; jxe += 1;
  if(flag == 3)  jyc += 1; jye += 1;
  if(flag == 4)  jzc += 1; jze += 1;

	if(flag == 1) {        // cell-center
#pragma omp parallel for private(jz,jy,jx,j)
		for(jz = 0; jz < jze; jz++) {
			for(jy = 0; jy < jye; jy++) {
				for(jx = 0; jx < jxe; jx++) {
					j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
					uc[j] = u[j];
					vc[j] = v[j];
					wc[j] = w[j];//*/

/*					if (fs && flg->IBM == ON) {
						if(0.5 < fs[j] || 0.5 < fs[j - 1]) uc[j] = 0.0;
						if(0.5 < fs[j] || 0.5 < fs[j -mx]) vc[j] = 0.0;
						if(0.5 < fs[j] || 0.5 < fs[j-mxy]) wc[j] = 0.0;
					} else {
						uc[j] = u[j];
						vc[j] = v[j];
						wc[j] = w[j];
//							printf("uvw = %f %f %f \n",uc[j],vc[j],wc[j]);
//							if(uc[j] !=0) printf("uvw = %e %e %e \n",uc[j],vc[j],wc[j]);
					}*/
				}
			}
		}
	} else if(flag == 2 || flag == 3 || flag == 4) {
#pragma omp parallel for private(jz,jy,jx,j)
		for(jz = 0; jz < jze; jz++) {
			for(jy = 0; jy < jye; jy++) {
				for(jx = 0; jx < jxe; jx++) {
					j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
						uc[j] = 0.5*(u[j] + u[j-1]);
						vc[j] = 0.5*(v[j] + v[j-mx]);
						wc[j] = 0.5*(w[j] + w[j-mxy]);
				}
			}
		}
	}

//	if(flg->WENO == ON) {
	if(0) {
#pragma omp parallel for private(jz,jy,jx,j)
		for(jz = 0; jz < jze; jz++) {
			for(jy = 0; jy < jye; jy++) {
				for(jx = 0; jx < jxe; jx++) {
					j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
					flx[j] = weno_flux(uc[j], f[j-3],     f[j-2],     f[j-1],   f[j], f[j+1],   f[j+2]    );
					fly[j] = weno_flux(vc[j], f[j-3*mx],  f[j-2*mx],  f[j-mx],  f[j], f[j+mx],  f[j+2*mx] );
					flz[j] = weno_flux(wc[j], f[j-3*mxy], f[j-2*mxy], f[j-mxy], f[j], f[j+mxy], f[j+2*mxy]);
//					if(flz[j] == 0) printf("flux = %e\n",flz[j]);
				}
			}
		}
	}else{
#pragma omp parallel for private(jz,jy,jx,j)
		for(jz = 0; jz < jze; jz++) {
			for(jy = 0; jy < jye; jy++) {
				for(jx = 0; jx < jxe; jx++) {
					j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
					flx[j] = FD3_flux(uc[j], f[j-2],     f[j-1],   f[j], f[j+1]  );
					fly[j] = FD3_flux(vc[j], f[j-2*mx],  f[j-mx],  f[j], f[j+mx] );
					flz[j] = FD3_flux(wc[j], f[j-2*mxy], f[j-mxy], f[j], f[j+mxy]);
//					printf("uvw = %f %f %f \n",uc[j],vc[j],wc[j]);
//					if(uc[j] !=0) printf("uvw = %e %e %e \n",uc[j],vc[j],wc[j]);
				}
			}
		}
	}
	
	
#pragma omp parallel for private(jz,jy,jx,j,fx,fy,fz,fsi,fsc,fsw,fse,fss,fsn,fsb,fst,aw,ae,as,an,ab,at)
  for(jz = 0; jz < jxc; jz++) {
    for(jy = 0; jy < jyc; jy++) {
      for(jx = 0; jx < jzc; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
				
				ft[j] = 0.0;

				if(clip(1.0 - fs[j]) == 0.0) continue;
				fsi = 1.0 / clip(1.0 - fs[j]);

				// clip vof
				fsc = clip(1.0 - fs[j]);
				fsw = clip(1.0 - fs[j-1]);
				fse = clip(1.0 - fs[j+1]);
				fss = clip(1.0 - fs[j-mx]);
				fsn = clip(1.0 - fs[j+mx]);
				fsb = clip(1.0 - fs[j-mxy]);
				fst = clip(1.0 - fs[j+mxy]);
				//--- calc solid ratio at cell face ---
				aw = MIN2(fsc, fsw);
				ae = MIN2(fsc, fse);
				as = MIN2(fsc, fss);
				an = MIN2(fsc, fsn);
				ab = MIN2(fsc, fsb);
				at = MIN2(fsc, fst);

				fx = (flx[j+1]*ae   - flx[j]*aw)*dxi;
				fy = (fly[j+mx]*an  - fly[j]*as)*dyi;
				fz = (flz[j+mxy]*at - flz[j]*ab)*dzi;
				ft[j] = - (fx + fy + fz)*fsi;
			}
		}
	}


#ifdef _TIME_
  time_advection_eq += cpu_time()-time0;
#endif
  return 0.0;
}//*/

type advection_eq(int flag, type *ft, type *f, type *u, type *v, type *w,
                  type *eps, type *fs_ibm,
                  struct surface_boundary_data **surf_bnd_array,
                  parameter *prm)
{
#ifdef _TIME_
  type time0=cpu_time();
#endif
  flags  *flg=prm->flg;
  domain *cdo=prm->cdo;
  int  j,jx,jy,jz,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  int jxe=nx, jye=ny, jze=nz;
  type dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi,
       fx, fy, fz, uc=0.0, vc=0.0, wc=0.0;

  if(flag == 2) { jxe = nx+1;  jye = ny;   jze = nz;  }
  if(flag == 3) { jxe = nx;    jye = ny+1; jze = nz;  }
  if(flag == 4) { jxe = nx;    jye = ny;   jze = nz+1;}

  static type *ffs = NULL;

  if (eps && flag != 1) {
    if (!ffs) {
      size_t size = sizeof(type) * cdo->m;
      ffs = (type *)malloc(size);
      if (!ffs) {
        prm->status = ON;
        csvperror(__FILE__, __LINE__, 0, CSV_EL_FATAL, NULL, CSV_ERR_NOMEM, 0,
                  0, NULL);
        return 0.0;
      }
    }

#pragma omp parallel for private(jz, jy, jx, j)
    for (jz = 0; jz < jze; jz++) {
      for (jy = 0; jy < jye; jy++) {
        for (jx = 0; jx < jxe; jx++) {
          type fs_c, epsj, epsjn;
          j = (jx + stm) + mx * (jy + stm) + mxy * (jz + stm);
          epsj = eps[j];
          epsjn = 0.0;
          if (flag == 2) { // cell-face x
            epsjn = eps[j - 1];
          } else if (flag == 3) { // cell-face y
            epsjn = eps[j - mx];
          } else if (flag == 4) {
            epsjn = eps[j - mxy];
          }

          if (epsj <= 0.0) {
            epsj = epsjn;
          } else if (epsjn <= 0.0) {
            epsjn = epsj;
          }
          fs_c = MIN2(epsj, epsjn);
          if (fs_c > 0.0) {
            ffs[j] = f[j] / fs_c;
          } else {
            ffs[j] = f[j];
          }
        }
      }
    }
    bcf(ffs, prm);
    f = ffs;
  }

#pragma omp parallel for private(jz,jy,jx,j,uc,vc,wc,fx,fy,fz)
  for(jz = 0; jz < jze; jz++) {
    for(jy = 0; jy < jye; jy++) {
      for(jx = 0; jx < jxe; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        if(flag == 1) {        // cell-center
          if (fs_ibm && flg->IBM == ON && fs_ibm[j] > 0.5) {
            uc = 0.0;
            vc = 0.0;
            wc = 0.0;
          } else if (eps && flg->porous == ON) {
            /* IBM solids must be excluded here */
            CSVASSERT(eps[j] > 0.0);
            uc = 0.5*(u[j] + u[j+1])/eps[j];
            vc = 0.5*(v[j] + v[j+mx])/eps[j];
            wc = 0.5*(w[j] + w[j+mxy])/eps[j];
          } else {
            uc = 0.5*(u[j] + u[j+1]);
            vc = 0.5*(v[j] + v[j+mx]);
            wc = 0.5*(w[j] + w[j+mxy]);
          }
        } else if(flag == 2) { // cell-face x
          uc = u[j];
          vc = 0.25*(v[j] + v[j-1] + v[j+mx] + v[j-1+mx]);
          wc = 0.25*(w[j] + w[j-1] + w[j+mxy] + w[j-1+mxy]);
        } else if(flag == 3) { // cell-face y
          uc = 0.25*(u[j] + u[j+1] + u[j-mx] + u[j+1-mx]);
          vc = v[j];
          wc = 0.25*(w[j] + w[j-mx] + w[j+mxy] + w[j-mx+mxy]);
        } else if(flag == 4) {
          uc = 0.25*(u[j] + u[j+1] + u[j-mxy] + u[j+1-mxy]);
          vc = 0.25*(v[j] + v[j+mx] + v[j-mxy] + v[j+mx-mxy]);
          wc = w[j];
        }

        if(flg->WENO == ON) {
          int use_weno_x = 1;
          int use_weno_y = 1;
          int use_weno_z = 1;

          if (surf_bnd_array) {
            for (int k = -3; k <= 3; ++k) {
              int jxk = jx + k;
              if (jxk < 0 || jxk > nx) {
                use_weno_x = 0;
                break;
              }
              struct surface_boundary_data *sbx =
                surf_bnd_array[3 * (j + k)];
              if (sbx && sbx->cond == INLET) {
                use_weno_x = 0;
                break;
              }
            }
            for (int k = -3; k <= 3; ++k) {
              int jyk = jy + k;
              if (jyk < 0 || jyk > ny) {
                use_weno_y = 0;
                break;
              }
              struct surface_boundary_data *sby =
                surf_bnd_array[3 * (j + k * mx) + 1];
              if (sby && sby->cond == INLET) {
                use_weno_y = 0;
                break;
              }
            }
            for (int k = -3; k <= 3; ++k) {
              int jzk = jz + k;
              if (jzk < 0 || jzk > nz) {
                use_weno_z = 0;
                break;
              }
              struct surface_boundary_data *sbz =
                surf_bnd_array[3 * (j + k * mxy) + 2];
              if (sbz && sbz->cond == INLET) {
                use_weno_z = 0;
                break;
              }
            }
          }

          if (use_weno_x) {
            fx = weno(uc, f[j-3],    f[j-2],    f[j-1],  f[j],f[j+1],  f[j+2],    f[j+3],     dxi);
          } else {
            _3rdFD(fx, uc, f[j-2],f[j-1],f[j],f[j+1],f[j+2], dxi);
          }
          if (use_weno_y) {
            fy = weno(vc, f[j-3*mx], f[j-2*mx], f[j-mx], f[j],f[j+mx], f[j+2*mx], f[j+3*mx],  dyi);
          } else {
            _3rdFD(fy, vc, f[j-2*mx],f[j-mx],f[j],f[j+mx],f[j+2*mx], dyi);
          }
          if (use_weno_z) {
            fz = weno(wc, f[j-3*mxy],f[j-2*mxy],f[j-mxy],f[j],f[j+mxy],f[j+2*mxy],f[j+3*mxy], dzi);
          } else {
            _3rdFD(fz, wc, f[j-2*mxy],f[j-mxy],f[j],f[j+mxy],f[j+2*mxy], dzi);
          }
        } else {
          _3rdFD(fx, uc, f[j-2],f[j-1],f[j],f[j+1],f[j+2], dxi);
          _3rdFD(fy, vc, f[j-2*mx],f[j-mx],f[j],f[j+mx],f[j+2*mx], dyi);
          _3rdFD(fz, wc, f[j-2*mxy],f[j-mxy],f[j],f[j+mxy],f[j+2*mxy], dzi);
        }
        ft[j] = - uc*fx - vc*fy - wc*fz;
      }
    }
  }
#ifdef _TIME_
  time_advection_eq += cpu_time()-time0;
#endif
  return 0.0;
}

/* Advection scheme for porous medium */
type advection_eq_po(int flag, type *ft, type *f, type *u, type *v, type *w, type *fs, parameter *prm)
{
#ifdef _TIME_
  type time0=cpu_time();
#endif
  flags  *flg=prm->flg;
  domain *cdo=prm->cdo;
  int  j,jx,jy,jz,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       mx=cdo->mx, my=cdo->my, mxy=mx*my, stm=cdo->stm;
  int jxe=nx, jye=ny, jze=nz;
  type dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi,
       fx, fy, fz, uc=0.0, vc=0.0, wc=0.0;

  type fs_c;

  if(flag == 2) { jxe = nx+1;  jye = ny;   jze = nz;  }
  if(flag == 3) { jxe = nx;    jye = ny+1; jze = nz;  }
  if(flag == 4) { jxe = nx;    jye = ny;   jze = nz+1;}

  /* Yamashita, 2020/6/9 */
  // memory allocation for work array.
  static int  n1st = 0;
  static type *ffs;
  size_t size = sizeof(type)*(cdo->m);
  if (n1st++ == 0) {
    ffs = (type *) malloc( size );
  }
#pragma omp parallel for private(jz,jy,jx,j,fs_c)
  for(jz = 0; jz < jze; jz++) {
    for(jy = 0; jy < jye; jy++) {
      for(jx = 0; jx < jxe; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
        if(flag == 2) { // cell-face x
          fs_c   = MIN2(fs[j], fs[j-1]);
          ffs[j] = f[j]/fs_c;
        } else if(flag == 3) { // cell-face y
          fs_c   = MIN2(fs[j], fs[j-mx]);
          ffs[j] = f[j]/fs_c;
        } else if(flag == 4) {
          fs_c   = MIN2(fs[j], fs[j-mxy]);
          ffs[j] = f[j]/fs_c;
        }
      }
    }
  }
  bcf(ffs, prm);

#pragma omp parallel for private(jz,jy,jx,j,uc,vc,wc,fx,fy,fz)
  for(jz = 0; jz < jze; jz++) {
    for(jy = 0; jy < jye; jy++) {
      for(jx = 0; jx < jxe; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);

        if(flag == 1) {        // cell-center
          if (fs && flg->IBM == ON && fs[j] > 0.5) {
            uc = 0.0;
            vc = 0.0;
            wc = 0.0;
          } else {
            uc = 0.5*(u[j] + u[j+1]);
            vc = 0.5*(v[j] + v[j+mx]);
            wc = 0.5*(w[j] + w[j+mxy]);
          }
        } else if(flag == 2) { // cell-face x
          uc = u[j];
          vc = 0.25*(v[j] + v[j-1] + v[j+mx] + v[j-1+mx]);
          wc = 0.25*(w[j] + w[j-1] + w[j+mxy] + w[j-1+mxy]);
        } else if(flag == 3) { // cell-face y
          uc = 0.25*(u[j] + u[j+1] + u[j-mx] + u[j+1-mx]);
          vc = v[j];
          wc = 0.25*(w[j] + w[j-mx] + w[j+mxy] + w[j-mx+mxy]);
        } else if(flag == 4) {
          uc = 0.25*(u[j] + u[j+1] + u[j-mxy] + u[j+1-mxy]);
          vc = 0.25*(v[j] + v[j+mx] + v[j-mxy] + v[j+mx-mxy]);
          wc = w[j];
        }

        if(flg->WENO == ON) {
          fx = weno(uc, ffs[j-3],    ffs[j-2],    ffs[j-1],  ffs[j],ffs[j+1],  ffs[j+2],    ffs[j+3],     dxi);
          fy = weno(vc, ffs[j-3*mx], ffs[j-2*mx], ffs[j-mx], ffs[j],ffs[j+mx], ffs[j+2*mx], ffs[j+3*mx],  dyi);
          fz = weno(wc, ffs[j-3*mxy],ffs[j-2*mxy],ffs[j-mxy],ffs[j],ffs[j+mxy],ffs[j+2*mxy],ffs[j+3*mxy], dzi);
        } else {
          _3rdFD(fx, uc, ffs[j-2],ffs[j-1],ffs[j],ffs[j+1],ffs[j+2], dxi);
          _3rdFD(fy, vc, ffs[j-2*mx],ffs[j-mx],ffs[j],ffs[j+mx],ffs[j+2*mx], dyi);
          _3rdFD(fz, wc, ffs[j-2*mxy],ffs[j-mxy],ffs[j],ffs[j+mxy],ffs[j+2*mxy], dzi);
        }
        ft[j] = - uc*fx - vc*fy - wc*fz;
      }
    }
  }
#ifdef _TIME_
  time_advection_eq += cpu_time()-time0;
#endif
  return 0.0;
}
