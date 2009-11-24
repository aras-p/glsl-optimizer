TARGET     = vmwgfx_drv.so
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
	-I$(TOP)/src/gallium

LIBS = \
	$(TOP)/src/gallium/state_trackers/xorg/libxorgtracker.a \
	$(TOP)/src/gallium/winsys/drm/vmware/core/libsvgadrm.a \
	$(TOP)/src/gallium/drivers/trace/libtrace.a \
	$(TOP)/src/gallium/drivers/svga/libsvga.a \
	$(GALLIUM_AUXILIARIES)

DRIVER_DEFINES = \
	-DHAVE_CONFIG_H

TARGET_STAGING = $(TOP)/$(LIB_DIR)/gallium/$(TARGET)

#############################################



all default: $(TARGET) $(TARGET_STAGING)

$(TARGET): $(OBJECTS) Makefile $(TOP)/src/gallium/state_trackers/xorg/libxorgtracker.a $(LIBS)
	$(TOP)/bin/mklib -noprefix -o $@ \
	$(OBJECTS) $(LIBS) $(shell pkg-config --libs libdrm)

$(TOP)/$(LIB_DIR)/gallium:
	mkdir -p $@

$(TARGET_STAGING): $(TARGET) $(TOP)/$(LIB_DIR)/gallium
	$(INSTALL) $(TARGET) $(TOP)/$(LIB_DIR)/gallium

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
