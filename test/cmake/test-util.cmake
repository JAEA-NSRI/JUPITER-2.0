
if(_JUPITER_TEST_UTIL_INCLUDED)
  return()
endif()
set(_JUPITER_TEST_UTIL_INCLUDED TRUE)

##
# @brief Compare result data file
# @param RESULT_VARIABLE Variable name to store the result
# @param INPUT Result data or message file (relative to WORKING_DIRECTORY)
# @param WORKING_DIRECTORY Path to the test working directory
# @param REFERENCE_DIRECTORY Path to the reference data stored
# @param NAME Binary data type name
# @param FORMAT Data type name
# @param GENERATE_ONLY If given, only generate scripts
# @param EXPECTED If TRUE given, makes error if result data file does not exist
# @param NUMBER_OF_ELEMENTS Number of elements per cell (used for raw binary)
# @param NX Number of X sizes (used for raw binary formats)
# @param NY Number of Y sizes (used for raw binary formats)
# @param NZ Number of Z sizes (used for raw binary formats)
# @param NX_EXPR Expression of NX value calculation
# @param NY_EXPR Expression of NY value calculation
# @param NZ_EXPR Expression of NZ value calculation
# @param NX_EVAL Evaluated expression of NX value calculation
# @param NY_EVAL Evaluated expression of NY value calculation
# @param NZ_EVAL Evaluated expression of NZ value calculation
# @param RELATIVE_TOLERANCE Relative tolerance (used for raw binary formats)
# @param ABSOLUTE_TOLERANCE Absolute tolerance (used for raw binary formats)
# @param EXPECTED_ERRORS Expected messages (applied for TEXT_MSG files)
#
# Call this function if least one of following files are used or expected name:
#  1. ${WORKING_DIRECTORY}/${INPUT}
#  2. ${REFERENCE_DIRECTORY}/${INPUT}
#  3. ${REFERENCE_DIRECTORY}/${INPUT}.cmake
#
# If 3. exists, uses it as textization and comparison script. In that CMake
# script, `@...@` notation will always be configured. Use "@AT@" for literal
# `@`.
#
# FORMAT will be used to choose how textize and compare the output file. If
# 3. exists, FORMAT parameter will be ignored.
#
# If 2. or 3. exists, defaults EXPECTED to TRUE. However, it is normally ignored
# if FALSE is given.
#
# If the realpath of INPUT is outside of WORKING_DIRECTORY, textization and
# comparison will be aborted for security's sake.
#
# The textization and comparison script will be called using `cmake -P`,
# not by `include()`.
#
# Variables:
#  - TEST_CURRENT_SOURCE_DIR Path to the test directory in JUPITER source
#  - TEST_CURRENT_BINARY_DIR Path to the test directory in the build directory
#
# TEST_SOURCE_DIRECTORY and TEST_BINARY_DIRECTORY are defaults to the value from
# the same variable in the parent scope. These variables exists for
#
function(compare_result_file)
  set(_L GENERATE_ONLY)
  set(_S RESULT_VARIABLE INPUT WORKING_DIRECTORY REFERENCE_DIRECTORY NAME FORMAT
    EXPECTED NUMBER_OF_ELEMENTS
    NX NY NZ NX_EXPR NY_EXPR NZ_EXPR NX_EVAL NY_EVAL NZ_EVAL
    RELATIVE_TOLERANCE ABSOLUTE_TOLERANCE)
  set(_M EXPECTED_ERRORS)

  cmake_policy(PUSH)
  if (POLICY CMP0174)
    # CMP0174 was added in CMake 3.31.
    #
    # Actually, we do not depend on the OLD behavior, does not depend on whether
    # a variable is defined or not. Just avoiding CMake emits warnings.
    cmake_policy(SET CMP0174 OLD)
  endif()

  cmake_parse_arguments(PARSE_ARGV 0 _CRF "${_L}" "${_S}" "${_M}")
  cmake_policy(POP)

  if(NOT _CRF_INPUT)
    message(FATAL_ERROR "No input file (result data file) specified")
  endif()
  if(NOT _CRF_WORKING_DIRECTORY)
    message(FATAL_ERROR "No WORKING_DIRECTORY specified")
  endif()
  if(NOT _CRF_REFERENCE_DIRECTORY)
    message(FATAL_ERROR "No REFERENCE_DIRECTORY specified")
  endif()
  if(NOT _CRF_FORMAT)
    message(FATAL_ERROR "No FORMAT specified
Note: FORMAT argument is still required even if custom script has been applied for comparison")
  endif()

  set(RET TRUE)
  if(NOT IS_ABSOLUTE "${_CRF_INPUT}")
    set(INPUT "${_CRF_WORKING_DIRECTORY}/${_CRF_INPUT}")
  else()
    set(INPUT "${_CRF_INPUT}")
  endif()
  file(RELATIVE_PATH REL_INPUT "${_CRF_WORKING_DIRECTORY}" "${INPUT}")
  if("${REL_INPUT}" MATCHES "^\\.\\.[/\\]")
    message(FATAL_ERROR "Input file '${_CRF_INPUT}' is outside of working directory '${_CRF_WORKING_DIRECTORY}'")
  endif()

  set(REFERENCE "${_CRF_REFERENCE_DIRECTORY}/${REL_INPUT}")
  set(REFERENCE_SCRIPT "${_CRF_REFERENCE_DIRECTORY}/${REL_INPUT}.cmake")
  set(TESTER_SCRIPT "${REFERENCE_SCRIPT}")
  set(TEXTIZED_OUTPUT "${INPUT}.txt")
  set(NORMALIZED_OUTPUT "${INPUT}.nom")
  set(COMPARE_SCRIPT "${INPUT}.cmake")
  set(INPUT_NUMBER_OF_ELEMENTS "${_CRF_NUMBER_OF_ELEMENTS}")
  set(NAME "${_CRF_NAME}")
  set(FORMAT "${_CRF_FORMAT}")
  set(INPUT_NX "${_CRF_NX}")
  set(INPUT_NY "${_CRF_NY}")
  set(INPUT_NZ "${_CRF_NZ}")
  set(INPUT_NX_EXPR "${_CRF_NX_EXPR}")
  set(INPUT_NY_EXPR "${_CRF_NY_EXPR}")
  set(INPUT_NZ_EXPR "${_CRF_NZ_EXPR}")
  set(INPUT_NX_EVAL "${_CRF_NX_EVAL}")
  set(INPUT_NY_EVAL "${_CRF_NY_EVAL}")
  set(INPUT_NZ_EVAL "${_CRF_NZ_EVAL}")
  set(BINCOMP_ABSOLUTE_TOLERANCE "${_CRF_ABSOLUTE_TOLERANCE}")
  set(BINCOMP_RELATIVE_TOLERANCE "${_CRF_RELATIVE_TOLERANCE}")
  set(COMPARE_SCRIPT_OUT "${INPUT}.difo")
  set(COMPARE_SCRIPT_ERR "${INPUT}.dife")
  string(REPLACE "\\" "\\\\" EXPECTED_ERRORS "${_CRF_EXPECTED_ERRORS}")

  # Check for symbolic links in paths does not designates outside of working
  # directory.
  get_filename_component(REAL_INPUT "${INPUT}" REALPATH)
  get_filename_component(REAL_WORKDIR "${_CRF_WORKING_DIRECTORY}" REALPATH)
  file(RELATIVE_PATH REAL_REL_INPUT "${REAL_WORKDIR}" "${REAL_INPUT}")
  if("${REAL_REL_INPUT}" MATCHES "^\\.\\.[/\\]")
    message(FATAL_ERROR "Input file '${_CRF_INPUT}' (resolved as '${REAL_INPUT}') is outside of working directory '${_CRF_WORKING_DIRECTORY}' (resolved as '${REAL_WORKDIR}'")
  endif()

  # Existence check
  if(_CRF_EXPECTED AND NOT EXISTS "${REAL_INPUT}")
    message(SEND_ERROR "File '${_CRF_INPUT}' not found
Note: That file is expected to be written")
    set(RET FALSE)
  endif()
  if(_CRF_RESULT_VARIABLE)
    set(${_CRF_RESULT_VARIABLE} "${RET}" PARENT_SCOPE)
  endif()

  if(NOT EXISTS "${TESTER_SCRIPT}")
    set(SPATH "cmake/types/default_compare_${FORMAT}.cmake")
    if(EXISTS "${TEST_CURRENT_BINARY_DIR}/${SPATH}")
      set(TESTER_SCRIPT "${TEST_CURRENT_BINARY_DIR}/${SPATH}")
    else()
      set(TESTER_SCRIPT "${TEST_CURRENT_SOURCE_DIR}/${SPATH}")
    endif()
  endif()
  configure_file("${TESTER_SCRIPT}" "${COMPARE_SCRIPT}" @ONLY)

  set(COMP_MESSAGE "Compare '${REL_INPUT}'")
  if(EXISTS "${REFERENCE}" OR EXISTS "${REFERENCE_SCRIPT}" OR
      NOT "${EXPECTED_ERRORS}" STREQUAL "" OR
      "$ENV{JUPITER_TEST_COMPARE_VERBOSE}" GREATER 0)
    set(VERBOSE TRUE)
    if("$ENV{JUPITER_TEST_COMPARE_VERBOSE}" GREATER 1)
      if(EXISTS "${REFERENCE}" OR EXISTS "${REFERENCE_SCRIPT}")
        set(REFM "")
      else()
        set(REFM ", No reference")
      endif()
      set(COMP_MESSAGE "${COMP_MESSAGE} (Name '${NAME}', Type '${FORMAT}'${REFM})")
    endif()
  else()
    set(VERBOSE FALSE)
  endif()

  if(NOT _CRF_GENERATE_ONLY)
    execute_process(COMMAND "${CMAKE_COMMAND}" -P "${COMPARE_SCRIPT}"
      OUTPUT_FILE "${COMPARE_SCRIPT_OUT}"
      ERROR_FILE "${COMPARE_SCRIPT_ERR}"
      RESULT_VARIABLE RET)
    if(RET EQUAL 0)
      if(VERBOSE)
        if(EXISTS "${REAL_INPUT}")
          message(STATUS "${COMP_MESSAGE} -- OK")
        else()
          message(STATUS "${COMP_MESSAGE} -- OK (not exist)")
        endif()
      endif()
    else()
      message(SEND_ERROR "${COMP_MESSAGE} failed
Details at:
 ${COMPARE_SCRIPT_OUT}
 ${COMPARE_SCRIPT_ERR}
")
    endif()
  else()
    if(VERBOSE)
      message(STATUS "${COMP_MESSAGE} -- Skip")
    endif()
  endif()
endfunction()

##
# @brief Compare datafile for binary datatype name
# @param RESULT_VARIABLE Variable name to store the result
# @param INPUT Result data or message file (relative to WORKING_DIRECTORY)
# @param WORKING_DIRECTORY Path to the test working directory
# @param REFERENCE_DIRECTORY Path to the reference data stored
# @param NAME Binary data type name
# @param CATEGORY File category
# @param GENERATE_ONLY If given, only generate scripts
# @param USE_RESTART If 1 given, use type for restart run
# @param EXPECTED If TRUE given, makes error if result data file does not exist
# @param EXPECT_ERRORS Expected messages (applied for TEXT_MSG files)
# @param NX Number of X sizes (used for raw binary formats)
# @param NY Number of Y sizes (used for raw binary formats)
# @param NZ Number of Z sizes (used for raw binary formats)
# @param NP Nubmer of particles (used for raw binary formats)
#
# Compute parameters for data file NAME defined by set_jupiter_datatype().
#
# CATEGORY must be one of BINARY (sequentially outputed binary file),
# RESTART (restart data) or OTHERS (other files)
#
# Variables used:
#
# ${BTYPES}: List of supported NAME
# ${BTYPE_${NAME}_TYPE0_${CATEGORY}}: Type name for initial run
# ${BTYPE_${NAME}_TYPE1_${CATEGORY}}: Type name for restart run
# ${BTYPE_${NAME}_NUM_ELEMENTS}: Number of elements per cell
# ${BTYPE_${NAME}_ABSOLUTE_TOLERANCE}: Absolute tolerance value
# ${BTYPE_${NAME}_RELATIVE_TOLERANCE}: Relative tolerance value
# ${BTYPE_${NAME}_NX_EXPR}: Expression for compute the data file NX
# ${BTYPE_${NAME}_NY_EXPR}: Expression for compute the data file NY
# ${BTYPE_${NAME}_NZ_EXPR}: Expression for compute the data file NZ
#
function(compare_datatype_file)
  set(_L GENERATE_ONLY)
  set(_S RESULT_VARIABLE INPUT WORKING_DIRECTORY REFERENCE_DIRECTORY NAME
    USE_RESTART CATEGORY EXPECTED NX NY NZ NP)
  set(_M EXPECTED_ERRORS)

  cmake_parse_arguments(PARSE_ARGV 0 _CBF "${_L}" "${_S}" "${_M}")

  foreach(_V RESULT_VARIABLE INPUT WORKING_DIRECTORY REFERENCE_DIRECTORY NAME
      EXPECTED EXPECTED_ERRORS)
    if(NOT "${_CBF_${_V}}" STREQUAL "")
      set(_CRF_${_V} ${_V} "${_CBF_${_V}}")
    endif()
  endforeach()
  foreach(_V GENERATE_ONLY)
    if(${_CBF_${_V}})
      set(_CRF_${_V} ${_V})
    else()
      set(_CRF_${_V})
    endif()
  endforeach()

  if(NOT "${_CBF_NAME}" IN_LIST BTYPES)
    message(FATAL_ERROR "${_CBF_NAME} is not defined datatype
Note: '${_CBF_INPUT}' specified this name
")
  endif()
  if(_CRF_USE_RESTART)
    set(RVAL 1)
  else()
    set(RVAL 0)
  endif()
  set(TYPEVAR "BTYPE_${_CBF_NAME}_TYPE${RVAL}_${_CBF_CATEGORY}")
  if(NOT DEFINED ${TYPEVAR} OR "${${TYPEVAR}}" STREQUAL "")
    message(FATAL_ERROR "Invalid category '${_CBF_CATEGORY}'
Note: \${${TYPEVAR}} is not defined or empty
")
  endif()

  set(INPUT_NX "${_CBF_NX}")
  set(INPUT_NY "${_CBF_NY}")
  set(INPUT_NZ "${_CBF_NZ}")
  set(INPUT_NP "${_CBF_NP}")
  set(INPUT_NE "${BTYPE_${_CBF_NAME}_NUM_ELEMENTS}")
  foreach(_D NX NY NZ)
    set(EXPRVAR "BTYPE_${_CBF_NAME}_${_D}_EXPR")
    set(INPUT_${_D}_EXPR "${${EXPRVAR}}")
    string(CONFIGURE "${INPUT_${_D}_EXPR}" INPUT_${_D}_EVAL @ONLY)
    if(NOT "${INPUT_${_D}_EVAL}" STREQUAL "")
      math(EXPR INPUT_R_${_D} "${INPUT_${_D}_EVAL}")
    else()
      set(INPUT_R_${_D} 0)
    endif()
  endforeach()

  compare_result_file(${_CRF_GENERATE_ONLY}
    ${_CRF_RESULT_VARIABLE}
    ${_CRF_INPUT} ${_CRF_WORKING_DIRECTORY} ${_CRF_REFERENCE_DIRECTORY}
    ${_CRF_NAME} ${_CRF_EXPECTED_ERRORS} ${_CRF_EXPECTED}
    FORMAT "${${TYPEVAR}}" NUMBER_OF_ELEMENTS "${INPUT_NE}"
    NX "${INPUT_R_NX}" NY "${INPUT_R_NY}" NZ "${INPUT_R_NZ}"
    NX_EXPR "${INPUT_NX_EXPR}" NX_EVAL "${INPUT_NX_EVAL}"
    NY_EXPR "${INPUT_NY_EXPR}" NY_EVAL "${INPUT_NY_EVAL}"
    NZ_EXPR "${INPUT_NZ_EXPR}" NZ_EVAL "${INPUT_NZ_EVAL}"
    ABSOLUTE_TOLERANCE "${BTYPE_${_CBF_NAME}_ABSOLUTE_TOLERANCE}"
    RELATIVE_TOLERANCE "${BTYPE_${_CBF_NAME}_RELATIVE_TOLERANCE}")
endfunction()

##
# Glob files matching in PATTERN with `%c`, `%i`, `%n` and `%r`.
#
# Return variables:
#
# - ${PREFIX}_FILES             -- List of files matched
# - ${PREFIX}_COMPONENT_NAMES   -- Extracted component names (%c) for each files
# - ${PREFIX}_COMPONENT_INDICES -- Extracted component index (%i) for each files
# - ${PREFIX}_TIMESTEP_INDICES  -- Extracted timestep index (%n) for each files
# - ${PREFIX}_RANK_NUMBERS      -- Extracted rank number (%r) for each files
#
# * `%c` matches to arbitrary string (including slash `/` and other specials.
#   JUPITER component name setter prohibits the use of characters other than
#   [A-Za-z_][A-Za-z0-9_]* (valid C name), but technically all characters which
#   OS supported will work correctly).
# * `%i`, `%n` and `%r` matches to arbitrary decimal numbers.
#
# Format options in %c, %i, %n and %r are ignored (may matches files that
# are not correctly formatted in specified format).
#
# Leading 0s and spaces will be stripped in extracted numbers or indices.
#
# NOTE: Number of `%[cinr]` is limited to 9. See `CMAKE_MATCH_<n>`.
#
function(glob_cinr_pattern PREFIX PATTERN)
  cmake_parse_arguments(PARSE_ARGV 2 _ARG "" "DIRECTORY" "")

  set(STR_KEYS c)
  set(NUM_KEYS i n r)
  list(JOIN STR_KEYS "" STR_C)
  list(JOIN NUM_KEYS "" NUM_C)
  set(C "${STR_C}${NUM_C}")

  if(NOT _ARG_DIRECTORY)
    set(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
  else()
    set(DIRECTORY "${_ARG_DIRECTORY}")
  endif()

  # Remove format options
  string(REGEX REPLACE
    "%[ #'0+-]*([1-9][0-9]*)?(\\.[1-9][0-9]*)?([${C}])" "%\\3" PATTERN
    "${PATTERN}")
  if("${PATTERN}" MATCHES "%[^${C}%]")
    message(FATAL_ERROR "(dev) Invalid pattern '${PATTERN}'")
  endif()

  # Generate glob pattern
  string(REGEX REPLACE "(%[${C}])+" "*" GLOB_PATTERN "${PATTERN}")
  string(REGEX REPLACE "%%" "%" GLOB_PATTERN "${GLOB_PATTERN}")

  # Generate regex pattern
  string(REGEX REPLACE [[([][.+*?()\^$])]] [[\\\1]] REGEX_PATTERN "${PATTERN}")
  string(REGEX REPLACE "%[${STR_C}]" "(.+)" REGEX_PATTERN "^${REGEX_PATTERN}\$")
  string(REGEX REPLACE "%[${NUM_C}]" "( *[+-]? *[0-9]+ *)" REGEX_PATTERN
    "${REGEX_PATTERN}")
  string(REGEX REPLACE "%%" "%" REGEX_PATTERN "${REGEX_PATTERN}")

  # Compute extract regex match indices
  set(M "${PATTERN}")
  foreach(P IN LISTS STR_KEYS NUM_KEYS)
    set(I_${P})
  endforeach()
  set(I 1)
  while(TRUE)
    string(REGEX REPLACE "^[^%]*%([${C}%])(.*)$" "\\2" N "${M}")
    set(X "${CMAKE_MATCH_1}")
    if("${N}" STREQUAL "${M}")
      break()
    endif()
    if("${X}" STREQUAL "%")
      continue()
    endif()
    if("${I}" GREATER 9)
      message(FATAL_ERROR "(dev) Pattern '${PATTERN}' has too many embedments")
    endif()
    list(APPEND "I_${X}" "${I}")
    math(EXPR I "${I} + 1")
    set(M "${N}")
  endwhile()

  # Find file candidates
  file(GLOB FILES LIST_DIRECTORIES FALSE
    RELATIVE "${DIRECTORY}" "${DIRECTORY}/${GLOB_PATTERN}")
  set(MFILES)
  foreach(P IN LISTS STR_KEYS NUM_KEYS)
    set(M_${P})
  endforeach()

  # Reject files that does not match to regex pattern
  foreach(_F IN LISTS FILES)
    if(NOT "${_F}" MATCHES "${REGEX_PATTERN}")
      continue()
    endif()
    list(APPEND MFILES "${_F}")
    foreach(P IN LISTS STR_KEYS NUM_KEYS)
      unset(X_${P})
      foreach(I IN LISTS I_${P})
        set(V "${CMAKE_MATCH_${I}}")
        if("${P}" IN_LIST NUM_KEYS)
          math(EXPR V "${V}")
        endif()
        if(NOT DEFINED X_${P})
          set(X_${P} "${V}")
        elseif(NOT "${V}" STREQUAL "${X_${P}}")
          message(WARNING "File '${_F}' matches to pattern '${PATTERN}' but same
embed part of `%${P}` matches to different value:
 - First  : ${X_${P}}
 - Another: ${V}
")
        endif()
      endforeach()
      if("${X_${P}}" STREQUAL "")
        set(X_${P} "%${P}-NOTFOUND")
      endif()
      list(APPEND "M_${P}" "${X_${P}}")
    endforeach()
  endforeach()

  set(${PREFIX}_FILES "${MFILES}" PARENT_SCOPE)
  set(${PREFIX}_COMPONENT_NAMES "${M_c}" PARENT_SCOPE)
  set(${PREFIX}_COMPONENT_INDICES "${M_i}" PARENT_SCOPE)
  set(${PREFIX}_TIMESTEP_INDICES "${M_n}" PARENT_SCOPE)
  set(${PREFIX}_RANK_NUMBERS "${M_r}" PARENT_SCOPE)
endfunction()

##
# Merge and remove duplicates for files matcheed by glob_cinr_pattern()
#
# If COMPONENT_INDICES has been matched in PREFIX_2 but not in PREFIX_1,
# or nonexistent in PREFIX_1, uses it.
#
# This is macro (runs in caller's scope).
#
macro(merge_cinr_files PREFIX_1 PREFIX_2)
  set(_SUFFS FILES COMPONENT_NAMES COMPONENT_INDICES TIMESTEP_INDICES
    RANK_NUMBERS)
  list(LENGTH ${PREFIX_2}_FILES NFILE_2)
  if(NFILE_2 GREATER 0)
    math(EXPR LFILE_2 "${NFILE_2}-1")
    foreach(IDX2 RANGE ${LFILE_2})
      list(GET ${PREFIX_2}_FILES ${IDX2} F)
      list(FIND ${PREFIX_1}_FILES "${F}" IDX1)
      if("${IDX1}" GREATER -1)
        list(GET ${PREFIX_1}_COMPONENT_INDICES ${IDX1} I1)
        list(GET ${PREFIX_2}_COMPONENT_INDICES ${IDX2} I2)
        if(NOT "${I2}" STREQUAL "" AND "${I1}" STREQUAL "")
          foreach(_C IN LISTS _SUFFS)
            list(REMOVE_AT ${PREFIX_1}_${_C} ${IDX1})
            list(GET "${PREFIX_2}_${_C}" ${IDX2} V)
            list(APPEND "${PREFIX_1}_${_C}" "${V}")
          endforeach()
        endif()
      else()
        foreach(_C IN LISTS _SUFFS)
          list(GET "${PREFIX_2}_${_C}" ${IDX2} V)
          list(APPEND "${PREFIX_1}_${_C}" "${V}")
        endforeach()
      endif()
    endforeach()
  endif()
endmacro()

## Same as glob_cinr_pattern() but for multiple patterns
#
# @param CMAKE If specified, look for `${PATTERNS}.cmake` too.
# @param PATTERNS Patterns
# @param DIRECTORY Directory to look for
#
function(glob_cinr_patterns PREFIX)
  cmake_parse_arguments(PARSE_ARGV 1 _ARG "CMAKE" "DIRECTORY" "PATTERNS")
  set(DIRECTORY)
  if(_ARG_DIRECTORY)
    set(DIRECTORY "DIRECTORY" "${_ARG_DIRECTORY}")
  endif()
  set(_SUFFS FILES COMPONENT_NAMES COMPONENT_INDICES TIMESTEP_INDICES
    RANK_NUMBERS)
  foreach(_S IN LISTS _SUFFS)
    set(_F_${_S})
  endforeach()
  foreach(_P IN LISTS _ARG_PATTERNS)
    glob_cinr_pattern(_M "${_P}" ${DIRECTORY})
    merge_cinr_files(_F _M)
    if(_ARG_CMAKE)
      glob_cinr_pattern(_M "${_P}.cmake" ${DIRECTORY})
      list(TRANSFORM _M_FILES REPLACE "\\.cmake$" "")
      merge_cinr_files(_F _M)
    endif()
  endforeach()

  foreach(_S IN LISTS _SUFFS)
    set(${PREFIX}_${_S} "${_F_${_S}}" PARENT_SCOPE)
  endforeach()
endfunction()

##
# Find files to compare
#
# @param PREFIX Output variable prefix
# @param RESTART Whether RESTART_PATTERNS should be looked for
# @param PATTERNS List of %c, %i, %n, %r file patterns to find
# @param RESTART_PATTERNS Same as PATTERNS but for restart run
# @param WORKING_DIRECTORY Working directory
# @param REFERENCE_DIRECTORY Directory of references
#
# Look for ${PATTRENS} and ${PATTERNS}.cmake in ${REFERENCE_DIRECTORY}
#
# Return variables:
#
# - ${PREFIX}_FILES             -- List of files matched
# - ${PREFIX}_COMPONENT_NAMES   -- Extracted component name (%c) for each files
# - ${PREFIX}_COMPONENT_INDICES -- Extracted component index (%i) for each files
# - ${PREFIX}_TIMESTEP_INDICES  -- Extracted timestep index (%n) for each files
# - ${PREFIX}_RANK_NUMBERS      -- Extracted rank number (%r) for each files
# - ${PREFIX}_USE_RESTART       -- Whether matching to restart run patterns
#
function(find_compare_files PREFIX)
  set(_L "")
  set(_S "RESTART;WORKING_DIRECTORY;REFERENCE_DIRECTORY")
  set(_M "PATTERNS;RESTART_PATTERNS")
  cmake_parse_arguments(PARSE_ARGV 1 _FCF "${_L}" "${_S}" "${_M}")

  glob_cinr_patterns(_F0
    DIRECTORY "${_FCF_WORKING_DIRECTORY}"
    PATTERNS ${_FCF_PATTERNS})

  glob_cinr_patterns(_R0 CMAKE
    DIRECTORY "${_FCF_REFERENCE_DIRECTORY}"
    PATTERNS ${_FCF_PATTERNS})

  merge_cinr_files(_R0 _F0)

  if(_FCF_RESTART)
    glob_cinr_patterns(_F1
      DIRECTORY "${_FCF_WORKING_DIRECTORY}"
      PATTERNS ${_FCF_RESTART_PATTERNS})

    glob_cinr_patterns(_R1 CMAKE
      DIRECTORY "${_FCF_REFERENCE_DIRECTORY}"
      PATTERNS ${_FCF_RESTART_PATTERNS})

    merge_cinr_files(_R1 _F1)
    merge_cinr_files(_R0 _R1)
    set(_UR)
    foreach(_F IN LISTS _R0_FILES)
      if("${_F}" IN_LIST _R1_FILES)
        list(APPEND _UR 1)
      else()
        list(APPEND _UR 0)
      endif()
    endforeach()
  else()
    set(_UR)
    foreach(_F IN LISTS _R0_FILES)
      list(APPEND _UR 0)
    endforeach()
  endif()

  set(${PREFIX}_FILES "${_R0_FILES}" PARENT_SCOPE)
  set(${PREFIX}_COMPONENT_NAMES "${_R0_COMPONENT_NAMES}" PARENT_SCOPE)
  set(${PREFIX}_COMPONENT_INDICES "${_R0_COMPONENT_INDICES}" PARENT_SCOPE)
  set(${PREFIX}_TIMESTEP_INDICES "${_R0_TIMESTEP_INDICES}" PARENT_SCOPE)
  set(${PREFIX}_RANK_NUMBERS "${_R0_RANK_NUMBERS}" PARENT_SCOPE)
  set(${PREFIX}_USE_RESTART "${_UR}" PARENT_SCOPE)
endfunction()

## Display condition summary
function(print_test_conf)
  if(_JUPITER_TEST_PRINT_TEST_CONF)
    return()
  endif()
  set(_JUPITER_TEST_PRINT_TEST_CONF TRUE PARENT_SCOPE)

  message(STATUS "Case Name: ${NAME}")
  message(STATUS "Working directory: ${CMAKE_CURRENT_SOURCE_DIR}")
  message(STATUS "JUPITER Executable: ${JUPITER_EXECUTABLE}")
  message(STATUS "Parameter input: ${INPUT0}")
  message(STATUS "Flags input: ${FLAGS0}")
  message(STATUS "Property list input: ${PLIST0}")
  message(STATUS "Geometry input: ${GEOME0}")
  message(STATUS "Control input: ${CONTR0}")
  if(RESTART)
    if(NOT "${INPUT1}" STREQUAL "${INPUT0}")
      message(STATUS "Parameter input (for restart): ${INPUT1}")
    endif()
    if(NOT "${FLAGS1}" STREQUAL "${FLAGS0}")
      message(STATUS "Flags input (for restart): ${FLAGS1}")
    endif()
    if(NOT "${PLIST1}" STREQUAL "${PLIST0}")
      message(STATUS "Property list input (for restart): ${PLIST1}")
    endif()
    if(NOT "${GEOME1}" STREQUAL "${GEOME0}")
      message(STATUS "Geometry input (for restart): ${GEOME1}")
    endif()
    if(NOT "${CONTR1}" STREQUAL "${CONTR0}")
      message(STATUS "Control input (for restart): ${CONTR1}")
    endif()
  endif()
  if(USE_MPI)
    message(STATUS "MPI Parallel #: ${MPI_NP}")
  else()
    message(STATUS "MPI Disabled (Use ${MPI_NP} process(es) when enabled)")
  endif()
  if(USE_OMP)
    message(STATUS "OpenMP Threads #: ${OMP_NT}")
  else()
    message(STATUS "OpenMP Disabled (Use ${OMP_NT} thread(s) when enabled)")
  endif()

  if(RESTART)
    set(REST_CON 0 1 PARENT_SCOPE)
  else()
    set(REST_CON 0 PARENT_SCOPE)
  endif()
endfunction()

function(make_jupiter_args REST ADD_RESTART_ARGS RETVAL)
  set(ARGS)
  if(NOT INPUT${REST} STREQUAL "")
    list(APPEND ARGS "-input" "${INPUT${REST}}")
  endif()
  if(NOT FLAGS${REST} STREQUAL "")
    list(APPEND ARGS "-flags" "${FLAGS${REST}}")
  endif()
  if(NOT PLIST${REST} STREQUAL "")
    list(APPEND ARGS "-plist" "${PLIST${REST}}")
  endif()
  if(NOT GEOME${REST} STREQUAL "")
    list(APPEND ARGS "-geom" "${GEOME${REST}}")
  endif()
  if(NOT CONTR${REST} STREQUAL "")
    list(APPEND ARGS "-control" "${CONTR${REST}}")
  endif()
  if(ADD_RESTART_ARGS AND NOT REST EQUAL 0)
    if (NOT RESTART_N STREQUAL "")
      list(APPEND ARGS "-restart" "${RESTART_N}")
    else()
      list(APPEND ARGS "-restart_job")
    endif()
  endif()
  if(NOT EXARG${REST} STREQUAL "")
    list(APPEND ARGS ${EXARG${REST}})
  endif()
  set(${RETVAL} "${ARGS}" PARENT_SCOPE)
endfunction()

function(build_mpiexec_args RETV)
  list(POP_FRONT ARGV _1)
  if(USE_MPI AND NOT "${MPIEXEC_EXECUTABLE}" STREQUAL "")
    list(POP_FRONT ARGV EXEC)
    if("${MPI_NP}" STREQUAL "" OR "${MPI_NP}" LESS 1)
      set(MPI_NP 1)
    endif()
    set(__R "${MPIEXEC_EXECUTABLE}" ${MPIEXEC_FLAGS}
      ${MPIEXEC_NUMPROC_FLAG} ${MPI_NP}
      ${MPIEXEC_PREFLAGS} ${EXEC} ${MPIEXEC_POSTFLAGS} ${ARGV})
  else()
    if(MPI_NP GREATER 1)
      message(WARNING "MPI not supported. Skip.")
      set(${RETV} "MPIEXEC-NOTFOUND" PARENT_SCOPE)
      return()
    endif()

    set(__R ${ARGV})
  endif()
  set(${RETV} "${__R}" PARENT_SCOPE)
endfunction()

function(make_vtk_args RETV)
  cmake_parse_arguments(PARSE_ARGV 1 RUN_MAKE_VTK "" "COMMAND;RESTART" "ARGS")
  make_jupiter_args(${RUN_MAKE_VTK_RESTART} FALSE _A)
  list(PREPEND _A "${RUN_MAKE_VTK_COMMAND}")
  list(APPEND _A "--" ${RUN_MAKE_VTK_ARGS})
  build_mpiexec_args(_R ${_A})
  set(${RETV} "${_R}" PARENT_SCOPE)
endfunction()

function(csv2table INPUT OUTPUT)
  if(NOT CSV2TABLE_EXECUTABLE)
    message(FATAL_ERROR "csv2table is not available")
  endif()

  set(_A ${ARGV})
  list(REMOVE_AT _A 0 1)
  execute_process(COMMAND "${CSV2TABLE_EXECUTABLE}" ${_A}
                          "${INPUT}" "${OUTPUT}"
                  RESULT_VARIABLE _RETV
                  OUTPUT_FILE "${OUTPUT}.out"
                  ERROR_FILE "${OUTPUT}.err")
  if(NOT _RETV EQUAL 0)
    message(FATAL_ERROR "Failed to execute csv2table")
  endif()
endfunction()
