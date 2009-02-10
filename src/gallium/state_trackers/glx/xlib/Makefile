TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = xlib


DRIVER_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/gallium/auxiliary

C_SOURCES = \
	glxapi.c	\
	fakeglx.c	\
	fakeglx_fonts.c	\
	xm_api.c


include ../../../Makefile.template

symlinks:

