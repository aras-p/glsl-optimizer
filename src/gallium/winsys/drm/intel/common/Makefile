TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = inteldrm

C_SOURCES = \
	intel_be_batchbuffer.c \
	intel_be_context.c \
	intel_be_device.c \
	ws_dri_bufmgr.c \
	ws_dri_drmpool.c \
	ws_dri_fencemgr.c \
	ws_dri_mallocpool.c \
	ws_dri_slabpool.c


include ./Makefile.template

DRIVER_DEFINES = $(shell pkg-config libdrm --cflags \
                && pkg-config libdrm --atleast-version=2.3.1 \
                && echo "-DDRM_VBLANK_FLIP=DRM_VBLANK_FLIP")
symlinks:

