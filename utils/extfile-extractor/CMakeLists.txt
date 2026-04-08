
add_executable(jupiter-extfile-extractor extfile-extractor.c)
target_link_libraries(jupiter-extfile-extractor libjupiter)
set_target_properties(jupiter-extfile-extractor PROPERTIES
  INCLUDE_DIRECTORIES "${CMAKE_CURRENT_BINARY_DIR};${CMAKE_CURRENT_SOURCE_DIR}"
  LINK_FLAGS "${OpenMP_C_FLAGS}")

install(TARGETS jupiter-extfile-extractor RUNTIME DESTINATION bin)
