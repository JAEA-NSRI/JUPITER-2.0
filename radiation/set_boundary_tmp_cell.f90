! 温度の境界条件設定
subroutine Set_Boundary_tmp_cell(ndim)

	!$ use omp_lib
	use MODULE_RADIATION
	
	implicit none

	integer:: ndim
	integer:: i= 0, j= 0, k= 0, ierr

	if(rank1 .eq. -1) then
		! 開境界・断熱壁の場合には, tmp_cell は計算
		if(bcd_x_ini .eq. 1) then
			!$omp parallel do private(j)
			do k= 2, Nz- 1
				do j= 2, Ny- 1
					tmp_cell(1, j, k)= tmp_wall_x_ini(j, k); 
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_x_ini .eq. 3) then ! 周期境界
			!$omp parallel do private(j)
			do k= 2, Nz- 1
				do j= 2, Ny- 1
					tmp_cell(1, j, k)= tmp_cell(Nx-1, j, k); 
				enddo
			enddo
			!$omp end parallel do
		endif
	endif

	if(rank2 .eq. -1) then
		! 開境界・断熱壁の場合には, tmp_cell は計算
		if(bcd_x_fin .eq. 1) then
			!$omp parallel do private(j)
			do k= 2, Nz- 1
				do j= 2, Ny- 1
					tmp_cell(Nx, j, k)= tmp_wall_x_fin(j, k); 
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_x_fin .eq. 3) then ! 周期境界
			!$omp parallel do private(j)
			do k= 2, Nz- 1
				do j= 2, Ny- 1
					tmp_cell(Nx, j, k)= tmp_cell(2, j, k); 
				enddo
			enddo
			!$omp end parallel do
		endif
	endif


	if(rank3 .eq. -1) then
		! 開境界・断熱壁の場合には, tmp_cell は計算
		if(bcd_y_ini .eq. 1) then
			!$omp parallel do private(i)
			do k= 2, Nz- 1
				do i= 2, Nx- 1
					tmp_cell(i, 1, k)= tmp_wall_y_ini(i, k); 
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_y_ini .eq. 3) then ! 周期境界
			!$omp parallel do private(i)
			do k= 2, Nz- 1
				do i= 2, Nx- 1
					tmp_cell(i, 1, k)= tmp_cell(i, Ny-1, k) 
				enddo
			enddo
			!$omp end parallel do
		endif
	endif

	if(rank4 .eq. -1) then
		! 開境界・断熱壁の場合には, tmp_cell は計算
		if(bcd_y_fin .eq. 1) then
			!$omp parallel do private(i)
			do k= 2, Nz- 1
				do i= 2, Nx- 1
					tmp_cell(i, Ny, k)= tmp_wall_y_fin(i, k); 
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_y_fin .eq. 3) then ! 周期境界
			!$omp parallel do private(i)
			do k= 2, Nz- 1
				do i= 2, Nx- 1
					tmp_cell(i, Ny, k)= tmp_cell(i, 2, k) 
				enddo
			enddo
			!$omp end parallel do
		endif
	endif

	if(rank5 .eq. -1) then
		! 開境界・断熱壁の場合には, tmp_cell は計算
		if(bcd_z_ini .eq. 1) then
			!$omp parallel do private(i)
			do j= 2, Ny- 1
				do i= 2, Nx- 1
					tmp_cell(i, j, 1)= tmp_wall_z_ini(i, j)
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_z_ini .eq. 3) then ! 周期境界
			!$omp parallel do private(i)
			do j= 2, Ny- 1
				do i= 2, Nx- 1
					tmp_cell(i, j, 1)= tmp_cell(i, j, Nz-1)
				enddo
			enddo
			!$omp end parallel do
		endif
	endif

	if(rank6 .eq. -1) then
		! 開境界・断熱壁の場合には, tmp_cell は計算
		if(bcd_z_fin .eq. 1) then
			!$omp parallel do private(i)
			do j= 2, Ny- 1
				do i= 2, Nx- 1
					tmp_cell(i, j, Nz)= tmp_wall_z_fin(i, j)
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_z_ini .eq. 3) then ! 周期境界
			!$omp parallel do private(i)
			do j= 2, Ny- 1
				do i= 2, Nx- 1
					tmp_cell(i, j, Nz)= tmp_cell(i, j, 2)
				enddo
			enddo
			!$omp end parallel do
		endif
	endif

	call mpi_pack_x_dble(tmp_cell)
	call mpi_isend_irecv_x_dble
	call mpi_unpack_x_dble(tmp_cell)
	
	if(ndim .eq. 3) then
		call mpi_pack_y_dble(tmp_cell)
		call mpi_isend_irecv_y_dble
		call mpi_unpack_y_dble(tmp_cell)
	endif
	
	call mpi_pack_z_dble(tmp_cell)
	call mpi_isend_irecv_z_dble
	call mpi_unpack_z_dble(tmp_cell)

	return
end
