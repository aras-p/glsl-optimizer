# src/egl/drivers/demo/Makefile

TOP = ../../../..
include $(TOP)/configs/current


INCLUDE_DIRS = -I$(TOP)/include -I$(TOP)/src/egl/main


SOURCES = demo.c

OBJECTS = $(SOURCES:.c=.o)


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: $(TOP)/$(LIB_DIR)/demodriver.so


$(TOP)/$(LIB_DIR)/demodriver.so: $(OBJECTS)
	$(TOP)/bin/mklib -o demodriver.so -noprefix $(OBJECTS)



clean:
	rm -f *.o
	rm -f *.so
