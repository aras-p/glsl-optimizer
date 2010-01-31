# src/mesa/drivers/dri/fb/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = fb_dri.so


DRIVER_SOURCES = \
	fb_dri.c \
	$(EGL_SOURCES)


C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

# XXX not 100% sure this is right
#WINDOW_SYSTEM = solo

include ../Makefile.template

