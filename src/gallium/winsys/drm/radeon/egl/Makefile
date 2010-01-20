TOP = ../../../../../..
GALLIUMDIR = ../../../..
include $(TOP)/configs/current

LIBNAME = egl_r300.so

PIPE_DRIVERS = \
	$(TOP)/src/gallium/state_trackers/egl/libegldrm.a \
	$(GALLIUMDIR)/winsys/drm/radeon/core/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/r300/libr300.a

DRIVER_EXTRAS = -lm -lpthread -ldrm_radeon

OBJECTS = dummy.o

default: $(TOP)/$(LIB_DIR)/$(LIBNAME)

$(TOP)/$(LIB_DIR)/$(LIBNAME): $(LIBNAME)
	@mkdir -p $(TOP)/$(LIB_DIR)
	$(INSTALL) $(LIBNAME) $(TOP)/$(LIB_DIR)

$(LIBNAME): $(OBJECTS) $(GALLIUM_AUXILIARIES) $(PIPE_DRIVERS) Makefile
	$(MKLIB) -noprefix -o $@ $(OBJECTS) \
		-Wl,--whole-archive $(PIPE_DRIVERS) -Wl,--no-whole-archive \
		-Wl,--start-group $(GALLIUM_AUXILIARIES) -Wl,--end-group \
                 $(DRI_LIB_DEPS) $(DRIVER_EXTRAS)

clean:
	-rm -f *.o *.so *~

depend:

symlinks:

install: $(LIBNAME)
	$(MINSTALL) -m 755 $(LIBNAME) $(DESTDIR)$(INSTALL_DIR)/$(LIB_DIR)
