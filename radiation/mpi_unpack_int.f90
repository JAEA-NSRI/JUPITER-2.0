subroutine mpi_unpack_x_int(i_array)

	use MODULE_RADIATION
	
	implicit none
	integer(4):: i= 0, j= 0, k= 0
	integer(4):: i_array(1:Nx, 1:Ny, 1:Nz)

	k= 1
	do j= 1, Nz
		do i= 1, Ny
			i_array(1,  i, j)= mpi_recv_int_buffer1(k)
			i_array(Nx, i, j)= mpi_recv_int_buffer2(k)
			k= k+ 1
		enddo
	enddo

end

subroutine mpi_unpack_y_int(i_array)

	use MODULE_RADIATION
	
	implicit none
	integer(4):: i= 0, j= 0, k= 0
	integer(4):: i_array(1:Nx, 1:Ny, 1:Nz)

	k= 1
	do j= 1, Nz
		do i= 1, Nx
			i_array(i, 1,  j)= mpi_recv_int_buffer3(k)
			i_array(i, Ny, j)= mpi_recv_int_buffer4(k)
			k= k+ 1
		enddo
	enddo

end

subroutine mpi_unpack_z_int(i_array)

	use MODULE_RADIATION
	
	implicit none
	integer(4):: i= 0, j= 0, k= 0
	integer(4):: i_array(1:Nx, 1:Ny, 1:Nz)

	k= 1
	do j= 1, Ny
		do i= 1, Nx
			i_array(i, j, 1 )= mpi_recv_int_buffer5(k)
			i_array(i, j, Nz)= mpi_recv_int_buffer6(k)
			k= k+ 1
		enddo
	enddo

end