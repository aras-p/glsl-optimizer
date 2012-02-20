TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = r600

#This is for libdrm include path
CFLAGS+=$(RADEON_CFLAGS)

LIBRARY_INCLUDES = -I$(TOP)/include

# get C_SOURCES
include Makefile.sources

include ../../Makefile.template
