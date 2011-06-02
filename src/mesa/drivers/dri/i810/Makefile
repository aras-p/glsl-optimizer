# src/mesa/drivers/dri/i810/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = i810_dri.so

include ../Makefile.defines

DRIVER_SOURCES = \
	i810context.c \
	i810ioctl.c \
	i810render.c \
	i810screen.c \
	i810span.c \
	i810state.c \
	i810tex.c \
	i810texmem.c \
	i810texstate.c \
	i810tris.c \
	i810vb.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

include ../Makefile.targets

