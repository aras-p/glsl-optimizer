TOP = ../../../../../..
GALLIUMDIR = ../../../..
include $(TOP)/configs/current

LIBNAME = EGL_i965.so

PIPE_DRIVERS = \
	$(TOP)/src/gallium/state_trackers/egl/libegldrm.a \
	$(GALLIUMDIR)/winsys/drm/i965/gem/libi965drm.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/i965/libi965.a

DRIVER_SOURCES =

C_SOURCES = \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

DRIVER_EXTRAS = -ldrm_intel

ASM_SOURCES = 

DRIVER_DEFINES = -I../gem $(shell pkg-config libdrm --atleast-version=2.3.1 \
				&& echo "-DDRM_VBLANK_FLIP=DRM_VBLANK_FLIP")

include ../../Makefile.template

symlinks:
