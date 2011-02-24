
TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = radeonwinsys

C_SOURCES = \
	radeon_drm_bo.c \
	radeon_drm_cs.c \
	radeon_drm_common.c

LIBRARY_INCLUDES = -I$(TOP)/src/gallium/drivers/r300 \
		   $(shell pkg-config libdrm --cflags-only-I)

include ../../../Makefile.template

symlinks:
