# progs/demos/Makefile

TOP = ../..
include $(TOP)/configs/current

INCDIR = $(TOP)/include

OSMESA_LIBS = -L$(LIB_DIR) -lglut -lOSMesa -lGLU -lGL $(APP_LIB_DEPS)

OSMESA16_LIBS = -L$(LIB_DIR) -lglut -lOSMesa16 -lGLU -lGL $(APP_LIB_DEPS)

OSMESA32_LIBS = -L$(LIB_DIR) -lglut -lOSMesa32 -lGLU -lGL $(APP_LIB_DEPS)

LIB_DEP = $(LIB_DIR)/$(GL_LIB_NAME) $(LIB_DIR)/$(GLU_LIB_NAME) $(LIB_DIR)/$(GLUT_LIB_NAME)

PROGS = \
	arbfplight \
	arbocclude \
	bounce \
	clearspd \
	cubemap \
	drawpix \
	fire \
	fogcoord \
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
.c: $(LIB_DEP) readtex.o
	$(CC) -I$(INCDIR) $(CFLAGS) $< readtex.o $(APP_LIB_DEPS) -o $@


##### TARGETS #####

default: readtex.o $(PROGS)


readtex.c: $(TOP)/progs/util/readtex.c
	cp $< .

readtex.h: $(TOP)/progs/util/readtex.h
	cp $< .

readtex.o: readtex.c readtex.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) readtex.c


showbuffer.c: $(TOP)/progs/util/showbuffer.c
	cp $< .

showbuffer.h: $(TOP)/progs/util/showbuffer.h
	cp $< .

showbuffer.o: showbuffer.c showbuffer.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) showbuffer.c


reflect: reflect.o showbuffer.o readtex.o
	$(CC) -I$(INCDIR) $(CFLAGS) reflect.o showbuffer.o readtex.o $(APP_LIB_DEPS) -o $@

reflect.o: reflect.c showbuffer.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) reflect.c


shadowtex: shadowtex.o showbuffer.o
	$(CC) -I$(INCDIR) $(CFLAGS) shadowtex.o showbuffer.o $(APP_LIB_DEPS) -o $@

shadowtex.o: shadowtex.c showbuffer.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) shadowtex.c

# special case: need the -lOSMesa library:
osdemo: osdemo.c
	$(CC) -I$(INCDIR) $(CFLAGS) osdemo.c $(OSMESA_LIBS) -o $@

# another special case: need the -lOSMesa16 library:
osdemo16: osdemo16.c
	$(CC) -I$(INCDIR) $(CFLAGS) osdemo16.c $(OSMESA16_LIBS) -o $@

# another special case: need the -lOSMesa32 library:
osdemo32: osdemo32.c
	$(CC) -I$(INCDIR) $(CFLAGS) osdemo32.c $(OSMESA32_LIBS) -o $@



clean:
	-rm -f $(PROGS)
	-rm -f *.o *~
	-rm -f readtex.[ch]
