# This makefile produces a libXvMCg3dvl.so which is
# based on DRM/DRI

TOP = ../../../../../..
include $(TOP)/configs/current

C_SOURCES =

DRIVER_INCLUDES = $(shell pkg-config libdrm libdrm_nouveau --cflags-only-I) \
                   -I$(TOP)/src/gallium/winsys/drm/nouveau
DRIVER_DEFINES = $(shell pkg-config libdrm libdrm_nouveau --cflags-only-other)

PIPE_DRIVERS = \
	$(TOP)/src/gallium/winsys/drm/nouveau/drm/libnouveaudrm.a \
	$(TOP)/src/gallium/drivers/nv04/libnv04.a \
	$(TOP)/src/gallium/drivers/nv10/libnv10.a \
	$(TOP)/src/gallium/drivers/nv20/libnv20.a \
	$(TOP)/src/gallium/drivers/nv30/libnv30.a \
	$(TOP)/src/gallium/drivers/nv40/libnv40.a \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a \
	$(TOP)/src/gallium/drivers/nouveau/libnouveau.a \
        $(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a

DRIVER_LIB_DEPS += $(shell pkg-config libdrm_nouveau --libs)

include ../Makefile.template
