# src/egl/drivers/dri2/Makefile

TOP = ../../../..
include $(TOP)/configs/current

EGL_DRIVER = egl_dri2.so
EGL_SOURCES = egl_dri2.c

EGL_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/egl/main \
	-I$(TOP)/src/mesa \
	-DDEFAULT_DRIVER_DIR=\"$(DRI_DRIVER_SEARCH_DIR)\" \
	$(shell pkg-config --cflags xcb-dri2 xcb-xfixes x11-xcb libdrm)

EGL_CFLAGS =
EGL_LIBS = $(shell pkg-config --libs xcb-dri2 xcb-xfixes x11-xcb libdrm)

include ../Makefile.template
