TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = i915drm

C_SOURCES = \
	i915_drm_batchbuffer.c \
	i915_drm_buffer.c \
	i915_drm_fence.c \
	i915_drm_winsys.c

LIBRARY_INCLUDES = $(shell pkg-config libdrm --cflags-only-I)

LIBRARY_DEFINES = $(shell pkg-config libdrm --cflags-only-other)

include ../../../Makefile.template
