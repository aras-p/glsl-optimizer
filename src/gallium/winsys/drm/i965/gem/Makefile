TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = inteldrm

C_SOURCES = \
	intel_drm_batchbuffer.c \
	intel_drm_buffer.c \
	intel_drm_fence.c \
	intel_drm_api.c

LIBRARY_INCLUDES = $(shell pkg-config libdrm --cflags-only-I)

LIBRARY_DEFINES = $(shell pkg-config libdrm --cflags-only-other)

include ../../../../Makefile.template
