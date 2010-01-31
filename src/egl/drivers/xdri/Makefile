# src/egl/drivers/xdri/Makefile

TOP = ../../../..
include $(TOP)/configs/current

EGL_DRIVER = egl_xdri.so

# steal sources from GLX
GLX_SOURCES = dri_common.c XF86dri.c dri2.c dri2_glx.c dri_glx.c drisw_glx.c
GLX_SOURCES := $(addprefix ../../../glx/x11/,$(GLX_SOURCES))
GLX_INCLUDES = \
	$(shell pkg-config --cflags-only-I libdrm) \
	-I$(TOP)/include/GL/internal \
	-I$(TOP)/src/glx/x11 \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa
GLX_CFLAGS = -DGLX_DIRECT_RENDERING

EGL_SOURCES = egl_xdri.c glxinit.c driinit.c $(GLX_SOURCES)
EGL_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/egl/main \
	$(GLX_INCLUDES)

EGL_CFLAGS = $(GLX_CFLAGS)
EGL_LIBS = -lX11 -lGL

include ../Makefile.template
