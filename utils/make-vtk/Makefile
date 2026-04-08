
include ../common.mk

make-vtk_SRCS = bind.c make-vtk-unconventional.c

.PHONY: clean

make-vtk: $(make-vtk_SRCS:.c=.o) $(JUPITER_LIB_ALL)
	$(CC) $(CFLAGS) -o $@ $(make-vtk_SRCS:.c=.o) $(LFLAGS) $(JUPITER_LIB_ALL)

clean: jupiter-all-clean
	$(RM) make-vtk $(make-vtk_SRCS:.c=.o)

include ../common-rules.mk
