subroutine ray_trace(it, ndim, ibgn, iend)

	!$ use omp_lib
	use MODULE_STRUCT
	use MODULE_RADIATION !Ir(:, :, :), dIr(:, :, :), 
	                      !Nx, Ny, Nz, N_lat, N_lon, emi(:, :, :) (射出率: emissivity), Source_rad_part(:), Source_rad_cell(:, :, :)

	implicit none

	include 'parameter.fi'
	include 'mpif.h'

	integer:: it, nump, ndim ! 引数リスト
	integer :: ibgn, iend !分散並列用に引数追加

	integer:: ierr= 0
	integer:: iter= 0, iter_fin= 0, int_seq= 0
	integer:: i= 0, j= 0, k= 0, m= 0, n= 0
	real(8):: dt= 0.0d0, lat= 0.0d0, lon= 0.0d0, ads= 0.0d0
	real(8), parameter:: CFL_lim= 0.9d0
	character(256):: input_env_var
	integer, save:: count= 1
	real(8):: integration_normalizer= 0.0d0 ! Ir を角度方向に積分して, E を計算するときの規格化定数 (Integration normalizer)
	real(8):: norm_cell= 0.0d0, norm_x_cell= 0.0d0, norm_y_cell= 0.0d0, norm_z_cell= 0.0d0
	real(8):: l_eff= 0.0d0
	real(8):: orthogonal_threshold= 0.0d0 ! 角度分割の精度上, 絶対値がこれより小さい内積の角度は, 直交していると判断する閾値


	! 積分時の規格化定数を計算しておく& 直交内積の閾値を計算しておく
	integration_normalizer= 0.0d0
	if(ndim .eq. 2) then
		do m= 1, N_lat
			integration_normalizer= integration_normalizer+ d_lat
		enddo
		integration_normalizer= 2.0d0*M_PI/integration_normalizer

		orthogonal_threshold= sin(d_lat)
	else
		do m= 1, N_lat
			lon= -d_lon ! 0 deg. 〜 180 deg. まで d_lon 間隔で積分する
			do n= 1, N_lon  ! d_lon 間隔で0 deg. 〜 180 deg. まで積分するため, 3D の場合は, N_lon = ユーザー入力値 + 1 (set_radiation_configuration で 1 足している)
				lon= lon+ d_lon
				integration_normalizer= integration_normalizer+ sin(lon)*d_lat*d_lon
			enddo
		enddo
		integration_normalizer= 4.0d0*M_PI/integration_normalizer

		orthogonal_threshold= sin(d_lat)
		if(orthogonal_threshold .gt. sin(d_lon)) orthogonal_threshold= sin(d_lon)
	endif

	lat= (dble(ibgn)- 1.5d0) * d_lat
	do m= ibgn, iend		!< θ (緯度 latitude)  方向ループ
		lat= lat+ d_lat

		lon= -d_lon
		do n= 1, N_lon  !< φ (経度 longitude) 方向ループ ： d_lon 間隔で0 deg. 〜 180 deg. まで積分するため, 3D の場合は, N_lon = ユーザー入力値 + 1 (set_radiation_configuration で 1 足している)
			lon= lon+ d_lon
			! 2D の場合に, d_lon= 1.0 , lon= M_PI/2.0 とすることで, 
			! 2D の場合でも dΩ = sin(lon)*d_lat*d_lon の形式を使いまわせる
			if(ndim .eq. 2) lon= M_PI/2.0d0

			!$omp parallel do private(i, j)
			do k= 1, Nz
				do j= 1, Ny
					do i= 1, Nx
						Ir(i, j, k)= 0.0d0
						dIr(i, j, k)= 0.0d0
					enddo
				enddo
			enddo
			!$omp end parallel do


			!仮想時間刻み幅決定 (着目方向余弦に対して CFL= CFL_lim となるように)
			dt= 0.0d0
			if(abs(cos(lat)*sin(lon))/dx .ge. abs(sin(lat)*sin(lon))/dz) then
				if(abs(cos(lat)*sin(lon))/dx .ge. abs(cos(lon))/dy) then
					dt= CFL_lim*dx/abs(cos(lat)*sin(lon))
				else
					dt= CFL_lim*dy/abs(cos(lon))
				endif
			else
				if(abs(sin(lat)*sin(lon))/dz .ge. abs(cos(lon))/dy) then
					dt= CFL_lim*dz/abs(sin(lat)*sin(lon))
				else
					dt= CFL_lim*dy/abs(cos(lon))
				endif
			endif

			!時間繰り返し回数決定
			iter_fin= int( ( 2.0d0*dble((Nx- 2)*Nproc_x+ 3)*dx*abs(cos(lat)*sin(lon)) &
				           + 2.0d0*dble((Ny- 2)*Nproc_y+ 3)*dy*abs(cos(lon)) &
				           + 2.0d0*dble((Nz- 2)*Nproc_z+ 3)*dz*abs(sin(lat)*sin(lon)) )/dt )+ 1


			!仮想時間発展
			int_seq= 1
			call set_boundary_radiation(ndim, m, n, lat, lon, dt, orthogonal_threshold)
			do iter= 1, iter_fin+ 1

				if(ndim .eq. 2) then
					if(mod(iter, 2) .eq. 0) then
						call upwind_x(lat, lon, dt)
						call set_boundary_radiation(ndim, m, n, lat, lon, dt, orthogonal_threshold)
						call upwind_z(lat, lon, dt)
						call set_boundary_radiation(ndim, m, n, lat, lon, dt, orthogonal_threshold)
					else
						call upwind_z(lat, lon, dt)
						call set_boundary_radiation(ndim, m, n, lat, lon, dt, orthogonal_threshold)
						call upwind_x(lat, lon, dt)
						call set_boundary_radiation(ndim, m, n, lat, lon, dt, orthogonal_threshold)
					endif
				else
					! 以下の, 方向分離型移流計算手順の参考：http://www.ida.upmc.fr/~zaleski/codes/legacy_codes.html
					if(int_seq .eq. 1) then
						call upwind_x(lat, lon, dt)
						call set_boundary_radiation(ndim, m, n, lat, lon, dt, orthogonal_threshold)
						call upwind_y(lat, lon, dt)
						call set_boundary_radiation(ndim, m, n, lat, lon, dt, orthogonal_threshold)
						call upwind_z(lat, lon, dt)
						call set_boundary_radiation(ndim, m, n, lat, lon, dt, orthogonal_threshold)
					else if(int_seq .eq. 2) then
						call upwind_z(lat, lon, dt)
						call set_boundary_radiation(ndim, m, n, lat, lon, dt, orthogonal_threshold)
						call upwind_x(lat, lon, dt)
						call set_boundary_radiation(ndim, m, n, lat, lon, dt, orthogonal_threshold)
						call upwind_y(lat, lon, dt)
						call set_boundary_radiation(ndim, m, n, lat, lon, dt, orthogonal_threshold)
					else
						call upwind_y(lat, lon, dt)
						call set_boundary_radiation(ndim, m, n, lat, lon, dt, orthogonal_threshold)
						call upwind_z(lat, lon, dt)
						call set_boundary_radiation(ndim, m, n, lat, lon, dt, orthogonal_threshold)
						call upwind_x(lat, lon, dt)
						call set_boundary_radiation(ndim, m, n, lat, lon, dt, orthogonal_threshold)
					endif

					int_seq= int_seq+ 1
					if(int_seq .gt. 3) int_seq= 1
				endif

				!$omp parallel do private(i, j, ads, l_eff, norm_cell, norm_x_cell, norm_y_cell, norm_z_cell)
				do k= 1, Nz
					do j= 1, Ny
						do i= 1, Nx
							if(cell_state(i, j, k) .eq. -1) then
								! 気体中の吸収・発熱を考慮
								ads= emi(i, j, k)*l_opt(m, n)
								Ir(i, j, k)= Ir(i, j, k)+ dt*ads*stefan_boltzmann*tmp_cell(i, j, k)*tmp_cell(i, j, k)*tmp_cell(i, j, k)*tmp_cell(i, j, k)/intensity_distribution_factor- dt*ads*Ir(i, j, k)
							else if(cell_state(i, j, k) .ge. 0) then
								! セル中心の単位法線ベクトルを計算する
								! 2D の場合には, norm_y(:, :, :)= 0.0d0 が保証されている
								norm_x_cell= 0.125d0*( norm_x(i-1, j-1, k-1)+ norm_x(i-1, j-1, k)+ norm_x(i-1, j, k-1)+ norm_x(i-1, j, k) &
													 + norm_x(i, j-1, k-1)  + norm_x(i, j-1, k)  + norm_x(i, j, k-1)  + norm_x(i, j, k) )
								norm_y_cell= 0.125d0*( norm_y(i-1, j-1, k-1)+ norm_y(i-1, j-1, k)+ norm_y(i-1, j, k-1)+ norm_y(i-1, j, k) &
													 + norm_y(i, j-1, k-1)  + norm_y(i, j-1, k)  + norm_y(i, j, k-1)  + norm_y(i, j, k) )
								norm_z_cell= 0.125d0*( norm_z(i-1, j-1, k-1)+ norm_z(i-1, j-1, k)+ norm_z(i-1, j, k-1)+ norm_z(i-1, j, k) &
													 + norm_z(i, j-1, k-1)  + norm_z(i, j-1, k)  + norm_z(i, j, k-1)  + norm_z(i, j, k) )
								norm_cell= sqrt(norm_x_cell*norm_x_cell+ norm_y_cell*norm_y_cell+ norm_z_cell*norm_z_cell)

								! セル中心における大きさが, DBL_EPSILON 未満は固体内部とみなす (法線ベクトル 0.0)
								if(norm_cell .lt. DBL_EPSILON) then
									norm_x_cell= 0.0d0
									norm_y_cell= 0.0d0
									norm_z_cell= 0.0d0
									norm_cell= 0.0d0
								else
									norm_x_cell= norm_x_cell/norm_cell
									norm_y_cell= norm_y_cell/norm_cell
									norm_z_cell= norm_z_cell/norm_cell
								endif

								! 固体中の発熱を考慮
								ads= 0.0d0
								if(norm_x_cell*cos(lat)*sin(lon)+ norm_y_cell*cos(lon)+ norm_z_cell*sin(lat)*sin(lon) .gt. orthogonal_threshold) then
									ads= ( (emi(i, j, k)*stefan_boltzmann*tmp_cell(i, j, k)*tmp_cell(i, j, k)*tmp_cell(i, j, k)*tmp_cell(i, j, k)/(slr_cell(i, j, k)*intensity_distribution_factor)) &
										 - Ir(i, j, k) )/dt
									Ir(i, j, k)= Ir(i, j, k)+ ads*dt
								endif
							endif

							if(Ir(i, j, k) .lt. DBL_EPSILON) Ir(i, j, k)= 0.0d0
						enddo
					enddo
				enddo
				!$omp end parallel do 

				call set_boundary_radiation(ndim, m, n, lat, lon, dt, orthogonal_threshold)
			enddo !< 仮想時間発展ここまで

			!Ir の総和の増分 & 固体表面の内積の積分値計算 (仮想セルの値も積分する: dE_cell(:, :, :) は初期値: -E_cell(:, :, :) と設定 in program radiation )
			!$omp parallel do private(i, j, ads, l_eff, norm_cell, norm_x_cell, norm_y_cell, norm_z_cell)
			do k= 1, Nz
				do j= 1, Ny
					do i= 1, Nx
						if(cell_state(i, j, k) .eq. -1) then
							! 気体
							ads_tot(i, j, k)= ads_tot(i, j, k)+ emi(i, j, k)*l_opt(m, n)*integration_normalizer*sin(lon)*d_lat*d_lon
							dE_cell(i, j, k)= dE_cell(i, j, k)+ emi(i, j, k)*l_opt(m, n)*Ir(i, j, k)*integration_normalizer*sin(lon)*d_lat*d_lon
						else if(cell_state(i, j, k) .eq. 0) then
							! セル中心の単位法線ベクトルを計算する
							! 2D の場合には, norm_y(:, :, :)= 0.0d0 が保証されている
							norm_x_cell= 0.125d0*( norm_x(i-1, j-1, k-1)+ norm_x(i-1, j-1, k)+ norm_x(i-1, j, k-1)+ norm_x(i-1, j, k) &
												 + norm_x(i,   j-1, k-1)+ norm_x(i,   j-1, k)+ norm_x(i,   j, k-1)+ norm_x(i,   j, k) )
							norm_y_cell= 0.125d0*( norm_y(i-1, j-1, k-1)+ norm_y(i-1, j-1, k)+ norm_y(i-1, j, k-1)+ norm_y(i-1, j, k) &
												 + norm_y(i,   j-1, k-1)+ norm_y(i,   j-1, k)+ norm_y(i,   j, k-1)+ norm_y(i,   j, k) )
							norm_z_cell= 0.125d0*( norm_z(i-1, j-1, k-1)+ norm_z(i-1, j-1, k)+ norm_z(i-1, j, k-1)+ norm_z(i-1, j, k) &
												 + norm_z(i,   j-1, k-1)+ norm_z(i,   j-1, k)+ norm_z(i,   j, k-1)+ norm_z(i,   j, k) )
							norm_cell= sqrt(norm_x_cell*norm_x_cell+ norm_y_cell*norm_y_cell+ norm_z_cell*norm_z_cell)

							! セル中心における大きさが, 1.0e-16 未満は固体内部とみなす (法線ベクトル 0.0)
							if(norm_cell .lt. DBL_EPSILON) then
								norm_x_cell= 0.0d0
								norm_y_cell= 0.0d0
								norm_z_cell= 0.0d0
								norm_cell= 0.0d0
							else
								norm_x_cell= norm_x_cell/norm_cell
								norm_y_cell= norm_y_cell/norm_cell
								norm_z_cell= norm_z_cell/norm_cell
							endif

							! 固体
							! 2D の場合は, dy= 1.0d0
							l_eff= (dabs(norm_x_cell*cos(lat)*sin(lon))*dy*dz+ dabs(norm_y_cell*cos(lon))*dz*dx+ dabs(norm_z_cell*sin(lat)*sin(lon))*dx*dy)/(slr_cell(i, j, k)*dx*dy*dz)

							if(norm_x_cell*cos(lat)*sin(lon)+ norm_y_cell*cos(lon)+ norm_z_cell*sin(lat)*sin(lon) .gt. orthogonal_threshold) then
								ads_tot(i, j, k)= ads_tot(i, j, k)+ emi(i, j, k)*l_eff*integration_normalizer*sin(lon)*d_lat*d_lon
							else if(norm_x_cell*cos(lat)*sin(lon)+ norm_y_cell*cos(lon)+ norm_z_cell*sin(lat)*sin(lon) .lt. -orthogonal_threshold) then
								! 反射を考慮していないので, 反射分( (1-ε)Ir )の熱量は, どんどん減っていく
								dE_cell(i, j, k)= dE_cell(i, j, k)+ emi(i, j, k)*Ir(i, j, k)*l_eff*integration_normalizer*sin(lon)*d_lat*d_lon
							endif
						endif
					enddo 
				enddo
			enddo
			!$omp end parallel do 
		enddo !< n (lon) ループ端
	enddo !< m (lat) ループ端

	return 
end