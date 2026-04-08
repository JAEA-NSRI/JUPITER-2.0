### This makefile is only for a certain environment, and cannot be
### used widely. Use CMake instead.

# Environment modules to be loaded while build
USE_MODULES = pgi/20.9 openmpi/4.0.5_pgi cuda/10.1

# Extra environments should be assignd, modified or removed after
# loading modules
#
# These contents are assigned via `env` command.
#
# You can use `-u` to remove given variable (ex. `-u CC` to remove
# environment variable `CC`).
#
# You have to use `$$` for `$` to refer shell variables.
#
# If `USE_MODULES` is empty, these variables will not be applied.
EXTRA_ENVIRONMENTS = CUDA_HOME=$${CUDA_PATH}

### * For single-precision, remove `JUPITER_DOUBLE` from `SCHEME` and *add*
###   `JUPITER_LPT_API_SINGLE` to `LPT_SCHEME`.
###
SCHEME += -DJUPITER_MPI -DJUPITER_DOUBLE #-Dtest # -D_TIME_
LPT_SCHEME += -DJUPITER_LPT_USE_MPI # -DJUPITER_LPT_API_SINGLE

### YSE: Add re2c to create C source from Re source
RE2C = $(shell which re2c 2>/dev/null)
### -s: Use `if` statement and compare with greater/less instead of `switch`,
###     if possible.
###     (note: this flag will improve speed, but this requires unsigned type)
RE2CFLAGS = --no-generation-date -i -s
### Debug option...
# RE2CFLAGS = -d
### YSE Add end

### YSE: Extensions
LIBEXT = .a
EXEEXT =

BX = -DBX
ICEX = -DICEX

### For non-LPT, comment out or given empty string for variable `LPT`.
LPT = -DLPT $(LPT_SCHEME)

#-- Select Solver
### YSE: AMGS and PETSc options are removed.
SOLVER = -DCCSE -DJUPITER_SOLVER_ACC_CG #-DPW #-DCACG # -DBiCG
##

#-- Compiler
# CC   = mpiicc
# FC   = mpiifort
#####
CC   = pgcc
FC   = pgfortran
RM   = /bin/rm
PROG = run
LIB  = libjupiter.a
AR   = ar
ARFLAGS = cr

MPI_DIR = # ${HOME}/software/openmpi-1.6.5

#-------- Objects
### YSE: Add sources that for input process.
### YSE: Separete main.o for create archive.
OBJS = initial.o thinc.o plic.o phasefield.o level_set.o tvd_runge_kutta.o advection.o      \
       viscosity.o surface.o forch.o divergence_free.o materials.o      \
       lpt.o phase_change.o heat_source.o radiation.o IBM.o             \
       dt_control.o boundary.o set_param.o param.o io_process.o         \
       communication.o physical_properties.o analysis_cut.o cacg.o      \
       alloc_variable.o solute_transport.o oxidation.o ccse_poisson.o   \
       pbicg.o pcg.o support.o sleev_comm.o csv.o csvutil.o             \
	pcg_acc.o multi_layer.o \
       csvutil_extra.o boundary_util.o print_param.o optparse.o         \
       csvtmpl_format.o tempdep_properties.o field_control.o            \
       if_binary.o fv_time.o posix/os.o

M_ARCH = $(shell uname -m)

MPIMODDIR_CANDIDATES=$(subst :, ,${LIBRARY_PATH})
MPIMOD=$(wildcard $(patsubst %,%/mpi.mod,$(MPIMODDIR_CANDIDATES)))
MPIMODDIR=$(dir $(word 1,$(MPIMOD)))

# OMPFLAGS = -qopenmp
OMPFLAGS = -mp
ACCFLAGS = -acc -ta=tesla:cc70 -Minfo=accel -Minfo -acc=norequired 
# CFLAGS = -std=gnu99 -O3 -xCORE-AVX2 $(OMPFLAGS) #-no-ip -no-ipo -fno-inline -fno-inline-functions
# FCFLAGS = -cpp -O3 -xCORE-AVX2 $(OMPFLAGS)
CFLAGS = -fpic -fast -Mpreprocess -Mnofpapprox $(ACCFLAGS) $(OMPFLAGS) 
# CFLAGS = -fpic -fast -Mpreprocess -Mnofpapprox  $(OMPFLAGS) 
FCFLAGS = -fast -Mpreprocess $(OMPFLAGS) -I$(MPIMODDIR)
INCLUDES = -I.
JUPITER_DEFS  = ${SCHEME}
JUPITER_DEFS += ${BX}
JUPITER_DEFS += ${ICEX}
JUPITER_DEFS += ${SOLVER}
JUPITER_DEFS += ${LPT}

