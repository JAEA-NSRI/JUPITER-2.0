# Find re2c from your system.
#
# Defines: RE2C_FOUND               --- ON if re2c is found.
#          RE2C_EXECUTABLE          --- Path to re2c
#          RE2CFLAGS                --- Global flags for re2c
#          RE2CFLAGS_DEBUG          --- Flags for re2c in Debug
#          RE2CFLAGS_RELEASE        --- Flags for re2c in Release
#          RE2CFLAGS_MINSIZEREL     --- Flags for re2c in MinSizeRel
#          RE2CFLAGS_RELWITHDEBINFO --- Flags for re2c in RelWithDebInfo
#
#          add_re2c_target(OUT IN)  --- Add target to Generate OUT from IN.
#
# re2c execution flags in order of
#
#  1. RE2CFLAGS
#  2. RE2CFLAGS_${CMAKE_BUILD_TYPE}
#  3. COMPILE_OPTIONS source property of INPUT.
#
# Note: re2c refuses specifying conflicting flags (ex. `-8` (Generate
#       UTF-8 scanner) and `-x` (Generate UTF-16 scanner)) at once. These
#       options are usually source file specific, we recommend use
#       source property.

include(FindPackageHandleStandardArgs)

if(NOT RE2C_EXECUTABLE)
  find_program(RE2C_EXECUTABLE re2c
    PATHS /bin /usr/bin /usr/local/bin ENV PATH)
endif()

if(RE2C_EXECUTABLE)
  execute_process(COMMAND "${RE2C_EXECUTABLE}" -v
    OUTPUT_VARIABLE RE2C_VERSION
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
  string(REGEX REPLACE "^ *re2c +([0-9\\.]+).*$" "\\1"
    RE2C_VERSION "${RE2C_VERSION}")
endif()

find_package_handle_standard_args(RE2C
  REQUIRED_VARS RE2C_EXECUTABLE
  VERSION_VAR RE2C_VERSION)

if (RE2C_FOUND)
  set(RE2CFLAGS "" CACHE STRING "re2c flags")
  set(RE2CFLAGS_DEBUG "" CACHE STRING "re2c debug-only flags")
  set(RE2CFLAGS_RELEASE "--no-generation-date -i" CACHE STRING "re2c release-only flags")
  set(RE2CFLAGS_MINSIZEREL "" CACHE STRING "re2c minsizerel flags")
  set(RE2CFLAGS_RELWITHDEBINFO "" CACHE STRING "re2c relwithdebinfo flags")

  mark_as_advanced(RE2CFLAGS)
  mark_as_advanced(RE2CFLAGS_DEBUG)
  mark_as_advanced(RE2CFLAGS_RELEASE)
  mark_as_advanced(RE2CFLAGS_MINSIZEREL)
  mark_as_advanced(RE2CFLAGS_RELWITHDEBINFO)

  function(add_re2c_target OUT IN)
    get_source_file_property(FS "${IN}" COMPILE_OPTIONS)
    if (FS)
      string(REPLACE " " ";" FS "${FS}")
    else()
      set(FS)
    endif()
    string(TOUPPER "${CMAKE_BUILD_TYPE}" U)
    string(REPLACE " " ";" F "${RE2CFLAGS}")
    string(REPLACE " " ";" FN "${RE2CFLAGS_${U}}")
    if(NOT IS_ABSOLUTE "${IN}")
      get_source_file_property(GEN "${CMAKE_CURRENT_BINARY_DIR}/${IN}"
        GENERATED)
      if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${IN}" OR GEN)
        set(IN "${CMAKE_CURRENT_BINARY_DIR}/${IN}")
      else()
        set(IN "${CMAKE_CURRENT_SOURCE_DIR}/${IN}")
      endif()
    endif()
    if(NOT IS_ABSOLUTE "${OUT}")
      set(OUT "${CMAKE_CURRENT_BINARY_DIR}/${OUT}")
    endif()
    get_filename_component(OUTD "${OUT}" DIRECTORY)
    file(MAKE_DIRECTORY "${OUTD}")
    add_custom_command(OUTPUT "${OUT}"
      COMMAND "${RE2C_EXECUTABLE}" ${F} ${FN} ${FS}
      -o "${OUT}" "${IN}"
      WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
      DEPENDS "${IN}"
      COMMENT "[RE2C] Generating ${OUT}" VERBATIM)
  endfunction()
endif()
