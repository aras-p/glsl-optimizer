# src/egl/drivers/demo/Makefile

TOP = ../../../..
include $(TOP)/configs/current


INCLUDE_DIRS = -I$(TOP)/include -I$(TOP)/src/egl/main $(X11_INCLUDES)


SOURCES = demo.c

OBJECTS = $(SOURCES:.c=.o)


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: $(TOP)/$(LIB_DIR)/demodriver.so


$(TOP)/$(LIB_DIR)/demodriver.so: $(OBJECTS)
	$(MKLIB) -o demodriver.so -noprefix -linker '$(CC)' \
		-ldflags '$(LDFLAGS)' -install $(TOP)/$(LIB_DIR) \
		$(OBJECTS)

install:

clean:
	-rm -f *.o
	-rm -f *.so
