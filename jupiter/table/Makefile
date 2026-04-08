
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
TABLE_USE_OPENMP=

libjupiter-table_SRCS = table.c table-file.c                   \
          linear.c linear-save.c bin-tree.c bin-tree-minmax.c
libcsv2table_SRCS = csv2table.c
csv2table_SRCS = csv2table-main.c

ifeq ($(strip $(TABLE_USE_OPENMP)),1)
libjupiter-table_DEFS = -DTABLE_USE_OPENMP
else
libjupiter-table_DEFS =
endif

libcsv2table_DEFS = $(libjupiter-table_DEFS)

csv2table_LIBS = -lm libjupiter-table.a libcsv2table.a
csv2table_DEFS = $(libjupiter-table_DEFS)

LIB = libjupiter-table libcsv2table
EXE = csv2table

.SUFFIXES:
.PHONY: all clean
.PRECIOUS: %.c

all: $(patsubst %,%$(LIBEXT),$(LIB)) $(patsubst %,%$(EXEEXT),$(EXE))

libjupiter-table$(LIBEXT): $(libjupiter-table_SRCS) .libjupiter-table_LIB
libcsv2table$(LIBEXT): $(libcsv2table_SRCS) .libcsv2table_LIB
csv2table$(EXEEXT): $(csv2table_SRCS) .csv2table_CEXE

.csv2table_CEXE: libjupiter-table.a libcsv2table.a

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

ifneq ($(RE2C), )
%.c : %.c.re
	$(RE2C) $(RE2CFLAGS) -o $@ $<

%.h : %.h.re
	$(RE2C) $(RE2CFLAGS) -o $@ $<
endif
