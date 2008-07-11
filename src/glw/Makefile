# src/glw/Makefile

TOP = ../..
include $(TOP)/configs/current

MAJOR = 1
MINOR = 0
TINY = 0

INCDIRS = -I$(TOP)/include -I/usr/include/Motif1.2 $(X11_INCLUDES)


OBJECTS = $(GLW_SOURCES:.c=.o)



##### RULES #####

.c.o:
	$(CC) -c $(INCDIRS) $(CFLAGS) $<



##### TARGETS #####

default: $(TOP)/$(LIB_DIR)/$(GLW_LIB_NAME)

# GLU pkg-config file
pcedit = sed \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),' \
	-e 's,@INSTALL_LIB_DIR@,$(INSTALL_LIB_DIR),' \
	-e 's,@INSTALL_INC_DIR@,$(INSTALL_INC_DIR),' \
	-e 's,@VERSION@,$(MAJOR).$(MINOR).$(TINY),'
glw.pc: glw.pc.in
	$(pcedit) $< > $@

install: glw.pc
	$(INSTALL) -d $(DESTDIR)$(INSTALL_INC_DIR)/GL
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig
	$(INSTALL) -m 644 *.h $(DESTDIR)$(INSTALL_INC_DIR)/GL
	$(INSTALL) $(TOP)/$(LIB_DIR)/libGLw.* $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -m 644 glw.pc $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig

clean:
	-rm -f depend depend.bak
	-rm -f *.o *.pc *~


# Make the library
$(TOP)/$(LIB_DIR)/$(GLW_LIB_NAME): $(OBJECTS)
	$(MKLIB) -o $(GLW_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(MAJOR) -minor $(MINOR) -patch $(TINY) \
		$(MKLIB_OPTIONS) -install $(TOP)/$(LIB_DIR) \
		$(GLW_LIB_DEPS) $(OBJECTS)


#
# Run 'make depend' to update the dependencies if you change what's included
# by any source file.
# 
depend: $(GLW_SOURCES)
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) -I$(TOP)/include $(GLW_SOURCES) \
		$(X11_INCLUDES) > /dev/null


include depend
