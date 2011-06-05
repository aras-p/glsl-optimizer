TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = nouveau2_drv.so

C_SOURCES = \
	nouveau_target.c \
	nouveau_xorg.c

DRIVER_DEFINES = \
	-DHAVE_CONFIG_H -DGALLIUM_RBUG -DGALLIUM_TRACE

DRIVER_PIPES = \
	$(TOP)/src/gallium/state_trackers/xorg/libxorgtracker.a \
	$(TOP)/src/gallium/winsys/nouveau/drm/libnouveaudrm.a \
	$(TOP)/src/gallium/drivers/nvfx/libnvfx.a \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a \
	$(TOP)/src/gallium/drivers/nvc0/libnvc0.a \
	$(TOP)/src/gallium/drivers/nouveau/libnouveau.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a

DRIVER_LINKS = \
	$(shell pkg-config --libs libdrm libdrm_nouveau)

DRIVER_INCLUDES = \
	$(shell pkg-config --cflags-only-I libdrm libdrm_nouveau xf86driproto)

include ../Makefile.xorg
