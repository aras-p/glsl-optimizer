TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = vatracker

VA_MAJOR = 0
VA_MINOR = 3
LIBRARY_DEFINES = -DVER_MAJOR=$(VA_MAJOR) -DVER_MINOR=$(VA_MINOR) $(STATE_TRACKER_DEFINES)

LIBRARY_INCLUDES = \
	$(shell pkg-config --cflags-only-I va) \
	-I$(TOP)/src/gallium/winsys/g3dvl

C_SOURCES = htab.c \
	    ftab.c 


include ../../Makefile.template

