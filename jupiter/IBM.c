#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef GPU
#  include <cuda.h>
#  include <cutil.h>
#endif

#include "struct.h"
#include "func.h"


void IBM_ixiyiz(int  *ix, int  *iy, int  *iz, type *xx, type *yy, type *zz,
                int jx, int jy, int jz, int stm,
                int nx, int ny, int nz, type nv_x, type nv_y, type nv_z, type ls, type dxi, type dyi, type dzi)
{ 
  // ix
  if(nv_x >= 0.0) *ix = jx - (int)(2.0*ls*nv_x*dxi) - 1;
  else            *ix = jx - (int)(2.0*ls*nv_x*dxi) + 0;
  // iy
  if(nv_y >= 0.0) *iy = jy - (int)(2.0*ls*nv_y*dyi) - 1;
  else            *iy = jy - (int)(2.0*ls*nv_y*dyi) + 0;
  // iz
  if(nv_z >= 0.0) *iz = jz - (int)(2.0*ls*nv_z*dzi) - 1;
  else            *iz = jz - (int)(2.0*ls*nv_z*dzi) + 0;
  // xx, yy
  //  *xx = fabs( 2.0*ls*nv_x*dxi - abs(jx - *ix) );
  //  *yy = fabs( 2.0*ls*nv_y*dyi - abs(jy - *iy) );
  //  *zz = fabs( 2.0*ls*nv_z*dzi - abs(jz - *iz) );
  if(nv_x >= 0.0) *xx = abs(jx - *ix) - fabs( 2.0*ls*nv_x*dxi );
  else            *xx = fabs( 2.0*ls*nv_x*dxi ) - abs(jx - *ix);
  if(nv_y >= 0.0) *yy = abs(jy - *iy) - fabs( 2.0*ls*nv_y*dyi );
  else            *yy = fabs( 2.0*ls*nv_y*dyi ) - abs(jy - *iy);
  if(nv_z >= 0.0) *zz = abs(jz - *iz) - fabs( 2.0*ls*nv_z*dzi );
  else            *zz = fabs( 2.0*ls*nv_z*dzi ) - abs(jz - *iz);

  // clip
  *xx = MAX2(0.0, *xx);
  *yy = MAX2(0.0, *yy);
  *zz = MAX2(0.0, *zz);
  *xx = MIN2(1.0, *xx);
  *yy = MIN2(1.0, *yy);
  *zz = MIN2(1.0, *zz);
  // boundary
  *ix = MAX2(stm, *ix);
  *iy = MAX2(stm, *iy);
  *iz = MAX2(stm, *iz);
  *ix = MIN2(nx+stm-1, *ix);
  *iy = MIN2(ny+stm-1, *iy);
  *iz = MIN2(nz+stm-1, *iz);
}

type IBM_ext_vel(type x, type y, type z,
                 type fc, type fe,  type fn,  type fne,
                 type ft, type fte, type ftn, type ftne)
{
  return (1.0 - z)*( (1.0 - y)*((1.0 - x)*fc  + x*fe  )
                          + y *((1.0 - x)*fn  + x*fne ) )
               + z*( (1.0 - y)*((1.0 - x)*ft  + x*fte )
                          + y *((1.0 - x)*ftn + x*ftne) );
}

type clip(type f);
void IBM_vector(type *lss, type *fs, type *u, type *v, type *w, type *nvsx,
                type *nvsy, type *nvsz,
                struct surface_boundary_data **surf_bnd_array,
                type *bnd_norm_u, type *bnd_norm_v, type *bnd_norm_w,
                domain *cdo)
{
  int  j,jx,jy,jz, i,ix,iy,iz,
       nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
       mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my, stm=cdo->stm;
  type nv_x,nv_y,nv_z, xx,yy,zz, 
       dx=cdo->dx, dy=cdo->dy, dz=cdo->dz,
       dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi, lss_c,
       //Y SolWall=0.05;
       SolWall=1.0-1.0e-8; //Sato

//############### Sato
  type f_c, f_w, f_s, f_b, aw, as, ab;

  // u
/* dependency in u */
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx+1; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
//Y 20200410        if(fs[j-1] <     SolWall && fs[j] > 1.0-SolWall) { u[j] = 0.0; continue; }
//Y 20200410        if(fs[j-1] > 1.0-SolWall && fs[j] <     SolWall) { u[j] = 0.0; continue; }

        lss_c = 0.5*(lss[j] + lss[j-1]);
        //if(lss_c >= dx) {
				if(SolWall < fs[j-1] || SolWall < fs[j]){
          nv_x = 0.5*(nvsx[j] + nvsx[j-1]);
          nv_y = 0.5*(nvsy[j] + nvsy[j-1]);
          nv_z = 0.5*(nvsz[j] + nvsz[j-1]);

          IBM_ixiyiz(&ix,&iy,&iz,&xx,&yy,&zz, jx,jy,jz,stm,nx,ny,nz, nv_x,nv_y,nv_z, lss_c, dxi,dyi,dzi);

          i = (ix+stm) + mx*(iy+stm) + mxy*(iz+stm);
          u[j] = - IBM_ext_vel(xx,yy,zz, u[i],u[i+1],u[i+mx],u[i+1+mx],
                               u[i+mxy],u[i+1+mxy],u[i+mx+mxy],u[i+1+mx+mxy]);
          //Y u[j] = - 0.125*(u[i]+u[i+1]+u[i+mx]+u[i+1+mx]+u[i+mxy]+u[i+1+mxy]+u[i+mx+mxy]+u[i+1+mx+mxy]);
        }
      }
    }
  }

  // v
/* dependency in v */
  for(jz = 0; jz < nz; jz++) {
    for(jy = 0; jy < ny+1; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
//Y 20200410       if(fs[j-mx] <     SolWall && fs[j] > 1.0-SolWall) { v[j] = 0.0; continue; }
//Y 20200410       if(fs[j-mx] > 1.0-SolWall && fs[j] <     SolWall) { v[j] = 0.0; continue; }

        lss_c = 0.5*(lss[j] + lss[j-mx]);
        //if(lss_c >= dy) {
				if(SolWall < fs[j-mx] || SolWall < fs[j]){
          nv_x = 0.5*(nvsx[j] + nvsx[j-mx]);
          nv_y = 0.5*(nvsy[j] + nvsy[j-mx]);
          nv_z = 0.5*(nvsz[j] + nvsz[j-mx]);

          IBM_ixiyiz(&ix,&iy,&iz,&xx,&yy,&zz, jx,jy,jz,stm,nx,ny,nz, nv_x,nv_y,nv_z, lss_c, dxi,dyi,dzi);

          i = (ix+stm) + mx*(iy+stm) + mxy*(iz+stm);
          v[j] = - IBM_ext_vel(xx,yy,zz, v[i],v[i+1],v[i+mx],v[i+1+mx],
                               v[i+mxy],v[i+1+mxy],v[i+mx+mxy],v[i+1+mx+mxy]);
          //Y v[j] = - 0.125*(v[i]+v[i+1]+v[i+mx]+v[i+1+mx]+v[i+mxy]+v[i+1+mxy]+v[i+mx+mxy]+v[i+1+mx+mxy]);
        }
      }
    }
  }

  // w
