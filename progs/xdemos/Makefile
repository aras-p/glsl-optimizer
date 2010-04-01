# progs/xdemos/Makefile

TOP = ../..
include $(TOP)/configs/current


INCDIR = $(TOP)/include

LIB_DEP = $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME)

# Add X11 and pthread libs to satisfy GNU gold.
APP_LIB_DEPS += -lX11 -lpthread

LIBS = -L$(TOP)/$(LIB_DIR) -l$(GL_LIB) -L$(libdir) $(APP_LIB_DEPS)

PROGS = \
	corender \
	glsync \
	glthreads \
	glxdemo \
	glxgears \
	glxgears_fbconfig \
	glxgears_pixmap \
	glxcontexts \
	glxheads \
	glxinfo \
	glxpixmap \
	glxpbdemo \
	glxsnoop \
	glxswapcontrol \
	manywin \
	msctest \
	multictx \
	offset \
	omlsync \
	overlay \
	pbinfo \
	pbdemo \
	sharedtex \
        sharedtex_mt \
	texture_from_pixmap \
	wincopy \
	xfont \
	xrotfontdemo

# Don't build these by default because of extra library dependencies
EXTRA_PROGS = \
	shape \
	yuvrect_client \
	xdemo



##### RULES #####

.o: $(LIB_DEP)
	$(APP_CC) $(CFLAGS) $(LDFLAGS) $< $(LIBS) -o $@

.c.o:
	$(APP_CC) -I$(INCDIR) $(X11_INCLUDES) $(CFLAGS) $< -c -o $@


##### TARGETS #####

default: $(PROGS)

$(PROGS): $(PROGS:%=%.o)

extra: $(EXTRA_PROGS)


clean:
	-rm -f $(PROGS) $(EXTRA_PROGS)
	-rm -f *.o *~


# special cases
pbutil.o: pbutil.h
pbinfo.o: pbutil.h
pbinfo: pbinfo.o pbutil.o
	$(APP_CC) $(CFLAGS) $(LDFLAGS) pbinfo.o pbutil.o $(LIBS) -o $@

pbdemo.o: pbutil.h
pbdemo: pbdemo.o pbutil.o
	$(APP_CC) $(CFLAGS) $(LDFLAGS) pbdemo.o pbutil.o $(LIBS) -o $@

glxgears_fbconfig.o: pbutil.h
glxgears_fbconfig: glxgears_fbconfig.o pbutil.o
	$(APP_CC) $(CFLAGS) $(LDFLAGS) glxgears_fbconfig.o pbutil.o $(LIBS) -o $@

xuserotfont.o: xuserotfont.h
xrotfontdemo.o: xuserotfont.h
xrotfontdemo: xrotfontdemo.o xuserotfont.o
	$(APP_CC) $(CFLAGS) $(LDFLAGS) xrotfontdemo.o xuserotfont.o $(LIBS) -o $@

ipc.o: ipc.h
corender.o: ipc.h
corender: corender.o ipc.o
	$(APP_CC) $(CFLAGS) $(LDFLAGS) corender.o ipc.o $(LIBS) -o $@

yuvrect_client: yuvrect_client.o
	$(APP_CC) $(CFLAGS) $< $(LDFLAGS) $(LIBS) -l$(GLU_LIB) -o $@

