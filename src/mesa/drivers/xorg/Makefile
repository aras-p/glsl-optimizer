# src/mesa/drivers/xorg/Makefile

TOP = ../../../..
include $(TOP)/configs/current

LIBNAME = libGLcore.so

SYMLINKS =		\
	glxheader.h	\
	xmesaP.h	\
	xm_api.c	\
	xm_buffer.c	\
	xm_dd.c		\
	xm_image.c	\
	xm_image.h	\
	xm_line.c	\
	xm_span.c	\
	xm_tri.c

C_SOURCES =		\
	xm_api.c	\
	xm_buffer.c	\
	xm_dd.c		\
	xm_image.c	\
	xm_line.c	\
	xm_span.c	\
	xm_tri.c	\
	glcore.c

########################################

MESA_MODULES = $(TOP)/src/mesa/libmesa.a

C_SOURCES += ../common/driverfuncs.c

OBJECTS = $(C_SOURCES:.c=.o)

### Include directories
INCLUDES = \
	-I. \
	-I.. \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi \
	`pkg-config --cflags xorg-server`

# undef 'USE_XSHM' to make it explicit that 'XFree86Server' takes precedence
DRIVER_DEFINES = -UUSE_XSHM -DXFree86Server

##### RULES #####

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DRIVER_DEFINES) $< -o $@


##### TARGETS #####

default: depend symlinks $(LIBNAME)


$(LIBNAME): $(OBJECTS) $(MESA_MODULES) Makefile
	$(TOP)/bin/mklib -noprefix -o $@ \
		$(OBJECTS) $(MESA_MODULES) $(GLCORE_LIB_DEPS)


depend: $(C_SOURCES) $(SYMLINKS)
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(DRIVER_DEFINES) $(INCLUDES) $(C_SOURCES) \
		> /dev/null


clean:
	-rm -f *.o *.so $(SYMLINKS)
	-rm -f depend depend.bak

install: $(LIBNAME)
	$(INSTALL) -d $(DRI_DRIVER_INSTALL_DIR)
	$(INSTALL) -m 755 $(LIBNAME) $(DRI_DRIVER_INSTALL_DIR)

$(SYMLINKS):
	@[ -e $@ ] || ln -sf ../x11/$@ ./

symlinks: $(SYMLINKS)

include depend
