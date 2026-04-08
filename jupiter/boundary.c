#include "component_data.h"
#include "component_data_defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#ifdef GPU
#  include <cuda.h>
#  include <cutil.h>
#endif

#include "struct.h"
#include "func.h"
#include "common_util.h"
#include "boundary_util.h"

// local functions
void bcf_wall(type *f, parameter *prm);
void bcf_wall_n(type *nwx, type *nwy, type *nwz, parameter *prm);
void bcf_wall_s(type *f, type *v, parameter *prm, variable *val, material *mtl);
void BCP(type *f, parameter *prm, variable *val);
void bcf_wall_VOF(type *f, parameter *prm);
void bcf_wall_inflow(type *f, parameter *prm, variable *val);
void bcf_wall_inflow_layer(type *f_layer, parameter *prm, variable *val);
//Y void bcu_wall_slip_out(type *u, type *v, type *w, parameter *prm);
void bcu_wall_slip_out(type *u, type *v, type *w, material *mtl, parameter *prm, variable *val);
void bct_Ins_Iso_Diff(type *f, variable *val, parameter *prm);

/* YSE: Basic calculations */
static void
boundary_point_symmetry(type *base, ptrdiff_t offset, int stencil_size,
                        type symmetry_val)
{
  int j;
  symmetry_val *= 2.0;
  for (j = 0; j < stencil_size; ++j) {
    *(base + (j + 1) * offset) = symmetry_val - *(base - j * offset);
  }
}

static void
boundary_point_symmetry_stable(type *base, ptrdiff_t offset, int stencil_size,
                        type symmetry_val)
{
  int j;

  for (j = 0; j < stencil_size; ++j) {
    if(*(base - j * offset)>=0){
      *(base + (j + 1) * offset) = - *(base - j * offset);      
    }else{
      *(base + (j + 1) * offset) = *(base - j * offset);  
    }
  }
  
}

static void
boundary_line_symmetry(type *base, ptrdiff_t offset, int stencil_size)
{
  int j;
  for (j = 1; j <= stencil_size; ++j) {
    *(base + j * offset) = *(base - (j - 1) * offset);
  }
}

static void
boundary_set_value(type *base, ptrdiff_t offset, int stencil_size, type value)
{
  int j;
  for (j = 1; j <= stencil_size; ++j) {
    *(base + j * offset) = value;
  }
}

static void
boundary_point_symmetry_perp(type *base, ptrdiff_t offset, int stencil_size,
                             type symmetry_val)
{
  int j;
  if (offset > 0) {
    base += offset;
  }
  *base = symmetry_val;
  symmetry_val *= 2.0;
  for (j = 1; j <= stencil_size; ++j) {
    *(base + j * offset) = symmetry_val - *(base - j * offset);
  }
}

static void
boundary_line_symmetry_perp(type *base, ptrdiff_t offset, int stencil_size)
{
  int j;
  if (offset > 0) {
    base += offset;
  }
  for (j = 1; j <= stencil_size; ++j) {
    *(base + j * offset) = *(base - j * offset);
  }
}
/* YSE: End */


//================================
//         Velocity
//--------------------------------
type bcu(type *u, type *v, type *w, variable *val, material *mtl, parameter *prm)
{
  if(prm->flg->fluid_dynamics == OFF) return 0.0;
  flags  *flg=prm->flg;
  // MPI communication
  if(prm->mpi->npe > 1) {
    communication(u, prm);
    communication(v, prm);
    communication(w, prm);
  }
  // Immersed Boundary
  if (flg->IBM == ON) {
    type *nvx, *nvy, *nvz;
    type *ls;
    type *fs;
    int use_nvibm = 0;
    if (flg->solute_diff == ON) {
      fs = val->fs;
    } else {
      fs = val->fs_sum;
    }
    fs = val->fs_ibm ? val->fs_ibm : fs;
    ls = val->ls_ibm ? val->ls_ibm : val->ls;
    use_nvibm = val->nvibmx && val->nvibmy && val->nvibmz;
    nvx = use_nvibm ? val->nvibmx : val->nvsx;
    nvy = use_nvibm ? val->nvibmy : val->nvsy;
    nvz = use_nvibm ? val->nvibmz : val->nvsz;
    IBM_vector(ls, fs, u, v, w, nvx, nvy, nvz, val->surface_bnd,
           val->bnd_norm_u, val->bnd_norm_v, val->bnd_norm_w, prm->cdo);
  }
  // Boundary
  bcu_wall_slip_out(u, v, w, mtl, prm, val);
  return 0.0;
}

//================================
//         Velocity without IBM
//--------------------------------
type bcu_correct(type *u, type *v, type *w, variable *val, material *mtl, parameter *prm)
{
  if(prm->flg->fluid_dynamics == OFF) return 0.0;
  flags  *flg=prm->flg;
  // MPI communication
  if(prm->mpi->npe > 1) {
    communication(u, prm);
    communication(v, prm);
    communication(w, prm);
  }
  // Boundary
  bcu_wall_slip_out(u, v, w, mtl, prm, val);
  return 0.0;
}


//================================
//        Scalar values
//--------------------------------
type bcf_VOF(int flag, type *f, variable *val, parameter *prm)
{
  // MPI communication
  if(prm->mpi->npe > 1) {
    domain *cdo=prm->cdo;
    int m=cdo->m, icompo;
      if (prm->flg->solute_diff == ON) {
        communication(f, prm);
      } else {
        for(icompo = 0; icompo < cdo->NumberOfComponent; icompo++)
          communication(&f[icompo*m], prm);
      }      
  }
  // Boundary
  if(flag == 0) bcf_wall_VOF(f, prm);
  if(flag == 1) bcf_wall_inflow(f, prm, val);
  return 0.0;
}

//================================
//        Scalar values
//--------------------------------
type bcf_VOF_layer(int flag, type *f_layer, variable *val, parameter *prm)
{
  if(flag == 1) bcf_wall_inflow_layer(f_layer, prm, val);
  return 0.0;
}

//================================
//        Scalar values
//--------------------------------
type bcf(type *f, parameter *prm)
{
  // MPI communication
  if(prm->mpi->npe > 1) {
    communication(f, prm);
  }
  // Boundary
  bcf_wall(f, prm);
  return 0.0;
}

//================================
//        Set normal vector at the boundaries
//--------------------------------
type bcn(type *nwx, type *nwy, type *nwz, parameter *prm)
{
  // MPI communication
  if(prm->mpi->npe > 1) {
    communication(nwx, prm);
    communication(nwy, prm);
    communication(nwz, prm);
  }
  // Boundary
  bcf_wall_n(nwx, nwy, nwz, prm);
  return 0.0;
}

//================================
//        Communication only
//--------------------------------
type bcc(type *f, parameter *prm)
{
  // MPI communication
  if(prm->mpi->npe > 1) {
    communication(f, prm);
  }
  return 0.0;
}

type bcs(type *f, type *v, variable *val, parameter *prm, material *mtl)
{
  // MPI communication
  if(prm->mpi->npe > 1) {
		domain *cdo=prm->cdo;
		int m=cdo->m, icompo;
    for(icompo = 0; icompo < cdo->NumberOfComponent; icompo++) 
      communication(&f[icompo*m], prm);
    if (v) {
      for(icompo = 0; icompo < cdo->NBaseComponent; icompo++)
        communication(&v[icompo*m], prm);
    }
  }
  // Boundary
  bcf_wall_s(f, v, prm, val, mtl);
  return 0.0;
}

//================================
//        Pressure
//--------------------------------
type bcp(type *f, variable *val, parameter *prm)
{
  // MPI communication
  if(prm->mpi->npe > 1) {
    communication(f, prm);
  }
  // Boundary
  BCP(f, prm, val);
  return 0.0;
}

//================================
//        Temperature
//--------------------------------
type bct(type *f, variable *val, parameter *prm)
{
  // MPI communication
  if(prm->mpi->npe > 1) {
    communication(f, prm);
  }
  // Boundary
  bct_Ins_Iso_Diff(f, val, prm);
  return 0.0;
}

//================================
//  Wall boundary (scalar values)
//--------------------------------
void bcf_wall(type *f, parameter *prm)
{
  domain    *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;
  int icompo,i,j,jx,jy,jz,
      nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
      m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;
  type xc,zc;

  icompo = 0;
  // bottom
  if(mpi->nrk[0] == -1) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        i = jx + mx*jy;
        f[i + mxy*0 + icompo*m] = f[i + mxy*5 + icompo*m];
        f[i + mxy*1 + icompo*m] = f[i + mxy*4 + icompo*m];
        f[i + mxy*2 + icompo*m] = f[i + mxy*3 + icompo*m];
      }
    }
  }
  // top
  if(mpi->nrk[1] == -1) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        i = jx + mx*jy;
        f[i + mxy*(nz+3) + icompo*m] = f[i + mxy*(nz+2) + icompo*m];
        f[i + mxy*(nz+4) + icompo*m] = f[i + mxy*(nz+1) + icompo*m];
        f[i + mxy*(nz+5) + icompo*m] = f[i + mxy*(nz  ) + icompo*m];
        f[i + mxy*(nz+6) + icompo*m] = f[i + mxy*(nz-1) + icompo*m];
      }
    }
  }
  // south
  if(mpi->nrk[2] == -1) {
    for(jz = 0; jz < mz; jz++) {
      for(jx = 0; jx < mx; jx++) {
              i = jx + mx*my*jz;
              f[i + mx*0 + icompo*m] = f[i + mx*5 + icompo*m];
              f[i + mx*1 + icompo*m] = f[i + mx*4 + icompo*m];
              f[i + mx*2 + icompo*m] = f[i + mx*3 + icompo*m];
      }
    }
  }
  // north
  if(mpi->nrk[3] == -1) {
    for(jz = 0; jz < mz; jz++) {
      for(jx = 0; jx < mx; jx++) {
        i = jx + mx*my*jz;
        f[i + mx*(ny+3) + icompo*m] = f[i + mx*(ny+2) + icompo*m];
        f[i + mx*(ny+4) + icompo*m] = f[i + mx*(ny+1) + icompo*m];
        f[i + mx*(ny+5) + icompo*m] = f[i + mx*(ny  ) + icompo*m];
        f[i + mx*(ny+6) + icompo*m] = f[i + mx*(ny-1) + icompo*m];
      }
    }
  }
  // west
  if(mpi->nrk[4] == -1) {
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
        j = jy + my*jz;
        f[0 + mx*j + icompo*m] = f[5 + mx*j + icompo*m];
        f[1 + mx*j + icompo*m] = f[4 + mx*j + icompo*m];
        f[2 + mx*j + icompo*m] = f[3 + mx*j + icompo*m];
      }
    }
  }
  // east
  if(mpi->nrk[5] == -1) {
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
        j = jy + my*jz;
        f[(nx+3) + mx*j + icompo*m] = f[(nx+2) + mx*j + icompo*m];
        f[(nx+4) + mx*j + icompo*m] = f[(nx+1) + mx*j + icompo*m];
        f[(nx+5) + mx*j + icompo*m] = f[(nx  ) + mx*j + icompo*m];
        f[(nx+6) + mx*j + icompo*m] = f[(nx-1) + mx*j + icompo*m];
      }
    }
  }
}
//================================
//  Wall normal vector (boundary)
//--------------------------------
void bcf_wall_n(type *nwx, type *nwy, type *nwz, parameter *prm)
{
  domain    *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;
  int icompo,i,j,jx,jy,jz,
      nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
      m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;
  type xc,zc;

  icompo = 0;
  // bottom (zm)
  if(mpi->nrk[0] == -1) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        i = jx + mx*jy;
        nwx[i + mxy*0 + icompo*m] = 0.0;
        nwx[i + mxy*1 + icompo*m] = 0.0;
        nwx[i + mxy*2 + icompo*m] = 0.0;

        nwy[i + mxy*0 + icompo*m] = 0.0;
        nwy[i + mxy*1 + icompo*m] = 0.0;
        nwy[i + mxy*2 + icompo*m] = 0.0;

        nwz[i + mxy*0 + icompo*m] = -1.0;
        nwz[i + mxy*1 + icompo*m] = -1.0;
        nwz[i + mxy*2 + icompo*m] = -1.0;
      }
    }
  }
  // top (zp)
  if(mpi->nrk[1] == -1) {
    for(jy = 0; jy < my; jy++) {
      for(jx = 0; jx < mx; jx++) {
        i = jx + mx*jy;
        nwx[i + mxy*(nz+3) + icompo*m] = 0.0;
        nwx[i + mxy*(nz+4) + icompo*m] = 0.0;
        nwx[i + mxy*(nz+5) + icompo*m] = 0.0;
        nwx[i + mxy*(nz+6) + icompo*m] = 0.0;

        nwy[i + mxy*(nz+3) + icompo*m] = 0.0;
        nwy[i + mxy*(nz+4) + icompo*m] = 0.0;
        nwy[i + mxy*(nz+5) + icompo*m] = 0.0;
        nwy[i + mxy*(nz+6) + icompo*m] = 0.0;

        nwz[i + mxy*(nz+3) + icompo*m] = 1.0;
        nwz[i + mxy*(nz+4) + icompo*m] = 1.0;
        nwz[i + mxy*(nz+5) + icompo*m] = 1.0;
        nwz[i + mxy*(nz+6) + icompo*m] = 1.0;
      }
    }
  }
  // south (ym)
  if(mpi->nrk[2] == -1) {
    for(jz = 0; jz < mz; jz++) {
      for(jx = 0; jx < mx; jx++) {
        i = jx + mx*my*jz;
        nwx[i + mx*0 + icompo*m] = 0.0;
        nwx[i + mx*1 + icompo*m] = 0.0;
        nwx[i + mx*2 + icompo*m] = 0.0;
        
        nwy[i + mx*0 + icompo*m] = -1.0;
        nwy[i + mx*1 + icompo*m] = -1.0;
        nwy[i + mx*2 + icompo*m] = -1.0;
       
        nwz[i + mx*0 + icompo*m] = 0.0;
        nwz[i + mx*1 + icompo*m] = 0.0;
        nwz[i + mx*2 + icompo*m] = 0.0;
      }
    }
  }
  // north (yp)
  if(mpi->nrk[3] == -1) {
    for(jz = 0; jz < mz; jz++) {
      for(jx = 0; jx < mx; jx++) {
        i = jx + mx*my*jz;
        nwx[i + mx*(ny+3) + icompo*m] = 0.0;
        nwx[i + mx*(ny+4) + icompo*m] = 0.0;
        nwx[i + mx*(ny+5) + icompo*m] = 0.0;
        nwx[i + mx*(ny+6) + icompo*m] = 0.0;

        nwy[i + mx*(ny+3) + icompo*m] = 1.0;
        nwy[i + mx*(ny+4) + icompo*m] = 1.0;
        nwy[i + mx*(ny+5) + icompo*m] = 1.0;
        nwy[i + mx*(ny+6) + icompo*m] = 1.0;

        nwz[i + mx*(ny+3) + icompo*m] = 0.0;
        nwz[i + mx*(ny+4) + icompo*m] = 0.0;
        nwz[i + mx*(ny+5) + icompo*m] = 0.0;
        nwz[i + mx*(ny+6) + icompo*m] = 0.0;
      }
    }
  }
  // west (xm)
  if(mpi->nrk[4] == -1) {
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
        j = jy + my*jz;
        nwx[0 + mx*j + icompo*m] = -1.0;
        nwx[1 + mx*j + icompo*m] = -1.0;
        nwx[2 + mx*j + icompo*m] = -1.0;

        nwy[0 + mx*j + icompo*m] = 0.0;
        nwy[1 + mx*j + icompo*m] = 0.0;
        nwy[2 + mx*j + icompo*m] = 0.0;

        nwz[0 + mx*j + icompo*m] = 0.0;
        nwz[1 + mx*j + icompo*m] = 0.0;
        nwz[2 + mx*j + icompo*m] = 0.0;
      }
    }
  }
  // east (xp)
  if(mpi->nrk[5] == -1) {
    for(jz = 0; jz < mz; jz++) {
      for(jy = 0; jy < my; jy++) {
        j = jy + my*jz;
        nwx[(nx+3) + mx*j + icompo*m] = 1.0;
        nwx[(nx+4) + mx*j + icompo*m] = 1.0;
        nwx[(nx+5) + mx*j + icompo*m] = 1.0;
        nwx[(nx+6) + mx*j + icompo*m] = 1.0;

        nwy[(nx+3) + mx*j + icompo*m] = 0.0;
        nwy[(nx+4) + mx*j + icompo*m] = 0.0;
        nwy[(nx+5) + mx*j + icompo*m] = 0.0;
        nwy[(nx+6) + mx*j + icompo*m] = 0.0;
        
        nwz[(nx+3) + mx*j + icompo*m] = 0.0;
        nwz[(nx+4) + mx*j + icompo*m] = 0.0;
        nwz[(nx+5) + mx*j + icompo*m] = 0.0;
        nwz[(nx+6) + mx*j + icompo*m] = 0.0;
      }
    }
  }
}
/**
 * @brief Commonized pressure boundary calculation.
 * @param f          Pressure value array
 * @param cond_array boundaray data array.
 * @param jbx        X index for cond_array
 * @param jby        Y index for cond_array
 * @param nbx        X size of cond_array
 * @param nby        Y size of cond_array
 * @param stencil_size Number of stencil
 * @param offset     Array offset to next value on the direction of
 *                   perpendicular to boundary.
 * @param jx         X index on the boundary of velocities
 * @param jy         Y index on the boundary of velocities
 * @param jz         Z index on the boundary of velocities
 * @param mx         X size of velocities
 * @param my         Y size of velocities
 * @param mz         Z size of velocities
 * @param pconst     Constant value of outflow boundary
 *
 * The cell (jx, jy, jz) must be one of the most outer cell which is
 * on the boundary, **inside** the calculation domain. For example,
 * the cell at index `cdo->stm` will be lower side boundary and the
 * cell at index `cdo->nx - 1 + cdo->stm` will be upper side boundary.
 *
 * \p stencil_size must be `cdo->stm` or `cdo->stp - 1`, note that
 * subtract 1 from `cdo->stp` when calculating upper side boundary.
 *
 * \p offset is pointer offset to get next value on the boundary. If
 * calculating boundary is West (X-) boundary, offset will be -1. If
 * calculating boundary is East (X+) boundary, offset will be +1. So,
 * for South (Y-) boundary, offset will be `-cdo->mx`. For North (Y+)
 * boundary, offset will be `cdo->mx`. For Bottom (Z-) boundary,
 * offset will be `-cdo->mxy` and for Top (Z+) boundary, offset will
 * be `cdo->mxy`.
 */
static void
bcp_calc(type *f, fluid_boundary_data **cond_array,
         int jbx, int jby, int nbx, int nby, int stencil_size,
         ptrdiff_t offset, int jx, int jy, int jz, int mx, int my, int mz)
{
  fluid_boundary_data *fp;
  ptrdiff_t jj;
  type *base;

  fp = cond_array[jbx + jby * nbx];
  jj = calc_address(jx, jy, jz, mx, my, mz);

  base = &f[jj];
  if (fp->cond == OUT) {
    if (fp->out_p_cond == OUT_P_COND_NEUMANN) {
      boundary_line_symmetry(base, offset, stencil_size);
    } else if (fp->out_p_cond == OUT_P_COND_CONST) {
      type pconst = fp->const_p.current_value;
      boundary_point_symmetry(base, offset, stencil_size, pconst);
    }
  } else {
    boundary_line_symmetry(base, offset, stencil_size);
  }
}

//================================
//  Pressure boundary
//--------------------------------
void BCP(type *f, parameter *prm, variable *val)
{
  flags     *flg = prm->flg;
  domain    *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;
  int icompo,i,j,jx,jy,jz,
      nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
      m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;
  int nbx = cdo->nbx, nby = cdo->nby, nbz = cdo->nbz;

  // bottom [z-] (zm)
  if(mpi->nrk[0] == -1) {
#pragma omp parallel for collapse(2)
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        int jox, joy, jbx, jby;
        jox = jx + cdo->stm;
        joy = jy + cdo->stm;
        jbx = jx + cdo->stmb;
        jby = jy + cdo->stmb;

        bcp_calc(f, val->bnd_B.fl, jbx, jby, nbx, nby,
                 cdo->stm, -mxy, jox, joy, cdo->stm, mx, my, mz);
      }
    }
  }
  // top [z+] (zp)
  if(mpi->nrk[1] == -1) {
#pragma omp parallel for collapse(2)
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        int jox, joy, jbx, jby;
        jox = jx + cdo->stm;
        joy = jy + cdo->stm;
        jbx = jx + cdo->stmb;
        jby = jy + cdo->stmb;

        bcp_calc(f, val->bnd_T.fl, jbx, jby, nbx, nby,
                 cdo->stp - 1, mxy, jox, joy, cdo->stm + nz - 1, mx, my, mz);
      }
    }
  }

  // south [y-] (ym)
  if(mpi->nrk[2] == -1) {
#pragma omp parallel for collapse(2)
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
        int jox, joz, jbx, jbz;
        jox = jx + cdo->stm;
        joz = jz + cdo->stm;
        jbx = jx + cdo->stmb;
        jbz = jz + cdo->stmb;

        bcp_calc(f, val->bnd_S.fl, jbx, jbz, nbx, nbz,
                 cdo->stm, -mx, jox, cdo->stm, joz, mx, my, mz);
      }
    }
  }
  // north [y+] (yp)
  if(mpi->nrk[3] == -1) {
#pragma omp parallel for collapse(2)
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
        int jox, joz, jbx, jbz;
        jox = jx + cdo->stm;
        joz = jz + cdo->stm;
        jbx = jx + cdo->stmb;
        jbz = jz + cdo->stmb;

        bcp_calc(f, val->bnd_N.fl, jbx, jbz, nbx, nbz,
                 cdo->stp - 1, mx, jox, cdo->stm + ny - 1, joz, mx, my, mz);
      }
    }
  }
  // west [x-] (xm)
  if(mpi->nrk[4] == -1) {
#pragma omp parallel for collapse(2)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        int joy, joz, jby, jbz;
        joy = jy + cdo->stm;
        joz = jz + cdo->stm;
        jby = jy + cdo->stmb;
        jbz = jz + cdo->stmb;

        bcp_calc(f, val->bnd_W.fl, jby, jbz, nby, nbz,
                 cdo->stm, -1, cdo->stm, joy, joz, mx, my, mz);
      }
    }
  }
  // east [x+] (xp)
  if(mpi->nrk[5] == -1) {
#pragma omp parallel for collapse(2)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        int joy, joz, jby, jbz;
        joy = jy + cdo->stm;
        joz = jz + cdo->stm;
        jby = jy + cdo->stmb;
        jbz = jz + cdo->stmb;

        bcp_calc(f, val->bnd_E.fl, jby, jbz, nby, nbz,
                 cdo->stp - 1, 1, cdo->stm + nx - 1, joy, joz, mx, my, mz);
      }
    }
  }
}

