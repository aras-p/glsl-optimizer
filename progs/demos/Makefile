# progs/demos/Makefile

TOP = ../..
include $(TOP)/configs/current

INCDIR = $(TOP)/include

OSMESA_LIBS = -L$(LIB_DIR) -lglut -lOSMesa -lGLU -lGL $(APP_LIB_DEPS)

LIB_DEP = $(LIB_DIR)/$(GL_LIB) $(LIB_DIR)/$(GLU_LIB) $(LIB_DIR)/$(GLUT_LIB)

PROGS = \
	arbfplight \
	arbocclude \
	bounce \
	clearspd \
	cubemap \
	drawpix \
	fire \
	fplight \
	gamma \
	gears \
	geartrain \
	glinfo \
	gloss \
	gltestperf \
	glutfx \
	isosurf \
	ipers \
	lodbias \
	morph3d \
	multiarb \
	occlude \
	osdemo \
	paltex \
	pixeltex \
	pointblast \
	ray \
	readpix \
	reflect \
	renormal \
	shadowtex \
	spectex \
	spriteblast \
	stex3d \
	teapot \
	terrain \
	tessdemo \
	texcyl \
	texdown \
	texenv \
	texobj \
	trispd \
	tunnel \
	tunnel2 \
	winpos


##### RULES #####

.SUFFIXES:
.SUFFIXES: .c


# make executable from .c file:
.c: $(LIB_DEP)
	$(CC) -I$(INCDIR) $(CFLAGS) $< $(APP_LIB_DEPS) -o $@


##### TARGETS #####

default: readtex.c readtex.h $(PROGS)


clean:
	-rm -f *.o *~

realclean:
	-rm -f $(PROGS)
	-rm -f *.o *~
	-rm -f readtex.[ch]


readtex.c:
	cp $(TOP)/progs/util/readtex.c .

readtex.h:
	cp $(TOP)/progs/util/readtex.h .

# special case: need the -lOSMesa library:
osdemo: osdemo.c
	$(CC) -I$(INCDIR) $(CFLAGS) osdemo.c $(OSMESA_LIBS) -o $@
