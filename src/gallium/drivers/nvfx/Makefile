TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = nvfx

# get C_SOURCES
include Makefile.sources

LIBRARY_INCLUDES = \
	$(LIBDRM_CFLAGS) \
	-I$(TOP)/src/gallium/drivers/nouveau/include

include ../../Makefile.template
