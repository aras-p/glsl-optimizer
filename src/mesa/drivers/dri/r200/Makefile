# src/mesa/drivers/dri/r200/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = r200_dri.so

MINIGLX_SOURCES = server/radeon_dri.c 


COMMON_SOURCES = \
	../../common/driverfuncs.c \
	../common/mm.c \
	../common/utils.c \
	../common/texmem.c \
	../common/vblank.c \
	../common/xmlconfig.c \
	../common/dri_util.c \
	../common/drirenderbuffer.c \
	../common/glcontextmodes.c

DRIVER_SOURCES = r200_context.c \
		 r200_ioctl.c \
		 r200_lock.c \
		 r200_screen.c \
		 r200_state.c \
		 r200_state_init.c \
		 r200_cmdbuf.c \
		 r200_pixel.c \
		 r200_tex.c \
		 r200_texmem.c \
		 r200_texstate.c \
		 r200_tcl.c \
		 r200_swtcl.c \
		 r200_span.c \
		 r200_maos.c \
		 r200_sanity.c \
		 r200_vtxfmt.c \
		 r200_vtxfmt_c.c \
		 r200_vtxfmt_sse.c \
		 r200_vtxfmt_x86.c 

C_SOURCES = $(COMMON_SOURCES) $(DRIVER_SOURCES)

X86_SOURCES = r200_vtxtmp_x86.S

SYMLINKS = \
	server/radeon_dri.c  \
		  server/radeon_dri.h \
		  server/radeon.h \
		  server/radeon_macros.h \
		  server/radeon_reg.h

##### TARGETS #####


include ../Makefile.template

#INCLUDES += -I../radeon/server

server:
	mkdir -p server

$(SYMLINKS): server
	@[ -e $@ ] || ln -sf ../../radeon/$@ server/

symlinks: $(SYMLINKS)

