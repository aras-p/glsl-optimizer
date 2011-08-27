TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = r600

LIBRARY_INCLUDES = \
	$(shell pkg-config libdrm --cflags-only-I)

# get C_SOURCES
include Makefile.sources

include ../../Makefile.template
