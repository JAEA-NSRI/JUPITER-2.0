
#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "jupiter/os/os.h"

#include <cuda_runtime.h>
//#include <cuda_bf16.h>
//#include <cuda_fp16.h>
#include "cublas_v2.h"

#include "struct.h"
#include "func.h"
#include "cg.h"

#include "pcg_block3d.h"
#include "sleev_comm_block3d.h"
#include "pcg_cuda_BF16.h"
#include "pcg_block3d_cuda.h"
#include "sleev_comm_block3d_cuda.h"
#include "pcg_block3d_lowtype_cuda.h"
#include "pcg_block3d_precon_lowtype_cuda.h"
#include "pcg_block3d_matvec_cuda.h"

#if sMatrix
#else
@@@ " not support "
#endif

#define USE_KRYLOV_COMM_OVL 1
//#define USE_KRYLOV_COMM_OVL 0

#define USE_HOSTMEM 1
//#define USE_HOSTMEM 0
//#define USE_UNIFIEDMEM 1

#define CUDAHOSTALLOC_OPPTION cudaHostAllocDefault
//#define CUDAHOSTALLOC_OPPTION cudaHostAllocWriteCombined
//#define CUDAHOSTALLOC_OPPTION cudaHostAllocMapped

#define CUDAHOST_RBUFF_WriteCombined 0

