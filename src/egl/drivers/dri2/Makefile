# src/egl/drivers/dri2/Makefile

TOP = ../../../..
include $(TOP)/configs/current

EGL_DRIVER = egl_dri2
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

EGL_CFLAGS = -D_EGL_MAIN=_eglBuiltInDriverDRI2
EGL_BUILTIN = true

ifeq ($(SHARED_GLAPI),1)
EGL_CFLAGS += -DHAVE_SHARED_GLAPI
endif

include ../Makefile.template
