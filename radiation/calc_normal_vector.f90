! 固体表面の法線ベクトル (単位でない) を計算する
! 境界の法線ベクトルも含めてこのサブルーチンで計算する
subroutine calc_normal_vector(ndim)

	!$ use omp_lib
	use MODULE_RADIATION

	implicit none

	integer(4):: ndim !引数リスト
	integer(4):: i= 0, j= 0, k= 0

	! 2016/3/14 : 外部プロセスから slr_cell を受け取る際に, 周期境界も満足されていると仮定している.
	! (ここで改めて slr_cell に周期境界は課さない)

	! 2D のときの norm_y= 0.0 は, 境界条件にて実施
	!$omp parallel do private(i, j)
	do k= 1, Nz- 1
		do j= 1, Ny- 1
			do i= 1, Nx- 1
				norm_x(i, j, k)= 0.25d0*( slr_cell(i, j, k)  + slr_cell(i, j, k+1)  + slr_cell(i, j+1, k)  + slr_cell(i, j+1, k+1) &
										- slr_cell(i+1, j, k)- slr_cell(i+1, j, k+1)- slr_cell(i+1, j+1, k)- slr_cell(i+1, j+1, k+1) )/dx
				norm_y(i, j, k)= 0.25d0*( slr_cell(i, j, k)  + slr_cell(i, j, k+1)  + slr_cell(i+1, j, k)  + slr_cell(i+1, j, k+1) &
										- slr_cell(i, j+1, k)- slr_cell(i, j+1, k+1)- slr_cell(i+1, j+1, k)- slr_cell(i+1, j+1, k+1) )/dy
				norm_z(i, j, k)= 0.25d0*( slr_cell(i, j, k)  + slr_cell(i, j+1, k)  + slr_cell(i+1, j, k)  + slr_cell(i+1, j+1, k) &
										- slr_cell(i, j, k+1)- slr_cell(i, j+1, k+1)- slr_cell(i+1, j, k+1)- slr_cell(i+1, j+1, k+1) )/dz
			enddo
		enddo
	enddo
	!$omp end parallel do

	return
end
