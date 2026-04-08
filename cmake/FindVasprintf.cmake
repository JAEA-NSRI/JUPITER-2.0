# Find GNU/BSD extended function vasprintf
#
# HAVE_VASPRINTF       :: Set to ON if found
# VASRPINTF_DEFINTIONS :: Required definitions to use vasprintf function
#                        (Maybe -D_GNU_SOURCE for GNU and derived systems
#                         such as Linux, MinGW or Cygwin, and, (empty) in
#                         *BSD or macOS.)


include(CheckPrototypeDefinition)
include(FindPackageHandleStandardArgs)

CHECK_PROTOTYPE_DEFINITION("vasprintf"
  "int vasprintf(char **buf, const char *fmt, va_list ap)"
  "0"
  "stdio.h;stdarg.h" HAVE_VASPRINTF)
if(NOT HAVE_VASPRINTF)
  set(__SAVE_CMAKE_REQUIRED_DEFS "${CMAKE_REQUIRED_DEFINITIONS}")
  set(CMAKE_REQUIRED_DEFINITIONS "-D_GNU_SOURCE")
  CHECK_PROTOTYPE_DEFINITION("vasprintf"
    "int vasprintf(char **buf, const char *fmt, va_list ap)"
    "0"
    "stdio.h;stdarg.h" HAVE_VASPRINTF_WITH_GNU_SOURCE)
  if(HAVE_VASPRINTF_WITH_GNU_SOURCE)
    set(HAVE_VASPRINTF TRUE)
    set(VASPRINTF_DEFINITIONS "-D_GNU_SOURCE")
    set(__VASPRINTF_DEFINITIONS "${VASPRINTF_DEFINITIONS}")
    set(CMAKE_REQUIRED_DEFINITIONS "${__SAVE_CMAKE_REQUIRED_DEFS}")
  endif()
else()
  set(VASPRINTF_DEFINITIONS "")
  set(__VASPRINTF_DEFINITIONS "(none required)")
endif()
set(VASPRINTF_DEFINITIONS "${VASPRINTF_DEFINITIONS}"
  CACHE STRING "Required definitions to use vasprintf")
set(VASPRINTF_FOUND "${HAVE_VASPRINTF}"
  CACHE BOOL "vasprintf function found")

mark_as_advanced(VASPRINTF_DEFINITIONS)
mark_as_advanced(VASPRINTF_FOUND)

find_package_handle_standard_args(Vasprintf
  REQUIRED_VARS __VASPRINTF_DEFINITIONS VASPRINTF_FOUND)
