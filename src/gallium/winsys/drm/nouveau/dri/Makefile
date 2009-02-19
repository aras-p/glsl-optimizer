TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = nouveau_dri.so

MINIGLX_SOURCES =

PIPE_DRIVERS = \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a
#	$(TOP)/src/gallium/drivers/nv04/libnv04.a \
#	$(TOP)/src/gallium/drivers/nv10/libnv10.a \
#	$(TOP)/src/gallium/drivers/nv20/libnv20.a \
#	$(TOP)/src/gallium/drivers/nv30/libnv30.a \
#	$(TOP)/src/gallium/drivers/nv40/libnv40.a \
	
DRIVER_SOURCES = \
	nouveau_context_dri.c \
	nouveau_screen_dri.c \
	nouveau_swapbuffers.c \
	../common/libnouveaudrm.a

C_SOURCES = \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

DRIVER_DEFINES = $(shell pkg-config libdrm_nouveau --cflags)
DRI_LIB_DEPS += $(shell pkg-config libdrm_nouveau --libs)

include ../../Makefile.template

symlinks:
