# src/mesa/drivers/dri/fb/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = fb_dri.so

DRIVER_SOURCES = fb_dri.c

C_SOURCES = \
	$(DRIVER_SOURCES) \
	$(DRI_SOURCES) 


# Include directories
INCLUDE_DIRS = \
	-I. \
	-I../common \
	-Iserver \
	-I$(TOP)/src/glx/mini \
	-I$(TOP)/include \
	-I$(DRM_SOURCE_PATH)/shared \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa/math \
	-I$(TOP)/src/mesa/transform \
	-I$(TOP)/src/mesa/shader \
	-I$(TOP)/src/mesa/swrast \
	-I$(TOP)/src/mesa/swrast_setup

# Core Mesa objects
MESA_MODULES = $(TOP)/src/mesa/mesa.a

# Libraries that the driver shared lib depends on
LIB_DEPS = -lm -lpthread -lc


ASM_SOURCES = 

OBJECTS = $(C_SOURCES:.c=.o) \
	  $(ASM_SOURCES:.S=.o) 


##### RULES #####

.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $(DEFINES)  $< -o $@


##### TARGETS #####

default: depend $(LIB_DIR)/$(LIBNAME)


$(LIB_DIR)/$(LIBNAME): $(OBJECTS) $(MESA_MODULES) $(WINOBJ) Makefile
	CC=$(CC) CXX=$(CXX) $(TOP)/bin/mklib -o $(LIBNAME) -noprefix -install $(LIB_DIR) \
		$(WINLIB) $(LIB_DEPS) $(WINOBJ) $(MESA_MODULES) $(OBJECTS)


depend: $(C_SOURCES) $(ASM_SOURCES)
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDE_DIRS) $(C_SOURCES) $(ASM_SOURCES) \
		> /dev/null 2>&1


# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`


clean:
	-rm -f *.o server/*.o


include depend
