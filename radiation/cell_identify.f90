subroutine cell_identify(ndim, isub)

	!$ use omp_lib
	use MODULE_RADIATION

	implicit none
	integer(4):: ndim, isub
	integer(4):: i= 0, j= 0, k= 0

	!$omp parallel do private(i, j)
	do k= 1, Nz
		do j= 1, Ny
			do i= 1, Nx
				if(slr_cell(i, j, k) .le. DBL_EPSILON) then
					cell_state(i, j, k)= -1
				else
					cell_state(i, j, k)=  1
				endif
				cell_state_prev(i, j, k)= cell_state(i, j, k)
			enddo
		enddo
	enddo
	!$omp end parallel do
	
	call set_boundary_cell_state(ndim, isub)

	!$omp parallel do private(i, j)
	do k= 2, Nz- 1
		do j= 2, Ny- 1
			do i= 2, Nx- 1
				if(cell_state_prev(i, j, k) .eq. 1) then
					if( (cell_state_prev(i-1, j, k) .eq. -1) .or. &
					    (cell_state_prev(i, j-1, k) .eq. -1) .or. &
					    (cell_state_prev(i, j, k-1) .eq. -1) .or. &
					    (cell_state_prev(i, j, k+1) .eq. -1) .or. &
					    (cell_state_prev(i, j+1, k) .eq. -1) .or. &
					    (cell_state_prev(i+1, j, k) .eq. -1) ) then
						cell_state(i, j, k)= 0
					endif
				endif
			enddo
		enddo
	enddo
	!$omp end parallel do

	return
end