
TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = i915tex_dri.so

MINIGLX_SOURCES = server/intel_dri.c

PIPE_DRIVERS = \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/i915simple/libi915simple.a

DRIVER_SOURCES = \
	intel_winsys_pipe.c \
	intel_winsys_softpipe.c \
	intel_winsys_i915.c \
	intel_batchbuffer.c \
	intel_swapbuffers.c \
	intel_context.c \
	intel_lock.c \
	intel_screen.c \
	ws_dri_bufmgr.c \
	ws_dri_drmpool.c \
	ws_dri_fencemgr.c \
	ws_dri_mallocpool.c \
	ws_dri_slabpool.c

C_SOURCES = \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

DRIVER_DEFINES = -I$(TOP)/src/mesa/drivers/dri/intel $(shell pkg-config libdrm --atleast-version=2.3.1 \
				&& echo "-DDRM_VBLANK_FLIP=DRM_VBLANK_FLIP")

include ../Makefile.template

intel_tex_layout.o: $(TOP)/src/mesa/drivers/dri/intel/intel_tex_layout.c

symlinks:
