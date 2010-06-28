TOP = ../../../..
include $(TOP)/configs/current

EGL_DRIVER_NAME = radeon
EGL_DRIVER_SOURCES = target.c
EGL_DRIVER_LIBS = -ldrm_radeon

EGL_DRIVER_DEFINES = \
	-DGALLIUM_RBUG -DGALLIUM_TRACE -DGALLIUM_GALAHAD

EGL_DRIVER_PIPES = \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/galahad/libgalahad.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(TOP)/src/gallium/drivers/r300/libr300.a

include ../Makefile.egl
