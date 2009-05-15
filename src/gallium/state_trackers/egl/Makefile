TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = egldrm

LIBRARY_INCLUDES = \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/mesa/drivers/dri/common \
	-I$(TOP)/src/mesa \
	-I$(TOP)/include \
	-I$(TOP)/src/egl/main \
	$(shell pkg-config --cflags-only-I libdrm)


C_SOURCES = $(wildcard ./*.c)


include ../../Makefile.template
