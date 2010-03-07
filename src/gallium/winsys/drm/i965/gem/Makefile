TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = i965drm

C_SOURCES = \
	i965_drm_buffer.c \
	i965_drm_api.c

LIBRARY_INCLUDES = $(shell pkg-config libdrm --cflags-only-I)

LIBRARY_DEFINES = $(shell pkg-config libdrm --cflags-only-other)

include ../../../../Makefile.template
