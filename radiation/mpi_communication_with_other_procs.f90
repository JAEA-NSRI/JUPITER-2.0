subroutine mpi_communication_with_other_procs(master_lat, isub, iflg, it, dt)

	use MODULE_STRUCT
	use MODULE_RADIATION
	use MODULE_LOG
	
	implicit none

	include 'mpif.h'

	! 引数リスト
	integer(4):: master_lat, isub, iflg, it
	real(8):: dt

	integer(4):: i, j, k
	integer(4):: i_trs, node, ierr
	integer(4):: status(MPI_STATUS_SIZE), tag= 10, req

	if(isub .eq. 1) then
		!マスターからPOPCORN側への前回計算結果(差分温度)の送信
		if(myrank.eq.0) then
			!差分温度の計算
			if(it .eq. 1) then
				mpi_buffer(:) = 0.0d0 !初回は差分温度0を返す
			else
				i_trs= 1
				do k= 1, Nz
					do j= 1, Ny
						do i= 1, Nx
							mpi_buffer(i_trs)= tmp_cell(i, j, k)- tmp_cell_prev(i, j, k)
							i_trs= i_trs+ 1
						enddo
					enddo
				enddo

			endif
			call MPI_Send(mpi_buffer, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, 0, 100, MPI_COMM_WORLD, ierr)
		endif

		!輻射プロセス終了判定結果のPOPCORNからの受け取り
		call MPI_Bcast(iflg, 1, MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
		if(iflg .eq. 1) then
			return
		endif
		
		call MPI_Barrier(MPI_COMM_WORLD, ierr)
		call MPI_Bcast(particle_total, size(particle_total), MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(*,*) 'Bcast recv : particle_total', it
		call MPI_Bcast(tmp_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(*,*) 'Bcast recv : tmp_cell', it
		call MPI_Bcast(index_x, size(index_x), MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(*,*) 'Bcast recv : index_x', it
		call MPI_Bcast(index_y, size(index_y), MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(*,*) 'Bcast recv : index_y', it
		call MPI_Bcast(index_z, size(index_z), MPI_INTEGER, 0, MPI_COMM_WORLD, ierr)
!		write(*,*) 'Bcast recv : index_z', it
		call MPI_Bcast(rho_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(*,*) 'Bcast recv : rho_cell', it
		call MPI_Bcast(cv_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(*,*) 'Bcast recv : cv_cell', it
		call MPI_Bcast(slr_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(*,*) 'Bcast recv : slr_cell', it
		call MPI_Bcast(emi, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, 0, MPI_COMM_WORLD, ierr)
!		write(*,*) 'Bcast recv : emi', it
	else if(isub .eq. 2) then
		! 各角度マスターからJUPITER側への前回計算結果(差分温度)の送信
		! & 輻射プロセス終了判定結果のJUPITERからの受け取り
		if(myrank .lt. Nproc_x*Nproc_y*Nproc_z) then
			!以下の式は, it = 1 (初回) で差分温度 0.0
			i_trs= 1
			do k= 1, Nz
				do j= 1, Ny
					do i= 1, Nx
						mpi_buffer(i_trs)= rho_cell(i, j, k)*cv_cell(i, j, k)*(tmp_cell(i, j, k)- tmp_cell_prev(i, j, k))/dt
						i_trs= i_trs+ 1
					enddo
				enddo
			enddo
			
!write(LogStr(1), '((I3), (a), (I3), (a), (I3), (a))') it, ":) Start to send/receive with JUPITER : ", myrank, " (Global: ", myrank_glob, ")" ! DEBUG
			! JUPITER への熱ソースの送信
!write(6, *) "Radiation(heat_source): send From ", myrank_glob, " to ", myrank_glob- Nproc_x*Nproc_y*Nproc_z ! DEBUG
			call MPI_Send(mpi_buffer, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, myrank_glob- Nproc_x*Nproc_y*Nproc_z, tag, MPI_COMM_WORLD, ierr)
			! 輻射プロセス終了判定結果のJUPITERからの受け取り
!write(6, *) "Radiation(iflg): recv From ", myrank_glob- Nproc_x*Nproc_y*Nproc_z, " to ", myrank_glob ! DEBUG
			call MPI_Recv(iflg, 1, MPI_INTEGER, myrank_glob- Nproc_x*Nproc_y*Nproc_z, tag+ 10, MPI_COMM_WORLD, status, ierr);

			! iflg 角度スレーブへの送信 (輻射コミュニケーター内でのやりとり)
!write(6, *) "Radiation(iflg) Sending to angular slave...", myrank !DEBUG
			do node= Nproc_x*Nproc_y*Nproc_z+ (Nproc_lat- 1)*myrank, Nproc_x*Nproc_y*Nproc_z+ (Nproc_lat- 1)*(myrank+ 1)- 1
				call MPI_Isend(iflg, 1, MPI_INTEGER, node, tag, CommRADIATION, req, ierr);
			enddo
			if(Nproc_lat .gt. 1) then
				call MPI_Wait(req, status, ierr)
			endif

			if(iflg .eq. 1) then
!write(LogStr(2), '((I3), (a), (I3), (a), (I3), (a))') it, ":) Finished to send/receive with JUPITER : ", myrank, " (Global: ", myrank_glob, ")" ! DEBUG
!write(LogStr(3), '((a))') " iflg is called " ! DEBUG
!call output_exec_log_rad(isub) ! DEBUG
				return
			endif

			! JUPITER からの各種物性受け取り
!write(6, *) "Radiation(emi): recv From ", myrank_glob- Nproc_x*Nproc_y*Nproc_z, " to ", myrank ! DEBUG
			call MPI_Irecv(emi     , Nx*Ny*Nz, MPI_DOUBLE_PRECISION, myrank_glob- Nproc_x*Nproc_y*Nproc_z, tag, MPI_COMM_WORLD, req, ierr)
			call MPI_Wait(req, status, ierr)

!write(6, *) "Radiation(rho_cell): recv From ", myrank_glob- Nproc_x*Nproc_y*Nproc_z, " to ", myrank ! DEBUG
			call MPI_Irecv(rho_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, myrank_glob- Nproc_x*Nproc_y*Nproc_z, tag+ 10, MPI_COMM_WORLD, req, ierr)
			call MPI_Wait(req, status, ierr)

!write(6, *) "Radiation(cv_cell): recv From ", myrank_glob- Nproc_x*Nproc_y*Nproc_z, " to ", myrank ! DEBUG
			call MPI_Irecv(cv_cell , Nx*Ny*Nz, MPI_DOUBLE_PRECISION, myrank_glob- Nproc_x*Nproc_y*Nproc_z, tag+ 20, MPI_COMM_WORLD, req, ierr)
			call MPI_Wait(req, status, ierr)

!write(6, *) "Radiation(tmp_cell): recv From ", myrank_glob- Nproc_x*Nproc_y*Nproc_z, " to ", myrank ! DEBUG
			call MPI_Irecv(tmp_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, myrank_glob- Nproc_x*Nproc_y*Nproc_z, tag+ 30, MPI_COMM_WORLD, req, ierr)
			call MPI_Wait(req, status, ierr)

!write(6, *) "Radiation(slr_cell): recv From ", myrank_glob- Nproc_x*Nproc_y*Nproc_z, " to ", myrank ! DEBUG
			call MPI_Irecv(slr_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, myrank_glob- Nproc_x*Nproc_y*Nproc_z, tag+ 40, MPI_COMM_WORLD, req, ierr)
			call MPI_Wait(req, status, ierr)

!write(LogStr(2), '((I3), (a), (I3), (a), (I3), (a))') it, ":) Finished to send/receive with JUPITER : ", myrank, " (Global: ", myrank_glob, ")" ! DEBUG
!call output_exec_log_rad(isub) ! DEBUG
			
			! 角度スレーブへの送信 (輻射コミュニケーター内でのやりとり)
!return !DEBUG
!write(6, *) "Radiation(prop.) Sending to angular slave...", myrank !DEBUG
			do node= Nproc_x*Nproc_y*Nproc_z+ (Nproc_lat- 1)*myrank, Nproc_x*Nproc_y*Nproc_z+ (Nproc_lat- 1)*(myrank+ 1)- 1
				call MPI_Isend(emi     , Nx*Ny*Nz, MPI_DOUBLE_PRECISION, node, tag, CommRADIATION, req, ierr)
				call MPI_Isend(rho_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, node, tag+ 10, CommRADIATION, req, ierr)
				call MPI_Isend(cv_cell , Nx*Ny*Nz, MPI_DOUBLE_PRECISION, node, tag+ 20, CommRADIATION, req, ierr)
				call MPI_Isend(tmp_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, node, tag+ 30, CommRADIATION, req, ierr)
				call MPI_Isend(slr_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, node, tag+ 40, CommRADIATION, req, ierr)
			enddo
			if(Nproc_lat .gt. 1) then
				call MPI_Wait(req, status, ierr)
			endif
		else
			! iflg 角度マスターからの受信 (輻射コミュニケーター内でのやりとり)
			call MPI_Irecv(iflg, 1, MPI_INTEGER, master_lat, tag, CommRADIATION, req, ierr);
			call MPI_Wait(req, status, ierr)

			if(iflg .eq. 1) then
				return
			endif

			! 角度マスターからの受信 (輻射コミュニケーター内でのやりとり)
			call MPI_Irecv(emi     , Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag, CommRADIATION, req, ierr)
			call MPI_Irecv(rho_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag+ 10, CommRADIATION, req, ierr)
			call MPI_Irecv(cv_cell , Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag+ 20, CommRADIATION, req, ierr)
			call MPI_Irecv(tmp_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag+ 30, CommRADIATION, req, ierr)
			call MPI_Irecv(slr_cell, Nx*Ny*Nz, MPI_DOUBLE_PRECISION, master_lat, tag+ 40, CommRADIATION, req, ierr)
			call MPI_Wait(req, status, ierr)
		endif
	endif

	return
end
