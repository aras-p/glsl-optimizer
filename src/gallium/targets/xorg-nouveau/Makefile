TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = modesetting_drv.so

C_SOURCES = \
	nouveau_xorg.c

DRIVER_DEFINES = \
	-DHAVE_CONFIG_H

DRIVER_LINKS = \
	$(TOP)/src/gallium/state_trackers/xorg/libxorgtracker.a \
	$(TOP)/src/gallium/winsys/nouveau/drm/libnouveaudrm.a \
	$(TOP)/src/gallium/drivers/nvfx/libnvfx.a \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a \
	$(TOP)/src/gallium/drivers/nouveau/libnouveau.a \
	$(GALLIUM_AUXILIARIES) \
	$(shell pkg-config --libs libdrm libdrm_nouveau)

include ../Makefile.xorg