/* dependency in w */
  for(jz = 0; jz < nz+1; jz++) {
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        j = (jx+stm) + mx*(jy+stm) + mxy*(jz+stm);
//Y 20200410        if(fs[j-mxy] <     SolWall && fs[j] > 1.0-SolWall) { w[j] = 0.0; continue; }
//Y 20200410        if(fs[j-mxy] > 1.0-SolWall && fs[j] <     SolWall) { w[j] = 0.0; continue; }

        lss_c = 0.5*(lss[j] + lss[j-mxy]);
        //if(lss_c >= dz) {
				if(SolWall < fs[j-mxy] || SolWall < fs[j]){
          nv_x = 0.5*(nvsx[j] + nvsx[j-mxy]);
          nv_y = 0.5*(nvsy[j] + nvsy[j-mxy]);
          nv_z = 0.5*(nvsz[j] + nvsz[j-mxy]);

          IBM_ixiyiz(&ix,&iy,&iz,&xx,&yy,&zz, jx,jy,jz,stm,nx,ny,nz, nv_x,nv_y,nv_z, lss_c, dxi,dyi,dzi);
          i = (ix+stm) + mx*(iy+stm) + mxy*(iz+stm);
          w[j] = - IBM_ext_vel(xx,yy,zz, w[i],w[i+1],w[i+mx],w[i+1+mx],
                               w[i+mxy],w[i+1+mxy],w[i+mx+mxy],w[i+1+mx+mxy]);
          //Y w[j] = - 0.125*(w[i]+w[i+1]+w[i+mx]+w[i+1+mx]+w[i+mxy]+w[i+1+mxy]+w[i+mx+mxy]+w[i+1+mx+mxy]);
        }
      }
    }
  }

  // Wall boundary velocity 0
  for (jz = 0; jz < nz + 1; jz++) {
    for (jy = 0; jy < ny + 1; jy++) {
      for (jx = 0; jx < nx + 1; jx++) {
        j = (jx + stm) + mx * (jy + stm) + mxy * (jz + stm);

        // clip vof
        f_c = clip(fs[j]);
        f_w = clip(fs[j - 1]);
        f_s = clip(fs[j - mx]);
        f_b = clip(fs[j - mxy]);
        //--- calc solid ratio at cell face ---
        // aw = MAX2(f_c, f_w);
        // as = MAX2(f_c, f_s);
        // ab = MAX2(f_c, f_b);

        //--- flag to identify solid interface
        int iw = (f_c>=0.5 && f_w<0.5) || (f_c<0.5 && f_w>=0.5);
        int is = (f_c>=0.5 && f_s<0.5) || (f_c<0.5 && f_s>=0.5);
        int ib = (f_c>=0.5 && f_b<0.5) || (f_c<0.5 && f_b>=0.5);

        //if (SolWall < aw) {
        if (iw==1) {
          if (surf_bnd_array && surf_bnd_array[3 * j] &&
              surf_bnd_array[3 * j]->cond == INLET) {
            u[j] = calc_surface_boundary_vel(u[j], surf_bnd_array[3 * j],
                                             bnd_norm_u[3 * j]);

          } else {
            u[j] = 0.0;
          }
        }
        if (is==1) {
          if (surf_bnd_array && surf_bnd_array[3 * j + 1] &&
              surf_bnd_array[3 * j + 1]->cond == INLET) {
            v[j] = calc_surface_boundary_vel(v[j], surf_bnd_array[3 * j + 1],
                                             bnd_norm_v[3 * j + 1]);
          } else {
            v[j] = 0.0;
          }
        }
        if (ib==1) {
          if (surf_bnd_array && surf_bnd_array[3 * j + 2] &&
              surf_bnd_array[3 * j + 2]->cond == INLET) {
            w[j] = calc_surface_boundary_vel(w[j], surf_bnd_array[3 * j + 2],
                                             bnd_norm_w[3 * j + 2]);
          } else {
            w[j] = 0.0;
          }
        }
      }
    }
  }//

}

type IBM_solid_ratio(type fw, type fe)
{
  type sr;
  if(fw*fe < 0.0) {
    if(fw > 0.0) sr = fw/(fabs(fw) + fabs(fe));
    else         sr = fe/(fabs(fw) + fabs(fe));
  } else {
    if(fw > 0.0) sr = 1.0;
    else         sr = 0.0;
  }
  return sr;
}


