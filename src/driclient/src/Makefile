TARGET		= libdriclient.a
OBJECTS		= driclient.o XF86dri.o
DRMDIR		?= /usr

CFLAGS		+= -g -Wall -fPIC -Werror -I../include -I${DRMDIR}/include -I${DRMDIR}/include/drm

#############################################

.PHONY	= all clean

all: ${TARGET} test

${TARGET}: ${OBJECTS}
	ar rcs $@ $^
	cp ${TARGET} ../lib

test: test.o
	$(CC) -L../lib -L${DRMDIR}/lib ${LDFLAGS} -o $@ $^ -ldriclient -lX11 -lXext -ldrm

clean:
	rm -rf ${OBJECTS} ${TARGET} test test.o

