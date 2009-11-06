TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = i965_dri.so

PIPE_DRIVERS = \
	$(TOP)/src/gallium/state_trackers/dri/libdridrm.a \
	$(TOP)/src/gallium/winsys/drm/i965/gem/libi965drm.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/identity/libidentity.a \
	$(TOP)/src/gallium/drivers/i965/libi965.a


DRIVER_SOURCES =

C_SOURCES = \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

include ../../Makefile.template

DRI_LIB_DEPS += -ldrm_intel

symlinks: $(TOP)/$(LIB_DIR)/gallium
	@rm -f $(TOP)/$(LIB_DIR)/gallium/i965_dri.so