type clip(type f);
type IBM_divergence_vof_qs(type *u, type *v, type *w, type *f,
                           struct surface_boundary_data **surf_bnd_array,
                           type *bnd_norm_u, type *bnd_norm_v,
                           type *bnd_norm_w,
                           int j, domain *cdo)
{
  int  mx=cdo->mx,my=cdo->my,mxy=mx*my,stm=cdo->stm;
  type dxi=cdo->dxi, dyi=cdo->dyi, dzi=cdo->dzi;
  type fc,fw,fe,fs,fn,fb,ft, aw,ae,as,an,ab,at, qs;
  type um, up, vm, vp, wm, wp;

  // clip vof
  fc = clip(f[j]);
  fw = clip(f[j-1]);
  fe = clip(f[j+1]);
  fs = clip(f[j-mx]);
  fn = clip(f[j+mx]);
  fb = clip(f[j-mxy]);
  ft = clip(f[j+mxy]);
  //--- calc solid ratio at cell face ---
  aw = MAX2(fc, fw);
  ae = MAX2(fc, fe);
  as = MAX2(fc, fs);
  an = MAX2(fc, fn);
  ab = MAX2(fc, fb);
  at = MAX2(fc, ft);

  if (surf_bnd_array) {
    struct surface_boundary_data *sbw = surf_bnd_array[3 * j];
    struct surface_boundary_data *sbe = surf_bnd_array[3 * (j + 1)];
    struct surface_boundary_data *sbs = surf_bnd_array[3 * j + 1];
    struct surface_boundary_data *sbn = surf_bnd_array[3 * (j + mx) + 1];
    struct surface_boundary_data *sbb = surf_bnd_array[3 * j + 2];
    struct surface_boundary_data *sbt = surf_bnd_array[3 * (j + mxy) + 2];

    if (sbw && sbw->cond == INLET) aw = 0.0;
    if (sbe && sbe->cond == INLET) ae = 0.0;
    if (sbs && sbs->cond == INLET) as = 0.0;
    if (sbn && sbn->cond == INLET) an = 0.0;
    if (sbb && sbb->cond == INLET) ab = 0.0;
    if (sbt && sbt->cond == INLET) at = 0.0;
  }

  um = u[j];
  up = u[j + 1];
  vm = v[j];
  vp = v[j + mx];
  wm = w[j];
  wp = w[j + mxy];

  if (surf_bnd_array && bnd_norm_u && bnd_norm_v && bnd_norm_w) {
    struct surface_boundary_data *sum, *sup, *svm, *svp, *swm, *swp;
    type snxm, snxp, snym, snyp, snzm, snzp;
    int jxm, jxp, jym, jyp, jzm, jzp;

    jxm = 3 * j;
    jxp = 3 * (j + 1);
    jym = 3 * j + 1;
    jyp = 3 * (j + mx) + 1;
    jzm = 3 * j + 2;
    jzp = 3 * (j + mxy) + 2;

    sum = surf_bnd_array[jxm];
    sup = surf_bnd_array[jxp];
    svm = surf_bnd_array[jym];
    svp = surf_bnd_array[jyp];
    swm = surf_bnd_array[jzm];
    swp = surf_bnd_array[jzp];
    snxm = bnd_norm_u[jxm];
    snxp = bnd_norm_u[jxp];
    snym = bnd_norm_v[jym];
    snyp = bnd_norm_v[jyp];
    snzm = bnd_norm_w[jzm];
    snzp = bnd_norm_w[jzp];

    if (sum)
      um = calc_surface_boundary_vel(um, sum, snxm);
    if (sup)
      up = calc_surface_boundary_vel(up, sup, snxp);
    if (svm)
      vm = calc_surface_boundary_vel(vm, svm, snym);
    if (svp)
      vp = calc_surface_boundary_vel(vp, svp, snyp);
    if (swm)
      wm = calc_surface_boundary_vel(wm, swm, snzm);
    if (swp)
      wp = calc_surface_boundary_vel(wp, swp, snzp);
  }

  //--- mass source ---
  qs = ( ae*up - aw*um )*dxi
     + ( an*vp - as*vm )*dyi
     + ( at*wp - ab*wm )*dzi;

  return qs;
}

int IBM_flux_corr_vof_direction_split(type *flm, type *flp, int j, type *f, domain *cdo, int direction_flag)
{
  int  mx=cdo->mx, mxy=cdo->mxy;
  type fc, fm,fp,am,ap;

  fc = clip(f[j]);

  if(direction_flag==0){// ------------------------- x direction -------------------------
      fm = clip(f[j-1]);
      fp = clip(f[j+1]);
  }else if(direction_flag==1){ // ------------------------- y direction -------------------------
      fm = clip(f[j-mx]);
      fp = clip(f[j+mx]);
  }else if(direction_flag==2){// ------------------------- z direction -------------------------
      fm = clip(f[j-mxy]);
      fp = clip(f[j+mxy]);
  }

  am = MAX2(fc, fm);
  ap = MAX2(fc, fp);  
  //--- flux correction ---
  *flm *= (1.0 - am);
  *flp *= (1.0 - ap);

  return 0;
}

