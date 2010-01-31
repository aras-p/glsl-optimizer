# src/mesa/drivers/dri/r200/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

CFLAGS += $(RADEON_CFLAGS)

LIBNAME = r200_dri.so

MINIGLX_SOURCES = server/radeon_dri.c 

ifeq ($(RADEON_LDFLAGS),)
CS_SOURCES = radeon_cs_space_drm.c radeon_bo.c radeon_cs.c
endif

RADEON_COMMON_SOURCES = \
	radeon_bo_legacy.c \
	radeon_common_context.c \
	radeon_common.c \
	radeon_cs_legacy.c \
	radeon_dma.c \
	radeon_debug.c \
	radeon_fbo.c \
	radeon_lock.c \
	radeon_mipmap_tree.c \
	radeon_queryobj.c \
	radeon_span.c \
	radeon_texture.c \
	radeon_tex_copy.c

DRIVER_SOURCES = r200_context.c \
		 r200_ioctl.c \
		 r200_state.c \
		 r200_state_init.c \
		 r200_cmdbuf.c \
		 r200_pixel.c \
		 r200_tex.c \
		 r200_texstate.c \
		 r200_tcl.c \
		 r200_swtcl.c \
		 r200_maos.c \
		 r200_sanity.c \
		 r200_fragshader.c \
		 r200_vertprog.c \
		 r200_blit.c \
		 radeon_screen.c \
		 $(EGL_SOURCES) \
		 $(RADEON_COMMON_SOURCES) \
		 $(CS_SOURCES)

C_SOURCES = $(COMMON_SOURCES) $(DRIVER_SOURCES)

X86_SOURCES = 

DRIVER_DEFINES = -DRADEON_R200

DRI_LIB_DEPS += $(RADEON_LDFLAGS)

##### TARGETS #####


include ../Makefile.template

#INCLUDES += -I../radeon/server

