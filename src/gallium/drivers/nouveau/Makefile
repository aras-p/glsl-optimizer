TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = nouveau

LIBRARY_INCLUDES = \
	$(LIBDRM_CFLAGS) \
	-I$(TOP)/src/gallium/drivers/nouveau/include

C_SOURCES = nouveau_screen.c \
            nouveau_fence.c \
            nouveau_mm.c \
            nouveau_buffer.c

include ../../Makefile.template
