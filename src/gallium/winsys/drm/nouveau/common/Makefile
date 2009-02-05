TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = nouveaudrm

C_SOURCES = \
        nouveau_context.c \
	nouveau_lock.c \
	nouveau_screen.c \
	nouveau_winsys.c \
	nouveau_winsys_pipe.c \
	nouveau_winsys_softpipe.c

include ./Makefile.template

DRIVER_DEFINES = $(shell pkg-config libdrm --cflags \
                && pkg-config libdrm --atleast-version=2.3.1 \
                && pkg-config libdrm_nouveau --exact-version=0.5 \
                && pkg-config libdrm_nouveau --cflags \
                && echo "-DDRM_VBLANK_FLIP=DRM_VBLANK_FLIP")
symlinks:

