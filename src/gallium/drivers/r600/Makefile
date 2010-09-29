TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = r600

LIBRARY_INCLUDES = \
	$(shell pkg-config libdrm --cflags-only-I)

C_SOURCES = \
	r600_buffer.c \
	r600_state2.c \
	evergreen_state.c \
	r600_shader.c \
	r600_helper.c \
	r600_resource.c \
	r600_texture.c \
	r600_asm.c \
	r700_asm.c \
	eg_asm.c

include ../../Makefile.template
