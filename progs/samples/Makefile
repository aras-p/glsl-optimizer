# progs/samples/Makefile

TOP = ../..
include $(TOP)/configs/current

INCDIR = $(TOP)/include

LIB_DEP = $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLU_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLUT_LIB_NAME)

LIBS = -L$(TOP)/$(LIB_DIR) -l$(GLUT_LIB) -l$(GLU_LIB) -l$(GL_LIB) $(APP_LIB_DEPS)

PROGS = accum bitmap1 bitmap2 blendeq blendxor copy cursor depth eval fog \
	font line logo nurb olympic overlay point prim quad select \
	shape sphere star stencil stretch texture tri wave


##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c: $(LIB_DEP)
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) $< $(LIBS) -o $@


##### TARGETS #####

default: $(PROGS)


sphere: sphere.o readtex.o
	$(CC) -I$(INCDIR) $(CFLAGS) $(LDFLAGS) sphere.o readtex.o $(LIBS) -o $@

sphere.o: sphere.c readtex.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) sphere.c

readtex.c: $(TOP)/progs/util/readtex.c
	cp $< .

readtex.h: $(TOP)/progs/util/readtex.h
	cp $< . 

readtex.o: readtex.c readtex.h
	$(CC) -c -I$(INCDIR) $(CFLAGS) $< -o $@


clean:
	-rm -f $(PROGS)
	-rm -f *.o *~
	-rm -f readtex.c readtex.h
