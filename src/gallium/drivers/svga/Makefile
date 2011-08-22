TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = svga

# get C_SOURCES
include Makefile.sources

LIBRARY_INCLUDES = \
	-I$(TOP)/src/gallium/drivers/svga/include

# With linux-debug we get a lots of warnings, filter out the bad flags.
CFLAGS := $(filter-out -pedantic, $(filter-out -ansi, $(CFLAGS)))

LIBRARY_DEFINES = \
	-std=gnu99 -fvisibility=hidden \
	-DHAVE_STDINT_H -DHAVE_SYS_TYPES_H

include ../../Makefile.template
