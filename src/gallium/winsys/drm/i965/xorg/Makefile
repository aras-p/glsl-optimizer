TARGET     = modesetting_drv.so
CFILES     = $(wildcard ./*.c)
OBJECTS    = $(patsubst ./%.c,./%.o,$(CFILES))
TOP        = ../../../../../..

include $(TOP)/configs/current

INCLUDES = \
	$(shell pkg-config --cflags-only-I pixman-1 xorg-server libdrm xproto) \
	-I../gem \
	-I$(TOP)/src/gallium/include \
	-I$(TOP)/src/gallium/drivers \
	-I$(TOP)/src/gallium/auxiliary \
	-I$(TOP)/src/mesa \
	-I$(TOP)/include \
	-I$(TOP)/src/egl/main

LIBS = \
	$(TOP)/src/gallium/state_trackers/xorg/libxorgtracker.a \
	$(TOP)/src/gallium/winsys/drm/intel/gem/libinteldrm.a \
	$(TOP)/src/gallium/drivers/i915/libi915.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(GALLIUM_AUXILIARIES)

DRIVER_DEFINES = \
	-DHAVE_CONFIG_H


#############################################



all default: $(TARGET)

$(TARGET): $(OBJECTS) Makefile $(TOP)/src/gallium/state_trackers/xorg/libxorgtracker.a $(LIBS)
	$(TOP)/bin/mklib -noprefix -o $@ \
	$(OBJECTS) $(LIBS) $(shell pkg-config --libs libdrm) -ldrm_intel

clean:
	rm -rf $(OBJECTS) $(TARGET)

install:
	$(INSTALL) -d $(DESTDIR)/$(XORG_DRIVER_INSTALL_DIR)
	$(MINSTALL) -m 755 $(TARGET) $(DESTDIR)/$(XORG_DRIVER_INSTALL_DIR)


##############################################


.c.o:
	$(CC) -c $(CFLAGS) $(INCLUDES) $(DRIVER_DEFINES) $< -o $@


##############################################

.PHONY	= all clean install
