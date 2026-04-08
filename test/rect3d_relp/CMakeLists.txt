
add_executable(rect3d_relp_test
  rect3d_relp_test.c
  make_cdo_test.c
  shuffle_test.c
  rect3d_relp_test_index_X.c
  rect3d_relp_test_index_Y.c
  rect3d_relp_test_index_Z.c
  rect3d_relp_test_index_C.c
  rect3d_relp_test_axis_X.c
  rect3d_relp_test_axis_Y.c
  rect3d_relp_test_axis_Z.c
  rect3d1_relp_test.c
  rect3d2_relp_test.c
  rect3d1_boundary_test.c)

target_link_libraries(rect3d_relp_test m libjupiter jupiter-random test-util)
set_target_properties(rect3d_relp_test PROPERTIES
  INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")

if (JUPITER_USE_MPI AND MPIEXEC_EXECUTABLE)
  set(CMD ${MPIEXEC_EXECUTABLE} ${MPIEXEC_PREFLAGS} ${MPIEXEC_NUMPROC_FLAG} 1
    ${MPIEXEC_POSTFLAGS})
else()
  set(CMD)
endif()

set(TESTS
  make_cdo
  shuffle
  rect3d_index_X
  rect3d_index_Y
  rect3d_index_Z
  rect3d_index_C
  rect3d_axis_X
  rect3d_axis_Y
  rect3d_axis_Z
  rect3d1_relp
  rect3d2_relp
  rect3d1_boundary
)

foreach(__T IN LISTS TESTS)
  add_test(NAME rect3d_relp_test-${__T}
    COMMAND ${CMD} $<TARGET_FILE:rect3d_relp_test> ${__T})
endforeach()
