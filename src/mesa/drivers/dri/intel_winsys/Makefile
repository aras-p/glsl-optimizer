
TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = i915tex_dri.so

MINIGLX_SOURCES = server/intel_dri.c

PIPE_DRIVERS = \
	$(TOP)/src/mesa/pipe/softpipe/libsoftpipe.a \
	$(TOP)/src/mesa/pipe/i915simple/libi915simple.a

DRIVER_SOURCES = \
	intel_pipe_i915simple.c \
	intel_pipe_softpipe.c \
	intel_batchbuffer.c \
	intel_tex_layout.c \
	intel_buffers.c \
	intel_read_draw_buffer.c \
	intel_blit.c \
	intel_context.c \
	intel_ioctl.c \
	intel_screen.c \
	intel_surface.c \
	intel_fbo.c \
	intel_depthstencil.c \
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
