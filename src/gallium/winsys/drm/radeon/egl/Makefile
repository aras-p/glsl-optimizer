TOP = ../../../../../..
include $(TOP)/configs/current

EGL_DRIVER_NAME = radeon
EGL_DRIVER_SOURCES = dummy.c
EGL_DRIVER_LIBS = -ldrm_radeon

EGL_DRIVER_PIPES = \
	$(TOP)/src/gallium/winsys/drm/radeon/core/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/r300/libr300.a

include ../../Makefile.egl
