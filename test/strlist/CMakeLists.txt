if(NOT TARGET libjupiter)
  return()
endif()

add_library(strlist-test-util STATIC strlist_test_util.c)
target_link_libraries(strlist-test-util libjupiter test-util)

add_executable(strlist_test strlist_test.c)
target_link_libraries(strlist_test libjupiter strlist-test-util test-util)
set(__ARGS $<TARGET_FILE:strlist_test>)
if (JUPITER_USE_MPI)
  list(PREPEND __ARGS
    ${MPIEXEC_EXECUTABLE} ${MPIEXEC_PREFLAGS} ${MPIEXEC_NUMPROC_FLAG} 1
    ${MPIEXEC_POSTFLAGS})
endif()
add_test(NAME strlist-test COMMAND ${__ARGS})
