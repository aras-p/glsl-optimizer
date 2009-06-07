TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = nouveaudrm

C_SOURCES = nouveau_drm_api.c

LIBRARY_INCLUDES = $(shell pkg-config libdrm libdrm_nouveau --cflags-only-I)
LIBRARY_DEFINES = $(shell pkg-config libdrm libdrm_nouveau --cflags-only-other)

include ../../../../Makefile.template
