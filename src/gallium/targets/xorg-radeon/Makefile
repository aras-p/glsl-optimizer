TOP = ../../../..
include $(TOP)/configs/current

TARGET     = radeong_drv.so
CFILES     = $(wildcard ./*.c)
OBJECTS    = $(patsubst ./%.c,./%.o,$(CFILES))

CFLAGS = -DHAVE_CONFIG_H \
         -g -Wall -Wimplicit-function-declaration -fPIC \
         $(shell pkg-config --cflags pixman-1 xorg-server libdrm xproto) \
         -I$(TOP)/src/gallium/include \
         -I$(TOP)/src/gallium/drivers \
         -I$(TOP)/src/gallium/auxiliary \
         -I${TOP}/src/mesa \
         -I$(TOP)/include \
         -I$(TOP)/src/egl/main

LIBS = \
	$(TOP)/src/gallium/state_trackers/xorg/libxorgtracker.a \
	$(TOP)/src/gallium/winsys/radeon/drm/libradeonwinsys.a \
	$(TOP)/src/gallium/drivers/r300/libr300.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/rbug/librbug.a \
	$(TOP)/src/gallium/drivers/softpipe/libsoftpipe.a \
	$(GALLIUM_AUXILIARIES)

TARGET_STAGING = $(TOP)/$(LIB_DIR)/gallium/$(TARGET)
#############################################

all default: $(TARGET) $(TARGET_STAGING)

$(TARGET): $(OBJECTS) Makefile $(TOP)/src/gallium/state_trackers/xorg/libxorgtracker.a $(LIBS)
	$(TOP)/bin/mklib -noprefix -o $@ \
	$(OBJECTS) $(LIBS) $(shell pkg-config --libs libdrm) -ldrm_radeon

$(TOP)/$(LIB_DIR)/gallium:
	mkdir -p $@

$(TARGET_STAGING): $(TARGET) $(TOP)/$(LIB_DIR)/gallium
	$(INSTALL) $(TARGET) $(TOP)/$(LIB_DIR)/gallium

clean:
	rm -rf $(OBJECTS) $(TARGET)

install:
	$(INSTALL) -d $(DESTDIR)/$(XORG_DRIVER_INSTALL_DIR)
	$(MINSTALL) -m 755 $(TARGET) $(DESTDIR)/$(XORG_DRIVER_INSTALL_DIR)

.PHONY	= all clean install
