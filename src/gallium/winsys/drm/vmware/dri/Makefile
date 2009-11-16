
TOP = ../../../../../..
include $(TOP)/configs/current

LIBNAME = vmwgfx_dri.so

PIPE_DRIVERS = \
	$(TOP)/src/gallium/state_trackers/dri/libdridrm.a \
	$(TOP)/src/gallium/winsys/drm/vmware/core/libsvgadrm.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/svga/libsvga.a

C_SOURCES = \
	$(COMMON_GALLIUM_SOURCES)

include ../../Makefile.template

symlinks:
