# src/mesa/drivers/dri/r300/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

CFLAGS += $(RADEON_CFLAGS)

LIBNAME = r300_dri.so

MINIGLX_SOURCES = server/radeon_dri.c

ifeq ($(USING_EGL), 1)
EGL_SOURCES = server/radeon_egl.c
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
	radeon_texture.c \
	radeon_common_context.c \
	radeon_common.c \
	radeon_dma.c \
	radeon_lock.c \
	radeon_bo_legacy.c \
	radeon_cs_legacy.c \
	radeon_mipmap_tree.c \
	radeon_span.c \
	radeon_fbo.c

DRIVER_SOURCES = \
		 radeon_screen.c \
		 r300_context.c \
		 r300_ioctl.c \
		 r300_cmdbuf.c \
		 r300_state.c \
		 r300_render.c \
		 r300_tex.c \
		 r300_texstate.c \
		 radeon_program.c \
		 radeon_program_alu.c \
		 radeon_program_pair.c \
		 radeon_nqssadce.c \
		 r300_vertprog.c \
		 r300_fragprog_common.c \
		 r300_fragprog.c \
		 r300_fragprog_swizzle.c \
		 r300_fragprog_emit.c \
		 r500_fragprog.c \
		 r500_fragprog_emit.c \
		 r300_shader.c \
		 r300_emit.c \
		 r300_swtcl.c \
		 $(RADEON_COMMON_SOURCES) \
		 $(EGL_SOURCES)

C_SOURCES = $(COMMON_SOURCES) $(DRIVER_SOURCES)

DRIVER_DEFINES = -DCOMPILE_R300 -DR200_MERGED=0 \
	-DRADEON_COMMON=1 -DRADEON_COMMON_FOR_R300 \
#	-DRADEON_BO_TRACK \
	-Wall

SYMLINKS = \
	server/radeon_dri.c \
	server/radeon_dri.h \
	server/radeon.h \
	server/radeon_macros.h \
	server/radeon_reg.h \
	server/radeon_egl.c

COMMON_SYMLINKS = \
	radeon_chipset.h \
	radeon_screen.c \
	radeon_screen.h \
	radeon_span.h \
	radeon_span.c \
	radeon_bo_legacy.c \
	radeon_cs_legacy.c \
	radeon_bo_legacy.h \
	radeon_cs_legacy.h \
	radeon_bocs_wrapper.h \
	radeon_lock.c \
	radeon_lock.h \
	radeon_common.c \
	radeon_common.h \
	radeon_common_context.c \
	radeon_common_context.h \
	radeon_cmdbuf.h \
	radeon_dma.c \
	radeon_dma.h \
	radeon_mipmap_tree.c \
	radeon_mipmap_tree.h \
	radeon_texture.c \
	radeon_texture.h \
	radeon_fbo.c

DRI_LIB_DEPS += $(RADEON_LDFLAGS)

##### TARGETS #####

include ../Makefile.template

server:
	mkdir -p server

$(SYMLINKS): server
	@[ -e $@ ] || ln -sf ../../radeon/$@ server/

$(COMMON_SYMLINKS):
	@[ -e $@ ] || ln -sf ../radeon/$@ ./

symlinks: $(SYMLINKS) $(COMMON_SYMLINKS)
