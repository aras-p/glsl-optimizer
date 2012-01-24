# src/egl/drivers/dri2/Makefile

TOP = ../../../..
include $(TOP)/configs/current

EGL_DRIVER = egl_dri2
EGL_SOURCES = egl_dri2.c common.c

EGL_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/egl/main \
	-I$(TOP)/src/mapi \
	-I$(TOP)/src/gbm/main \
	-I$(TOP)/src/gbm/backends/dri \
	-DDEFAULT_DRIVER_DIR=\"$(DRI_DRIVER_SEARCH_DIR)\" \
	$(LIBUDEV_CFLAGS) \
	$(LIBDRM_CFLAGS)

EGL_LIBS = $(LIBUDEV_LIBS) $(DLOPEN_LIBS) $(LIBDRM_LIB)

EGL_CFLAGS = -D_EGL_MAIN=_eglBuiltInDriverDRI2
EGL_BUILTIN = true

ifeq ($(SHARED_GLAPI),1)
EGL_CFLAGS += -DHAVE_SHARED_GLAPI
endif

ifneq ($(findstring x11, $(EGL_PLATFORMS)),)
EGL_SOURCES += platform_x11.c
EGL_INCLUDES += -DHAVE_X11_PLATFORM $(XCB_DRI2_CFLAGS)
EGL_LIBS +=  $(XCB_DRI2_LIBS)
endif

ifneq ($(findstring drm, $(EGL_PLATFORMS)),)
EGL_SOURCES += platform_drm.c
EGL_INCLUDES += -DHAVE_DRM_PLATFORM
endif

ifneq ($(findstring wayland, $(EGL_PLATFORMS)),)
EGL_SOURCES += platform_wayland.c
EGL_INCLUDES += -DHAVE_WAYLAND_PLATFORM $(WAYLAND_CFLAGS) \
		-I$(TOP)/src/egl/wayland/wayland-egl \
		-I$(TOP)/src/egl/wayland/wayland-drm
EGL_LIBS += $(WAYLAND_LIBS) \
	    $(TOP)/src/egl/wayland/wayland-drm/libwayland-drm.a
endif

include ../Makefile.template
