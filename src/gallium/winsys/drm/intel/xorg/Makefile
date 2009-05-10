TARGET     = modesetting_drv.so
CFILES     = $(wildcard ./*.c)
OBJECTS    = $(patsubst ./%.c,./%.o,$(CFILES))
GALLIUMDIR = ../../../..
TOP        = ../../../../../..

include ${TOP}/configs/current

CFLAGS = -DHAVE_CONFIG_H \
         -g -Wall -Wimplicit-function-declaration -fPIC \
         $(shell pkg-config --cflags pixman-1 xorg-server libdrm xproto) \
	 -I../gem \
         -I${GALLIUMDIR}/include \
         -I${GALLIUMDIR}/drivers \
         -I${GALLIUMDIR}/auxiliary \
         -I${TOP}/src/mesa \
         -I$(TOP)/include \
         -I$(TOP)/src/egl/main

LIBS = \
	$(GALLIUMDIR)/state_trackers/xorg/libxorgtracker.a \
	$(GALLIUMDIR)/winsys/drm/intel/gem/libinteldrm.a \
	$(TOP)/src/gallium/drivers/i915simple/libi915simple.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(GALLIUM_AUXILIARIES)

#############################################



all default: $(TARGET)

$(TARGET): $(OBJECTS) Makefile $(GALLIUMDIR)/state_trackers/xorg/libxorgtracker.a $(LIBS)
	$(TOP)/bin/mklib -noprefix -o $@ \
	$(OBJECTS) $(LIBS) $(shell pkg-config --libs libdrm) -ldrm_intel

clean:
	rm -rf $(OBJECTS) $(TARGET)

install:
	$(INSTALL) -d $(DESTDIR)/$(XORG_DRIVER_INSTALL_DIR)
	$(INSTALL) -m 755 $(TARGET) $(DESTDIR)/$(XORG_DRIVER_INSTALL_DIR)

.PHONY	= all clean install
