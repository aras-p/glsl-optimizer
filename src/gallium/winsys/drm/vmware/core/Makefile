TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = svgadrm

C_SOURCES = \
        vmw_buffer.c \
        vmw_context.c  \
        vmw_fence.c  \
        vmw_screen.c  \
        vmw_screen_dri.c  \
        vmw_screen_ioctl.c  \
        vmw_screen_pools.c  \
        vmw_screen_svga.c  \
        vmw_surface.c

LIBRARY_INCLUDES = \
       -I$(TOP)/src/gallium/drivers/svga \
       -I$(TOP)/src/gallium/drivers/svga/include \
       -I$(GALLIUM)/src/mesa/drivers/dri/common \
       -I$(GALLIUM)/include \
       -I$(GALLIUM)/include/GL/internal \
       -I$(GALLIUM)/src/mesa \
       -I$(GALLIUM)/src/mesa/main \
       -I$(GALLIUM)/src/mesa/glapi \
       -I$(GALLIUM)/src/egl/main \
       -I$(GALLIUM)/src/egl/drivers/dri \
       $(shell pkg-config libdrm --cflags-only-I)

LIBRARY_DEFINES = \
       -std=gnu99 -fvisibility=hidden \
       -DHAVE_STDINT_H -D_FILE_OFFSET_BITS=64 \
       $(shell pkg-config libdrm --cflags-only-other)

include ../../../../Makefile.template
