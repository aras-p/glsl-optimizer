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

LIBRARY_INCLUDES = $(shell pkg-config libdrm libdrm_nouveau --cflags-only-I)

LIBRARY_DEFINES = $(shell pkg-config libdrm --cflags-only-other \
		&& pkg-config libdrm --atleast-version=2.3.1 \
		&& pkg-config libdrm_nouveau --exact-version=0.5 \
		&& pkg-config libdrm_nouveau --cflags-only-other \
		&& echo "-DDRM_VBLANK_FLIP=DRM_VBLANK_FLIP")

include ../../../../Makefile.template
