subroutine upwind_x(lat, lon, dt)

	!$ use omp_lib
	use MODULE_RADIATION

	implicit none
	integer:: i= 0, j= 0, k= 0
!   引数リスト
	real(8):: lat, lon, dt !lat = θ (緯度), lon = φ (経度)

	!$omp parallel do private(i, j)
	do k= 2, Nz- 1
		do j= 2, Ny- 1
			do i= 2, Nx- 1
				dIr(i, j, k)= 0.5d0*dt*cos(lat)*sin(lon)*(Ir(i-1, j, k)- Ir(i+1, j, k))/dx &
						    + 0.5d0*dt*dabs(cos(lat)*sin(lon))*(Ir(i-1, j, k)- 2.0d0*Ir(i, j, k)+ Ir(i+1, j, k))/dx
	    	enddo
		enddo
	enddo
	!$omp end parallel do


	!$omp parallel do private(i, j)
	do k= 2, Nz- 1
		do j= 2, Ny- 1
			do i= 2, Nx- 1
				Ir(i, j, k)= Ir(i, j, k)+ dIr(i, j, k)
	    	enddo
		enddo
	enddo
	!$omp end parallel do

	return
end



subroutine upwind_y(lat, lon, dt)

	!$ use omp_lib
	use MODULE_RADIATION

	implicit none
	integer:: i= 0, j= 0, k= 0
!	引数リスト
	real(8):: lat, lon, dt !lat = θ (緯度), lon = φ (経度)

	!$omp parallel do private(i, j)
	do k= 2, Nz- 1
		do j= 2, Ny- 1
			do i= 2, Nx- 1
				dIr(i, j, k)= 0.5d0*dt*cos(lon)*(Ir(i, j-1, k)- Ir(i, j+1, k))/dy &
					    	+ 0.5d0*dt*dabs(cos(lon))*(Ir(i, j-1, k)- 2.0d0*Ir(i, j, k)+ Ir(i, j+1, k))/dy
	    	enddo
		enddo
    enddo
	!$omp end parallel do


	!$omp parallel do private(i, j)
	do k= 2, Nz- 1
		do j= 2, Ny- 1
			do i= 2, Nx- 1
    			Ir(i, j, k)= Ir(i, j, k)+ dIr(i, j, k)
	    	enddo
		enddo
	enddo
	!$omp end parallel do

	return
end



subroutine upwind_z(lat, lon, dt)

	!$ use omp_lib
	use MODULE_RADIATION

	implicit none
	integer:: i= 0, j= 0, k= 0
!	引数リスト
	real(8):: lat, lon, dt !lat = θ (緯度), lon = φ (経度)

	!$omp parallel do private(i, j)
    do k= 2, Nz- 1
	    do j= 2, Ny- 1
		    do i= 2, Nx- 1
			    dIr(i, j, k)= 0.5d0*dt*sin(lat)*sin(lon)*(Ir(i, j, k-1)- Ir(i, j, k+1))/dz &
				    	    + 0.5d0*dt*dabs(sin(lat)*sin(lon))*(Ir(i, j, k-1)- 2.0d0*Ir(i, j, k)+ Ir(i, j, k+1))/dz
    		enddo
	    enddo
	enddo
	!$omp end parallel do


	!$omp parallel do private(i, j)
    do k= 2, Nz- 1
	    do j= 2, Ny- 1
		    do i= 2, Nx- 1
			    Ir(i, j, k)= Ir(i, j, k)+ dIr(i, j, k)
    		enddo
	    enddo
	enddo
	!$omp end parallel do

	return
end