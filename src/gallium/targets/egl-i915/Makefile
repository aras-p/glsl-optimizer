TOP = ../../../..
include $(TOP)/configs/current

EGL_DRIVER_NAME = i915
EGL_DRIVER_SOURCES = dummy.c
EGL_DRIVER_LIBS = -ldrm_intel

EGL_DRIVER_PIPES = \
	$(TOP)/src/gallium/winsys/i915/drm/libi915drm.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/i915/libi915.a

include ../Makefile.egl
