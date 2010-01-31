# progs/es1/xegl/Makefile

TOP = ../../..
include $(TOP)/configs/current


INCLUDE_DIRS = \
	-I$(TOP)/include \

HEADERS = $(TOP)/include/GLES/egl.h


ES1_LIB_DEPS = \
	$(TOP)/$(LIB_DIR)/libEGL.so \
	$(TOP)/$(LIB_DIR)/libGLESv1_CM.so


ES1_LIBS = \
	-L$(TOP)/$(LIB_DIR) -lEGL \
	-L$(TOP)/$(LIB_DIR) -lGLESv1_CM $(LIBDRM_LIB) -lX11

PROGRAMS = \
	bindtex \
	drawtex \
	es1_info \
	msaa \
	pbuffer \
	render_tex \
	torus \
	tri \
	two_win


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: $(PROGRAMS)



bindtex: bindtex.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) bindtex.o $(ES1_LIBS) -o $@


drawtex: drawtex.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) drawtex.o $(ES1_LIBS) -o $@


es1_info: es1_info.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) es1_info.o $(ES1_LIBS) -o $@


msaa: msaa.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) msaa.o $(ES1_LIBS) -o $@


pbuffer: pbuffer.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) pbuffer.o $(ES1_LIBS) -o $@


render_tex: render_tex.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) render_tex.o $(ES1_LIBS) -o $@


torus: torus.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) torus.o $(ES1_LIBS) -o $@


two_win: two_win.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) two_win.o $(ES1_LIBS) -o $@


tri: tri.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) tri.o $(ES1_LIBS) -o $@


clean:
	rm -f *.o *~
	rm -f $(PROGRAMS)

