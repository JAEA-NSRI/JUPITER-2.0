if(BUILD_JUPITER)
  add_executable(ostest ostest.c)
  target_link_libraries(ostest libjupiter jupiter-os)
  add_test(NAME ostest COMMAND $<TARGET_FILE:ostest>
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()
