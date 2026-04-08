
add_executable(serializer_test
  serializer_test.c
  serializer_test_buffer.c
  serializer_test_byteswap.c
  serializer_test_build.c
  serializer_test_parse.c
  serializer_test_array.c
  serializer_test_map.c)
set(SERIALIZER_TEST_JUPITER_LIB)
set(__P)
set(__T)
if (JUPITER_LINK_SHARED)
  set(__T shared)
else()
  set(__T static)
endif()
if (JUPITER_BUILD_DOUBLE)
  set(__P double)
elseif(JUPITER_BUILD_SINGLE)
  set(__P single)
endif()
if(__P AND __T)
  set(SERIALIZER_TEST_JUPITER_LIB libjupiter-${__P}-${__T})
endif()
target_link_libraries(serializer_test m jupiter-serializer
  ${SERIALIZER_TEST_JUPITER_LIB} test-util)
if(JUPITER_SERIALIZER_USE_OPENMP)
  set_target_properties(serializer_test PROPERTIES
    LINK_FLAGS "${OpenMP_C_FLAGS}")
endif()
if(NOT MPIEXEC STREQUAL "")
  set(__MPI "${MPIEXEC}" ${MPIEXEC_NUMPROC_FLAG} 1)
else()
  set(__MPI)
endif()
foreach(CASE buffer map array parse build byteswap)
  add_test(NAME serializer-test-${CASE}
    COMMAND ${__MPI} $<TARGET_FILE:serializer_test> ${CASE})
endforeach()
