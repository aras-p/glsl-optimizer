# src/egl/drivers/glx/Makefile

TOP = ../../../..
include $(TOP)/configs/current

EGL_DRIVER = egl_glx.so
EGL_SOURCES = egl_glx.c

EGL_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/egl/main

EGL_CFLAGS =
EGL_LIBS = -lX11 -lGL

include ../Makefile.template
