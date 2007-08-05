
TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = i915tex_dri.so

MINIGLX_SOURCES = server/intel_dri.c

PIPE_DRIVERS = \
	$(TOP)/src/mesa/pipe/softpipe/libsoftpipe.a

DRIVER_SOURCES = \
	intel_softpipe.c \
	intel_buffer_objects.c \
	intel_batchbuffer.c \
	intel_mipmap_tree.c \
	intel_tex_layout.c \
	intel_tex_image.c \
	intel_tex_subimage.c \
	intel_tex_copy.c \
	intel_tex_validate.c \
	intel_tex_format.c \
	intel_tex.c \
	intel_buffers.c \
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
