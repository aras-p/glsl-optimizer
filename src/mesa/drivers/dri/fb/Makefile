# src/mesa/drivers/dri/fb/Makefile

TOP = ../../../../..
include $(TOP)/configs/default

SHARED_INCLUDES = $(INCLUDE_DIRS) -I. -I../common -Iserver
MINIGLX_INCLUDES = -I$(TOP)/src/glx/mini

MESA_MODULES = $(TOP)/src/mesa/mesa.a

DRIVER_SOURCES = fb_dri.c


C_SOURCES = $(DRIVER_SOURCES) \
	    $(DRI_SOURCES) 

ASM_SOURCES = 

OBJECTS = $(C_SOURCES:.c=.o) \
	  $(ASM_SOURCES:.S=.o) 


### Include directories

INCLUDE_DIRS = \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	-I$(TOP)/src/mesa/math \
	-I$(TOP)/src/mesa/transform \
	-I$(TOP)/src/mesa/swrast \
	-I$(TOP)/src/mesa/swrast_setup


##### RULES #####

.c.o:
	$(CC) -c $(SHARED_INCLUDES) $(MINIGLX_INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(CC) -c $(SHARED_INCLUDES) $(MINIGLX_INCLUDES) $(CFLAGS) $(DEFINES)  $< -o $@


##### TARGETS #####

default: depend fb_dri.so

fb_dri.so:  $(OBJECTS) $(MESA_MODULES) $(WINOBJ) Makefile.solo
	rm -f $@ && gcc -o $@ -shared $(OBJECTS) $(MESA_MODULES) $(WINOBJ) $(WINLIB) -lc -lm
	rm -f $(LIB_DIR)/fb_dri.so && \
	install fb_dri.so $(LIB_DIR)/fb_dri.so

$(LIB_DIR)/fb_dri.so:	fb_dri.so
	rm -f $(LIB_DIR)/fb_dri.so && \
	install fb_dri.so $(LIB_DIR)/fb_dri.so

# Run 'make -f Makefile.solo dep' to update the dependencies if you change
# what's included by any source file.
depend: $(C_SOURCES) $(ASM_SOURCES)
	touch depend
	makedepend -fdepend -Y $(SHARED_INCLUDES) \
		$(C_SOURCES) $(ASM_SOURCES) >& /dev/null


# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`


# Remove .o and backup files
clean:
	-rm -f *.o *~ *.o *~ *.so

include depend
