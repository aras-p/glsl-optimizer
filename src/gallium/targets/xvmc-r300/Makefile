TOP = ../../../..
include $(TOP)/configs/current

LIBBASENAME = XvMCr300

PIPE_DRIVERS = \
        $(TOP)/src/gallium/drivers/r300/libr300.a \
	$(TOP)/src/gallium/winsys/g3dvl/dri/libvldri.a \
        $(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
        $(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/auxiliary/libgallium.a

C_SOURCES = \
	target.c \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

DRIVER_LIBS = $(shell $(PKG_CONFIG) libdrm --libs) -lXfixes

include ../Makefile.xvmc

symlinks:
