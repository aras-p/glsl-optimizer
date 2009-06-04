# src/mesa/drivers/osmesa/Makefile for libOSMesa.so

# Note that we may generate libOSMesa.so or libOSMesa16.so or libOSMesa32.so
# with this Makefile


TOP = ../../../..

include $(TOP)/configs/current



SOURCES = osmesa.c

OBJECTS = $(SOURCES:.c=.o)

INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main

# Standalone osmesa needs to be linked with core Mesa APIs
ifeq ($(DRIVER_DIRS), osmesa)
CORE_MESA = $(TOP)/src/mesa/libmesa.a $(TOP)/src/mesa/libglapi.a
else
CORE_MESA =
endif


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@


default: $(TOP)/$(LIB_DIR)/$(OSMESA_LIB_NAME)


# libOSMesa can be used in conjuction with libGL or with all other Mesa
# sources. We can also build libOSMesa16/libOSMesa32 by setting
# -DCHAN_BITS=16/32.
$(TOP)/$(LIB_DIR)/$(OSMESA_LIB_NAME): $(OBJECTS) $(CORE_MESA)
	$(MKLIB) -o $(OSMESA_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(MESA_MAJOR) -minor $(MESA_MINOR) -patch $(MESA_TINY) \
		-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
		-id $(INSTALL_LIB_DIR)/lib$(OSMESA_LIB).$(MESA_MAJOR).dylib \
		$(OSMESA_LIB_DEPS) $(OBJECTS) $(CORE_MESA)



clean:
	-rm -f *.o *~


# XXX todo install rule?
