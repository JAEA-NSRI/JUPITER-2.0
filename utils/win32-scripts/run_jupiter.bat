@if NOT DEFINED VERBOSE ( echo off )

setlocal ENABLEEXTENSIONS

chcp 65001

set BASEDIR=%~dp0

call input-files.bat
set "PATH=%BASEDIR%\bin;%PATH%"
set JUPITER_EXE=%BASEDIR%\bin\jupiter-double.exe
set MAKE_XDMF_EXE=%BASEDIR%\bin\make-xdmf.exe
set MAKE_VTK_EXE=%BASEDIR%\bin\make-vtk.exe
set EXTFILE_EXTRACTOR=%BASEDIR%\bin\jupiter-extfile-extractor.exe

echo.
echo.Case Directory = %CD%
echo.
echo.Welcome to JUPITER Execution batch.

:main_menu_help

echo.
echo.Select Execution Mode
echo.
echo. 1: Run JUPITER from the start. (default)
echo. 2: Run JUPITER from the last restart dump.
echo. 3: Run JUPITER from the restart dump of specified index.
echo.    - To use this mode, output_double and all variables
echo.      which does not take an ON/OFF options for restart
echo.      data output must be set to ON.
echo.    - Restart from index 0 is not possible. Use option 1.
echo.
echo. 8: Run VTK converter (with specifying time steps, into separated files)
echo. 9: Run XDMF converter (with specifying time steps, into separated files)
echo. 0: Run XDMF converter (for all time steps found in the output directory)
echo.
echo. S: Convert STL geometry files to binaries which can be read from.
echo. C: Convert Liquidus/Solidus temperature CSV database to binaries which
echo.    can be read from.
echo.
echo. Q: Abort.
echo. ?: Show this help.
echo.

:retry_select_mode

set EXEC_MODE=1
set /p EXEC_MODE=Execution Mode? (1,2,3,8,9,0,S,C,Q,?): 

set POST=
set RESTART=
set NCONVERT=
set EXE=%JUPITER_EXE%
set TITLE=JUPITER

if %EXEC_MODE% == ? ( goto main_menu_help )
if %EXEC_MODE% == Q ( goto end )
if %EXEC_MODE% == q ( goto end )
if %EXEC_MODE% == S ( goto stlconv )
if %EXEC_MODE% == s ( goto stlconv )
if %EXEC_MODE% == C ( goto tmdbconv )
if %EXEC_MODE% == c ( goto tmdbconv )
if %EXEC_MODE% == 2 ( 
  set RESTART=-restart_job
  goto start_jupiter
)
if %EXEC_MODE% == 3 ( goto get_restart_n )
if %EXEC_MODE% == 8 (
  set EXE=%MAKE_VTK_EXE%
  set POST=1
  set "TITLE=VTK Converter"
  goto get_convert_n
)
if %EXEC_MODE% == 9 (
  set EXE=%MAKE_XDMF_EXE%
  set POST=1
  set "TITLE=XDMF Converter"
  goto get_convert_n
)
if %EXEC_MODE% == 0 (
  set EXE=%MAKE_XDMF_EXE%
  set POST=1
  set "TITLE=XDMF Converter"
  goto start_jupiter
)
if %EXEC_MODE% NEQ 1 (
  echo.Invalid option %EXEC_MODE%.
  goto retry_select_mode
)

:start_jupiter
call :run %EXE% %INPUT% %FLAGS% %GEOMETRY% %CONTROL% %PLIST% %STDOUT% ^
          %WORK_DIR% %XDMF_DIR% %EXTRACTOUT%
pause -1
goto end

:run
set EXE=%~f1
shift /1
set INPUT=%~f1
shift /1
set FLAGS=%~f1
shift /1
set GEOMETRY=%~f1
shift /1
set CONTROL=%~f1
shift /1
set PLIST=%~f1
shift /1
set STDOUT=%~f1
shift /1
set WORK_DIR=%~f1
shift /1
set XDMF_DIR=%~f1
shift /1
set EXTRACTOUT=%~f1

cd %WORK_DIR%
if ERRORLEVEL 1 (
  echo.
  echo.Could not change directory to %WORK_DIR%.
  echo.
  exit /b 1
)

call :clean_output %NCONVERT%

echo.** EXE=%EXE%
echo.** WORK_DIR=%WORK_DIR%
echo.** INPUT=%INPUT%
echo.** FLAGS=%FLAGS%
echo.** GEOMETRY=%GEOMETRY%
echo.** CONTROL=%CONTROL%
echo.** PLIST=%PLIST%
echo.** STDOUT=%STDOUT%
if NOT DEFINED EXTARCT (
  echo.** NPROC=%NPROC%
)
if DEFINED RESTART (
  echo.** RESTART=%RESTART%
)
if DEFINED POST (
  echo.** XDMF_DIR=%XDMF_DIR%
)
if DEFINED NCONVERT (
  echo.** NCONVERT=%NCONVERT%
)
if DEFINED EXTRACT (
  echo.** EXTRACT=%EXTRACT%
  echo.** EXTRACTOUT=%EXTRACTOUT%
)
echo.
echo.** Starting %TITLE%... (at %DATE% %TIME%)
if NOT DEFINED POST (
  if NOT DEFINED EXTRACT (
    call :run_mpi_or_not ^
      %EXE% -input %INPUT% -flags %FLAGS% -control %CONTROL% ^
            -geom %GEOMETRY% -plist %PLIST% %RESTART% > %STDOUT% 2>&1
  ) else (
    REM Extractors are assumed not to use MPI.
    if %NPROC% GTR 0 (
      set NPROC=1
    )
    del %EXTRACTOUT%
    call :run_mpi_or_not ^
      %EXE% -input %INPUT% -flags %FLAGS% -control %CONTROL% ^
            -geom %GEOMETRY% -plist %PLIST% -- ^
                  %EXTRACT% -o %EXTRACTOUT%
    if %ERRORLEVEL% == 0 (
      echo.
      echo.** Running %EXTRACTOUT%
      @if NOT DEFINED VERBOSE ( echo on )
      call %EXTRACTOUT%
      @if NOT DEFINED VERBOSE ( echo off )
      echo.
      echo.!! Batch won't detect errors on these commands.
      echo.!! Please review the output carefully.
    )
  )
) else (
  mkdir %XDMF_DIR%
  call :run_mpi_or_not ^
    %EXE% -input %INPUT% -flags %FLAGS% ^
          -- %NCONVERT% -o %XDMF_DIR%
)
set SERRORLEVEL=%ERRORLEVEL%
if %ERRORLEVEL% NEQ 0 (
  echo.!! Completed with error. ^(at %DATE% %TIME%^)
) else (
  echo.** Completed successfully! ^(at %DATE% %TIME%^)
)
exit /b %SERRORLEVEL%

:run_mpi_or_not
if %NPROC% LSS 1 (
  %*
) else (
  mpiexec -n %NPROC% %*
)
exit /b %ERRORLEVEL%

:get_restart_n
set /p RESTART_INDEX=Restart From? (Number^): 
call :test_numeral "%RESTART_INDEX%"
if %ERRORLEVEL% NEQ 0 (
  echo.Invalid Number.
  echo.
  goto retry_select_mode
)
if %RESTART_INDEX% LSS 1 (
  echo.Cannot restart from index %RESTART_INDEX%.
  echo.
  goto retry_select_mode
)
set "RESTART=-restart %RESTART_INDEX%"
goto start_jupiter

:get_convert_n
set NCONVERT=

:get_convert_1
set CNV=
set /p CNV=Numbers to convert? (Number, or just ENTER to complete^): 
if NOT DEFINED CNV (
  if NOT DEFINED NCONVERT (
    echo.Nothing selected for data conversion.
    echo.
    goto retry_select_mode
  )
  echo.
  goto start_jupiter
)
call :conv_proc %CNV%
goto get_convert_1

:conv_proc
for %%i in (%*) do ( call :conv_num_proc %%i )
goto end

:conv_num_proc
if "%1" EQU "-1" (
  set "NCONVERT=%NCONVERT% -1"
  goto end
)
call :test_numeral "%1"
if %ERRORLEVEL% NEQ 0 (
  echo."%1" is not a valid number.
  goto end
)

REM https://stackoverflow.com/q/14762813
set var=%1
set /A var=1%var%-(11%var%-1%var%)/10
set "NCONVERT=%NCONVERT% %var%"
goto end

:test_numeral
set "var="&for /f "delims=0123456789" %%i in ("%~1") do set var=%%i
if DEFINED var (
  exit /b 1
)
exit /b 0

:clean_output
if %EXEC_MODE% == 0 (
  del %XDMF_DIR%\all.h5
  del %XDMF_DIR%\all.xmf
  exit /b 0
)
if %EXEC_MODE% == 9 (
  for %%i in (%*) do (
    if %%i NEQ -1 (
      del %XDMF_DIR%\time_%%i.h5
      del %XDMF_DIR%\time_%%i.xmf
    )
  )
  exit /b 0
)
exit /b 0

:stlconv
set EXE=%EXTFILE_EXTRACTOR%
set EXTRACT=geometry-files
set EXTRACTOUT=%STLCONVOUT%
set "TITLE=STL geometry files conversion"
goto start_jupiter

:tmdbconv
set EXE=%EXTFILE_EXTRACTOR%
set EXTRACT=tm-tables
set EXTRACTOUT=%TMDBCONVOUT%
set "TITLE=Liquidus/Solidus Table files conversion"
goto start_jupiter

:end
