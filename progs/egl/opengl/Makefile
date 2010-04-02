# progs/egl/Makefile

TOP = ../../..
include $(TOP)/configs/current


INCLUDE_DIRS = -I$(TOP)/include

HEADERS = $(TOP)/include/GLES/egl.h
LIB_DEP = $(TOP)/$(LIB_DIR)/libEGL.so

LIBS = -L$(TOP)/$(LIB_DIR) -lEGL -lGL -lm

EGLUT_DIR = $(TOP)/progs/egl/eglut

EGLUT_DEMOS = \
	eglgears \
	egltri

EGLUT_X11_DEMOS := $(addsuffix _x11,$(EGLUT_DEMOS))
EGLUT_SCREEN_DEMOS := $(addsuffix _screen,$(EGLUT_DEMOS))

PROGRAMS = \
	demo1 \
	demo2 \
	demo3 \
	eglinfo \
	eglscreen \
	peglgears \
	xeglgears \
	xeglthreads


.c.o:
	$(CC) -c $(INCLUDE_DIRS) -I$(EGLUT_DIR) $(CFLAGS) $< -o $@



default: $(PROGRAMS) $(EGLUT_X11_DEMOS) $(EGLUT_SCREEN_DEMOS)

demo1: demo1.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) $(LIBDRM_LIB)

demo2: demo2.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) $(LIBDRM_LIB)

demo3: demo3.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) $(LIBDRM_LIB)

eglinfo: eglinfo.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS)

eglscreen: eglscreen.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) $(LIBDRM_LIB)

peglgears: peglgears.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) $(LIBDRM_LIB) -lm

xeglgears: xeglgears.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) -lm $(X11_LIBS)

xeglthreads: xeglthreads.o $(HEADERS) $(LIB_DEP)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< $(LIBS) -lpthread -lm $(X11_LIBS)

$(EGLUT_DIR)/libeglut-x11.a $(EGLUT_DIR)/libeglut-screen.a:
	@$(MAKE) -C $(EGLUT_DIR)

# define the rules for EGLUT demos
define eglut-demo-rule
$(1)_x11 $(1)_screen: $(1)_%: $(1).o $(EGLUT_DIR)/libeglut-%.a $(LIB_DEP)
endef
$(foreach demo, $(EGLUT_DEMOS), $(eval $(call eglut-demo-rule,$(demo))))

# build EGLUT demos
$(EGLUT_X11_DEMOS):
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(EGLUT_DIR) -leglut-$* $(LIBS) -lX11
$(EGLUT_SCREEN_DEMOS):                                                
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(EGLUT_DIR) -leglut-$* $(LIBS)

clean:
	-rm -f *.o *~
	-rm -f $(PROGRAMS) $(EGLUT_X11_DEMOS) $(EGLUT_SCREEN_DEMOS)
	@$(MAKE) -C $(EGLUT_DIR) clean
