TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = dridrm

LIBRARY_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/drivers/dri/common \
	-I$(TOP)/src/mesa/main \
         $(shell pkg-config --cflags-only-I libdrm)


C_SOURCES = \
	dri_context.c \
	dri_screen.c \
	dri_drawable.c \
	dri_extensions.c

#	$(TOP)/src/mesa/drivers/dri/common/utils.c \
	$(TOP)/src/mesa/drivers/dri/common/vblank.c \
	$(TOP)/src/mesa/drivers/dri/common/dri_util.c \
	$(TOP)/src/mesa/drivers/dri/common/xmlconfig.c \
	$(TOP)/src/mesa/drivers/common/driverfuncs.c \
	$(TOP)/src/mesa/drivers/dri/common/texmem.c \
	$(TOP)/src/mesa/drivers/dri/common/drirenderbuffer.c

include ../../Makefile.template
