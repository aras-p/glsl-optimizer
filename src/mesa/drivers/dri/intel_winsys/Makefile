
TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = i915tex_dri.so

MINIGLX_SOURCES = server/intel_dri.c

PIPE_DRIVERS = \
	$(TOP)/src/mesa/pipe/softpipe/libsoftpipe.a \
	$(TOP)/src/mesa/pipe/i915simple/libi915simple.a

DRIVER_SOURCES = \
	intel_winsys_pipe.c \
	intel_winsys_softpipe.c \
	intel_winsys_i915.c \
	intel_batchbuffer.c \
	intel_buffers.c \
	intel_context.c \
	intel_lock.c \
	intel_screen.c \
	intel_batchpool.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(COMMON_BM_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

DRIVER_DEFINES = -I../intel $(shell pkg-config libdrm --atleast-version=2.3.1 \
				&& echo "-DDRM_VBLANK_FLIP=DRM_VBLANK_FLIP")

include ../Makefile.template

intel_tex_layout.o: ../intel/intel_tex_layout.c

symlinks:
