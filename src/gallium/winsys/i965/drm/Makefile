TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = i965drm

C_SOURCES = \
	i965_drm_buffer.c \
	i965_drm_winsys.c

LIBRARY_INCLUDES = $(shell $(PKG_CONFIG) libdrm --cflags-only-I)

LIBRARY_DEFINES = $(shell $(PKG_CONFIG) --cflags-only-other)

include ../../../Makefile.template
