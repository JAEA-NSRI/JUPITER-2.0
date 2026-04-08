
if(NOT BUILD_JUPITER)
  return()
endif()

add_executable(pack2txt main.c)
target_link_libraries(pack2txt jupiter-serializer)

install(TARGETS pack2txt RUNTIME DESTINATION bin)
