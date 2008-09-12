# progs/redbook/Makefile

TOP = ../..
include $(TOP)/configs/current

INCDIR = $(TOP)/include

LIB_DEP = $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLU_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME)

PROGS = aaindex aapoly aargb accanti accpersp alpha alpha3D anti \
	bezcurve bezmesh checker clip colormat cube depthcue dof \
	double drawf feedback fog fogindex font hello image light \
	lines list material mipmap model movelight nurbs pickdepth \
	picksquare plane planet polyoff polys quadric robot sccolorlight \
	scene scenebamb sceneflat select smooth stencil stroke surface \
	teaambient teapots tess tesswind texbind texgen texprox texsub \
	texturesurf torus trim unproject varray wrap 



##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c: $(LIB_DEP)
	$(APP_CC) -I$(INCDIR) $(CFLAGS) $< $(APP_LIB_DEPS) -o $@



##### TARGETS ######

default: $(PROGS)


clean:
	-rm -f $(PROGS)
	-rm -f *.o *~

