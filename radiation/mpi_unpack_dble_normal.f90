subroutine mpi_unpack_x_dble_normal(r_array)

	use MODULE_RADIATION
	
	implicit none
	integer(4):: i= 0, j= 0, k= 0
	real(8):: r_array(0:Nx, 0:Ny, 0:Nz)

	k= 1
	do j= 0, Nz
		do i= 0, Ny
			r_array(0,  i, j)= mpi_recv_dble_buffer1(k)
			r_array(Nx, i, j)= mpi_recv_dble_buffer2(k)
			k= k+ 1
		enddo
	enddo

end

subroutine mpi_unpack_y_dble_normal(r_array)

	use MODULE_RADIATION
	
	implicit none
	integer(4):: i= 0, j= 0, k= 0
	real(8):: r_array(0:Nx, 0:Ny, 0:Nz)

	k= 1
	do j= 0, Nz
		do i= 0, Nx
			r_array(i, 0,  j)= mpi_recv_dble_buffer3(k)
			r_array(i, Ny, j)= mpi_recv_dble_buffer4(k)
			k= k+ 1
		enddo
	enddo

end

subroutine mpi_unpack_z_dble_normal(r_array)

	use MODULE_RADIATION
	
	implicit none
	integer(4):: i= 0, j= 0, k= 0
	real(8):: r_array(0:Nx, 0:Ny, 0:Nz)

	k= 1
	do j= 0, Ny
		do i= 0, Nx
			r_array(i, j, 0 )= mpi_recv_dble_buffer5(k)
			r_array(i, j, Nz)= mpi_recv_dble_buffer6(k)
			k= k+ 1
		enddo
	enddo

end