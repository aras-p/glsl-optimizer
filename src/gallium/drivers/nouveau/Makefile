TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = nouveau

LIBRARY_INCLUDES = \
	$(LIBDRM_CFLAGS) \
	-I$(TOP)/src/gallium/drivers/nouveau/include

C_SOURCES = nouveau_screen.c \
            nouveau_fence.c

include ../../Makefile.template
