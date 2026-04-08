
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
GEOMETRY_USE_OPENMP=

INIT_FUNCS = if_const.c if_linear.c if_none.c if_poly.c if_poly_n.c	\
             if_exp_poly.c

SHAPE_FUNCS = shp_specials.c shp_box.c shp_pla.c shp_app.c \
              shp_wed.c shp_arb.c shp_rpr.c shp_trp.c \
              shp_sph.c shp_rcc.c shp_ell.c shp_trc.c \
              shp_rec.c shp_tec.c shp_tor.c shp_eto.c \
              shp_pl1.c shp_pl2.c shp_pl3.c shp_pl4.c \
              shp_pln.c \
              shp_tra.c shp_rot.c shp_sca.c shp_mat.c

libjupiter-geometry_SRCS = abuilder.c alloc_list.c common.c data.c  \
                           error.c file.c global.c infomap.c init.c \
                           shape.c variant.c udata.c vector.c svector.c \
                           matrix.c funcs_common.c shape_transform.c \
                           shape_mesh.c \
                           $(INIT_FUNCS) $(SHAPE_FUNCS)

ifeq ($(strip $(GEOMETRY_USE_OPENMP)),1)
libjupiter-geometry_DEFS = -DJUPITER_GEOMETRY_USE_OPENMP
else
libjupiter-geometry_DEFS =
endif

LIB = libjupiter-geometry
EXE =

.SUFFIXES:
.PHONY: all clean

all: $(patsubst %,%$(LIBEXT),$(LIB)) $(patsubst %,%$(EXEEXT),$(EXE))

libjupiter-geometry$(LIBEXT): $(libjupiter-geometry_SRCS) .libjupiter-geometry_LIB

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
