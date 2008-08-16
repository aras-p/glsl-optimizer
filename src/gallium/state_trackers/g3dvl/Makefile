TARGET		= libg3dvl.a
OBJECTS		= vl_display.o vl_screen.o vl_context.o vl_surface.o vl_data.o vl_shader_build.o vl_util.o vl_basic_csc.o \
		  vl_r16snorm_mc.o
GALLIUMDIR	= ../..

CFLAGS		+= -g -Wall -fPIC -Werror -I${GALLIUMDIR}/include -I${GALLIUMDIR}/auxiliary -I${GALLIUMDIR}/winsys/g3dvl

#############################################

.PHONY	= all clean

all: ${TARGET}

${TARGET}: ${OBJECTS}
	ar rcs $@ $^

clean:
	rm -rf ${OBJECTS} ${TARGET}
