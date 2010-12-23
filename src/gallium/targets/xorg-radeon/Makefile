TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = radeon_drv.so

C_SOURCES = \
	radeon_target.c \
	radeon_xorg.c

DRIVER_DEFINES = \
	-DHAVE_CONFIG_H -DGALLIUM_RBUG -DGALLIUM_TRACE -DGALLIUM_GALAHAD

DRIVER_PIPES = \
	$(TOP)/src/gallium/state_trackers/xorg/libxorgtracker.a \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/r300/libr300.a \
	$(TOP)/src/gallium/drivers/galahad/libgalahad.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a

DRIVER_LINKS = \
	$(shell pkg-config --libs libdrm)

include ../Makefile.xorg
