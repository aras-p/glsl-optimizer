TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = EGL_i915.so

PIPE_DRIVERS = \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/i915simple/libi915simple.a \
	$(TOP)/src/gallium/state_trackers/egl/libegldrm.a \
	../gem/libinteldrm.a

DRIVER_SOURCES = \
	intel_context.c \
	intel_device.c \
	intel_api.c

C_SOURCES = \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

DRIVER_EXTRAS = -ldrm_intel

ASM_SOURCES = 

DRIVER_DEFINES = -I../gem $(shell pkg-config libdrm --atleast-version=2.3.1 \
				&& echo "-DDRM_VBLANK_FLIP=DRM_VBLANK_FLIP")

include ../../Makefile.template

symlinks:
