
if(NOT BUILD_JUPITER)
  return()
endif()

jupiter_re2c_target(main.c.re)

add_executable(jupiter-dumpmesh main.c)
target_link_libraries(jupiter-dumpmesh libjupiter)

install(TARGETS jupiter-dumpmesh RUNTIME DESTINATION bin)
