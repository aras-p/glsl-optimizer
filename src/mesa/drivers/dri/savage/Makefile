# src/mesa/drivers/dri/r128/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = savage_dri.so

# Doesn't exist yet.
#MINIGLX_SOURCES = server/savage_dri.c 

COMMON_SOURCES = \
	../../common/driverfuncs.c \
	../common/mm.c \
	../common/utils.c \
	../common/texmem.c \
	../common/vblank.c \
	../common/xmlconfig.c \
	../common/dri_util.c \
	../common/glcontextmodes.c

DRIVER_SOURCES = \
	savage_xmesa.c \
	savagedd.c \
	savagestate.c \
	savagetex.c \
	savagetris.c \
	savageioctl.c \
	savagespan.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES) 

ASM_SOURCES = 


include ../Makefile.template

symlinks:
