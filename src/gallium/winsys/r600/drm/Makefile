
TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = r600winsys

C_SOURCES = \
	bof.c \
	evergreen_hw_context.c \
	radeon_bo.c \
	radeon_pciid.c \
	r600_bo.c \
	r600_drm.c \
	r600_hw_context.c \
	r600_bomgr.c

LIBRARY_INCLUDES = -I$(TOP)/src/gallium/drivers/r600 \
		   -I$(TOP)/include \
		   $(shell pkg-config libdrm --cflags-only-I)

include ../../../Makefile.template

symlinks:
