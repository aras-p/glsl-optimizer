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
	$(TOP)/src/gallium/winsys/drm/nouveau/drm/libnouveaudrm.a \
	$(TOP)/src/gallium/drivers/nv30/libnv30.a \
	$(TOP)/src/gallium/drivers/nv40/libnv40.a \
	$(TOP)/src/gallium/drivers/nv50/libnv50.a \
	$(TOP)/src/gallium/drivers/nouveau/libnouveau.a \
	$(GALLIUM_AUXILIARIES)

DRIVER_DEFINES = \
	-DHAVE_CONFIG_H


#############################################



all default: $(TARGET)

$(TARGET): $(OBJECTS) Makefile $(TOP)/src/gallium/state_trackers/xorg/libxorgtracker.a $(LIBS)
	$(TOP)/bin/mklib -noprefix -o $@ \
	$(OBJECTS) $(LIBS) $(shell pkg-config --libs libdrm) -ldrm_nouveau

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
