subroutine Set_Boundary_Radiation(ndim, m, n, lat, lon, dt, orthogonal_threshold)

	!$ use omp_lib
	use MODULE_RADIATION

	implicit none
	integer:: i= 0, j= 0, k= 0
!	    引数リスト
	integer:: ndim, m, n
	real(8):: lat, lon, dt, orthogonal_threshold ! 角度分割の精度上, 絶対値がこれより小さい内積の角度は, 直交していると判断する閾値
	real(8):: ads= 0.0d0

	if(rank1 .eq. -1) then
		if(bcd_x_ini .eq. 0) then ! 開境界
			!$omp parallel do private(j) 
			do k= 2, Nz- 1
				do j= 2, Ny- 1
					if(0.25d0*cos(lat)*sin(lon)*(norm_x(0, j-1, k-1)+ norm_x(0, j-1, k)+ norm_x(0, j, k-1)+ norm_x(0, j, k)) .gt. orthogonal_threshold) then
						Ir(1, j, k)= Ir(2, j, k)
					else
						Ir(1, j, k)= 0.0d0
					endif
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_x_ini .eq. 1) then ! 温度一定壁境界
			!$omp parallel do private(j) 
			do k= 2, Nz- 1
				do j= 2, Ny- 1
					if(0.25d0*cos(lat)*sin(lon)*(norm_x(0, j-1, k-1)+ norm_x(0, j-1, k)+ norm_x(0, j, k-1)+ norm_x(0, j, k)) .gt. orthogonal_threshold) then
						Ir(1, j, k)= emi(1, j, k)*stefan_boltzmann*tmp_cell(1, j, k)*tmp_cell(1, j, k)*tmp_cell(1, j, k)*tmp_cell(1, j, k)*slr_cell(1, j, k)/intensity_distribution_factor
					else
						Ir(1, j, k)= 0.0d0
					endif
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_x_ini .eq. 2) then ! 断熱壁境界
			!$omp parallel do private(j) 
			do k= 2, Nz- 1
				do j= 2, Ny- 1
					if(0.25d0*cos(lat)*sin(lon)*(norm_x(0, j-1, k-1)+ norm_x(0, j-1, k)+ norm_x(0, j, k-1)+ norm_x(0, j, k)) .gt. orthogonal_threshold) then
						Ir(1, j, k)= emi(1, j, k)*stefan_boltzmann*tmp_cell(1, j, k)*tmp_cell(1, j, k)*tmp_cell(1, j, k)*tmp_cell(1, j, k)*slr_cell(1, j, k)/intensity_distribution_factor
					else
						Ir(1, j, k)= Ir(2, j, k)
					endif
				enddo
			enddo
			!$omp end parallel do
		else ! 周期境界
			!$omp parallel do private(j) 
			do k= 2, Nz- 1
				do j= 2, Ny- 1
					Ir(1, j, k)= Ir(Nx-1, j, k)
				enddo
			enddo
			!$omp end parallel do
		endif
	endif
	

	if(rank2 .eq. -1) then
		if(bcd_x_fin .eq. 0) then ! 開境界
			!$omp parallel do private(j) 
			do k= 2, Nz- 1
				do j= 2, Ny- 1
					if(0.25d0*cos(lat)*sin(lon)*(norm_x(Nx, j-1, k-1)+ norm_x(Nx, j-1, k)+ norm_x(Nx, j, k-1)+ norm_x(Nx, j, k)) .gt. orthogonal_threshold) then
						Ir(Nx, j, k)= Ir(Nx-1, j, k)
					else
						Ir(Nx, j, k)= 0.0d0
					endif
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_x_fin .eq. 1) then ! 温度一定壁境界
			!$omp parallel do private(j) 
			do k= 2, Nz- 1
				do j= 2, Ny- 1
					if(0.25d0*cos(lat)*sin(lon)*(norm_x(Nx, j-1, k-1)+ norm_x(Nx, j-1, k)+ norm_x(Nx, j, k-1)+ norm_x(Nx, j, k)) .gt. orthogonal_threshold) then
						Ir(Nx, j, k)= emi(Nx, j, k)*stefan_boltzmann*tmp_cell(Nx, j, k)*tmp_cell(Nx, j, k)*tmp_cell(Nx, j, k)*tmp_cell(Nx, j, k)*slr_cell(Nx, j, k)/intensity_distribution_factor
					else
						Ir(Nx, j, k)= 0.0d0
					endif
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_x_fin .eq. 2) then ! 断熱壁境界
			!$omp parallel do private(j) 
			do k= 2, Nz- 1
				do j= 2, Ny- 1
					if(0.25d0*cos(lat)*sin(lon)*(norm_x(Nx, j-1, k-1)+ norm_x(Nx, j-1, k)+ norm_x(Nx, j, k-1)+ norm_x(Nx, j, k)) .gt. orthogonal_threshold) then
						Ir(Nx, j, k)= emi(Nx, j, k)*stefan_boltzmann*tmp_cell(Nx, j, k)*tmp_cell(Nx, j, k)*tmp_cell(Nx, j, k)*tmp_cell(Nx, j, k)*slr_cell(Nx, j, k)/intensity_distribution_factor
					else
						Ir(Nx, j, k)= Ir(Nx-1, j, k)
					endif
				enddo
			enddo
			!$omp end parallel do
		else ! 周期境界
			!$omp parallel do private(j) 
			do k= 2, Nz- 1
				do j= 2, Ny- 1
					Ir(Nx, j, k)= Ir(2, j, k)
				enddo
			enddo
			!$omp end parallel do
		endif
	endif


	if(rank3 .eq. -1) then
		if(bcd_y_ini .eq. 0) then ! 開境界
			!$omp parallel do private(i) 
			do k= 2, Nz- 1
				do i= 2, Nx- 1
					if(0.25d0*cos(lon)*(norm_y(i-1, 0, k-1)+ norm_y(i-1, 0, k)+ norm_y(i, 0, k-1)+ norm_y(i, 0, k)) .gt. orthogonal_threshold) then
						Ir(i, 1, k)= Ir(i, 2, k)
					else
						Ir(i, 1, k)= 0.0d0
					endif
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_y_ini .eq. 1) then ! 温度一定壁境界
			!$omp parallel do private(i) 
			do k= 2, Nz- 1
				do i= 2, Nx- 1
					if(0.25d0*cos(lon)*(norm_y(i-1, 0, k-1)+ norm_y(i-1, 0, k)+ norm_y(i, 0, k-1)+ norm_y(i, 0, k)) .gt. orthogonal_threshold) then
						Ir(i, 1, k)= emi(i, 1, k)*stefan_boltzmann*tmp_cell(i, 1, k)*tmp_cell(i, 1, k)*tmp_cell(i, 1, k)*tmp_cell(i, 1, k)*slr_cell(i, 1, k)/intensity_distribution_factor
					else
						Ir(i, 1, k)= 0.0d0
					endif
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_y_ini .eq. 2) then ! 断熱壁境界
			!$omp parallel do private(i) 
			do k= 2, Nz- 1
				do i= 2, Nx- 1
					if(0.25d0*cos(lon)*(norm_y(i-1, 0, k-1)+ norm_y(i-1, 0, k)+ norm_y(i, 0, k-1)+ norm_y(i, 0, k)) .gt. orthogonal_threshold) then
						Ir(i, 1, k)= emi(i, 1, k)*stefan_boltzmann*tmp_cell(i, 1, k)*tmp_cell(i, 1, k)*tmp_cell(i, 1, k)*tmp_cell(i, 1, k)*slr_cell(i, 1, k)/intensity_distribution_factor
					else
						Ir(i, 1, k)= Ir(i, 2, k)
					endif
				enddo
			enddo
			!$omp end parallel do
		else ! 周期境界
			!$omp parallel do private(i) 
			do k= 2, Nz- 1
				do i= 2, Nx- 1
					Ir(i, 1, k)= Ir(i, Ny-1, k)
				enddo
			enddo
			!$omp end parallel do
		endif
	endif

	if(rank4 .eq. -1) then
		if(bcd_y_fin .eq. 0) then ! 開境界
			!$omp parallel do private(i) 
			do k= 2, Nz- 1
				do i= 2, Nx- 1
					if(0.25d0*cos(lon)*(norm_y(i-1, Ny, k-1)+ norm_y(i-1, Ny, k)+ norm_y(i, Ny, k-1)+ norm_y(i, Ny, k)) .gt. orthogonal_threshold) then
						Ir(i, Ny, k)= Ir(i, Ny-1, k)
					else
						Ir(i, Ny, k)= 0.0d0
					endif
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_y_fin .eq. 1) then ! 温度一定壁境界
			!$omp parallel do private(i) 
			do k= 2, Nz- 1
				do i= 2, Nx- 1
					if(0.25d0*cos(lon)*(norm_y(i-1, Ny, k-1)+ norm_y(i-1, Ny, k)+ norm_y(i, Ny, k-1)+ norm_y(i, Ny, k)) .gt. orthogonal_threshold) then
						Ir(i, Ny, k)= emi(i, Ny, k)*stefan_boltzmann*tmp_cell(i, Ny, k)*tmp_cell(i, Ny, k)*tmp_cell(i, Ny, k)*tmp_cell(i, Ny, k)*slr_cell(i, Ny, k)/intensity_distribution_factor
					else
						Ir(i, Ny, k)= 0.0d0
					endif
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_y_fin .eq. 2) then ! 断熱壁境界
			!$omp parallel do private(i) 
			do k= 2, Nz- 1
				do i= 2, Nx- 1
					if(0.25d0*cos(lon)*(norm_y(i-1, Ny, k-1)+ norm_y(i-1, Ny, k)+ norm_y(i, Ny, k-1)+ norm_y(i, Ny, k)) .gt. orthogonal_threshold) then
						Ir(i, Ny, k)= emi(i, Ny, k)*stefan_boltzmann*tmp_cell(i, Ny, k)*tmp_cell(i, Ny, k)*tmp_cell(i, Ny, k)*tmp_cell(i, Ny, k)*slr_cell(i, Ny, k)/intensity_distribution_factor
					else
						Ir(i, Ny, k)= Ir(i, Ny-1, k)
					endif
				enddo
			enddo
			!$omp end parallel do
		else ! 周期境界
			!$omp parallel do private(i) 
			do k= 2, Nz- 1
				do i= 2, Nx- 1
					Ir(i, Ny, k)= Ir(i, 2, k)
				enddo
			enddo
			!$omp end parallel do
		endif
	endif


	if(rank5 .eq. -1) then
		if(bcd_z_ini .eq. 0) then ! 開境界
			!$omp parallel do private(i) 
			do j= 2, Ny- 1
				do i= 2, Nx- 1
					if(0.25d0*sin(lat)*sin(lon)*(norm_z(i-1, j-1, 0)+ norm_z(i-1, j, 0)+ norm_z(i, j-1, 0)+ norm_z(i, j, 0)) .gt. orthogonal_threshold) then
						Ir(i, j, 1)= Ir(i, j, 2)
					else
						Ir(i, j, 1)= 0.0d0
					endif
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_z_ini .eq. 1) then ! 温度一定壁境界
			!$omp parallel do private(i) 
			do j= 2, Ny- 1
				do i= 2, Nx- 1
					if(0.25d0*sin(lat)*sin(lon)*(norm_z(i-1, j-1, 0)+ norm_z(i-1, j, 0)+ norm_z(i, j-1, 0)+ norm_z(i, j, 0)) .gt. orthogonal_threshold) then
						Ir(i, j, 1)= emi(i, j, 1)*stefan_boltzmann*tmp_cell(i, j, 1)*tmp_cell(i, j, 1)*tmp_cell(i, j, 1)*tmp_cell(i, j, 1)*slr_cell(i, j, 1)/intensity_distribution_factor
					else
						Ir(i, j, 1)= 0.0d0
					endif
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_z_ini .eq. 2) then ! 断熱壁境界
			!$omp parallel do private(i) 
			do j= 2, Ny- 1
				do i= 2, Nx- 1
					if(0.25d0*sin(lat)*sin(lon)*(norm_z(i-1, j-1, 0)+ norm_z(i-1, j, 0)+ norm_z(i, j-1, 0)+ norm_z(i, j, 0)) .gt. orthogonal_threshold) then
						Ir(i, j, 1)= emi(i, j, 1)*stefan_boltzmann*tmp_cell(i, j, 1)*tmp_cell(i, j, 1)*tmp_cell(i, j, 1)*tmp_cell(i, j, 1)*slr_cell(i, j, 1)/intensity_distribution_factor
					else
						Ir(i, j, 1)= Ir(i, j, 2)
					endif
				enddo
			enddo
			!$omp end parallel do
		else  ! 周期境界
			!$omp parallel do private(i) 
			do j= 2, Ny- 1
				do i= 2, Nx- 1
					Ir(i, j, 1)= Ir(i, j, Nz-1)
				enddo
			enddo
			!$omp end parallel do
		endif
	endif


	if(rank6 .eq. -1) then
		if(bcd_z_fin .eq. 0) then ! 開境界
			!$omp parallel do private(i) 
			do j= 2, Ny- 1
				do i= 2, Nx- 1
					if(0.25d0*sin(lat)*sin(lon)*(norm_z(i-1, j-1, Nz)+ norm_z(i-1, j, Nz)+ norm_z(i, j-1, Nz)+ norm_z(i, j, Nz)) .gt. orthogonal_threshold) then
						Ir(i, j, Nz)= Ir(i, j, Nz-1)
					else
						Ir(i, j, Nz)= 0.0d0
					endif
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_z_fin .eq. 1) then ! 温度一定壁境界
			!$omp parallel do private(i) 
			do j= 2, Ny- 1
				do i= 2, Nx- 1
					if(0.25d0*sin(lat)*sin(lon)*(norm_z(i-1, j-1, Nz)+ norm_z(i-1, j, Nz)+ norm_z(i, j-1, Nz)+ norm_z(i, j, Nz)) .gt. orthogonal_threshold) then
						Ir(i, j, Nz)= emi(i, j, Nz)*stefan_boltzmann*tmp_cell(i, j, Nz)*tmp_cell(i, j, Nz)*tmp_cell(i, j, Nz)*tmp_cell(i, j, Nz)*slr_cell(i, j, Nz)/intensity_distribution_factor
					else
						Ir(i, j, Nz)= 0.0d0
					endif
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_z_fin .eq. 2) then ! 断熱壁境界
			!$omp parallel do private(i) 
			do j= 2, Ny- 1
				do i= 2, Nx- 1
					if(0.25d0*sin(lat)*sin(lon)*(norm_z(i-1, j-1, Nz)+ norm_z(i-1, j, Nz)+ norm_z(i, j-1, Nz)+ norm_z(i, j, Nz)) .gt. orthogonal_threshold) then
						Ir(i, j, Nz)= emi(i, j, Nz)*stefan_boltzmann*tmp_cell(i, j, Nz)*tmp_cell(i, j, Nz)*tmp_cell(i, j, Nz)*tmp_cell(i, j, Nz)*slr_cell(i, j, Nz)/intensity_distribution_factor
					else
						Ir(i, j, Nz)= Ir(i, j, Nz-1)
					endif
				enddo
			enddo
			!$omp end parallel do
		else  ! 周期境界
			!$omp parallel do private(i) 
			do j= 2, Ny- 1
				do i= 2, Nx- 1
					Ir(i, j, Nz)= Ir(i, j, 2)
				enddo
			enddo
			!$omp end parallel do
		endif
	endif

	call mpi_pack_x_dble(Ir)
	call mpi_isend_irecv_x_dble
	call mpi_unpack_x_dble(Ir)
	
	if(ndim .eq. 3) then
		call mpi_pack_y_dble(Ir)
		call mpi_isend_irecv_y_dble
		call mpi_unpack_y_dble(Ir)
	endif
	
	call mpi_pack_z_dble(Ir)
	call mpi_isend_irecv_z_dble
	call mpi_unpack_z_dble(Ir)

	return
end