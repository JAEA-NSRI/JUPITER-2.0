
set(JUPITER_TEST_ROOT "${CMAKE_CURRENT_BINARY_DIR}")

cmake_host_system_information(RESULT _NUMPROCS QUERY NUMBER_OF_PHYSICAL_CORES)
if(NOT DEFINED JUPITER_TEST_MAX_NUMPROCS)
  set(JUPITER_TEST_MAX_NUMPROCS "${_NUMPROCS}")
endif()
if(NOT BUILD_TESTING)
  set(JUPITER_TEST_MAX_NUMPROCS "${JUPITER_TEST_MAX_NUMPROCS}"
    CACHE INTERNAL "" FORCE)
else()
  set(JUPITER_TEST_MAX_NUMPROCS "${JUPITER_TEST_MAX_NUMPROCS}"
    CACHE STRING "Number of available cores to run test" FORCE)
endif()

include(CMakeDependentOption)
cmake_dependent_option(JUPITER_TEST_ENABLE_HEAVY_TESTS
  "Enable 'heavy' tests" OFF BUILD_TESTING OFF)

cmake_dependent_option(JUPITER_TEST_KEEP_COMPARED
  "Save modified output file when run test" OFF BUILD_TESTING OFF)

if (NOT BUILD_TESTING)
  return()
endif()

include(CheckTypeSize)

if(JUPITER_USE_MPI AND TARGET MPI::MPI_C)
  function(check_mpi_type_size TYPE VAR)
    set(CMAKE_REQUIRED_LIBRARIES MPI::MPI_C)
    set(CMAKE_EXTRA_INCLUDE_FILES mpi.h)
    check_type_size(${TYPE} ${VAR} BUILTIN_TYPES_ONLY LANGUAGE C)
  endfunction()
  check_mpi_type_size(MPI_Aint MPI_AINT_SIZE)
endif()

find_program(DIFF_COMMAND diff)

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${JUPITER_TEST_ROOT}/lib")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${JUPITER_TEST_ROOT}/lib")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${JUPITER_TEST_ROOT}/bin")

add_subdirectory(bin2txt)
add_subdirectory(getlptnp)
add_subdirectory(component_info)
add_subdirectory(test-util)
add_subdirectory(random)
add_subdirectory(dccalc)
add_subdirectory(geometry)
add_subdirectory(serializer)
add_subdirectory(control)
add_subdirectory(rect3d_relp)
add_subdirectory(lpt)
add_subdirectory(lptx)
add_subdirectory(csv)
add_subdirectory(csvutil)
add_subdirectory(os)
add_subdirectory(type_compat)
add_subdirectory(general_vector)
add_subdirectory(common_util)
add_subdirectory(strlist)
add_subdirectory(grid_data_feeder)
add_subdirectory(cxxoptparse)
add_subdirectory(vtklib)

include("cmake/jupiter-test.cmake")

## Data types

# - SINGLE if JUPITER has been complied for single precision.
# - SINGLE if [RESTART_]BINARY_OUTPUT_DOUBLE is OFF and for sequenced data
#   files (matches on `%n`).
# - SINGLE if [RESTART_]RESTART_OUTPUT_DOUBLE is OFF and for restart data
#   files (does not matches on `%n`).
# - DOUBLE otherwise.
define_jupiter_datatype(TYPE REAL
  ABSOLUTE_TOLERANCE -1 RELATIVE_TOLERANCE 1.0e-6)

# Raw SINGLE precision floating point (`float`) type
define_jupiter_datatype(TYPE SINGLE
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE -1 RELATIVE_TOLERANCE 1.0e-6)

# Raw DOUBLE precision floating point (`double`) type
define_jupiter_datatype(TYPE DOUBLE
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE -1 RELATIVE_TOLERANCE 1.0e-6)

# Raw signed 8-bit integer type
define_jupiter_datatype(TYPE INT8
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw signed 16-bit integer type
define_jupiter_datatype(TYPE INT16
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw signed 32-bit integer type
define_jupiter_datatype(TYPE INT32
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw signed 64-bit integer type
define_jupiter_datatype(TYPE INT64
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw unsigned 8-bit integer type
define_jupiter_datatype(TYPE UINT8
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw unsigned 16-bit integer type
define_jupiter_datatype(TYPE UINT16
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw unsigned 32-bit integer type
define_jupiter_datatype(TYPE UINT32
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw unsigned 64-bit integer type
define_jupiter_datatype(TYPE UINT64
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw default char type on target system
#
# Note: char may signed or unsigned
define_jupiter_datatype(TYPE CHAR
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw default signed char type on target system
define_jupiter_datatype(TYPE SCHAR
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw default unsigned char type on target system
define_jupiter_datatype(TYPE UCHAR
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw default signed integer type on target system
define_jupiter_datatype(TYPE INT
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw default unsigned integer type on target system
define_jupiter_datatype(TYPE UINT
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw default signed long integer type on target system
define_jupiter_datatype(TYPE LONG
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw `ptrdiff_t` on target system
define_jupiter_datatype(TYPE PTRDIFF_T
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw `size_t` on target system
define_jupiter_datatype(TYPE SIZE_T
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw `intmax_t` on target system
define_jupiter_datatype(TYPE INTMAX_T
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw `uintmax_t` on target system
define_jupiter_datatype(TYPE UINTMAX_T
  CONFIGURE cmake/default_binary_compare.cmake
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Raw LPTX_idtype (see jupiter/lptx/defs.h)
#
# Assumes that MPI_Aint is signed type.
define_jupiter_datatype(TYPE LPTX_IDTYPE
  ABSOLUTE_TOLERANCE 0 RELATIVE_TOLERANCE -1)

# Written using jupiter-serializer (msgpackx)
define_jupiter_datatype(TYPE SERIALIZED)

# Text (not binary), no normalization
define_jupiter_datatype(TYPE TEXT)

# Text, normalize for message file and standard outputs
define_jupiter_datatype(TYPE TEXT_MSG)

# Number of elements in lpt_flags
set(_LPTFLGN 9)

## Binary output names

# set_jupiter_datatype(TYPE [TYPE] NAMES [SCALAR-VARIABLES...])
set_jupiter_datatype(TYPE TEXT NAMES time)
set_jupiter_datatype(TYPE REAL NAMES fs fl ls ll lls fs_ibm ls_ibm)
set_jupiter_datatype(TYPE REAL NAMES df dfs entha)
set_jupiter_datatype(TYPE INT  NAMES mushy)
set_jupiter_datatype(TYPE REAL NAMES u v w p t)
set_jupiter_datatype(TYPE REAL NUM_ELEMENTS 3 NAMES uvw)
set_jupiter_datatype(TYPE REAL NAMES ox_dt ox_dt_local ox_lset ox_vof)
set_jupiter_datatype(TYPE REAL NAMES ox_q ox_h2 ox_f_h2o ox_lset_h2o)
set_jupiter_datatype(TYPE REAL NAMES ox_lset_h2o_s ox_kp ox_dens)
set_jupiter_datatype(TYPE REAL NAMES ox_recess_rate)
set_jupiter_datatype(TYPE INT  NAMES ox_flag)
set_jupiter_datatype(TYPE REAL NAMES h2_absorb_eval h2_absorb_Ks
  NX_EXPR 1 NY_EXPR 1 NZ_EXPR "@INPUT_NZ@")
set_jupiter_datatype(TYPE REAL NAMES h2_absorb_P h2_absorb_T
  NX_EXPR 1 NY_EXPR 1 NZ_EXPR "@INPUT_NZ@")
set_jupiter_datatype(TYPE REAL NAMES h2_absorb_Zr
  NX_EXPR 1 NY_EXPR 1 NZ_EXPR "@INPUT_NZ@")
set_jupiter_datatype(TYPE SERIALIZED NAMES lpt_ctrl)
set_jupiter_datatype(TYPE LPTX_IDTYPE NAMES lpt_ptid lpt_oid
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NAMES lpt_xpt lpt_ypt lpt_zpt
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NUM_ELEMENTS 3 NAMES lpt_pospt
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NAMES lpt_uxpt lpt_uypt lpt_uzpt
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NUM_ELEMENTS 3 NAMES lpt_upt lpt_uf
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NAMES lpt_muf lpt_densf lpt_cpf
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NAMES lpt_thcf lpt_tempf lpt_gradTf
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NAMES lpt_pathf lpt_mwf
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NAMES lpt_timpt
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NAMES lpt_fuxpt lpt_fuypt lpt_fuzpt
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NUM_ELEMENTS 3 NAMES lpt_fupt
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NAMES lpt_dTdt
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NAMES lpt_fduxt lpt_fduyt lpt_fduzt
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NUM_ELEMENTS 3 NAMES lpt_fdt
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NAMES lpt_diapt lpt_denspt lpt_cppt
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NAMES lpt_thcpt lpt_temppt lpt_htrpt
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NAMES lpt_tothtpt
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NUM_ELEMENTS 3 NAMES lpt_inipospt
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NUM_ELEMENTS 3 NAMES lpt_iniupt
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NAMES lpt_initemppt
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE INT  NAMES lpt_exit
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE CHAR NUM_ELEMENTS ${_LPTFLGN} NAMES lpt_flags
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NAMES lpt_parceln
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NUM_ELEMENTS 3 NAMES lpt_fbpt lpt_fTpt
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NAMES lpt_Y
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE UINT64 NUM_ELEMENTS 4 NAMES lpt_seed
  NX_EXPR "@INPUT_NP@" NY_EXPR 1 NZ_EXPR 1)
set_jupiter_datatype(TYPE REAL NAMES lpt_ewall
  NX_EXPR "@INPUT_NX@ + 2" NY_EXPR "@INPUT_NY@ + 2" NZ_EXPR "@INPUT_NZ@ + 2")
set_jupiter_datatype(TYPE REAL NAMES rad q qgeom qpt)
set_jupiter_datatype(TYPE REAL NAMES Y Yt Vf)
set_jupiter_datatype(TYPE REAL NUM_ELEMENTS 3 NAMES flux)
set_jupiter_datatype(TYPE REAL NAMES dens denss densf)
set_jupiter_datatype(TYPE REAL NAMES thc thcs thcf)
set_jupiter_datatype(TYPE REAL NAMES specht spechts spechtf)
set_jupiter_datatype(TYPE REAL NAMES mu)
set_jupiter_datatype(TYPE REAL NAMES t_liq t_soli)
set_jupiter_datatype(TYPE REAL NAMES diff_g latent emi)
set_jupiter_datatype(TYPE SERIALIZED NAMES bnd)
set_jupiter_datatype(TYPE REAL NAMES eps perm epss tf ts sgm)
set_jupiter_datatype(TYPE SERIALIZED NAMES uplsflg)
set_jupiter_datatype(TYPE REAL NUM_ELEMENTS 3 NAMES bnd_norm_u)
set_jupiter_datatype(TYPE REAL NUM_ELEMENTS 3 NAMES bnd_norm_v)
set_jupiter_datatype(TYPE REAL NUM_ELEMENTS 3 NAMES bnd_norm_w)
set_jupiter_datatype(TYPE REAL NAMES div_u)
set_jupiter_datatype(TYPE REAL NAMES mass_source_g)
set_jupiter_datatype(TYPE SERIALIZED NAMES comp_data)

# standard output and standard error output
set_jupiter_datatype(TYPE TEXT_MSG NAMES _STDOUT)
set_jupiter_datatype(TYPE TEXT_MSG NAMES _STDERR)

# Message file written by `fp`
set_jupiter_datatype(TYPE TEXT_MSG NAMES _MESSAGES)

# LPT result file written by `LPT_outf`
set_jupiter_datatype(TYPE TEXT NAMES _LPTOUTF)

# Files written by `Geom_dump`
set_jupiter_datatype(TYPE SINGLE NAMES _GEOM_DUMP)

# Files wrttten by 'jupiter-dumpmesh' (text)
set_jupiter_datatype(TYPE TEXT NAMES _MESH_DUMP)

## make-vtk execution definitions

jupiter_test_define_mkvtk_config(
  NAME MKVTKNP MAKE_VTK OUTNAME mkvtknp
  ARGS --read-geom-dump --no-particle OUTDIR vtk/np
  EXPECT 0)

jupiter_test_define_mkvtk_config(
  NAME MKVTKNF MAKE_VTK OUTNAME mkvtknf
  ARGS --read-geom-dump --no-fluid OUTDIR vtk/nf
  EXPECT 0)

jupiter_test_define_mkvtk_config(
  NAME MKVTKPF MAKE_VTK OUTNAME mkvtkpf
  ARGS --read-geom-dump OUTDIR vtk/pf
  EXPECT 0)

jupiter_test_define_mkvtk_config(
  NAME MKVTKU MAKE_VTKU OUTNAME mkvtku
  ARGS --read-geom-dump OUTDIR vtk/u
  EXPECT 0)

jupiter_test_define_mkvtk_config(
  NAME MKXDMFT MAKE_XDMF OUTNAME mkxdmft
  ARGS --read-geom-dump OUTDIR xdmf/t
  EXPECT 0)

jupiter_test_define_mkvtk_config(
  NAME MKXDMFA MAKE_XDMF OUTNAME mkxdmfa SKIP_INDICES
  ARGS --read-geom-dump OUTDIR xdmf/a
  EXPECT 0)

## Default make-vtk configs to generate on add_jupiter_input_test
set(JUPITER_INPUT_TEST_DEFAULT_MKVTK_CONFIGS
  MKVTKNP MKVTKNF MKVTKPF MKVTKU MKXDMFT MKXDMFA)

## Test for SKIP_MAKE_DIRECTORY (make-vtk creates for you)
jupiter_test_define_mkvtk_config(
  NAME MKVTK_NONEXISTENT_OUTDIR1 MAKE_VTKU OUTNAME mkvtknoout1
  OUTDIR vtk/nonexistent SKIP_MAKE_DIRECTORY EXPECT 0)

# Read restart
jupiter_test_define_mkvtk_config(
  NAME MKVTK_RESTART MAKE_VTK OUTNAME mkvtkr
  SKIP_INDICES RESTART OUTDIR vtk/r SKIP_INDICES EXPECT 0)

jupiter_test_define_mkvtk_config(
  NAME MKVTKU_RESTART MAKE_VTKU OUTNAME mkvtkur
  SKIP_INDICES RESTART OUTDIR vtk/ur SKIP_INDICES EXPECT 0)

jupiter_test_define_mkvtk_config(
  NAME MKXDMF_RESTART MAKE_XDMF OUTNAME mkxdmfr
  SKIP_INDICES RESTART OUTDIR xdmf/r SKIP_INDICES EXPECT 0)

add_jupiter_input_test(NAME "empty-input" EXPECT 1)
add_jupiter_input_test(NAME "empty-input-file-specified"
  PARAMS input/param/empty.txt FLAGS input/flags/empty.txt
  PLIST input/plist/empty.txt  GEOMETRY input/geom/empty.txt
  EXPECT 1 EXPECT_DUMPMESH 1)

add_jupiter_input_test(NAME "print-level-0"
  PARAMS input/param/empty.txt FLAGS input/flags/empty.txt
  PLIST input/plist/empty.txt  GEOMETRY input/geom/empty.txt
  ARGS "-print_level;-" EXPECT 1 EXPECT_DUMPMESH 1)

add_jupiter_input_test(NAME "print-level-1"
  PARAMS input/param/empty.txt FLAGS input/flags/empty.txt
  PLIST input/plist/empty.txt  GEOMETRY input/geom/empty.txt
  ARGS "-print_level;+" EXPECT 1 EXPECT_DUMPMESH 1)

add_jupiter_input_test(NAME "print-level-2"
  PARAMS input/param/empty.txt FLAGS input/flags/empty.txt
  PLIST input/plist/empty.txt  GEOMETRY input/geom/empty.txt
  ARGS "-print_level;-warn" EXPECT 1 EXPECT_DUMPMESH 1)

add_jupiter_input_test(NAME "print-level-3"
  PARAMS input/param/empty.txt FLAGS input/flags/empty.txt
  PLIST input/plist/empty.txt  GEOMETRY input/geom/empty.txt
  ARGS "-print_level;+debug" EXPECT 1 EXPECT_DUMPMESH 1)

add_jupiter_input_test(NAME "print-level-4"
  PARAMS input/param/empty.txt FLAGS input/flags/empty.txt
  PLIST input/plist/empty.txt  GEOMETRY input/geom/empty.txt
  ARGS "-print_level;+debug" MPI_NP 2 EXPECT 1 EXPECT_DUMPMESH 1)

add_jupiter_input_test(NAME "smallest-run"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0
  COMPARE_TOLERANCES RELATIVE fs 1.0 ABSOLUTE fl 1.0 Y 1e-4
  # these tolerances are just for testing script
  EXPECT_MESSAGES
  WARN "output_v: Key not found, using default"
  MKVTK_CONFIGS ${JUPITER_INPUT_TEST_DEFAULT_MKVTK_CONFIGS}
                MKVTK_NONEXISTENT_OUTDIR1 MKVTK_RESTART MKVTKU_RESTART
                MKXDMF_RESTART)

add_jupiter_input_test(NAME "fp-test"
  PARAMS input/param/small.txt FLAGS input/flags/fp-test.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/fp-test.txt
  EXPECT 0
  EXPECT_MSGFILES "messages/0000.lst"
  GEOM_DUMP_FILES BYPROCESS "data/whole_%04r.single.dat")

add_jupiter_input_test(NAME "fp-test2"
  PARAMS input/param/small.txt FLAGS input/flags/fp-test2.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/fp-test.txt
  EXPECT 0
  EXPECT_MSGFILES "messages/no-rank.lst"
  GEOM_DUMP_FILES BYPROCESS "data/whole_%04r.single.dat")

if (JUPITER_USE_MPI)
  add_jupiter_input_test(NAME "fp-test-mpi"
    PARAMS input/param/small-mpi.txt FLAGS input/flags/fp-test.txt
    PLIST input/plist.3.txt.in  GEOMETRY input/geom/fp-test.txt
    MPI_NP 2
    EXPECT 0
    EXPECT_MSGFILES "messages/0000.lst" "messages/0001.lst"
    GEOM_DUMP_FILES BYPROCESS "data/whole_%04r.single.dat")

  add_jupiter_input_test(NAME "fp-test2-mpi"
    PARAMS input/param/small-mpi.txt FLAGS input/flags/fp-test2.txt
    PLIST input/plist.3.txt.in  GEOMETRY input/geom/fp-test.txt
    MPI_NP 2
    EXPECT 0
    EXPECT_MSGFILES "messages/no-rank.lst"
    GEOM_DUMP_FILES BYPROCESS "data/whole_%04r.single.dat")
endif()

add_jupiter_input_test(NAME "restart-0"
  PARAMS input/param/small.txt FLAGS input/flags/restart-0.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  RESTART RESTART_N 1 EXPECT 0 BINARY_OUTPUT_DOUBLE ON)

add_jupiter_input_test(NAME "outp-boundary"
  PARAMS input/param/outp-boundary.txt FLAGS input/flags/outp-boundary.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0)

add_jupiter_input_test(NAME "outp-const-p-1"
  PARAMS input/param/outp-boundary.txt FLAGS input/flags/outp-boundary.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/const-p-1.txt
  EXPECT 0)

add_jupiter_input_test(NAME "outp-const-p-2"
  PARAMS input/param/outp-boundary.txt FLAGS input/flags/outp-boundary.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/const-p-2.txt
  EXPECT 0)

add_jupiter_input_test(NAME "heat-source"
  PARAMS input/param/heat_source.txt FLAGS input/flags/heat_source.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/heat_source.txt
  EXPECT 0)

add_jupiter_input_test(NAME "heat-source-err"
  PARAMS input/param/heat_source-1.txt FLAGS input/flags/heat_source.txt
  PLIST input/plist.5.txt.in  GEOMETRY input/geom/heat_source.txt
  EXPECT 1 EXPECT_DUMPMESH 1)

add_jupiter_input_test(NAME "phase-change"
  PARAMS input/param/phase_change.txt FLAGS input/flags/phase_change.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/phase_change.txt
  EXPECT 0)

# add_jupiter_input_test(NAME "phase-change-mushy"
#   PARAMS input/param/phase_change.txt FLAGS input/flags/phase_change.txt
#   PLIST input/plist.3.mushy.txt.in  GEOMETRY input/geom/phase_change.txt
#   EXPECT 0)

add_jupiter_input_test(NAME "phase-change-restart"
  PARAMS input/param/phase_change-r.txt FLAGS input/flags/phase_change.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/phase_change.txt
  RESTART RESTART_PARAMS input/param/phase_change.txt EXPECT 0)

# add_jupiter_input_test(NAME "phase-change-mushy-restart"
#   PARAMS input/param/phase_change-r.txt FLAGS input/flags/phase_change.txt
#   PLIST input/plist.3.mushy.txt.in  GEOMETRY input/geom/phase_change.txt
#   RESTART RESTART_PARAMS input/param/phase_change.txt EXPECT 0)

add_jupiter_input_test(NAME "phase-change-vapor"
  PARAMS input/param/phase_change_vc.txt FLAGS input/flags/phase_change_vc.txt
  PLIST input/plist.wat.txt.in  GEOMETRY input/geom/phase_change_vc.txt
  EXPECT 0)

add_jupiter_input_test(NAME "ibm-heat"
  PARAMS input/param/porous.txt FLAGS input/flags/ibm-heat.txt
  PLIST input/plist.darcy2.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "ibm-dyn-heat"
  PARAMS input/param/porous-dyn.txt FLAGS input/flags/ibm-dyn-heat.txt
  PLIST input/plist.darcy2.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-heat"
  PARAMS input/param/porous.txt FLAGS input/flags/porous-heat.txt
  PLIST input/plist.darcy0.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-dyn-heat"
  PARAMS input/param/porous-dyn.txt FLAGS input/flags/porous-dyn-heat.txt
  PLIST input/plist.darcy0.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-almost-ibm-heat"
  PARAMS input/param/porous-ai.txt FLAGS input/flags/porous-heat.txt
  PLIST input/plist.darcy1.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-almost-ibm-dyn-heat"
  PARAMS input/param/porous-ai-dyn.txt FLAGS input/flags/porous-dyn-heat.txt
  PLIST input/plist.darcy1.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-two-e-heat"
  PARAMS input/param/porous.txt FLAGS input/flags/porous-twoe-heat.txt
  PLIST input/plist.darcy0.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-two-e-dyn-heat"
  PARAMS input/param/porous-dyn.txt FLAGS input/flags/porous-dyn-twoe-heat.txt
  PLIST input/plist.darcy0.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-two-e-almost-ibm-heat"
  PARAMS input/param/porous-ai.txt FLAGS input/flags/porous-twoe-heat.txt
  PLIST input/plist.darcy1.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-two-e-almost-ibm-dyn-heat"
  PARAMS input/param/porous-ai-dyn.txt
  FLAGS input/flags/porous-dyn-twoe-heat.txt
  PLIST input/plist.darcy1.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-heat-exp"
  PARAMS input/param/porous.txt FLAGS input/flags/porous-heat-exp.txt
  PLIST input/plist.darcy0.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-almost-ibm-heat-exp"
  PARAMS input/param/porous-ai.txt FLAGS input/flags/porous-heat-exp.txt
  PLIST input/plist.darcy1.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-two-e-heat-exp"
  PARAMS input/param/porous.txt FLAGS input/flags/porous-twoe-heat-exp.txt
  PLIST input/plist.darcy0.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-two-e-almost-ibm-heat-exp"
  PARAMS input/param/porous-ai.txt FLAGS input/flags/porous-twoe-heat-exp.txt
  PLIST input/plist.darcy1.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)


add_jupiter_input_test(NAME "ibm-mix-heat"
  PARAMS input/param/porous.txt FLAGS input/flags/porous-mix-heat.txt
  PLIST input/plist.darcy2.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "ibm-mix-dyn-heat"
  PARAMS input/param/porous-dyn.txt FLAGS input/flags/porous-mix-dyn-heat.txt
  PLIST input/plist.darcy2.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-mix-heat"
  PARAMS input/param/porous.txt FLAGS input/flags/porous-mix-heat.txt
  PLIST input/plist.darcy0.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-mix-dyn-heat"
  PARAMS input/param/porous-dyn.txt FLAGS input/flags/porous-mix-dyn-heat.txt
  PLIST input/plist.darcy0.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-mix-almost-ibm-heat"
  PARAMS input/param/porous-ai.txt FLAGS input/flags/porous-mix-heat.txt
  PLIST input/plist.darcy1.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-mix-almost-ibm-dyn-heat"
  PARAMS input/param/porous-ai-dyn.txt FLAGS input/flags/porous-mix-dyn-heat.txt
  PLIST input/plist.darcy1.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-mix-two-e-heat"
  PARAMS input/param/porous.txt FLAGS input/flags/porous-mix-twoe-heat.txt
  PLIST input/plist.darcy0.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-mix-two-e-dyn-heat"
  PARAMS input/param/porous-dyn.txt
  FLAGS input/flags/porous-mix-dyn-twoe-heat.txt
  PLIST input/plist.darcy0.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-mix-two-e-almost-ibm-heat"
  PARAMS input/param/porous-ai.txt FLAGS input/flags/porous-mix-twoe-heat.txt
  PLIST input/plist.darcy1.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-mix-two-e-almost-ibm-dyn-heat"
  PARAMS input/param/porous-ai-dyn.txt
  FLAGS input/flags/porous-mix-dyn-twoe-heat.txt
  PLIST input/plist.darcy1.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-mix-heat-exp"
  PARAMS input/param/porous.txt FLAGS input/flags/porous-mix-heat-exp.txt
  PLIST input/plist.darcy0.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-mix-almost-ibm-heat-exp"
  PARAMS input/param/porous-ai.txt FLAGS input/flags/porous-mix-heat-exp.txt
  PLIST input/plist.darcy1.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-mix-two-e-heat-exp"
  PARAMS input/param/porous.txt FLAGS input/flags/porous-mix-twoe-heat-exp.txt
  PLIST input/plist.darcy0.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-mix-two-e-almost-ibm-heat-exp"
  PARAMS input/param/porous-ai.txt
  FLAGS input/flags/porous-mix-twoe-heat-exp.txt
  PLIST input/plist.darcy1.txt.in  GEOMETRY input/geom/darcy-0.txt
  EXPECT 0)

add_jupiter_input_test(NAME "porous-ibm-mix-dyn-heat"
  PARAMS input/param/porous-mix.txt FLAGS input/flags/porous-mix-dyn-heat.txt
  PLIST input/plist.darcy3.txt.in  GEOMETRY input/geom/darcy-3.txt
  EXPECT 0)

add_jupiter_input_test(NAME "uplsflg-fl-g-nompi-nt1"
  PARAMS input/param/small.txt FLAGS input/flags/uplsflg-base.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/uplsflg-fl.txt
  EXPECT 0
  EXPECT_MESSAGES
  INFO "lls: Updating level set function: there are non-0 initial VOF value")

if (ENABLE_OPENMP)
  add_jupiter_input_test(NAME "uplsflg-fl-g-nompi-nt2"
    PARAMS input/param/small.txt FLAGS input/flags/uplsflg-base.txt
    PLIST input/plist.3.txt.in  GEOMETRY input/geom/uplsflg-fl.txt
    OpenMP_NT 2
    EXPECT 0
    EXPECT_MESSAGES
    INFO "lls: Updating level set function: there are non-0 initial VOF value")
endif()

if (JUPITER_USE_MPI)
  add_jupiter_input_test(NAME "uplsflg-fl-g-mpi-nt1"
    PARAMS input/param/small-mpi.txt FLAGS input/flags/uplsflg-base.txt
    PLIST input/plist.3.txt.in  GEOMETRY input/geom/uplsflg-fl.txt
    MPI_NP 2
    EXPECT 0
    EXPECT_MESSAGES
    INFO "lls: Updating level set function: there are non-0 initial VOF value")
endif()

add_jupiter_input_test(NAME "uplsflg-fl-c"
  PARAMS input/param/small.txt FLAGS input/flags/uplsflg-base.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  CONTROL input/control/uplsflg-fl.txt
  EXPECT 0
  EXPECT_MESSAGES
  INFO "lls: Updating level set function: there are non-0 VOF value after controlled change")

add_jupiter_input_test(NAME "uplsflg-fs-c"
  PARAMS input/param/small.txt FLAGS input/flags/uplsflg-base.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  CONTROL input/control/uplsflg-fs.txt
  EXPECT 0
  EXPECT_MESSAGES
  INFO "lls: Updating level set function: there are non-0 VOF value after controlled change"
       "ls: Updating level set function: there are non-0 VOF value after controlled change")

add_jupiter_input_test(NAME "uplsflg-fsfl-c"
  PARAMS input/param/small.txt FLAGS input/flags/uplsflg-base.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  CONTROL input/control/uplsflg-fsfl.txt
  EXPECT 0
  EXPECT_MESSAGES
  INFO "lls: Updating level set function: there are non-0 VOF value after controlled change"
       "ls: Updating level set function: there are non-0 VOF value after controlled change")

add_jupiter_input_test(NAME "uplsflg-pc"
  PARAMS input/param/small.txt FLAGS input/flags/uplsflg-phase.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0
  EXPECT_MESSAGES
  INFO "lls: Updating level set function: phase change model is enabled"
       "ls: Updating level set function: phase change model is enabled")

add_jupiter_input_test(NAME "uplsflg-li-g"
  PARAMS input/param/small.txt FLAGS input/flags/uplsflg-base.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/uplsflg-li.txt
  EXPECT 0
  EXPECT_MESSAGES
  INFO "lls: Updating level set function: there are inlet condition\\\\(s\\\\) of liquids")

add_jupiter_input_test(NAME "uplsflg-li-c"
  PARAMS input/param/small.txt FLAGS input/flags/uplsflg-base.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  CONTROL input/control/uplsflg-li.txt
  EXPECT 0
  EXPECT_MESSAGES
  INFO "lls: Updating level set function: there are inlet condition\\\\(s\\\\) of liquids")

add_jupiter_input_test(NAME "uplsflg-li-rg"
  PARAMS input/param/small.txt FLAGS input/flags/uplsflg-base.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  RESTART
  RESTART_PRE_RUN "file(REMOVE_RECURSE \"data/binary_data/bnd\")"
  RESTART_GEOMETRY input/geom/uplsflg-li.txt
  EXPECT 0
  RESTART_EXPECT_MESSAGES
  INFO "Resetting boundaries with given data by input files"
       "lls: Updating level set function: there are inlet condition\\\\(s\\\\) of liquids")

add_jupiter_input_test(NAME "uplsflg-force-ls"
  PARAMS input/param/small.txt FLAGS input/flags/uplsflg-force-ls.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0
  EXPECT_MESSAGES
  INFO "ls: Updating level set function: it's set to be updated forcibly")

add_jupiter_input_test(NAME "uplsflg-force-lls"
  PARAMS input/param/small.txt FLAGS input/flags/uplsflg-force-lls.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0
  EXPECT_MESSAGES
  INFO "lls: Updating level set function: it's set to be updated forcibly")

add_jupiter_input_test(NAME "uplsflg-restart-ls"
  PARAMS input/param/small.txt FLAGS input/flags/uplsflg-force-ls.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  RESTART
  RESTART_FLAGS input/flags/uplsflg-base.txt
  EXPECT 0
  EXPECT_MESSAGES
  INFO "ls: Updating level set function: it's set to be updated forcibly"
  RESTART_EXPECT_MESSAGES
  INFO "ls: Updating level set function: restart data requires update")

add_jupiter_input_test(NAME "uplsflg-restart-lls"
  PARAMS input/param/small.txt FLAGS input/flags/uplsflg-force-lls.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  RESTART
  RESTART_FLAGS input/flags/uplsflg-base.txt
  EXPECT 0
  EXPECT_MESSAGES
  INFO "lls: Updating level set function: it's set to be updated forcibly"
  RESTART_EXPECT_MESSAGES
  INFO "lls: Updating level set function: restart data requires update")

if (JUPITER_ENABLE_METI)
  add_jupiter_input_test(NAME "solute-diff"
    PARAMS input/param/solute-diff.txt FLAGS input/flags/solute-diff.txt
    PLIST input/plist/solute-diff.txt.in GEOMETRY input/geom/solute-diff.txt
    EXPECT 0)

  add_jupiter_input_test(NAME "solute-diff-liquid"
    PARAMS input/param/solute-diff.txt FLAGS input/flags/solute-diff.txt
    PLIST input/plist/solute-diff.txt.in
    GEOMETRY input/geom/solute-diff-liquid.txt
    EXPECT 0)

  # This test is known not to be able to solve in SINGLE precision
  if (NOT "${JUPITER_FP_PRECISION}" STREQUAL SINGLE)
    add_jupiter_input_test(NAME "solute-diff-gas"
      PARAMS input/param/cavity.txt FLAGS input/flags/solute-diff.txt
      PLIST input/plist/solute-cavity.txt.in
      GEOMETRY input/geom/solute-diff-gas.txt
      EXPECT 0)
  endif()

  # This test is known not to be able to solve in SINGLE precision
  if (NOT "${JUPITER_FP_PRECISION}" STREQUAL SINGLE)
    add_jupiter_input_test(NAME "solute-diff-cavity"
      PARAMS input/param/cavity.txt FLAGS input/flags/solute-diff-cavity.txt
      PLIST input/plist/solute-cavity.txt.in GEOMETRY input/geom/cavity-l.txt
      EXPECT 0)
  endif()

  # This test is known not to be able to solve in SINGLE precision
  if (NOT "${JUPITER_FP_PRECISION}" STREQUAL SINGLE)
    add_jupiter_input_test(NAME "solute-diff-gas-cavity"
      PARAMS input/param/cavity.txt FLAGS input/flags/solute-diff-cavity.txt
      PLIST input/plist/solute-cavity.txt.in GEOMETRY input/geom/cavity-g.txt
      EXPECT 0)
  endif()

  # This test is known not to be able to solve in SINGLE precision
  if (NOT "${JUPITER_FP_PRECISION}" STREQUAL SINGLE)
    add_jupiter_input_test(NAME "solute-diff-gas-geom-empty"
      PARAMS input/param/cavity.txt FLAGS input/flags/solute-diff-cavity.txt
      PLIST input/plist/solute-cavity.txt.in GEOMETRY input/geom/no-init.txt
      EXPECT 0)
  endif()

  # This test is known not to be able to solve in SINGLE precision
  if (NOT "${JUPITER_FP_PRECISION}" STREQUAL SINGLE)
    add_jupiter_input_test(NAME "solute-diff-melt"
      PARAMS input/param/sdiff_phase_change_melt.txt
      FLAGS input/flags/sdiff_phase_change.txt
      PLIST input/plist/solute-diff-0.txt.in
      GEOMETRY input/geom/solute-diff-m.txt
      EXPECT 0)
  endif()

  # This test is known not to be able to solve in SINGLE precision
  if (NOT "${JUPITER_FP_PRECISION}" STREQUAL SINGLE)
    add_jupiter_input_test(NAME "solute-diff-solidify"
      PARAMS input/param/sdiff_phase_change_solidify.txt
      FLAGS input/flags/sdiff_phase_change.txt
      PLIST input/plist/solute-diff-0.txt.in
      GEOMETRY input/geom/solute-diff-s.txt
      EXPECT 0)
  endif()

  # This test is known not to be able to solve in SINGLE precision
  if (NOT "${JUPITER_FP_PRECISION}" STREQUAL SINGLE)
    add_jupiter_input_test(NAME "solute-diff-melt-mushy"
      PARAMS input/param/sdiff_phase_change_melt.txt
      FLAGS input/flags/sdiff_phase_change.txt
      PLIST input/plist/solute-diff-0-mushy.txt.in
      GEOMETRY input/geom/solute-diff-m.txt
      EXPECT 0)
  endif()

  # This test is known not to be able to solve in SINGLE precision
  if (NOT "${JUPITER_FP_PRECISION}" STREQUAL SINGLE)
    add_jupiter_input_test(NAME "solute-diff-solidify-mushy"
      PARAMS input/param/sdiff_phase_change_solidify.txt
      FLAGS input/flags/sdiff_phase_change.txt
      PLIST input/plist/solute-diff-0-mushy.txt.in
      GEOMETRY input/geom/solute-diff-s.txt
      EXPECT 0)
  endif()

  add_jupiter_input_test(NAME "solute-diff-restart"
    PARAMS input/param/solute-diff-r.txt FLAGS input/flags/solute-diff.txt
    PLIST input/plist/solute-diff.txt.in GEOMETRY input/geom/solute-diff.txt
    RESTART RESTART_PARAMS input/param/solute-diff.txt EXPECT 0)

  # This test is known not to be able to solve in SINGLE precision
  if (NOT "${JUPITER_FP_PRECISION}" STREQUAL SINGLE)
    add_jupiter_input_test(NAME "solute-diff-melt-restart"
      PARAMS input/param/sdiff_phase_change_melt-r.txt
      FLAGS input/flags/sdiff_phase_change.txt
      PLIST input/plist/solute-diff-0.txt.in
      GEOMETRY input/geom/solute-diff-m.txt
      RESTART RESTART_PARAMS input/param/sdiff_phase_change_melt.txt EXPECT 0)
  endif()

  # This test is known not to be able to solve in SINGLE precision
  if (NOT "${JUPITER_FP_PRECISION}" STREQUAL SINGLE)
    add_jupiter_input_test(NAME "solute-diff-melt-mushy-restart"
      PARAMS input/param/sdiff_phase_change_melt-r.txt
      FLAGS input/flags/sdiff_phase_change.txt
      PLIST input/plist/solute-diff-0-mushy.txt.in
      GEOMETRY input/geom/solute-diff-m.txt
      RESTART RESTART_PARAMS input/param/sdiff_phase_change_melt.txt EXPECT 0)
  endif()

  add_jupiter_input_test(NAME "oxidation"
    PARAMS input/param/oxidation.txt FLAGS input/flags/oxidation.txt
    PLIST input/plist/oxidation.txt.in GEOMETRY input/geom/oxidation.txt
    EXPECT 0)

  add_jupiter_input_test(NAME "solute-diff-table"
    PARAMS input/param/solute-diff.txt FLAGS input/flags/solute-diff.txt
    PLIST input/plist/solute-diff-tab.txt.in GEOMETRY input/geom/solute-diff.txt
    PRE_RUN input/dummy-liqt.cmake EXPECT 0)
endif()

if (JUPITER_ENABLE_LPT)
  add_jupiter_input_test(NAME "lpt-errors"
    PARAMS input/param/lpt-test-errors.txt
    FLAGS input/flags/lpt-test-errors.txt
    PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
    EXPECT 1 EXPECT_DUMPMESH 1)

  add_jupiter_input_test(NAME "lpt-small-run"
    PARAMS input/param/lpt-small-run.txt
    FLAGS input/flags/lpt-small-run.txt
    PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt EXPECT 0)

  add_jupiter_input_test(NAME "lpt-test-case-1-1"
    PARAMS input/param/lpt-case-1-1.txt
    FLAGS input/flags/lpt-only.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-case-1-1.txt EXPECT 0)

  add_jupiter_input_test(NAME "lpt-test-case-1-2"
    PARAMS input/param/lpt-case-1-2.txt
    FLAGS input/flags/lpt-only.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-case-1-2.txt EXPECT 0)

  add_jupiter_input_test(NAME "lpt-test-case-2-1"
    PARAMS input/param/lpt-case-2-1.txt
    FLAGS input/flags/lpt-only.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-case-2-1.txt EXPECT 0)

  add_jupiter_input_test(NAME "lpt-test-case-2-2"
    PARAMS input/param/lpt-case-2-2.txt
    FLAGS input/flags/lpt-only-2R.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-case-2-2.txt EXPECT 0)

  add_jupiter_input_test(NAME "lpt-test-case-3-1"
    PARAMS input/param/lpt-case-3-1.txt
    FLAGS input/flags/lpt-only.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-case-3-1.txt EXPECT 0)

  add_jupiter_input_test(NAME "lpt-test-pewall-1"
    PARAMS input/param/lpt-test-pewall-1.txt
    FLAGS input/flags/lpt-with-ibm.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-test-pewall-1.txt EXPECT 0)

  add_jupiter_input_test(NAME "lpt-test-case-1-1a"
    PARAMS input/param/lpt-case-1-1a.txt
    FLAGS input/flags/lpt-only.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-case-1-1.txt EXPECT 0)

  add_jupiter_input_test(NAME "lpt-test-case-1-2a"
    PARAMS input/param/lpt-case-1-2a.txt
    FLAGS input/flags/lpt-only.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-case-1-2.txt EXPECT 0)

  add_jupiter_input_test(NAME "lpt-test-case-2-1a"
    PARAMS input/param/lpt-case-2-1a.txt
    FLAGS input/flags/lpt-only.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-case-2-1.txt EXPECT 0)

  add_jupiter_input_test(NAME "lpt-test-case-2-2a"
    PARAMS input/param/lpt-case-2-2a.txt
    FLAGS input/flags/lpt-only-2R.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-case-2-2.txt EXPECT 0)

  add_jupiter_input_test(NAME "lpt-test-case-3-1a"
    PARAMS input/param/lpt-case-3-1a.txt
    FLAGS input/flags/lpt-only.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-case-3-1.txt EXPECT 0)

  add_jupiter_input_test(NAME "lpt-test-case-3-1a-rk2"
    PARAMS input/param/lpt-case-3-1a.txt
    FLAGS input/flags/lpt-only-rk2.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-case-3-1.txt EXPECT 0)

  add_jupiter_input_test(NAME "lpt-test-case-3-1a-rk3"
    PARAMS input/param/lpt-case-3-1a.txt
    FLAGS input/flags/lpt-only-rk3.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-case-3-1.txt EXPECT 0)

  if (JUPITER_ENABLE_LPT STREQUAL "LPTX")
    add_jupiter_input_test(NAME "lpt-test-case-br"
      PARAMS input/param/lpt-case-br.txt
      FLAGS input/flags/lpt-only-br.txt
      PLIST input/plist/lpt-test-br.txt.in
      GEOMETRY input/geom/lpt-case-br.txt EXPECT 0)

    add_jupiter_input_test(NAME "lpt-test-case-br2"
      PARAMS input/param/lpt-case-br2.txt
      FLAGS input/flags/lpt-only-br.txt
      PLIST input/plist/lpt-test-br.txt.in
      GEOMETRY input/geom/lpt-case-br.txt EXPECT 0 HEAVY)

    add_jupiter_input_test(NAME "lpt-test-case-thermophoretic"
      PARAMS input/param/lpt-thermophoretic.txt
      FLAGS input/flags/lpt-thermophoretic.txt
      PLIST input/plist/lpt-thermophoretic.txt.in
      GEOMETRY input/geom/lpt-thermophoretic.txt EXPECT 0)

    add_jupiter_input_test(NAME "lpt-test-case-follow-fluid"
      PARAMS input/param/lpt-case-heat-exc.txt
      FLAGS input/flags/lpt-heat-exc-ffluid.txt
      PLIST input/plist/lpt-tests.0.txt.in
      GEOMETRY input/geom/lpt-case-heat-exc.txt EXPECT 0)

    add_jupiter_input_test(NAME "lpt-test-case-ranz-marshall"
      PARAMS input/param/lpt-case-heat-exc.txt
      FLAGS input/flags/lpt-heat-exc-ranz.txt
      PLIST input/plist/lpt-tests.0.txt.in
      GEOMETRY input/geom/lpt-case-heat-exc.txt EXPECT 0)
  endif()

  add_jupiter_input_test(NAME "lpt-test-pewall-2"
    PARAMS input/param/lpt-test-pewall-2.txt
    FLAGS input/flags/lpt-with-ibm.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-test-pewall-1.txt EXPECT 0)

  set(MSG1)
  set(MSG2)
  set(MSG3)
  if(JUPITER_ENABLE_LPT STREQUAL "LPT")
    set(MSG1 EXPECT_MSGFILES "LPT_no_rank.lst")
    set(MSG2 EXPECT_MSGFILES "LPT_0000.lst")
    set(MSG3 EXPECT_MSGFILES ".")
  endif()

  add_jupiter_input_test(NAME "lpt-test-outf1"
    PARAMS input/param/lpt-test-outf1.txt
    FLAGS input/flags/lpt-only.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-case-1-1.txt
    EXPECT 0 ${MSG1})

  add_jupiter_input_test(NAME "lpt-test-outf2"
    PARAMS input/param/lpt-test-outf2.txt
    FLAGS input/flags/lpt-only.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-case-1-1.txt
    EXPECT 0 ${MSG2})

  add_jupiter_input_test(NAME "lpt-test-outf3"
    PARAMS input/param/lpt-test-outf3.txt
    FLAGS input/flags/lpt-only.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-case-1-1.txt EXPECT 0
    ${MSG3})

  if (JUPITER_USE_MPI)
    add_jupiter_input_test(NAME "lpt-test-outf1-mpi"
      PARAMS input/param/lpt-test-outf1-mpi.txt
      FLAGS input/flags/lpt-only.txt
      PLIST input/plist/lpt-tests.0.txt.in
      GEOMETRY input/geom/lpt-case-1-1.txt
      MPI_NP 2
      EXPECT 0 ${MSG1})

    add_jupiter_input_test(NAME "lpt-test-outf2-mpi"
      PARAMS input/param/lpt-test-outf2-mpi.txt
      FLAGS input/flags/lpt-only.txt
      PLIST input/plist/lpt-tests.0.txt.in
      GEOMETRY input/geom/lpt-case-1-1.txt
      MPI_NP 2
      EXPECT 0 ${MSG2})
  endif()
else()
  add_jupiter_input_test(NAME "lpt-errors-no-lpt"
    PARAMS input/param/lpt-test-errors.txt
    FLAGS input/flags/lpt-test-errors.txt
    PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt EXPECT 1)
endif()

add_jupiter_input_test(NAME "variable-delta1"
  PARAMS input/param/small-vard1.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0)

add_jupiter_input_test(NAME "variable-delta2"
  PARAMS input/param/small-vard2.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0)

add_jupiter_input_test(NAME "variable-delta3"
  PARAMS input/param/small-vard3.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0)

add_jupiter_input_test(NAME "variable-delta-error1"
  PARAMS input/param/small-vard-e1.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 1 EXPECT_DUMPMESH 1
  EXPECT_MESSAGES
  ERROR ": -0\\\\.2: The coordinate of ending region must be greater than the start, .*"
  ERROR ": variable_delta_y: Total length of domain \\\\( *-.*\\\\) is negative")

add_jupiter_input_test(NAME "variable-delta-error2"
  PARAMS input/param/small-vard-e2.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 1 EXPECT_DUMPMESH 1
  EXPECT_MESSAGES
  ERROR ": _____: Invalid value in non_uniform_grid_func")

add_jupiter_input_test(NAME "variable-delta-error3"
  PARAMS input/param/small-vard-e3.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 1 EXPECT_DUMPMESH 1
  EXPECT_MESSAGES
  ERROR ": -1: The number of divisions must be positive")

add_jupiter_input_test(NAME "negative-restart-dump"
  PARAMS input/param/negative-restart-dump.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0)

add_jupiter_input_test(NAME "output-float-byprocess"
  PARAMS input/param/pout-test.txt FLAGS input/flags/out-flt-byp.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0 MPI_NP 2)

add_jupiter_input_test(NAME "output-float-unify-mpi"
  PARAMS input/param/pout-test.txt FLAGS input/flags/out-flt-unim.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0 MPI_NP 2)

add_jupiter_input_test(NAME "output-float-unify-gather"
  PARAMS input/param/pout-test.txt FLAGS input/flags/out-flt-unig.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0 MPI_NP 2)

add_jupiter_input_test(NAME "output-double-byprocess"
  PARAMS input/param/pout-test.txt FLAGS input/flags/out-dbl-byp.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0 MPI_NP 2 BINARY_OUTPUT_DOUBLE ON)

add_jupiter_input_test(NAME "output-double-unify-mpi"
  PARAMS input/param/pout-test.txt FLAGS input/flags/out-dbl-unim.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0 MPI_NP 2 BINARY_OUTPUT_DOUBLE ON)

add_jupiter_input_test(NAME "output-double-unify-gather"
  PARAMS input/param/pout-test.txt FLAGS input/flags/out-dbl-unig.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0 MPI_NP 2 BINARY_OUTPUT_DOUBLE ON)

add_jupiter_input_test(NAME "output-template-errors"
  PARAMS input/param/small.txt FLAGS input/flags/out-tmpl-errors.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 1 EXPECT_DUMPMESH 1)

add_jupiter_input_test(NAME "output-num-change"
  PARAMS input/param/change-output.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  RESTART RESTART_PARAMS input/param/change-output-r.txt
  RESTART_PRE_RUN "message(STATUS \"RESTART_PRE_RUN test\")"
  RESTART_POST_RUN "message(STATUS \"RESTART_POST_RUN test\")"
  EXPECT 0)

add_jupiter_input_test(NAME "output-num-not-change"
  PARAMS input/param/change-output.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  RESTART RESTART_PARAMS input/param/change-output-n.txt
  EXPECT 0)

add_jupiter_input_test(NAME "output-end-change"
  PARAMS input/param/change-output.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  RESTART RESTART_PARAMS input/param/change-output-end.txt
  EXPECT 0)

add_jupiter_input_test(NAME "start-time-further-restart-time"
  PARAMS input/param/change-output.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  RESTART RESTART_PARAMS input/param/change-output-further-r.txt
  EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-0"
  PARAMS input/param/valvof.txt FLAGS input/flags/valvof-0.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s.txt
  EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-1"
  PARAMS input/param/valvof.txt FLAGS input/flags/valvof-1.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s.txt
  EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-10"
  PARAMS input/param/valvof.txt FLAGS input/flags/valvof-10.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s.txt
  EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-a"
  PARAMS input/param/valvof.txt FLAGS input/flags/valvof-a.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s.txt
  EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-10l"
  PARAMS input/param/valvof.txt FLAGS input/flags/valvof-10.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-l.txt
  EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-10ls"
  PARAMS input/param/valvof.txt FLAGS input/flags/valvof-10.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-ls.txt
  EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-a0"
  PARAMS input/param/valvof.txt FLAGS input/flags/valvof-a.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s2-0.txt
  EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-a1"
  PARAMS input/param/valvof.txt FLAGS input/flags/valvof-a.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s2-1.txt
  EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-a2"
  PARAMS input/param/valvof.txt FLAGS input/flags/valvof-a.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s2-a.txt
  EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-am"
  PARAMS input/param/valvof.txt FLAGS input/flags/valvof-a.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-m.txt
  EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-n"
  PARAMS input/param/valvof.txt FLAGS input/flags/valvof-n.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s.txt
  EXPECT 0)

add_jupiter_input_test(NAME "validate-vof-print-2p-0"
  PARAMS input/param/valvof-2.txt FLAGS input/flags/valvof-a.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s2-0.txt
  MPI_NP 2 EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-2p-1"
  PARAMS input/param/valvof-2.txt FLAGS input/flags/valvof-a.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s2-1.txt
  MPI_NP 2 EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-2p-a"
  PARAMS input/param/valvof-2.txt FLAGS input/flags/valvof-a.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s2-a.txt
  MPI_NP 2 EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-2p-m"
  PARAMS input/param/valvof-2.txt FLAGS input/flags/valvof-a.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-m.txt
  MPI_NP 2 EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-2p-10-0"
  PARAMS input/param/valvof-2.txt FLAGS input/flags/valvof-10.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s2-0.txt
  MPI_NP 2 EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-2p-10-1"
  PARAMS input/param/valvof-2.txt FLAGS input/flags/valvof-10.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s2-1.txt
  MPI_NP 2 EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-2p-10-a"
  PARAMS input/param/valvof-2.txt FLAGS input/flags/valvof-10.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s2-a.txt
  MPI_NP 2 EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-2p-10-m"
  PARAMS input/param/valvof-2.txt FLAGS input/flags/valvof-10.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-m.txt
  MPI_NP 2 EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-2p-1-0"
  PARAMS input/param/valvof-2.txt FLAGS input/flags/valvof-1.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s2-0.txt
  MPI_NP 2 EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-2p-1-1"
  PARAMS input/param/valvof-2.txt FLAGS input/flags/valvof-1.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s2-1.txt
  MPI_NP 2 EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-2p-1-a"
  PARAMS input/param/valvof-2.txt FLAGS input/flags/valvof-1.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s2-a.txt
  MPI_NP 2 EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-2p-1-m"
  PARAMS input/param/valvof-2.txt FLAGS input/flags/valvof-1.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-m.txt
  MPI_NP 2 EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-1y"
  PARAMS input/param/valvof.txt FLAGS input/flags/valvof-1y.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s.txt
  REQUIRES METI EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-10y"
  PARAMS input/param/valvof.txt FLAGS input/flags/valvof-10y.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s.txt
  REQUIRES METI EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-ay"
  PARAMS input/param/valvof.txt FLAGS input/flags/valvof-ay.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s.txt
  REQUIRES METI EXPECT 1)

add_jupiter_input_test(NAME "validate-vof-print-ny"
  PARAMS input/param/valvof.txt FLAGS input/flags/valvof-ny.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-s.txt
  REQUIRES METI EXPECT 0)

add_jupiter_input_test(NAME "validate-vof-print-my"
  PARAMS input/param/valvof.txt FLAGS input/flags/valvof-ay.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/valvof-m.txt
  REQUIRES METI EXPECT 1)

add_jupiter_input_test(NAME "tempdep-test"
  PARAMS input/param/tempdep.txt FLAGS input/flags/tempdep.txt
  PLIST input/plist/tdtest.txt.in GEOMETRY input/geom/tempdep.txt
  EXPECT 0)

add_jupiter_input_test(NAME "moving-bnd-1"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/moving-bnd-1.txt
  EXPECT 0)

add_jupiter_input_test(NAME "moving-bnd-2"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/moving-bnd-2.txt
  EXPECT 0)

add_jupiter_input_test(NAME "postp-vint"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/postp-vint.txt
  EXPECT 0)

add_jupiter_input_test(NAME "postp-sum"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/postp-sum.txt
  EXPECT 0)

add_jupiter_input_test(NAME "postp-mask"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/postp-mask.txt
  EXPECT 0)

add_jupiter_input_test(NAME "postp-del-variable"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/postp-delvar.txt
  EXPECT 0)

add_jupiter_input_test(NAME "postp-del-variable-except"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/postp-delvar-exc.txt
  EXPECT 0)

add_jupiter_input_test(NAME "mask-extent"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/mask-extent.txt
  EXPECT 0)

add_jupiter_input_test(NAME "mask-geometry"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/mask-geom.txt
  EXPECT 0)

add_jupiter_input_test(NAME "mask-point"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/mask-point.txt
  EXPECT 0)

add_jupiter_input_test(NAME "mask-or"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/mask-or.txt
  EXPECT 0)

add_jupiter_input_test(NAME "mask-and"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/mask-and.txt
  EXPECT 0)

add_jupiter_input_test(NAME "mask-sub"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/mask-sub.txt
  EXPECT 0)

add_jupiter_input_test(NAME "mask-xor"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/mask-xor.txt
  EXPECT 0)

add_jupiter_input_test(NAME "mask-eqv"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/mask-eqv.txt
  EXPECT 0)

add_jupiter_input_test(NAME "mask-nor"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/mask-nor.txt
  EXPECT 0)

add_jupiter_input_test(NAME "mask-nand"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/mask-nand.txt
  EXPECT 0)

add_jupiter_input_test(NAME "mask-nxor"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/mask-nxor.txt
  EXPECT 0)

add_jupiter_input_test(NAME "mask-neqv"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/mask-neqv.txt
  EXPECT 0)

add_jupiter_input_test(NAME "fv-get"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/fv-get.txt
  EXPECT 0)

add_jupiter_input_test(NAME "write-field-variables"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/small.txt
  CONTROL input/control/write_fvars.txt
  EXPECT 0)

add_jupiter_input_test(NAME "boundary-with-geom"
  PARAMS input/param/small.txt FLAGS input/flags/pboundary.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/pboundary.txt
  EXPECT 0)

add_jupiter_input_test(NAME "boundary-many"
  PARAMS input/param/small-flat.txt FLAGS input/flags/pboundary.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/manyboundary.txt
  EXPECT 0
  GEOM_DUMP_FILES UNIFY data/bianry_data/many_boundary.single.dat)

add_jupiter_input_test(NAME "boundary-io-nompi"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  RESTART EXPECT 0)

add_jupiter_input_test(NAME "boundary-io-mpi"
  PARAMS input/param/small-mpi.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  RESTART MPI_NP 2 EXPECT 0)

add_jupiter_input_test(NAME "geom-init-binary"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  RESTART RESTART_GEOMETRY input/geom/binary.txt
  RESTART_PRE_RUN "file(REMOVE \"data/binary_data/qgeom/0000.dat\")"
  EXPECT 0)

add_jupiter_input_test(NAME "geom-init-poly10"
  PARAMS input/param/small.txt FLAGS input/flags/double-output.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/init_poly.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-init-poly-n"
  PARAMS input/param/small.txt FLAGS input/flags/double-output.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/init_poly_n.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-init-exp-poly"
  PARAMS input/param/small.txt FLAGS input/flags/double-output.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/init_exp_poly.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-init-vortex-uvw"
  PARAMS input/param/small.txt FLAGS input/flags/double-output.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/init_vort_uvw.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-comb"
  PARAMS input/param/geom_comb.txt FLAGS input/flags/geom-test.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/geom_comb.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-rep"
  PARAMS input/param/geom_comb.txt FLAGS input/flags/geom-test.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/geom_rep.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-errors"
  PARAMS input/param/geom_comb.txt FLAGS input/flags/geom-test.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/geom_errors.txt
  EXPECT 1)

add_jupiter_input_test(NAME "geom-error-vof-no-solid"
  PARAMS input/param/geom_comb.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/geom_use_solid.txt
  EXPECT 1)

add_jupiter_input_test(NAME "geom-surface-shape-1"
  PARAMS input/param/geom_comb.txt FLAGS input/flags/surf-bnd.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/geom_surface_shape_1.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-surface-shape-2"
  PARAMS input/param/geom_comb.txt FLAGS input/flags/surf-bnd.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/geom_surface_shape_2.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-surface-shape-restart-1"
  PARAMS input/param/geom_comb.txt FLAGS input/flags/surf-bnd.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/geom_surface_shape_1.txt
  RESTART EXPECT 0)

add_jupiter_input_test(NAME "geom-surface-shape-restart-2"
  PARAMS input/param/geom_comb.txt FLAGS input/flags/surf-bnd.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/geom_surface_shape_2.txt
  RESTART EXPECT 0)

add_jupiter_input_test(NAME "geom-vof-reinit"
  PARAMS input/param/hsrc_reinit.txt FLAGS input/flags/vof_reinit.txt
  PLIST input/plist.darcy2.txt.in  GEOMETRY input/geom/small.txt
  RESTART RESTART_GEOMETRY input/geom/reinit-test.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-velu-reinit"
  PARAMS input/param/hsrc_reinit.txt FLAGS input/flags/velu_reinit.txt
  PLIST input/plist.darcy2.txt.in  GEOMETRY input/geom/small.txt
  RESTART RESTART_GEOMETRY input/geom/reinit-test.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-velv-reinit"
  PARAMS input/param/hsrc_reinit.txt FLAGS input/flags/velv_reinit.txt
  PLIST input/plist.darcy2.txt.in  GEOMETRY input/geom/small.txt
  RESTART RESTART_GEOMETRY input/geom/reinit-test.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-velw-reinit"
  PARAMS input/param/hsrc_reinit.txt FLAGS input/flags/velw_reinit.txt
  PLIST input/plist.darcy2.txt.in  GEOMETRY input/geom/small.txt
  RESTART RESTART_GEOMETRY input/geom/reinit-test.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-temp-reinit"
  PARAMS input/param/hsrc_reinit.txt FLAGS input/flags/temp_reinit.txt
  PLIST input/plist.darcy2.txt.in  GEOMETRY input/geom/small.txt
  RESTART RESTART_GEOMETRY input/geom/reinit-test.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-pres-reinit"
  PARAMS input/param/hsrc_reinit.txt FLAGS input/flags/pres_reinit.txt
  PLIST input/plist.darcy2.txt.in  GEOMETRY input/geom/small.txt
  RESTART RESTART_GEOMETRY input/geom/reinit-test.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-fixed-heat-source-reinit"
  PARAMS input/param/hsrc_reinit.txt FLAGS input/flags/hsrc_reinit.txt
  PLIST input/plist.darcy2.txt.in  GEOMETRY input/geom/small.txt
  RESTART RESTART_GEOMETRY input/geom/reinit-test.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-boundary-reinit"
  PARAMS input/param/hsrc_reinit.txt FLAGS input/flags/bnd_reinit.txt
  PLIST input/plist.darcy2.txt.in  GEOMETRY input/geom/small.txt
  RESTART RESTART_GEOMETRY input/geom/reinit-test.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-fluid-boundary-reinit"
  PARAMS input/param/hsrc_reinit.txt FLAGS input/flags/fbnd_reinit.txt
  PLIST input/plist.darcy2.txt.in  GEOMETRY input/geom/small.txt
  RESTART RESTART_GEOMETRY input/geom/reinit-test.txt
  EXPECT 0)

add_jupiter_input_test(NAME "geom-thermal-boundary-reinit"
  PARAMS input/param/hsrc_reinit.txt FLAGS input/flags/tbnd_reinit.txt
  PLIST input/plist.darcy2.txt.in  GEOMETRY input/geom/small.txt
  RESTART RESTART_GEOMETRY input/geom/reinit-test.txt
  EXPECT 0)

if(JUPITER_ENABLE_METI)
  add_jupiter_input_test(NAME "geom-error-vof-must-gas"
    PARAMS input/param/cavity.txt FLAGS input/flags/solute-diff-cavity.txt
    PLIST input/plist/solute-cavity.txt.in GEOMETRY input/geom/geom_use_gas.txt
    EXPECT 1)
endif()

add_jupiter_input_test(NAME "geom-issue-113"
  PARAMS input/param/geom_comb.txt FLAGS input/flags/geom-test.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/geom_issue_113.txt
  EXPECT 1)

add_jupiter_shape_test(NAME "geom-transform"
  NCELL 10 10 10 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION transform-1
  SHAPE SET BOX 1.0 1.0 1.0 2.0 2.0 2.0
  SHAPE SET TRA 0.0 0.0 -1.0 0
  SHAPE ADD BOX 0.0 0.0 0.0 1.0 1.0 1.0
  SHAPE SET TRA 1.0 1.0 3.0 0
  SHAPE ADD TRA 2.0 2.0 0.0 2
  SHAPE ADD TRA 0.0 0.0 2.0 1
  SHAPE PUSH BOX 6.0 6.0 6.0 7.0 7.0 7.0
  SHAPE ADD COMB
  SHAPE ADD TRA 0.0 0.0 2.0 1
  SHAPE SET GST
  SHAPE ADD BOX 2.0 2.0 5.0 3.0 3.0 6.0
  SHAPE ADD BOX 4.0 2.0 5.0 5.0 3.0 6.0
  SHAPE PUSH BOX 2.0 2.0 7.0 5.0 5.0 8.0
  SHAPE SUB BOX 3.0 3.0 7.0 4.0 4.0 8.0
  SHAPE SET GED
  SHAPE ADD TRA 0.0 0.0 2.0 1
  SHAPE ADD COMB
  SHAPE_DEFINITION transform-2
  SHAPE SET GST
  SHAPE SET BOX 2.0 2.0 5.0 3.0 3.0 6.0
  SHAPE ADD BOX 3.0 3.0 6.0 4.0 4.0 7.0
  SHAPE SET GED
  SHAPE ADD TRA 0.0 0.0 2.0 1
  SHAPE ADD TRA 0.0 2.0 0.0 1
  SHAPE SET SCA 3.0 3.0 6.0 2.0 2.0 1.0, 0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-box"
  NCELL 10 10 10 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "box-1"
  SHAPE SET BOX 1.0 1.0 1.0 5.0 5.0 5.0
  SHAPE ADD BOX 7.5 7.5 5.0 9.5 9.5 10.0
  SHAPE ADD BOX 6.0 6.0 4.0 7.0 7.0 5.0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-pla"
  NCELL 10 10 10 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "pla-1"
  SHAPE SET PLA 5.0 5.0 5.0 -1.0 1.0 1.0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-app"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "app-1"
  SHAPE SET APP 2.0 2.0 2.0 5.0 1.0 1.0 -1.0 3.0 -1.0 2.0 3.0 5.0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-wed"
  NCELL 16 16 16 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "wed-1"
  SHAPE SET WED 8.0 2.0 2.0 6.0 8.0 2.0 2.0 5.0 5.0
                8.0 2.0 7.0 6.0 8.0 6.0 2.0 5.0 8.0
  SHAPE_DEFINITION "wed-2"
  SHAPE SET WED 2.0 2.0 8.0 8.0 2.0 6.0 5.0 5.0 2.0
                2.0 7.0 8.0 8.0 6.0 6.0 5.0 8.0 2.0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-arb"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "arb-1"
  SHAPE SET ARB 7.65772 1.90848 2.14460 7.65772 7.56506 2.14460
                2.00262 7.23950 2.58988 2.00262 1.99938 2.58988
                6.92322 2.36122 4.36240 6.92322 5.94358 4.36240
                2.88038 3.95649 7.08029 2.88038 2.91606 7.08029
  SHAPE_DEFINITION "arb-non-convex"
  SHAPE SET ARB 7.65772 1.90848 2.14460 7.65772 7.56506 2.14460
                2.00262 7.23950 2.58988 2.00262 1.99938 2.58988
                4.12288 2.36122 4.36240 4.12288 5.94358 4.36240
                2.88038 3.95649 7.08029 2.88038 2.91606 7.08029
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-rpr"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "rpr-1"
  SHAPE SET RPR 5.0 5.0 1.0 3.0 0.0 0.0 0.0 0.0 7.0 3
  SHAPE_DEFINITION "rpr-3"
  SHAPE SET RPR 5.0 5.0 1.0 0.0 2.0 2.0 0.0 0.0 7.0 3
  SHAPE_DEFINITION "rpr-4"
  SHAPE SET RPR 1.0 5.0 5.0 0.0 0.0 3.0 7.0 0.0 0.0 5
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-trp"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "trp-1"
  SHAPE SET TRP 5.0 5.0 1.0 3.0 0.0 0.0 0.0 0.0 7.0 0.0 3
  SHAPE_DEFINITION "trp-3"
  SHAPE SET TRP 5.0 5.0 1.0 0.0 2.0 2.0 0.0 0.0 7.0 0.5 3
  SHAPE_DEFINITION "trp-4"
  SHAPE SET TRP 1.0 5.0 5.0 0.0 0.0 3.0 7.0 0.0 0.0 0.75 5
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-sph"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "sph-1"
  SHAPE SET SPH 5.0 5.0 5.0 4.0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-rcc"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "rcc-1"
  SHAPE SET RCC 5.0 5.0 2.0 0.0 0.0 6.0 3.0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-ell"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "ell-1"
  SHAPE SET ELL 5.0 5.0 5.0 2.13325 2.20559 2.88257
   -0.47933 2.43851 1.21593 1.68444 1.67958 0.31896
  SHAPE_DEFINITION "ell-2"
  SHAPE SET ELL 5.0 5.0 5.0 4.0 0.0 0.0 0.0 2.0 0.0 0.0 0.0 3.0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-trc"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "trc-1"
  SHAPE SET TRC 5.0 5.0 2.0 0.0 0.0 6.0 3.0 0.0
  SHAPE_DEFINITION "trc-2"
  SHAPE SET TRC 5.0 3.0 3.0 2.0 4.0 4.0 3.0 0.5
  SHAPE_DEFINITION "trc-3"
  SHAPE SET TRC 2.0 5.0 5.0 6.0 0.0 0.0 1.0 3.0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-rec"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "rec-1"
  SHAPE SET REC 5.0 5.0 2.0 0.0 0.0 6.0 2.0 0.0 0.0 4.0
  SHAPE_DEFINITION "rec-2"
  SHAPE SET REC 5.0 3.0 3.0 2.0 4.0 4.0 2.0 0.0 0.0 4.0
  SHAPE_DEFINITION "rec-3"
  SHAPE SET REC 2.0 5.0 5.0 6.0 0.0 0.0 0.0 1.0 0.0 3.0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-tec"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "tec-1"
  SHAPE SET TEC 5.0 5.0 2.0 0.0 0.0 6.0 2.0 0.0 0.0 4.0 0.0 0.0
  SHAPE_DEFINITION "tec-2"
  SHAPE SET TEC 5.0 3.0 3.0 2.0 4.0 4.0 2.0 0.0 0.0 4.0 0.5 0.5
  SHAPE_DEFINITION "tec-3"
  SHAPE SET TEC 2.0 5.0 5.0 6.0 0.0 0.0 0.0 1.0 0.0 1.0 2.0 3.0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-tor"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "tor-ring-1"
  SHAPE SET TOR 5.0 5.0 5.0 0.0 0.0 1.0 3.0 1.0
  SHAPE_DEFINITION "tor-ring-2"
  SHAPE SET TOR 5.0 5.0 5.0 2.0 4.0 4.0 3.0 1.0
  SHAPE_DEFINITION "tor-horn-1"
  SHAPE SET TOR 5.0 5.0 5.0 1.0 0.0 0.0 2.0 2.0
  SHAPE_DEFINITION "tor-spindle-1"
  SHAPE SET TOR 5.0 5.0 5.0 0.0 1.0 0.0 2.0 2.5
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-eto"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "eto-ring-1"
  SHAPE SET ETO 5.0 5.0 5.0 0.0 0.0 1.0 3.0 1.0 0.0 0.0 2.0
  SHAPE_DEFINITION "eto-ring-2"
  SHAPE SET ETO 5.0 5.0 5.0 2.0 4.0 4.0 3.0 1.0 0.0 0.0 2.0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-pl1"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "pl1-1"
  SHAPE SET PL1 1.0 1.0 1.0 -8.0 1.0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-pl2"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "pl2-1"
  SHAPE SET PL2 36.0 64.0 -144.0 0.0 0.0 0.0 0.0 0.0 0.0 -576.0 1.0
  SHAPE SET TRA 5.0 5.0 5.0 0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-pl3"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "pl3-1"
  SHAPE SET PL3 1.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0
                3.0 0.0 0.0 0.0 0.0 3.0 0.0 1.0
  SHAPE SET TRA 5.0 5.0 5.0 0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-pl4"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "pl4-1"
  SHAPE SET PL4 1.0 1.0 1.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0
                0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0
                0.0 0.0 0.0 0.0 0.0 0.0 0.0 0.0 -256.0 1.0
  SHAPE SET TRA 5.0 5.0 5.0 0
  SHAPE SUB TRA 2.5 2.5 2.5 1
  EXPECT 0)

# ref: http://news.livedoor.com/article/detail/14102722/
# sa:  https://twitter.com/CHARTMANq/status/946000672746487808
add_jupiter_shape_test(NAME "geom-shape-pln"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "pln-1"
  SHAPE SET PLN 20 1.0
   0 16  4 1.0       2 14  4 8.0       4 12  4 28.0      6 10  4 56.0
   8  8  4 70.0     10  6  4 56.0     12  4  4 28.0     14  2  4 8.0
  16  0  4 1.0       0 18  2 2.0       2 16  2 18.0      0 16  2 112.0
   4 14  2 72.0      2 14  2 992.0     6 12  2 168.0     4 12  2 3456.0
   8 10  2 252.0     6 10  2 6688.0   10  8  2 252.0     8  8  2 8224.0
  12  6  2 160.0    10  6  2 6688.0   14  4  2 72.0     12  4  2 3456.0
  16  2  2 18.0     14  2  2 992.0    18  0  2 2.0      16  0  2 112.0
   0 20  0 1.0       2 18  0 10.0      0 18  0 -113.0    4 16  0 45.0
   2 16  0 -1161.0   0 16  0 3136.0    6 14  0 120.0     4 14  0 -5396.0
   2 14  0 30464.0   8 12  0 210.0     6 12  0 -14116.0  4 12  0 108032.0
  10 10  0 252.0     8 10  0 -22478.0  6 10  0 195840.0 12  8  0 210.0
  10  8  0 -22478.0  8  8  0 246656.0 14  6  0 120.0    12  6  0 -14116.0
  10  6  0 195840.0 16  4  0 45.0     14  4  0 -5396.0  12  4  0 108032.0
  18  2  0 10.0     16  2  0 -1161.0  14  2  0 30464.0  20  0  0 1.0
  18  0  0 -113.0   16  0  0 3136.0   -1
  SHAPE SET SCA 0.0 0.0 0.0 0.4 0.4 0.4 0
  SHAPE SET TRA 5.0 5.0 5.0 0
  SHAPE ADD PLN 20 1.0
   0 16  4 1.0       2 14  4 8.0       4 12  4 28.0      6 10  4 56.0
   8  8  4 70.0     10  6  4 56.0     12  4  4 28.0     14  2  4 8.0
  16  0  4 1.0       0 18  2 2.0       2 16  2 18.0      0 16  2 120.0
   4 14  2 72.0      2 14  2 992.0     6 12  2 168.0     4 12  2 3296.0
   8 10  2 252.0     6 10  2 6176.0   10  8  2 252.0     8  8  2 7504.0
  12  6  2 168.0    10  6  2 6176.0   14  4  2 72.0     12  4  2 3296.0
  16  2  2 18.0     14  2  2 992.0    18  0  2 2.0      16  0  2 120.0
   0 20  0 1.0       2 18  0 10.0      0 18  0 -169.0    4 16  0 45.0
   2 16  0 -1217.0   0 16  0 3600.0    6 14  0 120.0     4 14  0 -4276.0
   2 14  0 30720.0   8 12  0 210.0     6 12  0 -9412.0   4 12  0 97216.0
  10 10  0 252.0     8 10  0 -13854.0  6 10  0 165888.0 12  8  0 210.0
  10  8  0 -13854.0  8  8  0 207968.0 14  6  0 120.0    12  6  0 -9412.0
  10  6  0 165888.0 16  4  0 45.0     14  4  0 -4276.0  12  4  0 97216.0
  18  2  0 10.0     16  2  0 -1217.0  14  2  0 30720.0  20  0  0 1.0
  18  0  0 -169.0   16  0  0 3600.0   -1
  SHAPE SET SCA 0.0 0.0 0.0 0.4 0.4 0.4 0
  SHAPE SET TRA 5.0 5.0 5.0 0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-rot"
  NCELL 20 20 20 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "rot-1"
  SHAPE SET BOX -3.0 -3.0 -3.0 3.0 3.0 3.0
  SHAPE SET ROT 0.0 0.0 0.0 0.0 0.0 1.0 30.0 0
  SHAPE SET TRA 5.0 5.0 5.0 0
  SHAPE SUB BOX 3.0 3.0 2.0 7.0 7.0 8.0
  SHAPE SET ROT 5.0 5.0 5.0 0.0 0.0 1.0 45.0 0
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-sca"
  NCELL 10 10 10 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "basic-scale"
  SHAPE SET BOX 0.0 0.0 0.0 2.0 2.0 2.0
  SHAPE SET SCA 0.0 0.0 0.0 0.5 4.0 2.0 0
  SHAPE_DEFINITION "scale-on-distant-point"
  SHAPE SET BOX 5.0 3.5 5.0 6.0 7.5 6.0
  SHAPE SET SCA 5.0 5.5 5.5 2.0 0.5 3.0 0
  # Add more cases.
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-mat"
  NCELL 10 10 10 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "mat-skew"
  SHAPE SET BOX -2.0 -2.0 -2.0 2.0 2.0 2.0
  SHAPE SET MAT 1.0 1.0 0.0 0.0
                0.0 1.0 0.0 0.0
                0.0 0.0 1.0 0.0 0
  SHAPE SET TRA 5.0 5.0 5.0 0
  # Add more cases.
  EXPECT 0)

add_jupiter_shape_test(NAME "geom-shape-sus-1"
  NCELL 10 10 10 LENGTH 10.0 10.0 10.0
  SHAPE_DEFINITIONS
  SHAPE_DEFINITION "bug1"
  SHAPE SET SPH 5.0 5.0 5.0 2.0
  SHAPE ADD TRP 3.0 3.0 3.0 2.0 2.0 0.0 0.0 0.0 2.0 0.0 4
  SHAPE ADD SCA 3.0 3.0 3.0 -1.0 1.0 1.0 1
  EXPECT 0)

### GitHub issues

add_jupiter_input_test(NAME "issue-73"
  PARAMS input/param/issue-73.txt FLAGS input/flags/issue-73.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0)

if (JUPITER_ENABLE_METI)
  add_jupiter_input_test(NAME "issue-154"
    PARAMS input/param/solute-diff.txt FLAGS input/flags/solute-diff.txt
    PLIST input/plist/solute-diff.txt.in GEOMETRY input/geom/issue-154.txt
    EXPECT 0)
endif()

if (JUPITER_ENABLE_METI)
  add_jupiter_input_test(NAME "issue-287"
    PARAMS input/param/sdiff_phase_change_melt-r.txt
    FLAGS input/flags/issue-287.txt
    PLIST input/plist/solute-diff-0-mushy.txt.in
    GEOMETRY input/geom/solute-diff-m.txt
    RESTART_N 2
    RESTART RESTART_PARAMS input/param/sdiff_phase_change_melt.txt EXPECT 0
    BINARY_OUTPUT_DOUBLE ON)

  # The input file in question is inalid (no oxide_zircaloy/zro2).
  add_jupiter_input_test(NAME "issue-436"
    PARAMS input/param/issue-436.txt FLAGS input/flags/issue-436.txt
    PLIST input/plist/issue-436.txt.in GEOMETRY input/geom/small.txt
    EXPECT 1 EXPECT_DUMPMESH 1)
endif()

add_jupiter_input_test(NAME "issue-298"
  PARAMS input/param/small.txt
  FLAGS input/flags/ibm-dyn-heat.txt
  PLIST input/plist/lpt-tests.0.txt.in
  GEOMETRY input/geom/lpt-test-pewall-1.txt
  CONTROL input/control/lpt-test-pewall-1.txt EXPECT 0)

add_jupiter_input_test(NAME "issue-301"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/issue-301.txt
  RESTART EXPECT 0)

add_jupiter_input_test(NAME "issue-303"
  PARAMS input/param/small.txt FLAGS input/flags/issue-303.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0)

add_jupiter_input_test(NAME "issue-289-fs"
  PARAMS input/param/issue-289.txt FLAGS input/flags/issue-289.txt
  PLIST input/plist.darcy3.txt.in  GEOMETRY input/geom/darcy-3.txt
  PRE_RUN "file(MAKE_DIRECTORY \"data/binary_data/fs_ibm/0000.0000.single.dat\")"
  EXPECT 1)

add_jupiter_input_test(NAME "issue-289-ls"
  PARAMS input/param/issue-289.txt FLAGS input/flags/issue-289.txt
  PLIST input/plist.darcy3.txt.in  GEOMETRY input/geom/darcy-3.txt
  PRE_RUN "file(MAKE_DIRECTORY \"data/binary_data/ls_ibm/0000.0000.single.dat\")"
  EXPECT 1)

add_jupiter_input_test(NAME "issue-314"
  PARAMS input/param/issue-314.txt FLAGS input/flags/geom-test.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/issue-314.txt
  EXPECT 0)

if (JUPITER_ENABLE_METI)
  add_jupiter_input_test(NAME "issue-318-s"
    PARAMS input/param/issue-318-s.txt FLAGS input/flags/solute-diff.txt
    PLIST input/plist/solute-cavity.txt.in GEOMETRY input/geom/issue-318.txt
    EXPECT 0)
endif()

add_jupiter_input_test(NAME "issue-318-m"
  PARAMS input/param/issue-318-m.txt FLAGS input/flags/ibm-heat.txt
  PLIST input/plist.5.txt.in GEOMETRY input/geom/issue-318.txt
  EXPECT 0)

add_jupiter_input_test(NAME "issue-323-1p"
  PARAMS input/param/issue-323-1p.txt FLAGS input/flags/ibm-dyn-heat.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/issue-323.txt
  EXPECT 0)

add_jupiter_input_test(NAME "issue-323-2p"
  PARAMS input/param/issue-323-2p.txt FLAGS input/flags/ibm-dyn-heat.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/issue-323.txt
  MPI_NP 2 EXPECT 0)

add_jupiter_input_test(NAME "issue-387g"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/issue-387.txt
  EXPECT 0)

add_jupiter_input_test(NAME "issue-387c"
  PARAMS input/param/small.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  CONTROL input/geom/issue-387.txt EXPECT 0)

add_jupiter_input_test(NAME "issue-544"
  PARAMS input/param/small.txt FLAGS input/flags/ibm-heat.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/issue-544.txt
  EXPECT 0)

if (JUPITER_ENABLE_LPT STREQUAL "LPTX")
  add_jupiter_input_test(NAME "issue-554-lptx"
    PARAMS input/param/lpt-test-issue-554.txt
    FLAGS input/flags/lpt-only-br.txt
    PLIST input/plist/lpt-tests.0.txt.in
    GEOMETRY input/geom/lpt-case-br.txt
    RESTART RESTART_PARAMS input/param/lpt-test-issue-554r.txt
    EXPECT 0)
endif()

add_jupiter_input_test(NAME "issue-123-m"
  PARAMS input/param/issue-123-m.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 1 EXPECT_DUMPMESH 1)

add_jupiter_input_test(NAME "issue-123-n"
  PARAMS input/param/issue-123-n.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 1 EXPECT_DUMPMESH 1)

# Currently default value of Output is 0, so same result as issue-123-o0.
add_jupiter_input_test(NAME "issue-123-o"
  PARAMS input/param/issue-123-o.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0)

add_jupiter_input_test(NAME "issue-123-o0"
  PARAMS input/param/issue-123-o0.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0)

add_jupiter_input_test(NAME "issue-123-t"
  PARAMS input/param/issue-123-t.txt FLAGS input/flags/small.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0)

add_jupiter_input_test(NAME "issue-553"
  PARAMS input/param/issue-553.txt FLAGS input/flags/issue-553.txt
  PLIST input/plist.3.txt.in GEOMETRY input/geom/issue-553.txt
  RESTART EXPECT 0 REQUIRES ANY_LPT)

# LPT_particle_set only, not modifying restart
add_jupiter_input_test(NAME "issue-538-n"
  PARAMS input/param/issue-538-n.txt
  FLAGS input/flags/lpt-only.txt
  PLIST input/plist/lpt-tests.0.txt.in
  GEOMETRY input/geom/lpt-case-1-1.txt
  RESTART EXPECT 0 REQUIRES ANY_LPT)

# LPT_particle_set only, add and modify on restart
add_jupiter_input_test(NAME "issue-538-na"
  PARAMS input/param/issue-538-n.txt
  FLAGS input/flags/lpt-only.txt
  PLIST input/plist/lpt-tests.0.txt.in
  GEOMETRY input/geom/lpt-case-1-1.txt
  RESTART_PARAMS input/param/issue-538-a.txt
  RESTART EXPECT 0 REQUIRES ANY_LPT)

# no particles with LPT enabled
add_jupiter_input_test(NAME "issue-538-z"
  PARAMS input/param/issue-538-z.txt
  FLAGS input/flags/lpt-only.txt
  PLIST input/plist/lpt-tests.0.txt.in
  GEOMETRY input/geom/lpt-case-1-1.txt
  RESTART EXPECT 0 REQUIRES ANY_LPT)

# LPT_particle_set only, add and modify on restart from 0
add_jupiter_input_test(NAME "issue-538-za"
  PARAMS input/param/issue-538-z.txt
  FLAGS input/flags/lpt-only.txt
  PLIST input/plist/lpt-tests.0.txt.in
  GEOMETRY input/geom/lpt-case-1-1.txt
  RESTART_PARAMS input/param/issue-538-a.txt
  RESTART EXPECT 0 REQUIRES ANY_LPT)

add_jupiter_input_test(NAME "issue-623"
  PARAMS input/param/issue-623.txt
  FLAGS input/flags/issue-623.txt
  PLIST input/plist/issue-623.txt.in
  GEOMETRY input/geom/issue-623.txt
  MPI_NP 2
  OpenMP_NT 1
  EXPECT 0)

add_jupiter_input_test(NAME "issue-602"
  PARAMS input/param/small-mpi.txt FLAGS input/flags/issue-602.txt
  PLIST input/plist.3.txt.in  GEOMETRY input/geom/small.txt
  EXPECT 0 MPI_NP 2
  MKVTK_CONFIGS ${JUPITER_INPUT_TEST_DEFAULT_MKVTK_CONFIGS}
                MKVTK_RESTART MKVTKU_RESTART MKXDMF_RESTART)
