TOP = ../../../../../..
include $(TOP)/configs/current

EGL_DRIVER_NAME = i965
EGL_DRIVER_SOURCES = dummy.c
EGL_DRIVER_LIBS = -ldrm_intel

EGL_DRIVER_PIPES = \
	$(TOP)/src/gallium/winsys/drm/i965/gem/libi965drm.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/i965/libi965.a

include ../../Makefile.egl
