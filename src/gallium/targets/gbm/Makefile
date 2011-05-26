# src/gallium/targets/gbm/Makefile

TOP = ../../../..
include $(TOP)/configs/current

PIPE_PREFIX := pipe_

GBM_BACKEND = gbm_gallium_drm
GBM_SOURCES = gbm.c pipe_loader.c

GBM_INCLUDES = \
	       -I$(TOP)/include \
	       -I$(TOP)/src/gallium/state_trackers/gbm \
	       -I$(TOP)/src/gbm/main \
	       -I$(TOP)/src/gallium/auxiliary \
	       -I$(TOP)/src/gallium/include \

GBM_LIBS = $(LIBUDEV_LIBS) $(LIBDRM_LIB) \
	   $(TOP)/src/gallium/state_trackers/gbm/libgbm.a \
	   $(TOP)/src/gallium/drivers/identity/libidentity.a \
	   $(TOP)/src/gallium/drivers/trace/libtrace.a \
	   $(TOP)/src/gallium/drivers/rbug/librbug.a \
	   $(GALLIUM_AUXILIARIES)


GBM_CFLAGS = \
	     -D_EGL_GALLIUM_DRIVER_SEARCH_DIR=\"$(EGL_DRIVER_INSTALL_DIR)\" \
	     -DPIPE_PREFIX=\"$(PIPE_PREFIX)\" \
	     $(LIBUDEV_CFLAGS) \
	     $(LIBDRM_CFLAGS)  

include $(TOP)/src/gbm/backends/Makefile.template
