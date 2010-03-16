TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = xlib

LIBRARY_INCLUDES = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	$(X_CFLAGS)

C_SOURCES = \
	glx_api.c \
	glx_getproc.c \
	glx_usefont.c \
	xm_api.c \
	xm_st.c

include ../../../Makefile.template
