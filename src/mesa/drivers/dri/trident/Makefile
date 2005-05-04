# src/mesa/drivers/dri/trident/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = trident_dri.so

# Not yet
# MINIGLX_SOURCES = server/trident_dri.c 

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
	trident_context.c \
	trident_state.c \
	trident_vb.c \
	trident_dd.c \
	trident_tris.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

include ../Makefile.template

symlinks:
