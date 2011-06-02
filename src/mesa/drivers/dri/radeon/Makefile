# src/mesa/drivers/dri/radeon/Makefile
# Note, this Makefile requires GNU make

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = radeon_dri.so

include ../Makefile.defines

ifeq ($(RADEON_LDFLAGS),)
CS_SOURCES = radeon_cs_space_drm.c radeon_bo.c radeon_cs.c
endif

RADEON_COMMON_SOURCES = \
	radeon_bo_legacy.c \
	radeon_buffer_objects.c \
	radeon_common_context.c \
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
	radeon_context.c \
	radeon_ioctl.c \
	radeon_screen.c \
	radeon_state.c \
	radeon_state_init.c \
	radeon_tex.c \
	radeon_texstate.c \
	radeon_tcl.c \
	radeon_swtcl.c \
	radeon_maos.c \
	radeon_sanity.c \
	radeon_blit.c \
	$(RADEON_COMMON_SOURCES)

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES) \
	$(CS_SOURCES)

DRIVER_DEFINES = -DRADEON_R100

INCLUDES += $(RADEON_CFLAGS)
DRI_LIB_DEPS += $(RADEON_LDFLAGS)

X86_SOURCES = 

include ../Makefile.targets

