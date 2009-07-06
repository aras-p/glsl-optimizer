# src/mesa/drivers/dri/r200/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

CFLAGS += $(RADEON_CFLAGS)

LIBNAME = r200_dri.so

MINIGLX_SOURCES = server/radeon_dri.c 

ifeq ($(USING_EGL), 1)
EGL_SOURCES = server/radeon_egl.c
endif

ifeq ($(RADEON_LDFLAGS),)
CS_SOURCES = radeon_cs_space_drm.c
endif

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
		 radeon_screen.c \
		 $(EGL_SOURCES) \
		 $(RADEON_COMMON_SOURCES) \
		 $(CS_SOURCES)

C_SOURCES = $(COMMON_SOURCES) $(DRIVER_SOURCES)

X86_SOURCES = 

DRIVER_DEFINES = -DRADEON_COMMON=1 -DRADEON_COMMON_FOR_R200

SYMLINKS = \
	server/radeon_egl.c  \
	server/radeon_dri.c  \
	server/radeon_dri.h \
	server/radeon.h \
	server/radeon_macros.h \
	server/radeon_reg.h

COMMON_SYMLINKS = \
	radeon_chipset.h \
	radeon_screen.c \
	radeon_screen.h \
	radeon_bo_legacy.c \
	radeon_cs_legacy.c \
	radeon_bo_legacy.h \
	radeon_cs_legacy.h \
	radeon_bocs_wrapper.h \
	radeon_span.h \
	radeon_span.c \
	radeon_lock.c \
	radeon_lock.h \
	radeon_common.c \
	radeon_common_context.c \
	radeon_common_context.h \
	radeon_common.h \
	radeon_cmdbuf.h \
	radeon_mipmap_tree.c \
	radeon_mipmap_tree.h \
	radeon_texture.c \
	radeon_texture.h \
	radeon_dma.c \
	radeon_dma.h \
	radeon_fbo.c \
	$(CS_SOURCES)

DRI_LIB_DEPS += $(RADEON_LDFLAGS)

##### TARGETS #####


include ../Makefile.template

#INCLUDES += -I../radeon/server

server:
	mkdir -p server

$(SYMLINKS): server
	@[ -e $@ ] || ln -sf ../../radeon/$@ server/


$(COMMON_SYMLINKS):
	@[ -e $@ ] || ln -sf ../radeon/$@ ./

symlinks: $(SYMLINKS) $(COMMON_SYMLINKS)

