# src/egl/drivers/glx/Makefile

TOP = ../../../..
include $(TOP)/configs/current

EGL_DRIVER = egl_glx
EGL_SOURCES = egl_glx.c

EGL_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/egl/main

EGL_CFLAGS = $(X11_CFLAGS)
EGL_LIBS = $(X11_LIBS) $(DLOPEN_LIBS)

EGL_CFLAGS += -D_EGL_MAIN=_eglBuiltInDriverGLX
EGL_BUILTIN = true

include ../Makefile.template
