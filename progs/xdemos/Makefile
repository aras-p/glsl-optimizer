# progs/xdemos/Makefile

TOP = ../../
include $(TOP)/configs/current


INCDIR = $(TOP)/include

LIBS = $(APP_LIB_DEPS)

LIB_DEP = $(LIB_DIR)/$(GL_LIB) $(LIB_DIR)/$(GLU_LIB)

PROGS = glthreads \
	glxdemo \
	glxgears \
	glxheads \
	glxinfo \
	glxpixmap \
	glxpbdemo \
	glxswapcontrol \
	manywin \
	offset \
	pbinfo \
	pbdemo \
	wincopy \
	xdemo \
	xfont \
	xrotfontdemo



##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c: $(LIB_DEP)
	$(CC) -I$(INCDIR) $(CFLAGS) $< $(LIBS) -o $@


##### TARGETS #####

default: $(PROGS)


clean:
	-rm -f $(PROGS)
	-rm -f *.o *~


# special cases
pbinfo: pbinfo.o pbutil.o
	$(CC) pbinfo.o pbutil.o $(LIBS) -o $@

pbdemo: pbdemo.o pbutil.o
	$(CC) pbdemo.o pbutil.o $(LIBS) -o $@

pbinfo.o: pbinfo.c pbutil.h
	$(CC) -c -I. -I$(INCDIR) $(CFLAGS) pbinfo.c

pbdemo.o: pbdemo.c pbutil.h
	$(CC) -c -I. -I$(INCDIR) $(CFLAGS) pbdemo.c

pbutil.o: pbutil.c pbutil.h
	$(CC) -c -I. -I$(INCDIR) $(CFLAGS) pbutil.c


xrotfontdemo: xrotfontdemo.o xuserotfont.o
	$(CC) xrotfontdemo.o xuserotfont.o $(LIBS) -o $@

xuserotfont.o: xuserotfont.c xuserotfont.h
	$(CC) -c -I. -I$(INCDIR) $(CFLAGS) xuserotfont.c

xrotfontdemo.o: xrotfontdemo.c xuserotfont.h
	$(CC) -c -I. -I$(INCDIR) $(CFLAGS) xrotfontdemo.c

