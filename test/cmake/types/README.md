## Template for output file comparator script

Please remove comment lines with `# FIXME:` when implemented.

```cmake
set(INPUT "@INPUT@")
set(REFERENCE "@REFERENCE@")
# And other variables...

include("@REFERENCE_UTILITY_SCRIPT@")

if(EXISTS "${INPUT}")
  # FIXME: Textize binary files and/or normalize data the input file here
endif()

if(EXISTS "${REFERENCE}")
  if(NOT EXISTS "${INPUT}")
    message(SEND_ERROR "${INPUT} expected to be written, but not found")
    return()
  endif()

  # FIXME: Compare the result file with reference data
endif()

#
# To update existing reference file:
#
#    @CMAKE_COMMAND@ -E env JUPITER_UPDATE_TEST_REFS=1 @CMAKE_COMMAND@ -P @COMPARE_SCRIPT@
#
# FIXME: Replace `${INPUT}` with the file to be stored as reference, if needed.
update_reference("${INPUT}" "${REFERENCE}")

#
# To create reference file:
#
#    @CMAKE_COMMAND@ -E env JUPITER_CREATE_TEST_REFS=1 @CMAKE_COMMAND@ -P @COMPARE_SCRIPT@
#
# @warning Running through ctest will copy all data files as reference.
# @note Existing references will not be updated.
#
# FIXME: Replace `${INPUT}` with the file to be stored as reference, if needed.
create_reference("${INPUT}" "${REFERENCE}")
```
