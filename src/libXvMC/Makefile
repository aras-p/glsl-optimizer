TARGET		= libXvMCg3dvl.so
SONAME		= libXvMCg3dvl.so.1
GALLIUMDIR	= ../gallium
OBJECTS		= block.o surface.o context.o subpicture.o attributes.o ${GALLIUMDIR}/winsys/g3dvl/xsp_winsys.o

CFLAGS	+= -g -fPIC -Wall -Werror			\
	   -I${GALLIUMDIR}/state_trackers/g3dvl		\
	   -I${GALLIUMDIR}/winsys/g3dvl			\
	   -I${GALLIUMDIR}/include			\
	   -I${GALLIUMDIR}/auxiliary			\
	   -I${GALLIUMDIR}/drivers
LDFLAGS	+= -L${GALLIUMDIR}/state_trackers/g3dvl		\
	   -L${GALLIUMDIR}/drivers/softpipe		\
	   -L${GALLIUMDIR}/auxiliary/tgsi		\
	   -L${GALLIUMDIR}/auxiliary/draw		\
	   -L${GALLIUMDIR}/auxiliary/translate		\
	   -L${GALLIUMDIR}/auxiliary/cso_cache		\
	   -L${GALLIUMDIR}/auxiliary/util		\
	   -L${GALLIUMDIR}/auxiliary/rtasm
LIBS	+= -lg3dvl -lsoftpipe -ldraw -ltgsi -ltranslate -lrtasm -lcso_cache -lutil -lm

#############################################

.PHONY	= all clean

all: ${TARGET}

${TARGET}: ${OBJECTS}
	$(CC) ${LDFLAGS} -shared -Wl,-soname,${SONAME} -o $@ $^ ${LIBS}

clean:
	rm -rf ${OBJECTS} ${TARGET}

