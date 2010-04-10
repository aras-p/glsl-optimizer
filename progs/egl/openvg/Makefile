# progs/egl/openvg/Makefile

TOP = ../../..
include $(TOP)/configs/current

VG_LIBS=-lm -lEGL -lOpenVG -L$(TOP)/lib -L$(TOP)/lib/gallium
INCLUDE_DIRS = -I$(TOP)/include

EGLUT_DIR = $(TOP)/progs/egl/eglut

EGLUT_DEMOS = \
	sp

EGLUT_X11_DEMOS := $(addsuffix _x11,$(EGLUT_DEMOS))
EGLUT_SCREEN_DEMOS := $(addsuffix _screen,$(EGLUT_DEMOS))

PROGRAMS = \
	lion_x11 \
	lion_screen

.c.o:
	$(CC) -c $(INCLUDE_DIRS) -I$(EGLUT_DIR) $(CFLAGS) $< -o $@



default: $(PROGRAMS) $(EGLUT_X11_DEMOS) $(EGLUT_SCREEN_DEMOS)

lion_x11: lion.o lion-render.o $(EGLUT_DIR)/libeglut-x11.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ lion.o lion-render.o \
		-L$(EGLUT_DIR) -leglut-x11 $(VG_LIBS) -lX11

lion_screen: lion.o lion-render.o $(EGLUT_DIR)/libeglut-screen.a
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ lion.o lion-render.o \
		-L$(EGLUT_DIR) -leglut-screen $(VG_LIBS) 


$(EGLUT_DIR)/libeglut-x11.a $(EGLUT_DIR)/libeglut-screen.a:
	@$(MAKE) -C $(EGLUT_DIR)

# define the rules for EGLUT demos
define eglut-demo-rule
$(1)_x11 $(1)_screen: $(1)_%: $(1).o $(EGLUT_DIR)/libeglut-%.a
endef
$(foreach demo, $(EGLUT_DEMOS), $(eval $(call eglut-demo-rule,$(demo))))

# build EGLUT demos
$(EGLUT_X11_DEMOS):
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(EGLUT_DIR) -leglut-$* $(VG_LIBS) -lX11
$(EGLUT_SCREEN_DEMOS):                                                
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $< -L$(EGLUT_DIR) -leglut-$* $(VG_LIBS)

clean:
	rm -f *.o *~
	rm -f $(EGLUT_X11_DEMOS) $(EGLUT_SCREEN_DEMOS)
	@$(MAKE) -C $(EGLUT_DIR) clean