/*
 * YSE: Add inflow condition for Y.
 */
static void
bcs_inlet(type *Ybase, type *Ytbase, type *fsbase, type *flbase,
          ptrdiff_t offset, int stencil_size, type ratio)
{
  int j;
  type fs;
  type Ys;
  type r;

  for (j = 1; j <= stencil_size; ++j) {
    fs = *(fsbase + j * offset);
    Ys = *(Ybase + j * offset);
    fs = clip(fs);
    r = Ys * fs + (1.0 - fs) * ratio;

    *(Ybase + j * offset) = r;
  }
}

/**
 * @brief Commonized solute Y boundary calculation.
 * @param f          solute Y value array
 * @param Yt         Sum of Y value array
 * @param fs         Solid VOF value array
 * @paarm fl         Liquid VOF value array
 * @param icompo     Component data for this Material ID number
 * @param cond_array boundary data array.
 * @param jbx        X index for cond_array
 * @param jby        Y index for cond_array
 * @param nbx        X size of cond_array
 * @param nby        Y size of cond_array
 * @param stencil_size Number of stencil
 * @param offset     Array offset to next value on the direction of
 *                   perpendicular to boundary.
 * @param jx         X index on the boundary of velocities
 * @param jy         Y index on the boundary of velocities
 * @param jz         Z index on the boundary of velocities
 * @param mx         X size of velocities
 * @param my         Y size of velocities
 * @param mz         Z size of velocities
 *
 * The cell (jx, jy, jz) must be one of the most outer cell which is
 * on the boundary, **inside** the calculation domain. For example,
 * the cell at index `cdo->stm` will be lower side boundary and the
 * cell at index `cdo->nx - 1 + cdo->stm` will be upper side boundary.
 *
 * \p stencil_size must be `cdo->stm` or `cdo->stp - 1`, note that
 * subtract 1 from `cdo->stp` when calculating upper side boundary.
 *
 * \p offset is pointer offset to get next value on the boundary. If
 * calculating boundary is West (X-) boundary, offset will be -1. If
 * calculating boundary is East (X+) boundary, offset will be +1. So,
 * for South (Y-) boundary, offset will be `-cdo->mx`. For North (Y+)
 * boundary, offset will be `cdo->mx`. For Bottom (Z-) boundary,
 * offset will be `-cdo->mxy` and for Top (Z+) boundary, offset will
 * be `cdo->mxy`.
 *
 * For \p f, give base address of material \p icompo.
 *
 * Normalize to \f$ \sum Y_\text{liquid} = 1 \f$ and \f$ \sum
 * Y_\text{gas} = 1 \f$, if any corresponding components are assigned
 * for inlet. \f$ Y_\text{gas} \f$ includes component ID -1.
 */
static void
bcs_calc(type *f, type *Yt, type *fs, type *fl, component_data *icompo,
         fluid_boundary_data **cond_array,
         int jbx, int jby, int nbx, int nby, int stencil_size,
         ptrdiff_t offset, int jx, int jy, int jz, int mx, int my, int mz)
{
  fluid_boundary_data *fp;
  ptrdiff_t jj;
  type *base;

  fp = cond_array[jbx + jby * nbx];
  jj = calc_address(jx, jy, jz, mx, my, mz);

  if (fp->cond == INLET) {
    type ratio;
    type lratsum;
    int j;
    int ncomp;
    int gas;
    gas = component_phases_is_gas_only(icompo->phases);
    ratio = 0.0;
    lratsum = 0.0;
    ncomp = inlet_component_data_ncomp(fp->comps);
    for (j = 0; j < ncomp; ++j) {
      int igas;
      struct inlet_component_element *e;
      e = inlet_component_data_get(fp->comps, j);
      if (e->comp.d == icompo) {
        ratio += e->ratio.current_value;
      }

      igas = component_phases_is_gas_only(e->comp.d->phases);
      if (gas == igas) { /* (gas && igas) || (!igas && !igas) */
        lratsum += e->ratio.current_value;
      }
    }
    if (ratio > 0.0 && lratsum > 0.0) {
      ratio = ratio / lratsum;
    } else {
      ratio = 0.0;
    }
    jj = jj - offset;
    base = &f[jj];
    bcs_inlet(base, &Yt[jj], &fs[jj], &fl[jj], offset, stencil_size + 1, ratio);
  } else {
    base = &f[jj];
    boundary_line_symmetry(base, offset, stencil_size);
  }
}

static void
bcvf_inlet(type *Vfbase, type *Yibase, type Vpi, type *Vp0, type *vf0,
           type **Y0a, ptrdiff_t offset, int stencil_size)
{
  type Yi, Y0;
  type Vf;
  type *Y0base;
  int j;

  for (j = 1; j <= stencil_size; ++j) {
    Y0base = Y0a[j - 1];
    if (Y0base) {
      if (Yibase == Y0base) {
        *(Vfbase + j * offset) = vf0[j - 1];
      } else {
        Yi = *(Yibase + j * offset);
        Y0 = *(Y0base + j * offset);
        Vf = Vp0[j - 1] * Y0;
        if (Vf <= 0.0) {
          *(Vfbase + j * offset) = 0.0;
        } else {
          *(Vfbase + j * offset) = (Vpi * Yi) * vf0[j - 1] / Vf;
        }
      }
    } else {
      *(Vfbase + j * offset) = 0.0;
    }
  }
}

/**
 * @brief Commonized solute Vf boundary calculation.
 * @param vf         solute Vf value array ((m*ncompo)-sized)
 * @param y          solute Y value array ((m*ncompo)-sized)
 * @param vp         solute Vp value array (ncompo-sized)
 * @param ncompo     Number of components
 * @param cond_array boundary data array.
 * @param jbx        X index for cond_array
 * @param jby        Y index for cond_array
 * @param nbx        X size of cond_array
 * @param nby        Y size of cond_array
 * @param stencil_size Number of stencil
 * @param offset     Array offset to next value on the direction of
 *                   perpendicular to boundary.
 * @param jx         X index on the boundary of velocities
 * @param jy         Y index on the boundary of velocities
 * @param jz         Z index on the boundary of velocities
 * @param mx         X size of velocities
 * @param my         Y size of velocities
 * @param mz         Z size of velocities
 *
 * The cell (jx, jy, jz) must be one of the most outer cell which is
 * on the boundary, **inside** the calculation domain. For example,
 * the cell at index `cdo->stm` will be lower side boundary and the
 * cell at index `cdo->nx - 1 + cdo->stm` will be upper side boundary.
 *
 * \p stencil_size must be `cdo->stm` or `cdo->stp - 1`, note that
 * subtract 1 from `cdo->stp` when calculating upper side boundary.
 *
 * \p offset is pointer offset to get next value on the boundary. If
 * calculating boundary is West (X-) boundary, offset will be -1. If
 * calculating boundary is East (X+) boundary, offset will be +1. So,
 * for South (Y-) boundary, offset will be `-cdo->mx`. For North (Y+)
 * boundary, offset will be `cdo->mx`. For Bottom (Z-) boundary,
 * offset will be `-cdo->mxy` and for Top (Z+) boundary, offset will
 * be `cdo->mxy`.
 *
 * Component iteration is performed *in* this function, which is not done
 * for bcs_calc(). Because `vf` on inlet is computed from `y` and it requires
 * all components value of `y`.
 *
 * Some stacks will be allocated by the size of `stencil_size`.
 */
static void
bcvf_calc(type *vf, type *y, type *vp, int ncompo,
          fluid_boundary_data **cond_array,
          int jbx, int jby, int nbx, int nby, int stencil_size,
          ptrdiff_t offset, int jx, int jy, int jz, int mx, int my, int mz)
{
  fluid_boundary_data *fp;
  type *Vfbase, *Yibase;
  ptrdiff_t jj, m;
  int j;
  type vp0[stencil_size];
  type vf0[stencil_size]; /* Assumes stencil_size is enough small */
  type *Y0base[stencil_size];
  int icompo;

  fp = cond_array[jbx + jby * nbx];
  jj = calc_address(jx, jy, jz, mx, my, mz);
  m  = mx;
  m *= my;
  m *= mz;

  if (fp->cond == INLET) {
    for (j = 1; j <= stencil_size; ++j) {
      type d;
      type y0;

      Y0base[j - 1] = NULL;
      for (icompo = 0; icompo < ncompo; ++icompo) {
        Yibase = &y[jj + m * icompo];
        y0 = *(Yibase + j * offset);
        y0 = vp[icompo] * y0;
        if (y0 != 0.0) {
          vp0[j - 1] = vp[icompo];
          Y0base[j - 1] = Yibase;
          break;
        }
      }

      if (Y0base[j - 1]) {
        type yi;
        d = 1.0;
        for (icompo = 0; icompo < ncompo; ++icompo) {
          Yibase = &y[jj + m * icompo];
          if (Yibase == Y0base[j - 1]) continue;
          yi = *(Yibase + j * offset);
          y0 = *(Y0base[j - 1] + j * offset);
          d += (vp[icompo] * yi) / (vp0[j - 1] * y0);
        }
        vf0[j - 1] = 1.0 / d;
      } else {
        vp0[j - 1] = 0.0;
        vf0[j - 1] = 0.0;
      }
    }

    for (icompo = 0; icompo < ncompo; ++icompo) {
      Yibase = &y[jj + m * icompo];
      Vfbase = &vf[jj + m * icompo];
      bcvf_inlet(Vfbase, Yibase, vp[icompo], vp0, vf0, Y0base,
                 offset, stencil_size);
    }
  } else {
    for (icompo = 0; icompo < ncompo; ++icompo) {
      Vfbase = &vf[jj + m * icompo];
      boundary_line_symmetry(Vfbase, offset, stencil_size);
    }
  }
}

