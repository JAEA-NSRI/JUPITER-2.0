subroutine mpi_pack_x_dble_normal(r_array)

	use MODULE_RADIATION
	
	implicit none
	integer(4):: i= 0, j= 0, k= 0
	real(8):: r_array(0:Nx, 0:Ny, 0:Nz)

	if(rank1 .gt. -1) then
		k= 1
		do j= 0, Nz
			do i= 0, Ny
				mpi_send_dble_buffer1(k)= r_array(2,  i, j)
				mpi_recv_dble_buffer1(k)= r_array(2,  i, j)
				k= k+ 1
			enddo
		enddo
	else
		k= 1
		do j= 0, Nz
			do i= 0, Ny
				mpi_send_dble_buffer1(k)= r_array(0,  i, j)
				mpi_recv_dble_buffer1(k)= r_array(0,  i, j)
				k= k+ 1
			enddo
		enddo
	endif
	
	if(rank2 .gt. -1) then
		k= 1
		do j= 0, Nz
			do i= 0, Ny
				mpi_send_dble_buffer2(k)= r_array(Nx-2, i, j)
				mpi_recv_dble_buffer2(k)= r_array(Nx-2, i, j)
				k= k+ 1
			enddo
		enddo
	else
		k= 1
		do j= 0, Nz
			do i= 0, Ny
				mpi_send_dble_buffer2(k)= r_array(Nx, i, j)
				mpi_recv_dble_buffer2(k)= r_array(Nx, i, j)
				k= k+ 1
			enddo
		enddo
	endif

end

subroutine mpi_pack_y_dble_normal(r_array)

	use MODULE_RADIATION
	
	implicit none
	integer(4):: i= 0, j= 0, k= 0
	real(8):: r_array(0:Nx, 0:Ny, 0:Nz)

	if(rank3 .gt. -1) then
		k= 1
		do j= 0, Nz
			do i= 0, Nx
				mpi_send_dble_buffer3(k)= r_array(i, 2,  j)
				mpi_recv_dble_buffer3(k)= r_array(i, 2,  j)
				k= k+ 1
			enddo
		enddo
	else
		k= 1
		do j= 0, Nz
			do i= 0, Nx
				mpi_send_dble_buffer3(k)= r_array(i, 0,  j)
				mpi_recv_dble_buffer3(k)= r_array(i, 0,  j)
				k= k+ 1
			enddo
		enddo
	endif

	if(rank4 .gt. -1) then
		k= 1
		do j= 0, Nz
			do i= 0, Nx
				mpi_send_dble_buffer4(k)= r_array(i, Ny-2, j)
				mpi_recv_dble_buffer4(k)= r_array(i, Ny-2, j)
				k= k+ 1
			enddo
		enddo
	else
		k= 1
		do j= 0, Nz
			do i= 0, Nx
				mpi_send_dble_buffer4(k)= r_array(i, Ny, j)
				mpi_recv_dble_buffer4(k)= r_array(i, Ny, j)
				k= k+ 1
			enddo
		enddo
	endif

end

subroutine mpi_pack_z_dble_normal(r_array)

	use MODULE_RADIATION
	
	implicit none
	integer(4):: i= 0, j= 0, k= 0
	real(8):: r_array(0:Nx, 0:Ny, 0:Nz)

	if(rank5 .gt. -1) then
		k= 1
		do j= 0, Ny
			do i= 0, Nx
				mpi_send_dble_buffer5(k)= r_array(i, j, 2)
				mpi_recv_dble_buffer5(k)= r_array(i, j, 2)
				k= k+ 1
			enddo
		enddo
	else
		k= 1
		do j= 0, Ny
			do i= 0, Nx
				mpi_send_dble_buffer5(k)= r_array(i, j, 0)
				mpi_recv_dble_buffer5(k)= r_array(i, j, 0)
				k= k+ 1
			enddo
		enddo
	endif

	if(rank6 .gt. -1) then
		k= 1
		do j= 0, Ny
			do i= 0, Nx
				mpi_send_dble_buffer6(k)= r_array(i, j, Nz-2)
				mpi_recv_dble_buffer6(k)= r_array(i, j, Nz-2)
				k= k+ 1
			enddo
		enddo
	else
		k= 1
		do j= 0, Ny
			do i= 0, Nx
				mpi_send_dble_buffer6(k)= r_array(i, j, Nz)
				mpi_recv_dble_buffer6(k)= r_array(i, j, Nz)
				k= k+ 1
			enddo
		enddo
	endif

end