TOP = ../../../..
include $(TOP)/configs/current

#LIBNAME =

PIPE_DRIVERS = \
	$(TOP)/src/gallium/winsys/g3dvl/dri/libvldri.a \
	$(TOP)/src/gallium/winsys/nouveau/drm/libnouveaudrm.a \
	$(TOP)/src/gallium/drivers/nvfx/libnvfx.a \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/nouveau/libnouveau.a \
	$(TOP)/src/gallium/auxiliary/libgallium.a

C_SOURCES = \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

DRIVER_LIBS = $(shell pkg-config libdrm_nouveau --libs)

include ../Makefile.xvmc

symlinks:
