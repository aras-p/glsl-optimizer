# src/mesa/drivers/fbdev/Makefile for libGL.so

TOP = ../../../..

include $(TOP)/configs/current


SOURCES = glfbdev.c

OBJECTS = $(SOURCES:.c=.o)

INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main

CORE_MESA = $(TOP)/src/mesa/libmesa.a $(TOP)/src/mesa/libglapi.a


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@


default: $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME)


$(TOP)/$(LIB_DIR)/$(GL_LIB_NAME): $(CORE_MESA) $(OBJECTS)
	@ $(MKLIB) -o $(GL_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(MESA_MAJOR) -minor $(MESA_MINOR) -patch $(MESA_TINY) \
		-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
		$(CORE_MESA) $(OBJECTS) $(GL_LIB_DEPS)


clean:
	-rm -f $(OBJECTS)
