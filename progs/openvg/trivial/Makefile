# These programs aren't intended to be included with the normal distro.
# They're not too interesting but they're good for testing.

TOP = ../../../
include $(TOP)/configs/current

INCLUDES = -I. -I$(TOP)/include
LIBS=-L$(TOP)/$(LIB_DIR) -lm -lEGL -lOpenVG -lpthread
CFLAGS += $(INCLUDES)

HEADERS=eglcommon.h

PROGRAMS = \
        arc \
        cap \
        clear \
        coord \
        dash \
        ellipse \
        filter \
        gradorigin \
        lineto \
        lingrad \
        lookup \
        mask4 \
        mask \
        path3 \
        radialgrad \
        readpixels \
        roundedrect \
        star-nonzero \
        star-oddeven \
        stroke2 \
        stroke \
        vguarc


.c.o:
	$(CC) -c $(INCLUDE_DIRS) $(CFLAGS) $< -o $@



default: $(PROGRAMS)


arc: arc.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

cap: cap.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

clear: clear.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

coord: coord.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

dash: dash.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

ellipse: ellipse.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

filter: filter.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

gradorigin: gradorigin.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

image: image.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

lineto: lineto.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

lingrad: lingrad.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

lookup: lookup.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

mask: mask.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

mask4: mask4.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

path3: path3.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

pattern: pattern.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

radialgrad: radialgrad.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

readpixels: readpixels.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

roundedrect: roundedrect.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

star-nonzero: star-nonzero.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

star-oddeven: star-oddeven.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

stroke: stroke.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

stroke2: stroke2.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@

vguarc: vguarc.c eglcommon.o
	$(CC) $(CFLAGS) $^ $(LIBS)  $(APP_LIB_DEPS) -o $@



eglcommon.o: eglcommon.c $(HEADERS)
	$(CC) -c $(CFLAGS) eglcommon.c


clean:
	rm -f *.o *~
	rm -f *.so
	rm -f $(PROGRAMS)
