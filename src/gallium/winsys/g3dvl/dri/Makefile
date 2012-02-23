TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = vldri

LIBRARY_INCLUDES = -I$(TOP)/src/gallium/winsys/g3dvl \
                   $(shell $(PKG_CONFIG) libdrm --cflags-only-I)

C_SOURCES = \
	dri_winsys.c

include ../../../Makefile.template
