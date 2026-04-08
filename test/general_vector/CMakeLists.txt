
add_executable(general_vector_test
  general_vector_test.c)

target_link_libraries(general_vector_test jupiter-random libjupiter test-util)

add_test(NAME general-vector-test
  COMMAND ${__MPI} $<TARGET_FILE:general_vector_test>)
