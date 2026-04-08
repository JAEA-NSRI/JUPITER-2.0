REM Lines which starts with `REM` is a comment.
REM
REM **** Input file set
REM
REM %BASEDIR% is the root of distribution.
REM
REM Do not add spaces around '='.
REM
REM You cannot include spaces in the paths.
REM

REM Directory of input files are located.
set INPUT_DIR=%BASEDIR%\input

REM Directory of output files should be stored.
REM
REM NOTE: This parameter does not take effect for the location of
REM       binary files and some specific files. These parameters are
REM       set by Flags input file, relative to %WORK_DIR%.
set OUTPUT_DIR=%BASEDIR%\output

REM Working directory
REM
REM Directory which JUPITER should be run on. Some outputs should be
REM stored here, so it would be %OUTPUT_DIR% usually. But you can use
REM another directory.
REM
REM All paths specified by the input files (binary directory, property
REM data, etc.) are relative path from here.
set WORK_DIR=%OUTPUT_DIR%

REM Parameter input file path.
set INPUT=%INPUT_DIR%\param.txt

REM Flag input file path.
set FLAGS=%INPUT_DIR%\flags.txt

REM Geometry input file path.
set GEOMETRY=%INPUT_DIR%\geom.txt

REM Control input file path.
REM
REM NOTE: Batch does not support not specifying the file.
set CONTROL=%INPUT_DIR%\control.txt

REM Property list input file path.
set PLIST=%INPUT_DIR%\plist.txt

REM Standard output file path.
set STDOUT=%OUTPUT_DIR%\STDOUT.txt

REM Path to output generated STL files conversion batch file.
REM
REM NOTE: The extension `.bat` is required, unless it won't be run.
REM
set STLCONVOUT=%OUTPUT_DIR%\stlconv.bat

REM Path to output generated Liquidus/Solidus table data conversion batch file.
REM
REM NOTE: The extension `.bat` is required, unless it won't be run.
REM
set TMDBCONVOUT=%OUTPUT_DIR%\tmdbconv.bat

REM Output directory of make-xdmf utility.
REM
REM XDMF output will be stored in this directory with name 'all.xmf'
REM (metadata) and 'all.h5' (binary data (HDF5))
REM
REM NOTE: With files all.xmf and all.h5, it will be a complete dataset.
REM       Other files are not required for visualization.
set XDMF_DIR=%OUTPUT_DIR%\xdmf

REM Number of MPI parallel
REM
REM NOTE: NPROC=0 or not set will start JUPITER without `mpiexec`.
REM       NPROC=1 will start JUPITER with `mpiexec -n 1`.
REM
REM NOTE: If stack overflows while the calculation, run in parallel
REM       with MPI can avoid this. (It will not say "stack
REM       overflowed". Actually it will just crashes around the
REM       calculation is going to start.)
set NPROC=0

REM Number of OpenMP threads
REM
REM In default, the number of OpenMP threads is not set, and it will be
REM set to the number of CPU cores you have.
REM
REM NOTE: OpenMP does not detect whether HyperThreading (or similar
REM       SMT) technology is enabled or not. If enabled, it may become
REM       faster by setting OMP_NUM_THREADS to the number of CPU cores
REM       **physically** you have, explicitly.
REM
REM NOTE: For using MPI parallelization in the local host, you may want to
REM       set OMP_NUM_THREADS=1. It may cause a dead-lock.
set OMP_NUM_THREADS=
