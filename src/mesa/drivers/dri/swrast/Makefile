# src/mesa/drivers/dri/swrast/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = swrast_dri.so

include ../Makefile.defines

DRIVER_DEFINES = -D__NOT_HAVE_DRM_H

DRIVER_SOURCES = \
	swrast.c \
	swrast_span.c

C_SOURCES = \
	$(SWRAST_COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES =

SWRAST_COMMON_SOURCES = \
	../../common/driverfuncs.c \
	../common/utils.c \
	../common/drisw_util.c

include ../Makefile.targets

