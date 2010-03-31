TOP = ../../../..
include $(TOP)/configs/current

# Do propperly
CFLAGS+="-I$(TOP)/src/gallium/include"

EGL_DRIVER_NAME = swrast
EGL_DRIVER_SOURCES = swrast_glue.c
EGL_DRIVER_LIBS =
EGL_DRIVER_PIPES =

include ../Makefile.egl
