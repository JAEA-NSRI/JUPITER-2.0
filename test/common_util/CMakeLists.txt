
add_executable(common_util_test
  common_util_test.c)
target_link_libraries(common_util_test libjupiter test-util)
set_target_properties(common_util_test PROPERTIES
  INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME common_util_test COMMAND $<TARGET_FILE:common_util_test>)
