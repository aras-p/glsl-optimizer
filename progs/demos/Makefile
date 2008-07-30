# progs/demos/Makefile

TOP = ../..
include $(TOP)/configs/current

INCDIR = $(TOP)/include

OSMESA_LIBS = -L$(TOP)/$(LIB_DIR) -lglut -lOSMesa -lGLU -lGL $(APP_LIB_DEPS)

OSMESA16_LIBS = -L$(TOP)/$(LIB_DIR) -lglut -lOSMesa16 -lGLU -lGL $(APP_LIB_DEPS)

OSMESA32_LIBS = -L$(TOP)/$(LIB_DIR) -lglut -lOSMesa32 -lGLU -lGL $(APP_LIB_DEPS)

LIB_DEP = $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLU_LIB_NAME) \
	$(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME)

LIBS = -L$(TOP)/$(LIB_DIR) -l$(GLUT_LIB) -l$(GLU_LIB) -l$(GL_LIB) \
	$(APP_LIB_DEPS)

PROGS = \
	arbfplight \
	arbfslight \
	arbocclude \
	bounce \
	clearspd \
	copypix \
	cubemap \
	drawpix \
	engine \
	fbo_firecube \
	fire \
	fogcoord \
	fplight \
	fslight \
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
	rain \
	ray \
	readpix \
	reflect \
	renormal \
	shadowtex \
	singlebuffer \
	streaming_rect \
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
.SUFFIXES: .cxx

# make executable from .c file:
.c: $(LIB_DEP) readtex.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) $< readtex.o $(LIBS) -o $@


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


extfuncs.h:  $(TOP)/progs/util/extfuncs.h
	cp $< .


reflect: reflect.o showbuffer.o readtex.o
	$(CC) $(CFLAGS) $(LDFLAGS) reflect.o showbuffer.o readtex.o $(LIBS) -o $@

reflect.o: reflect.c showbuffer.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) reflect.c


shadowtex: shadowtex.o showbuffer.o
	$(CC) $(CFLAGS) $(LDFLAGS) shadowtex.o showbuffer.o $(LIBS) -o $@

shadowtex.o: shadowtex.c showbuffer.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) shadowtex.c


gloss: gloss.o trackball.o readtex.o
	$(CC) $(CFLAGS) $(LDFLAGS) gloss.o trackball.o readtex.o $(LIBS) -o $@

gloss.o: gloss.c trackball.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) gloss.c


engine: engine.o trackball.o readtex.o
	$(CC) $(CFLAGS) $(LDFLAGS) engine.o trackball.o readtex.o $(LIBS) -o $@

engine.o: engine.c trackball.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) engine.c


fslight: fslight.o
	$(CC) $(CFLAGS) $(LDFLAGS) fslight.o $(LIBS) -o $@

fslight.o: fslight.c extfuncs.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) fslight.c

rain: particles.o rain.o readtex.o
	$(CXX) $(CXXFLAGS) $(LDFLAGS) $^ $(LIBS) -o $@

rain.o: rain.cxx readtex.h
	$(CXX) -c -I$(INCDIR) $(CXXFLAGS) $<

particles.o: particles.cxx
	$(CXX) -c -I$(INCDIR) $(CXXFLAGS) $<


viewdds: viewdds.c
	$(CC) -I$(INCDIR) $(CFLAGS) $< readtex.o $(LIBS) -L. -lgltc -o $@


clean:
	-rm -f $(PROGS)
	-rm -f *.o *~
	-rm -f readtex.[ch] showbuffer.[ch]
