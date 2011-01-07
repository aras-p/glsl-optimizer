# src/egl/drivers/dri2/Makefile

TOP = ../../../..
include $(TOP)/configs/current

EGL_DRIVER = egl_dri2.so
EGL_SOURCES = egl_dri2.c

EGL_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/egl/main \
	-I$(TOP)/src/mapi \
	-DDEFAULT_DRIVER_DIR=\"$(DRI_DRIVER_SEARCH_DIR)\" \
	$(XCB_DRI2_CFLAGS) \
	$(LIBUDEV_CFLAGS) \
	$(LIBDRM_CFLAGS)

EGL_LIBS = $(XCB_DRI2_LIBS) $(LIBUDEV_LIBS) $(DLOPEN_LIBS) $(LIBDRM_LIB)

include ../Makefile.template
