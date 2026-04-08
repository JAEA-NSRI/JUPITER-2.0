program radiation

#define MPI_MODE !MPIの有効化
	!$ use omp_lib !スペースは必須

	use MODULE_STRUCT
	use MODULE_RADIATION
	use MODULE_LOG

	implicit none


#ifdef MPI_MODE
	!DeinoMPIの場合は追加の依存ファイル./mpi/DenFMPIu.libが必要になる．
	include 'mpif.h'
	! 角度のマスター - スレーブ間の通信用変数, タグは, 0 以上でないとダメらしい (send のとき)
	integer :: node, master_lat, status(MPI_STATUS_SIZE), tag= 10, req
    real(8) :: MPI_Comm_Test_Double= 0.0d0 !計算開始前の MPI 通信環境テスト用データ
#endif

	integer:: ierr= 0, picard= 0, newton= 0
	integer:: ndim= 0, nump= 0, numm= 0, output_result_data_interval= 0
	integer:: i= 0, j= 0, k= 0, m= 0, n= 0, i_trs= 0
	character(255):: init_file_location= "\0", init_file_name= "\0", result_file_location= "\0"
	character(4):: myrank_str= ''
	real(8):: dtmp= 0.0d0
	real(8):: dt= 0.0d0
	real(8):: lat= 0.0d0, E_cell_err= 0.0d0, tmp_cell_err= 0.0d0, dtmp_cell_err= 0.0d0
	real(8):: E_cell_max= 0.0d0, tmp_cell_max= 0.0d0
    
    type(bindata4), pointer :: out_binary_buffer(:) ! バイナリデータ出力用のバッファ

	! -------------------------------------------------------------- 追加
	character(len=80) cVar
	integer :: isub   ! POPCORN のサブプロセスとして立ち上げるか否かのスイッチ (0: 単体, 1: サブ)
	integer :: master_rad = 0
	integer :: nth = 0
	integer :: nag = 0
	integer :: ibgn, iend, nang, ichk
	integer :: iflg= 0, iexit = 0
	integer :: it= 0, maxloop
	type(max_min) :: xyz_range(3)

	integer:: file= 0
	! -------------------------------------------------------------- 追加

	!コマンドライン引数による実行スレッド数 & 実効モードの指定
	nag = command_argument_count()
	! 1つめの引数は使用スレッド数/ノード
	if(nag .ge. 1) then
		call get_command_argument(1, cVar)
		read(cVar,*) nth
		if(nth .gt. mpisize) nth = mpisize
		if(nth .le. 0) nth = 1
	else
		nth = 1
	endif

	! 2つめの引数は, 実行モード
	!
	! 0：輻射プログラム単体 + 角度・領域分割 or ノード分割なし
	! 1：POPCORN 連成 + 角度方向分割
	! 2：JUPITER連成 + 領域分割
	!
	if(nag .ge. 2) then
		call get_command_argument(2, cVar)
		read(cVar,*) isub
		if(isub .gt. 2 .or. isub .lt. 0) then
			write(6, *) "!!! Radiation runnning mode ID must be from 0 to 2: !!!"
			write(6, *) "0 : Only radiation with the angular and spatial decomposition"
			write(6, *) "1 : POPCORN + radiation with the angular decomposition"
			write(6, *) "2 : JUPITER + radiation with the angular and spatial decomposition"
			stop
		endif
	else
		isub = 0
	endif
	
	!実行スレッド数の指定(omp_get_max_threads()の戻り値にも反映される)
	call omp_set_num_threads(nth)
	
	! MPI 関連変数 (輻射のマスターランク, 各方向の分割数, 隣接ランク, 角度方向の担当範囲) 初期化
	master_rad= 0
	Nproc_lat= 1
	Nproc_x= 1
	Nproc_y= 1
	Nproc_z= 1
		
	rank1= -1
	rank2= -1
	rank3= -1
	rank4= -1
	rank5= -1
	rank6= -1
	
	ibgn= 1
	iend= N_lat

	! -------------------------------------------------------------- MPI
#ifdef MPI_MODE
	call MPI_init(ierr) ! MPI 環境初期化
	call MPI_Comm_Rank(MPI_COMM_WORLD, myrank_glob, ierr) ! 自分のランクの取得
	call MPI_Comm_Size(MPI_COMM_WORLD, mpisize_glob, ierr) ! 全体のプロセス数
#else
	isub = 0 ! MPI 対応の実行形式でない場合は, 強制的に isub= 0
	mpi_nbuf= 0
	myrank_glob = 0
	mpisize_glob = 1
#endif
	! -------------------------------------------------------------- MPI
	! 0：輻射プログラム単体 + 角度・領域分割 or ノード分割なし
	if(isub .eq. 0) then
!DEBUG write(6, *) "At radiation... ", myrank_glob, myrank, isub
		master_rad= 0
		myrank= myrank_glob
		mpisize= mpisize_glob

		if(myrank .lt. Nproc_x*Nproc_y*Nproc_z) then
			call set_radiation_configuration(ndim, nump, numm, maxloop, output_result_data_interval, xyz_range, dt, 'rad_config.dat', init_file_location, init_file_name, result_file_location) !条件入力

			! 角度方向分割数
			if(Nproc_lat .lt. 1) Nproc_lat= 1

			! 空間方向分割数
			if(Nproc_x .lt. 1) Nproc_x= 1
			if(ndim .eq. 2) then
				Nproc_y= 1
			else
				if(Nproc_y .lt. 1) Nproc_y= 1
			endif
			if(Nproc_z .lt. 1) Nproc_z= 1
		endif

#ifdef MPI_MODE
		CommRADIATION= MPI_COMM_WORLD

!		write(6, *) 'Start broadcasting ... ', myrank
		call MPI_Bcast(ndim, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast ndim', myrank
		call MPI_Bcast(Nproc_lat, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast Nproc_lat', myrank
		call MPI_Bcast(Nproc_x, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast Nproc_x', myrank
		call MPI_Bcast(Nproc_y, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast Nproc_y', myrank
		call MPI_Bcast(Nproc_z, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast Nproc_z', myrank

		call MPI_Bcast(init_file_name, 255, MPI_CHARACTER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast init_file_name', myrank
		call MPI_Bcast(init_file_location, 255, MPI_CHARACTER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast init_file_location', myrank
		call MPI_Bcast(result_file_location, 255, MPI_CHARACTER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast result_file_location', myrank

		call MPI_Bcast(dt, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast dt', myrank
		call MPI_Bcast(maxloop, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast maxloop', myrank
		call MPI_Bcast(output_result_data_interval, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast result_data_interval', myrank
			
		call MPI_Bcast(xyz_range(1)%max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(xyz_range(2)%max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(xyz_range(3)%max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(xyz_range(1)%min, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(xyz_range(2)%min, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(xyz_range(3)%min, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast xyz_range', myrank
			
		call MPI_Bcast(Nx, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast Nx', myrank
		call MPI_Bcast(Ny, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast Ny', myrank
		call MPI_Bcast(Nz, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast Nz', myrank
		call MPI_Bcast(dx, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast dx', myrank
		call MPI_Bcast(dy, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast dy', myrank
		call MPI_Bcast(dz, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast dz', myrank
			
		call MPI_Bcast(N_lat, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast N_lat', myrank
		call MPI_Bcast(N_lon, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast N_lon', myrank
		call MPI_Bcast(d_lat, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast d_lat', myrank
		call MPI_Bcast(d_lon, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast d_lon', myrank

		call MPI_Bcast(picard_max, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)	
!		write(6, *) 'MPI_Bcast picard_max', myrank
		call MPI_Bcast(picard_out, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)	
!		write(6, *) 'MPI_Bcast picard_out', myrank
		call MPI_Bcast(newton_max, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast newton_max', myrank

		call MPI_Bcast(E_cell_err_max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast E_cell_err_max', myrank
		call MPI_Bcast(tmp_cell_err_max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast tmp_cell_err_max', myrank
		call MPI_Bcast(dtmp_cell_err_max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast dtmp_cell_err_max', myrank

		call MPI_Bcast(I_bcd_flg, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast I_bcd_flg', myrank

!		call MPI_Finalize(ierr)
!		stop
#endif

		! プロセス数の一貫性チェック
		if(mpisize .ne. Nproc_lat*Nproc_x*Nproc_y*Nproc_z) then
			if(myrank .eq. 0) then
				write(6, *) "!!! Radiation comm. size does not agree with required num. of processes !!!"
				write(6, '((a), (I3))') "Comm. size: ", mpisize
				write(6, '((a), (I3))') "Proc. in anglular: ", Nproc_lat
				write(6, '((a), (I3))') "Proc. in x: ", Nproc_x
				if(ndim .eq. 3) write(6, '((a), (I3))') "Proc. in y: ", Nproc_y
				write(6, '((a), (I3))') "Proc. in z: ", Nproc_z
			endif
			call MPI_Finalize(ierr)
			stop
		endif

		! 配列の割り付け & ゼロクリア (out_binary_buffer の 3 は, ベクトルデータ対応用) & 境界条件の設定
		call set_radiation(ndim, nump, numm, xyz_range)
		! *.vtk バッファ作成
		allocate(out_binary_buffer(1:3*(Nx+1)*(Nx+1)*(Nx+1)), stat= ierr)

		! 初期条件の読み込み
		if(myrank .lt. Nproc_x*Nproc_y*Nproc_z) then
			write(myrank_str, '(I4)') myrank
			do i= 1, 4
				if(myrank_str(i:i) .eq. '') myrank_str(i:i)= '0'
			enddo

			if(init_file_name(len(trim(init_file_name))-3:len(trim(init_file_name))) .eq. '.dat') then
				init_file_name= init_file_name(1:len(trim(init_file_name))-4)
			endif
			init_file_name= trim(init_file_name)//'_'//myrank_str//'.dat'
			call set_radiation_initial_condition(ndim, nump, numm, xyz_range, init_file_location, init_file_name)
		endif
	else if(isub .eq. 1) then ! 1：POPCORN 連成 + 角度方向分割
		! 以下は MPI 対応実行形式以外ありえない
#ifdef MPI_MODE
		master_rad= 1

		maxloop = 999999999 !メインループを半無限ループとする

		!POPCORNからの情報の受信
		call MPI_Bcast(dt, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(ndim, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(nump, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(numm, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(Nx, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(Ny, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(Nz, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(N_lat, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(N_lon, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(picard_max, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)	
		call MPI_Bcast(newton_max, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(tmp_wall_x_ini, Ny*Nz, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(emi_wall_x_ini, Ny*Nz, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(tmp_wall_x_fin, Ny*Nz, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(emi_wall_x_fin, Ny*Nz, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(tmp_wall_y_ini, Nz*Nx, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(emi_wall_y_ini, Nz*Nx, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(tmp_wall_y_fin, Nz*Nx, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(emi_wall_y_fin, Nz*Nx, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(tmp_wall_z_ini, Nx*Ny, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(emi_wall_z_ini, Nx*Ny, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(tmp_wall_z_fin, Nx*Ny, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(emi_wall_z_fin, Nx*Ny, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(E_cell_err_max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(tmp_cell_err_max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(dtmp_cell_err_max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(xyz_range, 6, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(dx, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(dy, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(dz, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(d_lat, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(d_lon, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(I_bcd_flg, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)

		! POPCORN からセル数を受信した場合には, 仮想セルの二つ (各軸方向の始点・終点) 分を足す
        Nx= Nx+ 2
        Ny= Ny+ 2
        Nz= Nz+ 2

		! 配列の割り付け & ゼロクリア (out_binary_buffer の 3 は, ベクトルデータ対応用) & 境界条件の設定
		call set_radiation(ndim, nump, numm, xyz_range)
		! *.vtk バッファ作成
		allocate(out_binary_buffer(1:3*(Nx+1)*(Nx+1)*(Nx+1)), stat= ierr)
		mpi_nbuf= size(Ir)  ! 角度方向分割時の MPI 通信配列の大きさ
#endif
	else if(isub .eq. 2) then ! 2：輻射プログラム単体 + JUPITER
		! 以下は MPI 対応実行形式以外ありえない
#ifdef MPI_MODE
		call MPI_COMM_SPLIT(MPI_COMM_WORLD, 1, myrank_glob, CommRADIATION, ierr)
		call MPI_Comm_Rank(CommRADIATION, myrank, ierr) ! 自分のランクの取得 (輻射のみのコミュニケータに対して)
		call MPI_Comm_Size(CommRADIATION, mpisize, ierr) ! 全体のプロセス数  (輻射のみのコミュニケータに対して)

!DEBUG write(6, *) "At radiation... ", myrank_glob, myrank, isub
!		write(6, *) "start.. ", isub, myrank
		call MPI_Bcast(Nproc_x, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast Nproc_x', myrank
		call MPI_Bcast(Nproc_y, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast Nproc_y', myrank
		call MPI_Bcast(Nproc_z, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast Nproc_z', myrank
		call MPI_Bcast(Nproc_lat, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast Nproc_lat', myrank

		call MPI_Bcast(dt, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast dt', myrank
		call MPI_Bcast(output_result_data_interval, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast output_result_data_interval', myrank
			
		call MPI_Bcast(xyz_range(1)%max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(xyz_range(2)%max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(xyz_range(3)%max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(xyz_range(1)%min, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(xyz_range(2)%min, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
		call MPI_Bcast(xyz_range(3)%min, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast xyz_range', myrank
			
		call MPI_Bcast(Nx, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast Nx', myrank
		call MPI_Bcast(Ny, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast Ny', myrank
		call MPI_Bcast(Nz, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast Nz', myrank
			
		call MPI_Bcast(N_lat, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast N_lat', myrank
		call MPI_Bcast(N_lon, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast N_lon', myrank

		call MPI_Bcast(picard_max, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast picard_max', myrank
		call MPI_Bcast(picard_out, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast picard_out', myrank
		call MPI_Bcast(newton_max, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast newton_max', myrank

		call MPI_Bcast(E_cell_err_max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast picard_max', E_cell_err_max
		call MPI_Bcast(tmp_cell_err_max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast picard_max', tmp_cell_err_max
		call MPI_Bcast(dtmp_cell_err_max, 1, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast picard_max', dtmp_cell_err_max

		call MPI_Bcast(I_bcd_flg, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast I_bcd_flg', myrank

		call MPI_Bcast(numm, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(6, *) 'MPI_Bcast numm', myrank

		master_rad= Nproc_x*Nproc_y*Nproc_z ! 輻射マスター(グローバルコミュニケータでの)ランクは, JUPITER の領域分割分オフセット
		
		ndim= 3											! JUPITER 連成時は常に 3D
		init_file_name= ""								! 初期条件ファイル名なし
		init_file_location= ""							! 初期条件ファイルディレクトリ名なし
		result_file_location= "./data/binary_data/"		! 結果ファイルディレクトリ名なし
		maxloop = 999999999								! メインループを半無限ループとする
		nump= 0											! 粒子数
		
		Nx= Nx+ 2
		Ny= Ny+ 2
		Nz= Nz+ 2
		
		dx= (xyz_range(1)%max- xyz_range(1)%min)/dble(Nx- 2)
		dy= (xyz_range(2)%max- xyz_range(2)%min)/dble(Ny- 2)
		dz= (xyz_range(3)%max- xyz_range(3)%min)/dble(Nz- 2)
		
		d_lat= 2.0d0*M_PI/dble(N_lat)
		d_lon= M_PI/dble(N_lon)

		! プロセス数の一貫性チェック
		if(mpisize .ne. Nproc_lat*Nproc_x*Nproc_y*Nproc_z) then
			if(myrank .eq. 0) then
				write(6, *) "!!! Radiation comm. size does not agree with required num. of processes !!!"
				write(6, '((a), (I3))') "Comm. size (Radiation only): ", mpisize
				write(6, '((a), (I3))') "Proc. in anglular: ", Nproc_lat
				write(6, '((a), (I3))') "Proc. in x: ", Nproc_x
				if(ndim .eq. 3) write(6, '((a), (I3))') "Proc. in y: ", Nproc_y
				write(6, '((a), (I3))') "Proc. in z: ", Nproc_z
			endif
				call MPI_Finalize(ierr)
			stop
		endif

		! 配列の割り付け & ゼロクリア (out_binary_buffer の 3 は, ベクトルデータ対応用) & 境界条件の設定
		! (isub= 0 のときに, set_radiation 後に初期条件を読み込むので, isub の各 if 分岐中で call している)
		call set_radiation(ndim, nump, numm, xyz_range)
		! *.vtk バッファ作成
		allocate(out_binary_buffer(1:3*(Nx+1)*(Nx+1)*(Nx+1)), stat= ierr)
		mpi_nbuf= size(Ir)  ! 角度方向分割時の MPI 通信配列の大きさ
#endif
	endif


#ifdef MPI_MODE
	! 領域分割情報作成
	if(myrank .lt. Nproc_x*Nproc_y*Nproc_z) then
		! 空間方向
		call mpi_initialize_decomposition

		! 各角度方向のマスターノードでは, master_lat== myrank
		master_lat= myrank
		do node= Nproc_x*Nproc_y*Nproc_z+ (Nproc_lat- 1)*myrank, Nproc_x*Nproc_y*Nproc_z+ (Nproc_lat- 1)*(myrank+ 1)- 1
			call MPI_Isend(rank1, 1, MPI_INTEGER, node, tag, CommRADIATION, req, ierr);
			call MPI_Isend(rank2, 1, MPI_INTEGER, node, tag, CommRADIATION, req, ierr);
			call MPI_Isend(rank3, 1, MPI_INTEGER, node, tag, CommRADIATION, req, ierr);
			call MPI_Isend(rank4, 1, MPI_INTEGER, node, tag, CommRADIATION, req, ierr);
			call MPI_Isend(rank5, 1, MPI_INTEGER, node, tag, CommRADIATION, req, ierr);
			call MPI_Isend(rank6, 1, MPI_INTEGER, node, tag, CommRADIATION, req, ierr);
		enddo

		if(Nproc_lat .gt. 1) then
			call MPI_Wait(req, status, ierr)
		endif
	else
		! 各角度方向のスレーブノードでは, どのランクが master_lat か,きちんと計算する
		master_lat= int((myrank- Nproc_x*Nproc_y*Nproc_z)/(Nproc_lat- 1))

		! まずは, 隣接領域の角度方向マスターランクを受領
		call MPI_Irecv(rank1, 1, MPI_INTEGER, master_lat, tag, CommRADIATION, req, ierr);
		call MPI_Irecv(rank2, 1, MPI_INTEGER, master_lat, tag, CommRADIATION, req, ierr);
		call MPI_Irecv(rank3, 1, MPI_INTEGER, master_lat, tag, CommRADIATION, req, ierr);
		call MPI_Irecv(rank4, 1, MPI_INTEGER, master_lat, tag, CommRADIATION, req, ierr);
		call MPI_Irecv(rank5, 1, MPI_INTEGER, master_lat, tag, CommRADIATION, req, ierr);
		call MPI_Irecv(rank6, 1, MPI_INTEGER, master_lat, tag, CommRADIATION, req, ierr);

		call MPI_Wait(req, status, ierr)

		! 自分の角度方向スレーブランクに対応する, 隣接角度マスターのスレーブランクを計算
		if(rank1 .ne. -1) rank1= myrank+ (Nproc_lat- 1)*(rank1- master_lat)
		if(rank2 .ne. -1) rank2= myrank+ (Nproc_lat- 1)*(rank2- master_lat)
		if(rank3 .ne. -1) rank3= myrank+ (Nproc_lat- 1)*(rank3- master_lat)
		if(rank4 .ne. -1) rank4= myrank+ (Nproc_lat- 1)*(rank4- master_lat)
		if(rank5 .ne. -1) rank5= myrank+ (Nproc_lat- 1)*(rank5- master_lat)
		if(rank6 .ne. -1) rank6= myrank+ (Nproc_lat- 1)*(rank6- master_lat)
	endif

	! 角度方向の分割
	nang = int(N_lat / Nproc_lat)
	ichk = mod(N_lat , Nproc_lat)

	if(myrank .lt. Nproc_x*Nproc_y*Nproc_z) then
		ibgn= 1 ! 角度方向マスターの ibgn はつねに 1
		iend = ibgn + nang - 1
	else
		ibgn= 1+ nang*(myrank+ 1- Nproc_x*Nproc_y*Nproc_z- (Nproc_lat- 1)*master_lat)

		! キリの悪い時は担当角度範囲を修正 (master_latは修正しない)
		if(myrank- Nproc_x*Nproc_y*Nproc_z- (Nproc_lat- 1)*master_lat .lt. ichk) then
			ibgn= ibgn+ (myrank- Nproc_x*Nproc_y*Nproc_z- (Nproc_lat- 1)*master_lat)
			iend= ibgn+ nang
		else
			ibgn= ibgn+ ichk
			iend= ibgn+ nang- 1
		endif
	endif

!	call MPI_Finalize(ierr)
!	stop
#endif


	if(myrank.eq.0) then
        ! 設定条件の表示
        call output_exec_log_rad(isub) ! バッファ初期化
 	    write(LogStr(1), '((a))') "------------------------- Radiation program -------------------------"
        if(isub .eq. 0) then
     	    write(LogStr(2), '((a))') "Running Mode: Alone running"
        else
     	    write(LogStr(2), '((a))') "Running Mode: Coupled with POPCORN program"
        endif
        write(LogStr(3), '((a1))') " "
	    write(LogStr(4), '((a), (I1))') "Dimension: ", ndim
        write(LogStr(5), '((a1))') " "
        write(LogStr(6), '((a))') "Initial condition file: "
        write(LogStr(7), '((a))') trim(init_file_location)//trim(init_file_name)
        write(LogStr(8), '((a1))') " "
        write(LogStr(9), '((a))') "Result file: "
        write(LogStr(10), '((a))') trim(result_file_location)//'result_%06d.vtk'
        write(LogStr(11), '((a1))') " "
        write(LogStr(12), '((a), (E10.3))') "dt: ", dt
        write(LogStr(13), '((a), (I))') "Maximum iteration of time loop: ", maxloop
        if(output_result_data_interval .gt. 0) then
            write(LogStr(14), '((a), (I))') "Interval of result file output: ", output_result_data_interval
        else
            write(LogStr(14), '((a))') "Suppressed to output result files"
        endif
        write(LogStr(15), '((a1))') " "
        call output_exec_log_rad(isub)


        if(ndim .eq. 2) then
            write(LogStr(1), '((a), (F8.3), (a4), (F8.3), (a10), (F8.3), (a1))') "Calculation domain in x:  From ", xyz_range(1)%min, " to ", xyz_range(1)%max, &
                                                                                    " (Length: ", (xyz_range(1)%max- xyz_range(1)%min), ")"
            write(LogStr(2), '((a), (F8.3), (a4), (F8.3), (a10), (F8.3), (a1))') "Calculation domain in z:  From ", xyz_range(3)%min, " to ", xyz_range(3)%max, &
                                                                                    " (Length: ", (xyz_range(3)%max- xyz_range(3)%min), ")"
            write(LogStr(3), '((a4), (I5), (a9), (E10.3))') "Nx: ", Nx, " --> dx: ", dx
            write(LogStr(4), '((a4), (I5), (a9), (E10.3))') "Nz: ", Nz, " --> dz: ", dz
            write(LogStr(5), '((a7), (I5), (a12), (E10.3))') "N_lat: ", N_lat, " --> d_lat: ", d_lat
            write(LogStr(6), '((a1))') " "
        else
            write(LogStr(1), '((a), (F8.3), (a4), (F8.3), (a10), (F8.3), (a1))') "Calculation domain in x:  From ", xyz_range(1)%min, " to ", xyz_range(1)%max, &
                                                                                    " (Length: ", (xyz_range(1)%max- xyz_range(1)%min), ")"
            write(LogStr(2), '((a), (F8.3), (a4), (F8.3), (a10), (F8.3), (a1))') "Calculation domain in y:  From ", xyz_range(2)%min, " to ", xyz_range(2)%max, &
                                                                                    " (Length: ", (xyz_range(2)%max- xyz_range(2)%min), ")"
            write(LogStr(3), '((a), (F8.3), (a4), (F8.3), (a10), (F8.3), (a1))') "Calculation domain in z:  From ", xyz_range(3)%min, " to ", xyz_range(3)%max, &
                                                                                    " (Length: ", (xyz_range(3)%max- xyz_range(3)%min), ")"
            write(LogStr(4), '((a4), (I5), (a9), (E10.3))') "Nx: ", Nx, " --> dx: ", dx
            write(LogStr(5), '((a4), (I5), (a9), (E10.3))') "Ny: ", Ny, " --> dy: ", dy
            write(LogStr(6), '((a4), (I5), (a9), (E10.3))') "Nz: ", Nz, " --> dz: ", dz
            write(LogStr(7), '((a7), (I5), (a12), (E10.3))') "N_lat: ", N_lat, " --> d_lat: ", d_lat
            write(LogStr(8), '((a7), (I5), (a12), (E10.3))') "N_lon: ", N_lon, " --> d_lon: ", d_lon
            write(LogStr(9), '((a1))') " "
        endif
        call output_exec_log_rad(isub)


        write(LogStr(1), '((a), (I5))') "Maximum iteration of Picard loop: ", picard_max
        write(LogStr(2), '((a), (E10.3), (a), (E10.3))') "tmp_err_max: ", tmp_cell_err_max, " E_err_max: ", E_cell_err_max
        write(LogStr(3), '((a), (I5))') "Maximum iteration of temperature Newton loop: ", newton_max
        write(LogStr(4), '((a), (E10.3))') "dtmp_err_max: ", dtmp_cell_err_max
        write(LogStr(5), '((a1))') " "
        call output_exec_log_rad(isub)
  

        if(bcd_x_ini .eq. 0) then
     	    write(LogStr(1), '((a))') "Boundary at x-axis initial point: Open"
        else if(bcd_x_ini .eq. 1) then
            write(LogStr(1), '((a))') "Boundary at x-axis initial point: Wall (isothermal)"
        else if(bcd_x_ini .eq. 2) then
            write(LogStr(1), '((a))') "Boundary at x-axis initial point: Wall (adiabatic)"
        else
            write(LogStr(1), '((a))') "Boundary at x-axis initial point: Periodic"
        endif
        if(bcd_x_fin .eq. 0) then
     	    write(LogStr(2), '((a))') "Boundary at x-axis final point:   Open"
        else if(bcd_x_fin .eq. 1) then
            write(LogStr(2), '((a))') "Boundary at x-axis final point:   Wall (isothermal)"
        else if(bcd_x_fin .eq. 2) then
            write(LogStr(2), '((a))') "Boundary at x-axis final point:   Wall (adiabatic)"
		else
            write(LogStr(2), '((a))') "Boundary at x-axis final point:   Periodic"
        endif
        call output_exec_log_rad(isub)

        if(ndim .eq. 3) then
            if(bcd_y_ini .eq. 0) then
     	        write(LogStr(1), '((a))') "Boundary at y-axis initial point: Open"
            else if(bcd_y_ini .eq. 1) then
                write(LogStr(1), '((a))') "Boundary at y-axis initial point: Wall (isothermal)"
            else if(bcd_y_ini .eq. 2) then
                write(LogStr(1), '((a))') "Boundary at y-axis initial point: Wall (adiabatic)"
            else
                write(LogStr(1), '((a))') "Boundary at y-axis initial point: Periodic"
            endif
            if(bcd_y_fin .eq. 0) then
     	        write(LogStr(2), '((a))') "Boundary at y-axis final point:   Open"
            else if(bcd_y_fin .eq. 1) then
                write(LogStr(2), '((a))') "Boundary at y-axis final point:   Wall (isothermal)"
            else if(bcd_y_fin .eq. 2) then
                write(LogStr(2), '((a))') "Boundary at y-axis final point:   Wall (adiabatic)"
            else
                write(LogStr(2), '((a))') "Boundary at y-axis final point:   Periodic"
            endif
        endif
        call output_exec_log_rad(isub)


        if(bcd_z_ini .eq. 0) then
     	    write(LogStr(1), '((a))') "Boundary at z-axis initial point: Open"
        else if(bcd_z_ini .eq. 1) then
            write(LogStr(1), '((a))') "Boundary at z-axis initial point: Wall (isothermal)"
        else if(bcd_z_fin .eq. 2) then
            write(LogStr(1), '((a))') "Boundary at z-axis initial point: Wall (adiabatic)"
        else
            write(LogStr(1), '((a))') "Boundary at z-axis initial point: Periodic"
        endif
        if(bcd_z_fin .eq. 0) then
     	    write(LogStr(2), '((a))') "Boundary at z-axis final point:   Open"
        else if(bcd_z_fin .eq. 1) then
            write(LogStr(2), '((a))') "Boundary at z-axis final point:   Wall (isothermal)"
        else if(bcd_z_fin .eq. 2) then
            write(LogStr(2), '((a))') "Boundary at z-axis final point:   Wall (adiabatic)"
		else
            write(LogStr(2), '((a))') "Boundary at z-axis final point:   Periodic"
        endif
        call output_exec_log_rad(isub)


        ! デバッグ用 (MS Windows 環境)
!		call TimeCounterQPC(1, "radiation")
	endif

	if(myrank .lt. Nproc_x*Nproc_y*Nproc_z) then
		call Set_Boundary_tmp_cell(ndim)
        ! 初期条件ファイル出力
		if(master_rad .eq. 0) then
			! データ出力機番は, プロセスごとに違っていたほうがよい
!			call output_result_data(ndim, nump, 10+ myrank, it, dt, xyz_range, out_binary_buffer, result_file_location)
		endif
	endif

#ifdef MPI_MODE
	! 輻射ローカルマスター - slave 間の送受信しかチェックしない
    if(myrank .eq. 0) then
        write(LogStr(1), '((a1))') " "
        write(LogStr(2), '((a))') "Testing MPI Communication... "
        write(LogStr(3), '((a1))') " "
        call output_exec_log_rad(isub)

        write(LogStr(1), '((a))') "Sending data to slave ..."
        call output_exec_log_rad(isub)
	    do i = 1, mpisize - 1
		    call MPI_Send(MPI_Comm_Test_Double, 1, MPI_DOUBLE_PRECISION, i, 100, CommRADIATION, ierr)
            write(LogStr(1), '((a), (I3))') "OK: Sending to process: ", i
            call output_exec_log_rad(isub)
        enddo

        write(LogStr(1), '((a1))') " "
        write(LogStr(2), '((a))') "Receiving data from slave ..."
        call output_exec_log_rad(isub)
	    do i = 1, mpisize - 1
		    call MPI_Recv(MPI_Comm_Test_Double, 1, MPI_DOUBLE_PRECISION, i, 100, CommRADIATION, status, ierr)
            write(LogStr(1), '((a), (I3))') "OK: Receiving from process: ", i
            call output_exec_log_rad(isub)
        enddo
    else
        call MPI_Recv(MPI_Comm_Test_Double, 1, MPI_DOUBLE_PRECISION, 0, 100, CommRADIATION, status, ierr)
        call MPI_Send(MPI_Comm_Test_Double, 1, MPI_DOUBLE_PRECISION, 0, 100, CommRADIATION, ierr)
    endif
#endif

	if(myrank.eq.0) then
 	    write(LogStr(1), '((a))') "---------------------------------------------------------------------"
        write(LogStr(2), '((a1))') " "
        write(LogStr(3), '((a))') "Starting time iteration... "
        write(LogStr(4), '((a1))') " "
        call output_exec_log_rad(isub)
	endif
	
!	call MPI_Finalize(ierr)
!	stop
	! -------------------------------------------------------------- 追加

!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! ここから先は半無限ループとする(POPCORN/JUPITER 側からの終了指示待ち)
do it = 1, maxloop
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

    ! デバッグ用 (MS Windows 環境)
!	if(myrank.eq.0) then
!		call TimeCounterQPC(2, "radiation_loop")
!	endif

#ifdef MPI_MODE
	if(isub .eq. 0) then
		if(mpisize .gt. 1 .and. Nproc_lat .gt. 1) then
			if(myrank .lt. Nproc_x*Nproc_y*Nproc_z) then
				do node= 0, Nproc_lat- 2
					call MPI_Send(emi     , Nx*Ny*Nz, MPI_DOUBLE_PRECISION, Nproc_x*Nproc_y*Nproc_z+ master_lat*(Nproc_lat- 1)+ node, tag, CommRADIATION, ierr)
					call MPI_Send(rho_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, Nproc_x*Nproc_y*Nproc_z+ master_lat*(Nproc_lat- 1)+ node, tag+ 10, CommRADIATION, ierr)
					call MPI_Send(cv_cell , Nx*Ny*Nz, MPI_DOUBLE_PRECISION, Nproc_x*Nproc_y*Nproc_z+ master_lat*(Nproc_lat- 1)+ node, tag+ 20, CommRADIATION, ierr)
					call MPI_Send(tmp_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, Nproc_x*Nproc_y*Nproc_z+ master_lat*(Nproc_lat- 1)+ node, tag+ 30, CommRADIATION, ierr)
					call MPI_Send(slr_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, Nproc_x*Nproc_y*Nproc_z+ master_lat*(Nproc_lat- 1)+ node, tag+ 40, CommRADIATION, ierr)
				enddo
			else
				call MPI_Recv(emi     , Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag, CommRADIATION, status, ierr)
				call MPI_Recv(rho_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag+ 10, CommRADIATION, status, ierr)
				call MPI_Recv(cv_cell , Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag+ 20, CommRADIATION, status, ierr)
				call MPI_Recv(tmp_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag+ 30, CommRADIATION, status, ierr)
				call MPI_Recv(slr_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag+ 40, CommRADIATION, status, ierr)
			endif
		endif
	else
		! 外部プロセス (POPCORN , JPITER とのやりとり)
		call mpi_communication_with_other_procs(master_lat, isub, iflg, it, dt)
	endif

	if(iflg .eq. 1) exit ! 外部プロセスから, 終了フラグが送られたら時間ループを脱出する
#endif

	! 他領域部分の射出率
	call mpi_pack_x_dble(emi)
	call mpi_isend_irecv_x_dble
	call mpi_unpack_x_dble(emi)
!write(6, *) "Radiation(emi boundary send/recv x) ... OK", myrank !DEBUG
	
	if(ndim .eq. 3) then
		call mpi_pack_y_dble(emi)
		call mpi_isend_irecv_y_dble
		call mpi_unpack_y_dble(emi)
!write(6, *) "Radiation(emi boundary send/recv y) ... OK", myrank !DEBUG
	endif
	
	call mpi_pack_z_dble(emi)
	call mpi_isend_irecv_z_dble
	call mpi_unpack_z_dble(emi)
!write(6, *) "Radiation(emi boundary send/recv z) ... OK", myrank !DEBUG

	! 他領域部分の比熱
	call mpi_pack_x_dble(cv_cell)
	call mpi_isend_irecv_x_dble
	call mpi_unpack_x_dble(cv_cell)
!write(6, *) "Radiation(cv_cell boundary send/recv x) ... OK", myrank !DEBUG
	
	if(ndim .eq. 3) then
		call mpi_pack_y_dble(cv_cell)
		call mpi_isend_irecv_y_dble
		call mpi_unpack_y_dble(cv_cell)
!write(6, *) "Radiation(cv_cell boundary send/recv y) ... OK", myrank !DEBUG
	endif
	
	call mpi_pack_z_dble(cv_cell)
	call mpi_isend_irecv_z_dble
	call mpi_unpack_z_dble(cv_cell)
!write(6, *) "Radiation(cv_cell boundary send/recv z) ... OK", myrank !DEBUG

	! 他領域部分の密度
	call mpi_pack_x_dble(rho_cell)
	call mpi_isend_irecv_x_dble
	call mpi_unpack_x_dble(rho_cell)
!write(6, *) "Radiation(rho_cell boundary send/recv x) ... OK", myrank !DEBUG
	
	if(ndim .eq. 3) then
		call mpi_pack_y_dble(rho_cell)
		call mpi_isend_irecv_y_dble
		call mpi_unpack_y_dble(rho_cell)
!write(6, *) "Radiation(rho_cell boundary send/recv y) ... OK", myrank !DEBUG
	endif
	
	call mpi_pack_z_dble(rho_cell)
	call mpi_isend_irecv_z_dble
	call mpi_unpack_z_dble(rho_cell)
!write(6, *) "Radiation(rho_cell boundary send/recv z) ... OK", myrank !DEBUG

	! 他領域部分の固相率
	call mpi_pack_x_dble(slr_cell)
	call mpi_isend_irecv_x_dble
	call mpi_unpack_x_dble(slr_cell)
!write(6, *) "Radiation(slr_cell boundary send/recv x) ... OK", myrank !DEBUG
	
	if(ndim .eq. 3) then
		call mpi_pack_y_dble(slr_cell)
		call mpi_isend_irecv_y_dble
		call mpi_unpack_y_dble(slr_cell)
!write(6, *) "Radiation(slr_cell boundary send/recv y) ... OK", myrank !DEBUG
	endif
	
	call mpi_pack_z_dble(slr_cell)
	call mpi_isend_irecv_z_dble
	call mpi_unpack_z_dble(slr_cell)
!write(6, *) "Radiation(slr_cell boundary send/recv z) ... OK", myrank !DEBUG

	! 法線ベクトルの計算
	call calc_normal_vector(ndim)
	! 法線ベクトルの境界値を渡すのは, ちょっとあとで考える (0 があるため)
	call set_boundary_normal_vector(ndim)
!write(6, *) "Radiation(normal_vector) ... OK", myrank !DEBUG

	!セル状態の特定
	call cell_identify(ndim, isub)
	call set_boundary_cell_state(ndim, isub)
!write(6, *) "Radiation(cell_identify) ... OK", myrank !DEBUG

	! 温度の境界条件
	call Set_Boundary_tmp_cell(ndim)
!write(6, *) "Radiation(tmp_cell_boundary) ... OK", myrank !DEBUG

	! 次の時間ステップの輻射場を計算する際に更新する配列の初期化
	!$omp parallel do private(i, j)
	do k= 1, Nz
		do j= 1, Ny
			do i= 1, Nx
				dE_cell(i, j, k)= 0.0d0
				E_cell(i, j, k)= 0.0d0
				dtmp_cell(i, j, k)= 0.0d0
				tmp_cell_prev(i, j, k)= tmp_cell(i, j, k) ! 前の時間ステップ(≠反復ステップ)の温度
			enddo
		enddo
	enddo
	!$omp end parallel do

	
	do picard= 1, picard_max

#ifdef MPI_MODE
!		if(mpisize-master_rad .gt. 1) then	 !popcornとのやりとりはない
		if(isub .eq. 0) then
			! 角度方向のマスターなら, 角度方向のスレーブに tmp_cell & E_cell を送信
			if(myrank .eq. master_lat) then
				do node= 0, Nproc_lat- 2
					call MPI_Send(tmp_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, Nproc_x*Nproc_y*Nproc_z+ master_lat*(Nproc_lat- 1)+ node, tag, CommRADIATION, ierr)
					call MPI_Send(  E_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, Nproc_x*Nproc_y*Nproc_z+ master_lat*(Nproc_lat- 1)+ node, tag+ 10, CommRADIATION, ierr)
				enddo
			else
				call MPI_Recv(tmp_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag, CommRADIATION, status, ierr)
				call MPI_Recv(  E_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag+ 10, CommRADIATION, status, ierr)
			endif
		else if(isub .eq. 1) then
			! POPCORN との通信 : 要見直し
		else
			! JUPITER との通信時
			! 角度方向のマスターなら, 角度方向のスレーブに tmp_cell & E_cell を送信
			if(myrank .eq. master_lat) then
				do node= 0, Nproc_lat- 2
					call MPI_Send(tmp_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, Nproc_x*Nproc_y*Nproc_z+ master_lat*(Nproc_lat- 1)+ node, tag, CommRADIATION, ierr)
					call MPI_Send(  E_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, Nproc_x*Nproc_y*Nproc_z+ master_lat*(Nproc_lat- 1)+ node, tag+ 10, CommRADIATION, ierr)
				enddo
			else
				call MPI_Recv(tmp_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag, CommRADIATION, status, ierr)
				call MPI_Recv(  E_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag+ 10, CommRADIATION, status, ierr)
			endif
		endif
#endif

		!反復初期値の設定
		if(myrank .eq. master_lat) then
			!$omp parallel do private(i, j)
			do k= 1, Nz
				do j= 1, Ny
					do i= 1, Nx
						ads_tot(i, j, k)= 0.0d0
						dtmp_cell(i, j, k)= -tmp_cell(i, j, k)
						dE_cell(i, j, k)= -E_cell(i, j, k)
					enddo
				enddo
			enddo
			!$omp end parallel do
		else
			!$omp parallel do private(i, j)
			do k= 1, Nz
				do j= 1, Ny
					do i= 1, Nx
						ads_tot(i, j, k)= 0.0d0
						dtmp_cell(i, j, k)= -tmp_cell(i, j, k)
						dE_cell(i, j, k)= 0.0d0
					enddo
				enddo
			enddo
			!$omp end parallel do
		endif

! go to 9999 ! DEBUG (ここは OK)
		! まず, 輻射輝度を計算
		call ray_trace(it, ndim, ibgn, iend) ! dE(:, :, :) を更新する
! go to 9999 ! DEBUG

#ifdef MPI_MODE
		if(isub .eq. 0) then
			! 角度方向のマスターなら, 角度方向のスレーブから dE_cell & ads_tot を受信
			if(myrank .eq. master_lat) then
				! dE_cell の総和
				do node= 0, Nproc_lat- 2
					call MPI_Recv(mpi_buffer, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, Nproc_x*Nproc_y*Nproc_z+ master_lat*(Nproc_lat- 1)+ node, tag, CommRADIATION, status, ierr)
					call add_array_double(Nx*Ny*Nz, mpi_buffer, dE_cell)
				enddo
				
				! ads_tot の総和
				do node= 0, Nproc_lat- 2
					call MPI_Recv(mpi_buffer, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, Nproc_x*Nproc_y*Nproc_z+ master_lat*(Nproc_lat- 1)+ node, tag, CommRADIATION, status, ierr)
					call add_array_double(Nx*Ny*Nz, mpi_buffer, ads_tot)
				enddo
			else
				call MPI_Send(dE_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag, CommRADIATION, ierr)
				call MPI_Send(ads_tot, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag, CommRADIATION, ierr)
			endif
		else if(isub .eq. 1) then
			! POPCORN との通信 : 要見直し
		else
			! JUPITER との通信時
			! 角度方向のマスターなら, 角度方向のスレーブから dE_cell & ads_tot を受信
			if(myrank .eq. master_lat) then
				! dE_cell の総和
				do node= 0, Nproc_lat- 2
					call MPI_Recv(mpi_buffer, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, Nproc_x*Nproc_y*Nproc_z+ master_lat*(Nproc_lat- 1)+ node, tag, CommRADIATION, status, ierr)
					call add_array_double(Nx*Ny*Nz, mpi_buffer, dE_cell)
				enddo
				
				! ads_tot の総和
				do node= 0, Nproc_lat- 2
					call MPI_Recv(mpi_buffer, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, Nproc_x*Nproc_y*Nproc_z+ master_lat*(Nproc_lat- 1)+ node, tag, CommRADIATION, status, ierr)
					call add_array_double(Nx*Ny*Nz, mpi_buffer, ads_tot)
				enddo
			else
				call MPI_Send(dE_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag, CommRADIATION, ierr)
				call MPI_Send(ads_tot, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag, CommRADIATION, ierr)
			endif
		endif
#endif


#ifdef MPI_MODE
		!温度更新や反復計算の終了判定:角度マスターが行う→輻射マスターが全領域の最大誤差監視
		if(myrank .eq. master_lat) then
			E_cell_max= 1.0d0
			E_cell_err= 0.0d0
			!$omp parallel do private(i, j) reduction(max: E_cell_max, E_cell_err)
			do k= 1, Nz
				do j= 1, Ny
					do i= 1, Nx
						E_cell(i, j, k)= E_cell(i, j, k)+ dE_cell(i, j, k)        !< Ir の総和

						if(E_cell_max .lt. abs(E_cell(i, j, k))) E_cell_max= abs(E_cell(i, j, k))
						if(E_cell_err .lt. abs(dE_cell(i, j, k))) E_cell_err= abs(dE_cell(i, j, k))
					enddo 
				enddo
			enddo
			!$omp end parallel do
			if(E_cell_err .gt. 1.0d-15) E_cell_err= E_cell_err/E_cell_max


			! 温度を計算 (角度方向マスター)
			call newton_loop(ndim, newton, dt, dtmp_cell_err)  ! tmp_cell(:, :, :) を更新する
			tmp_cell_max= 0.0d0
			tmp_cell_err= 0.0d0
			call Set_Boundary_tmp_cell(ndim)


			!$omp parallel do private(i, j) reduction(max: tmp_cell_max, tmp_cell_err)
			do k= 1, Nz
				do j= 1, Ny
					do i= 1, Nx
						dtmp_cell(i, j, k)= tmp_cell(i, j, k)+ dtmp_cell(i, j, k) !< 前の反復ステップの温度と次の反復ステップの温度の差 (dtmp_cell(:, :, :) は初期値 -tmp_cell(:, :, :) と設定)
						if(tmp_cell_max .lt. tmp_cell(i, j, k)) tmp_cell_max= tmp_cell(i, j, k)
						if(tmp_cell_err .lt. dabs(dtmp_cell(i, j, k))) tmp_cell_err= dabs(dtmp_cell(i, j, k))
					enddo 
				enddo
			enddo
			!$omp end parallel do
			if(tmp_cell_err .gt. 1.0d-15) tmp_cell_err= tmp_cell_err/tmp_cell_max
		endif


		! Picard 反復の E, tmp の誤差が閾値未満でループを抜ける(判定処理は輻射マスターのみが実行)
		if(myrank .eq. 0) then ! 輻射コミュニケーターのみのグループでは, 輻射マスターの値は常に 0
			do node= 1, Nproc_x*Nproc_y*Nproc_z- 1
				call MPI_Recv(mpi_buffer, 3, MPI_DOUBLE_PRECISION, node, tag, CommRADIATION, status, ierr)
				if(dabs(E_cell_err) .lt. dabs(mpi_buffer(1)))  E_cell_err= mpi_buffer(1)
				if(dabs(tmp_cell_err) .lt. dabs(mpi_buffer(2)))  tmp_cell_err= mpi_buffer(2)
				if(dabs(dtmp_cell_err) .lt. dabs(mpi_buffer(3)))  dtmp_cell_err= mpi_buffer(3)
			enddo
		else if(myrank .eq. master_lat) then
			mpi_buffer(1)= E_cell_err
			mpi_buffer(2)= tmp_cell_err
			mpi_buffer(3)= dtmp_cell_err
			call MPI_Send(mpi_buffer, 3, MPI_DOUBLE_PRECISION, 0, tag, CommRADIATION, ierr)
		endif
		
		if(myrank .eq. 0) then ! 輻射コミュニケーターのみのグループでは, 輻射マスターの値は常に 0
			iexit = 0
			if((tmp_cell_err .lt. tmp_cell_err_max) .and. (E_cell_err .lt. E_cell_err_max)) then
				iexit = 1
			endif

			do node= 1, Nproc_x*Nproc_y*Nproc_z*Nproc_lat- 1
				call MPI_Send(iexit, 1, MPI_INTEGER, node, tag, CommRADIATION, ierr)
			enddo			
		else
			call MPI_Recv(iexit, 1, MPI_INTEGER, 0, tag, CommRADIATION, status, ierr)
		endif
#endif

!9999 continue ! DEBUG
		if(iexit.gt.0) then
			if(myrank .eq. 0) then ! 輻射コミュニケーターのみのグループでは, 輻射マスターの値は常に 0
			    write(LogStr(1), '((a))') "--------------------------------------------------"
			    write(LogStr(2), '((a), (E10.3), (a9), (I10), (a1))') "Elapsed time: ", dble(it)*dt, ' (iter.= ', it, ')'
			    write(LogStr(3), '(2((a), (I5)))') "Picard loop: ", picard, ", Newton loop: ", newton
			    write(LogStr(4), '((a))')"Convergence: "
			    write(LogStr(5), '((a), (E12.5), (a1), (E9.2))') "E_cell   : ", E_cell_err, "/", E_cell_err_max
			    write(LogStr(6), '((a), (E12.5), (a1), (E9.2))') "tmp_cell : ", tmp_cell_err, "/", tmp_cell_err_max
			    write(LogStr(7), '((a), (E12.5), (a1), (E9.2))') "dtmp_cell: ", dtmp_cell_err, "/", dtmp_cell_err_max
			    write(LogStr(8), '((a))') "--------------------------------------------------"
			    call output_exec_log_rad(isub)
            endif

			exit
        else
			if(myrank .eq. 0) then ! 輻射コミュニケーターのみのグループでは, 輻射マスターの値は常に 0
			    if((picard .lt. 11) .or. (mod(picard, picard_out) .eq. 0)) then
				    write(LogStr(1), '((a))') "--------------------------------------------------"
				    write(LogStr(2), '((a), (E10.3), (a9), (I10), (a1))') "Elapsed time: ", dble(it)*dt, ' (iter.= ', it, ')'
				    write(LogStr(3), '(2((a), (I5)))') "Picard loop: ", picard, ", Newton loop: ", newton
				    write(LogStr(4), '((a))')"Convergence: "
				    write(LogStr(5), '((a), (E12.5), (a1), (E9.2))') "E_cell   : ", E_cell_err, "/", E_cell_err_max
				    write(LogStr(6), '((a), (E12.5), (a1), (E9.2))') "tmp_cell : ", tmp_cell_err, "/", tmp_cell_err_max
				    write(LogStr(7), '((a), (E12.5), (a1), (E9.2))') "dtmp_cell: ", dtmp_cell_err, "/", dtmp_cell_err_max
				    write(LogStr(8), '((a))') "--------------------------------------------------"
    			    call output_exec_log_rad(isub)
			    endif
            endif    
		endif
	enddo ! Picard 反復の終端
	

	if(myrank .eq. 0) then ! 輻射コミュニケーターのみのグループでは, 輻射マスターの値は常に 0
		! 収束しなかった場合
		if(picard .gt. picard_max) then
			write(LogStr(1), '((a))') "!!!!! The radiation field was not converged !!!!!"
			write(LogStr(2), '((a), (E10.3), (a9), (I10), (a1))') "Elapsed time: ", dble(it)*dt, ' (iter.= ', it, ')'
			write(LogStr(3), '(2((a), (I5)))') "Picard loop: ", picard, ", Newton loop: ", newton
			write(LogStr(4), '((a))')"Convergence: "
			write(LogStr(5), '((a), (E12.5), (a1), (E9.2))') "E_cell   : ", E_cell_err, "/", E_cell_err_max
			write(LogStr(6), '((a), (E12.5), (a1), (E9.2))') "tmp_cell : ", tmp_cell_err, "/", tmp_cell_err_max
			write(LogStr(7), '((a), (E12.5), (a1), (E9.2))') "dtmp_cell: ", dtmp_cell_err, "/", dtmp_cell_err_max
			write(LogStr(8), '((a))') "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
    		call output_exec_log_rad(isub)
		endif
	endif

    ! 結果出力
	if(myrank .lt. Nproc_x*Nproc_y*Nproc_z .and. output_result_data_interval .gt. 0) then
        if(mod(it, output_result_data_interval) .eq. 0) then
			! データ出力機番は, プロセスごとに違っていたほうがよい
!            call output_result_data(ndim, nump, 100+ myrank, it, dt, xyz_range, out_binary_buffer, result_file_location)
        endif
    endif

    ! デバッグ用 (MS Windows 環境)
!	if(myrank.eq.0) then
!		call TimeCounterQPC(2, "radiation_loop")
!		call TimeCounterQPC(0, "output")
!	endif
	if(isub .eq. 0) then
		call MPI_Barrier(CommRADIATION, ierr) ! なぜかわからないが, JUPITER 連成時にここで止まってしまう
	endif
!write(LogStr(1), '((a), (I3))') "Rdiation End of loop: ", myrank_glob ! DEBUG
!call output_exec_log_rad(isub) ! DEBUG
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
! ここまでを無限ループとする(POPCORN/JUPITER 側からの終了指示待ち)
enddo
!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

if(myrank.eq.0) then
	write(LogStr(1), '((a))') "--------------------------------------------------"
	write(LogStr(2), '((a))') "radiation main loop is normal end..."
	write(LogStr(3), '((a))') "--------------------------------------------------"
	call output_exec_log_rad(isub)

    ! デバッグ用 (MS Windows 環境)
!    call TimeCounterQPC(1, "radiation")
!    call TimeCounterQPC(0, "output")
endif

	! -------------------------------------------------------------- MPI
#ifdef MPI_MODE
	call MPI_Finalize (ierr) ! プロセスの終処理
#endif
	! -------------------------------------------------------------- MPI
stop
end

