TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = radeonsi

LIBRARY_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/gallium/drivers/radeon/


# get C_SOURCES
include Makefile.sources

LIBRADEON = $(TOP)/src/gallium/drivers/radeon/libradeon.a

EXTRA_OBJECTS = \
	$(LIBRADEON)

CFLAGS+=$(RADEON_CFLAGS)

include ../../Makefile.template
