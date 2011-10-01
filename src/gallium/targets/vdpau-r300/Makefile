TOP = ../../../..
include $(TOP)/configs/current

LIBBASENAME = vdpau_r300


PIPE_DRIVERS = \
        $(TOP)/src/gallium/drivers/r300/libr300.a \
	$(TOP)/src/gallium/winsys/g3dvl/dri/libvldri.a \
        $(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
        $(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
        $(TOP)/src/gallium/drivers/rbug/librbug.a \
        $(TOP)/src/gallium/drivers/trace/libtrace.a \
        $(TOP)/src/gallium/drivers/galahad/libgalahad.a \
	$(TOP)/src/gallium/auxiliary/libgallium.a \
	$(TOP)/src/mesa/libmesagallium.a

C_SOURCES = \
	target.c \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

DRIVER_LIBS = $(shell $(PKG_CONFIG) libdrm --libs) -lXfixes

include ../Makefile.vdpau

symlinks:
