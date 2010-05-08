TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = r600winsys

C_SOURCES = \
	bof.c \
	r600_state.c \
	radeon_ctx.c \
	radeon_draw.c \
	radeon_state.c \
	radeon_bo.c \
	radeon_pciid.c \
	radeon.c \
	r600_drm.c

LIBRARY_INCLUDES = -I$(TOP)/src/gallium/drivers/r600 \
		   $(shell pkg-config libdrm --cflags-only-I)

LIBRARY_DEFINES = $(shell pkg-config libdrm --cflags-only-other)

include ../../../../Makefile.template
