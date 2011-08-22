TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = i915drm

# get C_SOURCES
include Makefile.sources

LIBRARY_INCLUDES = $(shell pkg-config libdrm --cflags-only-I)

LIBRARY_DEFINES = $(shell pkg-config libdrm --cflags-only-other)

include ../../../Makefile.template
