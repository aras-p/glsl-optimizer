TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = nouveau

LIBRARY_INCLUDES = \
	$(LIBDRM_CFLAGS) \
	-I$(TOP)/src/gallium/drivers/nouveau/include

# get C_SOURCES
include Makefile.sources

include ../../Makefile.template
