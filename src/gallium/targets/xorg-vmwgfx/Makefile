TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = vmwgfx_drv.so

C_SOURCES = \
	vmw_xorg.c \
	vmw_video.c \
	vmw_ioctl.c \
	vmw_ctrl.c \
	vmw_screen.c

DRIVER_INCLUDES = \
	-I$(TOP)/src/gallium

DRIVER_DEFINES = \
	-std=gnu99 \
	-DHAVE_CONFIG_H

DRIVER_LINKS = \
	$(TOP)/src/gallium/state_trackers/xorg/libxorgtracker.a \
	$(TOP)/src/gallium/winsys/svga/drm/libsvgadrm.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(TOP)/src/gallium/drivers/svga/libsvga.a \
	$(GALLIUM_AUXILIARIES) \
	$(shell pkg-config --libs --silence-errors libkms) \
	$(shell pkg-config --libs libdrm)

include ../Makefile.xorg
