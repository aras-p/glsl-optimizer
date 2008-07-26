# Makefile for GLUT
#
# NOTICE:  The OpenGL Utility Toolkit (GLUT) distribution contains source
# code published in a book titled "Programming OpenGL for the X Window
# System" (ISBN: 0-201-48359-9) published by Addison-Wesley.  The
# programs and associated files contained in the distribution were
# developed by Mark J. Kilgard and are Copyright 1994, 1995, 1996 by Mark
# J. Kilgard (unless otherwise noted).  The programs are not in the
# public domain, but they are freely distributable without licensing
# fees.  These programs are provided without guarantee or warrantee
# expressed or implied.
#
# GLUT source included with Mesa with permission from Mark Kilgard.

# src/glut/beos/Makefile

TOP = ../../..
include $(TOP)/configs/current

GLUT_MAJOR = 3
GLUT_MINOR = 7
GLUT_TINY = 1

CPP_SOURCES = \
	glutBlocker.cpp \
	glutInit.cpp \
	glutWindow.cpp \
	glutEvent.cpp \
	glutCallback.cpp \
	glutOverlay.cpp \
	glutGet.cpp \
	glutColor.cpp \
	glutCursor.cpp \
	glutMenu.cpp \
	glutDstr.cpp \
	beos_x11.cpp

C_SOURCES = \
	glut_8x13.c \
	glut_9x15.c \
	glut_bitmap.c \
	glut_bwidth.c \
	glut_hel10.c \
	glut_hel12.c \
	glut_hel18.c \
	glut_tr10.c \
	glut_tr24.c \
	glut_mroman.c \
	glut_roman.c \
	glut_stroke.c \
	glut_swidth.c \
	glut_shapes.c \
	glut_teapot.c \
	glut_vidresize.c \
	glut_util.c \
	glut_ext.c

OBJECTS = \
	$(CPP_SOURCES:.cpp=.o) \
	$(C_SOURCES:.c=.o)

INCLUDES = \
	-I. \
	-I- \
	-I$(TOP)/include

# Rules

.cpp.o:
	$(CC) -c $< $(INCLUDES) $(CFLAGS) $(GLUT_CFLAGS) -o $@

.c.o:
	$(CC) -c $< $(INCLUDES) $(CFLAGS) $(GLUT_CFLAGS) -o $@


# ---- TARGETS

default: $(TOP)/$(LIB_DIR) $(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME)

# Create the lib directory if missing
$(TOP)/$(LIB_DIR):
	mkdir $(TOP)/$(LIB_DIR)

# Make the library
$(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME): depend $(OBJECTS)
	@$(MKLIB) -o $(GLUT_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(GLUT_MAJOR) -minor $(GLUT_MINOR) -patch $(GLUT_TINY) \
		-install $(TOP)/$(LIB_DIR) $(MKLIB_OPTIONS) \
		$(GLUT_LIB_DEPS) $(OBJECTS)

clean:
	-rm -f *.o

depend: $(SOURCES)
	touch depend
	$(MKDEP) $(MKDEP_OPTIONS) $(INCLUDES) $(SOURCES) > /dev/null 

include depend
