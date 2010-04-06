# progs/egl/opengles1/Makefile

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

EGLUT_DIR = $(TOP)/progs/egl/eglut

EGLUT_DEMOS = \
	drawtex \
	gears \
	torus \
	tri

EGLUT_X11_DEMOS := $(addsuffix _x11,$(EGLUT_DEMOS))
EGLUT_SCREEN_DEMOS := $(addsuffix _screen,$(EGLUT_DEMOS))

PROGRAMS = \
	bindtex \
	es1_info \
	msaa \
	pbuffer \
	render_tex \
	texture_from_pixmap \
	two_win


.c.o:
	$(CC) -c $(INCLUDE_DIRS) -I$(EGLUT_DIR) $(CFLAGS) $< -o $@



default: $(PROGRAMS) $(EGLUT_X11_DEMOS) $(EGLUT_SCREEN_DEMOS)



bindtex: bindtex.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) bindtex.o $(ES1_LIBS) -o $@


es1_info: es1_info.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) es1_info.o $(ES1_LIBS) -o $@


msaa: msaa.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) msaa.o $(ES1_LIBS) -o $@


pbuffer: pbuffer.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) pbuffer.o $(ES1_LIBS) -o $@


render_tex: render_tex.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) render_tex.o $(ES1_LIBS) -o $@


texture_from_pixmap: texture_from_pixmap.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) texture_from_pixmap.o $(ES1_LIBS) -o $@

torus: torus.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) torus.o $(ES1_LIBS) -o $@


two_win: two_win.o $(ES1_LIB_DEPS)
	$(CC) $(CFLAGS) two_win.o $(ES1_LIBS) -o $@


$(EGLUT_DIR)/libeglut-x11.a $(EGLUT_DIR)/libeglut-screen.a:
	@$(MAKE) -C $(EGLUT_DIR)

# define the rules for EGLUT demos
define eglut-demo-rule
$(1)_x11 $(1)_screen: $(1)_%: $(1).o $(EGLUT_DIR)/libeglut-%.a $(ES1_LIB_DEPS)
endef
$(foreach demo, $(EGLUT_DEMOS), $(eval $(call eglut-demo-rule,$(demo))))

# build EGLUT demos
$(EGLUT_X11_DEMOS):
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(EGLUT_DIR) -leglut-$* $(ES1_LIBS) -lX11
$(EGLUT_SCREEN_DEMOS):                                                
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(EGLUT_DIR) -leglut-$* $(ES1_LIBS)


clean:
	-rm -f *.o *~
	-rm -f $(PROGRAMS) $(EGLUT_X11_DEMOS) $(EGLUT_SCREEN_DEMOS)
	@$(MAKE) -C $(EGLUT_DIR) clean
