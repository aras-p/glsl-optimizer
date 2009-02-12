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

CORE_MESA = $(TOP)/src/mesa/libmesa.a $(TOP)/src/mesa/libglapi.a


.PHONY: osmesa8
.PHONY: osmesa16


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@


default: $(TOP)/$(LIB_DIR)/$(OSMESA_LIB_NAME)
	@ if [ "${DRIVER_DIRS}" = "osmesa" ] ; then \
		$(MAKE) osmesa16 ; \
	else \
		$(MAKE) osmesa8 ; \
	fi




# The normal libOSMesa is used in conjuction with libGL
osmesa8: $(TOP)/$(LIB_DIR)/$(OSMESA_LIB_NAME)

$(TOP)/$(LIB_DIR)/$(OSMESA_LIB_NAME): $(OBJECTS)
	$(MKLIB) -o $(OSMESA_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(MESA_MAJOR) -minor $(MESA_MINOR) -patch $(MESA_TINY) \
		-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
		-id $(INSTALL_LIB_DIR)/lib$(OSMESA_LIB).$(MESA_MAJOR).dylib \
		$(OSMESA_LIB_DEPS) $(OBJECTS)




# The libOSMesa16/libOSMesa32 libraries do not use libGL but rather are built
# with all the other Mesa sources (compiled with -DCHAN_BITS=16/32
osmesa16: $(OBJECTS) $(CORE_MESA)
	$(MKLIB) -o $(OSMESA_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(MESA_MAJOR) -minor $(MESA_MINOR) -patch $(MESA_TINY) \
		-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
		-id $(INSTALL_LIB_DIR)/lib$(OSMESA_LIB).$(MESA_MAJOR).dylib \
		$(OSMESA_LIB_DEPS) $(OBJECTS) $(CORE_MESA)



clean:
	-rm -f *.o *~


# XXX todo install rule?
