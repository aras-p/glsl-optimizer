TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = vmwgfx_drv.so

C_SOURCES = \
	vmw_xorg.c \
	vmw_video.c \
	vmw_ioctl.c \
	vmw_ctrl.c \
	vmw_target.c \
	vmw_screen.c

DRIVER_INCLUDES = \
	-I$(TOP)/src/gallium

DRIVER_DEFINES = \
	-std=gnu99 \
	-DGALLIUM_RBUG \
	-DGALLIUM_TRACE \
	-DHAVE_CONFIG_H

DRIVER_PIPES = \
	$(TOP)/src/gallium/state_trackers/xorg/libxorgtracker.a \
	$(TOP)/src/gallium/winsys/svga/drm/libsvgadrm.a \
	$(TOP)/src/gallium/drivers/svga/libsvga.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a

DRIVER_LINKS = \
	$(shell pkg-config --libs libdrm libkms)

include ../Makefile.xorg
