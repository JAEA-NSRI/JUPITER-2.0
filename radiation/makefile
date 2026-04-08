#SCHEME += -DDOUBLE
SCHEME += -DMPI -DDOUBLE #-Dtest # -D_TIME_

AMGS = false # true or false
MAC  = false # true or false

BX = -DBX
ICEX = -DICEX

#-- Selsect Solver
ifeq ($(strip $(AMGS)), true)
SOLVER = -DAMGS
else
SOLVER = -DPETSc
endif
##
SOLVER = -DCCSE #-DCACG # -DBiCG
##

#-- Compiler
CC   = icc
FC   = ifort
RM   = /bin/rm

#COMP = ${CC}
COMP = ${FC}
PROG = rad

MPI_DIR = # ${HOME}/software/openmpi-1.6.5
#----------- PETSc 
ifeq ($(strip $(AMGS)), false)
PETSc_DIR=/home/center/lib/petsc-3.6.1
ifeq ($(strip $(MAC)), true)
PETScARCH  = arch-darwin-c-debug
else
PETScARCH  =.
endif
PETSc_INC = -I${PETSc_DIR}     \
			      -I${PETSc_DIR}/${PETScARCH}/conf \
            -I${PETSc_DIR}/${PETScARCH}/include \
            -I${PETSc_DIR}/include \
            -I/usr/X11/include \
            -I/usr/include
PETSc_LIB = -L${PETSc_DIR}/${PETScARCH}/lib -lpetsc     \
            -lpthread -mkl -ldl \
            /usr/lib64/libX11.so.6
endif

#------- AMGS
#ifeq ($(strip $(AMGS)), true)
#ifeq ($(strip $(MAC)), false)
#AMGS_DIR = /home/g2/a104042/amgs-p-0.17
#AMGS_LIB = ${AMGS_DIR}/amgs_p.a
#else
#AMGS_DIR = /Users/yamas/CODE/amgs-p-0.17
#AMGS_LIB = ${AMGS_DIR}/amgs_p.a
#endif
#endif

#-------- Objects
ifeq ($(strip $(AMGS)), true)
OBJS = mod_def.o calc_normal_vector.o cell_identify.o mpi_communication_array_utility.o mpi_communication_with_other_procs.o \
       mpi_initialize_decomposition.o mpi_isend_irecv_dble.o mpi_isend_irecv_int.o mpi_pack_dble.o mpi_pack_dble_normal.o \
       mpi_pack_int.o mpi_unpack_dble.o mpi_unpack_dble_normal.o mpi_unpack_int.o newton_loop.o \
       output_exec_log_rad.o output_result_data.o radiation.o ray_trace.o readline.o \
       set_boundary_cell_state.o set_boundary_normal_vector.o set_boundary_radiation.o set_boundary_tmp_cell.o set_radiation_configuration.o \
       set_radiation_initial_condition.o upwind_schemes.o 
else
OBJS = mod_def.o calc_normal_vector.o cell_identify.o mpi_communication_array_utility.o mpi_communication_with_other_procs.o \
       mpi_initialize_decomposition.o mpi_isend_irecv_dble.o mpi_isend_irecv_int.o mpi_pack_dble.o mpi_pack_dble_normal.o \
       mpi_pack_int.o mpi_unpack_dble.o mpi_unpack_dble_normal.o mpi_unpack_int.o newton_loop.o \
       output_exec_log_rad.o output_result_data.o radiation.o ray_trace.o readline.o \
       set_boundary_cell_state.o set_boundary_normal_vector.o set_boundary_radiation.o set_boundary_tmp_cell.o set_radiation_configuration.o \
       set_radiation_initial_condition.o upwind_schemes.o 
endif

M_ARCH = $(shell uname -m)

FLAGS = -O3 -xCORE-AVX2 -qopenmp #-no-ip -no-ipo -fno-inline -fno-inline-functions
FLAGS += ${SCHEME}
FLAGS += ${BX}
FLAGS += ${ICEX}
FLAGS += ${SOLVER}
FLAGS += -I${MPI_DIR}/include
FLAGS += -fpp

LDFLAGS  = -lm -lmpi

all : ${PROG}

${PROG} : ${OBJS}
	${COMP}  ${FLAGS} -o $@ ${OBJS} ${LDFLAGS} 

%.o : %.c
	${COMP} -c ${FLAGS} $<

%.o : %.f90
	${COMP} -c -I${AMGS_DIR} -c ${FLAGS} $< 

clean :
	${RM} -f ${PROG} ${OBJS}
