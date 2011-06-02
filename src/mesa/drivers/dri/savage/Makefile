# src/mesa/drivers/dri/r128/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = savage_dri.so

include ../Makefile.defines

DRIVER_SOURCES = \
	savage_xmesa.c \
	savagedd.c \
	savagestate.c \
	savagetex.c \
	savagetris.c \
	savagerender.c \
	savageioctl.c \
	savagespan.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES) 

ASM_SOURCES = 


include ../Makefile.targets

