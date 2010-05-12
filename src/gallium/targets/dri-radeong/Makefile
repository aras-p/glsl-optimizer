TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = radeong_dri.so

PIPE_DRIVERS = \
	$(TOP)/src/gallium/state_trackers/dri/drm/libdridrm.a \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(TOP)/src/gallium/drivers/r300/libr300.a

C_SOURCES = \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

include ../Makefile.dri

DRI_LIB_DEPS += -ldrm_radeon

symlinks:
