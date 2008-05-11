# src/mesa/drivers/dri/swrast/Makefile

TOP = ../../../../..
include $(TOP)/configs/current

LIBNAME = swrast_dri.so

DRIVER_SOURCES = \
	swrast.c \
	swrast_span.c

C_SOURCES = \
	$(COMMON_SOURCES) \
	$(DRIVER_SOURCES)

########################################

MESA_MODULES = $(TOP)/src/mesa/libmesa.a

COMMON_SOURCES = \
	../../common/driverfuncs.c

ifeq ($(WINDOW_SYSTEM),dri)
COMMON_SOURCES += \
	../common/utils.c
endif

OBJECTS = $(C_SOURCES:.c=.o)

### Include directories
INCLUDES = \
	-I. \
	-I$(TOP)/include \
	-I$(TOP)/src/mesa \
	-I$(TOP)/src/mesa/main \
	-I$(TOP)/src/mesa/glapi

ifeq ($(WINDOW_SYSTEM),dri)
INCLUDES += \
	-I$(TOP)/src/mesa/drivers/dri/common \
	$(LIBDRM_CFLAGS)
endif

##### RULES #####

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $< -o $@


##### TARGETS #####

default: depend $(LIBNAME)


$(LIBNAME): $(OBJECTS) $(MESA_MODULES) Makefile
	$(TOP)/bin/mklib -ldflags '$(LDFLAGS)' -noprefix -o $@ \
		$(OBJECTS) $(MESA_MODULES) $(GLCORE_LIB_DEPS)


depend: $(C_SOURCES)
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(C_SOURCES) \
		> /dev/null


clean:
	-rm -f *.o *.so
	-rm -f depend depend.bak

install: $(LIBNAME)
	$(INSTALL) -d $(DESTDIR)$(DRI_DRIVER_INSTALL_DIR)
	$(INSTALL) -m 755 $(LIBNAME) $(DESTDIR)$(DRI_DRIVER_INSTALL_DIR)

include depend
