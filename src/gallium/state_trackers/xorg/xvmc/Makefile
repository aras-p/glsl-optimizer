TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = xvmctracker

LIBRARY_INCLUDES = \
	$(shell pkg-config --cflags-only-I xvmc) \
	-I$(TOP)/src/gallium/winsys/g3dvl

C_SOURCES = block.c \
            surface.c \
            context.c \
            subpicture.c \
            attributes.c

include ../../../Makefile.template
