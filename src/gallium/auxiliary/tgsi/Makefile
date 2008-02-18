
TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = tgsi

DRIVER_SOURCES = \
	exec/tgsi_exec.c \
	exec/tgsi_sse2.c \
	util/tgsi_build.c \
	util/tgsi_dump.c \
	util/tgsi_parse.c \
	util/tgsi_transform.c \
	util/tgsi_util.c

C_SOURCES = \
	$(DRIVER_SOURCES)

ASM_SOURCES = 

include ../../Makefile.template

symlinks:

