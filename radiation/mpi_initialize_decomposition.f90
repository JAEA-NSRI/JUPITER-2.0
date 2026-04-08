! Jupiter と連成させるときは, この中 & set_mpi_config で設定している各情報を Jupiter より受け取る
subroutine mpi_initialize_decomposition

	use MODULE_RADIATION
	
	implicit none
	
	include 'mpif.h'

	integer(4):: ierr= 0
	integer(4):: Nproc_xy= 0
	integer(4):: rank_yz= 0, rank_zx= 0, rank_xy= 0
	integer(4):: rank_xm= 0, rank_xp= 0
	integer(4):: rank_ym= 0, rank_yp= 0
	integer(4):: rank_zm= 0, rank_zp= 0

	Nproc_xy= Nproc_x*Nproc_y

	rank_x= mod(mod(myrank, Nproc_xy), Nproc_x);
	rank_y= mod(myrank, Nproc_xy)/Nproc_x;
	rank_z= myrank/Nproc_xy;

	rank_yz= rank_y+ Nproc_y*rank_z;
	rank_zx= rank_x+ Nproc_x*rank_z;
	rank_xy= rank_x+ Nproc_x*rank_y;

	!neighbor rank
	rank_xm= rank_x- 1;
	rank_xp= rank_x+ 1;
	rank_ym= rank_y- 1;
	rank_yp= rank_y+ 1;
	rank_zm= rank_z- 1;
	rank_zp= rank_z+ 1;

	!neighbor rank
	rank1= -1
	rank2= -1
	rank3= -1
	rank4= -1
	rank5= -1
	rank6= -1
	if(rank_xm .gt. -1 ) rank1= rank_xm+ Nproc_x*rank_y + Nproc_xy*rank_z
	if(rank_xp .lt. Nproc_x) rank2= rank_xp+ Nproc_x*rank_y + Nproc_xy*rank_z
	if(rank_ym .gt. -1 ) rank3= rank_x + Nproc_x*rank_ym+ Nproc_xy*rank_z
	if(rank_yp .lt. Nproc_y) rank4= rank_x + Nproc_x*rank_yp+ Nproc_xy*rank_z
	if(rank_zm .gt. -1 ) rank5= rank_x + Nproc_x*rank_y + Nproc_xy*rank_zm
	if(rank_zp .lt. Nproc_z) rank6= rank_x + Nproc_x*rank_y + Nproc_xy*rank_zp

! このサブルーチンは, 角度方向のマスターノードでのみ呼び出される
! →ここで同期をとろうとすると, 角度方向のノードでいつまでたっても同期がとれない → 先に進まない
!	call MPI_Barrier(MPI_COMM_WORLD, ierr)

end