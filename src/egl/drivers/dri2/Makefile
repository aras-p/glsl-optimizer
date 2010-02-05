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
	$(EGL_DRI2_CFLAGS)

EGL_LIBS = $(EGL_DRI2_LIBS)

include ../Makefile.template
