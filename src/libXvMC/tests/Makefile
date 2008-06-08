CFLAGS	+= -g -Wall -Werror
LDFLAGS	+=
LIBS	+= -lXvMCW -lXvMC -lXv

#############################################

.PHONY	= all clean

all: test_context test_surface test_blocks test_rendering

test_context: test_context.o testlib.o
	$(CC) ${LDFLAGS} -o $@ $^ ${LIBS}

test_surface: test_surface.o testlib.o
	$(CC) ${LDFLAGS} -o $@ $^ ${LIBS}

test_blocks: test_blocks.o testlib.o
	$(CC) ${LDFLAGS} -o $@ $^ ${LIBS}

test_rendering: test_rendering.o testlib.o
	$(CC) ${LDFLAGS} -o $@ $^ ${LIBS}

clean:
	rm -rf *.o test_context test_surface test_blocks test_rendering

