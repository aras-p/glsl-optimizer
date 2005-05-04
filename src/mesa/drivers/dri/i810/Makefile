# src/mesa/drivers/dri/i810/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = i810_dri.so

# Not yet
# MINIGLX_SOURCES = server/i810_dri.c 

COMMON_SOURCES = \
	../../common/driverfuncs.c \
        ../common/mm.c \
        ../common/utils.c \
        ../common/texmem.c \
        ../common/vblank.c \
	../common/dri_util.c \
	../common/glcontextmodes.c \
	../common/drirenderbuffer.c \
	../common/xmlconfig.c

DRIVER_SOURCES = \
	i810context.c \
	i810ioctl.c \
	i810render.c \
	i810screen.c \
	i810span.c \
	i810state.c \
	i810tex.c \
	i810texmem.c \
	i810texstate.c \
	i810tris.c \
	i810vb.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

include ../Makefile.template

symlinks:
