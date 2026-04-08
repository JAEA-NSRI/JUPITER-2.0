##
# @ingroup CMakeScripts
# @file jupiter-test.cmake
# @brief Template functions for creating tests using JUPITER
#

include(CMakeParseArguments)

# Collect JUPITER sources.
unset(__L)
if (JUPITER_DOUBLE_EXECUTABLE AND TARGET "${JUPITER_DOUBLE_EXECUTABLE}")
  list(APPEND __L ${JUPITER_DOUBLE_EXECUTABLE})
endif()
if (JUPITER_SINGLE_EXECUTABLE AND TARGET "${JUPITER_SINGLE_EXECUTABLE}")
  list(APPEND __L ${JUPITER_SINGLE_EXECUTABLE})
endif()
if (NOT "${__L}" STREQUAL "")
  list(REMOVE_DUPLICATES __L)
endif()

function(jupiter_configure_if INPUT FILE EXT)
  set(_F "${FILE}")
  if(NOT _F STREQUAL "" AND NOT "${_F}" MATCHES "\n")
    if(NOT IS_ABSOLUTE "${_F}")
      get_source_file_property(_F_GEN "${_F}" GENERATED)
      if(_F_GEN)
        set(_F_FN "${CMAKE_CURRENT_BINARY_DIR}/${_F}")
      else()
        set(_F_FN "${CMAKE_CURRENT_SOURCE_DIR}/${_F}")
      endif()
    else()
      file(RELATIVE_PATH _F_FN "${CMAKE_CURRENT_SOURCE_DIR}" "${_F}")
      if(NOT _F_FN STREQUAL "")
        set(_F "${_F_FN}")
        set(_F_FN "${CMAKE_CURRENT_SOURCE_DIR}/${_F_FN}")
      endif()
    endif()
  else()
    set(_CNF_F "")
    set(_F_FN "")
  endif()
  if(EXISTS "${_F_FN}")
    get_filename_component(_B "${_F}" NAME)
    set(_CNF_F "${_F_FN}")
    if("${_B}" MATCHES "\\.in$")
      string(REGEX REPLACE "\\.in$" "" _B "${_B}")
      get_filename_component(_D "${_F}" DIRECTORY)
      string(REPLACE "../" "__/" _D "${_D}")
      file(MAKE_DIRECTORY "${_D}")
      set(_CNF_F "${WORKDIR}/${_D}/${_B}")
      configure_file("${_F}" "${_CNF_F}" @ONLY)
    endif()
  else()
    string(TOLOWER "${INPUT}" KEYNAME)
    set(_CNF_F "${WORKDIR}/${KEYNAME}.${EXT}")
    file(WRITE "${_CNF_F}" "${_F}")
  endif()
  set(JUPITER_INPUT_TEST_${INPUT} "${_CNF_F}" PARENT_SCOPE)
endfunction()

function(jupiter_test_process_expected_message)
  cmake_parse_arguments(_E "" "OUTPUT" "WARN;INFO;ERROR;FATAL" ${ARGV})
  set(_M)
  foreach(_L WARN INFO ERROR FATAL)
    foreach(_X IN LISTS _E_${_L})
      list(APPEND _M "\\\\[ *${_L} *\\\\]: .*${_X}")
    endforeach()
  endforeach()
  set(${_E_OUTPUT} "${_M}" PARENT_SCOPE)
endfunction()

function(jupiter_test_generate_script_set LIST NAME)
  cmake_parse_arguments(PARSE_ARGV 2 _S "CONFIGURE;GENERATE" "INPUT;OUTPUT"
    "VARS")
  list(APPEND "${LIST}" "${NAME}")
  if(_S_CONFIGURE)
    set("${NAME}_TYPE" CONFIGURE PARENT_SCOPE)
  elseif(_S_GENERATE)
    set("${NAME}_TYPE" GENERATE PARENT_SCOPE)
  else()
    message(FATAL_ERROR "No 'CONFIGURE' OR 'GENERATE' specified for '${NAME}'")
  endif()
  set("${NAME}" "${_S_OUTPUT}" PARENT_SCOPE)
  set("${NAME}_INPUT" "${_S_INPUT}" PARENT_SCOPE)
  set("${LIST}" "${${LIST}}" PARENT_SCOPE)
  set(_VARS)
  string(REPLACE "\\;" "$<SEMICOLON>" _S_VARS "${_S_VARS}")
  list(LENGTH _S_VARS _I)
  while(_I GREATER 1)
    list(GET _S_VARS 0 _VAR)
    list(GET _S_VARS 1 _VAL)
    list(REMOVE_AT _S_VARS 0 1)
    list(APPEND _VARS "${_VAR}")
    string(REPLACE "$<SEMICOLON>" ";" _VAL "${_VAL}")
    set("${NAME}_VARS_${_VAR}" "${_VAL}" PARENT_SCOPE)
    list(LENGTH _S_VARS _I)
  endwhile()
  set("${NAME}_VARS" "${_VARS}" PARENT_SCOPE)
endfunction()

function(jupiter_test_generate_script NAME)
  if("${${NAME}_TYPE}" STREQUAL "CONFIGURE")
    foreach(_V IN LISTS ${NAME}_VARS)
      set("${_V}" "${${NAME}_VARS_${_V}}")
    endforeach()
    configure_file("${${NAME}_INPUT}" "${${NAME}}" @ONLY)
  elseif("${${NAME}_TYPE}" STREQUAL "GENERATE")
    file(GENERATE OUTPUT "${${NAME}}" INPUT "${${NAME}_INPUT}")
  else()
    message(FATAL_ERROR "(dev) Unknown script format type '${${NAME}_TYPE}' for '${NAME}'")
  endif()
endfunction()

function(jupiter_test_generate_scripts LIST)
  foreach(_N IN LISTS "${LIST}")
    jupiter_test_generate_script("${_N}")
  endforeach()
endfunction()

##
# @ingroup CMakeScripts
# @brief Add make-vtk running configuration for JUPITER input tests
# @param NAME  Name of configuration
# @param OUTNAME Logfile output name
# @param MAKE_VTK Use make-vtk `$<TARGET_FILE:make-vtk>` for executable
# @param MAKE_VTKU Use make-vtk-unconventional `$<TARGET_FILE:make-vtk-unconventional>` for executable
# @param MAKE_XDMF Use make-xdmf `$<TARGET_FILE:make-xdmf>` for executable
# @param ARGS Arguments should be passed to the executable
# @param SKIP_INDICES If given, skips adding time indices arguments
# @param SKIP_MAKE_DIRECTORY If given, skip making output directory before run
# @param RESTART Convert restart data
# @param OUTDIR Output directory (add_jupiter_input_test() appends case name)
# @param PRE_RUN Additional script should be run before run make-vtk
# @param POST_RUN Additional script should be run after run make-vtk
# @param EXPECT Expected return value for normal run
# @param EXPECT_NO_INDICES Expected return value when there are no time indices
#                          are given
#
# * If EXPECT is not given, skips the return value check.
# * If EXPECT_NO_INDICES is not given, treats 0 (success) if EXPECT is also not
#   given, or use the same value of EXPECT.
#
function(jupiter_test_define_mkvtk_config)
  cmake_parse_arguments(PARSE_ARGV 0 MKVTKCONF
    "MAKE_VTK;MAKE_VTKU;MAKE_XDMF;SKIP_INDICES;SKIP_MAKE_DIRECTORY;RESTART"
    "NAME;OUTNAME;OUTDIR;EXPECT;EXPECT_NO_INDICES"
    "ARGS")

  set(_PREFIX "JUPITER_MKVTKCONF_${MKVTKCONF_NAME}")
  if(DEFINED "${_PREFIX}_NAME")
    message(FATAL_ERROR "(dev) The make-vtk config '${MKVTKCONF_NAME}' is already defined. Choose another one")
  endif()
  if(MKVTKCONF_MAKE_VTK)
    set("${_PREFIX}_EXECUTABLE" MKVTK_EXECUTABLE PARENT_SCOPE)
    set("${_PREFIX}_EXECUTABLE_NAME" "make-vtk" PARENT_SCOPE)
    set("${_PREFIX}_EXECUTABLE_LABEL" "make-vtk" PARENT_SCOPE)
    set("${_PREFIX}_EXECUTABLE_SKIP" SKIP_MKVTK PARENT_SCOPE)
  elseif(MKVTKCONF_MAKE_XDMF)
    set("${_PREFIX}_EXECUTABLE" MKXDMF_EXECUTABLE PARENT_SCOPE)
    set("${_PREFIX}_EXECUTABLE_NAME" "make-xdmf" PARENT_SCOPE)
    set("${_PREFIX}_EXECUTABLE_LABEL" "make-xdmf" PARENT_SCOPE)
    set("${_PREFIX}_EXECUTABLE_SKIP" SKIP_MKXDMF PARENT_SCOPE)
  elseif(MKVTKCONF_MAKE_VTKU)
    set("${_PREFIX}_EXECUTABLE" MKVTKU_EXECUTABLE PARENT_SCOPE)
    set("${_PREFIX}_EXECUTABLE_NAME" "make-vtk-unconventional" PARENT_SCOPE)
    set("${_PREFIX}_EXECUTABLE_LABEL" "make-vtk-unconventional" PARENT_SCOPE)
    set("${_PREFIX}_EXECUTABLE_SKIP" SKIP_MKVTKU PARENT_SCOPE)
  else()
    message(FATAL_ERROR "(dev) One of MAKE_VTK, MAKE_VTKU or MAKE_XDMF is required for selecting running converter")
  endif()
  set("${_PREFIX}_NAME" "${MKVTKCONF_NAME}" PARENT_SCOPE)
  set("${_PREFIX}_ARGS" "${MKVTKCONF_ARGS}" PARENT_SCOPE)
  set("${_PREFIX}_SKIP_INDICES" "${MKVTKCONF_SKIP_INDICES}" PARENT_SCOPE)
  set("${_PREFIX}_SKIP_MAKE_DIRECTORY"
    "${MKVTKCONF_SKIP_MAKE_DIRECTORY}" PARENT_SCOPE)
  set("${_PREFIX}_RESTART" "${MKVTKCONF_RESTART}" PARENT_SCOPE)
  set("${_PREFIX}_OUTNAME" "${MKVTKCONF_OUTNAME}" PARENT_SCOPE)
  set("${_PREFIX}_OUTDIR" "${MKVTKCONF_OUTDIR}" PARENT_SCOPE)
  set("${_PREFIX}_EXPECT" "${MKVTKCONF_EXPECT}" PARENT_SCOPE)
  if(NOT MKVTKCONF_EXPECT_NO_INDICES)
    if(NOT "${MKVTKCONF_EXPECT}" STREQUAL "")
      set(MKVTKCONF_EXPECT_NO_INDICES "${MKVTKCONF_EXPECT}")
    else()
      set(MKVTKCONF_EXPECT_NO_INDICES 0)
    endif()
  endif()
  set("${_PREFIX}_EXPECT_NO_INDICES" "${MKVTKCONF_EXPECT_NO_INDICES}"
    PARENT_SCOPE)
endfunction()

##
# @ingroup CMakeScripts
# @brief Define datatype and configure file
# @param TYPE type name to define
# @param CONFIGURE [COMPARE] Configure comparison template
# @param CONFIGURE SET Configure type derivation script template
# @param ABSOLUTE_TOLERANCE Default absolute tolerance for raw binary files
# @param RELATIVE_TOLERANCE Default relative tolerance for raw binary files
#
# One following scripts required:
#
#  - `${CMAKE_CURRENT_SOURCE_DIR}/cmake/types/default_compare_${TYPE}.cmake`
#  - `${CMAKE_CURRENT_BIANRY_DIR}/cmake/types/default_compare_${TYPE}.cmake`
#  - `${CMAKE_CURRENT_SOURCE_DIR}/cmake/types/set_${TYPE}.cmake`
#  - `${CMAKE_CURRENT_BINARY_DIR}/cmake/types/set_${TYPE}.cmake`

# If CONFIGURE [COMPARE] given, stores result of `configure_file()` as
# `${CMAKE_CURRENT_BINARY_DIR}/cmake/types/default_compare_${TYPE}.cmake`.
#
# If CONFIGURE SET given, stores result of `configure_file()` as
# `${CMAKE_CURRENT_BINARY_DIR}/cmake/types/set_${TYPE}.cmake`.
#
# You can use `@AT@` as literal `@` while configure template.
#
# Each textization/normalization and comparison process is defined in
#  `${CMAKE_CURRENT_BINARY_DIR}/cmake/types/default_compare_${TYPE}.cmake` or
#  `${CMAKE_CURRENT_SOURCE_DIR}/cmake/types/default_compare_${TYPE}.cmake`.
#
# For creating dynamic template switch, placing a script in
#  `${CMAKE_CURRENT_BINARY_DIR}/cmake/types/set_${TYPE}.cmake` or
#  `${CMAKE_CURRENT_SOURCE_DIR}/cmake/types/set_${TYPE}.cmake`.
# This files are evaluated while creating test cases.
#
# In that scipt, set following variables:
#
#  - _T0N   --- Type name for numbered (time-step) data from initial run
#  - _T1N   --- Type name for numbered (time-step) data from restart run
#  - _T0R   --- Type name for restart data from initial run
#  - _T1R   --- Type name for restart data from restart run
#  - _T0M   --- Type name for other data from initial run
#  - _T1M   --- Type name for other data from restart run
#
# If restart has been run in a test and both execution writes to same file name,
# the type in restart run will be used.
#
# You should not hardcode tolerances into the script. They are modifiable while
# declaring component names or creating test cases.
#
function(define_jupiter_datatype)
  cmake_parse_arguments(PARSE_ARGV 0 _JBD
    ""
    "TYPE;ABSOLUTE_TOLERANCE;RELATIVE_TOLERANCE"
    "CONFIGURE")
  if("${_JBD_TYPE}" IN_LIST JUPITER_DATATYPES)
    message(FATAL_ERROR "(dev) Type ${_JBD_TYPE} has been already defined")
  endif()
  if(_JBD_CONFIGURE)
    set(CONFD "${CMAKE_CURRENT_BINARY_DIR}/cmake/types")
    set(CONFF "${CONFD}/default_compare_${_JBD_TYPE}.cmake")
    list(POP_FRONT _JBD_CONFIGURE _CONF_TYPE)
    if("${_CONF_TYPE}" STREQUAL "CONFIGURE")
    elseif("${_CONF_TYPE}" STREQUAL "SET")
      set(CONFF "${CONFD}/set_${_JBD_TYPE}.cmake")
      list(POP_FRONT _JBD_CONFIGURE _CONF_FILE)
    else()
      set(_CONF_FILE "${_CONF_TYPE}")
    endif()
    set(AT "@")
    configure_file("${_CONF_FILE}" "${CONFF}" @ONLY)
  endif()
  set(_SDIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/types")
  set(_BDIR "${CMAKE_CURRENT_BINARY_DIR}/cmake/types")
  set(_SSETTER "${_SDIR}/set_${_JBD_TYPE}.cmake")
  set(_BSETTER "${_BDIR}/set_${_JBD_TYPE}.cmake")
  set(_SCOMPARATOR "${_SDIR}/default_compare_${_JBD_TYPE}.cmake")
  set(_BCOMPARATOR "${_BDIR}/default_compare_${_JBD_TYPE}.cmake")
  if(NOT (EXISTS "${_SSETTER}" OR EXISTS "${_BSETTER}" OR
        EXISTS "${_SCOMPARATOR}" OR EXISTS "${_BCOMPARATOR}"))
    message(FATAL_ERROR "(dev) Invalid datatype declaration for ${_JBD_NAMES}, ${_JBD_TYPE}
Note: One of required script does not found:
 - ${_SSETTER}
 - ${_BSETTER}
 - ${_SCOMPARATOR}
 - ${_BCOMPARATOR}
")
  endif()
  set(_PREFIX "JUPITER_DATATYPE_${_JBD_TYPE}")
  if(EXISTS "${_SSETTER}")
    set("${_PREFIX}_SETTER" "${_SSETTER}" PARENT_SCOPE)
  elseif(EXISTS "${_BSETTER}")
    set("${_PREFIX}_SETTER" "${_BSETTER}" PARENT_SCOPE)
  endif()
  if(EXISTS "${_SCOMPARATOR}")
    set("${_PREFIX}_COMPARATOR" "${_SCOMPARATOR}" PARENT_SCOPE)
  elseif(EXISTS "${_BCOMPARATOR}")
    set("${_PREFIX}_COMPARATOR" "${_BCOMPARATOR}" PARENT_SCOPE)
  endif()
  foreach(V ABSOLUTE_TOLERANCE RELATIVE_TOLERANCE)
    set("${_PREFIX}_${V}" "${_JBD_${V}}" PARENT_SCOPE)
  endforeach()
  set(_L "${JUPITER_DATATYPES}")
  list(APPEND _L "${_JBD_TYPE}")
  set(JUPITER_DATATYPES "${_L}" PARENT_SCOPE)
endfunction()

##
# @ingroup CMakeScripts
# @brief Declare output file datatype and format
# @param NAMES component name(s)
# @param TYPE data type
# @param NUM_ELEMENTS Number of elements per cell
# @param ABSOLUTE_TOLERANCE Absolute tolerance for raw binary data files
# @param RELATIVE_TOLERANCE Relative tolerance for raw binary data files
# @param NX_EXPR `math()` expression which the X size of the data file should be
# @param NY_EXPR `math()` expression which the Y size of the data file should be
# @param NZ_EXPR `math()` expression which the Z size of the data file should be
#
# TYPE must one of TYPE name by define_jupiter_datatype().
#
# @note Unregistered datafiles and datafiles does not match to expected output
# file pattern will not be compared
#
# The binary files are expected to be NX * NY * NZ. bin2txt also uses NX to when
# wrap the output.
#
# Following values can be used for `NX_EXPR`, `NY_EXPR` and `NZ_EXPR`. Note that
# these variables must be referenced with `@VAR@`.
#
#  - @INPUT_NX@: X size specified in param.txt
#  - @INPUT_NY@: Y size specified in param.txt
#  - @INPUT_NZ@: Z size specified in param.txt
#  - @INPUT_NP@: Number of particles in corresponding lpt_ctrl.dat
#
function(set_jupiter_datatype)
  cmake_parse_arguments(PARSE_ARGV 0 _JBD
    ""
    "TYPE;NUM_ELEMENTS;ABSOLUTE_TOLERANCE;RELATIVE_TOLERANCE;NX_EXPR;NY_EXPR;NZ_EXPR"
    "NAMES")
  set(_L "${JUPITER_BINARY_DATATYPES}")
  list(APPEND _L "${_JBD_NAMES}")
  list(REMOVE_DUPLICATES _L)
  if(NOT _JBD_NUM_ELEMENTS)
    set(_JBD_NUM_ELEMENTS 1)
  endif()
  if(NOT "${_JBD_TYPE}" IN_LIST JUPITER_DATATYPES)
    message(FATAL_ERROR "(dev) Type ${_JBD_TYPE} is not defined.
Use define_jupiter_datatype(TYPE ${_JBD_TYPE} ...) to define.
")
  endif()
  foreach(_NAME IN LISTS _JBD_NAMES)
    set(_PREFIX "JUPITER_BINARY_DATATYPE_${_NAME}")
    foreach(_V TYPE NUM_ELEMENTS)
      set("${_PREFIX}_${_V}" "${_JBD_${_V}}" PARENT_SCOPE)
    endforeach()
    foreach(_V ABSOLUTE_TOLERANCE RELATIVE_TOLERANCE)
      if(NOT DEFINED _JBD_${_V})
        set("_JBD_${_V}" "${JUPITER_DATATYPE_${_JBD_TYPE}_${_V}}")
      endif()
      set("${_PREFIX}_${_V}" "${_JBD_${_V}}" PARENT_SCOPE)
    endforeach()
    foreach(_V NX NY NZ)
      if(NOT DEFINED _JBD_${_V}_EXPR)
        set("_JBD_${_V}_EXPR" "@INPUT_${_V}@")
      endif()
      set("${_PREFIX}_${_V}_EXPR" "${_JBD_${_V}_EXPR}" PARENT_SCOPE)
    endforeach()
  endforeach()
  set(JUPITER_BINARY_DATATYPES "${_L}" PARENT_SCOPE)
endfunction()

# set default value for parameter
macro(jupiter_test_set_default VAR DEFAULT_VALUE)
  if(NOT DEFINED JUPITER_INPUT_TEST_${VAR})
    set(JUPITER_INPUT_TEST_${VAR} "${DEFAULT_VALUE}")
  endif()
endmacro()

# set same parameter to initial run for restart run
macro(jupiter_test_set_restart VAR)
  if(NOT DEFINED JUPITER_INPUT_TEST_RESTART_${VAR})
    set(JUPITER_INPUT_TEST_RESTART_${VAR} "${JUPITER_INPUT_TEST_${VAR}}")
  endif()
endmacro()

#
# @param LIST_L Output variable for list of logical parameters
# @param LIST_S Output variable for list of single value parameters
# @param LIST_M Output variable for list of multiple value parameters
# @param LOGICAL_PARAMS List of logical parameters
# @param SINGLE_PARAMS  List of single parameters
# @param MULTI_PARAMS   List of multiple parameters
# @param JUPITER_LOGICAL_PARAMS List of logical restart changeable parameters
# @param JUPITER_SINGLE_PARAMS  List of single restart changeable parameters
# @param JUPITER_MULTI_PARAMS   List of multiple restart changeable parameters
#
function(jupiter_test_build_parse_arguments_params)
  set(_L)
  foreach(_X LOGICAL SINGLE MULTI)
    list(APPEND _L "${_X}_PARAMS" "JUPITER_${_X}_PARAMS")
  endforeach()
  cmake_parse_arguments(PARSE_ARGV 0 _RP "" "LIST_L;LIST_S;LIST_M" "${_L}")
  set(__LOGICAL)
  set(__SINGLE)
  set(__MULTI)
  foreach(_L LOGICAL SINGLE MULTI)
    set("__${_L}" ${_RP_${_L}_PARAMS})
    foreach(_X IN LISTS _RP_JUPITER_${_L}_PARAMS)
      list(APPEND "__${_L}" "${_X}" "RESTART_${_X}")
    endforeach()
  endforeach()
  set(${_RP_LIST_L} "${__LOGICAL}" PARENT_SCOPE)
  set(${_RP_LIST_S} "${__SINGLE}" PARENT_SCOPE)
  set(${_RP_LIST_M} "${__MULTI}" PARENT_SCOPE)
endfunction()

# Process REQUIRES conditions
function(jupiter_test_requires OUT)
  set(SINGLE FALSE)
  set(DOUBLE FALSE)
  if (JUPITER_DOUBLE)
    set(DOUBLE TRUE)
  else()
    set(SINGLE TRUE)
  endif()

  set(MPI FALSE)
  if (JUPITER_USE_MPI)
    set(MPI TRUE)
  endif()

  set(OpenMP FALSE)
  if (ENABLE_OPENMP)
    set(OpenMP TRUE)
  endif()

  set(METI FALSE)
  if (JUPITER_ENABLE_METI)
    set(METI TRUE)
  endif()

  set(LPT FALSE)
  set(LPTX FALSE)
  set(ANY_LPT FALSE)
  if(JUPITER_ENABLE_LPT STREQUAL "LPT")
    set(ANY_LPT TRUE)
    set(LPT TRUE)
  elseif(JUPITER_ENABLE_LPT STREQUAL "LPTX")
    set(ANY_LPT TRUE)
    set(LPTX TRUE)
  endif()

  set(__SOLVERS "CG" "BiCG" "CUDA_CG")
  foreach(S IN LISTS __SOLVERS)
    set(${S} FALSE)
  endforeach()
  set("${JUPITER_SOLVER}" TRUE)
  if(NOT JUPITER_SOLVER IN_LIST __SOLVERS)
    message(SEND_ERROR "Please add ${JUPITER_SOLVER} to __SOLVERS list here")
  endif()

  set(FPE FALSE)
  if (ENABLE_FPE)
    set(FPE TRUE)
  endif()

  set(UNIFORM FALSE)
  if(JUPITER_USE_UNIFORM_GRID_EQUATION)
    set(UNIFORM TRUE)
  endif()

  set(S_BIG FALSE)
  if (JUPITER_SERIALIZER_USE_BIG_ENDIAN)
    set(S_BIG TRUE)
  endif()

  set(MBREAK FALSE)
  set(MRENORM FALSE)
  set(MEQU FALSE)
  if (JUPITER_MASS_SOURCE_WITHIN_EQUATION)
    set(MEQU TRUE)
  else()
    if (JUPITER_MASS_SOURCE_USE_BREAK_CONSTRAINT)
      set(MBREAK TRUE)
    else()
      set(MRENORM TRUE)
    endif()
  endif()

  set(EMULPT FALSE)
  if (LPTX AND JUPITER_LPTX_EMULATE_LPT_BEHAVIOR)
    set(EMULPT TRUE)
  endif()

  set(PROFILE FALSE)
  if (JUPTER_ENABLE_PROFILE)
    set(PROFILE TRUE)
  endif()

  set(RESHIST FALSE)
  if (JUPITER_ENABLE_RES_HISTORY)
    set(RESHIST TRUE)
  endif()

  include(TestBigEndian)
  test_big_endian(BIG)

  set(A ${ARGV})
  list(REMOVE_AT A 0)
  if ("${A}" STREQUAL "")
    list(APPEND A TRUE)
  endif()
  if (${A})
    set(${OUT} TRUE PARENT_SCOPE)
  else()
    set(${OUT} FALSE PARENT_SCOPE)
  endif()
endfunction()

##
# @ingroup CMakeScripts
# @brief Add test JUPITER with specific input.
# @param NAME   Name of test
# @param PARAMS Parameter input file (-input)
# @param FLAGS  Flag input file (-flags)
# @param PLIST  Property list file (-plist)
# @param GEOMETRY Geometry input file (-geom)
# @param CONTROL Control geometry input file (-control)
# @param ARGS   Extra arguments for JUPITER
# @param PRE_RUN If given, execute given CMake script before JUPITER run.
# @param POST_RUN If given, execute given CMake script after JUPITER run.
# @param MPI_NP Number of processes for MPI run
# @param OpenMP_NT Number of threads for OpenMP parallel (note: default is 1)
# @param EXPECT If given, expected return value from JUPITER
# @param EXPECT_MESSAGES If given, expected error message (regexp)
# @param EXPECT_MSGFILES If given, list of expected message file names
#   (relative to message output directory)
# @param EXPECT_DUMPMESH If given, expected return value from dumpmesh
# @param MESSAGE_FILE Message output file names (default: `messages/%r.lst`)
# @param MESSAGE_FILE_DIRECTORY Message output directory (default: `messages`)
# @param BINARY_OUTPUT_DIRECTORY Expected binary output directory
#   (default: `data/binary_data`)
# @param RESTART_OUTPUT_DIRECTORY Expected restart output directory
#   (default: `data/binary_data`)
# @param BINARY_INPUT_DIRECTORY Copy files to here if restart is enabled
#   and RESTART_N is specified for test
#   (default: (same as BINARY_OUTPUT_DIRECTORY))
# @param RESTART_INPUT_DIRECTORY Copy files to here if restart is enabled
#   for test (default: (same as RESTART_OUTPUT_DIRECTORY))
# @param BINARY_OUTPUT_DOUBLE Parse files for binary outputs as double precision
#   for double precision JUPITER (default: `OFF`)
# @param RESTART_OUTPUT_DOUBLE Parse files for restart outputs as double
#   precision for double precision JUPITER (default: `ON`)
# @param BINARY_OUTPUT_MODE Unified or byprocess output for binary outputs
# @param RESTART_OUTPUT_MODE Unified or byprocess output for restart outputs
# @param BINARY_OUTPUT_TIME_FILE Binary output time filename pattern
#   (same as `output_filename_templates` specifier in flags input files,
#    but use just `%c`, `%n`, `%i`, `%r` to specify name location,
#    also, format options like `04` in `%04r` will ignored)
#    default: "%c/%n.dat")
# @param BINARY_OUTPUT_COMP_FILE Binary output component-based filename pattern
#   (see BINARY_OUTPUT_TIME_FILE, default: "%c_%i/%n.%r.single.dat" for
#    single precision output and "%c_%i/%n.%r.dat" for double precision output)
#   (note: suffix `.single.` is not required anymore. It's now controlled by
#    DOUBLE_PRECISION_OUTPUT option)
# @param BINARY_OUTPUT_DATA_FILE Binary output other data filename pattern
#   (see BINARY_OUTPUT_TIME_FILE, default: "%c/%n.%r.single.dat" for
#    single precision output and "%c/%n.%r.dat" for double precision output)
#   (note: suffix `.single.` is not required anymore. It's now controlled by
#    DOUBLE_PRECISION_OUTPUT option)
# @param RESTART_OUTPUT_TIME_FILE Same as BINARY_OUTPUT_TIME_FILE,
#   but for restart data files (default: "%c.dat")
# @param RESTART_OUTPUT_COMP_FILE Same as BINARY_OUTPUT_COMP_FILE,
#   but for restart data files (default: "%c_%i/%r.dat")
# @param RESTART_OUTPUT_DATA_FILE Same as BINARY_OUTPUT_DATA_FILE,
#   but for restart data files (default: "%c/%r.dat")
# @param GEOM_DUMP_FILES Filenames that expected to be written by Geom_dump
# @param LPT_OUTPUT_FILE LPT data output filename (default "LPT.dat")
# @param LPT_OUTPUT_EXPECT If given, expect whether LPT data output file has
#   been written or not
# @param RESTART If given, run restart with -restart_job.
# @param RESTART_N If given, run restart from specified number instead of
#   -restart_job
# @param RESTART_PRE_RUN Same as PRE_RUN but for restart run
# @param RESTART_POST_RUN Same as POST_RUN but for restart run
# @param RESTART_PARAMS If given, parameter input file for restart run
# @param RESTART_FLAGS If given, flag input file for restart run
# @param RESTART_PLIST If given, property list input file for restart run
# @param RESTART_ARGS  Extra arguements for JUPITER for restart run
# @param RESTART_GEOMETRY If given, geometry input file for restart run
# @param RESTART_CONTROL if given, control input file for restart run
# @param RESTART_EXPECT_MESSAGES If given, expected error message
#  for restart run
# @param RESTART_EXPECT_MSGFILES If given, list of expected message file names
#  for restart run
# @param RESTART_EXPECT_DUMPMESH If given, expected return value from dumpmesh
#  for restart inputs
# @param RESTART_MESSAGE_FILE Message output file names for restart run
# @param RESTART_MESSAGE_FILE_DIRECTORY Message output directory for restart run
# @param RESTART_BINARY_OUTPUT_DIRECTORY Same as BINARY_OUTPUT_DIRECTORY but
#   for restart run
# @param RESTART_RESTART_OUTPUT_DIRECTORY Same as RESTART_OUTPUT_DIRECTORY but
#   for restart run
# @param RESTART_DOUBLE_PRECISION_OUTPUT Same as DOUBLE_PRECISION_OUTPUT but for
#   restart run
# @param RESTART_BINARY_OUTPUT_TIME_FILE Same as BINARY_OUTPUT_TIME_FILE,
#   but for restart run
# @param RESTART_BINARY_OUTPUT_COMP_FILE Same as BINARY_OUTPUT_COMP_FILE,
#   but for restart run
# @param RESTART_BINARY_OUTPUT_DATA_FILE Same as BINARY_OUTPUT_DATA_FILE,
#   but for restart run
# @param RESTART_RESTART_OUTPUT_TIME_FILE Same as RESTART_OUTPUT_TIME_FILE,
#   but for restart run
# @param RESTART_RESTART_OUTPUT_COMP_FILE Same as RESTART_OUTPUT_COMP_FILE,
#   but for restart run
# @param RESTART_RESTART_OUTPUT_DATA_FILE Same as RESTART_OUTPUT_DATA_FILE,
#   but for restart run
# @param RESTART_LPT_OUTPUT_FILE LPT data output filename for restart run
# @param RESTART_LPT_OUTPUT_EXPECT Same as LPT_OUTPUT_EXPECT but for restart run
# @param RESTART_EXPECT Expected return value restart run (see below)
# @param COMPARE_TOLERANCES Case specific tolerances of binary file comparisons
# @param SKIP_MKVTK If given, skip all running make-vtk even if it is available.
# @param SKIP_MKVTKU If given, skip running make-vtk-conventional even if it is available.
# @param SKIP_MKXMDF If given, skip all running make-xdmf even if it is available.
# @param MKVTK_CONFIGS List of make-vtk running configurations for this test
# @param TIME_INDICES If given, specifies step numbers to be run on make-vtk/make-xdmf. If not given, converts all of available.
# @param RESTART_TIME_INDICES Same as TIME_INDICES but for restart run
# @param LABELS Labels for the test
# @param DISABLED Generate test files (e.g. script), but never runs it via ctest.
# @param HEAVY Marks that the test consumes many CPU, time, or memory resources.
# @param REQUIRES Specify prerequisites for test (see below)
#
# If ::BUILD_JUPITER evaluates to FALSE, this function does nothing.
#
# Generated test files will go `${CMAKE_BINARY_DIR}/input-tests/${NAME}`
# and tests run on there (WORKDIR).
#
# If any specified input file name ends with ".in" (and exists),
# format with `configure_file(... @ONLY)` and output result to
# ${WORKDIR}. Ex. if input file name is `a/b/c.txt.in`, create
# formatted input file `${CMAKE_CURRENT_SOURCE_DIR}/a/b/c.txt.in` with
# `configure_file(... @ONLY)` and output to `${WORKDIR}/a/b/c.txt`.
#
# Input files will referencing in absolute path.
#
# If an input files are not found in both of
# ::CMAKE_CURRENT_SOURCE_DIR and ::CMAKE_CURRENT_BINARY_DIR or
# includes newlines, the given text as input file. In this case,
# output file will be `${WORKDIR}/${KEYNAME}.txt`, where KEYNAME will
# be lowercased text of PARAMS, FLAGS, PLIST or GEOMETRY,
# corresponding specifier.
#
# Output files will be normalized to be effeciently compared with
# reference data. Binary output files will be converted to text format
# with bin2txt utility.
#
# If `RESTART_EXPECT` has been specified, it is used for expected return value
# for restart run, and `EXPECT` is expected return value for initial run.
#
# If `RESTART_EXPECT` has not been specified and RESTART is enabled,
# `EXPECT` is expected return value for **restart** run and always expects
# returning 0 for initial run.
#
# `EXPECT_MESSAGE` and `RESTART_EXPECT_MESSAGE` want CATEGORY and
# message pattern. For example, `ERROR "foo"` matches the lines with
# `[ERROR]: .... foo`. To pass multiple messages to same category, use
# `ERROR "foo" "bar"`.
#
# For `GEOM_DUMP_FILES` and `RESTART_GEOM_DUMP_FILES`, pass output mode
# `BYPROCESS` or `UNIFY` to determine the size of binary file.
#
#  * [RESTART_]GEOM_DUMP_FILES BYPROCESS a.dat b.dat c.dat
#    - Files a.dat, b.dat and c.dat are written in BYPROCESS mode
#  * [RESTART_]GEOM_DUMP_FILES UNIFY a.dat b.dat c.dat
#    - Files a.dat, b.dat and c.dat are written in UNIFY mode
#  * [RESTART_]GEOM_DUMP_FILES BYPROCESS a.dat b.dat UNIFY c.dat
#    - Files a.dat and b.dat are written in BYPROCESS mode
#    - File c.dat is written in UNIFY mode
#
# Files matching to `[RESTART_]GEOM_DUMP_FILES` are treat as written by
# `Geom_dump` and removed for general binary outputs.
#
# `COMPARE_TOLERANCES RELATIVE data_file_name value [...]`
# `COMPARE_TOLERANCES ABSOLUTE data_file_name value [...]` modifies
# default tolerances for binary file comparisons for this case only. Examples:
#
#  * COMPARE_TOLERANCES RELATIVE fl 1.0e-8
#    - Modifies relative tolerance of data `fl` to `1.0e-8`
#  * COMPARE_TOLERANCES ABSOLUTE fl 1.0e-8 Y 1.0e-9
#    - Modifies absolute tolerance of data `fl` to `1.0e-8`
#    - Modifies absolute tolerance of data `Y` to `1.0e-9`
#  * COMPARE_TOLERANCES RELATIVE fs -1
#                       ABSOLUTE Y  1.0e-6
#    - Modifies relative tolerance of data `fs` to `-1` (ignore)
#    - Modifies absolute tolerance of data `Y`  to `1.0e-6`
#
# To specify `[RESTART_]EXPECT_MSGFILES` to be an empty list, use the single
# colon `:`. Just specifying `[RESTART_]EXPECT_MSGFILES` is same as not
# specifying it (i.e., it will skip the check). See also CMake policy CMP0174,
# if you are using CMake 3.31 or later.
#
# To specify `MKVTK_CONFIGS` to be an empty list (no make-vtk/make-xdmf runs),
# use single colon `:`. Just specifying `MKVTK_CONFIGS` will choose the default.
#
# Tests with MPI_NP greater than 1 will be skipped if MPI is not enabled.
#
# OpenMP_NT will be ignored if OpenMP is not enabled.
#
# REQUIRES specifies prerequisites for the test. Given parameters will be passed
# as-is to `if()` (e.g., `MPI AND DOUBLE` for `MPI` and `DOUBLE`
# condition). Following variables are defined for writing conditions easily. By
# default, test will be run unconditionally (except for tests with MPI_NP > 1
# when MPI is not enabled).
#
#  - SINGLE  -- TRUE if JUPITER is single-precision (same as NOT DOUBLE)
#  - DOUBLE  -- TRUE if JUPITER is double-precision (same as NOT SINGLE)
#  - MPI     -- TRUE if MPI is enabled
#  - OpenMP  -- TRUE if OpenMP is enabled
#  - METI    -- TRUE if METI features (solute diffusion etc) are enabled
#  - LPT     -- TRUE if LPT (Fortran) module is used
#  - LPTX    -- TRUE if LPTX module is used
#  - ANY_LPT -- TRUE if particle track (LPT) feature is enabled
#  - CG      -- TRUE if the solver is CG
#  - BiCG    -- TRUE if the solver is BiCG
#  - CUDA_CG -- TRUE if the solver is CUDA CG
#  - FPE     -- TRUE if FPE is enabled
#  - UNIFORM -- TRUE if uniform-grid equation is used for uniform grid inputs
#  - S_BIG   -- TRUE if serializer stores data in big endian
#              (note: If not, serializer writes binary in system's endianess)
#  - MBREAK  -- TRUE if MASS_SOURCE_USE_BREAK_CONSTRAINT mode used
#  - MRENORM -- TRUE if MASS_SOURCE_USE_RENORMAL mode used
#  - MEQU    -- TRUE if MASS_SOURCE_WITHIN_EQUATION mode used
#  - EMULPT  -- TRUE if LPTX_EMULATE_LPT_BEHAVIOR is ON
#  - PROFILE -- TRUE if PROFILE is enabled
#  - RESHIST -- TRUE if printing residual history is enabled
#  - BIG     -- 1 if the target system is big endian (see TestBigEndian module)
#  - UNIX    -- TRUE if the target system is UNIX-like (see CMake's manual)
#  - WIN32   -- TRUE if the target system is native Windows (see CMake's manual)
#  - APPLE   -- TRUE if the target system is macOS family (see CMake's manual)
#  - CYGWIN  -- TRUE if the target system is Cygwin (see CMake's manual)
#
# @warning Setting different value for [BINARY|RESTART]_OUTPUT_*_FILE and
# RESTART_[BINARY|RESTART]_OUTPUT_*_FILE may not work correctly. Please report
# an issue if the behavior is suspicious.
#
# The generated test always includes `jupiter-input-test` label. `ctest -L`
# allows you to select test by their labels. See `ctest --help` and `cmake
# --help-property LABELS` for more info. The following labels are also set by
# automatically:
#
#  - cores-shortage: Test requires more CPU cores than the specified value in
#                    ${JUPITER_TEST_MAX_NUMPROCS}. If you build JUPITER with
#                    OpenMPI, you may have to add `--oversubscribe` to
#                    ${MPIEXEC_FLAGS} to run these tests. Or, you can exclude
#                    these with `ctest -LE cores-shortage`.
#
#  - disabled: DISABLED has been given. Note that `ctest -L disabled` does not
#              run these tests, i.e., they are still disabled.
#
#  - heavy: HEAVY has been given. Note that `ctest -L heavy` does not run these
#           tests if `JUPITER_TEST_ENABLE_HEAVY_TESTS` is `OFF`.
#
#  - missing-prereq: REQUIRES condition did not meet.
#
#  - jupiter-main: A test that runs JUPITER.
#
#  - jupiter-restart: A test that runs JUPITER with restart.
#
#  - compare-results: A test that compares the result of calculation or message
#
#  - make-vtk: A test that runs make-vtk for the result of a test.
#
#  - make-vtk-unconventional: A test that runs make-vtk-unconventional for the
#                             result of a test.
#
#  - make-xdmf: A test that runs make-xdmf for the result of a test.
#
# Small note: `ctest -L make-vtk` also selects make-vtk-unconventional. Use
#             `ctest -L make-vtk -LE make-vtk-unconventional` to exclude
#             unconventional version.
#
# Disabled tests will also be listed in `ctest`, but it can not be run via ctest
# in any way.
#
# Heavy tests will be enabled when build with `JUPITER_TEST_ENABLE_HEAVY_TESTS`
# is `ON`. Heavy tests are disabled by default. The DISABLED option has
# priority.
#
# Output files:
#   - ${WORKDIR}/${NAME}.out -- standard output
#   - ${WORKDIR}/${NAME}.err -- standard error output
#   - ${WORKDIR}/${NAME}.msh -- output of dumpmesh
#   - ${WORKDIR}/${NAME}-restart.out -- standard output for restart run
#   - ${WORKDIR}/${NAME}-restart.err -- standard error output for restart run
#   - ${WORKDIR}/${NAME}-restart.msh -- output of dumpmesh for restart inputs
#   - ${WORKDIR}/run-${NAME}-config.cmake -- Variable config script
#   - ${WORKDIR}/run-${NAME}-bin-config.cmake -- Binary config script
#   - ${WORKDIR}/run-${NAME}.cmake -- Test script (run all steps sequentially).
#   - ${WORKDIR}/run-${NAME}-jupiter.cmake         -- Run jupiter (initial)
#   - ${WORKDIR}/run-${NAME}-jupiter-restart.cmake -- Run jupiter (restart)
#   - ${WORKDIR}/run-${NAME}-compare.cmake -- Compare results
#   - ${WORKDIR}/run-${NAME}-mkvtk*.cmake -- Generate VTK files by VTK version
#   - ${WORKDIR}/run-${NAME}-mkvtku*.cmake -- Generate VTK files by unconventional version
#   - ${WORKDIR}/run-${NAME}-mkxdmf*.cmake -- Generate XDMF files
#   - ${WORKDIR}/data/binary_data -- Binary output directory (default)
#   - ${WORKDIR}/messages         -- Message output directory (default)
#   - ${WORKDIR}/**/*.difo  -- Difference to reference data
#   - ${WORKDIR}/**/*.dife  -- Error output text when run diff utility.
#   - ${WORKDIR}/**/*.nom   -- Data used for comparison (if normalized)
#   - ${WORKDIR}/**/*.txt   -- Data used for comparison (for binary files)
#   - ${WORKDIR}/**/*.cmake -- Script to compare the data
#   - ${WORKDIR}/*.dat -- Other output file from JUPITER if any.
#   - ${WORKDIR}/*.txt -- Input files which is directly given or configured.
#   - ${WORKDIR}/vtk  -- Output directory of VTK file
#   - ${WORKDIR}/vtku -- Output directory of VTK file by make-vtk-unconventional
#   - ${WORKDIR}/xdmf -- Output directory of XDMF file
#
# Test script run-${NAME}.cmake can be run directly via
#
#     cmake -P ${WORKDIR}/run-${NAME}.cmake
#
# In this case, the output will be written on the current directory of the
# shell, instead of ${WORKDIR}.
#
# Reference data stored in `data/${NAME}` as in same structure, but needs to be
# textized to be able to compare to different architectures.
#
# Message files and LPT output will be overwritten (or append) while restart by
# default configuration. To check separetely, please configure and make input
# files to output to different directory or file.
#
# Recommend to change output directory if you change configurations on restart
# run. In default configuration, files may be overwritten on restart run or may
# be undeterminable whether outputted by initial or restart run. If same output
# name to compare matches to spcified names for both initial and restart run, it
# treats them as outputted by restart run.
#
function(add_jupiter_input_test NAME)
  if(NOT BUILD_JUPITER)
    return()
  endif()

  set(INPUT_NAMES_BASE PARAMS FLAGS PLIST GEOMETRY CONTROL)
  set(SCRIPT_NAMES_BASE PRE_RUN POST_RUN)
  jupiter_test_build_parse_arguments_params(
    LIST_L _L LIST_S _S LIST_M _M

    LOGICAL_PARAMS
      RESTART SKIP_MKVTK SKIP_MKVTKU SKIP_MKXDMF DISABLED HEAVY

    SINGLE_PARAMS
      NAME MPI_NP OpenMP_NT RESTART_N
      BINARY_INPUT_DIRECTORY   RESTART_INPUT_DIRECTORY

    MULTI_PARAMS
      MKVTK_CONFIGS LABELS REQUIRES COMPARE_TOLERANCES

    JUPITER_SINGLE_PARAMS
      ${INPUT_NAMES_BASE} ${SCRIPT_NAMES_BASE} EXPECT EXPECT_DUMPMESH
      MESSAGE_FILE MESSAGE_FILE_DIRECTORY LPT_OUTPUT_FILE LPT_OUTPUT_EXPECT
      BINARY_OUTPUT_DIRECTORY  RESTART_OUTPUT_DIRECTORY
      BINARY_OUTPUT_DOUBLE     RESTART_OUTPUT_DOUBLE
      BINARY_OUTPUT_MODE       RESTART_OUTPUT_MODE
      BINARY_OUTPUT_TIME_FILE  RESTART_OUTPUT_TIME_FILE
      BINARY_OUTPUT_COMP_FILE  RESTART_OUTPUT_COMP_FILE
      BINARY_OUTPUT_DATA_FILE  RESTART_OUTPUT_DATA_FILE

    JUPITER_MULTI_PARAMS
      ARGS EXPECT_MESSAGES EXPECT_MSGFILES TIME_INDICES GEOM_DUMP_FILES
  )

  cmake_parse_arguments(JUPITER_INPUT_TEST "${_L}" "${_S}" "${_M}" ${ARGV})

  set(WORKDIR "${CMAKE_BINARY_DIR}/input-tests/${JUPITER_INPUT_TEST_NAME}")
  file(MAKE_DIRECTORY "${WORKDIR}")

  jupiter_test_set_default(BINARY_OUTPUT_DIRECTORY "data/binary_data")
  jupiter_test_set_default(RESTART_OUTPUT_DIRECTORY "data/binary_data")
  jupiter_test_set_default(BINARY_INPUT_DIRECTORY
    "${JUPITER_INPUT_TEST_BINARY_OUTPUT_DIRECTORY}")
  jupiter_test_set_default(RESTART_INPUT_DIRECTORY
    "${JUPITER_INPUT_TEST_RESTART_OUTPUT_DIRECTORY}")

  jupiter_test_set_restart(BINARY_OUTPUT_DIRECTORY)
  jupiter_test_set_restart(RESTART_OUTPUT_DIRECTORY)

  jupiter_test_set_default(BINARY_OUTPUT_DOUBLE OFF)
  jupiter_test_set_restart(BINARY_OUTPUT_DOUBLE)
  jupiter_test_set_default(BINARY_OUTPUT_MODE UNIFY)
  jupiter_test_set_restart(BINARY_OUTPUT_MODE)

  jupiter_test_set_default(RESTART_OUTPUT_DOUBLE ON)
  jupiter_test_set_restart(RESTART_OUTPUT_DOUBLE)
  jupiter_test_set_default(RESTART_OUTPUT_MODE BYPROCESS)
  jupiter_test_set_restart(RESTART_OUTPUT_MODE)

  jupiter_test_set_default(MESSAGE_FILE "messages/%r.lst")
  jupiter_test_set_default(MESSAGE_FILE_DIRECTORY "messages")

  jupiter_test_set_restart(MESSAGE_FILE)
  jupiter_test_set_restart(MESSAGE_FILE_DIRECTORY)

  jupiter_test_set_default(BINARY_OUTPUT_TIME_FILE "time/%n.dat")
  if(JUPITER_INPUT_TEST_BINARY_OUTPUT_DOUBLE)
    jupiter_test_set_default(BINARY_OUTPUT_COMP_FILE "%c_%i/%n.%r.dat")
    jupiter_test_set_default(BINARY_OUTPUT_DATA_FILE "%c/%n.%r.dat")
  else()
    jupiter_test_set_default(BINARY_OUTPUT_COMP_FILE "%c_%i/%n.%r.single.dat")
    jupiter_test_set_default(BINARY_OUTPUT_DATA_FILE "%c/%n.%r.single.dat")
  endif()
  jupiter_test_set_default(RESTART_OUTPUT_TIME_FILE "time.dat")
  jupiter_test_set_default(RESTART_OUTPUT_COMP_FILE "%c_%i/%r.dat")
  jupiter_test_set_default(RESTART_OUTPUT_DATA_FILE "%c/%r.dat")

  jupiter_test_set_restart(BINARY_OUTPUT_TIME_FILE)
  jupiter_test_set_restart(BINARY_OUTPUT_COMP_FILE)
  jupiter_test_set_restart(BINARY_OUTPUT_DATA_FILE)
  jupiter_test_set_restart(RESTART_OUTPUT_TIME_FILE)
  jupiter_test_set_restart(RESTART_OUTPUT_COMP_FILE)
  jupiter_test_set_restart(RESTART_OUTPUT_DATA_FILE)

  jupiter_test_set_default(MPI_NP 1)

  if (ENABLE_OPENMP)
    jupiter_test_set_default(OpenMP_NT 1)
  else()
    set(JUPITER_INPUT_TEST_OpenMP_NT 1)
  endif()

  jupiter_test_set_default(LPT_OUTPUT_FILE "LPT.dat")
  jupiter_test_set_default(LPT_OUTPUT_EXPECT IGNORE)
  jupiter_test_set_restart(LPT_OUTPUT_FILE)
  jupiter_test_set_restart(LPT_OUTPUT_EXPECT)

  jupiter_test_set_default(EXPECT 0)
  if(NOT DEFINED JUPITER_INPUT_TEST_RESTART_EXPECT)
    set(JUPITER_INPUT_TEST_RESTART_EXPECT "${JUPITER_INPUT_TEST_EXPECT}")
    if(JUPITER_INPUT_TEST_RESTART)
      set(JUPITER_INPUT_TEST_EXPECT 0)
    endif()
  endif()

  jupiter_test_set_default(EXPECT_DUMPMESH 0)
  jupiter_test_set_default(RESTART_EXPECT_DUMPMESH 0)

  foreach(P "" RESTART_)
    cmake_parse_arguments(${P}GEOM_DUMP_FILES "" "" "BYPROCESS;UNIFY"
      ${JUPITER_INPUT_TEST_${P}GEOM_DUMP_FILES})
  endforeach()

  jupiter_test_requires(JUPITER_INPUT_TEST_REQ ${JUPITER_INPUT_TEST_REQUIRES})

  foreach(N IN LISTS INPUT_NAMES_BASE)
    foreach(INPUT ${N} RESTART_${N})
      if(NOT JUPITER_INPUT_TEST_${INPUT})
        continue()
      endif()
      jupiter_configure_if("${INPUT}" "${JUPITER_INPUT_TEST_${INPUT}}" "txt")
      if("${JUPITER_INPUT_TEST_${INPUT}}" STREQUAL "")
        unset(JUPITER_INPUT_TEST_${INPUT})
      endif()
    endforeach()
    jupiter_test_set_restart(${N})
  endforeach()

  foreach(N IN LISTS SCRIPT_NAMES_BASE)
    foreach(INPUT ${N} RESTART_${N})
      if(NOT JUPITER_INPUT_TEST_${INPUT})
        continue()
      endif()
      jupiter_configure_if("${INPUT}" "${JUPITER_INPUT_TEST_${INPUT}}" "cmake")
      if("${JUPITER_INPUT_TEST_${INPUT}}" STREQUAL "")
        unset(JUPITER_INPUT_TEST_${INPUT})
      endif()
    endforeach()
  endforeach()

  set(_CMAKE_INPUTS PRE_RUN POST_RUN)
  foreach(INPUT IN LISTS INPUT_NAMES SCRIPT_NAMES)
    if(NOT JUPITER_INPUT_TEST_${INPUT})
      continue()
    endif()
    if ("${INPUT}" IN_LIST _CMAKE_INPUTS)
      set(_E "cmake")
    else()
      set(_E "txt")
    endif()
    jupiter_configure_if("${INPUT}" "${JUPITER_INPUT_TEST_${INPUT}}" "${_E}")
    if ("${JUPITER_INPUT_TEST_${INPUT}}" STREQUAL "")
      unset(JUPITER_INPUT_TEST_${INPUT})
    endif()
  endforeach()
  if(JUPITER_INPUT_TEST_RESTART)
    foreach(INPUT IN LISTS INPUT_NAMES)
      if(NOT JUPITER_INPUT_TEST_RESTART_${INPUT})
        set(JUPITER_INPUT_TEST_RESTART_${INPUT}
          "${JUPITER_INPUT_TEST_${INPUT}}")
      endif()
    endforeach()
  endif()

  if (NOT "${JUPITER_INPUT_TEST_EXPECT_MESSAGES}" STREQUAL "")
    jupiter_test_process_expected_message(
      OUTPUT JUPITER_INPUT_TEST_EXPECTED_ERRORS
      ${JUPITER_INPUT_TEST_EXPECT_MESSAGES})
  endif()
  if (NOT "${JUPITER_INPUT_TEST_RESTART_EXPECT_MESSAGES}" STREQUAL "")
    jupiter_test_process_expected_message(
      OUTPUT JUPITER_INPUT_TEST_EXPECTED_RESTART_ERRORS
      ${JUPITER_INPUT_TEST_RESTART_EXPECT_MESSAGES})
  endif()

  set(MPI_NP "${JUPITER_INPUT_TEST_MPI_NP}")
  set(OMP_NT "${JUPITER_INPUT_TEST_OpenMP_NT}")
  math(EXPR REQC "${MPI_NP} * ${OMP_NT}")
  if(REQC GREATER JUPITER_TEST_MAX_NUMPROCS OR
      (MPI_NP GREATER 1 AND NOT JUPITER_USE_MPI))
    list(APPEND JUPITER_INPUT_TEST_LABELS "cores-shortage")
  endif()
  if(JUPITER_INPUT_TEST_DISABLED)
    list(APPEND JUPITER_INPUT_TEST_LABELS "disabled")
  endif()
  if(JUPITER_INPUT_TEST_HEAVY)
    list(APPEND JUPITER_INPUT_TEST_LABELS "heavy")
  endif()
  if(NOT JUPITER_INPUT_TEST_REQ)
    list(APPEND JUPITER_INPUT_TEST_LABELS "missing-prereq")
  endif()

  if(NOT JUPITER_INPUT_TEST_MKVTK_CONFIGS)
    set(JUPITER_INPUT_TEST_MKVTK_CONFIGS
      ${JUPITER_INPUT_TEST_DEFAULT_MKVTK_CONFIGS})
  endif()
  list(REMOVE_ITEM JUPITER_INPUT_TEST_MKVTK_CONFIGS ":")

  if(JUPITER_INPUT_TEST_COMPARE_TOLERANCES)
    cmake_parse_arguments(_TOLS "" "" "ABSOLUTE;RELATIVE"
      "${JUPITER_INPUT_TEST_COMPARE_TOLERANCES}")
    foreach(_T ABSOLUTE RELATIVE)
      cmake_parse_arguments("_TOLS_${_T}" "" "${JUPITER_BINARY_DATATYPES}" ""
        "${_TOLS_${_T}}")
    endforeach()
  endif()

  set(_BTYPE_SETS)
  set(_TYPE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/cmake/types")
  set(_TYPE_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/cmake/types")
  foreach(_D IN LISTS JUPITER_BINARY_DATATYPES)
    set(_P JUPITER_BINARY_DATATYPE_${_D})
    set(_T "${${_P}_TYPE}")
    set(_SSET "${_TYPE_SOURCE_DIR}/set_${_T}.cmake")
    set(_BSET "${_TYPE_BINARY_DIR}/set_${_T}.cmake")
    unset(_T0N)
    unset(_T0R)
    unset(_T1N)
    unset(_T1R)
    unset(_T0M)
    unset(_T1M)
    if(JUPITER_DATATYPE_${_T}_SETTER)
      include("${JUPITER_DATATYPE_${_T}_SETTER}")
      set(_UNDEF "")
      foreach(V _T0N _T0R _T0M _T1N _T1R _T1M)
        if(NOT DEFINED "${V}")
          set(_UNDEF "${_UNDEF}
 * ${V}")
        endif()
      endforeach()
      if(NOT _UNDEF STREQUAL "")
        message(FATAL_ERROR "(dev) Script for type ${_T}, \"${JUPITER_DATATYPE_${_T}_SETTER}\", did not set required variable:${_UNDEF}
")
      endif()
    else()
      set(_T0N "${_T}")
      set(_T0R "${_T}")
      set(_T0M "${_T}")
      set(_T1N "${_T}")
      set(_T1R "${_T}")
      set(_T1M "${_T}")
    endif()
    foreach(V _T0N _T0R _T0M _T1N _T1R _T1M)
      if(NOT "${${V}}" IN_LIST JUPITER_DATATYPES)
        message(FATAL_ERROR "(dev) Type ${${V}} is not defined")
      endif()
    endforeach()
    if(DEFINED _TOLS_ABSOLUTE_${_D})
      set(_ABSTOL "${_TOLS_ABSOLUTE_${_D}}")
    elseif(DEFINED "${_P}_ABSOLUTE_TOLERANCE")
      set(_ABSTOL "${${_P}_ABSOLUTE_TOLERANCE}")
    endif()
    if(DEFINED _TOLS_RELATIVE_${_D})
      set(_RELTOL "${_TOLS_RELATIVE_${_D}}")
    elseif(DEFINED "${_P}_RELATIVE_TOLERANCE")
      set(_RELTOL "${${_P}_RELATIVE_TOLERANCE}")
    endif()
    set(_BTYPE_SETS "${_BTYPE_SETS}set(BTYPE_${_D}_TYPE \"${${_P}_TYPE}\")
set(BTYPE_${_D}_TYPE0_BINARY \"${_T0N}\")
set(BTYPE_${_D}_TYPE0_RESTART \"${_T0R}\")
set(BTYPE_${_D}_TYPE0_OTHERS \"${_T0M}\")
set(BTYPE_${_D}_TYPE1_BINARY \"${_T1N}\")
set(BTYPE_${_D}_TYPE1_RESTART \"${_T1R}\")
set(BTYPE_${_D}_TYPE1_OTHERS \"${_T1M}\")
set(BTYPE_${_D}_NUM_ELEMENTS \"${${_P}_NUM_ELEMENTS}\")
set(BTYPE_${_D}_ABSOLUTE_TOLERANCE \"${_ABSTOL}\")
set(BTYPE_${_D}_RELATIVE_TOLERANCE \"${_RELTOL}\")
set(BTYPE_${_D}_NX_EXPR \"${${_P}_NX_EXPR}\")
set(BTYPE_${_D}_NY_EXPR \"${${_P}_NY_EXPR}\")
set(BTYPE_${_D}_NZ_EXPR \"${${_P}_NZ_EXPR}\")
")
  endforeach()

  set(JUPITER_INPUT_TEST_UTIL_SCRIPT
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/test-util.cmake")
  set(JUPITER_INPUT_TEST_REFERENCE_UTILITY_SCRIPT
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/reference-utility.cmake")

  jupiter_test_generate_script_set(TEST_SCRIPTS
    VARCONFIG
    CONFIGURE
    OUTPUT "${WORKDIR}/run-${JUPITER_INPUT_TEST_NAME}-config.cmake"
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/jupiter-run-config.cmake.in")

  jupiter_test_generate_script_set(TEST_SCRIPTS
    BINCONFIG
    GENERATE
    OUTPUT "${WORKDIR}/run-${JUPITER_INPUT_TEST_NAME}-bin-config.cmake"
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/jupiter-run-bin-config.cmake")

  jupiter_test_generate_script_set(TEST_SCRIPTS
    JUPITER_RUN_SCRIPT
    CONFIGURE
    OUTPUT "${WORKDIR}/run-${JUPITER_INPUT_TEST_NAME}-jupiter.cmake"
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/jupiter-run-jupiter.cmake.in"
    VARS RESTART 0)

  set(RESTART_RUN_SCRIPTS "")
  if(JUPITER_INPUT_TEST_RESTART)
    jupiter_test_generate_script_set(TEST_SCRIPTS
      RESTART_RUN_SCRIPT
      CONFIGURE
      OUTPUT "${WORKDIR}/run-${JUPITER_INPUT_TEST_NAME}-jupiter-restart.cmake"
      INPUT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/jupiter-run-jupiter.cmake.in"
      VARS RESTART 1)
    set(RESTART_RUN_SCRIPTS "include(\"${RESTART_RUN_SCRIPT}\")")
  endif()

  jupiter_test_generate_script_set(TEST_SCRIPTS
    COMPARE_RUN_SCRIPT
    CONFIGURE
    OUTPUT "${WORKDIR}/run-${JUPITER_INPUT_TEST_NAME}-compare.cmake"
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/jupiter-run-compare.cmake.in")

  set(MKVTK_RUN_SCRIPTS)
  set(MKVTK_EXECS)
  foreach(_CONF IN LISTS JUPITER_INPUT_TEST_MKVTK_CONFIGS)
    set(_P "JUPITER_MKVTKCONF_${_CONF}")
    if(NOT DEFINED "${_P}_OUTNAME")
      message(FATAL_ERROR "Invalid MKVTK_CONFIG name: ${_CONF}")
    endif()
    set(_N "${${_P}_OUTNAME}")
    jupiter_test_generate_script_set(TEST_SCRIPTS
      "${_CONF}_RUN_SCRIPT"
      CONFIGURE
      OUTPUT "${WORKDIR}/run-${JUPITER_INPUT_TEST_NAME}-${_N}.cmake"
      INPUT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/jupiter-run-mkvtk.cmake.in"
      VARS
      MAKE_VTK_CONFIG_NAME "${_CONF}"
      MAKE_VTK_EXECUTABLE "${${_P}_EXECUTABLE}"
      MAKE_VTK_EXECUTABLE_NAME "${${_P}_EXECUTABLE_NAME}"
      MAKE_VTK_ARGS "${${_P}_ARGS}"
      MAKE_VTK_OUTNAME "${${_P}_OUTNAME}"
      MAKE_VTK_OUTDIR "${${_P}_OUTDIR}"
      MAKE_VTK_SKIP_INDICES "${${_P}_SKIP_INDICES}"
      MAKE_VTK_SKIP_MAKE_DIRECTORY "${${_P}_SKIP_MAKE_DIRECTORY}"
      MAKE_VTK_RESTART "${${_P}_RESTART}"
      MAKE_VTK_EXPECT "${${_P}_EXPECT}"
      MAKE_VTK_EXPECT_NO_INDICES "${${_P}_EXPECT_NO_INDICES}")
    set(MKVTK_RUN_SCRIPTS "${MKVTK_RUN_SCRIPTS}
if(NOT SKIP_${${_P}_EXECUTABLE})
  include(\"${${_CONF}_RUN_SCRIPT}\")
endif()
")
    list(APPEND MKVTK_EXECS "${${_P}_EXECUTABLE}")
  endforeach()

  # Unused for running via ctest, proiveded for running by cmake -P
  jupiter_test_generate_script_set(TEST_SCRIPTS
    ALL_RUN_SCRIPT
    CONFIGURE
    OUTPUT "${WORKDIR}/run-${JUPITER_INPUT_TEST_NAME}.cmake"
    INPUT "${CMAKE_CURRENT_SOURCE_DIR}/cmake/jupiter-run-test.cmake.in")

  jupiter_test_generate_scripts(TEST_SCRIPTS)

  set(TEST_BASE_NAME "${JUPITER_INPUT_TEST_NAME}")

  add_test(NAME "${TEST_BASE_NAME}"
    COMMAND ${CMAKE_COMMAND} -P "${JUPITER_RUN_SCRIPT}"
    WORKING_DIRECTORY "${WORKDIR}")

  if(JUPITER_INPUT_TEST_RESTART)
    add_test(NAME "${TEST_BASE_NAME}:restart"
      COMMAND ${CMAKE_COMMAND} -P "${RESTART_RUN_SCRIPT}"
      WORKING_DIRECTORY "${WORKDIR}")
  endif()

  add_test(NAME "${TEST_BASE_NAME}:compare"
    COMMAND ${CMAKE_COMMAND} -P "${COMPARE_RUN_SCRIPT}"
    WORKING_DIRECTORY "${WORKDIR}")

  foreach(_CONF IN LISTS JUPITER_INPUT_TEST_MKVTK_CONFIGS)
    set(_P "JUPITER_MKVTKCONF_${_CONF}")
    set(_N "${${_P}_OUTNAME}")
    add_test(NAME "${TEST_BASE_NAME}:${_N}"
      COMMAND "${CMAKE_COMMAND}" -P "${${_CONF}_RUN_SCRIPT}"
      WORKING_DIRECTORY "${WORKDIR}")
  endforeach()

  list(PREPEND JUPITER_INPUT_TEST_LABELS jupiter-input-test)
  list(REMOVE_DUPLICATES JUPITER_INPUT_TEST_LABELS)

  set(DISABLED FALSE)
  if(NOT JUPITER_USE_MPI AND MPI_NP GREATER 1)
    set(DISABLED TRUE)
  endif()
  if(JUPITER_INPUT_TEST_DISABLED)
    set(DISABLED TRUE)
  endif()
  if(JUPITER_INPUT_TEST_HEAVY AND NOT JUPITER_TEST_ENABLE_HEAVY_TESTS)
    set(DISABLED TRUE)
  endif()
  if(NOT JUPITER_INPUT_TEST_REQ)
    set(DISABLED TRUE)
  endif()

  set_tests_properties("${TEST_BASE_NAME}" PROPERTIES
    LABELS "${JUPITER_INPUT_TEST_LABELS};jupiter-main"
    FIXTURES_SETUP "${TEST_BASE_NAME}"
    DISABLED "${DISABLED}")

  set(FREQ "${TEST_BASE_NAME}")
  if(JUPITER_INPUT_TEST_RESTART)
    set_tests_properties("${TEST_BASE_NAME}:restart" PROPERTIES
      LABELS "${JUPITER_INPUT_TEST_LABELS};jupiter-main;jupiter-restart"
      FIXTURES_SETUP "${TEST_BASE_NAME}:restart"
      FIXTURES_REQUIRED "${TEST_BASE_NAME}"
      DISABLED "${DISABLED}")
    list(APPEND FREQ "${TEST_BASE_NAME}:restart")
  endif()

  set_tests_properties("${TEST_BASE_NAME}:compare" PROPERTIES
    LABELS "${JUPITER_INPUT_TEST_LABELS};compare-results"
    FIXTURES_REQUIRED "${FREQ}"
    DISABLED "${DISABLED}")

  foreach(_CONF IN LISTS JUPITER_INPUT_TEST_MKVTK_CONFIGS)
    set(_P "JUPITER_MKVTKCONF_${_CONF}")
    set(_N "${${_P}_OUTNAME}")
    set(_L "${${_P}_EXECUTABLE_LABEL}")
    set(_X "${${_P}_EXECUTABLE_SKIP}")
    set(_D ${DISABLED})
    if(JUPITER_INPUT_TEST_${_X})
      set(_D TRUE)
    endif()
    set_tests_properties("${TEST_BASE_NAME}:${_N}" PROPERTIES
      LABELS "${JUPITER_INPUT_TEST_LABELS};${_L};${_N}"
      FIXTURES_REQUIRED "${FREQ}"
      DISABLED "${_D}")
  endforeach()

  set(TARGET_NAME run-input-test-${TEST_BASE_NAME})
  add_custom_target(${TARGET_NAME}
    COMMAND "${CMAKE_COMMAND}" "-P" "${ALL_RUN_SCRIPT}"
    WORKING_DIRECTORY "${WORKDIR}"
    DEPENDS jupiter csv2table pack2txt jupiter-dumpmesh bin2txt bincomp
            getlptnp)
  if(TARGET "make-vtk" AND "MKVTK_EXECUTABLE" IN_LIST MKVTK_EXECS)
    add_dependencies(${TARGET_NAME} make-vtk)
  endif()
  if(TARGET "make-vtk-unconventional"
      AND "MKVTKU_EXECUTABLE" IN_LIST MKVTK_EXECS)
    add_dependencies(${TARGET_NAME} make-vtk-unconventional)
  endif()
  if(TARGET "make-xdmf" AND "MKXDMF_EXECUTABLE" IN_LIST MKVTK_EXECS)
    add_dependencies(${TARGET_NAME} make-xdmf)
  endif()
endfunction()

##
# @ingroup CMakeScripts
# @brief Add tests with create geometry's shape test
# @param NAME Name of the test
# @param NSUBCELL Number of subcells per cell
# @param FLAGS Flags input file name
# @param NCELL Number of cells (3 integers required)
# @param LENGTH Length of the domain (3 floats required)
# @param SHAPE_DEFINITIONS Start declaration of the shapes.
#
# This function supports to add a shape test without creating
# actual input files.
#
function(add_jupiter_shape_test NAME)
  if(NOT BUILD_JUPITER)
    return()
  endif()
  cmake_parse_arguments(JUPITER_SHAPE_TEST
    "" "NAME;NSUBCELL;FLAGS;EXPECT" "NCELL;LENGTH;SHAPE_DEFINITIONS"
    ${ARGV})

  if("${JUPITER_SHAPE_TEST_FLAGS}" STREQUAL "")
    set(JUPITER_SHAPE_TEST_FLAGS input/flags/geom-test.txt)
  endif()
  if("${JUPITER_SHAPE_TEST_NCELL}" STREQUAL "")
    set(JUPITER_SHAPE_TEST_NCELL 50 50 50)
  endif()
  if("${JUPITER_SHAPE_TEST_LENGTH}" STREQUAL "")
    set(JUPITER_SHAPE_TEST_LENGTH 50.0 50.0 50.0)
  endif()

  string(REPLACE ";" ", " NCELL "${JUPITER_SHAPE_TEST_NCELL}")
  string(REPLACE ";" ", " LENGTH "${JUPITER_SHAPE_TEST_LENGTH}")

  set(PARAM_FILE "${CMAKE_CURRENT_BINARY_DIR}/shape-test-params-${JUPITER_SHAPE_TEST_NAME}.txt")
  set(GEOM_FILE "${CMAKE_CURRENT_BINARY_DIR}/shape-test-geom-${JUPITER_SHAPE_TEST_NAME}.txt")

  set(PARAM_FILE ${CMAKE_CURRENT_SOURCE_DIR}/input/param/shape_test.txt.in)

  set(LINE "")
  set(FIRST TRUE)
  set(DEFSHAPE FALSE)
  set(SHAPE_SET_COUNT 1)
  set(SHAPE_DATA "")
  set(SHAPE_NAME "shape-1")
  set(SHAPE_LIST "${JUPITER_SHAPE_TEST_SHAPE_DEFINITIONS}")
  set(DUMPS)
  while(TRUE)
    list(LENGTH SHAPE_LIST __L)
    if(${__L} GREATER 0)
      list(GET SHAPE_LIST 0 ELEM)
      list(REMOVE_AT SHAPE_LIST 0)
    endif()
    if(${__L} EQUAL 0 OR "${ELEM}" STREQUAL "SHAPE_DEFINITION")
      if(NOT "${LINE}" STREQUAL "")
        if (JUPITER_SHAPE_TEST_NSUBCELL)
          set(LINE "${LINE}
Geom_num_subcell, ${JUPITER_SHAPE_TEST_NSUBCELL}")
        endif()
        set(_DNAME "data/binary_data/${SHAPE_NAME}.single.dat")
        list(APPEND DUMPS ${_DNAME})
        set(SHAPE_DATA "${SHAPE_DATA}
Geom_file, -${LINE}
Geom_vof, 0, SOLID, ADD, CONST, NaN, 1.0
Geom_name, \"${SHAPE_NAME}\"
Geom_dump, \"${_DNAME}\"
")
        math(EXPR SHAPE_SET_COUNT "${SHAPE_SET_COUNT}+1")
        set(LINE "")
      endif()
      if(${__L} EQUAL 0)
        break()
      endif()
      list(GET SHAPE_LIST 0 ELEM)
      if(NOT "${ELEM}" STREQUAL "SHAPE")
        set(SHAPE_NAME "${ELEM}")
        list(REMOVE_AT SHAPE_LIST 0)
      else()
        set(SHAPE_NAME "shape-${SHAPE_SET_COUNT}")
      endif()
    elseif("${ELEM}" STREQUAL "SHAPE")
      set(LINE "${LINE}
Geom_shape")
    elseif("${ELEM}" STREQUAL "OFFSET")
      set(LINE "${LINE}
Geom_offset")
    elseif("${ELEM}" STREQUAL "ORIGIN")
      set(LINE "${LINE}
Geom_origin")
    elseif("${ELEM}" STREQUAL "REPEAT")
      set(LINE "${LINE}
Geom_repeat")
    else()
      set(LINE "${LINE}, ${ELEM}")
    endif()
  endwhile()

  set(GEOM_TEXT "NumberOfGeom, ${SHAPE_SET_COUNT}

Geom_print_matrix, ON

Geom_file, -
Geom_vof, -1, GAS, SET, CONST, 0.0, 0.0
Geom_velocity_u,   SET, CONST, 0.0, 0.0
Geom_velocity_v,   SET, CONST, 0.0, 0.0
Geom_velocity_w,   SET, CONST, 0.0, 0.0
Geom_temperature,  SET, CONST, 0.0, 0.0
Geom_pressure,     SET, CONST, 0.0, 0.0

${SHAPE_DATA}
")

  if(NOT "${JUPITER_SHAPE_TEST_EXPECT}" STREQUAL "")
    set(JUPITER_SHAPE_TEST_EXPECT "EXPECT" ${JUPITER_SHAPE_TEST_EXPECT})
  endif()
  add_jupiter_input_test(NAME "${JUPITER_SHAPE_TEST_NAME}"
    PARAMS "${PARAM_FILE}"      FLAGS "${JUPITER_SHAPE_TEST_FLAGS}"
    PLIST input/plist.3.txt.in  GEOMETRY "${GEOM_TEXT}"
    GEOM_DUMP_FILES "${DUMPS}"
    ${JUPITER_SHAPE_TEST_EXPECT})
endfunction()
