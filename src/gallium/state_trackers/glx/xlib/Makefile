TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = xlib

LIBRARY_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main

C_SOURCES = \
	glxapi.c \
	fakeglx.c \
	fakeglx_fonts.c \
	xm_api.c

include ../../../Makefile.template
