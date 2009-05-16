TOP = ../../../../../..
GALLIUMDIR = ../../../..
include $(TOP)/configs/current

LIBNAME = EGL_r300.so

PIPE_DRIVERS = \
	$(TOP)/src/gallium/state_trackers/egl/libegldrm.a \
	$(GALLIUMDIR)/winsys/drm/radeon/core/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/r300/libr300.a

DRIVER_SOURCES =

C_SOURCES = \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

DRIVER_EXTRAS = -ldrm_radeon

ASM_SOURCES = 

include ../../Makefile.template

symlinks:
