
subroutine test
  use ISO_C_BINDING

  !! Format of module is very different for each compiler
  !! products, but we can make sure the MPI library is compiled with
  !! using compiler.
  use MPI

  implicit none

  double precision :: d(10)
  logical :: linit
  integer :: i(10), ierr, irank
  integer(kind=MPI_ADDRESS_KIND) :: ib, it, isint, isdbl
  integer(c_int) :: ic

  call MPI_Initialized(linit, ierr)
  if (.not. linit) then
     return
  endif

  call MPI_Comm_rank(MPI_COMM_WORLD, irank, ierr)
  if (irank .ne. 0) then
     return
  endif

  call MPI_Get_address(d(0:1), ib, ierr)
  call MPI_Get_address(d(1:2), it, ierr)
  isdbl = it - ib
  call MPI_Get_address(i(0:1), ib, ierr)
  call MPI_Get_address(i(1:2), it, ierr)
  isint = it - ib

  d(1) = 1.0d0           ! 0x1p+0
  d(2) = 0.5d0           ! 0x1p-1
  d(3) = 2.0d0           ! 0x1p+1
  d(4) = 0.1d0           ! 0

  i(1) =  0
  i(2) =  1
  i(3) = -2

  ic = isint
  call MPI_Send(ic, 1, MPI_INT, 1, 0, MPI_COMM_WORLD, ierr)
  ic = isdbl
  call MPI_Send(ic, 1, MPI_INT, 1, 1, MPI_COMM_WORLD, ierr)

  ic = 3
  call MPI_Send(ic, 1, MPI_INT, 1, 10, MPI_COMM_WORLD, ierr)
  ic = 4
  call MPI_Send(ic, 1, MPI_INT, 1, 11, MPI_COMM_WORLD, ierr)
  call MPI_Recv(ic, 1, MPI_INT, 1, 99, MPI_COMM_WORLD, MPI_STATUS_IGNORE, ierr)
  if (ic .ne. 0) then
     return
  end if

  call MPI_Send(i, 3, MPI_INTEGER, 1, 20, MPI_COMM_WORLD, ierr)
  call MPI_Send(d, 4, MPI_DOUBLE_PRECISION, 1, 21, MPI_COMM_WORLD, ierr)

  return
end subroutine test

program type_compat
  use ISO_C_BINDING
  use MPI

  implicit none

  interface
     function c_test() bind(c)
       use ISO_C_BINDING
       integer(c_int) :: c_test
     end function c_test
  end interface

  integer :: ierr, irank, nsize
  integer(c_int) :: ci

  ci = 0
  call MPI_Init(ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, irank, ierr)
  call MPI_Comm_rank(MPI_COMM_WORLD, nsize, ierr)
  if (irank .eq. 0) then
     call test
  elseif (irank .eq. 1) then
     ci = c_test()
  endif
  call MPI_Finalize(ierr)

  if (ci .ne. 0) then
     print *, 'error ', ci
     stop 1
  end if
  stop
end program type_compat
