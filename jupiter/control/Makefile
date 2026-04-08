
AR = $(shell which gcc-ar 2>/dev/null)
ifeq ($(strip $(AR)),)
AR = ar
ARC = cr
else
ARC = crs
endif
LIBEXT = .a
EXEEXT =

CC=icc
CFLAGS=
JCNTRL_USE_OPENMP=

EXECUTIVES = fv_table.c

libjupiter-control_SRCS = connection.c data_array.c error.c				\
executive.c field_variable.c global.c information.c input.c manager.c	\
output.c shared_object.c $(EXECUTIVES)

ifeq ($(strip $(JCNTRL_USE_OPENMP)),1)
libjupiter-control_DEFS = -DJCNTRL_USE_OPENMP
else
libjupiter-control_DEFS =
endif

libjupiter-control_FLAGS = -I..

LIB = libjupiter-control
EXE =

.SUFFIXES:
.PHONY: all clean

all: $(patsubst %,%$(LIBEXT),$(LIB)) $(patsubst %,%$(EXEEXT),$(EXE))


libjupiter-control$(LIBEXT): $(libjupiter-control_SRCS) .libjupiter-control_LIB

%_LIB: %_OBJS
	$(AR) $(ARC) $(patsubst .%_LIB,%$(LIBEXT),$@) $(patsubst %.c,%.o,$($(patsubst .%_LIB,%,$@)_SRCS))

%_CEXE: %_OBJS
	$(CC) $(CFLAGS) $($(patsubst .%_OBJS,%,$@)_FLAGS) $(LFLAGS) $($(patsubst .%_OBJS,%,$@)_LFLAGS) -o $(patsubst .%_CEXE,%$(EXEEXT),$@) $(patsubst %.c,%.o,$($(patsubst .%_CEXE,%,$@)_SRCS)) $($(patsubst .%_CEXE,%,$@)_LIBS)

__OBJECTS=
ifeq ($(__OBJECTS),)
%.o: %.c
	r=1; for t in $(LIB) $(EXE); do \
	   o=.$${t}_OBJS; s=.$${t}_SRCS; x=1; \
	   $(MAKE) $(MFLAGS) $$s && \
	   grep $@ $$s 1>/dev/null 2>/dev/null && x=$$? && \
	   echo	"$$o: $@" > $$s && \
	   $(MAKE) $(MFLAGS) $$o; r=$$?; \
	   $(RM) $$o $$s; if [ $$x == 0 ]; then break; fi \
	done; exit $$r

%_OBJS: %_SRCS
	$(MAKE) $(MFLAGS) CC=$(CC) CFLAGS="$(CFLAGS) $($(patsubst .%_OBJS,%,$@)_FLAGS) $($(patsubst .%_OBJS,%,$@)_DEFS)" __OBJECTS=$< $@
else
%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%_OBJS:
	echo 1 >$@

include $(__OBJECTS)
endif

%_SRCS:
	echo "$(patsubst .%_SRCS,.%_OBJS,$@): $(patsubst %.c,%.o,$($(patsubst .%_SRCS,%,$@)_SRCS))" >$@

%_LIB-clean:
	$(RM) $@
	$(RM) .$(patsubst .%_LIB-clean,%,$@)_LIB
	$(RM) .$(patsubst .%_LIB-clean,%,$@)_OBJS
	$(RM) .$(patsubst .%_LIB-clean,%,$@)_SRCS
	$(RM) $(patsubst .%_LIB-clean,%,$@)$(LIBEXT)
	$(RM) $(patsubst %.c,%.o,$($(patsubst .%_LIB-clean,%,$@)_SRCS))

%_CEXE-clean:
	$(RM) $@
	$(RM) .$(patsubst .%_CEXE-clean,%,$@)_CEXE
	$(RM) .$(patsubst .%_CEXE-clean,%,$@)_OBJS
	$(RM) .$(patsubst .%_CEXE-clean,%,$@)_SRCS
	$(RM) $(patsubst .%_CEXE-clean,%,$@)$(EXEEXT)
	$(RM) $(patsubst %.c,%.o,$($(patsubst .%_CEXE-clean,%,$@)_SRCS))

clean: $(patsubst %,.%_LIB-clean,$(LIB)) $(patsubst %,.%_CEXE-clean,$(EXE))
