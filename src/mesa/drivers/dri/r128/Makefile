# src/mesa/drivers/dri/r128/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = r128_dri.so

include ../Makefile.defines

DRIVER_SOURCES = \
	r128_context.c \
	r128_lock.c \
	r128_state.c \
	r128_texstate.c \
	r128_dd.c \
	r128_screen.c \
	r128_tex.c \
	r128_tris.c \
	r128_ioctl.c \
	r128_span.c \
	r128_texmem.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES) 

ASM_SOURCES = 


include ../Makefile.targets

