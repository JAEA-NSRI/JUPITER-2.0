
if (NOT BUILD_TESTING OR NOT BUILD_JUPITER)
  return()
endif()

add_executable(getlptnp main.c)
target_link_libraries(getlptnp libjupiter)
