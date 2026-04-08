#include <stdio.h>
#include <stdlib.h>
#ifdef JUPITER_MPI
#include "mpi.h"
#endif
#include "struct.h"
#include "cg.h"

int Sep_region(int* imyrank_,int* inprocess_,int* idown_,int* iup_,int* idown_my_,int* iup_my_){
  int imyrank,inprocess,idown,iup,idown_my,iup_my;
  int iregion,iblock,over_region;

  imyrank   = imyrank_[0];
  inprocess = inprocess_[0];
  idown     = idown_[0];
  iup       = iup_[0];
  idown_my  = -99;
  iup_my    = -99;

  iregion=iup-idown+1;
  iblock=(iregion)/inprocess;
  over_region=iregion%inprocess;

  if(iblock==0){
    printf("error process is over much \n");
    return -1;
  }

  if(imyrank < over_region){
    idown_my=idown+iblock*imyrank+imyrank;
    iup_my=idown_my+iblock;
  }
  else{
    idown_my=idown+iblock*imyrank+over_region;
    iup_my=idown_my+iblock-1;
  }

  if((iup_my-idown_my+1)<=1){
    printf("error process is over much \n");
    return -1;
  }

  idown_my_[0]=idown_my;
  iup_my_[0]=iup_my;
  return 0;
}

int set_Head_Tail(int* downmax_,int* upmax_,int* mydown_,int* myup_,int* myHead_,int* myTail_){
  //      upmax : :計算領域の上限
  //      downmax :: 計算領域の下限
  //      myup :: 分割領域の上限
  //      mydown :: 分割領域の下限
  //      myHead :: 計算範囲の下限(出力)
  //      myTail :: 計算範囲の上限(出力)

  int upmax,downmax,myup,mydown,myHead,myTail;

  downmax = downmax_[0];
  upmax   = upmax_[0];
  mydown  = mydown_[0];
  myup    = myup_[0];
  myHead  = myHead_[0];
  myTail  = myTail_[0];

  downmax_[0] = downmax;
  upmax_[0]   = upmax;
  mydown_[0]  = mydown;
  myup_[0]    = myup;
  myHead_[0]  = myHead;
  myTail_[0]  = myTail;

  if(mydown>downmax){
    myHead=mydown;
  }
  else{
    myHead=downmax;
  }
  if(myup<upmax){
    myTail=myup;
  }
  else{
    myTail=upmax;
  }

  // 領域外
  if(mydown>upmax){
    myHead=999;
    myTail=-999;
  }
  if(myup<downmax){
    myHead=999;
    myTail=-999;
  }
  return 0;
}

int convert(int topo_flag,parameter *prm,mpi_prm *mpi_para){

  domain    *cdo = prm->cdo;
  mpi_param *mpi = prm->mpi;

  mpi_para[0].n = cdo->n;
  mpi_para[0].nx = cdo->nx;
  mpi_para[0].ny = cdo->ny;
  mpi_para[0].nz = cdo->nz;
  mpi_para[0].gn = cdo->gn;
  mpi_para[0].gnx = cdo->gnx;
  mpi_para[0].gny = cdo->gny;
  mpi_para[0].gnz = cdo->gnz;

  mpi_para[0].npe=mpi->npe;
  mpi_para[0].npe_x=mpi->npe_x;
  mpi_para[0].npe_y=mpi->npe_y;
  mpi_para[0].npe_z=mpi->npe_z;

  mpi_para[0].rank=mpi->rank;
  mpi_para[0].rank_x=mpi->rank_x;
  mpi_para[0].rank_y=mpi->rank_y;
  mpi_para[0].rank_z=mpi->rank_z;

  switch(topo_flag) {
  case 1: /* U */
    if (mpi->rank_x == mpi->npe_x - 1) {
      mpi_para[0].nx += 1;
      mpi_para[0].n += mpi_para[0].ny * mpi_para[0].nz;
    }
    mpi_para[0].nxblock = prm->cdo->solver_u_block_size.nxblock;
    mpi_para[0].nyblock = prm->cdo->solver_u_block_size.nyblock;
    mpi_para[0].nzblock = prm->cdo->solver_u_block_size.nzblock;
    break;
  case 2: /* V */
    if (mpi->rank_y == mpi->npe_y - 1) {
      mpi_para[0].ny += 1;
      mpi_para[0].n += mpi_para[0].nx * mpi_para[0].nz;
    }
    mpi_para[0].nxblock = prm->cdo->solver_v_block_size.nxblock;
    mpi_para[0].nyblock = prm->cdo->solver_v_block_size.nyblock;
    mpi_para[0].nzblock = prm->cdo->solver_v_block_size.nzblock;
    break;
  case 3: /* W */
    if (mpi->rank_z == mpi->npe_z - 1) {
      mpi_para[0].nz += 1;
      mpi_para[0].n += mpi_para[0].nx * mpi_para[0].ny;
    }
    mpi_para[0].nxblock = prm->cdo->solver_w_block_size.nxblock;
    mpi_para[0].nyblock = prm->cdo->solver_w_block_size.nyblock;
    mpi_para[0].nzblock = prm->cdo->solver_w_block_size.nzblock;
    break;
  default:
    mpi_para[0].nxblock = prm->cdo->solver_p_block_size.nxblock;
    mpi_para[0].nyblock = prm->cdo->solver_p_block_size.nyblock;
    mpi_para[0].nzblock = prm->cdo->solver_p_block_size.nzblock;
    break;
  }

#if 1
  mpi_para[0].stm = 1;
  mpi_para[0].stp = 1;

  mpi_para[0].mx = mpi_para[0].stm + mpi_para[0].nx + mpi_para[0].stp;
  mpi_para[0].my = mpi_para[0].stm + mpi_para[0].ny + mpi_para[0].stp;
  mpi_para[0].mz = mpi_para[0].stm + mpi_para[0].nz + mpi_para[0].stp;

  mpi_para[0].mxy = mpi_para[0].mx * mpi_para[0].my;
  mpi_para[0].m   = mpi_para[0].mx * mpi_para[0].my * mpi_para[0].mz;
  
  mpi_para[0].gm  = mpi_para[0].stm + cdo->gn + mpi_para[0].stp;
  mpi_para[0].gmx = mpi_para[0].stm + cdo->gnx + mpi_para[0].stp;
  mpi_para[0].gmy = mpi_para[0].stm + cdo->gny + mpi_para[0].stp;
  mpi_para[0].gmz = mpi_para[0].stm + cdo->gnz + mpi_para[0].stp;
#endif


  int i;
  for(i=0;i<6;i++){
    mpi_para[0].nrk[i]=mpi->nrk[i];
  }

#ifdef JUPITER_MPI
  mpi_para[0].comm=prm->mpi->CommJUPITER;
#endif

  return 0;
}

int set_subdividing_blockjacobi(mpi_prm *prm){
  int rank = prm[0].rank;
  int stm;
  int nx,ny,nz;
  int m,mx,my,mxy;
  set_dim(prm[0], &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  // zの分割数 * z方向の最大サイズのフィルタ
  // yの分割数 * y方向の最大サイズのフィルタ
  int xdivblock = nx;
  int ydivblock = -1;
  int zdivblock = -1;
#if 0
  const int min_yblocksize = ny;
  const int min_zblocksize = nz;
#else
  const int min_yblocksize = SUBDIVIDING_YBLOCKSIZE;
  const int min_zblocksize = SUBDIVIDING_ZBLOCKSIZE;
  //  const int min_yblocksize = 4;
  //  const int min_zblocksize = 4;
#endif
  if(ny<min_yblocksize){
    ydivblock = 1;
  }else{
    ydivblock = ny/min_yblocksize;
  }
  if(nz<min_zblocksize){
    zdivblock = 1;
  }else{
    zdivblock = nz/min_zblocksize;
  }

  int nxblock =  1;
  int nyblock = -1;
  int j;
  for(j=0;j<ydivblock;j++){
    int js = 0;
    int je = ny-1;
    int js_j = -1;
    int je_j = -1;
    Sep_region(&j,&ydivblock,&js,&je,&js_j,&je_j);
    int nblock = je_j - js_j + 1;
    if(rank==0){
      //      printf("Y : j = %d, js_j, je_j = %d, %d , nblock = %d \n",j,js_j,je_j,nblock);
    }
    if(nblock>nyblock){
      nyblock = nblock;
    }
  }

  int nzblock = -1;
  for(j=0;j<zdivblock;j++){
    int js = 0;
    int je = nz-1;
    int js_j = -1;
    int je_j = -1;
    Sep_region(&j,&zdivblock,&js,&je,&js_j,&je_j);
    int nblock = je_j - js_j + 1;
    if(rank==0){
      //      printf("Z : j = %d, js_j, je_j = %d, %d , nblock = %d \n",j,js_j,je_j,nblock);
    }
    if(nblock>nzblock){
      nzblock = nblock;
    }
  }

  // 細分化ブロックヤコビのブロック数
  prm[0].zdivblock = zdivblock;
  prm[0].ydivblock = ydivblock;
  prm[0].xdivblock = xdivblock;

  // 不要な計算範囲を含むブロックサイズ(計算ブロックごとにサイズが異なるがGPUで同一な命令で実行させるため余分な計算を行う)
  prm[0].nxblock = nxblock;
  prm[0].nyblock = nyblock;
  prm[0].nzblock = nzblock;

  //  printf("nyblock,ydivbloc  = %d , %d \n",nyblock,ydivblock);
  //  printf("nzblock,zdivblock = %d , %d \n",nzblock,zdivblock);

  return 0;
}

int set_subdividing_3Dblockjacobi(
				  mpi_prm *prm
				  ){
  //  int rank = prm[0].rank;
  int stm;
  int nx,ny,nz;
  int m,mx,my,mxy;
  set_dim(prm[0], &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);
  const int xblocksize = SUBDIVIDING_XBLOCKSIZE;
  const int yblocksize = SUBDIVIDING_YBLOCKSIZE;
  const int zblocksize = SUBDIVIDING_ZBLOCKSIZE;

  int xdivblock = -1;
  int ydivblock = -1;
  int zdivblock = -1;

  if(nx<xblocksize){
    xdivblock = 1;
  }else{
    xdivblock = nx/xblocksize;
    if( (xdivblock * xblocksize) < nx){
      xdivblock = xdivblock + 1;
    }
  }
  if(ny<yblocksize){
    ydivblock = 1;
  }else{
    ydivblock = ny/yblocksize;
    if( (ydivblock * yblocksize) < ny){
      ydivblock = ydivblock + 1;
    }
  }
  if(nz<zblocksize){
    zdivblock = 1;
  }else{
    zdivblock = nz/zblocksize;
    if( (zdivblock * zblocksize) < nz){
      zdivblock = zdivblock + 1;
    }
  }

  // ここの加算は袖領域のために取得
  xdivblock = xdivblock + 1*2;
  ydivblock = ydivblock + 1*2;
  zdivblock = zdivblock + 1*2;


#if 0
  printf(" nx,ny,nz = %d, %d, %d \n",nx,ny,nz);
  printf(" xdivblock,ydivblock,zdivblock = %d, %d, %d \n",xdivblock,ydivblock,zdivblock);
  printf(" xdivblock * xblocksize , ydivblock * yblocksiye , zdivblock * zblocksize = %d, %d, %d \n",
	 xdivblock * xblocksize,
	 ydivblock * yblocksize,
	 zdivblock * zblocksize
	 );
#endif

  prm[0].nzblock   = zblocksize;
  prm[0].nyblock   = yblocksize;
  prm[0].nxblock   = xblocksize;

  prm[0].mzdivblock = zdivblock;
  prm[0].mydivblock = ydivblock;
  prm[0].mxdivblock = xdivblock;

  prm[0].nzdivblock = zdivblock-2;
  prm[0].nydivblock = ydivblock-2;
  prm[0].nxdivblock = xdivblock-2;

  prm[0].block_mx = xblocksize * xdivblock;
  prm[0].block_my = yblocksize * ydivblock;
  prm[0].block_mz = zblocksize * zdivblock;
  prm[0].block_m  =  prm[0].block_mx * prm[0].block_my * prm[0].block_mz;

  prm[0].block_nx = xblocksize * (xdivblock-2);
  prm[0].block_ny = yblocksize * (ydivblock-2);
  prm[0].block_nz = zblocksize * (zdivblock-2);

  return 0;
}

int set_subdividing_3Dblock_list(
				 mpi_prm prm,
				 int *block_nxs,int *block_nxe,
				 int *block_nys,int *block_nye,
				 int *block_nzs,int *block_nze,
				 int *stride_xp,int *stride_xm,
				 int *stride_yp,int *stride_ym,
				 int *stride_zp,int *stride_zm 
				 ){
  //  int rank = prm.rank;
  int stm;
  int nx,ny,nz;
  int m,mx,my,mxy;
  set_dim(prm, &stm, &nx, &ny, &nz, &m, &mx, &my, &mxy);

  int nzblock = prm.nzblock;
  int nyblock = prm.nyblock;
  int nxblock = prm.nxblock;

  int nzdivblock = prm.nzdivblock;
  int nydivblock = prm.nydivblock;
  int nxdivblock = prm.nxdivblock;

  int jjxb,jjyb,jjzb;
  int jxb,jyb,jzb;

  for(jxb=0;jxb<nxdivblock;jxb++){	     
    block_nxs[jxb] = 0;
    block_nxe[jxb] = nxblock-1;
    jjxb = block_nxe[jxb];
    int jxe = jxb * nxblock + jjxb;
    if(jxe >= nx-1){
      for(jjxb=0;jjxb<nxblock;jjxb++){
	int jx = jxb * nxblock + jjxb;
	if(nx==jx){
	  block_nxe[jxb] = jjxb-1;
	  continue;
	}
      }
    }
  }

  for(jyb=0;jyb<nydivblock;jyb++){	     
    block_nys[jyb] = 0;
    block_nye[jyb] = nyblock-1;
    jjyb = block_nye[jyb];
    int jye = jyb * nyblock + jjyb;
    if(jye >= ny-1){
      for(jjyb=0;jjyb<nyblock;jjyb++){
	int jy = jyb * nyblock + jjyb;
	if(ny==jy){
	  block_nye[jyb] = jjyb-1;
	  continue;
	}
      }
    }
  }

  for(jzb=0;jzb<nzdivblock;jzb++){	     
    block_nzs[jzb] = 0;
    block_nze[jzb] = nzblock-1;
    jjzb = block_nze[jzb];
    int jze = jzb * nzblock + jjzb;
    if(jze >= nz-1){
      for(jjzb=0;jjzb<nzblock;jjzb++){
	int jz = jzb * nzblock + jjzb;
	if(nz==jz){
	  block_nze[jzb] = jjzb-1;
	  continue;
	}
      }
    }
  }

  //  int jjb;
  int mzdivblock = prm.mzdivblock;
  int mydivblock = prm.mydivblock;
  int mxdivblock = prm.mxdivblock;

  int mdivblock = mxdivblock * mydivblock * mzdivblock;

  for(jjxb=0;jjxb<nxblock;jjxb++){
    stride_xm[jjxb] = -mdivblock;
    stride_xp[jjxb] =  mdivblock;
  }
  // 境界処理
  stride_xm[0]         =  mdivblock * (nxblock-1) - 1;
  stride_xp[nxblock-1] = -mdivblock * (nxblock-1) + 1;

  for(jjyb=0;jjyb<nyblock;jjyb++){
    stride_ym[jjyb] = -mdivblock * (nxblock);
    stride_yp[jjyb] =  mdivblock * (nxblock);
  }
  // 境界処理
  stride_ym[0]         =  mdivblock * (nxblock)*(nyblock-1) - mxdivblock;
  stride_yp[nyblock-1] = -mdivblock * (nxblock)*(nyblock-1) + mxdivblock; 

  for(jjzb=0;jjzb<nzblock;jjzb++){
    stride_zm[jjzb] = -mdivblock * (nxblock) * (nyblock);
    stride_zp[jjzb] =  mdivblock * (nxblock) * (nyblock);
  }
  // 境界処理
  stride_zm[0]         =  mdivblock * (nxblock)*(nyblock)*(nzblock-1) - mxdivblock*mydivblock;
  stride_zp[nzblock-1] = -mdivblock * (nxblock)*(nyblock)*(nzblock-1) + mxdivblock*mydivblock;

#if 0
  printf(" ----------------- \n");
  for(jjxb=0;jjxb<nxblock;jjxb++){
    printf("stride xp : %d \n",stride_xp[jjxb]);
  }
  for(jjxb=0;jjxb<nxblock;jjxb++){
    printf("stride xm : %d \n",stride_xm[jjxb]);
  }
  printf(" ----------------- \n");

  printf(" ----------------- \n");
  for(jjyb=0;jjyb<nyblock;jjyb++){
    printf("stride yp : %d \n",stride_yp[jjyb]);
  }
  for(jjyb=0;jjyb<nyblock;jjyb++){
    printf("stride ym : %d \n",stride_ym[jjyb]);
  }
  printf(" ----------------- \n");

  printf(" ----------------- \n");
  for(jjzb=0;jjzb<nzblock;jjzb++){
    printf("stride zp : %d \n",stride_zp[jjzb]);
  }
  for(jjzb=0;jjzb<nzblock;jjzb++){
    printf("stride zm : %d \n",stride_zm[jjzb]);
  }
  printf(" ----------------- \n");
#endif

#if 0
  for(jzb=0;jzb<nzdivblock;jzb++){
    const int jjzbs = block_nzs[jzb];
    const int jjzbe = block_nze[jzb];
    for(jjzb=jjzbs;jjzb<=jjzbe;jjzb++){    
      int jz = jzb * nzblock + jjzb;
      printf(" jz = %04d \n",jz);
    }
  }

  for(jyb=0;jyb<nydivblock;jyb++){
    const int jjybs = block_nys[jyb];
    const int jjybe = block_nye[jyb];
    for(jjyb=jjybs;jjyb<=jjybe;jjyb++){    
      int jy = jyb * nyblock + jjyb;
      printf(" jy = %04d \n",jy);
    }
  }

  for(jxb=0;jxb<nxdivblock;jxb++){
    const int jjxbs = block_nxs[jxb];
    const int jjxbe = block_nxe[jxb];
    for(jjxb=jjxbs;jjxb<=jjxbe;jjxb++){    
      int jx = jxb * nxblock + jjxb;
      printf(" jx = %04d \n",jx);
    }
  }

#endif

  return 0;
}
