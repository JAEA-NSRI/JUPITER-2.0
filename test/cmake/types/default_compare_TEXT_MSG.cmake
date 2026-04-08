set(INPUT "@INPUT@")
set(REFERENCE "@REFERENCE@")
set(NORMALIZED_OUTPUT "@NORMALIZED_OUTPUT@")
set(DIFF_COMMAND "@DIFF_COMMAND@")
set(JUPITER_TEST_KEEP_COMPARED "@JUPITER_TEST_KEEP_COMPARED@")
set(CMAKE_CURRENT_SOURCE_DIR "@CMAKE_CURRENT_SOURCE_DIR@")
set(TEST_SOURCE_DIR "@TEST_SOURCE_DIR@")
set(TEST_BINARY_DIR "@TEST_BINARY_DIR@")
set(EXPECTED_ERRORS "@EXPECTED_ERRORS@")

include("@REFERENCE_UTILITY_SCRIPT@")

if(EXISTS "${NORMALIZED_OUTPUT}")
  file(REMOVE "${NORMALIZED_OUTPUT}")
endif()

if(EXISTS "${INPUT}")
  file(READ "${INPUT}" MESSAGE)

  # Normalize CMAKE_CURRENT_(SOURCE|BINARY)_DIR from input. We also remove
  # OpenMPI and MPICH's MPI_Abort message. Note that other MPI library
  # use their own message.

  # OpenMPI's MPI_Abort message
  string(REGEX REPLACE "--*\nMPI_ABORT was invoked.*$" "\n" MESSAGE "${MESSAGE}")
  # MPICH's MPI_Abort message
  # (DeinoMPI, MSMPI and IntelMPI should match this pattern)
  string(REGEX REPLACE "\n(Abort\\([0-9]+\\) on node [0-9]+ \\(rank [0-9]+ in comm [0-9]+\\): )?application called MPI_Abort\\(.*$" "\n" MESSAGE "${MESSAGE}")
  # OpenMPI's non-0 exit message
  string(REGEX REPLACE "--*\nPrimary +job +terminated normally, +but +[0-9]+ process(es)? returned.*$" "" MESSAGE "${MESSAGE}")
  # Replace Host_name to dummy one
  string(REGEX REPLACE "Host_name = [^\n]*" "Host_name = localhost" MESSAGE "${MESSAGE}")
  # Normalize exponentinal notation to glibc (I'm not sure it is standardized at POSIX or SUS.)
  string(REGEX REPLACE "([-+]?[0-9.][0-9.]*[eE][+-])0([0-9][0-9])" "\\1\\2" MESSAGE "${MESSAGE}")
  # Normalize computational time
  string(REGEX REPLACE "\n([^\n]* = )[^\n]*( \\[sec/step\\])(, [^\n]* \\[percent\\])*[^\n]*" "\n\\1<certain>\\2" MESSAGE "${MESSAGE}")

  # Normalize source names
  string(REGEX REPLACE ";" "\\\\;" LINES "${MESSAGE}")
  string(REGEX REPLACE "\n" ";" LINES "${LINES}")
  set(FILES)
  foreach(__L IN LISTS LINES)
    # Assumes filenames do not have spaces and/or colons
    set(_R "^\\[[0-9 *]+\\]\\[( INFO| WARN|ERROR|FATAL)\\]: +(line +[0-9]+(, +column +[0-9]+)? +of +)?(([a-zA-Z]:)?[^ :]+).*$")
    string(REGEX MATCH "${_R}" _M "${__L}")
    if(NOT _M)
      continue()
    endif()
    string(REGEX REPLACE "${_R}" "\\4" __F "${__L}")
    if(NOT IS_ABSOLUTE "${__F}")
      set(__AFS)
      foreach(__C "${TEST_BINARY_DIR}" "${CMAKE_CURRENT_SOURCE_DIR}")
        file(TO_CMAKE_PATH "${__C}/${__F}" _AF)
        get_filename_component(_AF "${_AF}" ABSOLUTE)
        if (EXISTS "${_AF}")
          list(APPEND __AFS "${_AF}")
        endif()
      endforeach()
      list(LENGTH __AFS __AFS_L)
      if("${__AFS_L}" GREATER 1)
        set(__MSG "Multiple paths were hit for the name \"${__F}\":
")
        foreach(__C IN LISTS __AFS)
          set(__MSG "${__MSG}* ${__C}
")
        endforeach()
        message(WARNING "${__MSG}")
      elseif("${__AFS_L}" EQUAL 0)
        message(WARNING "No actual file found for \"${__F}\".
The function names are fine with this. These are indeterminable.")
        unset(_AF)
      else()
        list(GET __AFS 0 _AF)
      endif()
    else()
      set(_AF "${__F}")
    endif()
    if(DEFINED _AF)
      if (EXISTS "${_AF}")
        list(APPEND FILES "${_AF}")
      else()
        message(WARNING "Message contains the path of file \"${__F}\", but the absolute path of it, \"${_AF}\", could not be found.")
      endif()
    endif()
  endforeach()
  if(NOT "${FILES}" STREQUAL "")
    list(REMOVE_DUPLICATES FILES)
  endif()

  # Data files must not use these extensions.
  set(SOURCE_EXTS "\\.(cuh?|cxx|c\\+\\+|cpp|[cC]|[hH]|[fF](90|95|03|08|18)?)(\\.re)?$")
  foreach(__F IN LISTS FILES)
    file(RELATIVE_PATH __R "${TEST_BINARY_DIR}" "${__F}")
    set(__C "${__F}" "${__R}")
    file(TO_NATIVE_PATH "${__R}" _RN)
    if(NOT "${__R}" STREQUAL "${_RN}")
      list(APPEND __C "${_RN}")
    endif()
    file(TO_NATIVE_PATH "${__F}" _FN)
    if(NOT "${__F}" STREQUAL "${_FN}")
      list(APPEND __C "${_FN}")
    endif()
    foreach(__S IN LISTS __C)
      string(REPLACE "." "\\." __S "${__S}")
      set(__P "((: +)(line +[0-9]+(, +column +[0-9]+)? +of +)?)${__S}:")
      if("${__F}" MATCHES "${SOURCE_EXTS}")
        string(REGEX REPLACE "${__P}" "\\2<Source:${__F}>:" MESSAGE "${MESSAGE}")
      else()
        string(REGEX REPLACE "${__P}" "\\1${__F}:" MESSAGE "${MESSAGE}")
      endif()
    endforeach()
  endforeach()

  string(REPLACE "${TEST_BINARY_DIR}" "<PBD>" MESSAGE "${MESSAGE}")
  string(REPLACE "${TEST_SOURCE_DIR}" "<PSD>" MESSAGE "${MESSAGE}")
  file(WRITE "${NORMALIZED_OUTPUT}" "${MESSAGE}")

  if(NOT "${EXPECTED_ERRORS}" STREQUAL "")
    set(M "")
    foreach(_L IN LISTS EXPECTED_ERRORS)
      if("${MESSAGE}" MATCHES "(^|\n)(\\[[ 0-9*]+\\])?${_L}\n")
        message(STATUS "Message \"${_L}\" -- FOUND")
      else()
        set(M "${M}
 * \"${_L}\"")
      endif()
    endforeach()
    if(NOT "${M}" STREQUAL "")
      message(SEND_ERROR "Following message(s) has not been printed!${M}")
    endif()
  endif()
endif()

if(EXISTS "${REFERENCE}")
  if(NOT EXISTS "${INPUT}")
    message(SEND_ERROR "${INPUT} is expected to be written, but not found")
    return()
  endif()

  if(EXISTS "${DIFF_COMMAND}")
    # -w is not in POSIX's standard
    set(CMD "${DIFF_COMMAND}" -w -u "${REFERENCE}" "${NORMALIZED_OUTPUT}")
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
update_reference("${NORMALIZED_OUTPUT}" "${REFERENCE}")

#
# To create reference file:
#
#    @CMAKE_COMMAND@ -E env JUPITER_CREATE_TEST_REFS=1 @CMAKE_COMMAND@ -P @COMPARE_SCRIPT@
#
# @warning Running through ctest will copy all data files as reference.
# @note Existing references will not be updated.
#
create_reference("${NORMALIZED_OUTPUT}" "${REFERENCE}")

if(NOT JUPITER_TEST_KEEP_COMPARED AND EXISTS "${NORMALIZED_OUTPUT}")
  file(REMOVE "${NORMALIZED_OUTPUT}")
endif()
