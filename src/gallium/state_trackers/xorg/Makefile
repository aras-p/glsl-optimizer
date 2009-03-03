TARGET     = libxorgtracker.a
CFILES     = $(wildcard ./*.c)
OBJECTS    = $(patsubst ./%.c,./%.o,$(CFILES))
GALLIUMDIR = ../..
TOP        = ../../../..

include $(TOP)/configs/current

CFLAGS = -DHAVE_CONFIG_H \
         -g -Wall -Wimplicit-function-declaration -fPIC \
         $(shell pkg-config --cflags pixman-1 xorg-server libdrm xproto) \
         -I$(GALLIUMDIR)/include \
         -I$(GALLIUMDIR)/auxiliary \
         -I$(TOP)/src/mesa/drivers/dri/common \
         -I$(TOP)/src/mesa \
         -I$(TOP)/include \
         -I$(TOP)/src/egl/main

#############################################

.PHONY	= all clean

all: $(TARGET)

$(TARGET): $(OBJECTS)
	ar rcs $(TARGET) $(OBJECTS)

clean:
	rm -rf $(OBJECTS) $(TARGET)
