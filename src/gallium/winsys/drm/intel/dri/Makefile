TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = i915_dri.so

PIPE_DRIVERS = \
	$(TOP)/src/gallium/state_trackers/dri/libdridrm.a \
	$(TOP)/src/gallium/winsys/drm/intel/gem/libinteldrm.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/i915simple/libi915simple.a


DRIVER_SOURCES =

C_SOURCES = \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

include ../../Makefile.template

DRI_LIB_DEPS += -ldrm_intel

symlinks: $(TOP)/$(LIB_DIR)/gallium
	@rm -f $(TOP)/$(LIB_DIR)/gallium/i965_dri.so
	ln -s i915_dri.so $(TOP)/$(LIB_DIR)/gallium/i965_dri.so