/* YSE: end */

//================================
//  Wall boundary (scalar values)
//--------------------------------
void bcf_wall_s(type *f, type *v, parameter *prm, variable *val, material *mtl)
{
  domain    *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;
  int i,j,jx,jy,jz,
      nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
      m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;
  type xc,zc;
  int nbx = cdo->nbx, nby = cdo->nby, nbz = cdo->nbz;
  int nbcompo = cdo->NBaseComponent;

  // bottom [z-] (zm)
  if(mpi->nrk[0] == -1) {
#pragma omp parallel for collapse(2)
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        int jox, joy, jbx, jby;
        int icompo;
        jox = jx + cdo->stm;
        joy = jy + cdo->stm;
        jbx = jx + cdo->stmb;
        jby = jy + cdo->stmb;

        for (icompo = 0; icompo < cdo->NumberOfComponent; icompo++) {
          component_data *p;
          p = component_data_find_by_comp_index(&prm->comps_data_head, icompo);
          bcs_calc(f + icompo * cdo->m, val->Yt, val->fs, val->fl, p,
                   val->bnd_B.fl, jbx, jby, nbx, nby,
                   cdo->stm, -mxy, jox, joy, cdo->stm, mx, my, mz);
        }
        if (v && mtl) {
          bcvf_calc(v, f, mtl->Vp, cdo->NBaseComponent,
                    val->bnd_B.fl, jbx, jby, nbx, nby,
                    cdo->stm, -mxy, jox, joy, cdo->stm, mx, my, mz);
        }
      }
    }
  }
  // top [z+] (zp)
  if(mpi->nrk[1] == -1) {
#pragma omp parallel for collapse(2)
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        int jox, joy, jbx, jby;
        int icompo;
        jox = jx + cdo->stm;
        joy = jy + cdo->stm;
        jbx = jx + cdo->stmb;
        jby = jy + cdo->stmb;

        for (icompo = 0; icompo < cdo->NumberOfComponent; icompo++) {
          component_data *p;
          p = component_data_find_by_comp_index(&prm->comps_data_head, icompo);
          bcs_calc(f + icompo * cdo->m, val->Yt, val->fs, val->fl, p,
                   val->bnd_T.fl, jbx, jby, nbx, nby, cdo->stp - 1,
                   mxy, jox, joy, cdo->stm + nz - 1, mx, my, mz);
        }
        if (v && mtl) {
          bcvf_calc(v, f, mtl->Vp, cdo->NBaseComponent,
                    val->bnd_T.fl, jbx, jby, nbx, nby,
                    cdo->stp - 1, mxy, jox, joy, cdo->stm + nz - 1, mx, my, mz);
        }
      }
    }
  }
  // south [y-] (ym)
  if(mpi->nrk[2] == -1) {
#pragma omp parallel for collapse(2)
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
        int jox, joz, jbx, jbz;
        int icompo;
        jox = jx + cdo->stm;
        joz = jz + cdo->stm;
        jbx = jx + cdo->stmb;
        jbz = jz + cdo->stmb;

        for (icompo = 0; icompo < cdo->NumberOfComponent; icompo++) {
          component_data *p;
          p = component_data_find_by_comp_index(&prm->comps_data_head, icompo);
          bcs_calc(f + icompo * cdo->m, val->Yt, val->fs, val->fl, p,
                   val->bnd_S.fl, jbx, jbz, nbx, nbz,
                   cdo->stm, -mx, jox, cdo->stm, joz, mx, my, mz);
        }
        if (v && mtl) {
          bcvf_calc(v, f, mtl->Vp, cdo->NBaseComponent,
                    val->bnd_S.fl, jbx, jbz, nbx, nbz,
                    cdo->stm, -mx, jox, cdo->stm, joz, mx, my, mz);
        }
      }
    }
  }
  // north [y+] (yp)
  if(mpi->nrk[3] == -1) {
#pragma omp parallel for collapse(2)
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
        int jox, joz, jbx, jbz;
        int icompo;
        jox = jx + cdo->stm;
        joz = jz + cdo->stm;
        jbx = jx + cdo->stmb;
        jbz = jz + cdo->stmb;

        for (icompo = 0; icompo < cdo->NumberOfComponent; icompo++) {
          component_data *p;
          p = component_data_find_by_comp_index(&prm->comps_data_head, icompo);
          bcs_calc(f + icompo * cdo->m, val->Yt, val->fs, val->fl, p,
                   val->bnd_N.fl, jbx, jbz, nbx, nbz,
                   cdo->stp - 1, mx, jox, cdo->stm + ny - 1, joz, mx, my, mz);
        }
        if (v && mtl) {
          bcvf_calc(v, f, mtl->Vp, cdo->NBaseComponent,
                    val->bnd_N.fl, jbx, jbz, nbx, nbz,
                    cdo->stp - 1, mx, jox, cdo->stm + ny - 1, joz, mx, my, mz);
        }
      }
    }
  }
  // west [x-] (xm)
  if(mpi->nrk[4] == -1) {
#pragma omp parallel for collapse(2)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        int joy, joz, jby, jbz;
        int icompo;
        joy = jy + cdo->stm;
        joz = jz + cdo->stm;
        jby = jy + cdo->stmb;
        jbz = jz + cdo->stmb;

        for (icompo = 0; icompo < cdo->NumberOfComponent; icompo++) {
          component_data *p;
          p = component_data_find_by_comp_index(&prm->comps_data_head, icompo);
          bcs_calc(f + icompo * cdo->m, val->Yt, val->fs, val->fl, p,
                   val->bnd_W.fl, jby, jbz, nby, nbz,
                   cdo->stm, -1, cdo->stm, joy, joz, mx, my, mz);
        }
        if (v && mtl) {
          bcvf_calc(v, f, mtl->Vp, cdo->NBaseComponent,
                    val->bnd_W.fl, jby, jbz, nby, nbz,
                    cdo->stm, -1, cdo->stm, joy, joz, mx, my, mz);
        }
      }
    }
  }
  // east [x+] (xp)
  if(mpi->nrk[5] == -1) {
#pragma omp parallel for collapse(2)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        int joy, joz, jby, jbz;
        int icompo;
        joy = jy + cdo->stm;
        joz = jz + cdo->stm;
        jby = jy + cdo->stmb;
        jbz = jz + cdo->stmb;

        for (icompo = 0; icompo < cdo->NumberOfComponent; icompo++) {
          component_data *p;
          p = component_data_find_by_comp_index(&prm->comps_data_head, icompo);
          bcs_calc(f + icompo * cdo->m, val->Yt, val->fs, val->fl, p,
                   val->bnd_E.fl, jby, jbz, nby, nbz,
                   cdo->stp - 1, 1, cdo->stm + nx - 1, joy, joz, mx, my, mz);
        }
        if (v && mtl) {
          bcvf_calc(v, f, mtl->Vp, cdo->NBaseComponent,
                    val->bnd_E.fl, jby, jbz, nby, nbz,
                    cdo->stp - 1, 1, cdo->stm + nx - 1, joy, joz, mx, my, mz);
        }
      }
    }
  }
}


//================================
//  Wall boundary (scalar values)
//--------------------------------
void bcf_wall_VOF(type *f, parameter *prm)
{
  domain    *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;
  int icompo,i,j,jx,jy,jz,
    nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
    m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;
  type xc,zc;

  if(prm->flg->solute_diff == OFF){
    icompo = 0;
    for(icompo = 0; icompo < cdo->NumberOfComponent; icompo++)
    {
      // bottom
      if(mpi->nrk[0] == -1) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            i = jx + mx*jy;
            f[i + mxy*0 + icompo*m] = f[i + mxy*5 + icompo*m];
            f[i + mxy*1 + icompo*m] = f[i + mxy*4 + icompo*m];
            f[i + mxy*2 + icompo*m] = f[i + mxy*3 + icompo*m];
          }
        }
      }
      // top
      if(mpi->nrk[1] == -1) {
        for(jy = 0; jy < my; jy++) {
          for(jx = 0; jx < mx; jx++) {
            i = jx + mx*jy;
            f[i + mxy*(nz+3) + icompo*m] = f[i + mxy*(nz+2) + icompo*m];
            f[i + mxy*(nz+4) + icompo*m] = f[i + mxy*(nz+1) + icompo*m];
            f[i + mxy*(nz+5) + icompo*m] = f[i + mxy*(nz  ) + icompo*m];
            f[i + mxy*(nz+6) + icompo*m] = f[i + mxy*(nz-1) + icompo*m];
          }
        }
      }
      // south
      if(mpi->nrk[2] == -1) {
        for(jz = 0; jz < mz; jz++) {
          for(jx = 0; jx < mx; jx++) {
            i = jx + mx*my*jz;
            f[i + mx*0 + icompo*m] = f[i + mx*5 + icompo*m];
            f[i + mx*1 + icompo*m] = f[i + mx*4 + icompo*m];
            f[i + mx*2 + icompo*m] = f[i + mx*3 + icompo*m];
          }
        }
      }
      // north
      if(mpi->nrk[3] == -1) {
        for(jz = 0; jz < mz; jz++) {
          for(jx = 0; jx < mx; jx++) {
            i = jx + mx*my*jz;
            f[i + mx*(ny+3) + icompo*m] = f[i + mx*(ny+2) + icompo*m];
            f[i + mx*(ny+4) + icompo*m] = f[i + mx*(ny+1) + icompo*m];
            f[i + mx*(ny+5) + icompo*m] = f[i + mx*(ny  ) + icompo*m];
            f[i + mx*(ny+6) + icompo*m] = f[i + mx*(ny-1) + icompo*m];
          }
        }
      }
      // west
      if(mpi->nrk[4] == -1) {
        for(jz = 0; jz < mz; jz++) {
          for(jy = 0; jy < my; jy++) {
            j = jy + my*jz;
            f[0 + mx*j + icompo*m] = f[5 + mx*j + icompo*m];
            f[1 + mx*j + icompo*m] = f[4 + mx*j + icompo*m];
            f[2 + mx*j + icompo*m] = f[3 + mx*j + icompo*m];
          }
        }
      }
      // east
      if(mpi->nrk[5] == -1) {
        for(jz = 0; jz < mz; jz++) {
          for(jy = 0; jy < my; jy++) {
            j = jy + my*jz;
            f[(nx+3) + mx*j + icompo*m] = f[(nx+2) + mx*j + icompo*m];
            f[(nx+4) + mx*j + icompo*m] = f[(nx+1) + mx*j + icompo*m];
            f[(nx+5) + mx*j + icompo*m] = f[(nx  ) + mx*j + icompo*m];
            f[(nx+6) + mx*j + icompo*m] = f[(nx-1) + mx*j + icompo*m];
          }
        }
      }
    }
  } else {
    // bottom
    if(mpi->nrk[0] == -1) {
      for(jy = 0; jy < my; jy++) {
        for(jx = 0; jx < mx; jx++) {
          i = jx + mx*jy;
          f[i + mxy*0 ] = f[i + mxy*5 ];
          f[i + mxy*1 ] = f[i + mxy*4 ];
          f[i + mxy*2 ] = f[i + mxy*3 ];
        }
      }
    }
    // top
    if(mpi->nrk[1] == -1) {
      for(jy = 0; jy < my; jy++) {
        for(jx = 0; jx < mx; jx++) {
          i = jx + mx*jy;
          f[i + mxy*(nz+3) ] = f[i + mxy*(nz+2) ];
          f[i + mxy*(nz+4) ] = f[i + mxy*(nz+1) ];
          f[i + mxy*(nz+5) ] = f[i + mxy*(nz  ) ];
          f[i + mxy*(nz+6) ] = f[i + mxy*(nz-1) ];
        }
      }
    }
    // south
    if(mpi->nrk[2] == -1) {
      for(jz = 0; jz < mz; jz++) {
        for(jx = 0; jx < mx; jx++) {
          i = jx + mx*my*jz;
          f[i + mx*0 ] = f[i + mx*5 ];
          f[i + mx*1 ] = f[i + mx*4 ];
          f[i + mx*2 ] = f[i + mx*3 ];
        }
      }
    }
    // north
    if(mpi->nrk[3] == -1) {
      for(jz = 0; jz < mz; jz++) {
        for(jx = 0; jx < mx; jx++) {
          i = jx + mx*my*jz;
          f[i + mx*(ny+3) ] = f[i + mx*(ny+2) ];
          f[i + mx*(ny+4) ] = f[i + mx*(ny+1) ];
          f[i + mx*(ny+5) ] = f[i + mx*(ny  ) ];
          f[i + mx*(ny+6) ] = f[i + mx*(ny-1) ];
        }
      }
    }
    // west
    if(mpi->nrk[4] == -1) {
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          j = jy + my*jz;
          f[0 + mx*j ] = f[5 + mx*j ];
          f[1 + mx*j ] = f[4 + mx*j ];
          f[2 + mx*j ] = f[3 + mx*j ];
        }
      }
    }
    // east
    if(mpi->nrk[5] == -1) {
      for(jz = 0; jz < mz; jz++) {
        for(jy = 0; jy < my; jy++) {
          j = jy + my*jz;
          f[(nx+3) + mx*j ] = f[(nx+2) + mx*j ];
          f[(nx+4) + mx*j ] = f[(nx+1) + mx*j ];
          f[(nx+5) + mx*j ] = f[(nx  ) + mx*j ];
          f[(nx+6) + mx*j ] = f[(nx-1) + mx*j ];
        }
      }
    }
  }
}

