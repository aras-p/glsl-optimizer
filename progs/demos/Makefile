# progs/demos/Makefile

TOP = ../..
include $(TOP)/configs/current

INCDIR = $(TOP)/include

OSMESA_LIBS = -L$(TOP)/$(LIB_DIR) -lglut -lOSMesa -lGLU -lGL $(APP_LIB_DEPS)

OSMESA16_LIBS = -L$(TOP)/$(LIB_DIR) -lglut -lOSMesa16 -lGLU -lGL $(APP_LIB_DEPS)

OSMESA32_LIBS = -L$(TOP)/$(LIB_DIR) -lglut -lOSMesa32 -lGLU -lGL $(APP_LIB_DEPS)

LIB_DEP = $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLU_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME)

PROGS = \
	arbfplight \
	arbfslight \
	arbocclude \
	bounce \
	clearspd \
	cubemap \
	drawpix \
	engine \
	fire \
	fogcoord \
	fplight \
	gamma \
	gearbox \
	gears \
	geartrain \
	glinfo \
	gloss \
	glslnoise \
	gltestperf \
	glutfx \
	isosurf \
	ipers \
	lodbias \
	morph3d \
	multiarb \
	paltex \
	pointblast \
	ray \
	readpix \
	reflect \
	renormal \
	shadowtex \
	singlebuffer \
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
	vao_demo \
	winpos


##### RULES #####

.SUFFIXES:
.SUFFIXES: .c


# make executable from .c file:
.c: $(LIB_DEP) readtex.o
	$(CC) -I$(INCDIR) $(CFLAGS) $< readtex.o $(APP_LIB_DEPS) -o $@


##### TARGETS #####

default: $(PROGS)

$(PROGS): readtex.o

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


trackball.c: $(TOP)/progs/util/trackball.c
	cp $< .

trackball.h: $(TOP)/progs/util/trackball.h
	cp $< .

trackball.o: trackball.c trackball.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) trackball.c


reflect: reflect.o showbuffer.o readtex.o
	$(CC) -I$(INCDIR) $(CFLAGS) reflect.o showbuffer.o readtex.o $(APP_LIB_DEPS) -o $@

reflect.o: reflect.c showbuffer.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) reflect.c


shadowtex: shadowtex.o showbuffer.o
	$(CC) -I$(INCDIR) $(CFLAGS) shadowtex.o showbuffer.o $(APP_LIB_DEPS) -o $@

shadowtex.o: shadowtex.c showbuffer.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) shadowtex.c


gloss: gloss.o trackball.o readtex.o
	$(CC) -I$(INCDIR) $(CFLAGS) gloss.o trackball.o readtex.o $(APP_LIB_DEPS) -o $@

gloss.o: gloss.c trackball.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) gloss.c


engine: engine.o trackball.o readtex.o
	$(CC) -I$(INCDIR) $(CFLAGS) engine.o trackball.o readtex.o $(APP_LIB_DEPS) -o $@

engine.o: engine.c trackball.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) engine.c


clean:
	-rm -f $(PROGS)
	-rm -f *.o *~
	-rm -f readtex.[ch] showbuffer.[ch]
