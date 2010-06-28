TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = i915_dri.so

PIPE_DRIVERS = \
	$(TOP)/src/gallium/state_trackers/dri/drm/libdridrm.a \
	$(TOP)/src/gallium/winsys/i915/drm/libi915drm.a \
	$(TOP)/src/gallium/drivers/galahad/libgalahad.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/identity/libidentity.a \
	$(TOP)/src/gallium/drivers/i915/libi915.a

C_SOURCES = \
	target.c \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

DRIVER_DEFINES = \
	-DGALLIUM_RBUG -DGALLIUM_TRACE -DGALLIUM_GALAHAD

include ../Makefile.dri

DRI_LIB_DEPS += -ldrm_intel

symlinks:
