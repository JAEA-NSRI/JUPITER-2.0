!! TPFIT compatibility layer
module MT_Parallel_Attribute0
#ifdef JUPITER_LPT_USE_MPI
#if defined(LPT_USE_MPI_F08_INTERFACE)
  use MPI_F08, only: MPI_COMM_WORLD, MPI_Comm
#else
  use MPI, only: MPI_COMM_WORLD
#endif
#endif

  integer :: &
       numrank,  &                !!< Number of MPI processes
       myrank                     !!< Rank number of "this" process

  integer, dimension(6) :: &
       nrk = -1                   !!< Neighbor MPI rank (-1 means no neighbor)
                                  !!  Stored in order of (Z-,Z+,Y-,Y+,X-,X+)

#ifdef JUPITER_LPT_USE_MPI
#if defined(LPT_USE_MPI_F08_INTERFACE)
  type(MPI_Comm) :: &
       lpt_comm = MPI_COMM_WORLD  !!< MPI communicator to be used
#else
  integer :: &
       lpt_comm = MPI_COMM_WORLD  !!< MPI communicator to be used
#endif
#endif
end module MT_Parallel_Attribute0

module numcon
  real(kind=8), parameter :: pi = acos(-1.0d0)
end module numcon

!! misc module
module LPTtype
  use ISO_C_BINDING, only: c_double, c_float

#ifdef JUPITER_LPT_API_SINGLE
  integer, parameter :: LPT_c_type = c_float
#else
  integer, parameter :: LPT_c_type = c_double
#endif
end module LPTtype
