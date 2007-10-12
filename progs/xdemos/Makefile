# progs/xdemos/Makefile

TOP = ../..
include $(TOP)/configs/current


INCDIR = $(TOP)/include

LIB_DEP = $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLU_LIB_NAME)

PROGS = \
	corender \
	glthreads \
	glxdemo \
	glxgears \
	glxgears_fbconfig \
	glxcontexts \
	glxheads \
	glxinfo \
	glxpixmap \
	glxpbdemo \
	glxsnoop \
	glxswapcontrol \
	manywin \
	offset \
	overlay \
	pbinfo \
	pbdemo \
	texture_from_pixmap \
	wincopy \
	xfont \
	xrotfontdemo \
	yuvrect_client

# omit this XMesa API demo:	xdemo


##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c: $(LIB_DEP)
	$(CC) -I$(INCDIR) $(X11_INCLUDES) $(CFLAGS) $(LDFLAGS) $< $(APP_LIB_DEPS) -o $@


##### TARGETS #####

default: $(PROGS)


clean:
	-rm -f $(PROGS)
	-rm -f *.o *~


# special cases
pbinfo: pbinfo.o pbutil.o
	$(CC) $(LDFLAGS) pbinfo.o pbutil.o $(APP_LIB_DEPS) -o $@

pbdemo: pbdemo.o pbutil.o
	$(CC) $(LDFLAGS) pbdemo.o pbutil.o $(APP_LIB_DEPS) -o $@

pbinfo.o: pbinfo.c pbutil.h
	$(CC) -c -I. -I$(INCDIR) $(X11_INCLUDES) $(CFLAGS) pbinfo.c

pbdemo.o: pbdemo.c pbutil.h
	$(CC) -c -I. -I$(INCDIR) $(X11_INCLUDES) $(CFLAGS) pbdemo.c

pbutil.o: pbutil.c pbutil.h
	$(CC) -c -I. -I$(INCDIR) $(X11_INCLUDES) $(CFLAGS) pbutil.c

glxgears_fbconfig: glxgears_fbconfig.o pbutil.o
	$(CC) $(LDFLAGS) glxgears_fbconfig.o pbutil.o $(APP_LIB_DEPS) -o $@

glxgears_fbconfig.o: glxgears_fbconfig.c pbutil.h
	$(CC) -I$(INCDIR) $(X11_INCLUDES) $(CFLAGS) -c -I. $(CFLAGS) glxgears_fbconfig.c

xrotfontdemo: xrotfontdemo.o xuserotfont.o
	$(CC) $(LDFLAGS) xrotfontdemo.o xuserotfont.o $(APP_LIB_DEPS) -o $@

xuserotfont.o: xuserotfont.c xuserotfont.h
	$(CC) -c -I. -I$(INCDIR) $(X11_INCLUDES) $(CFLAGS) xuserotfont.c

xrotfontdemo.o: xrotfontdemo.c xuserotfont.h
	$(CC) -c -I. -I$(INCDIR) $(X11_INCLUDES) $(CFLAGS) xrotfontdemo.c

corender: corender.o ipc.o
	$(CC) $(CFLAGS) corender.o ipc.o $(APP_LIB_DEPS) -o $@

corender.o: corender.c ipc.h
	$(CC) -c -I. -I$(INCDIR) $(X11_INCLUDES) $(CFLAGS) corender.c

ipc.o: ipc.c ipc.h
	$(CC) -c -I. -I$(INCDIR) $(X11_INCLUDES) $(CFLAGS) ipc.c