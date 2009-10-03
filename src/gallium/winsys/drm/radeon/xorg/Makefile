TARGET     = modesetting_drv.so
CFILES     = $(wildcard ./*.c)
OBJECTS    = $(patsubst ./%.c,./%.o,$(CFILES))
GALLIUMDIR = ../../../..
TOP        = ../../../../../..

include ${TOP}/configs/current

CFLAGS = -DHAVE_CONFIG_H \
         -g -Wall -Wimplicit-function-declaration -fPIC \
         $(shell pkg-config --cflags pixman-1 xorg-server libdrm xproto) \
         -I${GALLIUMDIR}/include \
         -I${GALLIUMDIR}/drivers \
         -I${GALLIUMDIR}/auxiliary \
         -I${TOP}/src/mesa \
         -I$(TOP)/include \
         -I$(TOP)/src/egl/main

LIBS = \
	$(GALLIUMDIR)/state_trackers/xorg/libxorgtracker.a \
	$(GALLIUMDIR)/winsys/drm/radeon/core/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/r300/libr300.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(GALLIUM_AUXILIARIES)

#############################################



all default: $(TARGET)

$(TARGET): $(OBJECTS) Makefile $(GALLIUMDIR)/state_trackers/xorg/libxorgtracker.a
	$(TOP)/bin/mklib -noprefix -o $@ \
	$(OBJECTS) $(LIBS) $(shell pkg-config --libs libdrm) -ldrm_radeon

clean:
	rm -rf $(OBJECTS) $(TARGET)

install:
	$(INSTALL) -d $(DESTDIR)/$(XORG_DRIVER_INSTALL_DIR)
	$(MINSTALL) -m 755 $(TARGET) $(DESTDIR)/$(XORG_DRIVER_INSTALL_DIR)

.PHONY	= all clean install
