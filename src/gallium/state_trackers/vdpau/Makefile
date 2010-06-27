TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = vdpautracker

LIBRARY_INCLUDES = \
	$(shell pkg-config --cflags-only-I vdpau) \
	-I$(TOP)/src/gallium/winsys/g3dvl

C_SOURCES = htab.c \
	    ftab.c \
	    device.c \
	    query.c

include ../../Makefile.template
