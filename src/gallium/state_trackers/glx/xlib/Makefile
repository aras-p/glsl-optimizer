TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = xlib

LIBRARY_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main

C_SOURCES = \
	glx_api.c \
	glx_getproc.c \
	glx_usefont.c \
	xm_api.c

include ../../../Makefile.template
