# src/egl/drivers/glx/Makefile

TOP = ../../../..
include $(TOP)/configs/current

EGL_DRIVER = egl_glx.so
EGL_SOURCES = egl_glx.c

EGL_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/egl/main

EGL_CFLAGS = $(X11_CFLAGS)
EGL_LIBS = $(X11_LIBS) -lGL

include ../Makefile.template
