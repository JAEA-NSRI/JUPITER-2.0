subroutine mpi_pack_x_int(i_array)

	use MODULE_RADIATION
	
	implicit none
	integer(4):: i= 0, j= 0, k= 0
	integer(4):: i_array(1:Nx, 1:Ny, 1:Nz)

	if(rank1 .gt. -1) then
		k= 1
		do j= 1, Nz
			do i= 1, Ny
				mpi_send_int_buffer1(k)= i_array(2,  i, j)
				mpi_recv_int_buffer1(k)= i_array(2,  i, j)
				k= k+ 1
			enddo
		enddo
	else
		k= 1
		do j= 1, Nz
			do i= 1, Ny
				mpi_send_int_buffer1(k)= i_array(1,  i, j)
				mpi_recv_int_buffer1(k)= i_array(1,  i, j)
				k= k+ 1
			enddo
		enddo
	endif
	
	if(rank2 .gt. -1) then
		k= 1
		do j= 1, Nz
			do i= 1, Ny
				mpi_send_int_buffer2(k)= i_array(Nx-1, i, j)
				mpi_recv_int_buffer2(k)= i_array(Nx-1, i, j)
				k= k+ 1
			enddo
		enddo
	else
		k= 1
		do j= 1, Nz
			do i= 1, Ny
				mpi_send_int_buffer2(k)= i_array(Nx, i, j)
				mpi_recv_int_buffer2(k)= i_array(Nx, i, j)
				k= k+ 1
			enddo
		enddo
	endif

end

subroutine mpi_pack_y_int(i_array)

	use MODULE_RADIATION
	
	implicit none
	integer(4):: i= 0, j= 0, k= 0
	integer(4):: i_array(1:Nx, 1:Ny, 1:Nz)

	if(rank3 .gt. -1) then
		k= 1
		do j= 1, Nz
			do i= 1, Nx
				mpi_send_int_buffer3(k)= i_array(i, 2,  j)
				mpi_recv_int_buffer3(k)= i_array(i, 2,  j)
				k= k+ 1
			enddo
		enddo
	else
		k= 1
		do j= 1, Nz
			do i= 1, Nx
				mpi_send_int_buffer3(k)= i_array(i, 1,  j)
				mpi_recv_int_buffer3(k)= i_array(i, 1,  j)
				k= k+ 1
			enddo
		enddo
	endif

	if(rank4 .gt. -1) then
		k= 1
		do j= 1, Nz
			do i= 1, Nx
				mpi_send_int_buffer4(k)= i_array(i, Ny-1, j)
				mpi_recv_int_buffer4(k)= i_array(i, Ny-1, j)
				k= k+ 1
			enddo
		enddo
	else
		k= 1
		do j= 1, Nz
			do i= 1, Nx
				mpi_send_int_buffer4(k)= i_array(i, Ny, j)
				mpi_recv_int_buffer4(k)= i_array(i, Ny, j)
				k= k+ 1
			enddo
		enddo
	endif

end

subroutine mpi_pack_z_int(i_array)

	use MODULE_RADIATION
	
	implicit none
	integer(4):: i= 0, j= 0, k= 0
	integer(4):: i_array(1:Nx, 1:Ny, 1:Nz)

	if(rank5 .gt. -1) then
		k= 1
		do j= 1, Ny
			do i= 1, Nx
				mpi_send_int_buffer5(k)= i_array(i, j, 2)
				mpi_recv_int_buffer5(k)= i_array(i, j, 2)
				k= k+ 1
			enddo
		enddo
	else
		k= 1
		do j= 1, Ny
			do i= 1, Nx
				mpi_send_int_buffer5(k)= i_array(i, j, 1)
				mpi_recv_int_buffer5(k)= i_array(i, j, 1)
				k= k+ 1
			enddo
		enddo
	endif

	if(rank6 .gt. -1) then
		k= 1
		do j= 1, Ny
			do i= 1, Nx
				mpi_send_int_buffer6(k)= i_array(i, j, Nz-1)
				mpi_recv_int_buffer6(k)= i_array(i, j, Nz-1)
				k= k+ 1
			enddo
		enddo
	else
		k= 1
		do j= 1, Ny
			do i= 1, Nx
				mpi_send_int_buffer6(k)= i_array(i, j, Nz)
				mpi_recv_int_buffer6(k)= i_array(i, j, Nz)
				k= k+ 1
			enddo
		enddo
	endif

end