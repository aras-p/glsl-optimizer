# src/egl/wayland/wayland-drm/Makefile

TOP = ../../../..
include $(TOP)/configs/current

GEN_SOURCES = wayland-drm-protocol.c

GEN_HEADERS = wayland-drm-client-protocol.h wayland-drm-server-protocol.h

wayland_drm_SOURCES = wayland-drm.c $(GEN_SOURCES)
wayland_drm_OBJECTS = $(wayland_drm_SOURCES:.c=.o)

wayland_drm_INCLUDES = \
		$(WAYLAND_CFLAGS) \
		-I$(TOP)/src/egl/main \
		-I$(TOP)/include

# Generate protocol sources
prefix=$(shell $(PKG_CONFIG) --variable=prefix wayland-server)
exec_prefx=$(shell $(PKG_CONFIG) --variable=exec_prefix wayland-server)
wayland_protocoldir = $(PWD)/protocol
wayland_scanner=$(exec_prefix)/bin/wayland-scanner

default: depend libwayland-drm.a $(GEN_SOURCES) $(GEN_HEADERS)

libwayland-drm.a: $(wayland_drm_OBJECTS) Makefile
	$(MKLIB) -o wayland-drm -static $(wayland_drm_OBJECTS)
 
depend: 
	rm -f depend
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(wayland_drm_INCLUDES) $(wayland_drm_SOURCES) 2> /dev/null

clean:
	rm -rf libwayland-drm.a $(wayland_drm_OBJECTS) \
		$(GEN_SOURCES) $(GEN_HEADERS)
	rm -f depend depend.bak

install:
	@echo -n ""

$(wayland_drm_OBJECTS): %.o: %.c $(GEN_HEADERS)
	$(CC) -c $(wayland_drm_INCLUDES) $(CFLAGS) $< -o $@

include $(prefix)/share/aclocal/wayland-scanner.mk

sinclude depend
