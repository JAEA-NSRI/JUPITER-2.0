! MPI 通信用配列に, 通信内容をコピーする
subroutine copy_array_double(nbuf, src, dst)
	implicit none
	integer :: nbuf
	real(8) :: src(nbuf)
	real(8) :: dst(nbuf)
	integer :: i
	dst(1:nbuf) = src(1:nbuf)
	return
end subroutine

! MPI 通信用配列から, 通信内容を抽出する
subroutine sub_array_double(nbuf, src, dst)
	implicit none
	integer :: nbuf
	real(8) :: src(nbuf)
	real(8) :: dst(nbuf)
	integer :: i
	dst(1:nbuf) = dst(1:nbuf) - src(1:nbuf)
	return
end subroutine

! 配列に, MPI 通信結果を足し合わせる
subroutine add_array_double(nbuf, src, dst)
	implicit none
	integer :: nbuf
	real(8) :: src(nbuf)
	real(8) :: dst(nbuf)
	integer :: i
	dst(1:nbuf) = dst(1:nbuf) + src(1:nbuf)
	return
end subroutine