# Library names are from OpenMPI.
LDFLAGS  = $(OMPFLAGS) $(ACCFLAGS) $(patsubst %,-L%,$(subst :, ,${LIBRARY_PATH})) -lmpi -lmpi_mpifh

JUPITER_LIBS = table/libjupiter-table$(LIBEXT)	\
               geometry/libjupiter-geometry$(LIBEXT) \
               serializer/libjupiter-serializer$(LIBEXT) \
               control/libjupiter-control$(LIBEXT)
ifneq ($(LPT),)
JUPITER_LIBS += lpt/libjupiter-lpt$(LIBEXT)
LDFLAGS +=
LD = $(CC) -fortranlibs
else
LDFLAGS += -lm
LD = $(CC)
LD += $(CFLAGS)
endif

# Target List in the table directory.
TABLE_TARGETS = table/libjupiter-table$(LIBEXT) table/csv2table$(EXEEXT) \
                table/libcsv2table$(LIBEXT)

# Target List in the geometry directory
GEOMETRY_TARGETS = geometry/libjupiter-geometry$(LIBEXT)

# Target List in the control directory
CONTROL_TARGETS = control/libjupiter-control$(LIBEXT)

# Target List in the serializer directory
SERIALIZER_TARGETS = serializer/libjupiter-serializer$(LIBEXT)

# Target List in the lpt direcotry
LPT_TARGETS = lpt/libjupiter-lpt$(LIBEXT)

### YSE: Add debug option
# Comment out for debug (debug mode enables assertion test
#                        and make DEBUG messages will be written).
CFLAGS += -DNDEBUG
# Uncomment to emit CSV raw parsed data and re2c state.
# CFLAGS += -DCSV_DEBUG

### YSE: These targets should be always executed when called.
.PHONY: all clean show-env cd $(JUPITER_LIBS)

.PRECIOUS: %.c

# You can bypass auto module loader with `make MODULE_LOADED=1`, but
# recommend to use `make USE_MODULES=` instead.

ifeq ($(MODULE_LOADED), 1)
all : ${PROG}
else
all :
	$(MAKE) all-with-module
endif

ifeq ($(MODULE_LOADED), 1)
${LIB}: ${OBJS}
	${AR} ${ARFLAGS} $@ ${OBJS}
else
.PHONY: ${LIB}
${LIB}:
	$(MAKE) $@-with-module
endif

ifeq ($(MODULE_LOADED), 1)
${PROG} : main.o ${LIB} ${JUPITER_LIBS}
	${LD} ${LDFLAGS} -o $@ main.o ${LIB} ${LDFLAGS} ${JUPITER_LIBS}
else
.PHONY: ${PROG}
${PROG} :
	$(MAKE) $@-with-module
endif

### YSE: Target for handling .re files.
ifneq ($(RE2C), )
%.c : %.c.re
ifeq ($(MODULE_LOADED), 1)
	$(RE2C) $(RE2CFLAGS) -o $@ $<
else
	$(MAKE) $@-with-module
endif

%.h : %.h.re
ifeq ($(MODULE_LOADED), 1)
	$(RE2C) $(RE2CFLAGS) -o $@ $<
else
	$(MAKE) $@-with-module
endif
endif

%.o : %.c
ifeq ($(MODULE_LOADED), 1)
	${CC} ${CFLAGS} ${INCLUDES} ${JUPITER_DEFS} -c -o $@ $<
else
	$(MAKE) $@-with-module
endif

$(TABLE_TARGETS): table/%:
ifeq ($(MODULE_LOADED), 1)
	${MAKE} ${MFLAGS} CC=${CC} CFLAGS="${CFLAGS}" RE2C=${RE2C} RE2CFLAGS="${RE2CFLAGS}" TABLE_USE_OPENMP=1 -C table $(subst table/,,$@)
else
	$(MAKE) $@-with-module
endif

$(GEOMETRY_TARGETS): geometry/%:
ifeq ($(MODULE_LOADED), 1)
	${MAKE} ${MFLAGS} CC=${CC} CFLAGS="${CFLAGS}" RE2C=${RE2C} RE2CFLAGS="${RE2CFLAGS}" GEOMETRY_USE_OPENMP=1 -C geometry $(subst geometry/,,$@)
else
	$(MAKE) $@-with-module
endif

$(CONTROL_TARGETS): control/%:
ifeq ($(MODULE_LOADED), 1)
	${MAKE} ${MFLAGS} CC=${CC} CFLAGS="${CFLAGS}" RE2C=${RE2C} RE2CFLAGS="${RE2CFLAGS}" JCNTRL_USE_OPENMP=1 -C control $(subst control/,,$@)
else
	$(MAKE) $@-with-module
endif

$(SERIALIZER_TARGETS): serializer/%:
ifeq ($(MODULE_LOADED), 1)
	${MAKE} ${MFLAGS} CC=${CC} CFLAGS="${CFLAGS}" RE2C=${RE2C} RE2CFLAGS="${RE2CFLAGS}" SERIALIZER_USE_OPENMP=1 SERIALIZER_USE_MPI=1 -C serializer $(subst serializer/,,$@)
else
	$(MAKE) $@-with-module
endif

$(LPT_TARGETS): lpt/%:
ifeq ($(MODULE_LOADED), 1)
	${MAKE} ${MFLAGS} CC=${CC} CFLAGS="${CFLAGS}" FC=${FC} FCFLAGS="${FCFLAGS}" LPT_SCHEME=$(LPT_SCHEME) -C lpt $(subst lpt/,,$@)
else
	$(MAKE) $@-with-module
endif

clean :
	${MAKE} ${MFLAGS} -C table clean
	${MAKE} ${MFLAGS} -C geometry clean
	${MAKE} ${MFLAGS} -C control clean
	${MAKE} ${MFLAGS} -C serializer clean
	${MAKE} ${MFLAGS} -C lpt clean
	${RM} -f ${PROG} ${LIB} ${OBJS}  *.o core *.bmp *.linkinfo *.vtk

cd:
	${RM} -rf  ./data/binary_data/*.dat  \
	${RM} -rf  ./data/1D_data/*.dat \
	${RM} -rf  ./data/2D_data/*.dat \
	${RM} -rf  ./data/*.vtk \
	${RM} -rf  ./stat/*.dat \
	${RM} -rf  ./gnuplot/data/dens/*.dat \
	${RM} -rf  ./gnuplot/data/v/*.dat \
	${RM} -rf  ./gnuplot/data/t/*.dat \
	${RM} -rf  ./gnuplot/data/fs/*.dat \
	${RM} -rf  ./gnuplot/data/fl/*.dat \
	${RM} -rf  ./gnuplot/data/ls/*.dat \
	${RM} -rf  ./gnuplot/data/ll/*.dat \
	${RM} -rf  ./gnuplot/data/time/*.dat \
	${RM} -rf  ./gnuplot/data/surf_temp/*.dat \
	${RM} -rf  ./gnuplot/data/surf_temp/*.gp  \
	${RM} -rf  ./gnuplot/Dens/figure_eps/*.eps \
	${RM} -rf  ./gnuplot/Dens/figure_png/*.png \
	${RM} -rf  ./gnuplot/TempVel/figure_eps/*.eps \
	${RM} -rf  ./gnuplot/TempVel/figure_png/*.png

csv.o: re2c_lparser.h

ifeq ($(MODULE_LOADED), 1)
show-env:
	@echo "------ Assigned environments ----------------------" && \
	env && \
	echo "---------------------------------------------------"
else
show-env:
	$(MAKE) show-env-with-module
endif

%-with-module:
ifneq ($(USE_MODULES), )
	source /etc/profile.d/modules.sh; \
	module purge && \
	module load $(USE_MODULES) && \
	env $(EXTRA_ENVIRONMENTS) \
	$(MAKE) MODULE_LOADED=1 $(patsubst %-with-module,%,$@)
else
	$(MAKE) MODULE_LOADED=1 $(patsubst %-with-module,%,$@)
endif
