
	! 温度の境界条件設定
subroutine Set_Boundary_Cell_State(ndim, isub)

	!$ use omp_lib
	use MODULE_RADIATION
	
	implicit none

	integer:: ndim, isub
	integer:: i= 0, j= 0, k= 0

	if(isub .lt. 2)then
		if(rank1 .eq. -1) then
			if(bcd_x_ini .eq. 0) then ! 開境界
				!$omp parallel do private(j)
				do k= 2, Nz- 1
					do j= 2, Ny- 1
						cell_state(1, j, k)= -1
					enddo
				enddo
				!$omp end parallel do
			else if(bcd_x_ini .eq. 1 .or. bcd_x_ini .eq. 2) then ! 壁 (温度一定・断熱) 境界
				!$omp parallel do private(j)
				do k= 2, Nz- 1
					do j= 2, Ny- 1
						cell_state(1, j, k)= 0
					enddo
				enddo
				!$omp end parallel do
			else ! 周期境界
				!$omp parallel do private(j)
				do k= 2, Nz- 1
					do j= 2, Ny- 1
						cell_state(1, j, k)= cell_state(Nx-1, j, k)
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
						cell_state(Nx, j, k)= -1
					enddo
				enddo
				!$omp end parallel do
			else if(bcd_x_fin .eq. 1 .or. bcd_x_fin .eq. 2) then ! 壁 (温度一定・断熱) 境界
				!$omp parallel do private(j)
				do k= 2, Nz- 1
					do j= 2, Ny- 1
						cell_state(Nx, j, k)= 0
					enddo
				enddo
				!$omp end parallel do
			else ! 周期境界
				!$omp parallel do private(j)
				do k= 2, Nz- 1
					do j= 2, Ny- 1
						cell_state(Nx, j, k)= cell_state(2, j, k)
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
						cell_state(i, 1 , k)= -1
					enddo
				enddo
				!$omp end parallel do
			else if(bcd_y_ini .eq. 1 .or. bcd_y_ini .eq. 2) then ! 壁 (温度一定・断熱) 境界
				!$omp parallel do private(i)
				do k= 2, Nz- 1
					do i= 2, Nx- 1
						cell_state(i, 1 , k)= 0
					enddo
				enddo
				!$omp end parallel do
			else ! 周期境界
				!$omp parallel do private(i)
				do k= 2, Nz- 1
					do i= 2, Nx- 1
						cell_state(i, 1, k)= cell_state(i, Ny-1, k)
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
						cell_state(i, Ny, k)= -1
					enddo
				enddo
				!$omp end parallel do
			else if(bcd_y_fin .eq. 1 .or. bcd_y_fin .eq. 2) then ! 壁 (温度一定・断熱) 境界
				!$omp parallel do private(i)
				do k= 2, Nz- 1
					do i= 2, Nx- 1
						cell_state(i, Ny, k)= 0
					enddo
				enddo
				!$omp end parallel do
			else ! 周期境界
				!$omp parallel do private(i)
				do k= 2, Nz- 1
					do i= 2, Nx- 1
						cell_state(i, Ny, k)= cell_state(i, 2, k)
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
						cell_state(i, j, 1)= -1
					enddo
				enddo
				!$omp end parallel do
			else if(bcd_z_ini .eq. 1 .or. bcd_z_ini .eq. 2) then ! 壁 (温度一定・断熱) 境界
				!$omp parallel do private(i)
				do j= 2, Ny- 1
					do i= 2, Nx- 1
						cell_state(i, j, 1)= 0
					enddo
				enddo
				!$omp end parallel do
			else ! 周期境界
				!$omp parallel do private(i)
				do j= 2, Ny- 1
					do i= 2, Nx- 1
						cell_state(i, j, 1)= cell_state(i, j, Nz-1)
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
						cell_state(i, j, Nz)= -1
					enddo
				enddo
				!$omp end parallel do
			else if(bcd_z_fin .eq. 1 .or. bcd_z_fin .eq. 2) then ! 壁 (温度一定・断熱) 境界
				!$omp parallel do private(i)
				do j= 2, Ny- 1
					do i= 2, Nx- 1
						cell_state(i, j, Nz)= 0
					enddo
				enddo
				!$omp end parallel do
			else ! 周期境界
				!$omp parallel do private(i)
				do j= 2, Ny- 1
					do i= 2, Nx- 1
						cell_state(i, j, Nz)= cell_state(i, j, 2)
					enddo
				enddo
				!$omp end parallel do
			endif
		endif
	else
		if(rank1 .eq. -1) then
			if(bcd_x_ini .lt. 3) then ! 開境界
				!$omp parallel do private(j)
				do k= 2, Nz- 1
					do j= 2, Ny- 1
						cell_state(1, j, k)= cell_state(2, j, k)
					enddo
				enddo
				!$omp end parallel do
			else ! 周期境界
				!$omp parallel do private(j)
				do k= 2, Nz- 1
					do j= 2, Ny- 1
						cell_state(1, j, k)= cell_state(Nx-1, j, k)
					enddo
				enddo
				!$omp end parallel do
			endif
		endif

		if(rank2 .eq. -1) then
			if(bcd_x_fin .lt. 3) then ! 開境界
				!$omp parallel do private(j)
				do k= 2, Nz- 1
					do j= 2, Ny- 1
						cell_state(Nx, j, k)= cell_state(Nx-1, j, k)
					enddo
				enddo
				!$omp end parallel do
			else ! 周期境界
				!$omp parallel do private(j)
				do k= 2, Nz- 1
					do j= 2, Ny- 1
						cell_state(Nx, j, k)= cell_state(2, j, k)
					enddo
				enddo
				!$omp end parallel do
			endif
		endif


		if(rank3 .eq. -1) then
			if(bcd_y_ini .lt. 3) then ! 開境界
				!$omp parallel do private(i)
				do k= 2, Nz- 1
					do i= 2, Nx- 1
						cell_state(i, 1 , k)= cell_state(i, 2 , k)
					enddo
				enddo
				!$omp end parallel do
			else ! 周期境界
				!$omp parallel do private(i)
				do k= 2, Nz- 1
					do i= 2, Nx- 1
						cell_state(i, 1, k)= cell_state(i, Ny-1, k)
					enddo
				enddo
				!$omp end parallel do
			endif
		endif
	
		if(rank4 .eq. -1) then
			if(bcd_y_fin .lt. 3) then ! 開境界
				!$omp parallel do private(i)
				do k= 2, Nz- 1
					do i= 2, Nx- 1
						cell_state(i, Ny, k)= cell_state(i, Ny-1, k)
					enddo
				enddo
				!$omp end parallel do
			else ! 周期境界
				!$omp parallel do private(i)
				do k= 2, Nz- 1
					do i= 2, Nx- 1
						cell_state(i, Ny, k)= cell_state(i, 2, k)
					enddo
				enddo
				!$omp end parallel do
			endif
		endif
	

		if(rank5 .eq. -1) then
			if(bcd_z_ini .lt. 3) then ! 開境界
				!$omp parallel do private(i)
				do j= 2, Ny- 1
					do i= 2, Nx- 1
						cell_state(i, j, 1)= cell_state(i, j, 2)
					enddo
				enddo
				!$omp end parallel do
			else ! 周期境界
				!$omp parallel do private(i)
				do j= 2, Ny- 1
					do i= 2, Nx- 1
						cell_state(i, j, 1)= cell_state(i, j, Nz-1)
					enddo
				enddo
				!$omp end parallel do
			endif
		endif

		if(rank6 .eq. -1) then
			if(bcd_z_fin .lt. 3) then ! 開境界
				!$omp parallel do private(i)
				do j= 2, Ny- 1
					do i= 2, Nx- 1
						cell_state(i, j, Nz)= cell_state(i, j, Nz-1)
					enddo
				enddo
				!$omp end parallel do
			else ! 周期境界
				!$omp parallel do private(i)
				do j= 2, Ny- 1
					do i= 2, Nx- 1
						cell_state(i, j, Nz)= cell_state(i, j, 2)
					enddo
				enddo
				!$omp end parallel do
			endif
		endif
	endif

	call mpi_pack_x_int(cell_state)
	call mpi_isend_irecv_x_int
	call mpi_unpack_x_int(cell_state)
	
	if(ndim .eq. 3) then
		call mpi_pack_y_int(cell_state)
		call mpi_isend_irecv_y_int
		call mpi_unpack_y_int(cell_state)
	endif
	
	call mpi_pack_z_int(cell_state)
	call mpi_isend_irecv_z_int
	call mpi_unpack_z_int(cell_state)

	return
end
