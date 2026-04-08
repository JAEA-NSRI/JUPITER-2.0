function(copy_to_reference INPUT REFERENCE)
  if(NOT EXISTS "${INPUT}")
    message(STATUS "No file can be copied to reference from ${INPUT}")
    return()
  endif()
  execute_process(
    COMMAND "${CMAKE_COMMAND}" -E copy "${INPUT}" "${REFERENCE}"
    RESULT_VARIABLE RETV)
  if(NOT "${RETV}" EQUAL 0)
    message(FATAL_ERROR "Failed to copy ${INPUT} to ${REFERENCE}")
  else()
    message(STATUS "Reference copied: ${INPUT} -> ${REFERENCE}")
  endif()
endfunction()

function(update_reference INPUT REFERENCE)
  if(EXISTS "${REFERENCE}" AND
      ("$ENV{JUPITER_UPDATE_TEST_REFS}" OR "${JUPITER_UPDATE_TEST_REFS}"))
    copy_to_reference("${INPUT}" "${REFERENCE}")
  endif()
endfunction()

function(create_reference INPUT REFERENCE)
  if(NOT EXISTS "${REFERENCE}" AND
      ("$ENV{JUPITER_CREATE_TEST_REFS}" OR "${JUPITER_CREATE_TEST_REFS}"))
    copy_to_reference("${INPUT}" "${REFERENCE}")
  endif()
endfunction()
