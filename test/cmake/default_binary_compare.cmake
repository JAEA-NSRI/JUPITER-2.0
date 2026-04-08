if(POLICY CMP0053)
  cmake_policy(SET CMP0053 NEW)
endif()

set(INPUT "@AT@INPUT@AT@")
set(TEXTIZED_OUTPUT "@AT@TEXTIZED_OUTPUT@AT@")
set(REFERENCE "@AT@REFERENCE@AT@")
set(BIN2TXT_EXECUTABLE "@AT@BIN2TXT_EXECUTABLE@AT@")
set(BINCOMP_EXECUTABLE "@AT@BINCOMP_EXECUTABLE@AT@")
# *_EXPR is original expression, and *_EVAL is evaluated expression.
set(INPUT_NX_EXPR "@AT@INPUT_NX_EXPR@AT@")
set(INPUT_NX_EVAL "@AT@INPUT_NX_EVAL@AT@")
set(INPUT_NX "@AT@INPUT_NX@AT@")
set(INPUT_NY_EXPR "@AT@INPUT_NY_EXPR@AT@")
set(INPUT_NY_EVAL "@AT@INPUT_NY_EVAL@AT@")
set(INPUT_NY "@AT@INPUT_NY@AT@")
set(INPUT_NZ_EXPR "@AT@INPUT_NZ_EXPR@AT@")
set(INPUT_NZ_EVAL "@AT@INPUT_NZ_EVAL@AT@")
set(INPUT_NZ "@AT@INPUT_NZ@AT@")
set(INPUT_NUMBER_OF_ELEMENTS "@AT@INPUT_NUMBER_OF_ELEMENTS@AT@")
set(INPUT_FORMAT "@_JBD_TYPE@")
set(JUPITER_TEST_KEEP_COMPARED "@AT@JUPITER_TEST_KEEP_COMPARED@AT@")
set(BINCOMP_ABSOLUTE_TOLERANCE "@AT@BINCOMP_ABSOLUTE_TOLERANCE@AT@")
set(BINCOMP_RELATIVE_TOLERANCE "@AT@BINCOMP_RELATIVE_TOLERANCE@AT@")
set(BINCOMP_WRITE_ALL 1)

include("@AT@REFERENCE_UTILITY_SCRIPT@AT@")

if(EXISTS "${TEXTIZED_OUTPUT}")
  file(REMOVE "${TEXTIZED_OUTPUT}")
endif()

set(RET 1)
if(EXISTS "${INPUT}" AND EXISTS "${BIN2TXT_EXECUTABLE}")
  execute_process(COMMAND
    "${BIN2TXT_EXECUTABLE}" "${INPUT}"
    "${INPUT_NUMBER_OF_ELEMENTS}" "${INPUT_NX}" "${INPUT_NY}" "${INPUT_NZ}"
    "${TEXTIZED_OUTPUT}" "${INPUT_FORMAT}"
    RESULT_VARIABLE RET)
  if(NOT RET EQUAL 0)
    message(SEND_ERROR "bin2txt exited with failure. Return value: ${RET}")
  endif()
endif()

if(EXISTS "${REFERENCE}")
  if(NOT EXISTS "${INPUT}")
    mesasge(SEND_ERROR "${INPUT} is expected to be written, but not found")
    return()
  endif()
  set(CMD "${BINCOMP_EXECUTABLE}" "${INPUT}" "${REFERENCE}" "${INPUT_FORMAT}"
    TEXTIZED "${BINCOMP_RELATIVE_TOLERANCE}" "${BINCOMP_ABSOLUTE_TOLERANCE}"
    "${BINCOMP_WRITE_ALL}")
  string(REPLACE ";" " " CMDSTR "${CMD}")
  message(STATUS "Executing: ${CMDSTR}")
  execute_process(COMMAND ${CMD} RESULT_VARIABLE RETV)
  if(NOT "${RETV}" EQUAL 0)
    message(SEND_ERROR "Binary differences")
  endif()
endif()

#
# To update existing reference file:
#
#    @CMAKE_COMMAND@ -E env JUPITER_UPDATE_TEST_REFS=1 @CMAKE_COMMAND@ -P @AT@COMPARE_SCRIPT@AT@
#
update_reference("${TEXTIZED_OUTPUT}" "${REFERENCE}")

#
# To create reference file:
#
#    @CMAKE_COMMAND@ -E env JUPITER_CREATE_TEST_REFS=1 @CMAKE_COMMAND@ -P @AT@COMPARE_SCRIPT@AT@
#
# @warning Running through ctest will copy all data files as reference.
# @note Existing references will not be updated.
#
create_reference("${TEXTIZED_OUTPUT}" "${REFERENCE}")

if(NOT JUPITER_TEST_KEEP_COMPARED AND EXISTS "${TEXTIZED_OUTPUT}")
  file(REMOVE "${TEXTIZED_OUTPUT}")
endif()
