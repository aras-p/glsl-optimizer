TARGET		= libdriclient.a
OBJECTS		= driclient.o XF86dri.o
DRMDIR		?= /usr

CFLAGS		+= -g -Wall -fPIC -I../include -I${DRMDIR}/include -I${DRMDIR}/include/drm

#############################################

.PHONY	= all clean

all: ${TARGET}

${TARGET}: ${OBJECTS}
	ar rcs $@ $^
	if ! test -d ../lib; then mkdir ../lib; fi
	cp ${TARGET} ../lib

clean:
	rm -rf ${OBJECTS} ${TARGET} ../lib/${TARGET}
