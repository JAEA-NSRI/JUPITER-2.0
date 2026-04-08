subroutine mpi_isend_irecv_x_int

	use MODULE_RADIATION
	
	implicit none

	include 'mpif.h'
	integer(4):: ierr
	integer(4):: tag1= 10, tag2= 20
	integer(4):: req_send1, req_recv1, req_send2, req_recv2
	integer(4):: istat_send1(MPI_STATUS_SIZE), istat_recv1(MPI_STATUS_SIZE)
	integer(4):: istat_send2(MPI_STATUS_SIZE), istat_recv2(MPI_STATUS_SIZE)

	if(rank1 .gt. -1) then
		! Proc. #1 (0 より受信) (-側データ -> + 側)
		call MPI_Irecv(mpi_recv_int_buffer1, Ny*Nz, MPI_INTEGER, rank1, tag1, CommRADIATION, req_recv1, ierr)
		! Proc. #1 (0 へ送信)   (+側データ -> - 側)
		call MPI_Isend(mpi_send_int_buffer1, Ny*Nz, MPI_INTEGER, rank1, tag2, CommRADIATION, req_send1, ierr)
	endif
 
	if(rank2 .gt. -1) then
		! Proc. #0 (1 へ送信)   (-側データ -> + 側)
		call MPI_Isend(mpi_send_int_buffer2, Ny*Nz, MPI_INTEGER, rank2, tag1, CommRADIATION, req_send2, ierr)
		! Proc. #0 (1 より受信) (+側データ -> - 側)
		call MPI_Irecv(mpi_recv_int_buffer2, Ny*Nz, MPI_INTEGER, rank2, tag2, CommRADIATION, req_recv2, ierr)
	endif

	if(rank1 .gt. -1) then
		call MPI_Wait(req_recv1, istat_recv1, ierr)
		call MPI_Wait(req_send1, istat_send1, ierr)
	endif
	if(rank2 .gt. -1) then
		call MPI_Wait(req_recv2, istat_recv2, ierr)
		call MPI_Wait(req_send2, istat_send2, ierr)
	endif

	return
end

subroutine mpi_isend_irecv_y_int

	use MODULE_RADIATION
	
	implicit none

	include 'mpif.h'
	integer(4):: ierr
	integer(4):: tag3= 30, tag4= 40
	integer(4):: req_send3, req_recv3, req_send4, req_recv4
	integer(4):: istat_send3(MPI_STATUS_SIZE), istat_recv3(MPI_STATUS_SIZE)
	integer(4):: istat_send4(MPI_STATUS_SIZE), istat_recv4(MPI_STATUS_SIZE)

	if(rank3 .gt. -1) then
		! Proc. #1 (0 より受信) (-側データ -> + 側)
		call MPI_Irecv(mpi_recv_int_buffer3, Nz*Nx, MPI_INTEGER, rank3, tag3, CommRADIATION, req_recv3, ierr)
		! Proc. #1 (0 へ送信)   (+側データ -> - 側)
		call MPI_Isend(mpi_send_int_buffer3, Nz*Nx, MPI_INTEGER, rank3, tag4, CommRADIATION, req_send3, ierr)
	endif
 
	if(rank4 .gt. -1) then
		! Proc. #0 (1 へ送信)   (-側データ -> + 側)
		call MPI_Isend(mpi_send_int_buffer4, Nz*Nx, MPI_INTEGER, rank4, tag3, CommRADIATION, req_send4, ierr)
		! Proc. #0 (1 より受信) (+側データ -> - 側)
		call MPI_Irecv(mpi_recv_int_buffer4, Nz*Nx, MPI_INTEGER, rank4, tag4, CommRADIATION, req_recv4, ierr)
	endif

	if(rank3 .gt. -1) then
		call MPI_Wait(req_recv3, istat_recv3, ierr)
		call MPI_Wait(req_send3, istat_send3, ierr)
	endif
	if(rank4 .gt. -1) then
		call MPI_Wait(req_recv4, istat_recv4, ierr)
		call MPI_Wait(req_send4, istat_send4, ierr)
	endif

	return
end

subroutine mpi_isend_irecv_z_int

	use MODULE_RADIATION
	
	implicit none

	include 'mpif.h'
	integer(4):: ierr
	integer(4):: tag5= 50, tag6= 60
	integer(4):: req_send5, req_recv5, req_send6, req_recv6
	integer(4):: istat_send5(MPI_STATUS_SIZE), istat_recv5(MPI_STATUS_SIZE)
	integer(4):: istat_send6(MPI_STATUS_SIZE), istat_recv6(MPI_STATUS_SIZE)

	if(rank5 .gt. -1) then
		! Proc. #1 (0 より受信) (-側データ -> + 側)
		call MPI_Irecv(mpi_recv_int_buffer5, Nx*Ny, MPI_INTEGER, rank5, tag5, CommRADIATION, req_recv5, ierr)
		! Proc. #1 (0 へ送信)   (+側データ -> - 側)
		call MPI_Isend(mpi_send_int_buffer5, Nx*Ny, MPI_INTEGER, rank5, tag6, CommRADIATION, req_send5, ierr)
	endif
 
	if(rank6 .gt. -1) then
		! Proc. #0 (1 へ送信)   (-側データ -> + 側)
		call MPI_Isend(mpi_send_int_buffer6, Nx*Ny, MPI_INTEGER, rank6, tag5, CommRADIATION, req_send6, ierr)
		! Proc. #0 (1 より受信) (+側データ -> - 側)
		call MPI_Irecv(mpi_recv_int_buffer6, Nx*Ny, MPI_INTEGER, rank6, tag6, CommRADIATION, req_recv6, ierr)
	endif

	if(rank5 .gt. -1) then
		call MPI_Wait(req_recv5, istat_recv5, ierr)
		call MPI_Wait(req_send5, istat_send5, ierr)
	endif
	if(rank6 .gt. -1) then
		call MPI_Wait(req_recv6, istat_recv6, ierr)
		call MPI_Wait(req_send6, istat_send6, ierr)
	endif

	return
end