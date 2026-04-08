
add_library(test-util STATIC test-util.c)

if(VASPRINTF_FOUND)
  target_compile_definitions(test-util PRIVATE HAVE_VASPRINTF ${VASPRINTF_DEFINITIONS})
endif()

target_link_libraries(test-util m)
target_include_directories(test-util
  INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

add_executable(test-util-test test-util-test.c)
target_link_libraries(test-util-test test-util m)

add_test(NAME test-util-test
  COMMAND ${CMAKE_COMMAND} -DTEST_EXECUTABLE=$<TARGET_FILE:test-util-test>
          -P ${CMAKE_CURRENT_SOURCE_DIR}/test-util-test.cmake)

add_library(test-util-cxx INTERFACE)
target_include_directories(test-util-cxx INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})

if (CMAKE_CXX_COMPILER_LOADED)
  add_executable(test-util-cxx-test test-util-cxx-test.cxx)
  target_link_libraries(test-util-cxx-test test-util-cxx)
  add_test(NAME test-util-cxx-test COMMAND $<TARGET_FILE:test-util-cxx-test>)
endif()
