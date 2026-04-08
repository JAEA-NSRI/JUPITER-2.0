! 独立動作時の初期条件ファイルを読み取るサブルーチン
subroutine set_radiation_initial_condition(ndim, nump, numm, xyz_range, init_file_location, init_file_name)

	use MODULE_STRUCT
	use MODULE_RADIATION

	implicit none
	type(max_min):: xyz_range(3)

	integer :: ndim, nump, numm
    character(255) :: init_file_location, init_file_name

	integer:: i= 0, j= 0, k= 0, m= 0, n= 0
	real(8):: lat= 0.0d0, lon= 0.0d0

	if(myrank .eq. 0) then
		do k= 1, Nz
			do j= 1, Ny
				do i= 1, Nx
					emi(i, j, k)= 0.0d0
					rho_cell(i, j, k)= 1.0d0
					cv_cell(i, j, k) = 1.0d0
					slr_cell(i, j, k)= 0.0d0
					tmp_cell(i, j, k)= 0.0d0

					if(i .eq. 1) then
						emi(i, j, k)= 0.5d0
						rho_cell(i, j, k)= 1.0d0
						cv_cell(i, j, k) = 1.0d0
						slr_cell(i, j, k)= 1.0d0
						tmp_cell(i, j, k)= 250.0d0
					endif

					if(dabs(dble(i)*dx- 6.0d0) .lt. 2.0d0 .and. &
					   dabs(dble(j)*dy- 6.0d0) .lt. 2.0d0 .and. &
					   dabs(dble(k)*dz- 6.0d0) .lt. 2.0d0) then
						emi(i, j, k)= 0.3d0
						rho_cell(i, j, k)= 0.2d0
						cv_cell(i, j, k) = 0.5d0
						slr_cell(i, j, k)= 1.0d0
						tmp_cell(i, j, k)= 0.0d0
					endif
				enddo
			enddo
		enddo
	else if(myrank .eq. 1) then
		do k= 1, Nz
			do j= 1, Ny
				do i= 1, Nx
					emi(i, j, k)= 0.0d0
					rho_cell(i, j, k)= 1.0d0
					cv_cell(i, j, k) = 1.0d0
					slr_cell(i, j, k)= 0.0d0
					tmp_cell(i, j, k)= 0.0d0

					if(dabs(dble(i)*dx- 2.0d0) .lt. 2.0d0 .and. &
					   dabs(dble(j)*dy- 6.0d0) .lt. 2.0d0 .and. &
					   dabs(dble(k)*dz- 6.0d0) .lt. 2.0d0) then
						emi(i, j, k)= 0.3d0
						rho_cell(i, j, k)= 0.2d0
						cv_cell(i, j, k) = 0.5d0
						slr_cell(i, j, k)= 1.0d0
						tmp_cell(i, j, k)= 0.0d0
					endif
				enddo
			enddo
		enddo
	else if(myrank .eq. 2) then
		do k= 1, Nz
			do j= 1, Ny
				do i= 1, Nx
					emi(i, j, k)= 0.0d0
					rho_cell(i, j, k)= 1.0d0
					cv_cell(i, j, k) = 1.0d0
					slr_cell(i, j, k)= 0.0d0
					tmp_cell(i, j, k)= 0.0d0

					if(i .eq. 1) then
						emi(i, j, k)= 0.5d0
						rho_cell(i, j, k)= 1.0d0
						cv_cell(i, j, k) = 1.0d0
						slr_cell(i, j, k)= 1.0d0
						tmp_cell(i, j, k)= 250.0d0
					endif

					if(dabs(dble(i)*dx- 6.0d0) .lt. 2.0d0 .and. &
					   dabs(dble(j)*dy- 2.0d0) .lt. 2.0d0 .and. &
					   dabs(dble(k)*dz- 6.0d0) .lt. 2.0d0) then
						emi(i, j, k)= 0.3d0
						rho_cell(i, j, k)= 0.2d0
						cv_cell(i, j, k) = 0.5d0
						slr_cell(i, j, k)= 1.0d0
						tmp_cell(i, j, k)= 0.0d0
					endif
				enddo
			enddo
		enddo
	else if(myrank .eq. 3) then
		do k= 1, Nz
			do j= 1, Ny
				do i= 1, Nx
					emi(i, j, k)= 0.0d0
					rho_cell(i, j, k)= 1.0d0
					cv_cell(i, j, k) = 1.0d0
					slr_cell(i, j, k)= 0.0d0
					tmp_cell(i, j, k)= 0.0d0

					if(dabs(dble(i)*dx- 2.0d0) .lt. 2.0d0 .and. &
					   dabs(dble(j)*dy- 2.0d0) .lt. 2.0d0 .and. &
					   dabs(dble(k)*dz- 6.0d0) .lt. 2.0d0) then
						emi(i, j, k)= 0.3d0
						rho_cell(i, j, k)= 0.2d0
						cv_cell(i, j, k) = 0.5d0
						slr_cell(i, j, k)= 1.0d0
						tmp_cell(i, j, k)= 0.0d0
					endif
				enddo
			enddo
		enddo
	else if(myrank .eq. 4) then
		do k= 1, Nz
			do j= 1, Ny
				do i= 1, Nx
					emi(i, j, k)= 0.0d0
					rho_cell(i, j, k)= 1.0d0
					cv_cell(i, j, k) = 1.0d0
					slr_cell(i, j, k)= 0.0d0
					tmp_cell(i, j, k)= 0.0d0

					if(i .eq. 1) then
						emi(i, j, k)= 0.5d0
						rho_cell(i, j, k)= 1.0d0
						cv_cell(i, j, k) = 1.0d0
						slr_cell(i, j, k)= 1.0d0
						tmp_cell(i, j, k)= 250.0d0
					endif

					if(dabs(dble(i)*dx- 6.0d0) .lt. 2.0d0 .and. &
					   dabs(dble(j)*dy- 6.0d0) .lt. 2.0d0 .and. &
					   dabs(dble(k)*dz- 2.0d0) .lt. 2.0d0) then
						emi(i, j, k)= 0.3d0
						rho_cell(i, j, k)= 0.2d0
						cv_cell(i, j, k) = 0.5d0
						slr_cell(i, j, k)= 1.0d0
						tmp_cell(i, j, k)= 0.0d0
					endif
				enddo
			enddo
		enddo
	else if(myrank .eq. 5) then
		do k= 1, Nz
			do j= 1, Ny
				do i= 1, Nx
					emi(i, j, k)= 0.0d0
					rho_cell(i, j, k)= 1.0d0
					cv_cell(i, j, k) = 1.0d0
					slr_cell(i, j, k)= 0.0d0
					tmp_cell(i, j, k)= 0.0d0

					if(dabs(dble(i)*dx- 2.0d0) .lt. 2.0d0 .and. &
					   dabs(dble(j)*dy- 6.0d0) .lt. 2.0d0 .and. &
					   dabs(dble(k)*dz- 2.0d0) .lt. 2.0d0) then
						emi(i, j, k)= 0.3d0
						rho_cell(i, j, k)= 0.2d0
						cv_cell(i, j, k) = 0.5d0
						slr_cell(i, j, k)= 1.0d0
						tmp_cell(i, j, k)= 0.0d0
					endif
				enddo
			enddo
		enddo
	else if(myrank .eq. 6) then
		do k= 1, Nz
			do j= 1, Ny
				do i= 1, Nx
					emi(i, j, k)= 0.0d0
					rho_cell(i, j, k)= 1.0d0
					cv_cell(i, j, k) = 1.0d0
					slr_cell(i, j, k)= 0.0d0
					tmp_cell(i, j, k)= 0.0d0

					if(i .eq. 1) then
						emi(i, j, k)= 0.5d0
						rho_cell(i, j, k)= 1.0d0
						cv_cell(i, j, k) = 1.0d0
						slr_cell(i, j, k)= 1.0d0
						tmp_cell(i, j, k)= 250.0d0
					endif

					if(dabs(dble(i)*dx- 6.0d0) .lt. 2.0d0 .and. &
					   dabs(dble(j)*dy- 2.0d0) .lt. 2.0d0 .and. &
					   dabs(dble(k)*dz- 2.0d0) .lt. 2.0d0) then
						emi(i, j, k)= 0.3d0
						rho_cell(i, j, k)= 0.2d0
						cv_cell(i, j, k) = 0.5d0
						slr_cell(i, j, k)= 1.0d0
						tmp_cell(i, j, k)= 0.0d0
					endif
				enddo
			enddo
		enddo
	else if(myrank .eq. 7) then
		do k= 1, Nz
			do j= 1, Ny
				do i= 1, Nx
					emi(i, j, k)= 0.0d0
					rho_cell(i, j, k)= 1.0d0
					cv_cell(i, j, k) = 1.0d0
					slr_cell(i, j, k)= 0.0d0
					tmp_cell(i, j, k)= 0.0d0

					if(dabs(dble(i)*dx- 2.0d0) .lt. 2.0d0 .and. &
					   dabs(dble(j)*dy- 2.0d0) .lt. 2.0d0 .and. &
					   dabs(dble(k)*dz- 2.0d0) .lt. 2.0d0) then
						emi(i, j, k)= 0.3d0
						rho_cell(i, j, k)= 0.2d0
						cv_cell(i, j, k) = 0.5d0
						slr_cell(i, j, k)= 1.0d0
						tmp_cell(i, j, k)= 0.0d0
					endif
				enddo
			enddo
		enddo
	endif

	do i= 1, Nx
		emi(i, 1,  1 )= 0.0d0
		emi(i, 1,  Nz)= 0.0d0
		emi(i, Ny, 1 )= 0.0d0
		emi(i, Ny, Nz)= 0.0d0
	enddo

	do j= 1, Ny
		emi(1,  j, 1 )= 0.0d0
		emi(1,  j, Nz)= 0.0d0
		emi(Nx, j, 1 )= 0.0d0
		emi(Nx, j, Nz)= 0.0d0
	enddo
	
	do k= 1, Nz
		emi(1,  1,  k)= 0.0d0
		emi(1,  Ny, k)= 0.0d0
		emi(Nx, 1,  k)= 0.0d0
		emi(Nx, Ny, k)= 0.0d0
	enddo
	

	if(bcd_x_ini .eq. 3) then
		do k= 1, Nz
			do j= 1, Ny
				slr_cell(1 , j, k)= slr_cell(Nx-1, j, k)
				slr_cell(Nx, j, k)= slr_cell(2   , j, k)
			enddo
		enddo
	endif
	
	if(bcd_y_ini .eq. 3) then
		do k= 1, Nz
			do i= 1, Nx
				slr_cell(i, 1 , k)= slr_cell(i, Ny-1, k)
				slr_cell(i, Ny, k)= slr_cell(i, 2   , k)
			enddo
		enddo
	endif
	
	if(bcd_z_ini .eq. 3) then
		do j= 1, Ny
			do i= 1, Nx
				slr_cell(i, j, 1 )= slr_cell(i, j, Nz-1)
				slr_cell(i, j, Nz)= slr_cell(i, j, 2)
			enddo
		enddo
	endif
	

	do k= 1, Nz
		do j= 1, Ny
			tmp_wall_x_ini(j, k)= tmp_cell(1 , j, k)
			tmp_wall_x_fin(j, k)= tmp_cell(Nx, j, k)
			emi_wall_x_ini(j, k)= tmp_cell(1 , j, k)
			emi_wall_x_fin(j, k)= tmp_cell(Nx, j, k)
		enddo
	enddo

	do k= 1, Nz
		do i= 1, Nx
			tmp_wall_y_ini(i, k)= tmp_cell(i, 1 , k)
			tmp_wall_y_fin(i, k)= tmp_cell(i, Ny, k)
			emi_wall_y_ini(i, k)= tmp_cell(i, 1 , k)
			emi_wall_y_fin(i, k)= tmp_cell(i, Ny, k)
		enddo
	enddo
	
	do j= 1, Ny
		do i= 1, Nx
			tmp_wall_z_ini(i, j)= tmp_cell(i, j, 1 )
			tmp_wall_z_fin(i, j)= tmp_cell(i, j, Nz)
			emi_wall_z_ini(i, j)= tmp_cell(i, j, 1 )
			emi_wall_z_fin(i, j)= tmp_cell(i, j, Nz)
		enddo
	enddo

	return 
end