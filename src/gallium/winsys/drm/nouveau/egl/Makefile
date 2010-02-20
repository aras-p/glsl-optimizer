TOP = ../../../../../..
include $(TOP)/configs/current

EGL_DRIVER_NAME = nouveau
EGL_DRIVER_SOURCES = dummy.c
EGL_DRIVER_LIBS = -ldrm_nouveau

# hideous hack
-Wl,--start-group:
-Wl,--end-group:

EGL_DRIVER_PIPES = \
	$(TOP)/src/gallium/winsys/drm/nouveau/drm/libnouveaudrm.a \
	-Wl,--start-group \
	$(TOP)/src/gallium/drivers/nv30/libnv30.a \
	$(TOP)/src/gallium/drivers/nv40/libnv40.a \
	$(TOP)/src/gallium/drivers/nv40/libnvfx.a \
	-Wl,--end-group \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a \
	$(TOP)/src/gallium/drivers/nouveau/libnouveau.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a

include ../../Makefile.egl
