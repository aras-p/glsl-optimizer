TOP = ../../../../../..
include $(TOP)/configs/current

EGL_DRIVER_NAME = swrast
EGL_DRIVER_SOURCES = dummy.c
EGL_DRIVER_LIBS =

EGL_DRIVER_PIPES = \
	$(TOP)/src/gallium/winsys/drm/swrast/core/libswrastdrm.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a

include ../../Makefile.egl
