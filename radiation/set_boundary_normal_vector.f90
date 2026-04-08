! 温度の境界条件設定
subroutine Set_Boundary_Normal_Vector(ndim)

	!$ use omp_lib
	use MODULE_RADIATION
	
	implicit none

	integer:: ndim
	integer:: i= 0, j= 0, k= 0


	if(ndim .eq. 2) then
		!$omp parallel do private(i, j)
		do k= 0, Nz
			do j= 0, Ny
				do i= 0, Nx
					norm_y(i, j, k)= 0.0d0
				enddo
			enddo
		enddo
		!$omp end parallel do
	endif


	if(rank1 .eq. -1) then
		if(bcd_x_ini .eq. 0) then ! 開境界の場合は, 負向き
			!$omp parallel do private(j)
			do k= 1, Nz- 1
				do j= 1, Ny- 1
					norm_x(0, j, k)= -1.0d0
					norm_y(0, j, k)=  0.0d0
					norm_z(0, j, k)=  0.0d0
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_x_ini .eq. 1 .or. bcd_x_ini .eq. 2) then ! x 軸始点の境界 : 壁面の場合は, x 軸正向き
			!$omp parallel do private(j)
			do k= 1, Nz- 1
				do j= 1, Ny- 1
					norm_x(0, j, k)= 1.0d0
					norm_y(0, j, k)= 0.0d0
					norm_z(0, j, k)= 0.0d0
				enddo
			enddo
			!$omp end parallel do
		else ! 周期境界
			!$omp parallel do private(j)
			do k= 1, Nz- 1
				do j= 1, Ny- 1
					norm_x(0, j, k)= norm_x(Nx-2, j, k)
					norm_y(0, j, k)= norm_y(Nx-2, j, k)
					norm_z(0, j, k)= norm_z(Nx-2, j, k)
				enddo
			enddo
			!$omp end parallel do
		endif
	endif

	if(rank2 .eq. -1) then
		if(bcd_x_fin .eq. 0) then ! 開境界の場合は, 正向き
			!$omp parallel do private(j)
			do k= 1, Nz- 1
				do j= 1, Ny- 1
					norm_x(Nx,   j, k)= 1.0d0
					norm_y(Nx,   j, k)= 0.0d0
					norm_z(Nx,   j, k)= 0.0d0
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_x_fin .eq. 1 .or. bcd_x_fin .eq. 2) then ! x 軸終点の境界 : 壁面の場合は, x 軸負向き
			!$omp parallel do private(j)
			do k= 1, Nz- 1
				do j= 1, Ny- 1
					norm_x(Nx,   j, k)= -1.0d0
					norm_y(Nx,   j, k)=  0.0d0
					norm_z(Nx,   j, k)=  0.0d0
				enddo
			enddo
			!$omp end parallel do
		else ! 周期境界
			!$omp parallel do private(j)
			do k= 1, Nz- 1
				do j= 1, Ny- 1
					norm_x(Nx,   j, k)= norm_x(2, j, k)
					norm_y(Nx,   j, k)= norm_y(2, j, k)
					norm_z(Nx,   j, k)= norm_z(2, j, k)
				enddo
			enddo
			!$omp end parallel do
		endif
	endif


	if(rank3 .eq. -1) then
		if(bcd_y_ini .eq. 0) then ! 開境界の場合は, 負向き
			!$omp parallel do private(i)
			do k= 1, Nz- 1
				do i= 1, Nx- 1
					norm_x(i, 0, k)=  0.0d0
					norm_y(i, 0, k)= -1.0d0
					norm_z(i, 0, k)=  0.0d0
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_y_ini .eq. 1 .or. bcd_y_ini .eq. 2) then ! y 軸始点の境界 : 壁面の場合は, y 軸正向き
			!$omp parallel do private(i)
			do k= 1, Nz- 1
				do i= 1, Nx- 1
					norm_x(i, 0, k)= 0.0d0
					norm_y(i, 0, k)= 1.0d0
					norm_z(i, 0, k)= 0.0d0
				enddo
			enddo
			!$omp end parallel do
		else ! 周期境界
			!$omp parallel do private(i)
			do k= 1, Nz- 1
				do i= 1, Nx- 1
					norm_x(i, 0, k)= norm_x(i, Ny-2, k)
					norm_y(i, 0, k)= norm_y(i, Ny-2, k)
					norm_z(i, 0, k)= norm_z(i, Ny-2, k)
				enddo
			enddo
			!$omp end parallel do
		endif
	endif

	if(rank4 .eq. -1) then
		if(bcd_y_fin .eq. 0) then ! 開境界の場合は, 正向き
			!$omp parallel do private(i)
			do k= 1, Nz- 1
				do i= 1, Nx- 1
					norm_x(i, Ny,   k)= 0.0d0
					norm_y(i, Ny,   k)= 1.0d0
					norm_z(i, Ny,   k)= 0.0d0
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_y_fin .eq. 1 .or. bcd_y_fin .eq. 2) then ! y 軸終点の境界 : 壁面の場合は, y 軸負向き
			!$omp parallel do private(i)
			do k= 1, Nz- 1
				do i= 1, Nx- 1
					norm_x(i, Ny,   k)=  0.0d0
					norm_y(i, Ny,   k)= -1.0d0
					norm_z(i, Ny,   k)=  0.0d0
				enddo
			enddo
			!$omp end parallel do
		else ! 周期境界
			!$omp parallel do private(i)
			do k= 1, Nz- 1
				do i= 1, Nx- 1
					norm_x(i, Ny,   k)= norm_x(i, 2, k)
					norm_y(i, Ny,   k)= norm_y(i, 2, k)
					norm_z(i, Ny,   k)= norm_z(i, 2, k)
				enddo
			enddo
			!$omp end parallel do
		endif
	endif


	if(rank5 .eq. -1) then
		if(bcd_z_ini .eq. 0) then ! 開境界の場合は, 負向き
			!$omp parallel do private(i)
			do j= 1, Ny- 1
				do i= 1, Nx- 1
					norm_x(i, j, 0)=  0.0d0
					norm_y(i, j, 0)=  0.0d0
					norm_z(i, j, 0)= -1.0d0
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_z_ini .eq. 1 .or. bcd_z_ini .eq. 2) then ! z 軸始点の境界 : 壁面の場合は, z 軸正向き
			!$omp parallel do private(i)
			do j= 1, Ny- 1
				do i= 1, Nx- 1
					norm_x(i, j, 0)= 0.0d0
					norm_y(i, j, 0)= 0.0d0
					norm_z(i, j, 0)= 1.0d0
				enddo
			enddo
			!$omp end parallel do
		else ! 周期境界
			!$omp parallel do private(i)
			do j= 1, Ny- 1
				do i= 1, Nx- 1
					norm_x(i, j, 0)= norm_x(i, j, Nz-2)
					norm_y(i, j, 0)= norm_y(i, j, Nz-2)
					norm_z(i, j, 0)= norm_z(i, j, Nz-2)
				enddo
			enddo
			!$omp end parallel do
		endif
	endif

	if(rank6 .eq. -1) then
		if(bcd_z_fin .eq. 0) then ! 開境界の場合は, 正向き
			!$omp parallel do private(i)
			do j= 1, Ny- 1
				do i= 1, Nx- 1
					norm_x(i, j, Nz  )= 0.0d0
					norm_y(i, j, Nz  )= 0.0d0
					norm_z(i, j, Nz  )= 1.0d0
				enddo
			enddo
			!$omp end parallel do
		else if(bcd_z_fin .eq. 1 .or. bcd_z_fin .eq. 2) then ! z 軸終点の境界 : 壁面の場合は, z 軸負向き
			!$omp parallel do private(i)
			do j= 1, Ny- 1
				do i= 1, Nx- 1
					norm_x(i, j, Nz  )=  0.0d0
					norm_y(i, j, Nz  )=  0.0d0
					norm_z(i, j, Nz  )= -1.0d0
				enddo
			enddo
			!$omp end parallel do
		else ! 周期境界
			!$omp parallel do private(i)
			do j= 1, Ny- 1
				do i= 1, Nx- 1
					norm_x(i, j, Nz  )= norm_x(i, j, 2)
					norm_y(i, j, Nz  )= norm_y(i, j, 2)
					norm_z(i, j, Nz  )= norm_z(i, j, 2)
				enddo
			enddo
			!$omp end parallel do
		endif
	endif

	call mpi_pack_x_dble_normal(norm_x)
	call mpi_isend_irecv_x_dble
	call mpi_unpack_x_dble_normal(norm_x)
	
	if(ndim .eq. 3) then
		call mpi_pack_y_dble_normal(norm_x)
		call mpi_isend_irecv_y_dble
		call mpi_unpack_y_dble_normal(norm_x)
	endif
	
	call mpi_pack_z_dble_normal(norm_x)
	call mpi_isend_irecv_z_dble
	call mpi_unpack_z_dble_normal(norm_x)


	if(ndim .eq. 3) then
		call mpi_pack_x_dble_normal(norm_y)
		call mpi_isend_irecv_x_dble
		call mpi_unpack_x_dble_normal(norm_y)
	
		call mpi_pack_y_dble_normal(norm_y)
		call mpi_isend_irecv_y_dble
		call mpi_unpack_y_dble_normal(norm_y)

		call mpi_pack_z_dble_normal(norm_y)
		call mpi_isend_irecv_z_dble
		call mpi_unpack_z_dble_normal(norm_y)
	endif
	

	call mpi_pack_x_dble_normal(norm_z)
	call mpi_isend_irecv_x_dble
	call mpi_unpack_x_dble_normal(norm_z)
	
	if(ndim .eq. 3) then
		call mpi_pack_y_dble_normal(norm_z)
		call mpi_isend_irecv_y_dble
		call mpi_unpack_y_dble_normal(norm_z)
	endif
	
	call mpi_pack_z_dble_normal(norm_z)
	call mpi_isend_irecv_z_dble
	call mpi_unpack_z_dble_normal(norm_z)

	return
end
