
TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = radeonwinsys

C_SOURCES = \
	radeon_drm_bo.c \
	radeon_drm_cs.c \
	radeon_drm_winsys.c

LIBRARY_INCLUDES = -I$(TOP)/include \
		   $(shell pkg-config libdrm --cflags-only-I)

include ../../../Makefile.template

symlinks:
