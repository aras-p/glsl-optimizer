
TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = nouveau_dri.so

MINIGLX_SOURCES =

PIPE_DRIVERS = \
	$(TOP)/src/mesa/pipe/softpipe/libsoftpipe.a \
	$(TOP)/src/mesa/pipe/nv30/libnv30.a \
	$(TOP)/src/mesa/pipe/nv40/libnv40.a \
	$(TOP)/src/mesa/pipe/nv50/libnv50.a

DRIVER_SOURCES = \
	nouveau_bo.c \
	nouveau_channel.c \
	nouveau_context.c \
	nouveau_device.c \
	nouveau_dma.c \
	nouveau_fence.c \
	nouveau_grobj.c \
	nouveau_lock.c \
	nouveau_notifier.c \
	nouveau_pushbuf.c \
	nouveau_resource.c \
	nouveau_screen.c \
	nouveau_swapbuffers.c \
	nouveau_winsys.c \
	nouveau_winsys_pipe.c \
	nouveau_winsys_softpipe.c \
	nv04_surface.c \
	nv50_surface.c

C_SOURCES = \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

include ../Makefile.template

symlinks:
