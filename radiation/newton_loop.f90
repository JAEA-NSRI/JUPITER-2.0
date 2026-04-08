subroutine newton_loop(ndim, newton, dt, dtmp_cell_err)

	!$ use omp_lib
	use MODULE_STRUCT
	use MODULE_RADIATION 

	implicit none
	integer:: ndim, newton ! 引数リスト
	real(8):: dt, dtmp_cell_err

	integer:: i= 0, j= 0, k= 0, m= 0, n= 0
	real(8):: lat= 0.0d0, dtmp= 0.0d0


	do newton= 1, newton_max
		dtmp_cell_err= 0.0d0

		!$omp parallel do private(i, j, dtmp) reduction(max: dtmp_cell_err)
		do k= 1, Nz
			do j= 1, Ny
				do i= 1, Nx
					dtmp= ( -(rho_cell(i, j, k)*cv_cell(i, j, k)*tmp_cell(i, j, k))				 &
							+ (rho_cell(i, j, k)*cv_cell(i, j, k)*tmp_cell_prev(i, j, k))		 &
							- (dt*ads_tot(i, j, k)*stefan_boltzmann*tmp_cell(i, j, k)*tmp_cell(i, j, k)*tmp_cell(i, j, k)*tmp_cell(i, j, k)/intensity_distribution_factor) &
							+ (dt*E_cell(i, j, k)) )												 &
							/( rho_cell(i, j, k)*cv_cell(i, j, k)+ (4.0d0*dt*ads_tot(i, j, k)*stefan_boltzmann*tmp_cell(i, j, k)*tmp_cell(i, j, k)*tmp_cell(i, j, k)/intensity_distribution_factor) )

					if(dtmp_cell_err< abs(dtmp)) dtmp_cell_err= abs(dtmp)
					tmp_cell(i, j, k)= tmp_cell(i, j, k)+ dtmp
				enddo
			enddo
		enddo
		!$omp end parallel do

		if(dtmp_cell_err< dtmp_cell_err_max) exit
	enddo

end