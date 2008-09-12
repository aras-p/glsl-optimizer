# progs/xdemos/Makefile

TOP = ../..
include $(TOP)/configs/current


INCDIR = $(TOP)/include

LIB_DEP = $(TOP)/$(LIB_DIR)/$(GL_LIB_NAME) $(TOP)/$(LIB_DIR)/$(GLU_LIB_NAME)

PROGS = glthreads \
	glxdemo \
	glxgears \
	glxgears_fbconfig \
	glxcontexts \
	glxheads \
	glxinfo \
	glxpixmap \
	glxpbdemo \
	glxswapcontrol \
	manywin \
	offset \
	overlay \
	pbinfo \
	pbdemo \
	sharedtex \
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
	$(APP_CC) -I$(INCDIR) $(CFLAGS) $< $(APP_LIB_DEPS) -o $@


##### TARGETS #####

default: $(PROGS)


clean:
	-rm -f $(PROGS)
	-rm -f *.o *~


# special cases
pbinfo: pbinfo.o pbutil.o
	$(APP_CC) -I$(INCDIR) $(CFLAGS) pbinfo.o pbutil.o $(APP_LIB_DEPS) -o $@

pbdemo: pbdemo.o pbutil.o
	$(APP_CC) -I$(INCDIR) $(CFLAGS) pbdemo.o pbutil.o $(APP_LIB_DEPS) -o $@

pbinfo.o: pbinfo.c pbutil.h
	$(APP_CC) -c -I. -I$(INCDIR) $(CFLAGS) pbinfo.c

pbdemo.o: pbdemo.c pbutil.h
	$(APP_CC) -c -I. -I$(INCDIR) $(CFLAGS) pbdemo.c

pbutil.o: pbutil.c pbutil.h
	$(APP_CC) -c -I. -I$(INCDIR) $(CFLAGS) pbutil.c

glxgears_fbconfig: glxgears_fbconfig.o pbutil.o
	$(APP_CC) -I$(INCDIR) $(CFLAGS) glxgears_fbconfig.o pbutil.o $(APP_LIB_DEPS) -o $@

glxgears_fbconfig.o: glxgears_fbconfig.c pbutil.h
	$(APP_CC) -I$(INCDIR) $(CFLAGS) -c -I. $(CFLAGS) glxgears_fbconfig.c

xrotfontdemo: xrotfontdemo.o xuserotfont.o
	$(APP_CC) -I$(INCDIR) $(CFLAGS) xrotfontdemo.o xuserotfont.o $(APP_LIB_DEPS) -o $@

xuserotfont.o: xuserotfont.c xuserotfont.h
	$(APP_CC) -c -I. -I$(INCDIR) $(CFLAGS) xuserotfont.c

xrotfontdemo.o: xrotfontdemo.c xuserotfont.h
	$(APP_CC) -c -I. -I$(INCDIR) $(CFLAGS) xrotfontdemo.c

