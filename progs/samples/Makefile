# progs/samples/Makefile

TOP = ../..
include $(TOP)/configs/current

INCDIR = $(TOP)/include

LIB_DEP = $(LIB_DIR)/$(GL_LIB_NAME) $(LIB_DIR)/$(GLU_LIB_NAME) $(LIB_DIR)/$(GLUT_LIB_NAME)

PROGS = accum bitmap1 bitmap2 blendeq blendxor copy cursor depth eval fog \
	font line logo nurb olympic overlay point prim quad select \
	shape sphere star stencil stretch texture tri wave


##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c: $(LIB_DEP)
	$(CC) -I$(INCDIR) $(CFLAGS) $< $(APP_LIB_DEPS) -o $@



##### TARGETS #####

default: $(PROGS)


clean:
	-rm -f $(PROGS)
	-rm -f *.o *~