/*
 * YSE: Add inflow condition for fl.
 */
static void
bcfl_inlet(type *flbase, type *fsbase, ptrdiff_t offset,
           int stencil_size, type ratio)
{
  int j;
  type frem;
  for (j = 1; j <= stencil_size; ++j) {
    frem = *(fsbase + j * offset);
    if (frem < 0.0) frem = 0.0;
    frem = 1.0 - frem;
    if (frem < 0.0) frem = 0.0;
    *(flbase + j * offset) = frem * ratio;
  }
}

/**
 * @brief Commonized liquid VOF boundary calculation
 * @param f          liquid VOF value array
 * @param fs_sum     Sum of solid VOF array
 * @param icompo     Inlet component data (NULL for solute_diff is used)
 * @param cond_array boundary data array.
 * @param jbx        X index for cond_array
 * @param jby        Y index for cond_array
 * @param nbx        X size of cond_array
 * @param nby        Y size of cond_array
 * @param stencil_size Number of stencil
 * @param offset     Array offset to next value on the direction of
 *                   perpendicular to boundary.
 * @param jx         X index on the boundary of velocities
 * @param jy         Y index on the boundary of velocities
 * @param jz         Z index on the boundary of velocities
 * @param mx         X size of velocities
 * @param my         Y size of velocities
 * @param mz         Z size of velocities
 *
 * The cell (jx, jy, jz) must be one of the most outer cell which is
 * on the boundary, **inside** the calculation domain. For example,
 * the cell at index `cdo->stm` will be lower side boundary and the
 * cell at index `cdo->nx - 1 + cdo->stm` will be upper side boundary.
 *
 * \p stencil_size must be `cdo->stm` or `cdo->stp - 1`, note that
 * subtract 1 from `cdo->stp` when calculating upper side boundary.
 *
 * \p offset is pointer offset to get next value on the boundary. If
 * calculating boundary is West (X-) boundary, offset will be -1. If
 * calculating boundary is East (X+) boundary, offset will be +1. So,
 * for South (Y-) boundary, offset will be `-cdo->mx`. For North (Y+)
 * boundary, offset will be `cdo->mx`. For Bottom (Z-) boundary,
 * offset will be `-cdo->mxy` and for Top (Z+) boundary, offset will
 * be `cdo->mxy`.
 *
 * For \p f, give base address of material \p icompo.
 *
 * If \p icompo is negative value, assumes solute diff model is
 * enabled.
 */
static void
bcfl_calc(type *f, type *fs_sum, component_data *icompo,
          fluid_boundary_data **cond_array,
          int jbx, int jby, int nbx, int nby, int stencil_size,
          ptrdiff_t offset, int jx, int jy, int jz, int mx, int my, int mz)
{
  fluid_boundary_data *fp;
  ptrdiff_t jj;
  type *base;

  fp = cond_array[jbx + jby * nbx];
  jj = calc_address(jx, jy, jz, mx, my, mz);

  if (fp->cond == INLET) {
    type ratio;
    type ratsum;
    int j;
    int ncomp;
    struct inlet_component_element *e;
    ratsum = 0.0;
    ratio = 0.0;
    ncomp = inlet_component_data_ncomp(fp->comps);
    if (!icompo) { /* solute diff model used */
      for (j = 0; j < ncomp; ++j) {
        e = inlet_component_data_get(fp->comps, j);
        if (component_phases_has_liquid(e->comp.d->phases)) {
          ratio += e->ratio.current_value;
        }
        ratsum += e->ratio.current_value;
      }
    } else {
      for (j = 0; j < ncomp; ++j) {
        e = inlet_component_data_get(fp->comps, j);
        if (e->comp.d == icompo) {
          ratio += e->ratio.current_value;
        }
        ratsum += e->ratio.current_value;
      }
    }
    if (ratio > 0.0 && ratsum > 0.0) {
      ratio = ratio / ratsum;
    } else {
      ratio = 0.0;
    }
    //jj = jj - offset;
    base = &f[jj];
    bcfl_inlet(base, &fs_sum[jj], offset, stencil_size + 1, ratio);
  } else {
    base = &f[jj];
    boundary_line_symmetry(base, offset, stencil_size);
  }
}
/* YSE: end */

static void
bcfl_calc_layer(type *f_layer, type* f, type *fs_sum, component_data *icompo,
          fluid_boundary_data **cond_array,
          int jbx, int jby, int nbx, int nby, int stencil_size,
          ptrdiff_t offset, int jx, int jy, int jz, int mx, int my, int mz, int ilayer)
{
  fluid_boundary_data *fp;
  ptrdiff_t jj;
  type *base;

  type EPS = 1e-10;

  fp = cond_array[jbx + jby * nbx];
  jj = calc_address(jx, jy, jz, mx, my, mz);

  if (fp->cond == INLET) {

    type ratio;
    type ratsum;
    int j;
    int ncomp;
    struct inlet_component_element *e;
    ratsum = 0.0;
    ratio = 0.0;
    ncomp = inlet_component_data_ncomp(fp->comps);

    for (j = 0; j < ncomp; ++j) {
      e = inlet_component_data_get(fp->comps, j);
      if (e->comp.d == icompo) {

      ratio += e->ratio.current_value;

      }
      ratsum += e->ratio.current_value;
    }

    if (ratio > 0.0 && ratsum > 0.0) {
      ratio = ratio / ratsum;
    } else {
      ratio = 0.0;
    }

    //　着目layerにおいて、走査面上のセルにそのlayerの気相が存在しなければ、それに接する面が仮に流入境界に接していても、流入用の気相VOFの設定しない。
    if(f_layer[jj]>1-EPS){
      //　しかし、それだと、計算開始タイミングなど、どのレイヤーの気相も流入境界面上に存在しない場合(f[jj]>1-EPS)に対応できないので、
      // その場合は、第０レイヤーの気相を流入することとする。
      if(ilayer!=0){
        return;
      }else if(ilayer==0){
        if(f[jj]<1-EPS) return;
      }
    } 

    jj = jj - offset;
    base = &f_layer[jj];
    bcfl_inlet(base, &fs_sum[jj], offset, stencil_size + 1, ratio);

  } else {

    // OUTFLOW is for every layer

    base = &f_layer[jj];
    boundary_line_symmetry(base, offset, stencil_size);

  }
}

//================================
//  inflow boundary (scalar values)
//--------------------------------
void bcf_wall_inflow(type *f, parameter *prm, variable *val)
{
  domain    *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;
  int icompo,i,j, jx,jy,jz,
      nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
      m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;
  type xc,zc;
  int nbx = cdo->nbx, nby = cdo->nby, nbz = cdo->nbz;
  int nbcompo = cdo->NBaseComponent;

  if(prm->flg->solute_diff == OFF){
    for(icompo = 0; icompo < cdo->NBaseComponent; icompo++)
    {
      component_data *icd;
      icd = component_data_find_by_comp_index(&prm->comps_data_head, icompo);

      // bottom
      if(mpi->nrk[0] == -1) {
#pragma omp parallel for collapse(2)
        for(jy = 0; jy < ny; jy++) {
          for(jx = 0; jx < nx; jx++) {
            int jox, joy, jbx, jby;
            jox = jx + cdo->stm;
            joy = jy + cdo->stm;
            jbx = jx + cdo->stmb;
            jby = jy + cdo->stmb;

            bcfl_calc(f + icompo * cdo->m, val->fs_sum, icd,
                      val->bnd_B.fl, jbx, jby, nbx, nby, cdo->stm,
                      -mxy, jox, joy, cdo->stm, mx, my, mz);
          }
        }
      }
      // top
      if(mpi->nrk[1] == -1) {
#pragma omp parallel for collapse(2)
        for(jy = 0; jy < ny; jy++) {
          for(jx = 0; jx < nx; jx++) {
            int jox, joy, jbx, jby;
            jox = jx + cdo->stm;
            joy = jy + cdo->stm;
            jbx = jx + cdo->stmb;
            jby = jy + cdo->stmb;

            bcfl_calc(f + icompo * cdo->m, val->fs_sum, icd,
                      val->bnd_T.fl, jbx, jby, nbx, nby, cdo->stp - 1,
                      mxy, jox, joy, cdo->stm + nz - 1, mx, my, mz);
          }
        }
      }
      // south
      if(mpi->nrk[2] == -1) {
#pragma omp parallel for collapse(2)
        for(jz = 0; jz < nz; jz++) {
          for(jx = 0; jx < nx; jx++) {
            int jox, joz, jbx, jbz;
            jox = jx + cdo->stm;
            joz = jz + cdo->stm;
            jbx = jx + cdo->stmb;
            jbz = jz + cdo->stmb;

            bcfl_calc(f + icompo * cdo->m, val->fs_sum, icd,
                      val->bnd_S.fl, jbx, jbz, nbx, nbz, cdo->stm,
                      -mx, jox, cdo->stm, joz, mx, my, mz);
          }
        }
      }
      // north
      if(mpi->nrk[3] == -1) {
#pragma omp parallel for collapse(2)
        for(jz = 0; jz < nz; jz++) {
          for(jx = 0; jx < nx; jx++) {
            int jox, joz, jbx, jbz;
            jox = jx + cdo->stm;
            joz = jz + cdo->stm;
            jbx = jx + cdo->stmb;
            jbz = jz + cdo->stmb;

            bcfl_calc(f + icompo * cdo->m, val->fs_sum, icd,
                      val->bnd_N.fl, jbx, jbz, nbx, nbz, cdo->stp - 1,
                      mx, jox, cdo->stm + ny - 1, joz, mx, my, mz);
          }
        }
      }
      // west
      if(mpi->nrk[4] == -1) {
#pragma omp parallel for collapse(2)
        for(jz = 0; jz < nz; jz++) {
          for(jy = 0; jy < ny; jy++) {
            int joy, joz, jby, jbz;
            joy = jy + cdo->stm;
            joz = jz + cdo->stm;
            jby = jy + cdo->stmb;
            jbz = jz + cdo->stmb;

            bcfl_calc(f + icompo * cdo->m, val->fs_sum, icd,
                      val->bnd_W.fl, jby, jbz, nby, nbz, cdo->stm,
                      -1, cdo->stm, joy, joz, mx, my, mz);
          }
        }
      }
      // east
      if(mpi->nrk[5] == -1) {
#pragma omp parallel for collapse(2)
        for(jz = 0; jz < nz; jz++) {
          for(jy = 0; jy < ny; jy++) {
            int joy, joz, jby, jbz;
            joy = jy + cdo->stm;
            joz = jz + cdo->stm;
            jby = jy + cdo->stmb;
            jbz = jz + cdo->stmb;

            bcfl_calc(f + icompo * cdo->m, val->fs_sum, icd,
                      val->bnd_E.fl, jby, jbz, nby, nbz, cdo->stp - 1,
                      1, cdo->stm + nx - 1, joy, joz, mx, my, mz);
          }
        }
      }
    }
  } else { //------- solute_diff == ON -----------------------------//
    if(mpi->nrk[0] == -1) {
#pragma omp parallel for collapse(2)
      for(jy = 0; jy < ny; jy++) {
        for(jx = 0; jx < nx; jx++) {
          int jox, joy, jbx, jby;
          jox = jx + cdo->stm;
          joy = jy + cdo->stm;
          jbx = jx + cdo->stmb;
          jby = jy + cdo->stmb;

          bcfl_calc(f, val->fs, NULL,
                    val->bnd_B.fl, jbx, jby, nbx, nby, cdo->stm,
                    -mxy, jox, joy, cdo->stm, mx, my, mz);
        }
      }
    }
    // top
    if(mpi->nrk[1] == -1) {
#pragma omp parallel for collapse(2)
      for(jy = 0; jy < ny; jy++) {
        for(jx = 0; jx < nx; jx++) {
          int jox, joy, jbx, jby;
          jox = jx + cdo->stm;
          joy = jy + cdo->stm;
          jbx = jx + cdo->stmb;
          jby = jy + cdo->stmb;

          bcfl_calc(f, val->fs, NULL,
                    val->bnd_T.fl, jbx, jby, nbx, nby, cdo->stp - 1,
                    mxy, jox, joy, cdo->stm + nz - 1, mx, my, mz);
        }
      }
    }
    // south
    if(mpi->nrk[2] == -1) {
#pragma omp parallel for collapse(2)
      for(jz = 0; jz < nz; jz++) {
        for(jx = 0; jx < nx; jx++) {
          int jox, joz, jbx, jbz;
          jox = jx + cdo->stm;
          joz = jz + cdo->stm;
          jbx = jx + cdo->stmb;
          jbz = jz + cdo->stmb;

          bcfl_calc(f, val->fs_sum, NULL,
                    val->bnd_S.fl, jbx, jbz, nbx, nbz, cdo->stm,
                    -mx, jox, cdo->stm, joz, mx, my, mz);
        }
      }
    }
    // north
    if(mpi->nrk[3] == -1) {
#pragma omp parallel for collapse(2)
      for(jz = 0; jz < nz; jz++) {
        for(jx = 0; jx < nx; jx++) {
          int jox, joz, jbx, jbz;
          jox = jx + cdo->stm;
          joz = jz + cdo->stm;
          jbx = jx + cdo->stmb;
          jbz = jz + cdo->stmb;

          bcfl_calc(f, val->fs, NULL,
                    val->bnd_N.fl, jbx, jbz, nbx, nbz, cdo->stp - 1,
                    mx, jox, cdo->stm + ny - 1, joz, mx, my, mz);
        }
      }
    }
    // west
    if(mpi->nrk[4] == -1) {
#pragma omp parallel for collapse(2)
      for(jz = 0; jz < nz; jz++) {
        for(jy = 0; jy < ny; jy++) {
          int joy, joz, jby, jbz;
          joy = jy + cdo->stm;
          joz = jz + cdo->stm;
          jby = jy + cdo->stmb;
          jbz = jz + cdo->stmb;

          bcfl_calc(f, val->fs, NULL,
                    val->bnd_W.fl, jby, jbz, nby, nbz, cdo->stm,
                    -1, cdo->stm, joy, joz, mx, my, mz);
        }
      }
    }
    // east
    if(mpi->nrk[5] == -1) {
#pragma omp parallel for collapse(2)
      for(jz = 0; jz < nz; jz++) {
        for(jy = 0; jy < ny; jy++) {
          int joy, joz, jby, jbz;
          joy = jy + cdo->stm;
          joz = jz + cdo->stm;
          jby = jy + cdo->stmb;
          jbz = jz + cdo->stmb;

          bcfl_calc(f, val->fs, NULL,
                    val->bnd_E.fl, jby, jbz, nby, nbz, cdo->stp - 1,
                    1, cdo->stm + nx - 1, joy, joz, mx, my, mz);
        }
      }
    }
  }
}

//================================
//  inflow boundary for multi_layer VOF (scalar values)
//--------------------------------
void bcf_wall_inflow_layer(type *f_layer, parameter *prm, variable *val)
{
  domain    *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;
  int icompo,i,j, jx,jy,jz,
      nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
      m=cdo->m, mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;
  type xc,zc;
  int nbx = cdo->nbx, nby = cdo->nby, nbz = cdo->nbz;
  int nbcompo = cdo->NBaseComponent;

  component_data *icd;
  icd = component_data_find_by_comp_index(&prm->comps_data_head, 0); 
  // the last argument is icompo and it's 0 here
  // this function is called only after multi-layer process where only icompo=0 has liquid phase, hence only icompo=0 should be updated.

  for(int ilayer = 0; ilayer < cdo->NumberOfLayer; ilayer++){

    // bottom
    if(mpi->nrk[0] == -1) {
#pragma omp parallel for collapse(2)
      for(jy = 0; jy < ny; jy++) {
        for(jx = 0; jx < nx; jx++) {
          int jox, joy, jbx, jby;
          jox = jx + cdo->stm;
          joy = jy + cdo->stm;
          jbx = jx + cdo->stmb;
          jby = jy + cdo->stmb;

          bcfl_calc_layer(f_layer + ilayer * cdo->m, val->fl, val->fs_sum, icd,
                    val->bnd_B.fl, jbx, jby, nbx, nby, cdo->stm,
                    -mxy, jox, joy, cdo->stm, mx, my, mz, ilayer);
        }
      }
    }
    // top
    if(mpi->nrk[1] == -1) {
#pragma omp parallel for collapse(2)
      for(jy = 0; jy < ny; jy++) {
        for(jx = 0; jx < nx; jx++) {
          int jox, joy, jbx, jby;
          jox = jx + cdo->stm;
          joy = jy + cdo->stm;
          jbx = jx + cdo->stmb;
          jby = jy + cdo->stmb;

          bcfl_calc_layer(f_layer + ilayer * cdo->m, val->fl, val->fs_sum, icd,
                    val->bnd_T.fl, jbx, jby, nbx, nby, cdo->stp - 1,
                    mxy, jox, joy, cdo->stm + nz - 1, mx, my, mz, ilayer);
        }
      }
    }
    // south
    if(mpi->nrk[2] == -1) {
#pragma omp parallel for collapse(2)
      for(jz = 0; jz < nz; jz++) {
        for(jx = 0; jx < nx; jx++) {
          int jox, joz, jbx, jbz;
          jox = jx + cdo->stm;
          joz = jz + cdo->stm;
          jbx = jx + cdo->stmb;
          jbz = jz + cdo->stmb;

          bcfl_calc_layer(f_layer + ilayer * cdo->m, val->fl, val->fs_sum, icd,
                    val->bnd_S.fl, jbx, jbz, nbx, nbz, cdo->stm,
                    -mx, jox, cdo->stm, joz, mx, my, mz, ilayer);
        }
      }
    }
    // north
    if(mpi->nrk[3] == -1) {
#pragma omp parallel for collapse(2)
      for(jz = 0; jz < nz; jz++) {
        for(jx = 0; jx < nx; jx++) {
          int jox, joz, jbx, jbz;
          jox = jx + cdo->stm;
          joz = jz + cdo->stm;
          jbx = jx + cdo->stmb;
          jbz = jz + cdo->stmb;

          bcfl_calc_layer(f_layer + ilayer * cdo->m, val->fl, val->fs_sum, icd,
                    val->bnd_N.fl, jbx, jbz, nbx, nbz, cdo->stp - 1,
                    mx, jox, cdo->stm + ny - 1, joz, mx, my, mz, ilayer);
        }
      }
    }
    // west
    if(mpi->nrk[4] == -1) {
#pragma omp parallel for collapse(2)
      for(jz = 0; jz < nz; jz++) {
        for(jy = 0; jy < ny; jy++) {
          int joy, joz, jby, jbz;
          joy = jy + cdo->stm;
          joz = jz + cdo->stm;
          jby = jy + cdo->stmb;
          jbz = jz + cdo->stmb;

          bcfl_calc_layer(f_layer + ilayer * cdo->m, val->fl, val->fs_sum, icd,
                    val->bnd_W.fl, jby, jbz, nby, nbz, cdo->stm,
                    -1, cdo->stm, joy, joz, mx, my, mz, ilayer);
        }
      }
    }
    // east
    if(mpi->nrk[5] == -1) {
#pragma omp parallel for collapse(2)
      for(jz = 0; jz < nz; jz++) {
        for(jy = 0; jy < ny; jy++) {
          int joy, joz, jby, jbz;
          joy = jy + cdo->stm;
          joz = jz + cdo->stm;
          jby = jy + cdo->stmb;
          jbz = jz + cdo->stmb;

          bcfl_calc_layer(f_layer + ilayer * cdo->m, val->fl, val->fs_sum, icd,
                    val->bnd_E.fl, jby, jbz, nby, nbz, cdo->stp - 1,
                    1, cdo->stm + nx - 1, joy, joz, mx, my, mz, ilayer);
        }
      }
    }
  }
}

