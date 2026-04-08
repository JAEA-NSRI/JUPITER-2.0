
add_executable(dccalc_test
  dccalc_test.c)
target_link_libraries(dccalc_test m libjupiter jupiter-os test-util)
set_target_properties(dccalc_test PROPERTIES
  INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")

add_test(NAME dccalc_test COMMAND $<TARGET_FILE:dccalc_test>)
