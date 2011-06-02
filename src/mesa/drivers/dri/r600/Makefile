# src/mesa/drivers/dri/r300/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = r600_dri.so

include ../Makefile.defines

ifeq ($(RADEON_LDFLAGS),)
CS_SOURCES = radeon_cs_space_drm.c radeon_bo.c radeon_cs.c
endif

COMMON_SOURCES = \
	../../common/driverfuncs.c \
	../common/drirenderbuffer.c \
	../common/utils.c \
	../common/texmem.c \
	../common/vblank.c \
	../common/xmlconfig.c \
	../common/dri_util.c

RADEON_COMMON_SOURCES = \
	radeon_bo_legacy.c \
	radeon_common_context.c \
	radeon_buffer_objects.c \
	radeon_common.c \
	radeon_cs_legacy.c \
	radeon_dma.c \
	radeon_debug.c \
	radeon_fbo.c \
	radeon_lock.c \
	radeon_mipmap_tree.c \
	radeon_pixel_read.c \
	radeon_queryobj.c \
	radeon_span.c \
	radeon_texture.c \
	radeon_tex_copy.c \
	radeon_tex_getimage.c \
	radeon_tile.c

DRIVER_SOURCES = \
		 radeon_screen.c \
		 r600_context.c \
		 r600_cmdbuf.c \
		 r600_emit.c       \
		 r700_assembler.c  \
		 r700_fragprog.c \
		 r700_vertprog.c \
		 r700_shader.c \
		 r700_shaderinst.c \
		 r700_ioctl.c \
		 r700_oglprog.c \
		 r700_chip.c     \
		 r700_state.c    \
		 r700_clear.c    \
		 r700_render.c   \
		 r600_tex.c      \
		 r600_texstate.c      \
		 r600_blit.c     \
		 r700_debug.c    \
         evergreen_context.c    \
         evergreen_state.c      \
         evergreen_tex.c        \
         evergreen_ioctl.c      \
         evergreen_render.c     \
         evergreen_chip.c       \
         evergreen_vertprog.c   \
         evergreen_fragprog.c   \
         evergreen_oglprog.c    \
		 evergreen_blit.c     \
		 $(RADEON_COMMON_SOURCES) \
		 $(EGL_SOURCES) \
		 $(CS_SOURCES)

C_SOURCES = $(COMMON_SOURCES) $(DRIVER_SOURCES)

DRIVER_DEFINES = -DRADEON_R600
#	-DRADEON_BO_TRACK \

INCLUDES += $(RADEON_CFLAGS)
DRI_LIB_DEPS += $(RADEON_LDFLAGS)

##### TARGETS #####

include ../Makefile.targets

