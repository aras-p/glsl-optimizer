TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = r300

# get C_SOURCES
include Makefile.sources

# needed to build non-libmesagallium.a targets
C_SOURCES += \
	$(TOP)/src/glsl/ralloc.c \
	$(TOP)/src/mesa/program/register_allocate.c

#This is for libdrm include path
CFLAGS+=$(RADEON_CFLAGS)

LIBRARY_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/glsl

include ../../Makefile.template

test: default
	@$(MAKE) -s -C compiler/tests/
