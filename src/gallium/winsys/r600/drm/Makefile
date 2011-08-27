
TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = r600winsys

# get C_SOURCES
include Makefile.sources

LIBRARY_INCLUDES = -I$(TOP)/src/gallium/drivers/r600 \
		   -I$(TOP)/include \
		   $(shell pkg-config libdrm --cflags-only-I)

include ../../../Makefile.template

symlinks:
