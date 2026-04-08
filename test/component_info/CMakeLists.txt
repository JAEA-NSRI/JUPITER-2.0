
add_executable(component_info_test
  component_info_test.c)
target_link_libraries(component_info_test libjupiter test-util)
set_target_properties(component_info_test PROPERTIES
  INCLUDE_DIRECTORIES "${CMAKE_CURRENT_SOURCE_DIR}")
add_test(NAME component_info_test COMMAND $<TARGET_FILE:component_info_test>)
