
TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = radeon_dri.so

MINIGLX_SOURCES =

PIPE_DRIVERS = \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/r300/libr300.a

DRIVER_SOURCES = \
	radeon_buffer.c \
	radeon_context.c \
	radeon_drm.c \
	radeon_r300.c \
	radeon_screen.c \
	radeon_winsys_softpipe.c

C_SOURCES = \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

DRIVER_DEFINES = -I../../../drivers/r300

include ../Makefile.template

DRI_LIB_DEPS += -ldrm_radeon

symlinks:
