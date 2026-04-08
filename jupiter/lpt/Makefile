
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
FC=ifort
CFLAGS=
FCFLAGS=-cpp

libjupiter-lpt_SRCS = LPTcal.f LPTval.f90 LPTjupiter.f90 LPTbnd.c LPTbnd.f90
libjupiter-lpt_DEFS = -DLPT $(subst -DJUPITER_LPT_USE_MPI,-Dmpi -DJUPITER_LPT_USE_MPI,$(LPT_SCHEME))
libjupiter-lpt_CLEAN = *.mod

LIB = libjupiter-lpt
EXE =

.SUFFIXES:
.PHONY: all clean

all: $(patsubst %,%$(LIBEXT),$(LIB)) $(patsubst %,%$(EXEEXT),$(EXE))

libjupiter-lpt$(LIBEXT): $(libjupiter-lpt_SRCS) .libjupiter-lpt_LIB

%_LIB: %_OBJS
	$(AR) $(ARC) $(patsubst .%_LIB,%$(LIBEXT),$@) $(patsubst %,%.o,$($(patsubst .%_LIB,%,$@)_SRCS))

%_CEXE: %_OBJS
	$(CC) $(CFLAGS) $($(patsubst .%_OBJS,%,$@)_FLAGS) $(LFLAGS) $($(patsubst .%_OBJS,%,$@)_LFLAGS) -o $(patsubst .%_CEXE,%$(EXEEXT),$@) $(patsubst %,%.o,$($(patsubst .%_CEXE,%,$@)_SRCS)) $($(patsubst .%_CEXE,%,$@)_LIBS)

__OBJECTS=
ifeq ($(__OBJECTS),)
%.o: %
	r=1; for t in $(LIB) $(EXE); do \
	   o=.$${t}_OBJS; s=.$${t}_SRCS; x=1; \
	   $(MAKE) $(MFLAGS) $$s && \
	   grep $@ $$s 1>/dev/null 2>/dev/null && x=$$? && \
	   echo	"$$o: $@" > $$s && \
	   $(MAKE) $(MFLAGS) $$o; r=$$?; \
	   $(RM) $$o $$s; if [ $$x == 0 ]; then break; fi \
	done; exit $$r

%_OBJS: %_SRCS
	$(MAKE) $(MFLAGS) CC=$(CC) FC=$(FC) FCFLAGS="$(FCFLAGS) $($(patsubst .%_OBJS,%,$@)_FLAGS)  $($(patsubst .%_OBJS,%,$@)_DEFS)" CFLAGS="$(CFLAGS) $($(patsubst .%_OBJS,%,$@)_FLAGS) $($(patsubst .%_OBJS,%,$@)_DEFS)" __OBJECTS=$< $@
else
%.c.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

%.f.o: %.f
	$(FC) $(FCFLAGS) -c -o $@ $<

%.f90.o: %.f90
	$(FC) $(FCFLAGS) -c -o $@ $<

%_OBJS:
	echo 1 >$@

include $(__OBJECTS)
endif

%_SRCS:
	echo "$(patsubst .%_SRCS,.%_OBJS,$@): $(patsubst %,%.o,$($(patsubst .%_SRCS,%,$@)_SRCS))" >$@

%_LIB-clean:
	$(RM) $@
	$(RM) .$(patsubst .%_LIB-clean,%,$@)_LIB
	$(RM) .$(patsubst .%_LIB-clean,%,$@)_OBJS
	$(RM) .$(patsubst .%_LIB-clean,%,$@)_SRCS
	$(RM) $(patsubst .%_LIB-clean,%,$@)$(LIBEXT)
	$(RM) $(patsubst %,%.o,$($(patsubst .%_LIB-clean,%,$@)_SRCS))
	$(RM) $($(patsubst .%_LIB-clean,%,$@)_CLEAN)

%_CEXE-clean:
	$(RM) $@
	$(RM) .$(patsubst .%_CEXE-clean,%,$@)_CEXE
	$(RM) .$(patsubst .%_CEXE-clean,%,$@)_OBJS
	$(RM) .$(patsubst .%_CEXE-clean,%,$@)_SRCS
	$(RM) $(patsubst .%_CEXE-clean,%,$@)$(EXEEXT)
	$(RM) $(patsubst %,%.o,$($(patsubst .%_CEXE-clean,%,$@)_SRCS))
	$(RM) $($(patsubst .%_CEXE-clean,%,$@)_CLEAN)

clean: $(patsubst %,.%_LIB-clean,$(LIB)) $(patsubst %,.%_CEXE-clean,$(EXE))

LPTval.f90.o: LPTjupiter.f90.o
LPTbnd.f90.o: LPTval.f90.o LPTjupiter.f90.o
LPTcal.f.o: LPTval.f90.o LPTjupiter.f90.o LPTbnd.f90.o
