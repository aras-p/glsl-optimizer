TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = r600_dri.so

PIPE_DRIVERS = \
	$(TOP)/src/gallium/state_trackers/dri/libdridrm.a \
	$(TOP)/src/gallium/winsys/drm/r600/core/libr600winsys.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/r600/libr600.a

C_SOURCES = \
	$(COMMON_GALLIUM_SOURCES) \
	$(DRIVER_SOURCES)

include ../../Makefile.template

symlinks: $(TOP)/$(LIB_DIR)/gallium
	@rm -f $(TOP)/$(LIB_DIR)/gallium/r600_dri.so
