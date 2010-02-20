TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = nouveau_dri.so

# hideous hack
-Wl,--start-group:
-Wl,--end-group:

PIPE_DRIVERS = \
	$(TOP)/src/gallium/state_trackers/dri/libdridrm.a \
	$(TOP)/src/gallium/winsys/drm/nouveau/drm/libnouveaudrm.a \
	-Wl,--start-group \
	$(TOP)/src/gallium/drivers/nv30/libnv30.a \
	$(TOP)/src/gallium/drivers/nv40/libnv40.a \
	$(TOP)/src/gallium/drivers/nvfx/libnvfx.a \
	-Wl,--end-group \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a \
	$(TOP)/src/gallium/drivers/nouveau/libnouveau.a

DRIVER_SOURCES =

C_SOURCES = \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

include ../../Makefile.template

DRI_LIB_DEPS += $(shell pkg-config libdrm_nouveau --libs)

symlinks:
