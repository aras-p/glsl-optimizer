TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = nvc0

# get C/CPP_SOURCES
include Makefile.sources

LIBRARY_INCLUDES = \
	$(LIBDRM_CFLAGS)

include ../../Makefile.template

# FIXME: Remove when this driver is converted to automake.
all: default
