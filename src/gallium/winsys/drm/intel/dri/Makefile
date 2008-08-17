TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = i915_dri.so
LIBNAME_EGL = egl_i915_dri.so

PIPE_DRIVERS = \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	../common/libinteldrm.a \
	$(TOP)/src/gallium/drivers/i915simple/libi915simple.a


DRIVER_SOURCES = \
	intel_winsys_softpipe.c \
	intel_swapbuffers.c \
	intel_context.c \
	intel_lock.c \
	intel_screen.c

C_SOURCES = \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

DRIVER_DEFINES = -I../common $(shell pkg-config libdrm --atleast-version=2.3.1 \
				&& echo "-DDRM_VBLANK_FLIP=DRM_VBLANK_FLIP")

include ../../Makefile.template

#intel_tex_layout.o: $(TOP)/src/mesa/drivers/dri/intel/intel_tex_layout.c

symlinks:
