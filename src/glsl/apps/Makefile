# src/glsl/apps/Makefile

TOP = ../../..

include $(TOP)/configs/current

LIBS = \
	$(TOP)/src/glsl/pp/libglslpp.a \
	$(TOP)/src/glsl/cl/libglslcl.a

SOURCES = \
	compile.c \
	process.c \
	purify.c \
	tokenise.c \
	version.c

APPS = $(SOURCES:%.c=%)

INCLUDES = -I.


##### RULES #####

.SUFFIXES:
.SUFFIXES: .c

.c:
	$(APP_CC) $(INCLUDES) $(CFLAGS) $(LDFLAGS) $< $(LIBS) -o $@

.c.o:
	$(APP_CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@


##### TARGETS #####

default: $(APPS)

install:

clean:
	-rm -f $(APPS)
	-rm -f *.o
