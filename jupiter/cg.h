// #ifdef __cplusplus
// extern "C" {
// #endif
#ifndef HEADER_CG
#define HEADER_CG

#define sMatrix 1 

//#define precon_low_type 0
#define precon_low_type 1

#define precon_BF16 0
#define precon_FP32 1

#if precon_BF16
#define low_type __nv_bfloat16
#define MPI_LOW_TYPE MPI_SHORT
// #define MPI_LOW_TYPE MPI_HALF
#elif precon_FP32
#define low_type float
#define MPI_LOW_TYPE MPI_FLOAT
#else
#define low_type double
#define MPI_LOW_TYPE MPI_DOUBLE
#define precon_FP64 1
#endif

/*
#define SUBDIVIDING_XBLOCKSIZE 2
#define SUBDIVIDING_YBLOCKSIZE 2
#define SUBDIVIDING_ZBLOCKSIZE 5
*/

/*
#define SUBDIVIDING_XBLOCKSIZE 8
#define SUBDIVIDING_YBLOCKSIZE 8
#define SUBDIVIDING_ZBLOCKSIZE 30
*/

#ifndef JUPITER_SOLVER_CUDA_NXBLOCK
#define SUBDIVIDING_XBLOCKSIZE 4
#else
#define SUBDIVIDING_XBLOCKSIZE JUPITER_SOLVER_CUDA_NXBLOCK
#endif

#ifndef JUPITER_SOLVER_CUDA_NYBLOCK
#define SUBDIVIDING_YBLOCKSIZE 4
#else
#define SUBDIVIDING_YBLOCKSIZE JUPITER_SOLVER_CUDA_NYBLOCK
#endif

#ifndef JUPITER_SOLVER_CUDA_NZBLOCK
#define SUBDIVIDING_ZBLOCKSIZE 4
#else
#define SUBDIVIDING_ZBLOCKSIZE JUPITER_SOLVER_CUDA_NZBLOCK
#endif

// ----------------------------------

#ifdef JUPITER_MPI
#include <mpi.h>
#endif

#ifdef _OPENMP
#include <omp.h>
#endif

#if defined(__cplusplus) ||  defined(__NVCC__)
// #if defined(__cplusplus) 
extern "C" {
#endif

  //#if JUPITER_KERNEL
  //#include "kernel.h"
  //#endif
  
#ifdef MG_DOUBLE
  typedef double mgtype;
  #ifdef JUPITER_MPI
    #define MPI_MGTYPE MPI_DOUBLE
  #endif
#else
  typedef float mgtype;
  #ifdef JUPITER_MPI
    #define MPI_MGTYPE MPI_FLOAT
  #endif
#endif

typedef struct{
  int smax;
  int order;

  int npe;
  int rank;
  int rank_x,rank_y,rank_z;
  int npe_x,npe_y,npe_z; // 分割数

  int nxl,nyl,nzl; 
  int nxs,nxe;
  int nys,nye;
  int nzs,nze;

  int m,n; 
  int nx,ny,nz;
  int mx,my,mz;
  int mxy;
  int stm,stp;

#if 0
  int64_t gm,gn;
  int64_t gnx,gny,gnz;
  int64_t gmx,gmy,gmz;
#else
  int gm,gn;
  int gnx,gny,gnz;
  int gmx,gmy,gmz;
#endif
  int gnxs,gnxe;
  int gnys,gnye;
  int gnzs,gnze;

  //  int ptr[6];
  //  int Nbound[6];
  int nrk[6];
  //  int nrks[6];
  //  int nrkr[6];

  // 2D block
  int zdivblock;
  int ydivblock;
  int xdivblock;

  // 3D block 
  int nzblock;
  int nyblock;
  int nxblock;

  int mzdivblock;
  int mydivblock;
  int mxdivblock;

  int nzdivblock;
  int nydivblock;
  int nxdivblock;

  int nxdiv;

  int block_m;
  int block_mx;
  int block_my;
  int block_mz;
  int block_nx;
  int block_ny;
  int block_nz;

  // ----------
  // 3次元多ブロック構造のサイズ
  // 袖領域についても小ブロックのサイズを持つ

#ifdef JUPITER_MPI
  MPI_Request req_send[6];
  MPI_Request req_recv[6];
  MPI_Status stat_send[6];
  MPI_Status stat_recv[6];
  MPI_Comm comm;
  MPI_Fint comm_f;
#else
  int comm_f;
#endif

}mpi_prm;

typedef struct{
  // グローバル座標の初期位置
  int nx0,ny0,nz0;

  mgtype *A;
  mgtype *Dinv;
  mgtype *x;
  mgtype *r;
  mgtype *b;
  mgtype *d;
  int itrmax;
  mgtype Emax;
  mgtype Emin;

  mpi_prm prm;

  mgtype time[20];

#ifdef JUPITER_MPI
  MPI_Request req_send_init[6];
  MPI_Request req_recv_init[6];
#endif

  int preFlag;

  // -------
  // 低精度演算に利用する作業配列
  double *A_low;
  double *s_low;
  double *s2_low;
  float *r_fp32;
  // -------

  mgtype* sb_b;
  mgtype* rb_b;
  mgtype* sb_t;
  mgtype* rb_t;
  mgtype* sb_s;
  mgtype* rb_s;
  mgtype* sb_n;
  mgtype* rb_n;
  mgtype* sb_w;
  mgtype* rb_w;
  mgtype* sb_e;
  mgtype* rb_e;
}mGrid;


#include "krylov_call.h"
#include "sleev_comm.h"
#include "pcg.h"
#include "pbicg.h"
#if JUPITER_KERNEL  
#include "pcbcg.h"
#include "pcacg.h"
#include "GMG.h"
#include "cagmres.h"
#include "calanczos.h"
// GMG_conifg.cの関数はカーネルと実コードでルーチンを切り替える必要がある
#include "GMG_config.h"
#include "restriction.h"
#include "prolongation.h"
#include "smoother_preChebyshev_iteration.h"
#include "Relaxation.h"
#include "sleev_comm_MG.h"
#include "GMG_Eigen.h"
#include "GMG_float.h"
#include "precon.h"
#include "precon_flat.h"
#include "preconYY.h"
#include "tsqr.h"
#endif
  
// ----- 
// support.cの関数はカーネルと実コードで異なる convert など
#include "support.h"
// ------

// -----
// c言語でfp16を利用するなら利用
/* #if 0 */
/* typedef float pre_datatype; */
/* typedef float pretype; */
/* #else */
/* typedef __fp16 pre_datatype; */

/* #if 0 */
/* typedef _Float16 pretype; */
/* #else */
/* typedef __fp16 pretype; */
/* #endif */

/* #endif */

#define CAGMRES_Q_calc 0
#define BlockJacobi_Preconditioner 0
#define FP_BlockJacobi_Preconditioner -1
#define BlockJacobiIR_Preconditioner 10
#define FP_BlockJacobiIR_Preconditioner 11

#define BlockJacobiFlat_Preconditioner 22

#define EIGENVALUE_Algorithm_Lanczos 1
#define Eigenvalue_Algorithm_Power 2
#define Eigenvalue_Algorithm_CALanczos 3

#ifdef CAGMRES
#define Newton_Basis 0
#else
#define Newton_Basis 1
#endif
// -----
#endif

// #ifdef __cplusplus
// }
// #endif

#if defined(__cplusplus) ||  defined(__NVCC__)
}
#endif
