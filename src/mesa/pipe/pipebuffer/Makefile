
TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = pipebuffer

	
DRIVER_SOURCES = \
	pb_buffer.c \
	pb_buffer_client.c \
	pb_buffer_handle.c \
	pb_buffer_fenced.c \
	pb_buffer_malloc.c \
	pb_buffer_null.c \
	pb_bufmgr_fenced.c \
	pb_bufmgr_mm.c \
	pb_bufmgr_pool.c

C_SOURCES = \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

include ../Makefile.template

symlinks:

