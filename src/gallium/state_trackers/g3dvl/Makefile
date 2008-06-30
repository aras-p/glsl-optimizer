TARGET		= libg3dvl.a
OBJECTS		= vl_context.o vl_data.o vl_surface.o vl_shader_build.o
GALLIUMDIR	= ../..

CFLAGS		+= -g -Wall -fPIC -Werror -I${GALLIUMDIR}/include -I${GALLIUMDIR}/auxiliary

#############################################

.PHONY	= all clean

all: ${TARGET}

${TARGET}: ${OBJECTS}
	ar rcs $@ $^

clean:
	rm -rf ${OBJECTS} ${TARGET}

