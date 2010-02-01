# This makefile produces a libXvMCg3dvl.so which is
# based on DRM/DRI

TOP = ../../../../../..
include $(TOP)/configs/current

C_SOURCES =

DRIVER_INCLUDES = $(shell pkg-config libdrm libdrm_radeon --cflags-only-I) \
                   -I$(TOP)/src/gallium/winsys/drm/radeon \
DRIVER_DEFINES = $(shell pkg-config libdrm libdrm_radeon --cflags-only-other)

PIPE_DRIVERS = \
	$(TOP)/src/gallium/winsys/drm/radeon/core/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/r300/libr300.a \
        $(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a

DRIVER_LIB_DEPS += $(shell pkg-config libdrm_radeon --libs)

include ../Makefile.template