extern "C" {

int pcg_block3d_call_cuda(mpi_prm prm,
		     int itrmax,
		     type rtolmax,type abstolmax,
		     type* x_xyz_cpu,type* b_xyz_cpu,type* A_xyz_cpu
		     ){
 
  int rank;
  cudaError_t cu_err;

#ifdef JUPITER_MPI
  MPI_Comm_rank(prm.comm, &rank);
#else
  rank=0;
#endif

#if 1
  int num_gpu;
  cudaGetDeviceCount(&num_gpu);
  const int selected_gpu = rank % num_gpu;
  cu_err = cudaSetDevice(selected_gpu);
  if (cu_err != cudaSuccess) {
    printf(" cudaSetDevice error \n");
    fprintf(stderr, "[Error] %s (error code: %d) at %s line %d\n", cudaGetErrorString(cu_err), cu_err, __FILE__, __LINE__); \
    //    return -1;
  }


  cudaDeviceSynchronize();
  //  printf(" rank  = %d ,num_gpu = %d selected_gpu = %d \n",rank,num_gpu,selected_gpu);
#endif

  int i,itr;
  double time[20];
  double time_comm[20];
  double time_precon[20];
  double stime,etime; 
  double ts,te,ts0; 
  for(i=0;i<20;i++){time[i]=(double)0.0;}
  for(i=0;i<20;i++){time_comm[i]=(double)0.0;}
  for(i=0;i<20;i++){time_precon[i]=(double)0.0;}
  cudaDeviceSynchronize();
  stime=cpu_time();
  ts=cpu_time();

  int m = prm.m;
  int mx =prm.mx;
  int my =prm.my;
  int mz =prm.mz;
  int stm  = prm.stm;
  int nx = prm.nx;
  int ny = prm.ny;
  int nz = prm.nz;

  // --------------

  set_subdividing_3Dblockjacobi(&prm);
  int nxdivblock = prm.nxdivblock;
  int nydivblock = prm.nydivblock;
  int nzdivblock = prm.nzdivblock;
  int block_nxs_cpu[nxdivblock];
  int block_nxe_cpu[nxdivblock];
  int block_nys_cpu[nydivblock];
  int block_nye_cpu[nydivblock];
  int block_nzs_cpu[nzdivblock];
  int block_nze_cpu[nzdivblock];

  int nzblock = prm.nzblock;
  int nyblock = prm.nyblock;
  int nxblock = prm.nxblock;

  int stride_zp_cpu[nzblock];
  int stride_zm_cpu[nzblock];
  int stride_yp_cpu[nyblock];
  int stride_ym_cpu[nyblock];
  int stride_xp_cpu[nxblock];
  int stride_xm_cpu[nxblock];

  set_subdividing_3Dblock_list(
			       prm,
			       block_nxs_cpu,block_nxe_cpu,
			       block_nys_cpu,block_nye_cpu,
			       block_nzs_cpu,block_nze_cpu,
			       stride_xp_cpu,stride_xm_cpu,
			       stride_yp_cpu,stride_ym_cpu,
			       stride_zp_cpu,stride_zm_cpu			       
			       );
  int block_m = prm.block_m;
  int block_nx = prm.block_nx;
  int block_ny = prm.block_ny;
  int block_nz = prm.block_nz;
  int block_nxy = block_nx * block_ny;
  int block_nxz = block_nx * block_nz;
  int block_nyz = block_ny * block_nz;

  type *x_cpu = (type*)malloc(sizeof(type)*(block_m));
  type *b_cpu = (type*)malloc(sizeof(type)*(block_m));
  zero_initialize(block_m,x_cpu);
  zero_initialize(block_m,b_cpu);
  XYZ_to_blockXYZ(
		  prm,
		  x_xyz_cpu, x_cpu,
		  block_nxs_cpu,  block_nxe_cpu, 
		  block_nys_cpu,  block_nye_cpu, 
		  block_nzs_cpu,  block_nze_cpu);
  XYZ_to_blockXYZ(
		  prm,
		  b_xyz_cpu, b_cpu,
		  block_nxs_cpu,  block_nxe_cpu, 
		  block_nys_cpu,  block_nye_cpu, 
		  block_nzs_cpu,  block_nze_cpu);

  type *A_cpu = (type*)malloc(sizeof(type)*(block_m*7));
  zero_initialize(block_m*7,A_cpu);
  for(int j=0;j<7;j++){
    XYZ_to_blockXYZ(
		    prm,
		    &A_xyz_cpu[m*j], &A_cpu[block_m*j],
		    block_nxs_cpu,  block_nxe_cpu, 
		    block_nys_cpu,  block_nye_cpu, 
		    block_nzs_cpu,  block_nze_cpu);
  }

#if sMatrix
  make_sMat_block3D(prm,A_cpu,
		    stride_xp_cpu,stride_xm_cpu,
		    stride_yp_cpu,stride_ym_cpu,
		    stride_zp_cpu,stride_zm_cpu 
		    );
#endif


#if 1
  type *block_Bottom_filter_cpu = (type*)malloc(sizeof(type)*(nzblock*nzdivblock));
  type *block_Top_filter_cpu    = (type*)malloc(sizeof(type)*(nzblock*nzdivblock));
  zero_initialize(nzblock*nzdivblock,block_Bottom_filter_cpu);
  zero_initialize(nzblock*nzdivblock,block_Top_filter_cpu);
  type *block_South_filter_cpu = (type*)malloc(sizeof(type)*(nyblock*nydivblock));
  type *block_North_filter_cpu = (type*)malloc(sizeof(type)*(nyblock*nydivblock));
  zero_initialize(nyblock*nydivblock,block_South_filter_cpu);
  zero_initialize(nyblock*nydivblock,block_North_filter_cpu);
  type *block_West_filter_cpu = (type*)malloc(sizeof(type)*(nxblock*nxdivblock));
  type *block_East_filter_cpu = (type*)malloc(sizeof(type)*(nxblock*nxdivblock));
  zero_initialize(nxblock*nxdivblock,block_West_filter_cpu);
  zero_initialize(nxblock*nxdivblock,block_East_filter_cpu);

  make_Matvec_ovl_flter(
			prm,
			block_nxs_cpu,block_nxe_cpu,
			block_nys_cpu,block_nye_cpu,
			block_nzs_cpu,block_nze_cpu,			
			block_West_filter_cpu,block_East_filter_cpu,
			block_South_filter_cpu,block_North_filter_cpu,
			block_Bottom_filter_cpu,block_Top_filter_cpu
			);
  

  type *block_Bottom_filter;
  type *block_Top_filter;
  cudaMalloc(&block_Bottom_filter ,sizeof(type)*nzblock*nzdivblock);
  cudaMalloc(&block_Top_filter ,sizeof(type)*nzblock*nzdivblock);
  cudaMemcpy( block_Bottom_filter ,block_Bottom_filter_cpu , sizeof(type)*nzblock*nzdivblock, cudaMemcpyHostToDevice);
  cudaMemcpy( block_Top_filter ,block_Top_filter_cpu , sizeof(type)*nzblock*nzdivblock, cudaMemcpyHostToDevice);

  type *block_South_filter;
  type *block_North_filter;
  cudaMalloc(&block_South_filter ,sizeof(type)*nyblock*nydivblock);
  cudaMalloc(&block_North_filter ,sizeof(type)*nyblock*nydivblock);
  cudaMemcpy( block_South_filter ,block_South_filter_cpu , sizeof(type)*nyblock*nydivblock, cudaMemcpyHostToDevice);
  cudaMemcpy( block_North_filter ,block_North_filter_cpu , sizeof(type)*nyblock*nydivblock, cudaMemcpyHostToDevice);

  type *block_West_filter;
  type *block_East_filter;
  cudaMalloc(&block_West_filter ,sizeof(type)*nxblock*nxdivblock);
  cudaMalloc(&block_East_filter ,sizeof(type)*nxblock*nxdivblock);
  cudaMemcpy( block_West_filter ,block_West_filter_cpu , sizeof(type)*nxblock*nxdivblock, cudaMemcpyHostToDevice);
  cudaMemcpy( block_East_filter ,block_East_filter_cpu , sizeof(type)*nxblock*nxdivblock, cudaMemcpyHostToDevice);
  
#endif
  
  type *block_zfilterL_cpu = (type*)malloc(sizeof(type)*(nzblock*nzdivblock));
  type *block_zfilterU_cpu = (type*)malloc(sizeof(type)*(nzblock*nzdivblock));
  zero_initialize(nzblock*nzdivblock,block_zfilterL_cpu);
  zero_initialize(nzblock*nzdivblock,block_zfilterU_cpu);
  type *block_yfilterL_cpu = (type*)malloc(sizeof(type)*(nyblock*nydivblock));
  type *block_yfilterU_cpu = (type*)malloc(sizeof(type)*(nyblock*nydivblock));
  zero_initialize(nyblock*nydivblock,block_yfilterL_cpu);
  zero_initialize(nyblock*nydivblock,block_yfilterU_cpu);
  type *block_xfilterL_cpu = (type*)malloc(sizeof(type)*(nxblock*nxdivblock));
  type *block_xfilterU_cpu = (type*)malloc(sizeof(type)*(nxblock*nxdivblock));
  zero_initialize(nxblock*nxdivblock,block_xfilterL_cpu);
  zero_initialize(nxblock*nxdivblock,block_xfilterU_cpu);

  const int nBuf_cpu = ( block_nx*block_ny + block_nx*block_nz + block_ny*block_nz )*2*2 + 20;
  type *cpu_type_buf = (type*)malloc(sizeof(type)*(nBuf_cpu));
  zero_initialize(nBuf_cpu,cpu_type_buf);

  type *host_sb_b ;
  type *host_rb_b ;
  type *host_sb_t ;
  type *host_rb_t ;

  type *host_sb_s ;
  type *host_rb_s ;
  type *host_sb_n ;
  type *host_rb_n ;

  type *host_sb_w ;
  type *host_rb_w ;
  type *host_sb_e ;
  type *host_rb_e ;

#if USE_HOSTMEM

#if WriteCombined  
  
  cudaHostAlloc(&host_sb_b,sizeof(type)*(block_nxy),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_sb_t,sizeof(type)*(block_nxy),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_sb_s,sizeof(type)*(block_nxz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_sb_n,sizeof(type)*(block_nxz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_sb_w,sizeof(type)*(block_nyz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_sb_e,sizeof(type)*(block_nyz),CUDAHOSTALLOC_OPPTION);

  cudaHostAlloc(&host_rb_w,sizeof(type)*(block_nyz),cudaHostAllocWriteCombined);
  cudaHostAlloc(&host_rb_b,sizeof(type)*(block_nxy),cudaHostAllocWriteCombined);
  cudaHostAlloc(&host_rb_t,sizeof(type)*(block_nxy),cudaHostAllocWriteCombined);
  cudaHostAlloc(&host_rb_s,sizeof(type)*(block_nxz),cudaHostAllocWriteCombined);
  cudaHostAlloc(&host_rb_n,sizeof(type)*(block_nxz),cudaHostAllocWriteCombined);
  cudaHostAlloc(&host_rb_e,sizeof(type)*(block_nyz),cudaHostAllocWriteCombined);
#else
  cudaHostAlloc(&host_sb_b,sizeof(type)*(block_nxy),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_rb_b,sizeof(type)*(block_nxy),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_sb_t,sizeof(type)*(block_nxy),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_rb_t,sizeof(type)*(block_nxy),CUDAHOSTALLOC_OPPTION);

  cudaHostAlloc(&host_sb_s,sizeof(type)*(block_nxz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_rb_s,sizeof(type)*(block_nxz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_sb_n,sizeof(type)*(block_nxz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_rb_n,sizeof(type)*(block_nxz),CUDAHOSTALLOC_OPPTION);

  cudaHostAlloc(&host_sb_w,sizeof(type)*(block_nyz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_rb_w,sizeof(type)*(block_nyz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_sb_e,sizeof(type)*(block_nyz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_rb_e,sizeof(type)*(block_nyz),CUDAHOSTALLOC_OPPTION);
#endif  

#else

#if USE_UNIFIEDMEM
  
  cudaMallocManaged((void **)&host_sb_b,sizeof(type)*(block_nxy));
  cudaMallocManaged((void **)&host_rb_b,sizeof(type)*(block_nxy));
  cudaMallocManaged((void **)&host_sb_t,sizeof(type)*(block_nxy));
  cudaMallocManaged((void **)&host_rb_t,sizeof(type)*(block_nxy));

  cudaMallocManaged((void **)&host_sb_s,sizeof(type)*(block_nxz));
  cudaMallocManaged((void **)&host_rb_s,sizeof(type)*(block_nxz));
  cudaMallocManaged((void **)&host_sb_n,sizeof(type)*(block_nxz));
  cudaMallocManaged((void **)&host_rb_n,sizeof(type)*(block_nxz));

  cudaMallocManaged((void **)&host_sb_w,sizeof(type)*(block_nyz));
  cudaMallocManaged((void **)&host_rb_w,sizeof(type)*(block_nyz));
  cudaMallocManaged((void **)&host_sb_e,sizeof(type)*(block_nyz));
  cudaMallocManaged((void **)&host_rb_e,sizeof(type)*(block_nyz));

#else  
  host_sb_b = (type*)malloc( sizeof(type)*(block_nxy) );
  host_rb_b = (type*)malloc( sizeof(type)*(block_nxy) );
  host_sb_t = (type*)malloc( sizeof(type)*(block_nxy) );
  host_rb_t = (type*)malloc( sizeof(type)*(block_nxy) );
  
  host_sb_s = (type*)malloc( sizeof(type)*(block_nxz) );
  host_rb_s = (type*)malloc( sizeof(type)*(block_nxz) );
  host_sb_n = (type*)malloc( sizeof(type)*(block_nxz) );
  host_rb_n = (type*)malloc( sizeof(type)*(block_nxz) );
  
  host_sb_w = (type*)malloc( sizeof(type)*(block_nyz) );
  host_rb_w = (type*)malloc( sizeof(type)*(block_nyz) );
  host_sb_e = (type*)malloc( sizeof(type)*(block_nyz) );
  host_rb_e = (type*)malloc( sizeof(type)*(block_nyz) );
#endif
  
#endif

  zero_initialize(block_nxy,host_sb_b);
  zero_initialize(block_nxy,host_rb_b);
  zero_initialize(block_nxy,host_sb_t);
  zero_initialize(block_nxy,host_rb_t);
  
  zero_initialize(block_nxz,host_sb_s);
  zero_initialize(block_nxz,host_rb_s);
  zero_initialize(block_nxz,host_sb_n);
  zero_initialize(block_nxz,host_rb_n);
  
  zero_initialize(block_nyz,host_sb_w);
  zero_initialize(block_nyz,host_rb_w);
  zero_initialize(block_nyz,host_sb_e);
  zero_initialize(block_nyz,host_rb_e);

  type *Dinv_cpu = (type*)malloc(sizeof(type)*(block_m));
  zero_initialize(block_m,Dinv_cpu);
  make_pre_subdividing_idiagMat1_block3D(
					 prm,
					 A_cpu,Dinv_cpu,
					 block_nxs_cpu, block_nxe_cpu, 
					 block_nys_cpu, block_nye_cpu, 
					 block_nzs_cpu, block_nze_cpu,
					 stride_xp_cpu, stride_xm_cpu,
					 stride_yp_cpu, stride_ym_cpu,
					 stride_zp_cpu, stride_zm_cpu,
					 block_xfilterL_cpu,block_xfilterU_cpu,
					 block_yfilterL_cpu,block_yfilterU_cpu,
					 block_zfilterL_cpu,block_zfilterU_cpu
					 );

  // cudaパラメータ
  const type zero = 0.0;
  const size_t VecSize = sizeof(type)*block_m;
  const size_t MatSize = sizeof(type)*block_m*7;

  const int cuda_threads    = 512;
  const int cuda_blocks_m   = ceil((double)(block_m)/cuda_threads);
  const int cuda_blocks_mat = ceil((double)(block_m*7)/cuda_threads);

  const int nBuf = ( block_nx*block_ny + block_nx*block_nz + block_ny*block_nz )*2*2 + block_m*2 + 20;
  const size_t BufSize = sizeof(type)*nBuf;
  const int cuda_blocks_buf = ceil((double)(nBuf)/cuda_threads);

  type *cuda_type_buf;
  cudaMalloc(&cuda_type_buf,BufSize);
  initialize_type_vec<<<cuda_blocks_buf,cuda_threads>>>( cuda_type_buf, nBuf);

  type *x;
#if 0
  cudaMalloc(&x   ,VecSize);
  initialize_type_vec<<<cuda_blocks_m,cuda_threads>>>(x,block_m);
  cudaMemcpy(x   ,x_cpu   , VecSize, cudaMemcpyHostToDevice);
#else
  cu_err = cudaMalloc(&x   ,VecSize);
  if (cu_err != cudaSuccess) {
    printf(" xvec cudaMalloc error \n");
    fprintf(stderr, "[Error] %s (error code: %d) at %s line %d\n", cudaGetErrorString(cu_err), cu_err, __FILE__, __LINE__); \
    return -1;
  }
  initialize_type_vec<<<cuda_blocks_m,cuda_threads>>>(x,block_m);
  cu_err = cudaMemcpy(x   ,x_cpu   , VecSize, cudaMemcpyHostToDevice);
  if (cu_err != cudaSuccess) {
    printf(" xvec cudaMemcpy error \n");
    fprintf(stderr, "[Error] %s (error code: %d) at %s line %d\n", cudaGetErrorString(cu_err), cu_err, __FILE__, __LINE__); \
    return -1;
  }
#endif

  // -------------------
  type *b;
#if 0
  cudaMalloc(&b   ,VecSize);
  initialize_type_vec<<<cuda_blocks_m,cuda_threads>>>(b,block_m);
  cudaMemcpy(b   ,b_cpu   , VecSize, cudaMemcpyHostToDevice);
#else
  cu_err = cudaMalloc(&b   ,VecSize);
  if (cu_err != cudaSuccess) {
    printf(" bvec cudaMalloc error \n");
    fprintf(stderr, "[Error] %s (error code: %d) at %s line %d\n", cudaGetErrorString(cu_err), cu_err, __FILE__, __LINE__); 
    return -1;
  }

  initialize_type_vec<<<cuda_blocks_m,cuda_threads>>>(b,block_m);

  cu_err = cudaMemcpy(b   ,b_cpu   , VecSize, cudaMemcpyHostToDevice);
  if (cu_err != cudaSuccess) {
    printf(" bvec cudaMemcpy error \n");
    fprintf(stderr, "[Error] %s (error code: %d) at %s line %d\n", cudaGetErrorString(cu_err), cu_err, __FILE__, __LINE__); 
    return -1;
  }
#endif

  // -------------------
  type *A;
#if 0
  cudaMalloc(&A   ,MatSize);
  initialize_type_vec<<<cuda_blocks_mat,cuda_threads>>>(A,block_m*7);
  cudaMemcpy(A   ,A_cpu   , MatSize, cudaMemcpyHostToDevice);
#else
  cu_err = cudaMalloc(&A   ,MatSize);
  if (cu_err != cudaSuccess) {
    printf(" Amat cudaMalloc error \n");
    fprintf(stderr, "[Error] %s (error code: %d) at %s line %d\n", cudaGetErrorString(cu_err), cu_err, __FILE__, __LINE__); \
    return -1;
  }
  initialize_type_vec<<<cuda_blocks_mat,cuda_threads>>>(A,block_m*7);
  cu_err = cudaMemcpy(A   ,A_cpu   , MatSize, cudaMemcpyHostToDevice);
  if (cu_err != cudaSuccess) {
    printf(" Amat cudaMemcpy error \n");
    fprintf(stderr, "[Error] %s (error code: %d) at %s line %d\n", cudaGetErrorString(cu_err), cu_err, __FILE__, __LINE__); 
    return -1;
  }
#endif

  type *Dinv;
  cudaMalloc(&Dinv,VecSize);
  initialize_type_vec<<<cuda_blocks_m,cuda_threads>>>(Dinv,block_m);
  cudaMemcpy(Dinv,Dinv_cpu, VecSize, cudaMemcpyHostToDevice);

  type *r;
  type *r_;
  type *s;
  type *p;
  type *q;
  type *z;

  cudaMalloc(&r   ,VecSize);
  cudaMalloc(&r_  ,VecSize);
  cudaMalloc(&s   ,VecSize);
  cudaMalloc(&p   ,VecSize);
  cudaMalloc(&q   ,VecSize);
  cudaMalloc(&z   ,VecSize);

  initialize_type_vec<<<cuda_blocks_m,cuda_threads>>>(r,block_m);
  initialize_type_vec<<<cuda_blocks_m,cuda_threads>>>(r_,block_m);
  initialize_type_vec<<<cuda_blocks_m,cuda_threads>>>(s,block_m);
  initialize_type_vec<<<cuda_blocks_m,cuda_threads>>>(p,block_m);
  initialize_type_vec<<<cuda_blocks_m,cuda_threads>>>(q,block_m);
  initialize_type_vec<<<cuda_blocks_m,cuda_threads>>>(z,block_m);

   int *block_nzs;
   int *block_nze;
   int *block_nys;
   int *block_nye;
   int *block_nxs;
   int *block_nxe;

  cudaMalloc(&block_nzs ,sizeof(int)*nzdivblock);
  cudaMalloc(&block_nze ,sizeof(int)*nzdivblock);
  cudaMemcpy( block_nzs ,block_nzs_cpu , sizeof(int)*nzdivblock, cudaMemcpyHostToDevice);
  cudaMemcpy( block_nze ,block_nze_cpu , sizeof(int)*nzdivblock, cudaMemcpyHostToDevice);
   
  cudaMalloc(&block_nys ,sizeof(int)*nydivblock);
  cudaMalloc(&block_nye ,sizeof(int)*nydivblock);
  cudaMemcpy( block_nys ,block_nys_cpu , sizeof(int)*nydivblock, cudaMemcpyHostToDevice);
  cudaMemcpy( block_nye ,block_nye_cpu , sizeof(int)*nydivblock, cudaMemcpyHostToDevice);

  cudaMalloc(&block_nxs ,sizeof(int)*nxdivblock);
  cudaMalloc(&block_nxe ,sizeof(int)*nxdivblock);
  cudaMemcpy( block_nxs ,block_nxs_cpu , sizeof(int)*nxdivblock, cudaMemcpyHostToDevice);
  cudaMemcpy( block_nxe ,block_nxe_cpu , sizeof(int)*nxdivblock, cudaMemcpyHostToDevice);
     
  int *stride_zp;
  int *stride_zm;
  cudaMalloc(&stride_zp ,sizeof(int)*nzblock);
  cudaMalloc(&stride_zm ,sizeof(int)*nzblock);
  cudaMemcpy( stride_zp ,stride_zp_cpu , sizeof(int)*nzblock, cudaMemcpyHostToDevice);
  cudaMemcpy( stride_zm ,stride_zm_cpu , sizeof(int)*nzblock, cudaMemcpyHostToDevice);

  int *stride_yp;
  int *stride_ym;
  cudaMalloc(&stride_yp ,sizeof(int)*nyblock);
  cudaMalloc(&stride_ym ,sizeof(int)*nyblock);
  cudaMemcpy( stride_yp ,stride_yp_cpu , sizeof(int)*nyblock, cudaMemcpyHostToDevice);
  cudaMemcpy( stride_ym ,stride_ym_cpu , sizeof(int)*nyblock, cudaMemcpyHostToDevice);

  int *stride_xp;
  int *stride_xm;
  cudaMalloc(&stride_xp ,sizeof(int)*nxblock);
  cudaMalloc(&stride_xm ,sizeof(int)*nxblock);
  cudaMemcpy( stride_xp ,stride_xp_cpu , sizeof(int)*nxblock, cudaMemcpyHostToDevice);
  cudaMemcpy( stride_xm ,stride_xm_cpu , sizeof(int)*nxblock, cudaMemcpyHostToDevice);

  type *block_zfilterL;
  type *block_zfilterU;
  cudaMalloc(&block_zfilterL ,sizeof(type)*nzblock*nzdivblock);
  cudaMalloc(&block_zfilterU ,sizeof(type)*nzblock*nzdivblock);
  cudaMemcpy( block_zfilterL ,block_zfilterL_cpu , sizeof(type)*nzblock*nzdivblock, cudaMemcpyHostToDevice);
  cudaMemcpy( block_zfilterU ,block_zfilterU_cpu , sizeof(type)*nzblock*nzdivblock, cudaMemcpyHostToDevice);

  type *block_yfilterL;
  type *block_yfilterU;
  cudaMalloc(&block_yfilterL ,sizeof(type)*nyblock*nydivblock);
  cudaMalloc(&block_yfilterU ,sizeof(type)*nyblock*nydivblock);
  cudaMemcpy( block_yfilterL ,block_yfilterL_cpu , sizeof(type)*nyblock*nydivblock, cudaMemcpyHostToDevice);
  cudaMemcpy( block_yfilterU ,block_yfilterU_cpu , sizeof(type)*nyblock*nydivblock, cudaMemcpyHostToDevice);

  type *block_xfilterL;
  type *block_xfilterU;
  cudaMalloc(&block_xfilterL ,sizeof(type)*nxblock*nxdivblock);
  cudaMalloc(&block_xfilterU ,sizeof(type)*nxblock*nxdivblock);
  cudaMemcpy( block_xfilterL ,block_xfilterL_cpu , sizeof(type)*nxblock*nxdivblock, cudaMemcpyHostToDevice);
  cudaMemcpy( block_xfilterU ,block_xfilterU_cpu , sizeof(type)*nxblock*nxdivblock, cudaMemcpyHostToDevice);

#if precon_low_type

  low_type *host_lowtype_sb_b ;
  low_type *host_lowtype_rb_b ;
  low_type *host_lowtype_sb_t ;
  low_type *host_lowtype_rb_t ;

  low_type *host_lowtype_sb_s ;
  low_type *host_lowtype_rb_s ;
  low_type *host_lowtype_sb_n ;
  low_type *host_lowtype_rb_n ;

  low_type *host_lowtype_sb_w ;
  low_type *host_lowtype_rb_w ;
  low_type *host_lowtype_sb_e ;
  low_type *host_lowtype_rb_e ;

#if USE_HOSTMEM

#if WriteCombined
  
  cudaHostAlloc(&host_lowtype_sb_b,sizeof(low_type)*(block_nxy),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_lowtype_sb_t,sizeof(low_type)*(block_nxy),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_lowtype_sb_s,sizeof(low_type)*(block_nxz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_lowtype_sb_n,sizeof(low_type)*(block_nxz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_lowtype_sb_w,sizeof(low_type)*(block_nyz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_lowtype_sb_e,sizeof(low_type)*(block_nyz),CUDAHOSTALLOC_OPPTION);

  cudaHostAlloc(&host_lowtype_rb_w,sizeof(low_type)*(block_nyz),cudaHostAllocWriteCombined);
  cudaHostAlloc(&host_lowtype_rb_b,sizeof(low_type)*(block_nxy),cudaHostAllocWriteCombined);
  cudaHostAlloc(&host_lowtype_rb_t,sizeof(low_type)*(block_nxy),cudaHostAllocWriteCombined);
  cudaHostAlloc(&host_lowtype_rb_s,sizeof(low_type)*(block_nxz),cudaHostAllocWriteCombined);
  cudaHostAlloc(&host_lowtype_rb_n,sizeof(low_type)*(block_nxz),cudaHostAllocWriteCombined);
  cudaHostAlloc(&host_lowtype_rb_e,sizeof(low_type)*(block_nyz),cudaHostAllocWriteCombined);
  
#else
  cudaHostAlloc(&host_lowtype_sb_b,sizeof(low_type)*(block_nxy),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_lowtype_rb_b,sizeof(low_type)*(block_nxy),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_lowtype_sb_t,sizeof(low_type)*(block_nxy),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_lowtype_rb_t,sizeof(low_type)*(block_nxy),CUDAHOSTALLOC_OPPTION);

  cudaHostAlloc(&host_lowtype_sb_s,sizeof(low_type)*(block_nxz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_lowtype_rb_s,sizeof(low_type)*(block_nxz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_lowtype_sb_n,sizeof(low_type)*(block_nxz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_lowtype_rb_n,sizeof(low_type)*(block_nxz),CUDAHOSTALLOC_OPPTION);

  cudaHostAlloc(&host_lowtype_sb_w,sizeof(low_type)*(block_nyz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_lowtype_rb_w,sizeof(low_type)*(block_nyz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_lowtype_sb_e,sizeof(low_type)*(block_nyz),CUDAHOSTALLOC_OPPTION);
  cudaHostAlloc(&host_lowtype_rb_e,sizeof(low_type)*(block_nyz),CUDAHOSTALLOC_OPPTION);
#endif  

#else

  host_lowtype_sb_b = (low_type*)malloc( sizeof(low_type)*(block_nxy) );
  host_lowtype_rb_b = (low_type*)malloc( sizeof(low_type)*(block_nxy) );
  host_lowtype_sb_t = (low_type*)malloc( sizeof(low_type)*(block_nxy) );
  host_lowtype_rb_t = (low_type*)malloc( sizeof(low_type)*(block_nxy) );
  
  host_lowtype_sb_s = (low_type*)malloc( sizeof(low_type)*(block_nxz) );
  host_lowtype_rb_s = (low_type*)malloc( sizeof(low_type)*(block_nxz) );
  host_lowtype_sb_n = (low_type*)malloc( sizeof(low_type)*(block_nxz) );
  host_lowtype_rb_n = (low_type*)malloc( sizeof(low_type)*(block_nxz) );
  
  host_lowtype_sb_w = (low_type*)malloc( sizeof(low_type)*(block_nyz) );
  host_lowtype_rb_w = (low_type*)malloc( sizeof(low_type)*(block_nyz) );
  host_lowtype_sb_e = (low_type*)malloc( sizeof(low_type)*(block_nyz) );
  host_lowtype_rb_e = (low_type*)malloc( sizeof(low_type)*(block_nyz) );
  
#endif
  
  zero_initialize_lowtype(block_nxy,host_lowtype_sb_b);
  zero_initialize_lowtype(block_nxy,host_lowtype_rb_b);
  zero_initialize_lowtype(block_nxy,host_lowtype_sb_t);
  zero_initialize_lowtype(block_nxy,host_lowtype_rb_t);
  
  zero_initialize_lowtype(block_nxz,host_lowtype_sb_s);
  zero_initialize_lowtype(block_nxz,host_lowtype_rb_s);
  zero_initialize_lowtype(block_nxz,host_lowtype_sb_n);
  zero_initialize_lowtype(block_nxz,host_lowtype_rb_n);
  
  zero_initialize_lowtype(block_nyz,host_lowtype_sb_w);
  zero_initialize_lowtype(block_nyz,host_lowtype_rb_w);
  zero_initialize_lowtype(block_nyz,host_lowtype_sb_e);
  zero_initialize_lowtype(block_nyz,host_lowtype_rb_e);
  
  low_type *cuda_lowtype_buf;
  cudaMalloc(&cuda_lowtype_buf, sizeof(low_type)*nBuf);
  initialize_low_type_vec<<<cuda_blocks_buf,cuda_threads>>>( cuda_lowtype_buf, nBuf);

  
#if precon_BF16
  printf("not support BF16 preconditioning \n");
  return -1;
#elif precon_FP32
  low_type *A_lowtype;
  cudaMalloc(&A_lowtype,sizeof(low_type)*block_m*7);
  initialize_low_type_vec<<<cuda_blocks_mat,cuda_threads>>>(A_lowtype,block_m*7);

  low_type *Dinv_lowtype;
  cudaMalloc(&Dinv_lowtype,sizeof(low_type)*block_m);
  initialize_low_type_vec<<<cuda_blocks_m,cuda_threads>>>(Dinv_lowtype,block_m);

  FP64toFP32<<<cuda_blocks_m,cuda_threads>>>(A   ,A_lowtype   ,block_m*7);
  FP64toFP32<<<cuda_blocks_m,cuda_threads>>>(Dinv,Dinv_lowtype,block_m  );
#else
  type *A_lowtype = A;
  type *Dinv_lowtype = Dinv;
#endif

#endif
  // CUDA Event
  const int num_cudaEvent = 20;
  cudaEvent_t cudaEvent_OVL[num_cudaEvent];
  for(int i = 0;i<num_cudaEvent;i++){
    cudaEventCreateWithFlags(&cudaEvent_OVL[i],cudaEventDisableTiming);
  }

  // CUDA Stream
  const int num_cudaStream = 20;
  cudaStream_t cudaStream_Hi_Priority[num_cudaStream];
  cudaStream_t cudaStream_Low_Priority[num_cudaStream];
  //  cudaStreamCreate(&cudaStream_Hi_Priority);
  int leastPriority;    // 低優先度ストリーム
  int greatestPriority; // 高優先度ストリーム
  cudaDeviceGetStreamPriorityRange(&leastPriority, &greatestPriority);
  // cudaStreamNonBlocking : デフォルトストリーミングと暗黙的な同期を取らないストリーミング
  // cudaStreamDefault
  for(int j=0;j<num_cudaStream;j++){
    
#if 1

#if 1
    cudaStreamCreateWithPriority(&cudaStream_Hi_Priority[j] ,cudaStreamNonBlocking,greatestPriority);
    cudaStreamCreateWithPriority(&cudaStream_Low_Priority[j],cudaStreamNonBlocking,leastPriority);
#else    
    cudaStreamCreateWithPriority(&cudaStream_Hi_Priority[j] ,cudaStreamNonBlocking,leastPriority);
    cudaStreamCreateWithPriority(&cudaStream_Low_Priority[j],cudaStreamNonBlocking,greatestPriority);
#endif
    
#else
    cudaStreamCreate(&cudaStream_Hi_Priority[j] );
    cudaStreamCreate(&cudaStream_Low_Priority[j]);
#endif    

  }
  
  // --------------------------------------------------------------------------------------------------
  // 共役勾配法開始
  type rhon[2];
  rhon[0]=(double)0.0;
  rhon[1]=(double)0.0;
  type tmp;
  type abstol,rtol,abstol0;
  type alpha,beta,gamma,rmax;

  type bmax;
  calc_norm_block3D_cuda(
			 prm,b,b,&bmax,cuda_type_buf,
			 block_nxs,  block_nxe, 
			 block_nys,  block_nye, 
			 block_nzs,  block_nze
			 );
  //  calc_norm_cuda(prm,b,b,&bmax,cuda_type_buf);
  //  printf(" bnorm gpu = %f \n",bmax);
#if 0
  calc_norm_block3D(prm,b_cpu,b_cpu,&bmax);
  printf(" bnorm cpu = %f \n",bmax);
#endif

#ifdef JUPITER_MPI

#if 1
    sleev_comm_hostmem_block3D_cuda(
			    x,&prm,
			    block_nxs,  block_nxe, 
			    block_nys,  block_nye, 
			    block_nzs,  block_nze,
			    cuda_type_buf,
			    host_sb_b,  host_rb_b,  host_sb_t,  host_rb_t,
			    host_sb_s,  host_rb_s,  host_sb_n,  host_rb_n,
			    host_sb_w,  host_rb_w,  host_sb_e,  host_rb_e,
			    cudaStream_Hi_Priority
				 );
  
#else  
  sleev_comm_block3D_cuda(x,&prm,
			  block_nxs,  block_nxe, 
			  block_nys,  block_nye, 
			  block_nzs,  block_nze,
			  cuda_type_buf,
			  cpu_type_buf);
#endif
  
#endif


  calc_sres_block3D_cuda(
			prm, A, x, b, r, cuda_type_buf,
			block_nxs,  block_nxe, 
			block_nys,  block_nye, 
			block_nzs,  block_nze,			
			stride_xp,stride_xm,
			stride_yp,stride_ym,
			stride_zp,stride_zm 
		   );

  alpha=(double)0.0;
  beta=(double)0.0;
  calc_norm_block3D_cuda(prm,r,r,&abstol,cuda_type_buf,
			 block_nxs,  block_nxe, 
			 block_nys,  block_nye, 
			 block_nzs,  block_nze
			 );

#if precon_low_type
  solve_pre_subdividing_mat0_local_block3D_lowtype_cuda(
					   prm, A_lowtype, Dinv_lowtype,
					   r, s,
					   cuda_type_buf,
					   cuda_lowtype_buf,
					     host_lowtype_sb_b,host_lowtype_rb_b,host_lowtype_sb_t,host_lowtype_rb_t,
					     host_lowtype_sb_s,host_lowtype_rb_s,host_lowtype_sb_n,host_lowtype_rb_n,
					     host_lowtype_sb_w,host_lowtype_rb_w,host_lowtype_sb_e,host_lowtype_rb_e,
					   block_nxs, block_nxe, 
					   block_nys, block_nye, 
					   block_nzs, block_nze,
					   stride_xp, stride_xm,
					   stride_yp, stride_ym,
					   stride_zp, stride_zm,
					   block_xfilterL,block_xfilterU,
					   block_yfilterL,block_yfilterU,
					   block_zfilterL,block_zfilterU,
					     block_West_filter,block_East_filter,
					     block_South_filter,block_North_filter,
					     block_Bottom_filter,block_Top_filter,
					   cudaStream_Low_Priority
					   );
#else
  solve_pre_subdividing_mat0_local_block3D_cuda(
					   prm, A, Dinv,
					   r, s,
					   block_nxs, block_nxe, 
					   block_nys, block_nye, 
					   block_nzs, block_nze,
					   stride_xp, stride_xm,
					   stride_yp, stride_ym,
					   stride_zp, stride_zm,
					   block_xfilterL,block_xfilterU,
					   block_yfilterL,block_yfilterU,
					   block_zfilterL,block_zfilterU,
					   cuda_type_buf,
					   host_sb_b,  host_rb_b,  host_sb_t,  host_rb_t,
					   host_sb_s,  host_rb_s,  host_sb_n,  host_rb_n,
					   host_sb_w,  host_rb_w,  host_sb_e,  host_rb_e,
					   cudaStream_Hi_Priority
					   );
#endif  
  calc_dot_block3D_cuda(prm,r,s,&rhon[0],cuda_type_buf,
			 block_nxs,  block_nxe, 
			 block_nys,  block_nye, 
			 block_nzs,  block_nze
			);
  rtol=abstol/bmax;

#ifdef RES_history
  if(rank ==0 ){
    printf("Iteration ,Absolute res ,Relative res \n");
    printf("@res@  %4d, %16.14e %16.14e \n",0,abstol,rtol);
  }
#endif

  cudaDeviceSynchronize();
  te=cpu_time();
  time[11]=time[11]+te-ts;

  for(itr=0;itr<itrmax;itr++){

#if 0
    printf("      alpha,beta = %f, %f \n",alpha,beta);
    printf("      rho0, rho1 = %f, %f \n",rhon[0],rhon[1]);
    printf("      gamma      = %f \n",gamma);
#endif

    //    return 0;
    //    axpy2(prm,alpha,beta,s,p,x);
#if USE_KRYLOV_COMM_OVL

    ts=cpu_time();

    axpy_sMatVec_dot_OVL_block3D_cuda(
				      &prm,
				      alpha, beta,
				      A,
				      s,p,x,q,
				      block_nxs, block_nxe, 
				      block_nys, block_nye, 
				      block_nzs, block_nze,
				      stride_xp,stride_xm,
				      stride_yp,stride_ym,
				      stride_zp,stride_zm,
				      block_West_filter,block_East_filter,
				      block_South_filter,block_North_filter,
				      block_Bottom_filter,block_Top_filter,
				      cuda_type_buf,
				      time_comm,
				      host_sb_b,  host_rb_b,  host_sb_t,  host_rb_t,
				      host_sb_s,  host_rb_s,  host_sb_n,  host_rb_n,
				      host_sb_w,  host_rb_w,  host_sb_e,  host_rb_e,
				      cudaStream_Hi_Priority ,
				      cudaStream_Low_Priority ,
				      cudaEvent_OVL
				      );
    
    cudaDeviceSynchronize();
    te=cpu_time();
    time[2]=time[2]+te-ts;

    ts=cpu_time();
#ifdef JUPITER_MPI
    cuda_MPI_Allreduce_sum(1,&gamma,cuda_type_buf,prm.comm);
#else
    cudaMemcpy(&gamma,cuda_type_buf,sizeof(type),cudaMemcpyDeviceToHost);
#endif
    cudaDeviceSynchronize();

    te=cpu_time();
    time[9]=time[9]+te-ts;

#else

    ts=cpu_time();
    axpy2_block3D_cuda(prm,alpha,beta,s,p,x,
			 block_nxs,  block_nxe, 
			 block_nys,  block_nye, 
			 block_nzs,  block_nze
		       );
    cudaDeviceSynchronize();    
    te=cpu_time();
    time[0]=time[0]+te-ts;

    ts=cpu_time();
#ifdef JUPITER_MPI
    sleev_comm_hostmem_block3D_cuda(
			    p,&prm,
			    block_nxs,  block_nxe, 
			    block_nys,  block_nye, 
			    block_nzs,  block_nze,
			    cuda_type_buf,
			    host_sb_b,  host_rb_b,  host_sb_t,  host_rb_t,
			    host_sb_s,  host_rb_s,  host_sb_n,  host_rb_n,
			    host_sb_w,  host_rb_w,  host_sb_e,  host_rb_e,
			    cudaStream_Hi_Priority
				 );
#endif
    cudaDeviceSynchronize();
    te=cpu_time();
    time[1]=time[1]+te-ts;
    
    ts0=cpu_time();
    
#if sMatrix
    sMatVec_dot_local_block3D_cuda(
				   prm, A, p, q, cuda_type_buf,
				   block_nxs,  block_nxe, 
				   block_nys,  block_nye, 
				   block_nzs,  block_nze,
				  stride_xp,stride_xm,
				  stride_yp,stride_ym,
				  stride_zp,stride_zm
			     );
    
#else
    MatVec_dot_local_block3D_cuda(
				  prm, A, p, q, cuda_type_buf,
				  stride_xp,stride_xm,
				  stride_yp,stride_ym,
				  stride_zp,stride_zm
			     );

#endif

    // -----------------
    cudaDeviceSynchronize();
    ts=cpu_time();
#ifdef JUPITER_MPI
    cuda_MPI_Allreduce_sum(1,&gamma,cuda_type_buf,prm.comm);
#else
    cudaMemcpy(&gamma,cuda_type_buf,sizeof(type),cudaMemcpyDeviceToHost);
#endif
    cudaDeviceSynchronize();

    te=cpu_time();
    time[9]=time[9]+te-ts;

    te=cpu_time();
    time[2]=time[2]+te-ts0;

#endif

    // -----------------
    // -----------------

    ts0=cpu_time();
    alpha=-rhon[0]/gamma;

#if precon_low_type

#if USE_KRYLOV_COMM_OVL_

#if 1
    // res + ILU Overlap
    solve_pre_subdividing_mat2_local_OVL2_block3D_lowtype_cuda(							      
					     prm, A_lowtype, Dinv_lowtype,
					     r,s,q,
					     alpha,
					     cuda_type_buf,
					     cuda_lowtype_buf,
					     host_lowtype_sb_b,host_lowtype_rb_b,host_lowtype_sb_t,host_lowtype_rb_t,
					     host_lowtype_sb_s,host_lowtype_rb_s,host_lowtype_sb_n,host_lowtype_rb_n,
					     host_lowtype_sb_w,host_lowtype_rb_w,host_lowtype_sb_e,host_lowtype_rb_e,
					     block_nxs, block_nxe, 
					     block_nys, block_nye, 
					     block_nzs, block_nze,
					     stride_xp, stride_xm,
					     stride_yp, stride_ym,
					     stride_zp, stride_zm,
					     block_xfilterL,block_xfilterU,
					     block_yfilterL,block_yfilterU,
					     block_zfilterL,block_zfilterU,
					     block_West_filter,block_East_filter,
					     block_South_filter,block_North_filter,
					     block_Bottom_filter,block_Top_filter,				   
					     cudaStream_Hi_Priority,
					     cudaStream_Low_Priority,
					     cudaEvent_OVL,
					     time_precon
					     );
#else
    // res overlap
    solve_pre_subdividing_mat2_local_OVL_block3D_lowtype_cuda(							      
					     prm, A_lowtype, Dinv_lowtype,
					     r,s,q,
					     alpha,
					     cuda_type_buf,
					     cuda_lowtype_buf,
					     host_lowtype_sb_b,host_lowtype_rb_b,host_lowtype_sb_t,host_lowtype_rb_t,
					     host_lowtype_sb_s,host_lowtype_rb_s,host_lowtype_sb_n,host_lowtype_rb_n,
					     host_lowtype_sb_w,host_lowtype_rb_w,host_lowtype_sb_e,host_lowtype_rb_e,
					     block_nxs, block_nxe, 
					     block_nys, block_nye, 
					     block_nzs, block_nze,
					     stride_xp, stride_xm,
					     stride_yp, stride_ym,
					     stride_zp, stride_zm,
					     block_xfilterL,block_xfilterU,
					     block_yfilterL,block_yfilterU,
					     block_zfilterL,block_zfilterU,
					     block_West_filter,block_East_filter,
					     block_South_filter,block_North_filter,
					     block_Bottom_filter,block_Top_filter,				   
					     cudaStream_Hi_Priority
					     );
#endif
    
#else
    solve_pre_subdividing_mat2_local_block3D_lowtype_cuda(
					     prm, A_lowtype, Dinv_lowtype,
					     r,s,q,
					     alpha,
					     cuda_type_buf,
					     cuda_lowtype_buf,
					     host_lowtype_sb_b,host_lowtype_rb_b,host_lowtype_sb_t,host_lowtype_rb_t,
					     host_lowtype_sb_s,host_lowtype_rb_s,host_lowtype_sb_n,host_lowtype_rb_n,
					     host_lowtype_sb_w,host_lowtype_rb_w,host_lowtype_sb_e,host_lowtype_rb_e,
					   block_nxs, block_nxe, 
					   block_nys, block_nye, 
					   block_nzs, block_nze,
					   stride_xp, stride_xm,
					   stride_yp, stride_ym,
					   stride_zp, stride_zm,
					   block_xfilterL,block_xfilterU,
					   block_yfilterL,block_yfilterU,
					   block_zfilterL,block_zfilterU,
					     block_West_filter,block_East_filter,
					     block_South_filter,block_North_filter,
					     block_Bottom_filter,block_Top_filter,
					   cudaStream_Low_Priority
					   );
#endif

#else
    solve_pre_subdividing_mat2_local_block3D_cuda(
					     prm, A, Dinv,
					     r, s,q,
					     z,				   
					     alpha,
					     cuda_type_buf,
					     host_sb_b,  host_rb_b,  host_sb_t,  host_rb_t,
					     host_sb_s,  host_rb_s,  host_sb_n,  host_rb_n,
					     host_sb_w,  host_rb_w,  host_sb_e,  host_rb_e,
					     block_nxs, block_nxe, 
					     block_nys, block_nye, 
					     block_nzs, block_nze,
					     stride_xp, stride_xm,
					     stride_yp, stride_ym,
					     stride_zp, stride_zm,
					     block_xfilterL,block_xfilterU,
					     block_yfilterL,block_yfilterU,
					     block_zfilterL,block_zfilterU,
					     cudaStream_Hi_Priority					     
					     );
#endif

    cudaDeviceSynchronize();
    ts=cpu_time(); 

#ifdef JUPITER_MPI
    type r_reduce[2];
    cuda_MPI_Allreduce_sum(2,r_reduce,cuda_type_buf,prm.comm);
    rhon[1] = r_reduce[0];
    abstol  = sqrt(fabs(r_reduce[1])); 
#else
    cudaMemcpy(&rhon[1],&cuda_buf[0],sizeof(type),cudaMemcpyDeviceToHost);
    cudaMemcpy(&abstol ,&cuda_buf[1],sizeof(type),cudaMemcpyDeviceToHost);
#endif


    cudaDeviceSynchronize();
    te=cpu_time();
    time[8]=time[8]+te-ts;

    //--------
    te=cpu_time();
    time[4]=time[4]+te-ts0;

    alpha=-alpha;
    beta=rhon[1]/rhon[0];
    rhon[0]=rhon[1];
    rtol=abstol/bmax;

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
  //  axpy(prm,alpha,p,x);
  // x = x + alpha*p
  ts=cpu_time();
  axpy_block3D_cuda(prm,p,x,alpha,1.0,
			 block_nxs,  block_nxe, 
			 block_nys,  block_nye, 
			 block_nzs,  block_nze
		    );
  cudaDeviceSynchronize();
  //  if( (rtol < rtolmax) || (abstol < abstolmax) ){
  //  rtol = 1.0;
  if( (rtol < rtolmax) || (abstol < abstolmax) ){
    //    if(rank ==0 ){
    //      printf("GPU Solver \n");
    //    }
    cudaMemcpy(x_cpu,x,VecSize,cudaMemcpyDeviceToHost);
    cudaDeviceSynchronize();
    blockXYZ_to_XYZ(
		    prm,
		    x_xyz_cpu, x_cpu,
		    block_nxs_cpu,  block_nxe_cpu, 
		    block_nys_cpu,  block_nye_cpu, 
		    block_nzs_cpu,  block_nze_cpu);
    cudaDeviceSynchronize();
  }else{
    if(rank ==0 ){
      printf("Recalculat with cpu solver \n");
    }
    zero_initialize(m,x_xyz_cpu);
    itr = pcg_call( prm,
		    itrmax,
		    rtolmax, abstolmax,
		    x_xyz_cpu, b_xyz_cpu, A_xyz_cpu
		    );
  }
  te=cpu_time();
  time[12]=time[12]+te-ts;

  // --------------
  // --------------

  free(x_cpu);
  free(b_cpu);
  free(A_cpu);

#if 1
  free(block_Bottom_filter_cpu);
  free(block_Top_filter_cpu);
  free(block_South_filter_cpu);
  free(block_North_filter_cpu);
  free(block_West_filter_cpu);
  free(block_East_filter_cpu);
  
  cudaFree(block_Bottom_filter);
  cudaFree(block_Top_filter);
  cudaFree(block_South_filter);
  cudaFree(block_North_filter);
  cudaFree(block_West_filter);
  cudaFree(block_East_filter);
#endif  
  
  free(block_zfilterL_cpu);
  free(block_zfilterU_cpu);
  free(block_yfilterL_cpu);
  free(block_yfilterU_cpu);
  free(block_xfilterL_cpu);
  free(block_xfilterU_cpu);
  free(cpu_type_buf);
  
#if USE_HOSTMEM
  cudaFreeHost(host_sb_b);
  cudaFreeHost(host_rb_b);
  cudaFreeHost(host_sb_t);
  cudaFreeHost(host_rb_t);
  cudaFreeHost(host_sb_s);
  cudaFreeHost(host_rb_s);
  cudaFreeHost(host_sb_n);
  cudaFreeHost(host_rb_n);
  cudaFreeHost(host_sb_w);
  cudaFreeHost(host_rb_w);
  cudaFreeHost(host_sb_e);
  cudaFreeHost(host_rb_e);
#else

#if USE_UNIFIEDMEM

  cudaFree(host_sb_b);
  cudaFree(host_rb_b);
  cudaFree(host_sb_t);
  cudaFree(host_rb_t);
  cudaFree(host_sb_s);
  cudaFree(host_rb_s);
  cudaFree(host_sb_n);
  cudaFree(host_rb_n);
  cudaFree(host_sb_w);
  cudaFree(host_rb_w);
  cudaFree(host_sb_e);
  cudaFree(host_rb_e);

#else  
  free(host_sb_b);
  free(host_rb_b);
  free(host_sb_t);
  free(host_rb_t);
  free(host_sb_s);
  free(host_rb_s);
  free(host_sb_n);
  free(host_rb_n);
  free(host_sb_w);
  free(host_rb_w);
  free(host_sb_e);
  free(host_rb_e);
#endif
  
#endif
  
  free(Dinv_cpu);

  cudaFree(cuda_type_buf);
  cudaFree(x);
  cudaFree(b);
  cudaFree(A);
  cudaFree(Dinv);
  cudaFree(r);
  cudaFree(r_);
  cudaFree(s);
  cudaFree(p);
  cudaFree(q);
  cudaFree(z);
  
  cudaFree(block_nzs);
  cudaFree(block_nze);
  cudaFree(block_nys);
  cudaFree(block_nye);
  cudaFree(block_nxs);
  cudaFree(block_nxe);
  cudaFree(stride_zp);
  cudaFree(stride_zm);
  cudaFree(stride_yp);
  cudaFree(stride_ym);
  cudaFree(stride_xp);
  cudaFree(stride_xm);
  
  cudaFree(block_zfilterL);
  cudaFree(block_zfilterU);
  cudaFree(block_yfilterL);
  cudaFree(block_yfilterU);
  cudaFree(block_xfilterL);
  cudaFree(block_xfilterU);

#if precon_low_type

#if USE_HOSTMEM
  cudaFreeHost(host_lowtype_sb_b);
  cudaFreeHost(host_lowtype_rb_b);
  cudaFreeHost(host_lowtype_sb_t);
  cudaFreeHost(host_lowtype_rb_t);
  cudaFreeHost(host_lowtype_sb_s);
  cudaFreeHost(host_lowtype_rb_s);
  cudaFreeHost(host_lowtype_sb_n);
  cudaFreeHost(host_lowtype_rb_n);
  cudaFreeHost(host_lowtype_sb_w);
  cudaFreeHost(host_lowtype_rb_w);
  cudaFreeHost(host_lowtype_sb_e);
  cudaFreeHost(host_lowtype_rb_e);
#else
  free(host_lowtype_sb_b);
  free(host_lowtype_rb_b);
  free(host_lowtype_sb_t);
  free(host_lowtype_rb_t);
  free(host_lowtype_sb_s);
  free(host_lowtype_rb_s);
  free(host_lowtype_sb_n);
  free(host_lowtype_rb_n);
  free(host_lowtype_sb_w);
  free(host_lowtype_rb_w);
  free(host_lowtype_sb_e);
  free(host_lowtype_rb_e);
#endif
  cudaFree(cuda_lowtype_buf);
  
#if precon_BF16

#elif precon_FP32
  cudaFree(A_lowtype);
  cudaFree(Dinv_lowtype);
#else

#endif

#endif
  for(int j=0;j<num_cudaStream;j++){
    cudaStreamDestroy(cudaStream_Hi_Priority[j]);
    cudaStreamDestroy(cudaStream_Low_Priority[j]);
  }
  for(int i = 0;i<num_cudaEvent;i++){
    cudaEventDestroy(cudaEvent_OVL[i]);
  }


  etime=cpu_time();

#if 0
  if(rank==0 ){
    printf(" -- rank=0 time -- \n");
    printf("solve = %f [s] \n", etime-stime);
    printf("    initialize              = %f [s] \n",time[11]);
#if USE_KRYLOV_COMM_OVL
    printf("    axpy2_comm_MatVec    = %f [s] \n",time[2]);
    printf("       axpy_sleev           = %f [s] \n",time_comm[0]);
    printf("       CPU<--GPU copy X     = %f [s] \n",time_comm[1]);
    printf("       commX                = %f [s] \n",time_comm[2]);
    printf("       CPU<--GPU copy Y     = %f [s] \n",time_comm[3]);
    printf("       commY                = %f [s] \n",time_comm[4]);
    printf("       CPU<--GPU copy Z     = %f [s] \n",time_comm[5]);
    printf("       commZ                = %f [s] \n",time_comm[6]);
    printf("       CPU-->GPU copy Y     = %f [s] \n",time_comm[7]);
    printf("       CPU-->GPU copy Z     = %f [s] \n",time_comm[8]);
    printf("       wait time            = %f [s] \n",time_comm[9]);
    
    printf("    Matvec_All_reduce    = %f [s] \n",time[9]);
    
#else    
    printf("    axpy2                   = %f [s] \n",time[0]);
    printf("    sleev_comm              = %f [s] \n",time[1]);
    printf("       setTB                = %f [s] \n",time_comm[0]);
    printf("       setNS                = %f [s] \n",time_comm[1]);
    printf("       setWE                = %f [s] \n",time_comm[2]);
    printf("       DtoH memcp           = %f [s] \n",time_comm[3]);
    printf("       MPI comm             = %f [s] \n",time_comm[4]);
    printf("       HtoD memcp           = %f [s] \n",time_comm[5]);
    printf("       rbTB                 = %f [s] \n",time_comm[6]);
    printf("       rbNS                 = %f [s] \n",time_comm[7]);
    printf("       rbWE                 = %f [s] \n",time_comm[8]);    
    printf("    MatVec                  = %f [s] \n",time[2]);
    printf("       Matvec_All_reduce    = %f [s] \n",time[9]);
#endif        
    printf("    solve_pre               = %f [s] \n",time[4]);
#if USE_KRYLOV_COMM_OVL
    printf("       ILU_1                = %f [s] \n",time_precon[0]);
    printf("       RES - ILU_2          = %f [s] \n",time_precon[1]);
    printf("           setW             = %f [s] \n",time_precon[2]);
    printf("           commW            = %f [s] \n",time_precon[11]);
    printf("           setE             = %f [s] \n",time_precon[3]);
    printf("           commE            = %f [s] \n",time_precon[12]);    
    printf("           setS             = %f [s] \n",time_precon[4]);
    printf("           setN             = %f [s] \n",time_precon[5]);
    printf("           commS            = %f [s] \n",time_precon[13]);    
    printf("           commN            = %f [s] \n",time_precon[14]);    
    printf("           setB             = %f [s] \n",time_precon[6]);
    printf("           setT             = %f [s] \n",time_precon[7]);
    printf("           commB            = %f [s] \n",time_precon[15]);    
    printf("           commT            = %f [s] \n",time_precon[16]);        
    printf("           readbackSNBT     = %f [s] \n",time_precon[8]);
    printf("           wait time        = %f [s] \n",time_precon[9]);
#endif
    printf("       solve_pre_All_reduce = %f [s] \n",time[8]);
    printf("    Postprocessing          = %f [s] \n",time[12]);
  }
#endif

#if 0
  if(rank ==0 ){
    printf(" \n");
  }
  pcg_call( prm,
	    10,
	    1.0e-50,1.0e-50,
	    //	    itrmax,
	    //	     rtolmax, abstolmax,
	    x_xyz_cpu, b_xyz_cpu, A_xyz_cpu
	    );
#endif

  return itr ;

}

}