int IBM_flux_corr_vof(type *flw, type *fle, type *fls, type *fln, type *flb, type *flt, int j, type *f, domain *cdo)
{
  int  mx=cdo->mx, mxy=cdo->mxy;
  type fc,fw,fe,fs,fn,fb,ft, aw,ae,as,an,ab,at;
  // clip vof
  fc = clip(f[j]);
  fw = clip(f[j-1]);
  fe = clip(f[j+1]);
  fs = clip(f[j-mx]);
  fn = clip(f[j+mx]);
  fb = clip(f[j-mxy]);
  ft = clip(f[j+mxy]);
  //--- calc solid ratio at cell face ---
  aw = MAX2(fc, fw);
  ae = MAX2(fc, fe);
  as = MAX2(fc, fs);
  an = MAX2(fc, fn);
  ab = MAX2(fc, fb);
  at = MAX2(fc, ft);

  //--- flux correction ---
  *flw *= (1.0 - aw);
  *fle *= (1.0 - ae);
  *fls *= (1.0 - as);
  *fln *= (1.0 - an);
  *flb *= (1.0 - ab);
  *flt *= (1.0 - at);
  return 0;
}




int outflow_flux2(type *flw, type *fle, type *fls, type *fln, type *flb, type *flt, int jx, int jy, int jz, int nx, int ny, int nz, mpi_param *mpi, flags *flg)
{
  if(flg->bc_xm!=OUT && mpi->nrk[4]==-1 && jx==0    && *flw>=0.0) *flw = 0.0;// west
  if(flg->bc_xp!=OUT && mpi->nrk[5]==-1 && jx==nx-1 && *fle<=0.0) *fle = 0.0;// east
  if(flg->bc_ym!=OUT && mpi->nrk[2]==-1 && jy==0    && *fls>=0.0) *fls = 0.0;// south
  if(flg->bc_yp!=OUT && mpi->nrk[3]==-1 && jy==ny-1 && *fln<=0.0) *fln = 0.0;// north
  if(flg->bc_zm!=OUT && mpi->nrk[0]==-1 && jz==0    && *flb>=0.0) *flb = 0.0;// bottom
  if(flg->bc_zp!=OUT && mpi->nrk[1]==-1 && jz==nz-1 && *flt<=0.0) *flt = 0.0;// top
  return 0;
}

int outflow_flux_direction_split(type *flm, type *flp, int j, int n, mpi_param *mpi,int direction_flag)
{
  if(direction_flag==0){ // ------------------------- x direction -------------------------
      if(mpi->nrk[4]==-1 && j==0    && *flm>=0.0) *flm = 0.0;// west
      if(mpi->nrk[5]==-1 && j==n-1 && *flp<=0.0) *flp = 0.0;// east
  }else if(direction_flag==1){// ------------------------- y direction -------------------------
      if(mpi->nrk[2]==-1 && j==0    && *flm>=0.0) *flm = 0.0;// south
      if(mpi->nrk[3]==-1 && j==n-1 && *flp<=0.0) *flp = 0.0;// north
  }else if(direction_flag==2){// ------------------------- z direction -------------------------
      if(mpi->nrk[0]==-1 && j==0    && *flm>=0.0) *flm = 0.0;// bottom
      if(mpi->nrk[1]==-1 && j==n-1 && *flp<=0.0) *flp = 0.0;// top
  }
  return 0;
}

int outflow_flux(type *flw, type *fle, type *fls, type *fln, type *flb, type *flt, int jx, int jy, int jz, int nx, int ny, int nz, mpi_param *mpi)
{
  if(mpi->nrk[4]==-1 && jx==0    && *flw>=0.0) *flw = 0.0;// west
  if(mpi->nrk[5]==-1 && jx==nx-1 && *fle<=0.0) *fle = 0.0;// east
  if(mpi->nrk[2]==-1 && jy==0    && *fls>=0.0) *fls = 0.0;// south
  if(mpi->nrk[3]==-1 && jy==ny-1 && *fln<=0.0) *fln = 0.0;// north
  if(mpi->nrk[0]==-1 && jz==0    && *flb>=0.0) *flb = 0.0;// bottom
  if(mpi->nrk[1]==-1 && jz==nz-1 && *flt<=0.0) *flt = 0.0;// top
  return 0;
}


