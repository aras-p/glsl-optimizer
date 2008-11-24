# subset glut

TOP = ../../..
include $(TOP)/configs/current

MARK = $(TOP)/src/glut/glx

GLUT_MAJOR = 3
GLUT_MINOR = 7
GLUT_TINY = 1

INCLUDES = -I$(TOP)/include -I$(MARK)

CORE_SOURCES = \
	bitmap.c \
	callback.c \
	color.c \
	globals.c \
	init.c \
	menu.c \
	models.c \
	overlay.c \
	state.c \
	teapot.c \
	window.c


MARK_SOURCES = \
	$(MARK)/glut_8x13.c \
	$(MARK)/glut_9x15.c \
	$(MARK)/glut_hel10.c \
	$(MARK)/glut_hel12.c \
	$(MARK)/glut_hel18.c \
	$(MARK)/glut_tr10.c \
	$(MARK)/glut_tr24.c

SOURCES = $(CORE_SOURCES)  $(MARK_SOURCES)

OBJECTS =  $(SOURCES:.c=.o)


##### RULES #####

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

.S.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES)  $< -o $@


##### TARGETS #####

default: depend $(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME)


# Make the library
$(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME): depend $(OBJECTS)
	$(MKLIB) -o $(GLUT_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(GLUT_MAJOR) -minor $(GLUT_MINOR) -patch $(GLUT_TINY) \
		$(GLUT_LIB_DEPS) -install $(TOP)/$(LIB_DIR) \
		$(MKLIB_OPTIONS) $(OBJECTS)


# Run 'make -f Makefile.solo dep' to update the dependencies if you change
# what's included by any source file.
depend: $(SOURCES) 
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(SOURCES) > /dev/null 

# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`


# glut pkgconfig file
pcedit = sed \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),' \
	-e 's,@INSTALL_LIB_DIR@,$(INSTALL_LIB_DIR),' \
	-e 's,@INSTALL_INC_DIR@,$(INSTALL_INC_DIR),' \
	-e 's,@VERSION@,$(GLUT_MAJOR).$(GLUT_MINOR).$(GLUT_TINY),' \
	-e 's,@GLUT_PC_REQ_PRIV@,$(GLUT_PC_REQ_PRIV),' \
	-e 's,@GLUT_PC_LIB_PRIV@,$(GLUT_PC_LIB_PRIV),' \
	-e 's,@GLUT_PC_CFLAGS@,$(GLUT_PC_CFLAGS),'
glut.pc: glut.pc.in
	$(pcedit) $< > $@

install: glut.pc
	$(INSTALL) -d $(DESTDIR)$(INSTALL_INC_DIR)/GL
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig
	$(INSTALL) -m 644 $(TOP)/include/GL/glut.h $(DESTDIR)$(INSTALL_INC_DIR)/GL
	$(INSTALL) $(TOP)/$(LIB_DIR)/libglut* $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -m 644 glut.pc $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig


# Remove .o and backup files
clean: depend
	-rm -f depend depend.bak
	-rm -f *.o *~ *.o *~ *.so *.pc libglut.so.3.7

include depend
