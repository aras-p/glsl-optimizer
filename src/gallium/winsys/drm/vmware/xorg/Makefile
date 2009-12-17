TOP        = ../../../../../..

include $(TOP)/configs/current

TARGET = vmwgfx_drv.so

CFILES = \
	vmw_xorg.c \
	vmw_video.c \
	vmw_ioctl.c \
	vmw_screen.c

OBJECTS = $(patsubst %.c,%.o,$(CFILES))

INCLUDES = \
	$(shell pkg-config --cflags-only-I pixman-1 xorg-server libdrm xproto) \
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

LINKS = \
	$(shell pkg-config --libs --silence-errors libkms) \
	$(shell pkg-config --libs libdrm)

DRIVER_DEFINES = \
	-std=gnu99 \
	-DHAVE_CONFIG_H

TARGET_STAGING = $(TOP)/$(LIB_DIR)/gallium/$(TARGET)

#############################################



all default: $(TARGET) $(TARGET_STAGING)

$(TARGET): $(OBJECTS) Makefile $(LIBS)
	$(MKLIB) -noprefix -o $@ $(OBJECTS) $(LIBS) $(LINKS)

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
