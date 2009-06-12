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


TOP = ../../..

include $(TOP)/configs/current


##### MACROS #####

GLUT_MAJOR = 3
GLUT_MINOR = 7
GLUT_TINY = 1

SOURCES = \
	glut_8x13.c \
	glut_9x15.c \
	glut_bitmap.c \
	glut_bwidth.c \
	glut_cindex.c \
	glut_cmap.c \
	glut_cursor.c \
	glut_dials.c \
	glut_dstr.c \
	glut_event.c \
	glut_ext.c \
	glut_fcb.c \
	glut_fullscrn.c \
	glut_gamemode.c \
	glut_get.c \
	glut_glxext.c \
	glut_hel10.c \
	glut_hel12.c \
	glut_hel18.c \
	glut_init.c \
	glut_input.c \
	glut_joy.c \
	glut_key.c \
	glut_keyctrl.c \
	glut_keyup.c \
	glut_menu.c \
	glut_menu2.c \
	glut_mesa.c \
	glut_modifier.c \
	glut_mroman.c \
	glut_overlay.c \
	glut_ppm.c \
	glut_roman.c \
	glut_shapes.c \
	glut_space.c \
	glut_stroke.c \
	glut_swap.c \
	glut_swidth.c \
	glut_tablet.c \
	glut_teapot.c \
	glut_tr10.c \
	glut_tr24.c \
	glut_util.c \
	glut_vidresize.c \
	glut_warp.c \
	glut_win.c \
	glut_winmisc.c \
	layerutil.c


OBJECTS = $(SOURCES:.c=.o)


##### RULES #####

.c.o:
	$(CC) -c -I$(TOP)/include $(X11_INCLUDES) $(CFLAGS) $(GLUT_CFLAGS) $<



##### TARGETS #####

default: $(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME)


# Make the library
$(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME): depend $(OBJECTS)
	$(MKLIB) -o $(GLUT_LIB) -linker '$(CC)' -ldflags '$(LDFLAGS)' \
		-major $(GLUT_MAJOR) -minor $(GLUT_MINOR) -patch $(GLUT_TINY) \
		$(MKLIB_OPTIONS) -install $(TOP)/$(LIB_DIR) \
		-id $(INSTALL_LIB_DIR)/lib$(GLUT_LIB).$(GLUT_MAJOR).dylib \
		$(GLUT_LIB_DEPS) $(OBJECTS)


# glut pkgconfig file
pcedit = sed \
	-e 's,@INSTALL_DIR@,$(INSTALL_DIR),' \
	-e 's,@INSTALL_LIB_DIR@,$(INSTALL_LIB_DIR),' \
	-e 's,@INSTALL_INC_DIR@,$(INSTALL_INC_DIR),' \
	-e 's,@VERSION@,$(GLUT_MAJOR).$(GLUT_MINOR).$(GLUT_TINY),' \
	-e 's,@GLUT_PC_REQ_PRIV@,$(GLUT_PC_REQ_PRIV),' \
	-e 's,@GLUT_PC_LIB_PRIV@,$(GLUT_PC_LIB_PRIV),' \
	-e 's,@GLUT_PC_CFLAGS@,$(GLUT_PC_CFLAGS),' \
	-e 's,@GLUT_LIB@,$(GLUT_LIB),'
glut.pc: glut.pc.in
	$(pcedit) $< > $@

install: glut.pc
	$(INSTALL) -d $(DESTDIR)$(INSTALL_INC_DIR)/GL
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -d $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig
	$(INSTALL) -m 644 $(TOP)/include/GL/glut.h $(DESTDIR)$(INSTALL_INC_DIR)/GL
	$(MINSTALL) $(TOP)/$(LIB_DIR)/$(GLUT_LIB_GLOB) $(DESTDIR)$(INSTALL_LIB_DIR)
	$(INSTALL) -m 644 glut.pc $(DESTDIR)$(INSTALL_LIB_DIR)/pkgconfig


clean:
	-rm -f *.o *~
	-rm -f *.lo
	-rm -f *.la
	-rm -f *.pc
	-rm -rf .libs
	-rm -f depend depend.bak


depend: $(SOURCES)
	@ echo "running $(MKDEP)"
	@ rm -f depend
	@ touch depend
	@ $(MKDEP) $(MKDEP_OPTIONS) -I$(TOP)/include $(SOURCES) \
		$(X11_INCLUDES) > /dev/null 

include depend
