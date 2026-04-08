subroutine mpi_unpack_x_dble(r_array)

	use MODULE_RADIATION
	
	implicit none
	integer(4):: i= 0, j= 0, k= 0
	real(8):: r_array(1:Nx, 1:Ny, 1:Nz)

	k= 1
	do j= 1, Nz
		do i= 1, Ny
			r_array(1,  i, j)= mpi_recv_dble_buffer1(k)
			r_array(Nx, i, j)= mpi_recv_dble_buffer2(k)
			k= k+ 1
		enddo
	enddo

end

subroutine mpi_unpack_y_dble(r_array)

	use MODULE_RADIATION
	
	implicit none
	integer(4):: i= 0, j= 0, k= 0
	real(8):: r_array(1:Nx, 1:Ny, 1:Nz)

	k= 1
	do j= 1, Nz
		do i= 1, Nx
			r_array(i, 1,  j)= mpi_recv_dble_buffer3(k)
			r_array(i, Ny, j)= mpi_recv_dble_buffer4(k)
			k= k+ 1
		enddo
	enddo

end

subroutine mpi_unpack_z_dble(r_array)

	use MODULE_RADIATION
	
	implicit none
	integer(4):: i= 0, j= 0, k= 0
	real(8):: r_array(1:Nx, 1:Ny, 1:Nz)

	k= 1
	do j= 1, Ny
		do i= 1, Nx
			r_array(i, j, 1 )= mpi_recv_dble_buffer5(k)
			r_array(i, j, Nz)= mpi_recv_dble_buffer6(k)
			k= k+ 1
		enddo
	enddo

end