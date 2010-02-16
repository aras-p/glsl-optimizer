# progs/redbook/Makefile

TOP = ../..
include $(TOP)/configs/current

INCDIR = $(TOP)/include

LIB_DEP = $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLU_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME)

LIBS = -L$(TOP)/$(LIB_DIR) -l$(GLUT_LIB) -l$(GLEW_LIB) -l$(GLU_LIB) -l$(GL_LIB) $(APP_LIB_DEPS)

PROGS = aaindex \
	aapoly \
	aargb \
	accanti \
	accpersp \
	alpha \
	alpha3D \
	anti \
	bezcurve \
	bezmesh \
	checker \
	clip \
	colormat \
	combiner \
	convolution \
	cube \
	cubemap \
	depthcue \
	dof \
	double \
	drawf \
	feedback \
	fog \
	fogcoord \
	fogindex \
	font \
	hello \
	histogram \
	image \
	light \
	lines \
	list \
	material \
	minmax \
	mipmap \
	model \
	movelight \
	multitex \
	multisamp \
	mvarray \
	nurbs \
	pickdepth \
	picksquare \
	plane \
	planet \
	pointp \
	polyoff \
	polys \
	quadric \
	robot \
	sccolorlight \
	scene \
	scenebamb \
	sceneflat \
	select \
	shadowmap \
	smooth \
	stencil \
	stroke \
	surface \
	surfpoints \
	teaambient \
	teapots \
	tess \
	tesswind \
	texbind \
	texgen \
	texprox \
	texsub \
	texturesurf \
	texture3d \
	torus \
	trim \
	unproject \
	varray \
	wrap 



##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c: $(LIB_DEP)
	$(APP_CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) $< $(LIBS) -o $@



##### TARGETS ######

default: $(PROGS)


clean:
	-rm -f $(PROGS)
	-rm -f *.o *~

