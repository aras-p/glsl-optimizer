TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = ws_xlib

LIBRARY_INCLUDES = \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/gallium/auxiliary

C_SOURCES = \
	xlib_cell.c \
	xlib_sw_winsys.c \
	xlib_llvmpipe.c \
	xlib_softpipe.c

include ../../Makefile.template


