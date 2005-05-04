# src/mesa/drivers/dri/ffb/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = ffb_dri.so

# not yet
# MINIGLX_SOURCES = server/ffb_dri.c 

COMMON_SOURCES = \
	../../common/driverfuncs.c \
	../common/mm.c \
	../common/utils.c \
	../common/texmem.c \
	../common/vblank.c \
	../common/xmlconfig.c \
	../common/dri_util.c \
	../common/drirenderbuffer.c \
	../common/glcontextmodes.c

DRIVER_SOURCES = \
	ffb_bitmap.c \
	ffb_clear.c \
	ffb_dd.c \
	ffb_depth.c \
	ffb_fog.c \
	ffb_lines.c \
	ffb_points.c \
	ffb_span.c \
	ffb_state.c \
	ffb_stencil.c \
	ffb_tex.c \
	ffb_tris.c \
	ffb_vb.c \
	ffb_xmesa.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)


ASM_SOURCES = 

include ../Makefile.template

symlinks:
