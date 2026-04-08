
if(NOT BUILD_DOCS)
  return()
endif()

find_program(DOXYGEN_COMMAND doxygen)

configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in
  ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)

if(DOXYGEN_COMMAND)
  add_custom_target(doc
    COMMAND ${DOXYGEN_COMMAND} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
endif()