/* YSE: Commonize each functions */
static void
bcu_wall_perpendicular(type *base, ptrdiff_t offset, int stencil_size)
{
  boundary_point_symmetry_perp(base, offset, stencil_size, 0.0);
}

static void
bcu_wall_parallel(type *base, ptrdiff_t offset, int stencil_size)
{
  boundary_point_symmetry(base, offset, stencil_size, 0.0);
}

static void
bcu_slip_perpendicular(type *base, ptrdiff_t offset, int stencil_size)
{
  boundary_point_symmetry_perp(base, offset, stencil_size, 0.0);
}

static void
bcu_slip_parallel(type *base, ptrdiff_t offset, int stencil_size)
{
  boundary_line_symmetry(base, offset, stencil_size);
}

static void
bcu_out_perpendicular(type *base, ptrdiff_t offset, int stencil_size)
{
  boundary_line_symmetry_perp(base, offset, stencil_size);
}

static void
bcu_out_parallel(type *base, ptrdiff_t offset, int stencil_size)
{
  boundary_line_symmetry(base, offset, stencil_size);
}

static void
bcu_inlet_perpendicular(type *base, ptrdiff_t offset, int stencil_size,
                        type v)
{
  boundary_point_symmetry_perp(base, offset, stencil_size, v);
}

static void
bcu_inlet_parallel(type *base, ptrdiff_t offset, int stencil_size,
                   type v)
{
  boundary_point_symmetry(base, offset, stencil_size, v);
}

static fluid_boundary_data *
bcu_staggared_cond_select(fluid_boundary_data *fp, fluid_boundary_data *fn)
{
  int condp;
  int condn;

  condp = -1;
  condn = -1;
  if (fp == fn) return fp;
  if (fp) condp = fp->cond;
  if (fn) condn = fn->cond;

  switch(condp) {
  case WALL:
    return fp;
  case SLIP:
    if (condn == WALL) return fn;
    return fp;
  default:
    switch(condn) {
    case WALL:
      return fn;
    case SLIP:
      return fn; /* cond p is not WALL here */
    case OUT:
    case INLET:
      if (condp == INLET) return fp;
      return fn;
    default:
      return NULL;
    }
  }
}

static type
bcu_get_vin(fluid_boundary_data *f, type *u, type *v, type *w, type *target)
{
  if (target == u) return f->inlet_vel_u.current_value;
  if (target == v) return f->inlet_vel_v.current_value;
  if (target == w) return f->inlet_vel_w.current_value;
  return 0.0;
}

/**
 * @brief Commonized velocity boundary calculation.
 * @param perp_val   Velocity variable which is perpendicular to boundary
 * @param para_val_x Velocity variable which is parallel to boundary along X axis in cond_array
 * @param para_vel_y Velocity variable which is parallel to boundary along Y axis in cond_array
 * @param u          Velocity U (used for inlet)
 * @param v          Velocity V (used for inlet)
 * @param w          Velocity W (used for inlet)
 * @param cond_array boundaray data array.
 * @param jbx        X index for cond_array
 * @param jby        Y index for cond_array
 * @param nbx        X size of cond_array
 * @param nby        Y size of cond_array
 * @param stmbx      X- side stencil size for cond_array
 * @param stmby      Y- side stencil size for cond_array
 * @param stmpx      X+ side stencil size for cond_array
 * @param stmpy      Y+ side stencil size for cond_array
 * @param stencil_size Number of stencil
 * @param offset     Array offset to next value on the direction of
 *                   perpendicular to boundary.
 * @param jx         X index on the boundary of velocities
 * @param jy         Y index on the boundary of velocities
 * @param jz         Z index on the boundary of velocities
 * @param mx         X size of velocities
 * @param my         Y size of velocities
 * @param mz         Z size of velocities
 *
 * (jbx == nbx) or (jby == nby) is allowed for calculation of velocity
 * of parallel to boundary.
 *
 * The cell (jx, jy, jz) must be one of the most outer cell which is
 * on the boundary, **inside** the calculation domain. For example,
 * the cell at index `cdo->stm` will be lower side boundary and the
 * cell at index `cdo->nx - 1 + cdo->stm` will be upper side boundary.
 *
 * \p stencil_size must be `cdo->stm` or `cdo->stp - 1`, note that
 * subtract 1 from `cdo->stp` when calculating upper side boundary.
 *
 * \p offset is pointer offset to get next value on the boundary. If
 * calculating boundary is West (X-) boundary, offset will be -1. If
 * calculating boundary is East (X+) boundary, offset will be +1. So,
 * for South (Y-) boundary, offset will be `-cdo->mx`. For North (Y+)
 * boundary, offset will be `cdo->mx`. For Bottom (Z-) boundary,
 * offset will be `-cdo->mxy` and for Top (Z+) boundary, offset will
 * be `cdo->mxy`.
 *
 * Array u, v and w are used for which variable is
 */
static void
bcu_calc(type *perp_val, type *para_val_x, type *para_val_y,
         type *u, type *v, type *w,
         fluid_boundary_data **cond_array,
         int jbx, int jby, int nbx, int nby,
         int stmbx, int stmby, int stpbx, int stpby,
         int stencil_size, ptrdiff_t offset, int jx, int jy, int jz,
         int mx, int my, int mz)
{
  ptrdiff_t jj;
  fluid_boundary_data *fp, *fw, *fs;
  type *base;
  type vin;
  int jbxm, jbym, nbxm, nbym;

  fp = NULL;
  fw = NULL;
  fs = NULL;

  jbxm = jbx - stmbx;
  jbym = jby - stmby;
  nbxm = nbx - stmbx - stmbx;
  nbym = nby - stmby - stmby;

  if (jbxm >= 0 && jbym >= 0 && jbxm < nbxm && jbym < nbym) {
    jj = jbx + jby * nbx;
    fp = cond_array[jj];
    fw = cond_array[jj - 1];
    fs = cond_array[jj - nbx];
  } else if (jbxm == nbxm) {
    fw = cond_array[(jbx - 1) + jby * nbx];
  } else if (jbym == nbym) {
    fs = cond_array[jbx + (jby - 1) * nbx];
  } else {
    return;
  }

  jj = calc_address(jx, jy, jz, mx, my, mz);

  if (fp) {
    base = &perp_val[jj];
    switch(fp->cond) {
    case WALL:
      bcu_wall_perpendicular(base, offset, stencil_size);
      break;
    case SLIP:
      bcu_slip_perpendicular(base, offset, stencil_size);
      break;
    case OUT:
      bcu_out_perpendicular(base, offset, stencil_size);
      break;
    case INLET:
      vin = bcu_get_vin(fp, u, v, w, perp_val);
      bcu_inlet_perpendicular(base, offset, stencil_size, vin);
      break;
    }
  }

  if (fp || fw) {
    fluid_boundary_data *fa;

    fa = bcu_staggared_cond_select(fp, fw);

    if (fa) {
      base = &para_val_x[jj];
      switch(fa->cond) {
      case WALL:
        bcu_wall_parallel(base, offset, stencil_size);
        break;
      case SLIP:
        bcu_slip_parallel(base, offset, stencil_size);
        break;
      case OUT:
        bcu_out_parallel(base, offset, stencil_size);
        break;
      case INLET:
        vin = bcu_get_vin(fa, u, v, w, para_val_x);
        bcu_inlet_parallel(base, offset, stencil_size, vin);
        break;
      }
    }
  }

  if (fp || fs) {
    fluid_boundary_data *fa;

    fa = bcu_staggared_cond_select(fp, fs);

    if (fa) {
      base = &para_val_y[jj];
      switch(fa->cond) {
      case WALL:
        bcu_wall_parallel(base, offset, stencil_size);
        break;
      case SLIP:
        bcu_slip_parallel(base, offset, stencil_size);
        break;
      case OUT:
        bcu_out_parallel(base, offset, stencil_size);
        break;
      case INLET:
        vin = bcu_get_vin(fa, u, v, w, para_val_y);
        bcu_inlet_parallel(base, offset, stencil_size, vin);
        break;
      }
    }
  }
}
/* YSE: end */

