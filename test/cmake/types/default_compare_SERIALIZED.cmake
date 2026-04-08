set(INPUT "@INPUT@")
set(TEXTIZED_OUTPUT "@TEXTIZED_OUTPUT@")
set(REFERENCE "@REFERENCE@")
set(TWD "@TWD@")
set(DIFF_COMMAND "@DIFF_COMMAND@")
set(JUPITER_TEST_KEEP_COMPARED "@JUPITER_TEST_KEEP_COMPARED@")
set(PACK2TXT_EXECUTABLE "@PACK2TXT_EXECUTABLE@")

include("@REFERENCE_UTILITY_SCRIPT@")

if(EXISTS "${INPUT}")
  file(RELATIVE_PATH INPUT_REL "${TWD}" "${INPUT}")
  # pack2txt output contains specified file path
  execute_process(COMMAND "${PACK2TXT_EXECUTABLE}" "${INPUT_REL}"
    WORKING_DIRECTORY "${TWD}"
    OUTPUT_FILE "${TEXTIZED_OUTPUT}"
    RESULT_VARIABLE RETV)
  if(NOT "${RETV}" EQUAL 0)
    message(SEND_ERROR "pack2txt returned error ${RETV} for ${INPUT}")
  endif()
endif()

if(EXISTS "${REFERENCE}")
  if(NOT EXISTS "${INPUT}")
    message(SEND_ERROR "${INPUT} is expected to be written, but not found")
    return()
  endif()

  # TODO: Perform environment-independent comparison.
  # note: Output of pack2txt is technically environment-dependent.
  if(EXISTS "${DIFF_COMMAND}")
    # -w is not in POSIX's standard
    set(CMD "${DIFF_COMMAND}" -w -u "${REFERENCE}" "${TEXTIZED_OUTPUT}")
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
update_reference("${TEXTIZED_OUTPUT}" "${REFERENCE}")

#
# To create reference file:
#
#    @CMAKE_COMMAND@ -E env JUPITER_CREATE_TEST_REFS=1 @CMAKE_COMMAND@ -P @COMPARE_SCRIPT@
#
# @warning Running through ctest will copy all data files as reference.
# @note Existing references will not be updated.
#
create_reference("${TEXTIZED_OUTPUT}" "${REFERENCE}")

if(NOT JUPITER_TEST_KEEP_COMPARED AND EXISTS "${TEXTIZED_OUTPUT}")
  file(REMOVE "${TEXTIZED_OUTPUT}")
endif()
