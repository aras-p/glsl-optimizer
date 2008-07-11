# subset glut

TOP = ../../..
include $(TOP)/configs/current

MARK = $(TOP)/src/glut/glx

GLUT_MAJOR = 3
GLUT_MINOR = 7
GLUT_TINY = 1

INCLUDES = -I$(TOP)/include -I$(MARK) $(shell pkg-config --cflags directfb)

GLUT_LIB_DEPS += $(shell pkg-config --libs directfb)

CORE_SOURCES = \
	callback.c \
	color.c \
	cursor.c \
	ext.c \
	events.c \
	font.c \
	game.c \
	globals.c \
	init.c \
	menu.c \
	models.c \
	overlay.c \
	state.c \
	teapot.c \
	window.c \


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


# Remove .o and backup files
clean: depend
	-rm -f depend depend.bak
	-rm -f *.o *~ *.o *~ *.so libglut.so.3.7

include depend
