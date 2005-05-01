# src/mesa/drivers/dri/fb/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = fb_dri.so

COMMON_SOURCES = \
	../../common/driverfuncs.c \
        ../common/mm.c \
        ../common/utils.c \
        ../common/texmem.c \
        ../common/vblank.c \
	../common/dri_util.c \
	../common/xmlconfig.c \
	../common/glcontextmodes.c

DRIVER_SOURCES = \
	fb_dri.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

include ../Makefile.template

symlinks:

