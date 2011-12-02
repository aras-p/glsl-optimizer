TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = nouveau

LIBRARY_INCLUDES = \
	$(LIBDRM_CFLAGS) \
	-I$(TOP)/src/gallium/drivers/nouveau/include

LIBRARY_INCLUDES += $(shell $(PKG_CONFIG) libdrm libdrm_nouveau --cflags-only-I)
LIBRARY_DEFINES += $(shell $(PKG_CONFIG) libdrm libdrm_nouveau --cflags-only-other)

# get C_SOURCES
include Makefile.sources

include ../../Makefile.template
