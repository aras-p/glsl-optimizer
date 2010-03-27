TOP = ../../../..
include $(TOP)/configs/current

EGL_DRIVER_NAME = nouveau
EGL_DRIVER_SOURCES = dummy.c
EGL_DRIVER_LIBS = -ldrm_nouveau

EGL_DRIVER_PIPES = \
	$(TOP)/src/gallium/winsys/nouveau/drm/libnouveaudrm.a \
	$(TOP)/src/gallium/drivers/nvfx/libnvfx.a \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a \
	$(TOP)/src/gallium/drivers/nouveau/libnouveau.a

include ../Makefile.egl
