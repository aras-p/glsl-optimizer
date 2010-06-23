TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = i965_dri.so

PIPE_DRIVERS = \
	$(TOP)/src/gallium/state_trackers/dri/drm/libdridrm.a \
	$(TOP)/src/gallium/winsys/i965/drm/libi965drm.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(TOP)/src/gallium/winsys/sw/wrapper/libwsw.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/i965/libi965.a

C_SOURCES = \
	target.c \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

DRIVER_DEFINES = \
	-DGALLIUM_SOFTPIPE -DGALLIUM_RBUG -DGALLIUM_TRACE

include ../Makefile.dri

DRI_LIB_DEPS += -ldrm_intel

symlinks:
