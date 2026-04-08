! 独立動作時の設定条件ファイルを読み取るサブルーチン
subroutine set_radiation_configuration(ndim, nump, numm, maxloop, output_result_data_interval, xyz_range, dt, config_file_name, init_file_location, init_file_name, result_file_location)

	use MODULE_STRUCT
	use MODULE_RADIATION

	implicit none
	type(max_min):: xyz_range(3)

	integer :: ndim, nump, numm, maxloop, output_result_data_interval
    character(*) :: config_file_name, init_file_location, init_file_name, result_file_location
    real(8):: dt

	integer:: i= 0, j= 0, k= 0, m= 0, n= 0
	real(8):: lat= 0.0d0, lon= 0.0d0

	! 解析の次元
	ndim= 3
		
	! 各方向の領域分割数 (MPI が有効でない場合は, 適当な値で可)
	Nproc_lat= 1 ! 角度方向
	Nproc_x= 2   ! x 方向の領域分割数
	Nproc_y= 2   ! y 方向の領域分割数
	Nproc_z= 2   ! z 方向の領域分割数
	
	result_file_location= "/home/g8/f160008/jupi_radi00/data/" ! 結果ファイル (*.vtk) を保存するディレクトリ (最後に必ず / をつける)
!	result_file_location= "C:/mpi_work/data/" ! 結果ファイル (*.vtk) を保存するディレクトリ (最後に必ず / をつける)

	dt= 1.0d-03                      ! 時間ステップ幅
	maxloop= 5000                    ! 時間繰り返し回数
	output_result_data_interval= 10  ! 結果ファイル (*.vtk) 出力間隔

	xyz_range(1)%min= 0.0d0          ! x 方向の解析領域の下限
	xyz_range(1)%max= 5.0d0          ! x 方向の解析領域の上限
	xyz_range(2)%min= 0.0d0          ! y 方向の解析領域の下限
	xyz_range(2)%max= 5.0d0          ! y 方向の解析領域の上限
	xyz_range(3)%min= 0.0d0          ! z 方向の解析領域の下限
	xyz_range(3)%max= 5.0d0          ! z 方向の解析領域の上限

	Nx= 7                            ! x 方向のセル数 (前後の仮想セル: 2セルを含む)
	Ny= 7                            ! y 方向のセル数 (前後の仮想セル: 2セルを含む) < 2D の場合は, 以下で強制的に 3 に書き換えられる
	Nz= 7                            ! z 方向のセル数 (前後の仮想セル: 2セルを含む)

	N_lat= 12                        ! 緯度 (0≦θ≦2π) 方向の分割数
	N_lon= 6                         ! 経度 (0≦φ≦ π) 方向の分割数 < 2D の場合は, 以下で強制的に 1 に書き換えられる
	
	picard_max= 500                  ! 輻射輝度 I と温度 T の連成の最大反復回数
	picard_out= 50                   ! 輻射輝度 I と温度 T の連成の反復計算の収束状況表示間隔 (最初の 10 回は毎回出力, 10回目以降の出力間隔)
	newton_max= 50                   ! 温度 T の非線形方程式の Newton 法の最大反復回数

	E_cell_err_max=    1.0d-05       ! 輻射場の収束判定誤差
	tmp_cell_err_max=  1.0d-05       ! 温度場の収束判定誤差
	dtmp_cell_err_max= 1.0d-07       ! 温度の Newton 法の収束判定誤差
	
	I_bcd_flg= 100000                ! 境界条件フラグ (6 桁の整数: 6 桁目から順に, x 方向始点, x 方向終点, y 方向始点, y 方向終点, z 方向始点, z 方向終点)
	                                 ! 0: 輻射場の自由流出, 1: 温度一定の壁, 2: 断熱壁面
	
	if(ndim .eq. 2) then
		N_lon= 1
		d_lat= 2.0d0*M_PI/dble(N_lat)
		! 2D の場合に, d_lon= 1.0 , lon= M_PI/2.0 とすることで, 
		! 2D の場合でも dΩ = sin(lon)*d_lat*d_lon の形式を使いまわせる
		d_lon= 1.0d0

		Ny= 3
        xyz_range(2)%min= 0.0d0
        xyz_range(2)%max= 1.0d0
	else if(ndim .eq. 3) then
		d_lat= 2.0d0*M_PI/dble(N_lat)
		d_lon=       M_PI/dble(N_lon)
		
		! d_lon 間隔で0 deg. 〜 180 deg. まで積分するため, 3D の場合は, N_lon = ユーザー入力値 + 1
		N_lon= N_lon+ 1
	else
		stop "!!! Invalid dimension setting !!!"
	endif

	dx= (xyz_range(1)%max- xyz_range(1)%min)/dble(Nx- 2)
	dy= (xyz_range(2)%max- xyz_range(2)%min)/dble(Ny- 2)
	dz= (xyz_range(3)%max- xyz_range(3)%min)/dble(Nz- 2)

	return 
end