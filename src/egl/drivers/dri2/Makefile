# src/egl/drivers/dri2/Makefile

TOP = ../../../..
include $(TOP)/configs/current

EGL_DRIVER = egl_dri2
EGL_SOURCES = egl_dri2.c platform_x11.c platform_drm.c

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

ifneq ($(findstring wayland, $(EGL_PLATFORMS)),)
EGL_SOURCES += platform_wayland.c
EGL_INCLUDES += -DHAVE_WAYLAND_PLATFORM $(WAYLAND_CFLAGS) \
		-I$(TOP)/src/egl/wayland
EGL_LIBS += $(WAYLAND_LIBS)
endif

include ../Makefile.template
