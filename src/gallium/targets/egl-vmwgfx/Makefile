TOP = ../../../..
include $(TOP)/configs/current

EGL_DRIVER_NAME = vmwgfx
EGL_DRIVER_SOURCES = dummy.c
EGL_DRIVER_LIBS =

EGL_DRIVER_PIPES = \
	$(TOP)/src/gallium/winsys/svga/drm/libsvgadrm.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(TOP)/src/gallium/drivers/svga/libsvga.a

include ../Makefile.egl
