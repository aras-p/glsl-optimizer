TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = nouveau_dri.so

PIPE_DRIVERS = \
	$(TOP)/src/gallium/state_trackers/dri/libdridrm.a \
	$(TOP)/src/gallium/winsys/nouveau/drm/libnouveaudrm.a \
	$(TOP)/src/gallium/drivers/nvfx/libnvfx.a \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a \
	$(TOP)/src/gallium/drivers/nouveau/libnouveau.a

C_SOURCES = \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

include ../Makefile.dri

DRI_LIB_DEPS += $(shell pkg-config libdrm_nouveau --libs)

symlinks:
