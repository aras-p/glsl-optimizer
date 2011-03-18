TOP = ../../../..
include $(TOP)/configs/current

PIPE_DRIVERS = \
        $(TOP)/src/gallium/drivers/r600/libr600.a \
	$(TOP)/src/gallium/winsys/g3dvl/dri/libvldri.a \
        $(TOP)/src/gallium/winsys/r600/drm/libr600winsys.a \
        $(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/auxiliary/libgallium.a

C_SOURCES = \
	target.c \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

DRIVER_LIBS = $(shell pkg-config libdrm_radeon --libs) -lXfixes

include ../Makefile.vdpau

symlinks:
