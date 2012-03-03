# Meta-driver which combines whichever software rasterizers have been
# built into a single convenience library.

TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = noop

C_SOURCES = \
	noop_pipe.c \
	noop_state.c

include ../../Makefile.template

# FIXME: Remove when this driver is converted to automake.
all: default
