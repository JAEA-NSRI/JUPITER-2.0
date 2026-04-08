
if (WIN32)
  install(FILES
    input-files.bat run_jupiter.bat
    DESTINATION .)

  set(LICFILES)
  if (MINGW)
    list(APPEND LICFILES "license-mingw.txt")
  endif()
  list(APPEND LICFILES "license-vtk.txt")

  if (LICFILES)
    set(CUT_LINE "---8<------------------------------------------------------------------------")
    set(LICHEAD "# License Information

Runtime files on this directory are licenced under each own licenses.

## Index

")  
    set(LICBODY "## License Terms
")  
    foreach(LIC IN LISTS LICFILES)
      file(READ "${LIC}" LICTEXT)
      string(FIND "${LICTEXT}" "${CUT_LINE}" SCISSOR)
      if (SCISSOR)
        string(SUBSTRING "${LICTEXT}" 0 ${SCISSOR} INDEX_TEXT)
        set(LICHEAD "${LICHEAD}${INDEX_TEXT}")
        string(SUBSTRING "${LICTEXT}" ${SCISSOR} -1 BODY_TEXT)
        string(REGEX REPLACE "${CUT_LINE}* *\n" "" BODY_TEXT "${BODY_TEXT}")
      else()
        set(BODY_TEXT "${LICTEXT}")
      endif()
      set(LICBODY "${LICBODY}${BODY_TEXT}")
    endforeach()
    file(WRITE "${CMAKE_CURRENT_BINARY_DIR}/license.txt" "${LICHEAD}${LICBODY}")
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/license.txt" DESTINATION "bin")
  endif()
endif()
