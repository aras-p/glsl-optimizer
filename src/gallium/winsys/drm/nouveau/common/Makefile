TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = nouveaudrm

C_SOURCES = \
	nouveau_bo.c \
	nouveau_channel.c \
	nouveau_context.c \
	nouveau_device.c \
	nouveau_dma.c \
	nouveau_fence.c \
	nouveau_grobj.c \
	nouveau_lock.c \
	nouveau_notifier.c \
	nouveau_pushbuf.c \
	nouveau_resource.c \
	nouveau_screen.c \
	nouveau_winsys.c \
	nouveau_winsys_pipe.c \
	nouveau_winsys_softpipe.c \
	nv04_surface.c \
	nv50_surface.c


include ./Makefile.template

DRIVER_DEFINES = $(shell pkg-config libdrm --cflags \
                && pkg-config libdrm --atleast-version=2.3.1 \
                && echo "-DDRM_VBLANK_FLIP=DRM_VBLANK_FLIP")
symlinks:

