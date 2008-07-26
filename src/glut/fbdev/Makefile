# subset glut

TOP = ../../..
include $(TOP)/configs/current

GLX_SHARED = $(TOP)/src/glut/glx
MINI_SHARED = $(TOP)/src/glut/mini

GLUT_MAJOR = 3
GLUT_MINOR = 7
GLUT_TINY = 1

INCLUDES = -I$(TOP)/include -I$(GLX_SHARED)

CORE_SOURCES = \
	fbdev.c \
	colormap.c \
	cursor.c \
	menu.c \
	overlay.c \
	ext.c \
	state.c \
	input.c \
	callback.c \
	gamemode.c \
	vidresize.c \
	bitmap.c \
	stroke.c

GLX_SHARED_SOURCES = \
	$(GLX_SHARED)/glut_8x13.c \
	$(GLX_SHARED)/glut_9x15.c \
	$(GLX_SHARED)/glut_hel10.c \
	$(GLX_SHARED)/glut_hel12.c \
	$(GLX_SHARED)/glut_hel18.c \
	$(GLX_SHARED)/glut_tr10.c \
	$(GLX_SHARED)/glut_tr24.c \
	$(GLX_SHARED)/glut_mroman.c \
	$(GLX_SHARED)/glut_roman.c \

MINI_SHARED_SOURCES = \
	$(MINI_SHARED)/models.c \
	$(MINI_SHARED)/teapot.c

SOURCES = $(CORE_SOURCES)  $(GLX_SHARED_SOURCES) $(MINI_SHARED_SOURCES)

OBJECTS =  $(SOURCES:.c=.o)


##### RULES #####

.c.o:
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES)  $< -o $@

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

install:
	$(INSTALL) -d $(DESTDIR)$(INSTALL_INC_DIR)/GL
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -m 644 $(TOP)/include/GL/glut.h $(DESTDIR)$(INSTALL_INC_DIR)/GL
	$(INSTALL) $(TOP)/$(LIB_DIR)/libglut* $(DESTDIR)$(INSTALL_LIB_DIR)

# Run 'make -f Makefile.solo dep' to update the dependencies if you change
# what's included by any source file.
depend: $(SOURCES) 
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(SOURCES) > /dev/null 

# Emacs tags
tags:
	etags `find . -name \*.[ch]` `find ../include`


# Remove .o and backup files
clean: depend
	-rm -f depend depend.bak
	-rm -f *.o *~ *.o *~ *.so libglut.so.3.7

include depend
