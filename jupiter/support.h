#ifndef HEADER_SUPPORT
#define HEADER_SUPPORT

#include "common.h"

JUPITER_DECL
int Sep_region(int* imyrank_,int* inprocess_,int* idown_,int* iup_,int* idown_my_,int* iup_my_);
JUPITER_DECL
int set_Head_Tail(int* downmax_,int* upmax_,int* mydown_,int* myup_,int* myHead_,int* myTail_);
JUPITER_DECL
int convert(int topo_flag,parameter *prm,mpi_prm *mpi_para);
JUPITER_DECL
int set_subdividing_3Dblockjacobi(mpi_prm *prm);
JUPITER_DECL
int set_subdividing_blockjacobi(mpi_prm *prm);
JUPITER_DECL
int set_subdividing_3Dblock_list(
				 mpi_prm prm,
				 int *block_nxs,int *block_nxe,
				 int *block_nys,int *block_nye,
				 int *block_nzs,int *block_nze,
				 int *stride_xp,int *stride_xm,
				 int *stride_yp,int *stride_ym,
				 int *stride_zp,int *stride_zm 
				 );


#endif
