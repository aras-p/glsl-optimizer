# progs/samples/Makefile

TOP = ../..
include $(TOP)/configs/current

INCDIR = $(TOP)/include

LIB_DEP = $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLU_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME)

PROGS = accum bitmap1 bitmap2 blendeq blendxor copy cursor depth eval fog \
	font line logo nurb olympic overlay point prim quad select \
	shape sphere star stencil stretch texture tri wave


##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c: $(LIB_DEP)
	$(APP_CC) -I$(INCDIR) $(CFLAGS) $< $(APP_LIB_DEPS) -o $@


##### TARGETS #####

default: $(PROGS)


sphere: sphere.o readtex.o
	$(APP_CC) -I$(INCDIR) $(CFLAGS) sphere.o readtex.o $(APP_LIB_DEPS) -o $@

sphere.o: sphere.c readtex.h
	$(APP_CC) -c -I$(INCDIR) $(CFLAGS) sphere.c

readtex.c: $(TOP)/progs/util/readtex.c
	cp $< .

readtex.h: $(TOP)/progs/util/readtex.h
	cp $< . 

readtex.o: readtex.c readtex.h
	$(APP_CC) -c -I$(INCDIR) $(CFLAGS) $< -o $@


clean:
	-rm -f $(PROGS)
	-rm -f *.o *~
	-rm -f readtex.c readtex.h
