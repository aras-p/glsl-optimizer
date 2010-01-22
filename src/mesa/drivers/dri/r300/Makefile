# src/mesa/drivers/dri/r300/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

CFLAGS += $(RADEON_CFLAGS)

LIBNAME = r300_dri.so

MINIGLX_SOURCES = server/radeon_dri.c

ifeq ($(RADEON_LDFLAGS),)
CS_SOURCES = radeon_cs_space_drm.c radeon_bo.c radeon_cs.c
endif

COMMON_SOURCES = \
	../../common/driverfuncs.c \
	../common/mm.c \
	../common/utils.c \
	../common/texmem.c \
	../common/vblank.c \
	../common/xmlconfig.c \
	../common/dri_util.c

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
	radeon_span.c \
	radeon_queryobj.c \
	radeon_texture.c \
	radeon_tex_copy.c

DRIVER_SOURCES = \
		 radeon_screen.c \
		 r300_blit.c \
		 r300_context.c \
		 r300_draw.c \
		 r300_cmdbuf.c \
		 r300_state.c \
		 r300_render.c \
		 r300_tex.c \
		 r300_texstate.c \
		 r300_vertprog.c \
		 r300_fragprog_common.c \
		 r300_shader.c \
		 radeon_mesa_to_rc.c \
		 r300_emit.c \
		 r300_swtcl.c \
		 $(RADEON_COMMON_SOURCES) \
		 $(EGL_SOURCES) \
		 $(CS_SOURCES)

C_SOURCES = $(COMMON_SOURCES) $(DRIVER_SOURCES)

DRIVER_DEFINES = -DRADEON_R300
#	-DRADEON_BO_TRACK \

DRI_LIB_DEPS += $(RADEON_LDFLAGS)

SUBDIRS = compiler

EXTRA_MODULES = compiler/libr300compiler.a


##### TARGETS #####

include ../Makefile.template

