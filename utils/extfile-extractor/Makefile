
include ../common.mk

jupiter-extfile-extractor_SRCS = extfile-extractor.c ../make-vtk/bind.c
CFLAGS := $(CFLAGS) -I../make-vtk

.PHONY: clean

jupiter-extfile-extractor: $(jupiter-extfile-extractor_SRCS:.c=.o) $(JUPITER_LIB_ALL)
	$(CC) $(CFLAGS) -o $@ $(jupiter-extfile-extractor_SRCS:.c=.o) $(LFLAGS) $(JUPITER_LIB_ALL)

clean: jupiter-all-clean
	$(RM) jupiter-extfile-extractor $(jupiter-extfile-extractor_SRCS:.c=.o)

include ../common-rules.mk