//================================
//  Wall & Slip & Out (vector)
//--------------------------------
void bcu_wall_slip_out(type *u, type *v, type *w, material *mtl, parameter *prm, variable *val)
{
  flags     *flg = prm->flg;
  domain    *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;
  int i,j, jx,jy,jz,
      nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
      mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;
  int nbx = cdo->nbx, nby = cdo->nby, nbz = cdo->nbz;

  // bottom [z-] (zm)
  if(mpi->nrk[0] == -1) {
#pragma omp parallel for collapse(2)
    for(jy = 0; jy <= ny; jy++) {
      for(jx = 0; jx <= nx; jx++) {
        int jox, joy, jbx, jby;
        jox = jx + cdo->stm;
        joy = jy + cdo->stm;
        jbx = jx + cdo->stmb;
        jby = jy + cdo->stmb;

        bcu_calc(w, u, v, u, v, w, val->bnd_B.fl, jbx, jby, nbx, nby,
                 cdo->stmb, cdo->stmb, cdo->stpb, cdo->stpb,
                 cdo->stm, -mxy, jox, joy, cdo->stm, mx, my, mz);
      }
    }
  }
  // top [z+] (zp)
  if(mpi->nrk[1] == -1) {
#pragma omp parallel for collapse(2)
    for(jy = 0; jy <= ny; jy++) {
      for(jx = 0; jx <= nx; jx++) {
        int jox, joy, jbx, jby;
        jox = jx + cdo->stm;
        joy = jy + cdo->stm;
        jbx = jx + cdo->stmb;
        jby = jy + cdo->stmb;

        bcu_calc(w, u, v, u, v, w, val->bnd_T.fl, jbx, jby, nbx, nby,
                 cdo->stmb, cdo->stmb, cdo->stpb, cdo->stpb,
                 cdo->stp - 1, mxy, jox, joy, cdo->stm + nz - 1, mx, my, mz);
      }
    }
  }

  // south [y-] (ym)
  if(mpi->nrk[2] == -1) {
#pragma omp parallel for collapse(2)
    for(jz = 0; jz <= nz; jz++) {
      for(jx = 0; jx <= nx; jx++) {
        int jox, joz, jbx, jbz;
        jox = jx + cdo->stm;
        joz = jz + cdo->stm;
        jbx = jx + cdo->stmb;
        jbz = jz + cdo->stmb;

        bcu_calc(v, u, w, u, v, w, val->bnd_S.fl, jbx, jbz, nbx, nbz,
                 cdo->stmb, cdo->stmb, cdo->stpb, cdo->stpb,
                 cdo->stm, -mx, jox, cdo->stm, joz, mx, my, mz);
      }
    }
  }
  // north [y+] (yp)
  if(mpi->nrk[3] == -1) {
#pragma omp parallel for collapse(2)
    for(jz = 0; jz <= nz; jz++) {
      for(jx = 0; jx <= nx; jx++) {
        int jox, joz, jbx, jbz;
        jox = jx + cdo->stm;
        joz = jz + cdo->stm;
        jbx = jx + cdo->stmb;
        jbz = jz + cdo->stmb;

        bcu_calc(v, u, w, u, v, w, val->bnd_N.fl, jbx, jbz, nbx, nbz,
                 cdo->stmb, cdo->stmb, cdo->stpb, cdo->stpb,
                 cdo->stp - 1, mx, jox, cdo->stm + ny - 1, joz, mx, my, mz);
      }
    }
  }
  // west [x-] (xm)
  if(mpi->nrk[4] == -1) {
#pragma omp parallel for collapse(2)
    for(jz = 0; jz <= nz; jz++) {
      for(jy = 0; jy <= ny; jy++) {
        int joy, joz, jby, jbz;
        joy = jy + cdo->stm;
        joz = jz + cdo->stm;
        jby = jy + cdo->stmb;
        jbz = jz + cdo->stmb;

        bcu_calc(u, v, w, u, v, w, val->bnd_W.fl, jby, jbz, nby, nbz,
                 cdo->stmb, cdo->stmb, cdo->stpb, cdo->stpb,
                 cdo->stm, -1, cdo->stm, joy, joz, mx, my, mz);
      }
    }
  }
  // east [x+] (xp)
  if(mpi->nrk[5] == -1) {
#pragma omp parallel for collapse(2)
    for(jz = 0; jz <= nz; jz++) {
      for(jy = 0; jy <= ny; jy++) {
        int joy, joz, jby, jbz;
        joy = jy + cdo->stm;
        joz = jz + cdo->stm;
        jby = jy + cdo->stmb;
        jbz = jz + cdo->stmb;

        bcu_calc(u, v, w, u, v, w, val->bnd_E.fl, jby, jbz, nby, nbz,
                 cdo->stmb, cdo->stmb, cdo->stpb, cdo->stpb,
                 cdo->stp - 1, 1, cdo->stm + nx - 1, joy, joz, mx, my, mz);
      }
    }
  }
}

/* YSE: Commonized temperature boundary calculation */
static void
bct_insulation(type *base, ptrdiff_t offset, int stencil_size)
{
  boundary_line_symmetry(base, offset, stencil_size);
}

static void
bct_isothermal(type *base, ptrdiff_t offset, int stencil_size, type temp)
{
  boundary_point_symmetry(base, offset, stencil_size, temp);
}

static void
bct_diffusion(type *base, ptrdiff_t offset, int stencil_size, type limit)
{
  int j;
  type d;
  type t;
  d = *base - *(base - offset);
  for (j = 1; j <= stencil_size; ++j) {
    t = *base + j * d;
    *(base + j * offset) = MAX2(t, limit);
  }
}

/**
 * @brief Commonized temperature boundary calculation.
 * @param f          Temperature value array
 * @param cond_array boundaray data array.
 * @param jbx        X index for cond_array
 * @param jby        Y index for cond_array
 * @param nbx        X size of cond_array
 * @param nby        Y size of cond_array
 * @param stencil_size Number of stencil
 * @param offset     Array offset to next value on the direction of
 *                   perpendicular to boundary.
 * @param jx         X index on the boundary of velocities
 * @param jy         Y index on the boundary of velocities
 * @param jz         Z index on the boundary of velocities
 * @param mx         X size of velocities
 * @param my         Y size of velocities
 * @param mz         Z size of velocities
 *
 * The cell (jx, jy, jz) must be one of the most outer cell which is
 * on the boundary, **inside** the calculation domain. For example,
 * the cell at index `cdo->stm` will be lower side boundary and the
 * cell at index `cdo->nx - 1 + cdo->stm` will be upper side boundary.
 *
 * \p stencil_size must be `cdo->stm` or `cdo->stp - 1`, note that
 * subtract 1 from `cdo->stp` when calculating upper side boundary.
 *
 * \p offset is pointer offset to get next value on the boundary. If
 * calculating boundary is West (X-) boundary, offset will be -1. If
 * calculating boundary is East (X+) boundary, offset will be +1. So,
 * for South (Y-) boundary, offset will be `-cdo->mx`. For North (Y+)
 * boundary, offset will be `cdo->mx`. For Bottom (Z-) boundary,
 * offset will be `-cdo->mxy` and for Top (Z+) boundary, offset will
 * be `cdo->mxy`.
 */
static void
bct_calc(type *f, thermal_boundary_data **cond_array,
         int jbx, int jby, int nbx, int nby, int stencil_size,
         ptrdiff_t offset, int jx, int jy, int jz, int mx, int my, int mz)
{
  thermal_boundary_data *fp;
  ptrdiff_t jj;
  type *base;

  fp = cond_array[jbx + jby * nbx];
  jj = calc_address(jx, jy, jz, mx, my, mz);

  base = &f[jj];
  switch(fp->cond) {
  case INSULATION:
    bct_insulation(base, offset, stencil_size);
    break;
  case ISOTHERMAL:
    bct_isothermal(base, offset, stencil_size, fp->temperature.current_value);
    break;
  case DIFFUSION:
    bct_diffusion(base, offset, stencil_size, fp->diffusion_limit);
    break;
  default:
    CSVUNREACHABLE();
    break;
  }
}
/* YSE: end */

//========================================
// Insulation & Isothermal & Diffusion (Temperature)
//----------------------------------------
void bct_Ins_Iso_Diff(type *f, variable *val, parameter *prm)
{
  flags       *flg = prm->flg;
  domain      *cdo = prm->cdo;
  mpi_param   *mpi = prm->mpi;
  phase_value *phv = prm->phv;
  int i,j, jx,jy,jz,
      nx=cdo->nx, ny=cdo->ny, nz=cdo->nz,
      mx=cdo->mx, my=cdo->my, mz=cdo->mz, mxy=mx*my;
  type dtdx, dx=cdo->dx, dy=cdo->dy, dz=cdo->dz;
  type xc,yc,zc;
  int nbx = cdo->nbx, nby = cdo->nby, nbz = cdo->nbz;

  // bottom [z-] (zm)
  if(mpi->nrk[0] == -1) {
#pragma omp parallel for collapse(2)
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        int jox, joy, jbx, jby;
        jox = jx + cdo->stm;
        joy = jy + cdo->stm;
        jbx = jx + cdo->stmb;
        jby = jy + cdo->stmb;

        bct_calc(f, val->bnd_B.th, jbx, jby, nbx, nby,
                 cdo->stm, -mxy, jox, joy, cdo->stm, mx, my, mz);
      }
    }
  }
  // top [z+] (zp)
  if(mpi->nrk[1] == -1) {
#pragma omp parallel for collapse(2)
    for(jy = 0; jy < ny; jy++) {
      for(jx = 0; jx < nx; jx++) {
        int jox, joy, jbx, jby;
        jox = jx + cdo->stm;
        joy = jy + cdo->stm;
        jbx = jx + cdo->stmb;
        jby = jy + cdo->stmb;

        bct_calc(f, val->bnd_T.th, jbx, jby, nbx, nby,
                 cdo->stp - 1, mxy, jox, joy, cdo->stm + nz - 1, mx, my, mz);
      }
    }
  }

  // south [y-] (ym)
  if(mpi->nrk[2] == -1) {
#pragma omp parallel for collapse(2)
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
        int jox, joz, jbx, jbz;
        jox = jx + cdo->stm;
        joz = jz + cdo->stm;
        jbx = jx + cdo->stmb;
        jbz = jz + cdo->stmb;

        bct_calc(f, val->bnd_S.th, jbx, jbz, nbx, nbz,
                 cdo->stm, -mx, jox, cdo->stm, joz, mx, my, mz);
      }
    }
  }
  // north [y+] (yp)
  if(mpi->nrk[3] == -1) {
#pragma omp parallel for collapse(2)
    for(jz = 0; jz < nz; jz++) {
      for(jx = 0; jx < nx; jx++) {
        int jox, joz, jbx, jbz;
        jox = jx + cdo->stm;
        joz = jz + cdo->stm;
        jbx = jx + cdo->stmb;
        jbz = jz + cdo->stmb;

        bct_calc(f, val->bnd_N.th, jbx, jbz, nbx, nbz,
                 cdo->stp - 1, mx, jox, cdo->stm + ny - 1, joz, mx, my, mz);
      }
    }
  }
  // west [x-] (xm)
  if(mpi->nrk[4] == -1) {
#pragma omp parallel for collapse(2)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        int joy, joz, jby, jbz;
        joy = jy + cdo->stm;
        joz = jz + cdo->stm;
        jby = jy + cdo->stmb;
        jbz = jz + cdo->stmb;

        bct_calc(f, val->bnd_W.th, jby, jbz, nby, nbz,
                 cdo->stm, -1, cdo->stm, joy, joz, mx, my, mz);
      }
    }
  }
  // east [x+] (xp)
  if(mpi->nrk[5] == -1) {
#pragma omp parallel for collapse(2)
    for(jz = 0; jz < nz; jz++) {
      for(jy = 0; jy < ny; jy++) {
        int joy, joz, jby, jbz;
        joy = jy + cdo->stm;
        joz = jz + cdo->stm;
        jby = jy + cdo->stmb;
        jbz = jz + cdo->stmb;

        bct_calc(f, val->bnd_E.th, jby, jbz, nby, nbz,
                 cdo->stp - 1, 1, cdo->stm + nx - 1, joy, joz, mx, my, mz);
      }
    }
  }
}
