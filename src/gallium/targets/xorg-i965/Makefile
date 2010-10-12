TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = i965g_drv.so

C_SOURCES = \
	intel_target.c \
	intel_xorg.c

DRIVER_DEFINES = \
	-DHAVE_CONFIG_H -DGALLIUM_SOFTPIPE \
	-DGALLIUM_RBUG -DGALLIUM_TRACE

DRIVER_PIPES = \
	$(TOP)/src/gallium/state_trackers/xorg/libxorgtracker.a \
	$(TOP)/src/gallium/winsys/i965/drm/libi965drm.a \
	$(TOP)/src/gallium/drivers/i965/libi965.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(TOP)/src/gallium/winsys/sw/wrapper/libwsw.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a

DRIVER_LINKS = \
	$(shell pkg-config --libs libdrm libdrm_intel)

include ../Makefile.xorg
