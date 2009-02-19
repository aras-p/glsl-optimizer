TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = inteldrm

C_SOURCES = \
	intel_be_batchbuffer.c \
	intel_be_context.c \
	intel_be_device.c \
	intel_be_api.c

LIBRARY_INCLUDES = $(shell pkg-config libdrm --cflags-only-I)

LIBRARY_DEFINES = $(shell pkg-config libdrm --cflags-only-other)

include ../../../../Makefile.template
