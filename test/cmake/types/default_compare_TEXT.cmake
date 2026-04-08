set(INPUT "@INPUT@")
set(REFERENCE "@REFERENCE@")
set(DIFF_COMMAND "@DIFF_COMMAND@")

include("@REFERENCE_UTILITY_SCRIPT@")

if(EXISTS "${REFERENCE}")
  if(NOT EXISTS "${INPUT}")
    message(SEND_ERROR "${INPUT} is expected to be written, but not found")
    return()
  endif()

  if(EXISTS "${DIFF_COMMAND}")
    # -w is not in POSIX's standard
    set(CMD "${DIFF_COMMAND}" -w -u "${REFERENCE}" "${INPUT}")
    string(REPLACE ";" " " CMDSTR "${CMD}")
    message(STATUS "Executing: ${CMDSTR}")
    execute_process(COMMAND ${CMD}
      ERROR_VARIABLE DIFF_ERROR
      RESULT_VARIABLE RETV)
    if ("${RETV}" EQUAL 1)
      message(SEND_ERROR "Text differences")
    elseif("${RETV}" EQUAL 2)
      message(SEND_ERROR "Diff exited with error:
 ${DIFF_ERROR}")
    endif()
  endif()
endif()

#
# To update existing reference file:
#
#    @CMAKE_COMMAND@ -E env JUPITER_UPDATE_TEST_REFS=1 @CMAKE_COMMAND@ -P @COMPARE_SCRIPT@
#
update_reference("${INPUT}" "${REFERENCE}")

#
# To create reference file:
#
#    @CMAKE_COMMAND@ -E env JUPITER_CREATE_TEST_REFS=1 @CMAKE_COMMAND@ -P @COMPARE_SCRIPT@
#
# @warning Running through ctest will copy all data files as reference.
# @note Existing references will not be updated.
#
create_reference("${INPUT}" "${REFERENCE}")
